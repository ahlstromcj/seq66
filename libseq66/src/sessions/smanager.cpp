/*
 *  This file is part of seq66.
 *
 *  seq66 is free software; you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option) any later
 *  version.
 *
 *  seq66 is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with seq66; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          smanager.cpp
 *
 *  This module declares/defines a module for managing a generic seq66
 *  session.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2020-03-22
 * \updates       2023-12-13
 * \license       GNU GPLv2 or above
 *
 *  Note that this module is part of the libseq66 library, not the libsessions
 *  library.  That is because it provides functionality that is useful even if
 *  session support is not enabled.
 *
 *  The process:
 *
 *      -# Call main_settings(argc, argv).  It sets defaults, does some parsing
 *         of command-line options and files.  It saves the MIDI file-name, if
 *         provided.
 *      -# Call create_performer(). This does not launch the performer. Save
 *         the unique-pointer.
 *      -# Call open_playlist().  It will open it, if specified and possible.
 e      -# Call open_note_mapper().  It will open it, if specified and possible.
 *      -# If the MIDI file-name is set, open it via a call to open_midi_file().
 *      -# If a user-interface is needed, create a unique-pointer to it, then
 *         show it.  This will remove any previous pointer.  The function is
 *         virtual, create_window().
 */

#include <cstring>                      /* std::strlen()                    */

#include "seq66_features.hpp"           /* set_app_name()                   */
#include "cfg/cmdlineopts.hpp"          /* static command-line functions    */
#include "cfg/midicontrolfile.hpp"      /* seq66::midicontrolfile functions */
#include "cfg/notemapfile.hpp"          /* seq66::notemapfile functions     */
#include "cfg/playlistfile.hpp"         /* seq66::playlistfile functions    */
#include "cfg/sessionfile.hpp"          /* seq66::sessionfile               */
#include "cfg/settings.hpp"             /* seq66::usr() and seq66::rc()     */
#include "midi/midifile.hpp"            /* seq66::write_midi_file()         */
#include "play/performer.hpp"           /* seq66::performer                 */
#include "play/playlist.hpp"            /* seq66::playlist class            */
#include "sessions/smanager.hpp"        /* seq66::smanager()                */
#include "os/daemonize.hpp"             /* seq66::reroute_stdio(), etc.     */
#include "util/basic_macros.hpp"        /* seq66::msgprintf()               */
#include "util/filefunctions.hpp"       /* seq66::file_readable() etc.      */

#if defined SEQ66_PORTMIDI_SUPPORT
#include "portmidi.h"                   /* Pm_error_present()               */
#endif

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Port error message.
 */

static std::string s_port_error_msg
{
    "Check MIDI Clock & MIDI Input tabs for unavailable/missing ports. "
    "Make sure the MIDI tune is not using such ports. "
    "Click 'Remap and restart' or change the global output ports "
    "for the tune."
};

static std::string s_port_update_msg
{
    "There are more real ports than mapped ports. "
    "Click 'Remap and restart' to recreate the maps or edit "
    "them in the 'rc' file."
};

/**
 *  Does the usual construction.  It also calls set_defaults() from the
 *  settings.cpp module in order to guarantee that we have rc() and usr()
 *  available.  See that function for more information.
 *
 *  create_performer() has to wait until after the calls to
 *  main_settings(), create_session(), create_project(),
 */

smanager::smanager (const std::string & caps) :
    m_perf_pointer          (),                 /* perf() accessor */
    m_capabilities          (caps),
    m_session_manager_name  ("None"),
    m_session_manager_path  ("None"),
    m_session_display_name  ("None"),
    m_session_client_id     ("None"),
    m_midi_filename         (),
    m_is_help               (false),
    m_last_dirty_status     (false),
    m_rerouted              (false),
    m_extant_errmsg         (),
    m_extant_msg_active     (false)
{
    set_configuration_defaults();
}

/**
 *  We found that on a Debian developer laptop, this destructor took a couple
 *  of seconds to call get_deleter().  Works fine on our Ubuntu developer
 *  laptop.  Weird.  Actually might have been a side-effect of installing a
 *  KxStudio PPA.
 */

smanager::~smanager ()
{
    if (! is_help())
        session_message("Exiting session manager");
}

/**
 *  This function also checks to make sure the log file does not get too large
 *  (about a megabyte).
 */

void
smanager::reroute_to_log (const std::string & filepath) const
{
    static const size_t s_limit = 1048576;
    size_t sz = file_size(filepath);
    if (sz > s_limit)
    {
        (void) file_delete(filepath);
        session_message("Log file deleted", filepath);
    }
    session_message("Rerouting console messages", filepath);
    (void) reroute_stdio(filepath);
    m_rerouted = true;
}

/**
 *  A static function to be called in main() (that is, early in the life
 *  of the application), so that this information can be changed later from
 *  the command-line. Helps to regularize the handling of the GUI versus
 *  CLI versions of the application.
 *
 *  Note that ./configure sets the client name to seq66. But it is probably
 *  better to distinguish qseq66 from seq66cli.
 */

void
smanager::app_info (const std::string arg0, bool is_cli)
{
    set_app_name(SEQ66_APP_NAME);               /* set at ./configure time  */
    if (is_cli)
    {
        /*
         * See the CLI main() routine instead.
         *
         * seq66::usr().app_is_headless(true);  // conflated with cli
         */

        seq66::set_app_path(arg0);              /* log for future usage     */
        set_app_cli(true);                      /* the default is false     */
        set_app_type(SEQ66_APP_TYPE);           /* e.g. "qt5" vs "cli"      */
        set_client_name("seq66cli");            /* change from configure    */
        rc().set_config_files("seq66cli");      /* set 'rc', 'usr' names    */
    }
}

/**
 *  The first thing is to set the various settings defaults, and then read
 *  the 'usr' and 'rc' configuration files, in that order.  The last thing
 *  is to override any other settings via the command-line parameters.
 *
 * NSM:
 *
 *      This function can detect the parent process ("nsmd") under Linux.
 *      Under Windows, we don't really support NSM, and so the
 *      seq66::get_parent_process_name() function in the daemonize.cpp module
 *      returns a name of "None".
 *
 * \param argc
 *      The number of command-line parameters, including the name of the
 *      application as parameter 0.
 *
 * \param argv
 *      The array of pointers to the command-line parameters.
 *
 * \return
 *      Returns true if this code worked properly and was not a request for
 *      help/version information.
 */

bool
smanager::main_settings (int argc, char * argv [])
{
    static const std::string s_nsm_name{"nsmd"};
    bool result = true;                         /* false --> EXIT_FAILURE   */
    std::string parentname = get_parent_process_name();
    bool in_nsm = contains(parentname, s_nsm_name); /* this is tentative!   */

    /*
     *  Call app_info() above in main() instead of this.
     *
     *  if (seq_app_cli())
     *  {
     *      set_app_name("seq66cli");
     *      set_app_type("cli");
     *      set_client_name("seq66cli");
     *      rc().set_config_files("seq66cli");
     *  }
     *  else
     *      set_app_name(SEQ66_APP_NAME);
     *
     *  set_arg_0(argv[0]);
     */

    /**
     * Set up objects specific to the GUI for the performer.  Parse command-line
     * options to see if they affect what gets read from the 'rc' or 'usr'
     * configuration files. They will be parsed again later so that they can
     * still override whatever other settings were made via the configuration
     * files.  However, we currently have a issue where the mastermidibus
     * created by the performer object gets the default PPQN value, because the
     * "user" configuration file has not been read at that point.  See the
     * performer::launch() function.
     */

    if (in_nsm)
    {
        session_message("Parent process", parentname);
    }
    else
    {
        bool ishelp = cmdlineopts::help_check(argc, argv);
        if (ishelp)
        {
            /*
             * ca 2023-12-13 Why do this?
             */

            (void) cmdlineopts::parse_command_line_options(argc, argv);
            is_help(true);
            result = false;
        }
        else
        {
            int rcode = cmdlineopts::parse_command_line_options(argc, argv);
            result = rcode != (-1);
            if (result)
            {
#if ! defined SEQ66_PLATFORM_WINDOWS
                if (usr().want_nsm_session())
                {
                    in_nsm = true;
                    session_manager_name("Simulated NSM");
                    session_manager_path(rc().home_config_directory());
                }
#endif
            }
            else
                is_help(true);          /* a hack to avoid create_window()  */
        }
        if (result)
        {
            int optionindex = (-1);
            bool sessionmodified = false;

            /*
             * Check for a session, either defined by the environment variable
             * "SEQ66_SESSION_TAG" or by the "--session-tag tag" option. The
             * latter can override the first.
             */

            if (! rc().alt_session())
            {
                std::string sesstag = cmdlineopts::env_session_tag();
                if (! sesstag.empty())
                    rc().session_tag(sesstag);
            }
            if (rc().alt_session())
            {
                /*
                 * The name 'sessions.rc' is a bit more accurate.
                 */

                std::string sessionfilename =
                    rc().make_config_filespec("sessions.rc");

                if (file_readable(sessionfilename))
                {
                    std::string sesstag = rc().session_tag();
                    sessionfile sf(sessionfilename, sesstag, rc());
                    sessionmodified = sf.parse();
                    if (! sessionmodified)
                    {
                        /*
                         * The session tag is not found. It seems best
                         * to just bug out. The GUI will still appear in
                         * order to show the error message.
                         */

                        std::string msg = "Session tag [";
                        msg += rc().session_tag();
                        msg += "] in ";
                        msg += sessionfilename;
                        msg += " not found.\nExit and try again.";
                        append_error_message(msg);
                        return false;
                    }
                }
            }

            /*
             * If "-o log=file.ext" occurred, handle it early on in startup.
             */

            if (! sessionmodified)
                (void) cmdlineopts::parse_log_option(argc, argv);

            /*
             *  If parsing fails, report it and disable usage of the
             *  application and saving bad garbage out when exiting.  Still
             *  must launch, otherwise a segfault occurs via dependencies in
             *  the qsmainwnd.
             */

            if (! in_nsm)
            {
                std::string errmessage;             /* just in case!        */
                result = cmdlineopts::parse_options_files(errmessage);
                if (result)
                {
                    if (argc > 1)
                    {
                        optionindex = cmdlineopts::parse_command_line_options
                        (
                            argc, argv
                        );
                        result = optionindex >= 0;
                    }
                }
                else
                {
                    errprint(errmessage);
                    append_error_message(errmessage); /* raises the message */
                }
            }
            if (result)
            {
                /*
                 * The 'usr' file might not specify a log-file. Check again
                 * here.
                 */

                (void) cmdlineopts::parse_o_options(argc, argv);
                bool uselog = ! (seq_app_cli() && rc().verbose()) &&
                    usr().option_use_logfile();


                /*
                 * The user migh specify -o options that are also set up in
                 * the 'usr' file; the command line must take precedence. The
                 * "log" option is processed early in the startup sequence.
                 * These same settings are made in the cmdlineopts module.
                 */

                if (uselog)
                {
                    std::string logfile = usr().option_logfile();
                    if (logfile.empty())
                        logfile = "/dev/null";          /* Windows Mingw ok? */

                    reroute_to_log(logfile);
                }
                m_midi_filename.clear();
                if (optionindex > 0 && optionindex < argc) /* MIDI filename? */
                {
                    std::string fname = argv[optionindex];
                    std::string errmsg;
                    if (file_readable(fname))
                    {
                        std::string path;           /* not used here        */
                        std::string basename;
                        m_midi_filename = fname;
                        if (filename_split(fname, path, basename))
                        {
                            rc().midi_filename(basename);
                            rc().playlist_active(false);
                        }
                    }
                    else
                    {
                        char temp[512];
                        (void) snprintf
                        (
                            temp, sizeof temp,
                            "MIDI file not readable: '%s'", fname.c_str()
                        );
                        append_error_message(temp); /* raises the message   */
                        m_midi_filename.clear();
                    }
                }
            }
        }
    }
    return result;
}

/**
 *  This function is currently meant to be called by the owner of this
 *  smanager.  This call must occur before creating the application main
 *  window.
 *
 *  Otherwise, seq66 will not register with NSM (if enabled) in a timely
 *  fashion.  Also, we always have to launch, even if an error occurred, to
 *  avoid a segfault and show at least a minimal message.
 *
 * NSM:
 *
 *  The NSM API requires that applications MUST NOT register their JACK client
 *  until receiving an NSM "open" message.  So, in the main() function of the
 *  application, we make the smanager calls in this order:
 *
 *      -#  main_settings().  Gets the normal Seq66 configuration items from
 *          the "rc", "usr", "mutes", "ctrl", and "playlist" files.
 *      -#  create_session().  Sets up the session and does the NSM "announce"
 *          handshake protocol. We ignore the return code so that Seq66 can
 *          run even if NSM is not available.  This call also receives the
 *          "open" response, which provides the NSM path, display name, and
 *          the client ID.
 *      -#  create_project().  This function firsts makes sure that the client
 *          directory [$HOME/NSM Sessions/Session/seq66.nXYZZ] exists.
 *          It then gets the session configuration.  Do we want to
 *          reload a complete set of configuration files from this directory,
 *          or just a session-specific subset from an "nsm" file ???
 *          Most of this work is in the non-GUI-specific clinsmanager
 *          derived class.
 *      -#  create_performer().  This sets up the ports and launches the
 *          threads.
 *      -#  open_playist().  If applicable.  Probably better as part of the
 *          session.
 *      -#  open_midi_file().  If applicable.  Probably better as part of the
 *          session.
 *      -#  create_window().  This creates the main window, and the Sessions
 *          tab can be hidden if not needed.  Menu entries can also be
 *          adjusted for session support; see qsmainwnd.
 *      -#  run().  The program runs until the user or NSM kills it.  This is
 *          called from the main() function of the application.
 *      -#  close_session().  Should tell NSM that it is bowing out of the
 *          session.  Done normally in main(), but here if a serious error
 *          occurs.
 *
 * \return
 *      Returns false if the performer wasn't able to be created and launched.
 *      Other failures, such as not getting good settings, might be ignored.
 */

bool
smanager::create_performer ()
{
    bool result = false;
    int ppqn = choose_ppqn();
    int rows = usr().mainwnd_rows();
    int cols = usr().mainwnd_cols();
    pointer p(new (std::nothrow) performer(ppqn, rows, cols));
    result = bool(p);
    if (result)
    {
        m_perf_pointer = std::move(p);              /* change the ownership */
        (void) perf()->get_settings(rc(), usr());
        result = perf()->launch(ppqn);              // std::string perfmsgs;
        if (result)
        {
            // Anything to do?
        }
        else
        {
            errprint("performer launch failed");
        }
    }
    else
    {
        errprint("performer creation failed");
    }
    return result;
}

/**
 *  Code moved from rcfile to here while researching issue #89.
 *
 * \return
 *      Returns true if the 'ctrl' file was able to be opened.
 *      If not active, we don't bother to report the error.
 */

bool
smanager::open_midi_control_file ()
{
    std::string fullpath = rc().midi_control_filespec();
    bool result = ! fullpath.empty();
    if (result)
    {
        result = read_midi_control_file(fullpath, rc());
        if (rc().midi_control_active() && ! result)
            append_error_message("Read failed", fullpath);
    }
    return result;
}

/**
 *  Opens a playlist, if specified.  It is opened and read if there is an
 *  empty or non-empty play-list filename, even if specified to be inactive.
 *  An empty filename results in a basically empty, but ultimately populable,
 *  playlist.  Saves a lot of pointer checks.
 *
 * \return
 *      Returns true if a playlist was specified and successfully opened, or
 *      if there is no playlist called for.  In other words, true is returned
 *      if there is no error.
 */

bool
smanager::open_playlist ()
{
    bool result = not_nullptr(perf());
    if (result)
    {
        std::string playlistname = rc().playlist_filespec();
        result = perf()->open_playlist(playlistname);
        if (result)
        {
            result = perf()->open_current_song();
        }
        else
        {
            if (rc().playlist_active())
            {
                std::string msg = "Playlist open failed: '";
                msg += playlistname;
                msg += "'";
                append_error_message(msg);
            }
            result = true;                          /* avoid early exit  */
        }
    }
    else
    {
        append_error_message("Open playlist: no performer");
    }
    return result;
}

bool
smanager::open_note_mapper ()
{
    bool result = not_nullptr(perf());
    if (result)
    {
        std::string notemapname = rc().notemap_filespec();
        if (! notemapname.empty())
        {
            result = perf()->open_note_mapper(notemapname);
            if (! result)
                result = true;                          /* avoid early exit  */
        }
    }
    else
    {
        append_error_message("Open note-mapper: no performer");
    }
    return result;
}

/**
 *  Encapsulates opening the MIDI file, if specified (on the command-line).
 *
 * \return
 *      Returns the name of the MIDI file, if successful.  Otherwise, an empty
 *      string is returned.
 */

std::string
smanager::open_midi_file (const std::string & fname)
{
    std::string result = fname;
    bool ok = file_readable(result);
    midi_filename("");                                      /* side-effect  */
    if (ok)
    {
        std::string errmsg;
        ok = perf()->read_midi_file(result, errmsg);
        if (ok)
        {
            std::string infomsg = "PPQN set to ";
            infomsg += std::to_string(perf()->ppqn());
            info_message(infomsg);
            (void) perf()->apply_session_mutes();

            /*
             * Redundant: file_message("Open", result);
             */

            midi_filename(result);                          /* side-effect  */
            rc().playlist_active(false);                    /* disable it   */
        }
        else
            append_error_message(errmsg);
    }
    else
        append_error_message("MIDI unreadable", result);

    return result;
}

/**
 *  The clinsmanager::create_session() function supercedes this one, but calls
 *  it.
 */

bool
smanager::create_session (int /*argc*/, char * /*argv*/ [])
{
    session_setup();             /* daemonize: set basic signal handlers */
    return true;
}

/**
 *  Closes the session, with an option to handle errors in the session.
 *
 *  Note that we do not save if in a session, as we rely on the session
 *  manager to tell this application to save before forcing this application
 *  to end.
 *
 * \param [out] msg
 *      Provides a place to store any error message for the caller to use.
 *
 * \param ok
 *      Indicates if an error occurred, or not.  The default is true, which
 *      indicates "no problem".
 *
 * \return
 *      Returns the ok parameter if false, otherwise, the result of finishing
 *      up is returned.
 */

bool
smanager::close_session (std::string & msg, bool ok)
{
    bool result = not_nullptr(perf());
    if (result)
    {
        result = perf()->finish();             /* tear down performer       */
        perf()->put_settings(rc(), usr());     /* copy latest settings      */
        if (result)
            (void) save_session(msg, result);
    }
    result = ok;
    (void) session_close();                    /* daemonize signals exit   */
    return result;
}

/**
 *  This function saves the following files (so far):
 *
 *      -   *.rc
 *      -   *.ctrl (via the 'rc' file)
 *      -   *.mutes (via the 'rc' file)
 *      -   *.usr
 *      -   *.drums
 *
 *  The clinsmanager::save_session() function saves the MIDI file to the
 *  session, if applicable.  That function also clears the message parameter
 *  before the saving starts.
 *
 * \param [out] msg
 *      Provides a place to store any error message for the caller to use.
 *
 * \param ok
 *      Indicates if an error occurred, or not.  The default is true, which
 *      indicates "no problem".
 */

bool
smanager::save_session (std::string & msg, bool ok)
{
    bool result = not_nullptr(perf());
    if (result)
    {
        if (ok)                             /* code from clinsmanager here  */
        {
            if (perf()->modified())         /* i.e. MIDI file is modified   */
            {
                std::string filename = rc().midi_filename();
                if (filename.empty())
                {
                    /* Don't need this: msg = "MIDI file-name empty"; */
                }
                else
                {
                    bool is_wrk = file_extension_match(filename, ".wrk");
                    if (is_wrk)
                        filename = file_extension_set(filename, ".midi");

                    result = write_midi_file(*perf(), filename, msg);
                    if (result)
                        msg = result ? "Saved: " : "Not able to save: " ;

                    msg += filename;
                }
            }
        }
        if (result && ok)
        {
            bool save = rc().auto_options_save();
            if (save)
            {
                session_message("Save", "Options");
                if (! cmdlineopts::write_options_files())
                    msg = "Config writes failed";
            }
            if (rc().auto_ctrl_save())
            {
                std::string mcfname = rc().midi_control_filespec();
                session_message("Save", "Controls");
                result = write_midi_control_file(mcfname, rc());
            }
            if (rc().auto_mutes_save())
            {
                session_message("Save", "Mutes");
                result = perf()->save_mutegroups();         // add msg return?
            }
            if (rc().auto_playlist_save())
            {
                session_message("Save", "Playlist");
                result = perf()->save_playlist();           // add msg return?
            }
            if (rc().auto_drums_save())
            {
                session_message("Save", "Note-mapper");
                result = perf()->save_note_mapper();        // add msg return?
            }
        }
        else
        {
            result = false;
            if (! is_help())
            {
                (void) cmdlineopts::write_options_files("erroneous");
                if (error_active())
                {
                    errprint(error_message());
                    msg = error_message();
                }
            }
        }
    }
    else
    {
        msg = "no performer!";
    }
    return result;
}

/**
 *  This function is overridden in qt5nsmanager to actually create the
 *  user-interface.
 */

bool
smanager::create_window ()
{
    return true;
}

/**
 *  Sets the error flag and appends to the error message, which are both mutable
 *  so that we can safely call this function under any circumstances.
 *
 * \param message
 *      Provides the message to be set. If empty, the message-active flag and
 *      the message are both cleared.
 *
 * \param data
 *      Optional additional data for the message.
 */

void
smanager::append_error_message
(
    const std::string & msg,
    const std::string & data
) const
{
    if (msg.empty())
    {
        m_extant_errmsg.clear();
        m_extant_msg_active = false;
    }
    else
    {
        std::string fullmessage = msg;
        if (! data.empty())
        {
            fullmessage += ": '";
            fullmessage += data;
            fullmessage += "'";
        }
        m_extant_msg_active = true;
        if (! m_extant_errmsg.empty())
            m_extant_errmsg += "\n";

        m_extant_errmsg += fullmessage;
    }
}

/*
 * Having this here after creating the main window may cause issue
 * #100, where ladish doesn't see seq66's ports in time.
 *
 *      perf()->launch(usr().midi_ppqn());
 *
 * We also check for any "fatal" PortMidi errors, so we can display
 * them.  But we still want to keep going, in order to at least
 * generate the log-files and configuration files to
 * C:/Users/me/AppData/Local/seq66 or ~/.config/seq66.
 */

void
smanager::show_message (const std::string & tag, const std::string & msg) const
{
    std::string fullmsg = tag + ": " + msg;
    seq66::info_message(fullmsg);       /* checks for "debug" and adds "[]" */
}

void
smanager::show_error (const std::string & tag, const std::string & msg) const
{
    std::string fullmsg = tag + ": " + msg;
    seq66::error_message(msg);
}

/**
 *  Checks for an internal (e.g. PortMidi) error, storing the message if
 *  applicable.
 *
 *  ca 2023-09-24
 *  For portmidi, the internal_error_check() might not find an error,
 *  while a port-map error does exist. So we add a check of the latter here.
 *
 * \param [out] errmsg
 *      Provides a destination for the PortMidi error.  It is cleared if
 *      there is no error.
 *
 * \return
 *      Returns true if there is an error.  In this case, the caller should
 *      show the error message.
 */

bool
smanager::internal_error_check (std::string & errmsg) const
{
    std::string pmerrmsg;
    errmsg.clear();

#if defined SEQ66_PORTMIDI_SUPPORT

    /*
     * We should eventually get this code into the midibus arena for PortMidi
     * and for RtMidi support.
     */

    bool result = bool(Pm_error_present());
    if (result)
    {
        const char * perr = Pm_error_message();
        if (not_nullptr(perr) && std::strlen(perr) > 0)
            pmerrmsg = std::string(perr);
    }
    else
    {
        result = perf()->port_map_error();              /* ca 2023-09-24 */
    }

#else

    bool result = internal_error_pending();

#endif

    if (result)
    {
        if (pmerrmsg.empty())
            pmerrmsg = perf()->error_messages();
        else
            pmerrmsg += s_port_error_msg;

        append_error_message(pmerrmsg);
        errmsg = pmerrmsg;
    }
    return result;
}

/**
 *  Shouldn't we be using the 'usr' log-file name???
 */

void
smanager::error_handling ()
{
    std::string errmsg;
    bool internal_error = internal_error_check(errmsg);
    bool session_error = error_active();
    std::string path = seq66::rc().config_filespec(seq_default_logfile_name());

#if defined SEQ66_PORTMIDI_SUPPORT
    const char * pmerrmsg = pm_log_buffer();    /* guaranteed to be valid   */
    errmsg += "\n";
    errmsg += std::string(pmerrmsg);
#endif

    if (internal_error)
    {
        show_error("Internal error.", errmsg);
    }
    else if (session_error)
    {
        errmsg += error_message();
        show_error("Session error.", errmsg);
    }
    (void) seq66::file_append_log(path, errmsg);
}

/**
 *  Refactored so that the basic NSM session can be set up before launch(), as
 *  per NSM rules.
 *
 *  The following call detects a session, creates an nsmclient, sends an NSM
 *  announce message, waits for the response, uses it to set the session
 *  information.  What we really see:
 *
 *      nsmclient::announce()       Send announcement, wait for response
 *      <below>                     Gets manager path!!!
 *      nsmclient::open()           Sets manager path
*
 *  We run() the window, get the exit status, and close the session in the
 *  main() function of the application.
 *
 *  Call sequence summary:
 *
 *      -   main_settings()
 *      -   create_session()
 *      -   create_project()
 *      -   create_performer()
 *      -   open_playlist()
 *      -   open_note_mapper()
 *      -   open_midi_file() if specified on command-line; otherwise
 *      -   Open most-recent file if that option is enabled:
 *          Get full path to the most recently-opened or imported file.  What
 *          if smanager::open_midi_file() has already been called via the
 *          command-line? Then skip this step.
 *      -   create_window()
 *      -   run(), done in main()
 *      -   close_session(), done in main()
 */

bool
smanager::create (int argc, char * argv [])
{
    bool result = main_settings(argc, argv);
    if (result)
    {
        bool ok = create_session(argc, argv);       /* path, client ID, etc */
        if (ok)
        {
            /*
             * Need to test this change under NSM and Windows!
             *
             * rc().session_directory();
             */

            std::string homedir = manager_path();   /* session manager path */
            if (homedir == "None")
                homedir = rc().home_config_directory();

            session_message("Session manager path", homedir);
            (void) create_project(argc, argv, homedir);
        }
        if (ok)
            (void) open_midi_control_file();

        /*
         * We don't want to return a false result, otherwise seq66 will
         * exit mysteriously, without showing any message. So we use an
         * internal flag. Found while investigating issue #110. Done for
         * everything to line 875. We also have to call open_playlist()
         * to avoid a segfault later.
         */

        ok = create_performer();
        if (ok)
        {
            std::string fname = midi_filename();
            if (fname.empty())
            {
                if (rc().load_most_recent())
                {
                    std::string midifname = rc().recent_file(0, false);
                    if (! midifname.empty())
                    {
                        /*
                         * We don't want to necessarily save the 'rc' file
                         * at exit just because we're loading the most
                         * recent file.
                         */

                        bool autorcsave = rc().auto_rc_save();
                        (void) open_midi_file(midifname);
                        rc().auto_rc_save(autorcsave);
                    }
                }
            }
            else
                (void) open_midi_file(fname);

        }
        ok = open_playlist();
        if (ok)
            ok = open_note_mapper();

#if defined USE_CRIPPLED_RUN

        /*
         * This code is currently disabled because too many null pointers
         * end up being found, causing crashes.  For now we rely on
         * console error messages.
         */

        else                                            /* ca 2023-04-15    */
        {
            std::string msg;
            (void) create_window();
            error_handling();
            (void) create_session();
            (void) run();
            (void) close_session(msg, false);
        }
#endif

        if (result)
        {
            if (perf()->error_pending())
                append_error_message(perf()->error_messages());

            result = create_window();
            if (result)
            {
                if (perf()->new_ports_available())
                    show_message("Session note.", s_port_update_msg);
                else
                    error_handling();
            }
            else
            {
                std::string msg;                        /* maybe errmsg?    */
                result = close_session(msg, false);
                session_message("Window creation error", msg);
            }
        }
        if (! is_help())
            cmdlineopts::show_locale();
    }
    else
    {
        if (! is_help())
        {
            std::string msg;
            (void) create_performer();
            (void) create_window();
            error_handling();
            (void) create_session();
            (void) run();
            (void) close_session(msg, false);
        }
    }
    return result;
}

bool
smanager::create_configuration
(
    int argc, char * argv [],
    const std::string & mainpath,
    const std::string & cfgfilepath,
    const std::string & midifilepath
)
{
    bool result = ! cfgfilepath.empty();
    if (result)
    {
        std::string rcbase = rc().config_filename();
        rc().midi_filepath(midifilepath);               /* do this first    */
        rc().full_config_directory(cfgfilepath);        /* set session dir. */

        std::string rcpath = rc().home_config_directory();
        std::string rcfile = filename_concatenate(rcpath, rcbase);
        bool already_created = file_exists(rcfile);
        if (already_created)
        {
            session_message("File exists", rcfile);     /* comforting       */
            result = read_configuration(argc, argv, cfgfilepath, midifilepath);
            if (result)
            {
                if (usr().in_nsm_session())
                {
                    rc().auto_rc_save(true);
                }
                else
                {
                    /*
                     * In some cases we might require rc to be rewritten.
                     */

                    bool r = rc().auto_rc_save();       /* 'rc' save?       */
                    bool u = rc().auto_usr_save();      /* --user-save?     */
                    rc().set_save_list(false);          /* clear them all   */
                    rc().auto_rc_save(r);               /* restore it       */
                    rc().auto_usr_save(u);              /* restore it       */
                }
            }
        }
        else
        {
            if (! m_rerouted)
            {
                usr().option_logfile(seq_default_logfile_name());
                reroute_to_log(usr().option_logfile());
            }
            result = make_directory_path(mainpath);
            if (result)
            {
                session_message("Main path", mainpath);
                result = make_directory_path(cfgfilepath);
            }
            if (result && ! midifilepath.empty())
            {
                result = make_directory_path(midifilepath);
                if (result)
                    session_message("MIDI path", midifilepath);
            }
            rc().set_save_list(true);                   /* save all configs */
            if (usr().in_nsm_session())
            {
                usr().session_visibility(false);        /* new session=hide */
                rc().load_most_recent(true);            /* issue #41        */
                rc().jack_auto_connect(false);          /* issue #48        */
            }

            /*
             * The options files and other files are written at exit.
             * We might provide a menu command to write them at any time.
             */

#if defined WRITE_OPTIONS_FILES_AT_STARTUP
            if (result)
            {
                file_message("Saving session configuration", cfgfilepath);
                result = cmdlineopts::write_options_files();
                if (result)
                    result = create_playlist(cfgfilepath, midifilepath);

                if (result)
                    result = create_notemap(cfgfilepath);
            }
#endif
        }
    }
    return result;
}

bool
smanager::create_playlist
(
    const std::string & cfgfilepath,
    const std::string & midifilepath
)
{
    bool result = true;
    std::string srcplayfile = rc().playlist_filename();
    if (srcplayfile.empty())
        srcplayfile = "empty.playlist";

    /*
     * The following calls splits the srcplayfile into a path and basename,
     * then appends the basename to the cfgfilepath.
     */

    std::string dstplayfile = file_path_set(srcplayfile, cfgfilepath);
    if (! rc().playlist_active())
    {
        warnprint("Playlist inactive, saving anyway");
    }
    if (dstplayfile.empty())
    {
        file_error("Playlist file", "none");
    }
    else
    {
        std::string s("Temp");
        performer * p(nullptr);
        std::shared_ptr<playlist> plp;
        plp.reset(new (std::nothrow) playlist(p, s, false));
        result = bool(plp);
        if (result)
        {
            std::string homepath = rc().home_config_directory();
            srcplayfile = file_path_set(srcplayfile, homepath);
            (void) save_playlist(*plp, srcplayfile, dstplayfile);
            if (! midifilepath.empty())
            {
                (void) copy_playlist_songs
                (
                    *plp, srcplayfile, midifilepath
                );
            }

            /*
             * The first is where MIDI files are stored in the session, and
             * the second is where they are stored for the play-list.
             * Currently, the same directory.
             */

            rc().midi_filepath(midifilepath);
            rc().midi_base_directory(midifilepath);
        }
    }
    return result;
}

bool
smanager::create_notemap (const std::string & cfgfilepath)
{
    bool result = true;
    std::string srcnotefile = rc().notemap_filename();
    if (srcnotefile.empty())
        srcnotefile = "empty.drums";

    std::string dstnotefile = file_path_set(srcnotefile, cfgfilepath);
    if (! rc().notemap_active())
    {
        warnprint("Note-map not active, saving anyway");
    }

    std::string destination = rc().notemap_filename();
    if (destination.empty())
    {
        warnprint("Note-map file name empty");
    }
    else
    {
        std::string homepath = rc().home_config_directory();
        std::shared_ptr<notemapper> nmp;
        nmp.reset(new (std::nothrow) notemapper());
        result = bool(nmp);
        file_message("Note-mapper save", destination);
        srcnotefile = file_path_set(srcnotefile, homepath);
        (void) copy_notemapper(*nmp, srcnotefile, destination);
    }
    return result;
}

bool
smanager::read_configuration
(
    int argc, char * argv [],
    const std::string & cfgfilepath,
    const std::string & midifilepath
)
{
    rc().full_config_directory(cfgfilepath);    /* set NSM dir      */
    rc().midi_filepath(midifilepath);           /* set MIDI dir     */
    if (! midifilepath.empty())
    {
        file_message("MIDI path", rc().midi_filepath());
        file_message("MIDI file", rc().midi_filename());
    }

    std::string errmessage;
    bool result = cmdlineopts::parse_options_files(errmessage);
    if (result)
    {
        /*
         * Perhaps at some point, the "rc"/"usr" options might affect NSM
         * usage.  In the meantime, we still need command-line options, if
         * present, to override the file-specified options.  One big example
         * is the --buss override. The smanager::main_settings() function is
         * called way before create_project();
         */

        if (argc > 1)
        {
            int rcode = cmdlineopts::parse_command_line_options(argc, argv);
            result = rcode != (-1);
            if (result)
                (void) cmdlineopts::parse_o_options(argc, argv);
            else
                is_help(true);          /* a hack to avoid create_window()  */
        }
    }
    else
    {
        file_error(errmessage, rc().config_filespec());
    }
    return result;
}

bool
smanager::make_path_names
(
    const std::string & path,
    std::string & outcfgpath,
    std::string & outmidipath,
    const std::string & midisubdir
)
{
    bool result = ! path.empty();
    if (result)
    {
        std::string cfgpath = normalize_path(path);
        std::string midipath = cfgpath;
        if (! midisubdir.empty())
            midipath = pathname_concatenate(cfgpath, midisubdir);

        outcfgpath = cfgpath;
        outmidipath = midipath;
    }
    return result;
}

/**
 *  Imports configuration files from another directory into the current
 *  "home" directory. Implements File / Import / Project Configuration.
 *
 *  Function for the main window to call. It deletes the existing
 *  configuration, copies the source configuration, then calls
 *  import_configuration_items().
 *
 *  When this function succeeds, we need to signal a session-reload and the
 *  make settings as done in qsmainwnd::import_project().
 *
 *  We don't really need to care if NSM is active here, do we?
 *
 * \param sourcepath
 *      Provides the name of the source directory.
 *
 * \param sourcebase
 *      Provides the basename (e.g. "qseq66.rc") of the source
 *      configuration file.
 *
 * \return
 *      Returns true if the import succeeds. One source of failure
 *      is specifying the current "home" directory as the source
 *      path. The configuration to import must come from a different
 *      subdirectory.
 */

bool
smanager::import_into_session
(
    const std::string & sourcepath,
    const std::string & sourcebase              /* e.g. qrseq66.rc */
)
{
    bool result = ! sourcepath.empty() && ! sourcebase.empty();
    if (result)
    {
        std::string destdir = rc().home_config_directory();
        std::string destbase = rc().config_filename();
        result = destdir != sourcepath;
        if (result)
        {
            std::string cfgpath;
            std::string midipath;
            session_message("Source", sourcepath + sourcebase);
            session_message("Destination", destdir);
            result = make_path_names(destdir, cfgpath, midipath);
            if (result)
            {
                result = delete_configuration(cfgpath, destbase);
                if (result)
                {
                    result = copy_configuration(sourcepath, sourcebase, cfgpath);
                    if (result)
                    {
                        result = import_configuration_items
                        (
                            sourcepath, sourcebase, cfgpath, midipath
                        );
                    }
                }
            }
        }
    }
    return result;
}

/**
 *  Copies the relevant files in the "home" configuration to another directory.
 *
 *  How do we want this to work?
 *
 *      -   Append (and create if necessary) a subdirectory?
 *          -   "config" for NSM usage.
 *          -   No additional directory name otherwise.
 *          -   The destbase name (stripped of extension)
 *      -   Force the user to have already created the full desired path?
 *          No, we can make the directory as needed.
 *
 * Idea:
 *
 *  -   First save these:
 *      -   rc().set_config_directory()
 *      -   rc().set_config_files()
 *  -   Then change them.
 *  -   Make sure the destination path exists, creating it if necessary.
 *  -   Call some of the code present in save_session().
 *  -   Restore the saved values.
 *
 * \param destpath
 *      Provides the full destination path.
 *
 * \param destbase
 *      Provides the basename to use (e.g. "qseq66").
 */

bool
smanager::export_session_configuration
(
    const std::string & destpath,
    const std::string & destbase
)
{
    bool result = not_nullptr(perf());
    if (result)
    {
        std::string sourcedir = rc().home_config_directory();
        std::string sourcebase = rc().config_filename();
        result = ! destpath.empty() && ! destbase.empty();
        if (result)
        {
            result = sourcedir != destpath;
            if (result)
                file_message("Export destination", destpath);
            else
                file_error("Export destination = source", destpath);
        }
        if (result)
        {
            std::string srcpalette = rc().palette_filespec();
            std::string srcqss = rc().style_sheet_filespec();
            rc().home_config_directory(destpath);
            rc().config_filename(destbase);
            result = make_directory_path(destpath);
            if (result)
                result = cmdlineopts::alt_write_rc_file(destbase);

            if (result)
            {
                result = cmdlineopts::alt_write_usr_file(destbase);
                if (result)
                {
                    std::string mcfname = rc().midi_control_filespec();
                    result = write_midi_control_file(mcfname, rc());
                    if (result)
                        result = perf()->save_mutegroups();

                    if (result)
                        result = perf()->save_playlist();

                    if (result)
                        result = perf()->save_note_mapper();

                    if (result)
                    {
                        std::string destpalette = rc().palette_filespec();
                        std::string destqss = rc().style_sheet_filespec();
                        file_message("Write palette", destpalette);
                        result = file_copy(srcpalette, destpalette);
                        if (result)
                        {
                            file_message("Write qss", destpalette);
                            result = file_copy(srcqss, destqss);
                        }
                    }
                }
                if (! result)
                    file_error("usr export failed", destpath);
            }
            else
                file_error("rc export failed", destpath);

            rc().home_config_directory(sourcedir);
            rc().config_filename(sourcebase);
        }
    }
    else
    {
        file_error("no performer!", "TODO");
    }
    return result;
}

/**
 *  This function is like create_configuration(), but the source is not the
 *  standard configuration-file directory, but a directory chosen by the user.
 *  This function should be called only for importing a configuration into an
 *  NSM session directory. Called by import_into_session() above.
 *
 * \param sourcepath
 *      The source path is the path to the configuration files chosen for
 *      import by the user.  Often it is the usual ~/.config/seq66 directory.
 *
 * \param sourcebase
 *      This is the actual configuration file chosen by the user.  It can be
 *      any file qseq66.*, only the first part of the name (e.g. "qseq66") is
 *      used.
 *
 * \param cfgfilepath
 *      This is the configuration directory used in this NSM session.  It is
 *      often something like "~/NSM Sessions/MySession/seq66.nLTQC/config.
 *      This is the destination for the imported configuration files.
 *
 * \param midifilepath
 *      This is the MIDI directory used in this NSM session.  It is often
 *      something like "~/NSM Sessions/MySession/seq66.nLTQC/midi.  This is
 *      the destination for the imported MIDI files (due to being part of an
 *      imported play-list.
 *
 * \return
 *      Returns true if the import succeeded.
 */

bool
smanager::import_configuration_items
(
    const std::string & sourcepath,
    const std::string & sourcebase,
    const std::string & cfgfilepath,
    const std::string & midifilepath
)
{
    bool result = ! sourcepath.empty() && ! sourcebase.empty();
    if (result)
    {
        std::string rcbase = file_extension_set(sourcebase, ".rc");
        std::string rcfile = filename_concatenate(sourcepath, rcbase);
        result = file_exists(rcfile);                   /* a valid source   */
        if (result)
        {
            std::string usrbase = file_extension_set(sourcebase, ".usr");
            std::string usrfile = filename_concatenate(sourcepath, usrbase);
            file_message("File exists", rcfile);        /* comforting       */
            rc().config_filename(rcfile);
            rc().user_filename(usrfile);

            std::string errmessage;
            result = cmdlineopts::parse_rc_file(rcfile, errmessage);
            if (result)
                result = cmdlineopts::parse_usr_file(usrfile, errmessage);

            /*
             * Don't change this.
             */

            if (result)
            {
                if (usr().in_nsm_session())
                    rc().load_most_recent(true);        /* issue #41        */
            }
        }
        if (result)
        {
            std::string srcplayfile = rc().playlist_filename();
            std::string srcnotefile = rc().notemap_filename();
            if (srcplayfile.empty())
                srcplayfile = "empty.playlist";

            if (srcnotefile.empty())
                srcnotefile = "empty.drums";

            file_message("Saving imported configuration", cfgfilepath);
            rc().auto_usr_save(true);
            result = cmdlineopts::write_options_files();
            if (result)
                result = create_playlist(cfgfilepath, midifilepath);

            if (result)
                result = create_notemap(cfgfilepath);
        }
    }
    return result;
}

}           // namespace seq66

/*
 * smanager.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


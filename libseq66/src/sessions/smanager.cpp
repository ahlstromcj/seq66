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
 * \updates       2022-07-21
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
 *      -# Call create_performer(), which could delete an existing performer.
 *         It then launches the performer.  Save the unique-pointer.
 *      -# Call open_playlist().  It will open it, if specified and possible.
 e      -# Call open_note_mapper().  It will open it, if specified and possible.
 *      -# If the MIDI file-name is set, open it via a call to open_midi_file().
 *      -# If a user-interface is needed, create a unique-pointer to it, then
 *         show it.  This will remove any previous pointer.  The function is
 *         virtual, create_window().
 */

#include <cstring>                      /* std::strlen()                    */

#include "seq66_features.hpp"           /* set_app_name()                   */
#include "cfg/cmdlineopts.hpp"          /* command-line functions           */
#include "cfg/midicontrolfile.hpp"      /* seq66::midicontrolfile functions */

/*
 * These files are handled by performer, but also by "global" functions
 * in this module. Probably something to tighten up.
 */

#include "cfg/notemapfile.hpp"          /* seq66::notemapfile functions     */
#include "cfg/playlistfile.hpp"         /* seq66::playlistfile functions    */

#include "cfg/sessionfile.hpp"          /* seq66::sessionfile               */
#include "cfg/settings.hpp"             /* seq66::usr() and seq66::rc()     */
#include "midi/midifile.hpp"            /* seq66::write_midi_file()         */
#include "play/performer.hpp"           /* seq66::performer                 */
#include "play/playlist.hpp"            /* seq66::playlist class            */
#include "sessions/smanager.hpp"        /* seq66::smanager()                */
#include "os/daemonize.hpp"             /* seq66::reroute_stdio()           */
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
 *  Does the usual construction.  It also calls set_defaults() from the
 *  settings.cpp module in order to guarantee that we have rc() and usr()
 *  available.  See that function for more information.
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
    m_extant_errmsg         (),
    m_extant_msg_active     (false)
{
    /*
     * This has to wait: m_perf_pointer = create_performer();
     */

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
        (void) session_message("Exiting session manager");
}

/**
 *  The standard C/C++ entry point to this application.  The first thing is to
 *  set the various settings defaults, and then try to read the "user" and
 *  "rc" configuration files, in that order.  There are currently no options
 *  to change the names of those files.  If we add that code, we'll move the
 *  parsing code to where the configuration file-names are changed from the
 *  command-line.  The last thing is to override any other settings via the
 *  command-line parameters.
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
    bool result = false;                /* false --> EXIT_FAILURE           */
    if (seq_app_cli())
    {
        set_app_name("seq66cli");
        set_app_type("cli");
        set_client_name("seq66cli");
        rc().set_config_files("seq66cli");
    }
    else
        set_app_name(SEQ66_APP_NAME);   /* "qseq66" by default              */

    set_arg_0(argv[0]);                 /* how it got started               */

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

    bool ishelp = cmdlineopts::help_check(argc, argv);
    int optionindex = -1;
    bool sessionmodified = false;
    if (ishelp)
    {
        (void) cmdlineopts::parse_command_line_options(argc, argv);
        is_help(true);
        result = false;
    }
    else
    {
        int rcode = cmdlineopts::parse_command_line_options(argc, argv);
        result = rcode != (-1);
        if (! result)
            is_help(true);              /* a hack to avoid create_window()  */
    }
    if (result)
    {
        if (rc().inspecting())
        {
            std::string sessionfilename = rc().make_config_filespec("session.rc");
            if (file_readable(sessionfilename))
            {
                sessionfile sf(sessionfilename, rc().inspection_tag(), rc());
                sessionmodified = sf.parse();
            }
        }

        /*
         * If "-o log=file.ext" occurred, handle it early on in startup.
         */

        if (! sessionmodified)
            (void) cmdlineopts::parse_log_option(argc, argv);

        /*
         *  If parsing fails, report it and disable usage of the application and
         *  saving bad garbage out when exiting.  Still must launch, otherwise a
         *  segfault occurs via dependencies in the qsmainwnd.
         */

        std::string errmessage;                     /* just in case!        */
        result = cmdlineopts::parse_options_files(errmessage);
        if (! result)
        {
            errprint(errmessage);
            append_error_message(errmessage);       /* raises the message   */
        }
        optionindex = cmdlineopts::parse_command_line_options(argc, argv);
        result = optionindex >= 0;
        if (result)
        {
            (void) cmdlineopts::parse_o_options(argc, argv);

            /*
             * The user migh specify -o options that are also set up in the
             * 'usr' file; the command line must take precedence. The "log"
             * option is processed early in the startup sequence.  These same
             * settings are made in the cmdlineopts module.
             */

            std::string logfile = usr().option_logfile();
            if (usr().option_use_logfile())
                (void) reroute_stdio(logfile);

            m_midi_filename.clear();
            if (optionindex < argc)                 /* MIDI filename given? */
            {
                std::string fname = argv[optionindex];
                std::string errmsg;
                if (file_readable(fname))
                {
                    std::string path;               /* not used here        */
                    std::string basename;
                    m_midi_filename = fname;
                    if (filename_split(fname, path, basename))
                        rc().midi_filename(basename);
                }
                else
                {
                    char temp[512];
                    (void) snprintf
                    (
                        temp, sizeof temp,
                        "MIDI file not readable: '%s'", fname.c_str()
                    );
                    append_error_message(temp);     /* raises the message   */
                    m_midi_filename.clear();
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
        (void) p->get_settings(rc(), usr());
        m_perf_pointer = std::move(p);              /* change the ownership */
        result = perf()->launch(ppqn);
        if (! result)
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
 */

bool
smanager::open_midi_control_file ()
{
    std::string fullpath = rc().midi_control_filespec();
    bool result = ! fullpath.empty();
    if (result)
    {
        result = read_midi_control_file(fullpath, rc());
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
        result = perf()->open_playlist(playlistname, rc().verbose());
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
    std::string midifname;                          /* start out blank      */
    std::string errmsg;
    bool result = perf()->read_midi_file(fname, errmsg);
    if (result)
    {
        std::string infomsg = "PPQN set to ";
        infomsg += std::to_string(perf()->ppqn());
        (void) seq66::info_message(infomsg);
        midifname = fname;
        (void) perf()->apply_session_mutes();
    }
    else
    {
        append_error_message(errmsg); // errmsg = "Open failed: '" + fname + "'";
    }
    return midifname;
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

#if defined SEQ66_SESSION_DETACHABLE

/**
 *  Detaches the session, with an option to handle errors in the session.
 *  It does not stop the performer.
 *
 *  Compare to close_session().  Note sure where to go with this one.
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
smanager::detach_session (std::string & /*msg*/, bool ok)
{
    bool result = not_nullptr(perf());
    if (result)
    {
        result = ok;

        /*
         * Possible code:
         *
         *  if (result && perf()->modified())
         *      (void) save_session(msg, result);
         */
    }
    usr().in_nsm_session(false);                        /* global flag      */
    return result;
}

#endif

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
        if (ok)                     /* code moved from clinsmanager to here */
        {
            if (perf()->modified())
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
                file_message("Save session", "Options");
                if (! cmdlineopts::write_options_files())
                    msg = "Config writes failed";
            }
            if (rc().auto_ctrl_save())
            {
                std::string mcfname = rc().midi_control_filespec();
                file_message("Save session", "Controls");
                result = write_midi_control_file(mcfname, rc());
            }
            if (rc().auto_mutes_save())
            {
                file_message("Save session", "Mutes");
                result = perf()->save_mutegroups();         // add msg return?
            }
            if (rc().auto_playlist_save())
            {
                file_message("Save session", "Playlist");
                result = perf()->save_playlist();           // add msg return?
            }
            if (rc().auto_drums_save())
            {
                file_message("Save session", "Notemapper");
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
 */

void
smanager::append_error_message (const std::string & msg) const
{
    if (msg.empty())
    {
        m_extant_errmsg.clear();
        m_extant_msg_active = false;
    }
    else
    {
        m_extant_msg_active = true;
        if (m_extant_errmsg.empty())
        {
            /* m_extant_errmsg += "? "; */
        }
        else
            m_extant_errmsg += "\n";

        m_extant_errmsg += msg;
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

#else

    bool result = internal_error_pending();

#endif

    if (result)
    {
        pmerrmsg +=
            " Check Edit / Preferences / MIDI Clock and MIDI Input to "
            "see which devices are disabled. Check seq66.log."
            ;
        append_error_message(pmerrmsg);
        errmsg = pmerrmsg;
    }
    return result;
}

void
smanager::error_handling ()
{
    std::string errmsg;
    if (internal_error_check(errmsg))
        show_error("Session error", errmsg);

    std::string path = seq66::rc().config_filespec("seq66.log");

#if defined SEQ66_PORTMIDI_SUPPORT
    const char * pmerrmsg = pm_log_buffer();    /* guaranteed to be valid   */
    errmsg += "\n";
    errmsg += std::string(pmerrmsg);
#endif

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
 *          Get full path to the most recently-opened or imported file.  What if
 *          smanager::open_midi_file() has already been called via the
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
        bool ok = create_session(argc, argv);   /* get path, client ID, etc */
        if (ok)
        {
            std::string homedir = manager_path();
            if (homedir == "None")
                homedir = rc().home_config_directory();

            file_message("Session manager path", homedir);
            (void) create_project(argc, argv, homedir);
        }
        if (ok)
            (void) open_midi_control_file();

        result = create_performer();        /* fails if performer not made  */
        if (result)
        {
            result = open_playlist();
            if (result)
                result = open_note_mapper();
        }
        if (result)
        {
            std::string fname = midi_filename();
            if (fname.empty())
            {
                if (result && rc().load_most_recent())
                {
                    std::string midifname = rc().recent_file(0, false);
                    if (! midifname.empty())
                    {
                        std::string errmsg;
                        std::string tmp = open_midi_file(midifname);
                        if (! tmp.empty())
                        {
                            file_message("Opened", tmp);
                            midi_filename(midifname);
                        }
                    }
                }
            }
            else
            {
                /*
                 * No window at this time; should save the message for later.
                 * For now, write to the console.
                 */

                std::string errormessage;
                std::string tmp = open_midi_file(fname);
                if (! tmp.empty())
                {
                    file_message("Opened", tmp);
                    midi_filename(fname);
                }
            }
        }
        if (result)
        {
            result = create_window();
            if (result)
            {
                error_handling();
            }
            else
            {
                std::string msg;                        /* maybe errmsg? */
                result = close_session(msg, false);
            }

            /*
             * TODO:  expose the error message to the user here
             */
        }
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
        std::string rcfile = filename_concatenate(cfgfilepath, rcbase);
        bool already_created = file_exists(rcfile);
        rc().midi_filepath(midifilepath);               /* do this first    */
        if (already_created)
        {
            file_message("File exists", rcfile);        /* comforting       */
            result = read_configuration(argc, argv, cfgfilepath, midifilepath);
            if (result)
            {
                if (usr().in_nsm_session())
                {
                    rc().auto_rc_save(true);
                }
                else
                {
                    bool u = rc().auto_usr_save();      /* --user-save?     */
                    rc().set_save_list(false);          /* save them all    */
                    rc().auto_usr_save(u);              /* restore it       */
                }
            }
        }
        else
        {
            result = make_directory_path(mainpath);
            if (result)
            {
                file_message("Ready", mainpath);
                result = make_directory_path(cfgfilepath);
                if (result)
                {
                    file_message("Ready", cfgfilepath);
                    rc().full_config_directory(cfgfilepath);
                }
            }
            if (result && ! midifilepath.empty())
            {
                result = make_directory_path(midifilepath);
                if (result)
                    file_message("Ready", midifilepath);
            }
            rc().set_save_list(true);                   /* save all configs */
            if (usr().in_nsm_session())
            {
                usr().session_visibility(false);        /* new session=hide */

#if defined NSM_DISABLE_LOAD_MOST_RECENT
                rc().load_most_recent(false;            /* don't load MIDI  */
#else
                rc().load_most_recent(true);            /* issue #41        */
#endif
            }
#if defined DO_NOT_BELAY_UNTIL_EXIT
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
        std::string cfgpath = path;
        std::string midipath = path;
        std::string subdir = "midi";
        if (! midisubdir.empty())
            subdir = midisubdir;

        if (usr().in_nsm_session())         // nsm_active()
        {
            midipath = pathname_concatenate(cfgpath, subdir);
            cfgpath = pathname_concatenate(cfgpath, "config");
        }
        else
        {
            /*
             * There's no "config" subdirectory outside of an NSM session.
             */

            midipath = pathname_concatenate(midipath, subdir);
        }
        outcfgpath = cfgpath;
        outmidipath = midipath;
    }
    return result;
}

/**
 *  Function for the main window to call.
 *
 *  When this function succeeds, we need to signal a session-reload and the
 *  following settings:
 *
 *  rc().load_most_recent(false);               // don't load MIDI  //
 *  rc().set_save_list(true);                   // save all configs //
 *
 *  We don't really need to care that NSM is active here
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
        std::string cfgpath;
        std::string midipath;
        result = make_path_names(destdir, cfgpath, midipath);
        if (result)
            result = delete_configuration(cfgpath, destbase);

        if (result)
            result = copy_configuration(sourcepath, sourcebase, cfgpath);

        if (result)
        {
            result = import_configuration
            (
                sourcepath, sourcebase, cfgpath, midipath
            );
        }
    }
    return result;
}

/**
 *  This function is like create_configuration(), but the source is not the
 *  standard configuration-file directory, but a directory chosen by the user.
 *  This function should be called only for importing a configuration into an
 *  NSM session directory.
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
smanager::import_configuration
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
#if defined NSM_DISABLE_LOAD_MOST_RECENT
                if (usr().in_nsm_session())
                    rc().load_most_recent(false);       /* don't load MIDI  */
#else
                if (usr().in_nsm_session())
                    rc().load_most_recent(true);        /* issue #41        */
#endif
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


/*
 *  This file is part of seq66.
 *
 *  seq66 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  seq66 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq66; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          smanager.cpp
 *
 *  This module declares/defines a module for managing a generic seq66 session.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2020-03-22
 * \updates       2020-11-16
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
 *      -# Call open_note_mapper().  It will open it, if specified and possible.
 *      -# If the MIDI file-name is set, open it via a call to open_midi_file().
 *      -# If a user-interface is needed, create a unique-pointer to it, then
 *         show it.  This will remove any previous pointer.  The function is
 *         virtual, create_window().
 */

#include <cstring>                      /* std::strlen()                    */

#include "seq66_features.hpp"           /* set_app_name()                   */
#include "cfg/cmdlineopts.hpp"          /* command-line functions           */
#include "cfg/settings.hpp"             /* seq66::usr() and seq66::rc()     */
#include "play/performer.hpp"           /* seq66::performer                 */
#include "sessions/smanager.hpp"        /* seq66::smanager()                */
#include "os/daemonize.hpp"             /* seq66::reroute_stdio()           */
#include "util/basic_macros.hpp"        /* seq66::msgprintf()               */
#include "util/filefunctions.hpp"       /* seq66::file_accessible() etc.    */

#if defined SEQ66_LASH_SUPPORT_NEED_TO_MOVE_THIS
#include "lash/lash.hpp"                /* seq66::lash_driver functions     */
#endif

#if defined SEQ66_PORTMIDI_SUPPORT
#include "portmidi.h"        /*  Pm_error_present(), Pm_hosterror_message() */
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
     * This has to wait:
     *
     * m_perf_pointer = create_performer();
     */

    set_configuration_defaults();
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
    set_app_name(SEQ66_APP_NAME);       /* "qseq66" by default              */
    set_arg_0(argv[0]);                 /* how it got started               */

    /*
     * If "-o log=file.ext" occurred, handle it early on in startup.
     */

    (void) cmdlineopts::parse_log_option(argc, argv);

    /**
     * Set up objects that are specific to the GUI.  Pass them to the
     * performer constructor.  Then parse any command-line options to see if
     * they might affect what gets read from the 'rc' or 'user' configuration
     * files.  They will be parsed again later so that they can still override
     * whatever other settings were made via the configuration files.
     *
     * However, we currently have a issue where the mastermidibus created by
     * the performer object gets the default PPQN value, because the "user"
     * configuration file has not been read at that point.  See the
     * performer::launch() function.
     */

    (void) cmdlineopts::parse_command_line_options(argc, argv);
    bool is_help = cmdlineopts::help_check(argc, argv);
    int optionindex = -1;
    if (is_help)
    {
        m_is_help = true;
    }
    else
    {
        /*
         *  If parsing fails, report it and disable usage of the application
         *  and saving bad garbage out when exiting.  Still must launch,
         *  otherwise a segfault occurs via dependencies in the qsmainwnd.
         */

        std::string errmessage;                     /* just in case!        */
        result = cmdlineopts::parse_options_files(errmessage);
        if (! result)
        {
            errprint(errmessage);
            append_error_message(errmessage);       /* raises the message   */
        }
        optionindex = cmdlineopts::parse_command_line_options(argc, argv);
        if (cmdlineopts::parse_o_options(argc, argv))
        {
            /**
             * The user may have specified -o options that are also set up in
             * the "usr" file.  The command line needs to take precedence.  The
             * "log" option is processed early in the startup sequence.  These
             * same settings are made in the cmdlineopts module.
             */

            std::string logfile = usr().option_logfile();
            if (usr().option_use_logfile() && ! logfile.empty())
                (void) reroute_stdio(logfile);
        }
        if (result)                                 /* get MIDI file-name?  */
        {
            m_midi_filename.clear();
            if (optionindex < argc)                 /* MIDI filename given? */
            {
                std::string fname = argv[optionindex];
                std::string errmsg;
                if (file_accessible(fname))
                {
                    m_midi_filename = fname;
                }
                else
                {
                    char temp[512];
                    (void) snprintf
                    (
                        temp, sizeof temp,
                        "MIDI file error: '%s'", fname.c_str()
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
 *  Otherwise, seq66 will not register with LASH (if enabled) in a timely
 *  fashion.  Also, we always have to launch, even if an error occurred, to
 *  avoid a segfault and show at least a minimal message.  LASH support is now
 *  back in Seq66, sort of.  Working on NSM support at present.
 *
 * NSM:
 *
 *  The NSM API requires that applications MUST NOT register their JACK client
 *  until receiving an NSM "open" message.  So, in the main() function of the
 *  application, we make the smanager calls in this order:
 *
 *      -#  main_settings().  Gets the normal Seq66 configuration items from
 *          the "rc", "usr", "mutes", "ctrl", and "playlist" files.
 *      -#  create_session().  Sets up the session and does the "announce"
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
    int ppqn = choose_ppqn();                       /* usr().midi_ppqn()    */
    int rows = usr().mainwnd_rows();
    int cols = usr().mainwnd_cols();
    pointer p(new (std::nothrow) performer(ppqn, rows, cols));
    result = bool(p);
    if (result)
    {
        (void) p->get_settings(rc(), usr());
        m_perf_pointer = std::move(p);              /* change the ownership */

#if defined SEQ66_PLATFORM_DEBUG
        if (rc().verbose())                         /* for trouble-shooting */
        {
            rc().key_controls().show();
            rc().midi_control_in().show();
            rc().mute_groups().show();
        }
#endif

        result = perf()->launch(ppqn);              /* usr().midi_ppqn())   */
        if (! result)
        {
            errprint("performer::launch() failed");
        }
    }
    else
    {
        errprint("performer creation failed");
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
                std::string msg = "Play-list open failed: '";
                msg += playlistname;
                msg += "'";
                append_error_message(msg);
            }
            result = true;                          /* avoid early exit  */
        }
    }
    else
    {
        append_error_message("Open play-list: no performer");
    }
    return result;
}

/**
 *
 */

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
            if (result)
            {
            }
            else
            {
                // append_error_message(perf()->error_message());
                result = true;                          /* avoid early exit  */
            }
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
smanager::open_midi_file (const std::string & fname, std::string & errmsg)
{
    std::string midifname;                          /* start out blank      */
    bool result = perf()->read_midi_file(fname, errmsg);
    if (result)
    {
        std::string infomsg = "PPQN set to ";
        infomsg += std::to_string(perf()->ppqn());
        (void) seq66::info_message(infomsg);
        midifname = fname;
    }
    else
        (void) seq66::error_message(errmsg);

    return midifname;
}

/**
 *
 */

bool
smanager::create_session (int /*argc*/, char * /*argv*/ [])
{
#if defined SEQ66_LASH_SUPPORT_NEED_TO_MOVE_THIS
    if (usr().is_lash_session())    /* rc().lash_support() */
    {
        if (m_perf_pointer)
            create_lash_driver(*m_perf_pointer, argc, argv);
    }
    else
#endif

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

#if defined SEQ66_LASH_SUPPORT_NEED_TO_MOVE_THIS
        if (rc().lash_support())
            delete_lash_driver();
#endif

    result = ok;
    session_close();                            /* daemonize signals exit   */
    return result;
}

/**
 *  Detaches the session, with an option to handle errors in the session.
 *  It does not stop the performer.
 *
 *  Compare to close_session().
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
#if 0
        if (result && perf()->modified())
            (void) save_session(msg, result);
#endif
    }
    usr().in_session(false);                            /* global flag      */
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
        if (ok)
        {
            bool save = rc().auto_option_save() && ! usr().in_session();
            if (save)
            {
                file_message("Save session", "Options");
                if (! cmdlineopts::write_options_files())
                {
                    errprint("Config writes failed");
                }
            }
            else
                msg = "config auto-save option off";

            if (save)
            {
                file_message("Save session", "Play-list");
                result = perf()->save_playlist();           // add msg return?
            }
            if (save)
            {
                file_message("Save session", "Note-mapper");
                result = perf()->save_note_mapper();        // add msg return?
            }
        }
        else
        {
            result = false;
            if (! m_is_help)
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
    return result;
}

/**
 *  There is, of course, no window in this base class.  Therefore, we just show
 *  patterns if in verbose mode.
 *
 * \return
 *      Always returns true.  No window in the command-line application, no
 *      problem!
 */

bool
smanager::create_window ()
{
    if (rc().verbose())
        perf()->show_patterns();

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

/**
 *
 */

void
smanager::show_message (const std::string & tag, const std::string & msg) const
{
    std::string fullmsg = tag + ": " + msg;
    seq66::info_message(fullmsg);       /* checks for "debug" and adds "[]" */
}

/**
 *
 */

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
    if (result)
    {
        bool interror = internal_error_pending();
        if (interror)
        {
            pmerrmsg +=
                "Go to Edit / Preferences / MIDI Clock and "
                "MIDI Input to see which devices are disabled."
                ;
        }
    }

#else

    bool result = internal_error_pending();
    if (result)
    {
        pmerrmsg =
            "Internal error: Check Edit / Preferences / MIDI Clock and "
            "MIDI Input to see which devices are disabled.  Also check "
            "seq66.log in the configuration directory."
            ;
    }

#endif

    if (result)
    {
        append_error_message(pmerrmsg);
        errmsg = pmerrmsg;
    }
    return result;
}

/**
 *
 */

void
smanager::error_handling ()
{
    std::string errmsg;
    if (internal_error_check(errmsg))
        show_error("Session error", errmsg);

#if defined SEQ66_PORTMIDI_SUPPORT
    const char * pmerrmsg = pm_log_buffer();
#else
    const char * pmerrmsg = errmsg.c_str();
#endif

    if (not_nullptr(pmerrmsg) && std::strlen(pmerrmsg) > 0)
    {
        std::string path = seq66::rc().config_filespec("seq66.log");
        errmsg += "\n";
        errmsg += std::string(pmerrmsg);
        (void) seq66::file_append_log(path, errmsg);
    }
}

/**
 *  Refactored so that the basic NSM session can be set up before launch(), as
 *  per NSM rules.
 */

bool
smanager::create (int argc, char * argv [])
{
    bool result = main_settings(argc, argv);
    if (result)
    {
        if (create_session(argc, argv))     /* get path, client ID, etc.    */
        {
#if defined SEQ66_PLATFORM_DEBUG_TEST_NSM   /* enable to trouble-shoot      */
            warnprint("DEBUGGING, WRITING CONFIGS to ~/sessiontest");
            (void) create_project(argc, argv, "~/sessiontest");
#else
            std::string homedir = manager_path();
            if (homedir == "None")
                homedir = rc().home_config_directory();

            file_message("Session manager path", homedir);
            (void) create_project(argc, argv, homedir);
#endif
        }
        result = create_performer();        /* fails if performer not made  */
        if (result)
            result = open_playlist();

        if (result && rc().load_most_recent())
        {
            /*
             * Get full path to the most recently-opened or imported file.
             * What if smanager::open_midi_file() has already been
             * called via the command-line? Then skip this step.
             */

            if (midi_filename().empty())
            {
                std::string midifname = rc().recent_file(0, false);
                if (! midifname.empty())
                {
                    std::string errmsg;
                    std::string tmp = open_midi_file(midifname, errmsg);
                    if (tmp.empty())
                        file_error(errmsg, midifname);
                    else
                        file_message("Opened", tmp);
                }
            }
        }

        if (result)
            result = open_note_mapper();

        if (result)
        {
            std::string fname = midi_filename();
            if (fname.empty())
            {
                if (is_debug())
                    warnprint("MIDI filename empty, nothing to open");
            }
            else
            {
                std::string errormessage;
                std::string tmp = open_midi_file(fname, errormessage);

                /*
                 * We don't have a window at this time, and should save the
                 * message for display later.  For now, we write to the console.
                 */

                if (tmp.empty())
                    file_error(errormessage, fname);
                else
                    file_message("Opened", tmp);
            }
            result = create_window();
            if (result)
            {
                error_handling();

                /*
                 * We run() the window, get the exit status, and close the
                 * session in the main() function of the application.
                 */
            }
            else
            {
                std::string msg;
                result = close_session(msg, false);
            }
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

}           // namespace seq66

/*
 * smanager.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


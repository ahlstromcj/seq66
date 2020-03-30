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
 * \updates       2020-03-24
 * \license       GNU GPLv2 or above
 *
 *  The process:
 *
 *      1. Call main_settings(argc, argv).  It sets defaults, does some parsing
 *         of command-line options and files.  It saves the MIDI file-name, if
 *         provided.
 *      2. Call create_performer(), which could delete an existing performer.
 *         It then launches the performer.  Save the unique-pointer.
 *      3. Call open_playlist().  It will open it, if specified and possible.
 *      4. If the MIDI file-name is set, open it via a call to open_midi_file().
 *      5. If a user-interface is needed, create a unique-pointer to it, then
 *         show it.  This will remove any previous pointer.  The function is
 *         virtual, create_window().
 *
 *  PortMidi:
 *
 *      PortMidi handling shall be done by the main() of the application. ???
 */

#include "cfg/cmdlineopts.hpp"          /* command-line functions           */
#include "cfg/settings.hpp"             /* seq66::usr() and seq66::rc()     */
#include "play/performer.hpp"           /* seq66::performer                 */
#include "unix/daemonize.hpp"           /* seq66::reroute_stdio()           */
#include "sessions/smanager.hpp"        /* seq66::smanager()                */
#include "util/basic_macros.hpp"        /* seq66::msgprintf()               */
#include "util/filefunctions.hpp"       /* seq66::file_accessible()         */

#if defined SEQ66_LASH_SUPPORT_NO_LASH_HERE
#include "lash/lash.hpp"                /* seq66::lash_driver functions     */
#endif

#if defined SEQ66_PORTMIDI_SUPPORT
#include "portmidi.h"        /*  Pm_error_present(), Pm_hosterror_message()  */
#endif

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *
 */

smanager::smanager () :
    m_perf_pointer      (),
    m_midi_filename     (),
    m_extant_errmsg     ("unspecified error"),
    m_extant_msg_active (false)
{
    m_perf_pointer = create_performer();
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
 *      Returns EXIT_SUCCESS (0) or EXIT_FAILURE, depending on the status of
 *      the run.
 */

bool
smanager::main_settings (int argc, char * argv [])
{
    bool result = true;                     /* EXIT_FAILURE                 */
    seq66::set_app_name(SEQ66_APP_NAME);    /* "qseq66" by default          */
    seq66::rc().set_defaults();             /* start out with normal values */
    seq66::usr().set_defaults();            /* start out with normal values */

    /*
     * -o log=file.ext early
     */

    (void) seq66::cmdlineopts::parse_log_option(argc, argv);

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

    (void) seq66::cmdlineopts::parse_command_line_options(argc, argv);
    bool is_help = seq66::cmdlineopts::help_check(argc, argv);
    int optionindex = -1;
    if (! is_help)
    {
        /*
         *  If parsing fails, report it and disable usage of the application
         *  and saving bad garbage out when exiting.  Still must launch,
         *  otherwise a segfault occurs via dependencies in the qsmainwnd.
         */

        std::string errmessage;                     /* just in case!        */
        result = seq66::cmdlineopts::parse_options_files(errmessage, argc, argv);
        if (! result)
        {
            errprintf("parse_options_files(): %s\n", errmessage.c_str());
        }
        optionindex = seq66::cmdlineopts::parse_command_line_options(argc, argv);
        if (seq66::cmdlineopts::parse_o_options(argc, argv))
        {
            /**
             * The user may have specified -o options that are also set up in
             * the "usr" file.  The command line needs to take precedence.  The
             * "log" option is processed early in the startup sequence.  These
             * same settings are made in the cmdlineopts module.
             */

            std::string logfile = seq66::usr().option_logfile();
            if (seq66::usr().option_use_logfile() && ! logfile.empty())
                (void) seq66::reroute_stdio(logfile);
        }
        if (result)
        {
            if (optionindex < argc)                 /* MIDI filename given? */
            {
                std::string fname = argv[optionindex];
                if (seq66::file_accessible(fname))
                {
                    m_midi_filename = fname;
                    // set_error_message();            /* clears the message   */
                }
                else
                {
                    char temp[512];
                    (void) snprintf
                    (
                        temp, sizeof temp,
                        "? MIDI file not found: %s", fname.c_str()
                    );
                    printf("%s\n", temp);
                    set_error_message(temp);        /* raises the message   */
                }
            }
        }
    }
    return result;
}

/**
 *
 */

std::unique_ptr<seq66::performer>
smanager::create_performer ()
{
    std::unique_ptr<seq66::performer> result;
    result.reset
    (
        new seq66::performer
        (
            SEQ66_USE_DEFAULT_PPQN,
            seq66::usr().mainwnd_rows(),
            seq66::usr().mainwnd_cols()
        )
    );
    if (bool(result) && seq66::rc().verbose())          /* trouble-shooting */
    {
        seq66::rc().key_controls().show();
        seq66::rc().midi_controls().show();
        seq66::rc().mute_groups().show();
    }

    /*
     * This call must occur before creating the application main window.
     * Otherwise, seq66 will not register with LASH (if enabled) in a
     * timely fashion.  Also, we always have to launch, even if an error
     * occurred, to avoid a segfault and show at least a minimal message.
     * LASH support is now back in Seq66.
     */

    if (result)
    {
        /*
         * If this is the first time Seq66 is being run, this will always
         * fail, so ignore the return code.
         */

        bool ok = result->get_settings(seq66::rc());
        if (ok)
        {
            ok = result->launch(seq66::usr().midi_ppqn());
            if (! ok)
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
 *  Opens a playlist, if specified.
 *
 * \return
 *      Returns true if a playlist was specified and successfully opened.
 */

bool
smanager::open_playlist ()
{
    bool result = false;
    std::string errmsg = "unspecified error";
    std::string playlistname = seq66::rc().playlist_filespec();
    if (seq66::rc().playlist_active() && ! playlistname.empty())
    {
        result = perf()->open_playlist(playlistname, seq66::rc().verbose());
        if (result)
        {
            result = perf()->open_current_song();   /* p.playlist_test()    */
        }
        else
        {
            set_error_message(perf()->playlist_error_message());
            result = true;                          /* avoid early exit     */
        }
    }
    return result;
}

/**
 *  Encapsulates opening the MIDI file, if specified (on the command-line).
 */

std::string
smanager::open_midi_file (const std::string & fname)
{
    std::string midifname;                          /* start out blank      */
    std::string errmsg = "unspecified error";
    bool result = false;
    if (seq66::file_accessible(fname))
    {
        int pp = perf()->ppqn();
        midifname = fname;
        result = perf()->read_midi_file(fname, pp, errmsg);
        if (result)
        {
            std::string infomsg = "PPQN set to ";
            infomsg += std::to_string(pp);
            (void) seq66::info_message(infomsg);
        }
        else
        {
            (void) seq66::error_message(errmsg);
        }
    }
    return midifname;
}

/**
 *
 */

bool
smanager::create_session ()
{
#if defined SEQ66_PLATFORM_LINUX
#if defined SEQ66_LASH_SUPPORT_NO_LASH_HERE
    if (seq66::rc().lash_support())
        seq66::create_lash_driver(p, argc, argv);
    else
#endif
        seq66::session_setup();
#endif
    return false;
}

bool
smanager::close_session ()
{
    perf()->finish();                           /* tear down performer      */
    perf()->put_settings(seq66::rc());          /* copy latest settings     */
    if (true)                                   /* TODO */
    {
        (void) seq66::cmdlineopts::write_options_files("erroneous");
        if (error_active())
        {
            errprint(error_message().c_str());
        }
//      else if (! errmessage.empty())
//      {
//          errprint(errmessage.c_str());
//      }
    }
    else
    {
        if (seq66::rc().auto_option_save())
            (void) seq66::cmdlineopts::write_options_files();
        else
            printf("[auto-option-save off, not saving config files]\n");
    }

#if defined SEQ66_LASH_SUPPORT_NO_LASH_HERE
    if (seq66::rc().lash_support())
        seq66::delete_lash_driver();            /* deleted only exists      */
#endif

    return true;
}

/**
 *
 */

bool
smanager::create_window ()
{
    if (seq66::rc().verbose())
        perf()->show_patterns();

    return false;
}


/*
 * Having this here after creating the main window may cause issue
 * #100, where ladish doesn't see seq66's ports in time.
 *
 *  perf()->launch(seq66::usr().midi_ppqn());
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
smanager::show_message (const std::string & msg)
{
    seq66::info_message(msg);
}

/**
 *
 */

void
smanager::show_error (const std::string & msg)
{
    seq66::error_message(msg);
}

#if defined SEQ66_PORTMIDI_SUPPORT

/**
 *
 * \return
 *      Returns true if there is an error.  In this case, the caller should show
 *      the error message.
 */

bool
smanager::portmidi_error_check () const
{
    bool result = Pm_error_present();
    if (result)
    {
        set_error_message(std::string(Pm_hosterror_message()));
        // seq66_window->show_message_box(errmsg);
    }
    return result;
}

#endif

}           // namespace seq66

/*
 * smanager.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


/*
 *  This file is part of seq24/sequencer64.
 *
 *  seq24 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  seq24 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq24; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          main_impl.cpp
 *
 *  This module declares/defines the main module for the application.
 *
 * \library       main_impl application
 * \author        Chris Ahlstrom
 * \date          2019-01-19
 * \updates       2019-03-29
 * \license       GNU GPLv2 or above
 *
 *  This application is seq66 without a GUI, control must be done via MIDI.
 */

#include <stdio.h>                      /* can potentially reroute stdio    */
#include "seq66_features.hpp"           /* environment and build features   */

#if defined SEQ66_DAEMON_SUPPORT
#include <signal.h>
#include <unistd.h>
#include "daemonize.hpp"                /* seq66::daemonize()               */
#endif                                  /* SEQ66_DAEMON_SUPPORT             */

#include "cfg/cmdlineopts.hpp"          /* command-line functions           */
#include "cfg/settings.hpp"             /* seq66::usr() and seq66::rc()     */
#include "main_impl.hpp"                /* TODO                             */
#include "midi/midifile.hpp"            /* seq66::read_midi_file()          */
#include "play/performer.hpp"           /* seq66::perform, the main object  */

#if defined SEQ66_DAEMON_SUPPORT

/**
 *  Provides a static variable that is true while the program is running.
 */

static bool s_seq66cli_running = false;

/**
 *  Provides a signal handler for exiting the application gracefully.
 */

static void
seq66_signal_handler (int signalnumber)
{
    if (signalnumber == SIGINT)
        s_seq66cli_running = false;
    else if (signalnumber == SIGTERM)
        s_seq66cli_running = false;
}

#endif                                  /* SEQ66_DAEMON_SUPPORT             */

/**
 *  The standard C/C++ entry point to this application.  This first thing
 *  this function does is scan the argument vector and strip off all
 *  parameters known to GTK+.
 *
 *  The next thing is to set the various settings defaults, and then try to
 *  read the "user" and "rc" configuration files, in that order.  There are
 *  currently no options to change the names of those files.  If we add
 *  that code, we'll move the parsing code to where the configuration
 *  file-names are changed from the command-line.
 *
 *  The last thing is to override any other settings via the command-line
 *  parameters.
 *
 * Daemon support:
 *
 *  Apart from the usual daemon stuff, we need to handle the following issues:
 *
 *      -#  Detecting the need for daemonizing and doing it before all the
 *          normal configuration work is performed.
 *      -#  Loading the initial MIDI file.  Does this filename need to be
 *          grabbed before forking?  No, local variables are passed to the new
 *          process.
 *      -#  Setting the current-working directory.  Should it be grabbed from
 *          the "rc" file?
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

int
main_impl
(
    int argc, char * argv [],
    const std::string & appname
)
{
#if defined SEQ66_DAEMON_SUPPORT
    uint32_t usermask = 0;                  /* used only in daemonization   */
#endif

    bool stdio_rerouted = false;            /* used only in log-file option */
    seq66::set_app_name(appname);
    seq66::rc().set_defaults();             /* start out with normal values */
    seq66::usr().set_defaults();            /* start out with normal values */
    if (seq66::cmdlineopts::parse_o_options(argc, argv))
    {
        std::string logfile = seq66::usr().option_logfile();
        if (seq66::usr().option_use_logfile() && ! logfile.empty())
        {
#if defined SEQ66_DAEMON_SUPPORT
            (void) seq66::reroute_stdio(logfile);
            stdio_rerouted = true;
#endif
        }

#if defined SEQ66_DAEMON_SUPPORT
        if (seq66::usr().option_daemonize())
        {
            printf("Forking to background...\n");
            usermask = seq66::daemonize(seq66::seq_app_name(), ".");
        }
#endif
    }

    /**
     * We currently have a issue where the mastermidibus created by
     * the performer object gets the default PPQN value, because the "user"
     * configuration file has not been read at that point.  See the
     * perform::launch() function.
     */

    seq66::performer p;
    (void) seq66::cmdlineopts::parse_command_line_options(argc, argv);
    bool is_help = seq66::cmdlineopts::help_check(argc, argv);
    bool ok = true;
    int optionindex = -1;
    if (! stdio_rerouted)                           /* not done already?    */
    {
#if defined SEQ66_DAEMON_SUPPORT
        std::string logfile = seq66::usr().option_logfile();
        if (seq66::usr().option_use_logfile() && ! logfile.empty())
            (void) seq66::reroute_stdio(logfile);
#endif
    }
    if (! is_help)
    {
        std::string extant_errmsg = "unspecified error";
        bool extant_msg_active = false;             /* a kludge             */

        /*
         *  If parsing fails, report it and disable usage of the application
         *  and saving bad garbage out when exiting.  Still must launch,
         *  otherwise a segfault occurs via dependencies in the mainwnd.
         */

        std::string errmessage;                     /* just in case!        */
        ok = seq66::cmdlineopts::parse_options_files(errmessage, argc, argv);
        optionindex = seq66::cmdlineopts::parse_command_line_options(argc, argv);
        p.launch(seq66::usr().midi_ppqn());         /* set up performance   */
        if (ok)
        {
            if (! seq66::usr().option_daemonize())
            {
                /*
                 * Show information on the busses to help the user diagnose
                 * any configuration issues.  Still has ISSUES!
                 */

                p.print_busses();
            }
            std::string playlistname = seq66::rc().playlist_filespec();
            if (seq66::rc().playlist_active() && ! playlistname.empty())
            {
                ok = p.open_playlist(playlistname, seq66::rc().verbose());
                if (ok)
                {
                    ok = p.open_current_song();
                }
                else
                {
                    extant_errmsg = p.playlist_error_message();
                    extant_msg_active = true;
                }
            }
            if (ok && optionindex < argc)           /* MIDI filename given?   */
            {
                int ppqn = 0;
                std::string fn = argv[optionindex];
                ok = read_midi_file(p, fn, ppqn, extant_errmsg);
                if (! ok)
                    extant_msg_active = true;
            }
            if (ok)
            {

#if defined SEQ66_DAEMON_SUPPORT

                /*
                 * signal() is deprecated, but sigaction() is too
                 * complicated.
                 */

                if (signal(SIGINT, seq66_signal_handler) != SIG_ERR)
                {
                    if (signal(SIGTERM, seq66_signal_handler) != SIG_ERR)
                    {
                        s_seq66cli_running = true;
                        while (s_seq66cli_running)
                            usleep(1000000);
                    }
                    else
                        printf("? Cannot set SIGTERM handler\n");
                }
                else
                    printf("? Cannot set SIGINT handler\n");
#endif

                p.finish();                         /* tear down performer  */
                if (seq66::rc().auto_option_save())
                {
                    if (ok)                         /* don't write bad data */
                        ok = seq66::cmdlineopts::write_options_files();
                }
                else
                    printf("[auto-option-save off, not saving config files]\n");
            }
        }
        else
        {
            if (extant_msg_active)
                errprint(extant_errmsg.c_str());
            else
                errprint(errmessage.c_str());

            (void) seq66::cmdlineopts::write_options_files("erroneous");
        }

#if defined SEQ66_DAEMON_SUPPORT
        if (seq66::usr().option_daemonize())
            seq66::undaemonize(usermask);
#endif
    }
    return ok ? EXIT_SUCCESS : EXIT_FAILURE ;
}

/*
 * main_impl.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


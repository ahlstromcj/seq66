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
 * \file          seq66qt5.cpp
 *
 *  This module declares/defines the main module for the JACK/ALSA "qt5"
 *  implementation of this application.
 *
 * \library       seq66qt5 application
 * \author        Chris Ahlstrom
 * \date          2017-09-05
 * \updates       2019-07-25
 * \license       GNU GPLv2 or above
 *
 *  This is an attempt to change from the hoary old (or, as H.P. Lovecraft
 *  would style it, "eldritch") gtkmm-2.4 implementation of Seq66.
 */

#include <QApplication>                 /* QApplication etc.                */
#include <memory>                       /* std::unique_ptr<>                */

#include "cfg/cmdlineopts.hpp"          /* command-line functions           */
#include "cfg/settings.hpp"             /* seq66::usr() and seq66::rc()     */
#include "play/performer.hpp"           /* seq66::performer                 */
#include "unix/daemonize.hpp"           /* seqg4::reroute_stdio()           */
#include "util/basic_macros.hpp"        /* seq66::msgprintf()               */
#include "util/filefunctions.hpp"       /* seq66::file_accessible()         */
#include "qsmainwnd.hpp"                /* the main window of seq66qt5      */

#if defined SEQ66_PORTMIDI_SUPPORT
#include "portmidi.h"        /*  Pm_error_present(), Pm_hosterror_message()  */
#endif

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

int
main (int argc, char * argv [])
{
    QApplication app(argc, argv);           /* main application object      */
    int exit_status = EXIT_SUCCESS;         /* EXIT_FAILURE                 */
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
    bool ok = true;
    int optionindex = -1;
    if (! is_help)
    {
        /*
         *  If parsing fails, report it and disable usage of the application
         *  and saving bad garbage out when exiting.  Still must launch,
         *  otherwise a segfault occurs via dependencies in the qsmainwnd.
         */

        std::string errmessage;                     /* just in case!        */
        ok = seq66::cmdlineopts::parse_options_files(errmessage, argc, argv);
        if (! ok)
        {
            errprintf("parse_options_files(): %s\n", errmessage.c_str());
        }
        optionindex = seq66::cmdlineopts::parse_command_line_options(argc, argv);
        if (seq66::cmdlineopts::parse_o_options(argc, argv))
        {
            /**
             * The user may have specified the "wid" or other -o options that
             * are also set up in the "usr" file.  The command line needs to
             * take precedence.  The "log" option is processed early in the
             * startup sequence.  These same settings are made in the
             * cmdlineopts module.
             *
             * Now handled via optind incrementing:     ++optionindex;
             */

            std::string logfile = seq66::usr().option_logfile();
            if (seq66::usr().option_use_logfile() && ! logfile.empty())
                (void) seq66::reroute_stdio(logfile);
        }

        seq66::performer p                      /* main performance object      */
        (
            SEQ66_USE_DEFAULT_PPQN,
            seq66::usr().mainwnd_rows(),
            seq66::usr().mainwnd_cols()
        );

        /*
         * For testing.
         */

        if (seq66::rc().verbose())
        {
            seq66::rc().key_controls().show();
            seq66::rc().midi_controls().show();
            seq66::rc().mute_groups().show();
        }

        /*
         * Issue #100, moved this call to before creating the qsmainwnd.
         * Otherwise, seq66 will not register with LASH (if enabled) in a
         * timely fashion.  Also, we always have to launch, even if an error
         * occurred, to avoid a segfault and show at least a minimal message.
         *
         * Note:  LASH support not present in Sequencer66.
         */

        if (ok)
        {
            /*
             * If this is the first time Seq66 is being run, this will always
             * fail, so ignore the return code.
             */

            (void) p.get_settings(seq66::rc());
            ok = p.launch(seq66::usr().midi_ppqn());
            if (! ok)
                errprint("performer::launch() failed");
        }

        std::string midifname;                      /* start out blank      */
        std::string extant_errmsg = "unspecified error";
        bool extant_msg_active = false;             /* a kludge             */
        std::string errmsg = "unspecified error";
        if (ok)
        {
            std::string playlistname = seq66::rc().playlist_filespec();
            if (seq66::rc().playlist_active() && ! playlistname.empty())
            {
                ok = p.open_playlist(playlistname, seq66::rc().verbose());
                if (ok)
                {
                    ok = p.open_current_song();     /* p.playlist_test()    */
                }
                else
                {
                    extant_errmsg = p.playlist_error_message();
                    extant_msg_active = true;
                    ok = true;                      /* avoid early exit     */
                }
            }
            if (optionindex < argc)                 /* MIDI filename given? */
            {
                std::string fname = argv[optionindex];
                if (seq66::file_accessible(fname))
                {
                    int pp = p.ppqn();
                    midifname = fname;
                    ok = p.read_midi_file(fname, pp, errmsg);
                    if (ok)
                    {
                        std::string infomsg = "PPQN set to ";
                        infomsg += std::to_string(pp);
                        ok = seq66::info_message(infomsg);
                    }
                }
                else
                {
                    char temp[256];
                    (void) snprintf
                    (
                        temp, sizeof temp,
                        "? MIDI file not found: %s", fname.c_str()
                    );
                    printf("%s\n", temp);
                    ok = false;
                    errmsg = temp;
                }
            }
        }
        std::unique_ptr<seq66::qsmainwnd> seq66_window;
        if (ok)
        {
            /*
             * Push the qsmainwnd window onto the stack.  Also be sure to pass
             * along the PPQN value, which might be different than the default
             * (192), and affects some of the child objects of qsmainwnd.
             */

            seq66_window.reset
            (
                new seq66::qsmainwnd(p, midifname, seq66::usr().midi_ppqn())
            );
            seq66_window->show();
            if (seq66::rc().verbose())
                p.show_patterns();
        }

        /*
         * Having this here after creating the main window may cause issue
         * #100, where ladish doesn't see seq66's ports in time.
         *
         *  p.launch(seq66::usr().midi_ppqn());
         *
         * We also check for any "fatal" PortMidi errors, so we can display
         * them.  But we still want to keep going, in order to at least
         * generate the log-files and configuration files to
         * C:/Users/me/AppData/Local/seq66 or ~/.config/seq66.
         */

#if defined SEQ66_PORTMIDI_SUPPORT
        if (ok)
        {
            if (Pm_error_present())
            {
                ok = false;
                errmsg = std::string(Pm_hosterror_message());
                seq66_window->show_message_box(errmsg);
            }
        }
#endif


        if (ok)
        {
            if (extant_msg_active)
                seq66_window->show_message_box(extant_errmsg);
            else
                seq66_window->show_message_box(errmessage);

            exit_status = app.exec();           /* run main window loop     */
            p.finish();                         /* tear down performerer    */
            p.put_settings(seq66::rc());        /* copy latest settings     */
            if (seq66::rc().auto_option_save())
                (void) seq66::cmdlineopts::write_options_files();
            else
                printf("[auto-option-save off, not saving config files]\n");
        }
        else
        {
            (void) seq66::cmdlineopts::write_options_files("erroneous");
            if (extant_msg_active)
            {
                errprint(extant_errmsg.c_str());
            }
            else
            {
                errprint(errmessage.c_str());
            }
        }
    }
    return exit_status;
}

/*
 * seq66qt5.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


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
 * \file          seq66rtcli.cpp
 *
 *  This module declares/defines the main module for the application.
 *
 * \library       seq66rtcli application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2020-02-09
 * \updates       2023-03-19
 * \license       GNU GPLv2 or above
 *
 *  This application is seq66 without a GUI, control must be done via MIDI.
 */

#include "cfg/cmdlineopts.hpp"          /* command-line functions           */
#include "cfg/settings.hpp"             /* seq66::usr() and seq66::rc()     */
#include "os/daemonize.hpp"             /* seq66::daemonize()               */
#include "play/performer.hpp"           /* seq66::perform, the main object  */
#include "sessions/clinsmanager.hpp"    /* an seq66::smanager for CLI use   */

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
main (int argc, char * argv [])
{
    int exit_status = EXIT_SUCCESS;         /* EXIT_FAILURE might happen    */
    bool success = true;
    seq66::usr().app_is_headless(true);
    seq66::set_app_cli(true);               /* used in smanager             */

    /*
     * Move the daemonize() call to up here so that the configuration
     * files will be reread.  Note that currently the 'usr' option
     * is not read.  We will need to make a special function to do that.
     */

#if defined SEQ66_PLATFORM_LINUX
    mode_t usermask = 0;                    /* used in daemonization        */
    int flags = d_flag_no_to_all;           /* see daemonize.hpp            */
    if (seq66::usr().option_daemonize())
    {
        seq66::set_app_type("daemon");
        seq66::set_app_name("seq66daemon");
        infoprint("Forking to background...");

        int rc = seq66::daemonize(usermask, seq66::seq_app_name(), flags, ".");
        if (rc == EXIT_SUCCESS)
            exit(EXIT_SUCCESS);
    }
#endif
    if (seq66::cmdlineopts::parse_o_options(argc, argv))
    {
        std::string logfile = seq66::usr().option_logfile();
        if (seq66::usr().option_use_logfile() && ! logfile.empty())
            (void) seq66::reroute_stdio(logfile);
    }

    seq66::clinsmanager sm;
    success = sm.create(argc, argv);
    if (success)
    {

        std::string msg;
        bool ok = sm.run();
        exit_status = ok ? EXIT_SUCCESS : EXIT_FAILURE ;
        (void) sm.close_session(msg, ok);
    }
    else
        exit_status = EXIT_FAILURE;

#if defined SEQ66_PLATFORM_LINUX
    if (seq66::usr().option_daemonize())
        seq66::undaemonize(usermask);
#endif

    return exit_status;
}

/*
 * seq66rtcli.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


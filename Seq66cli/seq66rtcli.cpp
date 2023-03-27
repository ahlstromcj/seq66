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
 * \updates       2023-03-27
 * \license       GNU GPLv2 or above
 *
 *  This application is seq66 without a GUI, control must be done via MIDI.
 *  There are a few kinds of life-cycles for this application:
 *
 *      -#  On the command-line, "--option daemonize" is provided (the
 *          option "--option log=filename.log" can also be provided.).
 *          This causes the daemonize (and log) options to be immediately
 *          written to the default 'usr' file (seq66cli.usr). Then
 *          the application simply exits.
 *      -#  No daemonization option provided on the command-line.
 *          -   If step 1 has not been done, then the 'usr' daemonization
 *              is false, and the application runs as a normal console
 *              application. To keep the user from seeing console output,
 *              the application can be associated with a window manager
 *              application icon, menu entry, or hot-key.
 *          -   If the 'usr' file specifies daemonization, then the
 *              application forks itself and the child runs in the background
 *              with no terminal.
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
 *          -   Read "--option daemonize" from the command line.
 *          -   Scan the 'usr' file for just the daemonize option.
 *      -#  Read the configuratiopn options.
 *      -#  Loading the initial MIDI file.  Does this filename need to be
 *          grabbed before forking?  No, local variables are passed to the new
 *          process.
 *      -#  Setting the current-working directory.  Should it be grabbed from
 *          the 'rc' file?
 *
 *  We moved the daemonize() call to up here so that the configuration files
 *  will be reread.  Note that currently the 'usr' option is not read.  We
 *  will need to make a special function to do that.
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
    int exit_status = EXIT_SUCCESS;             /* EXIT_FAILURE might occur */
    bool success = true;
#if defined SEQ66_PLATFORM_LINUX
    mode_t usermask = 0;                        /* used in un-daemonization */
#endif
    seq66::usr().app_is_headless(true);
    seq66::set_app_cli(true);                   /* used in smanager         */
    seq66::set_app_name("seq66cli");            /* also done in smanager!!  */
    (void) seq66::cmdlineopts::parse_o_options(argc, argv);
    if (! seq66::usr().save_daemonize())
    {
#if defined SEQ66_PLATFORM_LINUX
        bool startdaemon = false;
        std::string logfile;
        std::string appname = seq66::seq_app_name();
        seq66::rc().set_config_files(appname);
        (void) seq66::cmdlineopts::parse_daemonization
        (
            startdaemon, logfile                /* two side-effects         */
        );
        if (startdaemon)
        {
            int flags = d_flags_seq66cli;       /* see daemonize.hpp        */
            seq66::set_app_type("daemon");
            seq66::set_app_name("seq66daemon");
            warnprint("Forking to background...");

            daemonization rc = seq66::daemonize(usermask, appname, flags, ".");
            if (rc == daemonization::parent)
            {
                warnprint("Parent exits with success...");
                exit(EXIT_SUCCESS);
            }
            else if (rc == daemonization::failure)
            {
                errprint("Parent exits with failure...");
                exit(EXIT_FAILURE);
            }
            warnprint("Child continues normal operations...");
        }
#endif
        std::string destination = logfile.empty() ? "/dev/null" : logfile ;
        (void) seq66::reroute_stdio(logfile);
    }

    seq66::clinsmanager sm;
    success = sm.create(argc, argv);
    if (success)
    {
        if (seq66::usr().save_daemonize())
        {
            if (seq66::cmdlineopts::write_usr_file())
                warnprint("Daemon setup: saved 'usr' settings, exiting...");
        }
        else
        {
            std::string msg;
            bool ok = sm.run();
            exit_status = ok ? EXIT_SUCCESS : EXIT_FAILURE ;
            (void) sm.close_session(msg, ok);
            infoprint(msg);
        }
    }
    else
        exit_status = EXIT_FAILURE;

#if defined SEQ66_PLATFORM_LINUX
    if (seq66::usr().option_daemonize() && ! seq66::usr().save_daemonize())
    {
        seq66::undaemonize(usermask);
        if (exit_status == EXIT_FAILURE)
            warnprint("Child exits with failure...");
        else
            warnprint("Child does normal exit...");
    }
#endif

    return exit_status;
}

/*
 * seq66rtcli.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


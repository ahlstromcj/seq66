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
 * \file          seq66qt5.cpp
 *
 *  This module declares/defines the main module for the JACK/ALSA "qt5"
 *  implementation of this application.
 *
 * \library       seq66qt5 application
 * \author        Chris Ahlstrom
 * \date          2017-09-05
 * \updates       2023-12-05
 * \license       GNU GPLv2 or above
 *
 *  This is an attempt to change from the hoary old (or, as H.P. Lovecraft
 *  would style it, "eldritch") gtkmm-2.4 implementation of Seq66.
 */

#include <QApplication>                 /* QApplication etc.                */

#include "seq66_features.hpp"           /* seq66::set_app_path()            */
#include "qt5nsmanager.hpp"             /* an seq66::smanager for Qt 5      */
#include "os/daemonize.hpp"             /* seq66::session_close(), etc.     */
#include "os/timing.hpp"                /* seq66::millisleep()              */

#undef  SEQ66_LOCALE_SUPPORT            /* using --locale instead           */
#undef  SEQ66_TRANSLATOR_SUPPORT        /* we're not ready for this at all  */
#undef  USE_RING_BUFFER_TEST            /* enable to check ring_buffer ops  */

/*
 * Without the QCoreApplication::setSetuidAllow() call, the application dumps
 * core if setuid root in the install. However, This yields this message many
 * many times when run as setuid root; "QCommonStyle::drawComplexControl:
 * Control 1 not handled" Not sure what is up with that. The result is
 * that the user interface is flat and cramped! In any case, Qt warns not
 * to use setuid root because Qt has "a large attack surface" :-D.
 */

#undef  SEQ66_SETUID_SUPPORT            /* Qt aborts with setuid otherwise  */

#if defined USE_RING_BUFFER_TEST        /* requires a debug build           */
#include "util/ring_buffer.hpp"
#endif

/**
 *  Let's give time for the existing connections to go away, since it
 *  seems sometimes new port settings do not work.
 */

static const int sc_sleep_time_ms = 250;

/**
 *  The standard C/C++ entry point to this application.  The first thing is to
 *  set the various settings defaults, and then try to read the "user" and
 *  "rc" configuration files, in that order.  There are currently no options
 *  to change the names of those files.  If we add that code, we'll move the
 *  parsing code to where the configuration file-names are changed from the
 *  command-line.  The last thing is to override any other settings via the
 *  command-line parameters.
 *
 *  We check for any "fatal" PortMidi errors, so we can display them.  But we
 *  still want to keep going, in order to at least generate the log-files and
 *  "erroneous" configuration files to C:/Users/me/AppData/Local/seq66 or
 *  $HOME/.config/seq66.
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
#if defined SEQ66_SETUID_SUPPORT
    QApplication::setSetuidAllowed(true);
#endif

    QApplication app(argc, argv);           /* main application object      */
#if defined USE_NEW_CODE
    seq66::smanager::app_info(argv[0], true);
#else
    seq66::set_app_path(argv[0]);           /* log for future usage         */
#endif

#if defined USE_RING_BUFFER_TEST && defined SEQ66_PLATFORM_DEBUG
    if (! seq66::run_ring_test())
    {
        printf("ring_buffer test FAILED\n");
        exit(1);
    }
    else
        exit(0);
#endif

#if defined SEQ66_LOCALE_SUPPORT

    /*
     * No longer used, it conflicts with getopt processing.  Instead, see
     * the --locale option in the cmdlineopts module.
     */

    Q_FOREACH(QString a, app.arguments())
    {
        const static QString s_locale_arg = "--locale:";
        if (a.startsWith(s_locale_arg))
        {
           QLocale::setDefault(QLocale(a.mid(sizeof(s_locale_arg) - 1)));
           break;
        }
    }
#endif

#if defined SEQ66_TRANSLATOR_SUPPORT
    QTranslator app_translator;
    if (! app_translator.load ("seq66_" + QLocale().name (), app_tr_dir))
    {
         qWarning
         (
            "Can't load app translator file for locale %s from %s",
            qPrintable(QLocale().name()),
            app_tr_dir.toLocal8Bit().data()
         );
    }
    else
         app.installTranslator(&app_translator);
#endif

    int exit_status = EXIT_SUCCESS;                 /* versus EXIT_FAILURE  */
    for (;;)
    {
        seq66::qt5nsmanager sm(app);
        bool result = sm.create(argc, argv);
        if (result)
        {
            std::string msg;
            bool result = sm.run();
            exit_status = result ? EXIT_SUCCESS : EXIT_FAILURE ;
            (void) sm.close_session(msg, result);
            if (! result)
                break;                              /* stop infinite loop   */

            if (seq66::session_close())
            {
                seq66::session_message("Closing session");
                break;                              /* stop infinite loop   */
            }
            if (seq66::session_restart())
            {
                seq66::millisleep(sc_sleep_time_ms);
                seq66::session_message("Reloading session");
                seq66::signal_end_restart();
            }
            else
                break;                              /* stop infinite loop   */
        }
        else
        {
            exit_status = EXIT_FAILURE;             /* --help or error      */
            break;
        }
    }
    return exit_status;
}

/*
 * seq66qt5.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


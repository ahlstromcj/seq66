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
 * \updates       2020-08-26
 * \license       GNU GPLv2 or above
 *
 *  This is an attempt to change from the hoary old (or, as H.P. Lovecraft
 *  would style it, "eldritch") gtkmm-2.4 implementation of Seq66.
 */

#include <QApplication>                 /* QApplication etc.                */

#include "cfg/settings.hpp"             /* seq66::usr() and seq66::rc()     */
#include "qt5nsmanager.hpp"             /* an seq66::smanager for Qt 5      */

#if defined SEQ66_PORTMIDI_SUPPORT
#include "portmidi.h"                   /* Pm_error_present(), ...          */
#include "util/filefunctions.hpp"       /* seq66::file_append_log()         */
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
    QApplication app(argc, argv);           /* main application object      */
    int exit_status = EXIT_SUCCESS;         /* EXIT_FAILURE                 */
    seq66::qt5nsmanager sm(app);            /* NEW, currently just a helper */
    bool result = sm.create(argc, argv);
    if (result)
    {
        exit_status = sm.run() ? EXIT_SUCCESS : EXIT_FAILURE ;
        result = sm.close_session();
    }
    else
        exit_status = EXIT_FAILURE;

    return exit_status;
}

/*
 * seq66qt5.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


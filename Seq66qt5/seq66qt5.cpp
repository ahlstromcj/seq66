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
 * \updates       2020-07-05
 * \license       GNU GPLv2 or above
 *
 *  This is an attempt to change from the hoary old (or, as H.P. Lovecraft
 *  would style it, "eldritch") gtkmm-2.4 implementation of Seq66.
 */

#include <QApplication>                 /* QApplication etc.                */

#include "cfg/settings.hpp"             /* seq66::usr() and seq66::rc()     */
#include "qt5nsmanager.hpp"             /* an seq66::smanager for Qt 5      */

#if defined SEQ66_LASH_SUPPORT
#include "lash/lash.hpp"                /* seq66::lash_driver functions     */
#endif

#if defined SEQ66_PORTMIDI_SUPPORT
#include "portmidi.h"                   /* Pm_error_present(), ...          */
#include "pmerrmm.h"                    /* pm_log_buffer()           */
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
    std::string errormessage;               /* just in case                 */
    seq66::qt5nsmanager sm(app);            /* NEW, currently just a helper */
    bool result = sm.main_settings(argc, argv); // bool ok = true;
    if (result)
    {
        result = sm.create_performer();     /* fails if performer not made  */
        if (result)
            result = sm.open_playlist();

        if (result)
        {
            std::string fname = sm.midi_filename();
            if (! fname.empty())
            {
                fname = sm.open_midi_file(fname, errormessage);
                if (fname.empty())
                {
                    warnprintf("Could not open %s\n", fname.c_str());
                }
            }
            result = sm.create_window();
        }
    }

#if defined SEQ66_PORTMIDI_SUPPORT

    /*
     * We check for any "fatal" PortMidi errors, so we can display them.  But
     * we still want to keep going, in order to at least generate the
     * log-files and "erroneous" configuration files to
     * C:/Users/me/AppData/Local/seq66 or ~/.config/seq66.
     */

    if (result)
    {
        std::string errmsg;
        if (sm.portmidi_error_check(errmsg))
        {
            sm.show_message(errmsg);
            result = false;
#if defined SEQ66_PLATFORM_WINDOWS
            const char * pmerrmsg = pm_log_buffer();
            if (not_nullptr(pmerrmsg) && strlen(pmerrmsg) > 0)
            {
                std::string path = seq66::rc().config_filespec("portmidi.log");
                errmsg += "\n\n";
                errmsg += std::string(pmerrmsg);
                (void) seq66::file_append_log(path, errmsg);
            }
#endif
        }
    }

#endif  // SEQ66_PORTMIDI_SUPPORT

    if (result)
    {
        result = sm.create_session();
        if (result)
        {
            exit_status = sm.run() ? EXIT_SUCCESS : EXIT_FAILURE ;
            result = sm.close_session();
        }
        else
            result = sm.close_session(false);
    }
    else
        result = sm.close_session(false);

    if (! result)
        sm.show_message("Error in session; see erroneous.* files");

    return exit_status;
}

/*
 * seq66qt5.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


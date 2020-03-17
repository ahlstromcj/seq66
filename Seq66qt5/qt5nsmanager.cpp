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
 * \file          qt5nsmanager.cpp
 *
 *  This module declares/defines the main module for the Non Session Manager
 *  control of qseq66.
 *
 * \library       qt5nsmanager application
 * \author        Chris Ahlstrom
 * \date          2020-03-15
 * \updates       2020-03-15
 * \license       GNU GPLv2 or above
 *
 *  Duty now for the future!
 */

#include "cfg/cmdlineopts.hpp"          /* command-line functions           */
#include "cfg/settings.hpp"             /* seq66::usr() and seq66::rc()     */
#include "play/performer.hpp"           /* seq66::performer                 */
#include "util/basic_macros.hpp"        /* seq66::msgprintf()               */

/**
 *
 * \return
 *      Returns EXIT_SUCCESS (0) or EXIT_FAILURE, depending on the status of
 *      the run.
 */

bool
qt5nsmanager::create_session ()
{
}

bool
qt5nsmanager::create_window ()
{
    std::unique_ptr<seq66::qsmainwnd> seq66_window;
    if (ok)
    {
        /*
         * Push the qsmainwnd window onto the stack.  Also be sure to pass
         * along the PPQN value, which might be different than the default
         * (192), and affects some of the child objects of qsmainwnd.
         * Also note the future support for NSM.
         */

        seq66_window.reset
        (
            new seq66::qsmainwnd
            (
                p, midifname, seq66::usr().midi_ppqn(), false /*usensm*/
            )
        );
        seq66_window->show();
        if (seq66::rc().verbose())
            p.show_patterns();
    }
}

void
qt5nsmanager::close_session ()
{
    p.finish();                                 /* tear down performer      */
    p.put_settings(seq66::rc());                /* copy latest settings     */
    if (seq66::rc().auto_option_save())
        (void) seq66::cmdlineopts::write_options_files();
    else
        printf("[auto-option-save off, not saving config files]\n");
}

/*
 * qt5nsmanager.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


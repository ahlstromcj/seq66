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
 * \updates       2020-03-30
 * \license       GNU GPLv2 or above
 *
 *  Duty now for the future!
 */

#include "cfg/settings.hpp"             /* seq66::usr() and seq66::rc()     */
#include "util/basic_macros.hpp"        /* seq66::msgprintf()               */
#include "qt5nsmanager.hpp"             /* seq66::qt5nsmanager              */
#include "qsmainwnd.hpp"                /* seq66::qsmainwnd                 */

namespace seq66
{

/*
 *-------------------------------------------------------------------------
 *  Optional NSM support
 *-------------------------------------------------------------------------
 */

/*
 *-------------------------------------------------------------------------
 * qt5nsmanager
 *-------------------------------------------------------------------------
 */

/**
 *
 */

qt5nsmanager::qt5nsmanager (QObject * parent) :
    QObject         (parent),
    smanager        (),
#if defined SEQ66_NSM_SESSION
    m_nsm_client    (),
#endif
    m_performer     ()
{
}

/**
 *
 */

qt5nsmanager::~qt5nsmanager ()
{
    // no code yet
}

/**
 *
 */

bool
qt5nsmanager::create_session ()
{
    return false;
}

/**
 *
 */

bool
qt5nsmanager::create_window ()
{
    std::unique_ptr<qsmainwnd> seq66_window;

    /*
     * Push the qsmainwnd window onto the stack.  Also be sure to pass
     * along the PPQN value, which might be different than the default
     * (192), and affects some of the child objects of qsmainwnd.
     * Also note the future support for NSM.
     */

    seq66_window.reset
    (
        new qsmainwnd
        (
            *perf(), midi_filename(),
            usr().midi_ppqn(), false /*usensm*/
        )
    );

    /*
     * Let NSM handle this....
     */

    seq66_window->show();
    return bool(seq66_window);
}

/**
 *
 */

void
qt5nsmanager::show_message (const std::string & /*msg*/)
{
    // TODO:
    // m_seq66_window->show_message_box(msg);
}

/**
 *
 */

void
qt5nsmanager::show_error (const std::string & /*msg*/)
{
#if defined SEQ66_PORTMIDI_SUPPORT

    if (Pm_error_present())
    {
        errmsg = std::string(Pm_hosterror_message());
        // TODO:
        // m_seq66_window->show_message_box(msg);
    }

#endif

    //// TODO: error_message(msg);
}

}           // namespace seq66

/*
 * qt5nsmanager.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


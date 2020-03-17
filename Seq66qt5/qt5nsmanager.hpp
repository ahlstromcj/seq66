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
 * \file          qt5nsmanager.hpp
 *
 *  This module declares/defines the main module for the JACK/ALSA "qt5"
 *  implementation of this application.
 *
 * \library       qt5nsmanager application
 * \author        Chris Ahlstrom
 * \date          2020-03-15
 * \updates       2020-03-15
 * \license       GNU GPLv2 or above
 *
 *  This is an attempt to change from the hoary old (or, as H.P. Lovecraft
 *  would style it, "eldritch") gtkmm-2.4 implementation of Seq66.
 */

// #include <QApplication>                 /* QApplication etc.                */
#include <memory>                       /* std::unique_ptr<>                */

// #include "cfg/cmdlineopts.hpp"          /* command-line functions           */
// #include "cfg/settings.hpp"             /* seq66::usr() and seq66::rc()     */
#include "play/performer.hpp"           /* seq66::performer                 */
// #include "unix/daemonize.hpp"           /* seqg4::reroute_stdio()           */
#include "util/basic_macros.hpp"        /* seq66::msgprintf()               */
// #include "util/filefunctions.hpp"       /* seq66::file_accessible()         */
// #include "qsmainwnd.hpp"                /* the main window of qt5nsmanager      */

/**
 *
 * \return
 *      Returns success or failure.
 */

class qt5nsmanager
{
private:

    std::unique_ptr<performer> m_performer;

public:

    qt5nsmanager ();
    ~qt5nsmanager ();

    bool create_session ();
    bool recreate_session ();
    bool create_window ();
    void close_session ();

/*
 * qt5nsmanager.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=hpp
 */


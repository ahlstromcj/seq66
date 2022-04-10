#if ! defined SEQ66_QT5_HELPER_H
#define SEQ66_QT5_HELPER_H

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
 * \file          qt5_helper.h
 *
 *  This module declares/defines more helpful macros.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2022-04-10
 * \updates       2022-04-10
 * \license       GNU GPLv2 or above
 *
 *  This file is meant to be included in the C++ "cpp" modules, after any
 *  Qt header-file #includes.
 */

/*
 * A macro to use in a few places, to make things a little easier.
 * Strictly macro substitution.
 */

#if QT_VERSION < QT_VERSION_CHECK(5, 8, 0)
#define QT5_HELPER_RADIO_SIGNAL \
    static_cast<void(QButtonGroup::*) (int)>(&QButtonGroup::buttonClicked)
#elif QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
#define QT5_HELPER_RADIO_SIGNAL  QOverload<int>::of(&QButtonGroup::buttonClicked)
#else
#define QT5_HELPER_RADIO_SIGNAL  QOverload<int>::of(&QButtonGroup::idClicked)
#endif

#endif          // SEQ66_QT5_HELPER_H

/*
 * qt5_helper.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


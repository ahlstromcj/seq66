#if ! defined SEQ24_QT5_HELPERS_HPP
#define SEQ24_QT5_HELPERS_HPP

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
 * \file          qt5_helpers.hpp
 *
 *  This module declares/defines some helpful macros or functions.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-03-14
 * \updates       2019-03-23
 * \license       GNU GPLv2 or above
 *
 */

#include "ctrl/keymap.hpp"              /* seq66::qt_modkey_ordinal()       */
#include "ctrl/keystroke.hpp"           /* seq66::keystroke wrapper class   */

class QKeyEvent;
class QPushButton;

/*
 * Don't document the namespace.
 */

namespace seq66
{

/*
 * Free functions in the seq66 namespace.
 */

extern void qt_set_icon (const char * pixmap_array [], QPushButton * button);
extern keystroke qt_keystroke (QKeyEvent * event, bool press);

}               // namespace seq66

#endif          // SEQ24_QT5_HELPERS_HPP

/*
 * qt5_helpers.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


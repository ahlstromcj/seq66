#if ! defined SEQ66_KEYMAP_HPP
#define SEQ66_KEYMAP_HPP

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
 * \file          keymap.hpp
 *
 *  This module defines some informative functions that are actually
 *  better off as functions.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-12
 * \updates       2020-08-25
 * \license       GNU GPLv2 or above
 *
 */

#include <string>                       /* std::string                      */

#include "midi/midibytes.hpp"           /* the ctrlkey type definition      */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Provides short names for these Qt::KeyboardModifier values, to make the
 *  table readable.
 */

const int KNONE   = 0x00000000;
const int KSHIFT  = 0x02000000;
const int KCTRL   = 0x04000000;
const int KALT    = 0x08000000;
const int KMETA   = 0x10000000;
const int KEYPAD  = 0x20000000;
const int KGROUP  = 0x40000000;

/*
 * Free functions and data.
 */

extern const std::string & undefined_qt_key_name ();
extern bool is_undefined_qt_key (const std::string & keyname);
extern int keymap_size ();
extern bool is_invalid_ordinal (ctrlkey ordinal);
extern unsigned ordinal_to_qt_key (ctrlkey ordinal);
extern ctrlkey qt_modkey_ordinal (ctrlkey qtkey, unsigned qtmodifier);
extern std::string qt_modkey_name (ctrlkey qtkey, unsigned qtmodifier);
extern ctrlkey qt_keyname_ordinal (const std::string & name);
extern std::string qt_ordinal_keyname (ctrlkey qtkey);

}               // namespace seq66

#endif          // SEQ66_KEYMAP_HPP

/*
 * keymap.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


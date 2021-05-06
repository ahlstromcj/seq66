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
 * \updates       2021-05-05
 * \license       GNU GPLv2 or above
 *
 */

#include <string>                       /* std::string                      */
#include "midi/midibytes.hpp"           /* ctrlkey & eventkey types         */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

namespace keyboard
{

/**
 *  Provides a list of supported keyboard layouts.
 *
 *  Only the normal and azerty layouts are supported at this time.
 */

enum class layout
{
    normal,                             /* U.S. key map             */
    qwerty = normal,                    /* Ditto                    */
    qwertz,                             /* Deutsche                 */
    azerty,                             /* French AZERTY key map    */
    max                                 /* terminator value         */
};

/**
 *  Provides short names for these Qt::KeyboardModifier values, to make the
 *  table readable.  Also used in the keystroke class.
 */

using modifiers = enum kmod_t
{
    KNONE       = 0x00000000,
    KSHIFT      = 0x02000000,
    KCTRL       = 0x04000000,
    KCTRLSHIFT  = 0x06000000,
    KALT        = 0x08000000,
    KMETA       = 0x10000000,
    KEYPAD      = 0x20000000,
    KGROUP      = 0x40000000
};

}   // namespace keyboard

/*
 * Free functions in the seq66 namespace.
 */

extern ctrlkey arrow_left ();
extern ctrlkey arrow_up ();
extern ctrlkey arrow_right ();
extern ctrlkey arrow_down ();
extern std::string modifier_names (unsigned kmod);
extern const std::string & undefined_qt_key_name ();
extern bool is_undefined_qt_key (const std::string & keyname);
extern int keymap_size ();
extern bool is_invalid_ordinal (ctrlkey ordinal);
extern unsigned ordinal_to_qt_key (ctrlkey ordinal);
extern ctrlkey qt_modkey_ordinal
(
    eventkey qtkey,
    unsigned qtmodifier,
    eventkey virtkey = 0
);
extern std::string qt_modkey_name
(
    eventkey qtkey,
    unsigned qtmodifier,
    eventkey virtkey = 0
);
extern ctrlkey qt_keyname_ordinal (const std::string & name);
extern std::string qt_ordinal_keyname (ctrlkey qtkey);
extern void modify_keyboard_layout (keyboard::layout el);

}               // namespace seq66

#endif          // SEQ66_KEYMAP_HPP

/*
 * keymap.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


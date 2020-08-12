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
 * \file          gdk_basic_keys.cpp
 *
 *  This module defines a non-Unicode subset of the keys defined by Gtk-2/3
 *  and is used in converting "rc" files.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-12-09
 * \updates       2019-03-25
 * \license       GNU GPLv2 or above
 *
 */

#include <map>                          /* std::map<>                       */

#include "ctrl/keymap.hpp"              /* accessors to static vectors      */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Holds the Qt key and the modifier necessary for the conversion.
 */

struct qt_keypair                       /* not a pair anymore! :-D  */
{
    unsigned qtkp_keycode;
    unsigned qtkp_modifier;
    std::string qtkp_keyname;
};

/**
 *  Provides a data type to convert from incoming Qt 5 key() and text() values
 *  into the corresponding Gdk/Gtkmm key value.
 *
 *  It can also be used to convert from Gtkmm 2.4 to Qt 5, but this conversion
 *  is slower and generally not necessary, as Sequencer64 uses the Gtkmm key
 *  set as the canonical key set, both in processing and in storage in the
 *  "rc" configuration file.
 */

using QtGtkKeyMap = std::map<unsigned, qt_keypair>;

/**
 *  The initializer list for the QtGtkKeyMap structure.  Requires C++11 and
 *  above in order to compile.
 *
 *  The first value is the Gtk version of the key, and is the key-value in the
 *  map. The second value is the corresponing Qt key code.
 *
 *      -   The djinned up name of the key.
 *      -   The key's value in Gtkmm 2.4.
 *      -   The key's value in Qt 5.  A copy of the map's key value.
 */

static QtGtkKeyMap sg_key_map =
{
    { 0x00ec, { 0x60,      KNONE,  "`"        } },  // "Backtick"
    { 0xff08, { 0x1000003, KNONE,  "BkSpace"  } },  // "Backspace"
    { 0xff09, { 0x1000001, KNONE,  "Tab"      } },  // "Tab"
    { 0xff0d, { 0x1000004, KNONE,  "Return"   } },  // "Return"
    { 0xff13, { 0x1000008, KNONE,  "Pause"    } },  // "Pause"
    { 0xff14, { 0x1000026, KNONE,  "ScrlLk"   } },  // "Scroll_Lock"
    { 0xff1b, { 0x1000000, KNONE,  "Esc"      } },  // "Escape"; see "ESC" below
    { 0xff50, { 0x1000010, KNONE,  "Home"     } },  // "Home"
    { 0xff51, { 0x1000012, KNONE,  "Left"     } },  // "Left"
    { 0xff52, { 0x1000013, KNONE,  "Up"       } },  // "Up"
    { 0xff53, { 0x1000014, KNONE,  "Right"    } },  // "Right"
    { 0xff54, { 0x1000015, KNONE,  "Down"     } },  // "Down"
    { 0xff55, { 0x1000016, KNONE,  "PageUp"   } },  // "Page Up"
    { 0xff56, { 0x1000017, KNONE,  "PageDn"   } },  // "Page Down"
    { 0xff57, { 0x1000011, KNONE,  "End"      } },  // "End"
    { 0xff61, { 0x1000009, KNONE,  "Print"    } },  // "Print Scrn"
    { 0xff63, { 0x1000006, KNONE,  "Ins"      } },  // "Insert"
    { 0xff67, { 0x1000055, KNONE,  "Menu"     } },  // "Menu"
    { 0xff7f, { 0x1000025, KNONE,  "NumLk"    } },  // "Num_Lock"
    { 0xff8d, { 0x1000005, KEYPAD, "Enter"    } },  // "KP_Enter"
    { 0xff98, { 0x1000014, KEYPAD, "KP_Right" } },  // "KP_Right"
    { 0xff99, { 0x1000015, KEYPAD, "KP_Down"  } },  // "KP_Down"
    { 0xff9c, { 0x1000011, KEYPAD, "KP_End"   } },  // "KP_End"
    { 0xff95, { 0x1000010, KEYPAD, "KP_Home"  } },  // "KP_Home"
    { 0xff9e, { 0x1000006, KEYPAD, "KP_Ins"   } },  // "KP_Insert"
    { 0xff9f, { 0x1000007, KEYPAD, "KP_Del"   } },  // "KP_Delete"
    { 0xffbe, { 0x1000030, KNONE,  "F1"       } },  // "F1"
    { 0xffbf, { 0x1000031, KNONE,  "F2"       } },  // "F2"
    { 0xffc0, { 0x1000032, KNONE,  "F3"       } },  // "F3"
    { 0xffc1, { 0x1000033, KNONE,  "F4"       } },  // "F4"
    { 0xffc2, { 0x1000034, KNONE,  "F5"       } },  // "F5"
    { 0xffc3, { 0x1000035, KNONE,  "F6"       } },  // "F6"
    { 0xffc4, { 0x1000036, KNONE,  "F7"       } },  // "F7"
    { 0xffc5, { 0x1000037, KNONE,  "F8"       } },  // "F8"
    { 0xffc6, { 0x1000038, KNONE,  "F9"       } },  // "F9"
    { 0xffc7, { 0x1000039, KNONE,  "F10"      } },  // "F10"
    { 0xffc8, { 0x100003a, KNONE,  "F11"      } },  // "F11"
    { 0xffc9, { 0x100003b, KNONE,  "F12"      } },  // "F12"
    { 0xffe1, { 0x1000020, KSHIFT, "Shift"    } },  // "Shift_L"
    { 0xffe2, { 0x1000020, KSHIFT, "Shift"    } },  // "Shift_R"
    { 0xffe3, { 0x1000021, KCTRL,  "Control_L"} },  // "Control_L"
    { 0xffe4, { 0x1000021, KCTRL,  "Control_R"} },  // "Control_R"
    { 0xffe9, { 0x1000023, KNONE,  "Alt_L"    } },  // "Alt" (Alt_L)
    { 0xffea, { 0x1000023, KNONE,  "Alt_R"    } },  // "Alt" (Alt_R)
    { 0xffeb, { 0x1000022, KNONE,  "Super_L"  } },  // "Super" (Super_L)
    { 0xffec, { 0x1000022, KNONE,  "Super_R"  } },  // "Super" (Super_R)
    { 0xffff, { 0x1000007, KNONE,  "Del"      } }   // "Delete"
};

/**
 *
 */

std::string
gdk_key_name (unsigned gdkkeycode)
{
    if (gdkkeycode <= 0x7f)
    {
        /*
         * return lookup_qt_key_name(gdkkeycode);
         */

        char tmp[16];
        if (gdkkeycode < ' ')
            snprintf(tmp, sizeof tmp, "Ctrl-%c", char(gdkkeycode - 64));
        else
            snprintf(tmp, sizeof tmp, "%c", char(gdkkeycode));

        return std::string(tmp);
    }
    else
    {
        auto ki = sg_key_map.find(gdkkeycode);
        if (ki != sg_key_map.end())
            return ki->second.qtkp_keyname;
        else
            return undefined_qt_key_name();
    }
}

}           // namespace seq66

/*
 * gdk_basic_keys.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


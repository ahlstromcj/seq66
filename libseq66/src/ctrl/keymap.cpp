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
 * \file          keymap.cpp
 *
 *  This module defines some informative functions that are actually
 *  better off as functions.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-12
 * \updates       2019-05-27
 * \license       GNU GPLv2 or above
 */

#include <map>                          /* std::map and std::multimap       */

#include "ctrl/keymap.hpp"              /* keymap function declarations     */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Provides a data type for key name/value pairs.
 */

struct qt_keycodes
{
    /**
     *  Provides an 8-bit ordinal number representing our universal code for
     *  that character.  Up to almost 255 characters can be supported.  The
     *  incoming keystroke is analyzed and converted to this number.  Many of
     *  the values are ASCII values.
     */

    ctrlkey qtk_ordinal;

    /**
     *  Provides the incoming code presented to the application by Qt,
     *  obtained by the QKeyEvent::key() function.
     */

    ctrlkey qtk_keycode;

    /**
     *  Provides our name for the keystroke.  These values are deliberately
     *  shortened to save space in the "rc" file.
     */

    std::string qtk_keyname;

    /**
     *  Provides the incoming modifier bit(s) presented to the application by
     *  Qt, obtained by the QKeyEvent::modifiers() function.  This is the
     *  integer version of the enum class qt_kbd_modifier defined above.
     */

    unsigned qtk_modifier;
};

/**
 *  Maps key names to the key integers.  These come mostly from
 *  /usr/include/x86_64-linux-gnu/qt5/QtCore/qnamespace.h, with the "Key_"
 *  prefix removed, or the name radically shortened.
 *
 *  Does not include keypad keys directly, one must include a test for the
 *  KeypadModifier (0x20) bit in the KeyboardModifier enumeration.  Similarly,
 *  the ShiftModifier (0x02), ControlModifier (0x04), AltModifier (0x08), and
 *  MetaModifier (0x10) bits need to be checked.  They are accessed via
 *  QKeyEvent::modifiers().
 *
 *      -   Code:       qtk_ordinal
 *      -   Qt Keycode: qtk_keycode
 *      -   Key Name:   qtk_keyname
 *      -   Modifier:   qtk_modifier
 *
 *  By using the modifier, we can distinguish between various version of the
 *  same key, such as the normal PageDn and the keypad's PageDn.
 *
 *  Also note that we restrict the keystroke names to 9 characters or less, to
 *  keep the MIDI control section readable.
 *
 *  Finally, we started encountering uninitialized static variables.  A g++ bug,
 *  or bug fix?  So now we enforce initialization by wrapping static values in
 *  accessor functions.
 */

static qt_keycodes
qt_keys (int i)
{
    static const qt_keycodes s_qt_keys [] =             // or a vector, meh
    {
        /*
         *  Code        Qt     Key
         * Ordinal    Keycode  Name        Modifier
         *
         *                Ctrl-key section
         */

        { 0x00,        0x40,  "NUL",        KCTRL  },    // ^@: Null
        { 0x01,        0x41,  "SOH",        KCTRL  },    // ^A: Start of Heading
        { 0x02,        0x42,  "STX",        KCTRL  },    // ^B: Start of Text
        { 0x03,        0x43,  "ETX",        KCTRL  },    // ^C: End of Text
        { 0x04,        0x44,  "EOT",        KCTRL  },    // ^D: End of Transmision
        { 0x05,        0x45,  "ENQ",        KCTRL  },    // ^E: Enquiry
        { 0x06,        0x46,  "ACK",        KCTRL  },    // ^F: Acknowledge
        { 0x07,        0x47,  "BEL",        KCTRL  },    // ^G: Bell/beep
        { 0x08,        0x48,  "BS",         KCTRL  },    // ^H: Backspace
        { 0x09,        0x49,  "HT",         KCTRL  },    // ^I: Horizontal Tab
        { 0x0a,        0x4a,  "LF",         KCTRL  },    // ^J: Line Feed
        { 0x0b,        0x4b,  "VT",         KCTRL  },    // ^K: Vertical Tab
        { 0x0c,        0x4c,  "FF",         KCTRL  },    // ^L: Form Feed
        { 0x0d,        0x4d,  "CR",         KCTRL  },    // ^M: Carriage Return
        { 0x0e,        0x4e,  "SO",         KCTRL  },    // ^N: Shift Out
        { 0x0f,        0x4f,  "SI",         KCTRL  },    // ^O: Shift In
        { 0x10,        0x50,  "DLE",        KCTRL  },    // ^P: Data Link Escape
        { 0x11,        0x51,  "DC1",        KCTRL  },    // ^Q: Device Control 1
        { 0x12,        0x52,  "DC2",        KCTRL  },    // ^R: Device Control 2
        { 0x13,        0x53,  "DC3",        KCTRL  },    // ^S: Device Control 3
        { 0x14,        0x54,  "DC4",        KCTRL  },    // ^T: Device Control 4
        { 0x15,        0x55,  "NAK",        KCTRL  },    // ^U: Negative ACK
        { 0x16,        0x56,  "SYN",        KCTRL  },    // ^V: Synchronous Idle
        { 0x17,        0x57,  "ETB",        KCTRL  },    // ^W: End of Trans Block
        { 0x18,        0x58,  "CAN",        KCTRL  },    // ^X: Cancel
        { 0x19,        0x59,  "EM",         KCTRL  },    // ^Y: End of Medium
        { 0x1a,        0x5a,  "SUB",        KCTRL  },    // ^Z: Substitute
        { 0x1b,        0x5b,  "ESC",        KCTRL  },    // ^[: Escape
        { 0x1c,        0x5c,  "FS",         KCTRL  },    // ^\: File Separator
        { 0x1d,        0x5d,  "GS",         KCTRL  },    // ^]: Group Separator
        { 0x1e,        0x5e,  "RS",         KCTRL  },    // ^^: Record Separator
        { 0x1f,        0x5f,  "US",         KCTRL  },    // ^_: Unit Separator

        /*
         * Block of Qt-based code moved to below.
         *
         * Code   Qt Keycode   Key Name      Modifier
         *
         *            Unmodified key section
         */

        { 0x20,        0x20,  "Space",      KNONE  },    // Space (" " not good)
        { 0x21,        0x21,  "!",          KSHIFT },    // Exclam
        { 0x22,        0x22,  "\"",         KSHIFT },    // QuoteDbl
        { 0x23,        0x23,  "#",          KSHIFT },    // NumberSign
        { 0x24,        0x24,  "$",          KSHIFT },    // Dollar
        { 0x25,        0x25,  "%",          KSHIFT },    // Percent
        { 0x26,        0x26,  "&",          KSHIFT },    // Ampersand
        { 0x27,        0x27,  "'",          KSHIFT },    // Apostrophe
        { 0x28,        0x28,  "(",          KSHIFT },    // ParenLeft
        { 0x29,        0x29,  ")",          KSHIFT },    // ParenRight
        { 0x2a,        0x2a,  "*",          KSHIFT },    // Asterisk
        { 0x2b,        0x2b,  "+",          KSHIFT },    // Plus
        { 0x2c,        0x2c,  ",",          KNONE  },    // Comma
        { 0x2d,        0x2d,  "-",          KNONE  },    // Minus
        { 0x2e,        0x2e,  ".",          KNONE  },    // Period
        { 0x2f,        0x2f,  "/",          KNONE  },    // Slash
        { 0x30,        0x30,  "0",          KNONE  },
        { 0x31,        0x31,  "1",          KNONE  },
        { 0x32,        0x32,  "2",          KNONE  },
        { 0x33,        0x33,  "3",          KNONE  },
        { 0x34,        0x34,  "4",          KNONE  },
        { 0x35,        0x35,  "5",          KNONE  },
        { 0x36,        0x36,  "6",          KNONE  },
        { 0x37,        0x37,  "7",          KNONE  },
        { 0x38,        0x38,  "8",          KNONE  },
        { 0x39,        0x39,  "9",          KNONE  },
        { 0x3a,        0x3a,  ":",          KSHIFT },
        { 0x3b,        0x3b,  ";",          KNONE  },
        { 0x3c,        0x3c,  "<",          KSHIFT },
        { 0x3d,        0x3d,  "=",          KNONE  },
        { 0x3e,        0x3e,  ">",          KSHIFT },
        { 0x3f,        0x3f,  "?",          KSHIFT },
        { 0x40,        0x40,  "@",          KSHIFT },
        { 0x41,        0x41,  "A",          KSHIFT },    /* Shift key modifier  */
        { 0x42,        0x42,  "B",          KSHIFT },
        { 0x43,        0x43,  "C",          KSHIFT },
        { 0x44,        0x44,  "D",          KSHIFT },
        { 0x45,        0x45,  "E",          KSHIFT },
        { 0x46,        0x46,  "F",          KSHIFT },
        { 0x47,        0x47,  "G",          KSHIFT },
        { 0x48,        0x48,  "H",          KSHIFT },
        { 0x49,        0x49,  "I",          KSHIFT },
        { 0x4a,        0x4a,  "J",          KSHIFT },
        { 0x4b,        0x4b,  "K",          KSHIFT },
        { 0x4c,        0x4c,  "L",          KSHIFT },
        { 0x4d,        0x4d,  "M",          KSHIFT },
        { 0x4e,        0x4e,  "N",          KSHIFT },
        { 0x4f,        0x4f,  "O",          KSHIFT },
        { 0x50,        0x50,  "P",          KSHIFT },
        { 0x51,        0x51,  "Q",          KSHIFT },
        { 0x52,        0x52,  "R",          KSHIFT },
        { 0x53,        0x53,  "S",          KSHIFT },
        { 0x54,        0x54,  "T",          KSHIFT },
        { 0x55,        0x55,  "U",          KSHIFT },
        { 0x56,        0x56,  "V",          KSHIFT },
        { 0x57,        0x57,  "W",          KSHIFT },
        { 0x58,        0x58,  "X",          KSHIFT },
        { 0x59,        0x59,  "Y",          KSHIFT },
        { 0x5a,        0x5a,  "Z",          KSHIFT },
        { 0x5b,        0x5b,  "[",          KNONE  },    // BracketLeft
        { 0x5c,        0x5c,  "\\",         KNONE  },    // Backslash
        { 0x5d,        0x5d,  "]",          KNONE  },    // BracketRight
        { 0x5e,        0x5e,  "^",          KSHIFT },    // AsciiCircum
        { 0x5f,        0x5f,  "_",          KSHIFT },    // Underscore
        { 0x60,        0x60,  "`",          KNONE  },    // QuoteLeft, Backtick
        { 0x61,        0x41,  "a",          KNONE  },
        { 0x62,        0x42,  "b",          KNONE  },
        { 0x63,        0x43,  "c",          KNONE  },
        { 0x64,        0x44,  "d",          KNONE  },
        { 0x65,        0x45,  "e",          KNONE  },
        { 0x66,        0x46,  "f",          KNONE  },
        { 0x67,        0x47,  "g",          KNONE  },
        { 0x68,        0x48,  "h",          KNONE  },
        { 0x69,        0x49,  "i",          KNONE  },
        { 0x6a,        0x4a,  "j",          KNONE  },
        { 0x6b,        0x4b,  "k",          KNONE  },
        { 0x6c,        0x4c,  "l",          KNONE  },
        { 0x6d,        0x4d,  "m",          KNONE  },
        { 0x6e,        0x4e,  "n",          KNONE  },
        { 0x6f,        0x4f,  "o",          KNONE  },
        { 0x70,        0x50,  "p",          KNONE  },
        { 0x71,        0x51,  "q",          KNONE  },
        { 0x72,        0x52,  "r",          KNONE  },
        { 0x73,        0x53,  "s",          KNONE  },
        { 0x74,        0x54,  "t",          KNONE  },
        { 0x75,        0x55,  "u",          KNONE  },
        { 0x76,        0x56,  "v",          KNONE  },
        { 0x77,        0x57,  "w",          KNONE  },
        { 0x78,        0x58,  "x",          KNONE  },
        { 0x79,        0x59,  "y",          KNONE  },
        { 0x7a,        0x5a,  "z",          KNONE  },
        { 0x7b,        0x7b,  "{",          KSHIFT },    // BraceLeft
        { 0x7c,        0x7c,  "|",          KSHIFT },    // Bar
        { 0x7d,        0x7d,  "}",          KSHIFT },    // BraceRight
        { 0x7e,        0x7e,  "~",          KSHIFT },    // AsciiTilde
        { 0x7f,        0x7f,  "DEL",        KNONE  },

        /*
         * Block moved from above to here.
         *
         * Code   Qt Keycode   Key Name      Modifier
         */

        { 0x80,  0x01000000,  "Esc",        KNONE  },
        { 0x81,  0x01000001,  "Tab",        KNONE  },    // best to avoid this one
        { 0x82,  0x01000002,  "BkTab",      KSHIFT },    // best to avoid this one
        { 0x83,  0x01000003,  "BkSpace",    KNONE  },    // differs from Ctrl-H !
        { 0x84,  0x01000004,  "Return",     KNONE  },
        { 0x85,  0x01000005,  "Enter",      KNONE  },
        { 0x86,  0x01000006,  "Ins",        KNONE  },
        { 0x87,  0x01000007,  "Del",        KNONE  },
        { 0x88,  0x01000008,  "Pause",      KNONE  },
        { 0x89,  0x01000009,  "Print",      KNONE  },
        { 0x8a,  0x0100000a,  "SysReq",     KNONE  },
        { 0x8b,  0x0100000b,  "Clear",      KNONE  },
        { 0x8c,  0x0100000c,  "Null_ff",    KNONE  },
        { 0x8d,  0x0100000d,  "Null_ff",    KNONE  },
        { 0x8e,  0x0100000e,  "Null_ff",    KNONE  },
        { 0x8f,  0x0100000f,  "Null_ff",    KNONE  },
        { 0x90,  0x01000010,  "Home",       KNONE  },
        { 0x91,  0x01000011,  "End",        KNONE  },
        { 0x92,  0x01000012,  "Left",       KNONE  },
        { 0x93,  0x01000013,  "Up",         KNONE  },
        { 0x94,  0x01000014,  "Right",      KNONE  },
        { 0x95,  0x01000015,  "Down",       KNONE  },
        { 0x96,  0x01000016,  "PageUp",     KNONE  },
        { 0x97,  0x01000017,  "PageDn",     KNONE  },
        { 0x98,  0x01000020,  "Shift",      KSHIFT },    // ignore this one?
        { 0x99,  0x01000021,  "Ctrl",       KCTRL  },
        { 0x9a,  0x01000022,  "Meta",       KMETA  },
        { 0x9b,  0x01000023,  "Alt",        KALT   },
        { 0x9c,  0x01000024,  "CapsLk",     KNONE  },
        { 0x9d,  0x01000025,  "NumLk",      KNONE  },
        { 0x9e,  0x01000026,  "ScrlLk",     KNONE  },
        { 0x9f,  0x01000027,  "Null_ff",    KNONE  },
        { 0xa0,  0x01000030,  "F1",         KNONE  },
        { 0xa1,  0x01000031,  "F2",         KNONE  },
        { 0xa2,  0x01000032,  "F3",         KNONE  },
        { 0xa3,  0x01000033,  "F4",         KNONE  },    // FR: lira
        { 0xa4,  0x01000034,  "F5",         KNONE  },
        { 0xa5,  0x01000035,  "F6",         KNONE  },
        { 0xa6,  0x01000036,  "F7",         KNONE  },
        { 0xa7,  0x01000037,  "F8",         KNONE  },    // FR: silcrow
        { 0xa8,  0x01000038,  "F9",         KNONE  },
        { 0xa9,  0x01000039,  "F10",        KNONE  },
        { 0xaa,  0x0100003a,  "F11",        KNONE  },
        { 0xab,  0x0100003b,  "F12",        KNONE  },
        { 0xac,  0x01000053,  "Super_L",    KNONE  },
        { 0xad,  0x01000054,  "Super_R",    KNONE  },
        { 0xae,  0x01000055,  "Menu",       KNONE  },
        { 0xaf,  0x01000056,  "Hyper_L",    KNONE  },
        { 0xb0,  0x01000057,  "Hyper_R",    KNONE  },    // FR: degree
        { 0xb1,  0x01000058,  "Help",       KNONE  },
        { 0xb2,  0x01000059,  "Dir_L",      KNONE  },    // FR: 2-super
        { 0xb3,  0x01000060,  "Dir_R",      KNONE  },    // Direction_R
        { 0xb4,  0x01000030,  "Sh_F1",      KSHIFT },
        { 0xb5,  0x01000031,  "Sh_F2",      KSHIFT },    // FR: mu
        { 0xb6,  0x01000032,  "Sh_F3",      KSHIFT },
        { 0xb7,  0x01000033,  "Sh_F4",      KSHIFT },
        { 0xb8,  0x01000034,  "Sh_F5",      KSHIFT },
        { 0xb9,  0x01000035,  "Sh_F6",      KSHIFT },
        { 0xba,  0x01000036,  "Sh_F7",      KSHIFT },
        { 0xbb,  0x01000037,  "Sh_F8",      KSHIFT },
        { 0xbc,  0x01000038,  "Sh_F9",      KSHIFT },
        { 0xbd,  0x01000039,  "Sh_F10",     KSHIFT },
        { 0xbe,  0x0100003a,  "Sh_F11",     KSHIFT },
        { 0xbf,  0x0100003b,  "Sh_F12",     KSHIFT },

        /*
         * Block moved from above to here.
         *
         * Code   Qt Keycode   Key Name      Modifier
         *
         *              Keypad section
         */

        { 0xc0,  0x01000006,  "KP_Ins",     KEYPAD },
        { 0xc1,  0x01000007,  "KP_Del",     KEYPAD },
        { 0xc2,  0x01000008,  "Pause",      KEYPAD },    // impossible
        { 0xc3,  0x01000009,  "Print",      KEYPAD },    // impossible
        { 0xc4,  0x01000010,  "KP_Home",    KEYPAD },
        { 0xc5,  0x01000011,  "KP_End",     KEYPAD },
        { 0xc6,  0x01000012,  "KP_Left",    KEYPAD },
        { 0xc7,  0x01000013,  "KP_Up",      KEYPAD },
        { 0xc8,  0x01000014,  "KP_Right",   KEYPAD },
        { 0xc9,  0x01000015,  "KP_Down",    KEYPAD },
        { 0xca,  0x01000016,  "KP_PageUp",  KEYPAD },
        { 0xcb,  0x01000017,  "KP_PageDn",  KEYPAD },
        { 0xcc,  0x01000099,  "0xcc",       KNONE  },
        { 0xcd,  0x01000099,  "0xcd",       KNONE  },
        { 0xce,  0x01000099,  "0xce",       KNONE  },
        { 0xcf,  0x01000099,  "0xcf",       KNONE  },
        { 0xd0,        0x2a,  "KP_*",       KEYPAD },    // Asterisk
        { 0xd1,        0x2b,  "KP_+",       KEYPAD },    // Plus
        { 0xd2,        0x2c,  "KP_,",       KEYPAD },    // Comma, impossible
        { 0xd3,        0x2d,  "KP_-",       KEYPAD },    // Minus
        { 0xd4,        0x2e,  "KP_.",       KNONE  },    // Period
        { 0xd5,        0x2f,  "KP_/",       KEYPAD },    // Slash
        { 0xd6,  0x01000099,  "0xd6",       KNONE  },

        /*
         *              Remainders
         */

        { 0xd7,  0x01000020,  "Shift",      KSHIFT },    // ignore this one
        { 0xd8,  0x01000021,  "Control",    KCTRL  },    // redundant!
        { 0xd9,  0x01000022,  "Super",      KMETA  },    // redundant!
        { 0xda,  0x01000023,  "Alt",        KALT   },    // redundant!
        { 0xdb,  0x01000024,  "CapsLk",     KNONE  },    // redundant!

        { 0xdc,  0x01000099,  "0xdc",       KNONE  },
        { 0xdd,  0x01000099,  "0xdd",       KNONE  },
        { 0xde,  0x01000099,  "0xde",       KNONE  },
        { 0xdf,  0x01000099,  "0xdf",       KNONE  },
        { 0xe0,  0x01000099,  "0xe0",       KNONE  },    // FR: a-grave
        { 0xe1,  0x01000099,  "0xe1",       KNONE  },
        { 0xe2,  0x01000099,  "0xe2",       KNONE  },
        { 0xe3,  0x01000099,  "0xe3",       KNONE  },
        { 0xe4,  0x01000099,  "0xe4",       KNONE  },
        { 0xe5,  0x01000099,  "0xe5",       KNONE  },
        { 0xe6,  0x01000099,  "0xe6",       KNONE  },
        { 0xe7,  0x01000099,  "0xe7",       KNONE  },    // FR: c-cedilla
        { 0xe8,  0x01000099,  "0xe8",       KNONE  },    // FR: e-grave
        { 0xe9,  0x01000099,  "0xe9",       KNONE  },    // FR: e-accent
        { 0xea,  0x01000099,  "0xea",       KNONE  },
        { 0xeb,  0x01000099,  "0xeb",       KNONE  },
        { 0xec,  0x01000099,  "0xec",       KNONE  },
        { 0xed,  0x01000099,  "0xed",       KNONE  },
        { 0xee,  0x01000099,  "0xee",       KNONE  },
        { 0xef,  0x01000099,  "0xef",       KNONE  },

        /*
         * This section is currently useful to fill in for future expansion.
         */

        { 0xf0,  0x01000099,  "0xf0",       KNONE  },
        { 0xf1,  0x01000099,  "0xf1",       KNONE  },
        { 0xf2,  0x01000099,  "0xf2",       KNONE  },
        { 0xf3,  0x01000099,  "0xf3",       KNONE  },
        { 0xf4,  0x01000099,  "0xf4",       KNONE  },
        { 0xf5,  0x01000099,  "0xf5",       KNONE  },
        { 0xf6,  0x01000099,  "0xf6",       KNONE  },
        { 0xf7,  0x01000099,  "0xf7",       KNONE  },
        { 0xf8,  0x01000099,  "0xf8",       KNONE  },
        { 0xf9,  0x01000099,  "0xf9",       KNONE  },    // FR: u-grave
        { 0xfa,  0x01000099,  "0xfa",       KNONE  },
        { 0xfb,  0x01000099,  "0xfb",       KNONE  },
        { 0xfc,  0x01000099,  "0xfc",       KNONE  },
        { 0xfd,  0x01000099,  "0xfd",       KNONE  },
        { 0xfe,  0x01000099,  "0xfe",       KNONE  },
        { 0xff,  0xffffffff,  "Null_ff",    KNONE  }     // end-of-list
    };
    if (i < 0 || i > int(0xff))
        i = 0xff;

    return s_qt_keys[i];
}

/**
 *  Returns the key-name for undefined keys, those not yet defined in the
 *  key-map.  Also used when out-of-range ordinals are encountered.
 */

const std::string &
undefined_qt_key_name ()
{
    static const std::string s_undefined_key_name = "Null_ff";
    return s_undefined_key_name;
}

/**
 *  Checks if the given key-name matches the undefined key name.
 */

bool
is_undefined_qt_key (const std::string & keyname)
{
    return keyname == undefined_qt_key_name();
}

/**
 *  Indicates that a keystroke could not be found in the keymap.
 */

ctrlkey
invalid_ordinal ()
{
    return 0xffffffff;
}

/**
 *  Indicates that a keystroke could not be found in the keymap.
 */

bool
is_invalid_ordinal (ctrlkey ordinal)
{
    return ordinal == invalid_ordinal();
}

/**
 *  Provides a data type to convert from incoming Qt 5 key() and text() values
 *  into the corresponding Gdk/Gtkmm key value.
 *
 *  It can also be used to convert from Gtkmm 2.4 to Qt 5, but this conversion
 *  is slower and generally not necessary, as Sequencer64 uses the Gtkmm key
 *  set as the canonical key set, both in processing and in storage in the
 *  "rc" configuration file.
 */

using qt_keycode_map = std::multimap<unsigned, qt_keycodes>;

/**
 *  Provide a built-in key map.  We could make all this stuff part of a class,
 *  but there doesn't seem to be any benefit to that.
 */

static qt_keycode_map &
keycode_map ()
{
    static qt_keycode_map s_keycode_map;
    return s_keycode_map;
}

/**
 *  Another map to use to look up keystroke names from the "rc" file.
 */

using qt_keyname_map = std::map<std::string, unsigned>;

/**
 *  Provide a built-in name map.
 */

static qt_keyname_map &
keyname_map ()
{
    static qt_keyname_map s_keyname_map;
    return s_keyname_map;
}

/**
 *  Returns the keymap's size.
 */

int
keymap_size ()
{
    return static_cast<int>(keycode_map().size());
}

/**
 *  Indicates if the key map is set up.

 *  We could let C++11 load this via the initializer list, but then we'd
 *  need an extra column for the key.
 */

void
initialize_key_maps ()
{
    static bool s_are_maps_initialized = false;
    if (! s_are_maps_initialized)
    {
        for (int k = 0; k < 255; ++k)
        {
            qt_keycodes temp = qt_keys(k);
            if (temp.qtk_keycode != 0)
            {
                /*
                 * auto --> std::pair<unsigned, qt_keycodes>
                 */

                auto pk = std::make_pair(temp.qtk_keycode, temp);
                (void) keycode_map().insert(pk);

                /*
                 * auto --> std::pair<std::string, unsigned>
                 */

                auto pn = std::make_pair(temp.qtk_keyname, temp.qtk_ordinal);
                (void) keyname_map().insert(pn);
            }
            else
                break;
        }
        if (keymap_size() == 255)
            s_are_maps_initialized = true;
    }

#if defined SEQ66_PLATFORM_DEBUG_TMI
    int index = 0;
    for (const auto & kp : keycode_map())
    {
        printf
        (
            "Key[%3d] = { 0x%2x <-- keycode 0x%8x, '%9s', mod 0x%2x }\n",
            index, kp.second.qtk_ordinal, kp.second.qtk_keycode,
            kp.second.qtk_keyname.c_str(), kp.second.qtk_modifier
        );
        ++index;
    }
    printf("keymap size = %d\n", keymap_size());
#endif
}

/**
 *  The inverse of qt_key_to_ordinal().  Slower, due to a brute force search.
 *
 * \param ordinal
 *      The ordinal key-code to look up.  The ordinals the application support
 *      ranges from 0 to 255.
 *
 * \return
 *      If the Gdk maps to a special character, this function returns the
 *      corresponding Qt 5 key-code (0x1000000 and above) or, if a normal
 *      character, the Qt 5 text-code (in the ASCII) range.
 */

unsigned
ordinal_to_qt_key (ctrlkey ordinal)
{
    unsigned result = 0;
    initialize_key_maps();
    for (const auto & qk : keycode_map())
    {
        if (qk.second.qtk_ordinal == ordinal)
        {
            result = qk.second.qtk_keycode;
            break;
        }
    }
    return result;
}

/**
 *  This function searches for a given keystroke, including the specified
 *  modifier.  This function lets us return different ordinals for variations
 *  on the same key.  For example, the normal PageUp key can be distinguished
 *  from the PageUP key on the key-pad.
 *
 *  This function does not use the "text()" result passed in by Qt; it has to
 *  search for all key-codes.
 *
 * \param qtkey
 *      The Qt 5 key-code, as provided to the Qt keyPressEvent() callback via
 *      QKeyEvent::key().  This value does not distinguish between lower-case
 *      and upper-case characters.  However, the qtmodifier parameter can do
 *      that, and more.
 *
 * \param qtmodifier
 *      The KeyboardModifier code returned from the QKeyEvent::modifiers()
 *      function.  It can alter the value returned, for example distinguishing
 *      normal keys from keypad keys.
 *
 * \return
 *      If the qttext parameter is greater than 0, it is returned unaltered.
 *      If the qtkey parameter is less than 0x1000000 (this should not
 *      happen), it is returned unaltered.  Otherwise, the keycode is looked
 *      up in the keycode_map(), and, if found, the corresponding Gdk keycode
 *      is returned.  If not found, then 0xffffffff is returned.  This would
 *      indicate an error of some kind.
 */

ctrlkey
qt_modkey_ordinal (ctrlkey qtkey, unsigned qtmodifier)
{
    initialize_key_maps();

    auto cqi = keycode_map().find(qtkey);          /* copy modified later! */
    if (cqi != keycode_map().end())
    {
        /*
         * std::multimap<unsigned, qt_keycodes>::size_type
         */

        auto c = keycode_map().count(qtkey);
        if (c > 1)
        {
            auto p = keycode_map().equal_range(qtkey);
            for (cqi = p.first; cqi != p.second; ++cqi)
            {
                if (cqi->second.qtk_modifier == qtmodifier)
                    break;
            }
        }
        return cqi->second.qtk_ordinal;
    }
    else
        return 0xffffffff;
}

/**
 *  Gets the name of the key/text combination, either from the text() value or
 *  via lookup in the key-code map.
 *
 * \param qtkey
 *      The Qt 5 key-code, as provided to the Qt keyPressEvent() callback via
 *      QKeyEvent::key().  This value does not distinguish between lower-case
 *      and upper-case characters.
 *
 * \param qtmodifier
 *      The KeyboardModifier code returned from the QKeyEvent::modifiers()
 *      function.  It can alter the value returned, for example distinguishing
 *      normal keys from keypad keys.
 *
 * \return
 *      Returns the name of the key.
 */

std::string
qt_modkey_name (ctrlkey qtkey, unsigned qtmodifier)
{
    initialize_key_maps();

    auto cqi = keycode_map().find(qtkey);          /* copy modified later! */
    if (cqi != keycode_map().end())
    {
        /*
         * std::multimap<unsigned, qt_keycodes>::size_type
         */

        auto c = keycode_map().count(qtkey);
        if (c > 1)
        {
            auto p = keycode_map().equal_range(qtkey);
            for (cqi = p.first; cqi != p.second; ++cqi)
            {
                if (cqi->second.qtk_modifier == qtmodifier)
                    break;
            }
        }
        if (cqi->second.qtk_keyname == " ")
            return std::string("Space");
        else
            return cqi->second.qtk_keyname;
    }
    else
        return std::string("unknown");
}

/**
 *  Gets the ordinal from the given key name.  It is the opposite of the
 *  qt_ordinal_keyname() function.
 *
 * \param name
 *      Provides the "official" name of the key.
 *
 * \return
 *      If the key is found, its ordinal value (unsigned) is returned.
 *      Otherwise, invalid_ordinal() is returned.  Check for this value with
 *      the is_invalid_ordinal() function.
 */

ctrlkey
qt_keyname_ordinal (const std::string & name)
{
    ctrlkey result = invalid_ordinal();
    initialize_key_maps();

    const auto & cki = keyname_map().find(name);
    if (cki != keyname_map().end())
        result = cki->second;

    return result;
}

/**
 *  Gets the key name from the given ordinal.  It uses a brute force lookup,
 *  but it is fast because all 255 ordinals have an entry in the qt_keycodes
 *  array s_qt_keys[].
 *
 *  This function is useful mainly in error reporting.  It is the opposite of
 *  the qt_keyname_ordinal() function.
 */

std::string
qt_ordinal_keyname (ctrlkey ordinal)
{
    return qt_keys(ordinal).qtk_keyname;
}

}           // namespace seq66

/*
 * keymap.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


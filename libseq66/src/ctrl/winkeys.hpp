#if ! defined SEQ66_WINKEYS_HPP
#define SEQ66_WINKEYS_HPP

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
 * \file          winkeys.hpp
 *
 *  This module defines some key-handling functionality  specific to Windows.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2023-07-18
 * \updates       2023-07-18
 * \license       GNU GPLv2 or above
 *
 *  Include this file in keymap.cpp if building for Windows. The main
 *  difference is in the values of the native virtual keys.  See the
 *  contrib/notes/win-virtual-keys.text file for source information.  Note
 *  that many of the Windows virtual keys represent functions we don't support
 *  here, usch as mouse button and Asion IME modes.
 *
 * #include <winuser.h>                    // VK_SHIFT and other key constants
 */

/**
 *  Maps key names to the key integers. The Windows version. See the
 *  original version in keymap.cpp for more details.
 */

static qt_keycodes &
qt_keys (int i)
{
    using namespace keyboard;
    static qt_keycodes s_qt_keys [] =
    {
        /*
         *  Code        Qt      Qt      Key
         * Ordinal   Key Event Virt Key Name        Modifier
         *
         * Ctrl-key section. It is best to avoid using Control keys to control
         * loops, mutes, and automation.  Too much chance of interfering with
         * the normal user interface.
         */

        {  0x00,        0x40,   0x00,  "NUL",        KCTRL  },  // ^@: Null
        {  0x01,        0x41,   0x01,  "SOH",        KCTRL  },  // ^A: Start of Heading
        {  0x02,        0x42,   0x02,  "STX",        KCTRL  },  // ^B: Start of Text
        {  0x03,        0x43,   0x03,  "ETX",        KCTRL  },  // ^C: End of Text
        {  0x04,        0x44,   0x04,  "EOT",        KCTRL  },  // ^D: End of Transmision
        {  0x05,        0x45,   0x05,  "ENQ",        KCTRL  },  // ^E: Enquiry
        {  0x06,        0x46,   0x06,  "ACK",        KCTRL  },  // ^F: Acknowledge
        {  0x07,        0x47,   0x07,  "BEL",        KCTRL  },  // ^G: Bell/beep
        {  0x08,        0x48,   0x08,  "BS",         KCTRL  },  // ^H: Backspace, VK_BACK
        {  0x09,        0x49,   0x09,  "HT",         KCTRL  },  // ^I: Tab, VK_TAB
        {  0x0a,        0x4a,   0x0a,  "LF",         KCTRL  },  // ^J: Line Feed
        {  0x0b,        0x4b,   0x0b,  "VT",         KCTRL  },  // ^K: Vertical Tab
        {  0x0c,        0x4c,   0x0c,  "FF",         KCTRL  },  // ^L: Form Feed
        {  0x0d,        0x4d,   0x0d,  "CR",         KCTRL  },  // ^M: CR, VK_RETURN
        {  0x0e,        0x4e,   0x0e,  "SO",         KCTRL  },  // ^N: Shift Out
        {  0x0f,        0x4f,   0x0f,  "SI",         KCTRL  },  // ^O: Shift In
        {  0x10,        0x50,   0x10,  "DLE",        KCTRL  },  // ^P: Data Link Escape
        {  0x11,        0x51,   0x11,  "DC1",        KCTRL  },  // ^Q: Device Control 1
        {  0x12,        0x52,   0x12,  "DC2",        KCTRL  },  // ^R: Device Control 2
        {  0x13,        0x53,   0x13,  "DC3",        KCTRL  },  // ^S: Device Control 3
        {  0x14,        0x54,   0x14,  "DC4",        KCTRL  },  // ^T: Device Control 4
        {  0x15,        0x55,   0x15,  "NAK",        KCTRL  },  // ^U: Negative ACK
        {  0x16,        0x56,   0x16,  "SYN",        KCTRL  },  // ^V: Synchronous Idle
        {  0x17,        0x57,   0x17,  "ETB",        KCTRL  },  // ^W: End of Trans Block
        {  0x18,        0x58,   0x18,  "CAN",        KCTRL  },  // ^X: Cancel
        {  0x19,        0x59,   0x19,  "EM",         KCTRL  },  // ^Y: End of Medium
        {  0x1a,        0x5a,   0x1a,  "SUB",        KCTRL  },  // ^Z: Substitute
        {  0x1b,        0x5b,   0x1b,  "ESC",        KCTRL  },  // ^[: Escape, VK_ESCAPE
        {  0x1c,        0x5c,   0x1c,  "FS",         KCTRL  },  // ^\: File Separator
        {  0x1d,        0x5d,   0x1d,  "GS",         KCTRL  },  // ^]: Group Separator
        {  0x1e,        0x5e,   0x1e,  "RS",    KCTRLSHIFT  },  // ^^: Record Separator
        {  0x1f,        0x5f,   0x1f,  "US",    KCTRLSHIFT  },  // ^_???: Unit Separator

        /*
         * Ordinal   Key Event Virt Key Name        Modifier
         */

        {  0x20,        0x20,   0x20,  "Space",      KNONE  },  // Space, VK_SPACE
        {  0x21,        0x21,   0x31,  "!",          KSHIFT },  // Exclam, Shift-1
        {  0x22,        0x22,   0xde,  "\"",         KSHIFT },  // QuoteDbl, VK_OEM_7
        {  0x23,        0x23,   0x33,  "#",          KSHIFT },  // NumberSign, Shift-3
        {  0x24,        0x24,   0x34,  "$",          KSHIFT },  // Dollar, Shift-4
        {  0x25,        0x25,   0x35,  "%",          KSHIFT },  // Percent, Shift-5
        {  0x26,        0x26,   0x37,  "&",          KSHIFT },  // Ampersand, Shift-7
        {  0x27,        0x27,   0xde,  "'",          KSHIFT },  // Apostrophe, VK_OEM_7
        {  0x28,        0x28,   0x39,  "(",          KSHIFT },  // ParenLeft, Shift-9
        {  0x29,        0x29,   0x30,  ")",          KSHIFT },  // ParenRight, Shift-0
        {  0x2a,        0x2a,   0x38,  "*",          KSHIFT },  // Asterisk, Shift-8
        {  0x2b,        0x2b,   0xbb,  "+",          KSHIFT },  // Plus, VK_OEM_PLUS
        {  0x2c,        0x2c,   0xbc,  ",",          KNONE  },  // Comma, VK_OEM_COMMA
        {  0x2d,        0x2d,   0xbd,  "-",          KNONE  },  // Minus, VK_OEM_MINUS
        {  0x2e,        0x2e,   0xbe,  ".",          KNONE  },  // Period, VK_OEM_PERIOD
        {  0x2f,        0x2f,   0xbf,  "/",          KNONE  },  // Slash, VK_OEM_2
        {  0x30,        0x30,   0x30,  "0",          KNONE  },  // ASCII
        {  0x31,        0x31,   0x31,  "1",          KNONE  },  // ASCII
        {  0x32,        0x32,   0x32,  "2",          KNONE  },  // ASCII
        {  0x33,        0x33,   0x33,  "3",          KNONE  },  // ASCII
        {  0x34,        0x34,   0x34,  "4",          KNONE  },  // ASCII
        {  0x35,        0x35,   0x35,  "5",          KNONE  },  // ASCII
        {  0x36,        0x36,   0x36,  "6",          KNONE  },  // ASCII
        {  0x37,        0x37,   0x37,  "7",          KNONE  },  // ASCII
        {  0x38,        0x38,   0x38,  "8",          KNONE  },  // ASCII
        {  0x39,        0x39,   0x39,  "9",          KNONE  },  // ASCII
        {  0x3a,        0x3a,   0xba,  ":",          KSHIFT },  // VK_OEM_1
        {  0x3b,        0x3b,   0xba,  ";",          KNONE  },  // VK_OEM_1
        {  0x3c,        0x3c,   0xbc,  "<",          KSHIFT },  // Shift-VK_OEM_COMMA
        {  0x3d,        0x3d,   0xbb,  "=",          KNONE  },  // Unshift-VK_OEM_PLUS
        {  0x3e,        0x3e,   0xbe,  ">",          KSHIFT },  // Shift-VK_OEM_PERIOD
        {  0x3f,        0x3f,   0xbf,  "?",          KSHIFT },  // VK_OEM_2
        {  0x40,        0x40,   0x32,  "@",          KSHIFT },  // Shift-2
        {  0x41,        0x41,   0x41,  "A",          KSHIFT },  // Shift key modifier, ASCII
        {  0x42,        0x42,   0x42,  "B",          KSHIFT },  // ASCII
        {  0x43,        0x43,   0x43,  "C",          KSHIFT },  // ASCII
        {  0x44,        0x44,   0x44,  "D",          KSHIFT },  // ASCII
        {  0x45,        0x45,   0x45,  "E",          KSHIFT },  // ASCII
        {  0x46,        0x46,   0x46,  "F",          KSHIFT },  // ASCII
        {  0x47,        0x47,   0x47,  "G",          KSHIFT },  // ASCII
        {  0x48,        0x48,   0x48,  "H",          KSHIFT },  // ASCII
        {  0x49,        0x49,   0x49,  "I",          KSHIFT },  // ASCII
        {  0x4a,        0x4a,   0x4a,  "J",          KSHIFT },  // ASCII
        {  0x4b,        0x4b,   0x4b,  "K",          KSHIFT },  // ASCII
        {  0x4c,        0x4c,   0x4c,  "L",          KSHIFT },  // ASCII
        {  0x4d,        0x4d,   0x4d,  "M",          KSHIFT },  // ASCII
        {  0x4e,        0x4e,   0x4e,  "N",          KSHIFT },  // ASCII
        {  0x4f,        0x4f,   0x4f,  "O",          KSHIFT },  // ASCII
        {  0x50,        0x50,   0x50,  "P",          KSHIFT },  // ASCII
        {  0x51,        0x51,   0x51,  "Q",          KSHIFT },  // ASCII
        {  0x52,        0x52,   0x52,  "R",          KSHIFT },  // ASCII
        {  0x53,        0x53,   0x53,  "S",          KSHIFT },  // ASCII
        {  0x54,        0x54,   0x54,  "T",          KSHIFT },  // ASCII
        {  0x55,        0x55,   0x55,  "U",          KSHIFT },  // ASCII
        {  0x56,        0x56,   0x56,  "V",          KSHIFT },  // ASCII
        {  0x57,        0x57,   0x57,  "W",          KSHIFT },  // ASCII
        {  0x58,        0x58,   0x58,  "X",          KSHIFT },  // ASCII
        {  0x59,        0x59,   0x59,  "Y",          KSHIFT },  // ASCII
        {  0x5a,        0x5a,   0x5a,  "Z",          KSHIFT },  // ASCII
        {  0x5b,        0x5b,   0xdb,  "[",          KNONE  },  // BracketLeft, VK_OEM_4
        {  0x5c,        0x5c,   0xdc,  "\\",         KNONE  },  // Backslash, VK_OEM_5
        {  0x5d,        0x5d,   0xdd,  "]",          KNONE  },  // BracketRight, VK_OEM_6
        {  0x5e,        0x5e,   0x36,  "^",          KSHIFT },  // AsciiCircumflex, Shift-6
        {  0x5f,        0x5f,   0xbd,  "_",          KSHIFT },  // Underscore, Shift-VK_OEM_MINUS
        {  0x60,        0x60,   0xc0,  "`",          KNONE  },  // QuoteLeft, Backtick, VK_OEM_3
        {  0x61,        0x41,   0x41,  "a",          KNONE  },  // ASCII
        {  0x62,        0x42,   0x42,  "b",          KNONE  },  // ASCII
        {  0x63,        0x43,   0x43,  "c",          KNONE  },  // ASCII
        {  0x64,        0x44,   0x44,  "d",          KNONE  },  // ASCII
        {  0x65,        0x45,   0x45,  "e",          KNONE  },  // ASCII
        {  0x66,        0x46,   0x46,  "f",          KNONE  },  // ASCII
        {  0x67,        0x47,   0x47,  "g",          KNONE  },  // ASCII
        {  0x68,        0x48,   0x48,  "h",          KNONE  },  // ASCII
        {  0x69,        0x49,   0x49,  "i",          KNONE  },  // ASCII
        {  0x6a,        0x4a,   0x4a,  "j",          KNONE  },  // ASCII
        {  0x6b,        0x4b,   0x4b,  "k",          KNONE  },  // ASCII
        {  0x6c,        0x4c,   0x4c,  "l",          KNONE  },  // ASCII
        {  0x6d,        0x4d,   0x4d,  "m",          KNONE  },  // ASCII
        {  0x6e,        0x4e,   0x4e,  "n",          KNONE  },  // ASCII
        {  0x6f,        0x4f,   0x4f,  "o",          KNONE  },  // ASCII
        {  0x70,        0x50,   0x50,  "p",          KNONE  },  // ASCII
        {  0x71,        0x51,   0x51,  "q",          KNONE  },  // ASCII
        {  0x72,        0x52,   0x52,  "r",          KNONE  },  // ASCII
        {  0x73,        0x53,   0x53,  "s",          KNONE  },  // ASCII
        {  0x74,        0x54,   0x54,  "t",          KNONE  },  // ASCII
        {  0x75,        0x55,   0x55,  "u",          KNONE  },  // ASCII
        {  0x76,        0x56,   0x56,  "v",          KNONE  },  // ASCII
        {  0x77,        0x57,   0x57,  "w",          KNONE  },  // ASCII
        {  0x78,        0x58,   0x58,  "x",          KNONE  },  // ASCII
        {  0x79,        0x59,   0x59,  "y",          KNONE  },  // ASCII
        {  0x7a,        0x5a,   0x5a,  "z",          KNONE  },  // ASCII
        {  0x7b,        0x7b,   0xdb,  "{",          KSHIFT },  // BraceLeft, VK_OEM_4
        {  0x7c,        0x7c,   0xdc,  "|",          KSHIFT },  // Bar, VK_OEM_5
        {  0x7d,        0x7d,   0xdd,  "}",          KSHIFT },  // BraceRight, VK_OEM_6
        {  0x7e,        0x7e,   0xc0,  "~",          KSHIFT },  // AsciiTilde, Shift-VK_OEM_3
        {  0x7f,        0x7f,   0x7f,  "DEL",        KNONE  },

        /*
         * Block moved from above to here.
         *
         * Ordinal   Key Event Virt Key Name        Modifier
         */

        {  0x80,  0x01000000,   0x1b,  "Esc",        KNONE  },  // VK_ESCAPE again
        {  0x81,  0x01000001,   0x09,  "Tab",        KNONE  },  // avoid, moves focus
        {  0x82,  0x01000002,   0x09,  "BkTab",      KSHIFT },  // avoid, moves focus
        {  0x83,  0x01000003,   0x08,  "BkSpace",    KNONE  },  // differs from Ctrl-H ! VK_BACK again
        {  0x84,  0x01000004,   0x0d,  "Return",     KNONE  },  // VK_RETURN again
        {  0x85,  0x01000005, 0xff8d,  "Enter",      KEYPAD },  // Keypad-Enter
        {  0x86,  0x01000006,   0x2d,  "Ins",        KNONE  },  // VK_INSERT
        {  0x87,  0x01000007,   0x2e,  "Del",        KNONE  },  // VK_DELETE
        {  0x88,        0x88,   0x13,  "0x88",       KNONE  },  // was "Pause", duplicate, VK_PAUSE
        {  0x89,        0x89,   0x89,  "0x89",       KNONE  },  // was "Print", duplicate
        {  0x8a,  0x0100000a,   0x8a,  "SysReq",     KNONE  },
        {  0x8b,  0x0100000b,   0x8b,  "Clear",      KNONE  },
        {  0x8c,  0x0100000c,   0x8c,  "0x8c",       KNONE  },
        {  0x8d,  0x0100000d,   0x8d,  "0x8d",       KNONE  },
        {  0x8e,  0x0100000e,   0x8e,  "0x8e",       KNONE  },
        {  0x8f,  0x0100000f,   0x8f,  "0x8f",       KNONE  },
        {  0x90,  0x01000010,   0x24,  "Home",       KNONE  },  // VK_HOME
        {  0x91,  0x01000011,   0x23,  "End",        KNONE  },  // VK_END
        {  0x92,  0x01000012,   0x25,  "Left",       KNONE  },  // VK_LEFT
        {  0x93,  0x01000013,   0x26,  "Up",         KNONE  },  // VK_UP
        {  0x94,  0x01000014,   0x27,  "Right",      KNONE  },  // VK_RIGHT
        {  0x95,  0x01000015,   0x28,  "Down",       KNONE  },  // VK_DOWN
        {  0x96,  0x01000016,   0x21,  "PageUp",     KNONE  },  // VK_PRIOR
        {  0x97,  0x01000017,   0x22,  "PageDn",     KNONE  },  // VK_NEXT

        /*
         * See starting around 0xd7 for the Right versions of these keys.
         *
         * Ordinal   Key Event Virt Key Name        Modifier
         *
        {  0x98,  0x01000020,   0x10,  "Shift_L",    KSHIFT },  // Left-Shift, VK_SHIFT
        {  0x99,  0x01000021,   0x11,  "Ctrl_L",     KCTRL  },  // Left-Ctrl, VK_CONTROL
           . . .
        {  0x9b,  0x01000023,   0x12,  "Alt_L",      KALT   },  // Left-Alt, VK_MENU
         *
         */

        {  0x98,  0x01000020,   0xa0,  "Shift_L",    KSHIFT },  // Left-Shift, VK_LSHIFT
        {  0x99,  0x01000021,   0xa3,  "Ctrl_L",     KCTRL  },  // Left-Ctrl, VK_LCONTROL
        {  0x9a,  0x01000022,   0x9a,  "Meta",       KMETA  },
        {  0x9b,  0x01000023,   0xa4,  "Alt_L",      KALT   },  // Left-Alt, VK_LMENU
        {  0x9c,  0x01000024,   0x14,  "CapsLk",     KNONE  },  // Shift-Lock, VK_CAPITAL
        {  0x9d,  0x01000025,   0x90,  "NumLk",      KNONE  },  // VK_NUMLOCK
        {  0x9e,  0x01000026,   0x90,  "ScrlLk",     KNONE  },  // VK_SCROLL
        {  0x9f,  0x01000027,   0x9f,  "0x9f",       KNONE  },
        {  0xa0,  0x01000030,   0x70,  "F1",         KNONE  },  // VK_F1
        {  0xa1,  0x01000031,   0x71,  "F2",         KNONE  },  // VK_F2
        {  0xa2,  0x01000032,   0x72,  "F3",         KNONE  },  // VK_F3
        {  0xa3,  0x01000033,   0x73,  "F4",         KNONE  },  // VK_F4
        {  0xa4,  0x01000034,   0x74,  "F5",         KNONE  },  // VK_F5
        {  0xa5,  0x01000035,   0x75,  "F6",         KNONE  },  // VK_F6
        {  0xa6,  0x01000036,   0x76,  "F7",         KNONE  },  // VK_F7
        {  0xa7,  0x01000037,   0x77,  "F8",         KNONE  },  // VK_F8
        {  0xa8,  0x01000038,   0x78,  "F9",         KNONE  },  // VK_F9
        {  0xa9,  0x01000039,   0x79,  "F10",        KNONE  },  // VK_F10
        {  0xaa,  0x0100003a,   0x7a,  "F11",        KNONE  },  // VK_F11
        {  0xab,  0x0100003b,   0x7b,  "F12",        KNONE  },  // VK_F12
        {  0xac,  0x01000053,   0x5b,  "Super_L",    KNONE  },  // Left-Windows, VK_LWIN
        {  0xad,  0x01000054,   0x5c,  "Super_R",    KNONE  },  // Right-Windows, VK_RWIN
        {  0xae,  0x01000055,   0x5d,  "Menu",       KNONE  },  // Win-Menu key, VK_APPS ?
        {  0xaf,  0x01000056,   0xaf,  "Hyper_L",    KNONE  },
        {  0xb0,  0x01000057,   0xb0,  "Hyper_R",    KNONE  },
        {  0xb1,  0x01000058,   0x2f,  "Help",       KNONE  },  // VK_HELP
        {  0xb2,  0x01000059,   0xb2,  "Dir_L",      KNONE  },
        {  0xb3,  0x01000060,   0xb3,  "Dir_R",      KNONE  },  // Direction_R
        {  0xb4,  0x01000030,   0x7c,  "Sh_F1",      KSHIFT },  // VK_F13
        {  0xb5,  0x01000031,   0x7d,  "Sh_F2",      KSHIFT },  // VK_F14
        {  0xb6,  0x01000032,   0x7e,  "Sh_F3",      KSHIFT },  // VK_F15
        {  0xb7,  0x01000033,   0x7f,  "Sh_F4",      KSHIFT },  // VK_F16
        {  0xb8,  0x01000034,   0x80,  "Sh_F5",      KSHIFT },  // VK_F17
        {  0xb9,  0x01000035,   0x81,  "Sh_F6",      KSHIFT },  // VK_F18
        {  0xba,  0x01000036,   0x82,  "Sh_F7",      KSHIFT },  // VK_F19
        {  0xbb,  0x01000037,   0x83,  "Sh_F8",      KSHIFT },  // VK_F20
        {  0xbc,  0x01000038,   0x84,  "Sh_F9",      KSHIFT },  // VK_F21
        {  0xbd,  0x01000039,   0x85,  "Sh_F10",     KSHIFT },  // VK_F22
        {  0xbe,  0x0100003a,   0x86,  "Sh_F11",     KSHIFT },  // VK_F23
        {  0xbf,  0x0100003b,   0x87,  "Sh_F12",     KSHIFT },  // VK_F24

        /*
         *  Keys missing: KP_0 to KP_9, accessible with NumLock on.
         *
        { 0x30,  0x30,       0xffb0,  "KP_0",       KEYPAD },   // VK_NUMPAD0
        { 0x31,  0x31,       0xffb1,  "KP_1",       KEYPAD },   // VK_NUMPAD1
                . . .           . . .           . . .
        { 0x39,  0x39,       0xffb9,  "KP_9",       KEYPAD },   // VK_NUMPAD9
         *
         */

        {  0xc0,  0x01000006, 0xff9e,  "KP_Ins",     KEYPAD },
        {  0xc1,  0x01000007, 0xff9f,  "KP_Del",     KEYPAD },
        {  0xc2,  0x01000008,   0x13,  "Pause",      KSHIFT },  // VK_PAUSE again
        {  0xc3,  0x01000009,   0x2a,  "Print",      KSHIFT },  // VK_PRINT
        {  0xc4,  0x01000010, 0xff95,  "KP_Home",    KEYPAD },
        {  0xc5,  0x01000011, 0xff9c,  "KP_End",     KEYPAD },
        {  0xc6,  0x01000012, 0xff96,  "KP_Left",    KEYPAD },
        {  0xc7,  0x01000013, 0xff97,  "KP_Up",      KEYPAD },
        {  0xc8,  0x01000014, 0xff98,  "KP_Right",   KEYPAD },
        {  0xc9,  0x01000015, 0xff99,  "KP_Down",    KEYPAD },
        {  0xca,  0x01000016, 0xff9a,  "KP_PageUp",  KEYPAD },
        {  0xcb,  0x01000017, 0xff9b,  "KP_PageDn",  KEYPAD },
        {  0xcc,  0x01000099, 0xff9d,  "KP_Begin",   KNONE  },  // KP_Begin
        {  0xcd,  0x01000099,   0xcd,  "0xcd",       KNONE  },
        {  0xce,  0x01000099,   0xce,  "0xce",       KNONE  },
        {  0xcf,  0x01000099,   0xcf,  "0xcf",       KNONE  },
        {  0xd0,        0x2a,   0x6a,  "KP_*",       KEYPAD },  // Asterisk, VK_MULTIPLY
        {  0xd1,        0x2b,   0x6b,  "KP_+",       KEYPAD },  // Plus, VK_ADD
        {  0xd2,        0x2c,   0x6c,  "KP_,",       KEYPAD },  // Comma, VK_SEPARATOr
        {  0xd3,        0x2d,   0x6d,  "KP_-",       KEYPAD },  // Minus, VK_SUBTRACT
        {  0xd4,        0x2e,   0x6e,  "KP_.",    KPADSHIFT },  // Period, VK_DECIMAL
        {  0xd5,        0x2f,   0x6f,  "KP_/",       KEYPAD },  // Slash, VK_DIVIDE

        /*
         *  Remainders.  Provides the Right version and key-release versions
         *  of some keys.  Keys not yet covered:
         *
         *      Alt and Alt_R releases.
         */

        {  0xd6,  0x01000099,   0xd6,  "0xd6",       KNONE  },  // available
        {  0xd7,  0x01000020,   0xa1,  "Shift_R",    KSHIFT },  // Right-Shift, VK_RSHIFT
        {  0xd8,  0x01000021,   0xa3,  "Ctrl_R",     KCTRL  },  // Right-Ctrl, VK_RCONTROL
        {  0xd9,        0x2e, 0xffae,  "KP_.",       KEYPAD },  // KP_Decimal release
        {  0xda,  0x01000023,   0xa4,  "Alt_R",      KGROUP },  // Right-Alt, VK_RMENU
        {  0xdb,  0x01000020, 0xffe1,  "Shift_Lr",   KNONE  },  // L-Shift release
        {  0xdc,  0x01000020, 0xffe2,  "Shift_Rr",   KNONE  },  // R-Shift release
        {  0xdd,  0x01000021, 0xffe3,  "Ctrl_Lr",    KNONE  },  // L-Ctrl release
        {  0xde,  0x01000021, 0xffe4,  "Ctrl_Rr",    KNONE  },  // R-Ctrl release
        {  0xdf,  0x01000099,   0xdf,  "Quit",       KNONE  },  // fake key, MIDI control only

        /*
         * This section is currently useful to fill in for future expansion or
         * for extended ASCII characters.  See setup_qt_azerty_fr_keys().
         */

        {  0xe0,  0x01000099,   0xe0,  "0xe0",       KNONE  },
        {  0xe1,  0x01000099,   0xe1,  "0xe1",       KNONE  },
        {  0xe2,  0x01000099,   0xe2,  "0xe2",       KNONE  },
        {  0xe3,  0x01000099,   0xe3,  "0xe3",       KNONE  },
        {  0xe4,  0x01000099,   0xe4,  "0xe4",       KNONE  },
        {  0xe5,  0x01000099,   0xe5,  "0xe5",       KNONE  },
        {  0xe6,  0x01000099,   0xe6,  "0xe6",       KNONE  },
        {  0xe7,  0x01000099,   0xe7,  "0xe7",       KNONE  },
        {  0xe8,  0x01000099,   0xe8,  "0xe8",       KNONE  },
        {  0xe9,  0x01000099,   0xe9,  "0xe9",       KNONE  },
        {  0xea,  0x01000099,   0xea,  "0xea",       KNONE  },
        {  0xeb,  0x01000099,   0xeb,  "0xeb",       KNONE  },
        {  0xec,  0x01000099,   0xec,  "0xec",       KNONE  },
        {  0xed,  0x01000099,   0xed,  "0xed",       KNONE  },
        {  0xee,  0x01000099,   0xee,  "0xee",       KNONE  },
        {  0xef,  0x01000099,   0xef,  "0xef",       KNONE  },
        {  0xf0,  0x01000099,   0xf0,  "0xf0",       KNONE  },
        {  0xf1,  0x01000099,   0xf1,  "0xf1",       KNONE  },
        {  0xf2,  0x01000099,   0xf2,  "0xf2",       KNONE  },
        {  0xf3,  0x01000099,   0xf3,  "0xf3",       KNONE  },
        {  0xf4,  0x01000099,   0xf4,  "0xf4",       KNONE  },
        {  0xf5,  0x01000099,   0xf5,  "0xf5",       KNONE  },
        {  0xf6,  0x01000099,   0xf6,  "0xf6",       KNONE  },
        {  0xf7,  0x01000099,   0xf7,  "0xf7",       KNONE  },
        {  0xf8,  0x01000099,   0xf8,  "0xf8",       KNONE  },
        {  0xf9,  0x01000099,   0xf9,  "0xf9",       KNONE  },
        {  0xfa,  0x01000099,   0xfa,  "0xfa",       KNONE  },
        {  0xfb,  0x01000099,   0xfb,  "0xfb",       KNONE  },
        {  0xfc,  0x01000099,   0xfc,  "0xfc",       KNONE  },
        {  0xfd,  0x01000099,   0xfd,  "0xfd",       KNONE  },
        {  0xfe,  0x01000099,   0xfe,  "0xfe",       KNONE  },
        {  0xff,  0xffffffff,   0xff,  "Null_ff",    KNONE  }   // end-of-list
    };

    if (i < 0 || i > 0xff)
        i = 0;

    return s_qt_keys[i];
}

#endif          // SEQ66_WINKEYS_HPP

/*
 * winkeys.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


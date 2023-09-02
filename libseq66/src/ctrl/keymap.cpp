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
 * \file          keymap.cpp
 *
 *  This module defines some informative functions that are actually
 *  better off as functions.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-12
 * \updates       2023-09-02
 * \license       GNU GPLv2 or above
 */

#include <map>                          /* std::map and std::multimap       */

#include "ctrl/keymap.hpp"              /* keymap function declarations     */
#include "util/strfunctions.hpp"        /* contains()                       */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

std::string
modifier_names (unsigned kmod)
{
    std::string result;
    if (kmod == keyboard::KNONE)
    {
        result = "None ";
    }
    else
    {
        if (kmod & keyboard::KSHIFT)
            result += "Shift ";

        if (kmod & keyboard::KCTRL)
            result += "Ctrl ";

        if (kmod & keyboard::KALT)
            result += "Alt ";

        if (kmod & keyboard::KMETA)
            result += "Meta ";

        if (kmod & keyboard::KEYPAD)
            result += "Keypad ";

        if (kmod & keyboard::KGROUP)
            result += "Group ";
    }
    return result;
}

unsigned
modifier_code (const std::string & name)
{
    unsigned result = keyboard::KNONE;
    if (contains(name, "Shift"))
        result |= keyboard::KSHIFT;

    if (contains(name, "Ctrl"))
        result |= keyboard::KCTRL;

    if (contains(name, "Alt"))
        result |= keyboard::KALT;

    if (contains(name, "Alt-Gr"))
        result |= keyboard::KCTRLALT;

    if (contains(name, "Keypad"))
        result |= keyboard::KEYPAD;

    if (contains(name, "Meta"))
        result |= keyboard::KMETA;

    if (contains(name, "Group"))
        result |= keyboard::KGROUP;

    return result;
}

ctrlkey
arrow_left ()
{
    return 0x92;
}

ctrlkey
arrow_up ()
{
    return 0x93;
}

ctrlkey
arrow_right ()
{
    return 0x94;
}

ctrlkey
arrow_down ()
{
    return 0x95;
}

/**
 *  Indicates the start of section where expansion/foreign characters can be
 *  placed.  Covers the ordinals from 0xe0 to 0xfe (31 characters).
 */

static const int s_expansion_start = 0xe0;

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
     *
     *  These ordinals are key-values in keycontainer::keymap for fast lookup
     *  of the keys the application actually loaded from the 'ctrl' file at
     *  startup.
     */

    ctrlkey qtk_ordinal;

    /**
     *  Provides the incoming key value presented to the application by Qt, as
     *  obtained by the QKeyEvent :: key() function.  It is important to note
     *  that this is NOT the ASCII value of the character.
     */

    eventkey qtk_keyevent;

    /**
     *  Provides the keycode for the key, as obtained by the QKeyEvent ::
     *  nativeVirtualKey() function.  However, see this link:
     *
     *  https://forum.qt.io/topic/95527/
     *      dealing-with-keyboard-layouts-for-input-on-multiple-platforms
     *
     *      "Each platform has a different set of virtual key codes. For
     *      Windows, the list can be found here. For mac, it's in Events.h. As
     *      a simple comparison to demonstrate that they are different, the
     *      virtual key code for the letter key O on Windows is 0x4F, while on
     *      mac the same key's code is 0x1F. Not to mention, I couldn't even
     *      find any relevant info for Linux.
     *
     *      This makes it impossible to set good default settings that would
     *      work on all platforms, forcing me to create separate defaults for
     *      each platform (unlike key())."
     *
     *  But key() is layout-dependent!
     */

    eventkey qtk_virtkey;

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

    /* unsigned */ keyboard::modifiers qtk_modifier;
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
 *      -   Code:           qtk_ordinal     [legal range is 0x01 to 0xfe]
 *      -   Qt Key Event:   qtk_keyevent    [QKeyEvent::key(), unsigned]
 *      -   Qt Virtual Key: qtk_virtkey     [QKeyEvent::nativeVirtualKey(), unsigned]
 *      -   Key Name:       qtk_keyname
 *      -   Modifier:       qtk_modifiers   [QKeyEvent::modifiers(), unsigned]
 *
 *  By using the modifier, we can distinguish between various version of the
 *  same key, such as the normal PageDn and the keypad's PageDn.
 *
 *  A number of these keys (e.g. the keypad keys) are defined in Seq64's
 *  gdk_basic_keys.h file, which we do not use here.
 *
 *  Also note that we restrict the keystroke names to 9 characters or less, to
 *  keep the MIDI control section readable.
 *
 *  Also note that, currently, "Quit" is a fake key, a placeholder for
 *  the new MIDI control for quitting the application (especially when running
 *  headless.
 *
 *  For alternate keyboards such as AZERTY, we have many characters in the
 *  Extened ASCII (code >= 0x80) set, besides the usual items (e.g. keypad
 *  characters) that Qt provides.  And, because the exact character can, as
 *  far as we know, depend on the code page, we cannot use the extended
 *  character itself as the name of the character.  Instead, we use the hex
 *  code (e.g. "0xe9") as the name of the character.  This affects the display
 *  of pattern and mute group keys as obtained by keycontainer::slot_key() and
 *  mute_key().
 *
 *  This array can be modified at run-time.  See setup_qt_azerty_fr_keys(),
 *  for example.
 *
 *  Finally, we started encountering uninitialized static variables.  A g++
 *  bug, or bug fix?  So now we enforce initialization by wrapping static
 *  values in accessor functions.
 */

#if defined SEQ66_PLATFORM_WINDOWS

#include "winkeys.hpp"                  /* provisional Windows key mapping  */

#else

static qt_keycodes &
qt_keys (int i)
{
    using namespace keyboard;
    static qt_keycodes s_qt_keys [] =   /* modifiable inside this module    */
    {
        /*
         *  Code        Qt      Qt      Key
         * Ordinal   Key Event Virt Key Name        Modifier
         *
         * Ctrl-key section. It is best to avoid using Control keys to control
         * loops, mutes, and automation.  Too much chance of interfering with
         * the normal user interface.
         */

        {  0x00,        0x40,   0x60,  "NUL",        KCTRL  }, // ^@: Null
        {  0x01,        0x41,   0x61,  "SOH",        KCTRL  }, // ^A: Start of Heading
        {  0x02,        0x42,   0x62,  "STX",        KCTRL  }, // ^B: Start of Text
        {  0x03,        0x43,   0x63,  "ETX",        KCTRL  }, // ^C: End of Text
        {  0x04,        0x44,   0x64,  "EOT",        KCTRL  }, // ^D: End of Transmision
        {  0x05,        0x45,   0x65,  "ENQ",        KCTRL  }, // ^E: Enquiry
        {  0x06,        0x46,   0x66,  "ACK",        KCTRL  }, // ^F: Acknowledge
        {  0x07,        0x47,   0x67,  "BEL",        KCTRL  }, // ^G: Bell/beep
        {  0x08,        0x48,   0x68,  "BS",         KCTRL  }, // ^H: Backspace
        {  0x09,        0x49,   0x69,  "HT",         KCTRL  }, // ^I: Horizontal Tab
        {  0x0a,        0x4a,   0x6a,  "LF",         KCTRL  }, // ^J: Line Feed
        {  0x0b,        0x4b,   0x6b,  "VT",         KCTRL  }, // ^K: Vertical Tab
        {  0x0c,        0x4c,   0x6c,  "FF",         KCTRL  }, // ^L: Form Feed
        {  0x0d,        0x4d,   0x6d,  "CR",         KCTRL  }, // ^M: Carriage Return
        {  0x0e,        0x4e,   0x6e,  "SO",         KCTRL  }, // ^N: Shift Out
        {  0x0f,        0x4f,   0x6f,  "SI",         KCTRL  }, // ^O: Shift In
        {  0x10,        0x50,   0x70,  "DLE",        KCTRL  }, // ^P: Data Link Escape
        {  0x11,        0x51,   0x71,  "DC1",        KCTRL  }, // ^Q: Device Control 1
        {  0x12,        0x52,   0x72,  "DC2",        KCTRL  }, // ^R: Device Control 2
        {  0x13,        0x53,   0x73,  "DC3",        KCTRL  }, // ^S: Device Control 3
        {  0x14,        0x54,   0x74,  "DC4",        KCTRL  }, // ^T: Device Control 4
        {  0x15,        0x55,   0x75,  "NAK",        KCTRL  }, // ^U: Negative ACK
        {  0x16,        0x56,   0x76,  "SYN",        KCTRL  }, // ^V: Synchronous Idle
        {  0x17,        0x57,   0x77,  "ETB",        KCTRL  }, // ^W: End of Trans Block
        {  0x18,        0x58,   0x78,  "CAN",        KCTRL  }, // ^X: Cancel
        {  0x19,        0x59,   0x79,  "EM",         KCTRL  }, // ^Y: End of Medium
        {  0x1a,        0x5a,   0x7a,  "SUB",        KCTRL  }, // ^Z: Substitute
        {  0x1b,        0x5b,   0x7b,  "ESC",        KCTRL  }, // ^[: Escape
        {  0x1c,        0x5c,   0x7c,  "FS",         KCTRL  }, // ^\: File Separator
        {  0x1d,        0x5d,   0x7d,  "GS",         KCTRL  }, // ^]: Group Separator
        {  0x1e,        0x5e,   0x7e,  "RS",    KCTRLSHIFT  }, // ^^: Record Separator
        {  0x1f,        0x5f,   0x7f,  "US",    KCTRLSHIFT  }, // ^_???: Unit Separator

        /*
         * Ordinal   Key Event Virt Key Name        Modifier
         */

        {  0x20,        0x20,   0x20,  "Space",      KNONE  }, // Space (" " not good)
        {  0x21,        0x21,   0x21,  "!",          KSHIFT }, // Exclam
        {  0x22,        0x22,   0x22,  "\"",         KSHIFT }, // QuoteDbl
        {  0x23,        0x23,   0x23,  "#",          KSHIFT }, // NumberSign
        {  0x24,        0x24,   0x24,  "$",          KSHIFT }, // Dollar
        {  0x25,        0x25,   0x25,  "%",          KSHIFT }, // Percent
        {  0x26,        0x26,   0x26,  "&",          KSHIFT }, // Ampersand
        {  0x27,        0x27,   0x27,  "'",          KSHIFT }, // Apostrophe
        {  0x28,        0x28,   0x28,  "(",          KSHIFT }, // ParenLeft
        {  0x29,        0x29,   0x29,  ")",          KSHIFT }, // ParenRight
        {  0x2a,        0x2a,   0x2a,  "*",          KSHIFT }, // Asterisk
        {  0x2b,        0x2b,   0x2b,  "+",          KSHIFT }, // Plus
        {  0x2c,        0x2c,   0x2c,  ",",          KNONE  }, // Comma
        {  0x2d,        0x2d,   0x2d,  "-",          KNONE  }, // Minus
        {  0x2e,        0x2e,   0x2e,  ".",          KNONE  }, // Period
        {  0x2f,        0x2f,   0x2f,  "/",          KNONE  }, // Slash
        {  0x30,        0x30,   0x30,  "0",          KNONE  },
        {  0x31,        0x31,   0x31,  "1",          KNONE  },
        {  0x32,        0x32,   0x32,  "2",          KNONE  },
        {  0x33,        0x33,   0x33,  "3",          KNONE  },
        {  0x34,        0x34,   0x34,  "4",          KNONE  },
        {  0x35,        0x35,   0x35,  "5",          KNONE  },
        {  0x36,        0x36,   0x36,  "6",          KNONE  },
        {  0x37,        0x37,   0x37,  "7",          KNONE  },
        {  0x38,        0x38,   0x38,  "8",          KNONE  },
        {  0x39,        0x39,   0x39,  "9",          KNONE  },
        {  0x3a,        0x3a,   0x3a,  ":",          KSHIFT },
        {  0x3b,        0x3b,   0x3b,  ";",          KNONE  },
        {  0x3c,        0x3c,   0x3c,  "<",          KSHIFT },
        {  0x3d,        0x3d,   0x3d,  "=",          KNONE  },
        {  0x3e,        0x3e,   0x3e,  ">",          KSHIFT },
        {  0x3f,        0x3f,   0x3f,  "?",          KSHIFT },
        {  0x40,        0x40,   0x40,  "@",          KSHIFT },
        {  0x41,        0x41,   0x41,  "A",          KSHIFT }, // Shift key modifier
        {  0x42,        0x42,   0x42,  "B",          KSHIFT },
        {  0x43,        0x43,   0x43,  "C",          KSHIFT },
        {  0x44,        0x44,   0x44,  "D",          KSHIFT },
        {  0x45,        0x45,   0x45,  "E",          KSHIFT },
        {  0x46,        0x46,   0x46,  "F",          KSHIFT },
        {  0x47,        0x47,   0x47,  "G",          KSHIFT },
        {  0x48,        0x48,   0x48,  "H",          KSHIFT },
        {  0x49,        0x49,   0x49,  "I",          KSHIFT },
        {  0x4a,        0x4a,   0x4a,  "J",          KSHIFT },
        {  0x4b,        0x4b,   0x4b,  "K",          KSHIFT },
        {  0x4c,        0x4c,   0x4c,  "L",          KSHIFT },
        {  0x4d,        0x4d,   0x4d,  "M",          KSHIFT },
        {  0x4e,        0x4e,   0x4e,  "N",          KSHIFT },
        {  0x4f,        0x4f,   0x4f,  "O",          KSHIFT },
        {  0x50,        0x50,   0x50,  "P",          KSHIFT },
        {  0x51,        0x51,   0x51,  "Q",          KSHIFT },
        {  0x52,        0x52,   0x52,  "R",          KSHIFT },
        {  0x53,        0x53,   0x53,  "S",          KSHIFT },
        {  0x54,        0x54,   0x54,  "T",          KSHIFT },
        {  0x55,        0x55,   0x55,  "U",          KSHIFT },
        {  0x56,        0x56,   0x56,  "V",          KSHIFT },
        {  0x57,        0x57,   0x57,  "W",          KSHIFT },
        {  0x58,        0x58,   0x58,  "X",          KSHIFT },
        {  0x59,        0x59,   0x59,  "Y",          KSHIFT },
        {  0x5a,        0x5a,   0x5a,  "Z",          KSHIFT },
        {  0x5b,        0x5b,   0x5b,  "[",          KNONE  }, // BracketLeft
        {  0x5c,        0x5c,   0x5c,  "\\",         KNONE  }, // Backslash
        {  0x5d,        0x5d,   0x5d,  "]",          KNONE  }, // BracketRight
        {  0x5e,        0x5e,   0x5e,  "^",          KSHIFT }, // AsciiCircumflex
        {  0x5f,        0x5f,   0x5f,  "_",          KSHIFT }, // Underscore
        {  0x60,        0x60,   0x60,  "`",          KNONE  }, // QuoteLeft, Backtick
        {  0x61,        0x41,   0x61,  "a",          KNONE  },
        {  0x62,        0x42,   0x62,  "b",          KNONE  },
        {  0x63,        0x43,   0x63,  "c",          KNONE  },
        {  0x64,        0x44,   0x64,  "d",          KNONE  },
        {  0x65,        0x45,   0x65,  "e",          KNONE  },
        {  0x66,        0x46,   0x66,  "f",          KNONE  },
        {  0x67,        0x47,   0x67,  "g",          KNONE  },
        {  0x68,        0x48,   0x68,  "h",          KNONE  },
        {  0x69,        0x49,   0x69,  "i",          KNONE  },
        {  0x6a,        0x4a,   0x6a,  "j",          KNONE  },
        {  0x6b,        0x4b,   0x6b,  "k",          KNONE  },
        {  0x6c,        0x4c,   0x6c,  "l",          KNONE  },
        {  0x6d,        0x4d,   0x6d,  "m",          KNONE  },
        {  0x6e,        0x4e,   0x6e,  "n",          KNONE  },
        {  0x6f,        0x4f,   0x6f,  "o",          KNONE  },
        {  0x70,        0x50,   0x50,  "p",          KNONE  },
        {  0x71,        0x51,   0x71,  "q",          KNONE  },
        {  0x72,        0x52,   0x72,  "r",          KNONE  },
        {  0x73,        0x53,   0x73,  "s",          KNONE  },
        {  0x74,        0x54,   0x74,  "t",          KNONE  },
        {  0x75,        0x55,   0x75,  "u",          KNONE  },
        {  0x76,        0x56,   0x76,  "v",          KNONE  },
        {  0x77,        0x57,   0x77,  "w",          KNONE  },
        {  0x78,        0x58,   0x78,  "x",          KNONE  },
        {  0x79,        0x59,   0x79,  "y",          KNONE  },
        {  0x7a,        0x5a,   0x7a,  "z",          KNONE  },
        {  0x7b,        0x7b,   0x7b,  "{",          KSHIFT }, // BraceLeft
        {  0x7c,        0x7c,   0x7c,  "|",          KSHIFT }, // Bar
        {  0x7d,        0x7d,   0x7d,  "}",          KSHIFT }, // BraceRight
        {  0x7e,        0x7e,   0x7e,  "~",          KSHIFT }, // AsciiTilde
        {  0x7f,        0x7f,   0x7f,  "DEL",        KNONE  },

        /*
         * Block moved from above to here.
         *
         * Ordinal   Key Event Virt Key Name        Modifier
         */

        {  0x80,  0x01000000, 0xff1b,  "Esc",        KNONE  },
        {  0x81,  0x01000001, 0xff09,  "Tab",        KNONE  }, // avoid, moves focus
        {  0x82,  0x01000002, 0xff09,  "BkTab",      KSHIFT }, // avoid, moves focus
        {  0x83,  0x01000003, 0xff08,  "BkSpace",    KNONE  }, // differs from Ctrl-H !
        {  0x84,  0x01000004, 0xff0d,  "Return",     KNONE  },
        {  0x85,  0x01000005, 0xff8d,  "Enter",      KEYPAD }, // Keypad-Enter
        {  0x86,  0x01000006, 0xff63,  "Ins",        KNONE  },
        {  0x87,  0x01000007, 0xffff,  "Del",        KNONE  },
        {  0x88,        0x88,   0x88,  "0x88",       KNONE  }, // was "Pause", duplicate
        {  0x89,        0x89,   0x89,  "0x89",       KNONE  }, // was "Print", duplicate
        {  0x8a,  0x0100000a,   0x8a,  "SysReq",     KNONE  },
        {  0x8b,  0x0100000b,   0x8b,  "Clear",      KNONE  },
        {  0x8c,  0x0100000c,   0x8c,  "0x8c",       KNONE  },
        {  0x8d,  0x0100000d,   0x8d,  "0x8d",       KNONE  },
        {  0x8e,  0x0100000e,   0x8e,  "0x8e",       KNONE  },
        {  0x8f,  0x0100000f,   0x8f,  "0x8f",       KNONE  },
        {  0x90,  0x01000010, 0xff50,  "Home",       KNONE  },
        {  0x91,  0x01000011, 0xff57,  "End",        KNONE  },
        {  0x92,  0x01000012, 0xff51,  "Left",       KNONE  },
        {  0x93,  0x01000013, 0xff52,  "Up",         KNONE  },
        {  0x94,  0x01000014, 0xff53,  "Right",      KNONE  },
        {  0x95,  0x01000015, 0xff54,  "Down",       KNONE  },
        {  0x96,  0x01000016, 0xff55,  "PageUp",     KNONE  },
        {  0x97,  0x01000017, 0xff56,  "PageDn",     KNONE  },

        /*
         * See starting around 0xd7 for the Right versions of these keys.
         *
         * Ordinal   Key Event Virt Key Name        Modifier
         */

        {  0x98,  0x01000020, 0xffe1,  "Shift_L",    KSHIFT }, // Left-Shift
        {  0x99,  0x01000021, 0xffe3,  "Ctrl_L",     KCTRL  }, // Left-Ctrl
        {  0x9a,  0x01000022,   0x9a,  "Meta",       KMETA  },
        {  0x9b,  0x01000023, 0xffe9,  "Alt_L",      KALT   }, // Left-Alt
        {  0x9c,  0x01000024, 0xffe5,  "CapsLk",     KNONE  }, // Shift-Lock too???
        {  0x9d,  0x01000025, 0xff7f,  "NumLk",      KNONE  },
        {  0x9e,  0x01000026, 0xff14,  "ScrlLk",     KNONE  }, // Good?
        {  0x9f,  0x01000027,   0x9f,  "0x9f",       KNONE  },
        {  0xa0,  0x01000030, 0xffbe,  "F1",         KNONE  },
        {  0xa1,  0x01000031, 0xffbf,  "F2",         KNONE  },
        {  0xa2,  0x01000032, 0xffc0,  "F3",         KNONE  },
        {  0xa3,  0x01000033, 0xffc1,  "F4",         KNONE  },
        {  0xa4,  0x01000034, 0xffc2,  "F5",         KNONE  },
        {  0xa5,  0x01000035, 0xffc3,  "F6",         KNONE  },
        {  0xa6,  0x01000036, 0xffc4,  "F7",         KNONE  },
        {  0xa7,  0x01000037, 0xffc5,  "F8",         KNONE  },
        {  0xa8,  0x01000038, 0xffc6,  "F9",         KNONE  },
        {  0xa9,  0x01000039, 0xffc7,  "F10",        KNONE  },
        {  0xaa,  0x0100003a, 0xffc8,  "F11",        KNONE  },
        {  0xab,  0x0100003b, 0xffc9,  "F12",        KNONE  },
        {  0xac,  0x01000053, 0xffeb,  "Super_L",    KNONE  }, // Left-Windows
        {  0xad,  0x01000054, 0xffec,  "Super_R",    KNONE  }, // Right-Windows
        {  0xae,  0x01000055, 0xff67,  "Menu",       KNONE  }, // Win-Menu key
        {  0xaf,  0x01000056,   0xaf,  "Hyper_L",    KNONE  },
        {  0xb0,  0x01000057,   0xb0,  "Hyper_R",    KNONE  },
        {  0xb1,  0x01000058,   0xb1,  "Help",       KNONE  },
        {  0xb2,  0x01000059,   0xb2,  "Dir_L",      KNONE  },
        {  0xb3,  0x01000060,   0xb3,  "Dir_R",      KNONE  }, // Direction_R
        {  0xb4,  0x01000030, 0xffbe,  "Sh_F1",      KSHIFT },
        {  0xb5,  0x01000031, 0xffbf,  "Sh_F2",      KSHIFT },
        {  0xb6,  0x01000032, 0xffc0,  "Sh_F3",      KSHIFT },
        {  0xb7,  0x01000033, 0xffc1,  "Sh_F4",      KSHIFT },
        {  0xb8,  0x01000034, 0xffc2,  "Sh_F5",      KSHIFT },
        {  0xb9,  0x01000035, 0xffc3,  "Sh_F6",      KSHIFT },
        {  0xba,  0x01000036, 0xffc4,  "Sh_F7",      KSHIFT },
        {  0xbb,  0x01000037, 0xffc5,  "Sh_F8",      KSHIFT },
        {  0xbc,  0x01000038, 0xffc6,  "Sh_F9",      KSHIFT },
        {  0xbd,  0x01000039, 0xffc7,  "Sh_F10",     KSHIFT },
        {  0xbe,  0x0100003a, 0xffc8,  "Sh_F11",     KSHIFT },
        {  0xbf,  0x0100003b, 0xffc9,  "Sh_F12",     KSHIFT },

        /*
         *  Keys missing: KP_0 to KP_9, accessible with NumLock on.
         *
        { 0x30,  0x30,       0xffb0,  "KP_0",       KEYPAD },
        { 0x31,  0x31,       0xffb1,  "KP_0",       KEYPAD },
                . . .           . . .           . . .
        { 0x39,  0x39,       0xffb9,  "KP_0",       KEYPAD },
         *
         */

        {  0xc0,  0x01000006, 0xff9e,  "KP_Ins",     KEYPAD },
        {  0xc1,  0x01000007, 0xff9f,  "KP_Del",     KEYPAD },
        {  0xc2,  0x01000008, 0xffe1,  "Pause",      KSHIFT },
        {  0xc3,  0x01000009, 0xff61,  "Print",      KSHIFT },
        {  0xc4,  0x01000010, 0xff95,  "KP_Home",    KEYPAD },
        {  0xc5,  0x01000011, 0xff9c,  "KP_End",     KEYPAD },
        {  0xc6,  0x01000012, 0xff96,  "KP_Left",    KEYPAD },
        {  0xc7,  0x01000013, 0xff97,  "KP_Up",      KEYPAD },
        {  0xc8,  0x01000014, 0xff98,  "KP_Right",   KEYPAD },
        {  0xc9,  0x01000015, 0xff99,  "KP_Down",    KEYPAD },
        {  0xca,  0x01000016, 0xff9a,  "KP_PageUp",  KEYPAD },
        {  0xcb,  0x01000017, 0xff9b,  "KP_PageDn",  KEYPAD },
        {  0xcc,  0x01000099, 0xff9d,  "KP_Begin",   KNONE  }, // KP_Begin
        {  0xcd,  0x01000099,   0xcd,  "0xcd",       KNONE  },
        {  0xce,  0x01000099,   0xce,  "0xce",       KNONE  },
        {  0xcf,  0x01000099,   0xcf,  "0xcf",       KNONE  },
        {  0xd0,        0x2a, 0xffaa,  "KP_*",       KEYPAD }, // Asterisk, KP_Multiply
        {  0xd1,        0x2b, 0xffab,  "KP_+",       KEYPAD }, // Plus, KP_Add
        {  0xd2,        0x2c, 0xffac,  "KP_,",       KEYPAD }, // Comma, KP_Separator
        {  0xd3,        0x2d, 0xffad,  "KP_-",       KEYPAD }, // Minus, KP_Subtract
        {  0xd4,        0x2e, 0xffae,  "KP_.",    KPADSHIFT }, // Period, KP_Decimal
        {  0xd5,        0x2f, 0xffaf,  "KP_/",       KEYPAD }, // Slash, KP_Divide

        /*
         *  Remainders.  Provides the Right version and key-release versions
         *  of some keys.  Keys not yet covered:
         *
         *      Alt and Alt_R releases.
         */

        {  0xd6,  0x01000099,   0xd6,  "0xd6",       KNONE  }, // available
        {  0xd7,  0x01000020, 0xffe2,  "Shift_R",    KSHIFT }, // Right-Shift
        {  0xd8,  0x01000021, 0xffe4,  "Ctrl_R",     KCTRL  }, // Right-Ctrl
        {  0xd9,        0x2e, 0xffae,  "KP_.",       KEYPAD }, // KP_Decimal release
        {  0xda,  0x01000023, 0xffea,  "Alt_R",      KGROUP }, // Right-Alt
        {  0xdb,  0x01000020, 0xffe1,  "Shift_Lr",   KNONE  }, // L-Shift release
        {  0xdc,  0x01000020, 0xffe2,  "Shift_Rr",   KNONE  }, // R-Shift release
        {  0xdd,  0x01000021, 0xffe3,  "Ctrl_Lr",    KNONE  }, // L-Ctrl release
        {  0xde,  0x01000021, 0xffe4,  "Ctrl_Rr",    KNONE  }, // R-Ctrl release
        {  0xdf,  0x01000099,   0xdf,  "Quit",       KNONE  }, // fake key, MIDI control only

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

/**
 *  Extended keys, running on system locale with French (fr) AZERTY keymap.
 *  The first number is the value returned by QKeyEvent::key().  The second
 *  column is the name returned by QKeyEvent::text().  The scan code is
 *  returned by QKeyEvent::nativeScanCode(), but is not used in this table.
 *  The keycode is returned by QKeyEvent::nativeVirtualKey(), and is used to
 *  look up the s_qt_keys[] slot that will be replaced by the information
 *  below.
 *
 *  Note that these code are what we found on Debian Linux after running
 *  "setxkbmap fr".  All of the standard ASCII symbols are unchanged with this
 *  mapping, even though their locations on the French keyboard are different,
 *  and numeric-key shifting is reversed compared to the US key-map.
 *
 *  Also note that we cannot place the characters themselves in a 'ctrl' file.
 *  When read, these characters have the value 0xffffffc2 or 0xffffffc3.  So
 *  we have to make up names to use in the 'ctrl' file.
 *
 * Sylvain's findings on a real AZERTY keyboard:
 *
\verbatim
        Key #0xa3   '£' scan = 0x23; keycode = 0xa3
        Key #0xa4   '¤' scan = 0x23; keycode = 0xa4
        Key #0xa7   '§' scan = 0x3d; keycode = 0xa7
        Key #0xb0   '°' scan = 0x14; keycode = 0xb0
        Key #0xb2   '²' scan = 0x31; keycode = 0xb2
        Key #0xc0   'à' scan = 0x13; keycode = 0xe0
        Key #0xc7   'ç' scan = 0x12; keycode = 0xe7
        Key #0xc8   'è' scan = 0x10; keycode = 0xe8
        Key #0xc9   'é' scan = 0x0b; keycode = 0xe9
        Key #0xd9   'ù' scan = 0x30; keycode = 0xf9
        Key #0x039c  'µ' scan = 0x33; keycode = 0xb5
        Key #0x20ac '€' scan = 0x1a; keycode = 0x20ac
        Key #0x1001252 '^' scan = 0x22; keycode = 0xfe52
        Key #0x1001257 '¨' scan = 0x22; keycode = 0xfe57
\endverbatim
 *
 *  Just call this function once.
 */

static void
setup_qt_azerty_fr_keys ()
{
    using namespace keyboard;
    static const qt_keycodes s_fr_keys [] =
    {
        /*
         *  Code     Qt      Qt      Key
         * Ordinal Evkey  Virtkey   Name        Modifier
         */

        { 0x21,   0x21,     0x21,   "!",         KNONE  }, // Exclam
        { 0x22,   0x22,     0x22,   "\"",        KNONE  }, // QuoteDbl
        { 0x23,   0x23,     0x23,   "#",         KALTGR }, // NumberSign
        { 0x26,   0x26,     0x26,   "&",         KNONE  }, // Ampersand
        { 0x27,   0x27,     0x27,   "'",         KNONE  }, // Apostrophe
        { 0x28,   0x28,     0x28,   "(",         KNONE  }, // ParenLeft
        { 0x29,   0x29,     0x29,   ")",         KNONE  }, // ParenRight
        { 0x2a,   0x2a,     0x2a,   "*",         KNONE  }, // Asterisk
        { 0x2e,   0x2e,     0x2e,   ".",         KSHIFT }, // Period
        { 0x2f,   0x2f,     0x2f,   "/",         KSHIFT }, // Slash
        { 0x3a,   0x3a,     0x3a,   ":",         KNONE  }, // Colon
        { 0x3c,   0x3c,     0x3c,   "<",         KNONE  },
        { 0x40,   0x40,     0x40,   "@",         KALTGR }, // AtSign
        { 0x5b,   0x5b,     0x5b,   "[",         KALTGR }, // BracketLeft
        { 0x5c,   0x5c,     0x5c,   "\\",        KALTGR }, // Backslash
        { 0x5d,   0x5d,     0x5d,   "]",         KALTGR }, // BracketRight
        { 0x5e,   0x5e,     0x5e,   "^",         KALTGR }, // AsciiCircumflex
        { 0x5f,   0x5f,     0x5f,   "_",         KNONE  }, // Underscore
        { 0x60,   0x60,     0x60,   "`",         KALTGR }, // QuoteLeft, Backtick
        { 0x7b,   0x7b,     0x7b,   "{",         KALTGR }, // BraceLeft
        { 0x7c,   0x7c,     0x7c,   "|",         KALTGR }, // Bar
        { 0x7d,   0x7d,     0x7d,   "}",         KALTGR }, // BraceRight
        { 0x7e,   0x7e,     0x7e,   "~",         KALTGR }, // Tilde (dead key)
        { 0xe0,   0xa3,     0xa3,   "L_pound",   KNONE  }, // £ <--F4
        { 0xe1,   0xa4,     0xa4,   "Currency",  KALTGR }, // ¤ <--F5
        { 0xe2,   0xa7,     0xa7,   "Silcrow",   KSHIFT }, // § <--F8
        { 0xe3,   0xb0,     0xb0,   "Degrees",   KSHIFT }, // ° <--Hyper_R
        { 0xe4, 0x01000022, 0xffec, "Super_2",   KMETA  }, // ² <--Dir_L press
        { 0xe5,   0xc0,     0xe0,   "a_grave",   KNONE  }, // à <--KP_Ins
        { 0xe6,   0xc7,     0xe7,   "c_cedilla", KNONE  }, // ç <--KP_Up
        { 0xe7,   0xc8,     0xe8,   "e_grave",   KNONE  }, // è <--KP_Right
        { 0xe8,   0xc9,     0xe9,   "e_acute",   KNONE  }, // é <--KP_Down
        { 0xe9,   0xd9,     0xf9,   "u_grave",   KNONE  }, // ù <--Super/Mod4/Win
        { 0xea,   0x039c,   0xb5,   "Mu",        KSHIFT }, // µ <--(new)
        { 0xeb,   0x20ac,   0xb6,   "Euro",      KALTGR }, // € <--(new)
        { 0xec, 0x1001252,  0xfe52, "Circflex",  KNONE  }, // ^ <--Caret
        { 0xed, 0x1001257,  0xfe57, "Umlaut",    KSHIFT }, // ¨ <--Diaeresis
        { 0xee, 0x01000022, 0xffec, "Super_2r",  KNONE  }, // ² <--Dir_L release
        { 0x00, 0xffffffff, 0xff,   "??",        KNONE  }  // terminator
    };
    for (int i = 0; s_fr_keys[i].qtk_keyevent != 0xffffffff; ++i)
    {
        int index = s_fr_keys[i].qtk_ordinal;           // not qtk_keyevent

#if defined SEQ66_PLATFORM_DEBUG_TMI
        printf
        (
            "Key #%03d '%s': Ord %02x; code %02x-->",
            i, qt_keys(index).qtk_keyname.c_str(),
            qt_keys(index).qtk_ordinal, qt_keys(index).qtk_keyevent
        );
#endif

        qt_keys(index) = s_fr_keys[i];

#if defined SEQ66_PLATFORM_DEBUG_TMI
        printf
        (
            "Key #%03d '%s': Ord %02x; code %02x\n",
            i, qt_keys(index).qtk_keyname.c_str(),
            qt_keys(index).qtk_ordinal, qt_keys(index).qtk_keyevent
        );
#endif
    }
}

#endif  // defined SEQ66_PLATFORM_WINDOWS

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
 *  It also is the last ordinal present in the keymap.
 */

ctrlkey
invalid_ordinal ()
{
    return 0xff;
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
 *  Provides a data type to convert from incoming Qt 5 key() values
 *  into the corresponding internal key value. It is important to note that
 *  more than one key can have the same value.  Some are examples are the
 *  keypad keys and their normal keys, or letters that can be typed on their
 *  own, but also be paired with the Control or Shift modifiers.  Thus we use
 *  a multi-map to hold the data, to allow multiple entries with the same
 *  event-key value.
 *
 *  Each event-key value is paired with a structure that contains the ordinal,
 *  virtual key, and modifier that completely define that key, plus the name
 *  of the key.
 *
 *  So, this map lets us look up an event-key, and, if there are more than
 *  one, search a little further to find the one we want.
 */

using qt_keycode_map = std::multimap<eventkey, qt_keycodes>;

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
 *  Another map to use to look up keystroke names from the 'ctrl' file. See
 *  the function qt_keyname_ordinal() in this module.  This lets us look up an
 *  ordinal value (ctrlkey), ranging from 0x00 to 0xff-1, given the name of
 *  the key.
 */

using qt_keyname_map = std::map<std::string, ctrlkey>;

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
 *  Indicates if the key maps are set up.  Also allows for re-initialization in
 *  order to alter the key maps for alternate keyboard maps.
 */

static bool
initialize_key_maps (bool reinit)
{
    static bool s_are_maps_initialized = false;
    if (reinit)
        s_are_maps_initialized = false;

    if (! s_are_maps_initialized)
    {
        keycode_map().clear();  /* multimap of ctrlkey and key-code structs */
        keyname_map().clear();  /* map of key-names and ctrlkey ordinals    */
        for (int k = 0x01; k < 0xff; ++k)
        {
            qt_keycodes temp = qt_keys(k);

            /*
             * auto --> std::pair<eventkey, qt_keycodes>
             */

            auto pk = std::make_pair(temp.qtk_keyevent, temp);
            (void) keycode_map().insert(pk);

            /*
             * auto --> std::pair<std::string, unsigned>
             */

            auto pn = std::make_pair(temp.qtk_keyname, temp.qtk_ordinal);
            (void) keyname_map().insert(pn);
        }
        s_are_maps_initialized = keymap_size() >= 0xfe;
        if (! s_are_maps_initialized)
            error_message("Key map unable to be initialized");

#if defined SEQ66_PLATFORM_DEBUG_TMI

        /*
         * std::multimap<eventkey, qt_keycodes>
         */

        printf("====================== Key Code Map ====================\n");
        int index = 0;
        for (const auto & kp : keycode_map())
        {
            printf
            (
                "Key[%3d] = ev key %8u { 0x%2x <-- code 0x%08x, "
                "'%9s', mod 0x%2x }\n",
                index, kp.first, kp.second.qtk_ordinal, kp.second.qtk_keyevent,
                kp.second.qtk_keyname.c_str(), kp.second.qtk_modifier
            );
            ++index;
        }
        printf("keymap size = %d\n", keymap_size());

        /*
         * = std::map<std::string, ctrlkey>
         */

        printf("====================== Key Name Map ====================\n");
        index = 0;
        for (const auto & kp : keyname_map())
        {
            printf
            (
                "Name[%3d] = %10s #%3u (0x%2x)\n",
                index, kp.first.c_str(), kp.second, kp.second
            );
            ++index;
        }
        printf("keyname size = %d\n", int(keyname_map().size()));

#endif

    }                                   /* if (! s_are_maps_initialized)    */
    return s_are_maps_initialized;
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
    if (initialize_key_maps(false))
    {
        for (const auto & qk : keycode_map())
        {
            if (qk.second.qtk_ordinal == ordinal)
            {
                result = qk.second.qtk_keyevent;
                break;
            }
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
 * \param virtkey
 *      This is the result of the QKeyEvent::nativeVirtualKey() function.
 *      For ASCII keys it is the ASCII code.  For extended keys it is values such
 *      as 0xffnn or even other values.  This parameter is used only if non-zero.
 *      For now, the default value is 0.
 *
 * \return
 *      If the combination is found in the key-map, then it's ordinal is
 *      returned.  If not found, then 0xff [invalid_ordinal()] is returned.
 *      This would indicate an error of some kind.
 */

ctrlkey
qt_modkey_ordinal (eventkey qtkey, unsigned qtmodifier, eventkey virtkey)
{
    ctrlkey result = invalid_ordinal();
    if (initialize_key_maps(false))
    {
        auto cqi = keycode_map().find(qtkey);      /* copy modified later! */
        if (cqi != keycode_map().end())
        {
            /*
             * std::multimap<unsigned, qt_keycodes>::size_type
             */

            auto c = keycode_map().count(qtkey);
            bool found = c == 1;
            if (c > 1)
            {
                auto p = keycode_map().equal_range(qtkey);
                for (cqi = p.first; cqi != p.second; ++cqi)
                {
                    found = cqi->second.qtk_modifier == qtmodifier;
                    if (found && virtkey > 0)
                        found = cqi->second.qtk_virtkey == virtkey;

                    if (found)
                        break;
                }
            }
            if (found)
                result = cqi->second.qtk_ordinal;
        }
    }

#if defined SEQ66_PLATFORM_DEBUG_TMI
    std::string name = qt_ordinal_keyname(result);
    char temp[132];
    (void) snprintf
    (
        temp, sizeof temp,
        "qt_modkey_ordinal(0x%x, 0x%x, 0x%x) --> 0x%x '%s'",
        qtkey, qtmodifier, virtkey, result, name.c_str()
    );
    printf("%s\n", temp);
#endif

    return result;
}

/**
 *  Gets the name of the key/text combination, either from the text() value or
 *  via lookup in the key-code map.
 *
 *      std::multimap<unsigned, qt_keycodes>::size_type
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
 * \param virtkey
 *      This is the result of the QKeyEvent::nativeVirtualKey() function.
 *      For ASCII keys it is the ASCII code.  For extended keys it is values such
 *      as 0xffnn or even other values.  This parameter is used only if non-zero.
 *      For now, the default value is 0.
 *
 * \return
 *      Returns the name of the key.
 */

std::string
qt_modkey_name (eventkey qtkey, unsigned qtmodifier, eventkey virtkey)
{
    ctrlkey ordinal = qt_modkey_ordinal(qtkey, qtmodifier, virtkey);
    return qt_ordinal_keyname(ordinal);
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
    if (initialize_key_maps(false))
    {
        const auto & cki = keyname_map().find(name);
        if (cki != keyname_map().end())
            result = cki->second;
    }
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
    return is_invalid_ordinal(ordinal) ?
        "Missing_Key" : qt_keys(ordinal).qtk_keyname ;
}

/**
 *  Tweaks the setup for some keyboard layouts that give direct access (i.e.
 *  without using a "dead key") to extended ASCII keys.
 */

void
modify_keyboard_layout (keyboard::layout el)
{
    if (el == keyboard::layout::azerty)
    {
        setup_qt_azerty_fr_keys();          /* this call must come first    */
        (void) initialize_key_maps(true);   /* reinitialize the key maps    */
    }
}

}           // namespace seq66

/*
 * keymap.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


#if ! defined SEQ66_KEYSTROKE_HPP
#define SEQ66_KEYSTROKE_HPP

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
 * \file          keystroke.hpp
 *
 *  This module declares/defines the base class for handling many facets
 *  of using a GUI representation of keystrokes.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-09-21
 * \updates       2019-08-22
 * \license       GNU GPLv2 or above
 *
 *  This class is used for encapsulating keystrokes, and is used for some Qt 5
 *  processing.
 */

#include <string>                       /* std::string class                */

#include "midi/midibytes.hpp"           /* seq66::ctrlkey alias             */

/**
 *  Provides readability macros for true and false, to indicate if a
 *  keystroke is pressed, or released.
 */

#define SEQ66_KEYSTROKE_RELEASE         false
#define SEQ66_KEYSTROKE_PRESS           true

/**
 *  Range limits for the various integer parameters.  Used for sanity-checking
 *  and unit-testing.
 */

#define SEQ66_KEYSTROKE_BAD_VALUE       0x0000      /* null   */
#define SEQ66_KEYSTROKE_MIN             0x0001      /* Ctrl-A */
#define SEQ66_KEYSTROKE_MAX             0xffff

/**
 *  Values from Qt 5.  The commented values indicate their value in the keymap
 *  module.
 */

#define SEQ66_QtBackSpace               0x01000002  // 0x83    // 0xff08 (Gtk)
#define SEQ66_QtDelete                  0x01000007  // 0x87    // 0xffff (Gtk)
#define SEQ66_QtLeft                    0x01000012  // 0x92
#define SEQ66_QtUp                      0x01000013  // 0x93
#define SEQ66_QtRight                   0x01000014  // 0x94
#define SEQ66_QtDown                    0x01000015  // 0x95

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Types of modifiers, essentially the set from gtk-2.0/gdk/gdktypes.h.
 *  We have to tweak the names to avoid redeclaration errors and to
 *  "personalize" the values.  We change "GDK" to "SEQ66".
 *
 *  Since we're getting events from, say Gtk-2.4, but using our (matching)
 *  values for comparison, use the CAST_EQUIVALENT() macro to compare them.
 *  Note that we might still end up having to a remapping (e.g. if trying to
 *  get the code to work with the Qt framework).
 */

using seq_modifier_t = enum seq_modifier_t_for_linkage
{
    SEQ66_NO_MASK           = 0,
    SEQ66_SHIFT_MASK        = 1,                // Shift modifier key
    SEQ66_LOCK_MASK	        = 1 << 1,           // Lock (scroll)? modifier key
    SEQ66_CONTROL_MASK      = 1 << 2,           // Ctrl modifier key
    SEQ66_MOD1_MASK	        = 1 << 3,           // Alt modifier key
    SEQ66_MOD2_MASK	        = 1 << 4,           // Num Lock modifier key
    SEQ66_MOD3_MASK	        = 1 << 5,           // Hyper_L (?)
    SEQ66_MOD4_MASK	        = 1 << 6,           // Super/Windoze modifier key
    SEQ66_MOD5_MASK	        = 1 << 7,           // Mode_Switch (?)
    SEQ66_BUTTON1_MASK      = 1 << 8,
    SEQ66_BUTTON2_MASK      = 1 << 9,
    SEQ66_BUTTON3_MASK      = 1 << 10,
    SEQ66_BUTTON4_MASK      = 1 << 11,
    SEQ66_BUTTON5_MASK      = 1 << 12,

    /**
     * Bits 13 and 14 are used by XKB, bits 15 to 25 are unused. Bit 29 is
     * used internally.
     */

    SEQ66_SUPER_MASK        = 1 << 26,
    SEQ66_HYPER_MASK        = 1 << 27,
    SEQ66_META_MASK         = 1 << 28,
    SEQ66_RELEASE_MASK      = 1 << 30,  // GDK_MODIFIER_MASK = 0x5c001fff
    SEQ66_MASK_MAX          = 1 << 31

};          // seq_modifier_t

/**
 *  Encapsulates any practical keystroke.  Useful in passing more generic
 *  events to non-GUI classes.
 */

class keystroke
{

private:

    /**
     *  Determines if the key was a press or a release.  See the
     *  SEQ66_KEYSTROKE_PRESS and SEQ66_KEYSTROKE_RELEASE readability macros.
     */

    bool m_is_press;                    /* versus a release of the key */

    /**
     *  The key that was pressed or released.  Generally, the extended ASCII
     *  range (0 to 255) is supported.  However, Gtk-2.x/3.x and Qt 5.0 will
     *  generally support the full gamut of characters defined in the
     *  gdk_basic_keys.h module.  We define minimum and maximum range macros for
     *  keystrokes that are a bit generous.
     */

    mutable ctrlkey m_key;

    /**
     *  The optional modifier value.  Note that SEQ66_NO_MASK is our word
     *  for 0, meaning "no modifier".
     */

    seq_modifier_t m_modifier;

public:

    keystroke ();
    keystroke
    (
        ctrlkey key,
        bool press = SEQ66_KEYSTROKE_PRESS,         /* true */
        int modkey = int(SEQ66_NO_MASK)
    );
    keystroke (const keystroke & rhs);
    keystroke & operator = (const keystroke & rhs);

    bool is_press () const
    {
        return m_is_press;
    }

    bool is_letter (ctrlkey ch = SEQ66_KEYSTROKE_BAD_VALUE) const;

    /**
     *  Tests the key value to see if it matches the given character exactly
     *  (no case-insensitivity).
     *
     * \param ch
     *      The character to be tested.
     *
     * \return
     *      Returns true if m_key == ch.
     */

    bool is (ctrlkey ch) const
    {
        return m_key == ch;
    }

    /**
     *  Tests the key value to see if it matches the given character exactly
     *  (no case-insensitivity).
     *
     * \param ch1
     *      The first character to be tested.
     *
     * \param ch2
     *      The second character to be tested.
     *
     * \return
     *      Returns true if m_key == ch1 or ch2.
     */

    bool is (ctrlkey ch1, ctrlkey ch2) const
    {
        return m_key == ch1 || m_key == ch2;
    }

    /**
     * \getter m_key to test for a delete-causing key.
     */

    bool is_delete () const
    {
        return m_key == SEQ66_QtDelete || m_key == SEQ66_QtBackSpace;
    }

    /*
     * The following functions support hard-wired usage of the arrow keys.
     */

    bool is_left () const
    {
        return m_key == 0x92;
    }

    bool is_up () const
    {
        return m_key == 0x93;
    }

    bool is_right () const
    {
        return m_key == 0x94;
    }

    bool is_down () const
    {
        return m_key == 0x95;
    }

    ctrlkey key () const
    {
        return m_key;
    }

    ctrlkey shifted () const;

    void shift_lock ()
    {
        m_key = shifted();
    }

    seq_modifier_t modifier () const
    {
        return m_modifier;
    }

    /**
     * \getter m_modifier tested for Ctrl key.
     */

    bool mod_control () const
    {
        return bool(m_modifier & SEQ66_CONTROL_MASK);   // GDK_CONTROL_MASK
    }

    /**
     * \getter m_modifier tested for Ctrl and Shift key.
     */

    bool mod_control_shift () const
    {
        return
        (
            bool(m_modifier & SEQ66_CONTROL_MASK) &&
            bool(m_modifier & SEQ66_SHIFT_MASK)
        );
    }

    /**
     * \getter m_modifier tested for Mod4/Super/Windows key.
     */

    bool mod_super () const
    {
        return bool(m_modifier & SEQ66_MOD4_MASK);
    }

    void toupper ();                    /* changes m_key    */
    void tolower ();                    /* changes m_key    */
    ctrlkey upper () const;
    ctrlkey lower () const;
    std::string name () const;

};          // class keystroke

}           // namespace seq66

#endif      // SEQ66_KEYSTROKE_HPP

/*
 * keystroke.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


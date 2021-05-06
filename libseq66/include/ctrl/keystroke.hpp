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
 * \updates       2020-05-05
 * \license       GNU GPLv2 or above
 *
 *  This class is used for encapsulating keystrokes, and is used for some Qt 5
 *  processing.
 */

#include <string>                       /* std::string class                */
#include "ctrl/keymap.hpp"              /* seq66::ctrlkey alias, etc.       */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Encapsulates any practical keystroke.  Useful in passing more generic
 *  events to non-GUI classes.
 */

class keystroke
{

public:

    /**
     *  Provides readable values to indicate if a keystroke is a press or a
     *  release.
     */

    enum class action
    {
        release,
        press
    };

private:

    /**
     *  Range limits for the various integer parameters.  Used for
     *  sanity-checking.
     */

    static const ctrlkey sm_bad_value       = 0x00;         /* null         */
    static const ctrlkey sm_minimum         = 0x01;         /* Ctrl-A       */
    static const ctrlkey sm_maximum         = 0xff;

    /**
     *  Values from Qt 5.  The commented values indicate their value in the
     *  keymap module.
     */

    static const eventkey sm_Qt_Backspace   = 0x01000003;   /* 0x83, 0xff08 */
    static const eventkey sm_Qt_Delete      = 0x01000007;   /* 0x87, 0xffff */
    static const eventkey sm_Qt_Left        = 0x01000012;   /* 0x92         */
    static const eventkey sm_Qt_Up          = 0x01000013;   /* 0x93         */
    static const eventkey sm_Qt_Right       = 0x01000014;   /* 0x94         */
    static const eventkey sm_Qt_Down        = 0x01000015;   /* 0x95         */

    /**
     *  Determines if the key was a press or a release.  See enum class
     *  action.
     */

    bool m_is_press;

    /**
     *  The key that was pressed or released.  Generally, the extended ASCII
     *  range (0 to 0xff) is supported.  However, Gtk-2.x/3.x and Qt 5.0 will
     *  generally support the full gamut of characters, with codes that are
     *  unsigned integers; and the modifiers might be needed for lookup.
     */

    mutable ctrlkey m_key;

    /**
     *  The optional modifiers value.  Note that "keyboard::KNONE" is our word
     *  for 0, meaning "no modifiers".
     */

    keyboard::modifiers m_modifiers;

public:

    keystroke () = default;
    keystroke
    (
        ctrlkey key,
        bool press,
        unsigned modkey = static_cast<unsigned>(keyboard::KNONE)
    );
    keystroke (const keystroke & rhs) = default;
    keystroke & operator = (const keystroke & rhs) = default;

    bool is_press () const
    {
        return m_is_press;
    }

    bool is_letter (ctrlkey ch = sm_bad_value) const;

    bool is_good () const
    {
        return m_key >= sm_minimum && m_key < sm_maximum;
    }

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

    /*
     * The following functions support hard-wired usage of the arrow keys.
     */

    bool is_left () const
    {
        return m_key == arrow_left();
    }

    bool is_up () const
    {
        return m_key == arrow_up();
    }

    bool is_right () const
    {
        return m_key == arrow_right();
    }

    bool is_down () const
    {
        return m_key == arrow_down();
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

    keyboard::modifiers modifiers () const
    {
        return m_modifiers;
    }

    void setmodifier (keyboard::modifiers km)
    {
        m_modifiers = km;
    }

    /**
     * \getter m_modifiers tested for Ctrl key.
     */

    bool mod_control () const
    {
        return bool(m_modifiers & keyboard::KCTRL);
    }

    /**
     * \getter m_modifiers tested for Ctrl and Shift key.
     */

    bool mod_control_shift () const
    {
        return mod_control() && (m_modifiers & keyboard::KSHIFT);
    }

    /**
     * \getter m_modifiers tested for Mod4/Super/Windows key.
     */

    bool mod_super () const
    {
        return bool(m_modifiers & keyboard::KMETA);
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


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
 * \file          keystroke.cpp
 *
 *  This module declares/defines the base class for handling many facets
 *  of using a GUI representation of keystrokes.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-09-30
 * \updates       2021-04-28
 * \license       GNU GPLv2 or above
 *
 *  This class makes access to keystroke features simpler.
 */

#include <cctype>                       /* std::islower(), std::toupper()   */

#include "ctrl/keymap.hpp"              /* seq66::qt_modkey_name()          */
#include "ctrl/keystroke.hpp"           /* seq66::keystroke class           */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  The default constructor for class keystroke.
 */

keystroke::keystroke ()
 :
    m_is_press  (false),
    m_key       (sm_minimum),
    m_modifier  (mod::None)
{
    // Empty body
}

/**
 *  The principal constructor.
 *
 * \param key
 *      The keystroke number of the key that was pressed or released.
 *
 * \param press
 *      If true, the keystroke action was a press, otherwise it was a release.
 *
 * \param modkey
 *      The modifier key combination that was pressed, if any, in the form of
 *      a bit-mask, as defined in the gdk_basic_keys module.  Common mask
 *      modifier values are Shift, Control, Mod1, and Mod4.  If no modifier,
 *      this value is None.
 */

keystroke::keystroke (ctrlkey key, bool press, int modkey) :
    m_is_press  (press),
    m_key       (key),
    m_modifier  (static_cast<mod>(modkey))
{
    // Empty body
}

/**
 * \getter m_key to test letters, handles ASCII only.
 *
 * \param ch
 *      An optional character to test as an ASCII letter.
 *
 * \return
 *      If a character is not provided, true is returned if it is an upper
 *      or lower-case letter.  Otherwise, true is returned if the m_key
 *      value matches the character case-insensitively.
 *      \tricky
 */

bool
keystroke::is_letter (ctrlkey ch) const
{
    if (ch == sm_bad_value)
        return bool(std::isalpha(m_key));
    else
        return std::tolower(m_key) == std::tolower(ch);
}

/**
 *  Holds a pair of characters.  These don't yet apply to A-Z, for speed.
 */

struct charpair_t
{
    ctrlkey m_character;                /**< The input character.           */
    ctrlkey m_shift;                    /**< The shift of input character.  */
};

/**
 *  The array of shifted mappings of the non-alphabetic characters.
 */

static struct charpair_t s_character_mapping [] =
{
    {   '0',    ')'     },      // no mapping
    {   '1',    '!'     },
    {   '2',    '@'     },      // "
    {   '3',    '#'     },
    {   '4',    '$'     },
    {   '5',    '%'     },
    {   '6',    '&'     },
    {   '7',    '^'     },      // Super-L
    {   '8',    '*'     },      // (
    {   '9',    '('     },      // no mapping
    {   '-',    '_'     },
    {   '=',    '+'     },
    {   '[',    '{'     },
    {   ']',    '}'     },
    {   ';',    ':'     },
    {   '\'',    '"'    },
    {   ',',    '<'     },
    {   '.',    '>'     },
    {   '/',    '?'     },
    {   '\\',   '|'     },
    {     0,      0     },
};

/**
 *  If a lower-case letter, a number, or another character on the "main" part
 *  of the keyboard, shift the m_key value to upper-case or the character
 *  shifted on a standard American keyboard.  Currently also assumes the ASCII
 *  character set.
 *
 *  There's an oddity here:  the shift of '2' is the '@' character, but seq66
 *  seems to have treated it like the '"' character. Some others were treated
 *  the same:
 *
\verbatim
    Key:        1 2 3 4 5 6 7 8 9 0
    Shift:      ! @ # $ % ^ & * ( )
    Seq24:      ! " # $ % & ' ( ) space
\endverbatim
 *
 *  This function is meant to avoid using the Caps-Lock when picking a
 *  group-learn character in the group-learn mode.
 */

ctrlkey
keystroke::shifted () const
{
    ctrlkey result = m_key;
    if (std::islower(m_key))
    {
        result = ctrlkey(std::toupper(m_key));
    }
    else
    {
        charpair_t * cp_ptr = &s_character_mapping[0];
        while (cp_ptr->m_character != 0)
        {
            if (cp_ptr->m_character == m_key)
            {
                result = cp_ptr->m_shift;
                break;
            }
            ++cp_ptr;
        }
    }
    return result;
}

/**
 *  If the character is lower-case, it is converted (internally) to
 *  upper-case.  Also see the shift_lock() function.
 */

void
keystroke::toupper ()
{
    if (std::islower(m_key))
        m_key = ctrlkey(std::toupper(m_key));
}

/**
 *  Returns the upper-case version of the character, if it is lower-case.
 */

ctrlkey
keystroke::upper () const
{
    return std::islower(m_key) ? ctrlkey(std::toupper(m_key)) : m_key ;
}

/**
 *  If the character is upper-case, it is converted (internally) to
 *  lower-case.
 */

void
keystroke::tolower ()
{
    if (std::isupper(m_key))
        m_key = ctrlkey(std::tolower(m_key));
}

/**
 *  Returns the lower-case version of the character, if it is upper-case.
 */

ctrlkey
keystroke::lower () const
{
    return std::isupper(m_key) ? ctrlkey(std::tolower(m_key)) : m_key ;
}

/**
 *  Assembles and returns the human-readable name of the key.
 */

std::string
keystroke::name () const
{
    return qt_ordinal_keyname(m_key);
}

}           // namespace seq66

/*
 * keystroke.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


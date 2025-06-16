#ifndef SEQ66_MIDIMACRO_HPP
#define SEQ66_MIDIMACRO_HPP

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
 * \file          midimacro.hpp
 *
 *  This module declares/defines the base class for handling MIDI control
 *  I/O of the application.
 *
 * \library       seq66 application
 * \author        C. Ahlstrom
 * \date          2021-11-22
 * \updates       2021-12-04
 * \license       GNU GPLv2 or above
 *
 *  Provides the base class for midicontrolout.
 *
 * Warning:
 *
 *      It is NOT a base class for midicontrol or midicontrolin!
 */

#include "midi/midibytes.hpp"           /* seq66::midistring data type      */
#include "util/basic_macros.hpp"        /* seq66::tokenization container    */

namespace seq66
{

/**
 *  Represents a string of midibytes and provides the infrastructure for
 *  reading them.
 */

class midimacro
{

    friend class midimacros;

private:

    /**
     *  The name of the macro.  This is also the key value for putting the
     *  midimacro in a container.
     */

    std::string m_name;

    /**
     *  This is a list of tokens making up the macro. Although it can take up
     *  extra space, it is useful to write the macro back to the configuration
     *  file. Also see the tokenize() function in the strfunctions module.
     */

    tokenization m_tokens;

    /**
     *  Provides the full list of midibytes to be sent via this macro after
     *  expanding any macros it includes.  A midistring is a basic_string of
     *  midibytes.
     */

    midistring m_bytes;

    /**
     *  Is the macro good?  It is good if there is a name, if there's at least
     *  one byte value or reference token, and the byte value isn't 0.
     *  Even if invalid, the macro will be loaded and saved.
     */

    bool m_is_valid;

public:

    midimacro () = default;
    midimacro (const std::string & name, const std::string & values);
    midimacro (const midimacro &) = default;
    midimacro & operator = (const midimacro &) = default;
    midimacro (midimacro &&) = default;
    midimacro & operator = (midimacro &&) = default;
    ~midimacro () = default;

    const std::string & name () const
    {
        return m_name;
    }

    tokenization tokens () const
    {
        return m_tokens;
    }

    std::string line () const;

    const midistring & bytes () const
    {
        return m_bytes;
    }

    bool is_valid () const
    {
        return m_is_valid;
    }

private:

    bool tokenize (const std::string & values);

    void name (const std::string & n)
    {
        m_name = n;
    }

    void bytes (const midistring & b);

};          // class midimacro

}           // namespace seq66

#endif      // SEQ66_MIDIMACRO_HPP

/*
 * midimacro.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


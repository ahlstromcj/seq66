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
 * \updates       2025-07-12
 * \license       GNU GPLv2 or above
 *
 *  Provides the base class for midicontrolout.
 *
 * Warning:
 *
 *      It is NOT a base class for midicontrol or midicontrolin!
 */

#include "midi/midibytes.hpp"           /* seq66::midibytes data type       */
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
     *  file. It also allows putting multiple events into one macro.
     *
     *  Also see the tokenize() function in the strfunctions module.
     */

    tokenization m_tokens;

    /**
     *  Provides the full list of midibytes to be sent via this macro after
     *  expanding any macros it includes.
     */

    midibytes m_bytes;

    /**
     *  The number of events in the macro. Normally just one, unless
     *  the vertical bar ("|") occurs in the list of tokens.
     */

    int m_event_count;

    /**
     *  Provides the midibytes for each separate event in a multiple-event
     *  macro. Populated only if the separator bar ("|") was present.
     */

    std::vector<midibytes> m_event_bytes;

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

    const midibytes & bytes (int index = (-1)) const;

    int event_count () const
    {
        return m_event_count;
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

    void bytes (const midibytes & b)
    {
        m_bytes = b;
    }

    void push_bytes (const midibytes & b)
    {
        m_event_bytes.push_back(b);
    }

};          // class midimacro

}           // namespace seq66

#endif      // SEQ66_MIDIMACRO_HPP

/*
 * midimacro.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


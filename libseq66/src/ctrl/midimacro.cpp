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
 * \file          midimacro.cpp
 *
 *  This module declares/defines the base class for handling MIDI control
 *  <i>I/O</i> of the application.
 *
 * \library       seq66 application
 * \author        C. Ahlstrom
 * \date          2021-11-21
 * \updates       2021-11-23
 * \license       GNU GPLv2 or above
 *
 *  The specification for the midimacro is of the following format:
 *
\verbatim
    macroname = { valuespec }
\endverbatim
 *
 *  where valuespec is either a byte value in hex format (e.g. 0xF3) or
 *  the name of another macro in the format "$othermacro".  Here are some
 *  examples:
 *
\verbatim
    header = 0xF0 0x00 0x20 0x29 0x02 0x0E    # Launchpad Pro MK3
    function = $header <command-byte> <function-code>
\endverbatim
 *
 */

#include "ctrl/midimacro.hpp"           /* seq66::midimacro class           */
#include "util/strfunctions.hpp"        /* seq66::tokenize()                */

namespace seq66
{


midimacro::midimacro (const std::string & name, const std::string & values) :
    m_name      (name),
    m_tokens    (),
    m_bytes     (),
    m_is_valid  ()
{
    m_is_valid = tokenize(values);          /* the member function below    */
    if (m_is_valid)
        m_is_valid = ! m_tokens[0].empty();
}

void
midimacro::bytes (const midistring & b)
{
    bool valid = false;
    m_bytes = b;
    for (auto c : b)
    {
        if (c > 0)
        {
            valid = true;
            break;
        }
    }
    m_is_valid = valid;
}

bool
midimacro::tokenize (const std::string & values)
{
    m_tokens = seq66::tokenize(values);         /* from strfunctions module */
    return m_tokens.size() > 0;
}

std::string
midimacro::line () const
{
    std::string result = name();
    result += " =";
    for (const auto & t : tokens())
    {
        result += " ";
        result += t;
    }
    return result;
}

}           // namespace seq66

/*
 * midimacro.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


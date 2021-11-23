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
 * \file          midimacros.cpp
 *
 *  This module declares/defines the base class for handling MIDI control
 *  <i>I/O</i> of the application.
 *
 * \library       seq66 application
 * \author        C. Ahlstrom
 * \date          2021-11-21
 * \updates       2021-11-22
 * \license       GNU GPLv2 or above
 *
 *  The specification for the midimacros is of the following format:
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

#include "ctrl/midimacros.hpp"          /* seq66::midimacros class          */
#include "util/strfunctions.hpp"        /* seq66::string_to_midibyte()      */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{


midimacros::midimacros () : m_macros ()
{
   // todo
}

/**
 *  This function handles parsing a line from the 'ctrl' file.  The line should
 *  have the form "macroname = <data bytes>" which is then converted in that
 *  file to a two-element vector of tokens:  tokens[0] = name, tokens[1] =
 *  <data bytes>, which is a space-separated list of midibyte-strings in
 *  hex format.
 */

bool
midimacros::add (const tokenization & tokens)
{
    bool result = tokens.size() == 2;           /* the name, then the data  */
    if (result)
    {
        std::string key = tokens[0];
        std::string data = tokens[1];
        if (result)
        {
            midimacro m(key, data);             /* further tokenizes        */
            auto p = std::make_pair(key, m);
            auto r = m_macros.insert(p);        /* r: pair<iteration, bool> */
            result = r.second;
        }
    }
    return result;
}

/**
 *  Converts all the loaded macros into midistrings, expanding macro
 *  references where needed.  References are tokens showing the name of
 *  another macro, e.g. "$header".
 */

bool
midimacros::expand ()
{
    bool result = count() > 0;
    if (result)
    {
        for (auto & m : m_macros)
        {
            midistring b = expand(m.second);
            result = ! b.empty();
            if (result)
                m.second.bytes(b);
            else
                break;
        }
    }
    return result;
}

midistring
midimacros::expand (midimacro & m)
{
    midistring result;
    for (const auto & token : m.tokens())
    {
        if (token[0] == '$')
        {
            std::string name = token.substr(1);
            const auto cit = m_macros.find(name);
            if (cit != m_macros.end())
            {
                result += expand(cit->second);
            }
            else
            {
                result.clear();
                break;
            }
        }
        else
        {
            midibyte b = string_to_midibyte(token);
            result += b;
        }
    }
    return result;
}

midistring
midimacros::bytes (const std::string & name)
{
    midistring result;
    const auto cit = m_macros.find(name);
    if (cit != m_macros.end())
        result = cit->second.bytes();

    return result;
}

std::string
midimacros::lines () const
{
    std::string result;
    for (auto & m : m_macros)
    {
        result += m.second.line();
        result += "\n";
    }
    return result;
}

}           // namespace seq66

/*
 * midimacros.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


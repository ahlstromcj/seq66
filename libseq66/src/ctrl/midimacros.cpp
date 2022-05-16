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
 * \updates       2021-12-29
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
#include "util/strfunctions.hpp"        /* seq66::tokenize()                */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Constant static macro names.
 */

const std::string midimacros::footer    = "footer";
const std::string midimacros::header    = "header";
const std::string midimacros::reset     = "reset";
const std::string midimacros::startup   = "startup";
const std::string midimacros::shutdown  = "shutdown";

/**
 *  Default constructor.
 */

midimacros::midimacros () :
    m_macros ()
{
   // No code needed
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
    bool result = tokens.size() >= 1;           /* the name, then the data  */
    if (result)
    {
        std::string key = tokens[0];
        std::string data;
        if (tokens.size() > 1)                  /* any data?                */
            data = tokens[1];

        midimacro m(key, data);                 /* further tokenizes        */
        auto p = std::make_pair(key, m);
        auto r = m_macros.insert(p);            /* r: pair<iteration, bool> */
        result = r.second;
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
            midimacro & mac = m.second;
            midistring b = expand(mac);
            result = ! b.empty();
            if (result)
                mac.bytes(b);
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
midimacros::bytes (const std::string & name) const
{
    midistring result;
    const auto cit = m_macros.find(name);
    if (cit != m_macros.end())
    {
        const midimacro & m = cit->second;
        if (m.is_valid())
            result = m.bytes();
    }
    return result;
}

std::string
midimacros::lines () const
{
    std::string result;
    for (const auto & m : m_macros)
    {
        result += m.second.line();
        result += "\n";
    }
    return result;
}

tokenization
midimacros::names () const
{
    tokenization result;
    for (const auto & m : m_macros)         /* const auto & [key, value] */
        result.push_back(m.second.name());

    return result;
}

std::string
midimacros::byte_strings () const
{
    std::string result;
    for (const auto & m : m_macros)
    {
        const midimacro & mac = m.second;
        result += mac.name();
        result += ": ";
        result += midi_bytes_string(mac.bytes());
        result += "\n";
    }
    return result;
}

/**
 *  This function creates some defaults to ensure that there is a valid
 *  macro-control section in the 'ctrl' file.  These are not useable, but will
 *  be checked for at (for example) startup and shutdown.
 */

bool
midimacros::make_defaults ()
{
    static const std::string s_defaults [] =
    {
        "footer = 0xF7                   # End-of-SysEx byte",
        "header = 0xF0 0x00 0x00         # device SysEx header, 0xF0 required",
        "reset = $header 0x00 $footer    # fill in with device's reset command",
        "startup = $header 0x00 $footer  # sent at start, if not empty",
        "shutdown = $header 0x00 $footer # sent at exit, if not empty",
        ""  /* list terminator */
    };
    bool result = count() == 0;
    if (result)
    {
        for (int i = 0; ! s_defaults[i].empty(); ++i)
        {
            tokenization t = seq66::tokenize(s_defaults[i], "=");
            if (! add(t))
                break;
        }
    }
    return result;
}

}           // namespace seq66

/*
 * midimacros.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


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
 * \updates       2026-08-23
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

#include <cstring>                      /* std::strlen()                    */

#include "ctrl/midimacro.hpp"           /* seq66::midimacro class           */
#include "util/strfunctions.hpp"        /* seq66::tokenize()                */

namespace seq66
{

/**
 *  Used to note that data must be read from a file.
 */

const std::string &
midimacro::file_marker ()
{
    static const std::string s_file_marker { "file:" };
    return s_file_marker;
}

/**
 *  Note that some defaults are defined "in-class".
 *
 * \param name
 *      Provides the name of the macro, which is also the key value.
 *
 * \param values
 *      Provides either a string of hexadecimal tokens, each represent a byte
 *      value, or a string of the form "file: <file-specification>".
 */

midimacro::midimacro
(
    const std::string & name,
    const std::string & values
) :
    m_name (name)
{
    m_is_valid = tokenize(values);              /* member function below    */
}

const midibytes &
midimacro::bytes (int index) const
{
    static midibytes s_dummy { 0 };
    if (index >= 0 && index < m_event_count)
        return m_event_bytes[index];
    else
        return s_dummy;
}

/**
 *  This function accepts a string of byte values ("0xF0 0xAB ...")
 *  and creates a tokenization (vector of strings) from them, and
 *  stores it in m_tokens.
 *
 *  We have added the ability to provide multiple " | "-separated events in
 *  one macro. We also have added support for a values string of the form
 *  "file: <filename>".
 *
 * \param values
 *      Provides a way to get the byte values necessary for the macro.
 *      In the first form, it is a list of bytes as hex strings, "0xnn",
 *      separated by spaces or by " | ". Any "|" tokens separate the bytes
 *      into one event, and this function counts the number of sets of bytes.
 *      In the second form, "file: <filename>", where an example of a
 *      filename is "roland-empty.syx" (SysEx file).
 *
 * \return
 *      Returns true if there were tokens.
 */

bool
midimacro::tokenize (const std::string & values)
{
    m_tokens = seq66::tokenize(values);         /* from strfunctions module */

    bool result { m_tokens.size() > 0 };
    if (result)
    {
        if (m_tokens[0] == file_marker())
        {
            use_file_storage(true);
            file_name(m_tokens[1]);
        }
        else
        {
            m_event_count = 1;
            if (m_tokens.size() >= 3)
            {
                for (const auto & t : m_tokens)
                {
                    if (t == "|")
                        ++m_event_count;        /* no. of "|" sep'd events  */
                }
            }
        }
    }
    return result;
}

/**
 *  Converts the tokens into a single string. Works best for events less
 *  than 32 bytes, otherwise the line is long.
 *
 *  The formats returned by this function are suitable for direct
 *  appending to a 'ctrl' file.
 *
 * \return
 *      Returns a string in one of these formats:
 *
 *          -   "macnam = 0xF0 0xAB ..."
 *          -   "macnam = file: full-file-specification"
 */

std::string
midimacro::line () const
{
    std::string result { name() };
    result += " =";
    for (const auto & t : tokens())
    {
        result += " ";
        result += t;
    }
    return result;
}

/**
 *  This function is more suitable for larger macros. It converts the
 *  byte values to strings limited to a decent line length, 72 bytes.
 *
 *  It needs to iterate over all of the events this macro contains.
 *
 * \return
 *      Returns one or more lines of the form "0xFF 0xAB ...", which
 *      are suitable for storage in the simplistic *.macro files.
 */

tokenization
midimacro::bytes_to_lines () const
{
    tokenization result;
    std::string line;
    const int limit { 72 };
    int count { event_count() };
    for (int index = 0; index < count; ++index)
    {
        const midibytes & byts { bytes(index) };
        int charcount { 0 };
        for (auto b : byts)
        {
            char tmp[8];
            if (charcount > limit)
            {
                charcount = 0;
                result.push_back(line);
                result.push_back("\n");
                line.clear();
            }
            snprintf(tmp, sizeof tmp, "0x%02x ", b);
            charcount += int(std::strlen(tmp));
            line += tmp;
        }
        if (index < (count - 1))
            line += " | ";
    }
    result.push_back(line);
    return result;
}

}           // namespace seq66

/*
 * midimacro.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

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
 * \updates       2026-08-29
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
#include "midi/midifile.hpp"            /* seq66::read_raw_midi()           */
#include "util/filefunctions.hpp"       /* seq66::file_read_lines()         */
#include "util/strfunctions.hpp"        /* seq66::tokenize()                */

namespace seq66
{

/**
 *  Constant static macro names.
 */

const std::string midimacros::footer        { "footer" };
const std::string midimacros::header        { "header" };
const std::string midimacros::reset         { "reset" };
const std::string midimacros::startup       { "startup" };
const std::string midimacros::shutdown      { "shutdown" };
const std::string midimacros::macro_header  { "Seq66 macro: " };

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
 *
 *  What should we do if the macro already exists? We could delete the
 *  existing macro and add the new one via modify(), or just return false.
 *
 * \param tokens
 *      Provides two tokens representing the macro. The first token is the
 *      name of the macro, and the second is either a string of hexadecimal
 *      tokens, each represent a byte value, or a string of the form
 *      "file: <file-specification>".
 *
 * \return
 *      Returns true if the macro did not exist already, and was inserted
 *      properly.
 */

bool
midimacros::add (const tokenization & tokens)
{
    bool result { tokens.size() == 2 };         /* the name, then the data  */
    if (result)
    {
        std::string key { tokens[0] };
        std::string data { tokens[1] };
        if (find(key))                          /* the macro already exists */
        {
            /*
             * What to do? The previous behavior was to report an error,
             * so let's keep it that way.
             *
             * result = modify(tokens);
             */

            result = false;
        }
        else
        {
            midimacro m(key, data);             /* further tokenizes        */
            auto p { std::make_pair(key, m) };
            auto r { m_macros.insert(p) };      /* r: pair<iteration, bool> */
            result = r.second;
            if (result)
                m_active = count() > 0;
        }
    }
    return result;
}

/**
 *  It doesn't matter if the token is found to be removed, just if it
 *  can be added.
 */

bool
midimacros::modify (const tokenization & tokens)
{
    bool result { tokens.size() == 2 };         /* the name, then the data  */
    if (result)
    {
        (void) remove(tokens[0]);               /* use name, find, & delete */
        result = add(tokens);
    }
    return result;
}

/**
 *  Removes a macro.
 */

bool
midimacros::remove (const std::string & macnam)
{
    bool result { count() > 0 };
    if (result)
    {
        std::string key { macnam };
        result = m_macros.erase(key) == 1;      /* 1 or 0 can be removed    */
        if (result)
            m_active = count() > 0;
    }
    return result;
}

/**
 *  Detects if a macro is present.
 */

bool
midimacros::find (const std::string & macnam) const
{
    bool result { count() > 0 };
    if (result)
    {
        std::string key { macnam };
        auto it { m_macros.find(key) };
        result = it != m_macros.end();          /* else "bad macnam" :-)    */
    }
    return result;
}

/**
 *  Converts all the loaded macros into midibytes, expanding macro
 *  variables where needed. Variables are tokens showing the name of
 *  another macro, e.g. "$header".
 *
 *  If a macro uses one of the macro variables, but that does not exist,
 *  then the macro is skipped, with no error, and showing up as empty in
 *  the Macro Execution dropdow in the main Session tab. To get them back,
 *  copy them from data/linux/qseq66.ctrl and restart.
 */

bool
midimacros::expand ()
{
    bool result { count() > 0 };
    if (result)
    {
        for (auto & m : m_macros)
        {
            midimacro & mac { m.second };
            midibytes b { expand_macro(mac) };  /* sets is_expanded()       */
            bool ok { ! b.empty() };            /* no longer an error       */
            if (ok)
            {
                /*
                 * The bytes are no longer stored in a separate vector,
                 * but in the midimacro event stack.
                 *
                 *      mac.bytes(b);
                 */

                mac.is_valid(true);
            }
        }
    }
    return result;
}

/**
 *  Expands an existing named macro. The call sequence necessary is:
 *
 *      -   add(tokens) to make a provisional macro and add it to
 *          midimacros.
 *      -   expand(tokens[0] to expand just this new macro to obtain
 *          all the bytes of that macro.
 *      -   bytes(tokens[0] to get access to those expanded bytes when
 *          needed.
 */

bool
midimacros::expand (const std::string & macnam)
{
    bool result { false };
    const auto cit { m_macros.find(macnam) };
    if (cit != m_macros.end())
    {
        midimacro & mac { cit->second };
        if (mac.is_expanded())
        {
            result = true;
        }
        else
        {
            midibytes temp { expand_macro(mac) };   /* sets is_expanded()   */
            result = temp.size() > 0;
            if (result)
                mac.bytes(temp);
        }
    }
    return result;
}

/**
 *  Recursively expands macro variables (e.g. "$footer") into the
 *  other bytes of a macro.
 *
 *  If a pipe ('|'), then there are multiple events, i.e. multiple sets
 *  of event bytes.
 */

midibytes
midimacros::expand_macro (midimacro & m)
{
    midibytes result;                   /* holds all of the bytes found     */
    midibytes temp;                     /* holds bytes of 1 event if ! N/A  */
    if (m.is_expanded())
    {
        return m.bytes();
    }
    else
    {
        bool separator_encountered { false };
        for (const auto & token : m.tokens())
        {
            if (token[0] == '$')
            {
                std::string macnam { token.substr(1) };
                const auto cit { m_macros.find(macnam) };
                if (cit != m_macros.end())
                {
                    midimacro & variable { cit->second };
                    midibytes xpanded { expand_macro(variable) };
                    result.insert(result.end(), xpanded.begin(), xpanded.end());
                }
                else
                {
                    result.clear();
                    temp.clear();
                    break;
                }
            }
            else if (token[0] == '|')
            {
                separator_encountered = true;
                m.push_bytes(temp);     /* push onto events stack/vector    */
                temp.clear();
            }
            else
            {
                midibyte b { string_to_midibyte(token) };
                result.push_back(b);    /* push onto the midibytes vector   */
                temp.push_back(b);      /* push onto the interim vector     */
            }
        }
        if (separator_encountered)
        {
            if (temp.size() > 0)
                m.push_bytes(temp);     /* push the bytes of last event     */
        }
        else
        {
            if (result.size() > 0)
                m.push_bytes(result);
        }
        if (result.size() > 0)
            m.is_expanded(true);

        return result;
    }
}

midibytes
midimacros::bytes (const std::string & macnam) const
{
    midibytes result;
    const auto cit = m_macros.find(macnam);
    if (cit != m_macros.end())
    {
        const midimacro & m = cit->second;
        if (m.is_valid())
            result = m.bytes();
    }
    return result;
}

const midimacro &
midimacros::macro (const std::string & macnam) const
{
    static midimacro s_dummy;           /* is_valid() will return false     */
    const auto cit = m_macros.find(macnam);
    return cit != m_macros.end() ? cit->second : s_dummy ;
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
 *  macro-control section in the 'ctrl' file.  Most are useable as is, or
 *  will be checked for at (for example) startup and shutdown.
 */

bool
midimacros::make_defaults ()
{
    static const std::string s_defaults [] =
    {
        "footer = 0xF7                   # End-of-SysEx byte",
        "header = 0xF0 0x00 0x00         # device SysEx header, 0xF0 required",
        "middlec_off = 0x80 0x3C 0x00    # turn off test note",
        "middlec_on = 0x90 0x3C 0x40     # turn on test note",
        "pitch = "
            "0xB0 0x65 0 0xB0 0x64 0x0 "
            "0xB0 0x06 0x02 0xB0 0x26 0 "
            "0xB0 0x65 0x7F 0xB0 0x64 0x7F",
        "reset = $header 0x00 $footer    # fill in with device's reset command",
        "rpnpitch = "
            "0xB0 0x65 0 0xB0 0x64 0x0 "
            "0xB0 0x06 0x0C 0xB0 0x26 0 "
            "0xB0 0x65 0x7F 0xB0 0x64 0x7F",
        "shutdown = $header 0x00 $footer # sent at exit, if not empty",
        "startup = $header 0x00 $footer  # sent at start, if not empty",
        ""                                          /* list terminator */
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

/**
 *  This function reads either a raw data file or a Seq66 macro
 *  file. A raw data file is often one that contains only a SysEx
 *  message. A Seq66 macro file is ASCII and contains data for a
 *  macro. We will eventually provide a file that can hold multiple
 *  macros. The presumed type of file is determined by the file
 *  extension:
 *
 *      -   ".macro". A simple file containing an ASCII specification
 *          of the macro data. Will eventually be folded into the next
 *          one;
 *      -   ".macros". A configfile (INI style) file containing
 *          multiple macro definitions
 *      -   ".syx" or ".sysex". This is a raw data file containing
 *          System Exclusive data. It is more specific.
 *      -   ".raw". Basically any type of MIDI binary data.
 *          Could be any other file extension the user selects as raw
 *          data.
 *      -   We check for ".macro" and ".macros" first, and assume
 *          any other extension is a raw data file.
 *
 *  Macro data items:
 *
 *      -   Macro/Macros file.
 *          -   Name. The macro name is a single token, no spaces
 *              or special characters except '-' of '_'. Currently,
 *              the macro name is at the top of the file
 *              ("Seq66 macro: <macroname>").
 *          -   Is valid. Set when expanding works.
 *          -   Is expanded. By definition, true if it succeeds.
 *          -   Expanded tokens. These are given by one or more lines
 *              of hexadecimal value that are collected into one
 *              tokenization (vector of strings).
 *          -   Use file storage and file name. True and set by default.
 *          -   Event count. If '|' characters separate bytes, this
 *              is greater than one.
 *      -   Raw file.
 *          -   Name. We adopt the convention that the base name of
 *              the file, without the extension, is the name of the
 *              macro.
 *          -   Is valid.
 *          -   Is expanded.
 *          -   Expanded tokens.
 *          -   Use file storage and file name.
 *          -   Event count. This can only be zero; the event list
 *              is empty, and all the bytes are stored in one midibytes
 *              vector..
 */

midimacro
midimacros::read_midi_data (const std::string & fn)
{
    midimacro result;
    std::string ext { file_dot_extension(fn) };         /* keep the '.'     */
    if (ext == ".macro")
    {
        return read_macro_file(fn);
    }
    else if (ext == ".macros")
    {
        // TODO when we derive the file from configfile
        return midimacro();
    }
    else                                                /* assume binary    */
    {
        midimacro result;
        std::string base { filename_base(fn, true) };   /* strips the .ext  */
        midibytes byts { read_raw_midi(fn) };
        result.name(base);
        if (byts.size() > 0)
        {
            result.file_name(fn);
            result.bytes(byts);
            result.is_valid(true);
        }
        return result;
    }
}

/**
 *  This function is like read_midi_data(), but it assumes
 *  the macro parameter is valid due to a previous reading
 *  that results in a file-name being in used.
 */

bool
midimacros::get_midi_data
(
    midimacro & mac,
    const std::string & fn
)
{
    bool result { false };
    std::string fname { fn.empty() ? mac.file_name() : fn };
    std::string ext { file_dot_extension(fname) };  /* keep the '.'     */
    if (ext == ".macro")
    {
        mac = read_macro_file(fname);
        result = mac.is_valid();
    }
    else if (ext == ".macros")
    {
        // TODO when we derive the file from configfile
        result = false;
    }
    else                                            /* assume binary    */
    {
        std::string base
        {
            filename_base(fname, true)              /* strips the .ext  */
        };
        midibytes byts { read_raw_midi(fname) };
        mac.name(base);
        if (byts.size() > 0)
        {
            mac.file_name(fname);
            mac.bytes(byts);
            mac.is_valid(true);
            result = true;
        }
    }
    return result;
}

/**
 *  This functions the macro name and a stream of tokens representing
 *  data bytes (e.g. "0xF0") and macro variables ("$footer").
 *
 *  Format example:
 *
 *      Line 1:     "Seq66 macro: <macro name>\n"
 *      Line 2+i:   "$header 0xab 0xcd ...$footer\n"
 *
 *  This function reads a file's lines into a vector of strings, with
 *  comment lines ("#") ignored, whitespace trimmed, newlines not
 *  stored.
 *
 *  Each line is then tokenized, and the tokens are appended in the
 *  macro.
 *
 *  This file will eventually follow the configfile conventions.
 */

midimacro
midimacros::read_macro_file (const std::string & fn)
{
    midimacro result;                   /* name() empty, is_valid() false   */
    bool ok { file_readable(fn) };
    if (ok)
    {
        tokenization & tokens { result.tokens() };
        tokenization lines;
        bool usefilename { false };   /* file might contain "file: fname" */
        std::string datafilename { };
        ok = file_read_lines(fn, lines, true);
        if (ok)
        {
            int count { 0 };
            for (const auto & line : lines)
            {
                tokenization linetokens { tokenize(line) };
                if (count == 0)
                {
                    if (linetokens[0] == "Seq66" && linetokens[1] == "macro:")
                    {
                        result.name(linetokens[2]);
                        ++count;
                        continue;
                    }
                    else
                    {
                        /*
                         * Not a Seq66 macro file.
                         */

                        break;
                    }
                }
                else
                {
                    for (const auto & t : linetokens)
                    {
                        /*
                         * file_read_lines() ignores empty lines and lines
                         * starting with a '#' (hash-mark).
                         *
                         * if (! t.empty() && t[0] != '#')
                         */

                        tokens.push_back(t);
                    }
                    if (tokens[0] == midimacro::file_marker())
                    {
                        datafilename = tokens[1];
                        usefilename = ! datafilename.empty();
                    }
                    ++count;
                }
            }
            if (count > 0)
            {
                if (usefilename)
                {
                    /*
                     * This function calls read_file_macro() [again] with
                     * the file-name stored in the macro file.
                     *
                     */

                    (void) get_midi_data(result, datafilename);
                }
                else
                {
                    /*
                     * This seems wasteful, but it is temporary. The bytes
                     * are already set in the events-list.
                     */

                    midibytes mbs { expand_macro(result) };
                    if (mbs.size() > 0)
                    {
                        /*
                         * We're not using the "file: <macnam>" since
                         * byte digits are the tokens.
                         *
                         *  result.file_name(fn);
                         */

                        result.is_valid(true);
                    }
                }
            }
        }
    }
    return result;
}

/**
 * \param macro
 *      Provides the information to write.
 *
 * \param fn
 *      The filename to write. If empty, the macro's file-name is
 *      used (if not empty).
 *
 * \return
 *      Returns true if there was data to write and it succeeded.
 */

bool
midimacros::write_midi_data
(
    midimacro & macro,
    const std::string & fn
)
{
    bool result { false };
    std::string filename { fn.empty() ? macro.file_name() : fn };
    if (! filename.empty())
    {
        std::string ext { file_dot_extension(filename) };   /* keep the '.' */
        if (ext == ".macro")
        {
            result = write_macro_file(macro, filename);
        }
        else if (ext == ".macros")
        {
            // TODO when we derive the file from configfile
            result = false;
        }
        else                                                /* assume raw   */
        {
            midibytes byts { macro.bytes() };
            if (byts.size() > 0 && macro.is_valid())
            {
                result = write_raw_midi(filename, byts);
                if (result)
                    macro.file_name(filename);
            }
        }
    }
    return result;
}

bool
midimacros::write_macro_file
(
    const midimacro & macro,
    const std::string & fn
)
{
    bool result { macro.is_valid() };
    if (result)
    {
        tokenization lines;
        std::string text { macro_header + macro.name() };
        lines.push_back(text);
        lines.push_back("\n");
        lines.push_back
        (
            "# This file contains data for a single Seq66 macro. Each line"
        );
        lines.push_back
        (
            "# contains strings that are converted to MIDI bytes."
        );
        lines.push_back("\n");
        if (macro.tokens().empty())
        {
            tokenization bytelines { macro.bytes_to_lines() };
            for (auto & bl : bytelines)
                lines.push_back(bl);
        }
        else
        {
            tokenization toklines { macro.tokens() };
            if (toklines[0] == midimacro::file_marker())
            {
                std::string tokfilespec { toklines[0] };
                tokfilespec += " ";
                tokfilespec += toklines[1];
                lines.push_back(tokfilespec);
            }
            else
            {
                /*
                 * Assemble the existing macro tokens into lines and push
                 * them all.
                 */

                tokenization datalines { macro.bytes_to_lines() };
                for (const auto & dl : datalines)
                    lines.push_back(dl);
            }
        }
        lines.push_back("\n");
        lines.push_back("# vim: sw=4 ts=4 wm=4 et ft=dosini");
        result = file_write_lines(fn, lines);
    }
    return result;
}

}           // namespace seq66

/*
 * midimacros.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

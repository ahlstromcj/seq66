#if ! defined SEQ66_MIDIMACRO_HPP
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
 * \updates       2026-08-27
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

public:

    using events = std::vector<midibytes>;

private:

    /**
     *  The name of the macro.  This is also the key value for putting the
     *  midimacro in a container.
     */

    std::string m_name { };

    /**
     *  This is a list of tokens making up the macro. Although it can take up
     *  extra space, it is useful to write the macro back to the configuration
     *  file. It also allows putting multiple events into one macro.
     *
     *  Also see the tokenize() function in the strfunctions module.
     */

    tokenization m_tokens { };

    /**
     *  Indicates the macro has already been expanded and the bytes
     *  are fine as is.
     */

    bool m_is_expanded { false };

    /**
     *  The number of events in the macro. Normally just one, unless
     *  the vertical bar ("|") occurs in the list of tokens. If
     *  data comes from a raw file, then this count is 1, and
     *  m_events_bytes[0] contains the full list of midibytes to be
     *  sent via this macro after expanding any macros it includes.

    int m_event_count { 0 };
     */

    /**
     *  Provides the midibytes for each separate event in a multiple-event
     *  macro. Populated multiply if the separator bar ("|") was present.
     *  Also includes expanded macros' bytes.
     */

    events m_event_bytes { };

    /**
     *  Is the macro good?  It is good if there is a name, if there's at least
     *  one byte value or reference token, and the byte value isn't 0.
     *  Even if invalid, the macro will be loaded and saved.
     */

    bool m_is_valid { false };

    /**
     *  If true, the first token in m_tokens[0] is "file:", and the
     *  second is the file-name for storage of the data.
     *
     *  It can indicate that the data is too large, more than 6 x 3 bytes,
     *  and needs to be saved to a file. We test the number of
     *  expanded bytes (that would be saved to a line in the 'ctrl' file)
     *  against this value. It's about 2 * 72.
     *
     *  But even small data can use a file, if the user wants it.
     */

    bool m_use_file_storage { false };

    /**
     *  Active file specification. Saved for the "Save" function.
     *  Normally this will be a raw data file, not an ASCII text file.
     *
     *  Though we could support a file of this format:
     *
     *      Line 1:     "Macro rpnpitch"
     *      Line > 1:   A long string of ASCII byte values ("0xff"),
     *                  concatenated into one super long line, newlines
     *                  ignored, with optional variables that can be
     *                  expanded to get the bytes. Stored in m_tokens.
     */

    std::string m_file_name { };

public:

    midimacro () = default;
    midimacro (const std::string & name, const std::string & values);
    midimacro (const tokenization & name_and_tokens);
    midimacro (const midimacro &) = default;
    midimacro & operator = (const midimacro &) = default;
    midimacro (midimacro &&) = default;
    midimacro & operator = (midimacro &&) = default;
    virtual ~midimacro () = default;

    static const std::string & file_marker ();

    const std::string & name () const
    {
        return m_name;
    }

    void name (const std::string & n)
    {
        m_name = n;
    }

    tokenization & tokens ()
    {
        return m_tokens;
    }

    const tokenization & tokens () const
    {
        return m_tokens;
    }

    std::string line () const;

    midibytes bytes (int index = (-1)) const;

    bool use_file_storage () const
    {
        return m_use_file_storage;
    }

    const std::string & file_name () const
    {
        return m_file_name;
    }

    void file_name (const std::string & s)
    {
        m_file_name = s;
        use_file_storage(! s.empty());
    }

    int event_count () const
    {
        return int(m_event_bytes.size());
    }

    bool is_expanded () const
    {
        return m_is_expanded;
    }

    bool is_valid () const
    {
        return m_is_valid;
    }

protected:

    tokenization bytes_to_lines () const;

    void is_expanded (bool flag)
    {
        m_is_expanded = flag;
    }

    /*
     * Specifically for use in rpn::create_rpn_events().
     */

    void is_valid (bool flag)
    {
        m_is_valid = flag;
    }

    void use_file_storage (bool flag)
    {
        m_use_file_storage = flag;
    }

    bool tokenize (const std::string & values);

    void bytes (const midibytes & b)
    {
        push_bytes(b);
    }

    events & event_bytes_list ()
    {
        return m_event_bytes;
    }

    const events & event_bytes_list () const
    {
        return m_event_bytes;
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

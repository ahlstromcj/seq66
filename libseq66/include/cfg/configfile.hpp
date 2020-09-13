#if ! defined SEQ66_CONFIGFILE_HPP
#define SEQ66_CONFIGFILE_HPP

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
 * \file          configfile.hpp
 *
 *  This module declares the abstract base class for configuration and
 *  options files.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-23
 * \updates       2020-09-13
 * \license       GNU GPLv2 or above
 *
 *  This is actually an elegant little parser, and works well as long as one
 *  respects its limitations.
 */

#include <fstream>
#include <string>

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Currently these strings are just for reference, and not yet used in the
 *  code.
 *
 *  std::string filetag = "config-type";
 *  std::string ctrltag = "ctrl";
 *  std::string mutetag = "mutes";
 *  std::string rctag   = "rc";
 *  std::string usrtag  = "usr";
 *  std::string playtag = "playlist";
 *
 */

/*
 * Forward references.
 */

class rcsettings;

/**
 *    This class is the abstract base class for rcfile and usrfile.
 */

class configfile
{

private:

    /**
     *  Holds the last error message, if any.  Not a 100% foolproof yet.
     */

    static std::string sm_error_message;

    /**
     * Indicates if we are in an error status.
     */

    static bool sm_is_error;

private:

    /**
     *  Hold a reference to the "rc" settings object.
     */

    rcsettings & m_rc;

    /**
     *  Provides the name of the configuration or other file being parsed.
     *  This will normally be a full-path specification.
     */

    std::string m_name;

    /**
     *  Provides the current version of the derived configuration file.  Mostly
     *  useful to turn on the "--user-save" option for changes in the format of
     *  the "usr" file.
     */

    std::string m_version;

protected:

    /**
     *  The current line of text being processed.  This member receives
     *  an input line, and so needs to be a character buffer.
     */

    std::string m_line;

    /**
     *  Holds the stream position before a line is obtained.
     */

    std::streampos m_prev_pos;

public:

    configfile (const std::string & name, rcsettings & rcs);

    configfile () = delete;
    configfile (const configfile &) = delete;
    configfile & operator = (const configfile &) = delete;
    configfile (configfile &&) = default;
    configfile & operator = (configfile &&) = default;

    /**
     *  A rote destructor needed for a base class.
     */

    virtual ~configfile()
    {
        // empty body
    }

    virtual bool parse () = 0;
    virtual bool write () = 0;

    std::string parse_comments (std::ifstream & file);
    std::string parse_version (std::ifstream & file);

    const std::string & name () const
    {
        return m_name;
    }

    void name (const std::string & n)
    {
        m_name = n;
    }

    const std::string & version () const
    {
        return m_version;
    }

    bool bad_position (int p) const
    {
        return p < 0;
    }

    static const std::string & error_message ()
    {
        return sm_error_message;
    }

    static bool is_error ()
    {
        return sm_is_error;
    }

protected:

    static void append_error_message (const std::string & msg);
    static bool make_error_message
    (
        const std::string & sectionname,
        const std::string & additional = ""
    );

    void version (const std::string & v)
    {
        if (! v.empty())
            m_version = v;
    }

    rcsettings & rc_ref ()
    {
        return m_rc;
    }

    /**
     *  Sometimes we need to know if there are new data lines at the end of an
     *  existing section.  One clue that there is not is that we're at the
     *  next section marker.  This function tests for that condition.
     *
     * \return
     *      Returns true if m_line[0] is the left-bracket character.
     */

    bool at_section_start () const
    {
        return m_line[0] == '[';
    }

    /**
     *  Provides the input string, to keep m_line private.
     *
     * \return
     *      Returns a constant reference to m_line.
     */

    const std::string & line () const
    {
        return m_line;
    }

    std::string trimline () const;

    /**
     *  Provides a pointer to the input string in a form that sscanf() can use.
     *
     * \return
     *      Returns a constant character pointer to the data in m_line.
     */

    const char * scanline () const
    {
        return m_line.c_str();
    }

    bool get_line (std::ifstream & file, bool strip = true);
    bool line_after
    (
        std::ifstream & file,
        const std::string & tag,
        int position = 0,
        bool strip = true
    );
    int find_tag (std::ifstream & file, const std::string & tag);
    int get_tag_value (const std::string & tag);
    bool next_data_line (std::ifstream & file, bool strip = true);
    bool next_section (std::ifstream & file, const std::string & tag);
    std::string get_variable
    (
        std::ifstream & file,
        const std::string & tag,
        const std::string & variablename,
        int position = 0
    );

};          // class configfile

}           // namespace seq66

#endif      // SEQ66_CONFIGFILE_HPP

/*
 * configfile.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


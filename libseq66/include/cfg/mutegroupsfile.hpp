#if ! defined SEQ66_MUTEGROUPSFILE_HPP
#define SEQ66_MUTEGROUPSFILE_HPP

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
 * \file          mutegroupsfile.hpp
 *
 *  This module declares/defines the base class for managind the qseq66.mutes
 *  configuration file.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-30
 * \updates       2021-04-27
 * \license       GNU GPLv2 or above
 *
 *  Provides support for a mute-groups configuration file.
 */

#include <fstream>                      /* std::ofstream and ifstream       */

#include "cfg/configfile.hpp"           /* seq66::configfile class          */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

class mutegroups;                       /* forward reference to class       */

/**
 *  Provides a file for reading and writing the application's mute-group
 *  configuration file.  The settings that are passed around are provided
 *  or used by the performer class.
 */

class mutegroupsfile final : public configfile
{
    friend class rcfile;

private:

    /**
     *  Indicates to use the old 4 x 8 format for a mute-groups stanza.
     */

    bool m_legacy_format;

    /**
     *  Indicates if empty mute-group stanzas are to be read in.  An empty
     *  stanza is all 0's (all false).  This option is useful in conversion to
     *  the new format.
     */

    bool m_allow_inactive;

    /**
     *  Similar to mutegroup::m_rows_in_group.  The default value is 4.
     */

    const int m_section_count;

    /**
     *  Similar to mutegroup::m_columns_in_group.  The default value is 8.
     */

    const int m_mute_count;

public:

    mutegroupsfile
    (
        const std::string & filename,
        rcsettings & rcs,
        bool allowinactive = false
    );

    mutegroupsfile () = delete;
    mutegroupsfile (const mutegroupsfile &) = delete;
    mutegroupsfile & operator = (const mutegroupsfile &) = delete;

    /*
     * WTF?
     *
    mutegroupsfile (mutegroupsfile &&) = default;
    mutegroupsfile & operator = (mutegroupsfile &&) = default;
     */

    virtual ~mutegroupsfile ();

    virtual bool parse () override;
    virtual bool write () override;

    bool parse_stream (std::ifstream & file);
    bool write_stream (std::ofstream & file);

private:

    bool parse_mutes_stanza (mutegroups & mutes);
    bool write_mute_groups (std::ofstream & file);

};              // class mutegroupsfile

/*
 * Free functions in the seq66 namespace.
 */

extern bool open_mutegroups (const std::string & source);
extern bool save_mutegroups (const std::string & destfile);

}               // namespace seq66

#endif          // SEQ66_MUTEGROUPSFILE_HPP

/*
 * mutegroupsfile.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


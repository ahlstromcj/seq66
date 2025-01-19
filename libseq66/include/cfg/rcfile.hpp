#if ! defined SEQ66_RCFILE_HPP
#define SEQ66_RCFILE_HPP

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
 * \file          rcfile.hpp
 *
 *  This module declares/defines the base class for managing the qseq66.rc
 *  configuration file.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2025-01-19
 * \license       GNU GPLv2 or above
 *
 *  The ~/.seq66rc or ~/.config/seq66.rc files are referred to as the "rc"
 *  files.
 */

#include "cfg/configfile.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Provides a file for reading and writing the application' main
 *  configuration file.  The settings that are passed around are provided
 *  or used by the performer class.
 */

class rcfile final : public configfile
{

private:

    /*
     * Currently no additional members.
     */

public:

    rcfile () = delete;
    rcfile (const std::string & name, rcsettings & rcs);
    rcfile (const rcfile &) = delete;
    rcfile & operator = (const rcfile &) = delete;
    virtual ~rcfile () = default;

    virtual bool parse () override;
    virtual bool write () override;
    bool parse_midi_control_section (const std::string & fname);
    bool get_usr_file ();

};          // class rcfile

/*
 *  Free functions.
 */

#if defined SEQ66_KEEP_RC_FILE_LIST

extern bool delete_configuration
(
    const std::string & path,
    const std::string & basename
);
extern bool copy_configuration
(
    const std::string & source,
    const std::string & basename,
    const std::string & destination
);

#endif  // defined SEQ66_KEEP_RC_FILE_LIST

}           // namespace seq66

#endif      // SEQ66_RCFILE_HPP

/*
 * rcfile.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


#if ! defined SEQ66_NOTEMAPFILE_HPP
#define SEQ66_NOTEMAPFILE_HPP

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
 * \file          notemapfile.hpp
 *
 *  This module declares/defines the base class for managind the ~/.seq66rc
 *  configuration file.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-11-05
 * \updates       2019-11-08
 * \license       GNU GPLv2 or above
 *
 *  Provides support for a mute-groups configuration file.
 */

#include <fstream>                      /* std::ofstream and ifstream       */

#include "cfg/configfile.hpp"           /* seq66::configfile class          */
#include "play/notemapper.hpp"          /* seq66::notemapper                */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Provides a file for reading and writing the application's mute-group
 *  configuration file.  The settings that are passed around are provided
 *  or used by the performer class.
 */

class notemapfile final : public configfile
{

private:

    /**
     *  Holds a reference to the notemapper object to be acted upon by this
     *  class.
     */

    notemapper & m_note_mapper;

public:

    notemapfile
    (
        notemapper & m_note_mapper,
        const std::string & filename,
        rcsettings & rcs
    );

    notemapfile () = delete;
    notemapfile (const notemapfile &) = delete;
    notemapfile & operator = (const notemapfile &) = delete;
    notemapfile (notemapfile &&) = default;
    notemapfile & operator = (notemapfile &&) = default;

    virtual ~notemapfile ();

    virtual bool parse () override;
    virtual bool write () override;

    bool parse_stream (std::ifstream & file);
    bool write_stream (std::ofstream & file);

private:

    bool write_map_entries (std::ofstream & file) const;

    notemapper & mapper ()
    {
        return m_note_mapper;
    }

};              // class notemapfile

}               // namespace seq66

#endif          // SEQ66_NOTEMAPFILE_HPP

/*
 * notemapfile.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


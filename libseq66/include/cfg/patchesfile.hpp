#if ! defined SEQ66_PATCHESFILE_HPP
#define SEQ66_PATCHESFILE_HPP

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
 * \file          patchesfile.hpp
 *
 *  This module declares/defines the base class for managind the qseq66.drums
 *  configuration file.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-11-05
 * \updates       2025-02-19
 * \license       GNU GPLv2 or above
 *
 *  Provides support for a mute-groups configuration file.
 */

#include <fstream>                      /* std::ofstream and ifstream       */

#include "cfg/configfile.hpp"           /* seq66::configfile class          */
#include "midi/patches.hpp"             /* seq66::patches                   */

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

class patchesfile final : public configfile
{

private:

    /**
     *  Holds a reference to the patches object to be acted upon by this
     *  class. However, currently we access the patches class solely
     *  via public free functions. See the patches modules.
     *
     *          patches & m_note_mapper;
     */

public:

    patchesfile
    (
        const std::string & filename,
        rcsettings & rcs
    );

    patchesfile () = delete;
    patchesfile (const patchesfile &) = delete;
    patchesfile & operator = (const patchesfile &) = delete;
    virtual ~patchesfile ();

    virtual bool parse () override;
    virtual bool write () override;

    bool parse_stream (std::ifstream & file);
    bool write_stream (std::ofstream & file);

private:

    bool write_map_entries (std::ofstream & file) const;

};              // class patchesfile


/*
 *  Free functions
 */

extern bool open_patches (const std::string & source);
extern bool save_patches (const std::string & destination);
extern bool save_patches
(
    const std::string & source,
    const std::string & destination
);

#if 0

extern bool copy_patches
(
    patches & pl,
    const std::string & source,
    const std::string & destination
);

#endif

}               // namespace seq66

#endif          // SEQ66_PATCHESFILE_HPP

/*
 * patchesfile.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


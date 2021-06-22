#if ! defined SEQ66_PALETTEFILE_HPP
#define SEQ66_PALETTEFILE_HPP

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
 * \file          palettefile.hpp
 *
 *  This module declares/defines the base class for managind the ~/.seq66rc
 *  configuration file.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2020-12-21
 * \updates       2021-06-22
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

class gui_palette_qt5;

/**
 *  Provides a file for reading and writing the application's mute-group
 *  configuration file.  The settings that are passed around are provided
 *  or used by the performer class.
 */

class palettefile final : public configfile
{

private:

    /**
     *  Holds a reference to the palette object to be acted upon by this
     *  class.
     */

    gui_palette_qt5 & m_palettes;

public:

    palettefile
    (
        gui_palette_qt5 & palettes,
        const std::string & filename,
        rcsettings & rcs
    );

    palettefile () = delete;
    palettefile (const palettefile &) = delete;
    palettefile & operator = (const palettefile &) = delete;
    ~palettefile () = default;

    virtual bool parse () override;
    virtual bool write () override;

    bool parse_stream (std::ifstream & file);
    bool write_stream (std::ofstream & file);

    static int palette_size ()
    {
        return 32;
    }

private:

    bool write_map_entries (std::ofstream & file) const;

    gui_palette_qt5 & mapper ()
    {
        return m_palettes;
    }

};              // class palettefile

/*
 *  Free functions for working with play-list files.
 */

extern bool open_palette
(
    gui_palette_qt5 & pal,
    const std::string & source
);
extern bool save_palette
(
    gui_palette_qt5 & pal,
    const std::string & destination
);
extern bool save_palette
(
    gui_palette_qt5 & pal,
    const std::string & source,
    const std::string & destination
);

}               // namespace seq66

#endif          // SEQ66_PALETTEFILE_HPP

/*
 * palettefile.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


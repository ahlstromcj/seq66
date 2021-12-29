#if ! defined SEQ66_SESSIONFILE_HPP
#define SEQ66_SESSIONFILE_HPP

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
 * \file          sessionfile.hpp
 *
 *  This module declares/defines the base class for managing the special
 *  read-only session.rc configuration file.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2021-12-29
 * \updates       2021-12-29
 * \license       GNU GPLv2 or above
 *
 */

#include "cfg/configfile.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Provides a file for reading a special session setup, usually for testing.
 */

class sessionfile final : public configfile
{

private:

    /*
     * Currently no additional members.
     */

public:

    sessionfile (const std::string & name, rcsettings & rcs);

    sessionfile () = delete;
    sessionfile (const sessionfile &) = delete;
    sessionfile & operator = (const sessionfile &) = delete;
    virtual ~sessionfile () = default;

    virtual bool parse () override;

    virtual bool write () override
    {
        return false;
    }

};          // class sessionfile

}           // namespace seq66

#endif      // SEQ66_SESSIONFILE_HPP

/*
 * sessionfile.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


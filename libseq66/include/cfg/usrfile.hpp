#if ! defined SEQ66_USRFILE_HPP
#define SEQ66_USRFILE_HPP

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
 * \file          usrfile.hpp
 *
 *  This module declares/defines the base class for managing the user's
 *  qseq66.usr configuration file.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-24
 * \updates       2019-03-26
 * \license       GNU GPLv2 or above
 *
 */

#include <string>

#include "cfg/configfile.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  A manifest constant for controlling the length of a line-reading array for
 *  use as a destination in a sscanf() call while parsing a configuration file.
 *  The value of 1024 should be more than enough, even given the parsing of long
 *  file-specifications.  But be careful!!!  A line longer than this causes
 *  std::ifstream::getline() to go off into the ozone.  This variable is now
 *  used only on the usrfile class.
 */

const int SEQ66_LINE_MAX = 1024;        /* 132 is *not* enough for paths    */

/**
 *    Supports the user's <code> ~/.config/sequencer66/seq66.usr </code>
 *    configuration file.
 */

class usrfile final : public configfile
{

public:

    usrfile (const std::string & name, rcsettings & rcs);

    usrfile () = delete;
    usrfile (const usrfile &) = delete;
    usrfile & operator = (const usrfile &) = delete;

    /*
     * WTF?
     *
    usrfile (usrfile &&) = default;
    usrfile & operator = (usrfile &&) = default;
     */

    virtual ~usrfile ();

    virtual bool parse () override;
    virtual bool write () override;

private:

    void dump_setting_summary ();

};          // class usrfile

}           // namespace seq66

#endif      // SEQ66_USRFILE_HPP

/*
 * usrfile.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


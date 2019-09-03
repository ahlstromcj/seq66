#if ! defined SEQ66_OPTIONSFILE_HPP
#define SEQ66_OPTIONSFILE_HPP

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
 * \file          optionsfile.hpp
 *
 *  This module declares/defines the base class for managind the ~/.seq66rc
 *  configuration file.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2019-02-03
 * \license       GNU GPLv2 or above
 *
 *  The ~/.seq66rc or ~/.config/seq66.rc files are
 *  referred to as the "rc" files.  Note that there are other variations on
 *  the name for the different versions of Seq66 that can be built.
 *
 *  This class only reads, and it reads only the older (Sequencer64) format of
 *  "rc" file.
 */

#include "cfg/configfile.hpp"
#include "cfg/rcsettings.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class performer;

/**
 *  Provides a file for reading and writing the application's main
 *  configuration file.  The settings that are passed around are provided
 *  or used by the performer class.
 */

class optionsfile : public configfile
{

private:

    std::string m_out_name;

public:

    optionsfile (rcsettings & rcs, const std::string & name);
    virtual ~optionsfile ();
    virtual bool parse ();

    /**
     *  This class cannot write "rc" files.
     */

    virtual bool write ()
    {
        return false;
    }

    bool parse_mute_group_section ();
    bool parse_midi_control_section (const std::string & fname);

private:

    bool make_error_message
    (
        const std::string & sectionname,
        const std::string & additional = ""
    );
    bool merge_key
    (
        automation::category opcat,
        unsigned key,
        unsigned slotnumber
    );
    midibooleans ints_to_booleans (const int * iarray, int len);

};          // class optionsfile

}           // namespace seq66

#endif      // SEQ66_OPTIONSFILE_HPP

/*
 * optionsfile.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


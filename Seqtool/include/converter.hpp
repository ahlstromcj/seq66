 #if ! defined SEQ66_CONVERTER_HPP
 #define SEQ66_CONVERTER_HPP

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
 * \file          converter.hpp
 * \library       Seqtool (from the Seq66 project)
 * \author        Chris Ahlstrom
 * \date          2018-12-07
 * \updates       2019-01-01
 * \version       $Revision$
 * \license       $XPC_SUITE_GPL_LICENSE$
 *
 *    This module provides file-converter functions for seqtool application.
 */

#include <string>

#include "cfg/rcsettings.hpp"           /* seq66::rcsettings configuration  */

namespace seq66
{

/**
 *  Provides a class to contain the data needed to convert "rc" files.
 */

class converter
{

private:

    /**
     *  The rcsettings object to use during the conversion.
     */

    rcsettings m_rc_settings;

    /**
     *  Base name of the input file involved in the conversion.
     */

    std::string m_in_base_filename;

    /**
     *  Base name of the output file(s) involved in the conversion.
     */

    std::string m_out_base_filename;

    /**
     *  The name of the old/legacy Sequencer64 configuration file.
     *  It will always be located in the ~/.config/sequencer64 configuration
     *  directory.  This is the full path file-specification for this file.
     */

    std::string m_input_filename;

    /**
     *  The name of the new Sequencer66 configuration file.  It will always be
     *  located in the ~/.config/sequencer66 configuration directory.  This is
     *  the full path file-specification for this file.
     */

    std::string m_output_filename;

    /**
     *  The name of the new Sequencer66 control file.  It will always be
     *  located in the ~/.config/sequencer66 configuration directory.  This is
     *  the full path file-specification for this file.  It has the
     *  file extension ".ctrl".
     */

    std::string m_ctrl_filename;

    /**
     *  The name of the new Sequencer66 mute-group (mutes) file.  It will
     *  always be located in the ~/.config/sequencer66 configuration directory.
     *  This is the full path file-specification for this file.  It has the
     *  file extension ".mutes".
     */

    std::string m_mutes_filename;

    /**
     *  The name of the new Sequencer66 playlist file.  It will always be
     *  located in the ~/.config/sequencer66 configuration directory.  This is
     *  the full path file-specification for this file.  It has the file
     *  extension ".playlist".  Currently it is just a renamed copy of the
     *  original playlist file.
     */

    std::string m_playlist_filename;

    /**
     *  The full path-name of the old Sequencer64 "user" file.  It has the same
     *  base name as the "rc" file, but a "usr" extension.
     */

    std::string m_user_filename;

    /**
     *  Indicates that the input file is good.
     */

    bool m_input_file_exists;

public:

    converter ();
    converter
    (
        rcsettings & configuration,
        const std::string & inbasefilename = "sequencer64",
        const std::string & outbasefilename = "seq66"
    );

    bool parse ();
    bool write ();

    std::string input_filename () const
    {
        return m_input_filename;
    }

    std::string output_filename () const
    {
        return m_output_filename;
    }

    std::string ctrl_filename () const
    {
        return m_ctrl_filename;
    }

    std::string mutes_filename () const
    {
        return m_mutes_filename;
    }

    std::string playlist_filename () const
    {
        return m_playlist_filename;
    }

    std::string user_filename () const
    {
        return m_user_filename;
    }

    bool input_file_exists () const
    {
        return m_input_file_exists;
    }

private:

    void initialize ();
    void show ();

};              // class converter

}               // namespace seq66

#endif          // SEQ66_CONVERTER_HPP

/*
 * converter.hpp
 *
 * vim: ts=4 sw=4 et ft=cpp
 */


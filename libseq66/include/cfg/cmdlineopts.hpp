#if ! defined SEQ66_CMDLINEOPTS_HPP
#define SEQ66_CMDLINEOPTS_HPP

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
 * \file    cmdlineopts.hpp
 *
 *    Provides the declarations for safe replacements for some C++
 *    file functions.
 *
 * \author  Chris Ahlstrom
 * \date    2015-11-20
 * \updates 2019-09-01
 * \version $Revision$
 *
 *    Also see the filefunctions.cpp and strfunctions modules.
 *    These modules together simplify the main() module considerably, which
 *    will be useful when we have more than one "Seq66" application.
 */

#include <string>
#include "seq66_features.hpp"           /* seq66::seq_version_text()        */

#if defined SEQ66_PLATFORM_UNIX || defined SEQ66_PLATFORM_MINGW
#include <getopt.h>
#endif

/*
 * This is the main namespace of Seq66.  Do not attempt to
 * Doxygenate the documentation here; it breaks Doxygen.
 */

namespace seq66
{

class rcsettings;

/**
 *  Provides a return value for parse_command_line_options() that indicates a
 *  help-related option was specified.
 */

const int SEQ66_NULL_OPTION_INDEX = 99999;

/**
 *  A wrapper class so that the whole class can be a friend.
 */

class cmdlineopts
{

private:

    static const std::string versiontext;
    static struct option s_long_options [];
    static const std::string s_arg_list;
    static const std::string s_help_1a;
    static const std::string s_help_1b;
    static const std::string s_help_2;
    static const std::string s_help_3;
    static const std::string s_help_4a;
    static const std::string s_help_4b;
    static const std::string s_help_5;

public:

    cmdlineopts ()
    {
        // no code
    }

    cmdlineopts (const cmdlineopts &) = default;
    cmdlineopts & operator = (const cmdlineopts &) = default;
    cmdlineopts (cmdlineopts &&) = default;
    cmdlineopts & operator = (cmdlineopts &&) = default;
    ~cmdlineopts () = default;

    static bool help_check (int argc, char * argv []);
    static bool parse_options_files
    (
        std::string & errmessage, int argc, char * argv []
    );
    static bool parse_mute_groups
    (
        rcsettings & rcs,
        std::string & errmessage
    );
    static bool parse_o_options (int argc, char * argv []);
    static bool parse_log_option (int argc, char * argv []);
    static int parse_command_line_options (int argc, char * argv []);
    static bool write_options_files (const std::string & errrcname = "");

private:

    static void show_help ();
    static std::string get_compound_option
    (
        const std::string & compound,
        std::string & optionname
    );

};          // class cmdlineopts

#endif      // SEQ66_CMDLINEOPTS_HPP

}           // namespace seq66

/*
 * cmdlineopts.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


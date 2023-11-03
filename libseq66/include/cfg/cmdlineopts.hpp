#if ! defined SEQ66_CMDLINEOPTS_HPP
#define SEQ66_CMDLINEOPTS_HPP

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
 * \file    cmdlineopts.hpp
 *
 *    Provides the declarations for safe replacements for some C++
 *    file functions.
 *
 * \author  Chris Ahlstrom
 * \date    2015-11-20
 * \updates 2023-11-03
 * \version $Revision$
 *
 *    Also see the filefunctions.cpp and strfunctions modules.
 *    These modules together simplify the main() module considerably, which
 *    will be useful when we have more than one "Seq66" application.
 */

#include <string>

#include "seq66_features.hpp"           /* seq66::seq_version_text(), etc.  */

#if defined SEQ66_PLATFORM_MING_OR_UNIX
#include <getopt.h>                     /* struct option s_long_options []  */
#endif

/*
 * This is the main namespace of Seq66.  Do not attempt to
 * Doxygenate the documentation here; it breaks Doxygen.
 */

namespace seq66
{

class rcsettings;

/**
 *  A wrapper class so that the whole class can be a friend.
 */

class cmdlineopts
{

private:

    static const std::string versiontext;
    static struct option s_long_options [];
    static const std::string s_optstring;

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
    static bool kill_check (int argc, char * argv []);
    static bool verbose_check (int argc, char * argv []);
    static bool parse_options_files (std::string & errmessage);
    static bool parse_rc_file
    (
        const std::string & filespec,
        std::string & errmessage
    );
    static bool get_usr_file ();
    static bool parse_usr_file
    (
        const std::string & filespec,
        std::string & errmessage
    );
    static bool parse_daemonization
    (
        bool & startdaemon,
        std::string & logfile
    );
    static bool parse_o_options (int argc, char * argv []);
    static bool parse_o_sets (const std::string & arg);
    static bool parse_o_mutes (const std::string & arg);
    static bool parse_o_virtual (const std::string & arg);
    static bool parse_log_option (int argc, char * argv []);
    static int parse_command_line_options (int argc, char * argv []);
    static void show_locale ();
    static bool set_global_locale (const std::string & lname = "");
    static bool write_options_files (const std::string & filename = "");
    static bool write_rc_file (const std::string & filename = "");
    static bool write_usr_file (const std::string & filename = "");
    static bool alt_write_rc_file (const std::string & filebase);
    static bool alt_write_usr_file (const std::string & filebase);

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


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
 * \file          sessionfile.cpp
 *
 *  This module declares/defines the base class for managing the special
 *  session.rc file.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2021-12-29
 * \updates       2021-12-29
 * \license       GNU GPLv2 or above
 *
 *  This file is a read-only file created manually by the user in order
 *  to create an isolated consistent setup, e.g. for testing.
 */

#include "cfg/sessionfile.hpp"          /* seq66::sessionfile class         */
#include "cfg/settings.hpp"             /* seq66::rc() accessor             */
#include "util/filefunctions.hpp"       /* seq66::file_extension_set()      */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

static const int s_session_file_version = 0;

/**
 *  Principal constructor.
 *
 * Versions:
 *
 *      0:  The initial version.
 *
 * \param rcs
 *      The destination for the configuration information.
 *
 * \param name
 *      Provides the name of the options file; this is a pathless
 *      file-specification.
 */

sessionfile::sessionfile (const std::string & name, rcsettings & rcs) :
    configfile  (name, rcs)
{
    version(s_session_file_version);
}

/**
 *  Parse the ~/.config/seq66/session.rc file.
 *
 * \return
 *      Returns true if the file was able to be opened for reading.
 *      Currently, there is no indication if the parsing actually succeeded.
 */

bool
sessionfile::parse ()
{
    bool result = false;
    std::ifstream file(name(), std::ios::in | std::ios::ate);
    if (set_up_ifstream(file))            /* verifies [Seq66]: version    */
    {
        std::string tag = "[Seq66]";
        std::string s = get_variable(file, tag, "home");
        if (! s.empty())
        {
            rc_ref().full_config_directory(s, true);  /* add HOME perhaps */
            file_message("Set home config to", rc_ref().home_config_directory());
            if (make_directory_path(rc_ref().home_config_directory()))
                result = true;
            else
                errprint("Could not create that directory");
        }
        s = get_variable(file, tag, "config");
        if (! s.empty())
        {
            result = true;
            rc_ref().set_config_files(s);
        }
        s = get_variable(file, tag, "client-name");
        if (! s.empty())
        {
            result = true;
            set_client_name(s);
        }
        s = get_variable(file, tag, "log");
        if (! s.empty())
        {
            result = true;
            usr().option_logfile(s);
        }

        /*
         * [comments] is not parsed in this read-only file.
         */

    }
    file.close();               /* done parsing the "rc" file               */
    return result;
}

}           // namespace seq66

/*
 * sessionfile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


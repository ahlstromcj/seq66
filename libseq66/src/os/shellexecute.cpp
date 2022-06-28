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
 * \file          shellexecute.cpp
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2022-05-19
 * \updates       2022-06-28
 * \license       GNU GPLv2 or above
 *
 *  Provides support for cross-platform time-related functions.
 */

#include <cstdlib>                      /* int std::system(commandline)     */

#include "cfg/settings.hpp"             /* for usr().use_pdf_viewer() etc.  */
#include "util/basic_macros.hpp"        /* error_message()                  */
#include "os/shellexecute.hpp"          /* seq66::open_document(), etc.     */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Generic C++ method for executing a command-line. It must
 */

bool
command_line (const std::string & cmdline)
{
    bool result = ! cmdline.empty();
    if (result)
    {
        int rc = std::system(cmdline.c_str());
        result = rc == 0;
        if (! result)
            (void) file_error("Command failed", cmdline);
    }
    return result;
}

#if defined SEQ66_PLATFORM_LINUX

/**
 *  Opens a PDF file.  Meant to be made more flexible later:
 *
 *      -   Add a user-configurable path to a PDF view, if specified.
 *          Currently, qsmainwnd uses open_url() if the local file
 *          is not found.
 *      -   Handle executable and file paths with spaces.
 */

bool
open_pdf (const std::string & pdfspec)
{
    std::string cmd = usr().user_pdf_viewer();
    if (cmd.empty())
        cmd = "/usr/bin/xdg-open";

    cmd += " ";
    cmd += pdfspec;
    cmd += "&";
    return command_line(cmd);
}

bool
open_url (const std::string & url)
{
    std::string cmd = usr().user_browser();
    if (cmd.empty())
        cmd = "/usr/bin/xdg-open";

    cmd += " ";
    cmd += url;
    cmd += "&";
    return command_line(cmd);
}

/**
 *  Currently not used anywhere.
 */

bool
open_local_url (const std::string & url)
{
    return open_url(url);
}

#elif defined SEQ66_PLATFORM_WINDOWS

bool
open_pdf (const std::string & pdfspec)
{
    return command_line(cmd);
}

bool
open_url (const std::string & url)
{
    return command_line(url);
}

bool
open_local_url (const std::string & url)
{
    return open_url(url);
}

#endif

}           // namespace seq66

/*
 * vim: ts=4 sw=4 et ft=cpp
 */


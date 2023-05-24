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
 * \updates       2022-08-02
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

/*
 *  Information for the future. However, Because ShellExecute() can delegate
 *  execution to Shell extensions implementations) that are activated using
 *  Component Object Model (COM), COM should be initialized before
 *  ShellExecuteEx() is called.
 *
\verbatim
	HINSTANCE ShellExecuteA
	(
		[in, optional] HWND   hwnd,
		[in, optional] LPCSTR lpOperation,
		[in]           LPCSTR lpFile,
		[in, optional] LPCSTR lpParameters,
		[in, optional] LPCSTR lpDirectory,
		[in]           INT    nShowCmd
	);
\endverbatim
 *
 *  -	hwnd. A handle to the parent window used for displaying a UI or error
 *      messages. This value can be NULL if the operation is not associated
 *      with a window.
 *  -	lpOperation.  A verb that specifies the action to be performed.
 *      Generally, the actions available from an object's shortcut menu are
 *      usable. Common verbs for Seq66 usage: edit.  Launches an editor and
 *      opens the document for editing. If lpFile is not a document file, the
 *      function will fail.
 *      -   open.  Opens the item specified by the lpFile parameter. Can be a
 *          file or folder.
 *      -   NULL.  The default verb is used, if available. If not, the "open"
 *          verb is used. If neither verb is available, the system uses the
 *          first verb listed in the registry.
 *  -	lpFile.  A string that specifies the file/object on which to execute
 *      the verb.
 *  -	lpParameters.  If lpFile specifies an executable file, this parameter
 *      is a pointer to a null-terminated string that specifies the parameters
 *      to be passed to the application. If lpFile specifies a document,
 *      lpParameters should be NULL.
 *  -	lpDirectory.  A string specifying the default (working) directory for
 *      the action. If this value is NULL, the current working directory is
 *      used.
 *  -	nShowCmd.  The flags that specify how an application is to be displayed
 *      when it is opened.
 *  -	Return value.  Success is a value greater than 32. If the function
 *      fails, it returns an error value that indicates the cause of the
 *      failure. The return value is cast as an HINSTANCE for backward
 *      compatibility with 16-bit Windows applications. It is not a true
 *      HINSTANCE, however. It can be cast only to an INT_PTR and compared to
 *      either 32 or error codes. (See the web page).
 */

bool
open_pdf (const std::string & pdfspec)
{
    std::string cmd = usr().user_pdf_viewer();
    if (cmd.empty())
    {
        return command_line(pdfspec);
    }
    else
    {
        cmd += " ";
        cmd += pdfspec;
        return command_line(cmd);
    }
}

bool
open_url (const std::string & url)
{
    std::string cmd = usr().user_browser();
    if (cmd.empty())
    {
        return command_line(url);
    }
    else
    {
        cmd += " ";
        cmd += url;
        return command_line(url);
    }
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


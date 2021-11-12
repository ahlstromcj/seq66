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
 * \file          basic_macros.cpp
 *
 *  This module defines some informative functions that are actually
 *  better off as functions.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-10
 * \updates       2021-11-12
 * \license       GNU GPLv2 or above
 *
 *  One of the big new feature of some of these functions is writing the name of
 *  the application in color before each message that is put out.
 */

#include <assert.h>
#include <cstdarg>                      /* see "man stdarg(3)"              */
#include <iostream>

#include "cfg/settings.hpp"             /* seq66::rc()                      */
#include "util/basic_macros.hpp"        /* basic macros-cum-functions       */

#if defined SEQ66_PLATFORM_UNIX
#include <unistd.h>                     /* C::write(2), C::isatty(3)        */
#endif

#if defined SEQ66_PLATFORM_WINDOWS
#include <io.h>                         /* C::_write()                      */
#endif

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Provides a way to still get the benefits of assert() output in release
 *  mode, without aborting the application.
 *
 * \todo
 *      This can slow down client code slightly, and it would be good to
 *      reduce the impact.
 *
 * \param ptr
 *      Provides the pointer to be tested.
 *
 * \param context
 *      Provides context for the message.  Usually the __func__ macro is
 *      the best option for this parameter.
 *
 * \return
 *      Returns true in release mode, if the pointer was not null.  In
 *      debug mode, will always return true, but the assert() will abort
 *      the application anyway.
 */

#if defined SEQ66_PLATFORM_DEBUG

bool
not_nullptr_assert (void * ptr, const std::string & context)
{
    bool result = true;
    int flag = int(not_nullptr(ptr));
    if (! flag)
    {
        std::cerr
            << seq_client_tag(msglevel::error) << " null pointer in context "
            << context << std::endl;

        result = false;
    }

#if defined SEQ66_PLATFORM_GNU_TMI              /* does not work in Mingw   */
    int errornumber = flag ? 0 : 1 ;
    assert_perror(errornumber);
#else
    assert(flag);
#endif

    return result;
}

#endif  // SEQ66_PLATFORM_DEBUG

static bool
is_a_tty (int fd)
{
#if defined SEQ66_PLATFORM_WINDOWS
#define STDIN_FILENO    0
#define STDOUT_FILENO   1
#define STDERR_FILENO   2

    int fileno;
    switch (fd)
    {
        case STDIN_FILENO:  fileno = _fileno(stdin);    break;
        case STDOUT_FILENO: fileno = _fileno(stdout);   break;
        case STDERR_FILENO: fileno = _fileno(stderr);   break;
        default:            fileno = (-1);              break;
    }
    int rc = (fileno >= 0) ? _isatty(fileno) : 90;
    return rc == 1;                             /* fd refers to a terminal  */
#else
    int rc = isatty(fd);
    return rc == 1;                             /* fd refers to a terminal  */
#endif
}

/**
 *  Common-code for console informationational messages.  Adds markers and a
 *  newline.
 *
 * \param msg
 *      The message to print, sans the newline.
 *
 * \return
 *      Returns true, so that the caller can show the message and return the
 *      status at the same time.
 */

bool
info_message (const std::string & msg)
{
    if (rc().verbose())
    {
        bool isterminal = is_a_tty(STDOUT_FILENO);
        std::cout << seq_client_tag(msglevel::info, isterminal)
            << " " << msg;

        if (! msg.empty())
            std::cout << std::endl;
    }
    return true;
}

bool
status_message (const std::string & msg)
{
    bool isterminal = is_a_tty(STDOUT_FILENO);
    std::cout << seq_client_tag(msglevel::status, isterminal)
        << " " << msg << std::endl;

    return true;
}

bool
special_message (const std::string & msg)
{
    bool isterminal = is_a_tty(STDOUT_FILENO);
    std::cout << seq_client_tag(msglevel::special, isterminal)
        << " " << msg << std::endl;

    return true;
}

/**
 *  Common-code for console warning messages.  Adds markers and a newline.
 *
 * \param msg
 *      The message to print, sans the newline.
 *
 * \return
 *      Returns true, so that the caller can show the message and return the
 *      status at the same time.
 */

bool
warn_message (const std::string & msg)
{
    bool isterminal = is_a_tty(STDERR_FILENO);
    std::cerr << seq_client_tag(msglevel::warn, isterminal)
        << " " << msg << std::endl;

    return true;
}

/**
 *  Common-code for error messages.  Adds markers, and returns false.
 *
 * \param msg
 *      The message to print, sans the newline.
 *
 * \return
 *      Returns false for convenience/brevity in setting function return
 *      values.
 */

bool
error_message (const std::string & msg, const std::string & data)
{
    bool isterminal = is_a_tty(STDERR_FILENO);
    std::cerr << seq_client_tag(msglevel::error, isterminal) << " " << msg;
    if (! data.empty())
        std::cerr << ": " << data;

    std::cerr << std::endl;
    return false;
}

bool
debug_message (const std::string & msg, const std::string & data)
{
    if (rc().investigate())
    {
        bool isterminal = is_a_tty(STDERR_FILENO);
        std::cerr << seq_client_tag(msglevel::debug, isterminal) << " " << msg;
        if (! data.empty())
            std::cerr << ": " << data;

        std::cerr << std::endl;
    }
    return false;
}

/**
 *  Common-code for error messages involving file issues, a very common use
 *  case in error-reporting.  Adds markers, and returns false.
 *
 * \param tag
 *      The message to print, sans the newline.
 *
 * \param path
 *      The name of the file to be shown.
 *
 * \return
 *      Returns false for convenience/brevity in setting function return
 *      values.
 */

bool
file_error (const std::string & tag, const std::string & path)
{
    bool isterminal = is_a_tty(STDERR_FILENO);
    std::cerr << seq_client_tag(msglevel::error, isterminal) << " "
        << tag << ": '" << path << "'" << std::endl;

    return false;
}

/**
 *  Shows a path-name (or other C++ string) as an info message.  This output
 *  is not contingent upon debugging or verbosity.
 *
 * \param tag
 *      Provides the text to precede the name of the path.
 *
 * \param path
 *      Provides the path-name to print.  This message can be something other
 *      than a path-name, by the way.
 */

void
file_message (const std::string & tag, const std::string & path)
{
    bool isterminal = is_a_tty(STDOUT_FILENO);
    std::cout << seq_client_tag(msglevel::info, isterminal) << " "
        << tag << ": '" << path << "'" << std::endl;
}

/**
 *  Takes a format string and a variable-argument list and returns the
 *  formatted string.
 *
 *  Although currently a public function, its usage is meant to be internal
 *  for the msgprintf() function.  See that function's description.
 *
 *  C++11 is required, due to the use of va_copy().
 *
 * \param fmt
 *      Provides the printf() format string.
 *
 * \param args
 *      Provides the variable-argument list.
 *
 * \return
 *      Returns the formatted string.  If an error occurs, the string is
 *      empty.
 */

static std::string
formatted (const std::string & fmt, va_list args)
{
    std::string result;
    va_list args_copy;                                      /* Step 2       */
    va_copy(args_copy, args);

    const char * const szfmt = fmt.c_str();
    int ilen = std::vsnprintf(NULL, 0, szfmt, args_copy);
    va_end(args_copy);
    if (ilen > 0)
    {
        std::vector<char> dest(ilen + 1);                   /* Step 3       */
        std::vsnprintf(dest.data(), dest.size(), szfmt, args);
        result = std::string(dest.data(), dest.size() - 1);
    }
    va_end(args);
    return result;
}

/**
 *  Shows a boolean value as a "true/false" info message.
 *
 * \param tag
 *      Provides the text to precede the boolean value.
 *
 * \param flag
 *      Provides the boolean value to print.
 */

void
boolprint (const std::string & tag, bool flag)
{
    std::string fmt = tag + " %s";
    msgprintf(msglevel::info, fmt, flag ? "true" : "false");
}

/**
 *  Shows a boolean value as an "on/off" info message.
 *
 * \param tag
 *      Provides the text to precede the boolean value.
 *
 * \param flag
 *      Provides the boolean value to print.
 */

void
toggleprint (const std::string & tag, bool flag)
{
    std::string fmt = tag + " %s";
    msgprintf(msglevel::info, fmt, flag ? "on" : "off");
}

/**
 *  Meant for use in signal handlers.
 */

void
async_safe_strprint (const char * msg, size_t count)
{
#if defined SEQ66_PLATFORM_UNIX
    if (write(STDOUT_FILENO, msg, count) == 0)
    {
        // ignore -Wunused-result warning
    }
#else
#if defined SEQ66_PLATFORM_WINDOWS
    (void) _write(STDOUT_FILENO, msg, count);
#endif
#endif
}

/**
 *  Formats a variable list of input and returns the formatted string.
 *  Not the fastest function ever, but useful.  Adapted from the last sample
 *  in:
 *
 *      https://stackoverflow.com/questions/19009094/
 *              c-variable-arguments-with-stdstring-only
 *
 *  -#  Initialize the usage of a variable-argument array.
 *  -#  Acquire the destination size from a copy of the variable-argument
 *      array; mock the formatting with vsnprintf() and a null destination to
 *      find out how many characters will be constructed.  The copy is
 *      necessary to avoid messing up the original variable-argument array.
 *  -#  Return a formatted string without needing memory management, and
 *      without assuming any compiler/platform-specific behavior.
 *
 * \param lev
 *      Indicates the desired message level: info, warn, or error.
 *
 * \param fmt
 *      Indicates the desired format for the message.  Use "%s" for strings.
 *
 * \param ...
 *      Provides the printf() parameters for the format string.  Please note
 *      that C++ strings cannot be used directly... std::string::c_str() must
 *      be used.
 */

void
msgprintf (msglevel lev, std::string fmt, ...)
{
    if (rc().verbose() && ! fmt.empty())
    {
        /*
         * cppcheck: Using reference 'fmt' as parameter for va_start() results
         * in undefined behaviour.  Also claims we need to add a va_end(), so we
         * did, below, on 2019-04-21.
         */

        va_list args;                                       /* Step 1       */
        va_start(args, fmt);

        std::string output = formatted(fmt, args);          /* Steps 2 & 3  */
        bool errtty = is_a_tty(STDERR_FILENO);
        bool outtty = is_a_tty(STDOUT_FILENO);
        switch (lev)
        {
        case msglevel::none:

            std::cout << seq_client_tag(lev, outtty) << " "
                << output << std::endl;
            break;

        case msglevel::info:
        case msglevel::status:
        case msglevel::special:

            std::cout << seq_client_tag(lev, outtty) << " "
                << output << std::endl;
            break;

        case msglevel::warn:
        case msglevel::error:
        case msglevel::debug:

            std::cerr << seq_client_tag(lev, errtty) << " "
                << output << std::endl;
            break;
        }
        va_end(args);                                       /* 2019-04-21   */
    }
}

/**
 *  Acts like msgprintf(), but returns the result as a string, and doesn't
 *  bother with "info level" and whether we're debugging or not.
 *
 * \param fmt
 *      Indicates the desired format for the message.  Use "%s" for strings.
 *
 * \param ...
 *      Provides the printf() parameters for the format string.  Please note
 *      that C++ strings cannot be used directly... std::string::c_str() must
 *      be used.
 */

std::string
msgsnprintf (std::string fmt, ...)
{
    std::string result;
    if (! fmt.empty())
    {
        va_list args;
        va_start(args, fmt);
        result = formatted(fmt, args);
        va_end(args);
    }
    return result;
}

}           // namespace seq66

/*
 * basic_macros.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


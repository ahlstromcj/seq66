#if ! defined SEQ66_BASIC_MACROS_HPP
#define SEQ66_BASIC_MACROS_HPP

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
 * \file          basic_macros.hpp
 *
 *    This module provides macros for generating simple messages, MIDI
 *    parameters, and more.
 *
 * \library       seq66
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2018-11-10
 * \updates       2021-11-30
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *  This file defines macros for both C and C++11 (or greater) code.  Seq66
 *  requires C++11 support or above.  The macros in this file cover:
 *
 *       -  Compiler-support macros.
 *       -  Error and information output macros.
 *       -  One or more global debugging functions that are better suited
 *          than using a macro.
 *
 *  This module replaces Seq64's easy_macros.
 */

#include <cstdio>                       /* std::printf()                    */
#include <vector>                       /* std::vector                      */

#include "seq66_features.hpp"           /* C++ definitions, std::string     */
#include "util/basic_macros.h"          /* C-style definitions/features     */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 *  Common data types.
 */

/**
 *  Provides an easy-to-search container for strings.
 */

using tokenization = std::vector<std::string>;

/*
 * Global functions.  The not_nullptr_assert() function is a macro in
 * release mode, to speed up release mode.  It cannot do anything at
 * all, since it is used in the conditional part of if-statements.
 */

#if defined SEQ66_PLATFORM_DEBUG
#if defined __cplusplus
extern bool not_nullptr_assert (void * ptr, const std::string & context);
#else
#define not_nullptr_assert(ptr, context) (not_nullptr(ptr))
#endif  // C++
#else
#define not_nullptr_assert(ptr, context) (not_nullptr(ptr))
#endif

#if defined __cplusplus

/**
 *    Provides reporting macros (which happens to match Chris's XPC message
 *    functions.  For C code, these macros are defined in basic_macros.h
 *    instead, and are not as "powerful".
 */

#define errprint(x)         (void) seq66::error_message(x)
#define warnprint(x)        (void) seq66::warn_message(x)
#define infoprint(x)        (void) seq66::info_message(x)

/**
 *  Usage:      errprintf(format, cstring|value);
 *
 *    Provides an error reporting macro that requires a sprintf() format
 *    specifier as well.
 */

#define errprintf(fmt, x)   msgprintf(seq66::msglevel::error, fmt, x)
#define warnprintf(fmt, x)  msgprintf(seq66::msglevel::warn, fmt, x)
#define infoprintf(fmt, x)  msgprintf(seq66::msglevel::info, fmt, x)

#endif  // C++

/**
 *  Usage:      errprintfunc(cstring);
 *
 *    Provides error and informational reporting macro that includes the
 *    function name.
 */

#if defined __cplusplus
#define errprintfunc(x) seq66::msgprintf(seq66::msglevel::error, \
 "%s: %s", __func__, x)
#define infoprintfunc() seq66::msgprintf(seq66::msglevel::info, "%s", __func__)
#endif

extern bool info_message (const std::string & msg);
extern bool status_message (const std::string & msg);
extern bool warn_message (const std::string & msg);
extern bool error_message
(
    const std::string & msg, const std::string & data = ""
);
extern bool debug_message
(
    const std::string & msg, const std::string & data = ""
);
extern bool session_message (const std::string & msg);
extern void file_message (const std::string & tag, const std::string & path);
extern bool file_error (const std::string & tag, const std::string & filename);
extern void print_client_tag (msglevel el);
extern void boolprint (const std::string & tag, bool flag);
extern void toggleprint (const std::string & tag, bool flag);
extern void async_safe_strprint (const char * msg, size_t count);
extern void msgprintf (seq66::msglevel lev, std::string fmt, ...);
extern std::string msgsnprintf (std::string fmt, ...);

}               // namespace seq66

#endif          // SEQ66_BASIC_MACROS_HPP

/*
 * basic_macros.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


#if ! defined SEQ66_BASIC_MACROS_HPP
#define SEQ66_BASIC_MACROS_HPP

/**
 * \file          basic_macros.hpp
 *
 *    This module provides macros for generating simple messages, MIDI
 *    parameters, and more.
 *
 * \library       seq66
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2018-11-10
 * \updates       2021-05-16
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

#include <cstdio>
#include <string>

#include "seq66_features.hpp"           /* platform and config macros       */
#include "util/basic_macros.h"          /* C-compatible definitions         */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  An indicator of the message level.
 */

enum class msg_level
{
    info,
    warn,
    error
};

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

#define errprintf(fmt, x)   msgprintf(seq66::msg_level::error, fmt, x)
#define warnprintf(fmt, x)  msgprintf(seq66::msg_level::warn, fmt, x)
#define infoprintf(fmt, x)  msgprintf(seq66::msg_level::info, fmt, x)

#endif  // C++

/**
 *  Usage:      errprintfunc(cstring);
 *
 *    Provides an error reporting macro that includes the function name.
 */

#if defined __cplusplus
#define errprintfunc(x) seq66::msgprintf(seq66::msg_level::error, \
 "%s: %s", __func__, x)
#else
#define errprintfunc(x) fprintf(stderr, "%s: %s", __func__, x)
#endif

extern bool info_message (const std::string & msg);
extern bool warn_message (const std::string & msg);
extern bool error_message (const std::string & msg);
extern bool file_error (const std::string & tag, const std::string & filename);
extern void file_message (const std::string & tag, const std::string & path);
extern void boolprint (const std::string & tag, bool flag);
extern void toggleprint (const std::string & tag, bool flag);
extern void async_safe_strprint (const std::string & msg);
extern void msgprintf (seq66::msg_level lev, std::string fmt, ...);
extern std::string msgsnprintf (std::string fmt, ...);
extern bool is_debug ();

/**
 *  Prepends "Debug" to the string and then printf()'s it.  No need
 *  for an iostream for this simple task.
 *
 * \param msg
 *      The debug message to be shown.
 */

inline void
debug_message (const std::string & msg)
{
    if (! msg.empty())
        printf("Debug: '%s'\n", msg.c_str());
}

}               // namespace seq66

#endif          // SEQ66_BASIC_MACROS_HPP

/*
 * basic_macros.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


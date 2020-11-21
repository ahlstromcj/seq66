#if ! defined SEQ66_BASIC_MACROS_H
#define SEQ66_BASIC_MACROS_H

/**
 * \file          basic_macros.h
 *
 *    This module provides macros for generating simple messages, MIDI
 *    parameters, and more.
 *
 * \library       seq66
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2018-11-10
 * \updates       2020-11-21
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

#if ! defined __cplusplus
#include <stdio.h>
#endif

#include "seq66_features.h"             /* C platform and config macros     */

/**
 *  An undefined macro purely to emphasize a C/C++ "{}" scope used for various
 *  exception-safe locking mechanisms.
 */

#define SEQ66_SCOPE_LOCK

/**
 *  Language macros:
 *
 *    "nullptr" provides an alternative to NULL in C compilers.
 *    "override" is undefined (defined to nothing) in C compilers.
 */

#if ! defined __cplusplus

#define nullptr                 0
#define override
#define noexcept

#endif                                  /* defined __cplusplus  */

/**
 *    Provides a way to declare functions as having either a C++ or C
 *    interface.
 */

#if ! defined EXTERN_C_DEC

#if defined __cplusplus
#define EXTERN_C_DEC extern "C" {
#define EXTERN_C_END }
#else
#define EXTERN_C_DEC
#define EXTERN_C_END
#endif

#endif

/**
 *  Reserved for future usage of gettext().
 */

#if ! defined GETTEXT_UNDERSCORE
#define GETTEXT_UNDERSCORE
#define T_(msg)                 (msg)
#define N_(msg)                 (msg)
#endif

/**
 *    Test for being a valid pointer.  The not_NULL() macro is meant for C
 *    code that returns NULL.
 */

#define not_NULL(x)             ((x) != NULL)
#define not_nullptr(x)          ((x) != nullptr)
#define not_nullptr_2(x1, x2)   ((x1) != nullptr && (x2) != nullptr)

/**
 *    Test for being an invalid pointer.  The is_NULL() macro is meant for C
 *    code that returns NULL.
 */

#define is_NULL(x)              ((x) == NULL)
#define is_nullptr(x)           ((x) == nullptr)
#define is_nullptr_2(x1, x2)    ((x1) == nullptr || (x2) == nullptr)

#if ! defined __cplusplus

/**
 *  Provides the "false" value of the wbool_t type definition.
 */

#define false    0

/**
 *  Provides the "true" value of the wbool_t type definition.
 */

#define true     1

#endif

/**
 *    GCC provides three magic variables which hold the name of the current
 *    function, as a string.  The first of these is `__func__', which is part
 *    of the C99 standard:
 *
 *    The identifier `__func__' is implicitly declared by the translator.  It
 *    is the name of the lexically-enclosing function.  This name is the
 *    unadorned name of the function.
 *
 *    `__FUNCTION__' is another name for `__func__'.  Older versions of GCC
 *    recognize only this name.  However, it is not standardized.  For maximum
 *    portability, use `__func__', but provide a fallback definition with
 *    the preprocessor, as done below.
 *
 *    `__PRETTY_FUNCTION__' is the decorated version of the function name.
 *    It is longer, but more informative.  It is also deprecated.  But, for
 *    now, we'll continue to use it
 *
 *    Visual Studio defines only __FUNCTION__, so a definition is provided
 *    below.
 */

#if defined SEQ66_PLATFORM_GNU

#if ! defined __func__
#if __STDC_VERSION__ < 199901L
#if __GNUC__ >= 2

/**
 *  Function names.
 */

#define __func__            __FUNCTION__                 // bald func names

#else
#define __func__            "<unknown>"
#endif  // __GNUC__
#endif  // __STDC_VERSION__
#endif  // __func__

#else   // ! SEQ66_PLATFORM_GNU

#if ! defined __func__
#define __func__        __FUNCTION__    // Windows?
#endif

#endif  // SEQ66_PLATFORM_GNU

/**
 *  Usage:      apiprint(function_name, context_tag);
 *
 *  This macro can be enabled in JACK modules in order to see the flow of
 *  calls to the JACK or ALSA API.  It also disables the hiding of JACK/ALSA
 *  information messages.
 */

#if defined SEQ66_SHOW_API_CALLS
#if defined __cplusplus
#define apiprint(name, tag)    fprintf(stderr, "= %s(%s)\n", name ## .c_str(), \
 tag ## .c_str())
#else
#define apiprint(name, tag)    fprintf(stderr, "= %s(%s)\n", name, tag)
#endif
#else
#define apiprint(name, tag)
#endif

#if ! defined __cplusplus

/**
 *    Provides reporting macros (which happens to match Chris's XPC message
 *    functions.  For C++ code, these macros are defined in basic_macros.hpp
 *    and are more convenient to use.
 */

#if defined SEQ66_PLATFORM_DEBUG

#define errprint(x)             fprintf(stderr, "%s\n", x)
#define errprintf(fmt, x)       fprintf(stderr, fmt, x)
#define errprintfunc(x)         fprintf(stderr, "%s: %s\n", __func__, x)

#define warnprint(x)            fprintf(stderr, "%s\n", x)
#define warnprintf(fmt, x)      fprintf(stderr, fmt, x)

#define infoprint(x)            fprintf(stderr, "%s\n", x)
#define infoprintf(fmt, x)      fprintf(stderr, fmt, x)

#else

#define errprint(x)
#define errprintf(fmt, x)
#define errprintfunc(x)

#define warnprint(x)
#define warnprintf(fmt, x)

#define infoprint(x)
#define infoprintf(fmt, x)

#endif          // SEQ66_PLATFORM_DEBUG

#endif          // ! C++

#endif          // SEQ66_BASIC_MACROS_H

/*
 * basic_macros.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */


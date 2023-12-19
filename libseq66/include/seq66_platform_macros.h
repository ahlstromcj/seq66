#if ! defined SEQ66_PLATFORM_MACROS_H
#define SEQ66_PLATFORM_MACROS_H

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
 * \file          seq66_platform_macros.h
 *
 *  Provides a rationale and a set of macros to make compile-time
 *  decisions covering Windows versus Linux, GNU versus Microsoft, and
 *  MINGW versus GNU.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2023-12-18
 * \license       GNU GPLv2 or above
 *
 *  Copyright (C) 2013-2023 Chris Ahlstrom <ahlstromcj@gmail.com>
 *
 *  We need a uniform way to specify OS and compiler features without
 *  littering the code with macros from disparate compilers.  Put all
 *  the compiler-specific stuff here to define "PLATFORM" macros.
 *
 * Determining useful macros:
 *
 *    -  GNU:  cpp -dM myheaderfile
 *
 * Settings to distinguish, based on compiler-supplied macros:
 *
 *    -  Platform macros (set in nar-maven-plugin's aol.properties):
 *       -  Windows
 *       -  Linux
 *       -  MacOSX
 *    -  Platform macros (in the absense of Windows, Linux macros):
 *       -  SEQ66_PLATFORM_WINDOWS
 *       -  SEQ66_PLATFORM_LINUX
 *       -  SEQ66_PLATFORM_FREEBSD
 *       -  SEQ66_PLATFORM_MACOSX
 *       -  SEQ66_PLATFORM_UNIX
 *    -  Architecture size macros:
 *       -  SEQ66_PLATFORM_32_BIT
 *       -  SEQ66_PLATFORM_64_BIT
 *    -  Debugging macros:
 *       -  SEQ66_PLATFORM_DEBUG
 *       -  SEQ66_PLATFORM_RELEASE
 *    -  Compiler:
 *       -  SEQ66_PLATFORM_MSVC (alternative to _MSC_VER)
 *       -  SEQ66_PLATFORM_GNU
 *       -  SEQ66_PLATFORM_XSI
 *       -  SEQ66_PLATFORM_MINGW
 *       -  SEQ66_PLATFORM_MING_OR_WINDOWS
 *       -  SEQ66_PLATFORM_CYGWIN
 *    -  API:
 *       -  SEQ66_PLATFORM_POSIX_API (alternative to POSIX)
 *    -  Language:
 *       -  SEQ66_PLATFORM_CPP_11
 *       -  SEQ66_PLATFORM_CPP_14
 *       -  SEQ66_PLATFORM_CPP_17
 *
 *  http://stackoverflow.com/questions/11053960/
 *      how-are-the-cplusplus-directive-defined-in-various-compilers
 *
 *    The 199711L stands for Year=1997, Month = 11 (i.e., November of 1997) --
 *    the date when the committee approved the standard that the rest of the
 *    ISO approved in early 1998.
 *
 *    For the 2003 standard, there were few enough changes that the committee
 *    (apparently) decided to leave that value unchanged.
 *
 *    For the 2011 standard, it's defined as 201103L, (year=2011, month = 03),
 *    meaning that the committee approved the standard as finalized in March
 *    of 2011.
 *
 *    For the 2014 standard, it's defined as 201402L, interpreted the same way
 *    as above (February 2014).
 *
 *    For the 2017 standard, it's defined as 201703L, interpreted the same way
 *    as above (March 2017).
 *
 *    Before the original standard was approved, quite a few compilers
 *    normally defined it as 0 (or just an empty definition like #define
 *    __cplusplus) to signify "not-conforming". When asked for their strictest
 *    conformance, many defined it to 1.  Ancient news!
 */

#undef SEQ66_MING_OR_WINDOWS
#undef SEQ66_PLATFORM_WINDOWS
#undef SEQ66_PLATFORM_LINUX
#undef SEQ66_PLATFORM_FREEBSD
#undef SEQ66_PLATFORM_MACOSX
#undef SEQ66_PLATFORM_UNIX
#undef SEQ66_PLATFORM_32_BIT
#undef SEQ66_PLATFORM_64_BIT
#undef SEQ66_PLATFORM_DEBUG
#undef SEQ66_PLATFORM_RELEASE
#undef SEQ66_PLATFORM_MSVC
#undef SEQ66_PLATFORM_GNU
#undef SEQ66_PLATFORM_XSI
#undef SEQ66_PLATFORM_MINGW
#undef SEQ66_PLATFORM_CYGWIN
#undef SEQ66_PLATFORM_POSIX_API
#undef SEQ66_PLATFORM_CPP_11
#undef SEQ66_PLATFORM_CPP_14
#undef SEQ66_PLATFORM_CPP_17

/**
 *  Provides a "Windows" macro, in case the environment doesn't provide
 *  it.  This macro is defined if not already defined and _WIN32 or WIN32
 *  are encountered.
 */

#if defined Windows                     /* defined by nar-maven-plugin      */
#define SEQ66_PLATFORM_WINDOWS
#else
#if defined _WIN32 || defined _WIN64    /* defined by Microsoft compiler    */
#define SEQ66_PLATFORM_WINDOWS
#define Windows
#else
#if defined WIN32 || defined WIN64      /* defined by Mingw compiler        */
#define SEQ66_PLATFORM_WINDOWS
#define Windows
#endif
#endif
#endif

/**
 *  FreeBSD macros.
 */

#if defined __FreeBSD__
#define SEQ66_PLATFORM_FREEBSD
#define SEQ66_PLATFORM_UNIX
#endif

/**
 *  Provides a "Linux" macro, in case the environment doesn't provide it.
 *  This macro is defined if not already defined.
 */

#if defined Linux                      /* defined by nar-maven-plugin       */
#define SEQ66_PLATFORM_LINUX
#else
#if defined __linux__                  /* defined by the GNU compiler       */
#define Linux
#define SEQ66_PLATFORM_LINUX
#endif
#endif

#if defined SEQ66_PLATFORM_LINUX
#define SEQ66_PLATFORM_UNIX
#endif                                 /* SEQ66_PLATFORM_LINUX              */

/**
 *  Provides a "MacOSX" macro, in case the environment doesn't provide it.
 *  This macro is defined if not already defined and __APPLE__ and
 *  __MACH__ are encountered.
 */

#if defined MacOSX                     /* defined by the nar-maven-plugin   */
#define SEQ66_PLATFORM_MACOSX
#else
#if defined __APPLE__ && defined __MACH__    /* defined by Apple compiler   */
#define SEQ66_PLATFORM_MACOSX
#define MacOSX
#endif
#endif

#if defined SEQ66_PLATFORM_MACOSX
#define SEQ66_PLATFORM_UNIX
#endif

#if defined SEQ66_PLATFORM_UNIX
#define SEQ66_PLATFORM_POSIX_API
#define SEQ66_PLATFORM_PTHREADS
#if ! defined POSIX
#define POSIX                          /* defined for legacy code purposes  */
#endif
#endif

/**
 *  Provides macros that mean 32-bit, and only 32-bit Windows.  For
 *  example, in Windows, _WIN32 is defined for both 32- and 64-bit
 *  systems, because Microsoft didn't want to break people's 32-bit code.
 *  So we need a specific macro.
 *
 *      -  SEQ66_PLATFORM_32_BIT is defined on all platforms.
 *      -  WIN32 is defined on Windows platforms.
 *
 *  Prefer the former macro.  The second is defined only for legacy
 *  purposes for Windows builds, and might eventually disappear.
 */

#if defined SEQ66_PLATFORM_WINDWS
#if defined _WIN32 && ! defined _WIN64

#if ! defined WIN32
#define WIN32                          /* defined for legacy purposes        */
#endif

#if ! defined SEQ66_PLATFORM_32_BIT
#define SEQ66_PLATFORM_32_BIT
#endif

#endif
#endif                                 /* SEQ66_PLATFORM_WINDOWS            */

/**
 *  Provides macros that mean 64-bit, and only 64-bit.
 *
 *      -  SEQ66_PLATFORM_64_BIT is defined on all platforms.
 *      -  WIN64 is defined on Windows platforms.
 *
 *  Prefer the former macro.  The second is defined only for legacy
 *  purposes for Windows builds, and might eventually disappear.
 *
 */

#if defined SEQ66_PLATFORM_WINDWS
#if defined _WIN64

#if ! defined WIN64
#define WIN64
#endif

#if ! defined SEQ66_PLATFORM_64_BIT
#define SEQ66_PLATFORM_64_BIT
#endif

#endif
#endif                                 /* SEQ66_PLATFORM_WINDOWS            */

/**
 *  Provides macros that mean 64-bit versus 32-bit when gcc or g++ are
 *  used. This can occur on Linux and other systems, and with mingw on
 *  Windows.
 *
 *      -  SEQ66_PLATFORM_64_BIT is defined on all platforms.
 *
 *  Prefer the former macro.  The second is defined only for legacy
 *  purposes for Windows builds, and might eventually disappear.
 */

#if defined __GNUC__
#if defined __x86_64__ || __ppc64__

#if ! defined SEQ66_PLATFORM_64_BIT
#define SEQ66_PLATFORM_64_BIT
#endif

#else

#if ! defined SEQ66_PLATFORM_32_BIT
#define SEQ66_PLATFORM_32_BIT
#endif

#endif
#endif

/**
 *  Provides macros that indicate if Microsoft C/C++ versus GNU are being
 *  used.  THe compiler being used normally provides test macros for itself.
 *
 *      -  SEQ66_PLATFORM_MSVC (replaces _MSC_VER)
 *      -  SEQ66_PLATFORM_GNU (replaces __GNUC__)
 *      -  SEQ66_PLATFORM_MINGW (replaces __MINGW32__)
 *      -  SEQ66_PLATFORM_CYGWIN
 */

#if defined _MSC_VER
#define SEQ66_PLATFORM_MSVC
#define SEQ66_PLATFORM_WINDOWS
#endif

#if defined __GNUC__
#define SEQ66_PLATFORM_GNU
#endif

#if (_POSIX_C_SOURCE >= 200112L) && ! _GNU_SOURCE
#define SEQ66_PLATFORM_XSI

/*
 * Hit this one compiling in Qt Creator on Linux.
 *
 * #error XSI defined, this is just a test
 */

#endif

#if defined __MINGW32__ || defined __MINGW64__
#define SEQ66_PLATFORM_MINGW
#define SEQ66_PLATFORM_WINDOWS
#endif

#if defined SEQ66_PLATFORM_WINDOWS

/*
 *  Without this #define, the InitializeCriticalSectionAndSpinCount() function
 *  is undefined.  This version level means "Windows 2000 and higher".
 *  For Windows 10, the value would be 0x0A00.
 */

#if ! defined _WIN32_WINNT
#define _WIN32_WINNT        0x0500
#endif

#endif  // defined SEQ66_PLATFORM_WINDOWS

#if defined __CYGWIN32__
#define SEQ66_PLATFORM_CYGWIN
#endif

/**
 *  Provides a way to flag unused parameters at each "usage", without disabling
 *  them globally.  Use it like this:
 *
 *     void foo(int UNUSED(bar)) { ... }
 *     static void UNUSED_FUNCTION(foo)(int bar) { ... }
 *
 *  The UNUSED macro won't work for arguments which contain parenthesis,
 *  so an argument like float (*coords)[3] one cannot do,
 *
 *      float UNUSED((*coords)[3]) or float (*UNUSED(coords))[3].
 *
 *  This is the only downside to the UNUSED macro; in these cases fall back to
 *
 *      (void) coords;
 *
 *  Another possible definition is casting the unused value to void in the
 *  function body.
 */

#if defined __GNUC__
#define UNUSED(x)               UNUSED_ ## x __attribute__((__unused__))
#else
#define UNUSED(x)               UNUSED_ ## x
#endif

#if defined __GNUC__
#define UNUSED_FUNCTION(x)      __attribute__((__unused__)) UNUSED_ ## x
#else
#define UNUSED_FUNCTION(x)      UNUSED_ ## x
#endif

#define UNUSED_VOID(x)          (void) (x)

/**
 *  Provides macros to indicate the level standards support for some key
 *  cases.  We may have to play with this a bit to get it right.  The main
 *  use-case right now is in avoiding defining the nullptr macro in C++11.
 *
 *      -  SEQ66_PLATFORM_CPP_11
 */

#if defined SEQ66_PLATFORM_MSVC

#if _MSC_VER >= 1700                /* __cplusplus value doesn't work, MS!  */
#define SEQ66_PLATFORM_CPP_11
#endif

#else

#if __cplusplus >= 201103L          /* i.e. C++11                           */
#define SEQ66_PLATFORM_CPP_11
#endif

#if __cplusplus >= 201402L          /* i.e. C++14                           */
#define SEQ66_PLATFORM_CPP_14
#endif

#if __cplusplus >= 201703L          /* i.e. C++17                           */
#define SEQ66_PLATFORM_CPP_17
#endif

#endif

/**
 *  Kind of a Windows-with-MingW-matching-Visual-Studio macro.
 */

#if defined SEQ66_PLATFORM_MSVC || defined SEQ66_PLATFORM_MINGW
#define SEQ66_PLATFORM_MING_OR_WINDOWS
#endif

/**
 *  A UNIX or MingW macro.
 */

#if defined SEQ66_PLATFORM_UNIX || defined SEQ66_PLATFORM_MINGW
#define SEQ66_PLATFORM_MING_OR_UNIX
#endif

/**
 *  Provides macros that mean "debugging enabled".
 *
 *      -  SEQ66_PLATFORM_DEBUG or SEQ66_PLATFORM_RELEASE
 *      -  DEBUG or NDEBUG for legacy usage
 *
 * Prefer the former macro.  The second is defined only for legacy
 * purposes for Windows builds, and might eventually disappear.
 */

#if ! defined SEQ66_PLATFORM_DEBUG
#if defined DEBUG || _DEBUG || _DEBUG_ || __DEBUG || __DEBUG__
#define SEQ66_PLATFORM_DEBUG
#endif
#endif

#if ! defined SEQ66_PLATFORM_DEBUG && ! defined SEQ66_PLATFORM_RELEASE
#define SEQ66_PLATFORM_RELEASE
#endif

/**
 *  Provides a check for error return codes from applications.  It is a
 *  non-error value for most POSIX-conformant functions.  This macro defines
 *  the integer value returned by many POSIX functions when they succeed --
 *  zero (0).
 *
 * \note
 *      Rather than testing this value directory, the macro functions
 *      is_posix_success() and not_posix_success() should be used.  See the
 *      descriptions of those macros for more information.
 */

#if ! defined SEQ66_PLATFORM_POSIX_SUCCESS
#define SEQ66_PLATFORM_POSIX_SUCCESS              0
#endif

/**
 *  SEQ66_PLATFORM_POSIX_ERROR is returned from a string function when it has
 *  processed an error.  It indicates that an error is in force.  Normally,
 *  the caller then uses this indicator to set a class-based error message.
 *  This macro defines the integer value returned by many POSIX functions when
 *  they fail -- minus one (-1).  The EXIT_FAILURE and
 *  SEQ66_PLATFORM_POSIX_ERROR macros also have the same value.
 *
 * \note
 *      Rather than testing this value directory, the macro functions
 *      is_posix_error() and not_posix_error() should be used.  See the
 *      descriptions of those macros for more information.
 */

#if ! defined SEQ66_PLATFORM_POSIX_ERROR
#define SEQ66_PLATFORM_POSIX_ERROR              (-1)
#endif

/**
 *    This macro tests the integer value against SEQ66_PLATFORM_POSIX_SUCCESS.
 *    Other related macros are:
 *
 *       -  is_posix_success()
 *       -  is_posix_error()
 *       -  not_posix_success()
 *       -  not_posix_error()
 *       -  set_posix_success()
 *       -  set_posix_error()
 *
 * \note
 *      -   Some functions return values other than SEQ66_PLATFORM_POSIX_ERROR
 *          when an error occurs.
 *      -   Some functions return values other than
 *          SEQ66_PLATFORM_POSIX_SUCCESS when the function succeeds.
 *      -   Please refer to the online documentation for these quixotic
 *          functions, and decide which macro one want to use for the test, if
 *          any.
 *      -   In some case, one might want to use a clearer test.  For example,
 *          the socket functions return a result that is
 *          SEQ66_PLATFORM_POSIX_ERROR (-1) if the function fails, but
 *          non-zero integer values are returned if the function succeeds.
 *          For these functions, the is_valid_socket() and not_valid_socket()
 *          macros are much more appropriate to use.
 *
 *//*-------------------------------------------------------------------------*/

#if ! defined is_posix_success
#define is_posix_success(x)      ((x) == SEQ66_PLATFORM_POSIX_SUCCESS)
#endif

/**
 *  This macro tests the integer value against SEQ66_PLATFORM_POSIX_ERROR (-1).
 */

#if ! defined is_posix_error
#define is_posix_error(x)        ((x) == SEQ66_PLATFORM_POSIX_ERROR)
#endif

/**
 *  This macro tests the integer value against SEQ66_PLATFORM_POSIX_SUCCESS (0).
 */

#if ! defined not_posix_success
#define not_posix_success(x)     ((x) != SEQ66_PLATFORM_POSIX_SUCCESS)
#endif

/**
 *  This macro tests the integer value against SEQ66_PLATFORM_POSIX_ERROR (-1).
 */

#if ! defined not_posix_error
#define not_posix_error(x)       ((x) != SEQ66_PLATFORM_POSIX_ERROR)
#endif

/**
 *  This macro set the integer value to SEQ66_PLATFORM_POSIX_SUCCESS (0).  The
 *  parameter must be an lvalue, as the assignment operator is used.
 */

#if ! defined set_posix_success
#define set_posix_success(x)     ((x) = SEQ66_PLATFORM_POSIX_SUCCESS)
#endif

/**
 *  This macro set the integer value to SEQ66_PLATFORM_POSIX_ERROR (-1).  The
 *  parameter must be an lvalue, as the assignment operator is used.
 */

#if ! defined set_posix_error
#define set_posix_error(x)       ((x) = SEQ66_PLATFORM_POSIX_ERROR)
#endif

#endif                  /* SEQ66_PLATFORM_MACROS_H */

/*
 * seq66_platform_macros.h
 *
 * vim: ts=4 sw=4 wm=4 et ft=c
 */


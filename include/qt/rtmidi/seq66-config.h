#if defined SEQ66_RTMIDI_SUPPORT

#if ! defined SEQ66_QT_MIDILIB_CONFIG
#define SEQ66_QT_MIDILIB_CONFIG

#undef SEQ66_PORTMIDI_SUPPORT

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
 * \file          seq66-config.h for Qt/RtMidi
 *
 *  This module provides platform/build-specific configuration that is not
 *  modifiable via a "configure" operation.  It is meant for those who do not
 *  want to use automake to build the Linux/Qt version of seq66.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2020-05-31
 * \updates       2021-12-11
 * \license       GNU GPLv2 or above
 *
 *  Qt Rtmidi Linux and Windows versions, hardwired for use with
 *  qtcreator/qmake instead of using GNU autotools.
 *
 *  One motivation for creating a Qt build for this version of seq66 is that,
 *  one a Debian Sid/Unstable laptop with gcc/g++ version 9 as the compiler,
 *  we get this error when building for debug more (but not for release mode):
 *
 *      /usr/bin/ld:
 *      /home/.../seq66/seq_qt5/src/.libs/libseq_qt5.a(qloopbutton.o):
 *      undefined reference to symbol
 *      '_ZN8QPainter8drawTextERK6QRectFiRK7QStringPS0_@@Qt_5'
 *      /usr/bin/ld:
 *      /usr/lib/x86_64-linux-gnu/libQt5Gui.so: error adding symbols: DSO
 *          missing from command line
 *
 *  Why is that "Qt_5" namespace tacked onto the end of that symbol?
 *
 *  Note:  This header file is NOT auto-generated for the rtmidi build.
 *  Therefore, the date and version information below must be edited by hand
 *  when needed.
 *
 *      SEQ66_VERSION_DATE_SHORT
 *      SEQ66_PACKAGE_STRING
 *      SEQ66_PACKAGE_VERSION
 *      SEQ66_VERSION
 *
 *  Updates to configure.ac might need to be incorporated into this file!
 */

#if defined _INCLUDE_SEQ___CONFIG_H
#error Automake-generated include file seq66-config.h already included.
#endif

#if ! defined SEQ66_VERSION_DATE_SHORT
#define SEQ66_VERSION_DATE_SHORT "2021-12-11"
#endif

#if ! defined SEQ66_VERSION
#define SEQ66_VERSION "0.98.0"
#endif

#if ! defined SEQ66_GIT_VERSION
#define SEQ66_GIT_VERSION SEQ66_VERSION
#endif

#if ! defined SEQ66_PACKAGE_VERSION
#define SEQ66_PACKAGE_VERSION SEQ66_VERSION
#endif

/**
 *  This macro helps us adapt our "ui" includes to freaking qmake's
 *  conventions.  We used "userinterface.ui.h", while qmake is stuck on
 *  "ui_user_interface.h".
 *
 *  It's almost enough to make you use Cmake.  :-D
 */

#if ! defined SEQ66_QMAKE_RULES
#define SEQ66_QMAKE_RULES
#endif

/* "Distro where build was done" */

#if ! defined SEQ66_APP_BUILD_OS
#define SEQ66_APP_BUILD_OS "'qmake'"
#endif

#if ! defined SEQ66_APP_BUILD_ISSUE
#define SEQ66_APP_BUILD_ISSUE "'Linux'"
#endif

/**
 * Names this version of application, plus the engine in use, and the type of
 * application.  Useful in Help / Build Info.
 * "qr" means "Qmake/Qt PortMidi-based".
 */

#if ! defined SEQ66_APP_ENGINE
#define SEQ66_APP_ENGINE "rtmidi"
#endif

#if ! defined SEQ66_APP_NAME
#define SEQ66_APP_NAME "qrseq66"
#endif

#if ! defined SEQ66_APP_TYPE
#define SEQ66_APP_TYPE "qt5"
#endif

#undef SEQ66_APP_CLI

/*
 * "The name to display as client/port".  This name is the same no matter what
 * is the name of the executable.  This is also the name of the default
 * configuration directory in the "home" area of the user.
 */

#if ! defined SEQ66_CLIENT_NAME
#define SEQ66_CLIENT_NAME "seq66"
#endif

/*
 * Define if LIBLO library is available.  If you get an error, either undefine
 * this value or install the liblo-dev package.
 */

#if ! defined SEQ66_LIBLO_SUPPORT
#define SEQ66_LIBLO_SUPPORT 1
#endif

/*
 * Names the configuration file for this version of application. The "q"
 * stands for Qt, and the "p" stands for "portmidi".
 */

#if ! defined SEQ66_CONFIG_NAME
#define SEQ66_CONFIG_NAME "qrseq66"
#endif

/*
 * Define COVFLAGS=-fprofile-arcs -ftest-coverage if coverage support is
 * wanted.
 */

#undef SEQ66_COVFLAGS

/*
 * Define DBGFLAGS=-g -O0 -DDEBUG -fno-inline if debug support is wanted.
 */

#if ! defined SEQ66_DBGFLAGS
#define SEQ66_DBGFLAGS -O0 -DDEBUG -D_DEBUG -fno-inline
#endif

/* Define to 1 if you have the <ctype.h> header file. */
#if ! defined SEQ66_HAVE_CTYPE_H
#define SEQ66_HAVE_CTYPE_H 1
#endif

/* Define to 1 if you have the <dlfcn.h> header file. */
#if ! defined SEQ66_HAVE_DLFCN_H
#define SEQ66_HAVE_DLFCN_H 1
#endif

/* Define to 1 if you have the <errno.h> header file. */
#if ! defined SEQ66_HAVE_ERRNO_H
#define SEQ66_HAVE_ERRNO_H 1
#endif

/* Define to 1 if you have the <fcntl.h> header file. */
#if ! defined SEQ66_HAVE_FCNTL_H
#define SEQ66_HAVE_FCNTL_H 1
#endif

/* Define to 1 if you have the <getopt.h> header file. */
#if ! defined SEQ66_HAVE_GETOPT_H
#define SEQ66_HAVE_GETOPT_H 1
#endif

/* Define to 1 if you have the <inttypes.h> header file. */
#if ! defined SEQ66_HAVE_INTTYPES_H
#define SEQ66_HAVE_INTTYPES_H 1
#endif

/* Define to 1 if you have the `asound' library (-lasound). */
#if ! defined SEQ66_HAVE_LIBASOUND
#define SEQ66_HAVE_LIBASOUND 1
#endif

/* Define to 1 if you have the <limits.h> header file. */
#if ! defined SEQ66_HAVE_LIMITS_H
#define SEQ66_HAVE_LIMITS_H 1
#endif

/* Define to 1 if you have the <memory.h> header file. */
#if ! defined SEQ66_HAVE_MEMORY_H
#define SEQ66_HAVE_MEMORY_H 1
#endif

/* Define if you have POSIX threads libraries and header files. */
#if ! defined SEQ66_HAVE_PTHREAD
#define SEQ66_HAVE_PTHREAD 1
#endif

/* Have PTHREAD_PRIO_INHERIT. */
#if ! defined SEQ66_HAVE_PTHREAD_PRIO_INHERIT
#define SEQ66_HAVE_PTHREAD_PRIO_INHERIT 1
#endif

/* Define to 1 if you have the <stdarg.h> header file. */
#if ! defined SEQ66_HAVE_STDARG_H
#define SEQ66_HAVE_STDARG_H 1
#endif

/* Define to 1 if you have the <stddef.h> header file. */
#if ! defined SEQ66_HAVE_STDDEF_H
#define SEQ66_HAVE_STDDEF_H 1
#endif

/* Define to 1 if you have the <stdint.h> header file. */
#if ! defined SEQ66_HAVE_STDINT_H
#define SEQ66_HAVE_STDINT_H 1
#endif

/* Define to 1 if you have the <stdio.h> header file. */
#if ! defined SEQ66_HAVE_STDIO_H
#define SEQ66_HAVE_STDIO_H 1
#endif

/* Define to 1 if you have the <stdlib.h> header file. */
#if ! defined SEQ66_HAVE_STDLIB_H
#define SEQ66_HAVE_STDLIB_H 1
#endif

/* Define to 1 if you have the <strings.h> header file. */
#if ! defined SEQ66_HAVE_STRINGS_H
#define SEQ66_HAVE_STRINGS_H 1
#endif

/* Define to 1 if you have the <string.h> header file. */
#if ! defined SEQ66_HAVE_STRING_H
#define SEQ66_HAVE_STRING_H 1
#endif

/* Define to 1 if you have the <syslog.h> header file. */
#if ! defined SEQ66_HAVE_SYSLOG_H
#define SEQ66_HAVE_SYSLOG_H 1
#endif

/* Define to 1 if you have the <sys/stat.h> header file. */
#if ! defined SEQ66_HAVE_SYS_STAT_H
#define SEQ66_HAVE_SYS_STAT_H 1
#endif

/* Define to 1 if you have the <sys/sysctl.h> header file. */
#if ! defined SEQ66_HAVE_SYS_SYSCTL_H
#define SEQ66_HAVE_SYS_SYSCTL_H 1
#endif

/* Define to 1 if you have the <sys/time.h> header file. */
#if ! defined SEQ66_HAVE_SYS_TIME_H
#define SEQ66_HAVE_SYS_TIME_H 1
#endif

/* Define to 1 if you have the <sys/types.h> header file. */
#if ! defined SEQ66_HAVE_SYS_TYPES_H
#define SEQ66_HAVE_SYS_TYPES_H 1
#endif

/* Define to 1 if you have the <time.h> header file. */
#if ! defined SEQ66_HAVE_TIME_H
#define SEQ66_HAVE_TIME_H 1
#endif

/* Define to 1 if you have the <unistd.h> header file. */
#if ! defined SEQ66_HAVE_UNISTD_H
#define SEQ66_HAVE_UNISTD_H 1
#endif

/*
 * Define to enable JACK session.  It is deprecated by the JACK developers,
 * who now recommend using the Non Session Manager (NSM).  However, we
 * still want to enable it in Linux (rtmidi) debug builds.
 */

#if ! defined SEQ66_JACK_SESSION
#define SEQ66_JACK_SESSION 1
#endif

/*
 * Define to enable JACK driver.
 */

#if ! defined SEQ66_JACK_SUPPORT
#define SEQ66_JACK_SUPPORT 1
#endif

/*
 * Define if NSM support is available.
 */

#if ! defined SEQ66_NSM_SUPPORT
#define SEQ66_NSM_SUPPORT 1
#endif

/*
 * Define to the sub-directory where libtool stores uninstalled libraries.
 * Useless for qmake, but keep it for now.
 */

#if ! defined SEQ66_LT_OBJDIR
#define SEQ66_LT_OBJDIR ".libs/"
#endif

#undef SEQ66_MULTI_MAINWID

/* Name of package */
#if ! defined SEQ66_PACKAGE
#define SEQ66_PACKAGE "seq66"
#endif

/* Define to the address where bug reports for this package should be sent. */
#if ! defined SEQ66_PACKAGE_BUGREPORT
#define SEQ66_PACKAGE_BUGREPORT "ahlstromcj@gmail.com"
#endif

/* Define to the full name of this package. */
#if ! defined SEQ66_PACKAGE_NAME
#define SEQ66_PACKAGE_NAME "Seq66"
#endif

/* Define to the full name and version of this package. */
#if ! defined SEQ66_PACKAGE_STRING
#define SEQ66_PACKAGE_STRING "Seq66 0.98.0"
#endif

/* Define to the one symbol short name of this package. */
#if ! defined SEQ66_PACKAGE_TARNAME
#define SEQ66_PACKAGE_TARNAME "seq66"
#endif

/* Define to the home page for this package. */
#if ! defined SEQ66_PACKAGE_URL
#define SEQ66_PACKAGE_URL ""
#endif

/*
 * Define PROFLAGS=-pg (gprof) or -p (prof) if profile support is wanted.
 */

#if ! defined SEQ66_PROFLAGS
#define SEQ66_PROFLAGS
#endif

/*
 * Indicates that Qt5 is enabled.  Currently not yet in configure.ac nor used in
 * any module, but....
 */

#if ! defined SEQ66_QTMIDI_SUPPORT
#define SEQ66_QTMIDI_SUPPORT 1
#endif

/*
 * Define to 1 if you have the ANSI C header files.
 */

#if ! defined SEQ66_STDC_HEADERS
#define SEQ66_STDC_HEADERS 1
#endif

#if ! defined SEQ66__GNU_SOURCE
#define SEQ66__GNU_SOURCE 1
#endif

#endif  // SEQ66_QT_MIDILIB_CONFIG
#endif  // SEQ66_RTMIDI_SUPPORT

/*
 * seq66-config.h for Qt/RtMidi
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */


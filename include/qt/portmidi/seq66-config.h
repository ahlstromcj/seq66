#ifdef SEQ66_PORTMIDI_SUPPORT

#ifndef SEQ66_QT_MIDILIB_CONFIG
#define SEQ66_QT_MIDILIB_CONFIG

#undef SEQ66_RTMIDI_SUPPORT

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
 * \file          seq66-config.h for Qt/PortMidi
 *
 *  This module provides platform/build-specific configuration that is not
 *  modifiable via a "configure" operation.  It is meant for the hardwired
 *  qmake build of the PortMidi Linux and Windows versions.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-10
 * \updates       2020-08-15
 * \license       GNU GPLv2 or above
 *
 *  Qt Portmidi Linux and Windows versions, hardwired for use with
 *  qtcreator/qmake.  That build uses PortMidi in order to support both Linux
 *  and Windows. Hence no support for JACK or LASH, for example.
 *  However, it still defines some things that are available on GNU/Linux/MingW
 *  systems.
 *
 *  Note that there is a native (i.e. automake) Linux Qt build that uses
 *  RtMidi, so that JACK is supported.  However, LASH support is not being
 *  ported to seq66.
 *
 *  Note:  This header file is NOT auto-generated for the portmidi build.
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

#ifdef _INCLUDE_SEQ___CONFIG_H
#error Automake-generated include file seq66-config.h already included.
#endif

#ifndef SEQ66_VERSION_DATE_SHORT
#define SEQ66_VERSION_DATE_SHORT "2020-08-15"
#endif

#ifndef SEQ66_VERSION
#define SEQ66_VERSION "0.90.5"
#endif

#ifndef SEQ66_GIT_VERSION
#define SEQ66_GIT_VERSION SEQ66_VERSION
#endif

#ifndef SEQ66_PACKAGE_VERSION
#define SEQ66_PACKAGE_VERSION SEQ66_VERSION
#endif

/**
 *  This macro helps us adapt our "ui" includes to freaking qmake's
 *  conventions.  We used "userinterface.ui.h", while qmake is stuck on
 *  "ui_user_interface.h".
 *
 *  It's almost enough to make you use Cmake.  :-D
 */

#ifndef SEQ66_QMAKE_RULES
#define SEQ66_QMAKE_RULES
#endif

/* "Distro where build was done" */

#ifndef SEQ66_APP_BUILD_OS
#define SEQ66_APP_BUILD_OS "'Windows/Mac/qmake'"
#endif

/**
 * Names this version of application, plus the engine in use, and the type of
 * application.  Useful in Help / Build Info.
 * "qp" means "Qmake/Qt PortMidi-based".
 */

#ifndef SEQ66_APP_ENGINE
#define SEQ66_APP_ENGINE "portmidi"
#endif

#ifndef SEQ66_APP_NAME
#define SEQ66_APP_NAME "qpseq66"
#endif

#ifndef SEQ66_APP_TYPE
#define SEQ66_APP_TYPE "qt5"
#endif

#undef SEQ66_APP_CLI

/*
 * "The name to display as client/port".  This name is the same no matter what
 * is the name of the executable.  This is also the name of the default
 * configuration directory in the "home" area of the user.
 */

#ifndef SEQ66_CLIENT_NAME
#define SEQ66_CLIENT_NAME "seq66"
#endif

/*
 * The LIBLO library is unavailable on Windows.  Don't know about Mac!
 */

#undef SEQ66_CONFIG_LIBLO

/*
 * Names the configuration file for this version of application. The "q"
 * stands for Qt, and the "p" stands for "portmidi".
 */

#ifndef SEQ66_CONFIG_NAME
#define SEQ66_CONFIG_NAME "qpseq66"
#endif

/*
 * Define COVFLAGS=-fprofile-arcs -ftest-coverage if coverage support is
 * wanted.
 */

#undef SEQ66_COVFLAGS

/*
 * Define DBGFLAGS=-ggdb -O0 -DDEBUG -fno-inline if debug support is wanted.
 */

#ifndef SEQ66_DBGFLAGS
#define SEQ66_DBGFLAGS -O3 -DDEBUG -D_DEBUG -fno-inline
#endif

/* Define to 1 if you have the <ctype.h> header file. */
#ifndef SEQ66_HAVE_CTYPE_H
#define SEQ66_HAVE_CTYPE_H 1
#endif

/* Define to 1 if you have the <dlfcn.h> header file. */
#ifndef SEQ66_HAVE_DLFCN_H
#define SEQ66_HAVE_DLFCN_H 1
#endif

/* Define to 1 if you have the <errno.h> header file. */
#ifndef SEQ66_HAVE_ERRNO_H
#define SEQ66_HAVE_ERRNO_H 1
#endif

/* Define to 1 if you have the <fcntl.h> header file. */
#ifndef SEQ66_HAVE_FCNTL_H
#define SEQ66_HAVE_FCNTL_H 1
#endif

/* Define to 1 if you have the <getopt.h> header file. */
#ifndef SEQ66_HAVE_GETOPT_H
#define SEQ66_HAVE_GETOPT_H 1
#endif

/* Define to 1 if you have the <inttypes.h> header file. */
#ifndef SEQ66_HAVE_INTTYPES_H
#define SEQ66_HAVE_INTTYPES_H 1
#endif

/* Define to 1 if you have the `asound' library (-lasound). */
#ifndef SEQ66_HAVE_LIBASOUND
#define SEQ66_HAVE_LIBASOUND 1
#endif

/* Define to 1 if you have the <limits.h> header file. */
#ifndef SEQ66_HAVE_LIMITS_H
#define SEQ66_HAVE_LIMITS_H 1
#endif

/* Define to 1 if you have the <memory.h> header file. */
#ifndef SEQ66_HAVE_MEMORY_H
#define SEQ66_HAVE_MEMORY_H 1
#endif

/* Define if you have POSIX threads libraries and header files. */
#ifndef SEQ66_HAVE_PTHREAD
#define SEQ66_HAVE_PTHREAD 1
#endif

/* Have PTHREAD_PRIO_INHERIT. */
#ifndef SEQ66_HAVE_PTHREAD_PRIO_INHERIT
#define SEQ66_HAVE_PTHREAD_PRIO_INHERIT 1
#endif

/* Define to 1 if you have the <stdarg.h> header file. */
#ifndef SEQ66_HAVE_STDARG_H
#define SEQ66_HAVE_STDARG_H 1
#endif

/* Define to 1 if you have the <stddef.h> header file. */
#ifndef SEQ66_HAVE_STDDEF_H
#define SEQ66_HAVE_STDDEF_H 1
#endif

/* Define to 1 if you have the <stdint.h> header file. */
#ifndef SEQ66_HAVE_STDINT_H
#define SEQ66_HAVE_STDINT_H 1
#endif

/* Define to 1 if you have the <stdio.h> header file. */
#ifndef SEQ66_HAVE_STDIO_H
#define SEQ66_HAVE_STDIO_H 1
#endif

/* Define to 1 if you have the <stdlib.h> header file. */
#ifndef SEQ66_HAVE_STDLIB_H
#define SEQ66_HAVE_STDLIB_H 1
#endif

/* Define to 1 if you have the <strings.h> header file. */
#ifndef SEQ66_HAVE_STRINGS_H
#define SEQ66_HAVE_STRINGS_H 1
#endif

/* Define to 1 if you have the <string.h> header file. */
#ifndef SEQ66_HAVE_STRING_H
#define SEQ66_HAVE_STRING_H 1
#endif

/* Define to 1 if you have the <syslog.h> header file. */
#ifndef SEQ66_HAVE_SYSLOG_H
#define SEQ66_HAVE_SYSLOG_H 1
#endif

/* Define to 1 if you have the <sys/stat.h> header file. */
#ifndef SEQ66_HAVE_SYS_STAT_H
#define SEQ66_HAVE_SYS_STAT_H 1
#endif

/* Define to 1 if you have the <sys/sysctl.h> header file. */
#ifndef SEQ66_HAVE_SYS_SYSCTL_H
#define SEQ66_HAVE_SYS_SYSCTL_H 1
#endif

/* Define to 1 if you have the <sys/time.h> header file. */
#ifndef SEQ66_HAVE_SYS_TIME_H
#define SEQ66_HAVE_SYS_TIME_H 1
#endif

/* Define to 1 if you have the <sys/types.h> header file. */
#ifndef SEQ66_HAVE_SYS_TYPES_H
#define SEQ66_HAVE_SYS_TYPES_H 1
#endif

/* Define to 1 if you have the <time.h> header file. */
#ifndef SEQ66_HAVE_TIME_H
#define SEQ66_HAVE_TIME_H 1
#endif

/* Define to 1 if you have the <unistd.h> header file. */
#ifndef SEQ66_HAVE_UNISTD_H
#define SEQ66_HAVE_UNISTD_H 1
#endif

/*
 * Define to value 1 to enable JACK session.  Purely experimental, probably
 * won't work.
 */

#undef SEQ66_JACK_SESSION
#undef SEQ66_JACK_SUPPORT
#undef SEQ66_LASH_SUPPORT
#undef SEQ66_NSM_SESSION

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#ifndef SEQ66_LT_OBJDIR
#define SEQ66_LT_OBJDIR ".libs/"
#endif

/*
 * Define to enable multiple main windows.  Not to be supported in Seq66.
 */

#undef SEQ66_MULTI_MAINWID

/* Name of package */
#ifndef SEQ66_PACKAGE
#define SEQ66_PACKAGE "seq66"
#endif

/* Define to the address where bug reports for this package should be sent. */
#ifndef SEQ66_PACKAGE_BUGREPORT
#define SEQ66_PACKAGE_BUGREPORT "ahlstromcj@gmail.com"
#endif

/* Define to the full name of this package. */
#ifndef SEQ66_PACKAGE_NAME
#define SEQ66_PACKAGE_NAME "Seq66"
#endif

/* Define to the full name and version of this package. */
#ifndef SEQ66_PACKAGE_STRING
#define SEQ66_PACKAGE_STRING "Seq66 0.90.4"
#endif

/* Define to the one symbol short name of this package. */
#ifndef SEQ66_PACKAGE_TARNAME
#define SEQ66_PACKAGE_TARNAME "seq66"
#endif

/* Define to the home page for this package. */
#ifndef SEQ66_PACKAGE_URL
#define SEQ66_PACKAGE_URL ""
#endif

/*
 * Define PROFLAGS=-pg (gprof) or -p (prof) if profile support is wanted.
 */

#ifndef SEQ66_PROFLAGS
#define SEQ66_PROFLAGS
#endif


/*
 * Indicates that Qt5 is enabled.  Currently not yet in configure.ac nor used in
 * any module, but....
 */

#ifndef SEQ66_QTMIDI_SUPPORT
#define SEQ66_QTMIDI_SUPPORT 1
#endif

#undef SEQTOOL_SUPPORT
#undef STATISTICS_SUPPORT
#undef SEQTOOL_TESTING_SUPPORT

#ifndef SEQ66_STDC_HEADERS
#define SEQ66_STDC_HEADERS 1
#endif

#ifndef SEQ66__GNU_SOURCE
#define SEQ66__GNU_SOURCE 1
#endif

#endif  // SEQ66_QT_MIDILIB_CONFIG
#endif  // SEQ66_PORTMIDI_SUPPORT 1

/*
 * seq66-config.h for Qt/PortMidi
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */


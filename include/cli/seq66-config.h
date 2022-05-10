#ifndef _INCLUDE_SEQ___CONFIG_H
#define _INCLUDE_SEQ___CONFIG_H 1
 
/* include/seq66-config.h. Generated automatically at end of configure. */
/* include/config.h.  Generated from config.h.in by configure.  */
/* include/config.h.in.  Generated from configure.ac by autoheader.  */

#ifndef SEQ66_VERSION_DATE_SHORT
#define SEQ66_VERSION_DATE_SHORT "2022-05-10"
#endif
#ifndef SEQ66_VERSION
#define SEQ66_VERSION "0.98.8"
#endif



/* "Distro of build" */
#ifndef SEQ66_APP_BUILD_ISSUE
#define SEQ66_APP_BUILD_ISSUE "Ubuntu 20.04.4 LTS"
#endif

/* "OS/kernel where build was done" */
#ifndef SEQ66_APP_BUILD_OS
#define SEQ66_APP_BUILD_OS "'Linux 5.13.0-30-generic x86_64'"
#endif

/* Indicate the CLI version */
#ifndef SEQ66_APP_CLI
#define SEQ66_APP_CLI 1
#endif

/* Name of the MIDI engine */
#ifndef SEQ66_APP_ENGINE
#define SEQ66_APP_ENGINE "rtmidi"
#endif

/* Name of this application */
#ifndef SEQ66_APP_NAME
#define SEQ66_APP_NAME "seq66cli"
#endif

/* Name of the UI */
#ifndef SEQ66_APP_TYPE
#define SEQ66_APP_TYPE "cli"
#endif

/* "Name to display as client/port" */
#ifndef SEQ66_CLIENT_NAME
#define SEQ66_CLIENT_NAME "seq66"
#endif

/* Configuration file name */
#ifndef SEQ66_CONFIG_NAME
#define SEQ66_CONFIG_NAME "seq66cli"
#endif

/* Define COVFLAGS=-fprofile-arcs -ftest-coverage if coverage support is
   wanted. */
#ifndef SEQ66_COVFLAGS
#define SEQ66_COVFLAGS 
#endif

/* Define DBGFLAGS=-g -O0 -DDEBUG -fno-inline if debug support is wanted. */
#ifndef SEQ66_DBGFLAGS
#define SEQ66_DBGFLAGS 
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

/* Define to 1 if you have the <jack/jack.h> header file. */
#ifndef SEQ66_HAVE_JACK_JACK_H
#define SEQ66_HAVE_JACK_JACK_H 1
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

/* Canonical icon name for Freedesktop */
#ifndef SEQ66_ICON_NAME
#define SEQ66_ICON_NAME "qseq66"
#endif

/* Enable JACK version string */
#ifndef SEQ66_JACK_GET_VERSION_STRING
#define SEQ66_JACK_GET_VERSION_STRING 1
#endif

/* Define to enable JACK metadata */
#ifndef SEQ66_JACK_METADATA
#define SEQ66_JACK_METADATA 1
#endif

/* Define to enable JACK session */
#ifndef SEQ66_JACK_SESSION
#define SEQ66_JACK_SESSION 1
#endif

/* Define to enable JACK driver */
#ifndef SEQ66_JACK_SUPPORT
#define SEQ66_JACK_SUPPORT 1
#endif

/* Define if LIBLO library is available */
#ifndef SEQ66_LIBLO_SUPPORT
#define SEQ66_LIBLO_SUPPORT 1
#endif

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#ifndef SEQ66_LT_OBJDIR
#define SEQ66_LT_OBJDIR ".libs/"
#endif

/* Define to enable JACK port refresh */
/* #undef MIDI_PORT_REFRESH */

/* Define to enable NSM */
#ifndef SEQ66_NSM_SUPPORT
#define SEQ66_NSM_SUPPORT 1
#endif

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
#define SEQ66_PACKAGE_STRING "Seq66 0.98.8"
#endif

/* Define to the one symbol short name of this package. */
#ifndef SEQ66_PACKAGE_TARNAME
#define SEQ66_PACKAGE_TARNAME "seq66"
#endif

/* Define to the home page for this package. */
#ifndef SEQ66_PACKAGE_URL
#define SEQ66_PACKAGE_URL ""
#endif

/* Define to the version of this package. */
#ifndef SEQ66_PACKAGE_VERSION
#define SEQ66_PACKAGE_VERSION "0.98.8"
#endif

/* Indicates if portmidi is enabled */
/* #undef PORTMIDI_SUPPORT */

/* Define PROFLAGS=-pg (gprof) or -p (prof) if profile support is wanted. */
#ifndef SEQ66_PROFLAGS
#define SEQ66_PROFLAGS 
#endif

/* Define to necessary symbol if this constant uses a non-standard name on
   your system. */
/* #undef PTHREAD_CREATE_JOINABLE */

/* Indicates that rtmidi is enabled */
#ifndef SEQ66_RTMIDI_SUPPORT
#define SEQ66_RTMIDI_SUPPORT 1
#endif

/* Define to 1 if you have the ANSI C header files. */
#ifndef SEQ66_STDC_HEADERS
#define SEQ66_STDC_HEADERS 1
#endif

/* Version number of package */
#ifndef SEQ66_VERSION
#define SEQ66_VERSION "0.98.8"
#endif

/* Define to 1 if the X Window System is missing or not being used. */
/* #undef X_DISPLAY_MISSING */

/* gnu source */
#ifndef SEQ66__GNU_SOURCE
#define SEQ66__GNU_SOURCE 1
#endif

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

#if defined SEQ66_PORTMIDI_SUPPORT
#/**/undef/**/ SEQ66_RTMIDI_SUPPORT
#endif

#if defined SEQ66_WINDOWS_SUPPORT
#/**/undef/**/ SEQ66_RTMIDI_SUPPORT
#endif


 
/* once: _INCLUDE_SEQ___CONFIG_H */
#endif

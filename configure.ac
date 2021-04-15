dnl ***************************************************************************
dnl configure.ac (Seq66)
dnl ---------------------------------------------------------------------------
dnl
dnl \file          configure.ac
dnl \library       Seq66
dnl \author        Chris Ahlstrom
dnl \date          2018-11-09
dnl \update        2021-04-17
dnl \version       $Revision$
dnl \license       $XPC_SUITE_GPL_LICENSE$
dnl
dnl   This script is for the Seq66 project, a repackaging and fixing
dnl   of sequencer64.
dnl
dnl   Process this file with bootstrap (autoconf) to produce a configure
dnl   script.  Run "./bootstrap --help" for more information.
dnl
dnl ---------------------------------------------------------------------------

AC_PREREQ(2.60)
AC_REVISION($Revision: 0.96$)
AC_INIT(Seq66, [0.93.0], [ahlstromcj@gmail.com])
AC_CONFIG_AUX_DIR([aux-files])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_HOST
AM_INIT_AUTOMAKE([dist-bzip2])
AC_DEFINE(_GNU_SOURCE, 1, [gnu source])
AC_CONFIG_HEADER([include/config.h])
export PKG_CONFIG=$(which pkg-config)

dnl Prepends "SEQ66_" to the macro definitions, for a better guarantee of
dnl avoiding naming conflicts in macros.

AX_PREFIX_CONFIG_H([include/seq66-config.h], [seq66])

dnl On a Debian Sid laptop, we have found that gcc/g++ version 9 breaks
dnl configuration with the error 'C++ preprocessor "/lib/cpp" fails sanity
dnl check' (it cannot find limits.h!!!), so we try version 8 first.  And that
dnl cannot be installed on Debian Sid (2020-03-31), so we fall back to
dnl version 7.
dnl
dnl AC_GCC_VERSION
dnl AC_LANG([C])
dnl AC_LANG([C++])
dnl
dnl Now debug-build issues have migrated to our other dev laptop!
dnl
dnl AC_PROG_CC([gcc-8 gcc-7 gcc])
dnl AC_PROG_CXX([g++-8 g++-7 g++])
dnl AC_PROG_CXXCPP([cpp-8 cpp-7 cpp])

AC_PROG_CC([gcc])
AC_PROG_CXX([g++])
AC_PROG_CXXCPP([cpp])
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MKDIR_P

dnl 5.c. Mingw support.  Provisional, qmake support is more reliable at
dnl present.

AC_MSG_CHECKING([whether to build with mingw32 for a Windows host])
AS_CASE([$host_os], [*mingw*], [windows_host=yes], [windows_host=no])

dnl Note that the BUILD_XXXX macros "merely" select which subdirectories we
dnl descend into during the build.

build_docs="no"
build_portmidi="no"
build_qtmidi="yes"
build_rtcli="no"
build_rtmidi="yes"
build_seqtool="no"
build_sessions="yes"
build_testing="no"
need_qt="yes"

dnl Untested.  Currently, qmake and mingw is used for Windows code.

if test "x$windows_host" = "xyes" ; then
    if test "x$CC" = "xgcc" ; then
        AC_MSG_ERROR([MingW build enabled, but MingW packages not installed.])
    else
        build_windows="yes"
        need_qt="yes"
        AC_DEFINE(WINDOWS_SUPPORT, 1, [Indicates limited Windows support])
        AC_MSG_RESULT([MingW Windows (Qt) build enabled.])
    fi
fi

dnl New for autoconf 2.60, prevents spurious datarootdir warnings.

AC_DEFUN([AC_DATAROOTDIR_CHECKED])

dnl 1. Package version information.  We define the name of the package, the
dnl    name of the client application for display, and the version.

PACKAGE="seq66"
AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE")
AC_SUBST(PACKAGE)

dnl AC_DEFINE(APP_NAME, ["seq66"], "The default name of the Seq66 executable")

AC_DEFINE(APP_TYPE, ["qt5"], "The type of the Seq66 executable, qt/cli")
AC_DEFINE(APP_ENGINE, ["rtmidi"], "Seq66 MIDI engine, rtmidi/portmidi")
build_os="'$(uname -srm)'"
AC_DEFINE_UNQUOTED(APP_BUILD_OS, ["$build_os"], "Distro where build was done")
AC_DEFINE(CLIENT_NAME, ["seq66"], "The name to display as client/port")

VERSION="0.93.0"
AC_DEFINE_UNQUOTED(VERSION, "$VERSION")
AC_SUBST(VERSION)

SEQ66_SUITE_NAME="SEQ66"
AC_SUBST([SEQ66_SUITE_NAME])

dnl Set up version information for this project.

SEQ66_API_MAJOR="0"
AC_SUBST([SEQ66_API_MAJOR])

SEQ66_API_MINOR="93"
AC_SUBST([SEQ66_API_MINOR])

SEQ66_API_PATCH="0"
AC_SUBST([SEQ66_API_PATCH])

SEQ66_OFFICIAL_VERSION="$SEQ66_API_MAJOR.$SEQ66_API_MINOR.$SEQ66_API_PATCH"

dnl For version 1.0.0, we will reset the libtool version information.

SEQ66_LT_CURRENT="0"
AC_SUBST([SEQ66_LT_CURRENT])

SEQ66_LT_REVISION="0"
AC_SUBST([SEQ66_LT_REVISION])

SEQ66_LT_AGE="0"
AC_SUBST([SEQ66_LT_AGE])

SEQ66_LIBTOOL_VERSION="$SEQ66_LT_CURRENT.$SEQ66_LT_REVISION.$SEQ66_LT_AGE"

dnl 1.b. Set up the version strings.  The primary version string of interest
dnl      is SEQ66_API_VERSION.

SEQ66_API_VERSION="$SEQ66_API_MAJOR.$SEQ66_API_MINOR"
AC_SUBST([SEQ66_API_VERSION])

dnl 1.c. Report the version information in the progress output.

AC_MSG_CHECKING(major version)
AC_MSG_RESULT($SEQ66_API_MAJOR)
AC_MSG_CHECKING(minor version)
AC_MSG_RESULT($SEQ66_API_MINOR)
AC_MSG_CHECKING(patchlevel)
AC_MSG_RESULT($SEQ66_API_PATCH)

dnl 1.d. Set up the suite name and current-project name.

SEQ66_PROJECT_NAME="SEQ66"
AC_SUBST([SEQ66_PROJECT_NAME])

dnl 2. Libtool
dnl
dnl Initialize libtool in the default manner, which turns on shared
dnl libraries if available, and enable static libraries if they do not conflict
dnl with shared libraries.  Also sets the shell variable LIBTOOL_DEPS.
dnl The latest GNU documentation specifies version 2 of libtool, but Debian
dnl testing provides only 1.5 (Debian unstable now provides 2.4).  So we don't
dnl use these new macros, just the older, deprecated macros.

AC_PROG_LIBTOOL

LT_PREREQ([2.4.2])
LT_INIT([win32-dll])
LT_RELEASE="$SEQ66_API_MAJOR.$SEQ66_API_MINOR"
AC_SUBST([LT_RELEASE])

LT_CURRENT="$SEQ66_API_MAJOR"
AC_SUBST([LT_CURRENT])

LT_REVISION="$SEQ66_API_MINOR"
AC_SUBST([LT_REVISION])

LT_AGE="$SEQ66_API_PATCH"
AC_SUBST([LT_AGE])

dnl 3. Set up expandable installation directory(ies).

seq66includedir="${includedir}/seq66-${SEQ66_API_VERSION}"
AC_SUBST(seq66includedir)

seq66libdir="${libdir}/seq66-${SEQ66_API_VERSION}"
AC_SUBST(seq66libdir)

seq66docdir="${datadir}/doc/seq66-${SEQ66_API_VERSION}"
AC_SUBST(seq66docdir)

seq66datadir="${datadir}/seq66-${SEQ66_API_VERSION}"
AC_SUBST(seq66datadir)

seq66doxygendir="${datadir}/doc/seq66-${SEQ66_API_VERSION}"
AC_SUBST(seq66doxygendir)

dnl 4. Get external flags, if any.

CFLAGS="${CFLAGS}"
ac_cflags="${CFLAGS}"

dnl Check for header files.  Added more to support daemonization.

AC_HEADER_STDC
AC_CHECK_HEADERS([getopt.h string.h limits.h])
AC_CHECK_HEADERS([stdarg.h stdio.h stddef.h stdlib.h string.h])
AC_CHECK_HEADERS([limits.h ctype.h sys/time.h time.h errno.h])
AC_CHECK_HEADERS([fcntl.h sys/stat.h sys/sysctl.h])
AC_CHECK_HEADERS([syslog.h unistd.h])

dnl Checks for typedefs, structures, and compiler characteristics.

AC_C_CONST

dnl Doxygen support has been added back!  However, it is disabled by default.
dnl Check for the presence of Doxygen.  Not an error to be missing, but
dnl we don't want to try to build documentation if it is not present.
dnl See http://www.gnu.org/software/autoconf-archive/ax_prog_doxygen.html
dnl Note the building the documentation also requires graphviz to be installed.

AC_CHECK_PROGS([DOXYGEN], [doxygen])
if test -z "$DOXYGEN"; then
    AC_MSG_WARN([Doxygen not found, no developer docs will be built])
else
    AC_ARG_ENABLE(docs,
        [AS_HELP_STRING(--enable-docs, [Enable developer document support])],
        [build_docs=$enableval],
        [build_docs=no])
fi

if test "x$build_docs" = "xyes" ; then
    AC_CHECK_PROGS([LATEX], [latex])
    if test -z "$LATEX"; then
        AC_MSG_WARN([LaTeX not found, no docs will be built])
        build_docs=no
    fi
fi

dnl Windows support in Linux.  Might be unnecessary though, though.

AC_ARG_ENABLE(mingw,
    [AS_HELP_STRING(--enable-mingw, [Enable MingW Windows support])],
    [mingw=$enableval],
    [mingw=no])

dnl Clear out the X11 flags for the case where we are cross-compiling
dnl for i586-mingw32 targets, under UNIX.

if test "x$mingw" = "xyes" ; then
   X_CFLAGS=""
   X_LIBS=""
fi

dnl JACK support

AC_ARG_ENABLE(jack,
    [AS_HELP_STRING(--disable-jack, [Disable JACK support])],
    [jack=$enableval],
    [jack=yes])

dnl JACK session support is now deprecated (circa 2020-10-26), and so now the
dnl default for this option is 'no'.  JACK recommends using NSM instead!

AC_ARG_ENABLE(jack-session,
    [AS_HELP_STRING(--disable-jack-session, [Disable JACK session support])],
    [jack_session=$enableval],
    [jack_session=no])

if test "$jack" != "no"; then
    PKG_CHECK_MODULES(JACK, jack >= 0.90.0, jack_found="yes", jack_found="no")
    if test "$jack_found" = "yes"; then
        AC_DEFINE(JACK_SUPPORT, 1, [Define to enable JACK driver])
        AC_SUBST(JACK_CFLAGS)
        AC_SUBST(JACK_LIBS)
        if test "$jack_session" != "no"; then
            AC_CHECK_HEADER(jack/session.h, jack_session_found="yes",
                            jack_session_found="no")
            if test "$jack_session_found" = "yes"; then
                AC_DEFINE(JACK_SESSION, 1, [Define to enable JACK session])
                AC_MSG_RESULT([JACK session support enabled.]);
            fi
        fi
        AC_MSG_RESULT([JACK support enabled.]);

    else
        AC_MSG_WARN([Could not find JACK library, disabling JACK.])
    fi
else
    AC_MSG_NOTICE([JACK disabled in the configuration.]);
fi

dnl LASH support has been added back!  However, it is disabled by default.

AC_ARG_ENABLE(lash,
    [AS_HELP_STRING(--enable-lash, [Enable LASH support (deprecated)])],
    [lash=$enableval],
    [lash=no])

if test "$lash" != "no"; then
    PKG_CHECK_MODULES([LASH], [lash-1.0 >= 0.5.0], [lash_found="yes"],
        [lash_found="no"])
    if test "$lash_found" = "yes"; then
        AC_DEFINE(LASH_SUPPORT, 1, [Define to enable LASH (default=no)])
        AC_SUBST(LASH_CFLAGS)
        AC_SUBST(LASH_LIBS)
        AC_MSG_RESULT([LASH support enabled.]);
        ac_nsm=yes
    else
        AC_MSG_WARN([LASH not found, session support will not be built.])
    fi
else
    AC_MSG_NOTICE([LASH support disabled.]);
fi

dnl Enable NSM support.  Nearly ready for prime time!

AC_ARG_ENABLE(nsm,
  AS_HELP_STRING([--enable-nsm], [Enable NSM support (default=yes)]),
  [ac_nsm=$enableval],
  [ac_nsm=yes])

dnl Check for NSM support.

NSM_CFLAGS=
NSM_LIBS=
NSM_DEPS=

if test "$ac_nsm" != "no"; then
    AC_DEFINE(NSM_SUPPORT, 1, [Define if NSM support is available.])
    NSM_CFLAGS="-I ../libsessions/include"
    NSM_LIBS="-llo -L../libsessions/src/.libs/ -lsessions"
    NSM_DEPS="../libsessions/src/.libs/libsessions.la"
    AC_DEFINE(NSM_SUPPORT, 1, [Define to enable NSM])
    AC_MSG_RESULT([Sessions/NSM support enabled.]);
fi

AC_SUBST(NSM_CFLAGS)
AC_SUBST(NSM_LIBS)
AC_SUBST(NSM_DEPS)

dnl Check for now-required LIBLO library, needed for NSM support.

PKG_CHECK_MODULES([LIBLO], [liblo], [ac_liblo="yes"], [ac_liblo="no"])
if test "x$ac_liblo" = "xyes"; then
    AC_DEFINE(LIBLO_SUPPORT, 1, [Define if LIBLO library is available.])
    AC_SUBST(LIBLO_CFLAGS)
    AC_SUBST(LIBLO_LIBS)
    AC_MSG_RESULT([LIBLO support enabled.]);
else
    AC_MSG_ERROR([LIBLO library not found, OSC dev package required.])
    ac_nsm="no"
fi

dnl "RtMidi" support.  This is now the default build for Seq66.

AC_ARG_ENABLE(rtmidi,
    [AS_HELP_STRING(--enable-rtmidi, [Enable rtmidi build, default MIDI engine])],
    [rtmidi=$enableval],
    [rtmidi=yes])

dnl AC_DEFINE(APP_TYPE, ["qt"], [Names the UI of the application])

if test "$rtmidi" != "no"; then
    build_rtmidi="yes"
    build_portmidi="no"
    AC_DEFINE(APP_NAME, ["seq66"], [Names the JACK/ALSA version of application])
    AC_DEFINE(APP_ENGINE, ["rtmidi"], [Names the MIDI engine of the application])
    AC_DEFINE(CONFIG_NAME, ["seq66"], [Configuration file name])
    AC_DEFINE(RTMIDI_SUPPORT, 1, [Indicates if rtmidi is enabled])
    AM_PATH_ALSA(0.9.0)
    AC_MSG_RESULT([rtmidi engine build enabled.]);
fi

dnl "Qt" support.  Enabled by default, and actually mandatory for this
dnl application.

AC_ARG_ENABLE(qt,
    [AS_HELP_STRING(--enable-qt, [Enable Qt5 user-interface build])],
    [ac_qtmidi=$enableval],
    [ac_qtmidi=yes])

dnl Will set the default to yes later.  Will need to distinguish between rtmidi
dnl and portmidi builds.

if test "$ac_qtmidi" != "no"; then
    build_qtmidi="yes"
    AC_DEFINE(APP_NAME, ["qseq66"], [Names the Qt/rtmidi version of application])
    AC_DEFINE(APP_ENGINE, ["rtmidi"], [Names the MIDI engine of the application])
    AC_DEFINE(APP_TYPE, ["qt5"], [Names the UI of the application])
    AC_DEFINE(CONFIG_NAME, ["qseq66"], [Configuration file name for Qt/rtmidi])
    AM_PATH_ALSA(0.9.0)
    AX_HAVE_QT_MIN
    AC_MSG_RESULT([qt user-interface build enabled.]);
fi

dnl "CLI" or "Command-line" support.

AC_ARG_ENABLE(cli,
    [AS_HELP_STRING(--enable-cli, [Enable rtmidi command-line build])],
    [cli=$enableval],
    [cli=no])

if test "$cli" != "no"; then
    AC_MSG_RESULT([rtmidi command-line build enabled.]);
    AC_DEFINE(APP_NAME, ["seq66cli"], [Names the CLI version of application])
    AC_DEFINE(APP_CLI, [1], [Indicate the CLI version of application])
    AC_DEFINE(APP_TYPE, ["cli"], [Names the UI of the application])
    AC_DEFINE(APP_ENGINE, ["rtmidi"], [Names the MIDI engine of the application])
    AC_DEFINE(CONFIG_NAME, ["seq66cli"], [Configuration file name])
    if test "x$mingw" != "xyes" ; then
        build_rtcli="yes"
        build_qtmidi="no"
        build_portmidi="no"
        AC_DEFINE(RTMIDI_SUPPORT, 1, [Indicates that rtmidi is enabled])
        AM_PATH_ALSA(0.9.0)
    fi
    need_qt="no"
fi

dnl PortMidi support.  Deprecated for Linux, but we still build it, for
dnl testing purposes. It needs the ALSA libraries to work.

AC_ARG_ENABLE(portmidi,
    [AS_HELP_STRING(--enable-portmidi, [Enable PortMidi build (for testing)])],
    [portmidi=$enableval],
    [portmidi=no])

if test "$portmidi" != "no"; then
    build_portmidi="yes"
    build_rtmidi="no"
    AC_DEFINE(PORTMIDI_SUPPORT, 1, [Indicates if PortMidi support is enabled])
    AC_DEFINE(APP_NAME, ["seq66portmidi"], [Names this version of application])
    AC_DEFINE(CONFIG_NAME, ["seq66portmidi"], [Configuration file name])
    AM_PATH_ALSA(0.9.0)
    AC_MSG_RESULT([PortMidi build enabled.]);
else
    AC_MSG_NOTICE([PortMidi build disabled.]);
fi

dnl "seqtool" support.  This is (optionally) a test application and a toolbox
dnl for Seq66.

AC_ARG_ENABLE(seqtool,
    [AS_HELP_STRING(--enable-seqtool, [Enable seqtool build, the tool app])],
    [seqtool=$enableval],
    [seqtool=no])

if test "$seqtool" != "no"; then
    build_seqtool="yes"
    build_qtmidi="yes"
    AC_DEFINE(TOOL_APP_NAME, ["seqtool"], [Names the Seq66 tool application])
    AC_DEFINE(TOOL_CONFIG_NAME, ["seqtool"], [Seqtool configuration file name])
    AC_DEFINE(SEQTOOL_SUPPORT, 1, [Indicates if seqtool is enabled])
    AM_PATH_ALSA(0.9.0)
    AC_MSG_RESULT([seqtool application build enabled.]);
fi

dnl "testing" support.  This enables seqtool to use the xpccut library to build
dnl a test application and toolbox for Seq66.
dnl Check that xpccut and xpccut++ libraries are installed.
dnl The latter seems to screw up, maybe because of the "++".

AC_ARG_ENABLE(testing,
    [AS_HELP_STRING(--enable-testing, [Enable a seqtool testing build])],
    [testing=$enableval],
    [testing=no])

if test "$testing" != "no"; then
    build_testing="yes"
    PKG_CHECK_MODULES([XPCCUT], [xpccut-1.1 >= 0.9])
    PKG_CHECK_MODULES([XPCCUTPP], [xpccut++-1.1 >= 0.9])
    AC_MSG_RESULT([seqtool testing build enabled.]);
    AC_DEFINE(SEQTOOL_TESTING_SUPPORT, 1, [Indicates seqtool testing is enabled])
fi

AC_SUBST(APP_NAME)
AC_SUBST(APP_TYPE)
AC_SUBST(APP_ENGINE)
AC_SUBST(APP_BUILD_OS)
AC_SUBST(CLIENT_NAME)
AC_SUBST(CONFIG_NAME)

AM_CONDITIONAL([BUILD_DOCS], [test "$build_docs" = "yes"])
AM_CONDITIONAL([BUILD_PORTMIDI], [test "$build_portmidi" = "yes"])
AM_CONDITIONAL([BUILD_QTMIDI], [test "$build_qtmidi" = "yes"])
AM_CONDITIONAL([BUILD_RTCLI], [test "$build_rtcli" = "yes"])
AM_CONDITIONAL([BUILD_RTMIDI], [test "$build_rtmidi" = "yes"])
AM_CONDITIONAL([BUILD_SEQTOOL], [test "$build_seqtool" = "yes"])
AM_CONDITIONAL([BUILD_SESSIONS], [test "$build_sessions" = "yes"])
AM_CONDITIONAL([BUILD_WINDOWS], [test "$build_windows" = "yes"])

dnl 6.0  Top portion of the config.h/seq66-config.h header files.  The
dnl      seq66-config.h header file has "SEQ66_" prepended to these
dnl      automatically for us.

AH_TOP(

#define VERSION_DATE_SHORT "2021-04-17"
#define VERSION "0.93.0"

)

dnl 7. Checks for build configuration.
dnl 7.a. Compiling with debugging, coverage, or profiling options.
dnl      Implemented in m4/xpc_debug.m4.
dnl
dnl      --enable-debug
dnl      --enable-coverage
dnl      --enable-profiling

AC_XPC_DEBUGGING

dnl 8.  Set up other options in the compiler macros.
dnl
dnl -Wno-error=date-time breaks the build on KX Studio (gcc 4.8), removed.
dnl gcc 8 uncovers a warnings issue in libsigc++, so remove -Wextra for now,
dnl and add -Wno-parentheses.  2019-11-30: Add it back, to match qmake's use of
dnl equivalent -W option, to uncover more issues.
dnl
dnl m4/xpc_debug.m4 defines DBGFLAGS to enable debugging/coverage/profiling.
dnl m4/xpc_errorlog.m4 defines NOERRLOG.  Currently not used.
dnl m4/xpc_thisptr.m4 defines NOTHISPTR.  Currently not used.

WARNFLAGS="-Wall -Wextra -pedantic -Wno-parentheses $WARNINGS"
APIDEF="-DAPI_VERSION=\"$SEQ66_API_VERSION\""
SPEEDFLAGS="-ffast-math"
COMMONFLAGS="$WARNFLAGS -D_REENTRANT $APIDEF $DBGFLAGS"
WARNINGS_DISABLED="-Wno-unused-parameter -Wno-non-virtual-dtor"

dnl Check for MinGW. Workaround for libtool's DLL_EXPORT stupidity.
dnl AX_PTHREAD leaves PTHREADS_LIBS empty for gcc, and sets PTHREADS_CFLAGS
dnl to -pthread, which causes problems if we need -lpthread to appear in
dnl pkgconfig files.
dnl
dnl AX_PTHREAD
dnl
dnl That macro will cause output of just what tools are being used to build
dnl code:  VERBCFLAGS="-v"

VERBCFLAGS=""

case "$host_os" in
    *cygwin*)
        gnuwin=yes
        AX_PTHREAD
        ;;

    *mingw*)
        gnuwin=yes
        AC_MSG_NOTICE([Setting up MingW pthreads support])
        CFLAGS="$CFLAGS -mthreads -pthread "
        CPPFLAGS="-DPTW32_STATIC_LIB $CPPFLAGS "
        CXXFLAGS="$CXXFLAGS -mthreads -pthread "
        LDFLAGS="$LDFLAGS -mthreads -pthread "
        AC_DEFINE(HAVE_PTHREAD,1,[Defined to POSIX threads for mingw])
        ;;

    *)
        AC_MSG_NOTICE([Normal pthreads support])
        AC_MSG_CHECKING([Running normal PTHREAD test])
        AX_PTHREAD
        ;;
esac

dnl Note the c++14 option.  Also note that PROLDFLAGS comes from xpc_debug.m4.
dnl Trying out gnu++14 or gnu++1y (they don't work) instead, to see if we can
dnl eliminate the problem of debug linkage on some of our laptops.

CFLAGS="$CFLAGS $COMMONFLAGS"
CXXFLAGS="$CFLAGS -std=gnu++1y -pipe -Wno-variadic-macros -Wno-deprecated-declarations"
LDFLAGS="$LDFLAGS $PROLDFLAGS"

AM_CONDITIONAL(GNU_WIN, test x$gnuwin = xyes)

dnl Try to fix  the  build flags; we enable RTMIDI by default, but have to
dnl disable it when ALSA or PortMIDI builds are specified.  We have to
dnl use a trick to fool configure, which will strip out any bare #undef
dnl statement it sees.  Don't like this one bit.

AH_BOTTOM(

#ifdef SEQ66_PORTMIDI_SUPPORT
#/**/undef/**/ SEQ66_RTMIDI_SUPPORT
#endif

#ifdef SEQ66_WINDOWS_SUPPORT
#/**/undef/**/ SEQ66_RTMIDI_SUPPORT
#endif

)

dnl Set up the Makefiles.

AC_CONFIG_FILES([
 Makefile
 libseq66/Makefile
 libseq66/include/Makefile
 libseq66/src/Makefile
 libsessions/Makefile
 libsessions/include/Makefile
 libsessions/src/Makefile
 m4/Makefile
 man/Makefile
 seq_portmidi/Makefile
 seq_portmidi/include/Makefile
 seq_portmidi/src/Makefile
 seq_rtmidi/Makefile
 seq_rtmidi/include/Makefile
 seq_rtmidi/src/Makefile
 seq_qt5/Makefile
 seq_qt5/include/Makefile
 seq_qt5/forms/Makefile
 seq_qt5/src/Makefile
 resources/pixmaps/Makefile
 Seq66qt5/Makefile
 Seq66cli/Makefile
 data/Makefile
 doc/Makefile
 doc/latex/Makefile
 doc/latex/tex/Makefile
 Seqtool/Makefile
 Seqtool/forms/Makefile
 Seqtool/include/Makefile
 Seqtool/src/Makefile
])

dnl See AC_CONFIG_COMMANDS

AC_OUTPUT

AC_MSG_RESULT([$SEQ66_APP_NAME]);

cat << E_O_F

Run 'make' to build the application.  Run 'make install' as root/sudo to install
the application.  Some tests can be run using the Seqtool project, if built.  Do
'make dist', etc. to create gzip and other archives.  To build the PDF
documentation, make sure Doxygen, Graphviz, and TexLive are installed, change
to the doc/dox directory, and run './make_dox reference'.  To clean the
documentation, run './make_dox clean'.  'install' and 'uninstall' are other
options for make_dox.

E_O_F

dnl configure.ac (seq66)
dnl
dnl vim: ts=4 sw=4 et ft=config

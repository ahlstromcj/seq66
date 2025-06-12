dnl ***************************************************************************
dnl configure.ac (Seq66)
dnl ---------------------------------------------------------------------------
dnl
dnl \file          configure.ac
dnl \library       Seq66
dnl \author        Chris Ahlstrom
dnl \date          2018-11-09
dnl \update        2025-06-12
dnl \version       $Revision$
dnl \license       $XPC_SUITE_GPL_LICENSE$
dnl
dnl   This script is for the Seq66 project, a repackaging and updating
dnl   of sequencer64.
dnl
dnl   Process this file with bootstrap (autoconf) to produce a configure
dnl   script.  Run "./bootstrap --help" for more information, or read the
dnl   INSTALL file.
dnl
dnl ---------------------------------------------------------------------------

AC_PREREQ([2.72])
AC_REVISION($Revision: 0.99$)
AC_INIT([Seq66],[0.99.21],[ahlstromcj@gmail.com])
AC_CONFIG_AUX_DIR([aux-files])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_HOST
AM_INIT_AUTOMAKE([dist-bzip2])
AC_DEFINE(_GNU_SOURCE, 1, [gnu source])
AC_CONFIG_HEADERS([include/config.h])
export PKG_CONFIG=$(which pkg-config)

dnl Prepends "SEQ66_" to the macro definitions, for a better guarantee of
dnl avoiding naming conflicts in macros.

AX_PREFIX_CONFIG_H([include/seq66-config.h], [seq66])

dnl c/c++ configuration.

AC_PROG_CC(gcc clang llvm-gcc)
AC_PROG_CXX(g++ clang++ llvm-g++)
AC_PROG_CPP()
AC_PROG_CXXCPP()

dnl Important: see
dnl
dnl    https://android.googlesource.com/toolchain/llvm/+/release_34/
dnl		   autoconf/configure.ac
dnl
dnl If CXX is Clang, check that it can find and parse C++ standard library
dnl headers.  Note that space between 'include' and '(' is required.
dnl There's a broken regex in aclocal that otherwise will think that we call
dnl m4's include builtin. The linker flags are used with gcc, but not
dnl clang.

ac_linker_flag="-Wl,--copy-dt-needed-entries"
ac_clang_active="no"

if test "$CXX" = "clang++" ; then
    AC_MSG_CHECKING([whether clang works])
    AC_LANG_PUSH([C++])
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <limits>
#if __has_include (<cxxabi.h>)
#include <cxxabi.h>
#endif
#if __has_include (<unwind.h>)
#include <unwind.h>
#endif
]])],
[
    AC_MSG_RESULT([yes])
    AC_DEFINE(CLANG_SUPPORT, 1, [Define if clang is active])
    ac_linker_flag=""
    ac_clang_active="yes"
],
[
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([Compiler could not parse C++ headers. Use CC=c-compiler CXX=c++-compiler ./configure ...])
])
    AC_LANG_POP([C++])
fi

LINKER_FLAGS=$ac_linker_flag
AC_SUBST([LINKER_FLAGS])

dnl 5.c. Mingw support.  Missing, qmake support is more reliable at
dnl present.

dnl Note that the BUILD_XXXX macros "merely" select which subdirectories we
dnl descend into during the build.

ac_build_docs="no"
ac_build_windows="no"
ac_build_portmidi="no"
ac_build_qtmidi="yes"
ac_build_rtcli="no"
ac_build_rtmidi="yes"
ac_build_sessions="yes"
ac_build_testing="no"
ac_build_qt="yes"

dnl The default names describing aspects of the application.

ac_app_name="qseq66"
ac_app_type="qt"
ac_app_engine="rtmidi"
ac_app_build_issue="Linux"
ac_app_build_os="Linux"
ac_client_name="seq66"
ac_config_name="qseq66"
ac_config_dir_name="seq66"
ac_icon_name="qseq66"

dnl Test for CYGWIN/MSYS2; this macro is marked as obsolete, though:
dnl AC_CYGWIN. Instead, see AC_CANONICAL_HOST at the top of this file.
dnl It uses the host_os variable.

case $host_os in
    *cygwin* | *msys* | windows*) ac_build_windows="yes";;
	* ) ac_build_windows="no";;
esac

if test "$ac_build_windows" = "yes" ; then
    ac_build_portmidi="yes"
    ac_build_rtmidi="no"
    ac_build_sessions="no"
fi

dnl Windows build not supported.  qmake and mingw are used for Windows code.
dnl But we are trying to change that.
dnl New for autoconf 2.60, prevents spurious datarootdir warnings.

AC_DEFUN([AC_DATAROOTDIR_CHECKED])

dnl 1. Package version information.  We define the name of the package, the
dnl    name of the client application for display, and the version.

PACKAGE="seq66"
AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE")
AC_SUBST([PACKAGE])

AC_DEFINE(APP_TYPE, ["qt5"], "Type of the Seq66 executable, qt/cli")
AC_DEFINE(APP_ENGINE, ["rtmidi"], "Seq66 MIDI engine, rtmidi/portmidi")
ac_app_build_os="'$(uname -srm)'"

SEQ66_SUITE_NAME="SEQ66"
AC_SUBST([SEQ66_SUITE_NAME])

dnl Set up version information for this project.

SEQ66_API_MAJOR="0"
AC_SUBST([SEQ66_API_MAJOR])

SEQ66_API_MINOR="99"
AC_SUBST([SEQ66_API_MINOR])

SEQ66_API_PATCH="0"
AC_SUBST([SEQ66_API_PATCH])

dnl For version 1.0.0, we will reset the libtool version information.

SEQ66_LT_CURRENT="0"
AC_SUBST([SEQ66_LT_CURRENT])

SEQ66_LT_REVISION="0"
AC_SUBST([SEQ66_LT_REVISION])

SEQ66_LT_AGE="0"
AC_SUBST([SEQ66_LT_AGE])

SEQ66_LIBTOOL_VERSION="$SEQ66_LT_CURRENT:$SEQ66_LT_REVISION:$SEQ66_LT_AGE"
AC_SUBST([SEQ66_LIBTOOL_VERSION])

dnl 1.b. Set up the version strings.  The primary version string of interest
dnl      is SEQ66_API_VERSION.

SEQ66_API_VERSION="$SEQ66_API_MAJOR.$SEQ66_API_MINOR"
AC_DEFINE(API_VERSION, ["0.99"], [Seq66 API version])
AC_SUBST([API_VERSION])

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
dnl The latest GNU documentation specifies version 2 of libtool.

LT_PREREQ([2.4.2])
LT_INIT
AC_SUBST([LIBTOOL_DEPS])

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

seq66pixdir="${datadir}/pixmaps/seq66-${SEQ66_API_VERSION}"
AC_SUBST(seq66pixdir)

dnl Allow the user to change client/port name.

AC_ARG_WITH(client,
    [AS_HELP_STRING(--with-client, [Change name of client/port from default])],
    [ac_client_name=$withval],
    [])

AC_DEFINE_UNQUOTED(CLIENT_NAME, ["$ac_client_name"], "Name of client/port")

dnl 4. Get external flags, if any.

CFLAGS="${CFLAGS} -I/usr/local/include "
CXXFLAGS="${CXXFLAGS} -I/usr/local/include "
LDFLAGS="${LDFLAGS} -L/usr/local/lib "

dnl Check for header files.  Added more to support daemonization.
dnl
dnl m4_warn([obsolete],
dnl [Preprocessor macro STDC_HEADERS is obsolete. 
dnl  Except in unusual embedded environments, one can include all ISO C90
dnl  headers unconditionally.])
dnl
dnl Autoupdate added the next two lines to ensure that your configure
dnl script's behavior did not change.  They are probably safe to remove.
dnl In fact, they cause issues on Ubuntu (as opposed to Debian Sid).
dnl
dnl AC_CHECK_INCLUDES_DEFAULT
dnl AC_PROG_EGREP

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
if test -z "$DOXYGEN" ; then
    AC_MSG_WARN([Doxygen not found, no developer docs will be built])
else
    AC_ARG_ENABLE(docs,
        [AS_HELP_STRING(--enable-docs, [Enable developer document support])],
        [docs=$enableval],
        [docs=no])
    if test "$docs" != no ; then
        ac_build_docs="yes"
        AC_MSG_RESULT([Doxygen document build enabled])
        AC_CHECK_PROGS([LATEX], [latex])
        if test -z "$LATEX" ; then
            AC_MSG_WARN([LaTeX not found, no docs will be built])
            ac_build_docs="no"
        fi
    fi
fi

dnl Clear out the X11 flags for the case where we are cross-compiling
dnl for i586-mingw32 targets, under UNIX.

if test "$mingw" = "yes" ; then
   X_CFLAGS=""
   X_LIBS=""
fi

dnl JACK support, plus session and metadata support.
dnl JACK session support is now deprecated (circa 2020-10-26), but still the
dnl default for this option is 'yes'. JACK recommends using NSM instead! But
dnl Seq66 will support it for awhile yet.
dnl JACK metadata, seems to work, so now enabled by default.

PKG_CHECK_MODULES(JACK, jack >= 0.90.0, jack_found="yes", jack_found="no")
if test "$jack_found" = "yes" -a "$ac_build_windows" = "no" ; then

    AC_ARG_ENABLE(jack,
        [AS_HELP_STRING(--disable-jack, [Disable JACK support])],
        [jack=$enableval],
        [jack=yes])

    AC_ARG_ENABLE(jack-session,
        [AS_HELP_STRING(--disable-jack-session, [Disable JACK session])],
        [jack_session=$enableval],
        [jack_session=yes])

    AC_ARG_ENABLE(jack-metadata,
        [AS_HELP_STRING(--disable-jack-metadata, [Disable JACK metadata])],
        [jack_metadata=$enableval],
        [jack_metadata=yes])

    if test "$jack" != "no" ; then
        AC_DEFINE(JACK_SUPPORT, 1, [Define to enable JACK driver])
        AC_SUBST(JACK_CFLAGS)
        AC_SUBST(JACK_LIBS)
        if test "$jack_session" != "no" ; then
            AC_CHECK_HEADER(jack/session.h, jack_session_found="yes",
                            jack_session_found="no")
            if test "$jack_session_found" = "yes" ; then
                AC_DEFINE(JACK_SESSION, 1, [Define to enable JACK session])
                AC_MSG_RESULT([JACK session support enabled]);
            fi
        fi
        AC_MSG_RESULT([JACK support enabled])

        AC_CHECK_HEADERS([jack/jack.h])
        AC_CHECK_LIB([jack], [jack_get_version_string],
            jack_get_version="yes", jack_get_version="no")
        if test "$jack_get_version" = "yes" ; then
            AC_DEFINE(JACK_GET_VERSION_STRING, 1, [Enable JACK version string])
            AC_MSG_RESULT([JACK jack_get_version_string available]);
        fi

        if test "$jack_metadata" = "yes" ; then
            AC_CHECK_HEADER(jack/metadata.h, jack_metadata="yes",
                jack_metadata="no")
            if test "$jack_metadata" = "yes" ; then
                AC_DEFINE(JACK_METADATA, 1, [Define to enable JACK metadata])
                AC_MSG_RESULT([JACK metadata support enabled]);
            fi
        fi
    fi
else
    AC_MSG_WARN([No JACK library, disabling JACK])
fi

dnl Port-refresh support.  Far from ready for prime-time, but added for
dnl exploration and testing.  Disabled by default.

AC_ARG_ENABLE(port-refresh,
    [AS_HELP_STRING(--enable-port-refresh, [Enable JACK port refresh support])],
    [port_refresh=$enableval],
    [port_refresh=no])

if test "$port_refresh" != "no" ; then
    AC_DEFINE(MIDI_PORT_REFRESH, 1, [Define to enable JACK port refresh])
    AC_MSG_RESULT([JACK port refresh test code enabled])
else
    AC_MSG_NOTICE([JACK port refresh not enabled])
fi

AC_SUBST(MIDI_PORT_REFRESH)

dnl LASH support has been deleted, this time for good.  We will support only
dnl JACK Session and NSM.  Enable NSM support.  Now ready for prime time!
dnl Check for now-required LIBLO library, needed only for NSM support.

NSM_CFLAGS=
NSM_LIBS=
NSM_DEPS=

PKG_CHECK_MODULES([LIBLO], [liblo], [ac_liblo="yes"], [ac_liblo="no"])
if test "$ac_liblo" = "yes" ; then

    AC_ARG_ENABLE(nsm,
      AS_HELP_STRING([--disable-nsm], [Disable NSM support]),
      [nsm=$enableval],
      [nsm=yes])

    AC_DEFINE(LIBLO_SUPPORT, 1, [Define if LIBLO library is available])
    AC_SUBST(LIBLO_CFLAGS)
    AC_SUBST(LIBLO_LIBS)
    AC_MSG_RESULT([LIBLO support for NSM enabled]);

    if test "$nsm" != "no" ; then
        AC_DEFINE(NSM_SUPPORT, 1, [Define if NSM support is available])
        NSM_CFLAGS="-I ../libsessions/include"
        NSM_LIBS="-llo -L../libsessions/src/.libs/ -lsessions"
        NSM_DEPS="../libsessions/src/.libs/libsessions.la"
        AC_MSG_RESULT([Sessions/NSM support enabled]);
    fi

else
    AC_MSG_WARN([LIBLO dev package not found, required for NSM])
fi

AC_SUBST(NSM_CFLAGS)
AC_SUBST(NSM_LIBS)
AC_SUBST(NSM_DEPS)

dnl Can enable oth "CLI" and "rtmidi/qtsupport". The CLI version ignores
dnl the macros and fills in its values with functions from the
dnl xxx module.

AC_ARG_ENABLE(both,
    [AS_HELP_STRING(--enable-both, [Enable Qt and command-line builds])],
    [both=$enableval],
    [both=no])

if test "$both" != "no" ; then
    AC_MSG_RESULT([Both rtmidi Qt and command-line builds enabled]);
    if test "$mingw" != "yes" ; then
        ac_build_rtcli="yes"
        ac_build_qtmidi="yes"
        AM_PATH_ALSA(0.9.0)
    fi
fi

dnl "rtmidi" support.  This is now the default build for Seq66.

if test "$ac_build_windows" = "no" ; then

    AC_ARG_ENABLE(rtmidi,
        [AS_HELP_STRING(--disable-rtmidi, [Disable rtmidi MIDI engine])],
        [rtmidi=$enableval],
        [rtmidi=yes])

    if test "$rtmidi" != "no" -o "$both" != "no" ; then
        ac_build_rtmidi="yes"
        AC_DEFINE(RTMIDI_SUPPORT, 1, [Indicates the rtmidi engine is enabled])
        AM_PATH_ALSA(0.9.0)
        AC_MSG_RESULT([rtmidi engine build enabled]);
    fi

fi

dnl "CLI" or "Command-line" support.

AC_ARG_ENABLE(cli,
    [AS_HELP_STRING(--enable-cli, [Enable rtmidi command-line build])],
    [cli=$enableval],
    [cli=no])

if test "$cli" != "no" ; then
    AC_MSG_RESULT([rtmidi command-line build enabled]);
    AC_DEFINE(APP_CLI, [1], [Indicate the CLI version])
    ac_app_name="seq66cli"
    ac_app_type="cli"
    ac_config_name="seq66cli"
    if test "$mingw" != "yes" ; then
        ac_build_rtcli="yes"
        ac_build_qtmidi="no"
        AC_DEFINE(RTMIDI_SUPPORT, 1, [Indicates that rtmidi is enabled])
        AM_PATH_ALSA(0.9.0)
    fi
    ac_build_qt="no"

else

dnl "Qt" support.  Enabled by default, and actually mandatory for this
dnl application if not built for the headless mode.

AC_ARG_ENABLE(qt,
    [AS_HELP_STRING(--disable-qt, [Disable Qt5 user-interface])],
    [ac_build_qt=$enableval],
    [ac_build_qt=yes])
fi

dnl Will set the default to yes later.  Will need to distinguish between rtmidi
dnl and portmidi builds.

if test "$ac_build_qt" != "no" ; then
    ac_app_name="qseq66"
    ac_app_type="qt"
    if test "$ac_build_windows" != "yes" ; then
        AM_PATH_ALSA(0.9.0)
    fi
    if test "$ac_build_windows" = "yes" ; then
        AX_HAVE_QT
    else
        if test "$ac_clang_active" = "yes" ; then
            AX_HAVE_QT_CLANG
        else
            AX_HAVE_QT_MIN
        fi
    fi
    AC_MSG_RESULT([qt user-interface build enabled]);
fi

dnl portmidi support.  Deprecated for Linux, but we still build it, for
dnl testing purposes. It needs the ALSA libraries to work, in Linux, but
dnl in Windows it uses the Windows MultiMedia API.

AC_ARG_ENABLE(portmidi,
    [AS_HELP_STRING(--enable-portmidi, [Enable portmidi build)])],
    [portmidi=$enableval],
    [portmidi=no])

if test "$portmidi" != "no" ; then
    ac_build_portmidi="yes"
    ac_build_rtmidi="no"
    ac_app_name="seq66portmidi"
    ac_config_name="seq66portmidi"

    AC_DEFINE(PORTMIDI_SUPPORT, 1, [Indicates if portmidi is enabled])
    AM_PATH_ALSA(0.9.0)
    AC_MSG_RESULT([Portmidi build enabled]);
else
    AC_MSG_NOTICE([Portmidi build disabled]);
fi

AC_DEFINE_UNQUOTED(APP_NAME, ["$ac_app_name"], [Name of the CLI application])
AC_DEFINE_UNQUOTED(APP_TYPE, ["$ac_app_type"], [Name of the GUI/UI])
AC_DEFINE_UNQUOTED(APP_ENGINE, ["$ac_app_engine"], [Name of the MIDI engine])
AC_DEFINE_UNQUOTED(APP_BUILD_ISSUE, "[m4_normalize(esyscmd([uname -o]))]",
    [Operating system of build])
AC_DEFINE_UNQUOTED(APP_BUILD_OS, ["$ac_app_build_os"], [OS/kernel of build])
AC_DEFINE_UNQUOTED(CLIENT_NAME, ["$ac_client_name"], [Client/port base name])
AC_DEFINE_UNQUOTED(CONFIG_NAME, ["$ac_config_name"], [Configuration base file name])
AC_DEFINE_UNQUOTED(CONFIG_DIR_NAME, ["$ac_config_dir_name"],
    [Configuration sub-directory])
AC_DEFINE_UNQUOTED(ICON_NAME, ["$ac_icon_name"], [Icon name for Freedesktop])

AC_SUBST(APP_NAME)
AC_SUBST(APP_TYPE)
AC_SUBST(APP_ENGINE)
AC_SUBST(APP_BUILD_ISSUE)
AC_SUBST(APP_BUILD_OS)
AC_SUBST(CLIENT_NAME)
AC_SUBST(CONFIG_NAME)
AC_SUBST(CONFIG_DIR_NAME)
AC_SUBST(ICON_NAME)

AM_CONDITIONAL([BUILD_DOCS], [test "$ac_build_docs" = "yes"])
AM_CONDITIONAL([BUILD_PORTMIDI], [test "$ac_build_portmidi" = "yes"])
AM_CONDITIONAL([BUILD_QTMIDI], [test "$ac_build_qtmidi" = "yes"])
AM_CONDITIONAL([BUILD_RTCLI], [test "$ac_build_rtcli" = "yes"])
AM_CONDITIONAL([BUILD_RTMIDI], [test "$ac_build_rtmidi" = "yes"])
AM_CONDITIONAL([BUILD_SESSIONS], [test "$ac_build_sessions" = "yes"])

dnl 6.0  Top portion of the config.h/seq66-config.h header files.  The
dnl      seq66-config.h header file has "SEQ66_" prepended to these
dnl      macros automatically.

AH_TOP(

#define VERSION_DATE_SHORT "2025-06-12"
#define API_VERSION "0.99"
#define VERSION "0.99.21"

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

ac_gnuwin="yes"

case "$host_os" in
    *cygwin*)
        ac_gnuwin="yes"
        AX_PTHREAD
        ;;

    *mingw*)
        ac_gnuwin="yes"
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
dnl The warning disabling for clang does prevents the "clang-16: warning:
dnl optimization flag '-fno-fat-lto-objects' is not supported".
dnl Finally, after getting an std::sort() error in the triggers class under
dnl clang-16.0.6, we will force C++14.

if test "$ac_clang_active" = "yes" ; then
CFLAGS="$CFLAGS $COMMONFLAGS -Wno-ignored-optimization-argument"
else
CFLAGS="$CFLAGS $COMMONFLAGS"
fi

CXXFLAGS="$CFLAGS -std=c++14 -pipe -Wno-variadic-macros -Wno-deprecated-declarations"
LDFLAGS="$LDFLAGS $PROLDFLAGS"

AM_CONDITIONAL(GNU_WIN, test "$ac_gnuwin" = "yes")

dnl Try to fix  the  build flags; we enable RTMIDI by default, but have to
dnl disable it when ALSA or PortMIDI builds are specified.  We have to
dnl use a trick to fool configure, which will strip out any bare #undef
dnl statement it sees.  Don't like this one bit.

AH_BOTTOM(

#if defined SEQ66_PORTMIDI_SUPPORT
#/**/undef/**/ SEQ66_RTMIDI_SUPPORT
#endif

#if defined SEQ66_WINDOWS_SUPPORT
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
])

dnl See AC_CONFIG_COMMANDS

AC_OUTPUT

AC_MSG_RESULT([$SEQ66_APP_NAME]);

cat << E_O_F

Run 'make' to build the application.  Run 'make install' as root/sudo to install
the application. Do 'make dist', etc. to create gzip and other archives.  To
build the PDF user manual, make sure TexLive and latexmk are installed.
Change to the doc/latex directory, and run make.

E_O_F

dnl configure.ac (seq66)
dnl
dnl vim: ts=4 sw=4 et ft=config

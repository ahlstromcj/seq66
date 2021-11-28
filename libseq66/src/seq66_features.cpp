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
 * \file          seq66_features.cpp
 *
 *  This module adds some functions that reveal the features compiled into
 *  the Seq66 application.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2017-03-12
 * \updates       2021-11-26
 * \license       GNU GPLv2 or above
 *
 *  The first part of this file defines a couple of global structure
 *  instances, followed by the global variables that these structures
 *  completely replace.
 */

#include <sstream>                      /* std::ostringstream               */

#include "seq66_features.hpp"           /* feature macros, seq66 namespace  */

#if defined SEQ66_PLATFORM_UNIX
#include <unistd.h>                     /* C::write(2)                      */
#endif

/*
 * To do: get QT_VERSION_STR from qconfig.h
 */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Hard-wired replacements for build macros.  Also modifiable at run-time
 *  via the "set" functions
 */

#if defined SEQ66_PLATFORM_WINDOWS
static std::string s_app_build_os    = "Windows 10";    /* FIXME */
static std::string s_app_build_issue = "Microsoft Windows";
#endif

#if defined SEQ66_PLATFORM_MACOSX
static std::string s_app_build_os    = "MacOSX";        /* FIXME */
static std::string s_app_build_issue = "Apple MacOSX";
#endif

#if defined SEQ66_PLATFORM_UNIX
static std::string s_app_build_os    = SEQ66_APP_BUILD_OS;
static std::string s_app_build_issue = SEQ66_APP_BUILD_ISSUE;
#endif

static std::string s_alsa_version;
static std::string s_jack_version;
static std::string s_qt_version;
static std::string s_app_engine = SEQ66_APP_ENGINE;
static std::string s_app_name = SEQ66_APP_NAME;
static std::string s_app_type = SEQ66_APP_TYPE;
static std::string s_apptag = SEQ66_APP_NAME " " SEQ66_VERSION;
static std::string s_arg_0 = "";
static std::string s_client_name = SEQ66_CLIENT_NAME;
static std::string s_client_name_short = SEQ66_CLIENT_NAME;
static std::string s_client_name_tag = "[" SEQ66_CLIENT_NAME "]";
static std::string s_package_name = SEQ66_PACKAGE_NAME;
static std::string s_session_tag = "Session";
static std::string s_version = SEQ66_VERSION;
static std::string s_versiontext = SEQ66_APP_NAME " " SEQ66_VERSION " "
    SEQ66_GIT_VERSION " " SEQ66_VERSION_DATE_SHORT "\n";

/**
 *  Sets version strings.  Meant to be called where the engine or API is used.
 *  Otherwise, empty.
 */

void
set_alsa_version (const std::string & v)
{
    s_alsa_version = v;
}

void
set_jack_version (const std::string & v)
{
    s_jack_version = v;
}

void
set_qt_version (const std::string & v)
{
    s_qt_version = v;
}

/**
 *  Sets the current name of the application.
 */

void
set_app_name (const std::string & aname)
{
    s_app_name = aname;
}

/**
 *  Sets the current type of the application.
 */

void
set_app_type (const std::string & atype)
{
    s_app_type = atype;
}

void
set_app_engine (const std::string & aengine)
{
    s_app_engine = aengine;
}

void
set_app_build_os (const std::string & abuild_os)
{
    s_app_build_os = abuild_os;
}

void
set_app_build_issue (const std::string & abuild_issue)
{
    s_app_build_issue = abuild_issue;
}

void
set_arg_0 (const std::string & arg)
{
    s_arg_0 = arg;
}

void
set_client_name (const std::string & cname)
{
    s_client_name = cname;          /* current base name of the client port */
    s_client_name_short = cname;    /* without any "random" session portion */

    auto pos = cname.find_first_of("./:");  /* common session delimiters    */
    if (pos != std::string::npos)
        s_client_name_short = cname.substr(0, pos);

    s_client_name_tag = "[";
    s_client_name_tag += s_client_name_short;
    s_client_name_tag += "]";
}

void
set_package_name (const std::string & pname)
{
    s_package_name = pname;
}

/**
 *  Returns the name of the application.  We could continue to use the macro
 *  SEQ66_APP_NAME, but we might eventually want to make this name
 *  configurable.  Not too likely, but possible.
 */

const std::string &
seq_app_name ()
{
    return s_app_name;
}

const std::string &
seq_app_type ()
{
    return s_app_type;
}

const std::string &
seq_app_engine ()
{
    return s_app_engine;
}

const std::string &
seq_app_build_os ()
{
    return s_app_build_os;
}

const std::string &
seq_app_build_issue ()
{
    return s_app_build_issue;
}

const std::string &
seq_arg_0 ()
{
    return s_arg_0;
}

/**
 *  Returns the name of the client for the application.  It starts as the
 *  macro SEQ66_CLIENT_NAME ("seq66"), but this name is now configurable.
 *  When session management is active, the session-manager's client ID, or
 *  something derived from it, is copied to this variable.
 */

const std::string &
seq_client_name ()
{
    return s_client_name;
}

const std::string &
seq_client_short ()
{
    return s_client_name_short;
}

bool
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
 * Text color codes ('*' indicates the color is used below):
 *
 *  -   30 = black *
 *  -   31 = red *
 *  -   32 = green *
 *  -   33 = yellow *
 *  -   34 = blue *
 *  -   35 = magenta *
 *  -   36 = cyan
 *  -   37 = white
 */

std::string
seq_client_tag (msglevel el)
{
    if (el == msglevel::none)
    {
        return s_client_name_tag;
    }
    else
    {
        static const char * s_level_colors [] =
        {
            "\033[0m",          /* goes back to normal console color    */
            "\033[1;32m",       /* info message green                   */
            "\033[1;33m",       /* warning message is yellow            */
            "\033[1;31m",       /* error message is red                 */
            "\033[1;34m",       /* status message is blue               */
            "\033[1;35m",       /* special message is magenta           */
            "\033[1;30m"        /* debug message is black               */
        };
        std::string result = "[";
        int index = static_cast<int>(el);
        bool iserror = el == msglevel::error || el == msglevel::warn ||
            el == msglevel::debug;

        bool showcolor = is_a_tty(iserror ? STDERR_FILENO : STDOUT_FILENO);
        if (showcolor)
            result += s_level_colors[index];

        result += s_client_name_short;
        if (showcolor)
            result += s_level_colors[0];

        result += "]";
        return result;
    }
}

/**
 *  Returns the name of the package for the application. This is the name of
 *  the product ("Seq66") no matter what executable has been generated.
 */

const std::string &
seq_package_name ()
{
    return s_package_name;
}

std::string
session_tag (const std::string & refinement)
{
    std::string result = s_session_tag;
    if (! refinement.empty())
    {
        result += " ";
        result += refinement;
    }
    return result;
}

/**
 *  Returns the version of the application.
 */

const std::string &
seq_version ()
{
    return s_version;
}

/**
 *  Sets up the "hardwired" version text for Seq66.  This value
 *  ultimately comes from the configure.ac script (for now). It holds,
 *  among other things, the hand-crafted date in that file.
 */

const std::string &
seq_version_text ()
{
    return s_versiontext;
}

const std::string &
seq_app_tag ()
{
    return s_apptag;
}

/**
 *  This section of variables provide static information about the options
 *  enabled or disabled during the build.
 */

#if defined SEQ66_PLATFORM_32_BIT
const static std::string s_bitness = "32-bit";
#else
const static std::string s_bitness = "64-bit";
#endif

/**
 *  Generates a string describing the features of the build.
 *
 * \return
 *      Returns an ordered, human-readable string enumerating the built-in
 *      features of this application.
 */

std::string
seq_build_details ()
{
    std::ostringstream result;
#if defined SEQ66_PLATFORM_DEBUG
    std::string buildmode = "Debug";
#else
    std::string buildmode = "Release";
#endif

    result
        << "Built " << __DATE__ << " " << __TIME__ "\n"
        << "C++ version " << std::to_string(__cplusplus) << "\n"
#if defined SEQ66_PLATFORM_GNU
        << "GNU C++ " << __GNUC__ << "." << __GNUC_MINOR__
        << "." << __GNUC_PATCHLEVEL__ << "\n"
#endif
        << "Executable: " << seq_app_name()
        << "; " << seq_app_type() << " interface"
        << "; " << seq_app_engine() << " engine" << "\n"
        ;

    result
        << "Package: " << seq_package_name() << "\n"
        << "Client: " << seq_client_name() << "\n"
        ;

    result
        << "Build OS: " << seq_app_build_os() << "\n"
        << "Build Type: " << s_bitness << " " << buildmode << "\n"
        ;

#if defined SEQ66_PLATFORM_UNIX
    result << "Distro: " << seq_app_build_issue() << "\n";
#endif

    if (! s_qt_version.empty())
        result << "Qt v. " << s_qt_version << "\n";

#if defined SEQ66_RTMIDI_SUPPORT_REDUNDANT_INFO
    result << "Native JACK/ALSA (rtmidi)\n";
#endif

#if defined SEQ66_PORTMIDI_SUPPORT
    result << "PortMIDI support\n";
#endif

#if defined SEQ66_JACK_SUPPORT
    result
        << "JACK  v. " << s_jack_version << " Transport and MIDI\n"
#if defined SEQ66_JACK_SESSION
        << "JACK Session\n"
#endif
        ;
#endif

    if (! s_alsa_version.empty())
        result << "ALSA v. " << s_alsa_version << "\n";

    result
#if defined SEQ66_NSM_SUPPORT
        << "NSM (Non Session Manager) support\n"
#endif
        <<
            "\n"
            "Chord generator, LFO, trigger transpose, Tap BPM, Song recording "
            "Pattern coloring, pause, save time-sig/tempo, "
            "event editor, follow-progress.\n"
        <<
            "\n"
            "Options can be enabled/disabled via the configure script,"
            " seq66_features.h, or build-specific seq66-config.h files in"
            " include/qt/* for qmake portmidi and rtmidi builds."
        << std::endl
        ;
    return result.str();
}

}           // namespace seq66

/*
 * seq66_features.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


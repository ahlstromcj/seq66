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
 * \updates       2021-09-15
 * \license       GNU GPLv2 or above
 *
 *  The first part of this file defines a couple of global structure
 *  instances, followed by the global variables that these structures
 *  completely replace.
 */

#include <sstream>                      /* std::ostringstream               */

#include "seq66_features.hpp"           /* feature macros, seq66 namespace  */

#if defined SEQ66_JACK_SUPPORT
#include <jack/jack.h>                  /* jack_get_version()               */
#include <alsa/version.h>               /* SND_LIB_VERSION_STR, tricky      */
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
static std::string s_app_build_os = "Windows";
#endif

#if defined SEQ66_PLATFORM_MACOSX
static std::string s_app_build_os = "MacOSX";
#endif

#if defined SEQ66_PLATFORM_UNIX
static std::string s_app_build_os = SEQ66_APP_BUILD_OS;
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
static std::string s_package_name = SEQ66_PACKAGE_NAME;
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
set_arg_0 (const std::string & arg)
{
    s_arg_0 = arg;
}

void
set_client_name (const std::string & cname)
{
    s_client_name = cname;  /* the current base name of the client port */
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

/**
 *  Returns the name of the package for the application. This is the name of
 *  the product ("Seq66") no matter what executable has been generated.
 */

const std::string &
seq_package_name ()
{
    return s_package_name;
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
        << "App name: " << seq_app_name()
        << "; type " << seq_app_type()
        << "; engine " << seq_app_engine() << "\n"
        ;

    if (! s_qt_version.empty())
        result << "Qt v. " << s_qt_version << "\n";

    result
        << "Build OS: " << seq_app_build_os() << " " << s_bitness
        << " " << buildmode << "\n"
        ;

#if defined SEQ66_RTMIDI_SUPPORT
    result << "Native JACK/ALSA (rtmidi)\n";
#endif

#if defined SEQ66_PORTMIDI_SUPPORT
    result << "PortMIDI support\n";
#endif

#if defined SEQ66_JACK_SUPPORT
    result
        << "JACK  v. " << s_jack_version << " Transport and MIDI\n"
#if defined SEQ66_JACK_SESSION
        << "JACK Session support\n"
#endif
        ;
#endif

    if (! s_alsa_version.empty())
        result << "ALSA v. " << s_alsa_version << "\n";

#if defined SEQ66_NSM_SUPPORT
    result
        << "NSM (Non Session Manager) support\n"
#endif
        <<
            "Chord generator, LFO, trigger transpose, Tap BPM, Song recording "
            "Pattern coloring, pause, save time-sig/tempo, "
            "event editor, follow-progress.\n"
        <<
            "Options are enabled/disabled via the configure script,"
            " seq66_features.h, or build-specific seq66-config.h files in"
            " include/qt/* for qmake builds."
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


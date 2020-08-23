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
 * \updates       2020-08-23
 * \license       GNU GPLv2 or above
 *
 *  The first part of this file defines a couple of global structure
 *  instances, followed by the global variables that these structures
 *  completely replace.
 */

#include <sstream>                      /* std::ostringstream               */

#include "seq66_features.hpp"           /* feature macros, seq66 namespace  */
#include "app_limits.h"                 /* macros for seq_build_details()   */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Hard-wired replacements for build macros.  Might be modifiable at run-time
 *  in the future.
 */

static std::string s_app_name = SEQ66_APP_NAME;
static std::string s_app_type = SEQ66_APP_TYPE;
static std::string s_app_engine = SEQ66_APP_ENGINE;
static std::string s_app_build_os = SEQ66_APP_BUILD_OS;
static std::string s_client_name = SEQ66_CLIENT_NAME;
static std::string s_version = SEQ66_VERSION;
static std::string s_apptag = SEQ66_APP_NAME " " SEQ66_VERSION;
static std::string s_versiontext = SEQ66_APP_NAME " " SEQ66_GIT_VERSION
    " " SEQ66_VERSION_DATE_SHORT "\n";

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

/**
 *
 */

void
set_app_engine (const std::string & aengine)
{
    s_app_engine = aengine;
}

/**
 *
 */

void
set_app_build_os (const std::string & abuild_os)
{
    s_app_build_os = abuild_os;
}

/**
 *  Sets the current base name of the client port.
 */

void
set_client_name (const std::string & cname)
{
    s_client_name = cname;
}

/**
 *  Returns the name of the application.  We could continue to use the macro
 *  SEQ66_APP_NAME, but we might eventually want to make this name
 *  configurable.  Not too likely, but possible.
 *
 * \return
 *      Returns SEQ66_APP_NAME.
 */

const std::string &
seq_app_name ()
{
    return s_app_name;
}

/**
 *
 */

const std::string &
seq_app_type ()
{
    return s_app_type;
}

/**
 *
 */

const std::string &
seq_app_engine ()
{
    return s_app_engine;
}

/**
 *
 */

const std::string &
seq_app_build_os ()
{
    return s_app_build_os;
}

/**
 *  Returns the name of the client for the application.  We could continue to
 *  use the macro SEQ66_CLIENT_NAME, but we might eventually want to make this
 *  name configurable.  More likely to be a configuration option in the
 *  future.
 *
 * \return
 *      Returns SEQ66_CLIENT_NAME.
 */

const std::string &
seq_client_name ()
{
    return s_client_name;
}

/**
 *  Returns the version of the application.  We could continue to use the macro
 *  SEQ66_VERSION, but ... let's be consistent.  :-D
 *
 * \return
 *      Returns SEQ66_VERSION.
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

/**
 *
 */

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
    result
        << "Built " << __DATE__ << " " << __TIME__ "\n"
        << "  C++ version " << std::to_string(__cplusplus) << "\n"
        << "  App name: " << seq_app_name()
        << "; type " << seq_app_type()
        << "; engine " << seq_app_engine() << "\n"
        << "  Build OS: " << seq_app_build_os() << "\n"
#ifdef SEQ66_RTMIDI_SUPPORT
        << "  Native JACK/ALSA (rtmidi)\n"
#endif
#if defined SEQ66_PORTMIDI_SUPPORT
        << "  PortMIDI\n"
#endif
        << "  Event editor\n"
        << "  Follow progress bar\n"
        << "  Highlight empty patterns\n"
#if defined SEQ66_JACK_SESSION
        << "  JACK session\n"
#endif
#if defined SEQ66_JACK_SUPPORT
        << "  JACK support\n"
#endif
#if defined SEQ66_LASH_SUPPORT
        << "  LASH support\n"
#endif
#if defined SEQ66_NSM_SUPPORT
        << "  NSM (Non Session Manager) support\n"
#endif
        << "  Seq32 chord generator, LFO window, menu buttons, transpose, BPM\n"
        << "  Tap button, solid piano-roll grid, song performance recording\n"
#if defined SEQ66_JE_PATTERN_PANEL_SCROLLBARS
        << "  Main window scroll-bars\n"
#endif
        << "  Pattern coloring\n"
#if defined SEQ66_SONG_BOX_SELECT
        << "  Box song selection\n"
#endif
#if defined SEQ66_PLATFORM_WINDOWS
        << "  Windows support\n"
#endif
        << "  Pause support\n"
        << "  Save time-sig/tempo\n"
#if defined SEQ66_PLATFORM_DEBUG
        << "  Debug code\n"
#endif
        << "  " << s_bitness << "\n\n"
    << "Options are enabled/disabled via the configure script,\n"
    << "libseq66/include/seq66_features.h(pp), or the build-specific\n"
    << "seq66-config.h file in include/ or in include/qt/portmidi/" << std::endl
    ;
    return result.str();
}

}           // namespace seq66

/*
 * seq66_features.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


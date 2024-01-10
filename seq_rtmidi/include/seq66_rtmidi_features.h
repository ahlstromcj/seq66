#ifndef SEQ66_RTMIDI_FEATURES_H
#define SEQ66_RTMIDI_FEATURES_H

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
 * \file          seq66_rtmidi_features.h
 *
 *    This module defines configure and build-time
 *    options available for Seq66's RtMidi-derived implementation.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2016-11-19
 * \updates       2024-01-10
 * \license       GNU GPLv2 or above
 *
 *  For now, this header file enables only the JACK interface.  That is our
 *  main interest, adding native JACK support to Seq66.
 */

#include "seq66_platform_macros.h"
#include "seq66-config.h"

/**
 *  Macros to enable the implementations that are supported under Linux.
 */

#if defined SEQ66_PLATFORM_LINUX || defined SEQ66_PLATFORM_FREEBSD
#define SEQ66_BUILD_UNIX_JACK           /* supported by Linux & FreeBSD     */
#define SEQ66_BUILD_LINUX_ALSA          /* also a wrapper for FreeBSD's OSS */
#define SEQ66_BUILD_RTMIDI_DUMMY        /* an alternative for Linux, etc.   */
#undef  SEQ66_AVOID_TIMESTAMPING        /* a feature of the ALSA rtmidi API */
#endif

#if defined SEQ66_PLATFORM_MACOSX
#define SEQ66_BUILD_MACOSX_CORE
#define SEQ66_BUILD_UNIX_JACK
#define SEQ66_BUILD_RTMIDI_DUMMY        /* an alternative for OSX, etc.     */
#endif

#endif      // SEQ66_RTMIDI_FEATURES_H

/*
 * seq66_rtmidi_features.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


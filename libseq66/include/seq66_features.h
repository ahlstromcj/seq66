#if ! defined SEQ66_FEATURES_H
#define SEQ66_FEATURES_H

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
 * \file          seq66_features.h
 *
 *    This module summarizes or defines all of the configure and build-time
 *    options available for Seq66.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-24
 * \updates       2021-01-12
 * \license       GNU GPLv2 or above
 *
 *    Some options (the "USE_xxx" options) specify experimental and
 *    unimplemented features.  Some options (the "SEQ66_xxx" options)
 *    might be experimental, or not, but are definitely supported, if defined,
 *    and may become configure-time options.
 *
 *    Some options are available (or can be disabled) by running the
 *    "configure" script generated using the configure.ac file.  These
 *    options are things that a normal user or a seq24 aficianado might want to
 *    disable.  They are defined as desired, in the auto-generated
 *    seq66-config.h file in the top-level "include" directory.
 *
 *    The rest of the options can be modified only by editing the source code
 *    (soon to be this file) to enable or disable features.  These options are
 *    those that we feel more strongly about.
 */

#include "seq66-config.h"               /* automake-generated or for qmake  */
#include "seq66_platform_macros.h"      /* indicates the build platform     */

/**
 *  Choose between C++ or bare pthreads.  Affects only the performer class.
 *  For condition_variables and mutexes, we are forced to stick with the
 *  pthreads versions, because the C++ versions can cause deadlock with the GUI
 *  in the current Seq66 processing.  This option is now permanent and needs no
 *  macro.
 *
 *      #define SEQ66_USE_STD_THREADING
 */

/**
 *  A future feature to more flexibly handle a daemonizable build.
 */

#undef SEQ66_DAEMON_SUPPORT

/**
 *  Kepler34 has a drum edit mode that we are still exploring and adding,
 *  marked by the following macro to keep it out of the way until the feature
 *  is ready for prime time.  Currently builds but is incomplete and not
 *  tested.
 */

#undef SEQ66_SEQUENCE_EDIT_MODE

/**
 *  Adds more SYSEX processing, plus the ability to read SYSEX information
 *  from a file.  This needs a fair amount of testing and debugging.
 */

#define SEQ66_USE_SYSEX_PROCESSING

/**
 * Configure-time options.
 *
 *    - SEQ66_HAVE_LIBASOUND
 *    - SEQ66_JACK_SESSION
 *    - SEQ66_JACK_SUPPORT
 *    - SEQ64_LASH_SUPPORT
 *    - SEQ64_NSM_SUPPORT
 */

/*
 * Edit-time (permanent) options.
 */

/**
 *  Not working, removed.  A very tough problem.  The idea
 *  is to go into an auto-screen-set mode via a menu entry, where the current
 *  screen-set is queued for muting, while the next selected screen-set is
 *  queued for unmuting.
 *
 *  #undef  SEQ66_USE_AUTO_SCREENSET_QUEUE
 */

/**
 *  This special value of zoom sets the zoom according to a power of two
 *  related to the PPQN value of the song.
 */

#define SEQ66_USE_ZOOM_POWER_OF_2       0

/*
 * Others
 */

/**
 *  A color option.  We prefer orange for selection boxes.
 */

#undef  SEQ66_USE_BLACK_SELECTION_BOX

/**
 *  An option to make the main-window time-indicator fancier by using alpha
 *  fade, at the expense of CPU cycles.
 */

#undef  SEQ66_USE_METRONOME_FADE

#endif          // SEQ66_FEATURES_H

/*
 * seq66_features.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */


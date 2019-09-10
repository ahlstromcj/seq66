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
 * \updates       2019-08-17
 * \license       GNU GPLv2 or above
 *
 *    Some options (the "USE_xxx" options) specify experimental and
 *    unimplemented features.  Some options (the "SEQ64_xxx" options)
 *    might be experimental, or not, but are definitely supported, if defined,
 *    and may become configure-time options.
 *
 *    Some options are available (or can be disabled) by running the
 *    "configure" script generated using the configure.ac file.  These
 *    options are things that a normal user or a seq24 aficianado might want to
 *    disable.  They are defined as desired, in the auto-generated
 *    seq64-config.h file in the top-level "include" directory.
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
 *  A future feature to more flexibly handle a daemonizable build.  See the
 *  in-progress module main_impl.cpp, still very provisional.
 */

#undef SEQ66_DAEMON_SUPPORT

/**
 *  We need to disable some features not yet available in the Qt 5 user
 *  interface.
 */

#if defined SEQ66_QTMIDI_SUPPORT
#define SEQ66_QT5_USER_INTERFACE
#endif

/**
 *  Kepler34 has a drum edit mode that we are still exploring and adding,
 *  marked by the following macro to keep it out of the way until the feature
 *  is ready for prime time.  Currently builds but is incomplete and not
 *  tested.
 */

#undef SEQ66_SEQUENCE_EDIT_MODE

/**
 *  Kepler34 has a feature for coloring sequence patterns.  However, it
 *  forces every MIDI file to carry 1024 instances of color information.
 *  Compiles, but don't enable until we decide it's worth storing an extra
 *  1024 bytes in the MIDI file.  Instead, see SEQ66_SHOW_COLOR_PALETTE.
 */

#undef USE_KEPLER34_SEQUENCE_COLOR

/**
 *  A better way to implement the Kepler34 sequence-color feature.  Here,
 *  each sequence that has color has an optional SeqSpec for pattern color.
 */

#define SEQ66_SHOW_COLOR_PALETTE

/**
 *  Kepler34 allows the user to select (and move) more than one sequence in
 *  the Song Editor.  Currently builds but is incomplete and not tested.
 */

#define SEQ66_SONG_BOX_SELECT           /* A Qt-only option in Seq66        */

/**
 *  Adds more SYSEX processing, plus the ability to read SYSEX information
 *  from a file.  This needs a fair amount of testing and debugging.
 */

#define SEQ66_USE_SYSEX_PROCESSING

/**
 * Configure-time options.
 *
 *    - SEQ66_HAVE_LIBASOUND
 *    - SEQ66_HIGHLIGHT_EMPTY_SEQS
 *    - SEQ66_JACK_SESSION
 *    - SEQ66_JACK_SUPPORT
 *      Provides support for up to a 3 x 2 array of mainwids.  Now a configure
 *      option.
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
 *  Try to highlight the selected pattern using black-on-cyan
 *  coloring, in addition to the red progress bar marking that already exists.
 *  Moved from seqmenu.  Seems to work pretty well now.
 */

#if defined SEQ66_QTMIDI_SUPPORT
#undef SEQ66_EDIT_SEQUENCE_HIGHLIGHT    /* not ready in Qt 5 support        */
#else
#define SEQ66_EDIT_SEQUENCE_HIGHLIGHT
#endif

/**
 *  This special value of zoom sets the zoom according to a power of two
 *  related to the PPQN value of the song.
 */

#define SEQ66_USE_ZOOM_POWER_OF_2       0

/*
 * Others
 */

/**
 *  A color option.
 */

#define SEQ66_USE_BLACK_SELECTION_BOX

#endif      // SEQ66_FEATURES_H

/*
 * seq66_features.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */


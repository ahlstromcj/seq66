#if ! defined SEQ66_FEATURES_H
#define SEQ66_FEATURES_H

/*
 *  This file is part of seq66.
 *
 *  seq66 is free software; you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option) any later
 *  version.
 *
 *  seq66 is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with seq66; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
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
 * \updates       2025-02-15
 * \license       GNU GPLv2 or above
 *
 *    Some options (the "USE_xxx" options) specify experimental and
 *    unimplemented features.  Some options (the "SEQ66_xxx" options) might be
 *    experimental, or not, but are definitely supported, if defined, and may
 *    become configure-time options.
 *
 *    Some options are available (or can be disabled) by running the
 *    "configure" script generated using the configure.ac file.  These options
 *    are things that a normal user or a seq24 aficianado might want to
 *    disable.  They are defined as desired, in the auto-generated
 *    seq66-config.h file in the top-level "include" directory.
 *
 *    The rest of the options can be modified only by editing the source code
 *    (soon to be this file) to enable or disable features.  These options are
 *    those that we feel more strongly about.
 */

#include "seq66-config.h"               /* automake-generated or for qmake  */
#include "seq66_platform_macros.h"      /* indicates the build platform     */

#if defined SEQ66_PLATFORM_WINDOWS
#if ! defined STDIN_FILENO
#define STDIN_FILENO    0
#define STDOUT_FILENO   1
#define STDERR_FILENO   2
#endif
#endif

/**
 *  In the pattern editor's data pane, we can show the full GM name
 *  of the program-change, instead of just the number. Later we can
 *  expand this to use the note-mapper's patch settings.
 */

#define SEQ66_SHOW_GM_PROGRAM_NAME

/**
 *  Trying to make configuration copying more flexible.  We want to use
 *  additional non-standard file extensions (e.g. ".notemap" versus ".drums"
 *  when iterating through the configuration files, Define this value to
 *  define alternates to copy_configuration() and delete_configuration.
 */

#define SEQ66_KEEP_RC_FILE_LIST

/**
 *  We're testing how to properly invert color palettes.
 */

#undef  SEQ66_PROVIDE_AUTO_COLOR_INVERSION  /* experimental, investigative  */

/**
 *  If defined, a button to show or hide the main menu bar and some
 *  of the layouts is available on the main window.
 */

#define SEQ66_USE_SHOW_HIDE_BUTTON

/**
 *  Make port-mapping the default.
 */

#define SEQ66_USE_DEFAULT_PORT_MAPPING

/**
 *  For issue #100, this macro enables using our new ring_buffer instead of
 *  jack_ringbuffer_t. We no longer attempt to add the timestamp to the JACK
 *  ringbuffer, even if this macro is disabled.  Too many side issues, too
 *  much code, so disabling this macro preserves the old behavior.
 */

#define SEQ66_USE_MIDI_MESSAGE_RINGBUFFER

/**
 *  Choose between C++ or bare pthreads.  Affects only the performer class.
 *  For condition_variables and mutexes, we are forced to stick with the
 *  pthreads versions, because the C++ versions can cause deadlock with the
 *  GUI in the current Seq66 processing.  This option is now permanent and
 *  needs no macro.
 *
 *      #define SEQ66_USE_STD_THREADING
 */

/**
 *  Adds more SYSEX processing, plus the ability to read SYSEX information
 *  from a file.  This needs a fair amount of testing and debugging.
 *
 *  The former is defined in seq66_features.h (included by basic_macros.h),
 *  but what about the latter?  Sequencer64 defines the former!!!
 *
 *  We cannot use this in portmidi.  Code exists to support it, but it is not
 *  called, and but where did we get it??? It's merely noted in the GitHub
 *  portmidi repository, not coded there.
 */

#undef  SEQ66_PORTMIDI_SYSEX_PROCESSING

/**
 * Configure-time options.
 *
 *    - SEQ66_HAVE_LIBASOUND
 *    - SEQ66_JACK_SESSION
 *    - SEQ66_JACK_SUPPORT
 *    - SEQ66_NSM_SUPPORT
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


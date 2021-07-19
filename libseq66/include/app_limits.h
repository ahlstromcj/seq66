#if ! defined SEQ66_APP_LIMITS_H
#define SEQ66_APP_LIMITS_H

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
 * \file          app_limits.h
 *
 *  This module holds macro constants related to the application limits of
 *  Seq66.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-11-08
 * \updates       2021-07-19
 * \license       GNU GPLv2 or above
 *
 *  This collection of macros describes some facets of the
 *  "Patterns Panel" or "Sequences Window", which is visually presented by
 *  the Gtk::Window-derived class called mainwnd (now qsmainwnd in the Qt 5
 *  version of the application.
 *
 *  The Patterns Panel contains an 8-by-4 grid of "pattern boxes" or
 *  "sequence boxes".  All of the patterns in this grid comprise what is
 *  called a "set" (in the musical sense) or a "screen set".
 *
 *  These macros also specify other parameters, as well.
 *
 *  Why do we use macros instead of const values?  First, it really doesn't
 *  matter for simple values.  Second, we want to follow the convention that
 *  important values are all upper-case, as is convention with most
 *  macros.  They just stand out more in code. Call us old school or old
 *  fools, you decide.  Hell, we still like snprintf() for some uses!
 */

/**
 *  Default MIDI control input buss.  This value preserves the old behavior,
 *  where the incoming MIDI events of a device on any buss would be acted on
 *  (if specified in the MIDI control stanzas).  This value is the same as
 *  c_bussbyte_max in the midibytes.hpp module.  It can be changed in the
 *  'ctrl' file.
 */

#define SEQ66_MIDI_CONTROL_IN_BUSS        0xFF

/**
 *  Default MIDI control output buss.  It is used with igorangst's
 *  MIDI-control-out feature at present.  It can be changed in the 'ctrl'
 *  file.
 */

#define SEQ66_MIDI_CONTROL_OUT_BUSS       15

/**
 *  Default value of number of slot toggle keys (shortcut keys) that
 *  can be defined.  Even if we end up adding more slots to a set, this
 *  would be about the maximum number of keys we could really support.
 */

#define SEQ66_SET_KEYS_MAX                32

/**
 *  This constant indicates that a configuration file numeric value is
 *  the default value for specifying that an instrument is a GM
 *  instrument.  Used in the "user" configuration-file processing.
 */

#define SEQ66_GM_INSTRUMENT_FLAG          (-1)

/**
 *  This value indicates to use the default value of PPQN and ignore (to some
 *  extent) what value is specified in the MIDI file.  Note that the default
 *  default PPQN is given by the global ppqn (192) or, if the "--ppqn qn"
 *  option is specified on the command-line or the "midi_ppqn" setting in the
 *  "usr" file.
 *
 *  However, if the "midi_ppqn" setting is 0, then the default PPQN is
 *  whatever the MIDI file specifies.
 */

#define SEQ66_USE_DEFAULT_PPQN            (-1)

/**
 *  Use the PPQN from the loaded file, rather than converting to the active
 *  default PPQN of the application.
 */

#define SEQ66_USE_FILE_PPQN               0

/**
 *  Default value for the global parts-per-quarter-note value.  This is
 *  the unit of time for delta timing.  It represents the units, ticks, or
 *  pulses per beat.  Note that we're migrating this global value into the
 *  user_settings class.  Please leave this value at 192 for all eternity, for
 *  compatibility with other versions of Seq24 or Kepler34.
 */

#define SEQ66_DEFAULT_PPQN               192

/**
 *  Minimum value for PPQN.  Mostly for sanity checking.  This was set to 96,
 *  but there have been tunes set to 32 PPQN, I think.  The minimum value is
 *  32, because the minimum number of pulses in a pixel is
 *  SEQ66_PIXELS_PER_SUBSTEP = 6, and at 24, the number of pulses in a pixel
 *  truncates to 0; bad for dividing!
 */

#define SEQ66_MINIMUM_PPQN                32

/**
 *  Maximum value for PPQN.  Mostly for sanity checking, with higher values
 *  possibly useful for debugging or for marketing claims :-D.
 *  value.  In the webs, people argue that 96 is generally sufficient.
 *  Reaper uses 960.  Finale uses 1024 (not a multiple of 24). Others use 480.
 */

#define SEQ66_MAXIMUM_PPQN              19200

/**
 *  This value represent the smallest horizontal unit in a Sequencer66 grid.
 *  It is the number of pixels in the smallest increment between vertical
 *  lines in the grid.
 */

#define SEQ66_PIXELS_PER_SUBSTEP           6

/**
 *  Minimum possible value for zoom, indicating that one pixel represents one
 *  tick.
 */

#define SEQ66_MINIMUM_ZOOM                 1

/**
 *  The default value of the zoom, indicating that one pixel represents two
 *  ticks.  However, it turns out we're going to have to support adapting the
 *  default zoom to the PPQN, in addition to allowing some extra zoom values.
 */

#define SEQ66_DEFAULT_ZOOM                 2

/**
 *  The maximum value of the zoom, indicating that one pixel represents 512
 *  ticks.  The old maximum was 32, but now that we support PPQN up to 19200,
 *  we need a couple of extra entries.
 */

#define SEQ66_MAXIMUM_ZOOM               512

/**
 *  The default value of the snap in the sequence editors.
 */

#define SEQ66_DEFAULT_SNAP                16

/**
 *  Defines the callback rate for gtk_timeout_add() as used by perfedit.
 *  As usual, this value is in milliseconds.
 */

#define SEQ66_FF_RW_TIMEOUT              120

/**
 *  Default value for major divisions per bar.  A graphics version of
 *  SEQ66_DEFAULT_BEATS_PER_MEASURE.
 */

#define SEQ66_DEFAULT_LINES_PER_MEASURE     4

/**
 *  Default value for perfedit snap.
 */

#define SEQ66_DEFAULT_PERFEDIT_SNAP         8

/**
 *  Provides a sanity check for transposition values.
 */

#define SEQ66_TRANSPOSE_UP_LIMIT        (128 / 2)
#define SEQ66_TRANSPOSE_DOWN_LIMIT      (-128 / 2)

/*
 *  This section converts the macros to values.
 */

#endif      // SEQ66_APP_LIMITS_H

/*
 * app_limits.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


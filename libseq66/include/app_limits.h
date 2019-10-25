#if ! defined SEQ66_APP_LIMITS_H
#define SEQ66_APP_LIMITS_H

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
 * \file          app_limits.h
 *
 *  This module holds macro constants related to the application limits of
 *  Seq66.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-11-08
 * \updates       2019-10-16
 * \license       GNU GPLv2 or above
 *
 *  This collection of macros describes some facets of the
 *  "Patterns Panel" or "Sequences Window", which is visually presented by
 *  the Gtk::Window-derived class called mainwnd (now qsmainwnd in the Qt 5
 *  version of the application..
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
 *  The maximum number of patterns supported is given by the number of
 *  patterns supported in the panel (32) times the maximum number of sets
 *  (32), or 1024 patterns.  However, this value is now independent of the
 *  maximum number of sets and the number of sequences in a set.  Instead,
 *  we limit them to a constant value, which seems to be well above the
 *  number of simultaneous playing sequences the application can support.
 *  See SEQ66_SEQUENCE_MAXIMUM.
 */

#define SEQ66_SEQUENCE_MAXIMUM          1024

/**
 *  Indicates the maximum number of MIDI channels, counted internally from 0
 *  to 15, and by humans (sequencer user-interfaces) from 1 to 16.
 */

#define SEQ66_MIDI_CHANNEL_MAX            16

/**
 *  The canonical set size.  Used in relation to the keystrokes used to access
 *  sequences (and mute-groups).
 */

#define SEQ66_BASE_SET_SIZE               32

/**
 *  Minimum value for c_max_sets.  The actual maximum number of sets will be
 *  reduced if we add rows (or columns) to each mainwid grid.  This is
 *  actually a derived value, but we still support a macro for it.
 */

#define SEQ66_MIN_SET_MAX                 16

/**
 *  Default value for c_max_sets.  The actual maximum number of sets will be
 *  reduced if we add rows (or columns) to each mainwid grid.  This is actually
 *  a derived value, but we still support a macro for it.
 */

#define SEQ66_DEFAULT_SET_MAX             32

/**
 *  Default set size.  This may need to be adjusted.  It is used with
 *  igorangst's MIDI-control-out feature at present.
 */

#define SEQ66_DEFAULT_SET_SIZE            32

/**
 *  Default MIDI control output buss.  It is used with igorangst's
 *  MIDI-control-out feature at present.
 */

#define SEQ66_MIDI_CONTROL_OUT_BUSS       15

/**
 *  Maximum value for c_max_sets.
 *  Note that the largest number of sets is 4 x 8 = 32.  This limitation is
 *  necessary because there are only so many available keys on the keyboard for
 *  pattern, mute-group, and set control.
 */

#define SEQ66_MAX_SET_MAX                 32

/**
 *  Default number of rows in the main-window's grid.  This value applies to the
 *  layout of the pattern and mute-group keystrokes, as well as the virtual
 *  layout of sets into rows and columns.
 */

#define SEQ66_DEFAULT_SET_ROWS             4

/**
 *  Minimum number of rows in the main-window's grid.  This will remain
 *  the same as the default number of rows; we will not reduce the number of
 *  sequences per set, at least at this time.
 */

#define SEQ66_MIN_SET_ROWS                 4

/**
 *  Maximum number of rows in the main-window's grid.  With the default number
 *  of columns, this will double the number of sequences per set from 32 to
 *  64, hence the name "seq66".
 */

#define SEQ66_MAX_SET_ROWS                12

/**
 *  Default number of columns in the main-window's grid.
 */

#define SEQ66_DEFAULT_SET_COLUMNS      8

/**
 *  Minimum number of columns in the main-window's grid.  Currently the same
 *  as the default number.  We currently cannot support more sets than 32,
 *  which would happen if we let rows or columns go below the default 4 x 8
 *  settings.
 */

#define SEQ66_MIN_SET_COLUMNS              4

/**
 *  Maximum number of columns in the main-window's grid.  Currently the same
 *  as the default number.
 */

#define SEQ66_MAX_SET_COLUMNS             12   // 8

/**
 *  Default value for c_seqs_in_set.
 */

#define SEQ66_DEFAULT_SEQS_IN_SET \
    (SEQ66_DEFAULT_SET_ROWS * SEQ66_DEFAULT_SET_COLUMNS)

/**
 *  Default value for c_max_groups.  This value replaces c_seqs_in_set for
 *  usage in obtaining mute-group information from the "rc" file.  Its value
 *  is only "coincidentally" equal to 32.  However, given that we have limited
 *  numbers of mute-group keys, this will be the highest value that could be
 *  used.
 */

#define SEQ66_DEFAULT_GROUP_MAX           32

/**
 *  Default value of number of slot toggle keys (shortcut keys) that
 *  can be defined.  Even if we end up adding more slots to a set, this
 *  would be about the maximum number of keys we could really support.
 */

#define SEQ66_SET_KEYS_MAX                32

/**
 *  Default value of the height of the piano keys in the Qt 5 qseqkeys
 *  user-interface.  Also configurable in the "usr" file.  No limit-checking
 *  right now, but keep the values from 8 to 16 for sanity's sake.
 */

#define SEQ66_SEQKEY_HEIGHT               10

/**
 *  Default value of the width (number of columns) of the slot toggle keys.
 *  Again, this matches with number of columns in a set in the main window of
 *  the application.
 */

#define SEQ66_SET_KEYS_COLUMNS             8

/**
 *  No global buss override is in force if the global buss override number is
 *  this value (255).
 */

#if defined __cplusplus
#define SEQ66_BAD_BUSS                    (midibyte(255))
#else
#define SEQ66_BAD_BUSS                    ((midibyte)(255))
#endif

/**
 *  An easier macro for testing SEQ66_BAD_BUSS.
 */

#define SEQ66_NO_BUSS_OVERRIDE(b)         (midibyte(b) == SEQ66_BAD_BUSS)

/**
 *  Default value for c_max_busses.
 */

#define SEQ66_DEFAULT_BUSS_MAX            32

/**
 *  The number of ALSA busses supported.  See mastermidibus::init().
 */

#define SEQ66_ALSA_OUTPUT_BUSS_MAX        16

/**
 *  Flags an unspecified buss number.  Two spellings are provided, one for
 *  youngsters and one for old men.  :-D
 */

#define SEQ66_NO_BUS                    (-1)
#define SEQ66_NO_BUSS                   (-1)
#define SEQ66_BAD_BUS_ID                (unsigned(-1))

/**
 *  Flags an unspecified port number, or indicates a bad client ID or port
 *  number.
 */

#define SEQ66_NO_PORT                   (-1)
#define SEQ66_BAD_PORT_ID               (unsigned(-1))

/**
 *  Flags an unspecified queue number.
 */

#define SEQ66_NO_QUEUE                  (-1)
#define SEQ66_BAD_QUEUE_ID              (unsigned(-1))

/**
 *  This value is used to indicated that the queued-replace (queued-solo)
 *  feature is reset and not in force.
 */

#define SEQ66_NO_QUEUED_SOLO            (-1)

/**
 *  This value indicates that there is no mute-group selected.
 */

#define SEQ66_NO_MUTE_GROUP_SELECTED    (-1)

/**
 *  Guessing that this has to do with the width of the performerance piano roll.
 *  See perfroll::init_before_show().
 */

#define SEQ66_PERFROLL_PAGE_FACTOR      4096

/**
 *  Guessing that this describes the number of subdivisions of the grid in a
 *  beat on the perfroll user-interace.  Changing this doesn't change anything
 *  obvious in the user-interface, though.
 */

#define SEQ66_PERFROLL_DIVS_PER_BEAT      16

/**
 *  The maximum number of rows of mainwids we will support, regardless of
 *  screen resolution.
 */

#define SEQ66_MAINWID_BLOCK_ROWS_MAX       3

/**
 *  The maximum number of columns of mainwids we will support, regardless of
 *  screen resolution.
 */

#define SEQ66_MAINWID_BLOCK_COLS_MAX       2

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
 *  but there have been tunes set to 32 PPQN, I think.  At this setting,
 *  the Gtkmm-2.4 maintime bar moves pretty jerkily.
 */

#define SEQ66_MINIMUM_PPQN                32

/**
 *  Maximum value for PPQN.  Mostly for sanity checking, with higher values
 *  possibly useful for debugging.
 */

#define SEQ66_MAXIMUM_PPQN             19200

/**
 *  This value represent the smallest horizontal unit in a Sequencer66 grid.  It
 *  is the number of pixels in the smallest increment between vertical lines in
 *  the grid.
 */

#define SEQ66_PIXELS_PER_SUBSTEP            6

/**
 *  Minimum possible value for zoom, indicating that one pixel represents one
 *  tick.
 */

#define SEQ66_MINIMUM_ZOOM                  1

/**
 *  The default value of the zoom, indicating that one pixel represents two
 *  ticks.  However, it turns out we're going to have to support adapting the
 *  default zoom to the PPQN, in addition to allowing some extra zoom values.
 */

#define SEQ66_DEFAULT_ZOOM                  2

/**
 *  The maximum value of the zoom, indicating that one pixel represents 512
 *  ticks.  The old maximum was 32, but now that we support PPQN up to 19200,
 *  we need a couple of extra entries.
 */

#define SEQ66_MAXIMUM_ZOOM                512

/**
 *  The default value of the snap in the sequence editors.
 */

#define SEQ66_DEFAULT_SNAP                 16

/**
 *  Minimum possible value for the global redraw rate.
 */

#define SEQ66_MINIMUM_REDRAW               10

/**
 *  The default value global redraw rate.
 */

#define SEQ66_DEFAULT_REDRAW               40     /* or 25 for Windows */

/**
 *  The maximum value for the global redraw rate.
 */

#define SEQ66_MAXIMUM_REDRAW              100

/**
 *  Defines the callback rate for gtk_timeout_add() as used by perfedit.
 *  As usual, this value is in milliseconds.
 */

#define SEQ66_FF_RW_TIMEOUT               120

/**
 *  Defines a scale value for BPM so that we can store a higher-precision
 *  version of it in the proprietary "bpm" section.  See the midifile class
 *  for more information.
 */

#define SEQ66_BPM_SCALE_FACTOR          1000.0

/**
 *  The amount of time to wait for inaction before clearing the tap-button
 *  values, in milliseconds.
 */

#define SEQ66_TAP_BUTTON_TIMEOUT         5000L

/**
 *  Default value for c_beats_per_minute (global beats-per-minute, also known
 *  as "BPM").  Do not confuse this "bpm" with the other one, "beats per
 *  measure".
 */

#define SEQ66_DEFAULT_BPM                120.0

/**
 *  Minimum value for c_beats_per_minute (global beats-per-minute, also known
 *  as "BPM").  Mostly for sanity-checking, with extra low values allowed for
 *  debugging and troubleshooting.
 */

#define SEQ66_MINIMUM_BPM                  1.0       /* 20   */

/**
 *  Maximum value for c_beats_per_minute (global beats-per-minute, also known
 *  as "BPM").  Mostly for sanity-checking.
 */

#define SEQ66_MAXIMUM_BPM                600.0       /* 500  */

/**
 *  Provides a fallback value for the BPM precision.  This is the "legacy"
 *  value.
 */

#define SEQ66_DEFAULT_BPM_PRECISION        0

/**
 *  Provides a minimum value for the BPM precision.  That is, no decimal
 *  point.
 */

#define SEQ66_MINIMUM_BPM_PRECISION        0

/**
 *  Provides a maximum value for the BPM precision, two decimal points.
 */

#define SEQ66_MAXIMUM_BPM_PRECISION        2

/**
 *  Provides a fallback value for the BPM increment.  This is the "legacy"
 *  value.
 */

#define SEQ66_DEFAULT_BPM_INCREMENT        1.0

/**
 *  Provides a minimum value for the BPM increment.
 */

#define SEQ66_MINIMUM_BPM_INCREMENT        0.01

/**
 *  Provides a maximum value for the BPM increment.
 */

#define SEQ66_MAXIMUM_BPM_INCREMENT       25.0

/**
 *  Provides a fallback value for the BPM step increment.  This is the "legacy"
 *  value.
 */

#define SEQ66_DEFAULT_BPM_STEP_INCREMENT   1.0

/**
 *  Provides a fallback value for the BPM page increment.
 */

#define SEQ66_DEFAULT_BPM_PAGE_INCREMENT  10.0

/**
 *  Minimum value for "beats-per-measure".  A new addition for the Qt 5
 *  user-interface.
 */

#define SEQ66_MINIMUM_BEATS_PER_MEASURE    1

/**
 *  Default value for "beats-per-measure".  This is the "numerator" in a 4/4
 *  time signature.  It also seems to be the value used for JACK's
 *  jack_position_t.beats_per_bar field.  For abbreviation, we will call this
 *  value "BPB", or "beats per bar", to distinguish it from "BPM", or "beats
 *  per minute".
 */

#define SEQ66_DEFAULT_BEATS_PER_MEASURE    4

/**
 *  Maximum value for "beats-per-measure".  A new addition for the Qt 5
 *  user-interface.
 */

#define SEQ66_MAXIMUM_BEATS_PER_MEASURE   16

/**
 *  Minimum number of measures in the official length of a pattern.
 */

#define SEQ66_MINIMUM_MEASURES             1

/**
 *  Default number of measures in the official length of a pattern.
 */

#define SEQ66_DEFAULT_MEASURES             1

/**
 *  Maximum number of measures in the official length of a pattern.
 */

#define SEQ66_MAXIMUM_MEASURES          1024

/**
 *  The minimum value of the beat width.  A new addition for the Qt 5
 *  user-interface.
 */

#define SEQ4_MINIMUM_BEAT_WIDTH             1

/**
 *  Default value for "beat-width".  This is the "denominator" in a 4/4 time
 *  signature.  It also seems to be the value used for JACK's
 *  jack_position_t.beat_type field. For abbreviation, we will call this value
 *  "BW", or "beat width", not to be confused with "bandwidth".
 */

#define SEQ66_DEFAULT_BEAT_WIDTH            4

/**
 *  The maximum value of the beat width.  A new addition for the Qt 5
 *  user-interface.
 */

#define SEQ4_MAXIMUM_BEAT_WIDTH            16

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
 *  Default value for c_thread_trigger_width_ms.
 */

#define SEQ66_DEFAULT_TRIGWIDTH_MS          4

/*
 *  Default value for c_thread_trigger_width_ms.  Not in use at present.
 *
 *  #define SEQ66_DEFAULT_TRIGLOOK_MS         2
 */

/**
 *  Defines the maximum number of MIDI values, and one more than the
 *  highest MIDI value, which is 17.
 */

#define SEQ66_MIDI_COUNT_MAX              128

/**
 *  Defines the minimum Note On velocity.
 */

#define SEQ66_MIN_NOTE_ON_VELOCITY          0

/**
 *  Defines the default Note On velocity, a new "stazed" feature.
 */

#define SEQ66_DEFAULT_NOTE_ON_VELOCITY    100

/**
 *  Defines the maximum Note On velocity.
 */

#define SEQ66_MAX_NOTE_ON_VELOCITY        127

/**
 *  Indicates to preserve the velocity of incoming MIDI Note events, for both
 *  on or off events.  This value represents the "Free" popup-menu entry for
 *  the "Vol" button in the seqedit window.
 */

#define SEQ66_PRESERVE_VELOCITY         (-1)

/**
 *  An older value, previously used for both Note On and Note Off velocity.
 *  See the "Stazed" note in the sequence::add_note() function.
 */

#define SEQ66_DEFAULT_NOTE_VELOCITY      100

/**
 *  Defines the default Note Off velocity, a new "stazed" feature.
 */

#define SEQ66_DEFAULT_NOTE_OFF_VELOCITY   64

/**
 *  Defines the maximum number of notes playing at one time that the
 *  application will support.
 */

#define SEQ66_MIDI_NOTES_MAX             256

/**
 *  Provides a sanity check for transposition values.
 */

#define SEQ66_TRANSPOSE_UP_LIMIT        (SEQ66_MIDI_COUNT_MAX / 2)
#define SEQ66_TRANSPOSE_DOWN_LIMIT      (-SEQ66_MIDI_COUNT_MAX / 2)

/**
 *  Indicates the maximum number of recently-opened MIDI file-names we will
 *  store.
 */

#define SEQ66_RECENT_FILES_MAX          10

#endif      // SEQ66_APP_LIMITS_H

/*
 * app_limits.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


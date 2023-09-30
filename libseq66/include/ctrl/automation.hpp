#if ! defined SEQ66_AUTOMATION_HPP
#define SEQ66_AUTOMATION_HPP

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
 * \file          automation.hpp
 *
 *  This module declares/defines the namespace for some enumeration classes
 *  used to specify control categories and actions.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-18
 * \updates       2023-09-19
 * \license       GNU GPLv2 or above
 *
 *  This module defines a number of constants relating to control of pattern
 *  unmuting, group control, and a number of additional controls to make
 *  Seq66 controllable without a graphical user interface.  See the cpp file
 *  for additional information.  This module requires C++11 and above.
 */

#include <string>

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

namespace automation
{

/**
 *  Provides the number of sub-stanzas in a midicontrol stanza in the
 *  "rc"/"ctrl" file.  The 3 sections are the valid values in the
 *  automation::action enumeration: toggle, on, and off.
 */

static const int ACTCOUNT = 3;

/**
 *  Manifest constants for midicontrolfile to use as array indices.
 *  These correspond to the MIDI Controls for UI (user-interface) actions;
 *  see the uiactions enumeration. This enumeration cannot be a class
 *  enumeration, because enum classes cannot be used as array indices.
 */

enum index
{
    inverse,
    status,
    data_1,
    data_2_min,
    data_2_max,
    max
};

/**
 *  Provides the number of values in a midicontrol sub-stanza.  Recall
 *  that one sub-stanza is represented by a <code> [ 0 0 0 0 0 ] </code>
 *  item in the 'ctrl' file.  We have removed the "enabled" value as
 *  redundant, and reduced the count to 5.
 */

static const int SUBCOUNT = int(index::max);

/**
 *  Provides enumerations for the main control sections.  The pattern,
 *  mute-group, and automation values are selected when the "rc" file is read,
 *  based on the name of the sections in which control values were read:
 *
 *      -   [loop-control]
 *      -   [mute-group-control]
 *      -   [automation-control]
 *
 *  In Sequencer64, keyboard control was set up in a [keyboard-control]
 *  section, and MIDI control was set up separately in a [midi-control]
 *  section. In Sequencer66, keyboard and MIDI controls are set up in the same
 *  sections, the three sections noted above.
 */

enum class category
{
    none,           /**< Not used, except to indicate "not initialized.     */
    loop,           /**< [loop-control], mutes/unmutes "Loops".             */
    mute_group,     /**< [mute-group-control], specifies multiple mutings.  */
    automation,     /**< [automation-control], GUI control automation.      */
    max             /**< Not used, except to check for illegal settings.    */
};

/**
 *  Provides the kind of MIDI control event found, used in the new
 *  perform::handle_midicontrol_ex() function.
 *
 * \var none
 *      -   Indicates that the control is not active (temporarily).
 *
 * \var toggle
 *      -   Normally, toggles the status of the given control.
 *      -   For the "playback" status, indicates the "pause" functionality.
 *      -   For the "playlist" and "playlist-song" status, indicates the
 *          "select-by-value" functionality.
 *
 * \var on
 *      -   Normally, turns on the status of the given control.
 *      -   For the "playback" status, indicates the "start" functionality.
 *      -   For the "playlist" and "playlist-song" status, indicates the
 *          "select-next" functionality.
 *
 * \var off
 *      -   Normally, turns off the status of the given control.
 *      -   For the "playback" status, indicates the "stop" functionality.
 *      -   For the "playlist" and "playlist-song" status, indicates the
 *          "select-previous" functionality.
 *
 * \var max
 *      -   Simply a limit number.
 */

enum class action
{
    none,
    toggle,
    on,
    off,
    max
};

/**
 *  Pseudo control values for associating MIDI events, for the automation of
 *  some of the controls in seq66. Unlike the earlier version, this version is
 *  not necessarily tied to the 32-pattern paradigm.
 *
 *  Each slot value is tied to a particular performer member function.  Each
 *  slot accesses the performer member function via a lambda loaded into a
 *  map.  Faster lookup than traversing a bunch of if-statements, it is to be
 *  hoped.
 *
 *  The controls are read in from the 'ctrl' configuration files, but are no
 *  longer written to the c_midictrl section of the "proprietary" final track
 *  in a Seq66 MIDI file.  The controls represented by slot values are part of
 *  the automation (user-interface) section of the 'ctrl' file.
 *
 *  Unlike the original controls, all of the control groups (pattern,
 *  mute-group, and automation) all support a number of controls not
 *  necessarily equal to 32.  Also, up/down controls have been folded into one
 *  control.  We need to be able to convert between old and new "control"
 *  numbers.
 *
 *  See opcontrol::slot_name() to get the display name of each slot.
 *
 * Notes:
 *
 *      -#  Replace, queue, and one-shot can be combined in an operation.
 *      -#  For loop-control and mute-group control, the slot is the
 *          pattern or group number, which redirect calls to the pattern
 *          and mute_group slot functions.  For automation-control, the
 *          slot numbers are in one-to-one correspondence with slot
 *          functions (also known as "operations"),
 *      -#  WARNING:  If one updates this list, one MUST also update the
 *          static opcontrol::s_slot_names vector to match!
 */

enum class slot
{
    none = -1,          /**< An out-of-range value, uninitialized.          */
    bpm_up = 0,         /**< 0: BPM up; for MIDI up and down.               */
    bpm_dn,             /**< 1: BPM down; for MIDI down and up.             */
    ss_up,              /**< 2: Screen-set (bank) up. And down for MIDI.    */
    ss_dn,              /**< 3: Screen-set (bank) down.                     */
    mod_replace,        /**< 4: Set status of replace control.              */
    mod_snapshot,       /**< 5: Set status of snapshot control.             */
    mod_queue,          /**< 6: Set status of queue control; group_on, _off */
    mod_gmute,          /**< 7: Set status of group-mute control.           */
    mod_glearn,         /**< 8: Set status of group-learn control.          */
    play_ss,            /**< 9: Sets the playing screen-set (bank).         */
    playback,           /**< 10: Key pause, and MIDI for pause/start/stop.  */
    song_record,        /**< 11: Sets recording of a live song performance. */
    solo,   /* grid? */ /**< 12: TODO, intended to solo track.              */
    thru,   /* grid? */ /**< 13: Enables/disables the MIDI THRU control.    */
    bpm_page_up,        /**< 14: Increments BMP by a configured page value. */
    bpm_page_dn,        /**< 15: Decrements BMP by a configured page value. */
    ss_set,             /**< 16: Key: set screen-set; MIDI: playing set.    */
    record_style,       /**< 17: Moves between recording styles like merge. */
    quan_record,        /**< 18: Moves to next/previous quantize type.      */
    reset_sets,         /**< 19: Resets all patterns/playing set.           */
    mod_oneshot,        /**< 20: Set status of one-shot queuing.            */
    FF,                 /**< 21: Fast-forwards the clock (pulse counter.)   */
    rewind,             /**< 22: Rewinds the clock (pulse counter).         */
    top,                /**< 23: Set to song beginning or L marker.         */
    playlist,           /**< 24: MIDI only, arrow keys hardwired.           */
    playlist_song,      /**< 25: MIDI only, arrow keys hardwired.           */
    tap_bpm,            /**< 26: Tap key for estimating BPM.                */
    start,              /**< 27: Start playback. Compare to playback above. */
    stop,               /**< 28: Stop playback. Compare to playback above.  */
    loop_LR,            /**< 29: Toggle looping between L/R markers.        */
    toggle_mutes,       /**< 30: Song mute, unmute, and toggle?             */
    song_pointer,       /**< 31: Reposition the song pointer.  TODO.        */

    /*
     * The following add to what Seq64 supports.
     */

    keep_queue,         /**< 32: Set keep-queue (the "Q" button).           */
    slot_shift,         /**< 33: Used for sets > 32 patterns.               */
    mutes_clear,        /**< 34: Set all mute groups to unarmed.            */
    quit,               /**< 35: Quit (close and exit) the application.     */
    pattern_edit,       /**< 36: GUI action, bring up pattern for editing.  */
    event_edit,         /**< 37: GUI action, bring up the event editor.     */
    song_mode,          /**< 38: GUI. Toggle between Song Mode & Live Mode. */
    toggle_jack,        /**< 39: GUI. Toggle between JACK and ALSA support. */
    menu_mode,          /**< 40: GUI. Switch menu between enabled/disabled. */
    follow_transport,   /**< 41: GUI. Toggle between following JACK or not. */
    panic,              /**< 42: The Panic Button.                          */
    visibility,         /**< 43: Toggle the visibility of the main window.  */
    save_session,       /**< 44: Save the MIDI and configuration files now. */
    record_toggle,      /**< 45: Enter toggle-record for next hot-key.      */
    grid_mutes,         /**< 46: Grid mode extension :-( for reserved_46    */
    reserved_47,        /**< 47: Reserved for expansion.                    */
    reserved_48,        /**< 48: Reserved for expansion.                    */

    /*
     * Massive expansion in automation. Record mode selection.
     */

    record_overdub,     /**< 49: Select overdub/merge recording triggering. */
    record_overwrite,   /**< 50: Select overdub recording triggering.       */
    record_expand,      /**< 51: Select expand recording triggering.        */
    record_oneshot,     /**< 52: Select oneshot recording triggering.       */

    /*
     * Massive expansion in automation. Grid mode selection.
     */

    grid_loop,          /**< 53: Normal operation of the main grid.         */
    grid_record,        /**< 54: Use one of the record modes for slots.     */
    grid_copy,          /**< 55: Grid slot copies the pattern.              */
    grid_paste,         /**< 56: Grid slot pastes to the pattern.           */
    grid_clear,         /**< 57: Grid slot clears only events.              */
    grid_delete,        /**< 58: Grid slot deletes (removes) the pattern.   */
    grid_thru,          /**< 59: Grid slot turns on MIDI thru.              */
    grid_solo,          /**< 60: Grid slot turns on solo.                   */
    grid_cut,           /**< 61: Grid slot cuts the pattern.                */
    grid_double,        /**< 62: Grid slot doubles the pattern length.      */

    /*
     * Grid quantization type selection.
     */

    grid_quant_none,    /**< 63: Grid slot remove recording quantization.   */
    grid_quant_full,    /**< 64: Grid slot full quantization recording.     */
    grid_quant_tighten, /**< 65: Grid slot tighten quantization recording.  */
    grid_quant_random,  /**< 66: Grid slot salts the magnitude randomly.    */
    grid_quant_jitter,  /**< 67: Grid slot jitter the timing.               */
    grid_quant_notemap, /**< 68: Reserved for expansion (e.g. note-mapping) */

    /*
     * A few more likely candidates.
     * NOT YET IMPLEMENTED.
     */

    mod_bbt_hms,        /**< 69: Toggle between time-display modes.         */
    mod_LR_loop,        /**< 70: Toggle looping between the L and R marks.  */
    mod_undo,           /**< 71: Undo events in current active pattern. ??? */
    mod_redo,           /**< 72: Redo events in current active pattern. ??? */
    mod_transpose_song, /**< 73: Apply song transpose. ??????               */
    mod_copy_set,       /**< 74: Copy the current playing set.              */
    mod_paste_set,      /**< 75: Paste into the current active set.         */
    mod_toggle_tracks,  /**< 76: Toggle the armed status of the active set. */

    /*
     * Set playing modes.
     * NOT YET IMPLEMENTED.
     */

    set_mode_normal,    /**< 77: A set selection replaces the playing set.  */
    set_mode_auto,      /**< 78: Set selection starts the new set playing.  */
    set_mode_additive,  /**< 79: Set selection adds the new set to playing. */
    set_mode_all_sets,  /**< 80: All sets play at the same time.            */

    /*
     * Tricky ending.
     */

    max,                /**< 81: Used only for termination/range-checking.  */

    /*
     * The following are used for selection the correct op function.  Pattern
     * and mute groups each need only one function (with an integer
     * parameter), while automation uses the codes above to select the proper
     * op function from a rather large set of them.
     */

    loop,               /**< Useful to set and retrieve op function.        */
    mute_group,         /**< Useful to set and retrieve op function.        */
    automation,         /**< Useful to set and retrieve the name.           */
    illegal             /**< A value to flag illegality.                    */
};

/**
 *  Provides the status bits that used to be in the old perform class.
 *  The values are explained in the top banner of this module.  They
 *  replace the free constants starting with "c_status_" in the old
 *  project (Seq64).  Do not confuse it with MIDI status, which is a value
 *  specifying a MIDI event.
 *
 *  These were purely internal constants used with the functions that
 *  implement MIDI control (and also some keystroke control) for the
 *  application.  However, we now have to expose them for the Qt5
 *  implementation, until we can entirely reconcile/refactor the
 *  Kepler34-based body of code.  Note how they specify different bit values,
 *  as it they could be masked together to signal multiple functions.
 *
 *  This value signals the "replace" functionality.  If this bit is set, then
 *  perform::sequence_playing_toggle() unsets this status and calls
 *  perform::off_sequences(), which calls sequence::set_playing(false) for all
 *  active sequences.
 *
 *  It works like this:
 *
 *      -#  The user presses the Replace key, or the MIDI control message for
 *          c_midi_control_mod_replace is received.
 *      -#  This bit is OR'd into perform::m_control_status.  This status bit
 *          is used in perform::sequence_playing_toggle().
 *          -   Called in perform::sequence_key() so that keystrokes in
 *              the main window toggle patterns in the main window.
 *          -   Called in peform::toggle_other_seqs() to implement
 *              Shift-click to toggle all other patterns but the one
 *              clicked.
 *          -   Called in seqmenu::toggle_current_sequence(), called in
 *              mainwnd to implement clicking on a pattern.
 *          -   Also used in MIDI control to toggle patterns 0 to 31,
 *              offset by the screen-set.
 *          -   perform::sequence_playing_off(), similarly used in MIDI control.
 *          -   perform::sequence_playing_on(), similarly used in MIDI control.
 *      -#  When the key is released, this bit is AND'd out of
 *          perform::m_control_status.
 *
 *      Both the MIDI control and the keystroke set the sequence to be
 *      "replaced".
 */

enum class ctrlstatus
{
    /**
     *  The default, non-functional value.
     */

    none        = 0x00,

    /**
     *  This value signals the "snapshot" functionality.  By default,
     *  perform::sequence_playing_toggle() calls sequence::toggle_playing() on
     *  the given sequence number, plus what is noted for c_status_snapshot.  It
     *  works like this:
     *
     *      -#  The user presses the Snapshot key.
     *      -#  This bit is OR'd into perform::m_control_status.
     *      -#  The playing state of the patterns is saved by
     *          perform::save_playing_state().
     *      -#  When the key is released, this bit is AND'd out of
     *          perform::m_control_status.
     *      -#  The playing state of the patterns is restored by
     *          perform::restore_playing_state().
     */

    replace     = 0x01,

    /**
     *  This value signals the "snapshot" functionality.  By default,
     *  perform::sequence_playing_toggle() calls sequence::toggle_playing() on
     *  the given sequence number, plus what is noted for c_status_snapshot.
     *  It works like this:
     *
     *      -#  The user presses the Snapshot key.
     *      -#  This bit is OR'd into perform::m_control_status.
     *      -#  The playing state of the patterns is saved by
     *          perform::save_playing_state().
     *      -#  When the key is released, this bit is AND'd out of
     *          perform::m_control_status.
     *      -#  The playing state of the patterns is restored by
     *          perform::restore_playing_state().
     */

    snapshot    = 0x02,

    /**
     *  This value signals the "queue" functionality.  If this bit is set,
     *  then perform::sequence_playing_toggle() calls sequence ::
     *  toggle_queued() on the given sequence number.  The regular queue key
     *  (configurable in File / Options / Keyboard) sets this bit when
     *  pressed, and unsets it when released.  The keep-queue key sets it, but
     *  it is not unset until the regular queue key is pressed and released.
     */

    queue       = 0x04,

    /**
     *  Performs keep-queue.  Currently queue and keep-queue are both keep
     *  functions.
     */

    keep_queue  = 0x08,

    /**
     *  This value signals the Kepler34 "one-shot" functionality.  If this bit
     *  is set, then perform::sequence_playing_toggle() calls
     *  sequence::toggle_oneshot() on the given sequence number.
     */

    oneshot     = 0x10,

    /**
     *  Signals that we are in mute-group learn mode.  This will eventually
     *  supplement the mutegroups "learn" flag, as we want to centralize all
     *  mode statuses.
     */

    learn       = 0x20
};

/**
 * "Bit" operators for the ctrlstatus values.
 */

inline ctrlstatus
operator | (ctrlstatus lhs, ctrlstatus rhs)
{
    return static_cast<ctrlstatus>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

inline ctrlstatus &
operator |= (ctrlstatus & lhs, ctrlstatus rhs)
{
    lhs = static_cast<ctrlstatus>(static_cast<int>(lhs) | static_cast<int>(rhs));
    return lhs;
}

inline ctrlstatus
operator & (ctrlstatus lhs, ctrlstatus rhs)
{
    return static_cast<ctrlstatus>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

inline ctrlstatus &
operator &= (ctrlstatus & lhs, ctrlstatus rhs)
{
    lhs = static_cast<ctrlstatus>(static_cast<int>(lhs) & static_cast<int>(rhs));
    return lhs;
}

inline ctrlstatus
operator ^ (ctrlstatus lhs, ctrlstatus rhs)
{
    return static_cast<ctrlstatus>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

inline ctrlstatus &
operator ^= (ctrlstatus & lhs, ctrlstatus rhs)
{
    lhs = static_cast<ctrlstatus>(static_cast<int>(lhs) ^ static_cast<int>(rhs));
    return lhs;
}

inline ctrlstatus
operator ~ (ctrlstatus rhs)
{
    return static_cast<ctrlstatus>(~ static_cast<int>(rhs));
}

/**
 *  For complex statuses (more than one bit set), this function returns true
 *  if there is any "on" bit in either of the two statuses.  More useful if
 *  there is only one bit, so stick to that use-case.
 */

inline bool
bit_test_or (ctrlstatus lhs, ctrlstatus rhs)
{
    return (static_cast<int>(lhs) | static_cast<int>(rhs)) != 0;
}

/**
 *  For complex statuses (more than one bit set), this function returns true
 *  if there is any overlay in "on" bits in the two statuses.  More useful if
 *  there is only one bit, so stick to that use-case.
 */

inline bool
bit_test_and (ctrlstatus lhs, ctrlstatus rhs)
{
    return (static_cast<int>(lhs) & static_cast<int>(rhs)) != 0;
}

/*
 *  Free functions in the automation namespace
 */

extern std::string category_to_string (category c);
extern category string_to_category (const std::string & s);
extern std::string action_to_string (action c);
extern action string_to_action (const std::string & s);
extern bool actionable (action a);

#if defined SEQ66_USE_SLOT_STRING_CONVERSIONS
extern std::string slot_to_string (slot s);
extern slot string_to_slot (const std::string & s);
#endif

}               // namespace automation

/*
 *  Free-functions for slots.
 */

inline automation::slot
int_to_slot_cast (int s)
{
    return static_cast<automation::slot>(s);
}

inline int
slot_to_int_cast (automation::slot s)
{
    return static_cast<int>(s);
}

inline int
original_slot_count ()
{
    return static_cast<int>(automation::slot::record_overdub);  /* tricky */
}

inline int
current_slot_count ()
{
    return static_cast<int>(automation::slot::max);
}

}               // namespace seq66

#endif          // SEQ66_AUTOMATION_HPP

/*
 * automation.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


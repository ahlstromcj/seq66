#if ! defined SEQ66_AUTOMATION_HPP
#define SEQ66_AUTOMATION_HPP

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
 * \file          automation.hpp
 *
 *  This module declares/defines the namespace for some enumeration classes
 *  used to specify control categories and actions.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-18
 * \updates       2019-06-08
 * \license       GNU GPLv2 or above
 *
 *  This module defines a number of constants relating to control of pattern
 *  unmuting, group control, and a number of additional controls to make
 *  Seq66 controllable without a graphical user interface.  See the cpp file for
 *  additional information.
 *
 *  It requires C++11 and above.
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
 *  Provides the number of values in a midicontrol sub-stanza.  Recall
 *  that one sub-stanza is represented by a <code> [ 0 0 0 0 0 0 ] </code>
 *  item in the "rc"/"ctrl" file.
 */

static const int SUBCOUNT = 6;

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
    maximum         /**< Not used, except to check for illegal settings.    */
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
 * \var maximum
 *      -   Simply a limit number.
 */

enum class action
{
    none,
    toggle,
    on,
    off,
    maximum
};

/**
 *  Pseudo control values for associating MIDI events, for the automation
 *  of some of the controls in seq66. Unlike the earlier version,
 *  this version is not necessarily tied to the 32-pattern paradigm.
 *
 *  Each slot value is tied to a particular performer member function.  Each
 *  slot accesses the performer member function via a lambda loaded into a
 *  map.  Faster lookup than traversing a bunch of if-statements, it is to be
 *  hoped.
 *
 *  The controls are read in from the "rc" configuration files, and are
 *  written to the c_midictrl section of the "proprietary" final track in
 *  a Seq66 MIDI file.  The controls represented by slot values are part
 *  of the automation (user-interface) section of the "rc" file.
 *
 *  Unlike the original controls, all of the control groups (pattern,
 *  mute-group, and automation) all support a number of controls not
 *  necessarily equal to 32.  Also, up/down controls have been folded into
 *  one control.  We need to be able to convert between old and new
 *  "control" numbers.
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
 *          opcontrol::sm_slot_names[] array to match!
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
    solo,               /**< 12: TODO, intended to solo track.              */
    thru,               /**< 13: Enables/disables the MIDI THRU control.    */
    bpm_page_up,        /**< 14: Increments BMP by a configured page value. */
    bpm_page_dn,        /**< 15: Decrements BMP by a configured page value. */
    ss_set,             /**< 16: Key: set screen-set; MIDI: playing set.    */
    record,             /**< 17: Enables/disables the MIDI record control.  */
    quan_record,        /**< 18: Enables/disables quantized recording.      */
    reset_seq,          /**< 19: Controls loop overwrite versus reset.      */
    mod_oneshot,        /**< 20: Set status of one-shot queuing.            */
    FF,                 /**< 21: Fast-forwards the clock (pulse counter.)   */
    rewind,             /**< 22: Rewinds the clock (pulse counter).         */
    top,                /**< 22: Set to song beginning or L marker.         */
    playlist,           /**< 24: MIDI only, arrow keys hardwired.           */
    playlist_song,      /**< 25: MIDI only, arrow keys hardwired.           */
    tap_bpm,            /**< 26: Tap key for estimating BPM.                */
    start,              /**< 27: Start playback. Compare to playback above. */
    stop,               /**< 28: Stop playback. Compare to playback above.  */
    mod_snapshot_2,     /**< 29: The second snapshot key.  TODO.            */
    toggle_mutes,       /**< 30: Song mute, unmute, and toggle?  TODO.      */
    song_pointer,       /**< 31: Reposition the song pointer.  TODO.        */

    /*
     * The following add to what Seq64 supports.
     */

    keep_queue,         /**< 32: Set keep-queue (the "Q" button).           */
    slot_shift,         /**< 33: Used for sets > 32 patterns.               */
    mutes_clear,        /**< 34: Set all mute groups to unarmed.            */
    reserved_35,        /**< 35: Reserved for expansion.                    */
    pattern_edit,       /**< 36: GUI action, bring up pattern for editing.  */
    event_edit,         /**< 37: GUI action, bring up the event editor.     */
    song_mode,          /**< 38: GUI. Toggle between Song Mode & Live Mode. */
    toggle_jack,        /**< 39: GUI. Toggle between JACK and ALSA support. */
    menu_mode,          /**< 40: GUI. Switch menu between enabled/disabled. */
    follow_transport,   /**< 41: GUI. Toggle between following JACK or not. */
    reserved_42,        /**< 42: Reserved for expansion.                    */
    reserved_43,        /**< 43: Reserved for expansion.                    */
    reserved_44,        /**< 44: Reserved for expansion.                    */
    reserved_45,        /**< 45: Reserved for expansion.                    */
    reserved_46,        /**< 46: Reserved for expansion.                    */
    reserved_47,        /**< 47: Reserved for expansion.                    */
    reserved_48,        /**< 48: Reserved for expansion.                    */

    maximum,            /**< Used only for termination/range-checking.      */

    /*
     * The following are used for selection the correct op function.  Pattern
     * and mute groups each need only one function (with an integer
     * parameter), while automation uses the codes above to select the proper
     * op function from a rather large set of them.
     */

    loop,               /**< Useful to set and retrieve op function.        */
    mute_group,         /**< Useful to set and retrieve op function.        */
    automation,         /**< Useful to set and retrieve the name.           */
    illegal             /**< A value to falg illegality.                    */
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
 *              mainwid to implement clicking on a pattern.
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
     *  This value signals the "queue" functionality.  If this bit is set, then
     *  perform::sequence_playing_toggle() calls sequence::toggle_queued() on the
     *  given sequence number.  The regular queue key (configurable in File /
     *  Options / Keyboard) sets this bit when pressed, and unsets it when
     *  released.  The keep-queue key sets it, but it is not unset until the
     *  regular queue key is pressed and released.
     */

    queue       = 0x04,

    /**
     *  This value signals the Kepler34 "one-shot" functionality.  If this bit
     *  is set, then perform::sequence_playing_toggle() calls
     *  sequence::toggle_oneshot() on the given sequence number.
     */

    oneshot     = 0x08
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

inline bool
bit_test_or (ctrlstatus lhs, ctrlstatus rhs)
{
    return (static_cast<int>(lhs) | static_cast<int>(rhs)) != 0;
}

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
extern std::string slot_to_string (slot s);
extern slot string_to_slot (const std::string & s);

}               // namespace automation

}               // namespace seq66

#endif          // SEQ66_AUTOMATION_HPP

/*
 * automation.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


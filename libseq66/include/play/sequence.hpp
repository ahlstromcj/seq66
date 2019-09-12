#if ! defined SEQ66_SEQUENCE_HPP
#define SEQ66_SEQUENCE_HPP

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
 * \file          sequence.hpp
 *
 *  This module declares/defines the base class for handling
 *  patterns/sequences.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-30
 * \updates       2019-09-12
 * \license       GNU GPLv2 or above
 *
 *  The functions add_list_var() and add_long_list() have been replaced by
 *  functions in the new midi_vector_base module.
 *
 *  We've offloaded most of the trigger code to the triggers class in its own
 *  module, and now just call its member functions to do the actual work.
 *
 * Special static test functions:
 *
 *  -   maximum(). Returns the maximum supported usable sequence
 *      number (plus one), which is 1024, but could be increased to 2048.  To
 *      clarify, usable sequence number range from 0 to 1023.
 *  -   limit().  Returns 2048 (0x0800), which indicates a legal value that
 *      represents "no background" sequence when present in a Sequencer66 MIDI
 *      file.
 *  -   legal(seqno). Returns true if the sequence number is between 0 and
 *      2048.
 *  -   valid(seqno). Returns true if the sequence number is between 0 and 2047.
 *  -   none(seqno). Returns true if the sequence number is -1.
 *  -   disabled(seqno). Return true if the sequence number is limit().
 *  -   null(seqno).
 *  -   all(seqno). Returns true if the sequence number is -1.  To be used only
 *      in the context of functions and can work on one sequence or all of them.
 *      The caller should pass unassigned() as the sequence number.
 *  -   unassigned().  Returns the value of -1 for sequence number.
 */

#include <string>
#include <stack>

#include "seq66_features.hpp"           /* various feature #defines     */
#include "cfg/scales.hpp"               /* key and scale constants      */
#include "midi/event_list.hpp"          /* seq66::event_list            */
#include "midi/midibus.hpp"             /* seq66::midibus               */
#include "play/triggers.hpp"            /* seq66::triggers, etc.        */
#include "util/automutex.hpp"           /* seq66::recmutex, automutex   */
#include "util/calculations.hpp"        /* measures_to_ticks()          */
#include "util/palette.hpp"             /* enum class ThumbColor        */

/**
 *  The maximum sequence number, in macro form.  This value indicates that no
 *  background sequence value has been assigned yet.  See the value
 *  seqedit::m_initial_sequence, which was originally set to -1 directly.
 *  However, we have issues saving a negative number in MIDI, so we will use
 *  the "proprietary" track's bogus sequence number, which doubles the 1024
 *  sequences we can support.  Values between 0 (inclusive) and
 *  SEQ66_SEQUENCE_LIMIT (exclusive) are valid.  But SEQ66_SEQUENCE_LIMIT is a
 *  <i> legal</i> value, used only for disabling the selection of a background
 *  sequence.
 */

#define SEQ66_SEQUENCE_LIMIT            2048    /* 0x0800 */
#define SEQ66_SEQUENCE_MAXIMUM          1024

/**
 *  Provides an integer value for color that matches PaletteColor::NONE.  That
 *  is, no color has been assigned.  Track colors are represent by a plain
 *  integer in the seq66::sequence class.
 */

#define SEQ66_COLOR_NONE                (-1)

/**
 *
 */

const int c_maxbeats    = 0xFFFF;

/**
 *
 */

const int c_num_keys    = 128;

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class mastermidibus;
    class performer;

/**
 *  The sequence class is firstly a receptable for a single track of MIDI
 *  data read from a MIDI file or edited into a pattern.  More members than
 *  you can shake a stick at.
 */

class sequence
{
    friend class performer;             /* access to set_parent()   */
    friend class triggers;              /* will unfriend later      */

public:

    /**
     *  Provides a setting for Live vs. Song mode.  Much easier to grok and
     *  expand than a boolean.
     */

    enum class playback
    {
        live,
        song
    };

    /**
     * Actions.  These variables represent actions that can be applied to a
     * selection of notes.  One idea would be to add a swing-quantize action.
     * We will reserve the value here, for notes only; not yet used or part of
     * the action menu.
     */

    enum class edit
    {
        select_all_notes = 1,
        select_all_events,
        select_inverse_notes,
        select_inverse_events,
        quantize_notes,
        quantize_events,
        randomize_events,           // #ifdef USE_STAZED_RANDOMIZE_SUPPORT
        tighten_events,
        tighten_notes,
        transpose_notes,            /* basic transpose          */
        reserved,                   /* later: quantize_swing    */
        transpose_harmonic,         /* harmonic transpose       */
        expand_pattern,
        compress_pattern,
        select_even_notes,
        select_odd_notes,
        swing_notes                 /* swing quantize           */
    };

    /**
     *  This enumeration is used in selecting events and note.  Se the
     *  select_note_events() and select_events() functions.
     */

    enum class select
    {
        selecting,      /**< Selection in progress.                     */
        select_one,     /**< To select a single event.                  */
        selected,       /**< The events are selected.                   */
        would_select,   /**< The events would be selected.              */
        deselect,       /**< To deselect event under the cursor.        */
        toggle,         /**< Toggle selection under cursor.             */
        remove,         /**< To remove one note under the cursor.       */
        onset,          /**< Kepler34, To select a single onset.        */
        is_onset        /**< New, from Kepler34, onsets selected.       */
    };

    /**
     *  Provides the supported looping recording modes.  These values are used
     *  by the seqedit class, which provides a button with a popup menu to
     *  select one of these recording modes.
     */

    enum class record
    {
        legacy,         /**< Incoming events are merged into the loop.  */
        overwrite,      /**< Incoming events overwrite the loop.        */
        expand          /**< Incoming events increase size of loop.     */
    };

    /**
     *  Provides a set of methods for drawing certain items.  These values are
     *  used in the sequence, seqroll, perfroll, and mainwid classes.
     */

    enum class draw
    {
        none,           /**< indicates that current event is not a note */
        finish,         /**< Indicates that drawing is finished.        */
        linked,         /**< Used for drawing linked notes.             */
        note_on,        /**< For starting the drawing of a note.        */
        note_off,       /**< For finishing the drawing of a note.       */
        tempo           /**< For drawing tempo meta events.             */
    };

    /**
     *  Provides two editing modes for a sequence.  A feature adapted from
     *  Kepler34.  Not yet ready for prime time.
     */

    enum class editmode
    {
        note,                   /**< Edit as a Note, the normal edit mode.  */
        drum                    /**< Edit as Drum note, using short notes.  */
    };

    /**
     *  A structure that holds note information, used, for example, in the
     *  sequence::get_next_note() overload.
     */

    class note_info
    {
        friend class sequence;

    private:

        midipulse ni_tick_start;
        midipulse ni_tick_finish;
        int ni_note;
        int ni_velocity;
        bool ni_selected;

    public:

        note_info () :
            ni_tick_start   (0),
            ni_tick_finish  (0),
            ni_note         (0),
            ni_velocity     (0),
            ni_selected     (false)
            {
                // no code
            }

       midipulse start () const
       {
           return ni_tick_start;
       }

       midipulse finish () const
       {
           return ni_tick_finish;
       }

       midipulse length () const
       {
           return ni_tick_finish - ni_tick_start;
       }

       int note () const
       {
           return ni_note;
       }

       int velocity () const
       {
           return ni_velocity;
       }

       bool selected () const
       {
           return ni_selected;
       }

       void show () const;

    };      // nested class note_info

private:

    /**
     *  Provides a stack of event-lists for use with the undo and redo
     *  facility.
     */

    using eventstack = std::stack<event_list>;

private:

    /*
     * Documented at the definition point in the cpp module.
     */

    static event_list m_clipboard;   /* shared between sequences */

    /**
     *  For pause support, we need a way for the sequence to find out if JACK
     *  transport is active.  We can use the rcsettings flag(s), but JACK
     *  could be disconnected.  We could use a reference here, but, to avoid
     *  modifying the midifile class as well, we use a pointer.  It is set in
     *  performer::add_sequence().  This member would also be using for passing
     *  modification status to the parent, so that the GUI code doesn't have
     *  to do it.
     */

    performer * m_parent;

    /**
     *  This list holds the current pattern/sequence events.  It used to be
     *  called m_list_events, but a map implementation is now available, and
     *  is the default.
     */

    event_list m_events;

    /**
     *  Holds the list of triggers associated with the sequence, used in the
     *  performance/song editor.
     */

    triggers m_triggers;

    /**
     *  Provides a list of event actions to undo for the Stazed LFO and
     *  seqdata support.
     */

    event_list m_events_undo_hold;

    /**
     *  A stazed flag indicating that we have some undo information.
     */

    bool m_have_undo;

    /**
     *  A stazed flag indicating that we have some redo information.
     *  Previously, unlike the perfedit, the seqedit did not provide a redo
     *  facility.
     */

    bool m_have_redo;

    /**
     *  Provides a list of event actions to undo.
     */

    eventstack m_events_undo;

    /**
     *  Provides a list of event actions to redo.
     */

    eventstack m_events_redo;

    /**
     *  An iterator for drawing events.
     */

    mutable event_list::const_iterator m_iterator_draw;

    /**
     *  A new feature for recording, based on a "stazed" feature.  If true
     *  (not yet the default), then the seqedit window will record only MIDI
     *  events that match its channel.  The old behavior is preserved if this
     *  variable is set to false.
     */

    bool m_channel_match;

    /**
     *  Contains the proper MIDI channel for this sequence.  However, if this
     *  value is EVENT_NULL_CHANNEL (0xFF), then this sequence is an SMF 0
     *  track, and has no single channel.  Please note that this is the output
     *  channel.
     */

    midibyte m_midi_channel;

    /**
     *  Contains the proper MIDI bus number for this sequence.
     */

    midibyte m_bus;

    /**
     *  Provides a flag for the song playback mode muting.
     */

    bool m_song_mute;

    /**
     *  Indicate if the sequence is transposable or not.  A potential feature
     *  from stazed's seq32 project.  Now it is an actual, configurable
     *  feature.
     */

    bool m_transposable;

    /**
     *  Provides a member to hold the polyphonic step-edit note counter.  We
     *  will never come close to the short limit of 32767.
     */

    short m_notes_on;

    /**
     *  Provides the master MIDI buss which handles the output of the sequence
     *  to the proper buss and MIDI channel.
     */

    mastermidibus * m_master_bus;

    /**
     *  Provides a "map" for Note On events.  It is used when muting, to shut
     *  off the notes that are playing. The number of notes playing will never
     *  come close to the unsigned short limit of 65535.
     */

    unsigned short m_playing_notes[SEQ66_MIDI_NOTES_MAX];

    /**
     *  Indicates if the sequence was playing.
     */

    bool m_was_playing;

    /**
     *  True if sequence playback currently is in progress for this sequence.
     *  In other words, the sequence is armed.
     */

    bool m_playing;

    /**
     *  True if sequence recording currently is in progress for this sequence.
     */

    bool m_recording;

    /**
     *  Provides an option for expanding the number of measures while
     *  recording.  In essence, the "infinite" track we've wanted, thanks
     *  to Stazed and his Seq32 project.  Defaults to false.
     */

    bool m_expanded_recording;

    /**
     *  True if recording in quantized mode.
     */

    bool m_quantized_rec;

    /**
     *  True if recording in MIDI-through mode.
     */

    bool m_thru;

    /**
     *  True if the events are queued.
     */

    bool m_queued;

    /**
     *  A member from the Kepler34 project to indicate we are in one-shot mode
     *  for triggering.  Set to false whenever playing-state changes.  Used in
     *  sequence :: play_queue() to maybe play from the one-shot tick, then
     *  toggle play and toggle queuing before playing normally.
     *
     *  One-shot mode is entered when the MIDI control c_status_oneshot event
     *  is received.  Kepler34 reserves the period '.' to initiate this event.
     */

    bool m_one_shot;

    /**
     *  A Member from the Kepler34 project, set in sequence ::
     *  toggle_one_shot() to m_last_tick adjusted to the length of the
     *  sequence.  Compare this member to m_queued_tick.
     */

    midipulse m_one_shot_tick;

    /**
     *  Indicates if we have turned off from a snap operation.
     */

    bool m_off_from_snap;

    /**
     *  Used to temporarily block Song Mode events while recording new
     *  ones.  Set to false if at a trigger transition in trigger playback.
     *  Otherwise, triggers are allow to be processed.  Turned off when
     *  song-recording stops.
     */

    bool m_song_playback_block;

    /**
     *  Used to keep on blocking Song Mode events while recording new ones.
     *  Allows recording a live performance, by storing the sequence triggers.
     *  Adapted from Kepler34.
     */

    bool m_song_recording;

    /**
     *  This value indicates that the following feature is active: the number
     *  of tick to snap recorded improvisations.
     */

    bool m_song_recording_snap;

    /**
     *  Saves the tick from when we started recording live song data.
     */

    midipulse m_song_record_tick;

    /**
     *  Indicates if overwrite recording of notes in a loop is in force.
     */

    bool m_overwrite_recording;

    /**
     *  Indicates if the play marker has gone to the beginning of the sequence
     *  upon looping.
     */

    bool m_loop_reset;

    /**
     *  Hold the current unit for a measure.  Need to clarifiy this one.
     *  It is calculated when needed (lazy evaluation).
     */

    mutable midipulse m_unit_measure;

    /**
     *  The "m_dirty"  flags indicate that the content of the sequence has
     *  changed due to recording, editing, performance management, or even (?) a
     *  name change.  They all start out as "true" in the sequence constructor.
     *
     *      -   The function sequence::set_dirty_mp() sets all but the "dirty
     *          edit" flag to true. It is set when modifying the BPM,
     *          beat-width, toggling cueing, changing the pattern name,
     *      -   The function sequence::set_dirty() sets all four flags to true.
     *
     *  Provides the main dirtiness flag.  In Seq24, it was:
     *
     *      -   Set in perform::is_dirty_main() to set the same status for a
     *          given sequence.  (It also set "was active main" for the
     *          sequence.)
     *      -   Cause mainwid to update a given sequence in the live frame.
     */

    mutable bool m_dirty_main;

    /**
     *  Provides the main is-edited flag. In Seq24, it was:
     *
     *      -   Set in perform::is_dirty_edit() to set the same status for a
     *          given sequence.  (It also set "was active edit" for the
     *          sequence.)
     *      -   Used in seqedit::timeout to refresh the seqroll, seqdata, and
     *          seqevent panes.
     */

    mutable bool m_dirty_edit;

    /**
     *  Provides performance dirty flagflag.
     *
     *      -   Set in perform::is_dirty_perf() to set the same status for a
     *          given sequence.  (It also set "was active perf" for the
     *          sequence.)
     *      -   Used in perfroll to redraw each "dirty perf" sequence.
     */

    mutable bool m_dirty_perf;

    /**
     *  Provides the names dirtiness flag.
     *
     *      -   Set in perform::is_dirty_names() to set the same status for a
     *          given sequence.  (It also set "was active names" for the
     *          sequence.)
     *      -   Used in perfnames to redraw each "dirty names" sequence.
     */

    mutable bool m_dirty_names;

    /**
     *  Indicates that the sequence is currently being edited.
     */

    bool m_editing;

    /**
     *  Used in seqmenu and seqedit.  It allows a sequence editor window to
     *  pop up if not already raised, in seqedit::timeout().
     */

    bool m_raise;

    /**
     *  Set by seqedit for the handle_action() function to use.
     */

    midibyte m_status;
    midibyte m_cc;
    midipulse m_snap;         // replace m_snap_tick???

    int m_scale;

    /**
     *  Provides the name/title for the sequence.
     */

    std::string m_name;

    /**
     *  Provides the default name/title for the sequence.
     */

    static const std::string sm_default_name;

    /**
     *  These members manage where we are in the playing of this sequence,
     *  including triggering.
     */

    midipulse m_last_tick;          /**< Provides the last tick played.     */
    midipulse m_queued_tick;        /**< Provides the tick for queuing.     */
    midipulse m_trigger_offset;     /**< Provides the trigger offset.       */

    /**
     *  This constant provides the scaling used to calculate the time position
     *  in ticks (pulses), based also on the PPQN value.  Hardwired to
     *  c_maxbeats at present.
     */

    const int m_maxbeats;

    /**
     *  Holds the PPQN value for this sequence, so that we don't have to rely
     *  on a global constant value.
     */

    unsigned short m_ppqn;

    /**
     *  A new member so that the sequence number is carried along with the
     *  sequence.  This number is set in the performer::install_sequence()
     *  function.
     */

    short m_seq_number;

    /**
     *  Implements a feature from the Kepler34 project.  It is an index into a
     *  palette.  The colorbyte type is defined in the midibytes.hpp file.
     */

    colorbyte m_seq_color;

    /**
     * A feature adapted from Kepler34.
     */

    editmode m_seq_edit_mode;

    /**
     *  Holds the length of the sequence in pulses (ticks).  This value should
     *  be a power of two when used as a bar unit.  This value depends on the
     *  settings of beats/minute, pulses/quarter-note, the beat width, and the
     *  number of measures.
     */

    midipulse m_length;

    /**
     *  The size of snap in units of pulses (ticks).  It starts out as the
     *  value m_ppqn / 4.
     */

    midipulse m_snap_tick;

    /**
     *  Provides the number of beats per bar used in this sequence.  Defaults
     *  to 4.  Used by the sequence editor to mark things in correct time on
     *  the user-interface.
     */

    unsigned short m_time_beats_per_measure;

    /**
     *  Provides with width of a beat.  Defaults to 4, which means the beat is
     *  a quarter note.  A value of 8 would mean it is an eighth note.  Used
     *  by the sequence editor to mark things in correct time on the
     *  user-interface.
     */

    unsigned short m_time_beat_width;

    /**
     *  Augments the beats/bar and beat-width with the additional values
     *  included in a Time Signature meta event.  This value provides the
     *  number of MIDI clocks between metronome clicks.  The default value of
     *  this item is 24.  It can also be read from some SMF 1 files, such as
     *  our hymne.mid example.
     */

    int m_clocks_per_metronome;     /* make it a short? */

    /**
     *  Augments the beats/bar and beat-width with the additional values
     *  included in a Time Signature meta event.  This value provides the
     *  number of notated 32nd notes in a MIDI quarter note (24 MIDI clocks).
     *  The usual (and default) value of this parameter is 8; some sequencers
     *  allow this to be changed.
     */

    int m_32nds_per_quarter;        /* make it a short? */

    /**
     *  Augments the beats/bar and beat-width with the additional values
     *  included in a Tempo meta event.  This value can be extracted from the
     *  beats-per-minute value (mastermidibus::m_beats_per_minute), but here
     *  we set it to 0 by default, indicating that we don't want to write it.
     *  Otherwise, it can be read from a MIDI file, and saved here to be
     *  restored later.
     */

    long m_us_per_quarter_note;

    /**
     *  The volume to be used when recording.  It can range from 0 to 127,
     *  or be set to SEQ66_PRESERVE_VELOCITY (-1).
     */

    short m_rec_vol;

    /**
     *  The Note On velocity used.  Currently set to
     *  SEQ66_DEFAULT_NOTE_ON_VELOCITY.  If the recording velocity
     *  (m_rec_vol) is non-zero, this value will be set to the desired
     *  recording velocity.  A "stazed" feature.
     */

    short m_note_on_velocity;

    /**
     *  The Note Off velocity used.  Currently set to
     *  SEQ66_DEFAULT_NOTE_OFF_VELOCITY, and currently unmodifiable.  A
     *  "stazed" feature.
     */

    short m_note_off_velocity;

    /**
     *  Holds a copy of the musical key for this sequence, which we now
     *  support writing to this sequence.  If the value is
     *  SEQ66_KEY_OF_C, then there is no musical key to be set.
     */

    midibyte m_musical_key;

    /**
     *  Holds a copy of the musical scale for this sequence, which we now
     *  support writing to this sequence.  If the value is the enumeration
     *  value scales::off, then there is no musical scale to be set.
     */

    midibyte m_musical_scale;

    /**
     *  Holds a copy of the background sequence number for this sequence,
     *  which we now support writing to this sequence.  If the value is
     *  greater than max_sequence(), then there is no background sequence to
     *  be set.
     */

    short m_background_sequence;

    /**
     *  Provides locking for the sequence.  Made mutable for use in
     *  certain locked getter functions.
     */

    mutable recmutex m_mutex;

    /**
     *  Provides the number of ticks to shave off of the end of painted notes.
     *  Also used when the user attempts to shrink a note to zero (or less
     *  than zero) length.
     */

    const midipulse m_note_off_margin;

private:

    /*
     * We're going to replace this operator with the more specific
     * partial_assign() function.
     */

    sequence & operator = (const sequence & rhs);

public:

    sequence (int ppqn = SEQ66_USE_DEFAULT_PPQN);
    ~sequence ();

    void partial_assign (const sequence & rhs);

    void set_editing (midibyte status, midibyte cc, midipulse snap, int scale)
    {
        m_status = status;
        m_cc = cc;
        m_snap = snap;
        m_scale = scale;
    }

    /**
     * \getter m_events
     *      Non-const version.
     */

    event_list & events ()
    {
        return m_events;
    }

    /**
     * \getter m_events
     *      Const version.
     */

    const event_list & events () const
    {
        return m_events;
    }

    /**
     * \getter m_events.any_selected_notes()
     */

    bool any_selected_notes () const
    {
        return m_events.any_selected_notes();
    }

    /**
     * \getter m_triggers
     *      This is the const version.
     */

    const triggers::List & triggerlist () const
    {
        return m_triggers.triggerlist();
    }

    /**
     * \getter m_triggers
     */

    triggers::List & triggerlist ()
    {
        return m_triggers.triggerlist();
    }

    /**
     *  Gets the trigger count, useful for exporting a sequence.
     */

    int trigger_count () const
    {
        return int(m_triggers.count());
    }

    /**
     *  Gets the number of selected triggers.  That is, selected in the
     *  perfroll.
     */

    int selected_trigger_count () const
    {
        return m_triggers.number_selected();
    }

    /**
     *  Sets the tick for pasting.
     *
     * \param tick
     *      Provides the pulse value to set.
     */

    void set_trigger_paste_tick (midipulse tick)
    {
        m_triggers.set_trigger_paste_tick(tick);
    }

    /**
     *  Gets the tick for pasting.
     *
     * \return
     *      Returns the current pulse value.
     */

    midipulse get_trigger_paste_tick () const
    {
        return m_triggers.get_trigger_paste_tick();
    }

    /**
     * \getter m_seq_number as a string
     */

    std::string seq_number_string () const
    {
        char temp[16];
        snprintf(temp, sizeof temp, "%d", int(m_seq_number));
        return std::string(temp);
    }

    int seq_number () const
    {
        return int(m_seq_number);
    }

    /**
     * \setter m_seq_number
     *      This setter will set the sequence number only if it has not
     *      already been set.
     */

    void seq_number (int seqno)
    {
        if (seqno >= 0 && seqno <= int(SHRT_MAX) && none(m_seq_number))
            m_seq_number = short(seqno);
    }

    int color () const
    {
        return int(m_seq_color);
    }

    bool color (int c);
    void empty_coloring ();

    /**
     * \getter m_seq_edit_mode
     *      A feature adapted from Kepler34.
     */

    editmode edit_mode () const
    {
        return m_seq_edit_mode;
    }

    /**
     * \setter m_seq_edit_mode
     *      A feature adapted from Kepler34.
     */

    void edit_mode (editmode mode)
    {
        m_seq_edit_mode = mode;
    }

    void modify ();
    int event_count () const;
    int note_count () /*const*/;
    bool minmax_notes (int & lowest, int & highest) /*const*/;

    /*
     * seqdata and lfownd hold for undo
     */

    void set_hold_undo (bool hold);

    /**
     * \getter m_events_undo_hold.count()
     */

    int get_hold_undo () const
    {
        return m_events_undo_hold.count();
    }

    /**
     * \setter m_have_undo
     */

    void set_have_undo ()
    {
        m_have_undo = m_events_undo.size() > 0;
        if (m_have_undo)                            /* ca 2016-08-16        */
            modify();                               /* have pending changes */
    }

    /**
     * \getter m_have_undo
     */

    bool have_undo () const
    {
        return m_have_undo;
    }

    /**
     * \setter m_have_redo
     *      No reliable way to "unmodify" the performance here.
     */

    void set_have_redo ()
    {
        m_have_redo = m_events_redo.size() > 0;
    }

    /**
     * \getter m_have_redo
     */

    bool have_redo () const
    {
        return m_have_redo;
    }

    void push_undo (bool hold = false);     /* adds stazed parameter    */
    void pop_undo ();
    void pop_redo ();
    void push_trigger_undo ();
    void pop_trigger_undo ();
    void pop_trigger_redo ();
    void set_name (const std::string & name = "");
    int calculate_measures () const;
    int get_measures () const;

    bool measure_threshold () const
    {
        return calculate_measures() > 4;
    }

    /**
     * \getter m_ppqn
     *      Provided as a convenience for the editable_events class.
     */

    int get_ppqn () const
    {
        return int(m_ppqn);
    }

    void set_beats_per_bar (int beatspermeasure);

    /**
     * \getter m_time_beats_per_measure
     */

    int get_beats_per_bar () const
    {
        return int(m_time_beats_per_measure);
    }

    void set_beat_width (int beatwidth);

    /**
     * \getter m_time_beat_width
     *
     * \threadsafe
     */

    int get_beat_width () const
    {
        return int(m_time_beat_width);
    }

    /**
     *  A convenience function for calculating the number of ticks in the given
     *  number of measures.
     */

    midipulse measures_to_ticks (int measures = 1) const
    {
        return seq66::measures_to_ticks     /* see "calculations" module    */
        (
            int(m_time_beats_per_measure), int(m_ppqn),
            int(m_time_beat_width), measures
        );
    }

    /**
     * \setter m_clocks_per_metronome
     */

    void clocks_per_metronome (int cpm)
    {
        m_clocks_per_metronome = cpm;       // needs validation
    }

    /**
     * \getter m_clocks_per_metronome
     */

    int clocks_per_metronome () const
    {
        return m_clocks_per_metronome;
    }

    /**
     * \setter m_32nds_per_quarter
     */

    void set_32nds_per_quarter (int tpq)
    {
        m_32nds_per_quarter = tpq;              // needs validation
    }

    /**
     * \getter m_32nds_per_quarter
     */

    int get_32nds_per_quarter () const
    {
        return m_32nds_per_quarter;
    }

    /**
     * \setter m_us_per_quarter_note
     */

    void us_per_quarter_note (long upqn)
    {
        m_us_per_quarter_note = upqn;       // needs validation
    }

    /**
     * \getter m_us_per_quarter_note
     */

    long us_per_quarter_note () const
    {
        return m_us_per_quarter_note;
    }

    void set_rec_vol (int rec_vol);

    /**
     * \setter m_song_mute
     *      This function also calls set_dirty_mp() to make sure that the
     *      perfnames panel is updated to show the new mute status of the
     *      sequence.
     *
     * \param mute
     *      True if the sequence is to be muted.
     *
     *  For Seq66, this function also calls set_playing() to the opposite of the
     *  mute value.
     */

    void set_song_mute (bool mute)
    {
        m_song_mute = mute;
        set_playing(! mute);            /* new with Sequencer66             */
        set_dirty_mp();
    }

    /**
     * \setter m_song_mute
     *      This function toogles the song muting status.
     */

    void toggle_song_mute ()
    {
        m_song_mute = ! m_song_mute;
        set_dirty_mp();
    }

    /**
     * \getter m_song_mute
     */

    bool get_song_mute () const
    {
        return m_song_mute;
    }

    void apply_song_transpose ();
    void set_transposable (bool flag);

    /**
     * \getter m_transposable
     */

    bool transposable () const
    {
        return m_transposable;
    }

    std::string title () const;

    /**
     * \getter m_name
     */

    const std::string & name () const
    {
        return m_name;
    }

    /**
     *  Tests the name for being changed.
     */

    bool is_default_name () const
    {
        return m_name == sm_default_name;
    }

    /**
     * \getter sm_default_name
     */

    static const std::string & default_name ()
    {
        return sm_default_name;
    }

    /**
     * \setter m_editing
     */

    void set_editing (bool edit)
    {
        m_editing = edit;
    }

    /**
     * \getter m_editing
     */

    bool get_editing () const
    {
        return m_editing;
    }

    /**
     * \setter m_raise
     */

    void set_raise (bool edit)
    {
        m_raise = edit;
    }

    /**
     * \getter m_raise
     */

    bool get_raise (void) const
    {
        return m_raise;
    }

    /*
     * Documented at the definition point in the cpp module.
     */

    bool set_length
    (
        midipulse len = 0,
        bool adjust_triggers = true,
        bool verify = true
    );

    /**
     *  Kepler34
     */

    void set_measures (int measures)
    {
        set_length
        (
            measures * get_beats_per_bar() * (m_ppqn * 4) / get_beat_width()
        );
    }

    bool apply_length (int bpb, int ppqn, int bw, int measures = 1);
    int extend (midipulse len);

    /**
     *  An overload that gets its values from this sequence object.
     *
     * \param meas
     *      The number of measures to apply.  Defaults to 1.
     */

    bool apply_length (int meas = 1)
    {
        return apply_length
        (
            get_beats_per_bar(), int(m_ppqn), get_beat_width(), meas
        );
    }

    /**
     * \getter m_length
     */

    midipulse get_length () const
    {
        return m_length;
    }

    midipulse get_last_tick () const;
    void set_last_tick (midipulse tick);

    /**
     *  Some MIDI file errors and other things can lead to an m_length of 0,
     *  which causes arithmetic errors when m_last_tick is modded against it.
     *  This function replaces the "m_last_tick % m_length", returning
     *  m_last_tick if m_length is 0 or 1.
     */

    midipulse mod_last_tick ()
    {
        return (m_length > 1) ? (m_last_tick % m_length) : m_last_tick ;
    }

    /*
     * Documented at the definition point in the cpp module.
     */

    void set_playing (bool p);

    bool get_playing () const
    {
        return m_playing;
    }

    /*
     * The midipulse and bool parameters of the overload of this function are
     * new, to support song-recording.
     */

    bool toggle_playing ()
    {
        set_playing(! get_playing());
        return get_playing();
    }

    void toggle_playing (midipulse tick, bool resumenoteons);
    void toggle_queued ();

    /**
     * \getter m_queued
     */

    bool get_queued () const
    {
        return m_queued;
    }

    /**
     * \getter m_queued_tick
     */

    midipulse get_queued_tick () const
    {
        return m_queued_tick;
    }

    /**
     *  Helper function for performer.
     */

    bool check_queued_tick (midipulse tick) const
    {
        return get_queued() && (get_queued_tick() <= tick);
    }

    void set_recording (bool record_active);
    void set_quantized_recording (bool qr);
    void set_input_recording (bool record_active, bool toggle = false);

    /**
     * \getter m_recording
     */

    bool get_recording () const
    {
        return m_recording;
    }

    /**
     * \getter m_expanded_recording
     */

    void expanded_recording (bool expand)
    {
        m_expanded_recording = expand;
    }

    /**
     * \setter m_expanded_recording
     */

    bool expanded_recording ()
    {
        return m_expanded_recording;
    }

    bool expand_recording () const;     /* does more checking for status    */

    midipulse get_snap_tick () const
    {
        return m_snap_tick;
    }

    void set_snap_tick (int st);

    bool get_quantized_rec () const
    {
        return m_quantized_rec;
    }

    void set_thru (bool thru_active);                               // seqedit
    void set_input_thru (bool thru_active, bool toggle = false);    // performer

    bool get_thru () const
    {
        return m_thru;
    }

    void off_one_shot ();
    void song_recording_start (midipulse tick, bool snap = false);
    void song_recording_stop (midipulse tick);

    midipulse one_shot_tick () const
    {
        return m_one_shot_tick;
    }

    bool song_recording () const
    {
        return m_song_recording;
    }

    bool one_shot () const
    {
        return m_one_shot;
    }

    bool off_from_snap () const
    {
        return m_off_from_snap;
    }

    bool snap_it () const
    {
        return get_playing() && (get_queued() || off_from_snap());
    }

    bool song_playback_block () const
    {
        return m_song_playback_block;
    }

    bool song_recording_snap () const
    {
        return m_song_recording_snap;
    }

    midipulse song_record_tick () const
    {
        return m_song_record_tick;
    }

    void resume_note_ons (midipulse tick);
    void toggle_one_shot ();

    bool is_dirty_main () const;
    bool is_dirty_edit () const;
    bool is_dirty_perf () const;
    bool is_dirty_names () const;
    void set_dirty_mp ();
    void set_dirty ();

    midibyte get_midi_channel () const
    {
        return m_midi_channel;
    }

    /**
     *  Returns true if this sequence is an SMF 0 sequence.
     */

    bool is_smf_0 () const
    {
        return m_midi_channel == EVENT_NULL_CHANNEL;
    }

    void set_midi_channel (midibyte ch, bool user_change = false);
    void print () const;
    void print_triggers () const;
    void play (midipulse tick, bool playback_mode, bool resume = false);
    void play_queue (midipulse tick, bool playbackmode, bool resume);
    bool add_note
    (
        midipulse tick, midipulse len, int note,
        bool paint = false,
        int velocity = SEQ66_PRESERVE_VELOCITY
    );
    bool add_chord (int chord, midipulse tick, midipulse len, int note);
    bool add_event (const event & er);      /* another one declared below */
    bool add_event
    (
        midipulse tick, midibyte status,
        midibyte d0, midibyte d1, bool paint = false
    );
    bool append_event (const event & er);

    /**
     *  Calls event_list::sort().
     */

    void sort_events ()
    {
        m_events.sort();
    }

    void add_trigger
    (
        midipulse tick, midipulse len,
        midipulse offset = 0, bool adjust_offset = true
    );
    void split_trigger (midipulse tick);
    void half_split_trigger (midipulse tick);
    void exact_split_trigger (midipulse tick);
    void grow_trigger (midipulse tick_from, midipulse tick_to, midipulse len);
    void delete_trigger (midipulse tick);
    bool get_trigger_state (midipulse tick) const;
    bool select_trigger (midipulse tick);
    triggers::List get_triggers () const;
    bool unselect_trigger (midipulse tick);
    bool unselect_triggers ();
    bool intersect_triggers (midipulse pos, midipulse & start, midipulse & end);
    bool intersect_triggers (midipulse pos);
    bool intersect_notes
    (
        midipulse position, int position_note,
        midipulse & start, midipulse & ender, int & note
    );
    bool intersect_events
    (
        midipulse posstart, midipulse posend,
        midibyte status, midipulse & start
    );
    bool delete_selected_triggers ();
    bool cut_selected_trigger ();
    void copy_selected_trigger ();
    void paste_trigger (midipulse paste_tick = SEQ66_NO_PASTE_TRIGGER);
    bool move_triggers
    (
        midipulse tick, bool adjust_offset,
        triggers::grow which = triggers::grow::move
    );

    void offset_triggers
    (
        midipulse offset,
        triggers::grow editmode = triggers::grow::move
    );

    bool selected_trigger
    (
        midipulse droptick, midipulse & tick0, midipulse & tick1
    );
    midipulse selected_trigger_start ();
    midipulse selected_trigger_end ();
    midipulse get_max_trigger () const;
    void move_triggers (midipulse start_tick, midipulse distance, bool direction);
    void copy_triggers (midipulse start_tick, midipulse distance);
    void clear_triggers ();

    /**
     * \getter m_trigger_offset
     */

    midipulse get_trigger_offset () const
    {
        return m_trigger_offset;
    }

    void set_midi_bus (char mb, bool user_change = false);

    /**
     * \getter m_bus
     */

    char get_midi_bus () const
    {
        return m_bus;
    }

    void set_master_midi_bus (const mastermidibus * mmb);
    int select_note_events
    (
        midipulse tick_s, int note_h,
        midipulse tick_f, int note_l, select action
    );
    int select_events
    (
        midipulse tick_s, midipulse tick_f,
        midibyte status, midibyte cc, select action
    );
    int select_events
    (
        midibyte status, midibyte cc, bool inverse = false
    );

#if defined USE_STAZED_SELECTION_EXTENSIONS

    int select_events (midipulse tick_s, midipulse tick_f, midibyte status);
    int select_event_handle
    (
        midipulse tick_s, midipulse tick_f, midibyte status,
        midibyte cc, int data_s
    );

#endif

    int select_linked (long tick_s, long tick_f, midibyte status);

#if defined USE_STAZED_ODD_EVEN_SELECTION

    /*
     *  Given a note length (in ticks) and a boolean indicating even or odd,
     *  select all notes where the note on even occurs exactly on an even (or
     *  odd) multiple of note length.  Example use: select every note that
     *  starts on an even eighth note beat.
     */

    int select_even_or_odd_notes (int note_len, bool even);

#endif

    /**
     *  New convenience function.  What about Aftertouch events?  I think we
     *  need to select them as well in seqedit, so let's add that selection
     *  here as well.
     *
     * \param inverse
     *      If set to true (the default is false), then this causes the
     *      selection to be inverted.
     */

    void select_all_notes (bool inverse = false)
    {
        (void) select_events(EVENT_NOTE_ON, 0, inverse);
        (void) select_events(EVENT_NOTE_OFF, 0, inverse);
        (void) select_events(EVENT_AFTERTOUCH, 0, inverse);
    }

    int get_num_selected_notes () const;
    int get_num_selected_events (midibyte status, midibyte cc) const;
    void select_all ();
    void copy_selected ();
    void cut_selected (bool copyevents = true);
    void paste_selected (midipulse tick, int note);
    bool get_selected_box
    (
        midipulse & tick_s, int & note_h, midipulse & tick_f, int & note_l
    );
    bool get_onsets_selected_box
    (
        midipulse & tick_s, int & note_h, midipulse & tick_f, int & note_l
    );
    bool get_clipboard_box
    (
        midipulse & tick_s, int & note_h, midipulse & tick_f, int & note_l
    );
    midipulse adjust_timestamp (midipulse t, bool isnoteoff);
    midipulse trim_timestamp (midipulse t);
    midipulse clip_timestamp (midipulse ontime, midipulse offtime);
    void move_selected_notes (midipulse deltatick, int deltanote);
    bool stream_event (event & ev);
    bool change_event_data_range
    (
        midipulse tick_s, midipulse tick_f,
        midibyte status, midibyte cc,
        int d_s, int d_f, bool useundo = false
    );
    bool change_event_data_relative
    (
        midipulse tick_s, midipulse tick_f,
        midibyte status, midibyte cc,
        int newval, bool useundo = false
    );
    void change_event_data_lfo
    (
        double value, double range, double speed, double phase,
        wave w, midibyte status, midibyte cc, bool useundo = false
    );
    void increment_selected (midibyte status, midibyte /*control*/);
    void decrement_selected (midibyte status, midibyte /*control*/);
    void grow_selected (midipulse deltatick);
    void stretch_selected (midipulse deltatick);

#if defined USE_STAZED_RANDOMIZE_SUPPORT
    void randomize_selected
    (
        midibyte status, midibyte control, int plus_minus
    );
    void adjust_data_handle (midibyte status, int data);
#endif

    bool mark_selected ();
    bool remove_selected ();
    bool remove_marked ();                      /* a forwarding function */
    void unpaint_all ();
    void unselect ();
    void verify_and_link ();
    void link_new ();

    /**
     *  A new function to re-link the tempo events added by the user.
     */

    void link_tempos ()
    {
        m_events.link_tempos();
    }

    /**
     *  Resets everything to zero.  This function is used when the sequencer
     *  stops.  This function currently sets m_last_tick = 0, but we would
     *  like to avoid that if doing a pause, rather than a stop, of playback.
     */

    void zero_markers ()
    {
        set_last_tick(0);
    }

    void play_note_on (int note);
    void play_note_off (int note);
    void off_playing_notes ();
    void stop (bool song_mode = false);     /* playback::live vs song   */
    void pause (bool song_mode = false);    /* playback::live vs song   */
    void inc_draw_marker ();
    void reset_draw_marker ();
    void reset_draw_trigger_marker ();
    void reset_ex_iterator (event_list::const_iterator & evi) const;
    bool reset_interval
    (
        midipulse t0, midipulse t1,
        event_list::const_iterator & it0,
        event_list::const_iterator & it1
    ) const;
    draw get_note_info
    (
        note_info & niout,
        event_list::const_iterator & evi
    ) const;
    draw get_next_note (note_info & niout) const;
    draw get_next_note_ex
    (
        note_info & niout,
        event_list::const_iterator & evi
    ) const;
    bool get_next_event (midibyte & status, midibyte & cc);
    bool get_next_event_match
    (
        midibyte status, midibyte cc,
        event_list::const_iterator & ev,
        int evtype = EVENTS_ALL
    );
    bool get_next_event_ex
    (
        midibyte & status, midibyte & cc,
        event_list::const_iterator & evi
    );
    bool next_trigger (trigger & trig);
    void quantize_events
    (
        midibyte status, midibyte cc,
        midipulse snap_tick, int divide, bool linked = false
    );
    void push_quantize
    (
        midibyte status, midibyte cc,
        midipulse snap_tick, int divide, bool linked = false
    );
    void transpose_notes (int steps, int scale);

#if defined USE_STAZED_SHIFT_SUPPORT
    void shift_notes (midipulse ticks);
#endif

#if defined USE_STAZED_COMPANDING
    void multiply_pattern (double multiplier);
#endif

    /**
     * \getter m_musical_key
     */

    midibyte musical_key () const
    {
        return m_musical_key;
    }

    void musical_key (int key)
    {
        if (key >= SEQ66_KEY_OF_C && key < SEQ66_OCTAVE_SIZE)
            m_musical_key = midibyte(key);
    }

    midibyte musical_scale () const
    {
        return m_musical_scale;
    }

    void musical_scale (int scale)
    {
        if (scale >= int(scales::off) && scale < int(scales::max))
            m_musical_scale = midibyte(scale);
    }

    int background_sequence () const
    {
        return int(m_background_sequence);
    }

    /**
     * \setter m_background_sequence
     *      Only partial validation at present, we do not want the upper
     *      limit to be hard-wired at this time.  Disabling the sequence
     *      number [setting it to limit()] is valid.
     */

    void background_sequence (int bs)
    {
        if (legal(bs))
            m_background_sequence = short(bs);
    }

    void show_events () const;
    void copy_events (const event_list & newevents);

    midipulse note_off_margin () const
    {
        return m_note_off_margin;
    }

    void calculate_unit_measure () const;
    midipulse unit_measure () const;
    midipulse expand_threshold () const;
    midipulse progress_value () const;

    /**
     * \getter m_channel_match
     *      The master bus needs to know if the match feature is truly in
     *      force, otherwise it must pass the incoming events to all recording
     *      sequences.  Compare this function to channels_match().
     */

    bool channel_match () const
    {
        return m_channel_match;
    }

    void overwrite_recording (bool ovwr);

    bool overwrite_recording ()
    {
        return m_overwrite_recording;
    }

    void loop_reset (bool reset);

    bool loop_reset ()
    {
        return m_loop_reset;
    }

    midipulse handle_size (midipulse start, midipulse finish);
    void handle_edit_action (edit action, int var);

    bool check_loop_reset ();

public:

    /**
     *  Defines the constant number of sequences/patterns.  This value has
     *  historically been 1024, which is 32 patterns per set times 32 sets.  But
     *  we don't want to support any more than this value, based on trials with
     *  the b4uacuse-stress.midi file, which has only about 4 sets (128 patterns)
     *  and pretty much loads up a CPU.
     */

    static int maximum ()
    {
        return SEQ66_SEQUENCE_MAXIMUM;
    }

    /**
     *  The above-maximum sequence number, in macro form.  This value indicates
     *  that no background sequence value has been assigned yet.  See the value
     *  seqedit::m_initial_sequence, which was originally set to -1 directly.
     *  However, we have issues saving a negative number in MIDI, so we will use
     *  the "proprietary" track's bogus sequence number, which doubles the 1024
     *  sequences we can support.  Values between 0 (inclusive) and
     *  sequence_limit(), exclusive, are valid.  It is a <i> legal</i> value,
     *  used for disabling the selection of a background sequence.
     */

    static int limit ()
    {
        return SEQ66_SEQUENCE_LIMIT;
    }

    /**
     *  A convenient macro function to test against sequence_limit().
     *  Although above the range of usable loop numbers, it is a legal value.
     *  Compare this to the valid() function.
     */

    static bool legal (int seqno)
    {
        return seqno >= 0 && seqno <= limit();     /* SEQ66_IS_LEGAL_SEQUENCE  */
    }

    /**
     *  Similar to legal(), but excludes sequence::limit().
     */

    static bool valid (int seqno)
    {
        return seqno >= 0 && seqno < maximum();
    }

    /**
     *  Checks if a the sequence number is an assigned one, i.e. not equal to
     *  -1.
     */

    static bool none (int seqno)
    {
        return seqno == SEQ66_UNASSIGNED;
    }

    /**
     *  A convenient function to test against sequence::limit().
     *  This function does not allow that value as a valid value to use.
     */

    static bool disabled (int seqno)
    {
        return seqno == limit();
    }

    /**
     *  A convenient function to test against SEQ66_SEQUENCE_LIMIT.  This
     *  function does not allow SEQ66_SEQUENCE_LIMIT as a valid value to use.
     */

    static bool null (int seqno)
    {
        return none(seqno);
    }

    /**
     *  Indicates that all patterns will be processed by a function taking a
     *  seq::number parameter.
     */

    static bool all (int seqno)
    {
        return seqno == SEQ66_ALL_TRACKS;
    }

    static int all_tracks ()
    {
        return SEQ66_ALL_TRACKS;
    }

    /**
     *  Indicates that a sequence number has not been assigned.
     */

    static int unassigned ()
    {
        return SEQ66_UNASSIGNED;
    }

    static int loop_record (record r)
    {
        return static_cast<int>(r);
    }

    bool update_recording (int index);

    /**
     *  Short hand for testing a draw parameter.
     */

    static bool is_draw_note (draw dt)
    {
        return dt == sequence::draw::linked ||
            dt == sequence::draw::note_on || dt == sequence::draw::note_off;
    }

private:

    bool event_in_range
    (
        const event & e, midibyte status,
        midipulse tick_s, midipulse tick_f
    ) const;

    void set_parent (performer * p);
    void put_event_on_bus (event & ev);
    void reset_loop ();
    void set_trigger_offset (midipulse trigger_offset);
    void adjust_trigger_offsets_to_length (midipulse newlen);
    midipulse adjust_offset (midipulse offset);
    void remove (event_list::iterator i);
    void remove (event & e);
    void remove_all ();

    /**
     *  Checks to see if the event's channel matches the sequence's nominal
     *  channel.
     *
     * \param e
     *      The event whose channel nybble is to be checked.
     *
     * \return
     *      Returns true if the channel-matching feature is enabled and the
     *      channel matches, or true if the channel-matching feature is turned
     *      off, in which case the sequence accepts events on any channel.
     */

    bool channels_match (const event & e) const
    {
        if (m_channel_match)
            return (e.get_status() & 0x0F) == m_midi_channel;
        else
            return true;
    }

    void one_shot (bool f)
    {
        m_one_shot = f;
    }

    void off_from_snap (bool f)
    {
        m_off_from_snap = f;
    }

    void song_playback_block (bool f)
    {
        m_song_playback_block = f;
    }

    void song_recording (bool f)
    {
        m_song_recording = f;
    }

    void song_recording_snap (bool f)
    {
        m_song_recording_snap = f;
    }

    void song_record_tick (midipulse t)
    {
        m_song_record_tick = t;
    }

};          // class sequence

}           // namespace seq66

#endif      // SEQ66_SEQUENCE_HPP

/*
 * sequence.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


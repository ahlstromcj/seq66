#if ! defined SEQ66_SEQUENCE_HPP
#define SEQ66_SEQUENCE_HPP

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
 * \file          sequence.hpp
 *
 *  This module declares/defines the base class for handling
 *  patterns/sequences.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-30
 * \updates       2021-08-12
 * \license       GNU GPLv2 or above
 *
 *  The functions add_list_var() and add_long_list() have been replaced by
 *  functions in the new midi_vector_base module.
 *
 *  We've offloaded most of the trigger code to the triggers class in its own
 *  module, and now just call its member functions to do the actual work.
 */

#include <atomic>                       /* std::atomic<bool> for dirt       */
#include <stack>                        /* std::stack<eventlist>            */
#include <string>                       /* std::string                      */

#include "seq66_features.hpp"           /* various feature #defines         */
#include "cfg/scales.hpp"               /* key and scale constants          */
#include "cfg/usrsettings.hpp"          /* enum class record                */
#include "midi/eventlist.hpp"           /* seq66::eventlist                 */
#include "midi/midibus.hpp"             /* seq66::midibus                   */
#include "play/triggers.hpp"            /* seq66::triggers, etc.            */
#include "util/automutex.hpp"           /* seq66::recmutex, automutex       */
#include "util/calculations.hpp"        /* measures_to_ticks()              */
#include "util/palette.hpp"             /* enum class ThumbColor            */

/**
 *  Provides an integer value for color that matches PaletteColor::NONE.  That
 *  is, no color has been assigned.  Track colors are represent by a plain
 *  integer in the seq66::sequence class.
 */

const int c_seq_color_none = (-1);

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class mastermidibus;
    class notemapper;
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
        song,
        automatic,
        max
    };

    /**
     *  Provides a set of methods for drawing certain items.  These values are
     *  used in the sequence, seqroll, perfroll, and main window classes.
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
     *  A structure that holds note information, used, for example, in
     *  sequence::get_next_note().
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

    using eventstack = std::stack<eventlist>;

private:

    /**
     *  The number of MIDI notes in what?  This value is used in the sequence
     *  module.  It looks like it is the maximum number of notes that
     *  seq24/seq66 can have playing at one time.  In other words, "only" 256
     *  simultaneously-playing notes can be managed.  Defines the maximum
     *  number of notes playing at one time that the application will support.
     */

    static const int c_playing_notes_max = 256;

    /**
     *  Used as the default velocity parameter in adding notes.
     */

    static short sm_preserve_velocity;

    /*
     * Documented at the definition point in the cpp module.
     */

    static eventlist sm_clipboard;      /* shared between sequences */

    /*
     * For fingerprinting check with speed.
     */

    static int sm_fingerprint_size;

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

    eventlist m_events;

    /**
     *  Holds the list of triggers associated with the sequence, used in the
     *  performance/song editor.
     */

    triggers m_triggers;

    /**
     *  Provides a list of event actions to undo for the Stazed LFO and
     *  seqdata support.
     */

    eventlist m_events_undo_hold;

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
     *  A new feature for recording, based on a "stazed" feature.  If true
     *  (not yet the default), then the seqedit window will record only MIDI
     *  events that match its channel.  The old behavior is preserved if this
     *  variable is set to false.
     */

    bool m_channel_match;

    /**
     *  Contains the global MIDI channel for this sequence.  However, if this
     *  value is c_midichannel_null (0x80), then this sequence is an SMF 0
     *  track, and has no single channel, or represents a track who's recorded
     *  channels we do not want to replace.  Please note that this is the
     *  output channel.
     */

    midibyte m_midi_channel;            /* pattern's global MIDI channel    */

    /**
     *  This value indicates that the global MIDI channel associated with this
     *  pattern is not used.  Instead, the actual channel of each event is
     *  used.  This is true when m_midi_channel == null_channel().
     */

    bool m_free_channel;

    /**
     *  Contains the nominal output MIDI bus number for this sequence/pattern.
     *  This number is saved in the sequence/pattern. If port-mapping is in
     *  place, this number is used only to look up the true output buss.
     */

    bussbyte m_nominal_bus;

    /**
     *  Contains the actual buss number to be used in output.
     */

    bussbyte m_true_bus;

    /**
     *  Provides a flag for pattern playback song muting.
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

    unsigned short m_playing_notes[c_playing_notes_max];

    /**
     *  Indicates if the sequence was playing.  This value is set at the end
     *  of the play() function.  It is used to continue playing after changing
     *  the pattern length.
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
     *  If true, the first incoming event in the step-edit (auto-step) part of
     *  stream_event() will reset the starting tick to 0.  Useful when
     *  recording a stock pattern from a drum machine.
     */

    bool m_auto_step_reset;

    /**
     *  Provides an option for expanding the number of measures while
     *  recording.  In essence, the "infinite" track we've wanted, thanks
     *  to Stazed and his Seq32 project.  Defaults to false.
     */

    bool m_expanded_recording;

    /**
     *  Indicates if overwrite recording of notes in a loop is in force.
     *  In this mode, shortly after then end of the pattern length, the existing
     *  notes are erased.  This lets the player try again and again to get the
     *  pattern perfect.
     */

    bool m_overwrite_recording;

    /**
     *  If true, when recording reaches the end of the length, stop recording.
     *  Useful with the m_auto_step_reset value, too.
     */

    bool m_oneshot_recording;

    /**
     *  True if recording in quantized mode.
     */

    bool m_quantized_recording;

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
     *  A member from the Kepler34 project, set in sequence ::
     *  toggle_one_shot() to m_last_tick adjusted to the length of the
     *  sequence.  Compare this member to m_queued_tick.
     */

    midipulse m_one_shot_tick;

    /**
     *  A counter used in the step-edit (auto-edit) feature.
     */

    int m_step_count;

    /**
     *  Number of times to play the pattern in Live mode.  A value of 0 means
     *  to play the pattern endlessly in Live mode, like normal.  The maximum
     *  loop-count, if non-zero, is stored in a c_seq_loopcount SeqSpec as a
     *  short integer.
     */

    int m_loop_count_max;

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
     *  changed due to recording, editing, performance management, or a name
     *  change.  They all start out as "true" in the sequence constructor.
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
     *      -   Cause mainwnd to update a given sequence in the live frame.
     */

    mutable std::atomic<bool> m_dirty_main;

    /**
     *  Provides the main is-edited flag. In Seq24, it was:
     *
     *      -   Set in perform::is_dirty_edit() to set the same status for a
     *          given sequence.  (It also set "was active edit" for the
     *          sequence.)
     *      -   Used in seqedit::timeout to refresh the seqroll, seqdata, and
     *          seqevent panes.
     */

    mutable std::atomic<bool> m_dirty_edit;

    /**
     *  Provides performance dirty flagflag.
     *
     *      -   Set in perform::is_dirty_perf() to set the same status for a
     *          given sequence.  (It also set "was active perf" for the
     *          sequence.)
     *      -   Used in perfroll to redraw each "dirty perf" sequence.
     */

    mutable std::atomic<bool> m_dirty_perf;

    /**
     *  Provides the names dirtiness flag.
     *
     *      -   Set in perform::is_dirty_names() to set the same status for a
     *          given sequence.  (It also set "was active names" for the
     *          sequence.)
     *      -   Used in perfnames to redraw each "dirty names" sequence.
     */

    mutable std::atomic<bool> m_dirty_names;

    /**
     *  Indicates that the sequence is currently being edited.
     */

    bool m_seq_in_edit;

    /**
     *  Set by seqedit for the handle_action() function to use.
     */

    midibyte m_status;
    midibyte m_cc;

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
     *  or be set to the preserve-velocity (-1).
     */

    short m_rec_vol;

    /**
     *  The Note On velocity used, set to usr().note_on_velocity().  If the
     *  recording velocity (m_rec_vol) is non-zero, this value will be set to
     *  the desired recording velocity.  A "stazed" feature.
     */

    short m_note_on_velocity;

    /**
     *  The Note Off velocity used, set to usr().note_on_velocity(), and
     *  currently unmodifiable.  A "stazed" feature.
     */

    short m_note_off_velocity;

    /**
     *  Holds a copy of the musical key for this sequence, which we now
     *  support writing to this sequence.  If the value is
     *  c_key_of_C, then there is no musical key to be set.
     */

    midibyte m_musical_key;

    /**
     *  Holds a copy of the musical scale for this sequence, which can be
     *  written to this sequence.  If the value is the enumeration
     *  value scales::off, then there is no musical scale to be set.
     *  Provides the index pointing to the optional scale to be shown on the
     *  background of the pattern.
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

private:

    /*
     * We're going to replace this operator with the more specific
     * partial_assign() function.
     */

    sequence & operator = (const sequence & rhs);

public:

    sequence (int ppqn = c_use_default_ppqn);
    ~sequence ();

    void partial_assign (const sequence & rhs);

#if defined USE_SET_EDITING_FULL_SIGNATURE

    /*
     * Not needed, but kept based on Seq64's seqedit usage.
     */

    void seq_in_edit (midibyte status, midibyte cc, midipulse snap, int scale)
    {
        m_status = status;
        m_cc = cc;
    }

#endif

    eventlist & events ()
    {
        return m_events;
    }

    const eventlist & events () const
    {
        return m_events;
    }

    bool any_selected_notes () const
    {
        return m_events.any_selected_notes();
    }

    const triggers::container & triggerlist () const
    {
        return m_triggers.triggerlist();
    }

    triggers::container & triggerlist ()
    {
        return m_triggers.triggerlist();
    }

    std::string trigger_listing () const
    {
        return m_triggers.to_string();
    }

    /**
     *  Gets the trigger count, useful for exporting a sequence.
     */

    int trigger_count () const
    {
        return int(m_triggers.count());
    }

    int triggers_datasize (midilong seqspec) const
    {
        return m_triggers.datasize(seqspec);
    }

    int any_trigger_transposed () const
    {
        return m_triggers.any_transposed();
    }

    /**
     *  Gets the number of selected triggers.  That is, selected in the
     *  perfroll.
     */

    int selected_trigger_count () const
    {
        return m_triggers.number_selected();
    }

    void set_trigger_paste_tick (midipulse tick)
    {
        m_triggers.set_trigger_paste_tick(tick);
    }

    midipulse get_trigger_paste_tick () const
    {
        return m_triggers.get_trigger_paste_tick();
    }

    int seq_number () const
    {
        return int(m_seq_number);
    }

    std::string seq_number_string () const
    {
        return std::to_string(seq_number());
    }

    void seq_number (int seqno)
    {
        if (seqno >= 0 && seqno <= int(SHRT_MAX))
            m_seq_number = short(seqno);
    }

    int color () const
    {
        return int(m_seq_color);
    }

    bool color (int c);
    void empty_coloring ();

    editmode edit_mode () const
    {
        return m_seq_edit_mode;
    }

    midibyte edit_mode_byte () const
    {
        return static_cast<midibyte>(m_seq_edit_mode);
    }

    void edit_mode (editmode mode)
    {
        m_seq_edit_mode = mode;
    }

    void edit_mode (midibyte b)
    {
        m_seq_edit_mode = b == 0 ? editmode::note : editmode::drum ;
    }

    bool loop_count_max (int m);
    void modify (bool notifychange = true);
    int event_count () const;
    int note_count () const;
    int playable_count () const;
    bool is_playable () const;
    bool minmax_notes (int & lowest, int & highest);

    void set_have_undo ()
    {
        m_have_undo = m_events_undo.size() > 0;
        if (m_have_undo)
            modify();                               /* have pending changes */
    }

    bool have_undo () const
    {
        return m_have_undo;
    }

    /**
     *  No reliable way to "unmodify" the performance here.
     */

    void set_have_redo ()
    {
        m_have_redo = m_events_redo.size() > 0;
    }

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

    bool event_threshold () const
    {
        return note_count() > sm_fingerprint_size;
    }

    int get_ppqn () const
    {
        return int(m_ppqn);
    }

    void set_beats_per_bar (int beatspermeasure);

    int get_beats_per_bar () const
    {
        return int(m_time_beats_per_measure);
    }

    void set_beat_width (int beatwidth);

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

    void clocks_per_metronome (int cpm)
    {
        m_clocks_per_metronome = cpm;       // needs validation
    }

    int clocks_per_metronome () const
    {
        return m_clocks_per_metronome;
    }

    void set_32nds_per_quarter (int tpq)
    {
        m_32nds_per_quarter = tpq;          // needs validation
    }

    int get_32nds_per_quarter () const
    {
        return m_32nds_per_quarter;
    }

    void us_per_quarter_note (long upqn)
    {
        m_us_per_quarter_note = upqn;       // needs validation
    }

    long us_per_quarter_note () const
    {
        return m_us_per_quarter_note;
    }

    void set_rec_vol (int rec_vol);
    void set_song_mute (bool mute);
    void toggle_song_mute ();

    bool get_song_mute () const
    {
        return m_song_mute;
    }

    void apply_song_transpose ();
    void set_transposable (bool flag);

    bool transposable () const
    {
        return m_transposable;
    }

    std::string title () const;

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

    bool is_new_pattern () const
    {
        return is_default_name() && event_count() == 0;
    }

    static const std::string & default_name ()
    {
        return sm_default_name;
    }

    void seq_in_edit (bool edit)
    {
        m_seq_in_edit = edit;
    }

    bool seq_in_edit () const
    {
        return m_seq_in_edit;
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

    midipulse get_length () const
    {
        return m_length;
    }

    midipulse get_tick () const;
    midipulse get_last_tick () const;
    void set_last_tick (midipulse tick = c_null_midipulse);

    midipulse last_tick () const
    {
        return m_last_tick;
    }

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

    bool set_playing (bool p);

    bool playing () const
    {
        return m_playing;
    }

    bool sequence_playing_toggle ();
    bool toggle_playing ();
    bool toggle_playing (midipulse tick, bool resumenoteons);
    bool toggle_queued ();

    bool get_queued () const
    {
        return m_queued;
    }

    midipulse get_queued_tick () const
    {
        return m_queued_tick;
    }

    bool check_queued_tick (midipulse tick) const
    {
        return get_queued() && (get_queued_tick() <= tick);
    }

    bool set_recording (bool recordon, bool toggle = false);
    bool set_quantized_recording (bool qr, bool toggle = false);
    bool set_overwrite_recording (bool ovwr, bool toggle = false);
    bool set_thru (bool thru_active, bool toggle = false);

    bool recording () const
    {
        return m_recording;
    }

    bool quantized_recording () const
    {
        return m_quantized_recording;
    }

    bool quantizing () const
    {
        return m_recording && m_quantized_recording;
    }

    bool expanded_recording () const
    {
        return m_expanded_recording;
    }

    bool expanding () const
    {
        return m_recording && m_expanded_recording;
    }

    bool auto_step_reset () const
    {
        return m_auto_step_reset;
    }

    bool oneshot_recording () const
    {
        return m_oneshot_recording;
    }

    void auto_step_reset (bool flag)
    {
        m_auto_step_reset = flag;
        m_step_count = 0;
    }

    void oneshot_recording (bool flag)
    {
        m_oneshot_recording = flag;
    }

    void expanded_recording (bool expand)
    {
        m_expanded_recording = expand;
    }

    bool expand_recording () const;     /* does more checking for status    */

    bool overwriting () const
    {
        return m_recording && m_overwrite_recording;
    }

    bool thru () const
    {
        return m_thru;
    }

    midipulse snap () const
    {
        return m_snap_tick;
    }

    void snap (int st);
    void off_one_shot ();
    void song_recording_start (midipulse tick, bool snap = true);
    void song_recording_stop (midipulse tick);

    bool one_shot () const
    {
        return m_one_shot;
    }

    midipulse one_shot_tick () const
    {
        return m_one_shot_tick;
    }

    bool check_one_shot_tick (midipulse tick) const
    {
        return one_shot() && (one_shot_tick() <= tick);
    }

    int step_count () const
    {
        return m_step_count;
    }

    int loop_count_max () const
    {
        return m_loop_count_max;
    }

    bool song_recording () const
    {
        return m_song_recording;
    }

    bool off_from_snap () const
    {
        return m_off_from_snap;
    }

    bool snap_it () const
    {
        return playing() && (get_queued() || off_from_snap());
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
    bool toggle_one_shot ();

    bool is_dirty_main () const;
    bool is_dirty_edit () const;
    bool is_dirty_perf () const;
    bool is_dirty_names () const;
    void set_dirty_mp ();
    void set_dirty ();
    std::string channel_string () const;            /* "F" or "<channel+1>" */

    midibyte seq_midi_channel () const
    {
        return m_midi_channel;                      /* midi_channel() below */
    }

    midibyte midi_channel (const event & ev) const
    {
        return m_free_channel ? ev.channel() : m_midi_channel ;
    }

    midibyte midi_channel () const
    {
        return m_free_channel ? null_channel() : m_midi_channel ;
    }

    bool free_channel () const
    {
        return m_free_channel;
    }

    /**
     *  Returns true if this sequence is an SMF 0 sequence.
     */

    bool is_smf_0 () const
    {
        return is_null_channel(m_midi_channel);
    }

    std::string to_string () const;
    void play (midipulse tick, bool playback_mode, bool resume = false);
    void play_queue (midipulse tick, bool playbackmode, bool resume);
    bool push_add_note
    (
        midipulse tick, midipulse len, int note,
        bool paint = false,
        int velocity = sm_preserve_velocity
    );
    bool push_add_chord
    (
        int chord, midipulse tick, midipulse len,
        int note, int velocity = sm_preserve_velocity
    );
    bool add_note
    (
        midipulse tick, midipulse len, int note,
        bool paint = false,
        int velocity = sm_preserve_velocity
    );
    bool add_chord
    (
        int chord, midipulse tick, midipulse len, int note,
        int velocity = sm_preserve_velocity
    );
    bool add_event (const event & er);      /* another one declared below */
    bool add_event
    (
        midipulse tick, midibyte status,
        midibyte d0, midibyte d1, bool paint = false
    );
    bool append_event (const event & er);
    void sort_events ();
    void notify_change (bool userchange = true);
    void notify_trigger ();
    void print_triggers () const;
    void add_trigger
    (
        midipulse tick, midipulse len,
        midipulse offset    = 0,
        midibyte tpose      = 0,
        bool adjust_offset  = true
    );
    bool split_trigger (midipulse tick, trigger::splitpoint splittype);
    void grow_trigger (midipulse tick_from, midipulse tick_to, midipulse len);
    void delete_trigger (midipulse tick);
    bool get_trigger_state (midipulse tick) const;
    bool transpose_trigger (midipulse tick, int transposition);
    bool select_trigger (midipulse tick);
    triggers::container get_triggers () const;
    bool unselect_trigger (midipulse tick);
    bool unselect_triggers ();

#if defined USE_INTERSECT_FUNCTIONS
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
#endif

    bool delete_selected_triggers ();
    bool cut_selected_trigger ();
    void copy_selected_trigger ();
    void paste_trigger (midipulse paste_tick = c_no_paste_trigger);
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
    midipulse get_max_timestamp () const;
    midipulse get_max_trigger () const;
    void move_triggers (midipulse start_tick, midipulse distance, bool direction);
    void copy_triggers (midipulse start_tick, midipulse distance);
    void clear_triggers ();

    midipulse get_trigger_offset () const
    {
        return m_trigger_offset;
    }

    bussbyte seq_midi_bus () const
    {
        return m_nominal_bus;
    }

    bussbyte true_bus () const
    {
        return m_true_bus;
    }

    bool set_master_midi_bus (const mastermidibus * mmb);
    bool set_midi_bus (bussbyte mb, bool user_change = false);
    bool set_midi_channel (midibyte ch, bool user_change = false);
    int select_note_events
    (
        midipulse tick_s, int note_h,
        midipulse tick_f, int note_l, eventlist::select action
    );
    int select_events
    (
        midipulse tick_s, midipulse tick_f,
        midibyte status, midibyte cc, eventlist::select action
    );
    int select_events
    (
        midibyte status, midibyte cc, bool inverse = false
    );

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
    bool repitch (const notemapper & nmap, bool all = false);
    bool copy_selected ();
    bool cut_selected (bool copyevents = true);
    bool paste_selected (midipulse tick, int note);
    bool merge_events (const sequence & source);
    bool selected_box
    (
        midipulse & tick_s, int & note_h, midipulse & tick_f, int & note_l
    );
    bool onsets_selected_box
    (
        midipulse & tick_s, int & note_h, midipulse & tick_f, int & note_l
    );
    bool clipboard_box
    (
        midipulse & tick_s, int & note_h, midipulse & tick_f, int & note_l
    );
    midipulse clip_timestamp (midipulse ontime, midipulse offtime);
    bool move_selected_notes (midipulse deltatick, int deltanote);
    bool move_selected_events (midipulse deltatick);
    bool stream_event (event & ev);
    bool change_event_data_range
    (
        midipulse tick_s, midipulse tick_f,
        midibyte status, midibyte cc,
        int d_s, int d_f
    );
    bool change_event_data_relative
    (
        midipulse tick_s, midipulse tick_f,
        midibyte status, midibyte cc,
        int newval
    );
    void change_event_data_lfo
    (
        double value, double range, double speed, double phase,
        wave w, midibyte status, midibyte cc, bool usemeasure = false
    );
    void increment_selected (midibyte status, midibyte /*control*/);
    void decrement_selected (midibyte status, midibyte /*control*/);
    bool grow_selected (midipulse deltatick);
    bool stretch_selected (midipulse deltatick);
    bool randomize_selected (midibyte status, int plus_minus);
    bool randomize_selected_notes (int jitter = 8, int range = 8);
    bool mark_selected ();
    void unpaint_all ();
    void unselect ();
    void verify_and_link ();
    void link_new ();
    bool edge_fix ();
#if defined USE_ADJUST_DATA_HANDLE
    void adjust_data_handle (midibyte status, int data);
#endif

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
    void reset_draw_trigger_marker ();

    event::buffer::const_iterator cbegin () const
    {
        return m_events.cbegin();
    }

    bool cend (event::buffer::const_iterator & evi) const
    {
        return evi == m_events.cend();
    }

    bool reset_interval
    (
        midipulse t0, midipulse t1,
        event::buffer::const_iterator & it0,
        event::buffer::const_iterator & it1
    ) const;
    draw get_next_note
    (
        note_info & niout,
        event::buffer::const_iterator & evi
    ) const;
    bool get_next_event_match
    (
        midibyte status, midibyte cc,
        event::buffer::const_iterator & ev
    );
    bool get_next_event
    (
        midibyte & status, midibyte & cc,
        event::buffer::const_iterator & evi
    );
    bool next_trigger (trigger & trig);
    bool push_quantize
    (
        midibyte status, midibyte cc,
        int divide, bool linked = false
    );
    bool transpose_notes (int steps, int scale, int key = 0);

#if defined USE_STAZED_SHIFT_SUPPORT
    void shift_notes (midipulse ticks);
#endif

#if defined USE_STAZED_COMPANDING
    void multiply_pattern (double multiplier);
#endif

    midibyte musical_key () const
    {
        return m_musical_key;
    }

    void musical_key (int key)
    {
        if (legal_key(key))
            m_musical_key = midibyte(key);
    }

    midibyte musical_scale () const
    {
        return m_musical_scale;
    }

    void musical_scale (int scale)
    {
        if (legal_scale(scale))
            m_musical_scale = midibyte(scale);
    }

    int background_sequence () const
    {
        return int(m_background_sequence);
    }

    void background_sequence (int bs)
    {
        m_background_sequence = short(bs);      /* no validation */
    }

    void show_events () const;
    void copy_events (const eventlist & newevents);

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

    void loop_reset (bool reset);

    bool loop_reset () const
    {
        return m_loop_reset;
    }

    midipulse handle_size (midipulse start, midipulse finish);
    void handle_edit_action (eventlist::edit action, int var);
    bool check_loop_reset ();

public:

    static void clear_clipboard ()
    {
        sm_clipboard.clear();      /* shared between sequences */
    }

    bool remove_selected ();
    bool remove_marked ();                      /* a forwarding function */

    static int loop_record (recordstyle r)
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

    /**
     *  Necessary for drawing notes in a perf roll.  Why?
     */

    static bool is_draw_note_onoff (draw dt)
    {
        return dt == sequence::draw::note_on || dt == sequence::draw::note_off;
    }

private:

    static int unassigned ()
    {
        return (-1);
    }

    static int limit ()
    {
        return 2048;                    /* 0x0800   */
    }

    mastermidibus * master_bus ()
    {
        return m_master_bus;
    }

    const performer * perf () const
    {
        return m_parent;
    }

    performer * perf ()
    {
        return m_parent;
    }

    bool quantize_events
    (
        midibyte status, midibyte cc, int divide, bool linked = false
    );
    bool change_ppqn (int p);
    void set_parent (performer * p);
    void put_event_on_bus (event & ev);
    void reset_loop ();
    void set_trigger_offset (midipulse trigger_offset);
    void adjust_trigger_offsets_to_length (midipulse newlen);
    midipulse adjust_offset (midipulse offset);
    draw get_note_info                      /* used only internally     */
    (
        note_info & niout,
        event::buffer::const_iterator & evi
    ) const;

#if defined USE_SEQUENCE_REMOVE_EVENTS
    void remove (event::buffer::iterator i);
    void remove (event & e);
#endif

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
        return m_channel_match ?
            event::mask_channel(e.get_status()) == m_midi_channel : true ;
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


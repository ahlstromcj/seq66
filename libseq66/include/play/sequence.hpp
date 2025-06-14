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
 * \updates       2025-06-14
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
#include "cfg/usrsettings.hpp"          /* enum class record                */
#include "midi/calculations.hpp"        /* seq66::lengthfix, alteration     */
#include "midi/eventlist.hpp"           /* seq66::eventlist                 */
#include "play/triggers.hpp"            /* seq66::triggers, etc.            */
#include "util/automutex.hpp"           /* seq66::recmutex, automutex       */

/**
 *  Provides an integer value for color that matches PaletteColor::none.  That
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
 *  Provides a way to save a sequence palette color in a single byte.  This
 *  value is signed since we need a value of -1 to indicate no color, and 0 to
 *  127 to indicate the index that "points" to a palette color. (The actual
 *  limit is currently 31, though, which ought to be enough colors.)
 */

using colorbyte = char;

/**
 *  A structure for encapsulating the many input parameters of sequence ::
 *  fix_pattern(). Also serves as an output to describe exactly what
 *  happened with the calculations.
 *
 *  Must be created using an initializer list.
 *
 * \var fp_fix_type
 *      Indicates if the length of the pattern is to be affected, either by
 *      setting the number of measures, or by scaling the pattern.  In either
 *      of those cases, the timestamps of all events will be adjusted
 *      accordingly.
 *
 w \var fp_alter_type
 *      Indicates how all events are to be altered, such as being tightened,
 *      quantized, note-mapping, etc.
 *
 * \var fp_length
 *      Indicates to set the pattern length to a specified value, in ticks.
 *
 * \var fp_tighten_range
 *      Set a range for tightening (partial quantization) of the pattern's
 *      events.
 *
 * \var fp_random_range
 *      Set a range for randomization of events. Randomize velocity for notes.
 *
 * \var fp_pitch_range
 *      Set a range for randomization of note-event pitches.
 *
 * \var fp_quantize_range
 *      Set a range for full quantization) of the pattern's events.
 *
 * \var fp_jitter_range
 *      Set a range, in MIDI ticks, for "humanizing" a pattern.
 *
 * \var fp_align_left
 *      Indicates if the offset of the first event or, preferably first note
 *      event, is to be adjusted to 0, shifting all events leftward by the same
 *      ammount of time.
 *
 * \var fp_align_right
 *      The opposite of fp_align_right.
 *
 * \var fp_reverse
 *      Reverses the timestamps of event, while preserving the duration of the
 *      notes. The new timestamp is the distance of the event from the end
 *      (length) of the pattern, which we call the "reference".
 *
 * \var fp_reverse_in_place
 *      Similar to fp_reverse, except that the last event is used as the
 *      "reference" (instead of the pattern length).
 *
 * \var fp_save_note_length
 *      If true, do not scale the note-off timestamps.  Keep them at the same
 *      offset against the linked note-on event.
 *
 * \var fp_use_time_signature
 *      If true, try to alter the time signature.  This occurs if the measures
 *      string is a fraction (e.g. "3/4" or "5/4").
 *
 * \var fp_beats_per_bar
 *      If fp_use_time_signature is true, then this value is assumed to be the
 *      (possibly new) beats per bar.
 *
 * \var fp_beat_width
 *      If fp_use_time_signature is true, then this value is assumed to be the
 *      (possibly new) beat width.
 *
 * \var fp_measures
 *      The final length of the pattern, Ignored if the fix_type is not
 *      lengthfix::measures, but the new bar count is returned here for
 *      display purposes.
 *
 * \var fp_scale_factor
 *      The factor used to change the length of the pattern,  Ignored if the
 *      fix_type is not lengthfix::rescale. Sanity checked to not too small,
 *      not too large, and not 0.  Might be changed according to process, so
 *      that the final value can be displayed.
 *
 * \par fp_notemap_file
 *      Provides the name of the note-map file to use to re-map notes.
 *
 * \par fp_reverse_notemap
 *      Re-map notes in the other directions
 *
 * \var fp_effect
 *      Indicates the effect(s) of the change, using the fixeffect enumeration
 *      in the calculations module. Intended to be an output field.
 */

struct fixparameters
{
    lengthfix fp_fix_type;
    alteration fp_alter_type;
    midipulse fp_length;
    int fp_tighten_range;
    int fp_quantize_range;
    int fp_random_range;
    int fp_pitch_range;
    int fp_jitter_range;
    bool fp_align_left;
    bool fp_align_right;
    bool fp_reverse;
    bool fp_reverse_in_place;
    bool fp_save_note_length;
    bool fp_use_time_signature;
    int fp_beats_per_bar;
    int fp_beat_width;
    double fp_measures;
    double fp_scale_factor;
    std::string fp_notemap_file;
    bool fp_reverse_notemap;
    fixeffect fp_effect;
};

/**
 *  The sequence class is firstly a receptable for a single track of MIDI
 *  data read from a MIDI file or edited into a pattern.  More members than
 *  you can shake a stick at.
 */

class sequence
{
    friend class performer;             /* access to set_parent()   */
    friend class triggers;

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
     *  used in the sequence, seqroll, perfroll, qloopbutton, and main window
     *  classes. Also see the seq66::qseqdata::type enumeration.
     */

    enum class draw
    {
        none,           /**< indicates that current event is not a note */
        finish,         /**< Indicates that drawing is finished.        */
        linked,         /**< Used for drawing linked notes.             */
        note_on,        /**< For starting the drawing of a note.        */
        note_off,       /**< For finishing the drawing of a note.       */
        tempo,          /**< For drawing tempo meta events.             */
        program,        /**< For drawing program change (patch) events. */
        controller,     /**< For all control-change events.             */
        pitchbend,      /**< For indicating a pitch-wheel event.        */
        max
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
     *
     *  If the note is invalid (as might happen in searches), then the
     *  note value is (-1).
     *
     *  The usage of this small class has evolved to support other
     *  events, as indicated by the draw enumeration above.
     */

    class note_info
    {
        friend class sequence;

    private:

        midipulse ni_tick_start;
        midipulse ni_tick_finish;
        int ni_note;                /* for tempo, the location to paint it  */
        int ni_velocity;            /* for tempo, the truncated tempo value */
        bool ni_selected;
        bool ni_non_note;           /* true for all non-note events         */

    public:

        note_info () :
            ni_tick_start   (0),
            ni_tick_finish  (0),
            ni_note         (0),    /* we could initialize this to (-1)     */
            ni_velocity     (0),
            ni_selected     (false),
            ni_non_note     (false)
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

       bool valid () const
       {
           return note() >= 0;
       }

       int velocity () const
       {
           return ni_velocity;
       }

       bool selected () const
       {
           return ni_selected;
       }

       bool non_note ()
       {
           return ni_non_note;
       }

       void show () const;

    };      // nested class note_info

private:

    /**
     *  Provides a stack of event-lists for use with the undo and redo
     *  facility.
     */

    using eventstack = std::stack<eventlist>;

public:

    /**
     *  Holds partial information about a time signature.
     */

    using timesig = struct
    {
        double sig_start_measure;   /* Starting measure, precalculated.     */
        double sig_measures;        /* Size in measures, precalculated.     */
        int sig_beats_per_bar;      /* The beats-per-bar in the time-sig.   */
        int sig_beat_width;         /* The size of each beat in the bar.    */
        int sig_ticks_per_beat;     /* Simplifies later calculations.       */
        midipulse sig_start_tick;   /* The pulse where time-sig was placed. */
        midipulse sig_end_tick;     /* Next time-sig start (0 == end?).     */
    };

    /**
     *  A list of time-signatures, which assumes that only the beats/bar and
     *  beat width vary.
     */

    using timesig_list = std::vector<timesig>;

private:

    /**
     *  The number of MIDI notes in what?  This value is used in the sequence
     *  module.  It looks like it is the maximum number of notes that
     *  seq24/seq66 can have playing at one time.  In other words, "only" 256
     *  simultaneously-playing notes can be managed.  Defines the maximum
     *  number of notes playing at one time that the application will support.
     *  BOGUS.  It was meant for counting legal notes, and only 128 are
     *  available (see the constant c_notes_count).
     *
     *      static const int c_playing_notes_max = 256;
     */

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

private:

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
     *  called m_list_events, but another implementation is now available, and
     *  is the default.
     */

    eventlist m_events;

    /**
     *  Holds the list of triggers associated with the sequence, used in the
     *  performance/song editor.
     */

    triggers m_triggers;

    /**
     *  Holds a list of time-signatures in the pattern, for use when drawing
     *  the vertical grid-lines in the pattern-editor time, piano roll, and
     *  event (qstriggereditor) panes.
     */

    timesig_list m_time_signatures;

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
     *  (not the default), then Seq66 will record only MIDI events that match
     *  its output channel.  The old behavior is preserved if this variable is
     *  set to false.
     */

    bool m_channel_match;

    /**
     *  Contains the global MIDI output channel for this sequence.  However,
     *  if this value is null_channel() (0x80), then this sequence is a
     *  multi-chanel track, and has no single channel, or represents a track
     *  who's recorded channels we do not want to replace.  Please note that
     *  this is the output channel.  However, if set to a valid channel, then
     *  that channel will be forced on notes created via painting in the
     *  seqroll.
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
     *  Similar to the above, but for the input buss, a new feature.
     *  Unlike the output buss, this input buss is optional.
     */

    bussbyte m_nominal_in_bus;
    bussbyte m_true_in_bus;

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
     *  off the notes that are playing.
     *
     * unsigned short m_playing_notes[c_notes_count];
     */

    std::vector<unsigned short> m_playing_notes;

    /**
     *  Indicates if the sequence was playing.  This value is set at the end
     *  of the play() function.  It is used to continue playing after changing
     *  the pattern length. Turns out to be unused in both Seq24 and Seq66.
     *
     *      bool m_was_playing;
     */

    /**
     *  True if sequence playback currently is possible for this sequence.
     *  In other words, the sequence is armed.
     */

    bool m_armed;

    /**
     *  True if sequence recording currently is in progress for this sequence.
     */

    bool m_recording;
    mutable bool m_draw_locked;

    /**
     *  Eliminates a bunch of booleans. The default style is merge (overdub).
     */

    recordstyle m_recording_style;

    /**
     *  Replaces a potential bunch of booleans. The data type is defined in
     *  the calculations module.
     */

    alteration m_record_alteration;

    /**
     *  True if recording in MIDI-through mode.
     */

    bool m_thru;

    /**
     *  True if there's a popup-menu present. See how it is used in
     *  qloopbutton.
     */

    bool m_has_popup;

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
     *  of ticks to snap recorded improvisations and manually-added triggers.
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
     *  Indicates the pattern was modified.  Unlike the is_dirty_xxx flags,
     *  this one is not reset when checked.  Useful when closing a file or the
     *  application to cause a "Save?" prompt.
     */

    mutable bool m_is_modified;

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
     *  function. Also see the alias seq::number, which is not short,
     *  but int!
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
     *  Used in handling one-shot recording while playback is in progress.
     *  This value allows the user to wait a few loops before starting to play
     *  the one-shot pattern.
     */

    midipulse m_next_boundary;

    /**
     *  Holds the last number of measures, purely for detecting changes that
     *  affect the measure count.  Normally, get_measures() makes a live
     *  calculation of the current measure count.  For example, changing the
     *  beat-width to a smaller value could increase the number of measures.
     */

    mutable int m_measures;

    /**
     *  The size of snap in units of pulses (ticks).  It starts out as the
     *  value m_ppqn / 4.
     */

    midipulse m_snap_tick;

    /**
     *  The size of adding an auto-step (step-edit) note in units of pulses
     *  (ticks).  It starts out as the value m_ppqn / 4.
     */

    midipulse m_step_edit_note_length;

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
     *  the desired recording velocity.  A "stazed" feature.  Note that
     *  we use (-1) for flagging preserving the velocity of incoming notes.
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

    /*
     * What is the cost of adding virtual here, at runtime?
     */

    virtual ~sequence ();

    void partial_assign (const sequence & rhs, bool toclipboard = false);

    static short maximum ()
    {
        return 1024;
    }

    static short recorder ()
    {
        return 2040;
    }

    static short is_recorder (int s)
    {
        return short(s) == 2040;
    }

    static short metronome ()
    {
        return 2047;
    }

    static bool is_metronome (int s)
    {
        return s == 2047;
    }

    static int limit ()
    {
        return 2048;                                /* 0x0800               */
    }

    static bool is_normal (int s)
    {
        return s < 1024;                            /* see maximum() above  */
    }

    static int unassigned ()
    {
        return (-1);
    }

    eventlist & events ()
    {
        return m_events;
    }

    const eventlist & events () const
    {
        return m_events;
    }

    bool empty () const
    {
        return m_events.empty();
    }

    bool any_selected_notes () const
    {
        return m_events.any_selected_notes();
    }

    bool any_selected_events () const
    {
        return m_events.any_selected_events();
    }

    bool any_selected_events (midibyte status, midibyte cc) const
    {
        return m_events.any_selected_events(status, cc);
    }

    bool is_exportable () const
    {
        return ! get_song_mute() && trigger_count() > 0;
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

    bool analyze_time_signatures ();

    int time_signature_count () const
    {
        return int(m_time_signatures.size());
    }

    const timesig & get_time_signature (size_t index) const;
    bool current_time_signature (midipulse p, int & beats, int & beatwidth) const;
    int measure_number (midipulse p) const;
    midipulse time_signature_pulses (const std::string & s) const;

    bool is_recorder_seq () const
    {
        return m_seq_number == recorder();
    }

    bool is_metro_seq () const
    {
        return m_seq_number == metronome();
    }

    /*
     * Indicates a normal, modifiable sequence. The sequence is not one of
     * our hidden workhorses for metronome and auto-recording functions.
     * It is normally not visible and not modifiable.
     */

    bool is_normal_seq () const
    {
        return m_seq_number < maximum();
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
        if (seqno >= 0 && seqno <= limit())
            m_seq_number = short(seqno);
    }

    int color () const
    {
        return int(m_seq_color);
    }

    bool set_color (int c, bool user_change = false);
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

    bool loop_count_max (int m, bool user_change = false);
    void modify (bool notifychange = true);

    void unmodify ()
    {
        m_is_modified = false;
    }

    int event_count () const;
    int note_count () const;
    bool first_notes (midipulse & ts, int & n) const;
    int playable_count () const;
    bool is_playable () const;
    bool minmax_notes (int & lowest, int & highest);

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

    void set_have_undo ();
    void push_undo (bool hold = false);     /* adds stazed parameter    */
    void pop_undo ();
    void pop_redo ();
    void push_trigger_undo ();
    void pop_trigger_undo ();
    void pop_trigger_redo ();
    void set_name (const std::string & name = "");
    int calculate_measures (bool reset = false) const;
    int get_measures (midipulse newlength) const;
    int get_measures () const;

    int measures () const
    {
        return m_measures;
    }

    bool event_threshold () const
    {
        return note_count() > sm_fingerprint_size;
    }

    int get_ppqn () const
    {
        return int(m_ppqn);
    }

    void set_beats_per_bar (int beatspermeasure, bool user_change = false);

    int get_beats_per_bar () const
    {
        return int(m_time_beats_per_measure);
    }

    void set_beat_width (int beatwidth, bool user_change = false);

    int get_beat_width () const
    {
        return int(m_time_beat_width);
    }

    void set_time_signature (int bpb, int bw);

    /**
     *  A convenience function for calculating the number of ticks in the
     *  given number of measures.
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
    void set_transposable (bool flag, bool user_change = false);

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

    static bool valid_scale_factor (double s, bool ismeasure = false);
    static int trunc_measures (double m);

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

    bool set_length
    (
        midipulse len = 0,
        bool adjust_triggers = true,
        bool verify = true
    );

    bool set_measures (int measures, bool user_change = false);
    int increment_measures ();

    bool apply_length
    (
        int bpb, int ppqn, int bw,
        int measures = 0, bool user_change = false
    );

    bool apply_length (int meas = 0, bool user_change = false)
    {
        return apply_length(0, 0, 0, meas, user_change);
    }

    bool extend_length ();
    bool double_length ();

    midipulse get_length () const
    {
        return m_length;
    }

    midipulse get_length_plus () const
    {
        int bpmeas = m_time_beats_per_measure;
        if (bpmeas == 0)
            bpmeas = 4;

        return m_length + m_unit_measure / bpmeas;
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

    bool set_armed (bool p);

    bool armed () const
    {
        return m_armed;
    }

    bool muted () const
    {
        return ! m_armed;
    }

    bool sequence_playing_toggle ();
    bool toggle_playing ();
    bool toggle_playing (midipulse tick, bool resumenoteons);
    bool toggle_queued ();

    void set_popup (bool flag)
    {
        m_has_popup = flag;
    }

    bool has_popup () const
    {
        return m_has_popup;
    }

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

    bool set_recording_style (recordstyle rs);

    /*
     * The seq66::toggler flag enumeration is off, on, and flip!
     */

    bool set_recording (toggler flag);
    bool set_recording (alteration q, toggler flag);
    bool set_thru (bool thru_active, bool toggle = false);

    bool recording () const
    {
        return m_recording;
    }

    bool alter_recording () const
    {
        return m_record_alteration != alteration::none;
    }

    alteration record_alteration () const
    {
        return m_record_alteration;
    }

    void record_alteration (alteration a)
    {
        m_record_alteration = a;
    }

    bool quantized_recording () const
    {
        return m_record_alteration == alteration::quantize;
    }

    bool quantizing () const
    {
        return quantized_recording();
    }

    bool tightened_recording () const
    {
        return m_record_alteration == alteration::tighten;
    }

    bool tightening () const
    {
        return tightened_recording();
    }

    bool notemapped_recording () const
    {
        return m_record_alteration == alteration::notemap;
    }

    bool notemapping () const
    {
        return notemapped_recording();
    }

    bool expanded_recording () const
    {
        return m_recording_style == recordstyle::expand;
    }

    bool expanding () const
    {
        return recording() && expanded_recording();
    }

    bool oneshot_recording () const
    {
        return m_recording_style == recordstyle::oneshot;
    }

    bool expand_recording () const;     /* does more checking for status    */

    bool overwriting () const
    {
        return m_recording_style == recordstyle::overwrite;
    }

    bool thru () const
    {
        return m_thru;
    }

    midipulse snap () const
    {
        return m_snap_tick;
    }

    midipulse step_edit_note_length () const    /* auto-step/step-edit      */
    {
        return m_step_edit_note_length;
    }

    void snap (int st);
    void step_edit_note_length (int len);
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
        return armed() && (get_queued() || off_from_snap());
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

    bool modified () const
    {
        return m_is_modified;
    }

    bool is_dirty_main () const;
    bool is_dirty_edit () const;
    bool is_dirty_perf () const;
    bool is_dirty_names () const;
    void set_dirty_mp ();
    void set_dirty ();
    std::string channel_string () const;            /* "F" or "<channel+1>" */
    bool set_channels (int channel);                /* modifies event list  */

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
    void live_play (midipulse tick);
    void play_queue (midipulse tick, bool playbackmode, bool resume);
    bool push_add_note
    (
        midipulse tick, midipulse len, int note,
        bool repaint = false,
        int velocity = sm_preserve_velocity
    );
    bool push_add_chord
    (
        int chord, midipulse tick, midipulse len,
        int note, int velocity = sm_preserve_velocity
    );
    bool add_painted_note
    (
        midipulse tick, midipulse len, int note,
        bool repaint = false,
        int velocity = sm_preserve_velocity
    );
    bool add_note (midipulse len, const event & e);
    bool add_chord
    (
        int chord, midipulse tick, midipulse len, int note,
        int velocity = sm_preserve_velocity
    );
    bool add_tempo (midipulse tick, midibpm tempo, bool repaint = false);
    bool add_tempos
    (
        midipulse tick_s, midipulse tick_f,
        int tempo_s, int tempo_f
    );
    bool log_time_signature (midipulse tick, int beats, int width);
    bool add_timesig_event (const event & e, bool main_ts = false);
    bool add_c_timesig (int bpb, int bw, bool main_ts = false);
    bool delete_time_signature (midipulse tick);
    bool detect_time_signature
    (
        midipulse & tstamp, int & numerator, int & denominator,
        midipulse start = 0,
        midipulse limit = c_null_midipulse
    );
    bool add_event (const event & er);      /* another one declared below */
    bool add_event
    (
        midipulse tick, midibyte status,
        midibyte d0, midibyte d1, bool repaint = false
    );
    bool append_event (const event & er);
    void sort_events ();
    event find_event (const event & e, bool nextmatch = false);
    note_info find_note (midipulse tick, int note);
    bool remove_duplicate_events (midipulse tick, int note = (-1));
    void notify_change (bool userchange = true);
    void notify_trigger ();
    void print_triggers () const;
    bool add_trigger
    (
        midipulse tick, midipulse len,
        midipulse offset    = 0,
        midibyte tpose      = 0,
        bool adjust_offset  = true
    );
    bool split_trigger (midipulse tick, trigger::splitpoint splittype);
    bool grow_trigger (midipulse tick_from, midipulse tick_to, midipulse len);
    bool grow_trigger (midipulse tick_from, midipulse tick_to);
    const trigger & find_trigger (midipulse tick) const;
    bool delete_trigger (midipulse tick);
    bool clear_triggers ();
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
    bool cut_selected_triggers ();
    bool copy_selected_triggers ();
    bool paste_trigger (midipulse paste_tick = c_no_paste_trigger);
    bool move_triggers
    (
        midipulse start_tick, midipulse distance,
        bool direction, bool single = true
    );
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
    void copy_triggers (midipulse start_tick, midipulse distance);

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

    bussbyte seq_midi_in_bus () const
    {
        return m_nominal_in_bus;
    }

    bussbyte true_in_bus () const
    {
        return m_true_in_bus;
    }

    bool has_in_bus () const
    {
        return is_good_buss(m_true_in_bus);
    }

    bool set_master_midi_bus (const mastermidibus * mmb);
    bool set_midi_bus (bussbyte mb, bool user_change = false);
    bool set_midi_channel (midibyte ch, bool user_change = false);
    bool set_midi_in_bus (bussbyte mb, bool user_change = false);
    int select_note_events
    (
        midipulse tick_s, int note_h,
        midipulse tick_f, int note_l, eventlist::select action
    );
    int select_events
    (
        midipulse tick_s, midipulse tick_f,
        midibyte astatus, midibyte cc, eventlist::select action
    );
    int select_events
    (
        midibyte astatus, midibyte cc, bool inverse = false
    );
    int select_event_handle
    (
        midipulse tick_s, midipulse tick_f,
        midibyte astatus, midibyte cc,
        midibyte data
    );
    void adjust_event_handle (midibyte astatus, midibyte data);

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
    void select_by_channel (int channel);
    void select_notes_by_channel (int channel);
    void unselect ();
    bool repitch (const notemapper & nmap, bool all = false);
    bool copy_selected ();
    bool cut_selected (bool copyevents = true);
    bool paste_selected (midipulse tick, int note);
    bool merge_events (const sequence & source);
    bool selected_box
    (
        midipulse & tick_s, int & note_h,
        midipulse & tick_f, int & note_l
    );
    bool onsets_selected_box
    (
        midipulse & tick_s, int & note_h,
        midipulse & tick_f, int & note_l
    );
    bool clipboard_box
    (
        midipulse & tick_s, int & note_h,
        midipulse & tick_f, int & note_l
    );
    midipulse clip_timestamp (midipulse ontime, midipulse offtime);
    bool move_selected_notes (midipulse deltatick, int deltanote);
    bool move_selected_events (midipulse deltatick);
    bool stream_event (event & ev);
    bool change_event_data_range
    (
        midipulse tick_s, midipulse tick_f,
        midibyte status, midibyte cc,
        int d_s, int d_f, bool finalize = false
    );
    bool change_event_data_relative
    (
        midipulse tick_s, midipulse tick_f,
        midibyte status, midibyte cc,
        int newval, bool finalize = false
    );
    void change_event_data_lfo
    (
        double dcoffset, double range, double speed, double phase,
        waveform w, midibyte status, midibyte cc, bool usemeasure = false
    );
    bool fix_pattern (fixparameters & param);   /* for qpatternfix dialog   */
    void increment_selected (midibyte status, midibyte /*control*/);
    void decrement_selected (midibyte status, midibyte /*control*/);
    bool grow_selected (midipulse deltatick);
    bool stretch_selected (midipulse deltatick);
    bool randomize (midibyte status, int range = (-1), bool all = false);
    bool randomize_note_velocities (int range = (-1), bool all = false);
    bool randomize_note_pitches (int range = (-1), bool all = false);
    bool jitter_notes (int jitter = (-1), bool all = false);
    bool mark_selected ();
    void unpaint_all ();
    bool verify_and_link (bool wrap = false);
    bool edge_fix ();
    bool remove_unlinked_notes ();

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
    bool clear_events ();
    void draw_lock () const;
    void draw_unlock () const;

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
        event::buffer::const_iterator & evi
    );
    bool get_next_meta_match
    (
        midibyte metamsg,
        event::buffer::const_iterator & evi,
        midipulse start = 0,
        midipulse range = c_null_midipulse
    );
    bool get_next_event
    (
        midibyte & status, midibyte & cc,
        event::buffer::const_iterator & evi
    );
    bool next_trigger (trigger & trig);
    bool push_quantize (midibyte status, midibyte cc, int divide);
    bool push_quantize_notes (int divide);
    bool push_jitter_notes (int range = -1);
    bool transpose_notes (int steps, int scale, int key = 0);

#if defined SEQ66_SEQ32_SHIFT_SUPPORT
    void shift_notes (midipulse ticks);
#endif

    midibyte musical_key () const
    {
        return m_musical_key;
    }

    midibyte musical_scale () const
    {
        return m_musical_scale;
    }

    int background_sequence () const
    {
        return int(m_background_sequence);
    }

    void musical_key (int key, bool user_change = false);
    void musical_scale (int scale, bool user_change = false);
    bool background_sequence (int bs, bool user_change = false);
    void show_events () const;
    bool copy_events (const eventlist & newevents);
    midipulse unit_measure (bool reset = false) const;
    midipulse expand_threshold () const;
    midipulse expand_value ();

    /**
     *  The master bus needs to know if the match feature is truly in force,
     *  otherwise it must pass the incoming events to all recording sequences.
     *  Compare this function to channels_match().
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
        sm_clipboard.clear();                   /* shared between sequences */
    }

    static recordstyle loop_record_style (int ri);

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

    bool remove_selected ();
    bool remove_marked ();                      /* a forwarding function    */
    bool update_recording (int index);
    bool remove_orphaned_events ();

private:

    bool flatten (sequence & destseq, bool maketrigger = true);
    midipulse flatten_trigger
    (
        sequence & destseq,
        const trigger & trig,
        midipulse prev_timestamp
    );

protected:

    void set_parent (performer * p);

    void armed (bool flag)
    {
        m_armed = flag;
    }

    void free_channel (bool flag)
    {
        m_free_channel = flag;
    }

private:

    midipulse apply_time_factor
    (
        double factor,
        bool savenotelength = false,
        bool relink = false
    );

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

    bool check_oneshot_recording ();
    bool quantize_events (midibyte status, midibyte cc, int divide = 1);
    bool quantize_notes (int divide = 1);
    bool change_ppqn (int p);
    void put_event_on_bus (const event & ev);
    void set_trigger_offset (midipulse trigger_offset);
    void adjust_trigger_offsets_to_length (midipulse newlen);
    midipulse adjust_offset (midipulse offset);
    draw get_note_info                      /* used only internally     */
    (
        note_info & niout,
        event::buffer::const_iterator & evi
    ) const;

    timesig default_time_signature () const;
    void push_default_time_signature ();

#if defined USE_SEQUENCE_REMOVE_EVENTS
    void remove (event::buffer::iterator i);
    void remove (event & e);
#endif

    bool remove_first_match (const event & e, midipulse starttick = 0);
    bool remove_all ();

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

    void draw_locked (bool flag)
    {
        m_draw_locked = flag;
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

    void channel_match (bool flag)
    {
        m_channel_match = flag;
    }

};          // class sequence

}           // namespace seq66

#endif      // SEQ66_SEQUENCE_HPP

/*
 * sequence.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


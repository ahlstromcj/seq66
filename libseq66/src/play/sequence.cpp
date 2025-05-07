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
 * \file          sequence.cpp
 *
 *  This module declares/defines the base class for handling the data and
 *  management of patterns/sequences.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2025-05-07
 * \license       GNU GPLv2 or above
 *
 *  The functionality of this class also includes handling some of the
 *  operations of pattern triggers.
 *
 *  We had added null-pointer checks for the master MIDI buss pointer, but
 *  these just take up needless time, in most cases.
 *
 *  Provides an option to save the Time Signature and Tempo data that may be
 *  present in a MIDI file (in the first track) in the sequence object, and
 *  write them back to the MIDI file when saved again, in Seq66 format.
 *  The SeqSpec events that Seq24 and Seq66 save for these "events" are
 *  not readable by other MIDI applications, such as QTractor.  So other
 *  sequencers can read the correct time-signature and tempo values.
 *
 * \note
 *      We leave a small gap in various functions where mark_selected() locks
 *      and unlocks, then we lock again.  This should only be an issue if
 *      altering selected notes while recording.  We will test this at some
 *      point, and add better locking coverage if necessary.
 */

#include <cstring>                      /* std::memset()                    */
#include <cmath>                        /* std::trunc()                     */

#include "cfg/settings.hpp"             /* seq66::rc() and usr()            */
#include "cfg/scales.hpp"               /* key and scale constants          */
#include "midi/mastermidibus.hpp"       /* seq66::mastermidibus             */
#include "midi/midibus.hpp"             /* seq66::midibus                   */
#include "play/notemapper.hpp"          /* seq66::notemapper                */
#include "play/performer.hpp"           /* seq66::performer                 */
#include "play/sequence.hpp"            /* seq66::sequence                  */
#include "os/timing.hpp"                /* seq66::microsleep()              */
#include "util/palette.hpp"             /* seq66::palette_to_int(), colors  */
#include "util/strfunctions.hpp"        /* bool_to_string()                 */

namespace seq66
{

/**
 *  This value is used as the minimal increment for growing a trigger during
 *  song-recording.  This value was originally 10, but let's use a power of 2.
 *  This increment allows the rest of the threads to notice the change.
 */

static const int c_song_record_incr = 16;
static const int c_maxbeats         = 0xFFFF;

/**
 *  The c_handlesize value is an internal variable for handle size.  Note
 *  that, with the default PPQN of 192, a sixteenth note (a typical snap
 *  value) is 48 pulses (ticks), so that a sixteenth note is broken into equal
 *  left, center, and right sides.  However, for a PPQN of, say, 960, 16
 *  pulses is 5 times smaller in width.  We really need to scale the handle
 *  size.
 */

static const midipulse c_handlesize = 16;

/**
 *  Static members for validating scale factors in pattern compression and
 *  expanding.
 */

static const double c_scale_min     =    0.01;
static const double c_scale_max     =  200.00;
static const double c_measure_max   = 1000.00;

/**
 *  The divisor for detecting when to reset auto-step. The original value
 *  was 2. Let's try something else. Maybe 8 would work, too.
 */

static const midipulse c_reset_divisor = 4;

/*
 * Member value.  A fingerprint size of 0 means to not use a fingerprint...
 * display the whole track in the progress box, no matter how long.
 */

int sequence::sm_fingerprint_size   = 0;

/*
 * Member for convenience.
 */

short sequence::sm_preserve_velocity;

/**
 *  Provides the default name/title for the sequence.
 */

const std::string sequence::sm_default_name = "Untitled";

/**
 *  A static clipboard for holding pattern/sequence events.  Being static
 *  allows for copy/paste between patterns.  Please note that this is used
 *  only for selected events.  For whole patterns, see the sequence object
 *  performer::m_seq_clipboard.
 */

eventlist sequence::sm_clipboard;

/**
 *  Shows the note_info values. Purely for dev trouble-shooting.
 */

void
sequence::note_info::show () const
{
    printf
    (
        "note_info %d: ticks %ld to %ld, velocity %d\n",
        ni_note, long(ni_tick_start), long(ni_tick_finish), ni_velocity
    );
}

/**
 *  Principal constructor.
 *
 * \param ppqn
 *      Provides the PPQN parameter to perhaps alter the default PPQN value of
 *      this sequence.
 */

sequence::sequence (int ppqn) :
    m_parent                    (nullptr),      /* set when seq installed   */
    m_events                    (),
    m_triggers                  (*this),
    m_time_signatures           (),
    m_events_undo_hold          (),
    m_have_undo                 (false),
    m_have_redo                 (false),
    m_events_undo               (),
    m_events_redo               (),
    m_channel_match             (false),
    m_midi_channel              (0),            /* null_channel() better?   */
    m_free_channel              (false),
    m_nominal_bus               (0),            /* out buss default value   */
    m_true_bus                  (null_buss()),
    m_nominal_in_bus            (null_buss()),  /* optional input buss no.  */
    m_true_in_bus               (null_buss()),
    m_song_mute                 (false),
    m_transposable              (true),
    m_notes_on                  (0),
    m_master_bus                (nullptr),
    m_playing_notes             (),
    m_armed                     (false),
    m_recording                 (false),
    m_draw_locked               (false),
    m_recording_style           (usr().pattern_record_style()),
    m_record_alteration         (usr().record_alteration()),
    m_thru                      (false),
    m_queued                    (false),
    m_one_shot                  (false),
    m_one_shot_tick             (0),
    m_loop_count_max            (0),
    m_off_from_snap             (false),
    m_song_playback_block       (false),
    m_song_recording            (false),
    m_song_recording_snap       (true),
    m_song_record_tick          (0),
    m_loop_reset                (false),
    m_unit_measure              (0),
    m_dirty_main                (true),
    m_dirty_edit                (true),
    m_dirty_perf                (true),
    m_dirty_names               (true),
    m_is_modified               (false),
    m_seq_in_edit               (false),
    m_status                    (0),
    m_cc                        (0),
    m_name                      (),
    m_last_tick                 (0),
    m_queued_tick               (0),
    m_trigger_offset            (0),
    m_maxbeats                  (c_maxbeats),
    m_ppqn                      (choose_ppqn(ppqn)),
    m_seq_number                (unassigned()),
    m_seq_color                 (c_seq_color_none),
    m_seq_edit_mode             (sequence::editmode::note),
    m_length                    (4 * midipulse(m_ppqn)),  /* 1 bar of ticks */
    m_next_boundary             (0),
    m_measures                  (0),
    m_snap_tick                 (int(m_ppqn) / 4),
    m_step_edit_note_length     (int(m_ppqn) / 4),
    m_time_beats_per_measure    (0),
    m_time_beat_width           (0),
    m_clocks_per_metronome      (24),
    m_32nds_per_quarter         (8),
    m_us_per_quarter_note       (tempo_us_from_bpm(usr().bpm_default())),
    m_rec_vol                   (usr().preserve_velocity()),
    m_note_on_velocity          (usr().note_on_velocity()),
    m_note_off_velocity         (usr().note_off_velocity()),
    m_musical_key               (usr().seqedit_key()),
    m_musical_scale             (usr().seqedit_scale()),
    m_background_sequence       (usr().seqedit_bgsequence()),
    m_mutex                     ()
{
    sm_preserve_velocity = usr().preserve_velocity();
    sm_fingerprint_size = usr().fingerprint_size();
    m_events.set_length(m_length);
    m_events.zero_len_correction(m_snap_tick / 2);
    m_triggers.set_ppqn(int(m_ppqn));
    m_triggers.set_length(m_length);
    for (auto & p : m_playing_notes)            /* no notes playing now     */
        p = 0;
}

/**
 *  A rote destructor.
 */

sequence::~sequence ()
{
    // Empty body
}

/**
 *  A convenience function that we have to put here so that the m_parent
 *  pointer can be used without an additional include-file in the sequence.hpp
 *  module.  One minor issue is how can we unmodify the performance?  We'd
 *  need to keep a count/stack of modifications over all sequences in the
 *  performance.  Probably not practical, in general.  We will probably keep
 *  track of the modification of the buss (port) and channel numbers, as per
 *  GitHub Issue #47.
 *
 * Issue #19: Crash when recording note.
 *
 *  The notify_change() call eventually causes the "64" version of the edit
 *  frame to crash, and also makes it do a lot of unnecessary rebuilding of
 *  the grid buttons.  So we revert to the original call.  There's a chance
 *  this might cause updates to be missed, but that's a lesser issue than a
 *  segfault.  But now we have added a feature that a complete recreation
 *  requires a performer::change::recreate value; the default is
 *  performer::change::yes.
 *
 *  Note that now we don't call performer::modify(), now we call its
 *  notification function for sequence-changes, which notifies all subscribers
 *  and also calls modify().
 *
 *  Lastly, the metronome pattern (#2047) is set up programmatically, and
 *  we will rebuild it if its configuration is changed on the fly. So
 *  no flag-raising needed.
 *
 * \param notifychange
 *      If true (the default), then notification is done (via a
 *      performer::callbacks function).
 */

void
sequence::modify (bool notifychange)
{
    if (is_normal_seq())                /* currently, a seq-number < 1024   */
    {
        m_is_modified = true;
        set_dirty();
        if (notifychange)
            notify_change();
    }
}

/**
 *  A cut-down version of principal assignment operator or copy constructor.
 *
 * \threadsafe
 *
 * \param rhs
 *      Provides the source of the new member values.
 */

void
sequence::partial_assign (const sequence & rhs, bool toclipboard)
{
    if (this != &rhs)
    {
        automutex locker(m_mutex);
        m_parent                    = rhs.m_parent;         /* a pointer    */
        m_events                    = rhs.m_events;         /* container!   */
        m_triggers                  = rhs.m_triggers;       /* 2021-07-27   */

        /*
         *  The triggers class has a parent that cannot be reassigned.
         *  Is this an issue?  We don't care about copying undo/redo containers.
         *
         *  m_events_undo_hold
         *  m_have_undo
         *  m_have_redo
         *  m_events_undo
         *  m_events_redo
         */

        m_channel_match             = rhs.m_channel_match;
        m_midi_channel              = rhs.m_midi_channel;
        m_free_channel              = rhs.m_free_channel;
        m_nominal_bus               = rhs.m_nominal_bus;
        m_true_bus                  = rhs.m_true_bus;
        m_nominal_in_bus            = rhs.m_nominal_in_bus;
        m_true_in_bus               = rhs.m_true_in_bus;
        m_song_mute                 = rhs.m_song_mute;
        m_transposable              = rhs.m_transposable;
        m_notes_on                  = 0;
        m_master_bus                = rhs.m_master_bus;     /* a pointer    */
        m_unit_measure              = rhs.m_unit_measure;
        m_name                      = rhs.m_name;
        m_ppqn                      = rhs.m_ppqn;

        /*
         *  These values are set fine for this purpose by the constructor
         *
         *  m_playing_notes
         *  m_armed
         *  m_recording
         *  m_draw_locked
         *  m_expanded_recording
         *  m_overwrite_recording
         *  m_oneshot_recording
         *  m_record_alteration
         *  m_thru
         *  m_queued
         *  m_soloed
         *  m_one_shot
         *  m_one_shot_tick
         *  m_loop_count_max
         *  m_off_from_snap
         *  m_song_playback_block
         *  m_song_recording
         *  m_song_recording_snap
         *  m_song_record_tick
         *  m_loop_reset
         *  m_dirty_main
         *  m_dirty_edit
         *  m_dirty_perf
         *  m_dirty_names
         *  m_seq_in_edit
         *  m_status
         *  m_cc
         *  m_last_tick
         *  m_queued_tick
         *  m_trigger_offset
         *  m_maxbeats
         *  m_seq_number
         *  m_mutex
         */

        m_seq_color                 = rhs.m_seq_color;
        m_seq_edit_mode             = rhs.m_seq_edit_mode;
        m_length                    = rhs.m_length;
        m_snap_tick                 = rhs.m_snap_tick;
        m_step_edit_note_length     = rhs.m_step_edit_note_length;
        m_time_beats_per_measure    = rhs.m_time_beats_per_measure;
        m_time_beat_width           = rhs.m_time_beat_width;
        m_clocks_per_metronome      = rhs.m_clocks_per_metronome;
        m_32nds_per_quarter         = rhs.m_32nds_per_quarter;
        m_us_per_quarter_note       = rhs.m_us_per_quarter_note;
        m_rec_vol                   = rhs.m_rec_vol;
        m_note_on_velocity          = rhs.m_note_on_velocity;
        m_note_off_velocity         = rhs.m_note_off_velocity;
        m_musical_key               = rhs.m_musical_key;
        m_musical_scale             = rhs.m_musical_scale;
        m_background_sequence       = rhs.m_background_sequence;
        for (auto & p : m_playing_notes)            /* no notes playing now */
            p = 0;

        m_last_tick = 0;                            /* reset to tick 0      */
        (void) verify_and_link();                   /* NoteOn <---> NoteOff */
        if (! toclipboard)
            modify();
    }
}

/*
 *  These two functions are an attempt to remove a seqfault that can occur
 *  in qseqdata, qseqroll, qloopbutton, etc. when processing multiple
 *  inputs hitting keys wildly.  May need to update all event-drawing GUI
 *  classes if the segfaults keep occuring.
 */

void
sequence::draw_lock () const
{
    if (recording() && ! m_draw_locked)
    {
        m_mutex.lock();
        m_draw_locked = true;       /* mutable */
    }
}

void
sequence::draw_unlock () const
{
    if (m_draw_locked)
    {
        m_draw_locked = false;      /* mutable */
        m_mutex.unlock();
    }
}

void
sequence::musical_key (int key, bool user_change)
{
    if (legal_key(key))
    {
        bool change = key != m_musical_key;
        if (change)
        {
            m_musical_key = midibyte(key);
            if (user_change)
                modify();
        }
    }
}

void
sequence::musical_scale (int scale, bool user_change)
{
    if (legal_scale(scale))
    {
        bool change = scale != m_musical_scale;
        if (change)
        {
            m_musical_scale = midibyte(scale);
            if (user_change)
                modify();
        }
    }
}

bool
sequence::background_sequence (int bs, bool user_change)
{
    bool result = false;
    if (seq::legal(bs))
    {
        result = bs != m_background_sequence;
        if (result)
        {
            m_background_sequence = short(bs);
            if (user_change)
                modify();
        }
    }
    return result;
}

/**
 * \setter m_seq_color
 *
 * \param c
 *      Provides the index into the color-palette.  The only rules here are
 *      that -1 represents no color or a default color, and values of zero
 *      and above (to an unknown limit) represent a legal palette color.
 *
 * \param user_change
 *      If true (the default value is false), the user has decided to change
 *      this value, and we might need to modify the performer's dirty flag, so
 *      that the user gets prompted for a change.
 *
 * \return
 *      Returns true if the color actually changed.
 */

bool
sequence::set_color (int c, bool user_change)
{
    automutex locker(m_mutex);
    bool result = false;
    if (c >= 0 || c == c_seq_color_none)
    {
        if (colorbyte(c) != m_seq_color)
        {
            m_seq_color = colorbyte(c);
            result = true;
            if (user_change)
                modify();                   /* no easy way to undo this     */
        }
    }
    return result;
}

bool
sequence::loop_count_max (int m, bool user_change)
{
    automutex locker(m_mutex);
    bool result = false;
    if (m >= 0 && m != m_loop_count_max)
    {
        m_loop_count_max = m;
        if (user_change)
            result = true;
    }
    if (result)
        modify();                                   /* have pending changes */

    return result;
}

/**
 *  If empty, sets the color to classic Sequencer64 yellow.  Called by
 *  performer when installing a sequence.
 */

void
sequence::empty_coloring ()
{
    if (event_count() == 0 && m_seq_color == c_seq_color_none)
        (void) set_color(palette_to_int(PaletteColor::yellow));
}

bool
sequence::clear_events ()
{
    automutex locker(m_mutex);
    bool result = ! m_events.empty();
    if (result)
    {
        m_events.clear();
        modify();                                   /* have pending changes */
    }
    return result;
}

/**
 *  Returns the number of events stored in m_events.  Note that only playable
 *  events are counted in a sequence.  If a sequence class function provides a
 *  mutex, call m_events.count() instead.
 *
 * \threadsafe
 *
 * \return
 *      Returns m_events.count().
 */

int
sequence::event_count () const
{
    automutex locker(m_mutex);
    return m_events.count();
}

int
sequence::note_count () const
{
    automutex locker(m_mutex);
    return m_events.note_count();
}

/**
 *  Gets the average of the notes within a snap value of the first note.
 */

bool
sequence::first_notes (midipulse & ts, int & n) const
{
    automutex locker(m_mutex);
    return m_events.first_notes(ts, n, m_snap_tick);
}

int
sequence::playable_count () const
{
    automutex locker(m_mutex);
    return m_events.playable_count();
}

bool
sequence::is_playable () const
{
    automutex locker(m_mutex);
    return m_events.is_playable();
}

/*-------------------------------------------------------------------------
 * Experimental time-signature functions
 *-------------------------------------------------------------------------*/

/**
 *  Here, we count the time-signatures, if any, in the pattern. If there
 *  are not any, then we create one representing the default beats and beat
 *  width and push it on the time-signatures stack. Apart from this
 *  possible default one, only actual existing time-signature events are saved
 *  in the pattern. This stack is used only in drawing in the panes of the
 *  pattern editor. It is not saved with the pattern.
 *
 *  What if there is no beginning time signature, but ones deeper into the
 *  pattern? We detect this and add a default one at the beginning. It gets
 *  tweaked later in the process unless it is the only one.
 *
 *  Note that this function assumes there are not two time-signature events
 *  at the same timestamp. If there are, one will be ignored. Unlikely to
 *  be a big issue.
 *
 *  Currently the seqedit calls this function. Should the event editor
 *  also call it? Yes, because it needs an up-to-date list of time signatures
 *  in order to add events properly.
 *
 * \return
 *      Returns true if a true time-signature was found. If false, then
 *      the only time-signature saved will be the default one.
 */

bool
sequence::analyze_time_signatures ()
{
    bool result = false;
    midipulse start = 0;
    midipulse limit = snap() / 2;   /* allow some slop at the beginning    */
    bool found = false;
    int count = 0;
    int ppq = get_ppqn();
    m_time_signatures.clear();
    for (auto cev = cbegin(); ! cend(cev); ++cev)
    {
        if (get_next_meta_match(EVENT_META_TIME_SIGNATURE, cev, start))
        {
            midipulse ts = cev->timestamp();
            if (count == 0 && ts > limit)
            {
                push_default_time_signature();  /* ensure one at the start  */
                ++count;
            }

            timesig t;                          /* push a real time-sig     */
            t.sig_start_measure = 0.0;          /* calculated later         */
            t.sig_measures = 0.0;               /* ditto                    */
            t.sig_beats_per_bar = int(cev->get_sysex(0));
            t.sig_beat_width = beat_power_of_2(int(cev->get_sysex(1)));
            t.sig_ticks_per_beat = pulses_per_beat(ppq, t.sig_beat_width);
            t.sig_start_tick = ts;
            t.sig_end_tick = 0;                 /* tritto                   */
            m_time_signatures.push_back(t);
            start = ts + 1;                     /* better than ++start;     */
            ++count;
            found = result = true;
        }
        else
            break;
    }
    if (! found)
    {
        push_default_time_signature();
        found = true;
    }
    if (found)
    {
        /*
         * Calculate the "calculate later" values here. These will save
         * valuable time in drawing.
         */

        size_t sz = m_time_signatures.size();
        if (sz > 1)
        {
            size_t count = 0;
            double lastmeasure = 1.0;   /* always at least one measure, #1  */
            for (auto & t : m_time_signatures)
            {
                midipulse ender = count < (sz - 1) ?
                    m_time_signatures[count + 1].sig_start_tick : get_length() ;

                t.sig_end_tick = ender;
                ender -= t.sig_start_tick;

                double mcurrent = pulses_to_measures
                (
                    ender, ppq, t.sig_beats_per_bar, t.sig_beat_width
                );
                t.sig_start_measure = lastmeasure;
                t.sig_measures = mcurrent;
                t.sig_ticks_per_beat = pulses_per_beat(ppq, t.sig_beat_width);
                lastmeasure += mcurrent;
                ++count;
            }
        }
        else                            /* 1 time signature for whole song  */
        {
            auto & t = m_time_signatures[0];
            t.sig_start_measure = 1;
            t.sig_measures = measures();
            t.sig_end_tick = get_length();
        }
    }
    return result;
}

/**
 *  Makes the default time signature.
 */

sequence::timesig
sequence::default_time_signature () const
{
    timesig t;
    t.sig_start_measure = 0.0;
    t.sig_measures = 0.0;
    t.sig_beats_per_bar = m_time_beats_per_measure;
    t.sig_beat_width = m_time_beat_width;
    t.sig_ticks_per_beat = pulses_per_beat(get_ppqn(), m_time_beat_width);
    t.sig_start_tick = t.sig_end_tick = 0;
    return t;
}

/**
 *  Pushes a default time-signature based on the beats/bar and beat width set
 *  for the pattern. The extent and measure are calculated at the end of the
 *  time-signature analysis stage.
 */

void
sequence::push_default_time_signature ()
{
    timesig t = default_time_signature();
    m_time_signatures.push_back(t);
}

const sequence::timesig &
sequence::get_time_signature (size_t index) const
{
    static timesig s_ts_dummy;                  /* { 0.0, 0.0, 0, 0, ... }; */
    static bool s_uninitialized = true;
    if (s_uninitialized)
    {
        s_ts_dummy = default_time_signature();
        s_uninitialized = false;
    }
    return index < m_time_signatures.size() ?
        m_time_signatures[index] : s_ts_dummy ;
}

/**
 *  Do we want to call analyze_time_signatures() here? Probably better to
 *  let the caller do it.
 *
 * \param p
 *      Provides the current time in ticks (pulses).
 *
 * \param [out] beats
 *      The value to which the time-signatures beats/pbar is passed if the
 *      function succeeds.
 *
 * \param [out] beatwidth
 *      The value to which the time-signatures beat width is passed if the
 *      function succeeds.
 *
 * \return
 *      Returns true if a time-signature was found.
 */

bool
sequence::current_time_signature
(
    midipulse p, int & beats, int & beatwidth
) const
{
    bool result = false;
    int count = time_signature_count();
    if (count > 0)
    {
        for (int i = 1; i < count; ++i)
        {
            const timesig & current = get_time_signature(i);
            midipulse p0 = current.sig_start_tick;
            midipulse p1 = current.sig_end_tick;
            if (p >= p0 && p < p1)
            {
                beats = current.sig_beats_per_bar;
                beatwidth = current.sig_beat_width;
                result = true;
                break;
            }
        }
    }
    else
    {
        beats = get_beats_per_bar();
        beatwidth = get_beat_width();
        result = true;
    }
    return result;
}

/**
 *  Finds the current measure number for a tick based on the list of
 *  time-signatures. Although it is possible for a time-signature to last
 *  for a non-integral number of measures, here we force the value to
 *  be integral.
 *
 *  This function is meant to be used in the time-lines of the pattern or song
 *  editors.
 *
 *  If the tick is past the start of the last time-signature (there is always
 *  one, the default time signature), then we get the duration since that start,
 *  convert it to a measure value, then add it to the firts measure of the last
 *  time-signature.
 *
 *  Note: for speed, we should use the old method, just incrementing the measure.
 *
 * \param p
 *      Provides the tick for which we want to get the measure it it in.
 *
 * \return
 *      Returns the rounded up integer version of the measure.
 */

int
sequence::measure_number (midipulse p) const
{
    int result = 0;
    int count = time_signature_count();
    if (count > 0)
    {
        int ppq = get_ppqn();
        for (int i = 0; i < count; ++i)
        {
            const timesig & t = get_time_signature(i);
            midipulse p0 = t.sig_start_tick;
            if (p >= p0)
            {
                midipulse p1 = t.sig_end_tick;
                midipulse duration = p - p0;
                double mnew = t.sig_start_measure;
                double m = pulses_to_measures
                (
                    duration, ppq, t.sig_beats_per_bar,
                    t.sig_beat_width
                );
                result += int(mnew + m + 0.5);      /* round up for now */
                if (p >= p1)
                {
                    /*
                     * We're past the last time-signature event. See banner.
                     */

                    break;
                }
            }
        }
    }
    else
        result = measures();

    return result;
}

/**
 *  Converts a "B:B:T" string to pulses, given that the analysis, if done,
 *  added time signatures. Compare it to seq66::measurestring_to_pulses()
 *  or string_to_pulses().
 *
 *  -   We have a string such as "4:l:000".
 *      -   Fill a midi_measures structure from that string.
 *          -   Extract the 4 (measures).
 *          -   Extract the 1 (beat).
 *          -   Extract the ticks.
 *      -   Iterate the time-signatures to get to the one where
 *          measure >= t.sig_start_measure and if not at the end
 *          where measure < t+1.sig_start_measure.
 *          -   Set pulses = t.sig_start_tick.
 *          -   Get the measure size, 
 *          -   Get the percentage of the measure size:
 *              percent = beat/t.sig_beats_per_bar as doubles;
 *          -   pulsesbeat = percent * (t.sig_end_tick - t.sig_start_tick);
 *          -   pulses += pulsebeat
 *          -   pulses += ticks.
 *      -   If after the last time-signature change:
 *
 *  After completion, a re-analysis will be required.
 */

midipulse
sequence::time_signature_pulses (const std::string & s) const
{
    midipulse result = 0;
    midi_measures mm = string_to_measures(s);   /* measures, beats, ticks   */
    int count = time_signature_count();
    if (count > 0)
    {
        double mtarget = double(mm.measures());
        bool got_it = false;
        for (int i = 0; i < count; ++i)
        {
            const timesig & t0 = get_time_signature(i);
            double m0 = t0.sig_start_measure;
            double m1;
            if (i < (count - 1))
            {
                const timesig & t1 = get_time_signature(i + 1);
                m1 = t1.sig_start_measure;
                if (mtarget >= m0 && mtarget < m1)
                    got_it = true;
            }
            else    /* target measure is after last time-signature change.  */
                got_it = true;

            if (got_it)
            {
                /*
                 * Some minor updates needed to fix inserting, e.g.,
                 * Program events, with the proper timestamp.
                 */

                midibpm bpminute = perf()->get_beats_per_minute();
                int bpb = t0.sig_beats_per_bar;
                int bw = t0.sig_beat_width;
                midi_timing mt{bpminute, bpb, bw, get_ppqn()};
                midipulse mticks = midi_measures_to_pulses(mm, mt);
                result = t0.sig_start_tick + mticks;
                break;
            }
        }
    }
    else
    {
        /*
         * Convert midi_measures to midi_timing, which has beats/bar and width
         * elements, and adds beats-per-minute and PPQN.
         */

        midibpm bpminute = perf()->get_beats_per_minute();
        int bpb = get_beats_per_bar();
        int bwidth = get_beat_width();
        midi_timing mt{bpminute, bpb, bwidth, get_ppqn()};
        result = seq66::string_to_pulses(s, mt);
    }
    return result;
}

/**
 *  Rescales the eventlist, then sets the pattern length to the result.
 */

midipulse
sequence::apply_time_factor
(
    double factor,
    bool savenotelength,
    bool relink
)
{
    midipulse result = m_events.apply_time_factor
    (
        factor, savenotelength, relink
    );
    if (result > 0)
        (void) set_length(result);          /* triggers, verify defaults    */

    return result;
}

/*-------------------------------------------------------------------------
 * Undo/redo functions
 *-------------------------------------------------------------------------*/

/**
 *  Pushes the event-list into the undo-list or the upcoming undo-hold-list.
 *
 * \threadsafe
 *
 * \param hold
 *      A new parameter for the stazed undo/redo support, not yet used.
 *      If true (the default is false), then the events go into the
 *      undo-hold-list.
 */

void
sequence::push_undo (bool hold)
{
    automutex locker(m_mutex);
    if (hold)
        m_events_undo.push(m_events_undo_hold);     /* stazed   */
    else
        m_events_undo.push(m_events);

    set_have_undo();                                /* stazed   */
}

/**
 *  Do not modify the performer here!  First, just because we push-undo does
 *  not mean a change will occur.  Second, we are now checking for changed
 *  sequences at exit, so do not need (as far as we know) to modify the
 *  performer at this point.
 */

void
sequence::set_have_undo ()
{
    m_have_undo = m_events_undo.size() > 0;
}

/**
 *  If there are items on the undo list, this function pushes the event-list
 *  into the redo-list, puts the top of the undo-list into the event-list, pops
 *  from the undo-list, calls verify_and_link(), and then calls unselect().
 *
 *  We would like to be able to set performer's modify flag to false here, but
 *  other sequences might still be in a modified state.  We could add a modify
 *  flag to sequence, and falsify that flag here.  Something to think about.
 *
 * \threadsafe
 */

void
sequence::pop_undo ()
{
    automutex locker(m_mutex);
    if (! m_events_undo.empty())
    {
        m_events_redo.push(m_events);
        m_events = m_events_undo.top();
        m_events_undo.pop();
        (void) verify_and_link();
        unselect();
    }
    set_have_undo();
    set_have_redo();
}

/**
 *  If there are items on the redo list, this function pushes the
 *  event-list into the undo-list, puts the top of the redo-list into the
 *  event-list, pops from the redo-list, calls verify_and_link(), and then
 *  calls unselect.
 *
 * \threadsafe
 */

void
sequence::pop_redo ()
{
    automutex locker(m_mutex);
    if (! m_events_redo.empty())                // move to triggers module?
    {
        m_events_undo.push(m_events);
        m_events = m_events_redo.top();
        m_events_redo.pop();
        (void) verify_and_link();
        unselect();
    }
    set_have_undo();
    set_have_redo();
}

/**
 *  Calls triggers::push_undo() with locking.
 *
 * \threadsafe
 */

void
sequence::push_trigger_undo ()
{
    automutex locker(m_mutex);
    m_triggers.push_undo();
}

/**
 *  Calls triggers::pop_undo() with locking.
 *
 * \threadsafe
 */

void
sequence::pop_trigger_undo ()
{
    automutex locker(m_mutex);
    m_triggers.pop_undo();
}

/**
 *  Calls triggers::pop_redo() with locking.
 *
 * \threadsafe
 */

void
sequence::pop_trigger_redo ()
{
    automutex locker(m_mutex);
    m_triggers.pop_redo();
}

/**
 * \setter m_master_bus
 *      Do we need to call set_dirty_mp() here?  It doesn't affect any
 *      user-interface elements.
 *
 * \threadsafe
 *
 * \param mmb
 *      Provides a pointer to the master MIDI buss for this sequence.  This
 *      should be a reference, but isn't, nor is it checked.
 */

bool
sequence::set_master_midi_bus (const mastermidibus * mmb)
{
    automutex locker(m_mutex);
    m_master_bus = const_cast<mastermidibus *>(mmb);
    return not_nullptr(mmb);
}

/**
 * \setter m_time_beats_per_measure
 *
 * \threadsafe
 *
 * \param bpb
 *      The new setting of the beats-per-bar value.
 *
 * \param user_change
 *      If true (default is false), then call the change a modification.
 *      This change can happen at load time, which is not a modification.
 */

void
sequence::set_beats_per_bar (int bpb, bool user_change)
{
    automutex locker(m_mutex);
    bool modded = false;
    if (bpb != int(m_time_beats_per_measure))
    {
        m_time_beats_per_measure = (unsigned short)(bpb);
        if (user_change)
            modded = true;

        int m = get_measures();
        if (m != m_measures)
        {
            /*
             * Adjust the markers so that R matches END after a time-signature
             * change? No, the markers are a song-wide thing and the user
             * can move "R" if desired.
             *
             *      midipulse E = perf()->get_length();          // END
             *      midipulse R = perf()->get_right_tick();      // R
             */

            m_measures = m;
            if (user_change)
                modded = true;
        }
        if (modded)
            modify();
    }
}

/**
 * \setter m_time_beat_width
 *
 * \threadsafe
 *
 * \param bw
 *      The new setting of the beat width value.
 *
 * \param user_change
 *      If true (default is false), then call the change a modification.
 *      This change can happen at load time, which is not a modification.
 */

void
sequence::set_beat_width (int bw, bool user_change)
{
    automutex locker(m_mutex);
    bool modded = false;
    if (bw != int(m_time_beat_width))
    {
        m_time_beat_width = (unsigned short)(bw);
        if (user_change)
            modded = true;

        int m = get_measures();
        if (m != m_measures)
        {
            m_measures = m;
            if (user_change)
                modded = true;
        }
        if (modded)
            modify();
    }
}

/**
 *  Used in midifile, this function replaces consecutive calls to
 *  set_beats_per_bar() and set_beat_width() with a single call that
 *  gets all the necessary information before making calculations.
 */

void
sequence::set_time_signature (int bpb, int bw)
{
    m_time_beats_per_measure = (unsigned short)(bpb);   /* get first item   */
    set_beat_width(bw, false);                          /* no user change   */
    int m = get_measures();
    m_measures = m;
}

/**
 *  Calculates and sets u = 4BP/W, where u is m_unit_measure, B is the
 *  beats/bar, P is the PPQN, and W is the beat-width. When any of these
 *  quantities change, we need to recalculate.
 *
 * \param reset
 *      If true (the default is false), make the calculateion anyway.
 *
 * \return
 *      Returns the size of a measure.
 */

midipulse
sequence::unit_measure (bool reset) const
{
    automutex locker(m_mutex);
    if (m_unit_measure == 0 || reset)
        m_unit_measure = measures_to_ticks();       /* length of 1 measure  */

    return m_unit_measure;
}

/**
 *  Changed this from a void to a boolean. Most usages still do not use the
 *  return value.  A fix for the double_length() function.
 *
 * \param measures
 *      The number of measures to add.
 *
 * \param user_change
 *      True if the change was initiated by the user, and not automatic.
 *      The default is false.
 */

bool
sequence::set_measures (int measures, bool user_change)
{
    bool modded = set_length(measures * unit_measure(true));
    if (modded)
    {
        m_measures = measures;
        if (user_change)
            modify();
    }
    return modded;
}

int
sequence::increment_measures ()
{
    int m = get_measures();
    bool ok = set_measures(m + 1);
    return ok ? m + 1 : m ;
}

/**
 *  Provides a consistent threshold value for the triggering of the
 *  addition of a measure when recording a pattern in Expand mode.
 *
 *  We want to have the threshold be a quarter of a beat, that is,
 *  unit_measure() / 4 / 4, but that setting makes the drawing occur twice, in
 *  slightly different places, and we have not figured that out yet.  So we
 *  stick with unit_measure() / 4, which is one beat.
 *
 *  \return
 *      Returns the current length of the sequence minus a beat.
 */

midipulse
sequence::expand_threshold () const
{
    return get_length() - unit_measure() / 4;
}

/**
 *  Provides the new value of the horizontal scroll bar to set when doing
 *  expanded recording.  See expand_threshold().
 *
 *  If the latest (recorded) note has a tick value greater than the threshold,
 *  then the number of measures is increment.
 *
 * \return
 *      Returns the expand_threshold() minus a unit_measure() and a quarter.
 *
 *      return expand_threshold() - (unit_measure() + unit_measure() / 4);
 *
 *      This is wasteful and might be wrong. Call it "x".
 */

midipulse
sequence::expand_value ()
{
    midipulse result = get_last_tick();
    if (result >= expand_threshold())
    {
#if defined SEQ66_PLATFORM_DEBUG_TMI
        int m = increment_measures();
        printf("expanded measures = %d\n", m);
#else
        (void) increment_measures();
        result = get_length();
#endif
    }
    else
        result = 0;

    return result;
}

/**
 *  Calculates the number of measures in the sequence based on the
 *  unit-measure and the current length, in pulses, of the sequence.  Used by
 *  seqedit.  If m_unit_measure hasn't been calculated yet, it is calculated
 *  here.
 *
 * \change ca 2017-08-15
 *      Fixed issue #106, where the measure count of a pattern kept
 *      incrementing when edited.
 *
 * \return
 *      Returns the sequence length divided by the measure length, roughly.
 *      m_unit_measure is 0.  The lowest valid measure is 1.
 */

int
sequence::calculate_measures (bool reset) const
{
    midipulse um = unit_measure(reset);
    return um > 0 ? (1 + (get_length() - 1) / um) : 1 ;
}

/**
 *  Encapsulates a calculation needed in the qseqbase class (and elsewhere).
 *  We could just assume m_unit_measures is always up-to-date and use that
 *  value.
 *
 *  Does the number of measures depend on the beat-width? What is the size of
 *  a measure in ticks? What if we change the beats-per-bar and beat-width?
 *
 *  Let's say we have a measure of notes in a 4/4 sequence. If we want to fit
 *  it into a 3/4 measure, the number of ticks is 3/4th of the original, and
 *  we'd have to rescale the notes to that new number.  If we leave the notes
 *  alone, then the measure-count increments, and in playback an space of
 *  silence is introduced.  Better to avoid changing the numerator.
 *
 *  If we change 4/4 to 4/8, then playback slows down by half.  Be aware of
 *  this feature.
 *
 * \param newlength
 *      If 0 (the default), then the current pattern length is used in the
 *      calculation of measures.  Otherwise, this parameter is used, and is
 *      useful in the process of changing the number of measures in the
 *      fix_pattern() function.
 *
 * \return
 *      Returns the whole number of measure in the specified length of the
 *      sequence.  Essentially rounds up if there is some leftover ticks.
 */

int
sequence::get_measures (midipulse newlength) const
{
    midipulse um = unit_measure();
    midipulse len = newlength > 0 ? newlength : get_length() ;
    int measures = 1;
    if (um > 0)
    {
        measures = int(len / um);
        if (len % int(um) != 0)
            ++measures;
    }
    return measures;
}

int
sequence::get_measures () const
{
    m_measures = get_measures(0);
    return m_measures;
}

/**
 * \setter m_rec_vol
 *      If this velocity is greater than zero, then m_note_on_velocity will
 *      be set as well.
 *
 * \threadsafe
 *
 * \param recvol
 *      The new setting of the recording volume setting.  It is used only if
 *      it ranges from 1 to usr().max_note_on_velocity(), or is set to
 *      the preserve-velocity (-1).
 */

void
sequence::set_rec_vol (int recvol)
{
    automutex locker(m_mutex);
    bool valid = recvol > 0 && recvol <= usr().max_note_on_velocity();
    if (! valid)
        valid = recvol == usr().preserve_velocity();

    if (valid)
    {
        m_rec_vol = short(recvol);
        if (m_rec_vol > 0)
            m_note_on_velocity = m_rec_vol;
    }
}

/**
 *  Replaces toggle_playing(), and allows play to be toggled in the same way
 *  as done by the automation function performer::loop_control().
 *  Enhances the consistency of control of the patterns.  It's a little
 *  round-about, though.
 */

bool
sequence::sequence_playing_toggle ()
{
    return perf()->sequence_playing_toggle(seq_number());
}

/**
 *  A simple version of toggle_playing().
 *
 * \return
 *      Returns true if playing.
 */

bool
sequence::toggle_playing ()
{
    return toggle_playing(perf()->get_tick(), perf()->resume_note_ons());
}

/**
 *  Toggles the playing status of this sequence.  How exactly does this differ
 *  from toggling the mute status?  The midipulse and bool parameters of the
 *  overload of this function are new, to support song-recording.
 *
 * \param tick
 *      The position from which to resume Note Ons, if appplicable. Resuming
 *      is a song-playback/song-recording feature.
 *
 * \param resumenoteons
 *      A song-recording option.  (This option, "note-resume", is stored in
 *      the "usr" file.
 */

bool
sequence::toggle_playing (midipulse tick, bool resumenoteons)
{
    set_armed(! armed());
    if (armed() && resumenoteons)
        resume_note_ons(tick);

    off_from_snap(false);
    return armed();
}

/**
 * \setter m_song_mute
 *      This function also calls set_dirty_mp() to make sure that the
 *      perfnames panel is updated to show the new mute status of the
 *      sequence.
 *
 * \param mute
 *      True if the sequence is to be muted.
 *
 *  For Seq66, this function also calls set_armed() to the opposite of the
 *  mute value.
 */

void
sequence::set_song_mute (bool mute)
{
    m_song_mute = mute;
    set_armed(! mute);
    set_dirty_mp();
}

void
sequence::toggle_song_mute ()
{
    bool mute = ! m_song_mute;
    set_song_mute(mute);
}

/**
 *  Toggles the queued flag and sets the dirty-mp flag.  Also calculates the
 *  queued tick based on m_last_tick.
 *
 *  Issue #89: This function is called only when queing is turned on! To fix
 *  this for status announcements, see set_armed() below.  Perhaps this
 *  function should merely be "set_queued()".
 *
 * \threadsafe
 */

bool
sequence::toggle_queued ()
{
    automutex locker(m_mutex);
    set_dirty_mp();
    m_queued = ! m_queued;

#if defined SEQ66_PLATFORM_DEBUG_TMI
    printf("seq %d: queuing %s\n", int(seq_number()), m_queued ? "on" : "off");
#endif

    m_queued_tick = m_last_tick - mod_last_tick() + get_length();
    off_from_snap(true);
    perf()->announce_pattern(seq_number());     /* for issue #89        */
    return true;
}

/**
 *  The play() function dumps notes starting from the given tick, and it
 *  pre-buffers ahead.  This function is called by the sequencer thread in
 *  performer.  The tick comes in as global tick.  It turns the sequence off
 *  after we play in this frame.
 *
 * \note
 *      With pause support, the progress bar for the pattern/sequence editor
 *      does what we want:  pause with the pause button, and rewind with the
 *      stop button.  Works with JACK, with issues, but we'd like to have
 *      the stop button do a rewind in JACK, too.
 *
 *  If we are playing the song data (sequence on/off triggers, we are in
 *  playback mode.  And if we are song-recording, we then keep growing the
 *  sequence's song-data triggers.
 *
 *  Note that the song_playback_block() is handled in the trigger :: play()
 *  function.  If we have reached a new chunk of drawn patterns in the song
 *  data, and we are not recording, then trigger unsets the playback block on
 *  this pattern's events.
 *
 *  The trigger calculations have been offloaded to the triggers :: play()
 *  function.  Its return value and side-effects tell if there's a change in
 *  playing based on triggers, and provides the ticks that bracket it.
 *
 * Issue #103:
 *
 *  No problem with running in ALSA.  However, if unmuted, the progress bar
 *  kept going after the loop-count.  Fixed that.
 *
 *  Running as a JACK Transport Slave, the count works the first time, but
 *  then the Master's time confuses the loop-count mechanism and the progress
 *  stops at odd spots.  Rewinding the Master (e.g. qjackctl) makes the
 *  loop-count work again.
 *
 *  Running as JACK Master, no problem with the loop-count.
 *
 *  Can we somehow reset the times-played?
 *
 * \param tick
 *      Provides the current end-tick value.  The tick comes in as a global
 *      tick.
 *
 * \param playback_mode
 *      Provides how playback is managed.  True indicates that it is
 *      performance/song-editor playback, controlled by the set of patterns
 *      and triggers set up in that editor, and saved with the song in seq66
 *      format.  False indicates that the playback is controlled by the main
 *      window, in live mode.
 *
 * \param resumenoteons
 *      A song-recording parameter.
 *
 * \threadsafe
 */

void
sequence::play
(
    midipulse tick,
    bool playback_mode,
    bool resumenoteons
)
{
    automutex locker(m_mutex);
    bool trigger_turning_off = false;       /* turn off after in-frame play */
    int trigtranspose = 0;                  /* used with c_trig_transpose   */
    midipulse start_tick = m_last_tick;     /* modified in triggers::play() */
    midipulse len = get_length() > 0 ? get_length() : m_ppqn ;

    /*
     * Issue #103. This fix allows the progress bar to behave well under
     * JACK Slave transport.
     *
     *      midipulse times_played = m_last_tick / len;
     */

    midipulse times_played = tick / len;
    m_trigger_offset = 0;                   /* from Seq24                   */
    if (m_song_mute)
    {
        set_armed(false);
    }
    else
    {
        /*
         *  We make the song_recording() clause active for both Live and Song
         *  mode now. From Sequencer64 on 2021-05-06.
         */

        if (song_recording())                       /* song-record triggers */
        {
            (void) perf()->calculate_snap(tick);  /* issue #44 redux      */

            bool added = grow_trigger(song_record_tick(), tick);
            if (added)
                notify_trigger();
        }
        if (playback_mode)                          /* song mode: triggers  */
        {
            trigger_turning_off = m_triggers.play   /* side-effects !!!     */
            (
                start_tick, tick, trigtranspose, resumenoteons
            );
        }
    }
    if (armed())                                    /* play notes in frame  */
    {
        midipulse offset = len - m_trigger_offset;
        midipulse start_tick_offset = start_tick + offset;
        midipulse end_tick_offset = tick + offset;
        midipulse offset_base = times_played * len;
        if (loop_count_max() > 0)
        {
            if (times_played >= loop_count_max())
            {
                if (is_metro_seq())                 /* count-in is complete */
                    perf()->finish_count_in();

                return;
            }
        }

        int transpose = trigtranspose;
        if (transpose == 0)
            transpose = transposable() ? perf()->get_transpose() : 0 ;

        auto e = m_events.begin();
        while (e != m_events.end())
        {
#if defined USE_NULL_EVENT_DETECTION

            /*
             * This doesn't solve the problem of hiccups when moving to the
             * next song in the playlist.
             */

            if (is_nullptr(e))
                return;
#endif
            event & er = eventlist::dref(e);
            midipulse ts = er.timestamp();
            midipulse stamp = ts + offset_base;
            if (stamp >= start_tick_offset && stamp <= end_tick_offset)
            {
                if (transpose != 0 && er.is_note()) /* includes Aftertouch  */
                {
                    event trans_event = er;         /* assign ALL members   */
                    trans_event.transpose_note(transpose);
                    put_event_on_bus(trans_event);
                }
                else
                {
                    if (er.is_tempo())
                    {
                        perf()->set_beats_per_minute(er.tempo());
                    }
                    else
                    {
                        if (er.is_ex_data())
                        {
                            if (er.is_sysex())
                                put_event_on_bus(er);   /* ca 2024-05-22    */
                        }
                        else
                            put_event_on_bus(er);   /* frame still going    */
                    }
                }
            }
            else if (stamp > end_tick_offset)
                break;                              /* frame is done        */

            ++e;                                    /* go to next event     */
            if (e == m_events.end())                /* did we hit the end ? */
            {
                e = m_events.begin();               /* yes, start over      */
                offset_base += len;              /* for another go at it */

                /*
                 * Putting this sleep here doesn't reduce the total CPU load,
                 * but it does prevent one CPU from being hammered at 100%.
                 * millisleep(1) made the live-grid progress bar jittery when
                 * unmuting shorter patterns, which play() relentlessly.
                 */

                (void) microsleep(1);
            }
        }
    }
    else
    {
        if (loop_count_max() > 0 && times_played >= loop_count_max())
            return;                                 /* issue #103           */
    }
    if (trigger_turning_off)                        /* triggers: "turn off" */
    {
        set_armed(false);
    }
    m_last_tick = tick + 1;                         /* for next frame       */

    /*
     * This causes control-output spewage during playback, but we need to
     * send an announcement when queuing is in force. At that point there
     * is a quick burst of events (all Note 2, Vel 62) we still have to figure
     * out.  They all apply to the slot being queued/unqueued. So we move this
     * code to toggle_queued().
     *
     *  if (get_queued())
     *      perf()->announce_pattern(seq_number()); // for issue #89        //
     */
}

/**
 *  This function plays without supporting song-mode, triggers, transposing,
 *  resuming notes, loop count, meta events, and song recording.  It is
 *  meant to be used for a metronome pattern.
 *
 *  Do we want to support tempo in a metronome pattern?
 *
 *  One issue is in removing the metronome, and we end up with a null event,
 *  causing a segfault.  We should just mute the metronome.
 */

void
sequence::live_play (midipulse tick)
{
    automutex locker(m_mutex);
    midipulse start_tick = m_last_tick;     /* modified in triggers::play() */
    midipulse end_tick = tick;              /* ditto                        */
    if (m_song_mute)
        set_armed(false);

    if (armed())                            /* play notes in the frame      */
    {
        midipulse len = get_length() > 0 ? get_length() : m_ppqn ;
        midipulse start_tick_offset = start_tick + len;
        midipulse end_tick_offset = end_tick + len;
        midipulse times_played = m_last_tick / len;
        midipulse offset_base = times_played * len;
        if (loop_count_max() > 0)
        {
            if (times_played >= loop_count_max())
            {
                if (is_metro_seq())                 /* count-in is complete */
                    perf()->finish_count_in();

                return;
            }
        }

        auto e = m_events.begin();
        while (e != m_events.end())
        {
            event & er = eventlist::dref(e);
            midipulse stamp = er.timestamp() + offset_base;
            if (stamp >= start_tick_offset && stamp <= end_tick_offset)
            {
#if defined SUPPORT_TEMPO_IN_LIVE_PLAY
                if (er.is_tempo())
                {
                    perf()->set_beats_per_minute(er.tempo());
                }
#endif
                put_event_on_bus(er);               /* frame still going    */
            }
            else if (stamp > end_tick_offset)
                break;                              /* frame is done        */

            ++e;                                    /* go to next event     */
            if (e == m_events.end())                /* did we hit the end ? */
            {
                e = m_events.begin();               /* yes, start over      */
                offset_base += len;                 /* for another go at it */
                (void) microsleep(1);
            }
        }
    }
    m_last_tick = end_tick + 1;                     /* for next frame       */
}

/**
 *  This function verifies state: all note-ons have a note-off, and it links
 *  note-offs with their note-ons and vice-versa.
 *
 * \threadsafe
 *
 * \param wrap
 *      Optionally (the default is false) wrap when relinking.  Can be used to
 *      override usr().pattern_wraparound().  Defaults to false.
 */

bool
sequence::verify_and_link (bool wrap)
{
    automutex locker(m_mutex);
    midipulse len = expanded_recording() ? 0 : get_length() ;
    return m_events.verify_and_link(len, wrap);
}

/**
 *  Fixes selected notes that started near the very end of the pattern, due to
 *  a clumsy keyboard artist (like the author of this module). Used by
 *  qseqroll.
 */

bool
sequence::edge_fix ()
{
    automutex locker(m_mutex);
    m_events_undo.push(m_events);                   /* push_undo(), no lock */
    bool result = m_events.edge_fix(snap(), get_length());
    if (result)
        modify();

    return result;
}

/**
 *  Removes unlinked notes.
 */

bool
sequence::remove_unlinked_notes ()
{
    automutex locker(m_mutex);
    m_events_undo.push(m_events);                   /* push_undo(), no lock */
    bool result = m_events.remove_unlinked_notes();
    if (result)
        modify();

    return result;
}

#if defined USE_SEQUENCE_REMOVE_EVENTS

/*
 * These functions are currently not used.  Might as well save some space-time.
 */

/**
 *  A helper function, which does not lock/unlock, so it is unsafe to call
 *  without supplying an iterator from the event-list.  If the event is a note
 *  off, and that note is currently playing, then send a note off before
 *  removing the note.
 *
 * \threadunsafe
 *
 * \param i
 *      Provides the iterator to the event to remove from the event list.
 */

void
sequence::remove (event::buffer::iterator evi)
{
    if (evi != m_events.end())
    {
        event & er = eventlist::dref(evi);
        if (er.is_note_off() && m_playing_notes[er.get_note()] > 0)
        {
            master_bus()->play_and_flush(m_true_bus, &er, midi_channel(er));
            --m_playing_notes[er.get_note()];
        }
        if (m_events.remove(evi))
            modify();
    }
}

/**
 *  A helper function, which does not lock/unlock, so it is unsafe to call
 *  without supplying an iterator from the event-list.
 *
 *  Finds the given event in m_events, and removes the first iterator
 *  matching that.  If there are events that would match after that, they
 *  remain in the container.  This matches seq66 behavior.
 *
 * \todo
 *      Use the find() function to find the matching event more
 *      conventionally.
 *
 * \threadunsafe
 *
 * \param e
 *      Provides a reference to the event to be removed.
 */

void
sequence::remove (event & e)
{
    if (m_events.remove_event(e))
        modify();
}

#endif  // defined USE_SEQUENCE_REMOVE_EVENTS

/**
 *  This function is use in replacing the "song info" Meta Text event.
 */

bool
sequence::remove_first_match (const event & e, midipulse starttick)
{
    automutex locker(m_mutex);
    return m_events.remove_first_match(e, starttick);
}

/**
 *  Clears all events from the event container.  Also see copy_events().
 */

bool
sequence::remove_all ()
{
    automutex locker(m_mutex);
    bool result = false;
    int count = m_events.count();
    if (count > 0)
    {
        m_events.clear();
        count = m_events.count();
        result = count == 0;
        if (result)
            modify();                       /* issue #90 */
    }
    return result;
}

/**
 *  Removes any events timestamps after the last measure. See
 *  the use of would_truncate() in qseqeditframe64.
 */

bool
sequence::remove_orphaned_events ()
{
    automutex locker(m_mutex);
    bool result = m_events.remove_trailing_events(get_length());
    if (result)
    {
        if (result)
            modify();
    }
    return result;
}

/**
 *  Removes marked events.  Before removing the events, any Note Ons are
 *  turned off, just in case.  This function forwards the call to
 *  m_event.remove_marked().
 *
 * \threadsafe
 *
 * \return
 *      Returns true if at least one event was removed.
 */

bool
sequence::remove_marked ()
{
    automutex locker(m_mutex);
    for (auto & e : m_events)
    {
        if (e.is_marked() && e.is_note_on())
            play_note_off(int(e.get_note()));
    }

    bool result = m_events.remove_marked();
    if (result)
        modify();

    return result;
}

/**
 *  Marks the selected events.
 *
 * \threadsafe
 *
 * \return
 *      Returns true if there were any events that got marked.
 */

bool
sequence::mark_selected ()
{
    automutex locker(m_mutex);
    return m_events.mark_selected();
}

/**
 *  Removes selected events.  This is a new convenience function to fold in
 *  the push_undo() and mark_selected() calls.  It makes the process slightly
 *  faster, as well.
 *
 * \threadsafe
 *      Also makes the whole process threadsafe.
 */

bool
sequence::remove_selected ()
{
    automutex locker(m_mutex);
    m_events_undo.push(m_events);               /* push_undo() without lock */

    bool result = m_events.remove_selected();
    if (result)
        modify();

    return result;
}

/**
 *  Unpaints all events in the event-list.
 *
 * \threadsafe
 */

void
sequence::unpaint_all ()
{
    automutex locker(m_mutex);
    m_events.unpaint_all();
}

/**
 *  Returns the 'box' of the selected items.  Note the common-code betweem
 *  this function and clipboard_box().  Also note we return a
 *  boolean indicating if the return values were filled in.
 *
 * \threadsafe
 *
 * \param [out] tick_s
 *      Side-effect return reference for the start time.
 *
 * \param [out] note_h
 *      Side-effect return reference for the high note.
 *
 * \param [out] tick_f
 *      Side-effect return reference for the finish time.
 *
 * \param [out] note_l
 *      Side-effect return reference for the low note.
 *
 * \return
 *      Returns true if all the values are usable.
 */

bool
sequence::selected_box
(
    midipulse & tick_s, int & note_h,
    midipulse & tick_f, int & note_l
)
{
    automutex locker(m_mutex);
    tick_s = m_maxbeats * m_ppqn;       /* the largest tick/pulse we allow  */
    tick_f = 0;                         /* the smallest tick possible       */
    note_l = c_midibyte_data_max;       /* the largest note value possible  */
    note_h = (-1);                      /* the lowest, impossible note      */
    for (auto & e : m_events)
    {
        if (e.is_selected())
        {
            midipulse time = e.timestamp();
            if (time < tick_s)
                tick_s = time;

            if (time > tick_f)
                tick_f = time;

            int note = e.get_note();
            if (note < note_l)
                note_l = note;

            if (note > note_h)
                note_h = note;
        }
    }

    bool result =
    (
        (tick_s < m_maxbeats * m_ppqn) &&
        (tick_f > 0) &&
        (note_l < c_midibyte_data_max) &&
        (note_h >= 0)
    );
    return result;
}

/**
 *  Returns the 'box' of the selected items for only Note On values.
 *  Compare to selected_box().
 *
 * \threadsafe
 *
 * \param [out] tick_s
 *      Side-effect return reference for the start time.
 *
 * \param [out] note_h
 *      Side-effect return reference for the high note.
 *
 * \param [out] tick_f
 *      Side-effect return reference for the finish time.
 *
 * \param [out] note_l
 *      Side-effect return reference for the low note.
 *
 * \return
 *      Returns true if a selected Note On event is found.
 */

bool
sequence::onsets_selected_box
(
    midipulse & tick_s, int & note_h,
    midipulse & tick_f, int & note_l
)
{
    automutex locker(m_mutex);
    bool result = false;
    tick_s = m_maxbeats * m_ppqn;
    tick_f = note_h = 0;
    note_l = c_midibyte_data_max;
    for (auto & e : m_events)
    {
        if (e.is_selected_note_on())
        {
            /*
             * We cannot check On/Off here.  It screws up seqevent selection,
             * which has no "off".
             */

            midipulse time = e.timestamp();
            if (time < tick_s)
                tick_s = time;

            if (time > tick_f)
                tick_f = time;

            int note = e.get_note();
            if (note < note_l)
                note_l = note;

            if (note > note_h)
                note_h = note;

            result = true;
        }
    }
    return result;
}

/**
 *  Returns the 'box' of the clipboard items.  Note the common-code betweem
 *  this function and selected_box().  Also note we could return a boolean
 *  indicating if the return values were filled in.
 *
 *  This function is called in qstriggereditor and in qseqroll.
 *
 * \threadsafe
 *
 * \param [out] tick_s
 *      Side-effect return reference for the start time.
 *
 * \param [out] note_h
 *      Side-effect return reference for the high note.
 *
 * \param [out] tick_f
 *      Side-effect return reference for the finish time.
 *
 * \param [out] note_l
 *      Side-effect return reference for the low note.
 */

bool
sequence::clipboard_box
(
    midipulse & tick_s, int & note_h,
    midipulse & tick_f, int & note_l
)
{
    automutex locker(m_mutex);
    bool result = false;
    tick_s = m_maxbeats * m_ppqn;
    tick_f = 0;
    note_h = 0;
    note_l = c_midibyte_data_max;
    if (sm_clipboard.empty())
    {
        tick_s = tick_f = note_h = note_l = 0;
    }
    else
    {
        result = true;                  /* FIXME */
        for (auto & e : sm_clipboard)
        {
            midipulse time = e.timestamp();
            int note = e.get_note();
            if (time < tick_s)
                tick_s = time;

            if (time > tick_f)
                tick_f = time;

            if (note < note_l)
                note_l = note;

            if (note > note_h)
                note_h = note;
        }
    }
    return result;
}

/**
 *  Counts the selected notes in the event list.
 *
 * \threadsafe
 *
 * \return
 *      Returns m_events.count_selected_notes().
 */

int
sequence::get_num_selected_notes () const
{
    automutex locker(m_mutex);
    return m_events.count_selected_notes();
}

/**
 *  Counts the selected events, with the given status, in the event list.
 *  If the event is a control change (CC), then it must also match the
 *  given CC value.
 *
 * \threadsafe
 *
 * \param status
 *      The desired kind of event to count.
 *
 * \param cc
 *      The desired control-change to count, if the event is a control-change.
 *
 * \return
 *      Returns m_events.count_selected_events().
 */

int
sequence::get_num_selected_events (midibyte status, midibyte cc) const
{
    automutex locker(m_mutex);
    return m_events.count_selected_events(status, cc);
}

/**
 *  This function selects events in range of tick start, note high, tick end,
 *  and note low.
 *
 *  Compare this function to the convenience function select_all_notes(), which
 *  doesn't use range information.
 *
 *  Note that we have not offloaded this function to eventlist because it
 *  depends on the sequence::select enumeration, and we're too lazy at the
 *  moment to move that enumeration to eventlist.
 *
 * \threadsafe
 *
 * \param tick_s
 *      The start time of the selection.
 *
 * \param note_h
 *      The high note of the selection, inclusive.
 *
 * \param tick_f
 *      The finish time of the selection.
 *
 * \param note_l
 *      The low note of the selection, inclusive.
 *
 * \param action
 *      The action to perform, one of the values of the sequence::select
 *      enumeration.
 *
 * \return
 *      Returns the number of events acted on, or 0 if no desired event was
 *      found.
 */

int
sequence::select_note_events
(
    midipulse tick_s, int note_h,
    midipulse tick_f, int note_l, eventlist::select action
)
{
    automutex locker(m_mutex);
#if defined USE_TEST_CODE
    if (expanded_recording())           // for painting notes; TEST CODE ONLY
        return 0;                       // assume no note can be selected
    else
#endif
    return m_events.select_note_events(tick_s, note_h, tick_f, note_l, action);
}

/**
 *  Select all events in the given range, and returns the number
 *  selected.  Note that there is also an overloaded version of this
 *  function.
 *
 * \threadsafe
 *
 * \param tick_s
 *      The start time of the selection.
 *
 * \param tick_f
 *      The finish time of the selection.
 *
 * \param status
 *      The desired event in the selection.  Now, as a new feature, tempo
 *      events are also selectable, in addition to events selected by this
 *      parameter.
 *
 * \param cc
 *      The desired control-change in the selection, if the event is a
 *      control-change.
 *
 * \param action
 *      The desired selection action.
 *
 * \return
 *      Returns the number of events selected.
 */

int
sequence::select_events
(
    midipulse tick_s, midipulse tick_f,
    midibyte status, midibyte cc, eventlist::select action
)
{
    automutex locker(m_mutex);
    return m_events.select_events(tick_s, tick_f, status, cc, action);
}

/**
 *  Select all events with the given status, and returns the number
 *  selected.  Note that there is also an overloaded version of this
 *  function.
 *
 * \threadsafe
 *
 * \warning
 *      This used to be a void function, so it just returns 0 for now.
 *
 * \param status
 *      Provides the status value to be selected.
 *
 * \param cc
 *      If the status is EVENT_CONTROL_CHANGE, then data byte 0 must
 *      match this value.
 *
 * \param inverse
 *      If true, invert the selection.
 *
 * \return
 *      Always returns 0.
 */

int
sequence::select_events (midibyte status, midibyte cc, bool inverse)
{
    automutex locker(m_mutex);
    midibyte d0, d1;
    for (auto & er : m_events)
    {
        er.get_data(d0, d1);
        bool match = er.match_status(status);
        bool canselect;
        if (status == EVENT_CONTROL_CHANGE)
            canselect = match && d0 == cc;  /* correct status and correct cc */
        else
            canselect = match;              /* correct status, cc irrelevant */

        if (canselect)
        {
            if (inverse)
            {
                if (! er.is_selected())
                    er.select();
                else
                    er.unselect();
            }
            else
                er.select();
        }
    }
    return 0;
}

int
sequence::select_event_handle
(
    midipulse tick_s, midipulse tick_f,
    midibyte astatus, midibyte cc, midibyte data
)
{
    automutex locker(m_mutex);
    int result = m_events.select_event_handle
    (
        tick_s, tick_f, astatus, cc, data
    );
    set_dirty();
    return result;
}

/**
 *  Selects all events, unconditionally.
 *
 * \threadsafe
 */

void
sequence::select_all ()
{
    automutex locker(m_mutex);
    m_events.select_all();
}

void
sequence::select_by_channel (int channel)
{
    if (is_good_channel(midibyte(channel)))
    {
        automutex locker(m_mutex);
        m_events.select_by_channel(channel);
    }
}

void
sequence::select_notes_by_channel (int channel)
{
    if (is_good_channel(midibyte(channel)))
    {
        automutex locker(m_mutex);
        m_events.select_notes_by_channel(channel);
    }
}

/**
 *  Deselects all events, unconditionally.
 *
 * \threadsafe
 */

void
sequence::unselect ()
{
    automutex locker(m_mutex);
    m_events.unselect_all();
}

/**
 *  Removes and adds selected notes in position.  Also currently moves any
 *  other events in the range of the selection.
 *
 *  Also, we've moved external calls to push_undo() into this function.
 *  The caller shouldn't have to do that.
 *
 *  Another thing this function does is wrap-around when movement occurs.
 *  Any events (except Note Off) that will start just after the END of the
 *  pattern will be wrapped around to the beginning of the pattern.
 *
 * Fixed:
 *
 *  Select all notes in a short pattern that starts at time 0 and has non-note
 *  events starting at time 0 (see contrib/midi/allofarow.mid); move them with
 *  the right arrow, and move them back with the left arrow; then view in the
 *  event editor, and see that the non-Note events have not moved back, and in
 *  fact move way too far to the right, actually to near the END marker.
 *  We've fixed that in the new adjust_timestamp() function.
 *
 *  This function checks for any marked events in seq66, but now we make sure
 *  the event is a Note On or Note Off event before dealing with it.  We now
 *  handle properly events like Program Change, Control Change, and Pitch
 *  Wheel. Remember that Aftertouch is treated like a note, as it has
 *  velocity. For non-Notes, event::get_note() returns m_data[0], and we don't
 *  want to adjust that.
 *
 * \note
 *      We leave a small gap where mark_selected() locks and unlocks, then
 *      we lock again.  This should only be an issue if moving notes while
 *      the sequence is playing.
 *
 * \param delta_tick
 *      Provides the amount of time to move the selected notes.  Note that it
 *      also applies to events.  Note-Off events are expanded to m_length if
 *      their timestamp would be 0.  All other events will wrap around to 0.
 *
 * \param delta_note
 *      Provides the amount of pitch to move the selected notes.  This value
 *      is applied only to Note (On and Off) events.  Also, if this value
 *      would bring a note outside the range of 0 to 127, that note is not
 *      changed and the event is not moved.
 */

bool
sequence::move_selected_notes (midipulse delta_tick, int delta_note)
{
    automutex locker(m_mutex);
    m_events_undo.push(m_events);                  /* push_undo(), no lock */
    bool result = m_events.move_selected_notes(delta_tick, delta_note);
    if (result)
        modify();

    return result;
}

bool
sequence::move_selected_events (midipulse delta_tick)
{
    automutex locker(m_mutex);
    m_events_undo.push(m_events);                  /* push_undo(), no lock */
    bool result = m_events.move_selected_events(delta_tick);
    if (result)
        modify();

    return result;
}

/**
 *  Performs a stretch operation on the selected events.  Also, we've moved
 *  external calls to push_undo() into this function.  The caller shouldn't have
 *  to do that.
 *
 * \threadsafe
 *
 * \param delta_tick
 *      Provides the amount of time to stretch the selected notes.
 */

bool
sequence::stretch_selected (midipulse delta_tick)
{
    automutex locker(m_mutex);
    m_events_undo.push(m_events);           /* push_undo(), no lock  */
    bool result = m_events.stretch_selected(delta_tick);
    if (result)
        modify();

    return result;
}

/**
 *  The original description was "Moves note off event."  But this also gets
 *  called when simply selecting a second note via a ctrl-left-click, even in
 *  seq66.  And, though it doesn't move Note Off events, it does reconstruct
 *  them.
 *
 *  This function is called when doing a ctrl-left mouse move on the selected
 *  notes or when using ctrl-left-arrow or ctrl-right-arrow to shrink or
 *  stretch the selected notes.  Using the mouse allows pretty much any amount
 *  of growth or shrinkage, but use the arrow keys limits the changes to the
 *  current snap value.
 *
 *  This function grows/shrinks only Note On events that are marked and
 *  linked.  If an event is not linked, this function now ignores the event's
 *  timestamp, rather than risk a segfault on a null pointer.  Compare this
 *  function to the stretch_selected() and move_selected_notes() functions.
 *
 *  This function would strip out non-Notes, but now it at least preserves
 *  them and moves them, to try to preserve their relative position re the
 *  notes.
 *
 *  In any case, we want to mark the original off-event for deletion,
 *  otherwise we get duplicate off events, for example in the "Begin/End"
 *  pattern in the test.midi file.
 *
 *  This function now tries to prevent pathological growth, such as trying to
 *  shrink the notes to zero length or less, or stretch them beyond the length
 *  of the sequence.  Otherwise we get weird and unexpected results.  Also,
 *  we've moved external calls to push_undo() into this function.  The caller
 *  shouldn't have to do that.
 *
 *  A comment on terminology:  The user "selects" notes, while the sequencer
 *  "marks" notes. The first thing this function does is mark all the selected
 *  notes.
 *
 * \threadsafe
 *
 * \param delta
 *      An offset for each linked event's timestamp.
 */

bool
sequence::grow_selected (midipulse delta)
{
    automutex locker(m_mutex);                  /* lock it again, dude  */
    m_events_undo.push(m_events);               /* push_undo(), no lock */

    bool result = m_events.grow_selected(delta, snap());
    if (result)
        modify();

    return result;
}

/**
 *  Randomizes the selected events.  Adapted from Seq32 with the unused
 *  control parameter removed.
 *
 * \param status
 *      The kind of events to be randomized.
 *
 * \param plus_minus
 *      The range of the randomization.
 *
 * \return
 *      Returns true if the randomization was performed.  If true, the
 *      sequence is flagged as modified.
 */

bool
sequence::randomize (midibyte status, int range, bool all)
{
    automutex locker(m_mutex);
    m_events_undo.push(m_events);               /* push_undo(), no lock  */
    if (range == (-1))
        range = usr().randomization_amount();

    bool result = m_events.randomize(status, range, all);
    if (result)
        modify();

    return result;
}

bool
sequence::randomize_notes (int range, bool all)
{
    automutex locker(m_mutex);
    m_events_undo.push(m_events);               /* push_undo(), no lock  */
    if (range == (-1))
        range = usr().randomization_amount();

    bool result = m_events.randomize_notes(range, all);
    if (result)
        modify();

    return result;
}

/**
 *  For usage by fix_pattern() and by Tools / Timing / Jitter.
 *  Note the snap() parameter, to avoid gross jittering.
 *
 * \param jitr
 *      Provides the maximum range of the jittering in MIDI tick units.
 *
 * \param all
 *      If true (the default is false), all events are jittered,
 *      not just the selected events. The value "true" is to be
 *      used by fix_pattern().
 *
 * \return
 *      Returns true if events got jittered.
 */

bool
sequence::jitter_notes (int jitr, bool all)
{
    automutex locker(m_mutex);
    bool result = m_events.jitter_notes(snap(), jitr, all);
    if (result)
        modify();

    return result;
}

/**
 *  Used for moving the data value of an event in the seqdata pane up or
 *  down.
 *
 *  One issue in adjusting data is Pitch events, which have two
 *  components [d0() and d1()] which must be combined. But d0() is the
 *  least-significant byte, and d1() is the most-significant byte.
 *  Since pitch is a 2-byte message, d1() will be adjusted.
 *
 * \param astatus
 *      Provides the status byte of the event, which can indicate
 *      a normal MIDI event or a Meta event (currently only Tempo is
 *      handled.)
 */

void
sequence::adjust_event_handle (midibyte astatus, midibyte adata)
{
    midibyte data[2];
    midibyte datitem;
    int dataindex = event::is_two_byte_msg(astatus) ? 1 : 0 ;
    automutex locker(m_mutex);
    for (auto & er : m_events)
    {
        if (er.is_selected_status(astatus))
        {
            if (er.is_tempo())
            {
                midibpm tempo = note_value_to_tempo(midibyte(adata));
                if (er.set_tempo(tempo))
                    modify();
            }
            else if (er.is_program_change())
            {
                /*
                 * Currently handled by the line-draw mechanism. But let's
                 * support handle dragging as well.
                 */

                er.set_data(adata);                         /* test later   */
                modify();
            }
            else
            {
                astatus = event::mask_status(astatus);
                er.get_data(data[0], data[1]);              /* \tricky code */
                datitem = adata;
                if (datitem > (c_midibyte_data_max - 1))
                    datitem = (c_midibyte_data_max - 1);

                data[dataindex] = datitem;
                er.set_data(data[0], data[1]);
                modify();
            }
        }
    }
}

/**
 *  Increments events the match the given status and control values.
 *  The supported statuses are:
 *
 *      -   EVENT_NOTE_ON
 *      -   EVENT_NOTE_OFF
 *      -   EVENT_AFTERTOUCH
 *      -   EVENT_CONTROL_CHANGE
 *      -   EVENT_PITCH_WHEEL
 *      -   EVENT_PROGRAM_CHANGE
 *      -   EVENT_CHANNEL_PRESSURE
 *
 * \threadsafe
 *
 * \param astat
 *      The desired event.
 *
 *  Parameter "acontrol", the desired control-change, is unused.
 *  This might be a bug, or at least a missing feature.
 */

void
sequence::increment_selected (midibyte astat, midibyte /*acontrol*/)
{
    automutex locker(m_mutex);
    bool modded = false;
    for (auto & e : m_events)
    {
        if (e.is_selected_status(astat))   /* && er.get_control == acontrol */
        {
            if (event::is_two_byte_msg(astat))
            {
                e.increment_d1();
                modded = true;
            }
            else if (event::is_one_byte_msg(astat))
            {
                e.increment_d0();
                modded = true;
            }
        }
    }
    if (modded)
        modify();                           /* issue #90 */
}

/**
 *  Decrements events the match the given status and control values.
 *  The supported statuses are:
 *
 *  -   One-byte messages
 *      -   EVENT_PROGRAM_CHANGE
 *      -   EVENT_CHANNEL_PRESSURE
 *  -   Two-byte messages
 *      -   EVENT_NOTE_ON
 *      -   EVENT_NOTE_OFF
 *      -   EVENT_AFTERTOUCH
 *      -   EVENT_CONTROL_CHANGE
 *      -   EVENT_PITCH_WHEEL
 *
 * \threadsafe
 *
 * \param astat
 *      The desired event.
 *
 *  Parameter "acontrol", the desired control-change, is unused.
 *  This might be a bug, or at least a missing feature.
 */

void
sequence::decrement_selected (midibyte astat, midibyte /*acontrol*/)
{
    automutex locker(m_mutex);
    bool modded = false;
    for (auto & e : m_events)
    {
        if (e.is_selected_status(astat))   /* && er.get_control == acontrol */
        {
            if (event::is_two_byte_msg(astat))
            {
                e.decrement_d1();
                modded = true;
            }
            else if (event::is_one_byte_msg(astat))
            {
                e.decrement_d0();
                modded = true;
            }
        }
    }
    if (modded)
        modify();                           /* issue #90 */
}

/**
 *  Why "change" velocity here? Why verify_and_link()? FIXME
 */

bool
sequence::repitch (const notemapper & nmap, bool all)
{
    automutex locker(m_mutex);
    bool result = false;
    push_undo();
    for (auto & e : m_events)
    {
        if (e.is_note() && (all || e.is_selected()))
        {
            midibyte pitch, velocity;                           /* d0 & d1  */
            e.get_data(pitch, velocity);
            pitch = midibyte(nmap.convert(int(pitch)));
            e.set_data(pitch, velocity);
            result = true;
        }
    }
    if (result && ! all)
    {
        if (verify_and_link())
            modify();
    }
    return result;
}

/**
 *  Copies the selected events.  This function also has the danger, discovered
 *  by user 0rel, of events being modified after being added to the clipboard.
 *  So we add his reconstruction fix here as well.  To summarize the steps:
 *
 *      -#  Clear the sm_clipboard.  NO!  If we have no events to
 *          copy to the clipboard, we do not want to clear it.  This kills
 *          cut-and-paste functionality.
 *      -#  Add all selected events in this clipboard to the sequence.
 *      -#  Normalize the timestamps of the events in the clip relative to the
 *          timestamp of the first selected event.
 *      -#  Reconstruct/reconstitute the sm_clipboard.
 *
 *  This process is a bit easier to manage than erase/insert on events because
 *  std::multimap has no erase() function that returns the next valid
 *  iterator.  Also, we use a local clipboard first, to save on copying.
 *  We've enhanced the error-checking, too.
 *
 *  Finally, note that sm_clipboard is a static member of sequence, so:
 *
 *      -#  Copying can be done between sequences.
 *      -#  Access to it needs to be protected by a mutex.
 *
 * \threadsafe
 */

bool
sequence::copy_selected ()
{
    automutex locker(m_mutex);
    eventlist clipbd;
    bool result = m_events.copy_selected(clipbd);
    if (result)
        sm_clipboard = clipbd;

    return result;
}

/**
 *  Cuts the selected events.  Pushes onto the undo stack, may copy the
 *  events, marks the selected events, and removes them.  Now also sets the
 *  dirty flag so that the caller doesn't have to.  Also raises the modify
 *  flag on the parent performer object.
 *
 * \threadsafe
 *
 * \param copyevents
 *      If true, copy the selected events before marking and removing them.
 */

bool
sequence::cut_selected (bool copyevents)
{
    push_undo();
    if (copyevents)
        copy_selected();

    return remove_selected();                   /* works and modifies   */
}

/**
 *  Pastes the selected notes (and only note events) at the given tick and
 *  the given note value.
 *
 *  Also, we've moved external calls to push_undo() into this function.
 *  The caller shouldn't have to do that.
 *
 *  The event::key used to access/sort the multimap eventlist is not updated
 *  after changing timestamp/rank of the stored events.  Regenerating all
 *  key/value pairs before merging them solves this issue, so that
 *  the order of events in the sequence will be preserved.  This action is not
 *  needed for moving or growing events.  Nor is it needed if the old
 *  std::list implementation of the event container is compiled in.  However,
 *  it is needed in any operation that modifies the timestamp of an event
 *  inside the container:
 *
 *      -   copy_selected()
 *      -   paste_selected()
 *      -   quantize_events() [quantizes or tightens base on a parameter]
 *
 *  The alternative to reconstructing the map is to erase-and-insert the
 *  events modified in the code above, rather than just tweaking their values,
 *  which have an effect on sorting for the event-map implementation.
 *  However, multimap does not provide an erase() function that returns the
 *  next valid iterator, which would complicate this method of operation.  So
 *  we're inclined to stick with this solution.
 *
 *  There was an issue with copy/pasting a whole sequence.  The pasted events
 *  did not go to their destination, but overlayed the original events.  This
 *  bug also occurred in Seq24 0.9.2.  It occurs with the allofarow.mid file
 *  when doing Ctrl-A Ctrl-C Ctrl-V Move-Mouse Left-Click.  It turns out the
 *  original code was checking only the first event to see if it was a Note
 *  event.  For sequences that started with a Control Change or Program Change
 *  (or other non-Note events), the highest note was never modified, and none
 *  of the note events were adjusted.
 *
 *  Finally, we only want to transpose note events (i.e. alter m_data[0]),
 *  and not other kinds of events.  We still need to figure out what to do
 *  with aftertouch, though.  Currently likely to be covered by the processing
 *  of the note that it accompanies.
 *
 * \threadsafe
 *
 * \param tick
 *      The time destination for the paste. This represents the "x" coordinate
 *      of the upper left corner of the paste-box.  It will be converted to an
 *      offset, for example pasting every event 48 ticks forward from the
 *      original copy.
 *
 * \param note
 *      The note/pitch destination for the paste. This represents the "y"
 *      coordinate of the upper left corner of the paste-box.  It will be
 *      converted to an offset, for example pasting every event 7 notes
 *      higher than the original copy.
 */

bool
sequence::paste_selected (midipulse tick, int note)
{
    automutex locker(m_mutex);
    eventlist clipbd = sm_clipboard;            /* copy the clipboard   */
    push_undo();                                /* push undo, no lock   */
    bool result = m_events.paste_selected(clipbd, tick, note);
    if (result)
        modify();

    return result;
}

/**
 *  This function can be tricky.  Do we want to make sure the beat-widths and
 *  beats-per-bar match, or force the destination to match?  We also need to
 *  lengthen the destination if necessary.
 */

bool
sequence::merge_events (const sequence & source)
{
    const eventlist & clipbd = source.events();
    int bw = source.get_beat_width();
    int bpb = source.get_beats_per_bar();
    midipulse len = source.get_length();
    automutex locker(m_mutex);
    set_beat_width(bw);
    set_beats_per_bar(bpb);

    bool result = len == get_length();              /* no change no problem */
    if (! result)
        result = set_length(len, false, false);

    if (result)
    {
        push_undo();                                /* push undo, no lock   */
        result = m_events.merge(clipbd);
        if (result)
            modify();
    }
    return result;
}

/**
 *  Changes the event data range.  Changes only selected events, if there are
 *  any selected events.  Otherwise, all events intersected are changed.  This
 *  function is used in qseqdata to implement the line/height functionality.
 *  We also match tempo events here.  But we have to treat them differently from
 *  the matched status events.  We also match tempo events here.  But we have to
 *  treat them differently from the matched status events.
 *
 * \threadsafe
 *
 *  Let t == the current tick value; ts == tick start value; tf == tick
 *  finish value; ds = data start value; df == data finish value; d = the
 *  new data value.  Then
 *
\verbatim
             df (t - ts) + ds (tf - t)
        d = --------------------------
                    tf  -   ts
\endverbatim
 *
 *  If this were an interpolation formula it would be:
 *
\verbatim
                            t -  ts
        d = ds + (df - ds) ---------
                            tf - ts
\endverbatim
 *
 *  Something is not quite right; to be investigated.
 *
 * \param tick_s
 *      Provides the starting tick value.
 *
 * \param tick_f
 *      Provides the ending tick value.
 *
 * \param status
 *      Provides the event status that is to be changed.
 *
 * \param cc
 *      Provides the event control value.
 *
 * \param data_s
 *      Provides the starting data value.
 *
 * \param data_f
 *      Provides the finishing data value.
 *
 * \return
 *      Returns true if the data was changed.
 */

bool
sequence::change_event_data_range
(
    midipulse tick_s, midipulse tick_f,
    midibyte status, midibyte cc,
    int data_s, int data_f, bool finalize
)
{
    automutex locker(m_mutex);
    bool result = false;
    bool haveselection = any_selected_events(status, cc);
    for (auto & er : m_events)
    {
        bool match = false;
        if (haveselection && ! er.is_selected())
            continue;

        midipulse tick = er.timestamp();
        match = er.is_desired_ex(status, cc);
        if (match)
        {
            if (tick > tick_f)                          /* in range?        */
                break;
            else if (tick < tick_s)                     /* in range?        */
                continue;
        }
        else
            continue;

        if (tick_f == tick_s)
            tick_f = tick_s + 1;                        /* no divide-by-0   */

        int newdata =
        (
            (tick - tick_s) * data_f + (tick_f - tick) * data_s
        ) / (tick_f - tick_s);
        newdata = int(clamp_midibyte_value(newdata));   /* 0 to 127         */
        if (er.is_tempo())
        {
            midibpm tempo = note_value_to_tempo(midibyte(newdata));
            result = er.set_tempo(tempo);
        }
        else
        {
            midibyte d0, d1;
            er.get_data(d0, d1);
            if (event::is_one_byte_msg(status))         /* patch | pressure */
                d0 = newdata;
            else if (event::is_two_byte_msg(status))
                d1 = newdata;

            er.set_data(d0, d1);
            result = true;
        }
        if (result && finalize)
            modify();
    }
    return result;
}

/**
 *  Changes the event data range in a relative fashion, as opposed to plain
 *  (line) fashion.  This function is used if there are events in the given
 *  tick range.
 *
 * \threadsafe
 *
 * \param tick_s
 *      Provides the starting tick value.
 *
 * \param tick_f
 *      Provides the ending tick value.
 *
 * \param status
 *      Provides the event status that is to be changed.
 *
 * \param cc
 *      Provides the event control value.
 *
 * \param newval
 *      Provides the new data value for (additive) "scaling".
 *
 * \return
 *      Returns true if the data was changed.
 */

bool
sequence::change_event_data_relative
(
    midipulse tick_s, midipulse tick_f,
    midibyte status, midibyte cc,
    int newval, bool finalize
)
{
    automutex locker(m_mutex);
    bool result = false;
    bool haveselection = any_selected_events(status, cc);
    for (auto & er : m_events)
    {
        bool match = false;
        if (haveselection && ! er.is_selected())
            continue;

        midipulse tick = er.timestamp();
        match = er.is_desired_ex(status, cc);
        if (match)
        {
            if (tick > tick_f)                          /* in range?        */
                break;
            else if (tick < tick_s)                     /* in range?        */
                continue;
        }
        else
            continue;

        if (er.is_tempo())
        {
            midibpm tempo = note_value_to_tempo(midibyte(newval));
            result = er.set_tempo(tempo);
        }
        else
        {
            /*
             * Two-byte messages: Note On/Off, Aftertouch, Control, Pitch.
             * One-byte messages: Program or Channel Pressure.
             */

            midibyte d0, d1;
            er.get_data(d0, d1);

            int newdata = int(clamp_midibyte_value(d1 + newval));
            if (event::is_one_byte_msg(status))
                d0 = newdata;
            else
                d1 = newdata;

            er.set_data(d0, d1);
            result = true;
        }
        if (result && finalize)
            modify();
    }
    return result;
}

/**
 *  Modifies data events according to the parameters active in the LFO window.
 *  If the event is in the selection, or there is no selection at all, and if
 *  it has the desired status and not CC, or the desired status and the correct
 *  control-change number, then we will modify (set) the event.
 *
 * \param dcoffset
 *      Provides the base amplitude for the event data value.  Ranges from 0
 *      to 127 in increments of 0.1.  This amount is added to the result of
 *      the wave_func() calculation.
 *
 * \param range
 *      Provides the range for the event data value.  Ranges from 0 to
 *      127 in increments of 0.1.
 *
 * \param speed
 *      Provides the number of periods in the measure or the full length for the
 *      modifications.  Ranges from 0 to 16 in increments of 0.01.
 *
 * \param phase
 *      The phase of the event modification.  Ranges from 0 to 1 in increments
 *      of 0.01.  This represents a phase shift of 0 to 360 degrees.
 *
 * \param w
 *      The wave type to apply.  See enum class wave in the calculations
 *      module.
 *
 * \param status
 *      The status (event number) for the events to modify.
 *
 * \param cc
 *      Provides the control-change value for Control Change events that are
 *      to be modified.
 *
 * \param usemeasure
 *      If true, then use a measure as the length for wave periodicity, rather
 *      than the full length of the sequence.
 */

void
sequence::change_event_data_lfo
(
    double dcoffset, double range, double speed, double phase,
    waveform w, midibyte status, midibyte cc, bool usemeasure
)
{
    automutex locker(m_mutex);
    bool modified = false;
    double dlength = double(get_length());
    bool noselection = ! any_selected_events(status, cc);
    if (get_length() == 0)                  /* should never happen, though  */
        dlength = double(m_ppqn);

    if (usemeasure)
        dlength = double(measures_to_ticks());

    m_events_undo.push(m_events);           /* experimental, seems to work  */
    for (auto & er : m_events)
    {
        bool match = false;
        if (noselection || er.is_selected())
            match = er.is_desired_ex(status, cc);

        if (match)
        {
            double dtick = double(er.timestamp());
            double angle = speed * dtick / dlength + phase;
            double value = wave_func(angle, w);
            int newdata = int(range * value + dcoffset);
            newdata = int(abs_midibyte_value(newdata)); /* keep at 0 to 127 */
            if (er.is_tempo())
            {
                midibpm tempo = note_value_to_tempo(midibyte(newdata));
                (void) er.set_tempo(tempo);
            }
            else
            {
                midibyte d0, d1;
                er.get_data(d0, d1);
                if (event::is_one_byte_msg(status))
                    d0 = midibyte(newdata);
                else if (event::is_two_byte_msg(status))
                    d1 = midibyte(newdata);

                er.set_data(d0, d1);
            }
            modified = true;
        }
    }
    if (modified)
        modify();
}

/**
 *  Validates a scale-factor or measures value for scaling. A static function.
 *
 * \param ismeasure
 *      If true, then a much larger value is allowed.
 *
 * \return
 *      Returns true if valid.
 */

bool
sequence::valid_scale_factor (double s, bool ismeasure)
{
    bool result = s >= c_scale_min;
    if (result)
    {
        double maximum = ismeasure ? c_measure_max : c_scale_max ;
        result = s <= maximum;
    }
    return result;
}

/**
 *  Recalculates the number of measures, making sure that values less than 1.0
 *  become 1, and that, otherwise, the measure count is either very close
 *  (0.01) to the lower integer number, or moved up the next integer measure
 *  value. A static function.
 */

int
sequence::trunc_measures (double measures)
{
    return int(seq66::trunc_measures(measures));
}

/**
 *  Does a lot of things at once, to all events in the pattern.
 *  See the qpatternfix dialog.
 *
 *  Here's the process:
 *
 *      -#  If align_left is set, all events timestamps are decreased by the
 *          offset of the first event, so that the pattern starts at time 0.
 *          This will not change pattern length, note length, measures, and
 *          time signature. The align_right setting is similar.
 *      -#  If the fix_type is lengthfix::measures:
 *          -#  The scale-factor is calculated by the desired measures divided
 *              by the pattern's current measure value.
 *          -#  The pattern length is scaled to the new measure size.
 *          -#  If a time-signature is specifed, that is applied to the
 *              pattern.
 *      -#  If the fix_type is lengthfix::rescale, then the scale factor is
 *          used directly. What should happen?
 *          -   All event timestamps are moved according to the expansion or
 *              compression scale factor.
 *          -   The measure count and time signature remain the same.
 *      -#  If quantization or tightened are supplied, these operations are
 *          performed.
 *      -#  The maximum timestamp is calculated, and the measures (and hence
 *          length) may end up being modified.
 *
 *  See the documentation for the fixparameters structure in the sequence.hpp
 *  module. Apart from the numeric settings (and note-map file-name), the
 *  following enumerations from the caluclations module apply:
 *
 *      -   lengthfix: none, measures, or rescale.
 *          -   The effect depends on the measures number as entered by the
 *              user. There are three choices for the measures number:
 *              -   Strict integer. "lengthfix::measures" is used, and
 *                  the changes yields a non-1.0 scale factor.
 *              -   Float (i.e. with a decimal point), then
 *                  "lengthfix::rescale" is used.
 *                  The difference??? TBD.
 *              -   Time signature (e.g. "3/4"). Again the setting is
 *                  "lengthfix::measures), the scale factor is set, and
 *                  in addition, the use-time-signature flag is set. In this
 *                  process, the number of measures might not actually change.
 *              Note that once specified, the number of measures is rounded
 *              up to the nearest integral value.
 *
 *      -   alteration: none, quantize, jitter, etc.
 *      -   fixeffect: none, shifted, shrunk, etc. This starts out as none.
 *          The fix function below then sets bits according to the adjustments
 *          actually made.
 *
 * \param [inout] fp
 *      Provides input to the changes to be made, and also receives any
 *      side-effects of the change.
 *
 * \return
 *      Returns true if the fixup succeeded.
 */

bool
sequence::fix_pattern (fixparameters & fp)
{
    automutex locker(m_mutex);
    double newmeasures = fp.fp_measures;
    double newscalefactor = fp.fp_scale_factor;
    bool result = valid_scale_factor(newscalefactor) &&
        valid_scale_factor(newmeasures, true);

    if (result)
    {
        bool doscale = fnotequal(newscalefactor, 1.0);
        bool shrunk = flessthan(newscalefactor, 1.0);
        midipulse currentlen = get_length();
        midipulse newlength = 0;
        fixeffect tempefx = fixeffect::none;
        push_undo();
        if (fp.fp_align_left)
        {
            fp.fp_align_left = m_events.align_left();       /* realigned?   */
            if (fp.fp_align_left)
                tempefx = bit_set(tempefx, fixeffect::shifted);
            else
                result = false;                             /* op failed    */
        }
        else if (fp.fp_align_right)
        {
            fp.fp_align_right = m_events.align_right();     /* realigned?   */
            if (fp.fp_align_right)
                tempefx = bit_set(tempefx, fixeffect::shifted);
            else
                result = false;                             /* op failed    */
        }
        if (result && (fp.fp_reverse || fp.fp_reverse_in_place))
        {
            result = m_events.reverse_events(fp.fp_reverse_in_place);
            if (result)
            {
                if (fp.fp_reverse)
                    tempefx = bit_set(tempefx, fixeffect::reversed);
                else
                    tempefx = bit_set(tempefx, fixeffect::reversed_abs);
            }
        }
        if (fp.fp_alter_type != alteration::none)
        {
            switch (fp.fp_alter_type)
            {
            case alteration::tighten:

                result = m_events.quantize_events
                (
                    fp.fp_tighten_range, 1, true    /* all events   */
                );
                if (result)
                    tempefx = bit_set(tempefx, fixeffect::alteration);
                break;

            case alteration::quantize:

                result = m_events.quantize_events
                (
                    fp.fp_quantize_range, 1, true   /* all events   */
                );
                if (result)
                    tempefx = bit_set(tempefx, fixeffect::alteration);
                break;

            case alteration::jitter:

                result = jitter_notes(fp.fp_jitter_range, true);
                if (result)
                    tempefx = bit_set(tempefx, fixeffect::alteration);
                break;

            case alteration::random:

                result = randomize_notes (fp.fp_random_range, true);
                if (result)
                    tempefx = bit_set(tempefx, fixeffect::alteration);
                break;

            case alteration::notemap:

                result = ! fp.fp_notemap_file.empty();
                if (result)
                {
                    result = perf()->repitch_fix
                    (
                        fp.fp_notemap_file, *this, false    /* forward  */
                    );
                    if (result)
                        tempefx = bit_set(tempefx, fixeffect::alteration);
                }
                break;

            case alteration::rev_notemap:

                result = ! fp.fp_notemap_file.empty();
                if (result)
                {
                    result = perf()->repitch_fix
                    (
                        fp.fp_notemap_file, *this, true     /* reverse  */
                    );
                    if (result)
                        tempefx = bit_set(tempefx, fixeffect::alteration);
                }
                break;

            default:

                break;
            }
        }
        else if (result)
        {
            bool fixmeasures = fp.fp_fix_type == lengthfix::measures;
            bool fixscale = fp.fp_fix_type == lengthfix::rescale;
            bool timesig = fp.fp_use_time_signature;
            if (timesig)
            {
                /*
                 * We no longer apply length or set the beats and width
                 * here. It is done in qpatternfix::slot_set(), along
                 * with a verify-and-link to ensure refresh. We do have
                 * to scale here, unless converting 4/4/to 8/8.
                 */

                if (doscale)
                {
                    newlength = apply_time_factor
                    (
                        newscalefactor, fp.fp_save_note_length
                    );
                    result = newlength > 0;
                    if (result)
                        tempefx = bit_set(tempefx, fixeffect::time_sig);
                }
            }
            else if (fixmeasures)
            {
                if (doscale)                                /* must do 1st  */
                {
                    newlength = apply_time_factor
                    (
                        newscalefactor, fp.fp_save_note_length
                    );
                    result = newlength > 0;
                }
            }
            else if (fixscale)
            {
                newlength = apply_time_factor
                (
                    newscalefactor, fp.fp_save_note_length
                );
                result = newlength > 0;
            }
            if (result && ! timesig && (newlength != currentlen))
            {
                int measures = get_measures(newlength);
                if (fixmeasures)
                {
                    if (measures < int(newmeasures))
                        measures = int(newmeasures);
                }
                (void) apply_length(measures);
            }
            if (result)
            {
                fp.fp_length = newlength;
                fp.fp_measures = double(get_measures());
                fp.fp_scale_factor = newscalefactor;
                fp.fp_effect = tempefx;
                if (shrunk)
                    tempefx = bit_set(tempefx, fixeffect::shrunk);
                else
                    tempefx = bit_set(tempefx, fixeffect::expanded);
            }
        }
        else
            pop_undo();
    }
    return result;
}

/**
 *  Adds a note of a given length and  note value, at a given tick
 *  location.  It adds a single Note-On/Note-Off pair.
 *
 *  Supports the step-edit (auto-step) feature, where we are entering notes
 *  without playback occurring, so we set the generic default note length and
 *  volume to the snap.  There are two ways to enter notes:
 *
 *      -   Mouse movement in the seqroll.  Here, velocity defaults to the
 *          preserve_velocity>
 *      -   Input from a MIDI keyboard.  Velocity ranges from 0 to 127.
 *
 *  Will be consistent with how Note On velocity is handled; enable 0 velocity
 *  (a standard?) for Note Off when not playing. Note that the event
 *  constructor sets channel to 0xFF, while event::set_data() currently sets
 *  it to 0!!!
 *
 *  The paint parameter indicates if we care about the painted event, so then
 *  the function runs though the events and deletes the painted ones that
 *  overlap the ones we want to add.  An event is painted if manually
 *  created in the seqroll.  There is also a case in the add_chord() function.
 *
 *  Also note that push_undo() is not incorporated into this function, for
 *  the sake of speed.
 *
 *  Here, we could ignore events not on the sequence's channel, as an option.
 *  We have to be careful because this function can be used in painting notes.
 *
 * Stazed:
 *
 *      http://www.blitter.com/~russtopia/MIDI/~jglatt/tech/midispec.htm
 *
 *      Note Off: The first data is the note number. There are 128 possible
 *      notes on a MIDI device, numbered 0 to 127 (where Middle C is note
 *      number 60). This indicates which note should be released.  The second
 *      data byte is the velocity, a value from 0 to 127. This indicates how
 *      quickly the note should be released (where 127 is the fastest). It's
 *      up to a MIDI device how it uses velocity information. Often velocity
 *      will be used to tailor the VCA release time.  MIDI devices that can
 *      generate Note Off messages, but don't implement velocity features,
 *      will transmit Note Off messages with a preset velocity of 64.
 *
 *  Seq24 never used the recording-velocity member (m_rec_vol).  We use it to
 *  modify the new m_note_on_velocity member if the user changes it in the
 *  seqroll.
 *
 * \threadsafe
 *
 * \param tick
 *      The time destination of the new note, in pulses.
 *
 * \param len
 *      The duration of the new note, in pulses.
 *
 * \param note
 *      The pitch destination of the new note.  We no longer check it. Either
 *      it is a good value from a MIDI device, or the caller (qseqroll) ensures
 *      it.
 *
 * \param repaint
 *      If true, repaint the whole set of events, in order to be left with
 *      a clean view of the inserted event.  We run through the events, deleting
 *      the painted ones that overlap the one we want to add. The default is
 *      false.
 *
 * \param velocity
 *      If not set to the preserve-velocity, the velocity of the note is
 *      set to this value.  Otherwise, it is hard-wired to the stored note-on
 *      velocity.  The name of this macro is counter-intuitive here.
 *
 * \return
 *      Returns true if the event was added.
 */

bool
sequence::add_painted_note
(
    midipulse tick, midipulse len, int note,
    bool repaint, int velocity
)
{
    bool result = false;
    bool ignore = false;
    if (repaint)                                    /* see banner above     */
    {
        automutex locker(m_mutex);
        ignore = remove_duplicate_events(tick, note);
    }
    if (ignore)
    {
        result = true;
    }
    else
    {
        bool hardwire = velocity == usr().preserve_velocity();
        midibyte v = hardwire ? midibyte(m_note_on_velocity) : velocity ;
        event e(tick, EVENT_NOTE_ON, midi_channel(), note, v);
        if (repaint)
            e.paint();

        result = add_event(e);
        if (result)
        {
            midibyte v = hardwire ? midibyte(m_note_off_velocity) : 0 ;
            event e(tick + len, EVENT_NOTE_OFF, midi_channel(), note, v);
            result = add_event(e);
        }
        if (result && expanded_recording())
            set_last_tick(tick + len);
    }
    if (result)
    {
        if (verify_and_link())
            modify();                           /* no easy way to undo this */
    }
    return result;
}

/**
 *  Overload for use with auto-step MIDI keyboard input. The version above
 *  always sets channel to 0, and can repaint, and is used by the seqroll.
 *
 *  Note: Like the version above, this code simulates a Note Off.  We will
 *  fix that AT SOME POINT.
 *
 * \param len
 *      The length between the Note On and Note Off events to be added here.
 *
 * \param e
 *      The Note On to use for the Note On and Note Off event additions.  This
 *      event already has its timestamp and velocity tailored.
 */

bool
sequence::add_note (midipulse len, const event & e)
{
    bool result = add_event(e);
    if (result)
    {
        midipulse tick = e.timestamp() + len;
        midibyte v = midibyte(m_note_off_velocity);
        event eoff(tick, EVENT_NOTE_OFF, e.channel(), e.get_note(), v);
        result = add_event(eoff);
    }
    if (result)
        result = verify_and_link();

    if (result)
    {
        /*
         * Notification of step-edit note entry from a keyboard causes:
         *
         * QObject::setParent: Cannot set parent, new parent is in a different
         * thread
         *
         * So we pass false to avoid the notification. However, this causes a
         * delay in refreshing the notes in the pattern editor.
         *
         *      modify(false);
         */

        m_is_modified = true;
        set_dirty();
        perf()->notify_sequence_change(seq_number(), performer::change::no);
    }
    return result;
}

/**
 *  An overload to ignore painting values and increase efficiency during input
 *  recording.
 *
 *  Will be consistent with how Note On velocity is handled; enable 0 velocity
 *  (a standard?) for Note Off when not playing. Note that the event
 *  constructor sets channel to 0xFF, while event::set_data() currently sets
 *  it to 0!!!
 *
 *  Add note, preceded by a push-undo.  This is meant to be used only by the
 *  user-interface, when manually entering notes.
 */

bool
sequence::push_add_note
(
    midipulse tick, midipulse len, int note,
    bool repaint, int velocity
)
{
    m_events_undo.push(m_events);                   /* push_undo(), no lock */
    return add_painted_note(tick, len, note, repaint, velocity);
}

bool
sequence::push_add_chord
(
    int chord, midipulse tick, midipulse len,
    int note, int velocity
)
{
    m_events_undo.push(m_events);                   /* push_undo(), no lock */
    return add_chord(chord, tick, len, note, velocity);
}

/**
 *  Adds a chord of a given length and  note value, at a given tick
 *  location.
 *
 * \todo
 *      Add the ability to preserve the incoming velocity.
 *
 * \threadsafe
 *
 * \param chord
 *      If greater than 0 (and valid), a chord (multiple notes) will be
 *      generated using this chord in the c_chord_table[] array.  Otherwise,
 *      only a single note will be added.
 *
 * \param tick
 *      The time destination of the new note, in pulses.
 *
 * \param len
 *      The duration of the new note, in pulses.
 *
 * \param note
 *      The pitch destination of the new note.
 *
 * \return
 *      Returns true if the events were added.
 */

bool
sequence::add_chord
(
    int chord, midipulse tick, midipulse len,
    int note, int velocity
)
{
    bool result = false;
    if (chord > 0 && chord_number_valid(chord))
    {
        const chord_notes & cn = chord_entry(chord);
        for (auto cnote : cn)
        {
            if (cnote == -1)
                break;

            result = add_painted_note(tick, len, note + cnote, false, velocity);
            if (! result)
                break;
        }
    }
    else
        result = add_painted_note(tick, len, note, true, velocity);

    return result;
}

bool
sequence::add_tempo (midipulse tick, midibpm tempo, bool repaint)
{
    automutex locker(m_mutex);
    bool valid = tempo >= usr().midi_bpm_minimum() &&
        tempo <= usr().midi_bpm_maximum();

    bool result = valid && tick >= 0;
    if (result)
    {
        if (repaint)
            (void) remove_duplicate_events(tick);

        event e(tick, tempo);
        if (repaint)
            e.paint();

        result = add_event(e);

        /*
         *  Currently we draw tempo as a circle, not a bar, so we don't need
         *  to know about the "next" tempo event. We don't (yet) need
         *  to link tempo events: verify_and_link();
         */

         if (result)
            modify();                               /* added ca 2022-04-25  */
    }
    return result;
}

/**
 *  Use for adding Tempo events via a drag-line in qseqdata. The events are
 *  added at each snap point in the tick range. Currently only a linear
 *  sequence of tempos can be added. The formula is linear:
 *
\verbatim
                 (B1 - B0) (t - t0)
            B = -------------------- + B0
                        (t1 - t0)
\endverbatim
 *
 *  We should generalize this is the calculations module at some point.
 *
 * \param tick_s
 *      Provides the starting tick value. Might get adjusted to the previous
 *      snap value. This is t0 above.
 *
 * \param tick_f
 *      Provides the ending tick value. Might get adjusted to the next
 *      snap value. This is t1 above.
 *
 * \param tempo_s
 *      Provides the starting tempo value. This is B0 above.
 *
 * \param tempo_f
 *      Provides the finishing tempo value. This is B1 above.
 *
 * \return
 *      Returns true if all tempos in the time-range were added.
 */

bool
sequence::add_tempos
(
    midipulse tick_s, midipulse tick_f,
    int data_s, int data_f
)
{
    automutex locker(m_mutex);
    bool result = false;
    midipulse S = snap();
    midibpm B0 = note_value_to_tempo(midibyte(data_s));
    midibpm B1 = note_value_to_tempo(midibyte(data_f));
    midipulse t0 = down_snap(S, tick_s);
    midipulse t1 = up_snap(S, tick_f);      /* later truncate to seq length */
    double slope = (B1 - B0) / double(t1 - t0);
    for (midipulse t = t0; t <= t1; t += S)
    {
        double value = slope * double(t - t0) + B0;
        result = add_tempo(t, value);
        if (! result)
            break;
    }
    return result;
}

/**
 *  Constructs a time-signature event and appends it to the pattern. It is
 *  used in the pattern editor.
 *
 *  In setting the time signature here, all we want to do is change the
 *  beats and beat width. The clocks_per_metronome() and
 *  get_32nds_per_quarter() stay the same, as they would affect the whole tune.
 *
 *  Data: FF 58 04 n d c b
 */

bool
sequence::log_time_signature (midipulse tick, int beats, int bw)
{
    automutex locker(m_mutex);
    bool result = beats > 0 && is_power_of_2(bw);
    if (result)
    {
        m_events_undo.push(m_events);               /* push_undo(), no lock */
        event e (tick, EVENT_MIDI_META);
        midibyte bt[4];
        bw = beat_log2(bw);                                 /* log2(bw)     */
        bt[0] = midibyte(beats);                            /* numerator    */
        bt[1] = midibyte(bw);                               /* denominator  */
        bt[2] = midibyte(clocks_per_metronome());
        bt[3] = midibyte(get_32nds_per_quarter());
        result = e.append_meta_data(EVENT_META_TIME_SIGNATURE, bt, 4);
        if (result)
        {
            result = append_event(e);
            if (result)
            {
                sort_events();
                modify(true);
            }
        }
    }
    return result;
}

/**
 *  Gets the data from a time-signature event and makes some settings.
 *  Then it adds the event. See its usage in the midifile class.
 *  It is *not* considered a modification.
 *
 * Variables:
 *
 *      -   bpb: Provides beats/bar (i.e. beats per measure).
 *      -   bw: Provides beat-width (i.e. denominator of the time signature).
 *      -   cpm: Provides clocks/metronome.
 *      -   tpq: Provides 32nds/quarter.
 *
 * \param e
 *      The event, which must be a well-defined time-signature event.
 *
 * \param main_ts
 *      True (the default is false) if it need to be set as the main
 *      sequence time-signature.
 *
 * \return
 *      Returns true if the event was a time-signature and was added to
 *      the pattern.
 */

bool
sequence::add_timesig_event (const event & e, bool main_ts)
{
    automutex locker(m_mutex);
    bool result = e.is_time_signature();
    if (result)
    {
        if (main_ts)
        {
            int bpb = e.get_sysex(0);
            int bw = beat_power_of_2(e.get_sysex(1));
            int cpm = e.get_sysex(2);
            int tpq = e.get_sysex(4);
            clocks_per_metronome(cpm);
            set_32nds_per_quarter(tpq);
            set_time_signature(bpb, bw);
        }
        result = append_event(e);
        if (result)
            sort_events();
    }
    return result;
}

bool
sequence::add_c_timesig (int bpb, int bw, bool main_ts)
{
    automutex locker(m_mutex);
    if (main_ts)
    {
        set_beats_per_bar(bpb);
        set_beat_width(bw);
    }
    return true;
}

bool
sequence::delete_time_signature (midipulse tick)
{
    bool result = false;
    event e(tick, EVENT_MIDI_META);
    midibyte bt[4];
    bt[0] = bt[1] = bt[2] = bt[3] = 0;
    result = e.append_meta_data(EVENT_META_TIME_SIGNATURE, bt, 4);
    if (result)
        result = remove_first_match(e, tick);

    return result;
}

/**
 *  This function looks for a time-signature, and if it is within the first
 *  snap interval, sets the beats value accordingly.
 *
 * \param [out] tstamp
 *      The timestamp of the time-signature event, if one is found.
 *
 * \param [out] numerator
 *      A return parameter for the beats per bar. Use the value only if true
 *      is returned.
 *
 * \param [out] denominator
 *      A return parameter for the beat width. Use the value only if true
 *      is returned.
 *
 * \param start
 *      Provides the starting point for the search.  Events before this tick
 *      are ignored.  The default value is 0, the beginning of the pattern.
 *
 * \param range
 *      Provides how far to look for a time signature. The default is
 *      c_null_midipulse, which means just detect the first time-signature
 *      no matter how deep into the pattern. Another useful value is the
 *      snap() value from the seqedit. In that case the event must be
 *      with \a range ticks of .....
 *
 * \return
 *      Returns true if a time signature was detected before the range was
 *      reached.
 */

bool
sequence::detect_time_signature
(
    midipulse & tstamp,
    int & numerator,
    int & denominator,
    midipulse start,
    midipulse range
)
{
    bool result = false;
    auto cev = cbegin();
    if (! cend(cev))
    {
        if (get_next_meta_match(EVENT_META_TIME_SIGNATURE, cev, start, range))
        {
            tstamp = cev->timestamp();
            numerator = int(cev->get_sysex(0));
            denominator = beat_power_of_2(int(cev->get_sysex(1)));
            result = true;
        }
    }
    return result;
}

/**
 *  Adds an event to the internal event list in a sorted manner.  Then it
 *  resets the draw-marker and sets the dirty flag.
 *
 *  Currently, when reading a MIDI file [see the midifile::parse() function],
 *  only the main events (notes, after-touch, pitch, program changes, etc.)
 *  are added with this function.  So, we can rely on reading only playable
 *  events into a sequence.  Well, actually, certain meta-events are also
 *  read, to obtain channel, buss, and more settings.  Also read for a
 *  sequence, if the global-sequence flag is not set, are the new key, scale,
 *  and background sequence parameters.
 *
 *  This module (sequencer) adds all of those events as well, but it
 *  can surely add other events.  We should assume that any events
 *  added by sequencer are playable/usable.
 *
 *  Here, we could ignore events not on the sequence's channel, as an option.
 *  We have to be careful because this function can be used in painting events.
 *
 *  To speed things up a bit, do not try to verify and link notes unless the
 *  incoming event is a note.  We will first try allowing it only for a Note
 *  Off for even more savings :-D
 *
 *  Also, instead of modifying and notifying, we just modify. We need to see
 *  if this prevents a weird unknown-signal error in qseqeditframe64 in the
 *  update_midi_buttons() function.  We don't need that to see added note
 *  events anyway.
 *
 * \threadsafe
 *
 * \warning
 *      This pushing (and, in writing the MIDI file, the popping),
 *      causes events with identical timestamps to be written in
 *      reverse order.  Doesn't affect functionality, but it's puzzling
 *      until one understands what is happening.  Actually, this is true only
 *      in Seq24, we've fixed that behavior for Seq66.
 *
 * \param er
 *      Provide a reference to the event to be added; the event is copied into
 *      the events container.
 *
 * \return
 *      Returns true if the event was added.
 */

bool
sequence::add_event (const event & er)
{
    automutex locker(m_mutex);
    bool result = m_events.append(er);  /* no-sort insertion of event       */
    if (result)
    {
        if (er.is_note_off())
            (void) verify_and_link();   /* for proper seqroll draw; sorts   */

        modify(false);                  /* do not call notify_change()      */
    }
    return result;
}

/**
 *  An alternative to add_event() that does not sort the events, even if the
 *  event list is implemented by an std::list.  This function is meant mainly
 *  for reading the MIDI file, to save a lot of time.  We could also add a
 *  channel parameter, if the event has a channel.  This reveals that in
 *  midifile and wrkfile, we update the channel setting too many times.
 *
 * \param er
 *      Provide a reference to the event to be added; the event is copied into
 *      the events container.
 *
 * \return
 *      Returns true if the event was appended.
 */

bool
sequence::append_event (const event & er)
{
    automutex locker(m_mutex);
    return m_events.append(er);     /* does *not* sort, too time-consuming  */
}

void
sequence::sort_events ()
{
    automutex locker(m_mutex);
    m_events.sort();
}

event
sequence::find_event (const event & e, bool nextmatch)
{
    automutex locker(m_mutex);
    static event s_null_result{0, 0, 0};
    event::iterator evi = nextmatch ?
        m_events.find_next_match(e) : m_events.find_first_match(e) ;

    return evi != m_events.end() ?  *evi : s_null_result ;
}

sequence::note_info
sequence::find_note (midipulse tick, int note)
{
    bool found = false;
    note_info result;
    for (auto cev = cbegin(); ! cend(cev); ++cev)
    {
        draw status = get_note_info(result, cev);
        if (status == draw::linked || status == draw::note_on)
        {
            found = tick >= result.start() &&
                tick < result.finish() && result.note() == note;

            if (found || result.start() > tick)
                break;
        }
    }
    if (! found)
        result.ni_note = (-1);

    return result;
}

bool
sequence::remove_duplicate_events (midipulse tick, int note)
{
    automutex locker(m_mutex);                  /* ca 2023-04-29    */
    bool ignore = false;
    for (auto & er : m_events)
    {
        if (er.is_painted() && er.timestamp() == tick)
        {
            if (note >= 0 && er.is_note_on())
            {
                ignore = true;
                break;
            }
            er.mark();
            if (er.is_linked())
                er.link()->mark();

            set_dirty();
        }
    }
    (void) remove_marked();
    return ignore;
}

/**
 *  Adds a event of a given status value and data values, at a given tick
 *  location.
 *
 *  The paint parameter indicates if we care about the painted event,
 *  so then the function runs though the events and deletes the painted
 *  ones that overlap the ones we want to add.
 *
 * \threadsafe
 *
 * \param tick
 *      The time destination of the event.
 *
 * \param status
 *      The type of event to add.
 *
 * \param d0
 *      The first data byte for the event.
 *
 * \param d1
 *      The second data byte for the event (if needed).
 *
 * \param repaint
 *      If true, the inserted event is marked for painting.  The default value
 *      is false. Also, if true, then events that are marked as painted and match
 *      the tick parameter are marked.
 */

bool
sequence::add_event
(
    midipulse tick, midibyte status,
    midibyte d0, midibyte d1, bool repaint
)
{
    automutex locker(m_mutex);
    bool result = tick >= 0;
    if (result)
    {
        if (repaint)
            (void) remove_duplicate_events(tick);

        event e(tick, status, d0, d1);
        if (repaint)
            e.paint();

        result = m_events.append(e);    /* (add_event(e) locks & verifies)  */
        if (result)
        {
            (void) verify_and_link();   /* might be no note events to link  */
            modify();                   /* call notify_change()             */
        }
    }
    return result;
}

/**
 *  Handles loop/replace status on behalf of seqrolls.  This sets the
 *  loop-reset status, which is checked in the stream_event() function in
 *  this module [WRONG; it is check in qseqeditframe64].  This status is
 *  set when the time-stamp remainder is less than a quarter note,
 *  meaning we have just gotten back to the beginning of the loop.
 *  See the call in qseqeditframe64.
 */

bool
sequence::check_loop_reset ()
{
    bool result = false;
    midipulse ts = perf()->get_tick();
    midipulse len = get_length();
    if (len > 0 && ts > len)
    {
        midipulse tsmod = ts % len;
        if (tsmod < (m_ppqn / c_reset_divisor))
        {
            bool check = overwriting();
            if (check && perf()->is_running())
            {
                loop_reset(true);
                result = true;
            }
        }
    }
    return result;
}

/**
 *  This function is meant to return false until a note is struck, whether before
 *  the first end-of-pattern is reached or after that.
 *
 *  Special case, call only when playback is running: perf()->is_running()).
 *
 * \return
 *      Returns true if one-shot is over.
 */

bool
sequence::check_oneshot_recording ()
{
    bool result = false;
    if (oneshot_recording())
    {
        midipulse len = get_length();
        if (len > 0)
        {
            /*
             * Issues: (1) the second note is recalculated; (2) a note off
             * might appear outside.
             */

            midipulse ts = perf()->get_tick();
            if (note_count() > 0)
            {
                if (note_count() == 1)
                {
                    midipulse tmod = ts % len;
                    midipulse tbeg = ts - tmod;
                    m_next_boundary = tbeg + len - 1;
                }
                else
                    result = ts > m_next_boundary;
            }
        }
    }
    return result;
}

/**
 *  Streams (records) the given event.  The event's timestamp is adjusted, if
 *  needed.  If recording:
 *
 *      -   Pattern is playing.
 *      -   Pattern is no playing.
 *          -   If one-shot recording is in force, and the loop has reset,
 *              return with a value of false.
 *          -   If quantized/tightened/note-mapped recording is in force,
 *              the note timestamp or pitch value is altered.
 *          -   If the pattern is playing, the event is added.
 *      -   If not playing, but the event is a Note On or Note Off, we add it
 *          and keep track of it.
 *
 *  If MIDI Thru is enabled, the event is also put on the buss.
 *
 *  This function supports rejecting events if the channel doesn't match that
 *  of the sequence.  We do it here for comprehensive event support.  Also
 *  make sure the event-channel is preserved before this function is called,
 *  and also need to make sure that the channel is appended on both playback
 *  and in saving of the MIDI file.
 *
 *  If in overwrite loop-record mode, any events after reset should clear the
 *  old items from the previous pass through the loop.
 *
 * \todo
 *      If the last event was a Note Off, we should clear it here, and
 *      how?
 *
 *  The m_rec_vol member includes the "Free" menu entry in seqedit, which sets
 *  the velocity to the preserve-velocity (-1).
 *
 *  If the pattern is not playing, this function supports the step-edit
 *  (auto-step) feature, where we are entering notes without playback
 *  occurring, so we set the generic default note length and volume to the
 *  snap.  For Note Ons, this isgnores the actual Note Off and synthesizes a
 *  matching Note Off.
 *
 * \threadsafe
 *
 * \param ev
 *      Provides the event to stream.
 *
 * \return
 *      Returns true if the event's channel matched that of this sequence, and
 *      the channel-matching feature was set to true.  Also returns true if
 *      we're not using channel-matching.  A return value of true means the
 *      event should be saved.
 */

bool
sequence::stream_event (event & ev)
{
    automutex locker(m_mutex);
    bool result = channels_match(ev);           /* set if channel matches   */
    if (result)
    {
        if (loop_reset())
        {
            if (overwriting())
            {
                loop_reset(false);
                remove_all();                   /* vs m_events.clear()      */
                set_dirty();
            }
            else if (oneshot_recording())       /* is this necessary???     */
            {
                loop_reset(false);
                set_recording(toggler::off);
                set_dirty();
            }
        }

        /*
         *  If we are in expand mode, we do not want to wrap the timestamp.
         *  Expansion will occur only here, when an event is received.
         */

        if (expanded_recording())
        {
            int m = get_measures(perf()->get_tick());
            if (m != m_measures)
                (void) apply_length(m);
        }
        else
            ev.mod_timestamp(get_length());             /* adjust tick      */

        if (recording())
        {
            if (perf()->is_pattern_playing())           /* playhead moving  */
            {
                if (check_oneshot_recording())
                    return true;

                if (ev.is_note_on() && m_rec_vol > usr().preserve_velocity())
                    ev.note_velocity(m_rec_vol);        /* modify incoming  */

                /*
                 * We need to do this before adding the event. Issue #119.
                 */

                if (alter_recording() && ev.is_note())
                {
                    /*
                     * We want to quantize or tighten note-related events that
                     * comes in, This could potentially alter the note length
                     * by a couple of snaps. So what? Play better!
                     *
                     * Actually, it could result in zero-length notes.
                     */

                    if (quantizing())
                        (void) ev.quantize(snap(), get_length());
                    else if (tightening())
                        (void) ev.tighten(snap(), get_length());

                    if (notemapping())
                        perf()->repitch(ev);
                }
#if defined SEQ66_LINK_NEWEST_NOTE_ON_RECORD            /* has issues :-(   */
                m_events.append(ev);                    /* does *not* sort  */
                if (ev.is_note_off())                   /* later, tempo?    */
                    m_events.link_new_note();           /* one link no sort */

                modify(false);                          /* no notify call   */
#else
                add_event(ev);                          /* locks and sorts  */
#endif
            }
            else                                        /* use auto-step    */
            {
                /*
                 * Supports the step-edit (auto-step) feature; see banner.
                 */

                if (ev.is_note_off())
                {
                    m_last_tick += snap();
                    if (m_last_tick >= get_length())
                    {
                        loop_reset(true);
                        m_last_tick = 0;
                    }
                }
                else if (ev.is_note_on())
                {
                    /*
                     * For issue #97, check the last time-stamp only when
                     * one-shot is in force.  This allows normal Seq24
                     * looping-back when the end is reached.
                     */

                    bool add = true;
                    if (oneshot_recording())
                        add = m_last_tick < get_length();

                    if (add)
                    {
                        if (m_rec_vol != usr().preserve_velocity())
                            ev.note_velocity(m_rec_vol);    /* keep veloc.  */

                        ev.set_timestamp(mod_last_tick());  /* loop back    */

                        bool ok = add_note
                        (
                            snap() - m_events.note_off_margin(), ev
                        );
                        if (ok)
                        {
#if defined USE_THIS_CODE
                            if (oneshot_recording())        /* update stuff */
                                (void) verify_and_link();
                                perf()->set_needs_update();
                            else                            /* FIXME */
#endif
                                ++m_notes_on;
                        }
                    }
                }
                else
                {
                    if (rc().verbose())
                        ev.print();
                }
            }
        }
        if (m_thru)
            put_event_on_bus(ev);

        /*
         * We don't need to link note events until a note-off comes in.
         * Commenting this out has no apparently effect, but we still can
         * get extra long notes. (ca 2024-11-26)
         *
         * ca 2024-12-27 Shouldn't this be verify_and_link()???
         *
         *      (void) m_events.link_new();
         */

        if (ev.is_note_off())
            (void) m_events.verify_and_link();
    }
    return result;
}

/**
 *  Sets the dirty flags for names, main, and performance.  These flags are
 *  meant for causing user-interface refreshes, not for performance
 *  modification.
 *
 *  m_dirty_names is set to false in is_dirty_names(); m_dirty_main is set to
 *  false in is_dirty_main(); m_dirty_perf is set to false in
 *  is_dirty_perf().
 *
 * \threadunsafe
 */

void
sequence::set_dirty_mp ()
{
    m_dirty_names = m_dirty_main = m_dirty_perf = true;
}

/**
 *  Call set_dirty_mp() and then sets the dirty flag for editing. Note that it
 *  does not call performer::modify().
 */

void
sequence::set_dirty ()
{
    set_dirty_mp();
    m_dirty_edit = true;
}

/**
 *  Returns the value of the dirty names (heh heh) flag, and sets that
 *  flag to false.  Not sure that we need to lock a boolean on modern
 *  processors.
 *
 *  At this point, another thread might read the initial value of the flag,
 *  before it is falsified.  Since it is used to initiate updates in callers,
 *  the penalty is that the caller updates when it doesn't have to.
 *
 * \threadsafe
 *
 * \return
 *      Returns the dirty status.
 */

bool
sequence::is_dirty_names () const
{
    bool result = m_dirty_names;        /* atomic   */
    m_dirty_names = false;              /* mutable  */
    return result;
}

/**
 *  Returns the value of the dirty main flag, and sets that flag to false
 *  (i.e. resets it).  This flag signals that a redraw is needed from
 *  recording.
 *
 * \threadsafe
 *
 * \return
 *      Returns the dirty status.
 */

bool
sequence::is_dirty_main () const
{
    bool result = m_dirty_main;         /* atomic   */
    m_dirty_main = false;               /* mutable  */
    return result;
}

/**
 *  Returns the value of the dirty performance flag, and sets that
 *  flag to false.
 *
 * \threadsafe
 *
 * \return
 *      Returns the dirty status.
 */

bool
sequence::is_dirty_perf () const
{
    bool result = m_dirty_perf;         /* atomic   */
    m_dirty_perf = false;               /* mutable  */
    return result;
}

/**
 *  Returns the value of the dirty edit flag, and sets that flag to false.
 *  The m_dirty_edit flag is set by the function set_dirty().
 *
 * \threadsafe
 *
 * \return
 *      Returns the dirty status.
 */

bool
sequence::is_dirty_edit () const
{
    bool result = m_dirty_edit;         /* atomic   */
    m_dirty_edit = false;               /* mutable  */
    return result;
}

/**
 *  Plays a note from the piano roll on the main bus on the master MIDI
 *  buss.  It flushes a note to the midibus to preview its sound, used by
 *  the virtual piano.
 *
 * \threadsafe
 *
 * \param note
 *      The note to play.  It is not checked for range validity, for the sake
 *      of speed.
 */

void
sequence::play_note_on (int note)
{
    automutex locker(m_mutex);
    event e(0, EVENT_NOTE_ON, midibyte(note), midibyte(m_note_on_velocity));
    if (rc().investigate())
        perf()->repitch(e);

    master_bus()->play_and_flush(m_true_bus, &e, midi_channel(e));
}

/**
 *  Turns off a note from the piano roll on the main bus on the master MIDI
 *  buss.
 *
 * \threadsafe
 *
 * \param note
 *      The note to turn off.  It is not checked for range validity, for the
 *      sake of speed.
 */

void
sequence::play_note_off (int note)
{
    automutex locker(m_mutex);
    event e(0, EVENT_NOTE_OFF, midibyte(note), midibyte(m_note_on_velocity));
    if (rc().investigate())
        perf()->repitch(e);

    master_bus()->play_and_flush(m_true_bus, &e, midi_channel(e));
}

/*
 * Track-specific trigger functions. The performer object calls these functions
 * on behalf of the user-interface.
 */

bool
sequence::clear_triggers ()
{
    automutex locker(m_mutex);
    int count = m_triggers.count();
    bool result = count > 0;
    m_triggers.clear();
    if (result)
        modify(false);                  /* issue #90 flag change w/o notify */

    return result;
}

void
sequence::print_triggers () const
{
    automutex locker(m_mutex);
    m_triggers.print(m_name);
}

/**
 *  Adds a trigger.  A pass-through function that calls triggers::add().
 *  See that function for more details.
 *
 * \threadsafe
 *
 * \param tick
 *      The time destination of the trigger.
 *
 * \param len
 *      The duration of the trigger.
 *
 * \param offset
 *      The performance offset of the trigger. Defaults to 0.
 *
 * \param fixoffset
 *      If true, adjust the offset. Defaults to true.
 */

bool
sequence::add_trigger
(
    midipulse tick, midipulse len, midipulse offset,
    midibyte tpose, bool fixoffset
)
{
    automutex locker(m_mutex);
    m_triggers.add(tick, len, offset, tpose, fixoffset);
    modify(false);                      /* issue #90 flag change w/o notify */
    return true;
}

#if defined USE_INTERSECT_FUNCTIONS

/**
 *  This function examines each trigger in the trigger list.  If the given
 *  position is between the current trigger's tick-start and tick-end
 *  values, the these values are copied to the start and end parameters,
 *  respectively, and then we exit.  See triggers::intersect().
 *
 * \threadsafe
 *
 * \param position
 *      The position to examine.
 *
 * \param start
 *      The destination for the starting tick of the matching trigger.
 *
 * \param ender
 *      The destination for the ending tick of the matching trigger.
 *
 * \return
 *      Returns true if a trigger was found whose start/end ticks
 *      contained the position.  Otherwise, false is returned, and the
 *      start and end return parameters should not be used.
 */

bool
sequence::intersect_triggers
(
    midipulse position, midipulse & start, midipulse & ender
)
{
    automutex locker(m_mutex);
    return m_triggers.intersect(position, start, ender);
}

bool
sequence::intersect_triggers (midipulse position)
{
    automutex locker(m_mutex);
    return m_triggers.intersect(position);
}

/**
 *  This function examines each note in the event list.  If the given position
 *  is between the current note's on and off time values, the these
 *  values are copied to the start and end parameters, respectively, and the
 *  note value is copied to the note parameter, and then we exit.
 *
 * \threadsafe
 *
 * \param position
 *      The tick position to examine; where the mouse pointer is horizontally,
 *      already converted to a tick number.
 *
 * \param position_note
 *      This is the position of the mouse pointer vertically, already
 *      converted to a note number.
 *
 * \param [out] start
 *      The destination for the starting timestamp of the matching note.
 *
 * \param [out] ender
 *      The destination for the ending timestamp of the matching note.
 *
 * \param [out] note
 *      The destination for the note of the matching event.
 *
 * \return
 *      Returns true if a note-on event was found whose start/end ticks
 *      contained the position.  Otherwise, false is returned, and the
 *      start and end return parameters should not be used.
 */

bool
sequence::intersect_notes
(
    midipulse position, int position_note,
    midipulse & start, midipulse & ender, int & note
)
{
    automutex locker(m_mutex);
    auto on = m_events.begin();
    auto off = m_events.begin();
    while (on != m_events.end())
    {
        event & eon = eventlist::dref(on);
        if (position_note == eon.get_note() && eon.is_note_on())
        {
            off = on;                               /* for next "off"       */
            ++off;                                  /* hope this is it!     */

            /*
             *  Find the next Note Off event for the current Note On that
             *  matches the mouse position.
             */

            bool notematch = false;
            for ( ; off != m_events.end(); ++off)
            {
                event & eoff = eventlist::dref(off);
                if (eon.get_note() == eoff.get_note() && eoff.is_note_off())
                {
                    notematch = true;
                    break;
                }
            }
            if (notematch)
            {
                event & eoff = eventlist::dref(off);
                midipulse ontime = eon.timestamp();
                midipulse offtime = eoff.timestamp();
                if (ontime <= position && position <= offtime)
                {
                    start = eon.timestamp();
                    ender = eoff.timestamp();
                    note = eon.get_note();
                    return true;
                }
            }
        }
        ++on;
    }
    return false;
}

/**
 *  This function examines each event in the event list.  If the given
 *  position is between the current notes's timestamp-start and timestamp-end
 *  values, the these values are copied to the posstart and posend parameters,
 *  respectively, and then we exit.
 *
 * \threadsafe
 *
 * \param posstart
 *      The starting position to examine.
 *
 * \param posend
 *      The ending position to examine.
 *
 * \param status
 *      The desired status value.
 *
 * \param start
 *      The destination for the starting timestamp  of the matching trigger.
 *
 * \return
 *      Returns true if a event was found whose start/end timestamps contained
 *      the position.  Otherwise, false is returned, and the start and end
 *      return parameters should not be used.
 */

bool
sequence::intersect_events
(
    midipulse posstart, midipulse posend,
    midibyte status, midipulse & start
)
{
    automutex locker(m_mutex);
    midipulse poslength = posend - posstart;
    for (auto & eon : m_events)
    {
        if (eon.match_status(status))
        {
            midipulse ts = eon.timestamp();
            if (ts <= posstart && posstart <= (ts + poslength))
            {
                start = eon.timestamp();    /* side-effect return value */
                return true;
            }
        }
    }
    return false;
}

#endif  // defined USE_INTERSECT_FUNCTIONS

/**
 *  Grows a trigger.  See triggers::grow_trigger() for more information.  Also
 *  indicate we're modified.  Tricky, because if modify(true) were called, that
 *  would call on_sequence_change(), which causes a segfault.  Here, we need to
 *  modify the performer, but call a different notification function.
 *
 * \threadsafe
 *
 * \param tickfrom
 *      The desired from-value back which to expand the trigger, if necessary.
 *
 * \param tickto
 *      The desired to-value towards which to expand the trigger, if necessary.
 *
 * \param len
 *      The additional length to append to tickto for the check.
 *
 */

bool
sequence::grow_trigger (midipulse tickfrom, midipulse tickto, midipulse len)
{
    automutex locker(m_mutex);
    m_triggers.grow_trigger(tickfrom, tickto, len);
    modify(false);                      /* issue #90 flag change w/o notify */
    set_dirty_mp();                     /* force redraw                     */
    return true;
}

/**
 *  This grows a trigger continuously.
 */

bool
sequence::grow_trigger (midipulse tickfrom, midipulse tickto)
{
    automutex locker(m_mutex);
    m_triggers.grow_trigger(tickfrom, tickto, c_song_record_incr);
    modify(false);                      /* issue #90 flag change w/o notify */
    set_dirty_mp();                     /* force redraw                     */
    return true;
}

const trigger &
sequence::find_trigger (midipulse tick) const
{
    automutex locker(m_mutex);
    return m_triggers.find_trigger(tick);
}

/**
 *  Deletes a trigger, that brackets the given tick, from the trigger-list.
 *  See triggers::remove().
 *
 * \threadsafe
 *
 * \param tick
 *      Provides the tick to be used for finding the trigger to be erased.
 */

bool
sequence::delete_trigger (midipulse tick)
{
    automutex locker(m_mutex);
    bool result = m_triggers.remove(tick);
    if (result)
        modify(false);                  /* issue #90 flag change w/o notify */

    return result;
}

/**
 *  Sets m_trigger_offset and wraps it to m_length.  If m_length is 0, then
 *  m_trigger_offset is simply set to the parameter.
 *
 * \threadsafe
 *
 * \param trigger_offset
 *      The full trigger offset to set.
 */

void
sequence::set_trigger_offset (midipulse trigger_offset)
{
    automutex locker(m_mutex);
    if (get_length() > 0)
    {
        m_trigger_offset = trigger_offset % get_length();
        m_trigger_offset += get_length();
        m_trigger_offset %= get_length();
    }
    else
        m_trigger_offset = trigger_offset;
}

/**
 *  Splits a trigger.
 *
 * \threadsafe
 *
 * \param splittick
 *      The time location of the split.
 *
 * \param splittype
 *      The nature of the split.
 */

bool
sequence::split_trigger (midipulse splittick, trigger::splitpoint splittype)
{
    automutex locker(m_mutex);
    bool result =  m_triggers.split(splittick, splittype);
    if (result)
        modify(false);                  /* issue #90 flag change w/o notify */

    return result;
}

/**
 *  Adjusts trigger offsets to the length specified for all triggers, and undo
 *  triggers.
 *
 * \threadsafe
 *
 *  Might can get rid of this function?
 *
 * \param newlength
 *      The new length of the adjusted trigger.
 */

void
sequence::adjust_trigger_offsets_to_length (midipulse newlength)
{
    automutex locker(m_mutex);
    m_triggers.adjust_offsets_to_length(newlength);
}

/**
 *  Copies triggers to another location.
 *
 * \threadsafe
 *
 * \param starttick
 *      The current location of the triggers.
 *
 * \param distance
 *      The distance away from the current location to which to copy the
 *      triggers.
 */

void
sequence::copy_triggers (midipulse starttick, midipulse distance)
{
    automutex locker(m_mutex);
    m_triggers.copy(starttick, distance);
}

bool
sequence::selected_trigger
(
    midipulse droptick, midipulse & tick0, midipulse & tick1
)
{
    automutex locker(m_mutex);
    bool result = m_triggers.select(droptick);
    tick0 = m_triggers.get_selected_start();
    tick1 = m_triggers.get_selected_end();
    return result;
}

/**
 *  Gets the last-selected trigger's start tick.
 *
 * \threadsafe
 *
 * \return
 *      Returns the tick_start() value of the last-selected trigger.  If no
 *      triggers are selected, then -1 is returned.
 */

midipulse
sequence::selected_trigger_start ()
{
    automutex locker(m_mutex);
    return m_triggers.get_selected_start();
}

/**
 *  Gets the selected trigger's end tick.
 *
 * \threadsafe
 *
 * \return
 *      Returns the tick_end() value of the last-selected trigger.  If no
 *      triggers are selected, then -1 is returned.
 */

midipulse
sequence::selected_trigger_end ()
{
    automutex locker(m_mutex);
    return m_triggers.get_selected_end();
}

/**
 *  Moves triggers in the trigger-list.
 *
 *  Note the dependence on the m_length member being kept in sync with the
 *  parent's value of m_length.
 *
 * \threadsafe
 *
 * \param starttick
 *      The current location of the triggers.
 *
 * \param distance
 *      The distance away from the current location to which to move the
 *      triggers.
 *
 * \param direction
 *      If true, the triggers are moved forward. If false, the triggers are
 *      moved backward.
 *
 * \param single
 *      If true, move only the first trigger encountered.
 *
 * \return
 *      Currently just returns true.
 */

bool
sequence::move_triggers
(
    midipulse starttick, midipulse distance,
    bool direction, bool single
)
{
    automutex locker(m_mutex);
    m_triggers.move(starttick, distance, direction, single);
    modify(false);                      /* issue #90 flag change w/o notify */
    return true;
}

/**
 *  Moves selected triggers as per the given parameters.
 *
\verbatim
          min_tick][0                1][max_tick
                            2
\endverbatim
 *
 *  The \a which parameter has three possible values:
 *
 *  -#  If we are moving the 0, use first as offset.
 *  -#  If we are moving the 1, use the last as the offset.
 *  -#  If we are moving both (2), use first as offset.
 *
 * \threadsafe
 *
 * \param tick
 *      The tick at which the trigger starts.
 *
 * \param adjustoffset
 *      Set to true if the offset is to be adjusted.
 *
 * \param which
 *      Selects which movement will be done, as discussed above.
 *
 * \return
 *      Returns the value of triggers::move_selected(), which indicates
 *      that the movement could be made.  Used in
 *      Seq24PerfInput::handle_motion_key().
 */

bool
sequence::move_triggers
(
    midipulse tick, bool adjustoffset, triggers::grow which
)
{
    automutex locker(m_mutex);
    bool result =  m_triggers.move_selected(tick, adjustoffset, which);
    if (result)
        modify(false);                  /* issue #90 flag change w/o notify */

    return result;
}

/**
 *  Used in the song-sequence grid TODO TODO TODO
 */

void
sequence::offset_triggers (midipulse tick, triggers::grow editmode)
{
    automutex locker(m_mutex);
    m_triggers.offset_selected(tick, editmode);
}

/**
 *  Get the ending value of the last trigger in the trigger-list.
 *
 * \threadsafe
 *
 * \return
 *      Returns the maximum trigger value.
 */

midipulse
sequence::get_max_trigger () const
{
    automutex locker(m_mutex);
    return m_triggers.get_maximum();
}

midipulse
sequence::get_max_timestamp () const
{
    automutex locker(m_mutex);
    return m_events.get_max_timestamp();
}

bool
sequence::get_trigger_state (midipulse tick) const
{
    automutex locker(m_mutex);
    return m_triggers.get_state(tick);
}

bool
sequence::transpose_trigger (midipulse tick, int transposition)
{
    automutex locker(m_mutex);
    bool result = m_triggers.transpose(tick, transposition);
    if (result)
        modify(false);                          /* no easy way to undo this */

    return result;
}

/**
 *  Returns a copy of the triggers for this sequence.  This function is
 *  basically a threadsafe version of sequence::triggerlist().
 *
 * \return
 *      Returns of copy of m_triggers.triggerlist().
 */

triggers::container
sequence::get_triggers () const
{
    automutex locker(m_mutex);
    return triggerlist();
}

/**
 *  Checks the list of triggers against the given tick.  If any
 *  trigger is found to bracket that tick, then true is returned, and
 *  the trigger is marked as selected.
 *
 * \param tick
 *      Provides the tick of interest.
 *
 * \return
 *      Returns true if a trigger is found that brackets the given tick;
 *      this is the return value of m_triggers.select().
 */

bool
sequence::select_trigger (midipulse tick)
{
    automutex locker(m_mutex);
    return m_triggers.select(tick);
}

/**
 *  Unselects the desired trigger.
 *
 * \param tick
 *      Indicates the trigger to be unselected.
 */

bool
sequence::unselect_trigger (midipulse tick)
{
    automutex locker(m_mutex);
    return m_triggers.unselect(tick);
}

/**
 *  Unselects all triggers.
 *
 * \return
 *      Returns the m_triggers.unselect() return value.
 */

bool
sequence::unselect_triggers ()
{
    automutex locker(m_mutex);
    return m_triggers.unselect();
}

/**
 *  Deletes the first selected trigger that is found.
 *
 * \return
 *      Returns true if a trigger was found and deleted.
 */

bool
sequence::delete_selected_triggers ()
{
    automutex locker(m_mutex);
    bool result = m_triggers.remove_selected();
    if (result)
        modify(false);                  /* issue #90 flag change w/o notify */

    return result;
}

/**
 *  Copies and deletes the first selected trigger that is found.
 *
 * \return
 *      Returns true if a trigger was found, copied, and deleted.
 */

bool
sequence::cut_selected_triggers ()
{
    automutex locker(m_mutex);
    copy_selected_triggers();                   /* locks itself (recursive) */
    return m_triggers.remove_selected();
}

/**
 *  First, this function clears any unpasted middle-click tick setting.
 *  Then it copies the first selected trigger that is found.
 */

bool
sequence::copy_selected_triggers ()
{
    automutex locker(m_mutex);
    set_trigger_paste_tick(c_no_paste_trigger);
    m_triggers.copy_selected();
    return true;
}

/**
 *  If there is a copied trigger, then this function grabs it from the trigger
 *  clipboard and adds it.
 *
 *  Why isn't this protected by a mutex?  We will enable this if anything bad
 *  happens, such as a deadlock, or corruption, that we can prove happens
 *  here.
 *
 * \param paste_tick
 *      A new parameter that provides the tick for pasting, or
 *      c_no_paste_trigger (-1) if there is none.
 */

bool
sequence::paste_trigger (midipulse paste_tick)
{
    automutex locker(m_mutex);
    m_triggers.paste(paste_tick);
    return true;
}

/**
 *  Provides a helper function simplify and speed up performer ::
 *  reset_sequences().  In Live mode, the user controls playback, while in
 *  Song mode, the performance/song editor controls playback.  This function
 *  used to be called "reset()".
 *
 * \param song_mode
 *      Set to true if song mode is in force.  This setting corresponds to
 *      sequence::playback::song.  False (the default) corresponds to
 *      sequence::playback::live.
 */

void
sequence::stop (bool songmode)
{
    bool state = armed();
    off_playing_notes();
    zero_markers();                         /* sets the "last-tick" value   */
    if (recording())                        /* ca 2023-04-25                */
        (void) verify_and_link();

    set_armed(songmode ? false : state);
    m_next_boundary = 0;
}

/**
 *  A pause version of stop().  It still includes the note-shutoff capability
 *  to prevent notes from lingering.  Note that we do not call set_arm(false);
 *  it disarms the sequence, which we do not want upon pausing.
 *
 * \param song_mode
 *      Set to true if song mode is in force.  This setting corresponds to
 *      performer::playback::song.  False (the default) corresponds to
 *      performer::playback::live.
 */

void
sequence::pause (bool song_mode)
{
    bool state = armed();
    off_playing_notes();
    if (! song_mode)
        set_armed(state);

    if (recording())                        /* ca 2023-04-25                */
        (void) verify_and_link();
}

/**
 *  Sets the draw-trigger iterator to the beginning of the trigger list.
 *
 * \threadsafe
 */

void
sequence::reset_draw_trigger_marker ()
{
    automutex locker(m_mutex);
    m_triggers.reset_draw_trigger_marker();
}

/**
 *  A new function provided so that we can find the minimum and maximum notes
 *  with only one (not two) traversal of the event list.
 *
 * \todo
 *      For efficency, we should calculate this only when the event set
 *      changes, and save the results and return them if good.
 *
 * \threadsafe
 *
 * \param lowest
 *      A reference parameter to return the note with the lowest value.
 *      if there are no notes, then it is set to max_midi_value(), and
 *      false is returned.
 *
 * \param highest
 *      A reference parameter to return the note with the highest value.
 *      if there are no notes, then it is set to 0, and false is returned.
 *
 * \return
 *      If there are no notes or tempo events in the list, then false is
 *      returned, and the results should be disregarded.  If true is returned,
 *      but there are only tempo events, then the low/high range is 0 to 127.
 */

bool
sequence::minmax_notes (int & lowest, int & highest) // const
{
    automutex locker(m_mutex);
    bool result = false;
    int low = int(max_midi_value());
    int high = -1;
    for (auto & er : m_events)
    {
        if (er.is_strict_note())
        {
            if (er.get_note() < low)
            {
                low = er.get_note();
                result = true;
            }
            else if (er.get_note() > high)
            {
                high = er.get_note();
                result = true;
            }
        }
        else if (er.is_tempo())
        {
            midibyte notebyte = tempo_to_note_value(er.tempo());
            if (notebyte < low)
                low = notebyte;
            else if (notebyte > high)
                high = notebyte;

            result = true;
        }
    }
    lowest = low;
    highest = high;
    return result;
}

/**
 *  Each call to seqdata() fills the passed references with a events
 *  elements, and returns true.  When it has no more events, returns a
 *  false.
 *
 *  Note that, before the first call to draw a sequence, the sequence::begin()
 *  function must be called.
 *
 * \param [out] niout
 *      Provides a pointer destination for a structure hold all of the values
 *      for a note.  Saves a lot of stack pushes.  The note_info class is
 *      nested in the sequence class, which is a friend.
 *
 * \param evi
 *      A caller-provided iterator.  Thus, it won't interfere with other
 *      callers.
 *
 * \return
 *      Returns a sequence::draw value:  linked, note_on, note_off, or finish.
 *      Note that the new value sequence::draw::tempo could be returned, as
 *      well. Note that a return of draw::finish indicates to exit a drawing
 *      loop.
 */

sequence::draw
sequence::get_next_note
(
    note_info & niout,
    event::buffer::const_iterator & evi
) const
{
    automutex locker(m_mutex);
    while (evi != m_events.cend())
    {
#if defined SEQ66_USE_ACTION_IN_PROGRESS_FLAG
        if (m_events.action_in_progress())      /* atomic boolean check     */
            return draw::finish;                /* bug out immediately      */
#endif

        draw status = get_note_info(niout, evi);
        if (status != draw::none)
            return status;                      /* must ++evi after call    */

        ++evi;
    }
    return draw::finish;
}

/**
 *  Copies important information for drawing a note event.
 *
 * \param evi
 *      The current event iterator value.  It is not checked, and is not
 *      iterated after getting the data.
 */

sequence::draw
sequence::get_note_info
(
    note_info & niout,
    event::buffer::const_iterator & evi
) const
{
    const event & drawevent = eventlist::cdref(evi);
    bool isnoteon = drawevent.is_note_on();
    bool islinked = drawevent.is_linked();
    niout.ni_tick_finish    = 0;
    niout.ni_tick_start     = drawevent.timestamp();
    niout.ni_note           = drawevent.get_note();             /* ie. d0() */
    niout.ni_selected       = drawevent.is_selected();
    niout.ni_velocity       = drawevent.note_velocity();
    if (isnoteon)
    {
        if (islinked)
        {
            niout.ni_tick_finish = drawevent.link()->timestamp();
            return draw::linked;
        }
        else
            return draw::note_on;
    }
    else if (drawevent.is_note_off() && ! islinked)
    {
        return draw::note_off;
    }
    else if (drawevent.is_tempo())
    {
        midibpm bpm = drawevent.tempo();
        midibyte notebyte = tempo_to_note_value(bpm);
        niout.ni_note = int(notebyte);
        niout.ni_velocity = int(bpm + 0.5);
        niout.ni_non_note = true;

        /*
         * Hmmmm, must check if tempo events ever have a link. No, they
         * don't but they might need one to properly draw tempo changes!!!
         * We might just draw tempo as a circle for simplicity.
         */

        if (islinked)
            niout.ni_tick_finish = drawevent.link()->timestamp();
        else
            niout.ni_tick_finish = get_length();

        /*
         * Tempo needs to be retained.  This is good only for drawing a
         * horizontal tempo line; we need a way to return both a starting
         * tempo and ending tempo.  Return the latter in velocity?
         */

        return draw::tempo;
    }
    else if (drawevent.is_program_change())
    {
        niout.ni_tick_finish = niout.ni_tick_start;
        niout.ni_non_note = true;
        return draw::program;
    }
    else if (drawevent.is_controller())
    {
        niout.ni_non_note = true;
        return draw::controller;
    }
    else if (drawevent.is_pitchbend())
    {
        niout.ni_non_note = true;
        return draw::pitchbend;
    }
    return draw::none;
}

/**
 *  Checks for non-terminated notes.
 *
 * \return
 *      Returns true if there is at least one non-terminated linked note in
 *      the interval.
 */

bool
sequence::reset_interval
(
    midipulse t0, midipulse t1,
    event::buffer::const_iterator & it0,
    event::buffer::const_iterator & it1
) const
{
    bool result = false;
    bool got_beginning = false;
    it0 = m_events.cbegin();
    it1 = m_events.cend();
    for (auto iter = cbegin(); ! cend(iter); ++iter)
    {
        midipulse t = iter->timestamp();
        if (t >= t0)
        {
            if (! got_beginning)
            {
                it0 = iter;                                 /* side-effect  */
                got_beginning = true;
            }
            if (iter->is_linked())
            {
                event::buffer::const_iterator ev = iter->link();
                if (ev->timestamp() >= t1)
                {
                    result = true;  // What about terminating iterator ??
                    break;
                }
            }
        }
        else if (t >= t1)
        {
            it1 = iter;                                     /* side-effect  */
            break;
        }
    }
    return result;
}

/**
 *  Get the next event in the event list.  Then set the status and control
 *  character parameters using that event.  This function requires that
 *  sequence::cbegin() be called to reset to the beginning of the events list.
 *
 * \param status
 *      Provides a pointer to the MIDI status byte to be set, as a way to
 *      retrieve the event.
 *
 * \param cc
 *      The return pointer for the control value.
 *
 * \param [out] evi
 *      An iterator return value for the next event found.  The caller might
 *      want to check if it is a Tempo event.  Do not use this iterator if
 *      false is returned!  For consistency with get_next_event_match(),
 *      we rely on the caller to increment this pointer for the next call.
 *
 * \return
 *      Returns true if the data is useable, and false if there are no more
 *      events.
 */

bool
sequence::get_next_event
(
    midibyte & status, midibyte & cc,
    event::buffer::const_iterator & evi
)
{
    automutex locker(m_mutex);
    bool result = evi != m_events.end();
    if (result)
    {
#if defined SEQ66_USE_ACTION_IN_PROGRESS_FLAG
        if (m_events.action_in_progress())      /* atomic boolean check     */
            return false;
#endif

        midibyte d1;                            /* will be ignored          */
        const event & ev = eventlist::cdref(evi);
        status = ev.get_status();
        ev.get_data(cc, d1);
    }
    return result;
}

/**
 *  This function makes the caller responsible for providing and maintaining
 *  the iterator, so that there are no conflicting operations on
 *  m_draw_iterator from seqdata, seqevent, seqroll, and perfroll.
 *
 *  This function returns the whole event, rather than filling in a bunch of
 *  parameters.  In addition, it allows Meta events to be found.  Gets
 *  the next event in the event list that matches the given status and control
 *  character.  Then set the rest of the parameters parameters using that
 *  event.  If the status is the new value EVENT_ANY, then any event will be
 *  obtained.
 *
 *  Note the usage of event::is_desired(status, cc); either we have a control
 *  change with the right CC or it's a different type of event.
 *
 * \param status
 *      The type of event to be obtained.  The special value EVENT_ANY can be
 *      provided so that no event statuses are filtered.
 *
 * \param cc
 *      The continuous controller value that might be desired.
 *
 * \param [out] evi
 *      An iterator return value for the next event found.  The caller might
 *      want to check if it is a Tempo event.  Do not use this iterator if
 *      false is returned!  The caller must increment it for the next call, just
 *      as for get_next_event().
 *
 * \return
 *      Returns true if the current event was one of the desired ones, or was
 *      a Tempo or Time Signature event (the Meta events whose drawing in qseqdata
 *      supported). In this case, the caller <i> must </i> increment the
 *      iterator.
 */

bool
sequence::get_next_event_match
(
    midibyte status, midibyte cc,
    event::buffer::const_iterator & evi
)
{
    automutex locker(m_mutex);
    bool ismeta = event::is_meta_msg(status);
    while (evi != m_events.end())
    {
#if defined SEQ66_USE_ACTION_IN_PROGRESS_FLAG
        if (m_events.action_in_progress())      /* atomic boolean check     */
            return false;                       /* bug out immediately      */
#endif

        const event & drawevent = eventlist::cdref(evi);
        bool ok = drawevent.match_status(status);
        if (ok && ismeta)
        {
            if (! event::is_meta_text_msg(status)) /* do all text the same  */
                ok = drawevent.channel() == cc; /* avoids redundant check   */

            if (ok)
                return true;                    /* must ++evi after call    */
        }
        else
        {
            if (! ok)
                ok = status == EVENT_ANY;

            if (ok)
            {
                midibyte d0;
                drawevent.get_data(d0);
                ok = event::is_desired_cc_or_not_cc(status, cc, d0);
                if (ok)
                    return true;                /* must ++evi after call    */
            }
        }
        ++evi;                                  /* keep going here          */
    }
    return false;
}

/**
 *  Similar to get_next_event_match(), but specific to Meta events and can
 *  be restricted to a limited range of time.
 *
 * \param metamsg
 *      Provides the type of Meta message. In Seq66, this value is stored
 *      in the "channel" member. A special case is EVENT_META_TEXT_EVENT,
 *      which covers a number of different textual meta messages.
 *
 * \param [out] evi
 *      An iterator return value for the next event found.  The caller might
 *      want to check it.  Do not use this iterator if false is returned!  The
 *      caller must increment it for the next call, just as for
 *      get_next_event() or get_next_event_match().
 *
 * \param start
 *      Provides the starting point for the search.  Events before this tick
 *      are ignored.  The default value is 0, the beginning of the pattern.
 *
 * \param range
 *      Provides how far to look for a time signature. The default is
 *      c_null_midipulse, which means just detect the first Meta event no
 *      matter how deep into the pattern. Another useful value is the snap()
 *      value from the seqedit. In that case the event time must be within
 *      \a start + \a range ticks.
 *
 * \return
 *      Returns true if the Meta event was detected before the range limit was
 *      reached.
 */

bool
sequence::get_next_meta_match
(
    midibyte metamsg,
    event::buffer::const_iterator & evi,
    midipulse start,
    midipulse range
)
{
    automutex locker(m_mutex);
    if (range != c_null_midipulse)
        range += start;

    while (evi != m_events.end())
    {
#if defined SEQ66_USE_ACTION_IN_PROGRESS_FLAG
        if (m_events.action_in_progress())      /* atomic boolean check     */
            return false;                       /* bug out immediately      */
#endif

        const event & drawevent = eventlist::cdref(evi);
        if (drawevent.is_meta())
        {
            bool match = metamsg == EVENT_META_TEXT_EVENT ?
                event::is_meta_text_msg(drawevent.channel()) :
                drawevent.channel() == metamsg ;

            if (match)
            {
                midipulse tick = evi->timestamp();
                if (tick >= start)
                {
                    if (tick < range || range == c_null_midipulse)
                        return true;
                }
            }
        }
        ++evi;                                  /* keep going here          */
    }
    return false;
}

/**
 *  Get the next trigger in the trigger list, and set the parameters based
 *  on that trigger.
 */

bool
sequence::next_trigger (trigger & trig)
{
    trig = m_triggers.next();
    return trig.is_valid();
}

/**
 * \setter m_last_tick
 *      This function used to be called "set_orig_tick()", now renamed to
 *      match up with get_last_tick().
 *
 * \threadsafe
 */

void
sequence::set_last_tick (midipulse tick)
{
    automutex locker(m_mutex);
    if (is_null_midipulse(tick))
        tick = m_length;

    m_last_tick = tick;
}

/**
 *  Get the performer's current tick so that setting position by a click works
 *  properly.
 */

midipulse
sequence::get_tick () const
{
    if (expanded_recording())
    {
        return perf()->get_tick();
    }
    else
    {
        if (get_length() > 0)
            return perf()->get_tick() % get_length();
        else
            return perf()->get_tick();
    }
}

/**
 *  Returns the last tick played, and is used by the editor's idle function.
 *  If m_length is 0, this function returns m_last_tick - m_trigger_offset, to
 *  avoid an arithmetic exception.  Should we return 0 instead?
 *
 *  Note that seqroll calls this function to help get the location of the
 *  progress bar.  What does perfedit do?
 */

midipulse
sequence::get_last_tick () const
{
    return get_length() > 0 ?
        (m_last_tick + get_length() - m_trigger_offset) % get_length() :
        m_last_tick - m_trigger_offset
        ;
}

/**
 *  Sets the MIDI buss/port number to dump MIDI data to. When first called,
 *  there is generally no performer yet, so all that it done is to set the
 *  nominal buss. Later, set_parent() is called and here we then determine
 *  the true system MIDI bus in use by this sequence.
 *
 * \threadsafe
 *
 * \param nominalbus
 *      The MIDI buss to set as the buss number for this sequence.  Also
 *      called the "MIDI port" number.  This number is not necessarily the
 *      true system bus number. We no longer check that nominalbus !=
 *      m_nominal_bus. Saves some trouble.
 *
 * \param user_change
 *      If true (the default value is false), the user has decided to change
 *      this value, and we might need to modify the performer's dirty flag, so
 *      that the user gets prompted for a change.  This is a response to
 *      GitHub issue #47, where buss changes do not cause a prompt to save the
 *      sequence.
 */

bool
sequence::set_midi_bus (bussbyte nominalbus, bool user_change)
{
    automutex locker(m_mutex);
    bool result = is_valid_buss(nominalbus);
    if (result)
    {
        m_nominal_bus = nominalbus;             /* log the putative buss    */
        if (is_nullptr(perf()))                 /* "parent" is not yet set  */
        {
            m_true_bus = null_buss();           /* an invalid value         */
        }
        else
        {
            off_playing_notes();                /* off notes except initial */
            m_true_bus = perf()->true_output_bus(nominalbus);
            if (is_null_buss(m_true_bus))
                m_true_bus = nominalbus;        /* buss no longer exists    */

            if (user_change)
                modify();                       /* no easy way to undo this */

#if defined USE_THIS_CALL                       /* see sequence::modify()   */
            notify_change(user_change);         /* better than set_dirty()  */
#endif
            set_dirty();                        /* for display updating     */
        }
    }
    return result;
}

/**
 *  Similar to set_midi_bus(), but supports the new and optional input
 *  buss.
 */

bool
sequence::set_midi_in_bus (bussbyte nominalbus, bool user_change)
{
    automutex locker(m_mutex);
    bool result = is_valid_buss(nominalbus);
    if (result)
    {
        m_nominal_in_bus = nominalbus;          /* log the putative buss    */
        if (is_nullptr(perf()))                 /* "parent" is not yet set  */
        {
            m_true_in_bus = null_buss();        /* an invalid value         */
        }
        else
        {
            m_true_in_bus = perf()->true_input_bus(nominalbus);
            if (is_null_buss(m_true_in_bus))
                m_true_in_bus = nominalbus;     /* named buss no longer exists  */

            if (user_change)
                modify();                       /* no easy way to undo this     */

#if defined USE_THIS_CALL                       /* see sequence::modify()   */
            notify_change(user_change);         /* more reliable than set dirty */
#endif
            set_dirty();                        /* this is for display updating */
        }
    }
    return result;
}

/**
 *  Sets the length (m_length) and adjusts triggers for it, if desired.
 *  This function is called in qseqeditframe64, when the user changes
 *  beats/bar or the sequence length in measures.  This function is also called
 *  when reading a MIDI file.
 *
 *  There's an issue, though.  If the application is compiled to use the
 *  original std::list container for MIDI events, that implementation sorts
 *  the container after every event insertion.  If the application is compiled
 *  to used the std::map container (to speed up the reading of large MIDI
 *  files *greatly*), sorting happens automatically.  But, if we use the
 *  original std::list implementation, but leave the sorting until later (to
 *  speed up the reading of large MIDI files *greatly*), then the
 *  verify_and_link() call that happens with every new event happens before
 *  the events are sorted, and the result is elongated notes showing up in the
 *  pattern slot in the main window.  Therefore, we need a way to skip the
 *  verification when reading a MIDI file, and do the verification only after
 *  all events are read.
 *
 *  That function calculates the length in ticks:
 *
\verbatim
    L = M x B x 4 x P / W
        L == length (ticks or pulses)
        M == number of measures
        B == beats per measure
        P == pulses per quarter-note
        W == beat width in beats per measure
\endverbatim
 *
 *  For our "b4uacuse" MIDI file, M can be about 100 measures, B is 4,
 *  P can be 192 (but we want to support higher values), and W is 4.
 *  So L = 100 * 4 * 4 * 192 / 4 = 76800 ticks.  Seems small.
 *
 * Issue #88:
 *
 *      If the time signature is something like 4/16, then the pattern length
 *      will be much less than PPQN * 4.   This affects the set_parent()
 *      function.
 *
 * \threadsafe
 *
 * \param len
 *      The length value to be set.  If it is smaller than ppqn/4, then
 *      it is set to that value, unless it is zero, in which case m_length is
 *      used and does not change.  It also sets the length value for the
 *      sequence's triggers.
 *
 * \param adjust_triggers
 *      If true, m_triggers.adjust_offsets_to_length() is called.  The value
 *      defaults to true.
 *
 * \param verify
 *      This new parameter defaults to true.  If true, verify_and_link() is
 *      called.  Otherwise, it is not, and the caller should call this
 *      function with the default value after reading all the events.
 *
 * \return
 *      Returns true if the length value actually changed.
 */

bool
sequence::set_length (midipulse len, bool adjust_triggers, bool verify)
{
    automutex locker(m_mutex);
    bool result = len != m_length;
    if (result)
    {
        bool was_playing = armed();             /* was it armed?            */
        set_armed(false);                       /* mute the pattern         */
        if (len > 0)
        {
            if (len < midipulse(m_ppqn / 4))
                len = midipulse(m_ppqn / 4);

            m_length = len;
            result = true;
        }
        else
            len = get_length();

        m_events.set_length(len);
        m_triggers.set_length(len);             /* must precede adjustment  */
        if (adjust_triggers)
            m_triggers.adjust_offsets_to_length(len);

        if (verify)
            (void) verify_and_link();

        if (was_playing)                        /* start up and refresh     */
            set_armed(true);
    }
    return result;
}

/**
 *  Sets the sequence length based on the first four parameters.  There's an
 *  implicit "adjust-triggers = true" parameter used in this function.  Please
 *  note that there is an overload that takes only a measure number and uses
 *  the current beats/bar, PPQN, and beat-width values of this sequence.  The
 *  calculate_unit_measure() function is called, but won't change any values
 *  just because the length (number of measures) changed.
 *
 * \warning
 *      The measures calculation is somewhat useless if the BPM (beats/minute)
 *      varies throughout the song.
 *
 * \param bpb
 *      Provides the beats per bar (measure). If 0, the current value is used.
 *
 * \param ppqn
 *      Provides the pulses-per-quarter-note to apply to the length
 *      application. If 0, the current value is used.
 *
 * \param bw
 *      Provides the beatwidth (typically 4) from the time signature. If 0, the
 *      current value is used.
 *
 * \param measures
 *      Provides the number of measures the sequence should cover, obtained
 *      from the user-interface. Defaults to 0, which means we calculate the
 *      current measures.
 *
 * \param user_change
 *      If true, this is a modification, not an initialization. The default
 *      is false.
 *
 * \return
 *      Returns true if the modification was initiated by the user.
 */

bool
sequence::apply_length
(
    int bpb, int ppq, int bw,
    int measures, bool user_change
)
{
    bool result = false;
    bool reset_L_R_markers = false;
    if (bpb == 0)
    {
        bpb = get_beats_per_bar();
    }
    else
    {
        if (bpb != get_beats_per_bar())     /* ca 2024-12-10                */
            reset_L_R_markers = true;

        set_beats_per_bar(bpb, user_change);
    }

    if (ppq == 0)
        ppq = int(get_ppqn());
    else
        change_ppqn(ppq);                   /* rarely changed; rescales if  */

    if (bw == 0)
    {
        bw = get_beat_width();
    }
    else
    {
        if (bw != get_beat_width())
            reset_L_R_markers = true;

        set_beat_width(bw, user_change);
    }
    if (measures == 0)
    {
        (void) unit_measure(true);          /* reset the unit-measure       */
        measures = get_measures(0);         /* calculate the current bars   */
        result = set_length(seq66::measures_to_ticks(bpb, ppq, bw, measures));
    }
    else
        result = set_measures(measures, user_change);   /* and set_length() */

    if (result)
        (void) unit_measure(true);          /* for progress and redrawing   */

    if (reset_L_R_markers)
    {
        if (not_nullptr(perf()))
        {
            perf()->set_left_tick(0);       /* move L marker to the start   */
            perf()->set_right_tick(0);      /* move R marker to measure end */
        }
    }
    return result;
}

/**
 *  Extends the length of the sequence.  Calls set_length() with the new
 *  length and its default parameters.  Currently used only when consolidating
 *  patterns into an SMF 0 track.
 *
 * \param len
 *      The new length of the sequence.
 *
 * \return
 *      Returns the new number of measures.
 */

bool
sequence::extend_length ()
{
    automutex locker(m_mutex);
    midipulse len = m_events.get_max_timestamp();
    bool result = len > get_length();
    if (len > get_length())
    {
        int measures = int(double(len) / unit_measure(true) + 0.5); /* TIME */
        len = m_unit_measure * measures;
        result = set_length(len, false, false); /* no trig adjust or verify */
    }
    return result;
}

/**
 *  Doubles the length of the pattern. This is always a modification.
 */

bool
sequence::double_length ()
{
    automutex locker(m_mutex);
    int m = get_measures();
    bool result = m > 0;
    if (result)
    {
        m *= 2;
        result = apply_length(m);
        if (result)
            modify();                               /* have pending changes */
    }
    return result;
}

/**
 *  Notifies the parent performer's subscribers that the sequence has
 *  changed in some way not based on a trigger or action, and is hence a
 *  modify action.  [The default change value is "yes" for performer ::
 *  notify_sequence_change().]
 *
 * \param userchange
 *      Indicates if the change was requested by a user or done in the
 *      "normal" course of operations. The default is true.
 */

void
sequence::notify_change (bool userchange)
{
    if (not_nullptr(perf()))
    {
        performer::change mod = userchange ?
            performer::change::yes : performer::change::no ;

        perf()->notify_sequence_change(seq_number(), mod);
    }
}

/**
 *  Notifies the parent performer's subscribers that the sequence has
 *  changed state based on a trigger or action.  This will not cause a modify
 *  action.
 */

void
sequence::notify_trigger ()
{
    if (not_nullptr(perf()))
        perf()->notify_trigger_change(seq_number(), performer::change::no);
}

/**
 *  Sets the playing state of this sequence.  When playing, and the sequencer
 *  is running, notes get dumped to the ALSA buffers.
 *
 *  If we're turning play on, we now (2021-04-27 issue #49) turn song-mute
 *  off, so that the pattern will not get turned off when playback starts.
 *  This covers the case where the user enables and then disables a mute
 *  group, which sets song-mute to true on all sequences.
 *
 * \param p
 *      Provides the playing status to set.  True means to turn on the
 *      playing, false means to turn it off, and turn off any notes still
 *      playing.
 */

bool
sequence::set_armed (bool p)
{
    automutex locker(m_mutex);
    bool result = p != armed();
    if (result)
    {
        armed(p);
        if (p)
            set_song_mute(false);                   /* see banner notes     */
        else
            off_playing_notes();

        /*
         * This call is meant to allow the grid slot and the pattern-editor
         * armed statuses to stay in sync. However, if colours.midi is opened
         * and a pattern is opened, then b4uacuse.mid is selected (as
         * a recent file) and a pattern is opened, then we go back to
         * colours.midi, a segfault occurs due to the pattern editor
         * being bogus. What to do???
         *
         *      notify_change(false);
         */

        set_dirty();
        m_queued = m_one_shot = false;
        perf()->announce_pattern(seq_number());     /* for issue #89        */
#if defined SEQ66_PLATFORM_DEBUG_TMI
        printf("seq %d: playing %s\n", int(seq_number()), p ? "on" : "off");
#endif
    }

    /*
     * Let's move these above so that announce_pattern() behaves properly.
     * Whenever playing state changes, we are unqueued.  Not yet sure
     * about one-shot.
     *
     * m_queued = false;
     * m_one_shot = false;
     */

    return result;
}

/**
 *  This sets the status of basic recording, with no other wrinkles such as
 *  quantization.
 *
 *  This function sets m_notes_on to 0, only if the recording status has
 *  changed.  It is called by set_recording().  We probably need to explicitly
 *  turn off all playing notes; not sure yet.
 *
 *  Except if already Thru and trying to turn recording (input) off, set input
 *  on here no matter what, because even if m_thru, input could have been
 *  replaced in another sequence.
 *
 * \param recordon
 *      Provides the desired status to set recording.
 *
 * \param toggle
 *      If true, ignore the first parameter and toggle the flag.  The default
 *      value is false.
 */

/**
 *  This function sets only the status of recording, regardless of alteration
 *  type.  And, if recording get turned off, the alteration type is set
 *  to alteration::none.
 *
 * Issue #129:
 *
 *      The mastermidibus::set_sequence_input() call ignores turning on
 *      alterations when an inputing sequence has already been selected.
 *      Fixed in mastermidibase.
 */

bool
sequence::set_recording (toggler flag)
{
    automutex locker(m_mutex);
    bool recordon = m_recording;
    if (flag == toggler::flip)
        recordon = ! recordon;
    else
        recordon = flag == toggler::on;

    bool result = master_bus()->set_sequence_input(recordon, this);
    if (result)
    {
        channel_match(false);
        m_recording = recordon;
        m_notes_on = 0;                 /* reset the step-edit note counter */
        m_last_tick = 0;
        if (recordon)
        {
            if (! perf()->record_by_buss() && perf()->record_by_channel())
                channel_match(true);
        }
        else
            m_record_alteration = alteration::none;

        set_dirty();
        notify_trigger();
    }
    return result;
}

/**
 *  Added some wrinkles from qseqeditframe64. If recording is on, and the
 *  flag is toggler::off, we want to disable basic recording only if the
 *  alteration is alteration::none.  Otherwise, we want to disable only
 *  the alteration option, leaving basic recording still active.
 *
 *  -   If toggler::on. Set the appropriate alteration and pass the flag
 *      to the basic set_recording() function above.
 *  -   If toggler::off.
 *      -   Set alteration::none. The next step also does that.
 *      -   If the specified alteration was none, pass the flag as above.
 *  -   If toggler::flip. See the code :-D
 */

bool
sequence::set_recording (alteration q, toggler flag)
{
    automutex locker(m_mutex);
    bool result = true;
    if (flag == toggler::on)
    {
        m_record_alteration = q;
        result = set_recording(toggler::on);
    }
    else if (flag == toggler::off)
    {
        m_record_alteration = alteration::none;

        /*
         * Don't change the status of recording just because
         * quantization/alteration is being turned off.
         *
         *  if (q == alteration::none)              // turn off alteration
         *      result = set_recording(toggler::off);
         */

        set_dirty();

        /*
         * notify_trigger();
         */

        notify_change(false);
    }
    else                                        /* toggler::flip            */
    {
        if (q == alteration::none)              /* plain basic recording    */
        {
            result = set_recording(toggler::flip);
        }
        else
        {
            if (q == m_record_alteration)
                m_record_alteration = alteration::none;
            else
                m_record_alteration = q;

            result = set_recording(toggler::flip);
        }
    }
    return result;
}

bool
sequence::set_recording_style (recordstyle rs)
{
    automutex locker(m_mutex);
    bool result = rs != recordstyle::max;
    if (result)
    {
        m_recording_style = rs;
        if (rs == recordstyle::overwrite)
            loop_reset(true);   /* on overwrite, always reset the sequence  */

        /*
         * Why do this?
         *
         * notify_trigger();                                   // tricky!  //
         */

        notify_change(false);                                   // xxxx
    }
    return result;
}

/**
 *  Sets the state of MIDI Thru.
 *
 * \param thru_active
 *      Provides the desired status to set the through state.
 *
 * \param toggle
 *      If true, ignore the first parameter and toggle the flag.  The default
 *      value is false.
 */

bool
sequence::set_thru (bool thruon, bool toggle)
{
    automutex locker(m_mutex);
    if (toggle)
        thruon = ! m_thru;

    bool result = thruon != m_thru;
    if (result)
    {
        /*
         * Except if already recording and trying to turn Thru (hence input)
         * off, set input to here no matter what, because even in m_recording,
         * input could have been replaced in another sequence.
         * LET's try putting in the original conditional.
         */

         if (! m_recording)
            result = master_bus()->set_sequence_input(thruon, this);

        if (result)
            m_thru = thruon;
    }
    return result;
}

void
sequence::snap (int st)
{
    automutex locker(m_mutex);
    m_snap_tick = midipulse(st);
    m_events.zero_len_correction(snap() / 2);
}

void
sequence::step_edit_note_length (int len)
{
    automutex locker(m_mutex);
    m_step_edit_note_length = midipulse(len);
}

void
sequence::loop_reset (bool reset)
{
    automutex locker(m_mutex);
    m_loop_reset = reset;
}

/**
 *  Sets the sequence name member, m_name.  This is the name shown in the top
 *  of a mainwnd pattern slot.
 *
 *  We now try to include the length of the sequences in measures at the end
 *  of the name, and limit the length of the entire string.  As noted in the
 *  printing of sequence::get_name() in mainwnd, this length is 13 characters.
 */

void
sequence::set_name (const std::string & name)
{
    if (name.empty())
        m_name = sm_default_name;
    else
        m_name = name;                                /* legacy behavior  */

    set_dirty();
}

/**
 *  Gets the title of the pattern, to show in the pattern slot.  This function
 *  differs from name, which just returns the value of m_name.  Here, we also
 *  include the length of the sequences in measures at the end of the name,
 *  and limit the length of the entire string.  As noted in the printing of
 *  sequence::get_name() in mainwnd, this length is 13 characters.
 *
 * \return
 *      Returns the name of the sequence, with the length in measures of the
 *      pattern wedged in at the end, if non-zero.
 */

std::string
sequence::title () const
{
    int measures = calculate_measures();
    bool showmeasures = true;
    if (measures > 0 && showmeasures)           /* do we have bars to show? */
    {
        char mtemp[16];                         /* holds measures as string */
        char fulltemp[32];                      /* seq name + measures      */
        std::memset(fulltemp, ' ', sizeof fulltemp);
        snprintf(mtemp, sizeof mtemp, " %d", measures);
        for (int i = 0; i < int(m_name.size()); ++i)
        {
            if (i <= (14 - 1))                  /* max size fitting in slot */
                fulltemp[i] = m_name[i];        /* add sequence name/title  */
            else
                break;
        }
        int mlen = int(strlen(mtemp));          /* no. of chars in measures */
        int offset = 14 - mlen;                 /* we're allowed 14 chars   */
        for (int i = 0; i < mlen; ++i)
            fulltemp[i + offset] = mtemp[i];

        fulltemp[14] = 0;                       /* guarantee C string term. */
        return std::string(fulltemp);
    }
    else
        return m_name;
}

/**
 *  Sets the m_midi_channel number, which is the output channel for this
 *  sequence.
 *
 * No longer true:
 *
 *  If the channel number provides equates to the null channel,
 *  this function does not change the channel number, but merely sets the
 *  m_free_channel flag.
 *
 * \threadsafe
 *
 * \param ch
 *      The MIDI channel to set as the output channel number for this
 *      sequence.  This value can range from 0 to 15, but c_midichannel_null
 *      equal to  0x80 means we are just setting the "no-channel" status.
 *
 * \param user_change
 *      If true (the default value is false), the user has decided to change
 *      this value, and we might need to modify the performer's dirty flag, so
 *      that the user gets prompted for a change,  This is a response to
 *      GitHub issue #47, where channel changes do not cause a prompt to save
 *      the sequence.
 */

bool
sequence::set_midi_channel (midibyte ch, bool user_change)
{
    automutex locker(m_mutex);
    bool result = ch != m_midi_channel;
    if (result)
        result = is_valid_channel(ch);      /* 0 to 15 or null_channel()    */

    if (result)
    {
        off_playing_notes();
        m_free_channel = is_null_channel(ch);
        m_midi_channel = ch;                /* if (! m_free_channel)        */
        if (user_change)
            modify();                       /* no easy way to undo this     */

        set_dirty();                        /* this is for display updating */
    }
    return result;
}

/**
 *  Translates the 0 to 15 channel to "1" to "16".  The "F" indicates "no
 *  channel" in the pattern slot grid.
 */

std::string
sequence::channel_string () const
{
    return m_free_channel ?
        std::string("F") : std::to_string(m_midi_channel + 1) ;
}

/**
 *  Modifies all the channel events in the pattern.  This also
 *  changes the pattern to use the "Free" channel.
 */

bool
sequence::set_channels (int channel)
{
    bool result = channel != c_midichannel_null;
    if (result)
    {
        result = m_events.set_channels(channel);
        if (result)
        {
            m_midi_channel = c_midichannel_null;
            m_free_channel = true;
        }
    }
    return result;
}

/**
 *  Constructs a list of the currently-held events.  We do it by brute force,
 *  not by std::sstream.
 *
 * \threadunsafe
 */

std::string
sequence::to_string () const
{
    midibyte channel = seq_midi_channel();
    std::string chanstring = is_null_channel(channel) ?
        "null" : std::to_string(int(channel) + 1) ;

    std::string result = "Pattern ";
    result += std::to_string(seq_number());
    result +=  " '";
    result += name();
    result += "'\n";
    result += "Channel ";
    result += chanstring;
    result += ", Bus ";
    result += std::to_string(seq_midi_bus());
    result += "\n Transposeable: ";
    result += bool_to_string(transposable());
    result += "\n Length (ticks): ";
    result += std::to_string(get_length());
    result += "Events:\n";
    result += m_events.to_string();
    return result;
}

/**
 *  Takes an event that this sequence is holding, and places it on the MIDI
 *  buss.  This function does not bother checking if m_master_bus is a null
 *  pointer.
 *
 *  Note that the call to midi_channel() yields the event channel if
 *  free_channel() is true.  Otherwise the global pattern channel is true.
 *
 * \param ev
 *      The event to put on the buss.
 *
 * \threadsafe
 */

void
sequence::put_event_on_bus (const event & ev)
{
    midibyte note = ev.get_note();
    bool skip = false;
    if (ev.is_note_on())
    {
        ++m_playing_notes[note];
    }
    else if (ev.is_note_off())
    {
        if (m_playing_notes[note] == 0)
            skip = true;
        else
            --m_playing_notes[note];
    }
    if (! skip)
    {
        event evout;
        evout.prep_for_send(perf()->get_tick(), ev);      /* issue #100   */
        master_bus()->play_and_flush(m_true_bus, &evout, midi_channel(ev));
    }
}

/**
 *  Sends a note-off event for all active notes.  This function does not
 *  bother checking if m_master_bus is a null pointer.
 *
 * \threadsafe
 */

void
sequence::off_playing_notes ()
{
    automutex locker(m_mutex);
    int channel = free_channel() ? 0 : seq_midi_channel() ;
    event e(0, EVENT_NOTE_OFF, channel, 0, 0);
    for (int x = 0; x < c_notes_count; ++x)
    {
        while (m_playing_notes[x] > 0)
        {
            e.set_data(x);
            master_bus()->play(m_true_bus, &e, channel);
            --m_playing_notes[x];
        }
    }
    if (not_nullptr(master_bus()))
        master_bus()->flush();
}

/**
 *  Transposes notes by the given steps, in accordance with the given
 *  scale.  If the scale value is 0, this is "no scale", which is the
 *  chromatic scale, where all 12 notes, including sharps and flats, are
 *  part of the scale.
 *
 *  Also, we've moved external calls to push_undo() into this function.
 *  The caller shouldn't have to do that.
 *
 * \note
 *      We noticed (ca 2016-06-10) that MIDI aftertouch events need to be
 *      transposed, but are not being transposed here.  Assuming they are
 *      selectable (another question!), the test for note-on and note-off is not
 *      sufficient, and so has been replaced by a call to event::is_note().
 *
 * \param steps
 *      The number of steps to transpose the notes.
 *
 * \param scale
 *      The scale to make the notes adhere to while transposing.  If the scale
 *      is 0, it is straight transposition.  If greater than 0, then the
 *      transposition is harmonic, and based on the "up" or "down" settings in
 *      the scales module for that scale.
 */

bool
sequence::transpose_notes (int steps, int scale, int key)
{
    automutex locker(m_mutex);
    const int * transposetable;
    bool result = false;
    m_events_undo.push(m_events);                   /* push_undo(), no lock */
    if (steps < 0)
    {
        transposetable = scales_down(scale, key);   /* 0 = chromatic scale  */
        steps *= -1;
    }
    else
        transposetable = scales_up(scale, key);     /* 0 = chromatic scale  */

    for (auto & er : m_events)
    {
        if (er.is_selected_note())                  /* transposable event?  */
        {
            int note = er.get_note();
            bool off_scale = false;
            if (transposetable[note % c_octave_size] == 0)
            {
                off_scale = true;
                note -= 1;
            }
            for (int x = 0; x < steps; ++x)
                note += transposetable[note % c_octave_size];

            if (off_scale)
                note += 1;

            er.set_note(note);
            result = true;
        }
    }
    if (result)
        modify();

    return result;
}

#if defined SEQ66_SEQ32_SHIFT_SUPPORT

/**
 *  We need to look into this function.
 */

void
sequence::shift_notes (midipulse ticks)
{
    automutex locker(m_mutex);
    if (get_length() > 0)
    {
        m_events_undo.push(m_events);               /* push_undo(), no lock */
        for (auto & er : m_events)
        {
            if (er.is_selected_note())              /* shiftable event?     */
            {
                midipulse timestamp = er.timestamp() + ticks;
                if (timestamp < 0L)                 /* wraparound           */
                    timestamp = get_length() - ((-timestamp) % get_length());
                else
                    timestamp %= get_length();

                er.set_timestamp(timestamp);
                result = true;
            }
        }
        if (result)
        {
            m_events.sort();
            set_dirty();                            /* seqedit to update    */
        }
    }
}

#endif  // SEQ66_SEQ32_SHIFT_SUPPORT

/**
 *  Applies the transpose value held by the master MIDI buss object, if
 *  non-zero, and if the sequence is set to be transposable.
 */

void
sequence::apply_song_transpose ()
{
    int transpose = transposable() ? perf()->get_transpose() : 0 ;
    if (transpose != 0)
    {
        automutex locker(m_mutex);
        m_events_undo.push(m_events);               /* push_undo(), no lock */
        for (auto & er : m_events)
        {
            if (er.is_note())                       /* also aftertouch      */
                er.transpose_note(transpose);
        }
        set_dirty();
    }
}

/**
 * \setter m_transposable
 *      Changing this flag modifies the sequence and performance.  Note that
 *      when a sequence is being read from a MIDI file, it will not yet have a
 *      parent, so we have to check for that before setting the performer modify
 *      flag.
 *
 * \param flag
 *      The value to set.
 *
 * \param user_change
 *      If true (default is false), then call the change a modification.
 *      This change can happen at load time, which is not a modification.
 */

void
sequence::set_transposable (bool flag, bool user_change)
{
    automutex locker(m_mutex);
    bool modded = flag != m_transposable && user_change;
    m_transposable = flag;
    if (modded)
        modify();
}

/**
 *  Quantizes the currently-selected set of events that match the type of
 *  event specified.  This function first marks the selected events.  Then it
 *  grabs the matching events, puts them into a list of events to be quantized
 *  and quantizes them against the snap ticks.  Linked events (which are
 *  always Note On or Note Off) are adjusted as well, with Note Offs that wrap
 *  around being adjust to be just at the end of the pattern.  This function
 *  them removes the marked event from the sequence, and merges the quantized
 *  events into the pattern's event container.  Finally, the modified event
 *  list is verified and linked.
 *
 * \param status
 *      Indicates the type of event to be quantized.
 *
 * \param cc
 *      The desired control-change to count, if the event is a control-change.
 *
 * \param snap_tick
 *      Provides the maximum amount to move the events.  Actually, events are
 *      moved to the previous or next snap_tick value depend on whether they
 *      are halfway to the next one or not.
 *
 * \param divide
 *      A rough indicator of the amount of quantization.  The only values used
 *      in the application are either 1 ("quantize") or 2 ("tighten").  The
 *      latter value reduces the amount of change slightly.  This value is
 *      tested for 0, and the function just returns if that is the case.
 */

bool
sequence::quantize_events (midibyte status, midibyte cc, int divide)
{
    automutex locker(m_mutex);
    if (divide == 0)
        return false;

    bool result = m_events.quantize_events(status, cc, snap(), divide);
    if (result)
        set_dirty();

    return result;
}

bool
sequence::quantize_notes (int divide)
{
    automutex locker(m_mutex);
    if (divide == 0)
        return false;

    bool result = m_events.quantize_notes(snap(), divide);
    if (result)
        set_dirty();

    return result;
}

/**
 *  We set the beats and width to 0 to use the current values.
 */

bool
sequence::change_ppqn (int p)
{
    automutex locker(m_mutex);
    bool result = p != m_ppqn;
    if (result)
        result = ppqn_in_range(p);

    if (result)
    {
        result = m_events.rescale(p, m_ppqn);           /* new & old PPQNs  */
        if (result)
        {
            m_length = rescale_tick(m_length, p, m_ppqn);
            m_ppqn = p;
            result = apply_length(0, 0, 0);             /* use new PPQN     */
            m_triggers.change_ppqn(p);
        }
    }
    return result;
}

/**
 *  A new convenience function.  See the sequence::quantize_events() function
 *  for more information.  This function just does locking and a push-undo
 *  before calling that function.
 *
 *  Note that only selected events are subject to quantization.
 *
 * \param status
 *      The kind of event to quantize, such as Note On, or the event type
 *      selected in the pattern editor's data pane.
 *
 * \param cc
 *      The control-change value to quantize, again as selected in the pattern
 *      editor's data pane.  For Note Ons, this value should be set to 0.
 *
 * \param divide
 *      Provides a division value, usually either 1 ("quantize") or 2
 *      ("tighten").
 *
 * \return
 *      Returns true if the events were quantized.
 */

bool
sequence::push_quantize (midibyte status, midibyte cc, int divide)
{
    automutex locker(m_mutex);
    m_events_undo.push(m_events);
    return quantize_events(status, cc, divide);
}

bool
sequence::push_quantize_notes (int divide)
{
    automutex locker(m_mutex);
    m_events_undo.push(m_events);
    return quantize_notes(divide);
}

bool
sequence::push_jitter_notes (int range)
{
    automutex locker(m_mutex);
    m_events_undo.push(m_events);
    if (range == (-1))
        range = usr().jitter_range(snap());

    return jitter_notes(range);
}

/**
 *  A member function to dump a summary of events stored in the event-list of
 *  a sequence.  Later, use to_string().
 */

void
sequence::show_events () const
{
    printf
    (
        "sequence #%d '%s': channel %d, events %d\n",
        seq_number(), name().c_str(), seq_midi_channel(), event_count()
    );
    for (auto iter = cbegin(); ! cend(iter); ++iter)
    {
        const event & er = eventlist::cdref(iter);
        std::string evdump = er.to_string();
        printf("%s", evdump.c_str());
    }
}

/**
 *  Copies an external container of events into the current container,
 *  effectively replacing all of its events.  Compare this function to the
 *  remove_all() function.  Copying the container is a lot of work, but fast.
 *  Also note that we have to recalculate the length of the sequence.
 *  Another option, if we have a new sequence length value (in pulses)
 *  would be to call sequence::set_length(len, adjust_triggers).  We
 *  need to make sure the length is rounded up to the next quarter note.
 *  Actually, should make it a full measure size!  Or do we always want to
 *  preserve the pattern length no matter how many trailing events are deleted?
 *
 * \threadsafe
 *      Note that we had to consolidate the replacement of all the events in
 *      the container in order to prevent the "Save to Sequence" button in the
 *      eventedit object from causing the application to segfault.  It would
 *      segfault when the mainwnd timer callback would fire, causing updates
 *      to the sequence's slot pixmap, which would then try to access deleted
 *      events.  Part of the issue was that note links were dropped when
 *      copying the events, so now we call verify_and_link() to hopefully
 *      reconstitute the links.
 *
 * \param newevents
 *      Provides the container of MIDI events that will completely replace the
 *      current container.  Normally this container is supplied by the event
 *      editor, via the eventslots class.
 */

bool
sequence::copy_events (const eventlist & newevents)
{
    automutex locker(m_mutex);
    bool result = false;
    m_events.clear();
    m_events = newevents;
    if (m_events.empty())
    {
        m_events.unmodify();

        /*
         * ca 2021-02-03 Not sure we want to change the length at all, let
         * alone set it to 0.  No pattern ever has a length of 0.
         *
         * m_length = 0;
         */
    }
    else
    {
        midipulse len = m_events.get_max_timestamp();
        bool change_length = false;
        if (len < get_ppqn())
        {
            double qn_per_beat = 4.0 / get_beat_width();
            int qnnum = int(get_beats_per_bar() * qn_per_beat);
            len = qnnum * get_ppqn();
            change_length = true;
        }
        else if (len < m_length)                /* trimmed trailing note(s) */
        {
            // leave the length as it was
        }
        else if (len > m_length)
        {
            change_length = true;               /* calc. measures & length  */
        }
        if (change_length)
            set_length(len);                    /* m_length = len           */

        (void) verify_and_link();               /* function uses m_length   */
        result = true;
    }
    modify();
    return result;
}

/**
 *  Sets the "parent" of this sequence, so that it can get some extra
 *  information about the performance.  Remember that m_parent is not at all
 *  owned by the sequence.  We just don't want to do all the work necessary to
 *  make it a reference, at this time.
 *
 *  Add the buss override, if specified.  We can't set it until after
 *  assigning the master MIDI buss, otherwise we get a segfault.
 *
 * Issue #88:
 *
 *      If the time signature is something like 4/16, then the beat length
 *      will be much less than PPQN.  It will be the quarter note size / 4.
 *      Also check out pulses_to_midi_measures().
 *
 * \param p
 *      A pointer to the parent, assigned only if not already assigned.
 */

void
sequence::set_parent (performer * p)
{
    if (not_nullptr(p))
    {
        int bpb = get_beats_per_bar();
        int bw = get_beat_width();
        if (bpb == 0)
            bpb = p->get_beats_per_bar();

        if (bw == 0)
            bw = p->get_beat_width();

        midipulse ppnote = 4 * get_ppqn() / bw; /* get_beat_width();        */
        midipulse barlength = ppnote * bpb;     /* get_beats_per_bar();     */
        bussbyte buss_override = usr().midi_buss_override();
        m_parent = p;                           /* perf() is the accessor   */
        set_master_midi_bus(p->master_bus());
        sort_events();                      /* sort the events now          */
        set_length();                       /* final verify_and_link()      */
        empty_coloring();                   /* yellow color if no events    */
        if (get_length() < barlength)       /* pad sequence to a measure    */
            set_length(barlength, false);

        (void) set_midi_in_bus(m_nominal_in_bus);
        if (is_null_buss(buss_override))
            (void) set_midi_bus(m_nominal_bus);
        else
            (void) set_midi_bus(buss_override);

        set_beats_per_bar(bpb);
        set_beat_width(bw);
        unmodify();                         /* for issue #90                */
    }
}

/**
 *  Why don't we see this in kepler34?  We do, in the MidiPerformance::play()
 *  function.  We refactored this, Chris.  Remember?  :-D
 *
 * \param tick
 *      Provides the current active pulse position, the tick/pulse from which
 *      to start playing.
 *
 * \param playbackmode
 *      If true, we are in Song mode.  Otherwise, Live mode.
 *
 * \param resumenoteons
 *      Indicates if we are to resume Note Ons.  Used by performer::play().
 */

void
sequence::play_queue (midipulse tick, bool playbackmode, bool resumenoteons)
{
    if (check_queued_tick(tick))
    {
        play(get_queued_tick() - 1, playbackmode, resumenoteons);
        (void) toggle_playing(tick, resumenoteons);
        if (! perf()->is_solo())
        {
            automation::action a = automation::action::off;
            automation::ctrlstatus cs = automation::ctrlstatus::queue;
            (void) perf()->set_ctrl_status(a, cs);
        }
    }
    if (check_one_shot_tick(tick))
    {
        play(one_shot_tick() - 1, playbackmode, resumenoteons);
        (void) toggle_playing(tick, resumenoteons);
        (void) toggle_queued(); /* queue it to mute it again after one play */
        (void) perf()->set_ctrl_status
        (
            automation::action::off, automation::ctrlstatus::oneshot
        );
    }
    if (is_metro_seq())
    {
        live_play(tick);
    }
    else
    {
        play(tick, playbackmode, resumenoteons);
    }
}

/**
 *  Actually, useful mainly for the user-interface, this function calculates
 *  the size of the left and right handles of a note.
 *
 * \param start
 *      The starting tick of the note event.
 *
 * \param finish
 *      The ending tick of the note event.
 *
 * \return
 *      Returns 16 or one-third of the note length.  This value is scaled
 *      according to PPQN if different from 192.
 */

midipulse
sequence::handle_size (midipulse start, midipulse finish)
{
    midipulse result = c_handlesize * m_ppqn / usr().base_ppqn();
    midipulse notelength = finish - start;
    if (notelength < result / 3)
        result = notelength / 3;

    return result;
}

/**
 *  Toggles the m_one_shot flag, sets m_off_from_snap to true, and adjusts
 *  m_one_shot_tick according to m_last_tick and m_length.
 */

bool
sequence::toggle_one_shot ()
{
    automutex locker(m_mutex);
    set_dirty_mp();
    m_one_shot = ! m_one_shot;
    m_one_shot_tick = m_last_tick - mod_last_tick() + get_length();
    perf()->announce_pattern(seq_number());     /* for issue #89        */
    off_from_snap(true);
    return m_one_shot;
}

/**
 *  Sets the dirty flag, sets m_one_shot to false, and m_off_from_snap to
 *  true. This function remains unused here and in Kepler34. Instead, see
 *  the set_armed() function above.
 */

void
sequence::off_one_shot ()
{
    automutex locker(m_mutex);
    set_dirty_mp();
    m_one_shot = false;
    off_from_snap(true);
    perf()->announce_pattern(seq_number());     /* for issue #89        */
}

/**
 *  Starts the growing of the sequence for Song recording.  This process
 *  starts by adding a chunk of c_song_record_incr ticks to the
 *  trigger, which allows the rest of the threads to notice the change.
 *
 * \question
 *      Do we need to call set_dirty_mp() here?
 *
 * \param tick
 *      Provides the current tick, which helps set the recorded block's
 *      boundaries, and is copied into m_song_record_tick.
 *
 * \param snap
 *      If true, trigger recording will snap.  Defaults to the preferred
 *      state, true. Note that the performer actually does the snapping on
 *      behalf of all patterns.
 */

void
sequence::song_recording_start (midipulse tick, bool snap)
{
    song_recording(true);
    song_recording_snap(snap);
    if (snap)
        (void) perf()->calculate_snap(tick);      /* issue #44 redux  */

    song_record_tick(tick);                         /* snapped or not   */
    add_trigger(tick, c_song_record_incr);
}

/**
 *  Stops the growing of the sequence for Song recording.  If we have been
 *  recording, we snap the end of the trigger segment to the next whole
 *  sequence interval.
 *
 *  However, for issue #44, we'd like to have the trigger stop at the snap
 *  point for the actual tick at which muting turns on.
 *
 *  In Kepler34, these were the grow_trigger() call parameters:
 *
 *      -   Song-recording start tick.
 *      -   Current tick, "tick".
 *      -   Length:  len = seq_length() - (tick % seq_length()).
 *
 *  That length snaps the end of the trigger to the next whole sequence
 *  interval.
 *
 * \question
 *      Do we need to call set_dirty_mp() here?
 *
 * \param tick
 *      Provides the current tick, which helps set the recorded block's
 *      boundaries. It is the tick at which the current trigger ends.
 */

void
sequence::song_recording_stop (midipulse tick)
{
    (void) perf()->calculate_snap(tick);      /* issue #44 redux  */
    grow_trigger(song_record_tick(), tick, 1);
    if (song_recording_snap())
        off_from_snap(true);

    m_song_playback_block = m_song_recording = false;
}

/**
 *  If we're playing a recorded pattern (one that has Song-mode triggers), and
 *  the user pauses playing in the middle of some notes, upon restart we want
 *  to replay the Note Ons that are before the current tick and before the
 *  Note On's linked Note Off.
 *
\verbatim
         t                     tick      t+length
         ----------------------------------------------------------------
         |                       :       |                               |
       A |        on XXXXXXXXXXXXXXX off |        on XXXXXXXXXXXXXXX off |
         |                       :       |                               |
       B |        on XXXXX off   :       |        on XXXXX off           |
         |                       :       |                               |
       C | off XXXXXXXXXXX on    :       | off XXXXXXXXXXX on            |
         |                       :       |                               |
       D | XXXXXXX off      on XXXXXXXXXX| XXXXXXX off      on XXXXXXXXXX|
         |                       :       |                               |
       E | XXXXXXX off           : on XXX| XXXXXXX off             on XXX|
         |                       :       |                               |
         ----------------------------------------------------------------
                   Wn                               Wn+1
\endverbatim
 *
 *  where Wn is the current window of the pattern length (demarcated by times
 *  t to t+length-1), and tick is the tick at which the user paused playback.
 *  Let T = tick % length.  Assuming all notes are linked properly, we have
 *  the following cases:
 *
 *      A: on <  T, off > T, so emit the Note On again.
 *      B: on < T, off < T, no note events to emit.
 *      C: same disposition as B.
 *      D: on < T and > off, so emit the Note On again.
 *      E: on > tick, emit the Note On normally.
 *
 *  Currently, cases D and E are not handled because we have disabled handling
 *  note wrap-around for now.  See eventlist::link_new() and the
 *  SEQ66_USE_STAZED_LINK_NEW_EXTENSION macro.
 *
 * We think this explanation in the Kepler34 code is incorrect:
 *
 *      "If the Note-On event is after the Note-Off event, the pattern wraps
 *      around, so that we play it now to resume."
 *
 *  One question is where is best to do the locking of put_event_on_bus().  In
 *  retrospect, probably better to do it just once, instead of for each event.
 *
 * \param tick
 *      The current tick-time, in MIDI pulses.
 */

void
sequence::resume_note_ons (midipulse tick)
{
    automutex locker(m_mutex);                          /* better here?     */
    if (get_length() > 0)
    {
        for (auto & ei : m_events)
        {
            if (ei.is_note_on_linked())                 /* note on linked   */
            {
                midipulse on = ei.timestamp();          /* see banner notes */
                midipulse off = ei.link()->timestamp();
                midipulse rem = tick % get_length();
                if (on < rem && (off > rem || on > off))
                    put_event_on_bus(ei);
            }
        }
    }
}

/**
 *  Makes a calculation for expanded recording, used in seqedit and qseqroll.
 *  This is the way Seq32 does it now, and it seems to work for Seq66.
 *
 * \return
 *      Returns true if we are recording, expanded-record is enabled,
 *      and we're far enough along in the current length to move to the next
 *      "time window".
 */

bool
sequence::expand_recording () const
{
    bool result = false;
    if (expanding())
    {
        midipulse tstamp = m_last_tick;
        if (tstamp >= expand_threshold())
        {
#if defined SEQ66_PLATFORM_DEBUG_TMI
            printf
            (
                "tick %ld >= length %ld - measure %ld / 4\n",
                tstamp, get_length(), unit_measure()
            );
#endif
            result = true;
        }
    }
    return result;
}

/*
 *  Static member function.
 */

recordstyle
sequence::loop_record_style (int ri)
{
    recordstyle result = recordstyle::merge;
    int min = usr().pattern_record_code(result);
    int max = usr().pattern_record_code(recordstyle::max);
    if (ri > min && ri < max)
        result = static_cast<recordstyle>(ri);

    return result;
}

/**
 *  Code to help user-interface callers. Used in qseqeditframe64.
 */

bool
sequence::update_recording (int index)
{
    recordstyle rectype = loop_record_style(index);
    bool result = rectype != recordstyle::max;
    if (result)
    {
        switch (rectype)
        {
        case recordstyle::merge:
        case recordstyle::overwrite:
        case recordstyle::expand:
        case recordstyle::oneshot:
            break;

        case recordstyle::oneshot_reset:

            /*
             *  This might be surprising. Use the new grid slot
             *  menu popup entry "Clear events" to do this.
             *
             * clear_events();
             */

            m_last_tick = 0;
            set_recording(toggler::on);
            break;

        default:

            result = false;
            break;
        }
        if (result)
            result = set_recording_style(rectype);
    }
    return result;
}

/**
 *  Implements the actions brought forth from the Tools (hammer) button.
 *
 *  Note that the push_undo() calls push all of the current events (in
 *  sequence::m_events) onto the stack (as a single entry).
 *
 * \todo
 *      Move this code into eventlist (without the redraw call).
 */

void
sequence::handle_edit_action (eventlist::edit action, int var)
{
    switch (action)
    {
    case eventlist::edit::select_all_notes:

        select_all_notes();
        break;

    case eventlist::edit::select_inverse_notes:

        select_all_notes(true);
        break;

    case eventlist::edit::select_all_events:

        select_events(m_status, m_cc);
        break;

    case eventlist::edit::select_inverse_events:

        select_events(m_status, m_cc, true);
        break;

    case eventlist::edit::randomize_events:

        (void) randomize(m_status, var);
        break;

    case eventlist::edit::quantize_notes:

        /*
         * sequence::quantize_events() is used in recording as well, so we do
         * not want to incorporate sequence::push_undo() into it.  So we make
         * a new function to do that.
         */

        push_quantize(EVENT_NOTE_ON, 0, 1);
        break;

    case eventlist::edit::quantize_events:

        push_quantize(m_status, m_cc, 1);
        break;

    case eventlist::edit::tighten_notes:

        push_quantize(EVENT_NOTE_ON, 0, 2);
        break;

    case eventlist::edit::tighten_events:

        push_quantize(m_status, m_cc, 2);
        break;

    case eventlist::edit::transpose_notes:      /* regular transpose    */

        transpose_notes(var, 0);
        set_dirty();                            /* updates perfedit     */
        break;

    case eventlist::edit::transpose_harmonic:   /* harmonic transpose   */

        transpose_notes(var, musical_scale());
        set_dirty();                            /* updates perfedit     */
        break;

    default:
        break;
    }
}

}           // namespace seq66

/*
 * sequence.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


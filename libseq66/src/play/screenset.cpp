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
 * \file          screenset.cpp
 *
 *  This module declares a two dimensional vector class solely to hold the
 *  mute status of a number of sequences in a set.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-02-12
 * \updates       2019-09-07
 * \license       GNU GPLv2 or above
 *
 *  Implements the screenset class.  The screenset class represent all of the
 *  sequences in a screen-set.  A screen has a size given by the concept of
 *  rows and columns, mirroring what the user sees in the Sequencer66 user
 *  interface.
 */

#include <algorithm>                    /* std::find_if()                   */
#include <iomanip>                      /* std::setw manipulator            */
#include <iostream>                     /* std::cout                        */
#include <sstream>                      /* std::stringstream                */

#include "cfg/rcsettings.hpp"           /* c_max_sequence, etc.             */
#include "play/screenset.hpp"           /* seq66::screenset class           */

/*
 *  This namespace is not documented because it screws up the document
 *  processing done by Doxygen.
 */

namespace seq66
{

/**
 *  Principal constructor with optional parameters.
 *
 * \param setnum
 *      Provides the set number for this screenset.  This number ranges from 0
 *      to whatever the maximum set size is (normally this is 32 - 1.)
 *
 * \param rows
 *      Provides the number of virtual rows in the set, which must match what
 *      the user-interface uses and the number of "rows" in the mutegroups.
 *
 * \param columns
 *      Provides the number of virtual columns in the set, which must match
 *      what the user-interface uses and the number of "columns" in the
 *      mutegroups.
 */

screenset::screenset (screenset::number setnum, int rows, int columns) :
    m_rows              (rows),
    m_columns           (columns),
    m_set_size          (rows * columns),
    m_container         (),
    m_set_number        (setnum),
    m_set_offset        (m_set_number * m_set_size),
    m_set_maximum       (m_set_offset + m_set_size),
    m_set_name          (),
    m_is_playscreen     (false)
{
    clear();
    m_set_name = usable() ? "New Set" : "Dummy Set" ;
}

/**
 *  If we move (swap) sets in the qsetmaster set-list, we have a lot of work to
 *  do.  The screensets are first moved, so that they have new set numbers (and
 *  a new set-offset and set-maximum number).  Then, we have to renumber all
 *  existing sequences in both sets to reflect the new set number.
 */

void
screenset::change_set_number (screenset::number setno)
{
    int seqno = m_set_number * m_set_size;
    m_set_number  = setno;
    m_set_offset  = seqno;
    m_set_maximum = m_set_offset + m_set_size;
    for (auto & s : m_container)            /* renumber all the sequences   */
        s.change_seq_number(seqno++);       /* works only if seq is active  */
}

/**
 *
 */

void
screenset::clear ()
{
    seq emptyseq;
    m_container.clear();
    for (int s = 0; s < m_set_size; ++s)
        m_container.push_back(emptyseq);
}

/**
 *
 */

void
screenset::initialize (int rows, int columns)
{
    m_rows              = rows;
    m_columns           = columns;
    m_set_size          = rows * columns;
    m_set_maximum       = m_set_size;
    m_is_playscreen     = false;
    clear();
}

/**
 *  Adds a sequence (and its seq maintenance object) to the container.  Recall
 *  that the container is a vector of constant size greater than 0.
 *
 * \param s
 *      Provides the sequencer pointer, assumed to be valid.
 *
 * \param seqno
 *      The desired sequence number, which is a linear value ranging from 0 to
 *      a number determined by the active set size multiplied by the number of
 *      sets.
 *
 * \return
 *      Returns true if the sequence number didn't already exist and the
 *      sequence pointer was able to be added and activated.
 */

bool
screenset::add (sequence * s, seq::number seqno)
{
#ifdef USE_ORIGINAL_CODE
    seq & sseq = seqinfo(seqno);
    bool result = ! sseq.active();              /* seq already in place?    */
    if (result)                                 /* no, we can add it        */
    {
        result = not_nullptr(s);
        if (result)
        {
            seq newseq(s);                      /* stored in shared pointer */
            result = newseq.activate(seqno);    /* activate sequence        */
            if (result)
            {
                seq::number index = seqno - m_set_offset;
                m_container[index] = newseq;
            }
        }
    }
#else
    bool result = false;
    if (not_nullptr(s))
    {
        for (seq::number i = seqno - m_set_offset; i < m_set_maximum; ++i)
        {
            seq sseq = seqinfo(i);
            if (! sseq.active())            /* seq already in slot?     */
            {
//              seq newseq(s);
//              sseq.loop(s);
                result = sseq.activate(s, seqno);
//              result = newseq.activate(seqno);
                if (result)
                {
                    m_container[i] = sseq;
//                  m_container[i] = newseq;
                    break;
                }
            }
        }
    }
#endif
    return result;
}

/**
 *  Removes the given sequence, by number.  The seq object is replaced with a
 *  default/inactive one. This action will not be done if the sequence
 *  currently is being edited.
 *
 * \param seqno
 *      Provides the number of the sequence to be removed.
 *
 * \return
 *      Returns true if the sequence existed, was not being edited,
 *      and was erased.
 */

bool
screenset::remove (seq::number seqno)
{
    bool result = false;
    seq::pointer sp = seqinfo(seqno).loop();
    if (sp && ! sp->get_editing())
    {
        seq newseq;                         /* non-functiona object         */
        sp->set_playing(false);             /* turns off all notes as well  */
        m_container[seqno - m_set_offset] = newseq;
        result = true;
    }
    return result;
}

/**
 *  Tests to see if the screenset is active.  By "active", we mean that the
 *  screen-set has at least one active pattern.
 *
 * \return
 *      Returns true if the screenset has an active pattern.
 */

bool
screenset::active () const
{
    for (auto & s : m_container)
    {
        if (s.active())
            return true;
    }
    return false;
}

/**
 *  Counts the number of active patterns in this screenset.
 */

int
screenset::active_count () const
{
    int result = 0;
    for (auto & s : m_container)
    {
        if (s.active())
            ++result;
    }
    return result;
}

/**
 *  Obtains the number of the first active sequence found in the screenset.
 *
 * \return
 *      Returns the first sequence number, or a bad value if none exists.
 */

seq::number
screenset::first_seq () const
{
    seq::number result = seq::none();
    for (auto & s : m_container)
    {
        if (s.active())
        {
            result = s.seq_number();
            break;
        }
    }
    return result;
}

/**
 *
 */

seq::number
screenset::calculate_seq (int row, int column) const
{
    if (row < 0)
        row = 0;
    else if (row >= m_rows)
        row = m_rows - 1;

    if (column < 0)
        column = 0;
    else if (column >= m_columns)
        row = m_columns - 1;

    return m_set_offset + row + m_rows * column;
}

/**
 *  Converts a sequence number to a (row, column) pair according to the layout
 *  shown below.
 *
/verbatim
    0   4   8   12  16  20  24  28
    1   5   9   13  17  21  25  29
    2   6  10   14  18  22  26  30
    3   7  11   15  19  23  27  31
/endverbatim
 *
 *  Note that this layout varies the row number faster than the column.  The
 *  calculation for this layout is
 *
/verbatim
    (row = s % rows, column = [s / rows] % columns)
/endverbatim
 *
 *  where s is offset from 0, not from the set number.  As an aside, to get
 *  the more conventional layout, where the column number varies faster, use:
 *
/verbatim
    (row = s / rows, column = s % columns)
/endverbatim
 *
 */

bool
screenset::seq_to_grid (seq::number seqno, int & row, int & column) const
{
    bool result = seq_in_set(seqno);
    if (result)
    {
        seqno -= m_set_offset;          /* convert to 0-to-31 range */
        row = seqno % m_rows;
        column = (seqno / m_rows);      //  % m_columns;    // TEST
    }
    return result;
}

/**
 *
 */

bool
screenset::is_seq_in_edit (seq::number seqno) const
{
    seq::pointer track = seqinfo(seqno).loop();
    return track ? track->get_editing() : false ;
}

/**
 *
 */

bool
screenset::any_in_edit () const
{
    for (auto & s : m_container)
    {
        if (s.active() && s.loop()->get_editing())
            return true;
    }
    return false;
}

/**
 *  Indicates if there is at least one armed sequence in this screen-set.
 */

bool
screenset::armed () const
{
    for (auto & s : m_container)
    {
        if (s.active() && s.loop()->get_playing())
            return true;
    }
    return false;
}

/**
 *
 */

bool
screenset::needs_update () const
{
    for (auto & s : m_container)
    {
        if (s.active() && s.loop()->is_dirty_main())
            return true;
    }
    return false;
}

/**
 *  Converts a sequence number from its raw form (o to 1023) to its range
 *  within a set (e.g. 0 to 31).  If the value is still outside this
 *  range, it is clamped to that range.
 */

seq::number
screenset::clamp (seq::number seqno) const
{
    if (seqno >= m_set_offset)
        seqno -= m_set_offset;

    if (seqno < 0)
        return 0;
    else if (seqno >= m_set_size)
        return m_set_size - 1;

    return seqno;
}

/**
 *  Clears the screenset states.  Needed when disabling the queue mode.
 */

void
screenset::clear_queued ()
{
    for (auto & s : m_container)
        s.queued(false);
}

void
screenset::off_sequences ()
{
    for (auto & s : m_container)
    {
        if (s.active())
            s.loop()->set_playing(false);
    }
}

void
screenset::song_recording_stop (midipulse current_tick)
{
    for (auto & s : m_container)
    {
        if (s.active())
            s.loop()->song_recording_stop(current_tick);
    }
}

void
screenset::clear_snapshot ()
{
    for (auto & s : m_container)
        s.clear_snapshot();
}

void
screenset::save_snapshot ()
{
    for (auto & s : m_container)
        s.save_snapshot();
}

void
screenset::restore_snapshot ()
{
    for (auto & s : m_container)
        s.restore_snapshot();
}

void
screenset::set_last_ticks (midipulse tick)
{
    for (auto & s : m_container)
    {
        if (s.active())
            s.loop()->set_last_tick(tick);
    }
}

void
screenset::armed_status (seq::number seqno, bool flag)
{
    seq & s = seqinfo(seqno);
    if (s.active())
        s.armed_status(flag);
}

/*
 * Replaces set_playing().  Should we also call set_song_mute()???
 */

void
screenset::armed (seq::number seqno, bool flag)
{
    const seq::pointer track = seqinfo(seqno).loop();
    if (track)
        track->set_playing(flag);
}

/**
 *  Runs a slot-handler function for all of the slots in this set.  Recall that
 *  a slot is a place to "store" a sequence, and a slot can be empty.  All slots
 *  can be drawn, but empty slots are drawn differently.
 */

bool
screenset::slots_function (slothandler p)
{
    bool result = false;
    seq::number sn = m_set_offset;
    for (auto & s : m_container)
    {
        result = p(s.loop(), sn++);         /* note post-increment of sn    */
        if (! result)
            break;
    }
    return result;
}

/**
 *  Run a function on each set, and another function on each sequence in each
 *  set.
 */

bool
screenset::sets_function (sethandler s, slothandler p)
{
    bool result = s(*this, 0);          /* handle the set, index not used   */
    if (result)
        result = slots_function(p);     /* handle slots/sequences in set    */

    return result;
}

/**
 *  Sets one sequence (or all of them) to be dirty.
 *
 * \param seqno
 *      Either a track number or SEQ66_ALL_TRACKS (the default value).
 */

void
screenset::set_dirty (seq::number seqno)
{
    if (sequence::all(seqno))
    {
        for (auto & s : m_container)
        {
            if (s.active())             /* guarantees a valid pointer */
                s.loop()->set_dirty();
        }
    }
    else
    {
        seq::pointer sp = find_by_number(seqno);
        if (sp)
            sp->set_dirty();
    }
}

/*
 * -------------------------------------------------------------------------
 * Triggers
 * -------------------------------------------------------------------------
 */

/**
 *  Finds the maximum trigger in the screenset.
 */

midipulse
screenset::max_trigger () const
{
    midipulse result = 0;
    for (const auto & s : m_container)
    {
        if (s.active())
        {
            midipulse t = s.loop()->get_max_trigger();
            if (t > result)
                result = t;
        }
    }
    return result;
}

/**
 *
 * \param seqno
 *      Either a track number or SEQ66_ALL_TRACKS (the default value).
 */

void
screenset::move_triggers
(
    midipulse lefttick, midipulse distance,
    bool direction, seq::number seqno
)
{
    if (sequence::all(seqno))
    {
        for (auto & s : m_container)
        {
            if (s.active())             /* guarantees a valid pointer */
                s.loop()->move_triggers(lefttick, distance, direction);
        }
    }
    else
    {
        seq::pointer sp = find_by_number(seqno);
        if (sp)
            sp->move_triggers(lefttick, distance, direction);
    }
}

/**
 *
 */

void
screenset::push_trigger_undo ()
{
    for (auto & s : m_container)
    {
        if (s.active())
            s.loop()->push_trigger_undo();
    }
}

/**
 *
 */

void
screenset::pop_trigger_undo ()
{
    for (auto & s : m_container)
    {
        if (s.active())
            s.loop()->pop_trigger_undo();
    }
}

/**
 *
 */

void
screenset::pop_trigger_redo ()
{
    for (auto & s : m_container)
    {
        if (s.active())
            s.loop()->pop_trigger_redo();
    }
}

/*
 * -------------------------------------------------------------------------
 * More stuff
 * -------------------------------------------------------------------------
 */

/**
 *
 */

bool
screenset::color (seq::number seqno, int c)
{
    bool result = false;
    seq::pointer track = seqinfo(seqno).loop();
    if (track)
        result = track->color(c);

    return result;
}

/**
 *
 */

void
screenset::set_seq_name (seq::number seqno, const std::string & name)
{
    seq::pointer track = seqinfo(seqno).loop();
    if (track)
        track->set_name(name);
}

/**
 *
 */

bool
screenset::name (const std::string & nm)
{
    bool result = nm != m_set_name;
    m_set_name = nm;
    return result;
}

/**
 *
 */

void
screenset::arm (seq::number seqno)
{
    const seq::pointer track = seqinfo(seqno).loop();
    if (track)
    {
        track->set_playing(true);
        track->set_song_mute(false);
    }
}

/**
 *
 */

void
screenset::mute (seq::number seqno)
{
    const seq::pointer track = seqinfo(seqno).loop();
    if (track)
    {
        track->set_playing(false);
        track->set_song_mute(true);
    }
}

/**
 *
 */

void
screenset::all_notes_off ()
{
    for (auto & s : m_container)
    {
        if (s.active())
            s.loop()->off_playing_notes();
    }
}

/**
 *  Looks up a seq::pointer by sequence number.  The alternative is to clamp
 *  the sequence number and do a bogus lookup.  We don't want to deal with
 *  try/catch blocks.
 *
 * \param seqno
 *      Provides the desired sequence number.
 *
 * \return
 *      Returns the sequence pointer, which should be checked using operator
 *      bool() by the caller.
 */

seq::pointer
screenset::find_by_number (seq::number seqno)
{
    static seq::pointer s_dummy;
    auto seqit = std::find_if
    (
        m_container.begin(), m_container.end(),
        [ & seqno ] (const seq & sn) { return sn.seq_number() == seqno; }
    );
    return seqit != m_container.end() ? seqit->loop() : s_dummy ;
}

/**
 *  Fills the performer's play-set.
 */

void
screenset::fill_play_set (playset & p)
{
    p.clear();
    for (auto & s : m_container)
    {
        if (s.active())
            p.push_back(s.loop());
    }
}

/**
 *
 * \param seqno
 *      Either a track number or SEQ66_ALL_TRACKS (the default value).
 */

void
screenset::copy_triggers
(
    midipulse lefttick, midipulse distance, seq::number seqno
)
{
    if (sequence::all(seqno))
    {
        for (auto & s : m_container)
        {
            if (s.active())             /* guarantees a valid pointer */
                s.loop()->copy_triggers(lefttick, distance);
        }
    }
    else
    {
        seq::pointer sp = find_by_number(seqno);
        if (sp)
            sp->copy_triggers(lefttick, distance);
    }
}

/**
 *
 * \param seqlow
 *      Provides the low track to be selected.
 *
 * \param seqhigh
 *      Provides the high track to be selected.  If not in the same set, nothing
 *      is done.  We need a way to report that.
 *
 * \param tick_start
 *      Provides the low end of the box.
 *
 * \param tick_finish
 *      Provides the high end of the box.
 */

void
screenset::select_triggers_in_range
(
    seq::number seqlow, seq::number seqhigh,
    midipulse tick_start, midipulse tick_finish
)
{
    /*
    /////////////// TODO TODO TODO

    for (int s = seqlow; s <= seqhigh; ++s)
    {
        auto seqit = m_container.find(s);
        if (seqit != m_container.end())
            seqit->loop()->unselect_triggers();

        if (goodddd)
        {
            for (long tick = tick_start; tick <= tick_finish; ++tick)
                sequence->select_trigger(tick);
        }
    }
     */
}

/**
 *
 * \param seqno
 *      Either a track number or SEQ66_ALL_TRACKS (the default value).
 */

void
screenset::unselect_triggers (seq::number seqno)
{
    if (sequence::all(seqno))
    {
        for (auto & s : m_container)
        {
            if (s.active())                 /* guarantees a valid pointer   */
                s.loop()->unselect_triggers();
        }
    }
    else
    {
        seq::pointer sp = find_by_number(seqno);
        if (sp)
            sp->unselect_triggers();
    }
}

/**
 *  For all active patterns/sequences, get its playing state, turn off the
 *  playing notes, set playing to false, zero the markers, and, if not in
 *  playback mode, restore the playing state.  Note that these calls are
 *  folded into one member function of the sequence class.
 *
 *  Note that std::shared_ptr<> does not provide operator->*, therefore we
 *  have to get the raw pointer and use that to access a sequence member
 *  function.
 */

void
screenset::reset_sequences (bool pause, sequence::playback mode)
{
    void (sequence::* f) (bool) = pause ? &sequence::pause : &sequence::stop ;
    bool songmode = mode == sequence::playback::song;
    for (auto & s : m_container)
    {
        if (s.active())                     /* guarantees a valid pointer */
        {
            sequence * sp = s.loop().get();
            (sp->*f)(songmode);
        }
    }
}

/**
 *
 */

void
screenset::arm ()
{
    for (auto & s : m_container)
    {
        if (s.active())                     /* guarantees a valid pointer */
        {
            seq::pointer sp = s.loop();
            sp->set_playing(true);          /* NEED ONLY ONE FUNCTION???  */
            sp->set_song_mute(false);
        }
    }
}

/**
 *
 */

void
screenset::mute ()
{
    for (auto & s : m_container)            // if (sequence::all(seqno))
    {
        if (s.active())                     /* guarantees a valid pointer */
        {
            seq::pointer sp = s.loop();
            sp->set_playing(false);
            sp->set_song_mute(true);
        }
    }
}

/**
 *
 */

void
screenset::toggle (seq::number seqno)
{
    if (sequence::all(seqno))
    {
        for (auto & s : m_container)
        {
            if (s.active())                     /* guarantees valid pointer */
            {
                seq::pointer sp = s.loop();
                bool playing = sp->get_playing();
                sp->set_playing(! playing);     /* or toggle_playing()      */
                sp->set_song_mute(playing);
            }
        }
    }
    else
    {
        const seq::pointer track = seqinfo(seqno).loop();
        if (track)
        {
            bool playing = track->get_playing();
            track->set_playing(! playing);
            track->set_song_mute(playing);
        }
    }
}

/**
 *  This looks like a gprof hot-spot!
 */

void
screenset::play (midipulse tick, sequence::playback mode, bool resumenoteons)
{
    bool songmode = mode == sequence::playback::song;
    for (auto & s : m_container)
    {
        if (s.active())
            s.loop()->play_queue(tick, songmode, resumenoteons);
    }
}

/**
 *
 */

void
screenset::toggle_song_mute (seq::number seqno)
{
    if (sequence::all(seqno))
    {
        for (auto & s : m_container)
        {
            if (s.active())                 /* guarantees a valid pointer   */
                s.loop()->toggle_song_mute();
        }
    }
    else
    {
        const seq::pointer track = seqinfo(seqno).loop();
        if (track)
            track->toggle_song_mute();
    }
}

/**
 *  Restores the armed statuses saved when learn_armed_statuses() saved the
 *  current status, before toggling them.  Remember that these statuses have
 *  nothing to do with mute-groups!
 */

void
screenset::apply_armed_statuses ()
{
    for (auto & s : m_container)
    {
        if (s.armed_status())                       /* checks active()  */
        {
            seq::pointer sp = s.loop();             /* guaranteed valid */
            sp->toggle_song_mute();
            sp->toggle_playing();
        }
    }
}

/**
 *  Learns the current armed status, and also toggles them.  Remember that these
 *  statuses have nothing to do with mute-groups!
 *
 * \return
 *      Returns true if an armed status was found.
 */

bool
screenset::learn_armed_statuses ()
{
    bool result = false;
    for (auto & s : m_container)
    {
        seq & seqstatus = s;
        if (seqstatus.active())             /* guarantees a valid pointer */
        {
            seq::pointer sp = seqstatus.loop();
            bool armed = sp->get_playing();
            seqstatus.armed_status(armed);
            if (armed)
            {
                result = true;
                sp->toggle_song_mute();
                sp->toggle_playing();
            }
        }
    }
    return result;
}

/**
 *
 * \param seqno
 *      Either a track number or SEQ66_ALL_TRACKS (the default value).
 */

void
screenset::apply_song_transpose (seq::number seqno)
{
    if (sequence::all(seqno))
    {
        for (auto & s : m_container)
        {
            if (s.active())          /* guarantees a valid pointer */
                s.loop()->apply_song_transpose();
        }
    }
    else
    {
        seq::pointer sp = find_by_number(seqno);
        if (sp)
            sp->apply_song_transpose();
    }
}

/**
 *
 * \param seqno
 *      The sequence number to be affected.
 *
 * \param on
 *      True if the sequence is to be turned on, false if it is to be turned
 *      off.
 *
 * \param qinprogress
 *      Indicates if queuing is in progress.  This status is obtained from the
 *      performer.
 */

void
screenset::sequence_playing_change
(
    seq::number seqno,
    bool on,
    bool qinprogress
)
{
    auto seqit = std::find_if               /* also see find_by_number()    */
    (
        m_container.begin(), m_container.end(),
        [ & seqno ] (const seq & sn) { return sn.seq_number() == seqno; }
    );
    if (seqit != m_container.end())
        seqit->sequence_playing_change(on, qinprogress);
    else
        printf("sequence %d not found!\n", seqno);
}

/**
 *  For all active patterns/sequences in the current (playing) screen-set,
 *  this function gets the playing status and saves it in m_sequence_queued.
 *  Inactive patterns get the value set to false.  Used in saving the
 *  screen-set state during the queued-replace (queued-solo) operation, which
 *  occurs when the c_status_replace is performed while c_status_queue is
 *  active.  This information is used in unqueue().
 *
 * \param repseq
 *      Provides the number of the pattern for which the replace functionality
 *      is invoked.  This pattern will set to "playing" whether it is on or
 *      off, so that it can stay active while toggling between "solo" and
 *      "playing with the rest of the patterns".
 */

void
screenset::save_queued (seq::number repseq)
{
    clear_queued();
    for (auto & s : m_container)
    {
        if (s.active())
        {
            seq::pointer sp = s.loop();
            seq::number seqno = sp->seq_number();
            bool on = sp->get_playing() || (seqno == repseq);
            s.queued(on);
        }
    }
}

/**
 *  Does a toggle-queueing for all of the sequences in this screenset,
 *  for all sequences that are on, and for the currently hot-keyed sequence.
 *  This function supports the queued-replace feature, which is
 *  activated when the keep-queue feature is turned on via its hot-key, and
 *  then the replace hot-key is used on a pattern.  When that happens, the
 *  current sequence is queued to be toggled, and all unmuted sequences are
 *  queued to toggle off at the same time.  Thus, this is a kind of
 *  queued-solo feature.
 *
 *  This function assumes we have called save_queued() first,
 *  so that the soloing can be exactly toggled.  Only sequences that were
 *  initially on should be toggled.
 *
 * \param hotseq
 *      This number is that of the sequence/pattern whose hot-key was struck.
 *      We don't want to toggle this one off, just on.
 */

void
screenset::unqueue (seq::number hotseq)
{
    for (auto & s : m_container)
    {
        if (s.active())
        {
            seq::pointer sp = s.loop();
            seq::number seqno = sp->seq_number();
            if (seqno == hotseq)
            {
                if (! sp->get_playing())
                    sp->toggle_queued();
            }
            else if (s.queued())
            {
                sp->toggle_queued();
            }
        }
    }
}

/**
 *  Applies the bits as track-muting values.  Note that not every sequence in a
 *  set will be active, in general.  Therefore we must check all seqs, and not
 *  quit if a sequence is missing.
 *
 * \param bits
 *      Provides the boolean container that is the source of mute statuses.
 *
 * \return
 *      Returns true if the bits were able to be applied.  A set of bits with a
 *      different count than this screenset's sequence count would cause this.
 */

bool
screenset::apply_bits (const midibooleans & bits)
{
    bool result = count() == int(bits.size());
    if (result)
    {
        int bit = 0;
        seq::number seqend = m_set_offset + m_set_size;
        for (seq::number seqno = m_set_offset; seqno != seqend; ++seqno, ++bit)
        {
            seq::pointer sp = find_by_number(seqno);
            if (sp)
            {
                bool armed = bits[bit];
                sp->set_song_mute(! armed);         /* calls set_playing()  */
            }
        }
    }
    return result;
}

/**
 *  Copies the current bits status of the screenset's sequences into the given
 *  boolean vector.  The vector is cleared before adding in the new bits.
 *
 * \param [out] bits
 *      Provides the destination for the sequence statuses.
 *
 * \return
 *      Returns true if the bits were filled with statuses.
 */

bool
screenset::learn_bits (midibooleans & bits)
{
    bool result = count() > 0;
    if (result)
    {
        int bit = 0;
        bits.clear();
        for (seq::number s = m_set_offset; s != m_set_maximum; ++s, ++bit)
        {
            seq::pointer sp = find_by_number(s);
            if (sp)
            {
                bool armed = sp->get_playing();     // playing / armed ???
                bits.push_back(midibool(armed));
            }
        }
    }
    return result;
}

/**
 *
 */

std::string
screenset::to_string (bool showseqs) const
{
    std::ostringstream result;
    int index = 0;
    result << "Set " << set_number() << " ('" << name() << "')" << std::endl;
    if (showseqs)
    {
        for (auto & s : m_container)
            result << s.to_string(index++);
    }
    return result.str();
}

/**
 *
 */

void
screenset::show (bool showseqs) const
{
    std::cout << to_string(showseqs);
}

}               // namespace seq66

/*
 * screenset.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


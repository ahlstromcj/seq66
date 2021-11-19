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
 * \file          screenset.cpp
 *
 *  This module declares a two dimensional vector class solely to hold the
 *  mute status of a number of sequences in a set.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-02-12
 * \updates       2021-11-19
 * \license       GNU GPLv2 or above
 *
 *  Implements the screenset class.  The screenset class represent all of the
 *  sequences in a screen-set.  A screen has a size given by the concept of
 *  rows and columns, mirroring what the user sees in the Sequencer66 user
 *  interface.
 */

#include <algorithm>                    /* std::find_if()                   */
#include <iomanip>                      /* std::setw() manipulator          */
#include <iostream>                     /* std::cout                        */
#include <sstream>                      /* std::ostringstream               */

#include "cfg/settings.hpp"             /* seq66::usr() and rc()            */
#include "play/screenset.hpp"           /* seq66::screenset class           */

/*
 *  This namespace is not documented because it screws up the document
 *  processing done by Doxygen.
 */

namespace seq66
{

/*
 * -------------------------------------------------------------------------
 * screenset
 * -------------------------------------------------------------------------
 */

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
    m_swap_coordinates  (usr().swap_coordinates()),
    m_set_size          (rows * columns),
    m_container         (),
    m_set_number        (setnum),
    m_set_offset        (m_set_number * m_set_size),
    m_set_maximum       (m_set_offset + m_set_size),
    m_set_name          (usable() ? "New" : ""),
    m_is_playscreen     (false),
    m_sequence_high     (0)
{
    clear();
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
    int seqno = setno * m_set_size;
    m_set_number = setno;
    m_set_offset = seqno;
    m_set_maximum = m_set_offset + m_set_size;
    for (auto & s : m_container)            /* renumber all the sequences   */
        s.change_seq_number(seqno++);       /* works only if seq is active  */
}

void
screenset::clear ()
{
    seq emptyseq;
    m_container.clear();
    for (int s = 0; s < m_set_size; ++s)
        m_container.push_back(emptyseq);
}

void
screenset::initialize (int rows, int columns)
{
    m_rows              = rows;
    m_columns           = columns;
    m_swap_coordinates  = usr().swap_coordinates();
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
 * \param [inout] seqno
 *      The desired sequence number, which is a linear value ranging from 0 to
 *      a number determined by the active set size multiplied by the number of
 *      sets.  The actual value is returned if the function result is true.
 *
 * \return
 *      Returns true if the sequence number didn't already exist and the
 *      sequence pointer was able to be added and activated.
 */

bool
screenset::add (sequence * s, seq::number & seqno)
{
    bool result = false;
    if (not_nullptr(s))
    {
        for (seq::number i = seqno - offset(); i < m_set_maximum; ++i)
        {
            seq sseq = seqinfo(i);              /* get seq info in the set  */
            if (! sseq.active())                /* no seq already in slot?  */
            {
                seqno = i + offset();           /* change to unused seqno   */
                result = sseq.activate(s, seqno);
                if (result)
                {
                    m_container[i] = sseq;
                    break;
                }
            }
        }
    }
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
    if (sp && ! sp->seq_in_edit())
    {
        seq newseq;                         /* non-functional pattern       */
        sp->set_playing(false);             /* turns off all notes as well  */
        m_container[seqno - offset()] = newseq;
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
 *  Counts the number of active patterns in this screenset.  Also updates
 *  m_sequence_high, in the same way that setmapper::add_sequence() does.
 */

int
screenset::active_count () const
{
    int result = 0;
    seq::number seqno = m_set_offset;
    m_sequence_high = 0;                        /* a mutable member         */
    for (auto & s : m_container)
    {
        if (s.active())
        {
            ++result;
            if (seqno > m_sequence_high)
                m_sequence_high = seqno;
        }
        ++seqno;
    }
    ++m_sequence_high;                          /* increment for for-loops  */
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
    seq::number result = seq::unassigned();
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
 *  The new version returns seq::unassigned if the row or column is out of
 *  range.
 *
 *  There is no "grid_to_index()" function needed as yet.
 */

seq::number
screenset::grid_to_seq (int row, int column) const
{
    if (row < 0 || row >= m_rows || column < 0 || column >= m_columns)
        return seq::unassigned();
    else if (swap_coordinates())
        return offset() + column + m_columns * row;
    else
        return offset() + row + m_rows * column;
}

seq::number
screenset::grid_to_seq (screenset::number setno, int row, int column) const
{
    if (row < 0 || row >= m_rows || column < 0 || column >= m_columns)
        return seq::unassigned();

    seq::number offset = setno * m_set_size;
    if (swap_coordinates())
        return offset + column + m_columns * row;
    else
        return offset + row + m_rows * column;
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
screenset::seq_to_grid
(
    seq::number seqno,
    int & row, int & column,
    bool global
) const
{
    seq::number index = seqno - offset();
    bool result = global || (index >= 0 && index < m_set_size);
    if (result)
        result = index_to_grid(index, row, column);

    return result;
}

bool
screenset::index_to_grid (seq::number index, int & row, int & column) const
{
    index %= m_set_size;                /* force it to be in range */
    if (swap_coordinates())
    {
        row = index / m_columns;
        column = index % m_columns;
    }
    else
    {
        row = index % m_rows;
        column = index / m_rows;
    }
    return true;
}

bool
screenset::is_seq_in_edit (seq::number seqno) const
{
    seq::pointer track = seqinfo(seqno).loop();
    return track ? track->seq_in_edit() : false ;
}

bool
screenset::any_in_edit () const
{
    for (auto & s : m_container)
    {
        if (s.active() && s.loop()->seq_in_edit())
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
        if (s.active() && s.loop()->playing())
            return true;
    }
    return false;
}

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
    if (seqno >= offset())
        seqno -= offset();

    if (seqno < 0)
        return 0;
    else if (seqno >= m_set_size)
        return m_set_size - 1;

    return seqno;
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
screenset::song_recording_start (midipulse current_tick)
{
    for (auto & s : m_container)
    {
        if (s.active())
            s.loop()->song_recording_start(current_tick);   /* snap = true  */
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

/**
 *  This function assumes that this set is already created with the proper set
 *  number and pattern offsets.  This is true when we first move to the next
 *  set, which creates a screenset if it doesn't already exist.
 *  For copy/paste of screensets, we cannot use operator =(), because that
 *  makes the two sets have the same set number, etc.
 */

bool
screenset::copy_patterns (const screenset & source)
{
    bool result = source.active_count() > 0;
    if (result)
    {
        m_set_name = source.m_set_name;
        m_set_name += " copy";
        clear();                /* clear our sequences, init the container  */
        int srci = int(source.offset());
        int destend = int(offset()) + set_size();
        for (int desti = int(offset()); desti < destend; ++desti, ++srci)
        {
            seq::pointer s = source.loop(srci);
            if (s)
            {
                sequence * d = new (std::nothrow) sequence();
                if (not_nullptr(d))
                {
                    d->partial_assign(*s);
                    add(d, desti);
                }
            }
        }
    }
    return result;
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
 *  Runs a slot-handler function for all of the slots in this set.  Recall
 *  that a slot is a place to "store" a sequence, and a slot can be empty.
 *  All slots can be drawn, but empty slots are drawn differently in general.
 *
 * \param p
 *      Provides a function with parameters of seq::pointer and seq::number.
 *
 * \param use_set_offset
 *      If true, start the sequence counter from this set's seq::number
 *      offset.  This is the default.  Otherwise, start from 0.  In this case,
 *      if the true sequence number is needed, the function must get it from
 *      the sequence itself.
 */

bool
screenset::exec_slot_function (slothandler p, bool use_set_offset)
{
    bool result = false;
    seq::number sn = use_set_offset ? offset() : 0 ;
    for (auto & s : m_container)
    {
        result = p(s.loop(), sn++);         /* note post-increment of sn    */
        if (! result)                       /* false only if serious        */
            break;
    }
    return result;
}

/**
 *  Run one function on this set, and another function on each sequence in
 *  this set.
 *
 *  -   sethandler:  bool (screenset &, screenset::number)
 *  -   slothandler: bool (seq::pointer, seq::number)
 */

bool
screenset::exec_set_function (sethandler s, slothandler p)
{
    bool result = s(*this, 0);              /* handle set, index not used   */
    if (result)
        result = exec_slot_function(p);     /* handle set's slots/sequences */

    return result;
}

/**
 *  Sets one sequence (or all of them) to be dirty.
 *
 * \param seqno
 *      Either a track number or seq::all() (the default value).
 */

void
screenset::set_dirty (seq::number seqno)
{
    if (seqno == seq::all())
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

midipulse
screenset::max_timestamp () const
{
    midipulse result = 0;
    int seqno = 0;
    for (const auto & s : m_container)
    {
        if (s.active())
        {
            if (s.loop())
            {
                midipulse t = s.loop()->get_max_timestamp();
                if (t > result)
                    result = t;
            }
            else
            {
                errprintf
                (
                    "screenset::max_timestamp(): active null pointer seq %d\n",
                    seqno
                );
            }
        }
        ++seqno;
    }
    return result;
}

midipulse
screenset::max_extent () const
{
    midipulse result = 0;
    for (const auto & s : m_container)
    {
        if (s.active())
        {
            midipulse t = s.loop()->get_length();
            if (t > result)
                result = t;
        }
    }
    return result;
}

/*
 * -------------------------------------------------------------------------
 * Triggers
 * -------------------------------------------------------------------------
 */

/**
 *  Counts the triggers in the screenset.
 */

int
screenset::trigger_count () const
{
    int result = 0;
    for (const auto & s : m_container)
    {
        if (s.active())
            result += s.loop()->trigger_count();
    }
    return result;
}

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
 *      Either a track number or seq::all() (the default value).
 */

void
screenset::move_triggers
(
    midipulse lefttick, midipulse distance,
    bool direction, seq::number seqno
)
{
    if (seqno == seq::all())
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

void
screenset::push_trigger_undo ()
{
    for (auto & s : m_container)
    {
        if (s.active())
            s.loop()->push_trigger_undo();
    }
}

void
screenset::pop_trigger_undo ()
{
    for (auto & s : m_container)
    {
        if (s.active())
            s.loop()->pop_trigger_undo();
    }
}

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

bool
screenset::color (seq::number seqno, int c)
{
    bool result = false;
    seq::pointer track = seqinfo(seqno).loop();
    if (track)
        result = track->color(c);

    return result;
}

void
screenset::set_seq_name (seq::number seqno, const std::string & name)
{
    seq::pointer track = seqinfo(seqno).loop();
    if (track)
        track->set_name(name);
}

bool
screenset::name (const std::string & nm)
{
    bool result = nm != m_set_name;
    m_set_name = nm;
    return result;
}

void
screenset::arm (seq::number seqno)
{
    const seq::pointer track = seqinfo(seqno).loop();
    if (track)
    {
        track->set_playing(true);
#if defined USE_SET_SONG_MUTE
        track->set_song_mute(false);
#endif
    }
}

/**
 *  Do not set song mute here.
 */

void
screenset::mute (seq::number seqno)
{
    const seq::pointer track = seqinfo(seqno).loop();
    if (track)
    {
        track->set_playing(false);
#if defined USE_SET_SONG_MUTE
        track->set_song_mute(true);
#endif
    }
}

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
 *
 * \param p
 *      The play-set vector, owned by performer.
 *
 * \param
 *      Indicates to clear the play-set first.  Defaults to true.  Set it to
 *      false to append more play-set sequences.
 *
 * \return
 *      Returns true if there were any active sequences found in the current
 *      call of this function.
 */

bool
screenset::fill_play_set (playset & p, bool clearit)
{
    return p.fill(*this, clearit);
}

bool
screenset::add_to_play_set (playset & p, seq::number seqno)
{
    return p.add(*this, seqno);
}

/**
 * \param seqno
 *      Either a track number or seq::all() (the default value).
 */

void
screenset::copy_triggers
(
    midipulse lefttick, midipulse distance, seq::number seqno
)
{
    if (seqno == seq::all())
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
 *  Selects the set of triggers bounded by a low and high sequence number and
 *  a low and high tick selection.  If there is an inactive sequence in this
 *  range, it is simply ignored.  Also, will not cross a set boundary.  The
 *  setmapper makes sure this is the case before calling this function.
 *
 * \param seqlow
 *      Provides the low track to be selected, the low sequence number in the
 *      pattern range.
 *
 * \param seqhigh
 *      Provides the high track to be selected.  The high sequence number in
 *      the pattern range.  If not in the same set, nothing is done.  We need
 *      a way to report that.
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
    for (seq::number s = seqlow; s <= seqhigh; ++s)
    {
        auto lambdafunc = [s] (const seq & sseq) ->
            bool { return sseq.seq_number() == s; };

        auto seqit = std::find_if
        (
            m_container.begin(), m_container.end(), lambdafunc
        );
        if (seqit != m_container.end())
        {
            if (seqit->loop()->unselect_triggers())
            {
                for (long tick = tick_start; tick <= tick_finish; ++tick)
                    seqit->loop()->select_trigger(tick);
            }
        }
    }
}

/**
 *
 * \param seqno
 *      Either a track number or seq::all() (the default value).
 */

void
screenset::unselect_triggers (seq::number seqno)
{
    if (seqno == seq::all())
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

#if defined USE_SETMAPPER_RESET_SEQUENCES               /* currently unused */

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

#endif

void
screenset::arm ()
{
    for (auto & s : m_container)
    {
        if (s.active())                     /* guarantees a valid pointer */
        {
            seq::pointer sp = s.loop();
            sp->set_playing(true);          /* NEED ONLY ONE FUNCTION???  */
#if defined USE_SET_SONG_MUTE
            sp->set_song_mute(false);
#endif
        }
    }
}

void
screenset::mute ()
{
    for (auto & s : m_container)
    {
        if (s.active())                     /* guarantees a valid pointer */
        {
            seq::pointer sp = s.loop();
            sp->set_playing(false);
#if defined USE_SET_SONG_MUTE
            sp->set_song_mute(true);
#endif
        }
    }
}

void
screenset::toggle (seq::number seqno)
{
    if (seqno == seq::all())
    {
        for (auto & s : m_container)
        {
            if (s.active())                     /* guarantees valid pointer */
            {
                seq::pointer sp = s.loop();
                bool playing = sp->playing();
                sp->set_playing(! playing);     /* or toggle_playing()      */
#if defined USE_SET_SONG_MUTE
                sp->set_song_mute(playing);
#endif
            }
        }
    }
    else
    {
        const seq::pointer track = seqinfo(seqno).loop();
        if (track)
        {
            bool playing = track->playing();
            track->set_playing(! playing);
#if defined USE_SET_SONG_MUTE
            track->set_song_mute(playing);
#endif
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

void
screenset::toggle_song_mute (seq::number seqno)
{
    if (seqno == seq::all())
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
            bool armed = sp->playing();
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
 *      Either a track number or seq::all() (the default value).
 */

void
screenset::apply_song_transpose (seq::number seqno)
{
    if (seqno == seq::all())
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
    {
        seqit->sequence_playing_change(on, qinprogress);
    }
    else
    {
        infoprintf("pattern %d is empty", seqno);
    }
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
    for (auto & s : m_container)
    {
        if (s.active())
        {
            seq::pointer sp = s.loop();
            seq::number seqno = sp->seq_number();
            bool on = sp->playing() || (seqno == repseq);
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
                if (! sp->playing())
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
 *  Note that an unapply_bits() function is not needed here; we just follow the
 *  bits given.
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
        seq::number seqend = offset() + m_set_size;
        for (seq::number seqno = offset(); seqno != seqend; ++seqno, ++bit)
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
        for (seq::number s = offset(); s != m_set_maximum; ++s, ++bit)
        {
            seq::pointer sp = find_by_number(s);
            if (sp)
            {
                bool armed = sp->playing();
                bits.push_back(midibool(armed));
            }
            else
                bits.push_back(midibool(false));
        }
    }
    return result;
}

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

void
screenset::show (bool showseqs) const
{
    std::cout << to_string(showseqs);
}

/*
 * -------------------------------------------------------------------------
 * playset
 * -------------------------------------------------------------------------
 */

playset::playset () :
    m_screen_sets       (),
    m_sequence_array    ()
{
    // No code needed at this time
}

bool
playset::set_found (screenset::number setno) const
{
    const auto seqiterator = m_screen_sets.find(setno);
    return seqiterator != m_screen_sets.cend();
}

bool
playset::fill (const screenset & sset, bool clearit)
{
    bool result = false;
    if (clearit)
        clear();

    auto p = std::make_pair(sset.set_number(), &sset);
    auto r = m_screen_sets.insert(p);
    if (r.second)
    {
        for (auto & s : sset.seq_container())
        {
            if (s.active())
            {
                m_sequence_array.push_back(s.loop());
                result = true;
            }
        }
    }
    return result;
}

/**
 *  This function does not clear the containers each time.  It assumes the
 *  sequence has been installed via the setmapper, so that the necessary set
 *  already exists, and is provided here.  If the set already exists, it won't
 *  be inserted, which is not an error.
 */

bool
playset::add (const screenset & sset, seq::number seqno)
{
    const seq & s = sset.seqinfo(seqno);
    bool result = s.active();
    if (result)
        m_sequence_array.push_back(s.loop());

    return result;
}

}               // namespace seq66

/*
 * screenset.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


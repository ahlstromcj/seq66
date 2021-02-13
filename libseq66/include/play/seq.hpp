#if ! defined SEQ66_SEQ_HPP
#define SEQ66_SEQ_HPP

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
 * \file          seq.hpp
 *
 *  This module declares a small manager for a set of sequences, to be used by
 *  the performer.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-02-12
 * \updates       2021-01-31
 * \license       GNU GPLv2 or above
 *
 *  This module also creates a small structure for managing sequence variables,
 *  to save on a bunch of arrays.  It adds the extra information about sequences
 *  that was formerly provided by separate arrays.
 *
 * Special static test functions:
 *
 *  -   maximum(). Returns the maximum supported usable sequence
 *      number (plus one), which is 1024, but could be increased to 2048.  To
 *      clarify, usable sequence numbers range from 0 to 1023 at present.
 *  -   limit().  Returns 2048 (0x0800), which indicates a legal value that
 *      represents "no background" sequence when present in a Sequencer66 MIDI
 *      file.
 *  -   legal(seqno). Returns true if the number is between 0 and 2048.
 *  -   valid(seqno). Returns true if the number is between 0 and 2047.
 *  -   none(seqno). Returns true if the sequence number is -1.
 *  -   disabled(seqno). Return true if the sequence number is limit().
 *  -   null(seqno).
 *  -   all(seqno). Returns true if the sequence number is -1.  To be used
 *      only in the context of functions and can work on one sequence or all
 *      of them.  The caller should pass sequence::unassigned() as the
 *      sequence number.
 *  -   unassigned().  Returns the value of -1 for sequence number.
 */

#include <memory>                       /* std::shared_ptr<>                */
#include <map>                          /* std::map<>                       */

#include "play/sequence.hpp"            /* seq66::sequence                  */

/*
 *  This namespace is not documented because it screws up the document
 *  processing done by Doxygen.
 */

namespace seq66
{

/**
 *  Holds the various additional statuses, including the pointer, for a single
 *  sequence (also known as a loop or pattern).  This small class consolidates
 *  data once held in separate arrays.  It is also generally meant to be
 *  private, to be used only by the screenset class.  However, simple accessors
 *  and the seq::pointer alias for a shared pointer are public.  Also, the
 *  destructor definitely must be public, otherwise there can be an error
 *  concerning static_assert::is_destructible.
 */

class seq
{
    friend class performer;
    friend class playset_temp;
    friend class screenset;
    friend class setmapper;

public:

    /**
     *  Provides a more descriptive alias for the sequences numbers (which range
     *  from 0 to the maximum sequence number allowed for a given run o fthe
     *  application.
     */

    using number = int;

    /**
     *  Provides public access to the shared pointer for a sequence.  No more
     *  raw pointers!  It cannot be a unique_ptr<> because m_seq needs to be
     *  returned to callers.
     */

    using pointer = std::shared_ptr<sequence>;

private:

    /**
     *  Provides a smart pointer to a pattern/sequence/loop.
     */

    pointer m_seq;

    /**
     *  Each boolean value in this array is set to true if a sequence is
     *  active, meaning that it will be used to hold some kind of MIDI data,
     *  even if only Meta events.  This array can have "holes" with inactive
     *  sequences, so every sequence needs to be checked before using it.
     *  This flag will be true only if the sequence pointer is not null and if
     *  the sequence potentially contains some MIDI data.
     */

    bool m_seq_active;

    /**
     *  Each boolean value in this array is set to true if a sequence was
     *  active, meaning that it was found to be active at the time we were
     *  setting it to inactive.  This value seems to be used only in
     *  maintaining dirtiness-status; did some process modify the sequence?
     *  Was it's mute/unmute status changed?
     */

    mutable bool m_was_active_main;

    /**
     *  Each boolean value in this array is set to true if a sequence was
     *  active, meaning that it was found to be active at the time we were
     *  setting it to inactive.  This value seems to be used only in
     *  maintaining dirtiness-status for editing the mute/unmute status during
     *  pattern editing.
     */

    mutable bool m_was_active_edit;

    /**
     *  Each boolean value in this array is set to true if a sequence was
     *  active, meaning that it was found to be active at the time we were
     *  setting it to inactive.  This value seems to be used only in
     *  maintaining dirtiness-status for editing the mute/unmute status during
     *  performance/song editing.
     */

    mutable bool m_was_active_perf;

    /**
     *  Each boolean value in this array is set to true if a sequence was
     *  active, meaning that it was found to be active at the time we were
     *  setting it to inactive.  This value seems to be used only in
     *  maintaining dirtiness-status for editing the mute/unmute status during
     *  performance names editing.  Not sure that it serves a real purpose;
     *  perhaps created with an eye to editing the pattern name in the song
     *  editor?
     */

    mutable bool m_was_active_names;

    /**
     *  Indicates the status of this sequence when the arming statuses of all
     *  sequences have been saved for later restoration.  Used by
     *  save_playing_state() and restore_playing_state() for handling the
     *  snapshot functionality.  Meant to be used for all existing sequences.
     */

    bool m_snapshot_status;

    /**
     *  Indicates the status of this sequence when the arming statuses of all
     *  sequences have been saved for later restoration.  Used by
     *  toggle_playing_tracks().  Meant to be used for all existing sequences.
     */

    bool m_armed_status;

    /**
     *  Saves the current playing state only for the current set.
     *  This is used in the new queue-replace (queue-solo) feature.
     */

    bool m_queued;

public:

    /**
     *  Most of these functions are compiler generated.
     */

    seq ();
    seq (const seq &) = default;
    seq & operator = (const seq &) = default;
    seq (seq &&) = default;
    seq & operator = (seq &&) = default;
    ~seq ();

    /**
     *  The limiting sequence number, in macro form.  This value indicates
     *  that no background sequence value has been assigned yet.  However, we
     *  have issues saving a negative number in MIDI, so we will use the
     *  "proprietary" track's bogus sequence number, which doubles the 1024
     *  sequences we can support.  Values between 0 (inclusive) and 2048
     *  (exclusive) are valid.  But 2048 is a <i> legal</i> value, used only
     *  for disabling the selection of a background sequence.
     */

    static number limit ()
    {
        return 2048;    /* 0x0800 */
    }

    /**
     *  The maximum number of patterns supported is given by the number of
     *  patterns supported in the panel (32) times the maximum number of sets
     *  (32), or 1024 patterns.  However, this value is now independent of the
     *  maximum number of sets and the number of sequences in a set.  Instead,
     *  we limit them to a constant value, which seems to be well above the
     *  number of simultaneous playing sequences the application can support.
     *  Based on trials, the b4uacuse-stress.midi file, which has only about 4
     *  sets (128 patterns) pretty much loads up a CPU.  Based on
     *  seq::limit(), we can have patterns ranging from 0 to 2047.  For
     *  testing right now, we leave the old limit in place.
     */

    static int maximum ()
    {
        return 1024;
    }

    /**
     *  Indicates that all patterns will be processed by a function taking a
     *  seq::number parameter.
     */

    static number all ()
    {
        return (-2);
    }

    /**
     *  Indicates that a sequence number has not been assigned.
     */

    static number unassigned ()
    {
        return (-1);
    }

    /**
     *  A convenient macro function to test against limit().  Although above
     *  the range of usable loop numbers, it is a legal value.
     *  Compare this to the valid() function.
     */

    static bool legal (int seqno)
    {
        return seqno >= 0 && seqno <= limit();
    }

    /**
     *  Checks if a the sequence number is an assigned one, i.e. not equal to
     *  -1.  Replaces the null() function.
     */

    static bool none (number seqno)
    {
        return seqno == unassigned();
    }

    static bool assigned (number seqno)
    {
        return seqno != unassigned();
    }

    /**
     *  Similar to legal(), but excludes limit().
     */

    static bool valid (number seqno)
    {
        return seqno >= 0 && seqno < maximum();
    }

    /**
     *  A convenient function to test against sequence::limit().
     *  This function does not allow that value as a valid value to use.
     */

    static bool disabled (number seqno)
    {
        return seqno == limit();
    }

    const pointer loop () const
    {
        return m_seq;
    }

    /**
     *  Checks if the sequence has been properly installed via the performer.
     *  Since we can have holes in the sequence "array", where there are
     *  inactive sequences, we check if the sequence is even active before
     *  emitting a message about a null pointer for the sequence.  We only
     *  want to see messages that indicate actual problems.
     *
     * \return
     *      Returns true if the m_seq_active flag is set.
     */

    bool active () const
    {
        return m_seq_active;
    }

    bool armed_status () const
    {
        return m_seq_active && m_armed_status;
    }

    bool queued () const
    {
        return m_queued;
    }

    void set_was_active ();

    void clear_snapshot ()
    {
        m_snapshot_status = false;
    }

    void save_snapshot ()
    {
        m_snapshot_status = active () ? m_seq->playing() : false ;
    }

    void restore_snapshot ()
    {
        if (active())
            m_seq->set_playing(m_snapshot_status);
    }

private:

    bool activate (sequence * s, number seqno);
    bool activate (number seqno, bool active = true);
    bool deactivate ();
    bool is_exportable () const;
    bool is_dirty_main () const;
    bool is_dirty_edit () const;
    bool is_dirty_perf () const;
    bool is_dirty_names () const;
    void sequence_playing_change (bool on, bool q_in_progress);
    std::string to_string (int index) const;
    void show (int index = 0) const;

    seq::number seq_number () const
    {
        return active() ?
            seq::number(m_seq->seq_number()) : seq::unassigned() ;
    }

    void change_seq_number (seq::number seqno)
    {
        if (active())
            m_seq->seq_number(seqno);
    }

    void armed_status (bool flag)
    {
        m_armed_status = flag;
    }

    void queued (bool flag)
    {
        m_queued = flag;
    }

    pointer loop ()
    {
        return m_seq;
    }

};              // class seq

}               // namespace seq66

#endif          // SEQ66_SEQ_HPP

/*
 * seq.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


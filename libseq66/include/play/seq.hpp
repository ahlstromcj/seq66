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
 * \updates       2019-08-04
 * \license       GNU GPLv2 or above
 *
 *  This module also creates a small structure for managing sequence variables,
 *  to save on a bunch of arrays.  It adds the extra information about sequences
 *  that was formerly provided by separate arrays.
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
     *  Provides public access to the shared pointer for a sequence.  No more raw
     *  pointers!  It cannot be a unique_ptr<> because m_seq needs to be
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
     *
     *      bool m_screenset_state[c_seqs_in_set];
     */

    bool m_queued;

public:

    /**
     *  Most of these functions are compiler generated.
     */

    seq (sequence * s = nullptr);
    seq (const seq &) = default;
    seq & operator = (const seq &) = default;
    seq (seq &&) = default;
    seq & operator = (seq &&) = default;
    ~seq ();                    /* = default */

    static number limit ()
    {
        return SEQ66_SEQUENCE_LIMIT;
    }

    static number none ()
    {
        return SEQ66_UNASSIGNED;
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
        m_snapshot_status = active () ? m_seq->get_playing() : false ;
    }

    void restore_snapshot ()
    {
        if (active())
            m_seq->set_playing(m_snapshot_status);
    }

private:

    bool activate (sequence * s, number seqnum);
    bool activate (number seqnum, bool active = true);
    bool deactivate(number seqnum);
    bool is_exportable () const;
    bool is_dirty_main () const;
    bool is_dirty_edit () const;
    bool is_dirty_perf () const;
    bool is_dirty_names () const;
    void sequence_playing_change (bool on, bool q_in_progress);
    std::string to_string (int index) const;
    void show (int index = 0) const;

    int seq_number () const
    {
        return active() ? int(m_seq->seq_number()) : sequence::unassigned() ;
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

    void sloop (sequence * s)       // \deprecated
    {
        m_seq.reset(s); // m_seq_active = not_nullptr(s);
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


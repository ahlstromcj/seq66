#if ! defined SEQ66_SCREENSET_HPP
#define SEQ66_SCREENSET_HPP

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
 * \file          screenset.hpp
 *
 *  This module declares a small manager for a set of sequences, to be used by
 *  the performer.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-02-12
 * \updates       2019-09-07
 * \license       GNU GPLv2 or above
 *
 *  This module also creates a small structure for managing sequence variables,
 *  to save on a bunch of arrays.  It manages screen-sets and mute-groups.
 *
 *  This module is meant to support the
 *  saved 'armed' statuses and the current states of the
 *  tracks or sets.  The members from the perform(er) class are:
 *
 *      -   bool m_armed_statuses[c_max_sequence];
 *      -   std::vector<bool> m_tracks_mute_state;
 */

#include <functional>                   /* std::function, function objects  */
#include <map>                          /* std::map<>                       */
#include <vector>                       /* std::vector<>                    */

#include "play/seq.hpp"                 /* seq66::seq extension class       */

/**
 *  Similar to SEQ66_SEQUENCE_LIMIT, indicates an item that is outside the legal
 *  range.
 *
 *  #define SEQ66_SET_LIMIT                 2048
 *
 */

/*
 *  This namespace is not documented because it screws up the document
 *  processing done by Doxygen.
 */

namespace seq66
{

/**
 *  Holds the various statuses, including the pointer, for a single sequence
 *  (also known as a loop or pattern).  This small class consolidates data once
 *  held in separate arrays.
 *
 *  This object could also represent mute statuses, but that would waste a lot of
 *  memory.
 */

class screenset
{
    friend class setmapper;

public:

    /**
     *  Provides a more recognizable alias for a screen-set number:
     *  screenset::number.
     */

    using number = int;

    /**
     *  Provides a type alias for a function that can be called on all
     *  sequences in a set.  A caller will create this function and pass it to
     *  the slots_function() function.  The value for the seq::number parameter
     *  is provided by slots_function(). See qseqeditframe64 ::
     *  popup_sequence_menu() for an example of the std::bind() call for this
     *  kind of function.
     */

    using slothandler = std::function<bool (seq::pointer, seq::number)>;

    /**
     *  Provides a type alias for a function that can be called on all slots in
     *  a set.  A caller will create this function and pass it to the
     *  sets_function() function. See qseqeditframe64 :: popup_sequence_menu()
     *  for an example of the std::bind() call for this kind of function.
     */

    using sethandler = std::function<bool (screenset &, screenset::number)>;

    /**
     *  Provides a type to support condensing the screenset into a smaller array
     *  for use by the performer, as a bit of optimization.
     */

    using playset = std::vector<seq::pointer>;

private:

    /**
     *  Provides an alias for a vector of seq objects.  The "key" is an integer
     *  which is the sequence number, and is basically an array index.
     *  The value is a seq object representing sequence.  This container holds
     *  both inactive and active slots/sequences.  It is a vector because it
     *  is cheaper to hold empty slots than it is in a map.
     */

    using container = std::vector<seq>;

private:

    /**
     *  Indicates that no set number has been assigned.  All valid set numbers are
     *  greater than 0.
     */

    static const int sm_number_none = (-1);

    /**
     *  Indicates the number of virtual rows in a screen-set (bank), which is
     *  also the same number of virtual rows as a mute-group.  This value will
     *  generally be the same as the size used in the rest of the application.
     *  The default value is the historical value of 4 rows per set or
     *  mute-group.  It can be mapped against a sequence number.  Note that we
     *  removed the const qualifier, as this causes issues with containers.
     */

    int m_rows;

    /**
     *  Indicates the number of virtual columns in a screen-set (bank), which
     *  is also the same number of virtual columns as a mute-group.  This
     *  value will generally be the same as the size used in the rest of the
     *  application.  The default value is the historical value of 8 columns
     *  per set or mute-group.  It can be mapped against a sequence number.
     */

    int m_columns;

    /**
     *  Indicates the size of a screenset, equivalent to the rows x columns
     *  measurement.  In this map implementation, it is not pre-allocated.
     */

    int m_set_size;

    /**
     *  Holds a generally sparse vector of seq objects.
     */

    container m_container;

    /**
     *  Indicates the the set (bank) number represented by this screenset
     *  object.  If set to sm_number_none, this screenset is not active.
     */

    number m_set_number;

    /**
     *  Indicates the screen-set offset (the number of the first loop/pattern
     *  in the screen-set).  This value is m_set_size * m_screenset_number.
     *  This saves a calculation.
     */

    seq::number m_set_offset;

    /**
     *  Indicates a number one above the maximum sequence number for this
     *  screenset.  Saves a calculation.
     */

    seq::number m_set_maximum;

    /**
     *  Holds the notepad text/name for this screenset.  Replaces the
     *  m_screenset_notepad[] array.
     */

    std::string m_set_name;

    /**
     *  Is this screenset the current play-screen?  Managed by the setmapper.
     */

    bool m_is_playscreen;

public:

    screenset () = delete;
    screenset
    (
        number setnum,
        int rows        = SEQ66_DEFAULT_SET_ROWS,
        int columns     = SEQ66_DEFAULT_SET_COLUMNS
    );

    /**
     *  The move and copy constructors, the move and copy assignment operators,
     *  and the destructors are all compiler generated.
     */

    screenset (const screenset &) = default;
    screenset & operator = (const screenset &) = default;
    screenset (screenset &&) = default;
    screenset & operator = (screenset &&) = default;
    ~screenset () = default;

    static number limit ()
    {
        return 2048;
    }

    static number none ()
    {
        return (-1);
    }

    bool dummy () const
    {
        return m_set_number == limit();
    }

    bool usable () const
    {
        return m_set_number != sm_number_none && ! dummy();
    }

    seq::number offset () const
    {
        return m_set_offset;
    }

    int rows () const
    {
        return m_rows;
    }

    int columns () const
    {
        return m_columns;
    }

    int count () const
    {
        return int(m_container.size());
    }

    screenset::number set_number () const
    {
        return m_set_number;
    }

    void change_set_number (screenset::number setno);

    const std::string & name () const
    {
        return m_set_name;
    }

    bool is_playscreen () const
    {
        return m_is_playscreen;
    }

    bool active () const;
    int active_count () const;
    seq::number first_seq () const;

    /**
     *  Gets the desired sequence / loop / pattern / track pointer.
     *
     *  A set may be newly created, and have no sequences.
     *
     * \param seqno
     *      Provides the index to the sequence (normally from 0 to 1023).
     *      Is it worth validating this parameter?  We want speed.
     *      But validity is enforced by the seqinfo() function.
     */

    seq::pointer loop (seq::number seqno)
    {
        return seqinfo(seqno).loop();
    }

    const seq::pointer loop (seq::number seqno) const
    {
        return seqinfo(seqno).loop();
    }

    int color (seq::number seqno) const
    {
        const seq::pointer track = seqinfo(seqno).loop();
        return track ? track->color() : (-1) ;
    }

    bool active (seq::number seqno) const
    {
        return seqinfo(seqno).active();
    }

    bool is_seq_in_edit (seq::number seqno) const;
    bool any_in_edit () const;

    bool is_exportable (seq::number seqno) const
    {
        return seqinfo(seqno).is_exportable();
    }

    bool is_dirty_main (seq::number seqno) const
    {
        return seqinfo(seqno).is_dirty_main();
    }

    bool is_dirty_edit (seq::number seqno) const
    {
        return seqinfo(seqno).is_dirty_edit();
    }

    bool is_dirty_perf (seq::number seqno) const
    {
        return seqinfo(seqno).is_dirty_perf();
    }

    bool is_dirty_names (seq::number seqno) const
    {
        return seqinfo(seqno).is_dirty_names();
    }

    void activate (seq::number slotnum, seq::number seqno, bool flag = true)
    {
        seqinfo(slotnum).activate(seqno, flag);
    }

    bool armed () const;

    bool armed (seq::number seqno) const
    {
        const seq::pointer track = seqinfo(seqno).loop();
        return track ? track->get_playing() : false ;
    }

    bool armed_status (seq::number seqno) const
    {
        const seq & s = seqinfo(seqno);
        return s.active() ? s.armed_status() : false ;
    }

    bool muted (seq::number seqno) const
    {
        return ! armed(seqno);
    }

    bool seq_in_set (seq::number seqno) const
    {
        return seqno >= m_set_offset && seqno < m_set_maximum;
    }

    seq::number calculate_seq (int row, int column) const;
    bool seq_to_grid (seq::number seqno, int & row, int & column) const;
    bool needs_update () const;

    bool sets_function (sethandler s, screenset::number index)
    {
        return s(*this, index);
    }

    bool sets_function (sethandler s, slothandler p);
    bool slots_function (slothandler p);

private:

    seq::number clamp (seq::number seqno) const;
    seq::pointer find_by_number (seq::number seqno);
    void fill_play_set (playset & p);

    seq::number play_seq (int delta)
    {
        return offset() != seq::unassigned() ?
            offset() + delta : seq::unassigned() ;
    }

    void clear_queued ();
    void off_sequences ();
    void song_recording_stop (midipulse current_tick);
    void clear_snapshot ();
    void save_snapshot ();
    void restore_snapshot ();
    void set_last_ticks (midipulse tick);

    midipulse max_trigger () const;
    void unselect_triggers (seq::number seqno = seq::all());
    void select_triggers_in_range
    (
        seq::number seqlow, seq::number seqhigh,
        midipulse tick_start, midipulse tick_finish
    );
    void move_triggers
    (
        midipulse lefttick, midipulse distance,
        bool direction, seq::number seqno = seq::all()
    );
    void copy_triggers
    (
        midipulse lefttick, midipulse distance,
        seq::number seqno = seq::all()
    );
    void push_trigger_undo ();
    void pop_trigger_undo ();
    void pop_trigger_redo ();

    bool apply_bits (const midibooleans & mg);
    bool learn_bits (midibooleans & mg);

    /*
     * For a non-existent sequence number, should this return a dummy (inactive)
     * object reference, or create an empty seq?  Or should it throw?
     */

    const seq & seqinfo (seq::number seqno) const
    {
        return m_container.at(clamp(seqno));
    }

private:

    bool add (sequence *, seq::number seqno);
    bool remove (seq::number seqno);
    void reset_sequences (bool pause, sequence::playback mode);
    void set_dirty (seq::number seqno = seq::all());
    void toggle (seq::number seqno = seq::all());
    void toggle_song_mute (seq::number seqno = seq::all());
    void arm ();
    void mute ();
    void apply_armed_statuses ();
    bool learn_armed_statuses ();
    void apply_song_transpose (seq::number seqno = seq::all());
    void sequence_playing_change (seq::number seqno, bool on, bool qinprogress);
    void save_queued (seq::number repseq); // save_current_screenset ()
    void unqueue (seq::number hotseq);
    void clear ();
    void initialize (int rows, int columns);
    std::string to_string (bool showseqs = true) const;
    void show (bool showseqs = true) const;
    void play (midipulse tick, sequence::playback mode, bool resumenoteons);
    bool color (seq::number seqno, int c);
    void set_seq_name (seq::number seqno, const std::string & name);
    bool name (const std::string & nm);

    void is_playscreen (bool flag)
    {
        m_is_playscreen = flag;
    }

    seq & seqinfo (seq::number seqno)
    {
        return m_container.at(clamp(seqno));
    }

    void panic ()
    {
        all_notes_off();
    }

    void armed_status (seq::number seqno, bool flag);
    void armed (seq::number seqno, bool flag);
    void arm (seq::number seqno);
    void mute (seq::number seqno);
    void all_notes_off ();

};              // class screenset

}               // namespace seq66

#endif          // SEQ66_SCREENSET_HPP

/*
 * screenset.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


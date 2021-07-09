#if ! defined SEQ66_QSEQBASE_HPP
#define SEQ66_QSEQBASE_HPP

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
 * \file          qseqbase.hpp
 *
 *  This module declares/defines the base class for the various editing panes
 *  of Seq66's Qt 5 version.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-06-20
 * \updates       2021-07-09
 * \license       GNU GPLv2 or above
 *
 *  This class is a base class for qseqroll, qseqdata, qtriggereditor, and
 *  qseqtime, and the four panes of the qseqeditframe64 class.  Used as a
 *  mix-in class.
 */

#include "app_limits.h"                 /* SEQ66_DEFAULT_ZOOM, _SNAP        */
#include "play/seq.hpp"                 /* seq66::seq::pointer              */
#include "qeditbase.hpp"                /* seq66::qbase basic UI date       */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class qseqeditframe64;

/**
 * The MIDI note grid in the sequence editor
 */

class qseqbase : public qeditbase
{

private:

    /**
     *  Holds a pointer to the edit-frame window.  We now have only one, more
     *  adaptable, seqedit frame to worry about.
     */

    qseqeditframe64 * m_parent_frame;

    /**
     *  Provides a reference to the sequence represented by piano roll.
     */

    seq::pointer m_seq;

    /**
     *  Tells where the dragging started, the x value.
     */

    int m_move_delta_x;

    /**
     *  Tells where the dragging started, the y value.
     */

    int m_move_delta_y;

    /**
     *  This item is used in the fruityseqroll module.
     */

    int m_move_snap_offset_x;

public:

    qseqbase
    (
        performer & p,
        seq::pointer seqp,
        qseqeditframe64 * frame,
        int zoom,
        int snap            = SEQ66_DEFAULT_SNAP,
        int unit_height     =  1,
        int total_height    =  1
    );

    virtual bool check_dirty () const override;

    void set_measures (int len);
    int get_measures ();

protected:

    int move_delta_x () const
    {
        return m_move_delta_x;
    }

    int move_delta_y () const
    {
        return m_move_delta_y;
    }

    int move_snap_offset_x () const
    {
        return m_move_snap_offset_x;
    }

    qseqeditframe64 * frame64 ()
    {
        return m_parent_frame;
    }

    const qseqeditframe64 * frame64 () const
    {
        return m_parent_frame;
    }

    void move_delta_x (int v)
    {
        m_move_delta_x = v;
    }

    void move_delta_y (int v)
    {
        m_move_delta_y = v;
    }

    void move_snap_offset_x (int v)
    {
        m_move_snap_offset_x = v;
    }

    /*
     * We are not the owner of this shared pointer.
     */

    const seq::pointer seq_pointer () const
    {
        return m_seq;
    }

    seq::pointer seq_pointer ()
    {
        return m_seq;
    }

    /*
     * Takes screen corrdinates, give us notes/keys (to be generalized to
     * other vertical user-interface quantities) and ticks (always the
     * horizontal user-interface quantity).
     */

    void convert_xy (int x, int y, midipulse & ticks, int & note);
    void convert_tn (midipulse ticks, int note, int & x, int & y);
    void convert_tn_box_to_rect
    (
        midipulse tick_s, midipulse tick_f, int note_h, int note_l,
        seq66::rect & r
    );

};          // class qseqbase

}           // namespace seq66

#endif      // SEQ66_QSEQBASE_HPP

/*
 * qseqbase.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


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
 * \file          qseqbase.cpp
 *
 *  This module declares/defines the base class for drawing on the piano
 *  roll of the patterns editor.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2020-07-29
 * \license       GNU GPLv2 or above
 *
 *  We are currently moving toward making this class a base class.
 *
 *  User jean-emmanual added support for disabling the following of the
 *  progress bar during playback.  See the seqroll::m_progress_follow member.
 */

#include "play/performer.hpp"           /* seq66::performer class           */
#include "qseqbase.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  We need to square this away.  This should be the configurable usr()
 *  key-height.
 */

static const int c_key_y =  8;

/**
 *  Primary constructor
 */

qseqbase::qseqbase
(
    performer & p,
    seq::pointer seqp,
    int zoom,
    int snap,
    int unitheight,
    int totalheight
) :
    qeditbase
    (
        p, zoom, snap, 1, c_keyboard_padding_x, unitheight, totalheight
    ),
    m_seq                   (seqp),
    m_move_delta_x          (0),
    m_move_delta_y          (0),
    m_move_snap_offset_x    (0)
{
    set_snap(m_seq->snap());
}

#if defined USE_SCROLLING_CODE    // not ready for this class

/**
 *  Sets the horizontal scroll value according to the current value of the
 *  horizontal scroll-bar.
 */

void
qseqbase::set_scroll_x (int x)
{
    m_scroll_offset_x = x;
    m_scroll_offset = pix_to_tix(x);
}

/**
 *  Sets the vertical scroll value according to the current value of the
 *  vertical scroll-bar.
 *
 *  Use the height member? See cppcheck as well.
 */

void
qseqbase::set_scroll_y (int y)
{
    m_scroll_offset_y = y;
    m_scroll_offset_key *= c_key_y;         // m_unit_height
    m_scroll_offset_key = y / c_key_y;      // m_unit_height
}

#endif  // USE_SCROLLING_CODE

/**
 *  Checks for the dirtiness of the user-interface or the current sequence.
 *  This function is an override of the qbase version.
 *
 * \return
 *      Returns true if an update is needed.
 */

bool
qseqbase::check_dirty () const
{
    bool dirty = qeditbase::check_dirty();
    if (! dirty)
    {
        performer & ncp = const_cast<performer &>(perf());
        dirty = ncp.needs_update(seq_pointer()->seq_number());
    }
    return dirty;
}

/**
 *  Set the measures value, using the given parameter, and some internal
 *  values passed to apply_length().
 *
 * \param len
 *      Provides the sequence length, in measures.
 */

void
qseqbase::set_measures (int len)
{
    seq_pointer()->apply_length(len);
    set_dirty();
}

int
qseqbase::get_measures ()
{
    return seq_pointer()->get_measures();
}

void
qseqbase::convert_xy (int x, int y, midipulse & tick, int & note)
{
    tick = pix_to_tix(x);
    note = (m_total_height - y - 2) / m_unit_height;
    if (note >= c_note_max)
        note = c_note_max;
    else if (note < 0)
        note = 0;

}

void
qseqbase::convert_tn (midipulse ticks, int note, int & x, int & y)
{
    if (note >= 0 && note <= c_note_max)
    {
        x = ticks / zoom();
        y = m_total_height - ((note + 1) * m_unit_height) - 1;
    }
    else
        x = y = 0;
}

/**
 *  See seqroll::convert_sel_box_to_rect() for a potential upgrade.
 *
 * \param tick_s
 *      The starting tick of the rectangle.
 *
 * \param tick_f
 *      The finishing tick of the rectangle.
 *
 * \param note_h
 *      The high note of the rectangle.
 *
 * \param note_l
 *      The low note of the rectangle.
 *
 * \param [out] r
 *      The destination rectangle for the calculations.
 */

void
qseqbase::convert_tn_box_to_rect
(
    midipulse tick_s, midipulse tick_f, int note_h, int note_l,
    seq66::rect & r
)
{
    int x1, y1, x2, y2;
    convert_tn(tick_s, note_h, x1, y1);     /* convert box to X,Y values    */
    convert_tn(tick_f, note_l, x2, y2);
    rect::xy_to_rect(x1, y1, x2, y2, r);
    r.height_incr(m_unit_height);
}

}           // namespace seq66

/*
 * qseqbase.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


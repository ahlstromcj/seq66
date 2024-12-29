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
 * \updates       2024-12-29
 * \license       GNU GPLv2 or above
 *
 *  We are currently moving toward making this class a base class.
 *
 *  User jean-emmanual added support for disabling the following of the
 *  progress bar during playback.  See the seqroll::m_progress_follow member.
 */

#include "play/performer.hpp"           /* seq66::performer class           */
#include "qseqbase.hpp"                 /* seq66::qseqbase class            */
#include "qseqeditframe64.hpp"          /* seq66::qseqeditframe64 class     */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  We need to square this away.  This should be the configurable usr()
 *  key-height. But it is not used, and we already have made it configurable.
 *
 *      static const int c_key_y =  8;
 */

/**
 *  Primary constructor
 */

qseqbase::qseqbase
(
    performer & p,
    sequence & s,
    qseqeditframe64 * frame,
    int zoom, int snap,
    int unitheight, int totalheight
) :
    qeditbase
    (
        p, zoom, snap, 1, c_keyboard_padding_x, unitheight, totalheight
    ),
    m_parent_frame          (frame),
    m_seq                   (s),
    m_move_delta_x          (0),
    m_move_delta_y          (0),
    m_move_snap_offset_x    (0)
{
    set_snap(track().snap());

    /*
     * four_pen_style(Qt::SolidLine);           // Qt::DotLine
     */

    if (usr().gridlines_thick())                /* otherise use defaults    */
        measure_pen_width(3);
}

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
        dirty = ncp.needs_update(track().seq_number());
    }
    return dirty;
}

bool
qseqbase::mark_unmodified ()
{
    if (not_nullptr(frame64()))
        frame64()->set_external_frame_title(false);

    return true;
}

bool
qseqbase::mark_modified ()
{
    if (not_nullptr(frame64()))
        frame64()->set_external_frame_title(true);

    return true;
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
    track().apply_length(len);
    set_dirty();
}

int
qseqbase::get_measures ()
{
    return track().get_measures();
}

void
qseqbase::convert_xy (int x, int y, midipulse & tick, int & note)
{
    tick = z().pix_to_tix(x);
    note = (m_total_height - y - 2) / m_unit_height;
    if (note >= c_note_max)
        note = c_note_max;
    else if (note < 0)
        note = 0;
}

/**
 *  Also see pulses_per_pixel() in the calculations module. Here we are
 *  doing the inverse calculation.
 */

void
qseqbase::convert_tn (midipulse ticks, int note, int & x, int & y)
{
    if (note >= 0 && note <= c_note_max)
    {
        x = ticks * usr().base_ppqn() / ppqn() / zoom();
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


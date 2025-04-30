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
 * \file          qperfbase.cpp
 *
 *  This module declares/defines the base class for drawing on the piano
 *  roll of the song editor.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-07-14
 * \updates       2025-04-30
 * \license       GNU GPLv2 or above
 *
 *  We are currently moving toward making this class a base class.
 *
 *  User jean-emmanual added support for disabling the following of the
 *  progress bar during playback.  See the seqroll::m_progress_follow member.
 */

#include <QWidget>                      /* QWidget::resize()                */

#include "cfg/settings.hpp"             /* seq66::usr()                     */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "qperfbase.hpp"                /* seq66::qperfbase class           */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Primary constructor.
 */

qperfbase::qperfbase
(
    performer & p,
    int zoom,
    int snap,
    int unitheight,
    int totalheight
) :
    qeditbase       (p, zoom, snap, c_perf_scale_x, 0, unitheight, totalheight),
    m_set_size      (usr().set_size()),
    m_width_factor  (1.25f),
    m_track_height  (c_names_y),
    m_track_thin    (false),
    m_track_thick   (false)
{
    beat_pen_style(Qt::DotLine);                /* default is SolidLine     */
    if (usr().gridlines_thick())                /* otherise use defaults    */
    {
        horiz_pen_width(2);
    }
    else
    {
        measure_pen_width(1);
    }
}

/**
 * performer::get_max_extent() gets the longest of the lengthiest trigger or
 * lengthiest pattern.  We pad it to 2000 or the result + 200, whichever is
 * greater.  This should cover most use cases.
 */

int
qperfbase::horizSizeHint () const
{
    int result = z().tix_to_pix(perf().get_max_extent());
    result = int(result * width_factor());
    return result;
}

/**
 *  Force a resize so that a sizeHint() call will occur so that the scrollbars
 *  for the perfroll and perfnames will update upon a change in the number of
 *  sets.  Have to do it twice because Qt sometimes optimizes the non-change
 *  out and doesn't call sizeHint().  Kind of crufty, but invisible to the
 *  user as far as we can see.
 */

void
qperfbase::force_resize (QWidget * self)
{
    int w = self->geometry().width();
    int h = self->geometry().height();
    self->resize(w + 1, h + 1);
    self->resize(w, h);
}

void
qperfbase::convert_x (int x, midipulse & tick)
{
    tick = z().pix_to_tix(x);               /* x * m_scale_zoom + tick_offset   */
}

/**
 *  Converts an (x, y) point on the user interface to the corresponding tick
 *  and patter number.
 */

void
qperfbase::convert_xy (int x, int y, midipulse & tick, int & seq)
{
    tick = z().pix_to_tix(x);
    seq = y / track_height();
    if (seq >= perf().sequence_max())
        seq = perf().sequence_max() - 1;
    else if (seq < 0)
        seq = 0;
}

/**
 *  Converts ticks to a user-interface x value, and converts a sequence number
 *  to a y value.  A kind of inverse function of convert_xy().
 */

void
qperfbase::convert_ts (midipulse ticks, int seq, int & x, int & y)
{
    if (seq >= 0)
    {
        x = z().tix_to_pix(ticks);
        y = m_total_height - ((seq + 1) * m_unit_height) - 1;
    }
    else
        x = y = 0;
}

/**
 *
 * \param tick_s
 *      The starting tick of the rectangle.
 *
 * \param tick_f
 *      The finishing tick of the rectangle.
 *
 * \param seq_h
 *      The high sequence row of the rectangle.
 *
 * \param seq_l
 *      The low sequence row of the rectangle.
 *
 * \param [out] r
 *      The destination rectangle for the calculations.
 */

void
qperfbase::convert_ts_box_to_rect
(
    midipulse tick_s, midipulse tick_f, int seq_h, int seq_l,
    seq66::rect & r
)
{
    int x1, y1, x2, y2;
    convert_ts(tick_s, seq_h, x1, y1);         /* convert box to X,Y values */
    convert_ts(tick_f, seq_l, x2, y2);
    rect::xy_to_rect(x1, y1, x2, y2, r);
    r.height_incr(m_unit_height);
}

}           // namespace seq66

/*
 * qperfbase.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


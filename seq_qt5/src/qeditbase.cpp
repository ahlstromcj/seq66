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
 * \file          qeditbase.cpp
 *
 *  This module declares/defines the base class for drawing on the piano
 *  roll of the song editor.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-08-05
 * \updates       2020-12-24
 * \license       GNU GPLv2 or above
 *
 *  We are currently moving toward making this class a base class.
 *
 *  User jean-emmanual added support for disabling the following of the
 *  progress bar during playback.  See the seqroll::m_progress_follow member.
 */

#include "play/performer.hpp"           /* seq66::performer class           */
#include "qeditbase.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *
 */

qeditbase::qeditbase
(
    performer & p,
    int initialzoom,
    int snap,
    int scalex,
    int padding,
    int unit_height,
    int total_height
) :
    qbase                   (p, initialzoom),
    m_back_color            (background_paint()),
    m_fore_color            (foreground_paint()),
    m_label_color           (label_paint()),
    m_sel_color             (sel_paint()),
    m_drum_color            (drum_paint()),
    m_progress_color        (progress_paint()),
    m_beat_color            (beat_paint()),
    m_step_color            (step_paint()),
    m_old                   (),                     /* past selection box   */
    m_selected              (),                     /* current sel box      */
    m_scale                 (scalex > 4 ? scalex / 4 : 1),
    m_scale_zoom            (m_scale * zoom()),     /* see change_ppqn()    */
    m_padding_x             (padding),
    m_snap                  (snap),
    m_grid_snap             (snap),
    m_beat_length           (p.ppqn()),             /* see change_ppqn()    */
    m_measure_length        (m_beat_length * 4),    /* see change_ppqn()    */
    m_selecting             (false),
    m_adding                (false),
    m_moving                (false),
    m_moving_init           (false),
    m_growing               (false),
    m_painting              (false),
    m_paste                 (false),
    m_drop_x                (0),
    m_drop_y                (0),
    m_current_x             (0),
    m_current_y             (0),
    m_progress_x            (0),
    m_old_progress_x        (0),
    m_scroll_page           (0),
    m_progress_follow       (false),
    m_scroll_offset         (0),
    m_scroll_offset_v       (0),
    m_scroll_offset_x       (0),
    m_scroll_offset_y       (0),
    m_unit_height           (unit_height),
    m_total_height          (total_height)
{
    // no other code needed
}

/**
 *  Why 2000?  I forget!
 */

int
qeditbase::horizSizeHint () const
{
    return perf().get_max_trigger() / scale_zoom() + 2000;
}

/**
 *
 */

bool
qeditbase::zoom_in ()
{
    bool result = qbase::zoom_in();
    if (result)
        m_scale_zoom = zoom() * m_scale;

    return result;
}

/**
 *
 */

bool
qeditbase::zoom_out ()
{
    bool result = qbase::zoom_out();
    if (result)
        m_scale_zoom = zoom() * m_scale;

    return result;
}

/**
 *
 */

bool
qeditbase::set_zoom (int z)
{
    bool result = qbase::set_zoom(z);
    if (result)
        m_scale_zoom = zoom() * m_scale;

    return result;
}

/**
 *  Handles changes to the PPQN value in one place.  Useful mainly at startup
 *  time for adjusting the user-interface zooming.
 *
 *  The m_ticks_per_bar member replaces the global ppqn times 16.  This
 *  construct is parts-per-quarter-note times 4 quarter notes times 4
 *  sixteenth notes in a bar.  (We think...)
 *
 *  The m_scale member starts out at c_perf_scale_x, which is 32 ticks
 *  per pixel at the default tick rate of 192 PPQN.  We adjust this now.
 *  But note that this calculation still involves the c_perf_scale_x constant.
 *
 * \todo
 *      Resolve the issue of c_perf_scale_x versus m_scale in perfroll.
 */

bool
qeditbase::change_ppqn (int p)
{
    m_scale_zoom = zoom() * m_scale;
    m_beat_length = p;                          /* perf().ppqn()            */
    m_measure_length = m_beat_length * 4;       /* what about beat width?   */
    return true;
}

/**
 *  Snap = number pulses to snap to.  Zoom = number of pulses per pixel.  Thus
 *  snap / zoom  = number pixels to snap to.
 */

void
qeditbase::snap_x (int & x)
{
    if (m_scale_zoom > 0)
    {
        int mod = m_snap / m_scale_zoom;
        if (mod <= 0)
            mod = 1;

        x -= x % mod;
    }
}

/**
 *  Checks to see if the song is running or if the "dirty" flag had been
 *  set.  The obtuse code here helps in debugging.  Also, we might consider
 *  using the new on_ui_change() callback.
 */

bool
qeditbase::check_dirty () const
{
    bool result = qbase::check_dirty();
    if (! result)
        result = perf().needs_update();

    return result;
}

/**
 *
 */

void
qeditbase::convert_x (int x, midipulse & tick)
{
    tick = pix_to_tix(x);                       /* x * m_scale_zoom */
}

/**
 *
 */

void
qeditbase::convert_ts (midipulse ticks, int seq, int & x, int & y)
{
    x = tix_to_pix(ticks);
    y = m_total_height - ((seq + 1) * m_unit_height) - 1;
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
qeditbase::convert_ts_box_to_rect
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
 * qeditbase.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


#if ! defined SEQ66_QEDITBASE_HPP
#define SEQ66_QEDITBASE_HPP

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
 * \file          qeditbase.hpp
 *
 *  This module declares/defines the base class for the sequence and
 *  performance editing frames of Seq66's Qt 5 version.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-08-05
 * \updates       2023-09-08
 * \license       GNU GPLv2 or above
 *
 *  This class will be the base class for the qseqbase and qperfbase classes.
 *  Both kinds of editing involve selection, movement, zooming, etc.
 */

#include "util/rect.hpp"                /* seq66::rect rectangle class      */
#include "gui_palette_qt5.hpp"          /* gui_pallete_qt5::Color etc.      */
#include "qbase.hpp"                    /* seq66:qbase super base class     */

/*
 * EXPERIMENT IN PROGRESS
 */

#undef SEQ66_USE_ZOOM_EXPANSION

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class performer;
    class sequence;

/**
 *  The dimensions and offset of the virtual keyboard at the left of the
 *  piano roll.  Some have been moved to the GUI (Qt) that needs them.
 */

const int c_keyboard_padding_x = 6;     /* Qt version of keys padding       */

/**
 *  The default value of the snap in the sequence/performance editors.
 */

const int c_default_snap    = 16;       /* default snap from app limits     */

/**
 *  The default value of the zoom indicates that one pixel represents two
 *  ticks.  However, it turns out we're going to have to support adapting the
 *  default zoom to the PPQN, in addition to allowing some extra zoom values.
 *  A redundant definition is used on the calculations module at present.
 *
 *  The maximum value of the zoom indicates that one pixel represents 512
 *  ticks.  The old maximum was 32, but now that we support PPQN up to 19200,
 *  we need extra entries.
 *
 *  Redundantly defined in usrsettings.
 */

const int c_minimum_zoom    =   1;      /* limit the amount of zoom         */
const int c_default_zoom    =   2;      /* default snap from app limits     */
const int c_maximum_zoom    = 512;      /* limit the amount of zoom         */

/**
 *  Provides basic functionality to be inherited by qseqbase and qperfbase.
 */

class qeditbase : public qbase
{

private:

    /**
     *  Colors common to the sequence and song edit panes.  We want to
     *  initialize them only at startup, to make painting faster.  We could
     *  make them constant, but we might eventually want to reload a new
     *  palette on the fly.  LATER.
     */

    const Color m_back_color;
    const Color m_fore_color;
    const Color m_label_color;
    const Color m_sel_color;
    const Color m_drum_color;
    const Color m_progress_color;
    const Color m_beat_color;
    const Color m_step_color;
    const Color m_note_in_color;
    const Color m_note_out_color;
    const Color m_tempo_color;
    const Color m_grey_color;

    /**
     *  Similarly, we provide stock brushes that are configurable.  This saves
     *  changing brush colors and styles so often.
     */

    const Brush m_blank_brush;      /* for clearing out rectangles, etc.    */
    const Brush m_note_brush;       /* also for perfroll trigger boxes?     */
    const Brush m_scale_brush;      /* background, usually hatched          */
    const Brush m_backseq_brush;    /* another background, usually hatched  */
    const bool m_use_gradient;      /* paint notes/triggers with gradient   */

protected:

    /**
     *  The previous selection rectangle, used for undrawing it.  Accessed by
     *  the getter/setting functions old_rect().
     */

    seq66::rect m_old;

    /**
     *  Used in moving and pasting notes.  Accessed by the getter/setting
     *  functions selection().
     */

    seq66::rect m_selected;

    /**
     *  X scaling.  Allows the caller to adjust the overall zoom.   A
     *  constant.
     */

    const int m_scale;

    /**
     *  Zoom times the scale, to save a very common calculation,
     *  m_zoom * m_scale.
     */

    int m_scale_zoom;

    /**
     *  An additional kind of zoom, useful for depicting dense events such as
     *  pitch-bend.  All it does is multiply the pixel numbers by this factor.
     *  The support values are 1 (the same as no expansion), 2, 4, and 8.
     *  It is accessible only via the zoom buttons and zoom keys, and applies
     *  only to the x (horizontal) direction.
     */

    size_t m_zoom_exp_index;                /* index into supported factors */
    int m_zoom_expansion;

    /**
     *  Provides additional padding to move items rightward to account for
     *  slight differences in the layout of items inside the edit frames.  It
     *  makes the time, roll, trigger/event, and data measures line up
     *  properly.
     */

    int m_padding_x;

    /**
     *  The event-snap setting for the piano roll grid.  Same meaning as for the
     *  event-bar grid.  This value is the denominator of the note size used
     *  for the snap.
     */

    int m_snap;

    /**
     *  The permanent snap for drawing the grid, barring a change in the PPQN.
     */

    int m_grid_snap;

    /**
     *  Provides the length of a beat, in ticks.
     */

    midipulse m_beat_length;

    /**
     *  Provides the length of a measure or bar, in ticks.
     */

    midipulse m_measure_length;

    /**
     *  Set when highlighting a bunch of events.
     */

    bool m_selecting;

    /**
     *  Set when in note-adding or trigger-adding mode.  This flag was moved
     *  from both the fruity and the seq66 seqroll classes.
     */

    bool m_adding;

    /**
     *  Set when moving a bunch of events.
     */

    bool m_moving;

    /**
     *  Indicates the beginning of moving some events.  Used in the fruity and
     *  seq66 mouse-handling modules.
     */

    bool m_moving_init;

    /**
     *  Indicates that the notes are to be extended or reduced in length.
     */

    bool m_growing;

    /**
     *  Indicates the painting of events.  Used in the fruity and seq66
     *  mouse-handling modules.
     */

    bool m_painting;

    /**
     *  Indicates that we are in the process of pasting notes.
     */

    bool m_paste;

    /**
     *  The x location of the mouse when dropped.  Would be good to allocate
     *  this to a base class for all grid panels.
     */

    int m_drop_x;

    /**
     *  The x location of the mouse when dropped.  Would be good to allocate
     *  this to a base class for all grid panels.
     */

    int m_drop_y;

    /**
     *  Current x coordinate of pointer.
     */

    int m_current_x;
    int m_last_snap_x;

    /**
     *  Current y coordinate of pointer.
     */

    int m_current_y;
    int m_last_snap_y;

    /**
     *  Provides the location of the progress bar.
     */

    int m_progress_x;

    /**
     *  Provides the old location of the progress bar, for "playhead"
     *  tracking.
     */

    int m_old_progress_x;

    /**
     *  Provides the current scroll page in which the progress bar resides.
     */

    int m_scroll_page;

    /**
     *  Progress bar follow state.
     */

    bool m_progress_follow;

    /**
     *  The horizontal value of the scroll window in units of
     *  ticks/pulses/divisions.
     */

    int m_scroll_offset;

    /**
     *  The vertical offset of the scroll window in units of sequences or MIDI
     *  notes/keys.
     */

    int m_scroll_offset_v;

    /**
     *  The horizontal value of the scroll window in units of pixels.
     */

    int m_scroll_offset_x;

    /**
     *  The vertical value of the scroll window in units of pixels.
     */

    int m_scroll_offset_y;

    /**
     *  Provides the height of a unit.  For qseqroll, this is note height.
     */

    int m_unit_height;

    /**
     *  See qseqroll::keyY * c_notes_count + 1.
     */

    int m_total_height;

public:

    qeditbase
    (
        performer & perf,
        int zoom,
        int scalex          = 1,
        int padding         = 0,
        int snap            = c_default_snap,
        int unit_height     = 1,
        int total_height    = 1
    );

    const Color & back_color () const
    {
        return m_back_color;
    }

    const Color & fore_color () const
    {
        return m_fore_color;
    }

    const Color & label_color () const
    {
        return m_label_color;
    }

    const Color & sel_color () const
    {
        return m_sel_color;
    }

    const Color & drum_color () const
    {
        return m_drum_color;
    }

    const Color & progress_color () const
    {
        return m_progress_color;
    }

    const Color & beat_color () const
    {
        return m_beat_color;
    }

    const Color & step_color () const
    {
        return m_step_color;
    }

    const Color & note_in_color () const
    {
        return m_note_in_color;
    }

    const Color & note_out_color () const
    {
        return m_note_out_color;
    }

    bool use_gradient () const
    {
        return m_use_gradient;      /* paint notes/triggers with gradient   */
    }

    const Color & tempo_color () const
    {
        return m_tempo_color;
    }

    const Color & grey_color () const
    {
        return m_grey_color;
    }

    const Brush & blank_brush () const
    {
        return m_blank_brush;
    }

    const Brush & note_brush () const
    {
        return m_note_brush;
    }

    const Brush & scale_brush () const
    {
        return m_scale_brush;
    }

    const Brush & backseq_brush () const
    {
        return m_backseq_brush;
    }

    const seq66::rect & old_rect () const
    {
        return m_old;
    }

    seq66::rect & old_rect ()
    {
        return m_old;
    }

    const seq66::rect & selection () const
    {
        return m_selected;
    }

    seq66::rect & selection ()
    {
        return m_selected;
    }

    int scale () const
    {
        return m_scale;
    }

    int scale_zoom () const
    {
        return m_scale_zoom;
    }

    /**
     *  Indicates if we're selecting, moving, growing, or pasting.
     *
     * \return
     *      Returns true if one of those four flags are set.
     */

    bool select_action () const
    {
        return selecting() || growing() || drop_action();
    }

    /**
     *  Indicates if we're drag-pasting, selecting, moving, growing, or
     *  pasting.
     *
     * \return
     *      Returns true if one of those five flags are set.
     */

    virtual bool normal_action () const
    {
        return select_action();
    }

    /**
     *  Indicates if we're moving or pasting.
     *
     * \return
     *      Returns true if one of those two flags are set.
     */

    virtual bool drop_action () const
    {
        return moving();
    }

    int snap () const
    {
        return m_snap;
    }

    /*
     * This value changes only when the PPQN changes.
     */

    int grid_snap () const
    {
        return m_grid_snap;
    }

    midipulse beat_length () const
    {
        return m_beat_length;
    }

    midipulse measure_length () const
    {
        return m_measure_length;
    }

    bool selecting () const
    {
        return m_selecting;
    }

    bool adding () const
    {
        return m_adding;
    }

    bool moving () const
    {
        return m_moving;
    }

    bool moving_init () const
    {
        return m_moving_init;
    }

    bool growing () const
    {
        return m_growing;
    }

    bool painting () const
    {
        return m_painting;
    }

    bool paste () const
    {
        return m_paste;
    }

    int drop_x () const
    {
        return m_drop_x;
    }

    int drop_y () const
    {
        return m_drop_y;
    }

    void snap_drop_x ()
    {
        snap_x(m_drop_x);
    }

    void snap_drop_y ()
    {
        snap_y(m_drop_y);
    }

    int current_x () const
    {
        return m_current_x;
    }

    int current_y () const
    {
        return m_current_y;
    }

    int progress_x () const
    {
        return m_progress_x;
    }

    int old_progress_x () const
    {
        return m_old_progress_x;
    }

    int scroll_page () const
    {
        return m_scroll_page;
    }

    bool progress_follow () const
    {
        return m_progress_follow;
    }

    virtual int scroll_offset () const
    {
        return m_scroll_offset;
    }

    int scroll_offset_v () const
    {
        return m_scroll_offset_v;
    }

    int scroll_offset_x () const
    {
        return m_scroll_offset_x;
    }

    int scroll_offset_y () const
    {
        return m_scroll_offset_y;
    }

    int unit_height () const
    {
        return m_unit_height;
    }

    int total_height () const
    {
        return m_total_height;
    }

public:

    virtual bool change_ppqn (int ppqn) override;
    virtual bool zoom_in () override;
    virtual bool zoom_out () override;
    virtual bool set_zoom (int z) override;
    virtual bool reset_zoom () override;
    virtual bool check_dirty () const override;

    void set_snap (midipulse snap)
    {
        m_snap = int(snap);
    }

    void set_grid_snap (midipulse snap)
    {
        m_grid_snap = int(snap);
    }

protected:

    virtual int horizSizeHint () const;

    void old_rect (seq66::rect & r)
    {
        m_old = r;
    }

    void selection (seq66::rect & r)
    {
        m_selected = r;
    }

    /**
     *  Clears all the mouse-action flags.
     */

    void clear_action_flags ()
    {
        m_selecting = m_moving = m_growing = m_paste = m_moving_init =
             m_painting = false;
    }

    void selecting (bool v)
    {
        m_selecting = v;
    }

    void adding (bool v)
    {
        m_adding = v;
    }

    void moving (bool v)
    {
        m_moving = v;
    }

    void moving_init (bool v)
    {
        m_moving_init = v;
    }

    void growing (bool v)
    {
        m_growing = v;
    }

    void painting (bool v)
    {
        m_painting = v;
    }

    void paste (bool v)
    {
        m_paste = v;
    }

    void drop_x (int v)
    {
        m_drop_x = v;
    }

    void drop_y (int v)
    {
        m_drop_y = v;
    }

    void current_x (int v)
    {
        m_current_x = v;
    }

    void current_y (int v)
    {
        m_current_y = v;
    }

    void progress_x (int v)
    {
        m_progress_x = v;
    }

    void old_progress_x (int v)
    {
        m_old_progress_x = v;
    }

    void scroll_page (int v)
    {
        m_scroll_page = v;
    }

    void progress_follow (bool v)
    {
        m_progress_follow = v;
    }

    virtual void scroll_offset (int v)
    {
        m_scroll_offset = v;
    }

    void scroll_offset_v (int v)
    {
        m_scroll_offset_v = v;
    }

    void scroll_offset_x (int v)
    {
        m_scroll_offset_x = v;
    }

    void scroll_offset_y (int v)
    {
        m_scroll_offset_y = v;
    }

    void unit_height (int v)
    {
        m_unit_height = v;
    }

    void total_height (int v)
    {
        m_total_height = v;
    }

protected:

    void snap_x (int & x);
    bool snap_current_x ();

    void snap_y (int & y)
    {
        y -= y % m_unit_height;             /* not c_names_y    */
    }

    bool snap_current_y ();

    void swap_x ()
    {
        int temp = m_current_x;
        m_current_x = m_drop_x;
        m_drop_x = temp;
    }

    void swap_y ()
    {
        int temp = m_current_y;
        m_current_y = m_drop_y;
        m_drop_y = temp;
    }

    /*
     * Takes screen coordinates, give us notes/keys (to be generalized to
     * other vertical user-interface quantities) and ticks (always the
     * horizontal user-interface quantity).  Compare this function to
     * qbase::pix_to_tix().
     */

    virtual midipulse pix_to_tix (int x) const override
    {
        midipulse result = x * pulses_per_pixel(perf().ppqn(), m_scale_zoom);
        if (m_zoom_expansion > 1)
            result /= m_zoom_expansion;

        return result;
    }

    virtual int tix_to_pix (midipulse ticks) const override
    {
        int result = ticks / pulses_per_pixel(perf().ppqn(), m_scale_zoom);
        if (m_zoom_expansion > 1)
            result *= m_zoom_expansion;

        return result;
    }

#if ! defined SEQ66_USE_ZOOM_EXPANSION

    /*
     * qseqtime: int right = position_pixel(righttick)
     *
     * m_scroll_offset is an int!
     */

    int position_pixel (midipulse tix)
    {
        return m_scroll_offset_x + tix_to_pix(tix - m_scroll_offset);
    }

#endif

    /*
     * qseqroll: int x_offset = xoffset(tick) - scroll_offset_x()
     */

    int xoffset (midipulse tick) const
    {
        return tix_to_pix(tick) + m_padding_x;
    }

#if 0
    midipulse position_tick (int pix)
    {
        return m_scroll_offset + pix_to_tix(pix - m_scroll_offset_x);
    }
#endif

    void convert_x (int x, midipulse & tick);
    void convert_xy (int x, int y, midipulse & ticks, int & seq);
    void convert_ts (midipulse ticks, int seq, int & x, int & y);
    void convert_ts_box_to_rect
    (
        midipulse tick_s, midipulse tick_f, int seq_h, int seq_l,
        seq66::rect & r
    );

    /**
     *  Meant to be overridden by derived classes to change a user-interface
     *  item, such as the mouse pointer, when entering an adding mode.
     *
     * \param a
     *      The value of the status of adding (e.g. a note).
     */

    virtual void set_adding (bool a)
    {
        adding(a);
    }

    void start_paste();

};          // class qeditbase

/*
 *  Free functions.
 */

extern int zoom_power_of_2 (int ppqn);

}           // namespace seq66

#endif      // SEQ66_QEDITBASE_HPP

/*
 * qeditbase.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


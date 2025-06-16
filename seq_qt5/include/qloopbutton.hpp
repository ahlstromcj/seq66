#if ! defined SEQ66_QLOOPBUTTON_HPP
#define SEQ66_QLOOPBUTTON_HPP

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
 * \file          qloopbutton.hpp
 *
 *  This module declares/defines the base class for drawing on a pattern-slot
 *  button.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-06-28
 * \updates       2025-04-27
 * \license       GNU GPLv2 or above
 *
 */

#include <QFont>

#include "qslotbutton.hpp"              /* seq66::qslotbutton base class    */

namespace seq66
{
    class performer;

/**
 *  This class provides for drawing text and a progress bar on a qslotbutton.
 */

class qloopbutton final : public qslotbutton
{

public:

    class textbox
    {
        friend class qloopbutton;

    private:

        int m_x, m_y, m_w, m_h, m_flags;
        std::string m_label;

    public:

        textbox ();
        void set
        (
            int x, int y, int w, int h, int flags, std::string label
        );

    };          // nested class textbox

    class progbox
    {
        friend class qloopbutton;

    private:

        int m_x;        /**< x coordinate in the button as per the width.   */
        int m_y;        /**< y coordinate in the button as per the height.  */
        int m_w;        /**< The actual width based on button and x value.  */
        int m_h;        /**< Actual heighth based on button and y value.    */
        int m_center_x; /**< The center of the progress box width.          */
        int m_center_y; /**< The center of the progress box height.         */

    public:

        progbox ();
        progbox (const progbox &) = default;
        progbox & operator = (const progbox &) = default;
        void set (int w, int h);

        int x () const
        {
            return m_x;
        }

        int y () const
        {
            return m_y;
        }

        int w () const
        {
            return m_w;
        }

        int h () const
        {
            return m_h;
        }

        int center_x () const
        {
            return m_center_x;
        }

        int center_y () const
        {
            return m_center_y;
        }

    };          // nested class progbox

private:

    /**
     *  Allows for tailorable progress-box sizes as a percentage of the button
     *  size.
     *
     *      Horizontal: 0.50 to 0.80
     *      Vertical:   0.10 to 0.40 with 0 to turn off drawing it.
     */

    static bool sm_draw_progress_box;
    static double sm_progress_w_fraction;
    static double sm_progress_h_fraction;
    static const int scm_progress_event_margin;
    static const int sm_vert_draw_text_threshold;
    static const int sm_vert_compressed_threshold;
    static const int sm_horiz_compressed_threshold;
    static const int sm_base_height;
    static const float sm_left_width_factor;
    static const float sm_right_width_factor;

    /**
     *  Provides a buffer that represents a condensed version of long
     *  patterns, so that we don't have to waste time drawing hundreds of
     *  events in the tiny box centered in the pattern button.
     */

    bool m_show_average;
    bool m_fingerprint_inited;
    bool m_fingerprinted;
    size_t m_fingerprint_size;
    std::vector<midishort> m_fingerprint;
    std::vector<midishort> m_fingerprint_count;

    /**
     *  Optional scaling for the notes in the progress box, to give a more
     *  realistic depiction of the pitches.
     */

    int m_note_min;
    int m_note_max;

    /**
     *  Provides a pointer to the sequence displayed by this button.  Note that
     *  we do not want to use a shared pointer.  First, semantically this button
     *  does not own the sequence, and second, there seems to be a race
     *  condition (valgrind.log) that causes this pointer to be deleted
     *  completely before performer can reset the sequence.
     *
     *      seq::pointer m_seq;
     *      sequence * m_seq;
     */

    seq::pointer m_seq;

    /**
     *  Checked status.
     */

    bool m_is_checked;

    /**
     *  Holds the value of usr().progress_box_show_cc().
     */

    bool m_show_cc;

    /**
     *  The thickness of the vertical progress bar, either 1 or 2.
     */

    int m_prog_thickness;

    /**
     *  Provides the background color of the progress bar.
     */

    Color m_prog_back_color;

    /**
     *  Provides the foreground color of the progress bar.
     */

    Color m_prog_fore_color;

    /**
     *  The font for drawing text.
     */

    QFont m_text_font;

    /**
     *  Text and progress-box support members.
     */

    bool m_text_initialized;
    bool m_draw_text;
    textbox m_top_left;
    textbox m_top_right;
    textbox m_bottom_left;
    textbox m_bottom_right;
    progbox m_progress_box;
    progbox m_event_box;
    bool m_use_gradient;

public:

    qloopbutton
    (
        const qslivegrid * const slotparent,
        seq::number slotnumber,
        const std::string & label,
        const std::string & hotkey,
        seq::pointer seqp,
        QWidget * parent = nullptr
    );

    virtual ~qloopbutton ()
    {
        // no code needed
    }

    bool use_gradient () const
    {
        return m_use_gradient;
    }

    static bool boxes_initialized (bool reset = false);
    static void progress_box_size (double w, double h);

    virtual seq::pointer loop () override
    {
        return m_seq;
    }

    virtual void setup () override;
    virtual void reupdate (bool all = true) override;
    virtual void set_checked (bool flag) override;
    virtual bool toggle_enabled () override;
    virtual bool toggle_checked () override;

protected:

    void draw_progress (QPainter & p, midipulse tick, bool tiny = false);
    void draw_progress_box (QPainter & painter);
    void draw_pattern (QPainter & painter);
    void initialize_fingerprint ();

private:

    /*
     *  Painting event to draw on the button surface.  Called automatically when
     *  the layout-grid is drawn.
     */

    virtual void paintEvent (QPaintEvent *) override;
    virtual void focusInEvent (QFocusEvent *) override;
    virtual void focusOutEvent (QFocusEvent *) override;
    virtual void resizeEvent (QResizeEvent *) override;

private:

    bool initialize_text ();

};          // class qloopbutton

}           // namespace seq66

#endif      // SEQ66_QLOOPBUTTON_HPP

/*
 * qloopbutton.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


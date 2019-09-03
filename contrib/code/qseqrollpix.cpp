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
 * \file          qseqrollpix.cpp
 *
 *  This module declares/defines the base class for drawing on the piano
 *  roll of the patterns editor for the Qt 5 implementation.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-06-04
 * \updates       2019-07-24
 * \license       GNU GPLv2 or above
 *
 *  An alternative implementation to qseqroll.  That class redraws the whole
 *  darn piano roll and events every time, and currently seems to eat up at
 *  least 15% CPU.  This implementation makes a new background pixmap, and seems
 *  to be a lot more efficient.  Still working on it, though.
 *
 * Strategy:
 *
 *  Create a whole set of contiguous frames, each the height of the keyboard
 *  area and the width of the current window frame. (We could extend this
 *  concept to height as well, two dimensions, if more speed [and complexity] is
 *  needed.)
 *
 *  We also make each frame slightly narrower than the window width, to support
 *  follow-progress more easily, by shifting the view by that amount.
 *
 *  Each frame would be numbered.  The number would determine its t0 and t1.
 *
 *  <----------- m_frame_size -------------->
 *  t0            Frame 0                   t1  |
 *  |                                       |   |                       Frame
 *  :       .       .       .       :       .   |   .       .       :   Bars
 *  |                                           |                       Window
 *   -------------------------------------------
 *
 *  <----------- m_frame_size -------------->
 *  t1            Frame 1                   t2  |
 *  |                                       |   |                       Frame
 *  .       .       .       :       .       .   |   .       :           Bars
 *  |                                           |                       Window
 *   -------------------------------------------
 *
 *  The smallest width allowed (the startup width) is a little over 2 measures.
 *  The largest width on a 1920 screen is about 4.75 measures.  A long song is
 *  about 100 measures.  So we could end up with at least 20 to 50 frames.
 *
 *  -#  Draw the background grid into a pixmap.  At every repaint, redraw the
 *      pixmap (it is fairly fast).
 *      -#  Then redraw the events.  This will automatically caused new events
 *          to show up, and deleted events to be removed.
 *      -#  Restore the previous area of the play head (progress bar), save the
 *          next area of the play head, and draw the new play head over it.
 *  -#  In addition to the grid, draw the existing events.  For new events,
 *      either add them (if feasible), or recreate the whole pixmaps, at the
 *      cost of stuttering.
 *  -#  Draw (or redraw) the events into a separate pixmap.  Then somehow
 *      overlay the events pixmap onto the background grid pixmap.
 */

#include <QApplication>                 /* QApplication keyboardModifiers() */
#include <QFrame>                       /* base class for seqedit frame(s)  */
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QScrollBar>                   /* needed by qscrollmaster          */

#include <cmath>                        /* std::trunc()                     */

#include "cfg/settings.hpp"             /* seq66::usr().key_height(), etc.  */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "qseqeditframe.hpp"            /* seq66::qseqeditframe legacy      */
#include "qseqeditframe64.hpp"          /* seq66::qseqeditframe64 class     */
#include "qseqframe.hpp"                /* interface class for seqedits     */
#include "qseqrollpix.hpp"              /* seq66::qseqrollpix class         */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Creates a small management object for a pixmap.  The caller creates the
 *  provided pixmap... too many parameters to pass along.
 *
 *  All parameters must be vetted by the caller.
 */

rollframe::rollframe
(
    const QPixmap & pixptr,
    int fnumber,
    midipulse t0,
    midipulse t1
) :
    m_grid_pixmap           (pixptr.copy()),
    m_frame_number          (fnumber),
    m_t0                    (t0),
    m_t1                    (t1)
{
    // no code
}

/**
 *
 */

rollframe::rollframe (const rollframe & rhs) :
    m_grid_pixmap           (rhs.m_grid_pixmap.copy()),
    m_frame_number          (rhs.m_frame_number),
    m_t0                    (rhs.m_t0),
    m_t1                    (rhs.m_t1)
{
    // todo?
}

/**
 *  Writes (dumps) the image to a file, useful for trouble-shooting.
 */

void
rollframe::dump () const
{
    char tmp[16];
    snprintf(tmp, sizeof tmp, "dump%02d.png", m_frame_number);
    m_grid_pixmap.save(tmp);
}

/**
 *
 */

qseqrollpix::qseqrollpix
(
    performer & p,
    seq::pointer seqp,
    qseqkeys * seqkeys_wid,
    int zoom,
    int snap,
    sequence::editmode mode,
    qseqframe * frame
) :
    QWidget                 (frame),
    qseqbase
    (
        p, seqp, zoom, snap,
        usr().key_height(),                         // m_key_y
        (usr().key_height() * c_num_keys + 1)       // m_keyarea_y
    ),
    m_parent_frame          (frame),
    m_is_new_edit_frame
    (
        not_nullptr(dynamic_cast<qseqeditframe64 *>(m_parent_frame))
    ),
    m_seqkeys_wid           (seqkeys_wid),
    m_grid_pixmap           (),
    m_pixmap_list           (),                 /* std::vector<rollframe>   */
    m_pixmap_count          (0),
    m_pixmap_width          (0),
    m_pixmap_height         (0),
    m_pixmaps_ready         (false),
    m_grid_canvas           (),
    m_timer                 (nullptr),
    m_scale                 (0),
    m_pos                   (0),
    m_chord                 (0),
    m_key                   (0),
    m_note_length           (p.ppqn() * 4 / 16),
    m_background_sequence   (0),
    m_drawing_background_seq (false),
    m_status                (0),
    m_cc                    (0),
    m_edit_mode             (mode),
    m_note_x                (0),
    m_note_width            (0),
    m_note_y                (0),
    m_note_height           (0),
    m_key_y                 (usr().key_height()),
    m_keyarea_y             (m_key_y * c_num_keys + 1),
    m_keypadding_x          (c_keyboard_padding_x),
    m_current_width         (0),
    m_current_height        (0),
    m_t0                    (0),
    m_t1                    (0),
    m_frame_ticks           (0),
    m_current_frame         (-1),
    cm_base_step_ticks      (6),
    m_ticks_per_step        (0),
    m_ticks_per_beat        (0),
    m_ticks_per_bar         (0)
{
    /*
     * Avoid intensively annoying repaints... thanks Qtractor!
     *
     * TODO:  look at qtractorMidiThumbView::paintEvent(),
     *        qtractorMidiThumbView::updatePlayHead().
     */

    // setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_StaticContents);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    set_snap(seqp->get_snap_tick());
    show();
    m_timer = new QTimer(this);                          // redraw timer !!!
    m_timer->setInterval(usr().window_redraw_rate());    // 20
    QObject::connect
    (
        m_timer, SIGNAL(timeout()), this, SLOT(conditional_update())
    );
    m_timer->start();
}

/**
 *  Creates a pixmap and paints only the grid (background).
 *
 *  There is one big issue, however.  Due to a limitation (short int), the
 *  largest pixmap can only be 32767 pixes wide.  This means that only about 86
 *  measures can typically be shown!
 *
 * \param ww
 *      Provides the width of pixmap.  However, if greater than 32767 only part
 *      of the grid can be drawn!
 *
 * \param wh
 *      Provides the height of the pixmap.  The limit of 32767 will never be
 *      hit.
 *
 * \return
 *      Returns true if the pixmap (m_grid_pixmap) was created.
 */

QPixmap *
qseqrollpix::create_pixmap (int ww, int wh)
{
    QPixmap * result = new QPixmap(ww, wh);
    if (not_nullptr(result))
    {
        result->fill();                     /* fills a white background     */

        QPainter painter(result);           /* draw grid on the pixmap      */
        QBrush brush(Qt::NoBrush);
        bool fruity_lines = true;
        QPen pen(Qt::lightGray);
        pen.setStyle(Qt::SolidLine);
        painter.setPen(pen);
        painter.setBrush(brush);
        pen.setColor(Qt::lightGray);
        pen.setStyle(Qt::SolidLine);
        painter.setPen(pen);
        m_edit_mode = perf().edit_mode(seq_pointer()->seq_number());

        /*
         * Horizontal lines. The modkey value is the remaining-keys value
         * adjusted for the octave and scroll offset.
         */

        int octkey = SEQ66_OCTAVE_SIZE - m_key;
        for (int key = 1; key <= c_num_keys; ++key) /* for each note row    */
        {
            int modkey = c_num_keys - key - scroll_offset_key() + octkey;

            /*
             * Set line colour dependent on the note row we're on.
             */

            if (fruity_lines)
            {
                if ((modkey % SEQ66_OCTAVE_SIZE) == 0)
                {
                    pen.setColor(Qt::darkGray);
                    pen.setStyle(Qt::SolidLine);
                    painter.setPen(pen);
                }
                else if ((modkey % SEQ66_OCTAVE_SIZE) == (SEQ66_OCTAVE_SIZE-1))
                {
                    pen.setColor(Qt::lightGray);
                    pen.setStyle(Qt::SolidLine);
                    painter.setPen(pen);
                }
            }

            /*
             * Draw horizontal grid lines differently depending on editing mode.
             */

            int y = key * m_key_y;
            if (m_edit_mode == sequence::editmode::drum)
                y -= (0.5 * m_key_y);

            painter.drawLine(0, y, ww, y);
            if (m_scale != static_cast<int>(scales::off))
            {
                if (! c_scales_policy[m_scale][(modkey - 1) % SEQ66_OCTAVE_SIZE])
                {
                    pen.setColor(Qt::lightGray);
                    brush.setColor(Qt::lightGray);
                    brush.setStyle(Qt::SolidPattern);
                    painter.setBrush(brush);
                    painter.setPen(pen);
                    painter.drawRect(0, y + 1, ww, m_key_y - 1);
                }
            }
        }

        pen.setColor(Qt::darkGray);                 // can we use Palette?
        painter.setPen(pen);

        /*
         * Draw vertical grid lines.  Incrementing by m_ticks_per_step only
         * works for PPQN of certain multiples.  Note that, for now all frames
         * are identical.  And they all have to start at "0".
         *
         * int t0 = frame * m_frame_ticks;
         */

        int t0 = 0;
        int t1 = t0 + m_frame_ticks;
        for (int tick = t0; tick < t1; tick += m_ticks_per_step)
        {
            int x_offset = xoffset(tick) - scroll_offset_x();
            pen.setWidth(1);
            if (tick % m_ticks_per_bar == 0)    /* solid line on every beat */
            {
                pen.setColor(Qt::black);
                pen.setStyle(Qt::SolidLine);
                pen.setWidth(2);                /* two pixels               */
            }
            else if (tick % m_ticks_per_beat == 0)
            {
                pen.setColor(Qt::darkGray);     // can we use Palette?
                pen.setStyle(Qt::SolidLine);    // pen.setColor(Qt::DashLine)
            }
            else
            {
                pen.setColor(Qt::lightGray);    // faint step lines
                pen.setStyle(Qt::DotLine);
                int tick_snap = tick - (tick % snap());
                if (tick == tick_snap)
                {
                    pen.setStyle(Qt::SolidLine);
                    pen.setColor(Qt::lightGray);    // faint step lines
                }
                else
                {
                    pen.setStyle(Qt::DotLine);      // Gdk::LINE_ON_OFF_DASH
                    pen.setColor(Qt::lightGray);    // faint step lines
                }
            }
            painter.setPen(pen);
            painter.drawLine(x_offset, 0, x_offset, m_keyarea_y);
        }
    }
    return result;
}

/**
 *  Creates all the pixmaps.
 */

bool
qseqrollpix::create_pixmaps ()
{
    bool result = m_pixmap_count > 0;
    if (result)
    {
        m_pixmap_list.clear();                      /* remove old pixmaps   */
        for (int f = 0; f < m_pixmap_count; ++f)
        {
            QPixmap * temp = create_pixmap(m_pixmap_width, m_pixmap_height);
            if (not_nullptr(temp))
            {
                midipulse t0 = f * m_frame_ticks;
                midipulse t1 = t0 + m_frame_ticks;
                rollframe roll(*temp, f, t0, t1);
                m_pixmap_list.push_back(roll);

                /*
                 * Uncomment to create one or more pixmap files for
                 * trouble-shooting.
                 *
                 * roll.dump();
                 */

                delete temp;
            }
            else
            {
                result = false;
                break;
            }
        }
    }
    return result;
}

/**
 *  Zooms in, first calling the base-class version of this function, then
 *  passing along the message to the parent edit frame, so that it can change
 *  the zoom on the other panels of the parent edit frame.
 */

bool
qseqrollpix::zoom_in ()
{
    bool result = qseqbase::zoom_in();
    if (result)
        result = m_parent_frame->set_zoom(zoom());

    return result;
}

/**
 *  Zooms out, first calling the base-class version of this function, then
 *  passing along the message to the parent edit frame, so that it can change
 *  the zoom on the other panels of the parent edit frame.
 */

bool
qseqrollpix::zoom_out ()
{
    bool result = qseqbase::zoom_out();
    if (result)
        result = m_parent_frame->set_zoom(zoom());

    return result;
}

/**
 *  Tells the parent frame to reset our zoom.
 */

bool
qseqrollpix::reset_zoom ()
{
    return m_parent_frame->reset_zoom();
}

/**
 *  This function sets the given sequence onto the piano roll of the pattern
 *  editor, so that the musician can have another pattern to play against.
 *  The state parameter sets the boolean m_drawing_background_seq.
 *
 * \param state
 *      If true, the background sequence will be drawn.
 *
 * \param seq
 *      Provides the sequence number, which is checked against
 *      sequence::legal() before being used.  This macro allows
 *      the value SEQ66_SEQUENCE_LIMIT (2048), which disables the background
 *      sequence.
 */

void
qseqrollpix::set_background_sequence (bool state, int seq)
{
    m_drawing_background_seq = state;
    if (sequence::legal(seq))
        m_background_sequence = seq;

    set_dirty();
}

/**
 *  In an effort to reduce CPU usage when simply idling, this function calls
 *  update() only if necessary.  See qseqbase::needs_update().
 */

void
qseqrollpix::conditional_update ()
{
    if (needs_update())
    {
        if (progress_follow())
            follow_progress();              /* keep up with progress    */

        update();
    }
}

/**
 *  Draws the piano roll.
 *
 *  In later usage, the width() function [and height() as well?], returns a
 *  humongous value (38800+).  So we store the current values to use, via
 *  window_width() and window_height(), in follow_progress().
 *
 *  https://stackoverflow.com/questions/35877437/
 *      qt-why-does-my-paintevent-erase-everything-before-doing-its-job
 *
 *  We note a few facts:
 *
 *      -#  The update() function causes the whole painted area to be blank, so
 *          redrawing must be done all over again.
 *      -#  Rendering a QPixmap is faster than rendering a QImage.
 *      -#  A QPixmap cannot have a width or height exceeding 32767 (short int).
 *
 * draw_progress_on_window():
 *
 *  Draw a progress line on the window.  This is done by first blanking out
 *  the line with the background, which contains white space and grey lines,
 *  using the the draw_drawable function.  Remember that we wrap the
 *  draw_drawable() function so it's parameters are xsrc, ysrc, xdest, ydest,
 *  width, and height.
 *
 *  The progress-bar position is based on the sequence :: get_last_tick()
 *  value, the current zoom, and the current scroll-offset x value.
 */

void
qseqrollpix::paintEvent (QPaintEvent *)
{
    int wh = height();
    QPainter painter(this);
    QBrush brush(Qt::NoBrush);
    QPen pen(Qt::lightGray);
    pen.setStyle(Qt::SolidLine);
    painter.setPen(pen);
    painter.setBrush(brush);
    m_edit_mode = perf().edit_mode(seq_pointer()->seq_number());

    int len = analyze_width();              /* side-effects     */
    if (len > 0)
    {
        m_pixmap_height = m_current_height = m_keyarea_y + 1;
        m_current_width = len;
        if (! m_pixmaps_ready)
        {
            m_pixmaps_ready = create_pixmaps();
#define INITIAL_DRAWING_DOES_NOT_GET_ERASED
#ifdef INITIAL_DRAWING_DOES_NOT_GET_ERASED
            if (m_pixmaps_ready)
            {
                int count = int(m_pixmap_list.size());
                for (int frame = 0; frame < count; ++frame)
                {
                    const rollframe & roll = m_pixmap_list[frame];
                    QPixmap pixmap = roll.get();

                    int px = tix_to_pix(roll.t0());
                    painter.drawPixmap(px, 0, pixmap);
                }
            }
#endif
        }
    }
    if (! m_pixmaps_ready)
        return;

    /*
     * Draw the current pixmap frame.  Note that, if the width and height
     * change, we will have to reevaluate.
     * We only need to draw the current pixmap.
     */

    midipulse current_tick = seq_pointer()->get_last_tick();
    int frame = current_tick / m_frame_ticks;
    const rollframe & roll = m_pixmap_list[frame];
    QPixmap pixmap = roll.get();                /* copies pixmap */

    int px = tix_to_pix(roll.t0());
    painter.drawPixmap(px, 0, pixmap);

    /*
     * Draw the events. This currently draws all of them.  Drawing all them only
     * needs to be drawn once.
     */

    bool redraw = frame != m_current_frame;
    if (redraw)
    {
        int px = tix_to_pix(roll.t0());
        painter.drawPixmap(px, 0, pixmap);
        m_current_frame = frame;
        if (m_drawing_background_seq)
        {
            if (m_edit_mode == sequence::editmode::drum)
                draw_drum_notes(painter, roll, true);
            else
                draw_notes(painter, roll, true);
        }

        if (m_edit_mode == sequence::editmode::drum)
            draw_drum_notes(painter, roll, false);
        else
            draw_notes(painter, roll, false);
    }

    /*
     * draw_progress_on_window():
     */

    int prog_x = old_progress_x();
    pen.setColor(Qt::red);                      // draw the playhead
    pen.setStyle(Qt::SolidLine);
    if (usr().progress_bar_thick())
        pen.setWidth(2);
    else
        pen.setWidth(1);

    painter.setPen(pen);
    painter.drawLine(prog_x, 0, prog_x, wh * 8);    // why * 8?
    old_progress_x(xoffset(seq_pointer()->get_last_tick()));

    /*
     * draw_progress_on_window() end.
     */

    /*
     * Draw selection box:
     */

    int x, y, w, h;                     /* draw selections              */
    brush.setStyle(Qt::NoBrush);        /* painter reset                */
    painter.setBrush(brush);
    if (select_action())                /* select/move/paste/grow       */
        pen.setStyle(Qt::SolidLine);

    if (selecting())
    {
        rect::xy_to_rect_get
        (
            drop_x(), drop_y(), current_x(), current_y(), x, y, w, h
        );

        old_rect().set(x, y, w, h + m_key_y);
        pen.setColor("orange");         /*  pen.setColor(Qt::black);    */
        painter.setPen(pen);

        /*
         * painter.drawRect(x + m_keypadding_x, y, w, h + m_key_y);
         */

        painter.drawRect(x, y, w, h);
    }

    if (drop_action())
    {
        int delta_x = current_x() - drop_x();
        int delta_y = current_y() - drop_y();
        x = selection().x() + delta_x;
        y = selection().y() + delta_y;
        pen.setColor(Qt::black);
        painter.setPen(pen);
        switch (m_edit_mode)
        {
        case sequence::editmode::note:

            painter.drawRect
            (
                x + m_keypadding_x, y,
                selection().width(), selection().height()
            );
            break;

        case sequence::editmode::drum:

            painter.drawRect
            (
                x - m_note_height * 0.5 + m_keypadding_x,
                y, selection().width() + m_note_height, selection().height()
            );
            break;
        }
        old_rect().x(x);
        old_rect().y(y);
        old_rect().width(selection().width());
        old_rect().height(selection().height());
    }

    if (growing())
    {
        int delta_x = current_x() - drop_x();
        int width = delta_x + selection().width();
        if (width < 1)
            width = 1;

        x = selection().x();
        y = selection().y();

        pen.setColor(Qt::black);
        painter.setPen(pen);
        painter.drawRect(x + m_keypadding_x, y, width, selection().height());
        old_rect().x(x);
        old_rect().y(y);
        old_rect().width(width);
        old_rect().height(selection().height());
    }

    /*
     * Draw selection box end.
     */
}

/*
 * Why floating point; just divide by 2.  Also, the polygon seems to be offset
 * downard by half the note height.
 */

void
qseqrollpix::draw_drum_note (QPainter & painter)
{
    m_note_height = m_key_y;

    int h2 = m_note_height / 2;
    int x0 = m_note_x - h2;
    int x1 = m_note_x + h2;
    int y1 = m_note_y + h2;
    QPointF points[4] =
    {
        QPointF(x0, y1),
        QPointF(m_note_x, m_note_y),
        QPointF(x1, y1),
        QPointF(m_note_x, m_note_y + m_note_height)
    };
    painter.drawPolygon(points, 4);

    /*
     * Draw note highlight.

    if (ni.selected())
        brush.setColor("orange");       // Qt::red
    else if (m_edit_mode == sequence::editmode::drum)
        brush.setColor(Qt::red);
     */
}

/**
 *
 */

void
qseqrollpix::mousePressEvent (QMouseEvent * event)
{
    midipulse tick_s, tick_f;
    int note, note_l, norm_x, norm_y, snapped_x, snapped_y;
    snapped_x = norm_x = event->x() - m_keypadding_x;
    snapped_y = norm_y = event->y();
    snap_x(snapped_x);
    snap_y(snapped_y);
    current_y(snapped_y);
    drop_y(snapped_y);                  /* y is always snapped */
    if (paste())
    {
        convert_xy(snapped_x, snapped_y, tick_s, note);
        paste(false);
        seq_pointer()->push_undo();
        seq_pointer()->paste_selected(tick_s, note);
        set_dirty();
    }
    else
    {
        if (event->button() == Qt::LeftButton)
        {
            current_x(norm_x);
            drop_x(norm_x);             // for selection, use non-snapped x
            switch (m_edit_mode)        // convert screen coords to ticks
            {
            case sequence::editmode::note:

                convert_xy(drop_x(), drop_y(), tick_s, note);
                tick_f = tick_s;
                break;

            case sequence::editmode::drum:    // padding for selecting drum hits

                convert_xy(drop_x() - m_note_height * 0.5, drop_y(), tick_s, note);
                convert_xy(drop_x() + m_note_height * 0.5, drop_y(), tick_f, note);
                break;
            }
            if (adding())               // painting new notes
            {
                painting(true);         /* start paint job   */
                current_x(snapped_x);
                drop_x(snapped_x);      /* adding, snapped x */
                convert_xy(drop_x(), drop_y(), tick_s, note);

                /*
                 * Test if a note is already there, fake select, if so, don't
                 * add, else add a note, length = little less than snap.
                 */

                if
                (
                    ! seq_pointer()->select_note_events
                    (
                        tick_s, note, tick_s, note,
                        sequence::select::would_select
                    )
                )
                {
                    seq_pointer()->push_undo();
                    seq_pointer()->add_note(tick_s, m_note_length - 2, note, true);
                    set_dirty();
                }
            }
            else                            /* we're selecting anew         */
            {
                bool isSelected = false;
                switch (m_edit_mode)
                {
                case sequence::editmode::note:

                    isSelected = seq_pointer()->select_note_events
                    (
                        tick_s, note, tick_f, note,
                        sequence::select::selected
                    );
                    break;

                case sequence::editmode::drum:

                    isSelected = seq_pointer()->select_note_events
                    (
                        tick_s, note, tick_f, note,
                        sequence::select::is_onset
                    );
                    break;
                }
                if (! isSelected)
                {
                    int numsel = 0;
                    if (! (event->modifiers() & Qt::ControlModifier))
                        seq_pointer()->unselect();

                    switch (m_edit_mode)    /* direct click; select 1 event */
                    {
                    case sequence::editmode::note:

                        numsel = seq_pointer()->select_note_events
                        (
                            tick_s, note, tick_f, note,
                            sequence::select::select_one
                        );
                        break;

                    case sequence::editmode::drum:

                        numsel = seq_pointer()->select_note_events
                        (
                            tick_s, note, tick_f, note,
                            sequence::select::select_one
                        );
                        break;
                    }
                    if (numsel == 0)    /* none selected, start selection box */
                    {
                        if (event->button() == Qt::LeftButton)
                            selecting(true);
                    }
                    else
                    {
                        set_dirty();
                    }
                }
                isSelected = false;
                switch (m_edit_mode)
                {
                case sequence::editmode::note:

                    isSelected = seq_pointer()->select_note_events
                    (
                        tick_s, note, tick_f, note,
                        sequence::select::selected
                    );
                    break;

                case sequence::editmode::drum:

                    isSelected = seq_pointer()->select_note_events
                    (
                        tick_s, note, tick_f, note, sequence::select::is_onset
                    );
                    break;
                }

                if (isSelected)
                {
                    if                          /* moving - left click only */
                    (
                        event->button() == Qt::LeftButton &&
                        ! (event->modifiers() & Qt::ControlModifier)
                    )
                    {
                        moving_init(true);
                        set_dirty();
                        switch (m_edit_mode)
                        {
                        case sequence::editmode::note:

                            seq_pointer()->get_selected_box  // use note length
                            (
                                tick_s, note, tick_f, note_l
                            );
                            break;

                        case sequence::editmode::drum:       // ignore it

                            seq_pointer()->get_onsets_selected_box
                            (
                                tick_s, note, tick_f, note_l
                            );
                            break;
                        }

                        convert_tn_box_to_rect
                        (
                            tick_s, tick_f, note, note_l, selection()
                        );

                        /* save offset that we get from the snap above */

                        int adjusted_selected_x = selection().x();
                        snap_x(adjusted_selected_x);
                        move_snap_offset_x(selection().x() - adjusted_selected_x);
                        current_x(snapped_x);
                        drop_x(snapped_x);
                    }
                    if          /* Middle mouse button or left-ctrl click   */
                    (
                        (
                            event->button() == Qt::MiddleButton ||
                            (
                                event->button() == Qt::LeftButton &&
                                (event->modifiers() & Qt::ControlModifier)
                            )
                        )
                        && m_edit_mode == sequence::editmode::note)
                    {
                        growing(true);
                        seq_pointer()->get_selected_box
                        (
                            tick_s, note, tick_f, note_l
                        );
                        convert_tn_box_to_rect
                        (
                            tick_s, tick_f, note, note_l, selection()
                        );
                    }
                }
            }
        }
        if (event->button() == Qt::RightButton)
            set_adding(true);
    }
    if (is_dirty())                                 /* something changed?   */
        seq_pointer()->set_dirty();
}

/**
 *
 */

void
qseqrollpix::mouseReleaseEvent (QMouseEvent * event)
{
    midipulse tick_s;                   // start of tick window
    midipulse tick_f;                   // end of tick window
    int note_h;                         // highest note in window
    int note_l;                         // lowest note in window
    int x, y, w, h;                     // window dimensions

    current_x(event->x() - m_keypadding_x);
    current_y(event->y());
    snap_current_y();
    if (moving())
        snap_current_x();

    int delta_x = current_x() - drop_x();
    int delta_y = current_y() - drop_y();
    midipulse delta_tick;
    int delta_note;
    if (event->button() == Qt::LeftButton)
    {
        if (selecting())
        {
            rect::xy_to_rect_get
            (
                drop_x(), drop_y(), current_x(), current_y(), x, y, w, h
            );
            switch (m_edit_mode)
            {
            case sequence::editmode::note:

                convert_xy(x, y, tick_s, note_h);
                convert_xy(x + w, y + h, tick_f, note_l);
                seq_pointer()->select_note_events
                (
                    tick_s, note_h, tick_f, note_l,
                    sequence::select::selecting
                );
                break;

            case sequence::editmode::drum:

                convert_xy(x, y, tick_s, note_h);
                convert_xy(x + w, y + h, tick_f, note_l);
                seq_pointer()->select_note_events
                (
                    tick_s, note_h, tick_f, note_l,
                    sequence::select::onset
                );
                break;
            }
            set_dirty();
        }

        if (moving())
        {
            delta_x -= move_snap_offset_x();            /* adjust for snap */

            /* convert deltas into screen corridinates */

            convert_xy(delta_x, delta_y, delta_tick, delta_note);

            /*
             * since delta_note was from delta_y, it will be filpped
             * ( delta_y[0] = note[127], etc.,so we have to adjust
             */

            delta_note = delta_note - (c_num_keys - 1);
            seq_pointer()->push_undo();
            seq_pointer()->move_selected_notes(delta_tick, delta_note);
            set_dirty();
        }
    }

    if (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton)
    {
        if (growing())
        {
            /* convert deltas into screen corridinates */

            convert_xy(delta_x, delta_y, delta_tick, delta_note);
            seq_pointer()->push_undo();
            if (event->modifiers() & Qt::ShiftModifier)
                seq_pointer()->stretch_selected(delta_tick);
            else
                seq_pointer()->grow_selected(delta_tick);

            set_dirty();
        }
    }

    if (event->button() == Qt::RightButton)
    {
        if (! QApplication::queryKeyboardModifiers().testFlag(Qt::MetaModifier))
        {
            set_adding(false);
            set_dirty();
        }
    }

    clear_action_flags();               /* turn off all the action flags    */
    seq_pointer()->unpaint_all();
    if (is_dirty())                     /* if clicked, something changed    */
        seq_pointer()->set_dirty();
}

/**
 *  Handles a mouse movement, including selection and note-painting.
 */

void
qseqrollpix::mouseMoveEvent (QMouseEvent * event)
{
    current_x(event->x() - m_keypadding_x);
    current_y(event->y());
    if (moving_init())
    {
        moving_init(false);
        moving(true);
    }
    snap_current_y();

    int note;
    midipulse tick;
    convert_xy(0, current_y(), tick, note);
    if (select_action())
    {
        if (drop_action())
            snap_current_x();
    }

    if (painting())
    {
        snap_current_x();
        convert_xy(current_x(), current_y(), tick, note);
        seq_pointer()->add_note(tick, m_note_length - 2, note, true);
    }
    set_dirty();
}

/**
 *  Handles keystrokes for note movement, zoom, and more.
 *
 *  We could simplify this a bit by creating a keystroke object.
 */

void
qseqrollpix::keyPressEvent (QKeyEvent * event)
{
    bool dirty = false;
//  keystroke kkey = qt_keystroke(event, SEQ66_KEYSTROKE_PRESS);
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
    {
        seq_pointer()->remove_selected();
        dirty = true;
    }
    else
    {
        // TODO: get these working and fix the 1:1 zoom in combo-dropdown.

        if (! perf().is_pattern_playing())
        {
            if (event->key() == Qt::Key_Home)
            {
                seq_pointer()->set_last_tick(0);
                dirty = true;
            }
            else if (event->key() == Qt::Key_Left)
            {
#if defined SEQ66_USE_KEPLER_TICK_SETTING

                /*
                 * Moved to Ctrl section below.
                 */

                seq_pointer()->set_last_tick
                (
                    seq_pointer()->get_last_tick() - snap()
                );
#else
                move_selected_notes(-1, 0);
#endif
                dirty = true;
            }
            else if (event->key() == Qt::Key_Right)
            {
#if defined SEQ66_USE_KEPLER_TICK_SETTING

                /*
                 * Moved to Ctrl section below.
                 */

                seq_pointer()->set_last_tick
                (
                    seq_pointer()->get_last_tick() + snap()
                );
#else
                move_selected_notes(1, 0);
#endif
                dirty = true;
            }
            else if (event->key() == Qt::Key_Down)
            {
                move_selected_notes(0, 1);
                dirty = true;
            }
            else if (event->key() == Qt::Key_Up)
            {
                move_selected_notes(0, -1);
                dirty = true;
            }
            else if (event->modifiers() & Qt::ControlModifier) // Ctrl + ...
            {
                /*
                 * We want to ignore Ctrl sequences here, so that Ctrl-Z's can
                 * be used for "undo".
                 */

                if (event->key() == Qt::Key_Left)
                {
                    seq_pointer()->set_last_tick
                    (
                        seq_pointer()->get_last_tick() - snap()
                    );
                    dirty = true;
                }
                else if (event->key() == Qt::Key_Right)
                {
                    seq_pointer()->set_last_tick
                    (
                        seq_pointer()->get_last_tick() + snap()
                    );
                    dirty = true;
                }
            }
            else if (event->modifiers() & Qt::ShiftModifier) // Shift + ...
            {
                if (event->key() == Qt::Key_Z)
                {
                    zoom_in();
                    dirty = true;
                }
            }
            else
            {
                if (event->key() == Qt::Key_Z)
                {
                    zoom_out();
                    dirty = true;
                }
                else if (event->key() == Qt::Key_0)
                {
                    reset_zoom();
                    dirty = true;
                }
            }
        }
        if (! dirty && (event->modifiers() & Qt::ControlModifier))
        {
            switch (event->key())
            {
            case Qt::Key_X:

                seq_pointer()->cut_selected();
                dirty = true;
                break;

            case Qt::Key_C:

                seq_pointer()->copy_selected();
                dirty = true;
                break;

            case Qt::Key_V:

                start_paste();
                dirty = true;
                break;

            case Qt::Key_Z:

                if (event->modifiers() & Qt::ShiftModifier)
                {
                    seq_pointer()->pop_redo();
                    dirty = true;
                }
                else
                    seq_pointer()->pop_undo();

                dirty = true;
                break;

            case Qt::Key_A:

                seq_pointer()->select_all();
                dirty = true;
                break;
            }
        }
        else
        {
            if
            (
                (event->modifiers() & Qt::ShiftModifier) == 0 &&
                (event->modifiers() & Qt::MetaModifier) == 0
            )
            {
                switch (event->key())
                {
                case Qt::Key_P:

                    set_adding(true);
                    dirty = true;
                    break;

                case Qt::Key_X:

                    set_adding(false);
                    dirty = true;
                    break;
                }
            }
        }
    }

    /*
     * If we reach this point, the key isn't relevant to us; ignore it so the
     * event is passed to the parent.
     */

    if (dirty)
        set_dirty();
    else
        QWidget::keyPressEvent(event);  // event->ignore();
}

/**
 *
 */

void
qseqrollpix::keyReleaseEvent (QKeyEvent *)
{
    // no code
}

/**
 *  Proposed new function to encapsulate the movement of selections even
 *  more fully.  Works with the four arrow keys.
 *
 *  Note that the movement vertically is different for the selection box versus
 *  the notes.  While the movement values are -1, 0, or 1, the differences are
 *  as follows:
 *
 *      -   Selection box vertical movement:
 *          -   -1 is up one note snap.
 *          -   0 is no vertical movement.
 *          -   +1 is down one note snap.
 *      -   Note vertical movement:
 *          -   -1 is down one note.
 *          -   0 is no note vertical movement.
 *          -   +1 is up one note.
 *
 * \param dx
 *      The amount to move the selection box or the selection horizontally.
 *      Values are -1 (left one time snap), 0 (no movement), and +1 (right one
 *      snap).  Obviously values other than +-1 can be used for larger
 *      movement, but the GUI doesn't yet support that ... we could implement
 *      movement by "pages" some day.
 *
 * \param dy
 *      The amount to move the selection box or the selection vertically.  See
 *      the notes above.
 */

void
qseqrollpix::move_selected_notes (int dx, int dy)
{
    if (paste())
    {
        //// move_selection_box(dx, dy);
    }
    else
    {
        int snap_x = dx * snap();                   /* time-stamp snap  */
        int snap_y = -dy;                           /* note pitch snap  */
        if (seq_pointer()->any_selected_notes())     /* redundant!       */
        {
            seq_pointer()->move_selected_notes(snap_x, snap_y);
        }
        else if (snap_x != 0)
        {
            seq_pointer()->set_last_tick(seq_pointer()->get_last_tick() + snap_x);
        }
    }
}

/**
 *  Proposed new function to encapsulate the movement of selections even
 *  more fully.
 *
 * \param dx
 *      The amount to grow the selection horizontally.  Values are -1 (left one
 *      time snap), 0 (no stretching), and +1 (right one snap).  Obviously
 *      values other than +-1 can be used for larger stretching, but the GUI
 *      doesn't yet support that.
 */

void
qseqrollpix::grow_selected_notes (int dx)
{
    if (! paste())
    {
        int snap_x = dx * snap();                   /* time-stamp snap  */
        growing(true);
        seq_pointer()->grow_selected(snap_x);
    }
}

/**
 *  Provides the base sizing of the piano roll.  If less than the width of the
 *  parent frame, it is increased to that, so that the roll covers the whole
 *  scrolling area (in qseqeditframe).
 */

QSize
qseqrollpix::sizeHint () const
{
    int len = analyze_width();                      /* side-effects     */
    int h = m_keyarea_y + 1;
    return QSize(len, h);                           // m_current_height
}

/**
 *  Steps for calculating the pixmap width.  (The pixmap height is not an
 *  issue at present.)
 *
 *  Currently, sizeHint() shows h=1538, w=890, len=38790, zoom=2.  "pix width"
 *  = 38790, so does "can width".
 *
 * \sideeffect
 *      Sets m_current_height (basically a constant of 1538) and sets
 *      m_current_width to the pattern length or
 *
 * Definitions:
 *
 *      -   Wlp. Width of the loop/pattern/sequence in pixels.  Affected by
 *          the current zoom.  It can be narrower than the piano roll window, or
 *          wider.  If narrower, then Nf = 2.
 *      -   Bwt. Beat-width of the loop in ticks.
 *      -   Wwp. Window width in pixels.  This is the visible area of the piano
 *          roll, and starts out at about 884 pixels.  Also known as pixels per
 *          window.
 *      -   Fwp. Frame width in pixels.  Sets m_pixmap_width.
 *      -   Fwt. Frame width in ticks.  Fwt = tpbeat * framebeats, but is also
 *          easily given by m_t0 - m_t1.
 *      -   Nw. Number of windows in pattern. If the pattern is shorter than
 *          the window, this value is set to 1.
 *      -   Nf. The frame count.  A frame is a pixmap that is close to the
 *          window in size, sometimes wider, sometimes narrower.
 *      -   bpbar. Beats per bar, beats per measure.
 *      -   tpbeat. Ticks per beat.
 *      -   ppbeat. Pixels per beat.
 *      -   bpwindow. Beats per window, can have a fractional part.
 *      -   framebeats. If only 1 frame is needed, it is set to the beats/window
 *          plus one beat. Otherwise it is just the beats per window, which may
 *          be too short!!!
 *
 * \return
 *      Returns the width needed to represent all of the frames in the sequence.
 *      The resulting width must be enough to paint all of them.
 */

#define EPSILON     0.01

int
qseqrollpix::analyze_width () const
{
    int result = 0;
    int Wlp = tix_to_pix(seq_pointer()->get_length());
    int Bwt = seq_pointer()->get_beat_width();
    int Wwp = m_parent_frame->width();
    int bpbar = seq_pointer()->get_beats_per_bar();
    int tpbeat = (4 * perf().ppqn()) / Bwt;
    int ppbeat = tix_to_pix(tpbeat);
    double bpwindow = double(Wwp) / double(ppbeat);     /* beats in window  */
    double beatcount = std::trunc(bpwindow);            /* whole beats      */
    int framebeats, Fwp;
    int Nf = Wlp / Wwp;
    if (Nf == 0)                                        /* need 1 big frame */
    {
        framebeats = int(beatcount) + 1;                /* frame > window   */
        Fwp = framebeats * ppbeat;
        Nf = 1;                                         /* a single frame   */
    }
    else
    {
        framebeats = int(beatcount);                    /* frame <= window  */
        Fwp = framebeats * ppbeat;
        if (Wlp > Wwp)
            Nf = Wlp / Fwp;                             /* else it is 1     */
    }

    m_ticks_per_step = pix_to_tix(cm_base_step_ticks);
    m_ticks_per_beat = tpbeat;
    m_ticks_per_bar = bpbar * m_ticks_per_beat;
    m_pixmap_count = Nf;                                /* pixmap == frame  */

    int beginning = scroll_offset();
    beginning -= (scroll_offset() % m_ticks_per_step);
    m_pixmap_width = Fwp;
    m_frame_ticks = tpbeat * framebeats;
    m_t0 = beginning;
    m_t1 = m_t0 + m_frame_ticks;                        /* Fwt not needed   */
    result = Nf * Fwp + m_keypadding_x;
    if (Fwp != m_pixmap_width || m_t0 != beginning)
        m_pixmaps_ready = false;

    return result;
}

/**
 *  Snaps the y pixel to the height of a piano key.
 *
 * \param [in,out] y
 *      The vertical pixel value to be snapped.
 */

void
qseqrollpix::snap_y (int & y)
{
    y -= y % m_key_y;
}

/**
 *  Provides an override to change the mouse "cursor" based on whether adding
 *  notes is active, or not.
 *
 * \param a
 *      The value of the status of adding (e.g. a note).
 */

void
qseqrollpix::set_adding (bool a)
{
    qseqbase::set_adding(a);
    if (a)
        setCursor(Qt::PointingHandCursor);  // Qt::CrossCursor ?
    else
        setCursor(Qt::ArrowCursor);

    set_dirty();
}

/**
 *  The current (x, y) drop points are snapped, and the pasting flag is set to
 *  true.  Then this function
 *  Gets the box that selected elements are in, then adjusts for the clipboard
 *  being shifted to tick 0.
 *
 */

void
qseqrollpix::start_paste ()
{
    snap_current_x();
    snap_current_y();
    drop_x(current_x());
    drop_y(current_y());
    paste(true);

    midipulse tick_s, tick_f;
    int note_h, note_l;
    seq_pointer()->get_clipboard_box(tick_s, note_h, tick_f, note_l);
    convert_tn_box_to_rect(tick_s, tick_f, note_h, note_l, selection());
    selection().xy_incr(drop_x(), drop_y() - selection().y());
}

/**
 *  Sets the drum/note mode status.
 *
 * \param mode
 *      The drum or note mode status.
 */

void
qseqrollpix::update_edit_mode (sequence::editmode mode)
{
    m_edit_mode = mode;
}

/**
 *  Sets the current chord to the given value.
 *
 * \param chord
 *      The desired chord value.
 */

void
qseqrollpix::set_chord (int chord)
{
    if (m_chord != chord)
        m_chord = chord;
}

/**
 *
 */

void
qseqrollpix::set_key (int key)
{
    if (m_key != key)
        m_key = key;
}

/**
 *
 */

void
qseqrollpix::set_scale (int scale)
{
    if (m_scale != scale)
        m_scale = scale;
}


/**
 *  Checks the position of the tick, and, if it is in a different piano-roll
 *  "page" than the last page, moves the page to the next page.
 *
 *  We don't want to do any of this if the length of the sequence fits in the
 *  window, but for now it doesn't hurt; the progress bar just never meets the
 *  criterion for moving to the next page.
 *
 *  This feature is not provided by qseqeditframe; it requires
 *  qseqeditframe64.
 *
 * \todo
 *      -   If playback is disabled (such as by a trigger), then do not update
 *          the page;
 *      -   When it comes back, make sure we're on the correct page;
 *      -   When it stops, put the window back to the beginning, even if the
 *          beginning is not defined as "0".
 */

void
qseqrollpix::follow_progress ()
{
    if (not_nullptr(m_parent_frame) && m_is_new_edit_frame)
    {
        reinterpret_cast<qseqeditframe64 *>(m_parent_frame)->follow_progress();
    }
}

/**
 * Draw the current pixmap frame.  Note that, if the width and height
 * change, we will have to reevaluate.
 * Draw the events. This currently draws all of them.  Drawing all them only
 * needs to be drawn once.
 */

void
qseqrollpix::draw_notes
(
    QPainter & painter,
    const rollframe & roll,
    bool background
)
{
    QBrush brush(Qt::NoBrush);
    QPen pen(Qt::lightGray);
    pen.setStyle(Qt::SolidLine);
    painter.setPen(pen);
    painter.setBrush(brush);

    midipulse seqlength = seq_pointer()->get_length();
    int ww = width();                   // m_current_width
    midipulse start_tick = roll.t0();
    midipulse end_tick = start_tick + pix_to_tix(ww);
    sequence * s = background ?
        perf().get_sequence(m_background_sequence) : seq_pointer().get() ;

    pen.setColor(Qt::black);            /* draw boxes from sequence */
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(1);

    /*
     * TODO: use reset_ex_iterator() or reset_interval().
     */

    s->reset_draw_marker();
    for (;;)
    {
        sequence::note_info ni;
        sequence::draw dt = s->get_next_note(ni);
        if (dt == sequence::draw::finish)
            break;

        break;

        bool inbounds = ni.start() >= start_tick && ni.start() < end_tick;
        bool linkedin = dt == sequence::draw::linked && inbounds;
        if (inbounds || linkedin)
        {
            m_note_x = xoffset(ni.start());
            m_note_y = m_keyarea_y - (ni.note() * m_key_y) - m_key_y + 1;
            m_note_height = m_key_y - 3;

            int in_shift = 0;
            int length_add = 0;
            if (dt == sequence::draw::linked)
            {
                if (ni.finish() >= ni.start())
                {
                    m_note_width = tix_to_pix(ni.finish() - ni.start());
                    if (m_note_width < 1)
                        m_note_width = 1;
                }
                else
                    m_note_width = tix_to_pix(seqlength - ni.start());
            }
            else
                m_note_width = tix_to_pix(16);

            if (dt == sequence::draw::note_on)
            {
                in_shift = 0;
                length_add = 2;
            }
            if (dt == sequence::draw::note_off)
            {
                in_shift = -1;
                length_add = 1;
            }
            pen.setColor(Qt::black);
            if (background)                     // draw background note
            {
                length_add = 1;
                pen.setColor(Qt::darkCyan);     // note border color
                brush.setColor(Qt::darkCyan);
            }
            else
            {
                pen.setColor(Qt::black);        // note border color
                brush.setColor(Qt::black);
            }

            brush.setStyle(Qt::SolidPattern);
            painter.setBrush(brush);
            painter.setPen(pen);
            painter.drawRect(m_note_x, m_note_y, m_note_width, m_note_height);
            if (ni.finish() < ni.start())   // shadow notes before zero
            {
                painter.setPen(pen);
                painter.drawRect
                (
                    m_keypadding_x, m_note_y,
                    tix_to_pix(ni.finish()), m_note_height
                );
            }

            /*
             * Draw note highlight if there's room.  Orange note if selected,
             * red if drum mode, otherwise plain white.
             */

            if (m_note_width > 3)
            {
                if (ni.selected())
                    brush.setColor("orange");
                else
                    brush.setColor(Qt::white);

                painter.setBrush(brush);
                if (! background)
                {
                    if (m_edit_mode == sequence::editmode::note)
                    {
                        int x_shift = m_note_x + in_shift;
                        int h_minus = m_note_height - 1;
                        if (ni.finish() >= ni.start())  // note highlight
                        {
                            painter.drawRect
                            (
                                x_shift, m_note_y,
                                m_note_width - 1 + length_add,
                                h_minus
                            );
                        }
                        else
                        {
                            int w = tix_to_pix(ni.finish()) - 3 + length_add;
                            painter.drawRect
                            (
                                x_shift, m_note_y,
                                m_note_width, h_minus
                            );
                            painter.drawRect
                            (
                                m_keypadding_x, m_note_y,
                                w, h_minus
                            );
                        }
                    }
                }
            }
        }
    }
}

/**
 *
 */

void
qseqrollpix::draw_drum_notes
(
    QPainter & painter,
    const rollframe & roll,
    bool background
)
{
    QBrush brush(Qt::NoBrush);
    QPen pen(Qt::lightGray);
    pen.setStyle(Qt::SolidLine);
    painter.setPen(pen);
    painter.setBrush(brush);
    m_edit_mode = perf().edit_mode(seq_pointer()->seq_number());

    midipulse seqlength = seq_pointer()->get_length();
    int ww = width();                   // m_current_width
    int start_tick = roll.t0();
    int end_tick = start_tick + pix_to_tix(ww);
    sequence * s = nullptr;
    if (background)
        s = perf().get_sequence(m_background_sequence);
    else
        s = seq_pointer().get();

    pen.setColor(Qt::red);              /* draw red boxes from drum loop    */
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(1);

    /*
     * TODO: use reset_ex_iterator() or reset_interval().
     */

    s->reset_draw_marker();
    for (;;)
    {
        sequence::note_info ni;
        sequence::draw dt = s->get_next_note(ni);
        if (dt == sequence::draw::finish)
            break;

        bool inbounds = ni.start() >= start_tick && ni.start() < end_tick;
        bool linkedin = dt == sequence::draw::linked && inbounds;
        if (inbounds || linkedin)
        {
            m_note_x = xoffset(ni.start());
            m_note_y = m_keyarea_y - (ni.note() * m_key_y) - m_key_y - 1 + 2;
            m_note_height = m_key_y;

#ifdef USE_THIS_CODE
            int in_shift = 0;
            int length_add = 0;
#endif

            // IFFY, because we should ignore length for drum notes

            if (dt == sequence::draw::linked)
            {
                if (ni.finish() >= ni.start())
                {
                    m_note_width = tix_to_pix(ni.finish() - ni.start());
                    if (m_note_width < 1)
                        m_note_width = 1;
                }
                else
                    m_note_width = tix_to_pix(seqlength - ni.start());
            }
            else
                m_note_width = tix_to_pix(16);

#ifdef USE_THIS_CODE
            if (dt == sequence::draw::note_on)
            {
                in_shift = 0;
                length_add = 2;
            }
            if (dt == sequence::draw::note_off)
            {
                in_shift = -1;
                length_add = 1;
            }
#endif

            pen.setColor(Qt::black);
            if (background)                     // draw background note
            {
#ifdef USE_THIS_CODE
                length_add = 1;
                pen.setColor(Qt::darkCyan);     // note border color
                brush.setColor(Qt::darkCyan);
#endif
            }
            else
            {
                pen.setColor(Qt::black);        // note border color
                brush.setColor(Qt::black);
            }

            brush.setStyle(Qt::SolidPattern);
            painter.setBrush(brush);
            painter.setPen(pen);
            draw_drum_note(painter);

            /*
             * Draw note highlight in drum mode.  Orange note if selected, red
             * if drum mode, otherwise plain white.
             */

            if (ni.selected())
                brush.setColor("orange");       // Qt::red
            else if (m_edit_mode == sequence::editmode::drum)
                brush.setColor(Qt::red);
            else
                brush.setColor(Qt::white);

            painter.setBrush(brush);
            if (! background)
                draw_drum_note(painter);    // background
        }
    }

    /*
     * Draw the events end.
     */
}

}           // namespace seq66

/*
 * qseqrollpix.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


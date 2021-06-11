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
 * \file          qseqroll.cpp
 *
 *  This module declares/defines the base class for drawing on the piano
 *  roll of the patterns editor for the Qt 5 implementation.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2021-05-07
 * \license       GNU GPLv2 or above
 *
 *  Please see the additional notes for the Gtkmm-2.4 version of this panel,
 *  seqroll.
 */

#include <QApplication>                 /* QApplication keyboardModifiers() */
#include <QFrame>                       /* base class for seqedit frame(s)  */
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QTimer>

#include "cfg/settings.hpp"             /* seq66::usr().key_height(), etc.  */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "qseqeditframe64.hpp"          /* seq66::qseqeditframe64 class     */
#include "qseqkeys.hpp"                 /* seq66::qseqkeys class            */
#include "qseqroll.hpp"                 /* seq66::qseqroll class            */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Default value for randomization.  Currently the only value supported.
 */

const int c_randomize_range = 4;        /* randomize range in ticks         */

/**
 *  Principal constructor.
 */

qseqroll::qseqroll
(
    performer & p,
    seq::pointer seqp,
    qseqkeys * seqkeys_wid,
    int zoom, int snap,
    sequence::editmode mode,
    qseqeditframe64 * frame                         /* qseqframe        */
) :
    QWidget                 (frame),
    qseqbase
    (
        p, seqp, zoom, snap,
        usr().key_height(),                         /* was m_key_y      */
        usr().key_height() * c_num_keys + 1         /* was m_keyarea_y  */
    ),
    m_backseq_color         (backseq_paint()),
    m_parent_frame          (frame),
    m_seqkeys_wid           (seqkeys_wid),
    m_timer                 (nullptr),
    m_progbar_width         (usr().progress_bar_thick() ? 2 : 1),
    m_roll_frame            (m_progbar_width),
    m_scale                 (scales::off),
    m_pos                   (0),
    m_chord                 (0),
    m_key                   (0),
    m_note_length           (p.ppqn() * 4 / 16),
    m_note_off_margin       (2),
    m_background_sequence   (seq::unassigned()),
    m_drawing_background_seq (false),
    m_status                (0),
    m_cc                    (0),
    m_edit_mode             (mode),
    m_draw_whole_grid       (true),
    m_t0                    (0),
    m_t1                    (0),
    m_frame_ticks           (0),
    m_note_x                (0),
    m_note_width            (0),
    m_note_y                (0),
    m_keypadding_x          (c_keyboard_padding_x),
    m_v_zooming             (false),
    m_last_base_note        (-1)
{
    setAttribute(Qt::WA_StaticContents);
    setAttribute(Qt::WA_OpaquePaintEvent);          /* no erase on repaint  */
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    set_snap(seqp->snap());
    setMouseTracking(true);         /* track mouse movement without a click */
    show();
    m_timer = new QTimer(this);
    m_timer->setInterval(1 * usr().window_redraw_rate());
    QObject::connect
    (
        m_timer, SIGNAL(timeout()), this, SLOT(conditional_update())
    );
    m_timer->start();
}

/**
 *  This virtual destructor stops the timer.
 */

qseqroll::~qseqroll ()
{
    m_timer->stop();
}

/**
 *  In an effort to reduce CPU usage when simply idling, this function calls
 *  update() only if necessary.  See qseqbase::check_dirty().
 *
 *  bool ok = seq_pointer()->playing();
 */

void
qseqroll::conditional_update ()
{
    bool ok = perf().needs_update() || check_dirty();
    if (ok)
    {
        if (progress_follow())
            follow_progress();              /* keep up with progress    */

        update();
    }
}

/**
 *  Zooms in, first calling the base-class version of this function, then
 *  passing along the message to the parent edit frame, so that it can change
 *  the zoom on the other panels of the parent edit frame.
 */

bool
qseqroll::zoom_in ()
{
    bool result = qseqbase::zoom_in();
    if (result)
    {
        result = frame64()->set_zoom(zoom());
        set_dirty();
    }
    return result;
}

/**
 *  Zooms out, first calling the base-class version of this function, then
 *  passing along the message to the parent edit frame, so that it can change
 *  the zoom on the other panels of the parent edit frame.
 */

bool
qseqroll::zoom_out ()
{
    bool result = qseqbase::zoom_out();
    if (result)
    {
        result = frame64()->set_zoom(zoom());
        set_dirty();
    }
    return result;
}

/**
 *  Tells the parent frame to reset our zoom.
 */

bool
qseqroll::reset_zoom ()
{
    bool result = frame64()->reset_zoom();
    set_dirty();
    return result;
}

bool
qseqroll::v_zoom_in ()
{
    bool result = m_seqkeys_wid->v_zoom_in();
    if (result)
    {
        int h = m_seqkeys_wid->note_height();
        unit_height(h);
        total_height(m_seqkeys_wid->total_height());
        m_v_zooming = true;
        set_dirty();
        frame64()->set_dirty();
    }
    return result;
}

bool
qseqroll::v_zoom_out ()
{
    bool result = m_seqkeys_wid->v_zoom_out();
    if (result)
    {
        int h = m_seqkeys_wid->note_height();
        unit_height(h);
        total_height(m_seqkeys_wid->total_height());
        m_v_zooming = true;
        set_dirty();
        frame64()->set_dirty();
    }
    return result;
}

bool
qseqroll::reset_v_zoom ()
{
    bool result = m_seqkeys_wid->reset_v_zoom();
    if (result)
    {
        int h = m_seqkeys_wid->note_height();
        unit_height(h);
        total_height(m_seqkeys_wid->total_height());
        set_dirty();
        frame64()->set_dirty();
    }
    m_v_zooming = false;
    return result;
}

int
qseqroll::note_height () const
{
    return m_seqkeys_wid->note_height();
}

/**
 *  Override.
 *
 * \param v
 *      The value of the scrollbar in pixels.
 */

void
qseqroll::scroll_offset (int x)
{
    midipulse ticks = pix_to_tix(x);
    midipulse ticks_per_step = pulses_per_substep(perf().ppqn(), zoom());
    m_t0 = ticks - (ticks % ticks_per_step);
    m_frame_ticks = pix_to_tix(frame64()->width());
    m_t1 = ticks + m_frame_ticks;
    qseqbase::scroll_offset(x);                         // WHY COMMENTED OUT?
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
 *      Provides the sequence number, which is checked against seq::legal()
 *      before being used.  This macro allows the value seq::limit (2048),
 *      which disables the background sequence.
 */

void
qseqroll::set_background_sequence (bool state, int seq)
{
    if (state != m_drawing_background_seq && m_background_sequence != seq)
    {
        if (seq::legal(seq))
        {
            m_drawing_background_seq = state;
            m_background_sequence = seq;
        }
        if (is_initialized())
            set_dirty();
    }
}

/**
 *  Does anybody use this one?
 */

void
qseqroll::set_redraw ()
{
    m_draw_whole_grid = true;
    set_dirty();
}

/**
 *  Draws the piano roll.
 *
 *  In later usage, the width() function [and height() as well?], returns a
 *  humongous value (38800+).  So we store the current values to use, via
 *  window_width() and window_height(), in follow_progress().
 *
 *  We have to decide how to handle repaints after the initialization.  Do we
 *  use the QRect of the paint-event?
 *
 *  Here, we could choose black instead white for "inverse" mode.
 */

void
qseqroll::paintEvent (QPaintEvent * qpep)
{
    QRect r = qpep->rect();
    QRect view(0, 0, width(), height());
    QPainter painter(this);
    QBrush brush(Qt::white, Qt::NoBrush);
    QPen pen(Qt::lightGray);
    pen.setStyle(Qt::SolidLine);
    m_frame_ticks = pix_to_tix(r.width());
    m_edit_mode = perf().edit_mode(seq_pointer()->seq_number());

    /*
     * Draw the border.  See the banner notes about width and height.
     * Doesn't seem to be needed: painter.drawRect(0, 0, ww, wh);
     */

    pen.setColor(Qt::lightGray);
    pen.setStyle(Qt::SolidLine);                    /* Qt::DotLine          */
    painter.setPen(pen);
    draw_grid(painter, view);
    set_initialized();

    /*
     * Draw the events. This currently draws all of them.  Drawing all them
     * only needs to be drawn once.
     */

    call_draw_notes(painter, view);
    pen.setWidth(1);

    /*
     *  Draw the playhead.
     */

    pen.setColor(progress_color());
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(m_progbar_width);
    painter.setPen(pen);
    old_progress_x(progress_x());
    progress_x(xoffset(seq_pointer()->get_tick()));
    painter.drawLine(progress_x(), r.y(), progress_x(), r.y() + r.height());

    /*
     * End of draw_progress_on_window().  The next step is to restore
     * the "empty" brush style in case the user draws a selection box.
     */

    brush.setStyle(Qt::NoBrush);            /* painter reset                */
    painter.setBrush(brush);
    if (select_action())                    /* select/move/paste/grow       */
        pen.setStyle(Qt::SolidLine);

    int x, y, w, h;                         /* draw selections              */
    if (selecting())
    {
        rect::xy_to_rect_get
        (
            drop_x(), drop_y(), current_x(), current_y(), x, y, w, h
        );
        old_rect().set(x, y, w, h + unit_height());
        pen.setColor(sel_color());
        painter.setPen(pen);
        painter.drawRect(x, y, w, h);
    }

    int selw = selection().width();
    int selh = selection().height();
    if (drop_action())
    {
        int delta_x = current_x() - drop_x();
        int delta_y = current_y() - drop_y();
        x = selection().x() + delta_x;
        y = selection().y() + delta_y;
        pen.setColor(Qt::black);            /* what palette color to use?   */
        painter.setPen(pen);
        if (is_drum_mode())
        {
            int drumx = x - unit_height() * 0.5 + m_keypadding_x;
            painter.drawRect(drumx, y, selw + unit_height(), selh);
        }
        else
            painter.drawRect(x + m_keypadding_x, y, selw, selh);

        old_rect().set(x, y, selw, selh);
    }
    if (growing())
    {
        int delta_x = current_x() - drop_x();
        selw += delta_x;
        if (selw < 1)
            selw = 1;

        x = selection().x();
        y = selection().y();
        pen.setColor(sel_color());          /* fore_color() Qt::black       */
        painter.setPen(pen);
        painter.drawRect(x + m_keypadding_x, y, selw, selh);
        old_rect().set(x, y, selw, selh);
    }
}

void
qseqroll::call_draw_notes (QPainter & painter, const QRect & view)
{
    if (m_drawing_background_seq)
        draw_notes(painter, view, true);

    if (is_drum_mode())
        draw_drum_notes(painter, view, false);
    else
        draw_notes(painter, view, false);
}

/**
 *  First, we clear the rectangle before drawing.  At this point, we could
 *  choose black instead white for "inverse" mode.
 */

void
qseqroll::draw_grid (QPainter & painter, const QRect & r)
{
    int octkey = c_octave_size - m_key;             /* used three times     */
    QBrush brush(back_color());                     /* brush(Qt::NoBrush)   */
    QPen pen(grey_color());                         /* pen(Qt::lightGray)   */
    pen.setStyle(Qt::SolidLine);                    /* Qt::DotLine          */
    painter.fillRect(r, brush);                     /* blank the viewport   */
    painter.setBrush(brush);
    painter.setPen(pen);
    painter.drawRect(r);
    for (int key = 1; key <= c_num_keys; ++key)     /* for each note row    */
    {
        int remkeys = c_num_keys - key;             /* remaining keys       */
        int modkey = remkeys - scroll_offset_v() + octkey;

        /*
         * Draw horizontal grid lines differently depending on editing mode.
         * Set line colour dependent on the note row we're on.
         */

        int y = key * unit_height();
        if ((modkey % c_octave_size) == 0)
            pen.setColor(fore_color());
        else
            pen.setColor(step_color());

        painter.setPen(pen);
        painter.drawLine(r.x(), y, r.x() + r.width(), y);
        if (m_scale != scales::off)
        {
            if (! c_scales_policy[int(m_scale)][(modkey - 1) % c_octave_size])
            {
                pen.setColor(fore_color());         /* Qt::lightGray        */
                painter.setBrush(scale_brush());
                painter.setPen(pen);
                painter.drawRect(0, y + 1, r.width(), unit_height() - 1);
            }
        }
    }

    /*
     * The ticks_per_step value needs to be figured out.  Why 6 * zoom()?  6
     * is the number of pixels in the smallest divisions in the default
     * seqroll background.
     *
     * This code needs to be put into a function.
     */

    int bpbar = seq_pointer()->get_beats_per_bar();
    int bwidth = seq_pointer()->get_beat_width();
    midipulse ticks_per_beat = (4 * perf().ppqn()) / bwidth;
    midipulse ticks_per_bar = bpbar * ticks_per_beat;
    midipulse ticks_per_step = pulses_per_substep(perf().ppqn(), zoom());
    midipulse increment = ticks_per_step;       /* ticks_per_step or 1      */
    midipulse endtick = pix_to_tix(r.x() + r.width());
    midipulse starttick = pix_to_tix(r.x());
    starttick -= starttick % ticks_per_step;

    /*
     * Draw vertical grid lines.  Incrementing by ticks_per_step only works for
     * PPQN of certain multiples or for certain time offsets.  Therefore, need
     * to check every darn tick!!!!
     */

    for (int tick = starttick; tick < endtick; tick += increment)
    {
        int x_offset = xoffset(tick) - scroll_offset_x();
        int penwidth = 1;
        enum Qt::PenStyle penstyle = Qt::SolidLine;
        if (tick % ticks_per_bar == 0)          /* solid line on every beat */
        {
            pen.setColor(fore_color());         /* Qt::black                */
            penwidth = 2;
        }
        else if (tick % ticks_per_beat == 0)
        {
            pen.setColor(beat_color());
            penwidth = 1;
        }
        else
        {
            pen.setColor(step_color());        /* faint step lines         */
            int tick_snap = tick - (tick % grid_snap());
            if (tick != tick_snap)
                penstyle = Qt::DotLine;
        }
        pen.setWidth(penwidth);
        pen.setStyle(penstyle);
        painter.setPen(pen);
        painter.drawLine(x_offset, 0, x_offset, total_height());
    }
}

/**
 * Draw the current pixmap frame.  Note that, if the width and height
 * change, we will have to reevaluate.
 * Draw the events. This currently draws all of them.  Drawing all them only
 * needs to be drawn once.
 */

void
qseqroll::draw_notes
(
    QPainter & painter,
    const QRect & r,
    bool background
)
{
    QBrush brush(Qt::white);            /* Qt::NoBrush)  breaks selection   */
    QPen pen(fore_color());
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(1);
    painter.setPen(pen);
    painter.setBrush(brush);

    midipulse seqlength = seq_pointer()->get_length();
    midipulse start_tick = pix_to_tix(r.x());
    midipulse end_tick = start_tick + pix_to_tix(r.width());
    seq::pointer s = background ?
        perf().get_sequence(m_background_sequence) : seq_pointer() ;

    int unitheight = unit_height();
    int unitdecr = unit_height() - 1;
    int noteheight = unitheight - 3;
    auto cev = s->cbegin();
    while (! s->cend(cev))
    {
        sequence::note_info ni;
        sequence::draw dt = s->get_next_note(ni, cev); /* ++cev */
        if (dt == sequence::draw::finish)
            break;

        if (dt == sequence::draw::tempo)
            continue;                               // DRAW_TEMPO_LINE_DISABLED

        bool start_in = ni.start() >= start_tick && ni.start() <= end_tick;
        bool end_in = ni.finish() >= start_tick && ni.finish() <= end_tick;
        bool linkedin = dt == sequence::draw::linked && end_in;
        if (start_in || linkedin)
        {
            int in_shift = 0;
            int length_add = 0;
            m_note_x = xoffset(ni.start());
            m_note_y = total_height() - (ni.note() * unitheight) - unitdecr;
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
            if (background)                         /* draw background note */
            {
                length_add = 1;
                painter.setBrush(backseq_brush());
            }
            else
            {
                painter.setBrush(note_brush());
            }

            painter.drawRect(m_note_x, m_note_y, m_note_width, noteheight);
            if (ni.finish() < ni.start())   // shadow notes before zero
            {
                painter.drawRect
                (
                    m_keypadding_x, m_note_y,
                    tix_to_pix(ni.finish()), noteheight
                );
            }

            /*
             * Draw note highlight if there's room.  Orange note if selected,
             * red if drum mode, otherwise plain white.
             */

            if (m_note_width > 3)
            {
                if (ni.selected())
                    brush.setColor(sel_color());        /* was "orange"    */
                else
                    brush.setColor(note_in_color());    /* was Qt::white   */

                painter.setBrush(brush);
                if (! background)
                {
                    int x_shift = m_note_x + in_shift;
                    int h_minus = noteheight - 1;
                    if (ni.finish() >= ni.start())  // note highlight
                    {
                        painter.drawRect
                        (
                            x_shift, m_note_y,
                            m_note_width + length_add - 1, h_minus
                        );
                    }
                    else
                    {
                        int w = tix_to_pix(ni.finish()) + length_add - 3;
                        painter.drawRect
                        (
                            x_shift, m_note_y, m_note_width, h_minus
                        );
                        painter.drawRect
                        (
                            m_keypadding_x, m_note_y, w, h_minus
                        );
                    }
                }
            }
        }
    }
}

/*
 * Why floating point; just divide by 2.  Also, the polygon seems to be offset
 * downard by half the note height.
 */

void
qseqroll::draw_drum_note (QPainter & painter)
{
    int noteheight = unit_height();
    int h2 = noteheight / 2;
    int x0 = m_note_x - h2;
    int x1 = m_note_x + h2;
    int y1 = m_note_y + h2 - 2;
    QPointF points[4] =
    {
        QPointF(x0, y1),
        QPointF(m_note_x, m_note_y - 2),
        QPointF(x1, y1),
        QPointF(m_note_x, m_note_y + noteheight - 2)
    };
    painter.drawPolygon(points, 4);

    /*
     * Draw note highlight.  Not really useful, save time by ignoring.
     *
     * if (ni.selected())
     *     brush.setColor("orange");       // Qt::red
     * else if (is_drum_mode())
     *     brush.setColor(Qt::red);
     */
}

void
qseqroll::draw_drum_notes
(
    QPainter & painter,
    const QRect & r,
    bool background
)
{
    QBrush brush(Qt::NoBrush);
    QPen pen(drum_color());         /* draw red boxes from drum loop    */
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(1);
    brush.setStyle(Qt::SolidPattern);
    painter.setPen(pen);
    painter.setBrush(brush);
    m_edit_mode = perf().edit_mode(seq_pointer()->seq_number());

    midipulse seqlength = seq_pointer()->get_length();
    midipulse start_tick = pix_to_tix(r.x());
    midipulse end_tick = start_tick + pix_to_tix(r.width());
    seq::pointer s = background ?
        perf().get_sequence(m_background_sequence) : seq_pointer() ;

    int noteheight = unit_height();
    int noteheight2 = unit_height() + 1;
    auto cev = s->cbegin();
    while (! s->cend(cev))
    {
        sequence::note_info ni;
        sequence::draw dt = s->get_next_note(ni, cev); /* ++cev */
        if (dt == sequence::draw::finish)
            break;

        if (dt == sequence::draw::tempo)
            continue;

        bool start_in = ni.start() >= start_tick && ni.start() <= end_tick;
        bool end_in = ni.finish() >= start_tick && ni.finish() <= end_tick;
        bool linkedin = dt == sequence::draw::linked && end_in;
        if (start_in || linkedin)
        {
            m_note_x = xoffset(ni.start());
            m_note_y = total_height() - (ni.note() * noteheight) - noteheight2;
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

            /*
             * Draw note highlight in drum mode.  Orange note if selected, red
             * if drum mode, otherwise plain white.
             */

            if (ni.selected())
                brush.setColor(sel_color());
            else if (is_drum_mode())
                brush.setColor(drum_paint());
            else
                brush.setColor(Qt::white);

            pen.setColor(fore_color());
            painter.setPen(pen);
            painter.setBrush(brush);
            draw_drum_note(painter);
        }
    }
}

int
qseqroll::note_off_length () const
{
    return m_note_length - m_note_off_margin;
}

/**
 * Convenience wrapper for sequence::add_note().  The length parameters is
 * obtained from the note_off_length() function.  This sets the note
 * length at a little less than the snap value.
 *
 * \param tick
 *      The time destination of the new note, in pulses.
 *
 * \param note
 *      The pitch destination of the new note.
 *
 * \param paint
 *      If true, repaint to be left with just the inserted event.  The
 *      default is true.  The value of false is useful in inserting a
 *      number of events and saving the repainting until last.  It is a
 *      bit tricky, as the default paint value for sequence::add_note() is
 *      false.
 */

bool
qseqroll::add_note (midipulse tick, int note)
{
    bool result;
    if (m_chord > 0)
    {
        result = seq_pointer()->push_add_chord
        (
            m_chord, tick, note_off_length(), note
        );
    }
    else
    {
        result = seq_pointer()->push_add_note
        (
            tick, note_off_length(), note, true         /* paint */
        );
    }
    if (result)
        set_dirty();

    return result;
}

void
qseqroll::resizeEvent (QResizeEvent * qrep)
{
#if defined SEQ66_PLATFORM_DEBUG_TMI
    static int s_count = 0;
    printf("qseqroll::resizeEvent(%d)\n", s_count++);
#endif
    QWidget::resizeEvent(qrep);         /* qrep->ignore() */
}

void
qseqroll::mousePressEvent (QMouseEvent * event)
{
    seq::pointer s = seq_pointer();
    midipulse tick_s, tick_f;
    int note, note_l, norm_x, norm_y, snapped_x, snapped_y;
    snapped_x = norm_x = event->x() - m_keypadding_x;
    snapped_y = norm_y = event->y();
    snap_x(snapped_x);
    snap_y(snapped_y);
    current_y(snapped_y);
    drop_y(snapped_y);                              /* y is always snapped  */
    if (paste())
    {
        convert_xy(snapped_x, snapped_y, tick_s, note);
        s->paste_selected(tick_s, note);
        paste(false);
        setCursor(Qt::ArrowCursor);
        set_dirty();
    }
    else
    {
        bool isctrl = bool(event->modifiers() & Qt::ControlModifier);
        bool lbutton = event->button() == Qt::LeftButton;
        bool rbutton = event->button() == Qt::RightButton;
        bool mbutton = event->button() == Qt::MiddleButton ||
            (lbutton && isctrl);

        if (lbutton)
        {
            current_x(norm_x);
            drop_x(norm_x);                         /* select non-snapped x */
            if (is_drum_mode())
            {
                int dropxadj = drop_x() - unit_height() / 2;    /* padding  */
                convert_xy(dropxadj, drop_y(), tick_s, note);
            }
            else
            {
                convert_xy(drop_x(), drop_y(), tick_s, note);
                tick_f = tick_s;
            }
            m_last_base_note = note;
            if (adding())                           /* painting new notes   */
            {
                eventlist::select selmode = eventlist::select::would_select;
                painting(true);                     /* start paint job      */
                current_x(snapped_x);
                drop_x(snapped_x);                  /* adding, snapped x    */
                convert_xy(drop_x(), drop_y(), tick_s, note);

                /*
                 * Test if a note is already there. Fake select, if so, don't
                 * add, else add a note, length = little less than snap.
                 */

                bool would_select = ! s->select_note_events
                (
                    tick_s, note, tick_s, note, selmode
                );
                if (would_select)
                    (void) add_note(tick_s, note);
            }
            else                                    /* we're selecting anew */
            {
                /*
                 *  In drum mode, were using "is_onset", but this breaks moving
                 *  the selected drum events. So we leave it at "selected".
                 */

                eventlist::select selmode = eventlist::select::selected;
                bool is_selected = s->select_note_events
                (
                    tick_s, note, tick_f, note, selmode
                );
                if (is_selected)
                {
                    if (! isctrl)
                    {
                        moving_init(true);          /* moving; L-click only */
                        set_dirty();
                        if (is_drum_mode())
                            s->onsets_selected_box(tick_s, note, tick_f, note_l);
                        else
                            s->selected_box(tick_s, note, tick_f, note_l);

                        convert_tn_box_to_rect
                        (
                            tick_s, tick_f, note, note_l, selection()
                        );

                        int adj_selected_x = selection().x();
                        snap_x(adj_selected_x);
                        move_snap_offset_x(selection().x() - adj_selected_x);
                        current_x(snapped_x);
                        drop_x(snapped_x);
                    }

                    /*
                     * Middle mouse button or left-ctrl click.
                     */

                    bool can_grow = mbutton && ! is_drum_mode();
                    if (can_grow)
                    {
                        growing(true);
                        s->selected_box(tick_s, note, tick_f, note_l);
                        convert_tn_box_to_rect
                        (
                            tick_s, tick_f, note, note_l, selection()
                        );
                    }
                }
                else
                {
                    if (! isctrl)
                    {
                        s->unselect();
                        frame64()->set_dirty();
                    }
                    selmode = is_drum_mode() ?
                        eventlist::select::onset :
                        eventlist::select::select_one ;

                    int numsel = s->select_note_events
                    (
                        tick_s, note, tick_f, note, selmode
                    );
                    if (numsel == 0)    /* none selected, start selection box */
                        selecting(true);
                    else
                        set_dirty();
                }
            }
        }
        if (rbutton)
            set_adding(true);
    }
}

void
qseqroll::mouseReleaseEvent (QMouseEvent * event)
{
    current_x(int(event->x()) - m_keypadding_x);
    current_y(event->y());
    snap_current_y();
    if (moving())
        snap_current_x();

    int delta_x = current_x() - drop_x();
    int delta_y = current_y() - drop_y();
    midipulse delta_tick;
    int delta_note;
    bool lbutton = event->button() == Qt::LeftButton;
    bool rbutton = event->button() == Qt::RightButton;
    bool isctrl = bool(event->modifiers() & Qt::ControlModifier);   /* Ctrl */
    bool mbutton = event->button() == Qt::MiddleButton || (lbutton && isctrl);
    if (lbutton)
    {
        if (selecting())
        {
            midipulse tick_s, tick_f;   /* start and  end of tick window    */
            int note_h, note_l;         /* high and low notes in window     */
            int x, y, w, h;             /* window dimensions                */
            eventlist::select selmode = eventlist::select::selecting;
            rect::xy_to_rect_get        /* copy drop dimensions to xywh     */
            (
                drop_x(), drop_y(), current_x(), current_y(), x, y, w, h
            );
            convert_xy(x, y, tick_s, note_h);
            convert_xy(x + w, y + h, tick_f, note_l);

            /*
             * This breaks the selection of events in drum mode.
             * if (is_drum_mode()) selmode = eventlist::select::onset;
             */

            int numsel = seq_pointer()->select_note_events
            (
                tick_s, note_h, tick_f, note_l, selmode
            );
            if (numsel > 0)
                frame64()->set_dirty();
        }
        if (moving())
        {
            /*
             * Adjust delta x for sna;, convert deltas into screen coordinates.
             * Since delta_note and delta_y are of opposite sign, we flip
             * the final result.  delta_y[0] = note[127].
             */

            int note;
            delta_x -= move_snap_offset_x();
            convert_xy(delta_x, current_y(), delta_tick, note);
            if (m_last_base_note >= 0)
            {
                delta_note = note - m_last_base_note;
            }
            else
            {
                convert_xy(delta_x, delta_y, delta_tick, delta_note);
                delta_note = delta_note - (c_num_keys - 1);
            }
            m_last_base_note = (-1);
            if (delta_x != 0 || delta_note != 0)
            {
                seq_pointer()->move_selected_notes(delta_tick, delta_note);
                set_dirty();
            }
        }
    }
    if (lbutton || mbutton)
    {
        if (growing())
        {
            convert_xy(delta_x, delta_y, delta_tick, delta_note);
            if (event->modifiers() & Qt::ShiftModifier)
                seq_pointer()->stretch_selected(delta_tick);
            else
                seq_pointer()->grow_selected(delta_tick);

            set_dirty();
        }
    }
    if (rbutton)
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
qseqroll::mouseMoveEvent (QMouseEvent * event)
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
    m_seqkeys_wid->set_preview_key(note);
    if (select_action())
    {
        if (drop_action())
            snap_current_x();
    }
    if (painting())
    {
        snap_current_x();
        convert_xy(current_x(), current_y(), tick, note);
        (void) add_note(tick, note);
    }
    set_dirty();
}

void
qseqroll::zoom_key_press (QKeyEvent * event)
{
    if (event->modifiers() & Qt::ShiftModifier)     /* Shift + ...  */
    {
        if (event->key() == Qt::Key_Z)
            (void) zoom_in();
        else if (event->key() == Qt::Key_V)
            (void) v_zoom_in();
    }
    else
    {
        if (event->key() == Qt::Key_Z)
        {
            (void) zoom_out();
        }
        else if (event->key() == Qt::Key_0)
        {
            if (m_v_zooming)
                (void) reset_v_zoom();
            else
                (void) reset_zoom();
        }
        else if (event->key() == Qt::Key_V)
            (void) v_zoom_out();
    }
}

/**
 *  Handles keystrokes for note movement, zoom, and more.  These key names are
 *  located in /usr/include/x86_64-linux-gnu/qt5/QtCore/qnamespace.h (for
 *  Debian/Ubuntu Linux).
 *
 *  We could simplify this a bit by creating a keystroke object.
 */

void
qseqroll::keyPressEvent (QKeyEvent * event)
{
    bool isctrl = bool(event->modifiers() & Qt::ControlModifier);
    bool isshift = bool(event->modifiers() & Qt::ShiftModifier);
    bool ismeta = bool(event->modifiers() & Qt::MetaModifier);
    seq::pointer s = seq_pointer();
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
    {
        if (s->remove_selected())
            set_dirty();
    }
    else
    {
        if (perf().is_pattern_playing())
        {
            /*
             * The space and period keystrokes are handled at the top of
             * qseqeditframe64::keyPressEvent(). The zoom keys are repeated
             * here and below as well.
             */

            if (! isctrl)
                zoom_key_press(event);
        }
        else
        {
            if (event->key() == Qt::Key_Left)
            {
                move_selected_notes(-1, 0);
                set_dirty();
            }
            else if (event->key() == Qt::Key_Right)
            {
                move_selected_notes(1, 0);
                set_dirty();
            }
            else if (event->key() == Qt::Key_Down)
            {
                move_selected_notes(0, 1);
                set_dirty();
            }
            else if (event->key() == Qt::Key_Up)
            {
                move_selected_notes(0, -1);
                set_dirty();
            }
            else if (isctrl)
            {
                /*
                 * We want to ignore Ctrl sequences here, so that Ctrl-Z's can
                 * be used for "undo".
                 */

                switch (event->key())
                {
                case Qt::Key_Left:

                    s->set_last_tick(s->get_last_tick() - snap());
                    set_dirty();
                    break;

                case Qt::Key_Right:

                    s->set_last_tick(s->get_last_tick() + snap());
                    set_dirty();
                    break;

                case Qt::Key_Home:

                    if (not_nullptr(frame64()))
                    {
                        frame64()->scroll_to_tick(0);
                    }
                    s->set_last_tick(0);        /* sets it to the beginning */
                    set_dirty();
                    break;

                case Qt::Key_End:

                    if (not_nullptr(frame64()))
                    {
                        frame64()->scroll_to_tick(s->get_length());
                    }
                    s->set_last_tick();         /* sets it to the length    */
                    set_dirty();
                    break;

                case Qt::Key_X:

                    if (s->cut_selected())
                        set_dirty();
                    break;

                case Qt::Key_C:

                    s->copy_selected();
                    break;

                case Qt::Key_V:

                    start_paste();
                    set_dirty();
                    setCursor(Qt::CrossCursor);
                    break;

                case Qt::Key_Z:

                    if (event->modifiers() & Qt::ShiftModifier)
                    {
                        /*
                         * Doesn't seem to do anything!
                         */

                        s->pop_redo();
                    }
                    else
                        s->pop_undo();

                    if (not_nullptr(frame64()))
                    {
                        frame64()->set_dirty();     /* set_dirty()      */
                    }
                    break;

                case Qt::Key_D:

                    sequence::clear_clipboard();    /* drop clipboard   */
                    break;

                case Qt::Key_A:

                    s->select_all();
                    set_dirty();
                    break;
                }
            }
            else
                zoom_key_press(event);
        }
        if (! is_dirty())
        {
            if (! isctrl && ! isshift && ! ismeta)
            {
                switch (event->key())
                {
                case Qt::Key_C:

                    if (frame64()->repitch_selected())
                        set_dirty();
                    break;

                case Qt::Key_F:

                    if (s->edge_fix())
                        set_dirty();
                    break;

                case Qt::Key_P:

                    set_adding(true);
                    break;

                case Qt::Key_Q:                 /* quantize selected notes  */

                    if (s->push_quantize(EVENT_NOTE_ON, 0, 1, true))
                        set_dirty();
                    break;

                case Qt::Key_R:                 /* default jitter == 8      */

                    if (s->randomize_selected_notes())
                        set_dirty();
                    break;

                case Qt::Key_T:                 /* tighten selected notes   */
                    if (s->push_quantize(EVENT_NOTE_ON, 0, 2, true))
                        set_dirty();
                    break;

                case Qt::Key_X:

                    set_adding(false);
                    break;
                }
            }
        }
    }

    /*
     * Erroneous conclusion, dude!
     *
     * If we reach this point, the key isn't relevant to us; ignore it so the
     * event is passed to the parent.
     */

    QWidget::keyPressEvent(event);      // event->ignore();
}

void
qseqroll::keyReleaseEvent (QKeyEvent * event)
{
    QWidget::keyReleaseEvent(event);    // event->ignore();
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
qseqroll::move_selected_notes (int dx, int dy)
{
    if (paste())
    {
        //// move_selection_box(dx, dy);
    }
    else
    {
        int snap_x = dx * snap();                   /* time-stamp snap  */
        int snap_y = -dy;                           /* note pitch snap  */
        seq::pointer s = seq_pointer();
        if (s->any_selected_notes())                /* redundant!       */
        {
            s->move_selected_notes(snap_x, snap_y);
        }
        else if (snap_x != 0)
        {
            s->set_last_tick(s->get_last_tick() + snap_x);
        }
    }
}

#if defined USE_GROW_SELECTED_NOTES_FUNCTION

/**
 *  Proposed new function to encapsulate the movement of selections even
 *  more fully.
 *
 *  Currently no one calls this function!
 *
 * \param dx
 *      The amount to grow the selection horizontally.  Values are -1 (left one
 *      time snap), 0 (no stretching), and +1 (right one snap).  Obviously
 *      values other than +-1 can be used for larger stretching, but the GUI
 *      doesn't yet support that.
 */

void
qseqroll::grow_selected_notes (int dx)
{
    if (! paste())
    {
        int snap_x = dx * snap();                   /* time-stamp snap  */
        growing(true);
        seq_pointer()->grow_selected(snap_x);
    }
}

#endif // defined USE_GROW_SELECTED_NOTES_FUNCTION

/**
 *  Provides the base sizing of the piano roll.  If less than the width of the
 *  parent frame, it is increased to that, so that the roll covers the whole
 *  scrolling area.
 */

QSize
qseqroll::sizeHint () const
{
    int h = total_height();                         /* + 1;             */
    int w = frame64()->width();
    int len = tix_to_pix(seq_pointer()->get_length());
    if (len < w)
        len = w;

    len += m_keypadding_x;
    return QSize(len, h);
}

/**
 *  Snaps the y pixel to the height of a piano key.
 *
 * \param [in,out] y
 *      The vertical pixel value to be snapped.
 */

void
qseqroll::snap_y (int & y)
{
    y -= y % unit_height();
}

/**
 *  Provides an override to change the mouse "cursor" based on whether adding
 *  notes is active, or not.
 *
 * \param a
 *      The value of the status of adding (e.g. a note).
 */

void
qseqroll::set_adding (bool a)
{
    qseqbase::set_adding(a);
    if (a)
        setCursor(Qt::PointingHandCursor);      /* Qt doesn't have a pencil */
    else
        setCursor(Qt::ArrowCursor);

    frame64()->update_note_entry(a);            /* updates checkable button */
    set_dirty();
}

/**
 *  The current (x, y) drop points are snapped, and the pasting flag is set to
 *  true.  Then this function Gets the box that selected elements are in, then
 *  adjusts for the clipboard being shifted to tick 0.
 */

void
qseqroll::start_paste ()
{
    snap_current_x();
    snap_current_y();
    drop_x(current_x());
    drop_y(current_y());
    paste(true);

    midipulse tick_s, tick_f;
    int note_h, note_l;
    seq_pointer()->clipboard_box(tick_s, note_h, tick_f, note_l);
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
qseqroll::update_edit_mode (sequence::editmode mode)
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
qseqroll::set_chord (int chord)
{
    if (m_chord != chord)
    {
        m_chord = chord;
        if (is_initialized())
            set_dirty();
    }
}

void
qseqroll::set_key (int key)
{
    if (m_key != key)
    {
        m_key = key;
        if (is_initialized())
            set_dirty();
    }
}

void
qseqroll::set_scale (int scale)
{
    if (int(m_scale) != scale)
    {
        m_scale = static_cast<scales>(scale);
        if (is_initialized())
            set_dirty();
    }
}

/**
 *  Checks the position of the tick, and, if it is in a different piano-roll
 *  "page" than the last page, moves the page to the next page.
 *
 *  We don't want to do any of this if the length of the sequence fits in the
 *  window, but for now it doesn't hurt; the progress bar just never meets the
 *  criterion for moving to the next page.
 *
 * \todo
 *      -   If playback is disabled (such as by a trigger), then do not update
 *          the page;
 *      -   When it comes back, make sure we're on the correct page;
 *      -   When it stops, put the window back to the beginning, even if the
 *          beginning is not defined as "0".
 */

void
qseqroll::follow_progress ()
{
    if (not_nullptr(frame64()))
        frame64()->follow_progress();
}

}           // namespace seq66

/*
 * qseqroll.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


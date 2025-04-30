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
 * \file          qseqtime.cpp
 *
 *  This module declares/defines the base class for drawing the
 *  time/measures bar at the top of the patterns/sequence editor.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2025-04-30
 * \license       GNU GPLv2 or above
 *
 */

#include <QResizeEvent>
#include <cstring>                      /* std::strcat()                    */

#include "cfg/settings.hpp"             /* seq66::usr() config functions    */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "qseqeditframe64.hpp"          /* seq66::qseqeditframe64 class     */
#include "qseqtime.hpp"                 /* seq66::qseqtime class            */
#include "qt5_helpers.hpp"              /* seq66::qt_timer()                */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Base font size in points.
 */

static const int sc_font_size    = 10;

/*
 *  Marker/label tweaks. The presence of these corrections means we need to
 *  coordinate between GUI elements better :-(. Also note we increased the font
 *  size and had to change locations and size of the boxes for the markers.
 */

static const int s_x_tick_fix    =  2;  /* adjusts vertical grid lines  (2) */
static const int s_time_fix      =  1;  /* seqtime offset from seqroll (9)  */
static const int s_timesig_fix   =  8;  /* time-sig offset from seqroll (18)*/
static const int s_L_timesig_fix =  8;  /* time-sig offset from "L" marker  */
static const int s_L_fix         =  3;  /* adjust position of "o" mark (6)  */
static const int s_o_fix         =  0;  /* adjust position of "o" mark (6)  */
static const int s_LR_box_y      = 10;
static const int s_LR_box_w      = sc_font_size;
static const int s_LR_box_h      = 24;
static const int s_END_fix       = 26;  /* adjust position of "END" box (18)*/
static const int s_END_box_w     = 27;
static const int s_END_box_h     = 24;
static const int s_END_y         = 20;  /* keep the same as s_text_y        */
static const int s_text_y        = 20;
static const int s_ts_text_y     = 19;

/**
 *  Principal constructor.
 */

qseqtime::qseqtime
(
    performer & p,
    sequence & s,
    qseqeditframe64 * frame,
    int zoom,
    QWidget * /* parent // QScrollArea */
) :
    QWidget         (frame),
    qseqbase        (p, s, frame, zoom, c_default_snap),
    m_timer         (nullptr),
    m_font          (),
    m_move_L_marker (false),
    m_expanding     (s.expanded_recording()),
    m_R_marker      ()
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_font.setBold(true);
    m_font.setPointSize(sc_font_size);
    setMouseTracking(true);         /* track mouse movement without a click */
    set_snap(track().snap());
    m_L_marker[0] = 'L'; m_L_marker[1] = 0;
    m_R_marker[0] = 'R'; m_R_marker[1] = 0;
    set_END_marker(s.expanded_recording());
    m_timer = qt_timer(this, "qseqtime", 4, SLOT(conditional_update())); // 2
}

/**
 *  This virtual destructor stops the timer.
 */

qseqtime::~qseqtime ()
{
    if (not_nullptr(m_timer))
        m_timer->stop();
}

void
qseqtime::set_END_marker (bool expanding)
{
    m_expanding = expanding;
    (void) snprintf(m_END_marker, sizeof m_END_marker, "END");
    if (expanding)
        std::strcat(m_END_marker, ">");
}

/**
 *  In an effort to reduce CPU usage when simply idling, this function calls
 *  update() only if necessary.  See qseqbase::check_dirty().
 */

void
qseqtime::conditional_update ()
{
    if (perf().needs_update() || check_dirty())
        update();
}

/**
 *  Draws the time panel.
 */

void
qseqtime::paintEvent (QPaintEvent * qpep)
{
    QRect r = qpep->rect();
    QPainter painter(this);
    QBrush brush(backtime_paint(), Qt::SolidPattern);
    QPen pen(Qt::black);
    pen.setStyle(Qt::SolidLine);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(m_font);
    painter.drawRect                    /* draw the border  */
    (
        c_keyboard_padding_x + 1, 0, width(), height() - 1
    );

    /*
     * The ticks_per_step value needs to be figured out.  Why 6 * m_zoom?  6
     * is the number of pixels in the smallest divisions in the default
     * seqroll background.  This code needs to be put into a function.
     * Actually, odd beats are not allowed in MIDI.
     */

    draw_grid(painter, r);
    draw_markers(painter);
}

/**
 * Code fixed to make sure the whole rectangle is filled with vertical
 * bars.  See qseqroll for more notes.
 *
 *  midipulse endtick = ts.sig_end_tick != 0 ?
 *      ts.sig_end_tick : pix_to_tix(r.x() + r.width());
 *
 *  Vertical line at each measure; number each measure.
 *
 *  Here we could speed things up by avoiding extra calculations if
 *  the time signature has not changed.
 */

void
qseqtime::draw_grid (QPainter & painter, const QRect & r)
{
    QBrush brush(Qt::lightGray, Qt::SolidPattern);
    QPen pen(Qt::black);
    int count = track().time_signature_count();
    midipulse ticks_per_step = z().pulses_per_substep();
    midipulse endtick = z().pix_to_tix(r.x() + r.width());
    int sizeheight = size().height();
    for (int tscount = 0; tscount < count; ++tscount)
    {
        const sequence::timesig & ts = track().get_time_signature(tscount);
        if (ts.sig_beat_width == 0)
            break;

        bool skip_substep = false;
        int bpbar = ts.sig_beats_per_bar;
        int bwidth = ts.sig_beat_width;
        bool valid_bw = is_power_of_2(bwidth);
        midipulse ticks_per_four = z().pulses_per_partial_beat(bpbar, bwidth);
        midipulse ticks_per_beat = midipulse(z().pulses_per_beat(bwidth));
        midipulse ticks_per_bar = z().pulses_per_bar(bpbar, bwidth);
        midipulse starttick = ts.sig_start_tick;
        starttick -= starttick % ticks_per_step;
        for (midipulse tick = starttick; tick < endtick; tick += ticks_per_step)
        {
            int x_offset = xoffset(tick) - scroll_offset_x() + s_x_tick_fix;
            int penwidth;
            enum Qt::PenStyle penstyle;
            if (tick % ticks_per_bar == 0)              /* thick solid line */
            {
                char bar[32];
                int measure = track().measure_number(tick);
                snprintf(bar, sizeof bar, "%d", measure);

                QString qbar(bar);
                pen.setColor(text_time_paint());
                painter.setPen(pen);
                painter.drawText(x_offset + 3, 10, qbar);
                penstyle = measure_pen_style();
                penwidth = measure_pen_width();
                pen.setColor(beat_color());
                skip_substep = true;                    /* don't mess meas. */
            }
            else if (tick % ticks_per_beat == 0)        /* thin every beat  */
            {
                penstyle = beat_pen_style();
                penwidth = beat_pen_width();
                pen.setColor(beat_color());
            }
            else if (tick % ticks_per_four == 0 && valid_bw)
            {
                /*
                 *  Adding this line is problematic if the beat-width is
                 *  not the power-of-two that MIDI requires. Keep the
                 *  display clean, if incomplete.
                 */

                penwidth = 1;
                penstyle = fourth_pen_style();          /* Qt::DashDotLine  */
                pen.setColor(extra_color());            /* beat_color())    */
            }
            else
            {
                if (skip_substep)
                {
                    penwidth = 0;
                    skip_substep = false;
                }
                else
                {
                    penwidth = 1;
                    penstyle = Qt::DotLine;
                    pen.setColor(step_color());
                }
            }
            if (penwidth > 0)
            {
                pen.setWidth(penwidth);
                pen.setStyle(penstyle);
                painter.setPen(pen);
                painter.drawLine(x_offset, 0, x_offset, sizeheight);
            }
        }
    }
}

void
qseqtime::draw_markers (QPainter & painter /* , const QRect & r */ )
{
    int xoff_left = scroll_offset_x();
    int xoff_right = scroll_offset_x() + width();
    midipulse length = track().get_length();
    int end = xoffset(length) - s_END_fix;
    int left = xoffset(perf().get_left_tick()) + s_L_fix;
    int right = xoffset(perf().get_right_tick()) + s_time_fix - s_LR_box_w;
    int now = xoffset(perf().get_tick() % length) + s_o_fix;
    QBrush brush(Qt::lightGray, Qt::SolidPattern);
    QPen pen(Qt::black);
    painter.setPen(pen);

    /*
     * Draw end of seq label, label background.
     */

    if (! perf().is_pattern_playing() && (now != left) && (now != right))
    {
        if (now >= xoff_left && now <= xoff_right)
        {
            pen.setColor(progress_color());
            painter.setPen(pen);
            painter.drawText(now, 18, "o");
        }
    }
    pen.setColor(Qt::black);
    brush.setColor(Qt::black);
    brush.setStyle(Qt::SolidPattern);
    painter.setBrush(brush);
    painter.setPen(pen);
    if (left >= xoff_left && left <= xoff_right)
    {
        painter.setBrush(brush);
        painter.drawRect(left, s_LR_box_y, s_LR_box_w, s_LR_box_h);
        pen.setColor(Qt::white);
        painter.setPen(pen);
        painter.drawText(left + 1, s_text_y, m_L_marker);       /* "L"      */
    }
    pen.setColor(Qt::black);
    painter.setPen(pen);
    painter.drawRect(end - 1, s_LR_box_y, s_END_box_w, s_END_box_h);
    pen.setColor(Qt::white);
    painter.setPen(pen);
    painter.drawText(end + 1, s_END_y, m_END_marker);           /* "END"    */
    if (right >= xoff_left && right <= xoff_right)
    {
        int drend = std::abs(end - right);
        if (drend > s_END_fix)
        {
            pen.setColor(Qt::black);
            painter.setBrush(brush);
            painter.setPen(pen);
            painter.drawRect(right - 1, s_LR_box_y, s_LR_box_w, s_LR_box_h);
            pen.setColor(Qt::white);                            /* text     */
            painter.setPen(pen);
            painter.drawText(right, s_text_y, m_R_marker);      /* "R"      */
        }
    }

    int count = track().time_signature_count();
    for (int tscount = 0; tscount < count; ++tscount)
    {
        const sequence::timesig & ts = track().get_time_signature(tscount);
        if (ts.sig_beat_width == 0)
            break;

        int n = ts.sig_beats_per_bar;
        int d = ts.sig_beat_width;
        midipulse start = ts.sig_start_tick;
        int pos = xoffset(start) + s_timesig_fix;;
        int dlstart = std::abs(pos - left);
        std::string text = std::to_string(n);
        text += "/";
        text += std::to_string(d);
        if (dlstart < 8)
            pos += s_L_timesig_fix;

        pen.setColor(text_time_paint());            /* Qt::black            */
        painter.setPen(pen);
        painter.drawText(pos + 1, s_ts_text_y, qt(text));
    }
}

void
qseqtime::resizeEvent (QResizeEvent * qrep)
{
    QWidget::resizeEvent(qrep);         /* qrep->ignore() */
}

/**
 *  There is a trick to this function that to document and take further
 *  advantage of.
 *
 *  Top half: If clicking in the top half of the time-bar, whether and
 *  L- or R-click, the time is set to that value, and the effect can be
 *  seen in the main window's beat-indicator.
 *
 *  Bottom half: If clicking in the bottom
 *  half, of course, the L or R marker is set, depending on which button
 *  is pressed.  See the qperftime versoin of this function.
 */

void
qseqtime::mousePressEvent (QMouseEvent * event)
{
    midipulse tick = z().pix_to_tix(event->x());
    if (snap() > 0)
        tick -= (tick % snap());

    if (event->y() > height() / 2)                      /* bottom half      */
    {
        bool isctrl = bool(event->modifiers() & Qt::ControlModifier);
        if (event->button() == Qt::LeftButton)          /* move L/R markers */
        {
            if (isctrl)
                perf().set_tick(tick, true);            /* set_start_tick() */
            else
                perf().set_left_tick_seq(tick, snap());

            set_dirty();
        }
        else if (event->button() == Qt::MiddleButton)   /* set start tick   */
        {
            perf().set_tick(tick, true);                /* set_start_tick() */
            set_dirty();
        }
        else if (event->button() == Qt::RightButton)
        {
            perf().set_right_tick_seq(tick, snap());
            set_dirty();
        }
    }
    else                                                /* top half         */
    {
        perf().set_tick(tick, true);                    /* reposition time  */
        set_dirty();
    }
    if (is_dirty())
        frame64()->set_dirty();
}

void
qseqtime::mouseReleaseEvent (QMouseEvent *)
{
    // no code
}

void
qseqtime::mouseMoveEvent(QMouseEvent * event)
{
    setCursor
    (
        event->y() > height() / 2 ? Qt::PointingHandCursor : Qt::UpArrowCursor
    );
}

void
qseqtime::keyPressEvent (QKeyEvent * event)
{
    bool isctrl = bool(event->modifiers() & Qt::ControlModifier);
    if (isctrl)
    {
        /* no code yet */
    }
    else
    {
        bool isshift = bool(event->modifiers() & Qt::ShiftModifier);
        midipulse s = snap() > 0 ? snap() : 1 ;
        if (event->key() == Qt::Key_Left)
        {
            if (m_move_L_marker)
            {
                midipulse tick = perf().get_left_tick() - s;
                perf().set_left_tick_seq(tick, snap());     /* ca 2022-08-17 */
            }
            else
            {
                midipulse tick = perf().get_right_tick() - s;
                perf().set_right_tick_seq(tick, snap());    /* ca 2022-08-17 */
            }
            set_dirty();
            event->accept();
        }
        else if (event->key() == Qt::Key_Right)
        {
            if (m_move_L_marker)
            {
                midipulse tick = perf().get_left_tick() + s;
                perf().set_left_tick_seq(tick, snap());     /* ca 2022-08-17 */
            }
            else
            {
                midipulse tick = perf().get_right_tick() + s;
                perf().set_right_tick_seq(tick, snap());    /* ca 2022-08-17 */
            }
            set_dirty();
            event->accept();
        }
        else if (event->key() == Qt::Key_L)
        {
            if (isshift)
            {
                m_move_L_marker = true;
                event->accept();
            }
        }
        else if (event->key() == Qt::Key_R)
        {
            if (isshift)
            {
                m_move_L_marker = false;
                event->accept();
            }
        }
    }
}

QSize
qseqtime::sizeHint () const
{
    int w = frame64()->width();
    int len = z().tix_to_pix(track().get_length_plus()); /* get_length());      */
    if (len < w)
        len = w;

    len += c_keyboard_padding_x;
    return QSize(len, 22);
}

/**
 *  We don't want the scroll wheel to accidentally scroll the time
 *  horizontally, so this override does nothing but accept() the event.
 *
 *  ignore() just let's the parent handle the event, which allows scrolling to
 *  occur. For issue #3, we have enabled the scroll wheel in the piano roll
 *  [see qscrollmaster::wheelEvent()], but we disable it here. So this is a
 *  partial solution to the issue.
 */

void
qseqtime::wheelEvent (QWheelEvent * qwep)
{
    qwep->accept();
}

}           // namespace seq66

/*
 * qseqtime.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


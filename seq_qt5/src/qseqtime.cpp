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
 * \updates       2023-06-24
 * \license       GNU GPLv2 or above
 *
 */

#include <QResizeEvent>

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

/*
 *  Tweaks. The presence of these corrections means we need to coordinate
 *  between GUI elements better :-(.
 */

static const int s_x_tick_fix    =  2;  /* adjusts vertical grid lines      */
static const int s_time_fix      =  9;  /* seqtime offset from seqroll      */
static const int s_timesig_fix   = 17;  /* time-sig offset from seqroll     */
static const int s_L_timesig_fix = 40;  /* time-sig offset from "L" marker  */
static const int s_o_fix         =  6;  /* adjust position of "o" mark      */
static const int s_end_fix       = 10;  /* adjust position of "END" box     */

/**
 *  Principal constructor.
 */

qseqtime::qseqtime
(
    performer & p,
    sequence & s,
    qseqeditframe64 * frame,
    int zoom,
    QWidget * parent /* QScrollArea */
) :
    QWidget         (parent),
    qseqbase        (p, s, frame, zoom, c_default_snap),
    m_timer         (nullptr),
    m_font          (),
    m_move_L_marker (false)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_font.setBold(true);
    m_font.setPointSize(8);
    setMouseTracking(true);         /* track mouse movement without a click */
    m_timer = qt_timer(this, "qseqtime", 2, SLOT(conditional_update()));
}

/**
 *  This virtual destructor stops the timer.
 */

qseqtime::~qseqtime ()
{
    m_timer->stop();
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

#if defined SEQ66_TIME_SIG_DRAWING
void
qseqtime::paintEvent (QPaintEvent * qpep)
{
    QRect r = qpep->rect();
#else
void
qseqtime::paintEvent (QPaintEvent *)
{
#endif
    QPainter painter(this);
    QBrush brush(Qt::lightGray, Qt::SolidPattern);
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
     *
     * EXPERIMENTAL.  For odd beat widths, use 1 as ticks_per_substep.
     * Actually, odd beats are not allowed in MIDI.
     */

#if defined SEQ66_TIME_SIG_DRAWING

    draw_grid(painter, r);
    draw_markers(painter);
}

#else

    int bpbar = track().get_beats_per_bar();
    int bwidth = track().get_beat_width();
    int measures_per_line = zoom() * bwidth * bpbar * 2;
    int sizeheight = size().height();
    midipulse ticks_per_step = pulses_per_substep(perf().ppqn(), zoom());
    midipulse ticks_per_four = ticks_per_step * 4;
    midipulse ticks_per_beat = (4 * perf().ppqn()) / bwidth;
    midipulse ticks_per_bar = bpbar * ticks_per_beat;
    midipulse starttick = scroll_offset() - (scroll_offset() % ticks_per_step);
    midipulse endtick = pix_to_tix(width()) + scroll_offset();
    if (measures_per_line <= 0)
        measures_per_line = 1;

    pen.setColor(Qt::black);
    painter.setPen(pen);
    for (midipulse tick = starttick; tick <= endtick; tick += ticks_per_step)
    {
        char bar[32];
        int x_offset = xoffset(tick) - scroll_offset_x() + s_x_tick_fix;

        /*
         * Vertical line at each bar; number each bar.
         */

        if (tick % ticks_per_bar == 0)
        {
            pen.setWidth(2);                    // two pixels
            pen.setStyle(Qt::SolidLine);
            painter.setPen(pen);
            painter.drawLine(x_offset, 0, x_offset, sizeheight);
            snprintf(bar, sizeof bar, "%ld", long(tick / ticks_per_bar + 1));

            QString qbar(bar);
            painter.drawText(x_offset + 3, 10, qbar);
        }
        else if (tick % ticks_per_beat == 0)
        {
            pen.setWidth(1);                    // back to one pixel
            pen.setStyle(Qt::SolidLine);
            painter.setPen(pen);
            painter.drawLine(x_offset, 0, x_offset, sizeheight);
        }
        else if (tick % (ticks_per_four) == 0)
        {
            pen.setWidth(1);                    // back to one pixel
            pen.setStyle(Qt::DotLine);
            painter.setPen(pen);
            painter.drawLine(x_offset, 0, x_offset, sizeheight);
        }
    }

    int xoff_left = scroll_offset_x();
    int xoff_right = scroll_offset_x() + width();
    midipulse length = track().get_length();
    int end = position_pixel(length) - s_end_fix;
    int left = position_pixel(perf().get_left_tick()) + s_time_fix;
    int right = position_pixel(perf().get_right_tick());
    int now = position_pixel(perf().get_tick() % length) + s_o_fix;

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
        painter.drawRect(left, 10, 8, 24);          // black background
        pen.setColor(Qt::white);                    // white label text
        painter.setPen(pen);
        painter.drawText(left + 1, 18, "L");
    }
    pen.setColor(Qt::black);
    painter.setPen(pen);
    painter.drawRect(end, 10, 16, 24);              // black background
    pen.setColor(Qt::white);                        // white label text
    painter.setPen(pen);
    painter.drawText(end + 1, 18, "END");
    if (right >= xoff_left && right <= xoff_right)
    {
        pen.setColor(Qt::black);
        painter.setBrush(brush);
        painter.setPen(pen);
        painter.drawRect(right, 10, 8, 24);         // black background
        pen.setColor(Qt::white);                    // white label text
        painter.setPen(pen);
        painter.drawText(right + 2, 18, "R");
    }
}

#endif

#if defined SEQ66_TIME_SIG_DRAWING

void
qseqtime::draw_grid (QPainter & painter, const QRect & r)
{
    QBrush brush(Qt::lightGray, Qt::SolidPattern);
    QPen pen(Qt::black);
    int count = track().time_signature_count();
    for (int tscount = 0; tscount < count; ++tscount)
    {
        const sequence::timesig & ts = track().get_time_signature(tscount);
        if (ts.sig_beat_width == 0)
            break;

        int bpbar = ts.sig_beats_per_bar;
        int bwidth = ts.sig_beat_width;
        int measures_per_line = zoom() * bwidth * bpbar * 2;
        int sizeheight = size().height();
        midipulse ticks_per_step = pulses_per_substep(perf().ppqn(), zoom());
        midipulse ticks_per_four = ticks_per_step * 4;
        midipulse ticks_per_beat = (4 * perf().ppqn()) / bwidth;
        midipulse ticks_per_bar = bpbar * ticks_per_beat;
        midipulse starttick = ts.sig_start_tick;
        midipulse endtick = ts.sig_end_tick != 0 ?
            ts.sig_end_tick : pix_to_tix(r.x() + r.width());

        if (measures_per_line <= 0)
            measures_per_line = 1;

        pen.setColor(Qt::black);
        painter.setPen(pen);
        for (midipulse tick = starttick; tick < endtick; tick += ticks_per_step)
        {
            int x_offset = xoffset(tick) - scroll_offset_x() + s_x_tick_fix;

            /*
             * Vertical line at each measure; number each measure.
             */

            if (tick % ticks_per_bar == 0)          /* thick solid line bar */
            {
                char bar[32];
                int measure = track().measure_number(tick);
                pen.setWidth(2);                    /* two pixels           */
                pen.setStyle(Qt::SolidLine);
                painter.setPen(pen);
                painter.drawLine(x_offset, 0, x_offset, sizeheight);
                snprintf(bar, sizeof bar, "%d", measure);

                QString qbar(bar);
                painter.drawText(x_offset + 3, 10, qbar);
            }
            else if (tick % ticks_per_beat == 0)    /* light on every beat  */
            {
                pen.setWidth(1);                    /* back to one pixel    */
                pen.setStyle(Qt::SolidLine);
                painter.setPen(pen);
                painter.drawLine(x_offset, 0, x_offset, sizeheight);
            }
            else if (tick % (ticks_per_four) == 0)
            {
                pen.setWidth(1);                    /* back to one pixel    */
                pen.setStyle(Qt::SolidLine);
                painter.setPen(pen);
                painter.drawLine(x_offset, 0, x_offset, sizeheight);
            }
            else                                    /* new 2023-06-21       */
            {
                pen.setWidth(1);                    /* back to one pixel    */
                pen.setStyle(Qt::DotLine);
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
    int end = position_pixel(length) - s_end_fix;
    int left = position_pixel(perf().get_left_tick()) + s_time_fix;
    int right = position_pixel(perf().get_right_tick());
    int now = position_pixel(perf().get_tick() % length) + s_o_fix;
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
        painter.drawRect(left, 10, 8, 24);          // black background
        pen.setColor(Qt::white);                    // white label text
        painter.setPen(pen);
        painter.drawText(left + 1, 18, "L");
    }
    pen.setColor(Qt::black);
    painter.setPen(pen);
    painter.drawRect(end, 10, 16, 24);              // black background
    pen.setColor(Qt::white);                        // white label text
    painter.setPen(pen);
    painter.drawText(end + 1, 18, "END");
    if (right >= xoff_left && right <= xoff_right)
    {
        pen.setColor(Qt::black);
        painter.setBrush(brush);
        painter.setPen(pen);
        painter.drawRect(right, 10, 8, 24);         // black background
        pen.setColor(Qt::white);                    // white label text
        painter.setPen(pen);
        painter.drawText(right + 2, 18, "R");
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
        std::string text = std::to_string(n);
        text += "/";
        text += std::to_string(d);
        start += start == perf().get_left_tick() ?
            s_L_timesig_fix : s_timesig_fix ;

        int pos = position_pixel(start);
        pen.setColor(Qt::white);
        pen.setColor(Qt::black);
        painter.setPen(pen);
        painter.drawText(pos + 1, 20, qt(text));
    }
}

#endif

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
    midipulse tick = pix_to_tix(event->x());
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
qseqtime::sizeHint() const
{
    int w = frame64()->width();
    int len = tix_to_pix(track().get_length());
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
#if defined SEQ66_ENABLE_SCROLL_WHEEL_ALL           /* see qscrollmaster.h  */
    qwep->ignore();
#else
    qwep->accept();
#endif
}

}           // namespace seq66

/*
 * qseqtime.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


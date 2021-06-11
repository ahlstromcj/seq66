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
 * \updates       2021-05-20
 * \license       GNU GPLv2 or above
 *
 */

#include <QResizeEvent>

#include "cfg/settings.hpp"             /* seq66::usr() config functions    */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "qseqeditframe64.hpp"          /* seq66::qseqeditframe64 class     */
#include "qseqtime.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 *  Tweaks. The presence of these corrections means we need to coordinate
 *  between GUI elements better.
 */

static const int s_x_tick_fix  =  2;        /* adjusts vertical grid lines  */
static const int s_time_fix    =  9;        /* seqtime offset from seqroll  */
static const int s_o_fix       =  6;        /* adjust position of "O" mark  */
static const int s_end_fix     = 10;        /* adjust position of "END" box */

/**
 *  Principal constructor.
 */

qseqtime::qseqtime
(
    performer & p, seq::pointer seqp,
    int zoom,
    QWidget * parent,       /* QScrollArea */
    qseqeditframe64 * frame
) :
    QWidget                 (parent),
    qseqbase                (p, seqp, zoom, SEQ66_DEFAULT_SNAP),
    m_parent_frame          (frame),
    m_timer                 (nullptr),
    m_font                  ()
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_font.setBold(true);
    m_font.setPointSize(6);
    setMouseTracking(true);         /* track mouse movement without a click */
    m_timer = new QTimer(this);
    m_timer->setInterval(2 * usr().window_redraw_rate());
    QObject::connect
    (
        m_timer, SIGNAL(timeout()),
        this, SLOT(conditional_update())
    );
    m_timer->start();
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

void
qseqtime::paintEvent (QPaintEvent *)
{
    int xwidth = width();       // int yheight = height();
    QPainter painter(this);
    QBrush brush(Qt::lightGray, Qt::SolidPattern);
    QPen pen(Qt::black);
    pen.setStyle(Qt::SolidLine);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(m_font);
    painter.drawRect                    /* draw the border  */
    (
        c_keyboard_padding_x + 1, 0, size().width(), size().height() - 1
    );

    /*
     * The ticks_per_step value needs to be figured out.  Why 6 * m_zoom?  6
     * is the number of pixels in the smallest divisions in the default
     * seqroll background.  This code needs to be put into a function.
     */

    int bpbar = seq_pointer()->get_beats_per_bar();
    int bwidth = seq_pointer()->get_beat_width();
    midipulse ticks_per_beat = (4 * perf().ppqn()) / bwidth;
    midipulse ticks_per_bar = bpbar * ticks_per_beat;
    int measures_per_line = zoom() * bwidth * bpbar * 2;
    int ticks_per_step = pulses_per_substep(perf().ppqn(), zoom());
    int ticks_per_four = ticks_per_step * 4;
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
            painter.drawLine(x_offset, 0, x_offset, size().height());
            snprintf(bar, sizeof bar, "%ld", tick / ticks_per_bar + 1);

            QString qbar(bar);
            painter.drawText(x_offset + 3, 10, qbar);
        }
        else if (tick % ticks_per_beat == 0)
        {
            pen.setWidth(1);                    // back to one pixel
            pen.setStyle(Qt::SolidLine);
            painter.setPen(pen);
            painter.drawLine(x_offset, 0, x_offset, size().height());
        }
        else if (tick % (ticks_per_four) == 0)
        {
            pen.setWidth(1);                    // back to one pixel
            pen.setStyle(Qt::DotLine);
            painter.setPen(pen);
            painter.drawLine(x_offset, 0, x_offset, size().height());
        }
    }

    int xoff_left = scroll_offset_x();
    int xoff_right = scroll_offset_x() + xwidth;
    midipulse length = seq_pointer()->get_length();
    int end = position_pixel(length) - s_end_fix;
    midipulse left = position_pixel(perf().get_left_tick()) + s_time_fix;
    midipulse right = position_pixel(perf().get_right_tick());
    midipulse now = position_pixel(perf().get_tick() % length) + s_o_fix;

    /*
     * Draw end of seq label, label background.
     */

    if (! perf().is_running() && (now != left) && (now != right))
    {
        if (now >= xoff_left && now <= xoff_right)
        {
            pen.setColor(progress_color());
            painter.setPen(pen);
            painter.drawText(now, 18, "O");
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

void
qseqtime::resizeEvent (QResizeEvent * qrep)
{

#if defined SEQ66_PLATFORM_DEBUG_TMI
    static int s_count = 0;
    printf("qseqtime::resizeEvent(%d)\n", s_count++);
#endif

    QWidget::resizeEvent(qrep);         /* qrep->ignore() */
}

/**
 *  There is a trick to this function that to document and take further
 *  advantage of.  If clicking in the top half of the time-bar, whether and
 *  L- or R-click, the time is set to that value, and the effect can be
 *  seen in the main window's beat-indicator.  If clicking in the bottom
 *  half, of course, the L or R marker is set, depending on which button
 *  is pressed.  See the qperftime versoin of this function.
 */

void
qseqtime::mousePressEvent (QMouseEvent * event)
{
    midipulse tick = pix_to_tix(event->x());
    if (snap() > 0)
        tick -= (tick % snap());

    if (event->y() > height() / 2)                      /* see banner note  */
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
    else
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

QSize
qseqtime::sizeHint() const
{
    int len = tix_to_pix(seq_pointer()->get_length()) + 100;
    return QSize(len, 22);
}

}           // namespace seq66

/*
 * qseqtime.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


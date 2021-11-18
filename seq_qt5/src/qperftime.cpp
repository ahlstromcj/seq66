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
 * \file          qperftime.cpp
 *
 *  The time bar shows markers and numbers for the measures of the song,
 *  and also depicts the left and right markers.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2021-11-18
 * \license       GNU GPLv2 or above
 *
 *  Compare to perftime, the Gtkmm-2.4 implementation of this class.
 */

#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

#include "cfg/settings.hpp"
#include "play/performer.hpp"           /* seq66::performer class           */
#include "qperfeditframe64.hpp"
#include "qperftime.hpp"
#include "qt5_helpers.hpp"              /* seq66::qt_timer()                */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Principal constructor.
 */

qperftime::qperftime
(
    performer & p, int zoom, int snap,
    qperfeditframe64 * frame,
    QWidget * parent
) :
    QWidget             (parent),
    qperfbase           (p, zoom, snap, 1, 1 * 1),
    m_parent_frame      (frame),                /* frame64() accessor   */
    m_timer             (nullptr),              /* refresh/redraw timer */
    m_font              (),
    m_move_left         (false)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFocusPolicy(Qt::StrongFocus);
    m_font.setBold(true);
    m_font.setPointSize(6);
    setMouseTracking(true);         /* track mouse movement without a click */
    m_timer = qt_timer(this, "qperftime", 2, SLOT(conditional_update()));
}

/**
 *  This virtual destructor stops the timer.
 */

qperftime::~qperftime ()
{
    m_timer->stop();
}

/**
 *  A timer callback/slot that updates the window only if it needs it.
 *  Without the check for needing to update, it is always called and increase
 *  the CPU load.
 */

void
qperftime::conditional_update ()
{
    if (perf().needs_update() || check_dirty() )
        update();
}

void
qperftime::paintEvent (QPaintEvent * /*qpep*/)
{
    int xwidth = width();
    int yheight = height();
    QPainter painter(this);
    QBrush brush(Qt::lightGray, Qt::SolidPattern);
    QPen pen(fore_color());             // Qt::black);
    pen.setStyle(Qt::SolidLine);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(m_font);
    painter.drawRect(0, 0, xwidth, yheight);
    if (! is_initialized())
        set_initialized();

    /*
     *  Draw the vertical lines for the measures and the beats.
     */

    midipulse tick0 = scroll_offset();
    midipulse windowticks = pix_to_tix(xwidth);
    midipulse tick1 = tick0 + windowticks;
    midipulse tickstep = beat_length();                 /* versus 1         */
    int measure = 0;
    for (midipulse tick = tick0; tick < tick1; tick += tickstep)
    {
        if (measure_length() == 0)
        {
            info_message("qperftime measure-length is 0, cannot draw");
            break;
        }

        int x_pos = position_pixel(tick);
        if (tick % measure_length() == 0)
        {
            pen.setColor(fore_color());                     /* measure */
            pen.setWidth(2);
            pen.setStyle(Qt::SolidLine);
            painter.setPen(pen);
            painter.drawLine(x_pos, 0, x_pos, yheight);

            /*
             * Draw the measure numbers if they will fit.  Determined
             * currently by trial and error.
             */

            if (zoom() >= c_minimum_zoom && zoom() <= c_maximum_zoom)
            {
                QString bar(QString::number(measure + 1));
                pen.setColor(Qt::black);                /* always black     */
                painter.setPen(pen);
                painter.drawText(x_pos + 2, 9, bar);
            }
            ++measure;
        }
        else if (tick % beat_length() == 0)             /* beat             */
        {
            pen.setColor(beat_color());
            pen.setWidth(1);
            pen.setStyle(Qt::DotLine);
            painter.setPen(pen);
            painter.drawLine(x_pos, 0, x_pos, yheight);
        }
    }

    int xoff_left = scroll_offset_x();
    int xoff_right = scroll_offset_x() + xwidth;
    int left = position_pixel(perf().get_left_tick());
    int right = position_pixel(perf().get_right_tick());
    int now = position_pixel(perf().get_tick());
    if (! perf().is_running() && (now != left) && (now != right))
    {
        if (now >= xoff_left && now <= xoff_right)
        {
            pen.setColor(progress_color());
            painter.setPen(pen);
            painter.drawText(now - 2, 18, "o");
        }
    }
    if (left >= xoff_left && left <= xoff_right)
    {
        pen.setColor(Qt::black);
        brush.setColor(Qt::black);
        painter.setBrush(brush);
        painter.setPen(pen);
        painter.drawRect(left, yheight - 12, 7, 10);
        pen.setColor(Qt::white);
        painter.setPen(pen);
        painter.drawText(left + 2, 18, "L");
    }
    if (right >= xoff_left && right <= xoff_right)
    {
        pen.setColor(Qt::black);
        brush.setColor(Qt::black);
        painter.setBrush(brush);
        painter.setPen(pen);
        painter.drawRect(right - 7, yheight - 12, 7, 10);
        pen.setColor(Qt::white);
        painter.setPen(pen);
        painter.drawText(right - 7 + 1, 18, "R");
    }
}

QSize
qperftime::sizeHint () const
{
    int height = 24;
    int width = horizSizeHint();
    int w = frame64()->width();
    if (width < w)
        width = w;

    return QSize(width, height);
}

void
qperftime::keyPressEvent (QKeyEvent * event)
{
    bool isctrl = bool(event->modifiers() & Qt::ControlModifier);   /* Ctrl */
    if (isctrl)
    {
        /* no code yet */
    }
    else
    {
        midipulse s = snap() > 0 ? snap() : 1 ;
        if (event->key() == Qt::Key_Left)
        {
            if (m_move_left)
            {
                midipulse tick = perf().get_left_tick() - s;
                perf().set_left_tick(tick);
            }
            else
            {
                midipulse tick = perf().get_right_tick() - s;
                perf().set_right_tick(tick);
            }
            set_dirty();
            event->accept();
        }
        else if (event->key() == Qt::Key_Right)
        {
            if (m_move_left)
            {
                midipulse tick = perf().get_left_tick() + s;
                perf().set_left_tick(tick);
            }
            else
            {
                midipulse tick = perf().get_right_tick() + s;
                perf().set_right_tick(tick);
            }
            set_dirty();
            event->accept();
        }
        else if (event->key() == Qt::Key_L)
            m_move_left = true;
        else if (event->key() == Qt::Key_R)
            m_move_left = false;
    }
}

/**
 *  There is a trick to this function that to document and take further
 *  advantage of.  If clicking in the top half of the time-bar, whether and
 *  L- or R-click, the time is set to that value, and the effect can be
 *  seen in the main window's beat-indicator.
 *
 *  If clicking in the bottom
 *  half, of course, the L or R marker is set, depending on which button
 *  is pressed.
 *
 *  We need to figure out the difference between setting start-tick and
 *  left-tick.  It might be better to make the two represent the same concept.
 */

void
qperftime::mousePressEvent (QMouseEvent * event)
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
            perf().set_right_tick(tick + snap());
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
qperftime::mouseReleaseEvent (QMouseEvent *)
{
    // no code
}

void
qperftime::mouseMoveEvent (QMouseEvent * event)
{
    setCursor
    (
        event->y() > height() / 2 ? Qt::PointingHandCursor : Qt::UpArrowCursor
    );
}

void
qperftime::set_guides (midipulse snap, midipulse measure)
{
    set_snap(snap);
    m_measure_length = measure;
    if (is_initialized())
        set_dirty();
}

}           // namespace seq66

/*
 * qperftime.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


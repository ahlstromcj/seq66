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
 * \file          qperftime.cpp
 *
 *  The time bar shows markers and numbers for the measures of the song,
 *  and also depicts the left and right markers.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2021-01-04
 * \license       GNU GPLv2 or above
 *
 *  Compare to perftime, the Gtkmm-2.4 implementation of this class.
 */

#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

#include "cfg/settings.hpp"
#include "qperftime.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class performer;

/**
 *  Principal constructor.
 */

qperftime::qperftime
(
    performer & p, int zoom, int snap,
    QWidget * parent
) :
    QWidget             (parent),
    qperfbase           (p, zoom, snap, 1, 1 * 1),
    m_timer             (new QTimer(this)),     /* refresh/redraw timer */
    m_font              (),
    m_move_left         (false)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFocusPolicy(Qt::StrongFocus);
    m_font.setBold(true);
    m_font.setPointSize(6);
    connect
    (
        m_timer, SIGNAL(timeout()), this, SLOT(conditional_update())
    );
    m_timer->setInterval(2 * usr().window_redraw_rate());
    m_timer->start();
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
    QPen pen(Qt::black);
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
    midipulse tickstep = 1;
    int measure = 0;
    for (midipulse tick = tick0; tick < tick1; tick += tickstep)
    {
        if (measure_length() == 0)
        {
            printf("qperftime measure-length is 0, cannot draw\n");
            break;
        }
        if (tick % measure_length() == 0)
        {
            int x_pos = position_pixel(tick);
            pen.setColor(Qt::black);                        /* measure */
            pen.setWidth(2);
            painter.setPen(pen);
            painter.drawLine(x_pos, 0, x_pos, yheight);

            /*
             * Draw the measure numbers if they will fit.  Determined
             * currently by trial and error.
             */

            if (zoom() >= SEQ66_MINIMUM_ZOOM && zoom() <= SEQ66_MAXIMUM_ZOOM)
            {
                QString bar(QString::number(measure + 1));
                pen.setColor(Qt::black);
                painter.setPen(pen);
                painter.drawText(x_pos + 2, 9, bar);
            }
            ++measure;
        }
    }

    int left = position_pixel(perf().get_left_tick());
    int right = position_pixel(perf().get_right_tick());
    if (left >= scroll_offset_x() && left <= scroll_offset_x() + xwidth)
    {
        pen.setColor(Qt::black);
        brush.setColor(Qt::black);
        painter.setBrush(brush);
        painter.setPen(pen);
        painter.drawRect(left, yheight - 12, 7, 10);
        pen.setColor(Qt::white);
        painter.setPen(pen);
        painter.drawText(left + 1, 18, "L");
    }
    if (right >= scroll_offset_x() && right <= scroll_offset_x() + xwidth)
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
        if (event->key() == Qt::Key_Left)
        {
            if (m_move_left)
            {
                midipulse tick = perf().get_left_tick() - snap();
                perf().set_left_tick(tick);
            }
            else
            {
                midipulse tick = perf().get_right_tick() - snap();
                perf().set_right_tick(tick);
            }
            set_dirty();
            event->accept();
        }
        else if (event->key() == Qt::Key_Right)
        {
            if (m_move_left)
            {
                midipulse tick = perf().get_left_tick() + snap();
                perf().set_left_tick(tick);
            }
            else
            {
                midipulse tick = perf().get_right_tick() + snap();
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
    midipulse tick = midipulse(event->x());
    tick *= scale_zoom();
    tick -= (tick % snap());
    if (event->y() > height() * 0.5)                    /* see banner note  */
    {
        bool isctrl = bool(event->modifiers() & Qt::ControlModifier);
        if (event->button() == Qt::LeftButton)          /* move L/R markers */
        {
            if (isctrl)
                perf().set_start_tick(tick);
            else
                perf().set_left_tick(tick);

            set_dirty();
        }
        else if (event->button() == Qt::MiddleButton)    /* set start tick  */
        {
            perf().set_start_tick(tick);
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
        perf().set_tick(tick);                          /* reposition time  */
        set_dirty();
    }
}

void
qperftime::mouseReleaseEvent (QMouseEvent *)
{
    // no code
}

void
qperftime::mouseMoveEvent (QMouseEvent *)
{
    // no code
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


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
 * \file          qseqtime.cpp
 *
 *  This module declares/defines the base class for drawing the
 *  time/measures bar at the top of the patterns/sequence editor.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2019-09-03
 * \license       GNU GPLv2 or above
 *
 */

#include <QResizeEvent>

#include "cfg/settings.hpp"             /* seq66::usr().key_height(), etc.  */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "qseqtime.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *
 */

qseqtime::qseqtime
(
    performer & p,
    seq::pointer seqp,
    int zoom,
    QWidget * parent
) :
    QWidget                 (parent),
    qseqbase                (p, seqp, zoom, SEQ66_DEFAULT_SNAP),
    m_timer                 (nullptr),
    m_font                  ()
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_timer = new QTimer(this);                             // redraw timer !!!
    m_timer->setInterval(2 * usr().window_redraw_rate());   // 50
    QObject::connect(m_timer, SIGNAL(timeout()), this, SLOT(conditional_update()));
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
 *  update() only if necessary.  See qseqbase::needs_update().
 */

void
qseqtime::conditional_update ()
{
    if (check_needs_update())   //  || perf().needs_update())
        update();
}

/**
 *  Draws the time panel.
 */

void
qseqtime::paintEvent (QPaintEvent *)
{

#if defined SEQ66_PLATFORM_DEBUG_TMI
    static int s_count = 0;
    printf("qseqtime::paintEvent(%d) zoom = %d\n", s_count++, zoom());
#endif

    QPainter painter(this);
    QBrush brush(Qt::lightGray, Qt::SolidPattern);
    QPen pen(Qt::black);
    pen.setStyle(Qt::SolidLine);
    m_font.setPointSize(6);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(m_font);

    /*
     * Draw the border
     */

    painter.drawRect
    (
        c_keyboard_padding_x, 0, size().width(), size().height() - 1
    );

    /*
     * The ticks_per_step value needs to be figured out.  Why 6 * m_zoom?  6
     * is the number of pixels in the smallest divisions in the default
     * seqroll background.
     *
     * This code needs to be put into a function.
     */

    int bpbar = seq_pointer()->get_beats_per_bar();
    int bwidth = seq_pointer()->get_beat_width();
    midipulse ticks_per_beat = (4 * ppqn()) / bwidth;
    midipulse ticks_per_bar = bpbar * ticks_per_beat;
    int measures_per_line = zoom() * bwidth * bpbar * 2;
    int ticks_per_step = pulses_per_substep(ppqn(), zoom());    // 6 * zoom()
    midipulse starttick = scroll_offset() - (scroll_offset() % ticks_per_step);
    midipulse endtick = pix_to_tix(width()) + scroll_offset();
    if (measures_per_line <= 0)
        measures_per_line = 1;

    pen.setColor(Qt::black);
    painter.setPen(pen);
    for (midipulse tick = starttick; tick <= endtick; tick += ticks_per_step)
    {
        char bar[32];
        int x_offset = xoffset(tick) - scroll_offset_x();

        /*
         * Vertical line at each bar; number each bar.
         */

        if (tick % ticks_per_bar == 0)
        {
            pen.setWidth(2);                    // two pixels
            painter.setPen(pen);
            painter.drawLine(x_offset, 0, x_offset, size().height());
            snprintf(bar, sizeof bar, "%ld", tick / ticks_per_bar + 1);
            painter.drawText(x_offset + 3, 10, bar);
        }
        else if (tick % ticks_per_beat == 0)
        {
            pen.setWidth(1);                    // back to one pixel
            painter.setPen(pen);
            pen.setStyle(Qt::SolidLine);        // pen.setColor(Qt::DashLine)
            painter.drawLine(x_offset, 0, x_offset, size().height());
        }
    }

    int end_x = xoffset(seq_pointer()->get_length()) - scroll_offset_x() - 20;

    /*
     * Draw end of seq label, label background.
     */

    pen.setColor(Qt::black);
    brush.setColor(Qt::black);
    brush.setStyle(Qt::SolidPattern);
    painter.setBrush(brush);
    painter.setPen(pen);
    painter.drawRect(end_x, 10, 20, 24);            // black background
    pen.setColor(Qt::white);                        // white label text
    painter.setPen(pen);
    painter.drawText(end_x, 18, tr("END"));
}

/**
 *
 */

void
qseqtime::resizeEvent (QResizeEvent * qrep)
{

#if defined SEQ66_PLATFORM_DEBUG_TMI
    static int s_count = 0;
    printf("qseqtime::resizeEvent(%d)\n", s_count++);
#endif

    qrep->ignore();                         /* QWidget::resizeEvent(qrep)   */
}

/**
 *
 */

void
qseqtime::mousePressEvent (QMouseEvent *)
{
    // no code
}

/**
 *
 */

void
qseqtime::mouseReleaseEvent (QMouseEvent *)
{
    // no code
}

/**
 *
 */

void
qseqtime::mouseMoveEvent(QMouseEvent *)
{
    // no code
}

/**
 *
 */

QSize
qseqtime::sizeHint() const
{
    return QSize(xoffset(seq_pointer()->get_length()) + 100, 22);
}

}           // namespace seq66

/*
 * qseqtime.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


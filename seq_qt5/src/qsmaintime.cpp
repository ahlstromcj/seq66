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
 * \file          qsmaintime.cpp
 *
 *  This module declares/defines the base class for the "time" progress
 *  window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2019-07-08
 * \license       GNU GPLv2 or above
 *
 */

#include "play/performer.hpp"           /* seq66::performer class           */
#include "qsmaintime.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *
 */

qsmaintime::qsmaintime
(
    performer & perf,
    QWidget * parent,
    int beatspermeasure,
    int beatwidth
) :
    QWidget             (parent),
    m_main_perf         (perf),
    m_color             (Qt::red),
    m_font              (),
    m_beats_per_measure (beatspermeasure),
    m_beat_width        (beatwidth),
#if defined USE_METRONOME_FADE
    m_alpha             (230),
#endif
    m_last_metro        (0)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_font.setPointSize(9);
    m_font.setBold(true);
#if defined USE_METRONOME_FADE
    m_color.setAlpha(m_alpha);
#endif
}

/**
 *  what about the pens, brushes, etc???
 */

qsmaintime::~qsmaintime ()
{
    // no code
}

/**
 *  Note that the timing of this event is controlled in qsmainwnd::refresh().
 */

void
qsmaintime::paintEvent (QPaintEvent *)
{
    QPainter painter(this);
    QPen pen(Qt::darkGray);
    QBrush brush(Qt::NoBrush);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(m_font);

    midipulse tick = perf().get_tick();
    int boxwidth = (width() - 1) / beats_per_measure();
    int metro = (tick / (perf().ppqn() / 4 * beat_width())) % beats_per_measure();

    /*
     * Flash on beats (i.e. if the metronome has changed, or we've just started
     * playing).
     *
     * \todo
     *      We need to select a color from the palette.
     */

    if (metro != m_last_metro || (tick < 50 && tick > 0))
    {
#if defined USE_METRONOME_FADE
        m_alpha = 230;
#endif
        if (metro == 0)
            m_color.setRgb(255, 50, 50);       // red on first beat in bar
        else
            m_color.setRgb(255, 255, 255);     // white on others
    }
    for (int b = 0; b < beats_per_measure(); ++b)       // draw beat blocks
    {
        int offset_x = boxwidth * b;
        int w = pen.width() - 1;
        if (b == metro && perf().is_running())    // flash on current beat
        {
            brush.setStyle(Qt::SolidPattern);
            pen.setColor(Qt::black);
        }
        else
        {
            brush.setStyle(Qt::NoBrush);
            pen.setColor(Qt::darkGray);
        }

#if defined USE_METRONOME_FADE
        m_color.setAlpha(m_alpha);
#endif
        brush.setColor(m_color);
        painter.setPen(pen);
        painter.setBrush(brush);
        painter.drawRect
        (
            offset_x + w, w, boxwidth - pen.width(), height() - pen.width()
        );
    }
    if (beats_per_measure() < 10)       // draw beat number (if there's space)
    {
        pen.setColor(Qt::black);
        pen.setStyle(Qt::SolidLine);
        painter.setPen(pen);
        painter.drawText
        (
            (metro + 1) * boxwidth - (m_font.pointSize() + 2),
            height() * 0.3 + m_font.pointSize(), QString::number(metro + 1)
        );
    }

#if defined USE_METRONOME_FADE

    /*
     * Lessen alpha on each redraw to have smooth fading done as a factor of
     * the BPM to get useful fades.  However, we have to scale this
     * differently than 300, because Seq66 allows BPM higher than 300.
     */

    m_alpha *= 0.7 - perf().bpm() / SEQ66_MAXIMUM_BPM;     /* 600 */
    if (m_alpha < 0)
        m_alpha = 0;

#endif

    m_last_metro = metro;
}

/**
 *
 */

QSize
qsmaintime::sizeHint () const
{
    return QSize(150, m_font.pointSize() * 2.4);
}

}           // namespace seq66

/*
 * qsmaintime.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


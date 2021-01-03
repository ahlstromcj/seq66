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
 * \file          qseqkeys.cpp
 *
 *  This module declares/defines the base class for the left-side piano of
 *  the pattern/sequence panel.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2020-12-25
 * \license       GNU GPLv2 or above
 *
 *      We've added the feature of a right-click toggling between showing the
 *      main octave values (e.g. "C1" or "C#1") versus the numerical MIDI
 *      values of the keys.
 */

#include <QMouseEvent>
#include <QPainter>                     /* QPainter, QPen, QBrush, ...      */
#include <QResizeEvent>

#include "cfg/settings.hpp"             /* seq66::usr().key_height(), etc.  */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "qseqkeys.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

class performer;

static const int sc_key_x = 20;     // 16;
static const int sc_key_y =  8;

/**
 *  The dimensions and offset of the virtual keyboard at the left of the
 *  piano roll.  These differ from the same members in the Gtkmm seqkeys
 *  class.
 */

static const int sc_keyarea_x = sc_key_x + 15;
static const int sc_keyoffset_x = sc_keyarea_x - sc_key_x;

/**
 *  Principal constructor.
 */

qseqkeys::qseqkeys
(
    performer & p,
    seq::pointer seqp,
    QWidget * parent,
    int keyheight,
    int keyareaheight
) :
    QWidget                 (parent),
    m_performer             (p),
    m_seq                   (seqp),
    m_font                  (),
    m_show_octave_letters   (true),
    m_key                   (0),
    m_key_y                 (keyheight),
    m_key_area_y            (keyareaheight),
    m_is_previewing         (false),
    m_preview_key           (-1)
{
    /*
     * This policy is necessary in order to allow the vertical scrollbar to work.
     */

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
    setMouseTracking(true);
    m_font.setPointSize(6);
}

void
qseqkeys::paintEvent (QPaintEvent *)
{
    QPainter painter(this);
    QPen pen(Qt::black);
    QBrush brush (Qt::SolidPattern);
    pen.setStyle(Qt::SolidLine);
    brush.setColor(Qt::lightGray);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(m_font);

    /*
     * Draw keyboard border.
     */

    painter.drawRect(0, 0, sc_keyarea_x, m_key_area_y); /* see sizeHint()   */
    for (int i = 0; i < c_num_keys; ++i)
    {
        pen.setColor(Qt::black);                /* draw keys                */
        pen.setStyle(Qt::SolidLine);
        brush.setColor(Qt::white);
        brush.setStyle(Qt::SolidPattern);
        painter.setPen(pen);
        painter.setBrush(brush);
        painter.drawRect
        (
            sc_keyoffset_x + 1, m_key_y * i + 1, sc_key_x - 2, m_key_y - 1
        );

        int keyvalue = c_num_keys - i - 1;
        int key = keyvalue % c_octave_size;
        if (is_black_key(key))                  /* draw black keys          */
        {
            pen.setStyle(Qt::SolidLine);
            pen.setColor(Qt::black);
            brush.setColor(Qt::black);
            painter.setPen(pen);
            painter.setBrush(brush);
            painter.drawRect
            (
                sc_keyoffset_x + 1, m_key_y * i + 3,
                sc_key_x - 4, m_key_y - 5
            );
        }
        if (keyvalue == m_preview_key)          /* highlight note preview   */
        {
            brush.setColor(Qt::red);
            pen.setStyle(Qt::NoPen);
            painter.setPen(pen);
            painter.setBrush(brush);
            painter.drawRect
            (
                sc_keyoffset_x + 3, m_key_y * i + 3,
                sc_key_x - 5, m_key_y - 4
            );
        }

        char note[20];
        if (m_show_octave_letters)
        {
            if (key == m_key)
            {
                int octave = (keyvalue / 12) - 1;   /* calculate notes      */
                if (octave < 0)
                    octave *= -1;

                snprintf                            /* see scales.hpp       */
                (
                    note, sizeof note, "%2s%1d",
                    musical_key_name(key).c_str(), octave
                );
                pen.setColor(Qt::black);            /* "Cx" octave labels   */
                pen.setStyle(Qt::SolidLine);
                painter.setPen(pen);
                painter.drawText(2, m_key_y * i + 11, note);
            }
        }
        else
        {
            if ((keyvalue % 2) == 0)
            {
                snprintf(note, sizeof note, "%3d", keyvalue);
                pen.setColor(Qt::black);
                pen.setStyle(Qt::SolidLine);
                painter.setPen(pen);
                painter.drawText(1, m_key_y * i + 9, note);
            }
        }
    }
}

void
qseqkeys::resizeEvent (QResizeEvent * qrep)
{
    qrep->ignore();                         /* QWidget::resizeEvent(qrep)   */
}

void
qseqkeys::mousePressEvent (QMouseEvent * event)
{
    if (event->button() == Qt::LeftButton)
    {
        int note;
        int y = event->y();
        convert_y(y, note);
        set_preview_key(note);
        seq_pointer()->play_note_on(note);
    }
    else if (event->button() == Qt::RightButton)
    {
        m_show_octave_letters = ! m_show_octave_letters;
    }
    update();
}

void
qseqkeys::mouseReleaseEvent (QMouseEvent * event)
{
    if (event->button() == Qt::LeftButton && m_is_previewing)
    {
        seq_pointer()->play_note_off(m_preview_key);
        set_preview_key(-1);
    }
    update();
}

void
qseqkeys::mouseMoveEvent (QMouseEvent * event)
{
    int note;
    int y = event->y();
    convert_y(y, note);
    if (m_is_previewing)
    {
        if (note != m_preview_key)
        {
            seq_pointer()->play_note_off(m_preview_key);
            seq_pointer()->play_note_on(note);
            m_preview_key = note;
        }
    }
    update();
}

/**
 * 31 x 1025
 */

QSize
qseqkeys::sizeHint () const
{
    return QSize(sc_keyarea_x, m_key_area_y);
}

void
qseqkeys::convert_y (int y, int & note)
{
    note = (m_key_area_y - y - 2) / m_key_y;
}

/**
 *  We don't want the scroll wheel to accidentally scroll the piano keys
 *  vertically, so this override does nothing but accept() the event.
 *
 *  ignore() just let's the parent handle the event, which allows scrolling to
 *  occur.
 *
 *  The best solution would be to pass the event along to the qscrollmaster.
 *  But this class doesn't have access to the scroll-master.  We might also try
 *  this in the constructor:
 *
 *      setMouseTracking(false);
 */

void
qseqkeys::wheelEvent (QWheelEvent * ev)
{
    ev->accept();
}

void
qseqkeys::set_preview_key (int key)
{
    m_is_previewing = key >= 0;
    m_preview_key = key;
    update();
}

bool
qseqkeys::set_note_height (int h)
{
    bool result = usr().valid_key_height(h) && h != note_height();
    if (result)
    {
        m_key_y = h;
        m_key_area_y = h * c_num_keys;
        update();
    }
    return result;
}

bool
qseqkeys::v_zoom_in ()
{
    return set_note_height(note_height() + 1);
}

bool
qseqkeys::v_zoom_out ()
{
    return set_note_height(note_height() - 1);
}

bool
qseqkeys::reset_v_zoom ()
{
    return set_note_height(usr().key_height());
}

}           // namespace seq66

/*
 * qseqkeys.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


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
 * \file          qseqkeys.cpp
 *
 *  This module declares/defines the base class for the left-side piano of
 *  the pattern/sequence panel.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2024-12-17
 * \license       GNU GPLv2 or above
 *
 *  We've added the feature of a right-click toggling between showing the main
 *  octave values (e.g. "C1" or "C#1") versus the numerical MIDI values of the
 *  keys.
 */

#include <QMouseEvent>
#include <QPainter>                     /* QPainter, QPen, QBrush, ...      */
#include <QResizeEvent>

#include "cfg/settings.hpp"             /* seq66::usr().key_height(), etc.  */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "qseqeditframe64.hpp"          /* seq66::qseqeditframe64 class     */
#include "qseqkeys.hpp"                 /* seq66::qseqkeys panel            */
#include "qt5_helpers.hpp"              /* seq66::qt() string conversion    */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

class performer;

/**
 *  The width and thickness of the keys drawn on the GUI.
 *
 *  Unused:
 *
 *      static const int sc_key_y =  8;
 */

static const int sc_key_x = 22;

/**
 *  The dimensions and offset of the virtual keyboard at the left of the piano
 *  roll.  We also add a y offset to line up the keys and the piano roll grid.
 */

static const int sc_keyoffset_x = 20;
static const int sc_keyarea_x   = sc_key_x + sc_keyoffset_x + 2;

/**
 *  Manifest constant.
 */

static const int sc_null_key        = (-1);
static const int sc_border_width    = 2;

/**
 *  Principal constructor.
 */

qseqkeys::qseqkeys
(
    performer & p,
    sequence & s,
    qseqeditframe64 * frame,
    QWidget * parent,
    int keyheight,
    int keyareaheight
) :
    QWidget             (parent),
    qseqbase
    (
        p, s, frame, c_default_seq_zoom, c_default_snap,
        keyheight, keyareaheight
    ),
    m_font              (),
    m_show_key_names    (usr().key_view()),     /* initial default      */
    m_key               (0),
    m_key_y             (keyheight),            /* note_height()        */
    m_key_area_y        (keyareaheight),        /* total_height()       */
    m_preview_color     (progress_paint()),     /* preview_color()      */
    m_white_key_color   (white_key_paint()),    /* white_color()        */
    m_black_key_color   (black_key_paint()),    /* black_color()        */
    m_is_previewing     (false),                /* previewing()         */
    m_preview_on        (false),                /* preview_on()         */
    m_preview_key       (sc_null_key)           /* preview_key()        */
{
    /*
     * This policy is necessary in order to allow the vertical scrollbar to
     * work.
     */

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
    setMouseTracking(true);
    m_font.setPointSize(8);                     /* 6, 7 are tiny        */
}

/*
 *  Provides additional vertical padding to adjust y values for notes, then
 *  subtract the built-in key offset.
 */

void
qseqkeys::paintEvent (QPaintEvent *)
{
    QPainter painter(this);
    QPen pen(Qt::black);
    QBrush brush (Qt::SolidPattern);
    const int keyx = sc_keyoffset_x + 1;
    const int numx = 1;                                     /* was 2        */
    int keyy = 0;
    int numy = 8;
    const int nh = note_height();
    const int nh_1 = nh - 1;
    const int nh_4 = nh - 4;
    const int nh_3 = nh - 3;
    const int key_x_3 = sc_key_x - 3;
    const int key_x_6 = sc_key_x - 6;
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(sc_border_width);
    brush.setColor(backkeys_paint());
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.drawRect(0, 0, sc_keyarea_x, total_height());   /* border       */
    pen.setWidth(1);
    for (int i = 0; i < c_notes_count; ++i, keyy += nh, numy += nh)
    {
        int keyvalue = c_notes_count - i - 1;
        int key = keyvalue % c_octave_size;
        pen.setStyle(Qt::SolidLine);
        pen.setColor(Qt::black);
        brush.setStyle(Qt::SolidPattern);
        if (is_black_key(key))                              /* black keys   */
        {
            /*
             * Inverting the keys is confusing, so alway use black.
             * brush.setColor(black_color());
             */

            brush.setColor(Qt::black);
            painter.setPen(pen);
            painter.setBrush(brush);
            painter.drawRect(keyx, keyy + 1, key_x_6, nh_3);
        }
        else
        {
            /*
             * Inverting the keys is confusing, so alway use white.
             * brush.setColor(white_color());
             */

            brush.setColor(Qt::white);
            painter.setPen(pen);
            painter.setBrush(brush);
            painter.drawRect(keyx, keyy, sc_key_x, nh_1);
        }
        if (is_preview_key(keyvalue))                       /* preview note */
        {
            brush.setColor(preview_color());                /* Qt::red      */
            pen.setStyle(Qt::NoPen);
            painter.setPen(pen);
            painter.setBrush(brush);
            painter.drawRect(keyx + 2, keyy + 2, key_x_3, nh_4);
        }

        std::string note;
        char notebuf[8];
        m_font.setBold(key == m_key);                       /* octave label */
        painter.setFont(m_font);
        pen.setColor(text_keys_paint());                    /* Qt::black)   */
        painter.setPen(pen);
        switch (m_show_key_names)
        {
        case showkeys::octave_letters:

            if (key == m_key)
            {
                note = musical_note_name(keyvalue);
                painter.drawText(numx, numy, qt(note));
            }
            break;

        case showkeys::even_letters:

            if ((keyvalue % 2) == 0)
            {
                note = musical_note_name(keyvalue);
                painter.drawText(numx, numy, qt(note));
            }
            break;

        case showkeys::all_letters:

            note = musical_note_name(keyvalue);
            painter.drawText(numx, numy, qt(note));
            break;

        case showkeys::even_numbers:

            if ((keyvalue % 2) == 0)
            {
                snprintf(notebuf, sizeof notebuf, "%3d", keyvalue);
                painter.drawText(numx, numy, notebuf);
            }
            break;

        case showkeys::all_numbers:

            snprintf(notebuf, sizeof notebuf, "%3d", keyvalue);
            painter.drawText(numx, numy, notebuf);
            break;
        }
    }
}

void
qseqkeys::mousePressEvent (QMouseEvent * event)
{
    if (event->button() == Qt::LeftButton)
    {
        int note;
        int y = event->y();
        convert_y(y, note);
        preview_key(note);
        preview_on(true);
        track().play_note_on(note);
    }
    else if (event->button() == Qt::RightButton)
    {
        preview_key(sc_null_key);
        switch (m_show_key_names)
        {
        case showkeys::octave_letters:
            m_show_key_names = showkeys::even_letters;
            break;

        case showkeys::even_letters:
            m_show_key_names = showkeys::all_letters;
            break;

        case showkeys::all_letters:
            m_show_key_names = showkeys::even_numbers;
            break;

        case showkeys::even_numbers:
            m_show_key_names = showkeys::all_numbers;
            break;

        case showkeys::all_numbers:
            m_show_key_names = showkeys::octave_letters;
            break;
        }
    }
    update();
}

void
qseqkeys::mouseReleaseEvent (QMouseEvent * event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (previewing())
        {
            if (preview_on())
            {
                track().play_note_off(preview_key());
                preview_on(false);
            }
            preview_key(sc_null_key);
        }
    }
    update();
}

void
qseqkeys::mouseMoveEvent (QMouseEvent * /* event */)
{
    if (previewing())
    {
        if (preview_on())
        {
            track().play_note_off(preview_key());
            preview_on(false);
        }
        preview_key(sc_null_key);
    }
    update();
}

QSize
qseqkeys::sizeHint () const
{
    int w = sc_keyarea_x;
    int h = total_height();
    return QSize(w, h);
}

void
qseqkeys::convert_y (int y, int & note)
{
    note = (total_height() - y - 2) / note_height();
}

/**
 *  We don't want the scroll wheel to accidentally scroll the piano keys
 *  vertically, so this override does nothing but accept() the event.
 *
 *  ignore() just let's the parent handle the event, which allows scrolling to
 *  occur. For issue #3, we have enabled the scroll wheel in the piano roll
 *  [see qscrollmaster::wheelEvent()], but we disable it here. So this is a
 *  partial solution to the issue.
 *
 *  The best solution would be to pass the event along to the qscrollmaster.
 *  But this class doesn't have access to the scroll-master.  We might also
 *  try this in the constructor:
 *
 *      setMouseTracking(false);
 */

void
qseqkeys::wheelEvent (QWheelEvent * qwep)
{
    qwep->accept();
}

void
qseqkeys::preview_key (int key)
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
        note_height(h);
        total_height(h * c_notes_count);
        resize(sc_keyarea_x, total_height());
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

void
qseqkeys::set_key (int k)
{
    if (legal_key(k))
    {
        m_key = k;
        update();
    }
}

}           // namespace seq66

/*
 * qseqkeys.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


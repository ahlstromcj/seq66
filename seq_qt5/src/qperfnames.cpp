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
 * \file          qperfnames.cpp
 *
 *  This module declares/defines the base class for performance names.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2021-05-21
 * \license       GNU GPLv2 or above
 *
 *  This module is almost exclusively user-interface code.  There are some
 *  pointers yet that could be replaced by references, and a number of minor
 *  issues that could be fixed.
 *
 *  Adjustments to the performance window can be made with the highlighting
 *  option.  Sequences that don't have events show up as black-on-yellow.
 *  This feature is enabled by default.  To disable this feature, configure
 *  the build with the "--disable-highlight" option.
 *
 * \todo
 *      When bringing up this dialog, and starting play from it, some
 *      extra horizontal lines are drawn for some of the sequences.  This
 *      happens even in seq64, so this is long standing behavior.  Is it
 *      useful, and how?  Where is it done?  In perfroll?
 */

#include <QMouseEvent>
#include <QPainter>
#include <QPen>

#include "play/performer.hpp"
#include "gui_palette_qt5.hpp"
#include "qperfnames.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class performer;

/**
 *  Alpha for coloring the names brightly, or not.
 */

const int s_alpha_bright = 255;
const int s_alpha_normal = 100;

/**
 *  Default font size.
 */

const int s_pointsize = 7;

/**
 * Sequence labels for the side of the song editor
 */

qperfnames::qperfnames (performer & p, QWidget * parent)
 :
    QWidget             (parent),
    qperfbase           (p),
    m_font              ("Monospace"),
    m_nametext_x        (6 * 2 + 6 * 20),       // not used!
    m_nametext_y        (c_names_y),
    m_set_text_y        (m_nametext_y * p.seqs_in_set() / 2),
    m_preview_color     (progress_paint()),
    m_is_previewing     (false),
    m_preview_row       (-1)
{
    /*
     * This policy is necessary in order to allow the vertical scrollbar to
     * work.
     */

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
    setFocusPolicy(Qt::StrongFocus);
    m_font.setStyleHint(QFont::Monospace);
    m_font.setLetterSpacing(QFont::AbsoluteSpacing, 1);
    m_font.setBold(true);
    m_font.setPointSize(s_pointsize);
    m_preview_color.setAlpha(s_alpha_normal);
}

/**
 *  A pass-along function for the parent frame to call.
 */

void
qperfnames::reupdate ()
{
    update();
}

/**
 *  Draws the sequence names down the side of the performance roll.
 */

void
qperfnames::paintEvent (QPaintEvent *)
{
    int y_s = 0;
    int y_f = height() / m_nametext_y;
    QPainter painter(this);
    QPen pen(Qt::blue);  // (Qt::black);
    QBrush brush(Qt::lightGray);
    pen.setStyle(Qt::SolidLine);
    brush.setStyle((Qt::SolidPattern));
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(m_font);
    painter.drawRect(0, 0, width(), height() - 1);      // rectangle and border
    for (int y = y_s; y <= y_f; ++y)
    {
        int seq_id = y;
        if (seq_id < int(perf().sequence_max()))        // sequences_in_sets()?
        {
            int rect_x = 6 * 2 + 2;
            int rect_y = m_nametext_y * seq_id;
            int rect_w = c_names_x - 15;
            int text_y = rect_y + m_set_text_y;
            if (seq_id % perf().seqs_in_set() == 0)     // if 1st seq in bank
            {
                char ss[16];
                int bank_id = seq_id / perf().seqs_in_set();
                snprintf(ss, sizeof ss, "%2d", bank_id);
                pen.setColor(Qt::black);                // black boxes, each bank
                brush.setColor(Qt::black);
                brush.setStyle(Qt::SolidPattern);
                painter.setPen(pen);
                painter.setBrush(brush);
                painter.drawRect(1, name_y(seq_id) + 1, 13, m_nametext_y - 1);

                QString bankss(ss);
                pen.setColor(Qt::white);                // for bank number
                painter.setPen(pen);
                painter.drawText(1, rect_y + 15, bankss);
                pen.setColor(Qt::black);                // bank name sideways
                painter.setPen(pen);
                painter.save();                         // {
                QString bankname(perf().bank_name(bank_id).c_str());
                painter.translate(12, text_y + bankname.length() * 4);
                painter.rotate(270);
                painter.drawText(0, 0, bankname);
                painter.restore();                      // }
            }
            if (perf().is_seq_active(seq_id))
            {
                std::string sname = perf().sequence_label(seq_id); // seq name
                seq::pointer s = perf().get_sequence(seq_id);
                bool muted = s->get_song_mute();
                char name[64];
                snprintf
                (
                    name, sizeof name, "%-14.14s  %2d",
                    s->name().c_str(), int(s->seq_midi_channel()) + 1
                );

                QString chinfo(name);
                if (muted)
                {
                    brush.setColor(grey_color());           // Qt::black
                    brush.setStyle(Qt::SolidPattern);
                    painter.setBrush(brush);
                    painter.drawRect(rect_x, rect_y, rect_w, m_nametext_y);
                    pen.setColor(back_color());             // Qt::white
                }
                else
                {
                    int c = s->color();
                    Color backcolor = get_color_fix(PaletteColor(c));
                    int alpha = seq_id == m_preview_row ?
                        s_alpha_bright : s_alpha_normal ;

#if defined USE_PREVIEW_COLOR_VS_ALPHA_HIGHTLIGHT
                    if (seq_id == m_preview_row)
                        backcolor = preview_color();
#endif
                    backcolor.setAlpha(alpha);
                    brush.setColor(backcolor);          // Qt::white

                    brush.setStyle(Qt::SolidPattern);
                    painter.setBrush(brush);
                    painter.drawRect(rect_x, rect_y, rect_w, m_nametext_y);
                    pen.setColor(fore_color());             // Qt::black
                }
                painter.setPen(pen);
                painter.drawText(18, rect_y + 10, chinfo);
                painter.drawText(18, rect_y + 20, sname.c_str());
                painter.drawRect(name_x(2), name_y(seq_id), 9, m_nametext_y);
                painter.drawText(name_x(4), name_y(seq_id) + 15, QString("M"));
            }
            else
            {
                pen.setStyle(Qt::SolidLine);
                pen.setColor(Qt::black);
                brush.setColor(Qt::lightGray);
                painter.setPen(pen);        /* fill seq label background    */
                painter.setBrush(brush);
                painter.drawRect(rect_x, rect_y, rect_w, m_nametext_y);
            }
        }
    }
}

QSize
qperfnames::sizeHint () const
{
    int count = perf().sequences_in_sets();
    int height = m_nametext_y * count;
    return QSize(c_names_x, height);
}

/**
 *  Converts a y-value into a sequence number and returns it.  Used in
 *  figuring out which sequence to mute/unmute in the performance editor.
 *
 * \param y
 *      The y value (within the vertical limits of the perfnames column to the
 *      left of the performance editor's piano roll.
 *
 * \return
 *      Returns the sequence number corresponding to the y value.
 */

int
qperfnames::convert_y (int y)
{
    int seqlimit = perf().sequences_in_sets();      // perf().sequence_max()
    int seq = y / m_nametext_y;
    if (seq >= seqlimit)
        seq = seqlimit - 1;
    else if (seq < 0)
        seq = 0;

    return seq;
}

void
qperfnames::keyPressEvent (QKeyEvent * event)
{
    bool ignored = false;
    bool isctrl = bool(event->modifiers() & Qt::ControlModifier);   /* Ctrl */
    if (isctrl)
    {
        /* no code yet */
    }
    else
    {
        if (event->key() == Qt::Key_Left)
        {
            ignored = true;
        }
        else if (event->key() == Qt::Key_Right)
        {
            ignored = true;
        }
        else if (event->key() == Qt::Key_Up)
        {
            ignored = true;
        }
        else if (event->key() == Qt::Key_Down)
        {
            ignored = true;
        }
    }
    if (ignored)
        event->accept();
    else
        QWidget::keyPressEvent(event);
}

void
qperfnames::mousePressEvent (QMouseEvent * ev)
{
    int y = int(ev->y());
    int seqnum = convert_y(y);
    if (ev->button() == Qt::LeftButton)
    {
        bool isshiftkey = (ev->modifiers() & Qt::ShiftModifier) != 0;
        (void) perf().toggle_sequences(seqnum, isshiftkey);
        update();
    }
}

void
qperfnames::mouseReleaseEvent (QMouseEvent * /*ev*/)
{
    // no code; add a call to update() if a change is made
}

void
qperfnames::mouseMoveEvent (QMouseEvent * /*event*/)
{
    // no code; add a call to update() if a change is made
}

void
qperfnames::set_preview_row (int row)
{
    m_is_previewing = row >= 0;
    m_preview_row = row;
    update();
}

}           // namespace seq66

/*
 * qperfnames.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


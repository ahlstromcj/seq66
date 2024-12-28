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
 * \file          qperfnames.cpp
 *
 *  This module declares/defines the base class for performance names.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2024-12-28
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
 */

#include <QMouseEvent>
#include <QPainter>
#include <QPen>

#include "cfg/settings.hpp"             /* seq66::usr() config functions    */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "gui_palette_qt5.hpp"          /* seq66::gui_palette_qt5 class     */
#include "qperfnames.hpp"               /* seq66::qperfnames panel class    */
#include "qt5_helpers.hpp"              /* seq66::qt() string conversion    */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

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
 *  Sequence labels for the side of the song editor.  This constructor
 *  uses the default zoom, snap, unit height, and total height values of
 *  qperfbase.
 */

qperfnames::qperfnames (performer & p, QWidget * parent) :
    QWidget             (parent),
    qperfbase           (p),
    m_font              ("Monospace"),
    m_nametext_x        (6 * 2 + 6 * 20),                   /* see name_x() */
    m_preview_color     (progress_paint()),
    m_is_previewing     (false),
    m_preview_row       (-1),
    m_use_gradient      (gui_use_gradient_brush())
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
    int h = track_height();
    int y_s = 0;
    int y_f = height() / h;
    int set_count = setmaster::Size();                  /* number of rows   */
    QPainter painter(this);
    QPen pen(text_paint());                             /* fore_color()     */
    QBrush brush(backnames_paint(), Qt::SolidPattern);
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(horiz_pen_width());
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(m_font);
    painter.drawRect(0, 0, width(), height() - 1);      // rectangle border
    int set_y = h * set_count / 2;
    for (int y = y_s; y <= y_f; ++y)
    {
        int seq_id = y;
        if (seq_id < int(perf().sequence_max()))        // or use set_count?
        {
            int rect_x = 6 * 2 + 2;
            int rect_y = h * seq_id;
            int rect_w = c_names_x - 15;
            if ((seq_id % set_count) == 0)              // 1st seq in bank?
            {
                char ss[16];
                int bank_id = seq_id / set_count;
                snprintf(ss, sizeof ss, "%2d", bank_id);
                pen.setColor(fore_color());             // black bank boxes
                brush.setColor(back_color());           // Qt::black
                brush.setStyle(Qt::SolidPattern);
                painter.setPen(pen);
                painter.setBrush(brush);
                painter.drawRect(1, name_y(seq_id) + 1, 13, h - 1);

                int text_y = rect_y + set_y;
                QString bankss(ss);

                /*
                 * pen.setColor(fore_color());          // for bank number
                 * pen.setColor(text_paint());          // for bank number
                 */

                pen.setColor(text_names_paint());       // for bank number
                painter.setPen(pen);
                painter.drawText(1, rect_y + 10, bankss);

                /*
                 * pen.setColor(Qt::black);             // bank name sideways
                 * painter.setPen(pen);
                 */

                painter.save();                         // {

                QString bank(qt(perf().set_name(bank_id)));
                painter.translate(12, text_y + bank.length() * 4);
                painter.rotate(270);
                painter.drawText(0, 0, bank);
                painter.restore();                      // }
            }
            if (perf().is_seq_active(seq_id))
            {
                std::string seq_name = perf().sequence_label(seq_id);
                seq::pointer s = perf().get_sequence(seq_id);
                bool muted = s->get_song_mute();        // ! s->armed()
                char name[64];
                snprintf
                (
                    name, sizeof name, "%-14.14s %3d",
                    s->name().c_str(), s->measures()
                );

                QString chinfo(name);
                if (use_gradient())
                {
                    QLinearGradient grad
                    (
                        rect_x, rect_y, rect_x, rect_y + h + 1
                    );
                    if (muted)
                    {
                        Color backcolor = grey_color();
                        grad.setColorAt(0.01, backcolor.darker());
                        grad.setColorAt(0.5, backcolor.lighter());
                        grad.setColorAt(0.99, backcolor.darker());
                    }
                    else
                    {
                        int c = s->color();
                        Color backcolor = get_color_fix(PaletteColor(c));
                        int alpha = seq_id == m_preview_row ?
                            s_alpha_bright : s_alpha_normal ;

                        backcolor.setAlpha(alpha);
                        grad.setColorAt(0.01, backcolor.darker(150));
                        grad.setColorAt(0.5, backcolor.lighter());
                        grad.setColorAt(0.99, backcolor.darker(150));
                    }
                    painter.fillRect
                    (
                        rect_x +2 , rect_y +1, rect_w -2, h - 1, grad
                    );
                    pen.setColor(fore_color());

                    /*
                     * 0.99.1: Draw a rectangle around the gradient.
                     */

                    pen.setStyle(Qt::SolidLine);
                    pen.setColor(fore_color());
                    painter.setPen(pen);
                    brush.setStyle(Qt::NoBrush);
                    painter.setBrush(brush);
                    painter.drawRect(rect_x, rect_y, rect_w, h);
                }
                else
                {
                    if (muted)
                    {
                        brush.setColor(grey_color());
                        brush.setStyle(Qt::SolidPattern);
                        painter.setBrush(brush);
                        painter.drawRect(rect_x, rect_y, rect_w, h);
                        pen.setColor(fore_color());
                    }
                    else
                    {
                        int c = s->color();
                        Color backcolor = get_color_fix(PaletteColor(c));
                        int alpha = seq_id == m_preview_row ?
                            s_alpha_bright : s_alpha_normal ;

                        backcolor.setAlpha(alpha);
                        brush.setColor(backcolor);
                        brush.setStyle(Qt::SolidPattern);
                        painter.setBrush(brush);
                        painter.drawRect(rect_x, rect_y, rect_w, h);
                        pen.setColor(fore_color());
                    }
                }

                /*
                 * painter.setPen(pen);
                 * painter.setPen(text_paint());
                 */

                painter.setPen(text_names_paint());
                painter.drawText(18, rect_y + 9, chinfo);
                if (! track_thin())
                {
                    char temp[8];
                    snprintf(temp, sizeof temp, "%3d", s->trigger_count());
                    painter.drawText(18, rect_y + 19, qt(seq_name));
                    painter.drawText(114, rect_y + 19, temp);
                }
                if (use_gradient())
                {
                    if (muted)
                    {
                        brush.setColor(grey_color());
                        brush.setStyle(Qt::SolidPattern);
                        painter.setBrush(brush);
                    }
                    else
                    {
                        int c = s->color();
                        Color backcolor = get_color_fix(PaletteColor(c));
                        int alpha = seq_id == m_preview_row ?
                            s_alpha_bright : s_alpha_normal ;

                        backcolor.setAlpha(alpha);
                        brush.setColor(backcolor);
                        brush.setStyle(Qt::SolidPattern);
                        painter.setBrush(brush);
                    }
                }
                painter.drawRect(name_x(2), name_y(seq_id), 9, h);
                painter.drawText(name_x(4), name_y(seq_id) + 9, QString("M"));
            }
            else
            {
                pen.setStyle(Qt::SolidLine);
                pen.setColor(fore_color());
                brush.setColor(backnames_paint());      /* Qt::lightGray)   */
                painter.setPen(pen);                    /* fill background  */
                painter.setBrush(brush);
                painter.drawRect(rect_x, rect_y, rect_w, h);
            }
        }
    }
}

QSize
qperfnames::sizeHint () const
{
int count = perf().sequences_in_sets();
    int height = track_height() * count;
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
    int seq = y / track_height();
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
        int keyvalue = event->key();
        if
        (
            keyvalue == Qt::Key_Left || keyvalue == Qt::Key_Right ||
            keyvalue == Qt::Key_Up || keyvalue == Qt::Key_Down
        )
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
    int seqno = convert_y(y);
    if (ev->button() == Qt::LeftButton)
    {
        bool isshiftkey = (ev->modifiers() & Qt::ShiftModifier) != 0;
        (void) perf().toggle_sequences(seqno, isshiftkey);
        update();
    }
    else if (ev->button() == Qt::RightButton)
    {
        (void) perf().sequence_playing_toggle(seqno);
        update();
    }
}

/**
 *  One issue is that a double-click yields a mouse-press and an
 *  mouse-double-click event, in that order. Note the krufty solution
 *  down at the end of this function.
 */

void
qperfnames::mouseDoubleClickEvent (QMouseEvent * ev)
{
    if (rc().allow_click_edit())
    {
        int seqno = convert_y(int(ev->y()));
        bool active = perf().is_seq_active(seqno);
        emit signal_call_editor_ex(seqno, active);
        (void) perf().toggle_sequences(seqno, false);           /* krufty ! */
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

/**
 *  Prevent qperfnames from scrolling on its own via the scroll-wheel event;
 *  this override does nothing but accept() the event.
 *
 *  ignore() just let's the parent handle the event, which allows scrolling to
 *  occur. For issue #3, we have enabled the scroll wheel in the piano roll
 *  [see qscrollmaster::wheelEvent()], but we disable it here. So this is a
 *  partial solution to the issue.
 */

void
qperfnames::wheelEvent (QWheelEvent * ev)
{
    ev->accept();
}

}           // namespace seq66

/*
 * qperfnames.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

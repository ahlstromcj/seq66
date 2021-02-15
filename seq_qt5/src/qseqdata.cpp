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
 * \file          qseqdata.cpp
 *
 *  This module declares/defines the base class for plastering
 *  pattern/sequence data information in the data area of the pattern
 *  editor.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2021-01-11
 * \license       GNU GPLv2 or above
 *
 *  The data pane is the drawing-area below the seqedit's event area, and
 *  contains vertical lines whose height matches the value of each data event.
 *  The height of the vertical lines is editable via the mouse.
 */

#include "cfg/settings.hpp"             /* seq66::usr() config functions    */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "qseqdata.hpp"                 /* seq66::qseqdata                  */
#include "util/rect.hpp"                /* seq66::rect::xy_to_rect_get()    */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 * Tweaks
 */

static const int s_x_data_fix =  8;

/**
 *  Principal constructor.
 */

qseqdata::qseqdata
(
    performer & p,
    seq::pointer seqp,
    int zoom,
    int snap,
    QWidget * parent,
    int xpadding
) :
    QWidget                 (parent),
    qseqbase                (p, seqp, zoom, snap),
    performer::callbacks    (p),
    m_timer                 (nullptr),
    m_font                  (),
    m_keyboard_padding_x    (xpadding + s_x_data_fix),
    m_status                (EVENT_NOTE_ON),
    m_cc                    (1),                /* modulation */
    m_line_adjust           (false),
    m_relative_adjust       (false),
    m_dragging              (false)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_font.setPointSize(6);
    cb_perf().enregister(this);
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(conditional_update()));
    m_timer->setInterval(4 * usr().window_redraw_rate());
    m_timer->start();
}

/**
 *  This virtual destructor stops the timer.
 */

qseqdata::~qseqdata ()
{
    m_timer->stop();
    cb_perf().unregister(this);
}

/**
 *  In an effort to reduce CPU usage when simply idling, this function calls
 *  update() only if necessary.  See qseqbase::dirty().
 */

void
qseqdata::conditional_update ()
{
    if (check_dirty())
    {
        update();
    }
}

bool
qseqdata::on_ui_change (seq::number seqno)
{
    if (seqno == seq_pointer()->seq_number())
        update();

    return true;
}

QSize
qseqdata::sizeHint () const
{
    return QSize(xoffset(seq_pointer()->get_length()) + 100, c_dataarea_y);
}

/**
 *
 * \note
 *      We had a weird issue with the following function, where d1 would be
 *      assigned a value inside the function, but d1 was 0 afterward.  So we
 *      decided to bite the bullet and ditch this call:
 *
 * seq_pointer()->get_next_event_kepler(m_status, m_cc, tick, d0, d1, selected)
 *
 *      Instead, we create an iterator and use sequence::get_next_event_ex().
 */

void
qseqdata::paintEvent (QPaintEvent * qpep)
{
    QRect r = qpep->rect();
    QPainter painter(this);
    QBrush brush(grey_color(), Qt::SolidPattern);
    QPen pen(Qt::black);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(m_font);
    painter.drawRect(0, 0, width() - 1, height() - 1);  /* data-box border  */

    char digits[4];
    seq::pointer s = seq_pointer();
    midipulse start_tick = pix_to_tix(r.x());
    midipulse end_tick = start_tick + pix_to_tix(r.width());

#if defined SEQ66_USE_TRY_CATCH

    /*
     * The try-catch seems to improve the robustness of the app when
     * recording a DD-11 drum pattern over and over while also drawing a
     * ton of notes.
     */

    try     /* EXPERIMENTAL: 2021-02-15 */
    {
#endif
        for (auto cev = s->ex_iterator(); s->ex_iterator_valid(cev); ++cev)
        {
            if (! s->get_next_event_match(m_status, m_cc, cev)) /* side-effect */
                break;

            midipulse tick = cev->timestamp();
            if (tick >= start_tick && tick <= end_tick)
            {
                /*
                 *  Convert to screen coordinates.
                 */

                int event_x = tix_to_pix(tick) + m_keyboard_padding_x;
                bool selected = cev->is_selected();
                midibyte d0, d1;
                cev->get_data(d0, d1);

                int event_height = d1;          /* generate the value       */
                if (event::is_one_byte_msg(m_status))
                    event_height = d0;

                pen.setWidth(2);                /* draw vertical grid lines */
                if (selected)
                    pen.setColor(sel_paint());  /* pen.setColor("orange")   */
                else
                    pen.setColor(fore_color()); /* pen.setColor(Qt::black)  */

                painter.setPen(pen);
                painter.drawLine
                (
                    event_x, height() - event_height, event_x, height()
                );

                int x_offset = event_x + s_x_data_fix;
                int y_offset = c_dataarea_y - 25;
                snprintf(digits, sizeof digits, "%3d", d1);

                QString val = digits;
                pen.setColor(fore_color());     /* pen.setColor(Qt::black)  */
                painter.drawText(x_offset, y_offset,      val.at(0));
                painter.drawText(x_offset, y_offset +  8, val.at(1));
                painter.drawText(x_offset, y_offset + 16, val.at(2));
            }
        }
#if defined SEQ66_USE_TRY_CATCH
    }
    catch (std::exception & e)          /* EXPERIMENTAL: 2021-02-15 */
    {
        errprintf("qseqdata: %s\n", e.what());
    }
    catch (...)
    {
        errprint("qseqdata exception\n");
    }
#endif
    if (m_line_adjust)                          /* draw edit line           */
    {
        int x, y, w, h;
        pen.setColor(sel_color());              /* Qt::black                */
        pen.setStyle(Qt::DashLine);
        painter.setPen(pen);
        rect::xy_to_rect_get
        (
            drop_x(), drop_y(), current_x(), current_y(), x, y, w, h
        );
        old_rect().set(x, y, w, h);
        painter.drawLine
        (
            current_x() + c_keyboard_padding_x,
            current_y(), drop_x() + c_keyboard_padding_x, drop_y()
        );
    }
}

void
qseqdata::resizeEvent (QResizeEvent * qrep)
{
    qrep->ignore();                         /* QWidget::resizeEvent(qrep)   */
}

void
qseqdata::mousePressEvent (QMouseEvent * event)
{
    seq::pointer s = seq_pointer();
    midipulse tick_start, tick_finish;
    int mouse_x = event->x() - c_keyboard_padding_x + scroll_offset_x();
    int mouse_y = event->y();

    /*
     * If near an event (4px), do relative adjustment.  Do we need to
     * push-undo here?  Not sure, won't change for now.
     */

    tick_start = pix_to_tix(mouse_x - 2);
    tick_finish = pix_to_tix(mouse_x + 2);
    s->push_undo();

    /*
     * Check if this tick range would select an event.
     */

    bool would_select = s->select_events
    (
        tick_start, tick_finish, m_status, m_cc, eventlist::select::would_select
    );
    if (would_select)
        m_relative_adjust = true;
    else
        m_line_adjust = true;   /* set new values for events under a line   */

    drop_x(mouse_x);                            /* set values for line      */
    drop_y(mouse_y);
    old_rect().clear();                         /* reset dirty redraw box   */
    m_dragging = true;                          /* may be dragging now      */
}

void
qseqdata::mouseReleaseEvent (QMouseEvent * event)
{
    current_x(int(event->x()) - c_keyboard_padding_x + scroll_offset_x());
    current_y(int(event->y()));
    if (m_line_adjust)
    {
        if (current_x() < drop_x())
        {
            swap_x();
            swap_y();
        }

        /*
         * Convert x,y to ticks, then set events in range
         */

        midipulse tick_s = pix_to_tix(drop_x());
        midipulse tick_f = pix_to_tix(current_x());
        bool ok = seq_pointer()->change_event_data_range
        (
            tick_s, tick_f, m_status, m_cc,
            c_dataarea_y - drop_y() - 1, c_dataarea_y - current_y() - 1
        );
        m_line_adjust = false;
        if (ok)
            set_dirty();
    }
    else if (m_relative_adjust)
        m_relative_adjust = false;

    m_dragging = false;
}

void
qseqdata::mouseMoveEvent (QMouseEvent * event)
{
    if (! m_dragging)
        return;

    current_x(int(event->x()) - c_keyboard_padding_x);
    current_y(int(event->y()));
    midipulse tick_s, tick_f;
    if (m_line_adjust)
    {
        int adj_x_min, adj_x_max, adj_y_min, adj_y_max;
        if (current_x() < drop_x())
        {
            adj_x_min = current_x();
            adj_y_min = current_y();
            adj_x_max = drop_x();
            adj_y_max = drop_y();
        }
        else
        {
            adj_x_max = current_x();
            adj_y_max = current_y();
            adj_x_min = drop_x();
            adj_y_min = drop_y();
        }

        tick_s = pix_to_tix(adj_x_min);
        tick_f = pix_to_tix(adj_x_max);
        bool ok = seq_pointer()->change_event_data_range
        (
            tick_s, tick_f, m_status, m_cc,
            c_dataarea_y - adj_y_min - 1, c_dataarea_y - adj_y_max - 1
        );
        if (ok)
            set_dirty();
    }
    else if (m_relative_adjust)
    {
        tick_s = pix_to_tix(drop_x() - 2);
        tick_f = pix_to_tix(drop_x() + 2);

        int adjy = drop_y() - current_y();
        bool ok = seq_pointer()->change_event_data_relative
        (
            tick_s, tick_f, m_status, m_cc, adjy
        );
        if (ok)
            set_dirty();

        /*
         * Move the drop location so we increment properly on next mouse move.
         */

        drop_y(current_y());
    }
}

void
qseqdata::set_data_type (midibyte status, midibyte control)
{
    if (m_status == c_midibyte_max && m_cc == c_midibyte_max)
    {
        m_status = EVENT_NOTE_ON;
        m_cc = 0;
    }
    else
    {
        if (status != m_status || control != m_cc)
        {
            m_status = status;
            m_cc = control;
            set_dirty();
        }
    }
}

}           // namespace seq66

/*
 * qseqdata.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


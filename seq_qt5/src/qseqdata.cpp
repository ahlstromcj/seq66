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
 * \file          qseqdata.cpp
 *
 *  This module declares/defines the base class for plastering
 *  pattern/sequence data information in the data area of the pattern
 *  editor.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2022-01-02
 * \license       GNU GPLv2 or above
 *
 *  The data pane is the drawing-area below the seqedit's event area, and
 *  contains vertical lines whose height matches the value of each data event.
 *  The height of the vertical lines is editable via the mouse.
 */

#include "cfg/settings.hpp"             /* seq66::usr() config functions    */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "qseqdata.hpp"                 /* seq66::qseqdata class            */
#include "qseqeditframe64.hpp"          /* seq66::qseqeditframe64 class     */
#include "qt5_helpers.hpp"              /* seq66::qt_timer()                */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  The height of the data-entry area for velocity, aftertouch, and other
 *  controllers, as well as note on and off velocity.  This value is pixels;
 *  one pixel per MIDI value, which ranges from 0 to 127.  We start with a
 *  hardwired constant for this variable, but it can be halved to help fit the
 *  pattern editor into a tab.
 */

static const int sc_dataarea_y = 128;

/*
 * Tweaks
 */

static const int s_x_data_fix   = 2;    /* adjusts x-value for the events   */
static const int s_key_padding  = 8;    /* adjusts x for keyboard padding   */
static const int s_circle_d     = 6;    /* diameter of tempo/prog. dots     */

/**
 *  Principal constructor.
 */

qseqdata::qseqdata
(
    performer & p,
    seq::pointer seqp,
    qseqeditframe64 * frame,
    int zoom, int snap,
    QWidget * parent,
    int height
) :
    QWidget                 (parent),
    qseqbase                (p, seqp, frame, zoom, snap),
    performer::callbacks    (p),
    m_timer                 (nullptr),
    m_font                  ("Monospace"),
    m_keyboard_padding_x    (s_key_padding),
    m_dataarea_y            (height > 0 ? height : sc_dataarea_y),
    m_is_tempo              (false),
    m_is_program_change     (false),
    m_status                (EVENT_NOTE_ON),
    m_cc                    (1),                /* modulation   */
    m_line_adjust           (false),
    m_relative_adjust       (false),
    m_dragging              (false)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_font.setPointSize(8);                     /* was 6        */
    m_font.setBold(true);
    cb_perf().enregister(this);
    m_timer = qt_timer(this, "qseqdata", 2, SLOT(conditional_update()));
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
    int w = frame64()->width();
    int len = tix_to_pix(seq_pointer()->get_length());
    if (len < w)
        len = w;

    len += c_keyboard_padding_x;
    return QSize(len, m_dataarea_y);
}

/**
 *  We create an iterator and use sequence::get_next_event_match().
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
    for (auto cev = s->cbegin(); ! s->cend(cev); ++cev)
    {
        if (! s->get_next_event_match(m_status, m_cc, cev))
            break;

#if defined SEQ66_PLATFORM_DEBUG_TMI
        cev->print();
#endif

        midipulse tick = cev->timestamp();
        if (tick >= start_tick && tick <= end_tick)
        {
            bool normal_event = ! cev->is_tempo() && ! cev->is_program_change();
            bool selected = cev->is_selected();
            int event_x = tix_to_pix(tick) + m_keyboard_padding_x;
            int x_offset = event_x + s_x_data_fix;
            int y_offset = m_dataarea_y - 25;
            midibyte d0, d1;
            cev->get_data(d0, d1);

            int event_height = event::is_one_byte_msg(m_status) ? d0 : d1 ;
            event_height = height() - byte_height(m_dataarea_y, event_height);
            pen.setWidth(2);
            if (cev->is_tempo() && is_tempo())
            {
                d1 = height() - tempo_to_note_value(cev->tempo());
                if (d1 < 4)
                    d1 = 4;                     /* avoid overlap with top   */

                snprintf(digits, sizeof digits, "%3d", int(cev->tempo()));
                brush.setColor(selected ? sel_color() : tempo_color());
                painter.setBrush(brush);
                painter.drawEllipse(event_x, d1 - 3, s_circle_d, s_circle_d);
                painter.drawText(x_offset + 6, d1 + 4, digits);
                brush.setColor(grey_color());
                painter.setBrush(brush);
            }
            if (cev->is_program_change() && is_program_change())
            {
                d1 = event_height;
                if (d1 > height() - 6)
                    d1 = height() - 6;          /* avoid overlap w/bottom   */

                snprintf(digits, sizeof digits, "%3d", d0);
                painter.drawEllipse(event_x, d1, s_circle_d, s_circle_d);
                painter.drawText(x_offset + 2, d1 + 6, digits);
            }
            if (normal_event)
            {
#if defined SEQ66_REQUIRE_SEQ_CHANNEL_MATCH                 /* too much! */
                bool ok;
                if (cev->has_channel())
                {
                    /*
                     * This is problematic.  The dropdown doesn't select notes
                     * with channel, it just selects note events. The
                     * event::match_status() doesn't filter on channel.
                     * We should not filter based on the sequence's hard-wired
                     * channel, iether.
                     */

                    midibyte schan = s->seq_midi_channel();
                    if (is_null_channel(schan))
                        ok = true;
                    else
                    {
                        midibyte chan = cev->channel();
                        ok = chan == schan;
                    }
                }
                else
                    ok = true;

                if (ok)
                {
                    // the code below
                }
#endif
                pen.setColor(selected ? sel_paint() : fore_color());
                painter.setPen(pen);
                painter.drawLine(event_x, event_height, event_x, height());
                snprintf(digits, sizeof digits, "%3d", d1);

                QString val = digits;
                pen.setColor(fore_color());
                painter.setPen(pen);
                painter.drawText(x_offset, y_offset,      val.at(0));
                painter.drawText(x_offset, y_offset +  9, val.at(1));
                painter.drawText(x_offset, y_offset + 18, val.at(2));
            }
        }
    }
    if (m_line_adjust)                          /* draw edit line           */
    {
        int x, y, w, h;
        pen.setColor(sel_color());              /* Qt::black                */
        pen.setStyle(Qt::DashLine);
        pen.setWidth(1);
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
        pen.setWidth(2);
        painter.setPen(pen);
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
        tick_start, tick_finish, m_status, m_cc,
        eventlist::select::would_select
    );
    if (would_select)
        m_relative_adjust = true;
    else
        m_line_adjust = true;                   /* set ev values under line */

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
        int ds = byte_value(m_dataarea_y, m_dataarea_y - drop_y());
        int df = byte_value(m_dataarea_y, m_dataarea_y - current_y() - 1);
        bool ok = seq_pointer()->change_event_data_range
        (
            tick_s, tick_f, m_status, m_cc, ds, df
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

        int ds = byte_value(m_dataarea_y, m_dataarea_y - adj_y_min - 1);
        int df = byte_value(m_dataarea_y, m_dataarea_y - adj_y_max - 1);
        bool ok = seq_pointer()->change_event_data_range
        (
            tick_s, tick_f, m_status, m_cc, ds, df
        );
        if (ok)
            set_dirty();
    }
    else if (m_relative_adjust)
    {
        int adjy = byte_value(m_dataarea_y, drop_y() - current_y());
        tick_s = pix_to_tix(drop_x() - 2);
        tick_f = pix_to_tix(drop_x() + 2);

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
    if (event::is_tempo_status(status))
    {
        is_tempo(true);
        is_program_change(false);
        m_status = status;
        m_cc = 0;
    }
    else if (event::is_program_change_msg(status))
    {
        is_tempo(false);
        is_program_change(true);
        m_status = status;
        m_cc = 0;
    }
    else
    {
        is_tempo(false);
        is_program_change(false);
        m_status = event::normalized_status(status);
        m_cc = control;
    }
    update();
}

}           // namespace seq66

/*
 * qseqdata.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


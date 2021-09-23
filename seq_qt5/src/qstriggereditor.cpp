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
 * \file          qstriggereditor.cpp
 *
 *  This module declares/defines the class for the thin event window pane
 *  "events" just below the qseqroll.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2021-09-22
 * \license       GNU GPLv2 or above
 *
 *  This class represents the central piano-roll user-interface area of the
 *  performance/song editor.  It is not the best name, since the earlier Seq64
 *  name was seqevent.
 */

#include <QApplication>                 /* QApplication keyboardModifiers() */

#include "cfg/settings.hpp"             /* seq66::usr().key_height(), etc.  */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "play/sequence.hpp"            /* seq66::sequence class            */
#include "qseqdata.hpp"                 /* seq66::qseqdata class            */
#include "qseqeditframe64.hpp"          /* seq66::qseqeditframe64 class     */
#include "qstriggereditor.hpp"          /* seq66::qstriggereditor class     */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

class performer;

/*
 * Tweaks
 */

static const int qc_eventarea_y     = 16;
static const int qc_eventevent_y    = 10;
static const int qc_eventevent_x    =  5;
static const int s_x_tick_fix       =  2;

/**
 *  Principal constructor.
 */

qstriggereditor::qstriggereditor
(
    performer & p,
    seq::pointer seqp,
    qseqeditframe64 * frame,
    int zoom, int snap, int keyheight,
    QWidget * parent,
    int xoffset
) :
    QWidget    (parent),
    qseqbase   (p, seqp, frame, zoom, snap),
    m_timer    (nullptr),
    m_x_offset (xoffset + s_x_tick_fix),
    m_key_y    (keyheight),
    m_status   (EVENT_NOTE_ON),
    m_cc       (0)                                  /* bank select          */
{
    setAttribute(Qt::WA_StaticContents);
    setAttribute(Qt::WA_OpaquePaintEvent);          /* no erase on repaint  */
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_timer = new QTimer(this);
    m_timer->setInterval(4 * usr().window_redraw_rate());
    QObject::connect(m_timer, SIGNAL(timeout()), this, SLOT(conditional_update()));
    m_timer->start();
}

/**
 *  This virtual destructor stops the timer.
 */

qstriggereditor::~qstriggereditor ()
{
    m_timer->stop();
}

/**
 *  A convenience function.
 *
 * \return
 *      Returns non-zero if events can be selected (we no longer allow this
 *      for note-related events in this editor, to prevent issues) and are
 *      selected.
 */

int
qstriggereditor::select_events
(
    eventlist::select selmode,
    midipulse start,
    midipulse finish
)
{
    int result = 0;
    if (! event::is_note_msg(m_status))
    {
        result = seq_pointer()->select_events
        (
            start, finish, m_status, m_cc, selmode
        );
    }
    return result;
}

/**
 *  In an effort to reduce CPU usage when simply idling, this function calls
 *  update() only if necessary.  See qseqbase::check_dirty().
 */

void
qstriggereditor::conditional_update ()
{
    if (perf().needs_update() || check_dirty())
        update();
}

QSize
qstriggereditor::sizeHint () const
{
    int w = frame64()->width();
    int len = tix_to_pix(seq_pointer()->get_length());
    if (len < w)
        len = w;

    len += c_keyboard_padding_x;
    return QSize(len, qc_eventarea_y + 1);
}

void
qstriggereditor::paintEvent (QPaintEvent *)
{
    QPainter painter(this);
    QBrush brush(Qt::darkGray, Qt::SolidPattern);
    QPen pen(Qt::black);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.drawRect(1, 0, width(), height() - 1); /* draw the background       */

    int bpbar = seq_pointer()->get_beats_per_bar();
    int bwidth = seq_pointer()->get_beat_width();
    midipulse ticks_per_beat = 4 * perf().ppqn() / bwidth;
    midipulse ticks_per_bar = bpbar * ticks_per_beat;
    midipulse ticks_per_step = pulses_per_substep(perf().ppqn(), zoom());
    midipulse starttick = scroll_offset() - (scroll_offset() % ticks_per_step);
    midipulse endtick = pix_to_tix(width());
    for (midipulse tick = starttick; tick < endtick; tick += ticks_per_step)
    {
        int x_offset = xoffset(tick) - scroll_offset_x() + m_x_offset;
        pen.setWidth(1);
        if (tick % ticks_per_bar == 0)          /* solid line on every beat */
        {
            pen.setColor(fore_color());         /* Qt::black                */
            pen.setStyle(Qt::SolidLine);
            pen.setWidth(2);                    /* two pixels               */
        }
        else if (tick % ticks_per_beat == 0)
        {
            pen.setColor(beat_color());         /* Qt::black                */
            pen.setStyle(Qt::SolidLine);
        }
        else
        {
            pen.setColor(step_color());         /* Qt::lightGray            */
            pen.setStyle(Qt::DashLine);
            int tick_snap = tick - (tick % snap());
            if (tick == tick_snap)
            {
                pen.setStyle(Qt::SolidLine);    // pen.setColor(Qt::DashLine)
                pen.setColor(Qt::lightGray);    // faint step lines
            }
            else
            {
                pen.setStyle(Qt::DashLine);     // Gdk::LINE_ON_OFF_DASH
                pen.setColor(Qt::lightGray);    // faint step lines
            }
        }
        painter.setPen(pen);
        painter.drawLine(x_offset, 1, x_offset, qc_eventarea_y);
    }

    /*
     * Draw boxes from sequence.
     */

    pen.setColor(fore_color());                     /* Qt::black            */
    pen.setStyle(Qt::SolidLine);
    for (auto cev = seq_pointer()->cbegin(); ! seq_pointer()->cend(cev); ++cev)
    {
        if (! seq_pointer()->get_next_event_match(m_status, m_cc, cev))
            break;

        midipulse tick = cev->timestamp();
        if ((tick >= starttick && tick <= endtick))
        {
            bool selected = cev->is_selected();
            int x = xoffset(tick) + m_x_offset;
            int y = (qc_eventarea_y - qc_eventevent_y) / 2;
            pen.setColor(fore_color());             /* Qt::black ev border  */
            brush.setStyle(Qt::SolidPattern);
            brush.setColor(fore_color());           /* Qt::black            */
            painter.setBrush(brush);
            painter.setPen(pen);
            painter.drawRect(x, y, qc_eventevent_x, qc_eventevent_y);
            if (selected)
                brush.setColor(sel_color());        /* "orange"             */
            else
                brush.setColor(back_color());       /* Qt::white            */

            painter.setBrush(brush);                /* draw event highlight */
            painter.drawRect(x, y, qc_eventevent_x - 1, qc_eventevent_y - 1);
        }
    }

    int h = qc_eventevent_y;
    int y = (qc_eventarea_y - h) / 2;               /* draw selection       */
    brush.setStyle(Qt::NoBrush);                    /* painter reset        */
    painter.setBrush(brush);
    if (selecting())
    {
        int x, w;
        x_to_w(drop_x(), current_x(), x, w);
        old_rect().x(x);
        old_rect().width(w);
        pen.setColor(sel_color());                  /* Qt::black            */
        painter.setPen(pen);
        painter.drawRect(x + c_keyboard_padding_x, y, w, h);
    }
    if (drop_action())
    {
        int delta_x = current_x() - drop_x();
        int x = selection().x() + delta_x;
        pen.setColor(Qt::black);
        painter.setPen(pen);
        painter.drawRect(x + c_keyboard_padding_x, y, selection().width(), h);
        old_rect().x(x);
        old_rect().width(selection().width());
    }
}

void
qstriggereditor::resizeEvent (QResizeEvent * qrep)
{
    qrep->ignore();                         /* QWidget::resizeEvent(qrep)   */
}

void
qstriggereditor::mousePressEvent (QMouseEvent * event)
{
    midipulse tick_s, tick_f, tick_w;
    convert_x(qc_eventevent_x, tick_w);

    /*
     * If it was a button press, set values for dragging.
     */

    current_x(int(event->x()) - c_keyboard_padding_x);
    drop_x(current_x());
    old_rect().clear();             /* reset box holding dirty redraw spot */
    if (paste())
    {
        snap_current_x();
        convert_x(current_x(), tick_s);
        seq_pointer()->paste_selected(tick_s, 0);
        paste(false);
        setCursor(Qt::ArrowCursor);
        set_dirty();        // NEW ca 2021-01-09
    }
    else
    {
        bool isctrl = bool(event->modifiers() & Qt::ControlModifier);
        bool lbutton = event->button() == Qt::LeftButton;
        bool rbutton = event->button() == Qt::RightButton;
        if (lbutton)
        {
            convert_x(drop_x(), tick_s);        /* turn x,y in to tick/note */
            tick_f = tick_s + zoom();           /* shift back a few ticks   */
            tick_s -= tick_w;
            if (tick_s < 0)
                tick_s = 0;

            if (adding())                       /* add note length < snap   */
            {
                eventlist::select selmode = eventlist::select::would_select;
                painting(true);
                snap_drop_x();
                convert_x(drop_x(), tick_s);    /* turn x,y in to tick/note */
                bool dropit = select_events(selmode, tick_s, tick_f) == 0;
                if (dropit)
                {
                    seq_pointer()->push_undo();
                    drop_event(tick_s);
                }
            }
            else                                /* we're selecting anew     */
            {
                eventlist::select selmode = eventlist::select::selected;
                bool is_selected = select_events(selmode, tick_s, tick_f) > 0;
                if (is_selected)
                {
                    if (! isctrl)
                    {
                        int note, x, w;
                        moving_init(true);
                        seq_pointer()->selected_box(tick_s, note, tick_f, note);
                        tick_f += tick_w;
                        convert_t(tick_s, x);   /* convert box to X,Y values */
                        convert_t(tick_f, w);
                        w -= x;
                        selection().set
                        (
                            x, w, (qc_eventarea_y - qc_eventevent_y) / 2,
                            qc_eventevent_y
                        );

                        int adjusted_selected_x = selection().x();
                        snap_x(adjusted_selected_x);
                        move_snap_offset_x(selection().x() - adjusted_selected_x);
                    }

                    /*
                     * Align selection for drawing. Save X as a variable so we
                     * can use the snap function.
                     */

                    int tempSelectedX = selection().x();
                    snap_x(tempSelectedX);
                    selection().x(tempSelectedX);
                    snap_current_x();
                    snap_drop_x();
                }
                else
                {
                    if (! isctrl)
                    {
                        seq_pointer()->unselect();
                        set_dirty();    // m_parent_frame->set_dirty(); needed
                    }
                    selmode = eventlist::select::select_one;
                    int numsel = select_events(selmode, tick_s, tick_f);
                    if (numsel == 0)
                    {
                        /*
                         * If we didn't select anything (the user clicked
                         * empty space) unselect all notes, and start the
                         * selection box, unless its a note message in force.
                         */

                        if (! event::is_note_msg(m_status))
                            selecting(true);
                    }
                    else
                        set_dirty();
                }
            }
        }
        if (rbutton)
            set_adding(true);
    }
}

void
qstriggereditor::mouseReleaseEvent (QMouseEvent * event)
{
    current_x(int(event->x()) - c_keyboard_padding_x);
    if (moving())
        snap_current_x();

    int delta_x = current_x() - drop_x();
    bool lbutton = event->button() == Qt::LeftButton;
    bool rbutton = event->button() == Qt::RightButton;
    if (lbutton)
    {
        if (selecting())
        {
            midipulse tick_s, tick_f;
            int x, w;
            eventlist::select selmode = eventlist::select::selecting;
            x_to_w(drop_x(), current_x(), x, w);
            convert_x(x, tick_s);
            convert_x(x + w, tick_f);

            int numsel = select_events(selmode, tick_s, tick_f);
            if (numsel > 0)
                set_dirty();        // m_parent_frame->set_dirty(); needed
        }
        if (moving())
        {
            /*
             * Adjust for snap, then convert deltas into screen coordinates.
             * Then move notes; not really notes, but still moves events.
             */

            midipulse delta_tick;
            delta_x -= move_snap_offset_x();
            convert_x(delta_x, delta_tick);
            seq_pointer()->move_selected_events(delta_tick);
        }
        set_adding(adding());
    }
    if (rbutton)
    {
        if (! QApplication::queryKeyboardModifiers().testFlag(Qt::MetaModifier))
        {
            set_adding(false);
            set_dirty();
        }
    }
    clear_action_flags();               /* turn off */
    seq_pointer()->unpaint_all();
    if (is_dirty())                     /* if clicked, something changed    */
        seq_pointer()->set_dirty();
}

void
qstriggereditor::mouseMoveEvent (QMouseEvent * event)
{
    midipulse tick = 0;
    if (moving_init())
    {
        moving_init(false);
        moving(true);
    }
    if (select_action())                // (m_selecting || m_moving || m_paste)
    {
        current_x(int(event->x()) - c_keyboard_padding_x);
        if (drop_action())              // (m_moving || m_paste)
            snap_current_x();
    }
    if (painting())
    {
        current_x(int(event->x()));
        snap_current_x();
        convert_x(current_x(), tick);
        drop_event(tick);
    }
    set_dirty();
}

void
qstriggereditor::keyPressEvent (QKeyEvent * event)
{
    bool ret = false;
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
    {
        seq_pointer()->remove_selected();
        ret = true;
    }
    if (event->modifiers() & Qt::ControlModifier)
    {
        switch (event->key())
        {
        case Qt::Key_X: /* cut */

            seq_pointer()->cut_selected();
            ret = true;
            break;

        case Qt::Key_C: /* copy */
            seq_pointer()->copy_selected();
            ret = true;
            break;

        case Qt::Key_V: /* paste */
            start_paste();
            ret = true;
            break;

        case Qt::Key_Z: /* Undo */
            if (event->modifiers() & Qt::ShiftModifier)
                seq_pointer()->pop_redo();
            else
                seq_pointer()->pop_undo();

            ret = true;
            break;
        }
    }
    if (! ret)
    {
        if (event->key() == Qt::Key_P)
        {
            set_adding(true);
            ret = true;
        }
        else if (event->key() == Qt::Key_X)
        {
            set_adding(false);
            ret = true;
        }
    }
    if (ret)
        seq_pointer()->set_dirty();
    else
        QWidget::keyPressEvent(event);
}

void
qstriggereditor::keyReleaseEvent (QKeyEvent *)
{
    // no code
}

/**
 *  This function checks the minimums and maximums, then fills in the x and w
 *  values.
 */

void
qstriggereditor::x_to_w (int x1, int x2, int & x, int & w)
{
    if (x1 < x2)
    {
        x = x1;
        w = x2 - x1;
    }
    else
    {
        x = x2;
        w = x1 - x2;
    }
}

void
qstriggereditor::start_paste ()
{
    snap_current_x();
    snap_current_y();
    drop_x(current_x());
    drop_y(current_y());
    paste(true);

    /*
     * Get the box that selected elements are in.
     */

    midipulse tick_s, tick_f;
    int note_h, note_l, x, w;
    seq_pointer()->clipboard_box(tick_s, note_h, tick_f, note_l);
    convert_t(tick_s, x);           /* convert box to X,Y values */
    convert_t(tick_f, w);

    /*
     * w is actually coordinates now, so we have to change.
     * Set the m_selected rectangle to hold the x,y,w,h of our selected events.
     * Adjust, clipboard shift to tick 0.
     */

    w -= x;
    selection().set
    (
        x, w, (qc_eventarea_y - qc_eventevent_y) / 2, qc_eventevent_y
    );
    selection().x(selection().x() + drop_x());
}

void
qstriggereditor::convert_x (int x, midipulse & tick)
{
    tick = pix_to_tix(x);
}

void
qstriggereditor::convert_t (midipulse ticks, int & x)
{
    x = tix_to_pix(ticks);
}

void
qstriggereditor::drop_event (midipulse a_tick)
{
    midibyte d0 = m_cc;
    midibyte d1 = 0x40;
    if (m_status == EVENT_AFTERTOUCH)
        d0 = 0;
    else if (m_status == EVENT_PROGRAM_CHANGE)
        d0 = 0;                                         /* d0 == new patch  */
    else if (m_status == EVENT_CHANNEL_PRESSURE)
        d0 = 0x40;                                      /* d0 == pressure   */
    else if (m_status == EVENT_PITCH_WHEEL)
        d0 = 0;

    seq_pointer()->add_event(a_tick, m_status, d0, d1, true);   /* sorts it */
}

/**
 *  Does not allow adding for note events: on, off, and aftertouch.
 *  These are too tricky due to the need for linking.
 */

void
qstriggereditor::set_adding (bool a)
{
    if (! event::is_note_msg(m_status))
    {
        adding(a);
        if (a)
            setCursor(Qt::PointingHandCursor);
        else
            setCursor(Qt::ArrowCursor);
    }
}

void
qstriggereditor::set_data_type (midibyte status, midibyte control)
{
    if (event::is_meta_status(status))
    {
    }
    else
    {
        status = event::normalize_status(status);
        if (status != m_status || control != m_cc)
        {
            m_status = status;
            m_cc = control;
        }
    }
    update();
}

}           // namespace seq66

/*
 * qstriggereditor.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


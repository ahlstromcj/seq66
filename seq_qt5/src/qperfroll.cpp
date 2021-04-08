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
 * \file          qperfroll.cpp
 *
 *  This module declares/defines the base class for the Qt 5 version of the
 *  Performance window piano roll.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2021-04-08
 * \license       GNU GPLv2 or above
 *
 *  This class represents the central piano-roll user-interface area of the
 *  performance/song editor.
 *
 *  Seq24's song editor does not snap when first drawing in the pattern
 *  extent.  It does snap when moving an existing pattern extent.  It does
 *  snap when enlarging or shrinking an existing pattern extent via the
 *  handle.  That is, if moving or growing, snap the tick.
 */

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

#include "cfg/settings.hpp"             /* seq66::usr() config functions    */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "util/rect.hpp"                /* seq66::rect::xy_to_rect_get()    */
#include "gui_palette_qt5.hpp"
#include "qperfeditframe64.hpp"
#include "qperfroll.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Alpha values for various states, not yet members, not yet configurable.
 */

const int s_alpha_playing       = 255;
const int s_alpha_muted         = 100;

/**
 *  Initial sizing for the perf-roll.
 */

const int c_background_x     = (SEQ66_DEFAULT_PPQN * 4 * 16) / c_perf_scale_x;
const int c_size_box_w       = 6;                /* 3 is too small */
const int c_size_box_click_w = c_size_box_w + 1 ;

/**
 *  Principal constructor.
 */

qperfroll::qperfroll
(
    performer & p,
    int zoom,
    int snap,
    QWidget * frame,        // must be a qseqeditframe/64 widget
    QWidget * parent
) :
    QWidget             (parent),
    qperfbase
    (
        p, zoom, snap, c_names_y, c_names_y * p.sequence_max()
    ),
    m_parent_frame      (reinterpret_cast<qperfeditframe64 *>(frame)),
    m_timer             (nullptr),
    m_font              ("Monospace"),
    m_measure_length    (0),
    m_beat_length       (0),
    m_trigger_transpose (0),
    m_drop_sequence     (-1),
    m_tick_s            (0),
    m_tick_f            (0),
    m_seq_h             (-1),
    m_seq_l             (-1),
    m_drop_tick         (0),
    m_drop_tick_offset  (0),
    mLastTick           (0),
    mBoxSelect          (false),
    m_grow_direction    (false),
    m_adding_pressed    (false)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setFocusPolicy(Qt::StrongFocus);
    m_font.setStyleHint(QFont::Monospace);
    m_font.setLetterSpacing(QFont::AbsoluteSpacing, 1);
    m_font.setBold(true);
    m_font.setPointSize(8);
    m_timer = new QTimer(this);                         // redraw timer
    m_timer->setInterval(2 * usr().window_redraw_rate());
    connect(m_timer, SIGNAL(timeout()), this, SLOT(conditional_update()));
    m_timer->start();
}

/**
 *  This virtual destructor stops the timer.
 */

qperfroll::~qperfroll ()
{
    m_timer->stop();
}

/**
 *  Calls update() if needed, and also implements follow-progress.
 */

void
qperfroll::conditional_update ()
{
    if (perf().needs_update() || check_dirty())
    {
        if (perf().follow_progress())
            follow_progress();              /* keep up with progress    */

        update();
    }
}

/**
 *  Checks the position of the tick, and, if it is in a different piano-roll
 *  "page" than the last page, moves the page to the next page.
 */

void
qperfroll::follow_progress ()
{
    if (not_nullptr(m_parent_frame))
        m_parent_frame->follow_progress();
}

/**
 *  Draws and redraws the performance roll.
 */

void
qperfroll::paintEvent (QPaintEvent * /*qpep*/)
{
    QPainter painter(this);
    QRect r(0, 0, width(), height());           //  QRect r = qpep->rect();
    QBrush brush(Qt::white, Qt::NoBrush);
    QPen pen(Qt::lightGray);                    // QPen pen(Qt::black);
    pen.setStyle(Qt::SolidLine);

#if defined THIS_CODE_ADDS_VALUE
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.drawRect(0, 0, width(), height());
#endif

    if (! is_initialized())
        set_initialized();

    draw_grid(painter, r);
    draw_triggers(painter, r);

    /*
     * Draw selections, if applicable.  Currently, only one box can be selected.
     * Kepler34 has a feature called "box select" that we've had trouble
     * implementing.
     */

    if (mBoxSelect)
    {
        int x, y, w, h;
        rect::xy_to_rect_get
        (
            drop_x(), drop_y(), current_x(), current_y(), x, y, w, h
        );
        old_rect().set(x, y, w, h + c_names_y);
        brush.setStyle(Qt::SolidPattern);           // doesn't work
        brush.setColor(grey_color());               // doesn't work
        pen.setStyle(Qt::SolidLine);
        pen.setColor(sel_color());
        pen.setWidth(2);
        painter.setBrush(brush);
        painter.setPen(pen);
        painter.drawRect(x, y, w, h + c_names_y);
    }
    else
    {
        brush.setStyle(Qt::NoBrush);                // painter reset
        painter.setBrush(brush);
    }

#if defined THIS_CODE_ADDS_VALUE
    int xwidth = r.width();
    int yheight = r.height() - 1;
    pen.setStyle(Qt::SolidLine);                // draw border
    pen.setColor(Qt::black);
    painter.setPen(pen);
    painter.drawRect(0, 0, xwidth, yheight);
#endif

    midipulse tick = perf().get_tick();         /* draw progresss playhead  */
    int progress_x = tick / scale_zoom();
    pen.setColor(progress_color());             // Qt::red
    pen.setStyle(Qt::SolidLine);
    if (usr().progress_bar_thick())
        pen.setWidth(2);

    painter.setPen(pen);
    painter.drawLine(progress_x, 1, progress_x, height() - 2);
    if (usr().progress_bar_thick())
        pen.setWidth(1);
}

/**
 *  The scale_zoom() function is the zoom value times the scale value.
 */

QSize
qperfroll::sizeHint () const
{
    int height = c_names_y * perf().sequence_max() + 1;
    int width = horizSizeHint();
    int w = m_parent_frame->width();
    if (width < w)
        width = w;

    return QSize(width, height);
}

bool
qperfroll::in_selection_area (midipulse tick)
{
    return
    (
        m_drop_sequence >= 0 &&
        m_drop_sequence >= m_seq_l && m_drop_sequence <= m_seq_h &&
        tick >= m_tick_s && tick <= m_tick_f
    );
}

void
qperfroll::mousePressEvent(QMouseEvent *event)
{
    bool isctrl = bool(event->modifiers() & Qt::ControlModifier);
    bool isshift = event->modifiers() & Qt::ShiftModifier;
    bool lbutton = event->button() == Qt::LeftButton;
    bool rbutton = event->button() == Qt::RightButton;
    bool mbutton = event->button() == Qt::MiddleButton || (lbutton && isctrl);
    drop_x(event->x());
    drop_y(event->y());
    convert_xy(drop_x(), drop_y(), m_drop_tick, m_drop_sequence);

    seq::pointer dropseq = perf().get_sequence(m_drop_sequence);
    bool on_pattern = not_nullptr(dropseq);
    if (mbutton)                                    /* split loop at cursor */
    {
        if (on_pattern)
        {
            bool state = dropseq->get_trigger_state(m_drop_tick);
            if (state)
            {
                /*
                 * Determine how and where the split should occur.
                 * Need to support an exact split at some point.
                 */

                midipulse tick = m_drop_tick;
                trigger::splitpoint sp = trigger::splitpoint::middle;
                if (rc().allow_snap_split())
                {
                    sp = trigger::splitpoint::snap;
                    tick -= m_drop_tick_offset;
                    if (snap() > 0)
                        tick -= tick % snap();
                }
                (void) perf().split_trigger(m_drop_sequence, tick, sp);
            }
        }
    }
    else if (rbutton)
    {
        if (isctrl)
        {
            /*
             * See Shift-Left-Click instead.
             *
             * if (on_pattern)
             *    dropseq->transpose_trigger(m_drop_tick, m_trigger_transpose);
             */
        }
        else
        {
            set_adding(true);
            perf().unselect_all_triggers();
            mBoxSelect = false;
        }
    }
    else if (lbutton)
    {
        /*
         * Add a new seq instance if we didn't select anything, and are holding
         * the right mouse button (or the finger button is active).
         */

        if (isshift)
        {
            if (on_pattern)
                dropseq->transpose_trigger(m_drop_tick, m_trigger_transpose);
        }
        else
        {
            midipulse tick = m_drop_tick;
            if (adding())
            {
                m_adding_pressed = true;
                if (on_pattern)
                {
                    bool trigger_state = dropseq->get_trigger_state(tick);
                    if (trigger_state)
                        delete_trigger(m_drop_sequence, tick);
                    else
                        add_trigger(m_drop_sequence, tick);
                }
            }
            else                                    /* not in paint mode    */
            {
                bool selected = false;
                if (on_pattern)
                {
                    /*
                     * ISSUE:  Just clicking in the perf roll gets us here,
                     * and this ends up setting the perf modify flag.  And at best
                     * we are only selecting.
                     *
                     *      perf().push_trigger_undo();
                     *
                     * If the current loop is not in selection range, bin it.
                     */

                    if (! in_selection_area(tick))
                    {
                        perf().unselect_all_triggers();
                        m_seq_h = m_seq_l = m_drop_sequence;
                    }
                    dropseq->select_trigger(tick);

                    midipulse start_tick = dropseq->selected_trigger_start();
                    midipulse end_tick = dropseq->selected_trigger_end();

                    /*
                     * Check for corner drag to grow sequence start.
                     */

                    int clickminus = c_size_box_click_w - 1;
                    int clickbox = c_size_box_click_w * scale_zoom();
                    if
                    (
                        tick >= start_tick && tick <= start_tick + clickbox &&
                        (drop_y() % c_names_y) <= clickminus
                    )
                    {
                        growing(true);
                        m_grow_direction = true;
                        selected = true;
                        m_drop_tick_offset = m_drop_tick - start_tick;
                    }
                    else if     // check for corner drag to grow sequence end
                    (
                        tick >= end_tick - clickbox && tick <= end_tick &&
                        (drop_y() % c_names_y) >= c_names_y - clickminus
                    )
                    {
                        growing(true);
                        selected = true;
                        m_grow_direction = false;
                        m_drop_tick_offset = m_drop_tick - end_tick;
                    }
                    else if (tick <= end_tick && tick >= start_tick)
                    {
                        moving(true);                   // we're moving the seq
                        selected = true;
                        m_drop_tick_offset = m_drop_tick - start_tick;
                    }
                }
                if (! selected)                         // select with a box
                {
                    perf().unselect_all_triggers();
                    snap_drop_y();                      // y always snapped to rows
                    current_x(drop_x());
                    current_y(drop_y());
                    mBoxSelect = true;
                }
            }
        }
    }
    set_dirty();                                    /* force a redraw       */
}

void
qperfroll::mouseReleaseEvent (QMouseEvent * event)
{
    bool lbutton = event->button() == Qt::LeftButton;
    bool rbutton = event->button() == Qt::RightButton;
    if (rbutton)
    {
        m_adding_pressed = false;
        set_adding(false);
    }
    else if (lbutton)
    {
        if (adding())
        {
            /*
             * A quandary.  This turns off trigger insertion. But the user may
             * have "permanently" turned on insertion via "p" or the "Entry
             * mode" button, and expect that to stay in force, and then
             * forgetfully click on an existing pattern.  May have to let user
             * feedback settle this one.
             *
             *  set_adding(false);
             */

            m_adding_pressed = false;
        }
        if (mBoxSelect)                 /* calculate selected seqs in box   */
        {
            int x, y, w, h;             /* window dimensions                */
            current_x(event->x());
            current_y(event->y());
            snap_current_y();
            rect::xy_to_rect_get
            (
                drop_x(), drop_y(), current_x(), current_y(), x, y, w, h
            );
            convert_xy(x,     y,     m_tick_s, m_seq_l);
            convert_xy(x + w, y + h, m_tick_f, m_seq_h);
            perf().select_triggers_in_range(m_seq_l, m_seq_h, m_tick_s, m_tick_f);
        }
    }
    clear_action_flags();
    mBoxSelect = false;
    mLastTick = 0;
    set_dirty();                                    /* force a redraw       */
}

void
qperfroll::mouseMoveEvent (QMouseEvent * event)
{
    midipulse tick = 0;
    int x = event->x();
    seq::pointer dropseq = perf().get_sequence(m_drop_sequence);
    if (is_nullptr(dropseq))
        return;

    if (adding() && m_adding_pressed)
    {
        midipulse seqlength = dropseq->get_length();
        midipulse s = snap() == 0 ? seqlength : snap();
        convert_x(x, tick);
        if (perf().song_record_snap())
            tick -= tick % s;

        dropseq->grow_trigger(m_drop_tick, tick, seqlength);    // or "s"??
    }
    else if (moving() || growing())
    {
        convert_x(x, tick);
        tick -= m_drop_tick_offset;
        if (perf().song_record_snap())      /* apply to move/grow too   */
            if (snap() > 0)
                tick -= tick % snap();

        if (moving())                       /* move selected triggers   */
        {
#if defined USE_SONG_BOX_SELECT
            for (int seqid = m_seq_l; seqid <= m_seq_h; ++seqid)
            {
                seq::pointer seq = perf().get_sequence(seqid);
                if (not_nullptr(seq))
                {
                    if (mLastTick != 0)
                        seq->offset_triggers(-(mLastTick - tick));
                }
            }
#else
            dropseq->move_triggers(tick, true);
#endif
        }
        if (growing())
        {
            midipulse lastoffset = tick - mLastTick;
            if (m_grow_direction)           // grow start, selected triggers
            {
                triggers::grow ts = triggers::grow::start;
                for (int seqid = m_seq_l; seqid <= m_seq_h; ++seqid)
                {
                    seq::pointer seq = perf().get_sequence(seqid);
                    if (not_nullptr(seq))
                    {
                        if (mLastTick != 0)
                            seq->offset_triggers(lastoffset, ts);
                    }
                }
            }
            else                            // grow end, selected triggers
            {
                triggers::grow te = triggers::grow::end;
                for (int seqid = m_seq_l; seqid <= m_seq_h; ++seqid)
                {
                    seq::pointer seq = perf().get_sequence(seqid);
                    if (not_nullptr(seq))
                    {
                        if (mLastTick != 0)
                            seq->offset_triggers(lastoffset - 1, te);
                    }
                }
            }
        }
    }
    else if (mBoxSelect)
    {
        current_x(event->x());
        current_y(event->y());
        snap_current_y();
        convert_xy(0, current_y(), tick, m_drop_sequence);
    }
    mLastTick = tick;
    set_dirty();                                    /* force a redraw       */
}

void
qperfroll::keyPressEvent (QKeyEvent * event)
{
    bool handled = false;
    bool dirty = false;
    if (perf().is_pattern_playing())
    {
        if (event->key() == Qt::Key_Space)
        {
            handled = true;
            stop_playing();
        }
        else if (event->key() == Qt::Key_Period)
        {
            handled = true;
            pause_playing();
        }
    }
    else
    {
        bool isctrl = event->modifiers() & Qt::ControlModifier;
        bool isshift = event->modifiers() & Qt::ShiftModifier;
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Period)
        {
            handled = true;
            start_playing();
        }
        else if (event->key() == Qt::Key_P)
        {
            handled = true;
            set_adding(true);
        }
        else if (event->key() == Qt::Key_X)
        {
            handled = true;
            set_adding(false);
        }
        if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
        {
            handled = true;
            perf().push_trigger_undo();                 // delete selected notes
            for (int seqid = m_seq_l; seqid <= m_seq_h; seqid++)
            {
                if (perf().is_seq_active(seqid))
                {
                    if (perf().get_sequence(seqid)->delete_selected_triggers())
                        dirty = true;
                }
            }
        }
        if (isctrl)
        {
            seq::pointer dropseq = perf().get_sequence(m_drop_sequence);
            switch (event->key())
            {
            case Qt::Key_X:
                handled = true;
                if (not_nullptr(dropseq))
                {
                    perf().push_trigger_undo();
                    if (dropseq->cut_selected_trigger())
                        dirty = true;
                }
                break;

            case Qt::Key_C:
                handled = true;
                if (not_nullptr(dropseq))
                    dropseq->copy_selected_trigger();
                break;

            case Qt::Key_V:
                handled = dirty = true;
                if (not_nullptr(dropseq))
                {
                    perf().push_trigger_undo();
                    dropseq->paste_trigger();
                }
                break;

            case Qt::Key_Z:
                handled = true;
                if (event->modifiers() & Qt::ShiftModifier)
                {
                    dirty = true;
                    perf().pop_trigger_redo();
                }
                else
                {
                    perf().pop_trigger_undo();
                }
                break;
            }
        }
        else if (isshift)
        {
            if (event->key() == Qt::Key_Z)
            {
                handled = true;
                m_parent_frame->zoom_in();
            }
        }
        else if (event->key() == Qt::Key_Z)
        {
            handled = true;
            m_parent_frame->zoom_out();
        }
        else if (event->key() == Qt::Key_0)
        {
            handled = true;
            m_parent_frame->reset_zoom();
        }
    }
    if (handled)
    {
        m_parent_frame->set_dirty();
        if (dirty)
            set_dirty();
    }
    else
        QWidget::keyPressEvent(event);
}

void
qperfroll::keyReleaseEvent (QKeyEvent * /*event*/)
{
    // no code
}

void
qperfroll::add_trigger(int seq, midipulse tick)
{
    perf().add_trigger(seq, tick, snap());
}

void
qperfroll::delete_trigger(int seq, midipulse tick)
{
    perf().delete_trigger(seq, tick);
}

/*
 *  Sets the snap, measure-length, and beat-length members.
 */

void
qperfroll::set_guides (midipulse snap, midipulse measure, midipulse beat)
{
    set_snap(snap);
    m_measure_length = measure;
    m_beat_length = beat;
    if (is_initialized())
        set_dirty();
}

void
qperfroll::set_adding (bool a)
{
    adding(a);
    if (a)
        setCursor(Qt::PointingHandCursor);      /* Qt doesn't have a pencil */
    else
        setCursor(Qt::ArrowCursor);

    m_parent_frame->update_entry_mode(a);       /* updates checkable button */
    set_dirty();
}

void
qperfroll::undo ()
{
    perf().pop_trigger_undo();
}

void
qperfroll::redo ()
{
    perf().pop_trigger_redo();
}

void
qperfroll::draw_grid (QPainter & painter, const QRect & r)
{
    int xwidth = r.width();
    int yheight = r.height();
    QBrush brush(back_color());                         /* Qt::NoBrush      */
    QPen pen(fore_color());                             /* Qt::black        */
    pen.setStyle(Qt::SolidLine);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.drawRect(0, 0, width(), height());          /* full width       */
    pen.setColor(step_color());                         /* Qt::lightGray    */
    for (int i = 0; i < height(); i += c_names_y)       /* horizontal lines */
        painter.drawLine(0, i, xwidth, i);

    /*
     *  Draw the vertical lines for the measures and the beats. Incrementing by
     *  the beat-length makes drawing go faster.
     */

    midipulse tick0 = scroll_offset();
    midipulse windowticks = pix_to_tix(xwidth);
    midipulse tick1 = tick0 + windowticks;
    midipulse tickstep = beat_length();                 /* versus 1         */
    int penwidth = 1;
    for (midipulse tick = tick0; tick < tick1; tick += tickstep)
    {
        if (tick % measure_length() == 0)               /* measure          */
        {
            pen.setColor(fore_color());                 /* Qt::black        */
            penwidth = 2;
        }
        else if (tick % beat_length() == 0)             /* measure          */
        {
            pen.setColor(step_color());                 /* Qt::lightGray    */
            penwidth = 1;
        }

        int x_pos = position_pixel(tick);
        pen.setWidth(penwidth);
        painter.setPen(pen);
        painter.drawLine(x_pos, 0, x_pos, yheight);
    }
}

void
qperfroll::draw_triggers (QPainter & painter, const QRect & r)
{
    int y_s = 0;
    int y_f = r.height() / c_names_y;
    int cbw = c_size_box_w;                     /* copied for readability   */
    QBrush brush(Qt::NoBrush);
    QPen pen(Qt::black);
    pen.setStyle(Qt::SolidLine);
    painter.setPen(pen);
    painter.setBrush(brush);
    pen.setWidth(1);
    for (int y = y_s; y <= y_f; ++y)
    {
        int seqid = y;
        if (perf().is_seq_active(seqid))
        {
            seq::pointer seq = perf().get_sequence(seqid);
            midipulse lens = seq->get_length();

            /*
             * Get the seq's assigned colour and beautify it.  Reduce the
             * strength of this color.  Not sure why get_color_fix() doesn't
             * do it here.  We can make this a user setting later.
             */

            int c = perf().color(seqid);
            Color backcolor = get_color_fix(PaletteColor(c));
            if (seq->playing())
                backcolor.setAlpha(s_alpha_playing);
            else
                backcolor.setAlpha(s_alpha_muted);

            int lenw = lens / scale_zoom();
            trigger trig;
            seq->reset_draw_trigger_marker();
            while (seq->next_trigger(trig))
            {
                if (trig.tick_end() > 0)
                {
                    int x_on = tix_to_pix(trig.tick_start());
                    int x_off = tix_to_pix(trig.tick_end());
                    int w = x_off - x_on + 1;
                    int x = x_on;
                    int xmax = x_off + 1;               /* same as x + w    */
                    int y = c_names_y * seqid + 1;
                    int h = c_names_y - 2;
                    if (trig.selected())
                    {
                        pen.setColor(sel_color());      /* orange, Qt::red  */
                        brush.setColor(grey_color());   /* make it obvious  */
                    }
                    else
                    {
                        pen.setColor(Qt::black);
                        brush.setColor(backcolor);
                    }

                    pen.setStyle(Qt::SolidLine);    /* main seq icon box    */
                    brush.setStyle(Qt::SolidPattern);
                    painter.setBrush(brush);
                    painter.setPen(pen);
                    painter.drawRect(x, y, w, h);

                    pen.setColor(Qt::lightGray);    /* lines between panels */
                    painter.setPen(pen);
                    painter.drawLine(x + 1, y -1,      x + w - 2, y - 1);
                    painter.drawLine(x + 1, y + h + 1, x + w - 2, y + h + 1);

                    brush.setStyle(Qt::NoBrush);    /* seq grab handle left */
                    painter.setBrush(brush);
                    pen.setColor(Qt::black);
                    painter.setPen(pen);
                    painter.drawRect(x, y, cbw, cbw);
                    painter.drawRect                /* grab handle right    */
                    (
                        xmax - cbw, y + h - cbw, cbw, cbw
                    );
                    pen.setColor(Qt::black);
                    painter.setPen(pen);
                    if (trig.transposed())
                    {
                        char temp[16];
                        int t = trig.transpose();
                        if (t > 0)
                            snprintf(temp, sizeof temp, "+%d", t);
                        else
                            snprintf(temp, sizeof temp, "-%d", -t);

                        painter.setFont(m_font);
                        painter.drawText(x + 2, y + cbw + h / 2, temp);
                    }

                    midipulse t = trig.trigger_marker(lens);
                    while (t < trig.tick_end())
                    {
                        int note0;
                        int note1;
                        (void) seq->minmax_notes(note0, note1);

                        int height = note1 - note0;
                        height += 2;
                        if (seq->transposable())
                            pen.setColor(fore_color());     // Qt::black
                        else
                            pen.setColor(drum_color());     // Qt::red

                        int cny = c_names_y - 6;
                        int marker_x = tix_to_pix(t);
                        painter.setPen(pen);
                        for (auto cev = seq->cbegin(); ! seq->cend(cev); ++cev)
                        {
                            sequence::note_info ni;
                            sequence::draw dt = seq->get_next_note_ex(ni, cev);
                            if (dt == sequence::draw::finish)
                                break;

                            midipulse tick_s = ni.start();
                            midipulse tick_f = ni.finish();
                            int note_y =
                            (
                                cny - (cny * (ni.note() - note0)) / height
                            ) + 1 + y;
                            int sx = tick_s * lenw / lens + marker_x;
                            int fx = tick_f * lenw / lens + marker_x;
                            if (sequence::is_draw_note_onoff(dt))
                                fx = sx + 1;

                            if (fx <= sx)
                                fx = sx + 1;

                            if (sx < x)
                                sx = x;

                            if (fx > xmax)
                                fx = xmax;

                            if (fx >= x && sx <= xmax)
                                painter.drawLine(sx, note_y, fx, note_y);
                        }
                        t += lens;
                    }
                }
            }
        }
    }
}

}           // namespace seq66

/*
 * qperfroll.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


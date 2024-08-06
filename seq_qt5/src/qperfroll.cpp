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
 * \file          qperfroll.cpp
 *
 *  This module declares/defines the base class for the Qt 5 version of the
 *  Performance window piano roll.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2024-08-06
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
#include "qperfnames.hpp"
#include "qperfroll.hpp"
#include "qt5_helpers.hpp"              /* seq66::qt_timer()                */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Alpha values for various states, not yet members, not yet configurable.
 */

static const int c_alpha_playing    = 255;
static const int c_alpha_muted      = 100;

/**
 *  Initial sizing for the perf-roll.  The baseline PPQN is defined in
 *  usrsettings.
 */

static const int c_ycorrection      = 1;    /* horizontal grid line fix */
static const int c_pen_width        = 2;
static const int c_size_box_w       = 8;
static const int c_size_box_click_w = c_size_box_w + 1 ;

#if defined THIS_CODE_ADDS_VALUE
static const int c_background_x     = (c_base_ppqn * 4 * 16) / c_perf_scale_x;
static const int c_border_width     = 2;
#endif

/**
 *  Font sizes for small, normal, and expanded vertical zoom
 */

static const int s_vfont_size_small     =  8;
static const int s_vfont_size_normal    = 12;
static const int s_vfont_size_large     = 16;

/**
 *  Principal constructor.
 */

qperfroll::qperfroll
(
    performer & p, int zoom, int snap,
    qperfnames * seqnames,
    qperfeditframe64 * frame,
    QWidget * parent
) :
    QWidget             (parent),
    qperfbase
    (
        p, zoom, snap, c_names_y,               /* unit height of tracks    */
        c_names_y * p.sequences_in_sets()       /* p.sequence_max()         */
    ),
    m_parent_frame      (frame),                /* frame64() accessor       */
    m_perf_names        (seqnames),
    m_timer             (nullptr),
    m_font              ("Monospace"),
    m_prog_thickness    (usr().progress_bar_thick() ? 2 : 1),
    m_trigger_transpose (0),
    m_tick_s            (0),
    m_tick_f            (0),
    m_seq_h             (-1),
    m_seq_l             (-1),
    m_drop_track        (-1),
    m_drop_tick         (0),
    m_drop_tick_offset  (0),
    m_last_tick         (0),
    m_box_select        (false),
    m_grow_direction    (false),
    m_adding_pressed    (false)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);         /* track mouse movement without a click */
    m_font.setStyleHint(QFont::Monospace);
    m_font.setLetterSpacing(QFont::AbsoluteSpacing, 1);
    m_font.setBold(true);
    m_font.setPointSize(s_vfont_size_normal);
    m_timer = qt_timer(this, "qperfroll", 2, SLOT(conditional_update()));
}

/**
 *  This virtual destructor stops the timer.
 */

qperfroll::~qperfroll ()
{
    if (not_nullptr(m_timer))
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
    if (not_nullptr(frame64()))
        frame64()->follow_progress();
}

/**
 *  Draws and redraws the performance roll.
 */

void
qperfroll::paintEvent (QPaintEvent * /*qpep*/)
{
    QPainter painter(this);
    QRect r(0, 0, width(), height());
    QBrush brush(Qt::white, Qt::NoBrush);
    QPen pen(fore_color());
    pen.setStyle(Qt::SolidLine);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.drawRect(0, 0, width(), height());
    if (! is_initialized())
        set_initialized();

    draw_grid(painter, r);
    draw_triggers(painter, r);

    /*
     * Draw selections, if applicable.  Currently, only one box can be selected.
     * Kepler34 has a feature called "box select" that we've had trouble
     * implementing.
     */

    if (m_box_select)
    {
        int x, y, w, h;
        rect::xy_to_rect_get
        (
            drop_x(), drop_y(), current_x(), current_y(), x, y, w, h
        );
        old_rect().set(x, y, w, h + track_height());
        brush.setStyle(Qt::SolidPattern);
        brush.setColor(grey_color());
        pen.setStyle(Qt::SolidLine);
        pen.setColor(sel_color());
        pen.setWidth(c_pen_width);
        painter.setBrush(brush);
        painter.setPen(pen);
        painter.drawRect(x, y, w, h + track_height());
    }
    else
    {
        brush.setStyle(Qt::NoBrush);                // painter reset
        painter.setBrush(brush);
    }

#if defined THIS_CODE_ADDS_VALUE
    int xwidth = r.width();
    int yheight = r.height() - 1;
    pen.setStyle(Qt::SolidLine);                    // draw border
    pen.setColor(Qt::black);
    pen.setWidth(c_border_width);
    painter.setPen(pen);
    painter.drawRect(0, 0, xwidth, yheight);
#endif

    midipulse tick = perf().get_tick();         /* draw progress playhead   */
    int progress_x = tix_to_pix(tick);          /* tick / scale_zoom()      */
    pen.setColor(progress_color());
    pen.setStyle(Qt::SolidLine);
    if (usr().progress_bar_thick())
        pen.setWidth(c_pen_width);

    painter.setPen(pen);
    painter.drawLine(progress_x, 1, progress_x, height() - 2);
}

bool
qperfroll::v_zoom_in ()
{
    bool result = not_nullptr(perf_names());
    if (result)
    {
        m_font.setPointSize(s_vfont_size_large);
        set_thick();
        perf_names()->set_thick();
        set_dirty();
        frame64()->set_dirty();
    }
    return result;
}

bool
qperfroll::v_zoom_out ()
{
    bool result = not_nullptr(perf_names());
    if (result)
    {
        m_font.setPointSize(s_vfont_size_small);
        set_thin();
        perf_names()->set_thin();
        set_dirty();
        frame64()->set_dirty();
    }
    return result;
}

bool
qperfroll::reset_v_zoom ()
{
    bool result = not_nullptr(perf_names());
    if (result)
    {
        m_font.setPointSize(s_vfont_size_normal);
        set_normal();
        perf_names()->set_normal();
        frame64()->reset_zoom();
        set_dirty();
        frame64()->set_dirty();
    }
    return result;
}

bool
qperfroll::zoom_in ()
{
    bool result = qperfbase::zoom_in();
    if (result)
        set_dirty();

    return result;
}

bool
qperfroll::zoom_out ()
{
    bool result = qperfbase::zoom_out();
    if (result)
        set_dirty();

    return result;
}

bool
qperfroll::reset_zoom (int ppq)
{
    bool result = qperfbase::reset_zoom(ppq);
    if (result)
        set_dirty();

    return result;
}

/**
 *  The scale_zoom() function is the zoom value times the scale value. At some
 *  point, might override horizSizeHint() a la qperfbase.
 */

QSize
qperfroll::sizeHint () const
{
    int count = perf().sequences_in_sets();
    int height = track_height() * count;
    int width = horizSizeHint();
    int w = frame64()->width();
    if (width < w)
        width = w;

    width *= width_factor();
    return QSize(width, height);
}

bool
qperfroll::in_selection_area (midipulse tick)
{
    return
    (
        m_drop_track >= 0 &&
        m_drop_track >= m_seq_l && m_drop_track <= m_seq_h &&
        tick >= m_tick_s && tick <= m_tick_f
    );
}

void
qperfroll::mousePressEvent(QMouseEvent * event)
{
    bool isctrl = bool(event->modifiers() & Qt::ControlModifier);
    bool isshift = bool(event->modifiers() & Qt::ShiftModifier);
    bool lbutton = event->button() == Qt::LeftButton;
    bool rbutton = event->button() == Qt::RightButton;
    bool mbutton = event->button() == Qt::MiddleButton || (lbutton && isctrl);
    drop_x(event->x());
    drop_y(event->y());
    convert_xy(drop_x(), drop_y(), m_drop_tick, m_drop_track);

    seq::pointer dropseq = perf().get_sequence(m_drop_track);
    bool on_pattern = not_nullptr(dropseq);
    if (mbutton)                                    /* split loop at cursor */
    {
        if (on_pattern)
        {
            bool state = perf().get_trigger_state(m_drop_track, m_drop_tick);
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
                (void) perf().split_trigger(m_drop_track, tick, sp);
            }
        }
    }
    else if (rbutton)
    {
        if (isctrl)
        {
            /*
             * See Shift-Left-Click instead.
             * if (on_pattern)
             *    perf().transpose_trigger(...);
             */
        }
        else
        {
            set_adding(true);
            perf().unselect_all_triggers();
            m_box_select = false;
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
            {
                perf().transpose_trigger
                (
                    m_drop_track, m_drop_tick, m_trigger_transpose
                );
            }
        }
        else
        {
            midipulse tick = m_drop_tick;
            if (adding())
            {
                m_adding_pressed = true;
                if (on_pattern)
                {
                    bool trigger_state = perf().get_trigger_state
                    (
                        m_drop_track, tick
                    );
                    if (trigger_state)
                        delete_trigger(m_drop_track, tick);
                    else
                        add_trigger(m_drop_track, tick);
                }
            }
            else                                    /* not in paint mode    */
            {
                bool selected = false;
                if (on_pattern)
                {
                    /*
                     * ISSUE:  Just clicking in the perf roll gets us here,
                     * and this ends up setting the perf modify flag.  And at
                     * best we are only selecting.
                     *
                     *      perf().push_trigger_undo();
                     *
                     * If the current loop is not in selection range, bin it.
                     */

                    if (! in_selection_area(tick))
                    {
                        perf().unselect_all_triggers();
                        m_seq_h = m_seq_l = m_drop_track;
                    }
                    perf().select_trigger(m_drop_track, tick);

                    midipulse start_tick, end_tick;
                    (void) perf().selected_trigger
                    (
                        m_drop_track, tick, start_tick, end_tick
                    );

                    /*
                     * Check for corner drag to grow sequence start.
                     */

                    int clickminus = c_size_box_click_w - 1;
                    int clickbox = c_size_box_click_w * scale_zoom();
                    if
                    (
                        tick >= start_tick && tick <= start_tick + clickbox &&
                        (drop_y() % track_height()) <= clickminus
                    )
                    {
                        growing(true);
                        m_grow_direction = true;
                        selected = true;
                        m_drop_tick_offset = m_drop_tick - start_tick;
                    }
                    else if     // check for corner drag to grow sequence end
                    (
                        tick >= (end_tick - clickbox) && tick <= end_tick &&
                        (drop_y() % track_height()) >=
                            (track_height() - clickminus)
                    )
                    {
                        growing(true);
                        selected = true;
                        m_grow_direction = false;
                        m_drop_tick_offset = m_drop_tick - end_tick;
                    }
                    else if (tick <= end_tick && tick >= start_tick)
                    {
                        moving(true);               /* we're moving the seq */
                        selected = true;
                        m_drop_tick_offset = m_drop_tick - start_tick;
                    }
                }
                if (! selected)                     /* select with a box    */
                {
                    perf().unselect_all_triggers();
                    snap_drop_y();                  /* always snap to rows  */
                    current_x(drop_x());
                    current_y(drop_y());
                    m_box_select = true;
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
        if (m_box_select)                 /* calculate selected seqs in box */
        {
            int x, y, w, h;
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
    m_box_select = false;
    m_last_tick = 0;
    set_dirty();                                    /* force a redraw       */
}

void
qperfroll::mouseMoveEvent (QMouseEvent * event)
{
    seq::pointer dropseq = perf().get_sequence(m_drop_track);
    int x = event->x();
    int y = event->y();
    int row;
    midipulse t, tick = 0;
    convert_xy(x, y, t, row);
    if (row >= 0)
        perf_names()->set_preview_row(row);

    if (is_nullptr(dropseq))
        return;

    if (adding() && m_adding_pressed)
    {
        midipulse seqlength = dropseq->get_length();
        midipulse s = snap() == 0 ? seqlength : snap();
        convert_x(x, tick);
        if (perf().song_record_snap())
            tick -= tick % s;

        (void) perf().grow_trigger
        (
            m_drop_track, m_drop_tick, tick, seqlength
        );
    }
    else if (moving() || growing())
    {
        convert_x(x, tick);
        tick -= m_drop_tick_offset;
        if (perf().song_record_snap())          /* apply to move/grow too   */
            if (snap() > 0)                     /* compare Seq64 issue #171 */
                tick -= tick % snap();

        if (moving())                           /* move selected triggers   */
        {
#if defined USE_SONG_BOX_SELECT
            if (m_last_tick != 0)
                perf().box_move_triggers(tick - m_last_tick);
#else
            perf().move_triggers(m_drop_track, tick, true);
#endif
        }
        if (growing())
        {
            if (m_last_tick != 0)
            {
                midipulse lastoffset = tick - m_last_tick;
                triggers::grow ts = m_grow_direction ?
                    triggers::grow::start : triggers::grow::end ;

                (void) perf().offset_triggers(ts, m_seq_l, m_seq_h, lastoffset);
            }

#if USE_OLD_CODE
            if (m_grow_direction)               /* grow start & trigger(s)  */
            {
                triggers::grow ts = triggers::grow::start;
                for (int seqid = m_seq_l; seqid <= m_seq_h; ++seqid)
                {
                    seq::pointer seq = perf().get_sequence(seqid);
                    if (not_nullptr(seq))
                    {
                        if (m_last_tick != 0)
                            seq->offset_triggers(lastoffset, ts);
                    }
                }
            }
            else                                /* grow end & trigger(s)    */
            {
                triggers::grow te = triggers::grow::end;
                for (int seqid = m_seq_l; seqid <= m_seq_h; ++seqid)
                {
                    seq::pointer seq = perf().get_sequence(seqid);
                    if (not_nullptr(seq))
                    {
                        if (m_last_tick != 0)
                            seq->offset_triggers(lastoffset - 1, te);
                    }
                }
            }
#endif
        }
    }
    else if (m_box_select)
    {
        current_x(event->x());
        current_y(event->y());
        snap_current_y();
        convert_xy(0, current_y(), tick, m_drop_track);
    }
    m_last_tick = tick;
    set_dirty();                                    /* force a redraw       */
    frame64()->set_dirty();
}

/**
 *  One issue is that a double-click yields a mouse-press and an
 *  mouse-double-click event, in that order.
 */

void
qperfroll::mouseDoubleClickEvent (QMouseEvent * event)
{
    if (rc().allow_click_edit())
    {
        int seqno = seq_id_from_xy(event->x(), event->y());
        bool active = perf().is_seq_active(seqno);
        emit signal_call_editor_ex(seqno, active);
    }
}

/**
 *  Converts the (x, y) coordinates of a click into a sequence/pattern ID.
 *  Compare this function to qslivegrid::seq_id_from_xy().
 *
 * \param click_x
 *      The x-coordinate of the mouse click. At present, this value
 *      is not checked to see if there is a trigger at that location.
 *      Thus, a pattern can be opened/created anywhere in the track
 *      line.
 *
 * \param click_y
 *      The y-coordinate of the mouse click.
 *
 * \return
 *      Returns the sequence/pattern number.  If not found, then a -1 (the
 *      value seq::unassigned) is returned.
 */

int
qperfroll::seq_id_from_xy (int click_x, int click_y)
{
    int result = seq::unassigned();
    midipulse tick = 0;
    convert_xy(click_x, click_y, tick, result);
    return result;
}

void
qperfroll::keyPressEvent (QKeyEvent * event)
{
    bool handled = false;
    bool dirty = false;
    seq::pointer dropseq = perf().get_sequence(m_drop_track);
    bool on_pattern = not_nullptr(dropseq);
    bool isshift = event->modifiers() & Qt::ShiftModifier;
    bool isctrl = event->modifiers() & Qt::ControlModifier;
    if (perf().is_pattern_playing())
    {
        if (event->key() == Qt::Key_Space)
        {
            handled = true;
            stop_playing();
        }
        else if (event->key() == Qt::Key_Escape)
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
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Period)
        {
            handled = true;
            start_playing();
        }
        else if (event->key() == Qt::Key_Escape)
        {
            if (adding())
            {
                handled = true;
                set_adding(false);
            }
            else
            {
                if (usr().escape_pattern())
                {
                    if (not_nullptr(frame64()))
                    {
                        if (frame64()->is_external())
                            frame64()->parentWidget()->close();
                    }
                }
            }
        }
        else if (event->key() == Qt::Key_I)
        {
            handled = true;
            set_adding(true);
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
        else if
        (
            event->key() == Qt::Key_Delete ||
            event->key() == Qt::Key_Backspace
        )
        {
            handled = true;
            perf().push_trigger_undo();         /* delete selected notes    */
            for (int seqid = m_seq_l; seqid <= m_seq_h; seqid++)
            {
                if (perf().is_seq_active(seqid))
                {
                    if (perf().delete_triggers(seqid))
                        dirty = true;
                }
            }
        }
        else if
        (
            event->key() == Qt::Key_Left ||
            event->key() == Qt::Key_Right
        )
        {
            if (on_pattern && ! isctrl)
                handled = move_by_key(event->key() == Qt::Key_Right);
        }
        if (isctrl)
        {
            switch (event->key())
            {
            case Qt::Key_X:

                handled = true;
                if (on_pattern)
                {
                    if (perf().cut_triggers(m_drop_track))
                        dirty = true;
                }
                break;

            case Qt::Key_C:

                handled = true;
                if (on_pattern)
                    perf().copy_triggers(m_drop_track);
                break;

            case Qt::Key_V:

                handled = true;
                if (on_pattern)
                {
                    if (perf().paste_trigger(m_drop_track))
                        dirty = true;
                }
                break;

            case Qt::Key_Z:

                handled = dirty = true;
                if (event->modifiers() & Qt::ShiftModifier)
                    perf().pop_trigger_redo();
                else
                    perf().pop_trigger_undo();
                break;

            case Qt::Key_Home:

                handled = true;
                if (not_nullptr(frame64()))
                    frame64()->scroll_to_tick(0);
                break;

            case Qt::Key_End:

                handled = true;
                if (not_nullptr(frame64()))
                    frame64()->scroll_to_tick(perf().get_max_extent());
                break;

            case Qt::Key_Left:
            case Qt::Key_Right:

                /*
                 * Redundant with non-Ctrl versions.
                 */

                handled = move_by_key(event->key() == Qt::Key_Right, false);
                break;
            }
        }
        else
        {
            /*
             * These can be done in playback or not. The grid is "continually"
             * redrawn.
             */

            if (isshift)
            {
                if (event->key() == Qt::Key_Z)
                {
                    handled = frame64()->zoom_in();
                }
                else if (event->key() == Qt::Key_V)
                {
                    handled = true;
                    v_zoom_in();
                }
            }
            else
            {
                if (event->key() == Qt::Key_Z)
                {
                    handled = frame64()->zoom_out();
                }
                else if (event->key() == Qt::Key_V)
                {
                    handled = v_zoom_out();
                }
                else if (event->key() == Qt::Key_0)
                {
                    handled = reset_v_zoom();   /* also resets horiz zoom   */
                }
                else if (event->key() == Qt::Key_Home)
                {
                    handled = true;
                    if (not_nullptr(frame64()))
                        frame64()->scroll_to_tick(0);
                }
                else if (event->key() == Qt::Key_End)
                {
                    handled = true;
                    if (not_nullptr(frame64()))
                        frame64()->scroll_to_tick(perf().get_max_extent());
                }
            }
        }
    }
    if (handled)
    {
        frame64()->set_dirty();
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

bool
qperfroll::move_by_key (bool forward, bool single)
{
    trigger t = perf().find_trigger(m_drop_track, m_drop_tick);
    bool result = t.is_valid();
    if (result)
    {
        m_drop_tick = t.tick_start();

        bool ok = perf().move_trigger
        (
            m_drop_track, m_drop_tick, snap(), forward, single
        );
        if (ok)
        {
            if (forward)
                m_drop_tick += snap();
            else
                m_drop_tick -= snap();
        }
    }
    else
        m_drop_tick = 0;

    return result;
}

void
qperfroll::add_trigger(int seq, midipulse tick)
{
    (void) perf().add_trigger(seq, tick, snap());
}

void
qperfroll::delete_trigger(int seq, midipulse tick)
{
    (void) perf().delete_trigger(seq, tick);
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

    frame64()->update_entry_mode(a);            /* updates checkable button */
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
    pen.setWidth(c_pen_width);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.drawRect(0, 0, width(), height());          /* full width       */
    for (int i = 0; i < height(); i += track_height())  /* horizontal lines */
    {
        int y = i + c_ycorrection;                      /* - 2 */
        painter.drawLine(0, y, xwidth, y);
    }

    /*
     *  Draw the vertical lines for the measures and the beats. Incrementing by
     *  the beat-length (PPQN) makes drawing go faster.
     */

    midipulse tick0 = scroll_offset();
    midipulse windowticks = pix_to_tix(xwidth);
    midipulse tick1 = tick0 + windowticks;
    midipulse tickstep = beat_length();                 /* versus 1         */
    int penwidth = 1;
    for (midipulse tick = tick0; tick < tick1; tick += tickstep)
    {
        int x_pos = xoffset(tick);
        if (tick % measure_length() == 0)               /* measure          */
        {
            pen.setColor(beat_paint());                 /* fore_color()     */
            penwidth = 2;
        }
        else if (tick % beat_length() == 0)             /* beat             */
        {
            pen.setColor(beat_color());
            penwidth = 1;
        }
        pen.setWidth(penwidth);
        painter.setPen(pen);
        painter.drawLine(x_pos, 0, x_pos, yheight);
    }
}

void
qperfroll::draw_triggers (QPainter & painter, const QRect & r)
{
    int y_s = 0;
    int y_f = r.height() / track_height();
    int cbw = c_size_box_w;                     /* copied for readability   */
    QBrush brush(Qt::NoBrush);
    QPen pen(fore_color());
    pen.setStyle(Qt::SolidLine);
    painter.setPen(pen);
    painter.setBrush(brush);
    pen.setWidth(1);

    /*
     * No real need to enlarge the corner handles, IMHO.
     *
     *  if (track_thick())
     *      cbw *= 2;
     */

    for (int y = y_s; y <= y_f; ++y)
    {
        int seqid = y;
        if (perf().is_seq_active(seqid))
        {
            seq::pointer s = perf().get_sequence(seqid);
            midipulse lens = s->get_length();

            /*
             * Get the seq's assigned colour and beautify it.  Reduce the
             * strength of this color.  Not sure why get_color_fix() doesn't
             * do it here.  We can make this a user setting later.
             */

            int c = perf().color(seqid);
            Color backcolor = get_color_fix(PaletteColor(c));
            if (s->armed())
                backcolor.setAlpha(c_alpha_playing);
            else
                backcolor.setAlpha(c_alpha_muted);

            int lenw = tix_to_pix(lens);
            int h = track_height() - 1;
            int cbwoffset = cbw + h / 2 - 2;
            trigger trig;
            painter.setFont(m_font);
            s->reset_draw_trigger_marker();
            while (s->next_trigger(trig))               /* side-effect      */
            {
                if (trig.tick_end() > 0)
                {
                    int x_on = tix_to_pix(trig.tick_start());
                    int x_off = tix_to_pix(trig.tick_end());
                    int w = x_off - x_on + 1;
                    int x = x_on;
                    int xmax = x_off + 1;               /* same as x + w    */
                    int y = track_height() * seqid - 1;
                    if (use_gradient())
                    {
                        QLinearGradient grad(x, y, x, y + h + 1);
                        if (trig.selected())
                        {
                            grad.setColorAt(0.01, sel_color().darker());
                            grad.setColorAt(0.5, sel_color().lighter());
                            grad.setColorAt(0.99, sel_color().darker());
                        }
                        else
                        {
                            grad.setColorAt(0.01, backcolor.darker(150));
                            grad.setColorAt(0.5, backcolor.lighter());
                            grad.setColorAt(0.99, backcolor.darker(150));
                        }
                        pen.setStyle(Qt::SolidLine);    /* seq trigger box  */
                        pen.setWidth(2);

                        /*
                         * painter.fillRect(x, y, w, h + 1, grad);
                         */

                        QBrush gradbrush(grad);
                        gradbrush.setStyle(Qt::LinearGradientPattern);
                        painter.setBrush(gradbrush);
                        pen.setColor(fore_color());     /* use box color    */
                        painter.setPen(pen);
                        painter.drawRect(x + 1, y + 3, w - 2, h - 1);
                    }
                    else
                    {
                        if (trig.selected())
                        {
                            pen.setColor(sel_color());
                            brush.setColor(grey_color());
                        }
                        else
                        {
                            pen.setColor(fore_color());
                            brush.setColor(backcolor);
                        }
                        pen.setStyle(Qt::SolidLine);    /* seq trigger box  */
                        pen.setWidth(2);
                        brush.setStyle(Qt::SolidPattern);
                        painter.setBrush(brush);
                        painter.setPen(pen);
                        painter.drawRect(x + 1, y + 1, w - 2, h - 1);
                    }
                    brush.setStyle(Qt::NoBrush);        /* grab handle L    */
                    painter.setBrush(brush);
                    pen.setColor(fore_color());
                    painter.setPen(pen);
                    painter.drawRect(x + 1, y + 2, cbw, cbw);
                    painter.drawRect                    /* grab handle R    */
                    (
                        xmax - cbw - 1, y + h - cbw + 2, cbw, cbw
                    );
                    pen.setColor(fore_color());
                    pen.setWidth(1);
                    painter.setPen(pen);
                    if (trig.transposed())
                    {
                        char temp[16];
                        int t = trig.transpose();
                        if (t > 0)
                            snprintf(temp, sizeof temp, "+%d", t);
                        else
                            snprintf(temp, sizeof temp, "-%d", -t);

                        int tx = x + 6;
                        if (track_thin())
                            tx += 6;

                        painter.drawText(tx, y + cbwoffset, temp);
                    }

                    midipulse t = trig.trigger_marker(lens);    /* offset   */
                    while (t < trig.tick_end())
                    {
                        int note0, note1;
                        (void) s->minmax_notes(note0, note1);

                        int height = note1 - note0;
                        height += 2;
                        if (s->transposable())
                            pen.setColor(fore_color());
                        else
                            pen.setColor(drum_color());

                        painter.setPen(pen);

                        int cny = track_height() - 6;
                        int marker_x = tix_to_pix(t);
                        for (auto cev = s->cbegin(); ! s->cend(cev); ++cev)
                        {
                            sequence::note_info ni;
                            sequence::draw dt = s->get_next_note(ni, cev);
                            if (dt == sequence::draw::finish)
                                break;

                            midipulse tick_s = ni.start();
                            int sx = tick_s * lenw / lens + marker_x;
                            if (dt == sequence::draw::tempo)
                            {
                                midibpm max = usr().midi_bpm_maximum();
                                midibpm min = usr().midi_bpm_minimum();
                                double tempo = double(ni.velocity());
                                int yt = int(cny * (max-tempo) / (max-min)) + y;
                                if (sx < x)
                                    sx = x;

                                if (sx <= xmax)
                                    painter.drawEllipse(sx, yt, 3, 3);
                            }
                            else
                            {
                                midipulse tick_f = ni.finish();
                                int note_y =
                                (
                                    cny - (cny * (ni.note() - note0)) / height
                                ) + 1 + y;
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


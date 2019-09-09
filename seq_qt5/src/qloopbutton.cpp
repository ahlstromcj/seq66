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
 * \file          qloopbutton.cpp
 *
 *  This module declares/defines the base class for drawing a pattern-slot
 *  button.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-06-28
 * \updates       2019-09-09
 * \license       GNU GPLv2 or above
 *
 * QWidget::paintEvent(QPaintEvent * ev):
 *
 *  A paint event is a request to repaint all/part of a widget. It happens for
 *  the following reasons: repaint() or update() was invoked; the widget was
 *  obscured and then uncovered; or other reasons.  Widgets can repaint their
 *  entire surface, but slow widgets need to optimize by painting only the
 *  requested region: QPaintEvent::region(); painting is clipped to that region.
 *
 *  Qt tries to speed painting by merging multiple paint events into one. When
 *  update() is called several times or the window system sends several paint
 *  events, Qt merges these events into one event with a larger region
 *  [QRegion::united()]. repaint() does not permit this optimization, so use
 *  update() whenever possible.
 *
 *  When paint occurs, the update region is normally erased, so you are painting
 *  on the widget's background.
 *
 *  The qloopbutton turns off the Qt::WA_Hover attribute.  This attribute
 *  makes the button repaint whenever the mouse moves over it, which wastes
 *  CPU cycles and makes it hard to keep the button text and progress bar
 *  intact.
 */

#include <QPainter>
#include <QPaintEvent>

#include <cmath>                        /* std::sin(radians)                */

#include "cfg/settings.hpp"             /* seq66::usr().key_height(), etc.  */
#include "gui_palette_qt5.hpp"          /* seq66::gui_palette_qt5::Color    */
#include "qloopbutton.hpp"

/**
 *
 */

#define SEQ66_USE_BACKGROUND_ROLE_COLOR

/**
 *  Selects showing a sine wave versus the real data.
 */

bool s_draw_events = false;
bool s_use_sine = false;

/**
 *  Alpha values for various states, not yet members, not yet configurable.
 */

const int s_alpha_playing       = 255;
const int s_alpha_muted         = 100;
const int s_alpha_qsnap         = 180;
const int s_alpha_queued        = 148;
const int s_alpha_oneshot       = 148;

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *
 */

qloopbutton::textbox::textbox () :
    m_x         (0),
    m_y         (0),
    m_w         (0),
    m_h         (0),
    m_flags     (0),
    m_label     ()
{
    // no code
}

/**
 *
 */

void
qloopbutton::textbox::set
(
    int x, int y, int w, int h, int flags, std::string label
)
{
    m_x     = x;
    m_y     = y;
    m_w     = w;
    m_h     = h;
    m_flags = flags;
    m_label = label;
}

/**
 *
 */

qloopbutton::progbox::progbox () :
    m_x         (0),
    m_y         (0),
    m_w         (0),
    m_h         (0)
{
    // no code
}

#define PROG_W_FRACTION     0.80
#define PROG_H_FRACTION     0.25                    // 0.40

/**
 * Let's do it like seq24/seq64, but not so tall, just enough to show
 * progress.  We don't really need to keep redrawing all the events over
 * and over in miniature.
 */

void
qloopbutton::progbox::set (int w, int h)
{
    m_x = int(double(w) * (1.0 - PROG_W_FRACTION) / 2.0);
    m_y = int(double(h) * (1.0 - PROG_H_FRACTION) / 2.0);
    m_w = w - 2 * m_x;
    m_h = h - 2 * m_y;
}

/**
 *
 */

qloopbutton::qloopbutton
(
    const qslivegrid * const slotparent,
    seq::number slotnumber,
    const std::string & label,
    const std::string & hotkey,
    seq::pointer seqp,
    QWidget * parent
) :
    qslotbutton         (slotparent, slotnumber, label, hotkey, parent),
    m_fingerprint       (),
    m_fingerprint_size  (0),
    m_seq               (seqp),
    m_is_checked        (m_seq->get_playing()),
    m_prog_back_color   (Qt::black),
    m_prog_fore_color   (Qt::green),
    m_text_font         (),
    m_text_initialized  (false),
    m_draw_background   (true),
    m_top_left          (),
    m_top_right         (),
    m_bottom_left       (),
    m_bottom_right      (),
    m_progress_box      ()
{
    int fontsize = usr().scale_size(6);
    m_text_font.setPointSize(fontsize);
    m_text_font.setBold(true);
    m_text_font.setLetterSpacing(QFont::AbsoluteSpacing, 1);
    make_checkable();
    set_checked(m_is_checked);
}

/**
 *
 */

bool
qloopbutton::initialize_text ()
{
    bool result = ! m_text_initialized;
    if (result)
    {
        int w = width();
        int h = height();
        int dx = usr().scale_size(2);
        int dy = usr().scale_size(2);
        int lw = int(0.70 * w);
        int rw = int(0.50 * w);
        int lx = dx;                            /* left x       */
        int ty = dy;                            /* top y        */
        int bh = usr().scale_size(12);          /* box height   */
        int rx = int(0.50 * w) + lx - dx;       /* right x      */
        int by = int(0.85 * h);                 /* bottom y     */

        /*
         * Code from performer::sequence_label().
         */

        bussbyte bus = m_seq->get_midi_bus();
        int chan = m_seq->is_smf_0() ? 0 : m_seq->get_midi_channel() + 1;
        int bpb = int(m_seq->get_beats_per_bar());
        int bw = int(m_seq->get_beat_width());
        int sn = m_seq->seq_number();
        int lflags = Qt::AlignLeft | Qt::AlignVCenter;
        int rflags = Qt::AlignRight | Qt::AlignVCenter;
        std::string lengthstr = std::to_string(m_seq->get_measures());
        std::string lowerleft, hotkey;
        char tmp[32];
        if (rc().show_ui_sequence_number())
            snprintf(tmp, sizeof tmp, "%-3d %d-%d %d/%d", sn, bus, chan, bpb, bw);
        else
            snprintf(tmp, sizeof tmp, "%d-%d %d/%d", bus, chan, bpb, bw);

        lowerleft = std::string(tmp);
        if (rc().show_ui_sequence_key())
            hotkey = m_hotkey;

        m_top_left.set(lx, ty, lw, bh, lflags, m_seq->name());
        m_top_right.set(rx, ty, rw, bh, rflags, lengthstr);
        m_bottom_left.set(lx, by, lw, bh, lflags, lowerleft);
        m_bottom_right.set(rx, by, rw, bh, rflags, hotkey);
        m_progress_box.set(w, h);
        m_text_initialized = true;
    }
    else
        result = m_text_initialized;

    return result;
}

/**
 *  Creates and array of absolute locations for the sine-wave in the
 *  progress-box.
 */

void
qloopbutton::initialize_sine_table ()
{
    if (m_fingerprint_size == 0)
    {
        int count = int(sizeof(m_fingerprint) / sizeof(int));
        int y = m_progress_box.y();
        int h = m_progress_box.h();
        double rmax = 2.0 * M_PI;
        double dr = rmax / double(count);
        int i;
        double r;
        for (i = 0, r = 0.0; i < count; ++i, r += dr)
            m_fingerprint[i] = y + int((1.0 + sin(r)) * h) / 2;

        m_fingerprint_size = count;
    }
}

/**
 *
 */

void
qloopbutton::initialize_fingerprint ()
{
    int n0, n1;
    bool have_notes = m_seq->minmax_notes(n0, n1);
    if (m_fingerprint_size == 0 && have_notes)
    {
        int i1 = int(sizeof(m_fingerprint) / sizeof(int));
        int x0 = m_progress_box.m_x + 3;
        int x1 = x0 + m_progress_box.m_w - 6;
        int xw = x1 - x0;
        int y0 = m_progress_box.m_y + 1;
        int y1 = y0 + m_progress_box.m_h - 2;
        int yh = y1 - y0;
        midipulse t1 = m_seq->get_length();             /* t0 = 0           */
        int nh = SEQ66_MAX_DATA_VALUE;
        for (int i = 0; i < i1; ++i)
            m_fingerprint[i] = 0;

        /*
         * Added an octave of padding above and below for looks.
         */

        n1 += 12;
        if (n1 > SEQ66_MAX_DATA_VALUE)
            n1 = SEQ66_MAX_DATA_VALUE;

        n0 -= 12;
        if (n0 < 0)
            n0 = 0;

        nh = n1 - n0;
        m_seq->reset_draw_marker();                     /* loop iterator    */
        for (;;)
        {
            sequence::note_info ni;                     /* two members used */
            sequence::draw dt = m_seq->get_next_note(ni);
            if (dt != sequence::draw::finish)
            {
                int x = (ni.start() * xw) / t1 + x0;    /* ni.finish()      */
                int y = yh * (ni.note() - n0) / nh + y0;
                int i = i1 * (x - x0) / xw;
                if (i < 0)
                    i = 0;
                else if (i > i1)
                    i = i1;

                m_fingerprint[i] = y;
            }
            else
                break;
        }
        m_fingerprint_size = i1;
    }
    /*
        else
        {
            int y = m_progress_box.y();
            for (int i = 0; i < count; ++i)
                m_fingerprint[i] = y;
        }
    */
}

/**
 *
 */

void
qloopbutton::setup ()
{
    QPalette pal = palette();
    int c = m_seq ? m_seq->color() : color_to_int(NONE) ;
    if (c == color_to_int(BLACK))
    {
        pal.setColor(QPalette::Button, QColor(Qt::black));
        pal.setColor(QPalette::ButtonText, QColor(Qt::yellow));
    }
    else
    {
        gui_palette_qt5::Color backcolor =
            slotpal().get_color_fix(PaletteColor(c));

        /*
         * Rather than having a black progress area, make it match the specified
         * sequence color.
         *
         *      pal.setColor(QPalette::Button, backcolor);
         *
         *  However, it comes out unfixed, actually a good effect.
         */

        pal.setColor(QPalette::Button, backcolor);
        m_prog_back_color = backcolor;
    }
    if (usr().grid_is_white())
        setAutoFillBackground(false);
    else
        setAutoFillBackground(true);

    setPalette(pal);
    setEnabled(true);
    setCheckable(is_checkable());
    setAttribute(Qt::WA_Hover, false);              /* avoid nasty repaints */
}

/**
 *
 */

void
qloopbutton::set_checked (bool flag)
{
    m_is_checked = flag;
    setChecked(flag);
}

/**
 *
 */

bool
qloopbutton::toggle_checked ()
{
    bool result = m_seq->toggle_playing();
    set_checked(result);
    reupdate();
    return result;
}

/**
 *  Call the update() function of this button.
 *
 * \param all
 *      If true, the whole button is updated.  Otherwise, only the progress box
 *      is updated.  The default is true.
 */

void
qloopbutton::reupdate (bool all)
{
    if (all)
    {
        update();
    }
    else
    {
        int x = m_progress_box.m_x;
        int y = m_progress_box.m_y;
        int w = m_progress_box.m_w;
        int h = m_progress_box.m_h;
        update(x, y, w, h);
    }
}

/**
 *  Draws the text and progress panel.
 *
 *           ----------------------------------
 *          | Title                     Length |
 *          |                                  |
 *          |                                  |
 *          |                                  |
 *          | buss-chan 4/4             hotkey |
 *           ----------------------------------
 *
 *  Note that we first call QPushButton::paintEvent(pev) to make sure that the
 *  click highlights/unhighlight this checkable button.  And this call muse be
 *  done first, otherwise the application segfaults.
 */

void
qloopbutton::paintEvent (QPaintEvent * pev)
{
    if (is_dirty())
    {
        QPushButton::paintEvent(pev);
        QPainter painter(this);
        if (m_seq)
        {
            midipulse tick = m_seq->get_last_tick();    // perf().get_tick()
            if (initialize_text() || tick == 0)
            {
                QRectF box
                (
                    m_top_left.m_x, m_top_left.m_y,
                    m_top_left.m_w, m_top_left.m_h
                );
                QString title(m_top_left.m_label.c_str());
                QPen pen(Qt::black);
                QBrush brush(Qt::black);

#if defined SEQ66_USE_BACKGROUND_ROLE_COLOR

                /*
                 * This color gets the background we painted, not the background
                 * actually shown by the current Qt 5 theme.
                 *
                 *      QColor trueback = this->palette().button().color();
                 *
                 */

                QWidget * rent = this->parentWidget();
                if (not_nullptr(rent))
                {
                    QColor trueback = rent->palette().color(QPalette::Background);
                    trueback = gui_palette_qt5::calculate_inverse(trueback);
                    pen.setColor(trueback);
                }
#endif

                // int fontsize = usr().scale_size(6); // EXPERIMENT
                // m_text_font.setPointSize(fontsize);

                painter.setPen(pen);
                painter.setBrush(brush);
                painter.setFont(m_text_font);
                painter.drawText(box, m_top_left.m_flags, title);
                title = m_top_right.m_label.c_str();
                box.setRect
                (
                    m_top_right.m_x, m_top_right.m_y,
                    m_top_right.m_w, m_top_right.m_h
                );
                painter.drawText(box, m_top_right.m_flags, title);

                title = m_bottom_left.m_label.c_str();
                box.setRect
                (
                    m_bottom_left.m_x, m_bottom_left.m_y,
                    m_bottom_left.m_w, m_bottom_left.m_h
                );
                painter.drawText(box, m_bottom_left.m_flags, title);

                title = m_bottom_right.m_label.c_str();
                box.setRect
                (
                    m_bottom_right.m_x, m_bottom_right.m_y,
                    m_bottom_right.m_w, m_bottom_right.m_h
                );
                painter.drawText(box, m_bottom_right.m_flags, title);

                if (m_seq->get_playing())
                    title = "Armed";
                else if (m_seq->get_queued())
                    title = "Queued";
                else if (m_seq->one_shot())
                    title = "One-shot";
                else
                    title = "Muted";

                box.setRect
                (
                    m_top_left.m_x, m_top_left.m_y + 12,
                    m_top_left.m_w, m_top_left.m_h
                );
                painter.drawText(box, m_top_left.m_flags, title);
            }
            if (s_use_sine)
                initialize_sine_table();
            else
                initialize_fingerprint();

            draw_progress_box(painter);
            draw_pattern(painter);
            draw_progress(painter, tick);
        }
        else
        {
            std::string snstring = std::to_string(m_slot_number);
            snstring += ": NO LOOP!";
            setEnabled(false);
            setText(snstring.c_str());
        }
    }
}

/**
 *      Draws the progress box, progress bar, and and indicator for non-empty
 *      pattern slots.
 *
 * Idea:
 *
 *      Merge this inside of drawing the events!
 */

void
qloopbutton::draw_progress (QPainter & painter, midipulse tick)
{
    QBrush brush(m_prog_back_color, Qt::SolidPattern);
    QPen pen(Qt::black);
    midipulse t1 = m_seq->get_length();
    int lx = m_progress_box.m_x + 3;
    int xw = m_progress_box.m_w - 6;
    int ly0 = m_progress_box.m_y + 1;
    int lyh = m_progress_box.m_h - 2;
    int ly1 = ly0 + lyh;
    lx += int(xw * tick / t1);
    pen.setWidth(2);
    pen.setStyle(Qt::SolidLine);
    painter.setBrush(brush);
    painter.setPen(pen);
    painter.drawLine(lx, ly1, lx, ly0);
}

/**
 *      Draws the progress box, progress bar, and and indicator for non-empty
 *      pattern slots.
 */

void
qloopbutton::draw_progress_box (QPainter & painter)
{
    QBrush brush(m_prog_back_color, Qt::SolidPattern);
    QPen pen(text_color());
    const int penwidth = 2;
    bool qsnap = m_seq->snap_it();
    int c = m_seq->color();
    if (c == color_to_int(BLACK))
    {
        // pal.setColor(QPalette::Button, QColor(Qt::black));
        // pal.setColor(QPalette::ButtonText, QColor(Qt::yellow));
    }

    gui_palette_qt5::Color backcolor = slotpal().get_color_fix(PaletteColor(c));
    if (qsnap)                                      /* playing, queued, ... */
    {
        backcolor.setAlpha(s_alpha_qsnap);
        pen.setColor(Qt::gray);                     /* instead of Qt::black */
        pen.setStyle(Qt::SolidLine);
    }
    else if (m_seq->get_playing())                  /* armed, playing       */
    {
        backcolor.setAlpha(s_alpha_playing);
    }
    else if (m_seq->get_queued())
    {
        backcolor.setAlpha(s_alpha_queued);
        pen.setWidth(penwidth);
        pen.setStyle(Qt::SolidLine);
    }
    else if (m_seq->one_shot())                     /* one-shot queued      */
    {
        backcolor.setAlpha(s_alpha_oneshot);
        pen.setColor(Qt::darkGray);
        pen.setStyle(Qt::DotLine);
    }
    else                                            /* unarmed, muted       */
    {
        backcolor.setAlpha(s_alpha_muted);
        pen.setStyle(Qt::SolidLine);
    }
    brush.setColor(backcolor);
    pen.setWidth(penwidth);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.drawRect
    (
        m_progress_box.m_x, m_progress_box.m_y,
        m_progress_box.m_w, m_progress_box.m_h
    );
}

/**
 *      Draws the progress box, progress bar, and and indicator for non-empty
 *      pattern slots.
 */

void
qloopbutton::draw_pattern (QPainter & painter)
{
    if (m_seq->event_count() > 0)
    {
        QBrush brush(m_prog_back_color, Qt::SolidPattern);
        QPen pen(text_color());
        int lx0 = m_progress_box.m_x + 3;
        int ly0 = m_progress_box.m_y + 1;
        int lxw = m_progress_box.m_w - 6;
        int lyh = m_progress_box.m_h - 2;
        midipulse t1 = m_seq->get_length();
        pen.setWidth(1);
        if (s_draw_events)
        {
            int lowest, highest;
            bool have_notes = m_seq->minmax_notes(lowest, highest);
            int height = SEQ66_MAX_DATA_VALUE;
            if (have_notes)
            {
                /*
                 * Added an octave of padding above and below for looks.
                 */

                highest += 12;
                if (highest > SEQ66_MAX_DATA_VALUE)
                    highest = SEQ66_MAX_DATA_VALUE;

                lowest -= 12;
                if (lowest < 0)
                    lowest = 0;

                height = highest - lowest;
            }

            if (! m_seq->transposable())
                pen.setColor(Qt::red);

            pen.setWidth(1);
            m_seq->reset_draw_marker();             /* reset loop iterator  */
            for (;;)
            {
                sequence::note_info ni;             /* only 2 members used! */
                sequence::draw dt = m_seq->get_next_note(ni);
                int tick_s_x = (ni.start() * lxw) / t1;
                int tick_f_x = (ni.finish() * lxw) / t1;
                if (dt == sequence::draw::finish)
                    break;

                if (! sequence::is_draw_note(dt))
                    tick_f_x = tick_s_x + 1;
                else if (tick_f_x <= tick_s_x)
                    tick_f_x = tick_s_x + 1;

                int y = lyh * (ni.note() - lowest) / height;

#if defined DRAW_TEMPO_LINE
                if (dt == sequence::draw::tempo)
                {
                    pen.setWidth(2);
                    pen.setColor(Qt::magenta);      /* tempo_paint()    */
                }
                else
                {
                    pen.setWidth(1);
                    pen.setColor(Qt::black);
                }
#endif

                int sx = lx0 + tick_s_x;            /* start x          */
                int fx = lx0 + tick_f_x;            /* finish x         */
                y += ly0;                           /* start & finish y */
                painter.setPen(pen);
                painter.drawLine(sx, y, fx, y);
            }
        }
        else
        {
            /*
             * Draw simple wave to indicate non-empty button.  We check the
             * sequence event-count.  Another, slower, check is
             * m_seq->minmax_notes(lo, hi))
             */

            if (m_fingerprint_size > 0)
            {
                int x = m_progress_box.m_x + 4;
                int dx = m_progress_box.m_w / (m_fingerprint_size - 1);
                for (int i = 0; i < m_fingerprint_size; ++i, x += dx)
                {
                    int y = m_fingerprint[i];
                    if (y > 0)
                        painter.drawRect(x, y, 1, 1);
                }
            }
        }
    }
}

/**
 *
 */

void
qloopbutton::focusInEvent (QFocusEvent *)
{
    m_text_initialized = false;        // occurs only with a click
}

/**
 *
 */

void
qloopbutton::focusOutEvent (QFocusEvent *)
{
    m_text_initialized = false;        // occurs only with a click
}

}           // namespace seq66

/*
 * qloopbutton.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


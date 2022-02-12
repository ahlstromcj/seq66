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
 * \file          qloopbutton.cpp
 *
 *  This module declares/defines the base class for drawing a pattern-slot
 *  button.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-06-28
 * \updates       2022-02-12
 * \license       GNU GPLv2 or above
 *
 *  A paint event is a request to repaint all/part of a widget. It happens for
 *  the following reasons: repaint() or update() was invoked; the widget was
 *  obscured and then uncovered; or other reasons.  Widgets can repaint their
 *  entire surface, but slow widgets need to optimize by painting only the
 *  requested region: QPaintEvent::region(); painting is clipped to that
 *  region.
 *
 *  Qt tries to speed painting by merging multiple paint events into one. When
 *  update() is called several times or the window system sends several paint
 *  events, Qt merges these events into one event with a larger region
 *  [QRegion::united()]. repaint() does not permit this optimization, so use
 *  update() whenever possible.
 *
 *  When paint occurs, the update region is normally erased, so you are
 *  painting on the widget's background.
 *
 *  The qloopbutton turns off the Qt::WA_Hover attribute.  This attribute
 *  makes the button repaint whenever the mouse moves over it, which wastes
 *  CPU cycles and makes it hard to keep the button text and progress bar
 *  intact.
 */

#include <QPainter>
#include <QPaintEvent>
#include <cmath>                        /* std::sin(radians)                */

#include "cfg/settings.hpp"             /* seq66::usr().scale_size(), etc.  */
#include "qloopbutton.hpp"              /* seq66::qloopbutton slot          */
#include "qt5_helpers.hpp"              /* seq66::qt(), qt_set_icon() etc.  */

/**
 *  An attempt to use the inverse of the background color for drawing text.
 *  It doesn't work with some GTK themes.  Use Qt style sheets or a 'palette'
 *  file instead.  The background border can look garish, too.
 */

#undef SEQ66_USE_BACKGROUND_ROLE_COLOR
#undef SEQ66_COLOR_BUTTON_BORDERS

/**
 *  Alpha values for various states, not yet members, not yet configurable.
 */

static const int s_alpha_playing       = 255;
static const int s_alpha_muted         = 100;
static const int s_alpha_qsnap         = 180;
static const int s_alpha_queued        = 148;
static const int s_alpha_oneshot       = 148;

/**
 *  Font and annunciator sizes.  These are for normal size, and get scaled for
 *  other sizes.
 */

static const int s_fontsize_main    = 7;
static const int s_fontsize_record  = 5;
static const int s_radius_record    = 8;

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Textbox functions.
 */

qloopbutton::textbox::textbox () :
    m_x     (0),
    m_y     (0),
    m_w     (0),
    m_h     (0),
    m_flags (0),
    m_label ()
{
    // no code
}

void
qloopbutton::textbox::set
(
    int x, int y, int w, int h, int flags, std::string label
)
{
    m_x = x;  m_y = y;  m_w = w;  m_h = h;
    m_flags = flags;
    m_label = label;
}

qloopbutton::progbox::progbox () :
    m_x (0),
    m_y (0),
    m_w (0),
    m_h (0)
{
    // no code
}

/**
 * Let's do it like seq24/seq64, but not so tall, just enough to show progress.
 */

void
qloopbutton::progbox::set (int w, int h)
{
    m_x = int(double(w) * (1.0 - sm_progress_w_fraction) / 2.0);
    m_y = int(double(h) * (1.0 - sm_progress_h_fraction) / 2.0);
    m_w = w - 2 * m_x;
    m_h = h - 2 * m_y;
}

/**
 *  Progress-box values.
 */

bool qloopbutton::sm_draw_progress_box = true;
double qloopbutton::sm_progress_w_fraction = 0.80;      /* 0.0, 0.50 - 0.80 */
double qloopbutton::sm_progress_h_fraction = 0.25;      /* 0.0, 0.10 - 0.40 */
const int qloopbutton::scm_progress_event_margin = 3;   /* better viewing   */

/**
 *  Principal constructor.
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
    qslotbutton             (slotparent, slotnumber, label, hotkey, parent),
    m_show_average          (false),                /* last versus average  */
    m_fingerprint_inited    (false),
    m_fingerprinted         (false),
    m_fingerprint_size      (usr().fingerprint_size()),
    m_fingerprint           (m_fingerprint_size),   /* reserve vector space */
    m_fingerprint_count     (m_fingerprint_size),
    m_note_min              (usr().progress_note_min()),
    m_note_max              (usr().progress_note_max()),
    m_seq                   (seqp),                 /* loop()               */
    m_is_checked            (loop()->playing()),
    m_prog_thickness        (usr().progress_bar_thick() ? 2 : 1),
    m_prog_back_color       (Qt::black),
    m_prog_fore_color       (Qt::green),
    m_text_font             (),
    m_text_initialized      (false),
    m_draw_background       (true),
    m_top_left              (),
    m_top_right             (),
    m_bottom_left           (),
    m_bottom_right          (),
    m_progress_box          (),
    m_event_box             ()
{
    m_text_font.setBold(usr().progress_bar_thick());
    m_text_font.setLetterSpacing(QFont::AbsoluteSpacing, 1);
    make_active();
    make_checkable();
    set_checked(m_is_checked);

#if defined SEQ66_FOLLOW_THEME_TEXT_COLOR
    /*
     * To mitigate issue #50, where the white text of a dark theme cannot be seen
     * against a yellow background.  Instead, we beefed up the palette mechanism.
     */

    QWidget tmp;
    text_color(tmp.palette().color(QPalette::ButtonText));
#else
    text_color(foreground_paint());
#endif

    int c = loop() ? loop()->color() : palette_to_int(none) ;
    pen_color(get_pen_color(PaletteColor(c)));
    if (c != palette_to_int(black))
        back_color(get_color_fix(PaletteColor(c)));
}

/**
 *  Static function to control when the qloopbutton text boxes get
 *  initialized. we want the statics to be initialized at "start up" and
 *  when the main window is resized.
 */

bool
qloopbutton::boxes_initialized (bool reset)
{
    static bool s_boxes_initialized = false;
    if (reset)
    {
        s_boxes_initialized = false;
        return false;
    }
    else
    {
        bool result = s_boxes_initialized;
        s_boxes_initialized = true;
        return result;
    }
}

void
qloopbutton::progress_box_size (double w, double h)
{
    if (w == 0.0 || h == 0.0)
    {
        sm_draw_progress_box = false;
    }
    else
    {
        if (w >= 0.50 && w <=1.0)
            sm_progress_w_fraction = w;

        if (h >= 0.10 && h <=1.0)
            sm_progress_h_fraction = h;
    }
}

/**
 *  We can set up the relative locations and the box sizes just once every
 *  time the UI size is re-established.
 *
 *  Variables: l = left, r = right, t = top, and b = bottom
 */

bool
qloopbutton::initialize_text ()
{
    bool result = ! m_text_initialized;
    if (result)
    {
        int w = width();
        int h = height();
        int dx = usr().scale_size(6);
        int dy = usr().scale_size_y(2);
        int lw = int(0.70 * w);
        int rw = int(0.50 * w);
        int lx = dx + 1;
        int ty = dy + 2;                                // ty = dy;
        int bh = usr().scale_size_y(12);
        int rx = int(0.50 * w) + lx - 2 * dx - 2;       // - 2 added
        int by = int(0.85 * h) + dy;
        int fontsize = usr().scale_font_size(s_fontsize_main);
        if (vert_compressed())
        {
            ty += 2;
            by -= 2;
        }
        if (horiz_compressed())
        {
            lx += 2;
            rx -= 2;
        }
        m_progress_box.set(w, h);
        m_event_box = m_progress_box;
        m_event_box.m_x += scm_progress_event_margin;
        m_event_box.m_y += scm_progress_event_margin;
        m_event_box.m_w -= scm_progress_event_margin * 2;
        m_event_box.m_h -= scm_progress_event_margin * 2;
        m_text_font.setPointSize(fontsize);

        /*
         * Code from performer::sequence_label().
         */

        bussbyte bus = loop()->seq_midi_bus();
        int bpb = int(loop()->get_beats_per_bar());
        int bw = int(loop()->get_beat_width());
        int sn = loop()->seq_number();
        int lflags = Qt::AlignLeft | Qt::AlignVCenter;
        int rflags = Qt::AlignRight | Qt::AlignVCenter;
        std::string lengthstr = std::to_string(loop()->get_measures());
        std::string chanstr = loop()->channel_string();
        std::string lowerleft, hotkey;
        char tmp[32];
        snprintf
        (
            tmp, sizeof tmp, "%-3d %d-%s %d/%d",
            sn, bus, chanstr.c_str(), bpb, bw
        );
        lowerleft = std::string(tmp);
        hotkey = m_hotkey;
        if (loop()->loop_count_max() > 0)
            lengthstr += "*";

        m_top_left.set(lx, ty, lw, bh, lflags, loop()->name());
        m_top_right.set(rx, ty, rw, bh, rflags, lengthstr);
        m_bottom_left.set(lx, by, lw, bh, lflags, lowerleft);
        m_bottom_right.set(rx, by, rw, bh, rflags, hotkey);
        m_text_initialized = true;
    }
    else
        result = m_text_initialized;

    return result;
}

/**
 *  This function examines the current sequence to determine how many notes it
 *  has, and the range of note values (pitches).
 */

void
qloopbutton::initialize_fingerprint ()
{
    const int i1 = int(m_fingerprint_size);
    if (! m_fingerprint_inited && i1 > 0)
    {
        int n0, n1;
        bool have_notes = loop()->minmax_notes(n0, n1); /* fill n0 and n1   */
        if (have_notes)
            have_notes = loop()->event_threshold();

        if (have_notes)
        {
            midipulse t1 = loop()->get_length();        /* t0 = 0           */
            if (t1 == 0)
                return;

            int x0 = m_event_box.x();
            int x1 = x0 + m_event_box.w();
            int xw = x1 - x0;
            int y0 = m_event_box.y();
            int y1 = y0 + m_event_box.h();
            int yh = y1 - y0;
            if (m_note_max > 0)
            {
                n0 = m_note_min;
                n1 = m_note_max;
            }

            /*
             * Added an octave of padding above and below for looks. Also use
             * n0 and n1 as the min/max notes of the whole sequence.
             */

            n0 -= 12 / 6;
            n0 = clamp_midibyte_value(n0);
            n1 += 12 / 6;
            n1 = clamp_midibyte_value(n1);
            for (int i = 0; i < i1; ++i)
                m_fingerprint[i] = m_fingerprint_count[i] = 0;

            int nh = n1 - n0;
            for (auto cev = loop()->cbegin(); ! loop()->cend(cev); ++cev)
            {
                sequence::note_info ni;
                sequence::draw dt = loop()->get_next_note(ni, cev);
                if (dt == sequence::draw::finish)
                    break;

                int x = x0 + (ni.start() * xw) / t1;
                int y = y0 + yh * (ni.note() - n0) / nh;
                int i = i1 * (x - x0) / xw;
                if (i < 0)
                    i = 0;
                else if (i >= i1)
                    i = i1 - 1;

                if (m_show_average)     /* not sure how useful this is  */
                {
                    ++m_fingerprint_count[i];
                    m_fingerprint[i] += midishort(y);
                }
                else
                    m_fingerprint[i] = midishort(y);
            }
            for (int i = 0; i < i1; ++i)
            {
                if (m_fingerprint_count[i] > 1)
                    m_fingerprint[i] /= m_fingerprint_count[i];
            }
            m_fingerprinted = true;
        }
    }
    m_fingerprint_inited = true;
}

/**
 *  Sets up the foreground and background colors of the button and the
 *  appropriate setAutoFillBackground() setting. We've macroed out the garish
 *  painting of the pattern color on the button borders.
 */

void
qloopbutton::setup ()
{
#if defined SEQ66_COLOR_BUTTON_BORDERS
    QPalette pal = palette();
#endif
    int c = loop() ? loop()->color() : palette_to_int(none) ;
    if (c == palette_to_int(black))
    {
#if defined SEQ66_COLOR_BUTTON_BORDERS
        pal.setColor(QPalette::Button, QColor(Qt::black));
        pal.setColor(QPalette::ButtonText, QColor(Qt::yellow)); /* useless  */
#endif
    }
    else
    {
        /*
         *  This sets the color to the background, which results in an ugly
         *  colored border around the button in many themes.
         */

        Color backcolor = get_color_fix(PaletteColor(c));
#if defined SEQ66_COLOR_BUTTON_BORDERS
        pal.setColor(QPalette::Button, backcolor);
#endif
        m_prog_back_color = backcolor;
    }
#if defined SEQ66_COLOR_BUTTON_BORDERS
    setAutoFillBackground(true);
    setPalette(pal);
#endif
    setEnabled(true);
    setCheckable(is_checkable());
    setAttribute(Qt::WA_Hover, false);              /* avoid nasty repaints */
}

void
qloopbutton::set_checked (bool flag)
{
    m_is_checked = flag;
    setChecked(flag);
}

bool
qloopbutton::toggle_enabled ()
{
    bool enabled = ! isEnabled();
    setEnabled(enabled);
    return true;
}

bool
qloopbutton::toggle_checked ()
{
    bool result = loop()->sequence_playing_toggle();
    if (result)
    {
        bool checked = loop()->playing();
        set_checked(checked);
        reupdate();
    }
    return result;
}

/**
 *  Call the update() function of this button.
 *
 * \param all
 *      If true, the whole button is updated.  Otherwise, only the progress
 *      box is updated.  The default is true.
 */

void
qloopbutton::reupdate (bool all)
{
    if (is_active())           /* NEW ca 2021-11-14 */
    {
        if (all)
        {
            m_text_initialized = false;
            if (initialize_text())
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
}

/**
 *  Draws the text and progress panel.
 *
\verbatim
             ----------------------------
       top  | Title               Length |
            | Armed                      |
            |        ------------        |
            |       |  P A N E L |       |
            |        ------------        |
            |                            |
    bottom  | buss-chan 4/4       hotkey |
             ----------------------------
\endverbatim
 *
 *  Note that we first call QPushButton::paintEvent(pev) to make sure that the
 *  click highlights/unhighlight this checkable button.  And this call must be
 *  done first, otherwise the application segfaults.
 */

void
qloopbutton::paintEvent (QPaintEvent * pev)
{
    QPushButton::paintEvent(pev);
    QPainter painter(this);
    if (loop())
    {
        midipulse tick = loop()->get_last_tick();
        if (initialize_text() || tick == 0)
        {
            QRectF box
            (
                m_top_left.m_x, m_top_left.m_y,
                m_top_left.m_w, m_top_left.m_h
            );
            QString title(qt(m_top_left.m_label));
            painter.setPen(label_color());      /* text issue #50   */
            painter.setFont(m_text_font);

#if defined SEQ66_USE_BACKGROUND_ROLE_COLOR
            QPen pen(text_color());             /* label_color()    */
            QBrush brush(Qt::black);
            painter.setBrush(brush);

            /*
             * This call gets the background we painted, not the background
             * actually shown by the current Qt 5 theme.
             *
             *      QColor trueback = this->palette().button().color();
             */

            QWidget * rent = this->parentWidget();
            if (not_nullptr(rent))
            {
                QColor trueback = rent->palette().color(QPalette::Background);
                trueback = gui_palette_qt5::calculate_inverse(trueback);
                pen.setColor(trueback);
            }
            painter.setPen(pen);
#endif

            painter.drawText(box, m_top_left.m_flags, title);
            title = qt(m_top_right.m_label);
            box.setRect
            (
                m_top_right.m_x, m_top_right.m_y,
                m_top_right.m_w, m_top_right.m_h
            );
            painter.drawText(box, m_top_right.m_flags, title);
            if (loop()->recording())
            {
                int radius = usr().scale_size(s_radius_record);
                int clx = m_top_right.m_x + m_top_right.m_w - radius - 2;
                int cly = m_top_right.m_y + m_top_right.m_h;
                int tlx = clx + usr().scale_size(2);
                int tly = cly + radius - usr().scale_size(2);
                QPen pen2(drum_paint());
                QBrush brush(drum_paint(), Qt::SolidPattern);
                painter.save();         // NEW
                painter.setPen(pen2);
                painter.setBrush(brush);
                painter.drawEllipse(clx, cly, radius, radius);
                if (loop()->quantizing_or_tightening())
                {
                    int fontsize = usr().scale_font_size(s_fontsize_record);
                    QFont font;
                    font.setPointSize(fontsize);
                    font.setBold(true);
                    painter.setPen(Qt::black);
                    painter.setFont(font);
                    if (loop()->quantizing())
                        painter.drawText(tlx, tly, "Q");
                    else if (loop()->tightening())
                        painter.drawText(tlx + 2, tly, "T");
                }
                painter.restore();      // NEW
            }
            title = qt(m_bottom_left.m_label);
            box.setRect
            (
                m_bottom_left.m_x, m_bottom_left.m_y,
                m_bottom_left.m_w, m_bottom_left.m_h
            );
            painter.drawText(box, m_bottom_left.m_flags, title);

            title = qt(m_bottom_right.m_label);
            box.setRect
            (
                m_bottom_right.m_x, m_bottom_right.m_y,
                m_bottom_right.m_w, m_bottom_right.m_h
            );
            painter.drawText(box, m_bottom_right.m_flags, title);
            set_checked(loop()->playing()); /* gets hot-key toggle to show  */
            if (loop()->playing())
                title = "Armed";
            else if (loop()->get_queued())
                title = "Queued";
            else if (loop()->one_shot())
                title = "One-shot";
            else
                title = "Muted";

            int line2y = 2 * usr().scale_font_size(6);
            box.setRect
            (
                m_top_left.m_x, m_top_left.m_y + line2y,
                m_top_left.m_w, m_top_left.m_h
            );
            painter.drawText(box, m_top_left.m_flags, title);
            initialize_fingerprint();
        }
        if (sm_draw_progress_box)
            draw_progress_box(painter);

        draw_pattern(painter);
        if (loop()->is_playable() && loop()->armed())
            draw_progress(painter, tick);
    }
    else
    {
        std::string snstring = std::to_string(m_slot_number);
        snstring += ": NO LOOP!";
        setEnabled(false);
        setText(qt(snstring));
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
    midipulse t1 = loop()->get_length();
    if (t1 > 0)
    {
        QBrush brush(m_prog_back_color, Qt::SolidPattern);
        QPen pen(progress_color());                         /* Qt::black */
        int x = m_event_box.x();
        int w = m_event_box.w();
        int y0 = m_event_box.y() + 1;
        int yh = m_event_box.h() - 2;
        int y1 = y0 + yh;
        x += int(w * tick / t1);
        pen.setWidth(m_prog_thickness);
        pen.setStyle(Qt::SolidLine);
        painter.setBrush(brush);
        painter.setPen(pen);
        painter.drawLine(x, y1, x, y0);
    }
}

/**
 *      Draws the progress box, progress bar, and and indicator for non-empty
 *      pattern slots.
 */

void
qloopbutton::draw_progress_box (QPainter & painter)
{
    QBrush brush(m_prog_back_color, Qt::SolidPattern);
    QPen pen(pen_color());                          /* #50: text_color()    */
    const int penwidth = m_prog_thickness;          /* 2 */
    bool qsnap = loop()->snap_it();
    Color backcolor = back_color();
    if (qsnap)                                      /* playing, queued, ... */
    {
        backcolor.setAlpha(s_alpha_qsnap);
        pen.setColor(Qt::gray);                     /* instead of Qt::black */
        pen.setStyle(Qt::SolidLine);
    }
    else if (loop()->playing())                     /* armed, playing       */
    {
        backcolor.setAlpha(s_alpha_playing);
    }
    else if (loop()->get_queued())
    {
        backcolor.setAlpha(s_alpha_queued);         // pen.setWidth(penwidth);
        pen.setStyle(Qt::SolidLine);
    }
    else if (loop()->one_shot())                   /* one-shot queued      */
    {
        backcolor.setAlpha(s_alpha_oneshot);
        pen.setColor(Qt::darkGray);
        pen.setStyle(Qt::DotLine);
    }
    else                                           /* unarmed, muted       */
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
        m_progress_box.x(), m_progress_box.y(),
        m_progress_box.w(), m_progress_box.h()
    );
}

/**
 *  Draws the progress box, progress bar, and and indicator for non-empty
 *  pattern slots.  Two style of drawing are done:
 *
 *      -   If sequence::event_threshold() is true, then the calculated
 *          number of measures is greater than 4.  We don't need to draw the
 *          whole set of notes.  Instead, we draw the calculated finger print.
 *      -   Otherwise, the sequence is short and we draw it normally.
 */

void
qloopbutton::draw_pattern (QPainter & painter)
{
    midipulse t1 = loop()->get_length();
    if (loop()->event_count() > 0 && t1 > 0)
    {
        QBrush brush(m_prog_back_color, Qt::SolidPattern);
        QPen pen(text_color());
        int x0 = m_event_box.x();
        int y0 = m_event_box.y();
        int xw = m_event_box.w();
        int yh = m_event_box.h();
        if (m_fingerprinted)
        {
            if (loop()->transposable())
                pen.setColor(pen_color());              /* not text_color() */
            else
                pen.setColor(drum_color());

            float x = float(x0);
            float dx = float(xw) / (m_fingerprint_size - 1);
            pen.setWidth(2);
            painter.setPen(pen);
            for (int i = 0; i < int(m_fingerprint_size); ++i, x += dx)
            {
                midishort fp = m_fingerprint[i];
                if (fp > 0 && fp != max_midibyte())
                    painter.drawPoint(int(x), int(fp));
            }
        }
        else
        {
            int height = max_midi_value();
            int n0, n1;
            if (m_note_max > 0)
            {
                n0 = m_note_min;
                n1 = m_note_max;
            }
            else
            {
                bool have_notes = loop()->minmax_notes(n0, n1);
                if (have_notes)
                {
                    /*
                     * Added an octave of padding above and below for looks.
                     */

                    n0 -= 12;
                    n0 = clamp_midibyte_value(n0);
                    n1 += 12;
                    n1 = clamp_midibyte_value(n1);
                }
                else
                {
                    n0 = 0;
                    n1 = max_midi_value();
                }
            }
            height = n1 - n0;
            pen.setWidth(1);
            if (loop()->transposable())
                pen.setColor(pen_color());      /* issue #50 text_color()   */
            else
                pen.setColor(drum_color());

            painter.setPen(pen);
            for (auto cev = loop()->cbegin(); ! loop()->cend(cev); ++cev)
            {
                sequence::note_info ni;
                sequence::draw dt = loop()->get_next_note(ni, cev);
                if (dt == sequence::draw::finish)
                    break;

                int tick_s_x = (ni.start() * xw) / t1;
                int sx = x0 + tick_s_x;                /* start x          */
                if (dt == sequence::draw::tempo)
                {
                    midibpm max = usr().midi_bpm_maximum();
                    midibpm min = usr().midi_bpm_minimum();
                    double tempo = double(ni.velocity());
                    int y = int((max - tempo) / (max - min) * yh) + y0;
                    brush.setColor(tempo_paint());
                    painter.setBrush(brush);
                    painter.drawEllipse(sx, y, 4, 4);
                }
                else
                {
                    int y = y0 + yh * (n1 - ni.note()) / height;
                    int tick_f_x = (ni.finish() * xw) / t1;
                    if (! sequence::is_draw_note(dt) || tick_f_x <= tick_s_x)
                        tick_f_x = tick_s_x + 1;

                    int fx = x0 + tick_f_x;            /* finish x         */
                    painter.drawLine(sx, y, fx, y);
                }
            }
        }
    }
}

/**
 *  This event occurs only upon a click.
 */

void
qloopbutton::focusInEvent (QFocusEvent *)
{
    m_text_initialized = false;
}

/**
 *  This event occurs only upon a click.
 */

void
qloopbutton::focusOutEvent (QFocusEvent *)
{
    m_text_initialized = false;
}

/**
 *  This event occurs only upon a click.
 */

void
qloopbutton::resizeEvent (QResizeEvent * qrep)
{
    QSize s = qrep->size();
    vert_compressed(s.height() < 90);       // hardwired for now
    horiz_compressed(s.width() < 90);

    /*
     * Weird, makes use re-init every damn time.
     *
     * boxes_initialized(true);        // reset to false //
     */

    QWidget::resizeEvent(qrep);
}

}           // namespace seq66

/*
 * qloopbutton.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


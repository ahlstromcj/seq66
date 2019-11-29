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
 * \file          qsliveframe.cpp
 *
 *  This module declares/defines the base class for holding pattern slots.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2019-08-30
 * \license       GNU GPLv2 or above
 *
 *  This class is the Qt counterpart to the mainwid class.
 */

#include <QErrorMessage>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QTimer>

#include "cfg/settings.hpp"             /* seq66::usr().key_height(), etc.  */
#include "ctrl/keystroke.hpp"           /* seq66::keystroke class           */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "qskeymaps.hpp"                /* mapping between Gtkmm and Qt     */
#include "qsliveframe.hpp"
#include "qsmacros.hpp"                 /* QS_TEXT_CHAR() macro             */
#include "qsmainwnd.hpp"                /* the true parent of this class    */
#include "qt5_helpers.hpp"              /* seq66::qt_keystroke() etc.       */

#if defined SEQ66_PLATFORM_DEBUG
#include <qnamespace.h>
#endif

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#if defined SEQ66_QMAKE_RULES
#include "forms/ui_qsliveframe.h"
#else
#include "forms/qsliveframe.ui.h"
#endif

/**
 *  Drawing a rounded box to indicate an armed (unmuted) pattern looks a bit
 *  better than a plain rectangle.
 */

#define SEQ66_DRAW_ROUNDED_BOX

#if defined SEQ66_DRAW_ROUNDED_BOX
#define BOX_RADIUS_X    16.0
#define BOX_RADIUS_Y    16.0
#endif

/**
 *  Constants to use to fine-tune the MIDI event preview boxes.  The original
 *  values are commented out.  The new values make the event boxes smaller and
 *  nicer looking.  However, we need a less krufty way to change these.
 */

static const int sc_preview_w_factor = 3;   // 2;
static const int sc_preview_h_factor = 8;   // 5;
static const int sc_base_x_offset = 12;     // 7;
static const int sc_base_y_offset = 24;     // 15;

/*
 * Do not document a namespace, it breaks Doxygen.
 */

namespace seq66
{

static const int c_text_x = 6;
static const int c_mainwid_border = 0;

/**
 *  The Qt 5 version of mainwid.
 *
 * \param p
 *      Provides the performer object to use for interacting with this sequence.
 *
 * \param window
 *      Provides the functional parent of this live frame.
 *
 * \param parent
 *      Provides the Qt-parent window/widget for this container window.
 *      Defaults to null.  Normally, this is a pointer to the tab-widget
 *      containing this frame.  If null, there is no parent, and this frame is
 *      in an external window.
 */

qsliveframe::qsliveframe (performer & p, qsmainwnd * window, QWidget * parent) :
    qslivebase          (p, window, parent),
    ui                  (new Ui::qsliveframe),
    m_popup             (nullptr),
    m_timer             (nullptr),
    m_msg_box           (nullptr),
    m_slot_function
    (
        std::bind
        (
            &qsliveframe::draw_sequence, this,
            std::placeholders::_1, std::placeholders::_2
        )
    ),
    m_gtkstyle_border   (! usr().grid_is_normal())
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);
    ui->setupUi(this);
    m_msg_box = new QMessageBox(this);
    m_msg_box->setText(tr("Sequence already present"));
    m_msg_box->setInformativeText
    (
        tr
        (
            "There is already a sequence stored in this slot. "
            "Overwrite it and create a new blank sequence?"
        )
    );
    m_msg_box->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    m_msg_box->setDefaultButton(QMessageBox::No);

    /*
     * This row, except for ui->labelPlaylistSong, are no longer used.
     * We'll just hide them for now, though.
     */

    ui->setNameLabel->hide();
    ui->setNumberLabel->hide();
    ui->txtBankName->hide();
    ui->spinBank->hide();
    ui->labelPlaylistSong->setText("");

    m_font.setPointSize(15);
    m_font.setBold(true);
    m_font.setLetterSpacing(QFont::AbsoluteSpacing, 1);

    m_timer = new QTimer(this);         /* timer for regular redraws    */
    m_timer->setInterval(usr().window_redraw_rate());
    connect(m_timer, SIGNAL(timeout()), this, SLOT(conditional_update()));
    m_timer->start();
}

/**
 *  Virtual destructor, deletes the user-interface objects and the message
 *  box.
 *
 *  Not needed: delete m_timer;
 */

qsliveframe::~qsliveframe()
{
    delete ui;
    if (not_nullptr(m_msg_box))
        delete m_msg_box;
}

/**
 *  Sets the name of the play-list.
 */

void
qsliveframe::set_playlist_name (const std::string & plname)
{
    QString pln = " ";
    pln += QString::fromStdString(plname);
    ui->labelPlaylistSong->setText(pln);
}

/**
 *  In an effort to reduce CPU usage when simply idling, this function calls
 *  update() only if necessary.  See qseqbase::needs_update(). All
 *  sequences are potentially checked.
 */

void
qsliveframe::conditional_update ()
{
    if (perf().needs_update())
        update();
}

/**
 *  This override simply calls draw_sequences().
 */

void
qsliveframe::paintEvent (QPaintEvent *)
{
    draw_sequences();
}

/**
 *  Provides a way to calculate the base x and y size values for the
 *  pattern map for a given sequence/pattern/loop.  The values are returned as
 *  side-effects.  Compare it to mainwid::calculate_base_sizes():
 *
 *      -   m_mainwid_border_x and m_mainwid_border_y are
 *          ui->frame->x() and ui->frame->y(), which can be altered by the
 *          user via resizing the main window.
 *      -   m_seqarea_x and m_seqarea_y are the m_slot_w and m_slot_h members.
 *
 * \param sn
 *      Provides the number of the sequence to calculate.
 *
 * \param [out] basex
 *      A return parameter for the x coordinate of the base.  This is
 *      basically the x coordinate of the rectangle for a pattern slot.
 *      It is the x location of the frame, plus the slot width offset by its
 *      "x coordinate".
 *
 * \param [out] basey
 *      A return parameter for the y coordinate of the base.  This is
 *      basically the y coordinate of the rectangle for a pattern slot.
 *      It is the y location of the frame, plus the slot height offset by its
 *      "y coordinate".
 */

void
qsliveframe::calculate_base_sizes (seq::number sn, int & basex, int & basey)
{
    int i = (sn / m_mainwnd_rows) % m_mainwnd_cols;
    int j =  sn % m_mainwnd_rows;
    basex = ui->frame->x() + 1 + (m_slot_w + m_mainwid_spacing) * i;
    basey = ui->frame->y() + 1 + (m_slot_h + m_mainwid_spacing) * j;
}

/**
 *
 */

void
qsliveframe::color_by_number (int i)
{
    qslivebase::color_by_number(i);
}

/**
 *  Draws a single pattern slot.  Support for the fading of timed elements is
 *  provided via m_last_metro.
 *
 * \param s
 *      The shared pointer to the pattern/sequence to be drawn.
 *
 * \param sn
 *      The sequencer number for an empty slot to be drawn.  If s is a null
 *      pointer, then draw_slot() is used to draw an empty slot.
 *
 * \return
 *      Returns true if the drawing was performed.
 */

bool
qsliveframe::draw_sequence (seq::pointer s, seq::number sn)
{
    bool result = bool(s);
    if (result)
    {
        seq::number seq = s->seq_number();
        midipulse tick = perf().get_tick();
        int metro = (tick / perf().ppqn()) % 2;

        /*
         * Slot background and font settings.  For background, we want
         * something between black and darkGray at some point.  Here, the pen
         * is black, and the brush is black.
         */

        QPainter painter(this);
        QPen pen(Qt::black);
        QBrush brush(Qt::black);
        m_font.setPointSize(6);
        m_font.setBold(true);
        m_font.setLetterSpacing(QFont::AbsoluteSpacing, 1);
        painter.setPen(pen);
        painter.setBrush(brush);
        painter.setFont(m_font);

        /*
         * Grab frame dimensions for scaled drawing.  Note that the frame size
         * can be modified by the user dragging a corner, in some window
         * managers.
         */

        int fw = ui->frame->width();
        int fh = ui->frame->height();
        m_slot_w = (fw - m_space_cols - 1) / m_mainwnd_cols;
        m_slot_h = (fh - m_space_rows - 1) / m_mainwnd_rows;

        /*
         * IDEA: Subtract 20 from height and add 10 to base y.
         */

        int preview_w = m_slot_w - m_font.pointSize() * sc_preview_w_factor;
        int preview_h = m_slot_h - m_font.pointSize() * sc_preview_h_factor;
        int c = s->color();
        int base_x, base_y;
        calculate_base_sizes(seq, base_x, base_y);  /* side-effects         */
        if (s->get_playing())                       /* playing, no queueing */
        {
            brush.setColor(Qt::black);
            pen.setColor(Qt::white);
        }
        else
        {
            brush.setColor(Qt::white);
            pen.setColor(Qt::black);
        }
        painter.setPen(pen);
        painter.setBrush(brush);

        /*
         * Outer pattern-slot border (Seq64) or whole box (Kepler34).  Do we
         * want to gray-up the border, too, for Seq64 format?
         */

        if (m_gtkstyle_border)                  /* Gtk/seq66 methods    */
        {
            painter.setPen(pen);
            painter.setBrush(brush);
            painter.setFont(m_font);
            if (s->get_playing() && (s->get_queued() || s->off_from_snap()))
            {
                // no code
            }
            else if (s->get_playing())          /* playing, no queueing */
            {
                Color backcolor(Qt::black);
                brush.setColor(backcolor);
                pen.setColor(Qt::white);
                painter.setBrush(brush);
                painter.setPen(pen);
            }
            else if (s->get_queued())           /* not playing, queued  */
            {
                // no code
            }
            else if (s->one_shot())             /* one-shot queued      */
            {
                // no code
            }
            else                                /* just not playing     */
            {
                Color backcolor(Qt::white);
                brush.setColor(backcolor);
                pen.setColor(Qt::black);
                painter.setBrush(brush);
                painter.setPen(pen);
            }
            draw_box(painter, base_x, base_y, m_slot_w + 1, m_slot_h + 1);
        }
        else                                    /* Kepler34 methods     */
        {
            /*
             * What color?  Qt::black? Qt::darkCyan? Qt::yellow? Qt::green?
             */

            const int penwidth = 3;             /* 2                    */
            pen.setColor(Qt::black);
            pen.setStyle(Qt::SolidLine);
            if (s->get_playing() && (s->get_queued() || s->off_from_snap()))
            {
                Color backcolor = get_color_fix(PaletteColor(c));
                backcolor.setAlpha(210);
                brush.setColor(backcolor);
                pen.setWidth(penwidth);
                pen.setColor(Qt::gray);         /* instead of Qt::black */
                pen.setStyle(Qt::SolidLine);    /* not Qt::DashLine     */
                painter.setPen(pen);
                painter.setBrush(brush);
                draw_box(painter, base_x, base_y, m_slot_w + 1, m_slot_h + 1);
            }
            else if (s->get_playing())          /* playing, no queueing */
            {
                Color backcolor = get_color_fix(PaletteColor(c));
                backcolor.setAlpha(210);
                brush.setColor(backcolor);
                pen.setWidth(penwidth);
                painter.setPen(pen);
                painter.setBrush(brush);
                draw_box(painter, base_x, base_y, m_slot_w + 1, m_slot_h + 1);
            }
            else if (s->get_queued())           /* not playing, queued  */
            {
                Color backcolor = get_color_fix(PaletteColor(c));
                backcolor.setAlpha(180);
                brush.setColor(backcolor);
                pen.setWidth(penwidth);
                pen.setColor(Qt::darkGray);
                pen.setStyle(Qt::SolidLine);    /* not Qt::DashLine     */
                painter.setPen(pen);
                painter.setBrush(brush);
                draw_box(painter, base_x, base_y, m_slot_w, m_slot_h);
            }
            else if (s->one_shot())             /* one-shot queued      */
            {
                Color backcolor = get_color_fix(PaletteColor(c));
                backcolor.setAlpha(180);
                brush.setColor(backcolor);
                pen.setWidth(penwidth);
                pen.setColor(Qt::darkGray);
                pen.setStyle(Qt::DotLine);
                painter.setPen(pen);
                painter.setBrush(brush);
                draw_box(painter, base_x, base_y, m_slot_w, m_slot_h);
            }
            else                                /* just not playing     */
            {
                Color backcolor = get_color_fix(PaletteColor(c));
                backcolor.setAlpha(100);        /* .setAlpha(180)       */
                brush.setColor(backcolor);
                pen.setStyle(Qt::NoPen);
                painter.setPen(pen);
                painter.setBrush(brush);
                painter.drawRect(base_x, base_y, m_slot_w, m_slot_h);
            }
        }

        std::string st = perf().sequence_title(*s);
        QString title(st.c_str());

        /*
         * Draws the text in the border of the pattern slot.
         */

        if (m_gtkstyle_border)
        {
            if (s->get_playing() && (s->get_queued() || s->off_from_snap()))
            {
                pen.setColor(Qt::white);
            }
            else if (s->get_playing())          /* playing, no queueing     */
            {
                pen.setColor(Qt::white);
            }
            else if (s->get_queued())           /* not playing, queued      */
            {
                pen.setColor(Qt::black);
            }
            else if (s->one_shot())             /* one-shot queued          */
            {
                pen.setColor(Qt::white);
            }
            else
            {
                pen.setColor(Qt::black);
            }
        }
        else
        {
            pen.setColor(Qt::black);            /* or best contrast color?  */
        }
        pen.setWidth(1);
        pen.setStyle(Qt::SolidLine);
        painter.setPen(pen);
        painter.drawText
        (
            base_x + c_text_x, base_y + 4, 80, 80, Qt::AlignLeft, title
        );

        std::string sl = perf().sequence_label(*s);
        QString label(sl.c_str());
        int y = base_y + m_slot_h - 5;
        painter.drawText(base_x + 8, y, label);
        if (rc().show_ui_sequence_key())
        {
            QString key = perf().lookup_slot_key(seq).c_str();
            int x = base_x + m_slot_w - 15;                 /* was "- 10"   */
            painter.drawText(x, y, key);
        }

        /*
         * Draws the inner box of the pattern slot.
         */

        Color backcolor = get_color_fix(PaletteColor(c));
        Color pencolor = get_pen_color(PaletteColor(c));
        if (m_gtkstyle_border)
        {
#if defined SEQ66_PLATFORM_DEBUG_TMI
            show_color_rgb(backcolor);
#endif
            brush.setColor(backcolor);
            if (s->get_playing() && (s->get_queued() || s->off_from_snap()))
            {
                backcolor = Qt::gray;
            }
            else if (s->get_playing())          /* playing, no queueing */
            {
                if (no_color(c))
                {
                    backcolor = Qt::black;
                    pencolor = Qt::white;
                }
                else
                {
                    // pen color set below
                }
            }
            else if (s->get_queued())           /* not playing, queued  */
            {
                backcolor = Qt::gray;
            }
            else if (s->one_shot())             /* one-shot queued      */
            {
                backcolor = Qt::darkGray;
            }
            else                                /* muted pattern        */
            {
                // pen color set below
            }
        }
        else
        {
            brush.setStyle(Qt::NoBrush);
        }
        brush.setColor(backcolor);
        pen.setColor(pencolor);

        int rectangle_x = base_x + sc_base_x_offset;
        int rectangle_y = base_y + sc_base_y_offset;

        if (m_gtkstyle_border)
        {
            rectangle_y -= 6;
            preview_h += 2 * 6;
        }
        painter.setBrush(brush);
        painter.setPen(pen);                    /* inner box of notes   */
        painter.drawRect
        (
            rectangle_x-2, rectangle_y-1, preview_w, preview_h
        );

        /*
         * Draw the notes.
         */

        int lowest, highest;
        bool have_notes = s->minmax_notes(lowest, highest);
        if (have_notes)
        {
            int height = highest - lowest + 2;
            int length = s->get_length();
            Color drawcolor = pencolor;         // fg_color();
            Color eventcolor = pencolor;        // fg_color();
            if (! s->transposable())
            {
                eventcolor = red();
                drawcolor = red();
            }
            preview_h -= 6;                     /* padding for box          */
            preview_w -= 6;
            rectangle_x += 2;
            rectangle_y += 2;
            s->reset_draw_marker();             /* reset iterator           */
            for (;;)
            {
                sequence::note_info ni;         /* only two members used!   */
                sequence::draw dt = s->get_next_note(ni);
                int tick_s_x = (ni.start() * preview_w) / length;
                int tick_f_x = (ni.finish() * preview_w) / length;
                if (dt == sequence::draw::finish)
                    break;

                if (sequence::is_draw_note(dt))
                    tick_f_x = tick_s_x + 1;

                if (tick_f_x <= tick_s_x)
                    tick_f_x = tick_s_x + 1;

                int note_y;
                if (dt == sequence::draw::tempo)
                {
                    /*
                     * Do not scale by the note range here.
                     */

                    pen.setWidth(2);
                    drawcolor = tempo_paint();
                    note_y = m_slot_h -         // NOT w!!!
                         m_slot_h * (ni.note() + 1) / c_max_midi_data_value;
                }
                else
                {
                    pen.setWidth(1);                    /* 2 too thick  */
                    note_y = preview_h -
                         (preview_h * (ni.note()+1 - lowest)) / height;
                }

                int sx = rectangle_x + tick_s_x;        /* start x      */
                int fx = rectangle_x + tick_f_x;        /* finish x     */
                int sy = rectangle_y + note_y;          /* start y      */
                int fy = sy;                            /* finish y     */
                pen.setColor(drawcolor);                /* note line    */
                painter.setPen(pen);
                painter.drawLine(sx, sy, fx, fy);
                if (dt == sequence::draw::tempo)
                {
                    pen.setWidth(1);                    /* 2 too thick  */
                    drawcolor = eventcolor;
                }
            }

            int a_tick = perf().get_tick();             /* for playhead */
            a_tick += (length - s->get_trigger_offset());
            a_tick %= length;

            midipulse tick_x = a_tick * preview_w / length;
            if (s->get_playing())
                pen.setColor(Qt::red);
            else
                pen.setColor(Qt::black);

            if (s->get_playing() && (s->get_queued() || s->off_from_snap()))
                pen.setColor(Qt::green);
            else if (s->one_shot())
                pen.setColor(Qt::blue);

            pen.setWidth(1);
            painter.setPen(pen);
            painter.drawLine
            (
                rectangle_x + tick_x - 1, rectangle_y - 1,
                rectangle_x + tick_x - 1, rectangle_y + preview_h + 1
            );
        }

        /*
         * Lessen m_alpha on each redraw to have smooth fading.  Done as a factor
         * of the BPM to get useful fades.
         */

        m_alpha *= 0.7 - perf().bpm() / 300.0;
        m_last_metro = metro;
    }
    else
    {
        result = draw_slot(sn);
    }
    return result;
}

/**
 *  Draws an empty pattern slot.
 *
 *  This function currently does not draw a box around the empty slot.
 */

bool
qsliveframe::draw_slot (seq::number sn)
{
    QPainter painter(this);
    QPen pen(Qt::white);
    QBrush brush(Qt::black);

    /*
     * This removes the black border around the empty sequence
     * boxes.  We like the border.
     *
     *  pen.setStyle(Qt::NoPen);
     */

    painter.setPen(pen);
    painter.setBrush(brush);
    painter.setFont(m_font);

    int fw = ui->frame->width();
    int fh = ui->frame->height();
    int base_x, base_y;
    m_slot_w = (fw - m_space_cols - 1) / m_mainwnd_cols;
    m_slot_h = (fh - m_space_rows - 1) / m_mainwnd_rows;
    calculate_base_sizes(sn, base_x, base_y);               /* side-effects */
    painter.drawRect(base_x, base_y, m_slot_w, m_slot_h);   /* black box    */

    /*
     * No sequence present. Insert placeholder.  (Not a big fan of this
     * one, which draws a big ugly plus-sign.)
     *
     *  pen.setStyle(Qt::SolidLine);
     *  painter.setPen(pen);
     *  painter.drawText(base_x + 2, base_y + 17, "+");
     */

    if (rc().show_ui_sequence_number())
    {
        int lx = base_x + (m_slot_w / 2) - 7;
        int ly = base_y + (m_slot_h / 2) + 5;
        char snum[8];
        snprintf(snum, sizeof snum, "%d", sn);

        QString ls(snum);
        m_font.setPointSize(8);
        pen.setColor(Qt::white);
        pen.setWidth(1);
        pen.setStyle(Qt::SolidLine);
        painter.setPen(pen);
        painter.setFont(m_font);
        painter.drawText(lx, ly, ls);
    }
    return true;
}

/**
 *
 *      m_last_tick_x[s] = 0;
 */

void
qsliveframe::draw_sequences ()
{
    (void) perf().slots_function(m_slot_function);
}

/**
 *
 */

void
qsliveframe::draw_box (QPainter & painter, int x, int y, int w, int h)
{
#if defined SEQ66_DRAW_ROUNDED_BOX
    painter.drawRoundedRect(x, y, w, h, BOX_RADIUS_X, BOX_RADIUS_Y);
#else
    painter.drawRect(x, y, w, h);
#endif
}

/**
 *  A virtual function to set the bank name in the user-interface.  Called by
 *  qslivebase::set_bank().
 */

void
qsliveframe::set_bank_values (const std::string & bankname, int bankid)
{
    QString bname = bankname.c_str();
    ui->txtBankName->setPlainText(bname);       // now hidden
    ui->spinBank->setValue(bankid);             // now hidden
}

/**
 *  This slot calls the base-class function of the same name.
 */

void
qsliveframe::update_bank (int bankid)
{
    qslivebase::update_bank(bankid);
}

/**
 *  Used to grab the std::string bank name and convert it to QString for
 *  display. Let performer set the modify flag, it knows when to do it.
 *  Otherwise, just scrolling to the next screen-set causes a spurious
 *  modification and an annoying prompt to a user exiting the application.
 *
 *  DEPRECATED.
 */

void
qsliveframe::update_bank_name ()
{
    std::string name = ui->txtBankName->document()->toPlainText().toStdString();
    perf().set_screenset_notepad(m_bank_id, name, m_is_external);
}

/**
 *  Converts the (x, y) coordinates of a click into a sequence/pattern ID.
 *  Normally, these values can range from 0 to 31, representing one of 32
 *  slots in the live frame.  But sets may be larger or smaller.
 *
 *  Compare this function to setmapper::set_get().
 *
 * \param click_x
 *      The x-coordinate of the mouse click.
 *
 * \param click_y
 *      The y-coordinate of the mouse click.
 *
 * \return
 *      Returns the sequence/pattern number.  If not found, then a -1 is
 *      returned.
 */

int
qsliveframe::seq_id_from_xy (int click_x, int click_y)
{
    int x = click_x - c_mainwid_border;         /* adjust for border */
    int y = click_y - c_mainwid_border;
    int w = m_slot_w + m_mainwid_spacing;
    int h = m_slot_h + m_mainwid_spacing;

    /*
     * Is it in the box?
     */

    if (x < 0 || x >= (w * m_mainwnd_cols) || y < 0 || y >= (h * m_mainwnd_rows))
        return -1;

    /*
     * Gives us x, y in box coordinates.  Then we test for the right inactive
     * side of area.
     */

    int box_test_x = x % w;
    int box_test_y = y % h;
    if (box_test_x > m_slot_w || box_test_y > m_slot_h)
        return -1;

    x /= w;
    y /= h;
    seq::number seqid = (x * m_mainwnd_rows + y) + (m_bank_id * m_screenset_slots);
    return seqid;
}

/**
 *  Sets m_current_seq based on the position of the mouse over the live frame.
 *
 * \param event
 *      Provides the mouse event.
 */

void
qsliveframe::mousePressEvent (QMouseEvent * event)
{
    m_current_seq = seq_id_from_xy(event->x(), event->y());
    if (m_current_seq != -1 && event->button() == Qt::LeftButton)
        m_button_down = true;
}

/**
 *
 */

void
qsliveframe::mouseReleaseEvent (QMouseEvent *event)
{
    /* get the sequence number we clicked on */

    m_current_seq = seq_id_from_xy(event->x(), event->y());
    m_button_down = false;

    /*
     * if we're on a valid sequence, hit the left mouse button, and are not
     * dragging a sequence - toggle playing.
     */

    bool assigned = m_current_seq != seq::unassigned();
    if (assigned && event->button() == Qt::LeftButton && ! m_moving)
    {
        if (perf().is_seq_active(m_current_seq))
        {
            perf().sequence_playing_toggle(m_current_seq);
            m_adding_new = false;
            update();
        }
        else
            m_adding_new = true;
    }

    /*
     * If it's the left mouse button and we're moving a pattern between slots,
     * then, if the sequence number is valid, inactive, and not in editing,
     * create a new pattern and copy the data to it.  Otherwise, copy the data
     * to the old sequence.
     */

    if (event->button() == Qt::LeftButton && m_moving)
    {
        m_moving = false;
        if (perf().finish_move(m_current_seq))
            update();
    }

    /*
     * Check for right mouse click; this action launches the popup menu for
     * the pattern slot underneath the mouse.
     */

    if (assigned && event->button() == Qt::RightButton)
    {
        m_popup = new QMenu(this);

        QAction * newseq = new QAction(tr("&New pattern"), m_popup);
        m_popup->addAction(newseq);
        QObject::connect(newseq, SIGNAL(triggered(bool)), this, SLOT(new_seq()));

        /*
         *  Add an action to bring up an external qsliveframe window based
         *  on the sequence number over which the mouse is resting.  This is
         *  pretty tricky, but might be reasonable.
         */

        if (! m_is_external)
        {
            if (m_current_seq < usr().max_sets())
            {
                char temp[32];
                snprintf
                (
                    temp, sizeof temp, "Extern &live frame set %d",
                    m_current_seq
                );
                QAction * liveframe = new QAction(tr(temp), m_popup);
                m_popup->addAction(liveframe);
                QObject::connect
                (
                    liveframe, SIGNAL(triggered(bool)),
                    this, SLOT(new_live_frame())
                );
            }

            if (perf().is_seq_active(m_current_seq))
            {
                QAction * editseq = new QAction
                (
                    tr("Edit pattern in &tab"), m_popup
                );
                m_popup->addAction(editseq);
                connect(editseq, SIGNAL(triggered(bool)), this, SLOT(edit_seq()));
            }
        }
        if (perf().is_seq_active(m_current_seq))
        {
            QAction * editseqex = new QAction
            (
                tr("Edit pattern in &window"), m_popup
            );
            m_popup->addAction(editseqex);
            connect(editseqex, SIGNAL(triggered(bool)), this, SLOT(edit_seq_ex()));

            if (! m_is_external)
            {
                QAction * editevents = new QAction
                (
                    tr("Edit e&vents in tab"), m_popup
                );
                m_popup->addAction(editevents);
                connect
                (
                    editevents, SIGNAL(triggered(bool)), this, SLOT(edit_events())
                );
            }

            /*
             * \todo
             *      Use the stored palette colors!
             */

            QMenu * menuColour = new QMenu(tr("Set pattern &color..."));
            int firstcolor = color_to_int(none);
            int lastcolor = color_to_int(grey);
            for (int c = firstcolor; c <= lastcolor; ++c)
            {
                if (c != color_to_int(black))
                {
                    PaletteColor pc = PaletteColor(c);
                    QString cname = get_color_name(pc).c_str();     // for now
                    QAction * a = new QAction(cname, menuColour);
                    connect
                    (
                        a, &QAction::triggered,
                        [this, c] { color_by_number(c); }
                    );
                    menuColour->addAction(a);
                }
            }

            QMenu * submenuColour = new QMenu(tr("More colors"));
            firstcolor = color_to_int(dk_red);
            lastcolor = color_to_int(dk_grey);
            for (int c = firstcolor; c <= lastcolor; ++c)
            {
                PaletteColor pc = PaletteColor(c);
                QString cname = get_color_name(pc).c_str();     // for now
                QAction * a = new QAction(cname, submenuColour);
                connect
                (
                    a, &QAction::triggered,
                    [this, c]  { color_by_number(c); }
                );
                submenuColour->addAction(a);
            }
            menuColour->addMenu(submenuColour);
            m_popup->addMenu(menuColour);

            QAction * actionCopy = new QAction(tr("Cop&y pattern"), m_popup);
            m_popup->addAction(actionCopy);
            connect
            (
                actionCopy, SIGNAL(triggered(bool)),
                this, SLOT(copy_sequence())
            );

            QAction * actionCut = new QAction(tr("Cu&t pattern"), m_popup);
            m_popup->addAction(actionCut);
            connect
            (
                actionCut, SIGNAL(triggered(bool)),
                this, SLOT(cut_sequence())
            );

            QAction * actionDelete = new QAction(tr("&Delete pattern"), m_popup);
            m_popup->addAction(actionDelete);
            connect
            (
                actionDelete, SIGNAL(triggered(bool)),
                this, SLOT(delete_sequence())
            );
        }
        else if (m_can_paste)
        {
            QAction * actionPaste = new QAction(tr("&Paste pattern"), m_popup);
            m_popup->addAction(actionPaste);
            connect
            (
                actionPaste, SIGNAL(triggered(bool)),
                this, SLOT(paste_sequence())
            );
        }
        m_popup->exec(QCursor::pos());
    }

    if                              /* middle button launches seq editor    */
    (
        assigned && event->button() == Qt::MiddleButton &&
        perf().is_seq_active(m_current_seq)
    )
    {
        signal_call_editor(m_current_seq);
    }
}

/**
 *
 */

void
qsliveframe::mouseMoveEvent (QMouseEvent * event)
{
    seq::number seqid = seq_id_from_xy(event->x(), event->y());
    if (m_button_down)
    {
        bool not_editing = ! perf().is_seq_in_edit(m_current_seq);
        if (seqid != m_current_seq && ! m_moving && not_editing)
        {
            /*
             * Drag a sequence between slots; save the sequence and clear the
             * old slot.
             */

            if (perf().move_sequence(m_current_seq))
            {
                m_moving = true;
                update();
            }
        }
    }
}

/**
 */

void
qsliveframe::mouseDoubleClickEvent (QMouseEvent * event)
{
    if (m_adding_new)
    {
        new_seq();
    }
    else
    {
        int m_current_seq = seq_id_from_xy(event->x(), event->y());
        if (! perf().is_seq_active(m_current_seq))
        {
            if (perf().new_sequence(m_current_seq))
                perf().get_sequence(m_current_seq)->set_dirty();
        }
        signal_call_editor_ex(m_current_seq);
    }
}

/**
 *
 */

void
qsliveframe::new_seq ()
{
    if (perf().is_seq_active(m_current_seq))
    {
        int choice = m_msg_box->exec();
        if (choice == QMessageBox::No)
            return;
    }
    if (perf().new_sequence(m_current_seq))
        perf().get_sequence(m_current_seq)->set_dirty();
}

/**
 *  We need to see if there is an external live-frame window already existing
 *  for the current sequence number (which is used as a screen-set number).
 *  If not, we can create a new one and add it to the list.
 */

void
qsliveframe::new_live_frame ()
{
    signal_live_frame(m_current_seq);
}

/**
 *  Emits the signal_call_editor() signal.  In qsmainwnd, this signal is connected to
 *  the loadEditor() slot.
 */

void
qsliveframe::edit_seq ()
{
    signal_call_editor(m_current_seq);
}

/**
 *  Emits the signal_call_editor_ex() signal.  In qsmainwnd, this signal is connected to
 *  the loadEditorEx() slot.
 */

void
qsliveframe::edit_seq_ex ()
{
    signal_call_editor_ex(m_current_seq);
}

/**
 *
 */

void
qsliveframe::edit_events ()
{
    signal_call_edit_events(m_current_seq);
}

/**
 *
 */

bool
qsliveframe::handle_key_press (const keystroke & k)
{
    bool done = perf().midi_control_keystroke(k);
    if (done)
    {
    }
    if (! done)
    {
        if (perf().seq_edit_pending())              /* self-resetting   */
        {
            signal_call_editor_ex(perf().pending_loop());
            done = true;
        }
        else if (perf().event_edit_pending())       /* self-resetting   */
        {
            signal_call_edit_events(perf().pending_loop());
            done = true;
        }
        else
            done = m_parent->handle_key_press(k);
    }
    return done;
}

/**
 *
 */

bool
qsliveframe::handle_key_release (const keystroke & k)
{
    bool done = perf().midi_control_keystroke(k);
    if (! done)
    {
        // so far, nothing needed upon key release, but...
    }
    return done;
}

/**
 *  The Gtkmm 2.4 version calls performer::mainwnd_key_event().  We have broken
 *  that function into pieces (smaller functions) that we can use here.  An
 *  important point is that keys that affect the GUI directly need to be
 *  handled here in the GUI.  Another important point is that other events are
 *  offloaded to the performer object, and we need to let that object handle as
 *  much as possible.  The logic here is an admixture of events that we will
 *  have to sort out.
 *
 *  Note that the QKeyEWvent::key() function does not distinguish between
 *  capital and non-capital letters, so we use the text() function (returning
 *  the Unicode text the key generated) for this purpose and provide a the
 *  QS_TEXT_CHAR() macro to make it obvious.
 *
 *  Weird.  After the first keystroke, for, say 'o' (ascii 111) == k, we
 *  get k == 0, presumably a terminator character that we have to ignore.
 *  Also, we can't intercept the Esc key.  Qt grabbing it?
 *
 * \param event
 *      Provides a pointer to the key event.
 */

void
qsliveframe::keyPressEvent (QKeyEvent * event)
{
    keystroke k = qt_keystroke(event, SEQ66_KEYSTROKE_PRESS);
    bool done = handle_key_press(k);
    if (done)
    {
        update();
    }
    else
    {
        done = m_parent->handle_key_press(k);
        QWidget::keyPressEvent(event);              /* event->ignore()      */
    }
}

/**
 *
 */

void
qsliveframe::keyReleaseEvent (QKeyEvent * event)
{
    keystroke k = qt_keystroke(event, SEQ66_KEYSTROKE_RELEASE);
    bool done = handle_key_release(k);
    if (done)
        update();
    else
        QWidget::keyReleaseEvent(event);            /* event->ignore()      */
}

void
qsliveframe::reupdate ()
{
    update();
}

void
qsliveframe::update_geometry ()
{
    updateGeometry();
}

void
qsliveframe::change_event (QEvent * evp)
{
    changeEvent(evp);
}

/**
 *
 */

void
qsliveframe::copy_sequence ()
{
    if (qslivebase::copy_seq())
    {
        // Anything to do?
    }
}

/**
 * \todo
 *      Dialog warning that the editor is the reason this seq cant be cut.
 */

void
qsliveframe::cut_sequence ()
{
    if (qslivebase::cut_seq())
    {
        // TODO
    }
}

/**
 *  If the sequence/pattern is delete-able (valid and not being edited), then
 *  it is deleted via the performer object.
 */

void
qsliveframe::delete_sequence ()
{
    if (qslivebase::delete_seq())
    {
    }
}

/**
 *
 */

void
qsliveframe::paste_sequence ()
{
    if (qslivebase::paste_seq())
    {
    }
}

/**
 *  This is not called when focus changes.  Instead, we have to call this from
 *  qliveframeex::changeEvent().
 */

void
qsliveframe::changeEvent (QEvent * event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange)
    {
        if (isActiveWindow())
        {
            m_has_focus = true;             // widget is now active
            (void) perf().set_playing_screenset(m_bank_id);
        }
        else
            m_has_focus = false;            // widget is now inactive
    }
}

}           // namespace seq66

/*
 * qsliveframe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


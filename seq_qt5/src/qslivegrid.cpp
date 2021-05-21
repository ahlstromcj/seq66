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
 * \file          qslivegrid.cpp
 *
 *  This module declares/defines the base class for holding pattern slots.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-06-21
 * \updates       2021-05-21
 * \license       GNU GPLv2 or above
 *
 *  This class is the Qt counterpart to the mainwid class.  This version is
 *  replaces qsliveframe, and uses buttons and draws faster.  However, we had
 *  to disable button callbacks and use keypresses directly in order to
 *  implement double-click and drag-and-drop functionality.
 *
 *  A two-dimensional vector of buttons containing a vector of rows, each
 *  row being a vector of columns.
 *
 *  Screen view and loop-grid slot numbers:
 *
\verbatim
        Column      0   1   2   3   4   5   6   7
        Row 0       0   4   8  12  16  20  24  28
        Row 1       1   5   9  13  17  21  25  29
        Row 2       2   6  10  14  18  22  26  30
        Row 3       3   7  11  15  19  23  27  31
\endverbatim
 *
 *  Button/Gridrow (m_loop_buttons[column][row]) view slot numbers:
 *
\verbatim
        Row         0   1   2   3   [one gridrow, a vector of slotbuttons]
        Column
           0        0   1   2   3
           1        4   5   6   7
           2        8   9  10  11
           3       12  13  14  15
           4       16  17  18  19
           5       20  21  22  23
           6       24  25  26  27
           7       28  29  30  31
\endverbatim
 *
 *  The fastest varying index is the row: m_loop_buttons[column][row].
 */

#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPaintEvent>
#include <QTimer>

#include "cfg/settings.hpp"             /* seq66::usr() config functions    */
#include "ctrl/keystroke.hpp"           /* seq66::keystroke class           */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "os/timing.hpp"                /* seq66::microsleep()              */
#include "gui_palette_qt5.hpp"
#include "qloopbutton.hpp"              /* seq66::qloopbutton (qslotbutton) */
#include "qslivegrid.hpp"               /* seq66::qslivegrid                */
#include "qsmacros.hpp"                 /* QS_TEXT_CHAR() macro             */
#include "qsmainwnd.hpp"                /* the true parent of this class    */
#include "qt5_helpers.hpp"              /* seq66::qt_keystroke() etc.       */

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#if defined SEQ66_QMAKE_RULES
#include "forms/ui_qslivegrid.h"
#else
#include "forms/qslivegrid.ui.h"
#endif

#if defined SEQ66_PLATFORM_DEBUG_KEY_TESTING
#define SEQ66_KEY_TESTING
#endif

/*
 * Do not document a namespace, it breaks Doxygen.
 */

namespace seq66
{

/*
 *  Provides padding between buttons.  Turns out not to be needed.
 */

static const int sc_button_padding = 0;

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

qslivegrid::qslivegrid
(
    performer & p,
    qsmainwnd * window,
    QWidget * parent
) :
    qslivebase          (p, window, parent),
    ui                  (new Ui::qslivegrid),
    m_popup             (nullptr),
    m_timer             (nullptr),
    m_msg_box           (nullptr),
    m_redraw_buttons    (true),
    m_loop_buttons      (),
    m_x_min             (0),
    m_x_max             (0),
    m_y_min             (0),
    m_y_max             (0),
    m_base_width        (0)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);
    ui->setupUi(this);
    m_msg_box = new QMessageBox(this);
    m_msg_box->setText(tr("Sequence already present."));
    m_msg_box->setInformativeText
    (
        tr
        (
            "There is already a pattern in this slot. "
            "Overwrite it with a new blank pattern?"
        )
    );
    m_msg_box->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    m_msg_box->setDefaultButton(QMessageBox::No);

    ui->setNameLabel->hide();
    ui->setNumberLabel->hide();

#if ! defined USE_TEXTBANKNAME_AND_SPINBANK
    ui->txtBankName->hide();
    ui->spinBank->hide();
#endif

    ui->labelPlaylistSong->setText("");
    set_mode_text();
    qloopbutton::progress_box_size
    (
        usr().progress_box_width(),
        usr().progress_box_height()
    );

    /*
     * When done here, the buttons don't resize to the actual frame size.
     * See paintEvent() instead.
     *
     * ui->loopGridLayout->activate();
     * setLayout(ui->loopGridLayout);
     * create_loop_buttons();
     */

    m_timer = new QTimer(this);         /* timer for regular redraws    */
    m_timer->setInterval(2 * usr().window_redraw_rate());
    connect(m_timer, SIGNAL(timeout()), this, SLOT(conditional_update()));
    m_timer->start();
}

/**
 *  Virtual destructor, deletes the user-interface objects and the message
 *  box.
 *
 *  Not needed: delete m_timer;  Why not?
 */

qslivegrid::~qslivegrid()
{
    m_timer->stop();
    clear_loop_buttons();               /* currently we use raw pointers    */
    if (not_nullptr(m_msg_box))
        delete m_msg_box;

    delete ui;
}

void
qslivegrid::set_mode_text (const std::string & mode)
{
    QString mtext = QString::fromStdString(mode);
    ui->labelMode->setText(mtext);
}

/**
 *  Sets the name of the play-list.
 */

void
qslivegrid::set_playlist_name (const std::string & plname)
{
    QString pln = " ";
    pln += QString::fromStdString(plname);
    ui->labelPlaylistSong->setText(pln);
    (void) recreate_all_slots();
}

/**
 *  In an effort to reduce CPU usage when simply idling, this function calls
 *  update() only if necessary.  See qlivebase::check_dirty(). All
 *  sequences are potentially checked.
 *
 *  Actually, we need a way to update only the loop slots in the grid layout,
 *  and only update the progress area.
 */

void
qslivegrid::conditional_update ()
{
    bool ok = ! m_loop_buttons.empty();
    if (! ok)
        return;

    sequence_key_check();
    if (perf().needs_update() || check_needs_update())
    {
        for (int column = 0; column < columns(); ++column)
        {
            for (int row = 0; row < rows(); ++row)
            {
                qslotbutton * pb = button(row, column);
                if (not_nullptr(pb))
                {
                    seq::pointer s = pb->loop();
                    if (s)
                    {
                        bool armed = s->playing();
                        pb->set_checked(armed);
                        pb->reupdate(true);
                    }
                    else
                        pb->reupdate(false);
                }
            }
        }
    }
}

/**
 *  Grab frame dimensions for scaled drawing.  Note that the frame size can be
 *  modified by the user dragging a corner, in some window managers.
 *
 *  The slot-sizes are provisional sizes, will be adjusted according to the size
 *  of the layout grid.
 *
 *  Note that this function is called only in paintEvent(), and only when
 *  m_redraw_buttons is true.
 *
 *  Note that m_space_rows and m_space_cols are defined in qslivebase, and are
 *  based ultimately on the usr().mainwid_spacing() value times the number of
 *  rows or columns in the grid.
 */

void
qslivegrid::create_loop_buttons ()
{
    int seqno = int(perf().playscreen_offset());
    int fw = ui->frame->width();
    int fh = ui->frame->height();
    m_slot_w = (fw - m_space_cols - 1) / columns() - sc_button_padding;
    m_slot_h = (fh - m_space_rows - 1) / rows() - sc_button_padding;

#if defined SEQ66_PLATFORM_DEBUG_TMI
    printf
    (
        "frame = %d x %d, slot = %d x %d, spacing = %d\n",
        fw, fh, m_slot_w, m_slot_h, spacing()
    );
#endif

    for (int row = 0; row < rows(); ++row)
        ui->loopGridLayout->setRowMinimumHeight(row, m_slot_h + spacing());

    for (int column = 0; column < columns(); ++column)
    {
        gridrow buttonrow;                      /* vector for push-buttons  */
        for (int row = 0; row < rows(); ++row, ++seqno)
        {
            qslotbutton * pb = create_one_button(seqno);
            buttonrow.push_back(pb);            /* always do this           */
        }
        m_loop_buttons.push_back(buttonrow);    /* always do this           */
    }
    measure_loop_buttons();                     /* always do this           */
}

/**
 *  This function just deletes all the pointers in our "2-D" array and then
 *  clears the array completely, so that it has a size of 0 x 0.  This function
 *  is meant to be used when completely recreating the button-layout grid.
 */

void
qslivegrid::clear_loop_buttons ()
{
    if (! m_loop_buttons.empty())
    {
        for (int column = 0; column < columns(); ++column)
        {
            for (int row = 0; row < rows(); ++row)
            {
                qslotbutton * pb = m_loop_buttons[column][row];
                if (not_nullptr(pb))
                    delete pb;
            }
        }
        m_loop_buttons.clear();
    }
}

/**
 *  Very brute force, but done rarely!
 */

void
qslivegrid::measure_loop_buttons ()
{
    m_slot_w = m_slot_h = m_x_max = m_y_max = 0;
    m_x_min = m_y_min = 99999;
    for (int column = 0; column < columns(); ++column)
    {
        for (int row = 0; row < rows(); ++row)
        {
            qslotbutton * pb = button(row, column);
            if (not_nullptr(pb))
            {
                QRect r = pb->geometry();
                if (m_slot_w == 0)
                {
                    m_slot_w = r.width();       /* all buttons same size    */
                    m_slot_h = r.height();
                }
                int x0 = r.x();
                int x1 = x0 + m_slot_w;         /* r.width()  */
                int y0 = r.y();
                int y1 = y0 + m_slot_h;         /* r.height() */
                if (x0 < m_x_min)
                    m_x_min = x0;

                if (x1 > m_x_max)
                    m_x_max = x1;

                if (y0 < m_y_min)
                    m_y_min = y0;

                if (y1 > m_y_max)
                    m_y_max = y1;
            }
            else
            {
                warnprint("qslivegrid::measure_loop_button(): null button");
            }
        }
    }
    if (m_base_width == 0)
        m_base_width = m_slot_w;
}

/**
 *  Given the (x, y) coordinate of a mouse click, calculates the row and
 *  column of the button over which the click occurred.
 *
 * \warning
 *      Note that this function currently does not check for being completely
 *      within the button, so that clicks in the space between the buttons cause
 *      a true result.
 *
 * \return
 *      Returns true if the calculation could be made.  Otherwise, the results
 *      are meaningless.
 */

bool
qslivegrid::get_slot_coordinate (int x, int y, int & row, int & column)
{
    bool result = m_x_max > 0;
    if (result)
    {
        int xslotsize = (m_x_max - m_x_min) / columns();
        int yslotsize = (m_y_max - m_y_min) / rows();
        row = (y - m_y_min) / yslotsize;
        column = (x - m_x_min) / xslotsize;
    }
    else
        row = column = 0;

    return result;
}

/**
 *  Creates a single button, either empty (qslotbutton) or representing a
 *  sequence (qloopbutton).
 *
 *  Uses pointers.  Beware of reconnecting to changed or new pointers!
 */

qslotbutton *
qslivegrid::create_one_button (int seqno)
{
    qslotbutton * result = nullptr;
    int row, column;
    bool valid = perf().seq_to_grid(seqno, row, column);
    if (valid)
    {
        bool enabled = perf().is_screenset_active(seqno);
        const QSize btnsize = QSize(m_slot_w, m_slot_h);
        std::string snstring;
        ui->loopGridLayout->setColumnMinimumWidth(column, m_slot_w + spacing());
        if (valid)
            snstring = std::to_string(seqno);

        std::string hotkey = perf().lookup_slot_key(seqno);
        seq::pointer pattern = perf().loop(seqno);          /* can be null  */
        if (pattern)
            result = new qloopbutton(this, seqno, snstring, hotkey, pattern);
        else
            result = new qslotbutton(this, seqno, snstring, hotkey);

        ui->loopGridLayout->addWidget(result, row, column);
        result->setFixedSize(btnsize);
        result->show();
        result->setEnabled(enabled);
        setup_button(result);
    }
    else
    {
        errprintf("create_one_button(seq %d): invalid\n", seqno);
    }
    return result;
}

/**
 *  This override simply calls creates the slot/loop buttons.  We have to do
 *  it here, because only here do we know the final size of the grid.  This
 *  function is called about a half-dozen times at startup, which really
 *  messages with "redraw" flags in the buttons.  We need call the internal
 *  code here just once at startup.
 */

void
qslivegrid::paintEvent (QPaintEvent * /*qpep*/)
{
    if (m_redraw_buttons)
    {
        create_loop_buttons();                  /* refresh_all_slots()  */
        m_redraw_buttons = false;
    }

#if defined SEQ66_PLATFORM_DEBUG_TMI
    static bool s_shown = false;
    if (! s_shown)
    {
        show_loop_buttons();
        s_shown = true;
    }
#endif
}

/**
 *  This function removes the slot-buttons from the layout and then deletes
 *  them.  Then it recreates them at the new size.  Works pretty fast on an i7!
 */

void
qslivegrid::resizeEvent (QResizeEvent * /*qrep*/)
{
    (void) recreate_all_slots();        /* sets m_redraw_buttons to true    */
}

/**
 *  A helper function for create_one_button().
 */

void
qslivegrid::setup_button (qslotbutton * pb)
{
    if (not_nullptr(pb))
    {
        pb->setup();
        pb->reupdate();
    }
}

/**
 *  Tell performer to set the current pattern's color and then call
 *  recreate_all_slots().  Simply calling a refresh or reupdate doesn't work
 *  to show the new color.  Luckily, this goes fast.
 */

void
qslivegrid::color_by_number (int i)
{
    qslivebase::color_by_number(i);     /* perf().color(m_current_seq, ...) */
    (void) recreate_all_slots();
}

/**
 *  A virtual function to set the bank name in the user-interface.  Called by
 *  qslivebase::set_bank().
 */

void
qslivegrid::set_bank_values (const std::string & /*bankname*/, int /*bankid*/)
{
#if defined USE_TEXTBANKNAME_AND_SPINBANK
    QString bname = bankname.c_str();
    ui->txtBankName->setPlainText(bname);       // now hidden
    ui->spinBank->setValue(bankid);             // now hidden
#endif
}

/**
 *  A brute-force function to get the desired slot-button pointer.
 */

qslotbutton *
qslivegrid::button (int row, int column)
{
    qslotbutton * result = nullptr;
    if (! m_loop_buttons.empty() && ! m_loop_buttons[column].empty())
        result = m_loop_buttons[column][row];

    return result;
}

/**
 *  Does a brute force lookup calculation for a button based on its sequence
 *  number.
 *
 * \param seqno
 *      The number of the sequence to be looked up. This determines the row and
 *      column by a simple calculation.
 *
 * \return
 *      Returns a pointer to a qslotbutton or qloopbutton.  Returns a null
 *      pointer if a failure occurs.
 */

qslotbutton *
qslivegrid::find_button (int seqno)
{
    qslotbutton * result = nullptr;
    int row, column;
    bool ok = perf().seq_to_grid(seq::number(seqno), row, column);
    if (ok)
        result = button(row, column);

    return result;
}

/**
 *  This function finds the desired layout-item in the grid, and then removes
 *  it.  The caller is responsible for deleting the slot-button.
 */

bool
qslivegrid::delete_slot (int row, int column)
{
    qslotbutton * pb = button(row, column);
    bool result = not_nullptr(pb);
    if (result)
    {
        QLayoutItem * item = ui->loopGridLayout->itemAtPosition(row, column);
        if (not_nullptr(item))
            ui->loopGridLayout->removeWidget(item->widget());
    }
    return result;
}

/**
 *  Generally, after calling this function, clear_loop_buttons() needs to be
 *  called to delete the slot-button pointers.
 */

bool
qslivegrid::delete_all_slots ()
{
    bool result = ! m_loop_buttons.empty();
    if (result)
    {
        bool failed = false;
        for (int column = 0; column < columns(); ++column)
        {
            for (int row = 0; row < rows(); ++row)
            {
                result = delete_slot(row, column);
                if (! result)
                    failed = true;
            }
        }
        if (failed)
            result = false;
    }
    return result;
}

/**
 *  Deletes all the slot-buttons, and recreates them from scratch.  This is
 *  done by setting m_redraw_buttons and letting create_loop_buttons() be
 *  called in paintEvent().
 */

bool
qslivegrid::recreate_all_slots ()
{
    bool result = delete_all_slots();
    if (result)
    {
        clear_loop_buttons();
        m_redraw_buttons = true;
        set_needs_update();
    }
    return result;
}

void
qslivegrid::refresh (seq::number seqno)
{
    int row, column;
    if (perf().seq_to_grid(seqno, row, column))             /* side-effects */
    {
        seq::pointer s = perf().get_sequence(seqno);
        if (not_nullptr(s))
        {
            bool armed = s->playing();
            qslotbutton * pb = button(row, column);
            if (not_nullptr(pb))
            {
                pb->set_checked(armed);
                pb->reupdate(true);
#if defined SEQ66_PLATFORM_DEBUG_TMI
                printf
                (
                    "button(%d, %d) = %s\n", row, column,
                    armed ? "on" : "off"
                );
#endif
            }
        }
    }
}

/**
 *  This function helps the caller (usually qsmainwnd) tell all the buttons to
 *  show their current state.  One big use case is when the command is given
 *  to mute, unmute, or toggle all of the patterns.
 *
 *  We still need to be able to refresh individual buttons when MIDI control
 *  or Song playback changes the state of a pattern.
 */

bool
qslivegrid::refresh_all_slots ()
{
    bool result = ! m_loop_buttons.empty();
    if (result)
    {
        seq::number offset = perf().playscreen_offset();
        for (int column = 0; column < columns(); ++column)
        {
            for (int row = 0; row < rows(); ++row)
            {
                seq::pointer s = perf().get_sequence(offset);
                if (not_nullptr(s))
                {
                    bool armed = s->playing();
                    qslotbutton * pb = button(row, column);
                    pb->set_checked(armed);
                    pb->reupdate(true);
#if defined SEQ66_PLATFORM_DEBUG_TMI
                    printf
                    (
                        "button(%d, %d) = %s\n", row, column,
                        armed ? "on" : "off"
                    );
#endif
                }
                ++offset;
            }
        }
    }
    return result;
}

bool
qslivegrid::modify_slot (qslotbutton * newslot, int row, int column)
{
    bool result = delete_slot(row, column);
    if (result)
    {
        ui->loopGridLayout->addWidget(newslot, row, column);
        m_loop_buttons[column][row] = newslot;
    }
    return result;
}

void
qslivegrid::update_bank (int bankid)
{
    qslivebase::update_bank(bankid);
    (void) recreate_all_slots();        /* sets m_redraw_buttons to true    */
}

void
qslivegrid::update_bank ()
{
    (void) recreate_all_slots();        /* sets m_redraw_buttons to true    */
}

/**
 *  Used to grab the std::string bank name and convert it to QString for
 *  display. Let performer set the modify flag, it knows when to do it.
 *  Otherwise, just scrolling to the next screen-set causes a spurious
 *  modification and an annoying prompt to a user exiting the application.
 */

void
qslivegrid::update_bank_name (const std::string & name)
{
#if defined USE_TEXTBANKNAME_AND_SPINBANK
    ui->txtBankName->setPlainText(name.c_str());
#endif
    perf().set_screenset_notepad(m_bank_id, name, m_is_external);
}

/**
 *  This function causes all slots to ultimately be deleted and flagged for
 * reconstruction.  That's a lot of work, and only occurs if a sequence slot
 * is created, deleted, or pasted.
 *
 * \param seqno
 *      Provides the number of the pattern being created, deleted, or pasted.
 */

void
qslivegrid::update_sequence (seq::number seqno, bool redo)
{
    if (redo)
    {
        alter_sequence(seqno);
    }
    else
    {
        int row, column;
        if (perf().seq_to_grid(seqno, row, column))
        {
            qslotbutton * pb = button(row, column);
            if (not_nullptr(pb))
                pb->reupdate(true);
        }
    }
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
 *      Returns the sequence/pattern number.  If not found, then a -1 (the
 *      value seq::unassigned) is returned.
 */

int
qslivegrid::seq_id_from_xy (int click_x, int click_y)
{
    int result = seq::unassigned();
    int row, column;
    if (get_slot_coordinate(click_x, click_y, row, column))
         result = int(perf().grid_to_seq(row, column));

    return result;
}

/**
 *  Sets m_current_seq based on the position of the mouse over the live frame.
 *  For the left button:
 *
 *  -   No modifier.  The button is looked up by the position of the mouse.
 *      If it has a sequence pointer, then the button's toggle_checked()
 *      function is called, which does what it says, plus toggles the
 *      sequence.  The sequence tells the performer to notify all subscribers
 *      of the on_trigger_change() function.
 *  -   Control key.  Create a new sequence.
 *  -   Shift key.  Create a new live frame with a screen-set offset by the
 *      button number times the size of a screenset.  A bit tricky.
 *
 * \param event
 *      Provides the mouse event.
 */

void
qslivegrid::mousePressEvent (QMouseEvent * event)
{
    m_current_seq = seq_id_from_xy(event->x(), event->y());

#if defined SEQ66_PLATFORM_DEBUG_TMI
    printf
    (
        "mousePressEvent(%d) at (%d, %d)\n",
        m_current_seq, event->x(), event->y()
    );
#endif

    bool assigned = m_current_seq != seq::unassigned();
    if (! assigned)
    {
        return;                 /* printf("mouse press unassigned\n");  */
    }
    if (event->button() == Qt::LeftButton)
    {
        if (event->modifiers() & Qt::ControlModifier)
            new_sequence();
        else if (event->modifiers() & Qt::ShiftModifier)
            new_live_frame();
        else if (assigned)
        {
            /*
             * This actually gets a qloopbutton.  Don't be fooled like I was.
             * The toggle call does its own button update.
             */

            qslotbutton * pb = find_button(m_current_seq);
            if (not_nullptr(pb))
            {
                seq::pointer s = pb->loop();
                if (s)
                {
                    m_button_down = true;
                    (void) pb->toggle_checked();
                }
            }
        }
    }
    if (event->button() == Qt::RightButton)
    {
        if (event->modifiers() & Qt::ControlModifier)
            new_sequence();
        else if (event->modifiers() & Qt::ShiftModifier)
            new_live_frame();
        else
            popup_menu();
    }
}

void
qslivegrid::slot_press (int seqno)
{
    if (seqno != seq::unassigned())
    {
        m_current_seq = seqno;
        m_button_down = true;
    }
}

/**
 *  Get the sequence number we clicked on.  If we're on a valid sequence, hit
 *  the left mouse button, and are not dragging a sequence, then toggle
 *  playing.
 */

void
qslivegrid::mouseReleaseEvent (QMouseEvent * event)
{
    m_current_seq = seq_id_from_xy(event->x(), event->y());
    m_button_down = false;

#if defined SEQ66_PLATFORM_DEBUG_TMI
    printf
    (
        "mouseReleaseEvent(%d) at (%d, %d)\n",
        m_current_seq, event->x(), event->y()
    );
#endif

    bool assigned = m_current_seq != seq::unassigned();
    if (! assigned)
    {
        return;                 /* printf("mouse release unassigned\n"); */
    }
    if (event->button() == Qt::LeftButton)
    {
        if (m_moving)
        {
            /*
             * If it's the left mouse button and we're moving a pattern between
             * slots, then, if the sequence number is valid, inactive, and not
             * in editing, create a new pattern and copy the data to it.
             * Otherwise, copy the data to the old sequence.
             */

            m_moving = false;
            if (perf().finish_move(m_current_seq))
                (void) recreate_all_slots();
        }
        else
        {
            if (perf().is_seq_active(m_current_seq))
                m_adding_new = false;
            else
                m_adding_new = true;
        }
    }

    /*
     * Check for right mouse click; this action launches the popup menu for
     * the pattern slot underneath the mouse.
     *
     * Can we do this just once and save the menus for reuse?
     */

    if (event->button() == Qt::RightButton)
    {
        /* moved to the press event handler */
    }
    else if                             /* middle button launches seq editor */
    (
        event->button() == Qt::MiddleButton &&
        perf().is_seq_active(m_current_seq)
    )
    {
        signal_call_editor(m_current_seq);
    }
}

/**
 *  Drag a sequence between slots; save the sequence into a "moving sequence"
 *  in the performer, and clear the old slot. But what if the user lets go of
 *  the pattern in the same slot?  To DO!  Also, this call does a
 *  partial-assign and removal all the time.
 */

void
qslivegrid::mouseMoveEvent (QMouseEvent * event)
{
    seq::number seqno = seq_id_from_xy(event->x(), event->y());
    if (m_button_down)
    {
        bool not_editing = ! perf().is_seq_in_edit(m_current_seq);
        if (seqno != m_current_seq && ! m_moving && not_editing)
        {
            if (perf().move_sequence(m_current_seq))
            {
                m_moving = true;
                update();
            }
        }
    }
}

void
qslivegrid::mouseDoubleClickEvent (QMouseEvent * event)
{
    bool showframe = true;
    if (m_adding_new)
    {
        new_sequence();
        // showframe = false;
    }
    if (showframe)
    {
        m_current_seq = seq_id_from_xy(event->x(), event->y());
        if (! perf().is_seq_active(m_current_seq))
        {
            if (perf().new_sequence(m_current_seq))
                perf().get_sequence(m_current_seq)->set_dirty();
        }
        signal_call_editor_ex(m_current_seq);
    }
}

void
qslivegrid::new_sequence ()
{
    bool createseq = true;
    if (perf().is_seq_active(m_current_seq))
    {
        int choice = m_msg_box->exec();
        if (choice == QMessageBox::Yes)
            createseq = perf().remove_sequence(m_current_seq);
        else
            createseq = false;
    }
    if (createseq)
    {
        if (perf().new_sequence(m_current_seq))
        {
            perf().get_sequence(m_current_seq)->set_dirty();
            alter_sequence(m_current_seq);
        }
    }
}

/**
 *  We need to see if there is an external live-frame window already existing
 *  for the current sequence number (which is used as a screen-set number).
 *  If not, we can create a new one and add it to the list.
 */

void
qslivegrid::new_live_frame ()
{
    signal_live_frame(m_current_seq);
}

/**
 *  Emits the signal_call_editor() signal.  In qsmainwnd, this signal is
 *  connected to the load_editor() slot.
 */

void
qslivegrid::edit_sequence ()
{
    signal_call_editor(m_current_seq);
}

/**
 *  Emits the signal_call_editor_ex() signal.  In qsmainwnd, this signal is
 *  connected to the loadEditorEx() slot.
 */

void
qslivegrid::edit_sequence_ex ()
{
    signal_call_editor_ex(m_current_seq);
}

void
qslivegrid::edit_events ()
{
    signal_call_edit_events(m_current_seq);
}

/**
 *  Handles any existing pattern-key statuses.  Used in the timer callback.
 */

void
qslivegrid::sequence_key_check ()
{
    seq::number seqno;
    bool ok = perf().got_seqno(seqno);
    if (perf().seq_edit_pending())
    {
        if (ok)
        {
            m_current_seq = seqno;
            edit_sequence_ex();
            perf().clear_seq_edits();
        }
    }
    else if (perf().event_edit_pending())
    {
        if (ok)
        {
            m_current_seq = seqno;
            edit_events();
            perf().clear_seq_edits();
        }
    }
    else if (ok)
    {
        /*
         * Currently ends up handled in performer's loop-control function.
         *
         * else
         *     perf().sequence_key(seqno);                 // toggle loop  //
         */
    }
}

/**
 *  Functionality moved to qsmainwnd::handle_key_press().
 *  The handling of seq-edit and event-edit is done via setting flags in
 *  performer and responding to them in the timer function.
 */

bool
qslivegrid::handle_key_press (const keystroke & k)
{
    return k.is_good() ? m_parent->handle_key_press(k) : false ;
}

bool
qslivegrid::handle_key_release (const keystroke & k)
{
    return k.is_good() ? m_parent->handle_key_press(k) : false ;
}

/**
 *  The Gtkmm 2.4 version calls performer::mainwnd_key_event().  We have
 *  broken that function into pieces (smaller functions) that we can use here.
 *  An important point is that keys that affect the GUI directly need to be
 *  handled here in the GUI.  Another important point is that other events are
 *  offloaded to the performer object, and we need to let that object handle
 *  as much as possible.  The logic here is an admixture of events that we
 *  will have to sort out.
 *
 *  Note that the QKeyEvent::key() function does not distinguish between
 *  capital and non-capital letters, so we use the text() function (returning
 *  the Unicode text the key generated) for this purpose and provide a the
 *  QS_TEXT_CHAR() macro to make it obvious.
 *
 *  Weird.  After the first keystroke, for, say 'o' (ascii 111) == k, we get k
 *  == 0, presumably a terminator character that we have to ignore.  Also, we
 *  can't intercept the Esc key.  Qt grabbing it?
 *
 * \param event Provides a pointer to the key event.
 */

void
qslivegrid::keyPressEvent (QKeyEvent * event)
{
#if defined SEQ66_KEY_TESTING
    (void) qt_keystroke(event, keystroke::action::press, true);
#else
    bool show = rc().verbose();
    keystroke k = qt_keystroke(event, keystroke::action::press, show);
    bool done = handle_key_press(k);
    if (done)
        update();
    else
        QWidget::keyPressEvent(event);              /* event->ignore()?     */
#endif
}

void
qslivegrid::keyReleaseEvent (QKeyEvent * event)
{
#if defined SEQ66_KEY_TESTING
    (void) qt_keystroke(event, keystroke::action::release, true);
#else
    bool show = rc().verbose();
    keystroke k = qt_keystroke(event, keystroke::action::release, show);
    bool done = handle_key_release(k);
    if (done)
        update();
    else
        QWidget::keyReleaseEvent(event);            /* event->ignore()?     */
#endif
}

void
qslivegrid::reupdate ()
{
    for (int column = 0; column < columns(); ++column)
    {
        for (int row = 0; row < rows(); ++row)
        {
            qslotbutton * pb = button(row, column);
            if (not_nullptr(pb))
            {
                pb->setup();
                pb->reupdate();
            }
            else
            {
                /*
                 * error_message("qslivegrid::reupdate() finds null button");
                 */

                break;
            }
        }
    }
}

void
qslivegrid::update_geometry ()
{
    updateGeometry();
}

void
qslivegrid::change_event (QEvent * evp)
{
    changeEvent(evp);
}

/**
 *  This function is a helper for the other xxx_sequence() functions.  It
 *  obtains the sequence number, figures out the row and column, and creates a
 *  new button based on whether the slot contains a pattern or not.
 */

void
qslivegrid::alter_sequence (seq::number seqno)
{
    int row, column;
    if (perf().seq_to_grid(seqno, row, column))
    {
        qslotbutton * pb = create_one_button(seqno);
        if (not_nullptr(pb))
        {
            if (modify_slot(pb, row, column))
                (void) recreate_all_slots();
        }
    }
}

void
qslivegrid::copy_sequence ()
{
    if (qslivebase::copy_seq())
    {
        // no code
    }
}

/**
 *  The "can-paste" flag is set by the base class.
 */

void
qslivegrid::cut_sequence ()
{
    if (qslivebase::cut_seq())
        alter_sequence(m_current_seq);
}

/**
 *  If the sequence/pattern is delete-able (valid and not being edited), then
 *  it is deleted via the performer object.  The "can-paste" flag is not set by
 *  the base class.
 */

void
qslivegrid::delete_sequence ()
{
    if (qslivebase::delete_seq())
        alter_sequence(m_current_seq);
}

void
qslivegrid::paste_sequence ()
{
    if (qslivebase::paste_seq())
        alter_sequence(m_current_seq);
}

void
qslivegrid::merge_sequence ()
{
    if (qslivebase::merge_seq())
        alter_sequence(m_current_seq);
}

/**
 *  This is not called when focus changes.  Instead, we have to call this from
 *  qsliveframeex::changeEvent().
 */

void
qslivegrid::changeEvent (QEvent * event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange)
    {
        if (isActiveWindow())
        {
            m_has_focus = true;                 /* widget is now active     */
            (void) perf().set_playing_screenset(m_bank_id);
        }
        else
            m_has_focus = false;                /* widget is now inactive   */
    }
}

void
qslivegrid::popup_menu ()
{
    m_popup = new QMenu(this);

    QAction * ns = new QAction(tr("&New pattern"), m_popup);
    QObject::connect(ns, SIGNAL(triggered(bool)), this, SLOT(new_sequence()));
    m_popup->addAction(ns);
    m_popup->addSeparator();

    /*
     *  Add an action to bring up an external qslivegrid window based
     *  on the sequence number over which the mouse is resting.  This is
     *  pretty tricky, but might be reasonable.
     */

    if (! m_is_external)
    {
        if (m_current_seq < perf().screenset_max())
        {
            char temp[48];
            snprintf
            (
                temp, sizeof temp, "External &live frame for set %d",
                m_current_seq
            );
            QAction * livegrid = new QAction(tr(temp), m_popup);
            m_popup->addAction(livegrid);
            m_popup->addSeparator();
            QObject::connect
            (
                livegrid, SIGNAL(triggered(bool)),
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
            connect
            (
                editseq, SIGNAL(triggered(bool)),
                this, SLOT(edit_sequence())
            );
        }
    }
    if (perf().is_seq_active(m_current_seq))
    {
        QAction * editseqex = new QAction
        (
            tr("Edit pattern in &window"), m_popup
        );
        m_popup->addAction(editseqex);
        connect
        (
            editseqex, SIGNAL(triggered(bool)),
            this, SLOT(edit_sequence_ex())
        );

        if (! m_is_external)
        {
            QAction * editevents = new QAction
            (
                tr("Edit e&vents in tab"), m_popup
            );
            m_popup->addAction(editevents);
            connect
            (
                editevents, SIGNAL(triggered(bool)),
                this, SLOT(edit_events())
            );
        }

        /**
         *  Color menus
         */

        QMenu * menuColour = new QMenu(tr("Pattern &color..."));
        int firstcolor = palette_to_int(none);
        int lastcolor = palette_to_int(white);
        for (int c = firstcolor; c <= lastcolor; ++c)
        {
            PaletteColor pc = PaletteColor(c);
            QString cname = QString::fromStdString
            (
                c == firstcolor ? get_color_name(pc) : get_color_name_ex(pc)
            );
            QAction * a = new QAction(cname, menuColour);
            connect
            (
                a, &QAction::triggered,
                [this, c] { color_by_number(c); }
            );
            menuColour->addAction(a);
        }

        QMenu * menu2Colour = new QMenu(tr("Dark colors"));
        firstcolor = palette_to_int(dk_black);
        lastcolor = palette_to_int(dk_white);
        for (int c = firstcolor; c <= lastcolor; ++c)
        {
            PaletteColor pc = PaletteColor(c);
            QString cname = get_color_name_ex(pc).c_str();
            QAction * a = new QAction(cname, menu2Colour);
            connect
            (
                a, &QAction::triggered,
                [this, c]  { color_by_number(c); }
            );
            menu2Colour->addAction(a);
        }

        QMenu * menu3Colour = new QMenu(tr("Other colors"));
        firstcolor = palette_to_int(orange);                /* color_16 */
        lastcolor = palette_to_int(grey);                   /* color_23 */
        for (int c = firstcolor; c <= lastcolor; ++c)
        {
            PaletteColor pc = PaletteColor(c);
            QString cname = get_color_name_ex(pc).c_str();
            QAction * a = new QAction(cname, menu3Colour);
            connect
            (
                a, &QAction::triggered,
                [this, c]  { color_by_number(c); }
            );
            menu3Colour->addAction(a);
        }

        QMenu * menu4Colour = new QMenu(tr("More colors"));
        firstcolor = palette_to_int(dk_orange);             /* color_24 */
        lastcolor = palette_to_int(dk_grey);                /* color_31 */
        for (int c = firstcolor; c <= lastcolor; ++c)
        {
            PaletteColor pc = PaletteColor(c);
            QString cname = get_color_name_ex(pc).c_str();
            QAction * a = new QAction(cname, menu4Colour);
            connect
            (
                a, &QAction::triggered,
                [this, c]  { color_by_number(c); }
            );
            menu4Colour->addAction(a);
        }
        menuColour->addMenu(menu2Colour);
        menuColour->addMenu(menu3Colour);
        menuColour->addMenu(menu4Colour);
        m_popup->addMenu(menuColour);

        /**
         *  Copy/Cut/Delete/Paste menus
         */

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

        QAction * actionMerge = new QAction(tr("&Merge into pattern"), m_popup);
        m_popup->addAction(actionMerge);
        connect
        (
            actionMerge, SIGNAL(triggered(bool)),
            this, SLOT(merge_sequence())
        );
    }
    else if (m_can_paste)
    {
        QAction * actionPaste = new QAction(tr("&Paste to pattern"), m_popup);
        m_popup->addAction(actionPaste);
        connect
        (
            actionPaste, SIGNAL(triggered(bool)),
            this, SLOT(paste_sequence())
        );

        QAction * actionMerge = new QAction(tr("&Merge into pattern"), m_popup);
        m_popup->addAction(actionMerge);
        connect
        (
            actionMerge, SIGNAL(triggered(bool)),
            this, SLOT(merge_sequence())
        );
    }
    if (perf().is_seq_active(m_current_seq))
    {
        /**
         *  Buss menu
         */

        mastermidibus * mmb = perf().master_bus();
        seq::pointer s = perf().get_sequence(m_current_seq);
        if (not_nullptr(mmb))
        {
            QMenu * menubuss = new QMenu(tr("Output Bus"));
            const clockslist & opm = output_port_map();
            int buses = opm.active() ?
                opm.count() : mmb->get_num_out_buses() ;

            for (int bus = 0; bus < buses; ++bus)
            {
                e_clock ec;
                std::string busname;
                if (perf().ui_get_clock(bussbyte(bus), ec, busname))
                {
                    bool disabled = ec == e_clock::disabled;
                    QString bname = QString::fromStdString(busname);
                    QAction * a = new QAction(bname, menubuss);
                    connect
                    (
                        a, &QAction::triggered,
                        [this, bus] { set_midi_bus(bus); }
                    );
                    menubuss->addAction(a);
                    if (disabled)
                        a->setEnabled(false);
                }
            }
            m_popup->addSeparator();
            m_popup->addMenu(menubuss);

            /**
             *  Channel menu
             */

            QMenu * menuchan = new QMenu(tr("Output Channel"));
            int buss = s->true_bus();
            for (int channel = 0; channel <= c_midichannel_max; ++channel)
            {
                char b[4];                              /* 2 digits or less */
                snprintf(b, sizeof b, "%2d", channel + 1);
                std::string name = std::string(b);
                std::string s = usr().instrument_name(buss, channel);
                if (! s.empty())
                {
                    name += " [";
                    name += s;
                    name += "]";
                }
                if (channel == c_midichannel_max)
                {
                    QString cname("Free");
                    QAction * a = new QAction(cname, menuchan);
                    connect
                    (
                        a, &QAction::triggered,
                        [this, buss, channel] { set_midi_channel(channel); }
                    );
                    menuchan->addAction(a);
                }
                else
                {
                    QString cname(QString::fromStdString(name));
                    QAction * a = new QAction(cname, menuchan);
                    connect
                    (
                        a, &QAction::triggered,
                        [this, buss, channel] { set_midi_channel(channel); }
                    );
                    menuchan->addAction(a);
                }
            }
            m_popup->addMenu(menuchan);
        }
    }

    m_popup->exec(QCursor::pos());

    /*
     * This really sucks.  Without this sleep, the popup menu, after any
     * selection, crashes much of the time.  It crashes deep in Qt territory,
     * with no clue as to why.  And it doesn't seem to matter what themes
     * (Qt/Gtk) are in use.
     */

    millisleep(10);
}

#if defined SEQ66_PLATFORM_DEBUG

/**
 *  By default, the rectangle for each slot is (0, 0, 91, 94).  The rectangle
 *  for the grid is (0, 0, 816, 405).
 */

void
qslivegrid::show_loop_buttons ()
{
    for (int column = 0; column < columns(); ++column)
    {
        for (int row = 0; row < rows(); ++row)
        {
            qslotbutton * pb = button(row, column);
            if (not_nullptr(pb))
            {
                QRect r = pb->geometry();
                printf
                (
                    "slot %d: (%d, %d, %d, %d)\n",
                    int (pb->slot_number()), r.x(), r.y(), r.width(), r.height()
                );
            }
        }
    }
}

#endif      // defined SEQ66_PLATFORM_DEBUG

}           // namespace seq66

/*
 * qslivegrid.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


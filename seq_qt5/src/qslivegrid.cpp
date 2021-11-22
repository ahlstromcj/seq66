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
 * \file          qslivegrid.cpp
 *
 *  This module declares/defines the base class for holding pattern slots.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-06-21
 * \updates       2021-11-22
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
#include "os/timing.hpp"                /* seq66::millisleep()              */
#include "util/filefunctions.hpp"       /* seq66::get_full_path()           */
#include "gui_palette_qt5.hpp"          /* seq66::gui_palette_qt5 class     */
#include "qloopbutton.hpp"              /* seq66::qloopbutton (qslotbutton) */
#include "qslivegrid.hpp"               /* seq66::qslivegrid                */
#include "qsmainwnd.hpp"                /* the true parent of this class    */
#include "qt5_helpers.hpp"              /* seq66::qt_keystroke() etc.       */

#if defined SEQ66_PLATFORM_DEBUG
#include "util/strfunctions.hpp"        /* seq66::pointer_to_string()       */
#endif

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#if defined SEQ66_QMAKE_RULES
#include "forms/ui_qslivegrid.h"
#else
#include "forms/qslivegrid.ui.h"
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
static const int c_minimum_width   = 300;
static const int c_minimum_height  = 180;

/**
 *  The Qt 5 version of mainwid.
 *
 * \param p
 *      Provides the performer object to use for interacting with this sequence.
 *
 * \param window
 *      Provides the functional parent of this live frame.
 *
 * \param bank
 *      Indicates the screenset number to use for the live grid.  If unassigned,
 *      then the performer's active screenset number is used.
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
    screenset::number setno,
    QWidget * parent
) :
    qslivebase          (p, window, setno, parent),
    ui                  (new Ui::qslivegrid),
    m_popup             (nullptr),
    m_timer             (nullptr),
    m_msg_box           (nullptr),
    m_redraw_buttons    (true),
    m_loop_buttons      (),
    m_x_min             (0),
    m_x_max             (0),
    m_y_min             (0),
    m_y_max             (0)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);
    ui->setupUi(this);
    m_msg_box = new QMessageBox(this);
    m_msg_box->setText(tr("A pattern is present."));
    m_msg_box->setInformativeText(tr("Overwrite it with a blank pattern?"));
    m_msg_box->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    m_msg_box->setDefaultButton(QMessageBox::No);

    int w = usr().scale_size(c_minimum_width);
    int h = usr().scale_size_y(c_minimum_height);
    ui->frame->setMinimumSize(QSize(w, h));
    if (is_external())
    {
        QString bname = qt(perf().set_name(bank_id()));
        ui->txtBankName->setText(bname);
        connect
        (
            ui->txtBankName, SIGNAL(editingFinished()),
            this, SLOT(slot_set_bank_name())
        );
        ui->buttonActivate->setEnabled(true);
        connect
        (
            ui->buttonActivate, SIGNAL(clicked(bool)),
            this, SLOT(slot_activate_bank(bool))
        );
        ui->buttonLoopMode->hide();
        ui->buttonRecordMode->hide();
    }
    else
    {
        ui->setNameLabel->hide();
        ui->txtBankName->hide();
        ui->buttonActivate->hide();
        ui->buttonLoopMode->setEnabled(true);
        show_loop_control_mode();
        show_record_mode();
        connect
        (
            ui->buttonLoopMode, SIGNAL(clicked(bool)),
            this, SLOT(slot_loop_control_mode(bool))
        );
        connect
        (
            ui->buttonRecordMode, SIGNAL(clicked(bool)),
            this, SLOT(slot_record_mode(bool))
        );
    }
    ui->labelPlaylistSong->setText("");

    /*
     * TODO: Set text according to play mode:  queue, keep queue, one-shot
     * etc.
     */

    set_mode_text();
    qloopbutton::progress_box_size
    (
        usr().progress_box_width(),
        usr().progress_box_height()
    );
    m_timer = qt_timer(this, "qslivegrid", 2, SLOT(conditional_update()));
}

/**
 *  Virtual destructor, deletes the user-interface objects and the message
 *  box.
 */

qslivegrid::~qslivegrid()
{
    m_timer->stop();
    clear_loop_buttons();               /* currently we use raw pointers    */
    delete ui;
}

void
qslivegrid::set_mode_text (const std::string & mode)
{
    QString mtext = qt(mode);
    ui->labelMode->setText(mtext);
}

/**
 *  Sets the name of the play-list.
 */

void
qslivegrid::set_playlist_name (const std::string & plname, bool modified)
{
    std::string fullname = get_full_path(plname);

    /*
     * TO DO:  Fix loading a recent file from a non-session directory into the
     * NSM midi directory.
     */

    if (fullname.empty())
        fullname = filename_base(plname);

    if (fullname.empty())
        fullname = rc().no_name();

    if (modified)
        fullname += " *";

    QString pln = qt(fullname);
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
    if (m_loop_buttons.empty())
        return;

    sequence_key_check();
    if (perf().needs_update() || check_needs_update())
    {
        show_loop_control_mode();
        show_record_mode();
        for (auto pb : m_loop_buttons)
        {
            if (not_nullptr(pb))
            {
                seq::pointer s = pb->loop();
                if (s)
                {
                    pb->set_checked(s->playing());
                    pb->reupdate(true);
                }
                else
                    pb->reupdate(false);
            }
        }
    }
}

/**
 *  Grab frame dimensions for scaled drawing.  Note that the frame size can be
 *  modified by the user dragging a corner, in some window managers.
 *
 *  The slot-sizes are provisional sizes, will be adjusted according to the
 *  size of the layout grid.
 *
 *  Note that this function is called only in paintEvent(), and only when
 *  m_redraw_buttons is true.
 *
 *  Note that m_space_rows and m_space_cols are defined in qslivebase, and are
 *  based ultimately on the usr().mainwnd_spacing() value times the number of
 *  rows or columns in the grid.
 */

void
qslivegrid::create_loop_buttons ()
{
    int fw = ui->frame->width();
    int fh = ui->frame->height();
    m_slot_w = (fw - m_space_cols - 1) / columns() - sc_button_padding;
    m_slot_h = (fh - m_space_rows - 1) / rows() - sc_button_padding;
    for (int row = 0; row < rows(); ++row)
        ui->loopGridLayout->setRowMinimumHeight(row, m_slot_h + spacing());

    for (int column = 0; column < columns(); ++column)
        ui->loopGridLayout->setColumnMinimumWidth(column, m_slot_w + spacing());

    int setsize = perf().screenset_size();
    int offset = seq_offset();
    for (int seqno = 0; seqno < setsize; ++seqno)
    {
        /*
         * This does old behavior (external live grid shows the desired set,
         * and so does the main window.  Using seqno + offset yields buttons
         * squashed to the right, and a blank main live grid!
         *
         * // int s = is_external() ? (seqno + offset) : seqno ;
         */

        int s = seqno + offset;                 /* provides old behavior    */
        qslotbutton * pb = create_one_button(s);
        if (not_nullptr(pb))
        {
            m_loop_buttons.push_back(pb);
        }
        else
        {
            (void) error_message("create button failed", std::to_string(s));
            break;
        }
    }
    measure_loop_buttons();                     /* always do this           */
}

/**
 *  This function just deletes all the pointers in our "2-D" array and then
 *  clears the array completely, so that it has a size of 0 x 0.  This
 *  function is meant to be used when completely recreating the button-layout
 *  grid. Note that we leave the original sequence/pattern along.
 */

void
qslivegrid::clear_loop_buttons ()
{
    if (! m_loop_buttons.empty())
    {
        int setsize = perf().screenset_size();
        for (int seqno = 0; seqno < setsize; ++seqno)
        {
            qslotbutton * pb = m_loop_buttons[seqno];
            if (not_nullptr(pb))
                delete pb;
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
    m_x_max = m_y_max = 0;
    m_x_min = m_y_min = 99999;
    int setsize = perf().screenset_size();
    for (int seqno = 0; seqno < setsize; ++seqno)
    {
        qslotbutton * pb = loop_button(seqno);
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
            break;
    }
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
qslivegrid::create_one_button (seq::number seqno)
{
    qslotbutton * result = nullptr;
    int row, column;
//  bool valid = seq_to_grid(seqno, row, column, is_external());
    bool valid = perf().seq_to_grid(seqno, row, column, is_external());
    if (valid)
    {
        bool enabled = perf().is_screenset_active(seqno);
        const QSize btnsize = QSize(m_slot_w, m_slot_h);
        std::string snstring;
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
qslivegrid::set_bank_values (const std::string & name, int id)
{
    qslivebase::set_bank_values(name, id);
    ui->txtBankName->setText(qt(name));
//  ui->spinBank->setValue(id);
}

/**
 *  A brute-force function to get the desired slot-button pointer.
 *  Currently similar to the grid_to_seq() functions.
 */

qslotbutton *
qslivegrid::button (int row, int column)
{
    qslotbutton * result = nullptr;
    if (! m_loop_buttons.empty())
    {
        int index = perf().grid_to_index(row, column);
        result = m_loop_buttons[index];
    }
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
 * \param offset
 *      The playscreen offset, if needed.  The default is 0.
 *
 * \return
 *      Returns a pointer to a qslotbutton or qloopbutton.  Returns a null
 *      pointer if a failure occurs.
 */

qslotbutton *
qslivegrid::loop_button (seq::number seqno, seq::number offset)
{
    seq::number base = seqno - offset;
    return m_loop_buttons[base];
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

bool
qslivegrid::delete_slot (seq::number seqno)
{
    int row, column;
    bool result = perf().index_to_grid(seqno, row, column);
    if (result)
        result = delete_slot(row, column);

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
        int setsize = perf().screenset_size();
        for (int seqno = 0; seqno < setsize; ++seqno)
        {
            result = delete_slot(seqno);
            if (! result)
                failed = true;
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
    qloopbutton::boxes_initialized(true);       /* actually sets it false i */
    qloopbutton::progress_box_size              /* in case user changed it  */
    (
        usr().progress_box_width(),
        usr().progress_box_height()
    );
    if (result)
    {
        clear_loop_buttons();
        m_redraw_buttons = true;
        set_needs_update();
    }
    return result;
}

#if defined USE_REFRESH_SEQNO

void
qslivegrid::refresh (seq::number seqno)
{
    int row, column;
    if (perf().seq_to_grid(seqno, row, column, is_external())) /* side-effects */
    {
        seq::pointer s = perf().get_sequence(seqno);
        if (not_nullptr(s))
        {
            qslotbutton * pb = button(row, column);
            if (not_nullptr(pb))
            {
                pb->set_checked(s->playing());
                pb->reupdate(true);
            }
        }
    }
}

#endif

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
        for (auto pb : m_loop_buttons)
        {
            seq::pointer s = perf().get_sequence(offset);
            if (not_nullptr(s))
            {
                pb->set_checked(s->playing());
                pb->reupdate(true);
            }
            ++offset;
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

        int index = perf().grid_to_index(row, column);
        m_loop_buttons[index] = newslot;
    }
    return result;
}

/**
 *  We have found we need to pause the timer when switching banks while
 *  playing, otherwise there is a high probability of a seqfault, due to
 *  updating the user-interface (which deletes and rebuilts the slot buttons).
 */

void
qslivegrid::update_bank (int bankid)
{
    m_timer->stop();
    qslivebase::update_bank(bankid);
    (void) recreate_all_slots();        /* sets m_redraw_buttons to true    */
    m_timer->start();
}

void
qslivegrid::update_bank ()
{
    m_timer->stop();
    (void) recreate_all_slots();        /* sets m_redraw_buttons to true    */
    m_timer->start();

    // TODO
    // Need to update the bank-name field with the new bank.
    // if (is_external())
    //     ui->txtBankName->setText(qt(name));
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
    if (is_external())
        ui->txtBankName->setText(qt(name));

    perf().screenset_name(bank_id(), name, is_external());
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
        alter_sequence(seqno);          /* similar to below, but recreates  */
    }
    else
    {
        int row, column;
        if (perf().seq_to_grid(seqno, row, column, is_external()))
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
         result = int(perf().grid_to_seq(bank_id(), row, column));

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
 *  One issue is that a double-click yields a mouse-press and an
 *  mouse-double-click event, in that order.  We log the current state of
 *  the pattern so that we can restore it in the double-click handler.
 *
 * \param event
 *      Provides the mouse event.
 */

void
qslivegrid::mousePressEvent (QMouseEvent * event)
{
    m_current_seq = seq_id_from_xy(event->x(), event->y());
    if (m_current_seq != seq::unassigned())
    {
        if (event->button() == Qt::LeftButton)
        {
            if (event->modifiers() & Qt::ControlModifier)
            {
                new_sequence();
            }
            else if (event->modifiers() & Qt::ShiftModifier)
            {
                new_live_frame();
            }
            else if (event->modifiers() & Qt::AltModifier)
            {
                (void) perf().replace_for_solo(m_current_seq);
            }
            else
            {
                button_toggle_checked(m_current_seq);
                m_button_down = true;
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
}

/**
 *  Get the sequence number we clicked on.  If we're on a valid sequence, hit
 *  the left mouse button, and are not dragging a sequence, then toggle
 *  playing.
 *
 * Moving:
 *
 *      If it's the left mouse button and we're moving a pattern between slots,
 *      then, if the sequence number is valid, inactive, and not in editing,
 *      create a new pattern and copy the data to it.  Otherwise, copy the data
 *      to the old sequence.
 *
 * Right Click:
 *
*       Check for right mouse click; this action launches the popup menu for the
*       pattern slot underneath the mouse.  Can we do this just once and save the
*       menus for reuse?  This is now done in the mouse-press handler.
 */

void
qslivegrid::mouseReleaseEvent (QMouseEvent * event)
{
    m_current_seq = seq_id_from_xy(event->x(), event->y());
    m_button_down = false;
    if (m_current_seq != seq::unassigned())
    {
        if (event->button() == Qt::LeftButton)
        {
            if (m_moving)                           /* see "Moving" above   */
            {
                m_moving = false;
                button_toggle_enabled(m_source_seq);
                m_source_seq = seq::unassigned();
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
        else if                                     /* launch seq editor    */
        (
            event->button() == Qt::MiddleButton &&
            perf().is_seq_active(m_current_seq)
        )
        {
            signal_call_editor(m_current_seq);
        }
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
        if (! perf().is_seq_in_edit(m_current_seq))
        {
            if (seqno == m_current_seq)
            {
                if (seq::unassigned(m_source_seq))
                {
                    m_source_seq = m_current_seq;
                    button_toggle_enabled(m_source_seq);
                }
            }
            else
            {
                if (! m_moving)
                {
                    if (perf().move_sequence(m_current_seq))
                    {
                        m_moving = true;
                        update();
                    }
                }
            }
        }
    }
}

/**
 *  One issue is that a double-click yields a mouse-press and an
 *  mouse-double-click event, in that order.
 */

void
qslivegrid::mouseDoubleClickEvent (QMouseEvent * event)
{
    if (rc().allow_click_edit())
    {
        if (m_adding_new)
            new_sequence();

        m_current_seq = seq_id_from_xy(event->x(), event->y());
        if (perf().is_seq_active(m_current_seq))
        {
            button_toggle_checked(m_current_seq);   /* undo press-toggle    */
        }
        else
        {
#if defined USE_OLD_CODE
            (void) perf().request_sequence(m_current_seq);
#else
#endif
        }
        signal_call_editor_ex(m_current_seq);
    }
}

void
qslivegrid::button_toggle_enabled (seq::number seqno)
{
    bool assigned = seqno != seq::unassigned();
    if (assigned)
    {
        qslotbutton * pb = loop_button(seqno);
        if (not_nullptr(pb))
        {
            seq::pointer s = pb->loop();
            if (s)
                (void) pb->toggle_enabled();
        }
    }
}

/**
 *  Moved control to performer, and now rely upon the full cycle to work,
 *  rather than toggling the button state(s) here.
 */

void
qslivegrid::button_toggle_checked (seq::number seqno)
{
    bool assigned = seqno != seq::unassigned();
    if (assigned)
    {
        (void) perf().loop_control
        (
            automation::action::toggle, (-1), (-1), int(seqno), false
        );
    }
}

void
qslivegrid::new_sequence ()
{
    bool createseq = true;
    seq::number current = current_seq();
    if (perf().is_seq_active(current))
    {
        int choice = m_msg_box->exec();
        if (choice == QMessageBox::Yes)
            createseq = perf().remove_sequence(current);
        else
            createseq = false;
    }
    if (createseq)
    {
        if (perf().request_sequence(current))
        {
            msgprintf(msglevel::status, "New Pattern %d", int(current));
            alter_sequence(current);
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
            seq::pointer s = perf().get_sequence(seqno);
            m_current_seq = seqno;
            if (is_nullptr(s))
                new_sequence();

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
 *  the Unicode text the key generated) for this purpose.
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
    bool show = rc().verbose();
    keystroke k = qt_keystroke(event, keystroke::action::press, show);
    bool done = handle_key_press(k);
    if (done)
        update();
    else
        QWidget::keyPressEvent(event);              /* event->ignore()?     */
}

void
qslivegrid::keyReleaseEvent (QKeyEvent * event)
{
    bool show = rc().verbose();
    keystroke k = qt_keystroke(event, keystroke::action::release, show);
    bool done = handle_key_release(k);
    if (done)
        update();
    else
        QWidget::keyReleaseEvent(event);            /* event->ignore()?     */
}

void
qslivegrid::reupdate ()
{
    for (auto pb : m_loop_buttons)
    {
        if (not_nullptr(pb))
        {
            pb->setup();
            pb->reupdate();
        }
        else
            break;
    }
}

void
qslivegrid::update_geometry ()
{
    updateGeometry();
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
    if (perf().seq_to_grid(seqno, row, column, is_external()))
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

void
qslivegrid::slot_set_bank_name ()
{
    QString newname = ui->txtBankName->text();
    std::string name = newname.toStdString();
    update_bank_name(name);
}

void
qslivegrid::slot_activate_bank (bool /*clicked*/)
{
    (void) perf().set_playing_screenset(bank_id());
}

void
qslivegrid::slot_loop_control_mode (bool /*clicked*/)
{
    perf().next_loop_control_mode();

    /*
     * The "needs-update" flag should get this refreshed by the timer
     * callback.
     *
     * show_loop_control_mode();
     */
}

void
qslivegrid::slot_record_mode (bool /*clicked*/)
{
    perf().next_record_mode();

    /*
     * The "needs-update" flag should get this refreshed by the timer
     * callback.
     *
     * show_record_mode();
     */
}

void
qslivegrid::show_loop_control_mode ()
{
    static bool s_uninitialized = true;
    static QPalette s_palette;
    QPushButton * button = ui->buttonLoopMode;
    if (s_uninitialized)
    {
        s_uninitialized = false;
        s_palette = button->palette();
    }
    if (usr().normal_loop_control())
    {
        button->setPalette(s_palette);
        button->update();
    }
    else
    {
        QPalette pal = button->palette();
        QColor c;
        switch (usr().loop_control_mode())
        {
        case recordstyle::merge:        c.setNamedColor("#FF0000");     break;
        case recordstyle::overwrite:    c.setNamedColor("#C00000");     break;
        case recordstyle::expand:       c.setNamedColor("#900000");     break;
        case recordstyle::oneshot:      c.setNamedColor("#600000");     break;
        default:
            break;
        }
        pal.setColor(QPalette::Button, c);
        button->setAutoFillBackground(true);
        button->setPalette(pal);
        button->update();
    }
    button->setText(qt(usr().loop_control_mode_label()));
}

void
qslivegrid::show_record_mode ()
{
    static bool s_uninitialized = true;
    static QPalette s_palette;
    QPushButton * button = ui->buttonRecordMode;
    if (s_uninitialized)
    {
        s_uninitialized = false;
        s_palette = button->palette();
    }
    if (perf().record_mode() == performer::recordmode::normal)
    {
        button->setPalette(s_palette);
        button->update();
    }
    else
    {
        QPalette pal = button->palette();
        QColor c;
        switch (perf().record_mode())
        {
        case performer::recordmode::quantize:
            c.setNamedColor("#00C0C0");
            break;

        case performer::recordmode::tighten:
            c.setNamedColor("#009090");
            break;

        default:
            break;
        }
        pal.setColor(QPalette::Button, c);
        button->setAutoFillBackground(true);
        button->setPalette(pal);
        button->update();
    }
    button->setText(qt(perf().record_mode_label()));
}

void
qslivegrid::change_event (QEvent * evp)
{
    changeEvent(evp);
}

/**
 *  This is not called when focus changes.  Instead, we have to call this from
 *  qsliveframeex::changeEvent().  See change_event() above.  Thus, this
 *  function is called only for the external live frame.
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

            /*
             *  We don't want the external frame to change the active
             *  screen-set.
             *
             *      if (show_perf_bank())
             *          (void) perf().set_playing_screenset(bank_id());
             */
        }
        else
        {
            m_has_focus = false;                /* widget is now inactive   */
        }
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
     *  pretty tricky, but might be reasonable. Note that we want to allow
     *  opening an external live frame only from the Live grid in the main
     *  window, and only if the 0th set is shown... we allow only 32 sets for
     *  this purpose.
     */

    if (! is_external())
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
        if (! is_external())
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
            QString cname = qt
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
            QString cname = qt(get_color_name_ex(pc));
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
            QString cname = qt(get_color_name_ex(pc));
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
            QString cname = qt(get_color_name_ex(pc));
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
    else if (perf().can_paste())
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
                    QString bname = qt(busname);
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
                    QString cname(qt(name));
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

}           // namespace seq66

/*
 * qslivegrid.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


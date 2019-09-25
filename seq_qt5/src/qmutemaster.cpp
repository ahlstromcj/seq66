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
 * \file          qmutemaster.cpp
 *
 *  This module declares/defines the base class for the main window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-05-29
 * \updates       2019-09-24
 * \license       GNU GPLv2 or above
 *
 *  The main window is known as the "Patterns window" or "Patterns
 *  panel".  It holds the "Pattern Editor" or "Sequence Editor".  The main
 *  window consists of two object:  mainwnd, which provides the user-interface
 *  elements that surround the patterns, and mainwid, which implements the
 *  behavior of the pattern slots.
 */

#include <QPushButton>
#include <QTableWidgetItem>
#include <QTimer>

#include "seq66-config.h"               /* defines SEQ66_QMAKE_RULES        */
#include "cfg/settings.hpp"             /* seq66::rc()                      */
#include "ctrl/keystroke.hpp"           /* seq66::keystroke class           */
#include "play/mutegroups.hpp"          /* seq66::mutegroup, mutegroups     */
#include "qmutemaster.hpp"              /* seq66::qmutemaster, this class   */
#include "qsmainwnd.hpp"                /* seq66::qsmainwnd main window     */
#include "qt5_helpers.hpp"              /* seq66::qt_keystroke() etc.       */

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#if defined SEQ66_QMAKE_RULES
#include "forms/ui_qmutemaster.h"
#else
#include "forms/qmutemaster.ui.h"
#endif

/**
 *  Specifies the current hardwired value for set_row_heights().
 */

#define SEQ66_TABLE_ROW_HEIGHT          18
#define SEQ66_TABLE_FIX                 16      // 2

/*
 * Don't document the namespace.
 */

namespace seq66
{

/**
 *
 * \param p
 *      Provides the performer object to use for interacting with this frame.
 *
 * \param mainparent
 *      Provides the parent window, which will call this frame up and also
 *      needs to be notified if it goes away.
 *
 * \param parent
 *      Provides the parent window/widget for this container window.  Defaults
 *      to null.
 *
 */

qmutemaster::qmutemaster
(
    performer & p,
    qsmainwnd * mainparent,
    QWidget * parent
) :
    QFrame                  (parent),
    performer::callbacks    (p),
    ui                      (new Ui::qmutemaster),
    m_operations            ("Set Master Operations"),
    m_timer                 (nullptr),
    m_main_window           (mainparent),
    m_group_buttons         (),                             /* 2-D arrary   */
    m_current_group         (seq::unassigned()),
    m_group_count           (perf().mutegroup_count()),
    m_modify_active         (false),
    m_needs_update          (true)
{
    ui->setupUi(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    ui->m_button_modify->setEnabled(true);
    connect(ui->m_button_modify, SIGNAL(clicked()), this, SLOT(slot_modify()));

    ui->m_button_reset->setEnabled(false);
    connect(ui->m_button_reset, SIGNAL(clicked()), this, SLOT(slot_reset()));

    ui->m_button_down->setEnabled(true);
    connect(ui->m_button_down, SIGNAL(clicked()), this, SLOT(slot_down()));

    ui->m_button_up->setEnabled(true);
    connect(ui->m_button_up, SIGNAL(clicked()), this, SLOT(slot_up()));

    /*
     * This "master" is always embedded in a tab.
     *
     *  if (m_is_permanent)
     *  else
     *      connect(ui->m_button_close, SIGNAL(clicked()), this, SLOT(close()));
     */

    ui->m_button_close->hide();         /* should eliminate eventually      */
    create_group_buttons();
    connect(ui->m_button_clear_all, SIGNAL(clicked()), this, SLOT(clear_mutes()));

    ui->m_mute_basename->setPlainText(rc().mute_group_filename().c_str());
    ui->m_mute_basename->setEnabled(false);

    perf().enregister(this);            /* register this for notifications  */
    setup_table();                      /* row and column sizing            */
    (void) initialize_table();          /* fill with sets                   */
    handle_group(0, 0);                 /* guaranteed to be present         */
    m_timer = new QTimer(this);         /* timer for regular redraws        */
    m_timer->setInterval(100);          /* doesn't need to be super fast    */
    connect(m_timer, SIGNAL(timeout()), this, SLOT(conditional_update()));
    m_timer->start();
}

/**
 *
 * \todo
 *      We can fold the unregister() call into performer::callbacks at sme
 *      point.  However, we would have to deal with the issues of multiple
 *      inheritance and the exact value of the "this" pointer.
 */

qmutemaster::~qmutemaster()
{
    perf().unregister(this);            /* unregister this immediately      */
    delete ui;
}

/**
 *
 */

void
qmutemaster::conditional_update ()
{
    if (needs_update())                 /*  perf().needs_update() too iffy  */
    {
        mutegroup::number group = mutegroup::number(current_group());
        midibooleans mutes = perf().get_mutes(group);
        if (int(mutes.size()) == perf().group_size())
        {
            // TODO:  sort of access to mute-group parameters.

            for (int row = 0; row < perf().mute_rows(); ++row)
            {
                for (int column = 0; column < perf().mute_columns(); ++column)
                {
                    /*
                     * TODO: make group-size its own set of values.
                     */

                    int group = int(perf().calculate_mute(row, column));
                    bool enabled = bool(mutes[group]);
                    m_group_buttons[row][column]->setEnabled(enabled);
                }
            }
            update();
        }
    }
}

/**
 *
 */

void
qmutemaster::clear_mutes ()
{
    perf().clear_mutes();
}

/**
 *
 */

void
qmutemaster::setup_table ()
{
    QStringList columns;
    int w = ui->m_group_table->width();
    columns << "Group" << "Active" << "Key" << "Group Name (future)";
    ui->m_group_table->setHorizontalHeaderLabels(columns);
    ui->m_group_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect
    (
        ui->m_group_table, SIGNAL(currentCellChanged(int, int, int, int)),
        this, SLOT(mute_table_click_ex(int, int, int, int))
    );
    set_column_widths(w - SEQ66_TABLE_FIX);
    const int rows = ui->m_group_table->rowCount();
    for (int r = 0; r < rows; ++r)
        ui->m_group_table->setRowHeight(r, SEQ66_TABLE_ROW_HEIGHT);

}

/**
 *  Scales the columns against the provided window width. The width factors
 *  should add up to 1.
 */

void
qmutemaster::set_column_widths (int total_width)
{
    ui->m_group_table->setColumnWidth(0, int(0.125f * total_width));
    ui->m_group_table->setColumnWidth(1, int(0.125f * total_width));
    ui->m_group_table->setColumnWidth(2, int(0.10f * total_width));
    ui->m_group_table->setColumnWidth(3, int(0.65f * total_width));
}

/**
 *
 */

bool
qmutemaster::set_current_group (int row)
{
    bool result = row >= 0 && row < perf().mutegroup_count();
    if (result)
    {
        result = row != current_group();
        if (result)
        {
            m_current_group = row;
        }
    }
    return result;
}

/**
 *
 */

bool
qmutemaster::initialize_table ()
{
    bool result = false;
    int rows = perf().mutegroup_count();
    ui->m_group_table->clearContents();
    if (rows > 0)
    {
        for (int row = 0; row < rows; ++row)
        {
            mutegroup::number g = mutegroup::number(row);
            int mutecount = perf().count_mutes(g);
            std::string keyname = perf().lookup_mute_key(row);
            (void) group_line(g, mutecount, keyname);
        }
    }
    return result;
}

/**
 *  Retrieve the table cell at the given row and column.
 *
 * \param row
 *      The row number, which should be in the range of 0 to 32.
 *
 * \param col
 *      The column enumeration value, which will be in range.
 *
 * \return
 *      Returns a pointer the table widget-item for the given row and column.
 *      If out-of-range, a null pointer is returned.
 */

QTableWidgetItem *
qmutemaster::cell (mutegroup::number row, column_id col)
{
    int column = int(col);
    QTableWidgetItem * result = ui->m_group_table->item(row, column);
    if (is_nullptr(result))
    {
        result = new QTableWidgetItem;
        ui->m_group_table->setItem(row, column, result);
    }
    return result;
}

/**
 *  Obtains the group number (row), number of active (arming) items in the
 *  mute-group, and the keystroke used to access that mute-group.
 */

bool
qmutemaster::group_line
(
    mutegroup::number row,
    int mutecount,
    const std::string & keyname
)
{
    bool result = false;
    QTableWidgetItem * qtip = cell(row, column_id::group_number);
    if (not_nullptr(qtip))
    {
        std::string groupnostr = std::to_string(int(row));
        qtip->setText(groupnostr.c_str());
        qtip = cell(row, column_id::group_count);
        if (not_nullptr(qtip))
        {
            std::string gcountstr = std::to_string(mutecount);
            qtip->setText(gcountstr.c_str());
            qtip = cell(row, column_id::group_keyname);
            if (not_nullptr(qtip))
            {
                qtip->setText(keyname.c_str());
                result = true;
            }
        }
    }
    return result;
}

/**
 *  Handles a click in the table that lists the mute groups.
 */

void
qmutemaster::mute_table_click_ex
(
    int row, int /*column*/, int /*prevrow*/, int /*prevcolumn*/
)
{
    int rows = perf().mutegroup_count();
    if (rows > 0 && row >= 0 && row < rows)
    {
        if (set_current_group(row))
        {
            ui->m_button_modify->setEnabled(true);
            ui->m_button_reset->setEnabled(false);
            ui->m_button_down->setEnabled(true);
            ui->m_button_up->setEnabled(true);
            if (modify())
                update_group_buttons(true);
            else
                group_needs_update();
        }
    }
}

/**
 *
 */

void
qmutemaster::closeEvent (QCloseEvent * event)
{
    perf().unregister(this);            /* unregister this immediately      */
    event->accept();
}

/**
 *  Creates a grid of buttons in the grid layout.  This grid is always
 *  4 x 8, as discussed in the setmapper::calculate_set() function, but if a
 *  smaller set number (count) is used, some buttons will be unlabelled and
 *  disabled.
 *
 *  Note that the largest number of sets is 4 x 8 = 32.  This limitation is
 *  necessary because there are only so many available keys on the keyboard for
 *  pattern, mute-group, and set control.
 */

void
qmutemaster::create_group_buttons ()
{
    const QSize btnsize = QSize(32, 32);
    for (int row = 0; row < perf().mute_rows(); ++row)
    {
        for (int column = 0; column < perf().mute_columns(); ++column)
        {
            int group = int(perf().calculate_mute(row, column));
            std::string gstring = std::to_string(group);
            QPushButton * temp = new QPushButton(gstring.c_str());
            ui->setGridLayout->addWidget(temp, row, column);
            temp->setFixedSize(btnsize);
            connect
            (
                temp, &QPushButton::released, [=] { handle_group(row, column); }
            );
            temp->show();
            temp->setEnabled(false);
            m_group_buttons[row][column] = temp;
        }
    }
}

/**
 *
 */

void
qmutemaster::update_group_buttons (bool tomodify)
{
    midibooleans mutes = perf().get_mutes(current_group());
    for (int row = 0; row < perf().mute_rows(); ++row)
    {
        for (int column = 0; column < perf().mute_columns(); ++column)
        {
            QPushButton * temp = m_group_buttons[row][column];
            int mute = int(perf().calculate_mute(row, column));
            std::string gstring = std::to_string(mute);
            bool enabled = bool(mutes[mute]);
            if (tomodify)
            {
                modify(true);
                if (enabled)
                    gstring += "*";

                enabled = true;             /* all buttons will be enabled  */
                temp->setEnabled(true);
            }
            temp->setText(gstring.c_str());
            temp->setEnabled(enabled);
        }
    }
}

/**
 *
 */

void
qmutemaster::slot_modify ()
{
    modify(! modify());
    update_group_buttons(modify());
}

/**
 *
 */

void
qmutemaster::slot_reset ()
{
}

/**
 *
 */

void
qmutemaster::slot_down ()
{
    if (set_current_group(current_group() + 1))
        handle_group(current_group());
}

/**
 *
 */

void
qmutemaster::slot_up ()
{
    if (set_current_group(current_group() - 1))
        handle_group(current_group());
}

/**
 *  This function handles one of the mute buttons in the grid of group buttons.
 */

void
qmutemaster::handle_group (int row, int column)
{
    // mutegroup::number setno = perf().calculate_mute(row, column);
    // handle_group(setno);

    if (modify())
    {
        midibooleans mutes = perf().get_mutes(current_group());
        int mute = int(perf().calculate_mute(row, column));
        std::string gstring = std::to_string(mute);
        bool enabled = ! bool(mutes[mute]);         /* toggle the mute  */
        if (enabled)
            gstring += "*";

        mutes[mute] = midibool(enabled);
        if (perf().set_mutes(current_group(), mutes))
        {
            QPushButton * temp = m_group_buttons[row][column];
            temp->setText(gstring.c_str());
        }
    }
}

/**
 * USELESS
 */

void
qmutemaster::handle_group (int groupno)
{
    if (groupno != m_current_group)
    {
        set_current_group(groupno);
        update_group_buttons(false);            /* non-modification         */
    }
}

/**
 *  Handles mute-group changes from other dialogs.
 */

bool
qmutemaster::on_mutes_change (mutegroup::number group)
{
    bool result = ! mutegroup::none(group);     /* only "all sets" changes  */
    if (result)
    {
        result = initialize_table();
        if (result)
            group_needs_update();
    }
    return result;
}

/**
 *
 */

void
qmutemaster::keyPressEvent (QKeyEvent * event)
{
    keystroke kkey = qt_keystroke(event, SEQ66_KEYSTROKE_PRESS);
    bool done = handle_key_press(kkey);
    if (done)
        group_needs_update();
    else
        QWidget::keyPressEvent(event);              /* event->ignore()      */
}

/**
 *
 */

void
qmutemaster::keyReleaseEvent (QKeyEvent * event)
{
    keystroke kkey = qt_keystroke(event, SEQ66_KEYSTROKE_RELEASE);
    bool done = handle_key_release(kkey);
    if (done)
        update();
    else
        QWidget::keyReleaseEvent(event);            /* event->ignore()      */
}

/**
 *  This is not called when focus changes.
 */

void
qmutemaster::changeEvent (QEvent * event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange)
    {
    }
}

/**
 *
 */

bool
qmutemaster::handle_key_press (const keystroke & k)
{
    ctrlkey ordinal = k.key();
    const keycontrol & kc = perf().key_controls().control(ordinal);
    bool result = kc.is_usable();
    if (result)
    {
    }
    return result;
}

/**
 *
 */

bool
qmutemaster::handle_key_release (const keystroke & k)
{
    bool done = perf().midi_control_keystroke(k);
    if (! done)
    {
    }
    return done;
}

/**
 *
 */

bool
qmutemaster::group_control
(
    automation::action a, int /*d0*/, int index, bool inverse
)
{
    bool result = a == automation::action::toggle;
    if (result && ! inverse)
        handle_group(index);

    return result;
}

}               // namespace seq66

/*
 * qmutemaster.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

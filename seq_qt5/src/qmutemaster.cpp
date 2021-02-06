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
 *  This module declares/defines the base class for the mute-master tab.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-05-29
 * \updates       2021-02-05
 * \license       GNU GPLv2 or above
 *
 */

#include <QPushButton>
#include <QTableWidgetItem>
#include <QTimer>

#include "seq66-config.h"               /* defines SEQ66_QMAKE_RULES        */
#include "cfg/settings.hpp"             /* seq66::rc()                      */
#include "ctrl/keystroke.hpp"           /* seq66::keystroke class           */
#include "util/filefunctions.hpp"       /* seq66::name_has_directory()      */
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
#define SEQ66_BUTTON_SIZE               24

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
    m_pattern_buttons       (),                             /* 2-D arrary   */
    m_current_group         (seq::unassigned()),            /* important    */
    m_group_count           (cb_perf().mutegroup_count()),
    m_button_row            (seq::unassigned()),
    m_button_column         (seq::unassigned()),
    m_trigger_active        (false),
    m_needs_update          (true),
    m_pattern_mutes         (),
    m_trigger_mode          (false),
    m_pattern_offset        (0)
{
    ui->setupUi(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    clear_pattern_mutes();              /* empty the pattern bits           */

    /*
     * Connect the bin/hex radio buttons and set them as per the configured
     * status at start-up.
     */

    connect
    (
        ui->m_radio_binary, SIGNAL(toggled(bool)),
        this, SLOT(slot_bin_mode(bool))
    );
    connect
    (
        ui->m_radio_hex, SIGNAL(toggled(bool)),
        this, SLOT(slot_hex_mode(bool))
    );
    set_bin_hex(! cb_perf().mutes().group_format_hex());

    ui->m_button_trigger->setEnabled(true);
    ui->m_button_trigger->setCheckable(true);
    connect
    (
        ui->m_button_trigger, SIGNAL(clicked()),
        this, SLOT(slot_trigger())
    );

    ui->m_button_set_mutes->setEnabled(false);
    connect
    (
        ui->m_button_set_mutes, SIGNAL(clicked()),
        this, SLOT(slot_set_mutes())
    );

    bool large = cb_perf().screenset_size() > mutegroups::Size();
    if (large)
        large = (cb_perf().screenset_size() % mutegroups::Size()) == 0;

    if (large)
    {
        int max = cb_perf().screenset_size() / mutegroups::Size() - 1;
        ui->m_pattern_offset_spinbox->setRange(0, max);
    }
    ui->m_pattern_offset_spinbox->setEnabled(large);
    connect
    (
        ui->m_pattern_offset_spinbox, SIGNAL(valueChanged(int)),
        this, SLOT(slot_pattern_offset(int))
    );

    ui->m_button_down->setEnabled(false);   // ui->m_button_down->hide();
    connect(ui->m_button_down, SIGNAL(clicked()), this, SLOT(slot_down()));

    ui->m_button_up->setEnabled(false);     // ui->m_button_up->hide();
    connect(ui->m_button_up, SIGNAL(clicked()), this, SLOT(slot_up()));

    ui->m_button_load->setEnabled(true);
    connect(ui->m_button_load, SIGNAL(clicked()), this, SLOT(slot_load()));

    /*
     * Set to false further down.
     *
     * ui->m_button_save->setEnabled(true);
     */

    connect(ui->m_button_save, SIGNAL(clicked()), this, SLOT(slot_save()));

    ui->m_check_to_midi->setEnabled(true);
    ui->m_check_to_midi->setChecked(cb_perf().mutes().group_save_to_midi());
    connect
    (
        ui->m_check_to_midi, SIGNAL(stateChanged(int)),
        this, SLOT(slot_write_to_midi())
    );

    ui->m_check_to_mutes->setEnabled(true);
    ui->m_check_to_mutes->setChecked(cb_perf().mutes().group_save_to_mutes());
    connect
    (
        ui->m_check_to_mutes, SIGNAL(stateChanged(int)),
        this, SLOT(slot_write_to_mutes())
    );

    create_group_buttons();
    create_pattern_buttons();
    connect
    (
        ui->m_button_clear_all, SIGNAL(clicked()),
        this, SLOT(slot_clear_all_mutes())
    );

    QString mgfname = "backup-";
    mgfname += QString::fromStdString(rc().mute_group_filename());
    ui->m_mute_basename->setPlainText(mgfname);
    ui->m_mute_basename->setEnabled(true);

    setup_table();                      /* row and column sizing            */
    (void) initialize_table();          /* fill with mute-group information */
    handle_group_button(0, 0);          /* guaranteed to be present         */
    handle_group(0);                    /* select the first group           */
    ui->m_button_save->setEnabled(false);

    cb_perf().enregister(this);         /* register this for notifications  */
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
    m_timer->stop();
    cb_perf().unregister(this);            /* unregister this immediately      */
    delete ui;
}

void
qmutemaster::conditional_update ()
{
    if (needs_update())                 /*  perf().needs_update() too iffy  */
    {
        update_group_buttons();
        update_pattern_buttons();
        update();
    }
}

void
qmutemaster::slot_pattern_offset (int index)
{
    m_pattern_offset = index * cb_perf().screenset_size();
}

void
qmutemaster::slot_clear_all_mutes ()
{
    cb_perf().clear_mutes();
    ui->m_button_save->setEnabled(true);
    group_needs_update();
}

void
qmutemaster::clear_pattern_mutes ()
{
    m_pattern_mutes.clear();
    m_pattern_mutes.resize(cb_perf().mutegroup_count());    /* always 32    */
    m_pattern_offset = 0;
    m_trigger_mode = false;
}

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
        this, SLOT(slot_table_click(int, int, int, int))
    );
    set_column_widths(w - SEQ66_TABLE_FIX);
    const int rows = ui->m_group_table->rowCount();
    for (int r = 0; r < rows; ++r)
        ui->m_group_table->setRowHeight(r, SEQ66_TABLE_ROW_HEIGHT);
}

/**
 *  Scales the columns against the provided window width. The width factors
 *  should add up to 1.  However, we make the name column narrower.
 */

void
qmutemaster::set_column_widths (int total_width)
{
    ui->m_group_table->setColumnWidth(0, int(0.150f * total_width));
    ui->m_group_table->setColumnWidth(1, int(0.150f * total_width));
    ui->m_group_table->setColumnWidth(2, int(0.100f * total_width));
    ui->m_group_table->setColumnWidth(3, int(0.55f * total_width)); // 0.65
}

bool
qmutemaster::set_current_group (int row)
{
    bool result = row != current_group();
    if (result)
        result = row >= 0 && row < cb_perf().mutegroup_count();

    if (result)
    {
        int gridrow, gridcolumn;
        if (mutegroups::group_to_grid(row, gridrow, gridcolumn))
        {
            if (m_button_row >= 0)
            {
                QPushButton * temp =
                    m_group_buttons[m_button_row][m_button_column];

                temp->setEnabled(false);
            }
            m_button_row = gridrow;
            m_button_column = gridcolumn;
            m_current_group = row;

            QPushButton * temp = m_group_buttons[m_button_row][m_button_column];
            temp->setEnabled(true);
        }
    }
    return result;
}

bool
qmutemaster::initialize_table ()
{
    bool result = false;
    int rows = cb_perf().mutegroup_count();
    ui->m_group_table->clearContents();
    if (rows > 0)
    {
        for (int row = 0; row < rows; ++row)
        {
            mutegroup::number g = mutegroup::number(row);
            int mutecount = cb_perf().count_mutes(g);
            std::string keyname = cb_perf().lookup_mute_key(row);
            std::string groupname = cb_perf().group_name(row);
            (void) group_line(g, mutecount, keyname, groupname);
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
 *      The column enumeration value, which indicates one of these four
 *      columns: number, count, keyname, and name.
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
    const std::string & keyname,
    const std::string & groupname
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
                qtip = cell(row, column_id::group_name);
                if (not_nullptr(qtip))
                {
                    qtip->setText(groupname.c_str());
                    result = true;
                }
            }
        }
    }
    return result;
}

/**
 *  Handles a click in the table that lists the mute groups.
 */

void
qmutemaster::slot_table_click
(
    int row, int /*column*/, int /*prevrow*/, int /*prevcolumn*/
)
{
    if (! trigger())
    {
        int rows = cb_perf().mutegroup_count();             /* always 32    */
        if (rows > 0 && row >= 0 && row < rows)
        {
            if (set_current_group(row))
            {
                ui->m_button_trigger->setEnabled(true);
                ui->m_button_set_mutes->setEnabled(false);

#if defined READY_FOR_PRIME_TIME
                ui->m_button_down->setEnabled(true);
                ui->m_button_up->setEnabled(true);
#endif
                group_needs_update();
            }
        }
    }
}

void
qmutemaster::closeEvent (QCloseEvent * event)
{
    cb_perf().unregister(this);            /* unregister this immediately      */
    event->accept();
}

/**
 *  Creates a grid of buttons in the group/set grid layout.  This grid is
 *  always 4 x 8, as discussed in the setmapper::grid_to_group() function, but
 *  if a smaller set number (count) is used, some buttons will be unlabelled
 *  and disabled.
 *
 *  Note that the largest number of sets is 4 x 8 = 32.  This limitation is
 *  necessary because there are only so many available keys on the keyboard
 *  for pattern, mute-group, and set control.
 */

void
qmutemaster::create_group_buttons ()
{
    const QSize btnsize = QSize(SEQ66_BUTTON_SIZE, SEQ66_BUTTON_SIZE);
    int group_rows = mutegroups::Rows();
    int group_columns = mutegroups::Columns();
    for (int row = 0; row < group_rows; ++row)
    {
        for (int column = 0; column < group_columns; ++column)
        {
            mutegroup::number m = mutegroups::grid_to_group(row, column);
            std::string gstring = std::to_string(m);
            QPushButton * temp = new QPushButton(gstring.c_str());
            ui->setGridLayout->addWidget(temp, row, column);
            temp->setFixedSize(btnsize);
            connect                             /* connect lambda function */
            (
                temp, &QPushButton::released,
                [=] { handle_group_button(row, column); }
            );
            temp->show();
            temp->setCheckable(true);
            temp->setEnabled(false);
            m_group_buttons[row][column] = temp;
        }
    }
}

/**
 *  Updates the top buttons that indicate the mute-groups present during this
 *  run of the current MIDI file.
 */

void
qmutemaster::update_group_buttons (enabling tomodify)
{
    midibooleans groups = cb_perf().get_active_groups();
    int group_rows = mutegroups::Rows();
    int group_columns = mutegroups::Columns();
    for (int row = 0; row < group_rows; ++row)
    {
        for (int column = 0; column < group_columns; ++column)
        {
            QPushButton * temp = m_group_buttons[row][column];
            mutegroup::number m = mutegroups::grid_to_group(row, column);
            temp->setChecked(bool(groups[m]));
            if (tomodify != enabling::leave)
                temp->setEnabled(tomodify == enabling::enable);
        }
    }
}

void
qmutemaster::set_bin_hex (bool bin_checked)
{
    if (bin_checked)
    {
        ui->m_radio_binary->setChecked(true);
        ui->m_radio_hex->setChecked(false);
    }
    else
    {
        ui->m_radio_binary->setChecked(false);
        ui->m_radio_hex->setChecked(true);
    }
}

void
qmutemaster::slot_bin_mode (bool ischecked)
{
    cb_perf().mutes().group_format_hex(! ischecked);
    set_bin_hex(ischecked);
    ui->m_button_save->setEnabled(true);
}

void
qmutemaster::slot_hex_mode (bool ischecked)
{
    cb_perf().mutes().group_format_hex(ischecked);
    set_bin_hex(! ischecked);
    ui->m_button_save->setEnabled(true);
}

void
qmutemaster::slot_trigger ()
{
    trigger(! trigger());
    if (trigger())
    {
        update_group_buttons(enabling::enable);
        update_pattern_buttons(enabling::disable);
    }
    else
    {
        QPushButton * temp = m_group_buttons[m_button_row][m_button_column];
        update_group_buttons(enabling::disable);
        update_pattern_buttons(enabling::disable);
        temp->setEnabled(true);
    }
}

/**
 *  The calls to set mutes:  fill midibooleans bit and call
 */

void
qmutemaster::slot_set_mutes ()
{
    midibooleans bits = m_pattern_mutes;
    bool ok = cb_perf().set_mutes(current_group(), bits);
    if (! ok)
    {
        // TODO show the error
    }
}

/**
 *  A slot for handle_group(), meant to move a table row down.
 *  Not yet ready for primetime.
 */

void
qmutemaster::slot_down ()
{
    if (set_current_group(current_group() + 1))
        handle_group(current_group());
}

/**
 *  A slot for handle_group(), meant to move a table row up.
 *  Not yet ready for primetime.
 */

void
qmutemaster::slot_up ()
{
    if (set_current_group(current_group() - 1))
        handle_group(current_group());
}

/**
 *  This looks goofy, but we offload the dialog handling to qsmainwnd, which
 *  has the boolean function qsmainwnd::open_mutes_dialog(), which returns true
 *  if the user clicked OK and the call to qmutemaster::load_mutegroups()
 *  succeeded.  Circular!
 */

void
qmutemaster::slot_load ()
{
    if (not_nullptr(m_main_window))
    {
        m_main_window->show_open_mutes_dialog();    /* to load_mutegroups() */
        ui->m_button_save->setEnabled(false);
    }
}

bool
qmutemaster::load_mutegroups (const std::string & mutefile)
{
    bool result = cb_perf().open_mutegroups(mutefile);
    if (result)
    {
        file_message("Opened mute-groups", mutefile);
        group_needs_update();
    }
    else
        file_message("Opened failed", mutefile);

    return result;
}

void
qmutemaster::slot_save ()
{
#if 0
    QString filename = ui->m_mute_basename->toPlainText();
    std::string mutefile = filename.toStdString();
    bool ok = show_file_dialog
    (
        this, mutefile, "Save mute-groups file",
        "Mute-Groups (*.mutes);;All files (*)", SavingFile, ConfigFile,
        ".mutes"
    );
    if (ok)
    {
        /*
         * EXPERIMENT: REPLACE if (save_mutegroups(mutefile))
         */

        if (cb_perf().save_mutegroups(mutefile))
            file_message("Wrote mute-groups", mutefile);
        else
            file_message("Write failed", mutefile);
    }
#endif
    if (not_nullptr(m_main_window))
    {
        m_main_window->show_save_mutes_dialog();
    }
}

bool
qmutemaster::save_mutegroups (const std::string & mutefile)
{
    bool result = cb_perf().save_mutegroups(mutefile);
    if (result)
        file_message("Wrote mute-groups", mutefile);
    else
        file_message("Write failed", mutefile);

    return result;
}

void
qmutemaster::slot_write_to_midi()
{
    bool ismidichecked = ui->m_check_to_midi->isChecked();
    bool ismuteschecked = ui->m_check_to_mutes->isChecked();
    cb_perf().mutes().group_save(ismidichecked, ismuteschecked);
}

void
qmutemaster::slot_write_to_mutes()
{
    bool ismidichecked = ui->m_check_to_midi->isChecked();
    bool ismuteschecked = ui->m_check_to_mutes->isChecked();
    cb_perf().mutes().group_save(ismidichecked, ismuteschecked);
}

/**
 *  This function handles one of the mute buttons in the grid of group
 *  buttons.
 */

void
qmutemaster::handle_group_button (int row, int column)
{
    QPushButton * button = m_group_buttons[row][column];
    bool checked = button->isChecked();
    if (trigger())                              /* we are in trigger mode   */
    {
        if (checked)                            /* ignore the button        */
        {
            button->setChecked(false);          /* keep it unchecked        */
        }
        else                                    /* turn on the mute-group   */
        {
            mutegroup::number m = mutegroups::grid_to_group(row, column);
            (void) cb_perf().toggle_mutes(m);
            button->setChecked(true);           /* keep it checked          */
            ui->m_group_table->selectRow(m);
        }
    }
    else
    {
        if (checked)                            /* was inactive, now active */
        {
            update_pattern_buttons(enabling::enable);
        }
        else                                    /* was active, now inactive */
        {
            update_pattern_buttons(enabling::disable);
        }
        ui->m_button_set_mutes->setEnabled(true);
    }
}

void
qmutemaster::handle_group (int groupno)
{
    if (groupno != m_current_group)
    {
        set_current_group(groupno);
        ui->m_group_table->selectRow(0);
        update_group_buttons();
        update_pattern_buttons();
        ui->m_button_save->setEnabled(true);
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

bool
qmutemaster::handle_key_press (const keystroke & k)
{
    ctrlkey ordinal = k.key();
    const keycontrol & kc = cb_perf().key_controls().control(ordinal);
    bool result = kc.is_usable();
    if (result)
    {
    }
    return result;
}

bool
qmutemaster::handle_key_release (const keystroke & k)
{
    bool done = cb_perf().midi_control_keystroke(k);
    if (! done)
    {
    }
    return done;
}

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

/*
 *  Section for controlling the pattern grid.
 */

/**
 *  Creates a grid of buttons in the pattern grid layout.  This grid is
 *  currently 4 x 8, but might eventually approach 12 x 8.  We shall see.
 *  Activating a button adds that pattern to the currently-selected
 *  mute-group.
 */

void
qmutemaster::create_pattern_buttons ()
{
    const QSize btnsize = QSize(SEQ66_BUTTON_SIZE, SEQ66_BUTTON_SIZE);
    int pattern_rows = cb_perf().mute_rows();
    int pattern_columns = cb_perf().mute_columns();
    for (int row = 0; row < pattern_rows; ++row)
    {
        for (int column = 0; column < pattern_columns; ++column)
        {
            mutegroup::number m = mutegroups::grid_to_group(row, column);
            std::string gstring = std::to_string(m);
            QPushButton * temp = new QPushButton(gstring.c_str());
            ui->patternGridLayout->addWidget(temp, row, column);
            temp->setFixedSize(btnsize);
            connect                             /* connect lambda function */
            (
                temp, &QPushButton::released,
                [=] { handle_pattern_button(row, column); }
            );
            temp->show();
            temp->setCheckable(true);
            temp->setEnabled(false);
            m_pattern_buttons[row][column] = temp;
        }
    }
}

/**
 *  Updates the top buttons that indicate the mute-groups present during this
 *  run of the current MIDI file.
 *
 * \todo
 *      -   Calculate the pattern rows and columns the proper way.
 *      -   Deal with cases other than 4 x 8 properly.
 */

void
qmutemaster::update_pattern_buttons (enabling tomodify)
{
    int pattern_rows = cb_perf().mute_rows();
    int pattern_columns = cb_perf().mute_columns();
    midibooleans mutes = cb_perf().get_mutes(current_group());
    if (! mutes.empty())
    {
        for (int row = 0; row < pattern_rows; ++row)
        {
            for (int column = 0; column < pattern_columns; ++column)
            {
                QPushButton * temp = m_pattern_buttons[row][column];
                seq::number s = cb_perf().grid_to_seq(row, column);
                temp->setChecked(bool(mutes[s]));
                if (tomodify != enabling::leave)
                    temp->setEnabled(tomodify == enabling::enable);
            }
        }
    }
}

/**
 *  This function handles one of the pattern buttons in the grid of pattern
 *  buttons.  All it does is change the status of the appropriate bit in the
 *  midibooleans pattern vector.
 */

void
qmutemaster::handle_pattern_button (int row, int column)
{
    QPushButton * temp = m_pattern_buttons[row][column];
    seq::number s = cb_perf().grid_to_seq(row, column);
    bool bitisset = bool(m_pattern_mutes[s]);
    bool enabled = temp->isChecked();
    if (bitisset != enabled)
    {
        m_pattern_mutes[s] = midibool(enabled);
        ui->m_button_set_mutes->setEnabled(true);
    }
}

}               // namespace seq66

/*
 * qmutemaster.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

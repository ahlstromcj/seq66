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
 *  Place, Suite 330, Boston, MA 02111-1307 USA
 */

/**
 * \file          qmutemaster.cpp
 *
 *  This module declares/defines the base class for the mute-master tab.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-05-29
 * \updates       2021-11-02
 * \license       GNU GPLv2 or above
 *
 */

#include <QKeyEvent>                    /* Needed for QKeyEvent::accept()   */
#include <QPushButton>
#include <QTableWidgetItem>
#include <QTimer>

#include "seq66-config.h"               /* defines SEQ66_QMAKE_RULES        */
#include "cfg/settings.hpp"             /* seq66::rc()                      */
#include "ctrl/keystroke.hpp"           /* seq66::keystroke class           */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "qmutemaster.hpp"              /* seq66::qmutemaster, this class   */
#include "qsmainwnd.hpp"                /* seq66::qsmainwnd main window     */
#include "qt5_helpers.hpp"              /* seq66::qt_keystroke() etc.       */
#include "util/filefunctions.hpp"       /* seq66::name_has_path()           */

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

static const int c_table_row_height = 18;
static const int c_table_fix        = 16;
static const int c_button_size      = 24;

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
    m_timer                 (nullptr),
    m_main_window           (mainparent),
    m_group_buttons         (mutegroups::Size(), nullptr),  /* "2-D" arrary */
    m_pattern_buttons       (mutegroups::Size(), nullptr),  /* "2-D" arrary */
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
    connect
    (
        ui->m_mute_basename, SIGNAL(textChanged()),
        this, SLOT(slot_mutes_file_modify())
    );

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

    ui->m_button_down->setEnabled(false);
    connect(ui->m_button_down, SIGNAL(clicked()), this, SLOT(slot_down()));
    ui->m_button_up->setEnabled(false);
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
    ui->m_check_load_mutes->setEnabled(true);
    ui->m_check_load_mutes->setChecked
    (
        cb_perf().mutes().group_load_from_mutes()
    );
    connect
    (
        ui->m_check_load_mutes, SIGNAL(stateChanged(int)),
        this, SLOT(slot_load_mutes())
    );
    ui->m_check_toggle_active->setEnabled(true);
    ui->m_check_toggle_active->setChecked
    (
        cb_perf().mutes().toggle_active_only()
    );
    connect
    (
        ui->m_check_toggle_active, SIGNAL(stateChanged(int)),
        this, SLOT(slot_toggle_active())
    );
    connect
    (
        ui->m_group_table, SIGNAL(cellChanged(int, int)),
        this, SLOT(slot_cell_changed(int, int))
    );

    QString mgfname = qt(rc().mute_group_filename());
    ui->m_mute_basename->setPlainText(mgfname);
    ui->m_mute_basename->setEnabled(true);
    setup_table();                      /* row and column sizing            */
    (void) initialize_table();          /* fill with mute-group information */
    handle_group_button(0, 0);          /* guaranteed to be present         */
    handle_group(0);                    /* select the first group           */
    ui->m_button_save->setEnabled(false);

    cb_perf().enregister(this);         /* register this for notifications  */
    group_needs_update();               /* guarantee the initial load       */
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
    columns << "Group" << "Active" << "Key" << "Group Name";
    ui->m_group_table->setHorizontalHeaderLabels(columns);
    ui->m_group_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect
    (
        ui->m_group_table, SIGNAL(currentCellChanged(int, int, int, int)),
        this, SLOT(slot_table_click(int, int, int, int))
    );
    set_column_widths(w - c_table_fix);
    const int rows = ui->m_group_table->rowCount();
    for (int r = 0; r < rows; ++r)
        ui->m_group_table->setRowHeight(r, c_table_row_height);
}

void
qmutemaster::slot_cell_changed (int row, int column)
{
    column_id cid = static_cast<column_id>(column);
    if (cid == column_id::group_name)
    {
        mutegroup::number m = mutegroup::number(row);
        QTableWidgetItem * c = cell(m, cid);
        QString qtext = c->text();
        std::string name = qtext.toStdString();

        /*
         * A cumbersome alternative.
         *
         * mutegroup & mg = cb_perf().mute_group(m);
         */

        cb_perf().group_name(m, name);
    }
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
    ui->m_group_table->setColumnWidth(2, int(0.150f * total_width));    // 0.100
    ui->m_group_table->setColumnWidth(3, int(0.55f * total_width));     // 0.65
}

bool
qmutemaster::set_current_group (int group)
{
    bool result = group != current_group();
    if (result)
        result = group >= 0 && group < cb_perf().mutegroup_count();

    if (result)
    {
        int gridrow, gridcolumn;
        if (mutegroups::group_to_grid(group, gridrow, gridcolumn))
        {
            if (m_button_row >= 0)          // ???????
            {
                QPushButton * temp = m_group_buttons[group];
                temp->setEnabled(false);
            }
            m_button_row = gridrow;
            m_button_column = gridcolumn;
            m_current_group = group;

            QPushButton * temp = m_group_buttons[group];
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
        qtip->setText(qt(groupnostr));
        qtip = cell(row, column_id::group_count);
        if (not_nullptr(qtip))
        {
            std::string gcountstr = std::to_string(mutecount);
            qtip->setText(qt(gcountstr));
            qtip = cell(row, column_id::group_keyname);
            if (not_nullptr(qtip))
            {
                qtip->setText(qt(keyname));
                qtip = cell(row, column_id::group_name);
                if (not_nullptr(qtip))
                {
                    qtip->setText(qt(groupname));
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
 *  Note that the largest number of mute-groups is 4 x 8 = 32.  This
 *  limitation is practically necessary because there are only so many
 *  available keys on the keyboard for pattern, mute-group, and set control.
 */

void
qmutemaster::create_group_buttons ()
{
    const QSize btnsize = QSize(c_button_size, c_button_size);
    for (int group = 0; group < mutegroups::Size(); ++group)
    {
        int row, column;
        if (mutegroups::group_to_grid(group, row, column))
        {
            std::string gstring = std::to_string(group);
            QPushButton * temp = new QPushButton(qt(gstring));
            temp->setFixedSize(btnsize);
            ui->setGridLayout->addWidget(temp, row, column);
            connect                             /* connect lambda function */
            (
                temp, &QPushButton::released,
                [=] { handle_group_button(row, column); }
            );
            temp->show();
            temp->setCheckable(true);
            temp->setEnabled(false);
            m_group_buttons[group] = temp;
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
    for (int group = 0; group < mutegroups::Size(); ++group)
    {
        QPushButton * temp = m_group_buttons[group];
        temp->setChecked(bool(groups[group]));
        if (tomodify != enabling::leave)
            temp->setEnabled(tomodify == enabling::enable);
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
qmutemaster::slot_mutes_file_modify ()
{
    ui->m_button_save->setEnabled(true);
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
        int row = m_button_row;
        int column = m_button_column;
        mutegroup::number group = mutegroups::grid_to_group(row, column);
        QPushButton * temp = m_group_buttons[group];
        update_group_buttons(enabling::disable);
        update_pattern_buttons(enabling::disable);
        temp->setEnabled(true);
    }
}

/**
 *  The calls to set mutes:  Fills midibooleans bit and calls
 *  performer::set_mutes(), with a parameter of true so that the mutes are
 *  also copied to rcsettings for when the user of the Mutes tab wants to save
 *  the file.
 */

void
qmutemaster::slot_set_mutes ()
{
    midibooleans bits = m_pattern_mutes;
    bool ok = cb_perf().set_mutes(current_group(), bits, true);
    if (ok)
    {
        ui->m_button_save->setEnabled(true);
    }
    else
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
        m_main_window->open_mutes_dialog();     /* calls load_mutegroups()  */
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
        ui->m_button_save->setEnabled(false);
    }
    else
        file_message("Opened failed", mutefile);

    return result;
}

void
qmutemaster::slot_save ()
{
    if (not_nullptr(m_main_window))
    {
        std::string fname = ui->m_mute_basename->toPlainText().toStdString();
        if (fname.empty())
        {
            /*
             * Use default 'mutes' name
             */
        }
        else
        {
            if (name_has_path(fname))
            {
                std::string directory;
                std::string basename;
                bool ok = filename_split(fname, directory, basename);
                if (ok)
                    fname = basename;
            }
            rc().mute_group_filename(fname);
        }

        /*
         *  Set the base-name of the 'mutes' files, then pass the mute-group
         *  bits to performer to update the group and its rcsettings copy.
         */

        midibooleans bits = m_pattern_mutes;
        bool ok = cb_perf().put_mutes();
        if (ok)
            m_main_window->save_mutes_dialog(rc().mute_group_filespec());
    }
}

bool
qmutemaster::save_mutegroups (const std::string & mutefile)
{
    bool result = cb_perf().save_mutegroups(mutefile);
    if (result)
    {
        file_message("Wrote mute-groups", mutefile);
        ui->m_button_save->setEnabled(false);
    }
    else
        file_message("Write failed", mutefile);

    return result;
}

void
qmutemaster::slot_write_to_midi ()
{
    bool ismidichecked = ui->m_check_to_midi->isChecked();
    bool ismuteschecked = ui->m_check_to_mutes->isChecked();
    cb_perf().mutes().group_save(ismidichecked, ismuteschecked);
}

void
qmutemaster::slot_write_to_mutes ()
{
    bool ismidichecked = ui->m_check_to_midi->isChecked();
    bool ismuteschecked = ui->m_check_to_mutes->isChecked();
    cb_perf().mutes().group_save(ismidichecked, ismuteschecked);
}

void
qmutemaster::slot_load_mutes ()
{
    bool ischecked = ui->m_check_load_mutes->isChecked();
    cb_perf().mutes().load_mute_groups(ischecked);
}

void
qmutemaster::slot_toggle_active ()
{
    bool ischecked = ui->m_check_toggle_active->isChecked();
    cb_perf().mutes().toggle_active_only(ischecked);
}

/**
 *  This function handles one of the mute buttons in the grid of group
 *  buttons.
 */

void
qmutemaster::handle_group_button (int row, int column)
{
    mutegroup::number group = mutegroups::grid_to_group(row, column);
    QPushButton * button = m_group_buttons[group];
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
            update_pattern_buttons(enabling::enable);
        else                                    /* was active, now inactive */
            update_pattern_buttons(enabling::disable);

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

        // ui->m_button_save->setEnabled(true);
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

/*
 *  We must accept() the key-event, otherwise even key-events in the QLineEdit
 *  items are propagated to the parent, where they then get passed to the
 *  performer as if they were keyboards controls (such as a pattern-toggle
 *  hot-key).
 *
 *  Plus, here, we have no real purpose for the code, so we macro it out.
 *  What's up with that, Spunky?
 */

void
qmutemaster::keyPressEvent (QKeyEvent * event)
{
#if defined PASS_KEYSTROKES_TO_PARENT
    keystroke kkey = qt_keystroke(event, keystroke::action::press);
    bool done = handle_key_press(kkey);
    if (done)
        group_needs_update();
    else
        QWidget::keyPressEvent(event);
#else
    event->accept();
#endif
}

void
qmutemaster::keyReleaseEvent (QKeyEvent * event)
{
#if defined PASS_KEYSTROKES_TO_PARENT
    keystroke kkey = qt_keystroke(event, keystroke::action::release);
    bool done = handle_key_release(kkey);
    if (done)
        update();
    else
        QWidget::keyReleaseEvent(event);
#else
    event->accept();
#endif
}

#if defined PASS_KEYSTROKES_TO_PARENT

bool
qmutemaster::handle_key_press (const keystroke & k)
{
    ctrlkey ordinal = k.key();
    const keycontrol & kc = cb_perf().key_controls().control(ordinal);
    bool result = kc.is_usable();
    if (result)
    {
        // no code
    }
    return result;
}

bool
qmutemaster::handle_key_release (const keystroke & k)
{
    bool done = cb_perf().midi_control_keystroke(k);
    if (! done)
    {
        // no code
    }
    return done;
}

#endif  // defined PASS_KEYSTROKES_TO_PARENT

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
    const QSize btnsize = QSize(c_button_size, c_button_size);
    int tracks = cb_perf().screenset_size();
    for (int t = 0; t < tracks; ++t)
    {
        int row, column;
        if (cb_perf().master_index_to_grid(t, row, column))
        {
            std::string gstring = std::to_string(t);
            QPushButton * temp = new QPushButton(qt(gstring));
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
            m_pattern_buttons[t] = temp;
        }
    }
}

/**
 *  Updates the bottom buttons that indicate the mute-groups present during
 *  this run of the current MIDI file.
 *
 * \todo
 */

void
qmutemaster::update_pattern_buttons (enabling tomodify)
{
    midibooleans mutes = cb_perf().get_mutes(current_group());
    if (! mutes.empty())
    {
        m_pattern_mutes = mutes;
        int tracks = cb_perf().screenset_size();
        for (int t = 0; t < tracks; ++t)
        {
            QPushButton * temp = m_pattern_buttons[t];
            temp->setChecked(bool(mutes[t]));
            if (tomodify != enabling::leave)
                temp->setEnabled(tomodify == enabling::enable);
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
    seq::number s = cb_perf().grid_to_seq(row, column);
    QPushButton * temp = m_pattern_buttons[s];
    bool bitisset = bool(m_pattern_mutes[s]);
    bool enabled = temp->isChecked();
    if (bitisset != enabled)
    {
        m_pattern_mutes[s] = midibool(enabled);
        ui->m_button_set_mutes->setEnabled(true);

        /*
         * Do not enable until the "Update Group" button is pressed.
         *
         * ui->m_button_save->setEnabled(true);
         */
    }
}

}               // namespace seq66

/*
 * qmutemaster.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

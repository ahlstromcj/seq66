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
 * \updates       2023-10-04
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
static const int c_button_size      = 26;   // 28; // 26; // 24;

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
    m_is_initialized        (false),
    m_to_midi_active        (false),
    m_to_mutes_active       (false),
    m_group_buttons         (mutegroups::Size(), nullptr),  /* "2-D" arrary */
    m_pattern_buttons       (p.screenset_size(), nullptr),  /* group_count()*/
    m_current_group         (seq::unassigned()),            /* important    */
    m_group_count           (cb_perf().mutegroup_count()),
    m_button_row            (seq::unassigned()),
    m_button_column         (seq::unassigned()),
    m_trigger_active        (false),
    m_needs_update          (true),
    m_pattern_mutes         (),
    m_pattern_offset        (0)
{
    ui->setupUi(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    clear_pattern_mutes();              /* empty the pattern bits           */
    create_group_buttons();
    create_pattern_buttons();
    setup_table();                      /* row and column sizing            */
    (void) initialize_table();          /* fill with mute-group information */

    QString mgfname = qt(rc().mute_group_filename());
    bool mutesactive = rc().mute_group_file_active();
    ui->m_mute_basename->setPlainText(mgfname);
    ui->m_check_mutes_active->setChecked(mutesactive);
#if defined USE_MUTES_FILE_TEXTEDIT
    bool mgfactive = rc().mute_group_file_active();
    ui->m_mute_basename->setEnabled(mgfactive);
    connect
    (
        ui->m_mute_basename, SIGNAL(textChanged()),
        this, SLOT(slot_mutes_file_modify())
    );
#endif

    /*
     * First set the status of the bin/hex radio buttons, and only then
     * connect the bin/hex radio buttons and set them as per the configured
     * status at start-up.
     */

    set_bin_hex(! cb_perf().group_format_hex());
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
    ui->m_button_trigger->setEnabled(true);
    ui->m_button_trigger->setCheckable(true);
    connect
    (
        ui->m_button_trigger, SIGNAL(clicked()),
        this, SLOT(slot_trigger())
    );
    ui->m_button_set_mutes->setEnabled(false);

#if defined USE_GROUP_UPDATE_BUTTON
    connect
    (
        ui->m_button_set_mutes, SIGNAL(clicked()),
        this, SLOT(slot_set_mutes())
    );
#endif

#if defined USE_REMOVED_MUTEMASTER_BUTTONS

    /*
     * If we add back the pattern-offset spin-box, we can put this
     * to the right of the xxxx label.
     */

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

#endif

    ui->m_button_load->setEnabled(true);
    connect(ui->m_button_load, SIGNAL(clicked()), this, SLOT(slot_load()));

    /*
     * We don't care if the user wants to save the current mute-groups
     * right away. They might have been read from the MIDI file, and the
     * user wants to keep a copy of the mute-groups.
     */

    ui->m_button_save->setEnabled(true);
    connect(ui->m_button_save, SIGNAL(clicked()), this, SLOT(slot_save()));

    /*
     * To MIDI
     */

    bool tomidi = cb_perf().group_save_to_midi();
    ui->m_check_to_midi->setEnabled(true);
    ui->m_check_to_midi->setChecked(tomidi);
    m_to_midi_active = tomidi;
    connect
    (
        ui->m_check_to_midi, SIGNAL(stateChanged(int)),
        this, SLOT(slot_write_to_midi())
    );

    /*
     * To Mutes
     */

    bool tofile = cb_perf().group_save_to_mutes();
    ui->m_check_to_mutes->setEnabled(true);
    ui->m_check_to_mutes->setChecked(tofile);
    m_to_mutes_active = tofile;
    connect
    (
        ui->m_check_to_mutes, SIGNAL(stateChanged(int)),
        this, SLOT(slot_write_to_mutes())
    );
    connect
    (
        ui->m_button_clear_all, SIGNAL(clicked()),
        this, SLOT(slot_clear_all_mutes())
    );
    connect
    (
        ui->m_button_fill, SIGNAL(clicked()),
        this, SLOT(slot_fill_mutes())
    );
    ui->m_check_strip_empty->setEnabled(true);
    ui->m_check_strip_empty->setChecked(cb_perf().strip_empty());
    connect
    (
        ui->m_check_strip_empty, SIGNAL(stateChanged(int)),
        this, SLOT(slot_strip_empty())
    );
    ui->m_check_from_mutes->setEnabled(true);
    ui->m_check_from_mutes->setChecked
    (
        cb_perf().group_load_from_mutes()
    );
    connect
    (
        ui->m_check_from_mutes, SIGNAL(stateChanged(int)),
        this, SLOT(slot_load_mutes())
    );
    ui->m_check_from_midi->setEnabled(true);
    ui->m_check_from_midi->setChecked
    (
        cb_perf().group_load_from_midi()
    );
    connect
    (
        ui->m_check_from_midi, SIGNAL(stateChanged(int)),
        this, SLOT(slot_load_midi())
    );
    ui->m_check_toggle_active->setEnabled(true);
    ui->m_check_toggle_active->setChecked
    (
        cb_perf().toggle_active_only()
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

    handle_group_button(0, 0);          /* guaranteed to be present         */
    handle_group_change(0);             /* select the first group           */
    cb_perf().enregister(this);         /* register this for notifications  */
    group_needs_update();               /* guarantee the initial load       */
    m_is_initialized = true;
    m_timer = qt_timer(this, "qmutemaster", 3, SLOT(conditional_update()));
}

qmutemaster::~qmutemaster()
{
    if (not_nullptr(m_timer))
        m_timer->stop();

    cb_perf().unregister(this);         /* unregister this immediately      */
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

#if defined USE_REMOVED_MUTEMASTER_BUTTONS

void
qmutemaster::slot_pattern_offset (int index)
{
    m_pattern_offset = index * cb_perf().screenset_size();
}

#endif

void
qmutemaster::slot_clear_all_mutes ()
{
    if (cb_perf().clear_mutes())
    {
        modify_mutes();
        update_group_buttons();         /* see conditional_update() */
        update_pattern_buttons();       /* ditto                    */
    }
}

void
qmutemaster::slot_fill_mutes ()
{
    if (cb_perf().mutegroup_reset())
    {
        if (initialize_table())
        {
            modify_mutes();
            group_needs_update();
        }
    }
}

/**
 *  Done only at construction.
 */

void
qmutemaster::clear_pattern_mutes ()
{
    int count = cb_perf().mutegroup_count();                /* always 32    */
    m_pattern_mutes.clear();                                /* midibooleans */
    if (count > 0)
        m_pattern_mutes.resize(count);

    m_pattern_offset = 0;
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

/**
 *  Currently, the only cell that can be changed is the "Group Name".
 *  This can modify the '.mutes' file, the MIDI file, or both.
 */

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
        if (cb_perf().group_name(m, name))
            modify_mutes();
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
    ui->m_group_table->setColumnWidth(2, int(0.150f * total_width));
    ui->m_group_table->setColumnWidth(3, int(0.55f * total_width));
}

/**
 *  Updated to not disabled the last group button when the next group button
 *  is clicked, when in trigger mode.
 */

bool
qmutemaster::set_current_group (int group)
{
    bool result = group != current_group();
    if (result)
        result = group >= 0 && group < cb_perf().mutegroup_count();

    if (result)
    {
        int lastgroup = current_group();
        int gridrow, gridcolumn;
        if (mutegroups::group_to_grid(group, gridrow, gridcolumn))
        {
            m_button_row = gridrow;
            m_button_column = gridcolumn;
            m_current_group = group;

            QPushButton * temp = m_group_buttons[group];
            temp->setEnabled(true);
            temp->setChecked(false);
            if (lastgroup != seq::unassigned())
            {
                temp = m_group_buttons[lastgroup];
                if (! trigger())
                    temp->setEnabled(false);

                temp->setChecked(false);
            }
        }
    }
    return result;
}

bool
qmutemaster::initialize_table ()
{
    int rows = cb_perf().mutegroup_count();
    bool result = rows > 0;
    ui->m_group_table->clearContents();
    if (result)
    {
        for (int row = 0; row < rows; ++row)
        {
            mutegroup::number g = mutegroup::number(row);
            int mutecount = cb_perf().count_mutes(g);
            std::string keyname = cb_perf().lookup_mute_key(row);
            std::string groupname = cb_perf().group_name(row);
            (void) group_line(g, mutecount, keyname, groupname);
        }
        ui->m_group_table->clearSelection();
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
        result = new (std::nothrow) QTableWidgetItem;
        if (not_nullptr(result))
        {
            ui->m_group_table->setItem(row, column, result);
            if (col != column_id::group_name)
                result->setFlags(result->flags() ^ Qt::ItemIsEditable);
        }
    }
    return result;
}

/**
 *  Obtains the group number (row), number of active (arming) items in the
 *  mute-group, and the keystroke used to access that mute-group. These are
 *  then set into the cells of the corresponding column.
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
    int rows = cb_perf().mutegroup_count();                 /* always 32    */
    if (rows > 0)
    {
        if (row >= 0 && row < rows)
        {
            if (set_current_group(row))
            {
                ui->m_button_trigger->setEnabled(true);
                ui->m_button_set_mutes->setEnabled(false);

#if defined USE_REMOVED_MUTEMASTER_BUTTONS
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
    cb_perf().unregister(this);            /* unregister this immediately   */
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
 *
 *  Done at construction time.
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

#if defined USE_MUTES_FILE_TEXTEDIT

void
qmutemaster::slot_mutes_file_modify ()
{
    modify_mutes_file(true);
}

#endif

void
qmutemaster::slot_bin_mode (bool ischecked)
{
    cb_perf().group_format_hex(! ischecked);
    set_bin_hex(ischecked);
    modify_mutes_file(true);
}

void
qmutemaster::slot_hex_mode (bool ischecked)
{
    cb_perf().group_format_hex(ischecked);
    set_bin_hex(! ischecked);
    modify_mutes_file(true);
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

#if defined USE_GROUP_UPDATE_BUTTON

/**
 *  The calls to set mutes:  Fills midibooleans bit and calls performer ::
 *  set_mutes(), with a parameter of true so that the mutes are also copied to
 *  rcsettings for when the user of the Mutes tab wants to save the file.
 */

void
qmutemaster::slot_set_mutes ()
{
    midibooleans bits = m_pattern_mutes;
    bool ok = cb_perf().set_mutes(current_group(), bits, true);
    if (ok)
    {
        ui->m_button_save->setEnabled(true);
        ui->m_button_set_mutes->setEnabled(false);
    }
    else
    {
        // TODO show the error
    }
}

#endif

#if defined USE_REMOVED_MUTEMASTER_BUTTONS

/**
 *  A slot for handle_group_change(), meant to move a table row down.
 *  Not yet ready for primetime.
 *
 *  Actually this concept makes no sense.
 */

void
qmutemaster::slot_down ()
{
    if (set_current_group(current_group() + 1))
        handle_group_change(current_group());
}

/**
 *  A slot for handle_group_change(), meant to move a table row up.
 *  Not yet ready for primetime.
 */

void
qmutemaster::slot_up ()
{
    if (set_current_group(current_group() - 1))
        handle_group_change(current_group());
}

#endif

/**
 *  This looks goofy, but we offload the dialog handling to qsmainwnd, which
 *  has the boolean function qsmainwnd::open_mutes_dialog(), which returns
 *  true if the user clicked OK and the call to qmutemaster::load_mutegroups()
 *  succeeded.  Circular!
 *
 *  ca 2023-10-01 TODO: copy to the m_mute_basename QPlainTextEdit
 *  and also update the .mutes name in Edit / Preferences / Session.
 */

void
qmutemaster::slot_load ()
{
    if (not_nullptr(m_main_window))
    {
        if (m_main_window->open_mutes_dialog()) /* calls load_mutegroups()  */
        {
            /*
             * Some of this also done in constructor.
             */

//          std::string mutename = file_basename(rc().mute_group_filename());
            QString mgfname = qt(rc().mute_group_filename());
            bool mutesactive = rc().mute_group_file_active();
            bool groupload = cb_perf().group_load_from_mutes();
            ui->m_mute_basename->setPlainText(mgfname);
            ui->m_check_mutes_active->setChecked(mutesactive);
            ui->m_check_from_mutes->setChecked(groupload);
            unmodify_mutes();
        }
    }
}

/**
 *  Called in qsmainwnd. See above.
 */

bool
qmutemaster::load_mutegroups (const std::string & mutefile)
{
    bool result = cb_perf().open_mutegroups(mutefile);
    if (result)
        file_message("Opened mute-groups", mutefile);
    else
        file_error("Opened failed", mutefile);

    return result;
}

/**
 *  Saves to the specified 'mutes' file, even if not checked or not
 *  active. Note that ui->m_mute_basename is read-only.
 */

void
qmutemaster::slot_save ()
{
    if (not_nullptr(m_main_window))
    {
        std::string fname = rc().mute_group_filename();
        if (fname.empty())
        {
            /*
             * Should we modify rc?
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
        }
        if (m_main_window->save_mutes_dialog(rc().mute_group_filespec()))
        {
            /*
             * fname is the previous name; the function above saves
             * the new name.
             *
             * rc().mute_group_filename(fname);
             */

            modify_mutes_file(false);
            if (fname != rc().mute_group_filespec())
                modify_rc();

            QString mgfname = qt(rc().mute_group_filename());
            ui->m_mute_basename->setPlainText(mgfname);
        }
    }
}

/**
 *  Currently not in use!  See slot_save() instead.
 */

bool
qmutemaster::save_mutegroups (const std::string & mutefile)
{
    bool result = cb_perf().save_mutegroups(mutefile);
    if (result)
    {
        file_message("Wrote mute-groups", mutefile);
        unmodify_mutes();           /* ui->m_button_save->setEnabled(false) */
    }
    else
        file_message("Write failed", mutefile);

    return result;
}

/**
 *  This function handles a change in the "To MIDI" check-box.
 */

void
qmutemaster::slot_write_to_midi ()
{
    bool midichecked = ui->m_check_to_midi->isChecked();
    bool muteschecked = ui->m_check_to_mutes->isChecked();
    m_to_midi_active = midichecked;
    m_to_mutes_active = muteschecked;
    if (cb_perf().group_save(midichecked, muteschecked))
        modify_mutes_file(true);
}

/**
 *  This function handles a change in the "To Mutes" check-box.
 *
 *  Do we want to set this here? The user should use the Session tab in
 *  Preferences to set this, IMHO.
 *
 *      bool mgfactive = rc().mute_group_file_active();
 */

void
qmutemaster::slot_write_to_mutes ()
{
    bool midichecked = ui->m_check_to_midi->isChecked();
    bool muteschecked = ui->m_check_to_mutes->isChecked();
    m_to_midi_active = midichecked;
    m_to_mutes_active = muteschecked;
    if (cb_perf().group_save(midichecked, muteschecked))
        modify_mutes_file(true);
}

void
qmutemaster::slot_strip_empty ()
{
    bool ischecked = ui->m_check_strip_empty->isChecked();
    if (cb_perf().strip_empty(ischecked))
        modify_mutes_file(true);
}

/**
 *  This function handles a change in the "From Mutes" check-box.
 *
 *  Do we want to set this here? The user should use the Session tab in
 *  Preferences to set this, IMHO.
 *
 *      bool mgfactive = rc().mute_group_file_active();
 */

void
qmutemaster::slot_load_mutes ()
{
    bool muteschecked = ui->m_check_from_mutes->isChecked();
    bool midichecked = ui->m_check_from_midi->isChecked();
    if (cb_perf().load_mute_groups(midichecked, muteschecked))
    {
        /*
         * Too much: modify_mutes() can also modify 'rc' and the song.
         */

        if (m_is_initialized)
        {
            ui->m_button_set_mutes->setText("*");
            ui->m_button_save->setEnabled(true);
        }
    }
}

/**
 *  This function handles a change in the "From MIDI" check-box.
 */

void
qmutemaster::slot_load_midi ()
{
    slot_load_mutes();
}

void
qmutemaster::slot_toggle_active ()
{
    bool ischecked = ui->m_check_toggle_active->isChecked();
    cb_perf().toggle_active_only(ischecked);
    modify_mutes_file(true);
}

/**
 *  This function handles one of the group-selection  buttons in the grid of
 *  "Mute-Groups" buttons.
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
qmutemaster::handle_group_change (int groupno)
{
    if (groupno != m_current_group)
    {
        set_current_group(groupno);
        update_group_buttons();
        update_pattern_buttons();
        if (cb_perf().mutegroup_count() > 0)
            ui->m_group_table->selectRow(0);
    }
}

/**
 *  Handles mute-group changes from other dialogs.  These changes involve only
 *  the adding/subtracting of patterns to an old or a new group.
 *
 *  Don't think we need to call modify_mutes() here....
 */

bool
qmutemaster::on_mutes_change (mutegroup::number group, performer::change mod)
{
    bool result = ! mutegroup::none(group);
    if (result)
    {
        bool ok = mod != performer::change::max;
        if (ok)
        {
            result = initialize_table();
            if (result)
            {
                update_group_buttons(enabling::enable);
                group_needs_update();
            }
        }
    }
    return result;
}

void
qmutemaster::reload_mute_groups ()
{
    bool ok = initialize_table();
    if (ok)
    {
        update_group_buttons(enabling::enable);
        group_needs_update();
    }
}

/*
 *  We must accept() the key-event, otherwise even key-events in the QLineEdit
 *  items are propagated to the parent, where they then get passed to the
 *  performer as if they were keyboards controls (such as a pattern-toggle
 *  hot-key).
 *
 *  Plus, here, we have no real purpose for the code, so we macro it out.
 *  What's up with that, Spanky?
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
        // no code
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
        handle_group_change(index);

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
 *
 *  Done at construction time.
 */

void
qmutemaster::create_pattern_buttons ()
{
    const QSize btnsize = QSize(c_button_size, c_button_size);
    int tracks = cb_perf().screenset_size();
    for (int t = 0; t < tracks; ++t)
    {
        /*
         * Issue #87.  This is the wrong mapper, it is a set mapper, not
         * a seqence mapper:
         *
         *  if (cb_perf().master_index_to_grid(t, row, column))
         */

        int row, column;
        if (cb_perf().seq_to_grid(t, row, column))
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
 */

void
qmutemaster::update_pattern_buttons (enabling tomodify)
{
    midibooleans mutes = cb_perf().get_mutes(current_group());
    if (! mutes.empty())
    {
        m_pattern_mutes = mutes;
        int tracks = cb_perf().screenset_size();
        if (size_t(tracks) > m_pattern_buttons.size())
        {
            file_message("too many tracks to mute", std::to_string(tracks));
            tracks = int(m_pattern_buttons.size());
        }
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
 *  buttons. All it does is change the status of the appropriate bit in the
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
#if defined USE_GROUP_UPDATE_BUTTON
        ui->m_button_set_mutes->setEnabled(true);
        modify_mutes_file(true);
#else
        midibooleans bits = m_pattern_mutes;
        bool ok = cb_perf().set_mutes(current_group(), bits, true);
        if (ok)
        {
            ui->m_button_save->setEnabled(true);
            modify_mutes_file(true);
        }
#endif
    }
}

/**
 *  For MIDI save only.
 */

void
qmutemaster::modify_midi ()
{
    cb_perf().modify();
    if (not_nullptr(m_main_window))
        m_main_window->enable_save(true);
}

/**
 *  Also see mute_file_change().
 *
 *  This function applies to.... Not sure yet, since we no longer allow
 *  the base 'mutes' file-name to be changed here.
 */

void
qmutemaster::modify_rc ()
{
    if (m_is_initialized)
    {
        rc().auto_rc_save(true);
        rc().modify();
    }
}

/**
 *  Also see modify_mutes_file().
 *
 *  This function applies to changes that affect mutes independent of a
 *  'mutes' file.
 */

void
qmutemaster::modify_mutes ()
{
    group_needs_update();
    if (m_is_initialized)
    {
        ui->m_button_set_mutes->setText("*");
        ui->m_button_save->setEnabled(true);
        if (m_to_mutes_active)
            rc().auto_mutes_save(true);

        if (m_to_midi_active)
            modify_midi();
    }
}

/**
 *  Can't unmodify the tune or the 'mutes' file here.
 */

void
qmutemaster::unmodify_mutes ()
{
    group_needs_update();
    ui->m_button_set_mutes->setText("-");
    ui->m_button_save->setEnabled(false);
    rc().auto_mutes_save(false);
}

/**
 *  Calls this whenever a mutes file is saved or loaded, no matter if the
 *  "To Mutes" option is active or not.
 *
 * \param flag
 *      If true, the 'mutes' file has been loaded, and we have to assume
 *      that they have been modified.  If false, the 'mutes' file has been
 *      saved.
 */

void
qmutemaster::modify_mutes_file (bool flag)
{
    if (flag)
    {
        modify_mutes();
        rc().auto_mutes_save(true);
    }
    else
    {
        unmodify_mutes();
        rc().auto_mutes_save(false);
    }
}

}               // namespace seq66

/*
 * qmutemaster.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

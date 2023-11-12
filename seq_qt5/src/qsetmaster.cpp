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
 * \file          qsetmaster.cpp
 *
 *  This module declares/defines the base class for the set-master tab.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2023-11-11
 * \license       GNU GPLv2 or above
 *
 *  The set-master controls the existence and usage of all sets.  For control
 *  of the play-screen (playing set), see the setmapper class.
 */

#include <QKeyEvent>                    /* Needed for QKeyEvent::accept()   */
#include <QPushButton>
#include <QTableWidgetItem>
#include <QTimer>

#include "seq66-config.h"               /* defines SEQ66_QMAKE_RULES        */
#include "ctrl/keystroke.hpp"           /* seq66::keystroke class           */
#include "util/strfunctions.hpp"        /* seq66::string_to_int()           */
#include "qsetmaster.hpp"               /* seq66::qsetmaster tab class      */
#include "qsmainwnd.hpp"                /* seq66::qsmainwnd main window     */
#include "qt5_helpers.hpp"              /* seq66::qt_keystroke() etc.       */

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#if defined SEQ66_QMAKE_RULES
#include "forms/ui_qsetmaster.h"
#else
#include "forms/qsetmaster.ui.h"
#endif

/**
 *  For correcting the width of the set table.  Otherwise clicking on the
 *  set-name obscures the first column. Weird.
 */

static const int c_set_table_fix    = 24;   // 0; // -12;

/**
 *  Specifies the current hardwired value for set_row_heights().
 */

static const int c_table_row_height = 18;

/*
 * Don't document the namespace.
 */

namespace seq66
{

/**
 *  Principal constructor.
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

qsetmaster::qsetmaster
(
    performer & p,                  /* bool embedded, */
    qsmainwnd * mainparent,
    QWidget * parent
) :
    QFrame                  (parent),
    performer::callbacks    (p),
    ui                      (new Ui::qsetmaster),
    m_operations            ("Set Master Operations"),
    m_timer                 (nullptr),
    m_main_window           (mainparent),
    m_set_buttons           (setmaster::Size(), nullptr),
    m_current_set           (seq::unassigned()),
    m_current_row           (seq::unassigned()),
    m_current_row_count     (cb_perf().screenset_count()),
    m_needs_update          (true),
    m_trigger_mode          (false),
    m_table_initializing    (true)
    /*
     * m_is_permanent       (embedded)
     */
{
    ui->setupUi(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect
    (
        ui->m_set_name_text, SIGNAL(textEdited(const QString &)),
        this, SLOT(slot_set_name())
    );
    ui->m_button_down->setEnabled(false);
    ui->m_button_up->setEnabled(false);
    ui->m_button_delete->setEnabled(false);

    /*
     * This button is no longer available.  The set-master is always available
     * in its tab.
     *
     * if (m_is_permanent)
     *     ui->m_button_close->hide();
     * else
     *     connect(ui->m_button_close, SIGNAL(clicked()), this, SLOT(close()));
     */

    connect(ui->m_button_show, SIGNAL(clicked()), this, SLOT(slot_show_sets()));
    connect(ui->m_button_down, SIGNAL(clicked()), this, SLOT(slot_move_down()));
    connect(ui->m_button_up, SIGNAL(clicked()), this, SLOT(slot_move_up()));
    connect(ui->m_button_delete, SIGNAL(clicked()), this, SLOT(slot_delete()));
    create_set_buttons();

    setup_table();                      /* row and column sizing            */
    (void) initialize_table();          /* fill with sets                   */
    (void) populate_default_ops();      /* load key-automation support      */
    handle_set(0);                      /* guaranteed to be present         */
    cb_perf().enregister(this);         /* register this for notifications  */
    m_timer = qt_timer(this, "qsetmaster", 3, SLOT(conditional_update()));
}

qsetmaster::~qsetmaster()
{
    if (not_nullptr(m_timer))
        m_timer->stop();

    cb_perf().unregister(this);
    delete ui;
}

void
qsetmaster::conditional_update ()
{
    if (needs_update())                 /*  perf().needs_update() too iffy  */
    {
        for (int s = 0; s < setmaster::Size(); ++s) /* s is the set number  */
        {
            bool enabled = cb_perf().is_screenset_available(s);
            bool checked = s == m_current_set;      /* s'set::unassigned()  */
            m_set_buttons[s]->setEnabled(enabled);
            m_set_buttons[s]->setChecked(checked);
        }
        update();
        m_needs_update = false;
    }
}

void
qsetmaster::setup_table ()
{
    QStringList columns;
    columns << "Set #" << "Seqs" << "Set Name";

    /*
     * This advice is bs. The actual answer is to set the horizontal header to
     * be visible.
     *
     *  ui->m_set_table->insertColumn(0), and 1 and 2;;
     */

    ui->m_set_table->setHorizontalHeaderLabels(columns);
    ui->m_set_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    /*
     * ui->m_set_table->setSelectionMode(QAbstractItemView::SingleSelection);
     */

    const int rows = ui->m_set_table->rowCount();
    for (int r = 0; r < rows; ++r)
        ui->m_set_table->setRowHeight(r, c_table_row_height);

    int w = ui->m_set_table->width();
    set_column_widths(w + c_set_table_fix);

    connect
    (
        ui->m_set_table, SIGNAL(currentCellChanged(int, int, int, int)),
        this, SLOT(slot_table_click_ex(int, int, int, int))
    );
    connect
    (
        ui->m_set_table, SIGNAL(cellChanged(int, int)),
        this, SLOT(slot_cell_changed(int, int))
    );
    ui->m_button_toggle_trigger_mode->setCheckable(true);
    ui->m_button_toggle_trigger_mode->setChecked(false);
    connect
    (
        ui->m_button_toggle_trigger_mode, SIGNAL(clicked()),
        this, SLOT(slot_toggle_trigger_mode())
    );
    ui->m_button_set_0->setEnabled(false);
    connect
    (
        ui->m_button_set_0, SIGNAL(clicked()),
        this, SLOT(slot_set_0())
    );
}

/**
 *  Scales the columns against the provided window width. The width factors
 *  should add up to 1.
 */

void
qsetmaster::set_column_widths (int total_width)
{
    ui->m_set_table->setColumnWidth(0, int(0.15f * total_width));
    ui->m_set_table->setColumnWidth(1, int(0.15f * total_width));
    ui->m_set_table->setColumnWidth(2, int(0.70f * total_width));
}

void
qsetmaster::current_row (int row)
{
    m_current_row = row;
}

/**
 *  Do we want to skip non-existent sets?
 */

bool
qsetmaster::initialize_table ()
{
    bool result = false;
    m_table_initializing = true;
    int rows = setmaster::Size();   /* cb_perf().screenset_count()  */
    if (rows > 0)
    {
        ui->m_set_table->clearContents();
        for (int r = 0; r < rows; ++r)
            ui->m_set_table->setRowHeight(r, c_table_row_height);

        int active = cb_perf().screenset_count();
        if (active > 0)
        {
            screenset::sethandler setfunc =
            (
                std::bind
                (
                    &qsetmaster::set_line, this,
                    std::placeholders::_1, std::placeholders::_2
                )
            );
            (void) cb_perf().exec_set_function(setfunc);
        }
    }
    m_table_initializing = false;
    return result;
}

/**
 *  Retrieve the table cell at the given row and column.
 *
 *  Issue:
 *
 *      When this is first called [via initialize_table()], the
 *      QWidgetTableItem does not exist.  So we make a new one, which has no
 *      text in it, and insert it.  This then sends the cellChanged(int, int)
 *      signal, which calls slot_cell_changed(), which sets the set's name to
 *      empty!  This issue must also be checked/fixed in qplaylistframe ::
 *      cell(), qseqeventframe :: cell(), and qmutemaster :: cell().
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
qsetmaster::cell (screenset::number row, column_id col)
{
    int column = int(col);
    QTableWidgetItem * result = ui->m_set_table->item(row, column);
    if (is_nullptr(result))
    {
        result = new (std::nothrow) QTableWidgetItem;
        if (not_nullptr(result))
        {
            ui->m_set_table->setItem(row, column, result);
            if (col != column_id::set_name)
                result->setFlags(result->flags() ^ Qt::ItemIsEditable);
        }
    }
    return result;
}

bool
qsetmaster::set_line (screenset & sset, screenset::number row)
{
    bool result = false;
    QTableWidgetItem * qtip = cell(row, column_id::set_number);
    if (not_nullptr(qtip))
    {
        int setno = int(row);                   // int(sset.set_number());
        std::string setnostr = std::to_string(setno);
        qtip->setText(qt(setnostr));
        qtip = cell(row, column_id::set_name);  // clears sset.name()!
        if (not_nullptr(qtip))
        {
            const std::string & setname = sset.name();
            qtip->setText(qt(setname));
            qtip = cell(row, column_id::set_seq_count);
            if (not_nullptr(qtip))
            {
                int seqcount = int(sset.active_count());
                std::string seqcountstr = std::to_string(seqcount);
                qtip->setText(qt(seqcountstr));
                result = true;
            }
        }
    }
    return result;
}

/**
 *  Indicates that the MIDI file needs to be updated (saved) and
 *  the GUI needs to be refreshed.
 */

void
qsetmaster::set_needs_update ()
{
    m_needs_update = true;
    m_main_window->enable_save();
}

/**
 *  The Delete button is always disabled for row 0.  The 0th set must always
 *  exist.
 */

void
qsetmaster::slot_table_click_ex
(
    int row, int /*column*/, int /*prevrow*/, int /*prevcolumn*/
)
{
    int rows = cb_perf().screenset_count();
    if (rows > 0 && row >= 0 && row < rows)
    {
        current_row(row);
        ui->m_button_down->setEnabled(true);
        ui->m_button_up->setEnabled(true);
        ui->m_button_delete->setEnabled(row > 0);
        ui->m_set_number_text->setText(qt(std::to_string(row)));
        ui->m_set_name_text->setText(qt(cb_perf().set_name(row)));
    }
}

void
qsetmaster::slot_cell_changed (int row, int column)
{
    if (! m_table_initializing)
    {
        column_id cid = static_cast<column_id>(column);
        if (cid == column_id::set_name)
        {
            screenset::number s = screenset::number(row);
            QTableWidgetItem * c = cell(s, cid);
            QString qtext = c->text();
            std::string name = qtext.toStdString();
            std::string oldname = cb_perf().set_name(s);
            if (name != oldname)
            {
                cb_perf().screenset_name(s, name);
                set_needs_update();
            }
        }
    }
}

void
qsetmaster::slot_toggle_trigger_mode ()
{
    m_trigger_mode = ! m_trigger_mode;
    ui->m_button_set_0->setEnabled(m_trigger_mode);
}

void
qsetmaster::slot_set_0 ()
{
    if (m_trigger_mode)
        m_main_window->slot_set_home();
}

void
qsetmaster::closeEvent (QCloseEvent * event)
{
    cb_perf().unregister(this);            /* unregister this immediately      */
    if (not_nullptr(m_main_window))
        m_main_window->remove_set_master();

    event->accept();
}

/**
 *  Creates a grid of buttons in the grid layout.  This grid is always
 *  4 x 8, as discussed in the setmapper::grid_to_set() function, but if a
 *  smaller set number (count) is used, some buttons will be unlabelled and
 *  disabled.
 *
 *  Note that the largest number of sets is 4 x 8 = 32.  This limitation is
 *  necessary because there are only so many available keys on the keyboard for
 *  pattern, mute-group, and set control.
 *
 *  A set button is valid if the set number falls within the true set count,
 *  which might be less than 32 (e.g. it will be 16 sets with an 8x8 grid).
 *  But for now we will allow all buttons to remain enabled.  A set button is
 *  enabled if the set has at least one sequence in it.
 */

void
qsetmaster::create_set_buttons ()
{
    const QSize btnsize = QSize(32, 32);
    for (int s = 0; s < setmaster::Size(); ++s)             /* s is set #   */
    {
        bool enabled = cb_perf().is_screenset_available(s);
        int row, column;
        bool valid = cb_perf().master_index_to_grid(s, row, column);
        if (valid)
        {
            std::string snstring = std::to_string(s);
            QPushButton * temp = new QPushButton(qt(snstring));
            ui->setGridLayout->addWidget(temp, row, column);
            temp->setFixedSize(btnsize);
            temp->show();
            temp->setEnabled(enabled);
            temp->setCheckable(true);
            connect(temp, &QPushButton::released, [=] { handle_set(s); });
            m_set_buttons[s] = temp;
        }
    }
}

void
qsetmaster::handle_set (int setno)
{
    if (setno != m_current_set)
    {
        cb_perf().set_playing_screenset(setno);
        ui->m_set_number_text->setText(qt(std::to_string(setno)));
        ui->m_set_name_text->setText(qt(cb_perf().set_name(setno)));
        m_current_set = setno;

        /*
         *  Highlight the current set in the list.  Find the row based on set
         *  number. Also show its patterns.
         */

        ui->m_set_table->selectRow(cb_perf().screenset_index(setno));
        ui->m_set_contents_text->setPlainText
        (
            qt(cb_perf().set_to_string(setno))
        );

        /*
         *  If trigger-mode is active, then update the playscreen to that
         *  screenset.
         */

        if (m_trigger_mode)
            m_main_window->update_bank(setno);

        set_needs_update();
    }
}

void
qsetmaster::slot_set_name ()
{
    if (m_current_set != screenset::unassigned())
    {
        std::string name = ui->m_set_name_text->text().toStdString();
        cb_perf().screenset_name(m_current_set, name);
        (void) initialize_table();      /* refill with sets */
    }
}

void
qsetmaster::slot_show_sets ()
{
    ui->m_set_contents_text->setPlainText(qt(cb_perf().sets_to_string()));
}

/**
 *  If this is slow, we will try something else. It is only 32 rows right now.
 *
 *  int rowcount = ui->m_set_table->rowCount() > 0;
 */

void
qsetmaster::slot_move_down ()
{
    int rows = cb_perf().screenset_count();
    if (rows > 1)                                   /* cannot move if 1 row */
    {
        int row = current_row();                    /* last row clicked     */
        if (row >= 0 && row < (rows - 1))           /* moveable down?       */
            move_helper(row, row + 1);
    }
}

void
qsetmaster::slot_move_up ()
{
    int rows = cb_perf().screenset_count();
    if (rows > 1)                                   /* cannot move if 1 row */
    {
        int row = current_row();                    /* last row clicked     */
        if (row > 0 && row < rows)                  /* moveable up?         */
            move_helper(row, row - 1);
    }
}

/**
 *  Provides common code for slot_move_down() and slot_move_up().  Note that
 *  there is a trick here.  We cannot swap by set number, but by key value.
 */

void
qsetmaster::move_helper (int oldrow, int newrow)
{
    QTableWidgetItem * c0 = cell(oldrow, column_id::set_number);
    if (not_nullptr(c0))
    {
        std::string snstring = c0->text().toStdString();
        int set0 = string_to_int(snstring);
        QTableWidgetItem * c1 = cell(newrow, column_id::set_number);
        if (not_nullptr(c1))
        {
            std::string snstring = c1->text().toStdString();
            int set1 = string_to_int(snstring);
            if (cb_perf().swap_sets(set0, set1))    /* a modify action      */
            {
                (void) initialize_table();          /* refill with sets     */
                ui->m_set_table->selectRow(newrow);
                set_needs_update();
            }
        }
    }
}

/**
 *  Handles the "Delete" button.  We do not allow deleting of set 0.  This
 *  causes too many issues.
 *
 *  We're going to make this just clear the screenset, by replacing it with a
 *  new (empty) screenset.
 */

void
qsetmaster::slot_delete ()
{
    int rows = cb_perf().screenset_count();
    if (rows > 1)                                   /* cannot move if 1 row */
    {
        int row = current_row();                    /* last row clicked     */
        if (row >= 0 && row < rows)                 /* deleteable?          */
        {
            QTableWidgetItem * qtip = cell(row, column_id::set_number);
            if (not_nullptr(qtip))
            {
                std::string snstr = qtip->text().toStdString();
                int setno = string_to_int(snstr);
                if (setno > 0)
                {
                    /*
                     * if (cb_perf().remove_set(setno))
                     */

                    if (cb_perf().clear_set(setno))
                    {
                        if (setno == m_current_set)
                            m_current_set = seq::unassigned();

                        set_needs_update();
                    }
                }
            }
        }
    }
}

/**
 *  Handles set changes from other dialogs. We have a quandary: now
 *  we don't necessarily change the current row, even when "removing"
 *  (actually just clearing now) a screenset..
 */

bool
qsetmaster::on_set_change (screenset::number setno, performer::change modtype)
{
    int rows = cb_perf().screenset_count();
    bool result = m_current_set != setno || rows != m_current_row_count;
    if (result)
    {
        m_current_row_count = rows;
        if (modtype != performer::change::removed)
            m_current_set = setno;

        (void) initialize_table();      /* redraw the set-list (set-table)  */
        set_needs_update();             /* cause set-buttons to redraw      */
    }
    else
    {
        result = initialize_table();    /* redraw the set-list (set-table)  */
        set_needs_update();             /* cause set-buttons to redraw      */
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
qsetmaster::keyPressEvent (QKeyEvent * event)
{
#if defined PASS_KEYSTROKES_TO_PARENT
    keystroke kkey = qt_keystroke(event, keystroke::action::press);
    bool done = handle_key_press(kkey);
    if (done)
        set_needs_update();
    else
        QWidget::keyPressEvent(event);              /* event->ignore()      */
#else
    event->accept();
#endif
}

void
qsetmaster::keyReleaseEvent (QKeyEvent * event)
{
#if defined PASS_KEYSTROKES_TO_PARENT
    keystroke kkey = qt_keystroke(event, keystroke::action::release);
    bool done = handle_key_release(kkey);
    if (done)
        update();
    else
        QWidget::keyReleaseEvent(event);            /* event->ignore()      */
#else
    event->accept();
#endif
}

#if defined PASS_KEYSTROKES_TO_PARENT

bool
qsetmaster::handle_key_press (const keystroke & k)
{
    ctrlkey ordinal = k.key();
    const keycontrol & kc = cb_perf().key_controls().control(ordinal);
    bool result = kc.is_usable();
    if (result)
    {
        automation::slot s = kc.slot_number();
        const midioperation & mop = m_operations.operation(s);
        if (mop.is_usable())
        {
            /*
             * Note that the "inverse" parameter is based on key press versus
             * release.  Not all automation functions care about this setting.
             */

            automation::action a = kc.action_code();
            bool invert = ! k.is_press();
            int d0 = 0;
            int index = kc.control_code();
            result = mop.call(a, d0, index, invert);
        }
    }
    return result;
}

bool
qsetmaster::handle_key_release (const keystroke & k)
{
    bool done = cb_perf().midi_control_keystroke(k);
    if (! done)
    {
        // no useful code yet
    }
    return done;
}

#endif  // defined PASS_KEYSTROKES_TO_PARENT

/**
 *  This is not called when focus changes.
 */

void
qsetmaster::changeEvent (QEvent * event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange)
    {
        // no useful code yet
    }
}

bool
qsetmaster::set_control
(
    automation::action a, int /*d0*/, int /*d1*/,
    int index, bool inverse
)
{
    bool result = a == automation::action::toggle;
    if (result && ! inverse)
        handle_set(index);              /* index == set number */

    return result;
}

/**
 * Add a "loop" operation.  In this window, it will simply select the
 * active set.
 */

bool
qsetmaster::populate_default_ops ()
{
    midioperation patmop
    (
        opcontrol::category_name(automation::category::loop),   /* name     */
        automation::category::loop,                             /* category */
        automation::slot::loop,                                 /* opnumber */
        [this]
        (
            automation::action a, int d0, int d1,
            int index, bool inverse
        )
        {
            return set_control(a, d0, d1, index, inverse);
        }
    );
    return m_operations.add(patmop);
}

}               // namespace seq66

/*
 * qsetmaster.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

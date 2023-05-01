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
 * \file          qseqeventframe.cpp
 *
 *  This module declares/defines the base class for plastering
 *  pattern/sequence data information into a table widget for potential event
 *  editing.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-08-13
 * \updates       2023-05-01
 * \license       GNU GPLv2 or above
 *
 *  This class is the "Event Editor".
 */

#include <QKeyEvent>                    /* Needed for QKeyEvent::accept()   */

#include "cfg/settings.hpp"             /* SEQ66_QMAKE_RULES indirectly     */
#include "midi/controllers.hpp"         /* seq66::controller_name(), etc.   */
#include "play/sequence.hpp"            /* seq66::sequence                  */
#include "util/filefunctions.hpp"       /* seq66::filename_split()          */
#include "qseqeventframe.hpp"           /* seq66::qseqeventframe            */
#include "qt5_helpers.hpp"              /* seq66::qt() string conversion    */

#if defined SEQ66_QMAKE_RULES
#include "forms/ui_qseqeventframe.h"
#else
#include "forms/qseqeventframe.ui.h"
#endif

/*
 *  Do not document the name space.
 */

namespace seq66
{

/**
 *  For correcting the width of the event table.  It tries to account for the
 *  width of the vertical scroll-bar, plus a bit more.
 */

static const int sc_event_table_fix = 48;

/**
 *  Specifies the current hardwired value for set_row_heights().
 */

static const int sc_event_row_height = 18;

/**
 *
 * \param p
 *      Provides the performer object to use for interacting with this sequence.
 *      Among other things, this object provides the active PPQN.
 *
 * \param s
 *      Provides the reference to the sequence represented by this seqedit.
 *
 * \param parent
 *      Provides the parent window/widget for this container window.  Defaults
 *      to null.
 *
 */

qseqeventframe::qseqeventframe
(
    performer & p,
    sequence & s,
    QWidget * parent
) :
    QFrame                  (parent),
    performer::callbacks    (p),
    ui                      (new Ui::qseqeventframe),
    m_seq                   (s),
    m_eventslots            (new qseventslots(p, *this, s)),
    m_show_data_as_hex      (false),
    m_show_time_as_pulses   (false),
    m_is_dirty              (false)
{
    ui->setupUi(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    /*
     * Sequence Title.
     */

    connect
    (
        ui->m_entry_name, SIGNAL(textChanged(const QString &)),
        this, SLOT(update_seq_name())
    );
    set_seq_title(make_seq_title());

    QString seqnolabel = qt(std::to_string(int(track().seq_number())));
    ui->label_seq_number->setText(seqnolabel);

    midibyte seqchan = track().seq_midi_channel();
    std::string ts_ppqn = "Ch. ";
    ts_ppqn += is_null_channel(seqchan) ?
        "Free" : std::to_string(int(seqchan) + 1);

    ts_ppqn += ": ";
    ts_ppqn += std::to_string(track().get_beats_per_bar());
    ts_ppqn += "/";
    ts_ppqn += std::to_string(track().get_beat_width());
    ts_ppqn += " ";
    ts_ppqn += " PPQN ";
    ts_ppqn += std::to_string(track().get_ppqn());
    ts_ppqn += " ticks ";
    ts_ppqn += std::to_string(track().get_length());
    set_seq_time_sig_and_ppqn(ts_ppqn);

//  ui->label_channel->hide();              /* set_seq_channel(channelstr); */
    set_seq_channel("");
    set_seq_lengths(get_lengths());

    /*
     * Event Table and scroll area.  Some setup is already done in the
     * setupUi() function as configured via Qt Creator.
     *
     * "This" object is the parent of eventScrollArea.  The scroll-area
     * contains scrollAreaWidgetContents, which is the parent of
     * eventTableWidget.
     *
     * Note that setRowHeight() will need to be called for any new rows that
     * get added to the table.  However, there is no function for that!
     *
     * ui->eventTableWidget->setRowHeight(16);
     */

    QStringList columns;
    columns << "Time" << "Event" << "Ch" << "D 0" << "D 1" << "Link";
    ui->eventTableWidget->setHorizontalHeaderLabels(columns);
    ui->eventTableWidget->setSelectionBehavior
    (
        QAbstractItemView::SelectRows               /* SelectItems          */
    );
    set_selection_multi(false);                     /* SingleSelection      */
    set_row_heights(sc_event_row_height);
    set_column_widths(ui->eventTableWidget->width() - sc_event_table_fix);

    /*
     * Doesn't make the table read-only.  We want that for now, until we can
     * get time to modify events in-place.
     *
     * ui->eventTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
     */

    connect
    (
        ui->eventTableWidget, SIGNAL(currentCellChanged(int, int, int, int)),
        this, SLOT(slot_table_click_ex(int, int, int, int))
    );
    connect
    (
        ui->eventTableWidget, SIGNAL(clicked(const QModelIndex &)),
        this, SLOT(slot_row_selected())
    );

    /*
     * Channel selection and (new) Event selection.
     */

    populate_midich_combo();
    connect
    (
        ui->channel_combo_box, SIGNAL(currentIndexChanged(int)),
        this, SLOT(slot_midi_channel(int))
    );
    populate_status_combo();
    connect
    (
        ui->combo_ev_name, SIGNAL(currentIndexChanged(int)),
        this, SLOT(slot_event_name(int))
    );
    populate_category_combo();
    connect
    (
        ui->combo_ev_category, SIGNAL(currentIndexChanged(int)),
        this, SLOT(slot_event_category(int))
    );

    /*
     * Time and data format check-boxes.
     */

    connect
    (
        ui->hex_data_check_box, SIGNAL(stateChanged(int)),
        this, SLOT(slot_hex_data_state(int))
    );
    connect
    (
        ui->pulse_time_check_box, SIGNAL(stateChanged(int)),
        this, SLOT(slot_pulse_time_state(int))
    );

    /*
     *  Plain-text edit control for Meta messages involving text.
     */

    ui->plainTextEdit->document()->setPlainText("N/A");
    ui->plainTextEdit->setEnabled(false);
    connect
    (
        ui->plainTextEdit, SIGNAL(textChanged()),
        this, SLOT(slot_meta_text_change())
    );

    /*
     * Delete button.  Will set to enabled/disabled once fully initialized.
     */

    connect
    (
        ui->button_del, SIGNAL(clicked(bool)),
        this, SLOT(slot_delete())
    );
    ui->button_del->setEnabled(false);

    /*
     * Insert button.
     */

    connect
    (
        ui->button_ins, SIGNAL(clicked(bool)),
        this, SLOT(slot_insert())
    );
    ui->button_ins->setEnabled(true);

    /*
     * Modify button.
     */

    connect
    (
        ui->button_modify, SIGNAL(clicked(bool)),
        this, SLOT(slot_modify())
    );
    ui->button_modify->setEnabled(false);

    /*
     * Save button.
     */

    connect
    (
        ui->button_save, SIGNAL(clicked(bool)),
        this, SLOT(slot_save())
    );
    ui->button_save->setEnabled(false);

    /*
     * Clear button.
     */

    connect
    (
        ui->button_clear, SIGNAL(clicked(bool)),
        this, SLOT(slot_clear())
    );
    ui->button_clear->setEnabled(true);

    /*
     * Dump button.
     */

    connect
    (
        ui->button_dump, SIGNAL(clicked(bool)),
        this, SLOT(slot_dump())
    );
    ui->button_dump->setEnabled(true);

    /*
     * Load the data.
     */

    initialize_table();

    /*
     *  The event editor is now in a tab, and it is not quite as critical as
     *  the pattern editor.  The following setting causes the "File / New"
     *  operation to seem to mysteriously fail.
     *
     *      track().seq_in_edit(true);
     */

    track().set_dirty_mp();
    cb_perf().enregister(this);
}

qseqeventframe::~qseqeventframe()
{
    cb_perf().unregister(this);
    delete ui;
}

void
qseqeventframe::populate_midich_combo ()
{
    int defchannel = int(track().seq_midi_channel());
    if (is_null_channel(defchannel))
        defchannel = 0;

    ui->channel_combo_box->clear();
    for (int channel = 1; channel <= c_midichannel_max; ++channel)
    {
        std::string name = std::to_string(channel);
        QString combotext(qt(name));
        ui->channel_combo_box->insertItem(channel - 1, combotext);
    }
    ui->channel_combo_box->setCurrentIndex(defchannel);
}

void
qseqeventframe::populate_status_combo ()
{
    ui->combo_ev_name->clear();
    int counter = 0;
    for ( ; /* counter value */ ; ++counter)
    {
        std::string name = editable_event::channel_event_name(counter);
        if (name.empty())
        {
            break;
        }
        else
        {
            QString combotext(qt(name));
            ui->combo_ev_name->insertItem(counter, combotext);
        }
    }

#if defined USE_META_TEXT_EDITING
    ui->combo_ev_name->insertSeparator(counter);
#endif

    ui->combo_ev_name->setCurrentIndex(0);
}

void
qseqeventframe::populate_category_combo ()
{
    ui->combo_ev_category->clear();
    for (int counter = 0; /* counter value */ ; ++counter)
    {
        std::string name = editable_event::category_name(counter);
        if (name.empty())
        {
            break;
        }
        else
        {
            QString combotext(qt(name));
            ui->combo_ev_category->insertItem(counter, combotext);
        }
    }
}

void
qseqeventframe::set_selection_multi (bool multi)
{
    QAbstractItemView::SelectionMode sm = multi ?
        QAbstractItemView::MultiSelection : QAbstractItemView::SingleSelection ;

    ui->eventTableWidget->setSelectionMode(sm);
}

void
qseqeventframe::slot_midi_channel (int /*index*/)
{
    // Anything to do? We just need the text.
}

void
qseqeventframe::slot_event_name (int /*index*/)
{
    // Anything to do? We just need the text.
}

void
qseqeventframe::slot_event_category (int /*index*/)
{
    // Anything to do? We just need the text.
}

void
qseqeventframe::slot_hex_data_state (int state)
{
    bool is_true = state != Qt::Unchecked;
    m_show_data_as_hex = is_true;
    m_eventslots->hexadecimal(is_true);
    initialize_table();
}

void
qseqeventframe::slot_pulse_time_state (int state)
{
    bool is_true = state != Qt::Unchecked;
    m_show_time_as_pulses = is_true;
    m_eventslots->pulses(is_true);
    initialize_table();
}

/**
 *  Also gets the characters remaining after translation to encoded
 *  MIDI bytes.  Too slow?
 */

void
qseqeventframe::slot_meta_text_change ()
{
    QString qtex = ui->plainTextEdit->toPlainText();
    set_dirty();

#if 0
    std::string text = string_to_midi_bytes(qtex.toStdString());
    size_t remainder = c_meta_text_limit - text.size();
    std::string rem = int_to_string(int(remainder));
    ui->labelCharactersRemaining->setText(qt(rem));
#endif
}

/**
 *  Check for dirtiness (perhaps), clear the table and settings, and reload as
 *  if starting again.
 */

bool
qseqeventframe::on_sequence_change
(
    seq::number seqno,
    performer::change ctype
)
{
    bool result = seqno == track().seq_number();
    if (result)
    {
        if (m_is_dirty)
        {
            result = false;     /* ignore and pop up a warning dialog?  */
        }
        else
        {
            bool recreate = ctype == performer::change::yes;
            if (recreate)
                initialize_table();
        }
    }
    return result;
}

void
qseqeventframe::set_row_heights (int height)
{
    const int rows = ui->eventTableWidget->rowCount();
    for (int r = 0; r < rows; ++r)
        ui->eventTableWidget->setRowHeight(r, height);  /* set_row_height() */
}

/**
 *  Sets the height of each row in the table.
 */

void
qseqeventframe::set_row_height (int row, int height)
{
    ui->eventTableWidget->setRowHeight(row, height);
}

/**
 *  Scales the columns against the provided window width.
 */

void
qseqeventframe::set_column_widths (int total_width)
{
    static float s_w [6] = { 0.20f, 0.25f, 0.1f, 0.125f, 0.125f, 0.25f };
    ui->eventTableWidget->setColumnWidth(0, int(s_w[0] * total_width));
    ui->eventTableWidget->setColumnWidth(1, int(s_w[1] * total_width));
    ui->eventTableWidget->setColumnWidth(2, int(s_w[2] * total_width));
    ui->eventTableWidget->setColumnWidth(3, int(s_w[3] * total_width));
    ui->eventTableWidget->setColumnWidth(4, int(s_w[4] * total_width));
    ui->eventTableWidget->setColumnWidth(5, int(s_w[5] * total_width));
}

/**
 *  Clears, then refills the event table from the qseventslots object.
 */

bool
qseqeventframe::initialize_table ()
{
    static const int s_default_rows = 28;
    bool result = false;
    if (m_eventslots)
    {
        int rows = m_eventslots->event_count();
        if (rows > 0)
        {
            ui->eventTableWidget->clearContents();
            ui->eventTableWidget->setRowCount(rows);
            set_row_heights(sc_event_row_height);
            if (m_eventslots->load_table())
                m_eventslots->select_event(0);      /* first row */

            ui->button_clear->setEnabled(true);
        }
        else
        {
            ui->eventTableWidget->clearContents();
            ui->eventTableWidget->setRowCount(s_default_rows);
            set_row_heights(sc_event_row_height);
            ui->button_clear->setEnabled(false);
            ui->button_del->setEnabled(false);
            ui->button_modify->setEnabled(false);
        }
        ui->eventTableWidget->clearSelection();
    }
    return result;
}

std::string
qseqeventframe::make_seq_title ()
{
    return track().name();
}

/**
 *  Sets ui->m_entry_name to the title.
 *
 * \param title
 *      The name of the sequence.
 */

void
qseqeventframe::set_seq_title (const std::string & title)
{
    ui->m_entry_name->setText(qt(title));
}

/**
 *  Handles edits of the sequence title.
 */

void
qseqeventframe::update_seq_name ()
{
    std::string name = ui->m_entry_name->text().toStdString();
    if (cb_perf().set_sequence_name(track(), name))
        set_dirty();
}

/**
 *  Sets ui->label_time_sig to the time-signature string.  Also adds the
 *  parts-per-quarter-note string, and now also the length in ticks.
 *
 *  Combines the set_seq_time_sig() and set_seq_ppqn() from the old
 *  user-interface.
 *
 * \param sig
 *      The time signature of the sequence.
 */

void
qseqeventframe::set_seq_time_sig_and_ppqn (const std::string & sig)
{
    ui->label_time_sig->setText(qt(sig));
}

void
qseqeventframe::set_seq_channel (const std::string & /*ch*/)
{
//  ui->label_channel->setText(qt(ch));
    ui->channel_combo_box->setCurrentIndex(0);
}

/**
 *  Sets the number of measure and the number of events string.
 *
 *      m_eventslots->track().calculate_measures()
 *
 *  Combines set_seq_count() and set_length() into one function.
 */

void
qseqeventframe::set_seq_lengths (const std::string & mevents)
{
    ui->label_measures_ev_count->setText(qt(mevents));
}

/**
 *  Sets ui->label_category to the category string.
 *
 * \param c
 *      The category string for the current event.
 */

void
qseqeventframe::set_event_category (const std::string & c)
{
    QString category = qt(c);       /* ui->label_category->setText(qt(c))   */
    ui->combo_ev_category->setCurrentText(category);
}

/**
 *  Sets ui->entry_ev_timestamp to the time-stamp string.
 *
 * \param ts
 *      The time-stamp string for the current event.
 */

void
qseqeventframe::set_event_timestamp (const std::string & ts)
{
    ui->entry_ev_timestamp->setText(qt(ts));
}

/**
 *  Sets ui->combo_ev_name to the name-of-event string.  Oops, now
 *  (2021-04-11), we used it to select the entry in the editable combo-box for
 *  event-status names.
 *
 * \param n
 *      The name-of-event string for the current event.
 */

void
qseqeventframe::set_event_name (const std::string & n)
{
    QString name = qt(n);
    ui->combo_ev_name->setCurrentText(name);
}

void
qseqeventframe::set_event_channel (int channel)
{
    ui->channel_combo_box->setCurrentIndex(channel);
}

/**
 *  Sets ui->entry_ev_data_0 to the first data byte string.
 *
 * \param d
 *      The first data byte string for the current event.
 */

void
qseqeventframe::set_event_data_0 (const std::string & d)
{
    ui->entry_ev_data_0->setText(qt(d));
}

/**
 *  Sets ui->entry_data_1 to the second data byte string.
 *
 * \param d
 *      The second data byte string for the current event.
 */

void
qseqeventframe::set_event_data_1 (const std::string & d)
{
    ui->entry_ev_data_1->setText(qt(d));
}

void
qseqeventframe::set_event_plaintext (const std::string & t)
{
    QString text = qt(t);
    ui->plainTextEdit->document()->setPlainText(text);
}

/**
 *  Retrieve the table cell at the given row and column.
 *
 * \param row
 *      The row number, which should be in range.
 *
 * \param col
 *      The column enumeration value, which will be in range.
 *
 * \return
 *      Returns a pointer the table widget-item for the given row and column.
 *      If out-of-range, a null pointer is returned.
 */

QTableWidgetItem *
qseqeventframe::cell (int row, column_id col)
{
    int column = int(col);
    QTableWidgetItem * result = ui->eventTableWidget->item(row, column);
    if (is_nullptr(result))
    {
        /*
         * Will test row/column and maybe add rows on the fly later.
         */

        result = new QTableWidgetItem;
        ui->eventTableWidget->setItem(row, column, result);
    }
    return result;
}

void
qseqeventframe::set_event_line
(
    int row,
    const std::string & evtimestamp,
    const std::string & evname,
    const std::string & evchannel,
    const std::string & evdata0,
    const std::string & evdata1,
    const std::string & linktime
)
{
    QTableWidgetItem * qtip = cell(row, column_id::timestamp);
    if (not_nullptr(qtip))
    {
        qtip->setText(qt(evtimestamp));

        qtip = cell(row, column_id::eventname);
        qtip->setText(qt(evname));

        qtip = cell(row, column_id::channel);
        qtip->setText(qt(evchannel));

        qtip = cell(row, column_id::data_0);
        qtip->setText(qt(evdata0));

        qtip = cell(row, column_id::data_1);
        qtip->setText(qt(evdata1));

        qtip = cell(row, column_id::link);
        qtip->setText(qt(linktime));
    }
}

void
qseqeventframe::set_event_line (int row, const editable_event & ev)
{
    const editable_event & ev2 = m_eventslots->lookup_link(ev);
    std::string linktime = ev2.timestamp_string();
    std::string evtimestamp = m_eventslots->time_string(ev.timestamp());
    std::string evname = ev.status_string();
    std::string evchannel = ev.channel_string();
    midibyte d0, d1;
    ev.get_data(d0, d1);

    std::string evdata0 = m_eventslots->data_string(d0);
    std::string evdata1 = m_eventslots->data_string(d1);
    set_event_line
    (
        row,  evtimestamp, evname, evchannel, evdata0, evdata1, linktime
    );
}

void
qseqeventframe::set_event_line (int row)
{
    if (not_nullptr(m_eventslots))
    {
        const editable_event & ev = m_eventslots->current_event();
        set_event_line(row, ev);
    }
}

/**
 *  Sets the "modified" status of the user-interface.  This includes changing
 *  a label, enabling/disabling the Save button, and modifying the event count
 *  and sequence length (in measures).
 *
 * \param flag
 *      If true, the modified status is indicated, otherwise it is cleared.
 *      The default value is true.
 */

void
qseqeventframe::set_dirty (bool flag)
{
    if (flag)
    {
        ui->button_save->setEnabled(true);
        m_is_dirty = true;
    }
    else
    {
        ui->button_save->setEnabled(false);
        m_is_dirty = false;
    }
}

/**
 *  Needs to be defined in cpp file due to being an incomplete type in the
 *  header.
 */

int
qseqeventframe::current_row () const
{
    return m_eventslots->current_row();
}

/**
 *  Needs to be defined in cpp file due to being an incomplete type in the
 *  header.
 */

void
qseqeventframe::current_row (int row)
{
    m_eventslots->current_row(row);
}

void
qseqeventframe::slot_table_click_ex
(
    int row, int /*column*/, int /*prevrow*/, int /*prevcolumn*/
)
{
    if (row >= 0)
    {
        m_eventslots->select_event(row);
        current_row(row);
        ui->button_del->setEnabled(true);
        ui->button_modify->setEnabled(true);

        if (ui->eventTableWidget->selectionModel()->selectedRows().count() > 1)
            ui->eventTableWidget->clearSelection();
    }
}

void
qseqeventframe::slot_row_selected ()
{
    QModelIndex index = ui->eventTableWidget->currentIndex();
    int row = index.row();
    m_eventslots->select_event(row);        /* also done by click */

    const editable_event & ev0 = m_eventslots->current_event();
    if (ev0.is_linked())
    {
        editable_event & ev1 = m_eventslots->lookup_link(ev0);
        if (ev1.valid_status())
        {
            int row1 = m_eventslots->count_to_link(ev0);
            if (row1 >= 0)
            {
                set_selection_multi(true);
                ui->eventTableWidget->selectRow(row1);
            }
        }
    }
    else
    {
        set_selection_multi(false);
    }
}

std::string
qseqeventframe::get_lengths ()
{
    std::string meas_events = std::to_string(m_eventslots->calculate_measures());
    meas_events += " measures, ";
    meas_events += std::to_string(m_eventslots->event_count());
    meas_events += " events";
    return meas_events;
}

/**
 *  Initiates the deletion of the current editable event.  We call both of the
 *  following.  Though they seem redundant, the first call is needed to
 *  hightlight the row visually, and the second makes the actual selection.
 *
 *  -   ui->eventTableWidget->setCurrentIndex(next);
 *  -   ui->eventTableWidget->selectionModel()->select(next, ...);
 *
 *  These are alternatives we tried, FYI only:
 *
 *  -   QItemSelectionModel::ClearAndSelect|QItemSelectionModel::Rows
 *  -   ui->eventTableWidget->viewport().update();
 */

void
qseqeventframe::slot_delete ()
{
    if (m_eventslots)
    {
        editable_event & current = m_eventslots->current_event();
        int row0 = m_eventslots->current_row();
        int row1 = m_eventslots->count_to_link(current);
        bool islinked = row1 >= 0;
        if (islinked && row0 > row1)
            std::swap(row0, row1);
        else
            row1 = row0;

        if (islinked)
            m_eventslots->select_event(row1, false);        /* only/last row */

        bool was_removed = m_eventslots->delete_current_event();
        if (was_removed)
        {
            int cr = row1;
            ui->eventTableWidget->removeRow(row1);
            if (islinked)
            {
                m_eventslots->select_event(row0, false);    /* first row     */
                was_removed = m_eventslots->delete_current_event();
                if (was_removed)
                {
                    cr = row0 - 1;
                    ui->eventTableWidget->removeRow(row0);
                }
            }
            if (! m_eventslots->empty())
            {
                QModelIndex next = ui->eventTableWidget->model()->index(cr, 0);
                ui->eventTableWidget->setCurrentIndex(next);
                ui->eventTableWidget->selectionModel()->select
                (
                    next, QItemSelectionModel::Rows
                );
                m_eventslots->select_event(cr);
                current_row(cr);
            }
            set_dirty();
        }
        if (m_eventslots->empty())
            (void) initialize_table();

        set_seq_lengths(get_lengths());
    }
    set_selection_multi(false);
}

/**
 *  Initiates the insertion of a new editable event.  The event's location
 *  will be determined by the timestamp and existing events.  Note that we
 *  have to recalibrate the scroll-bar when we insert/delete events.
 *
 *  As a feature, we will allow events to extend the official length of the
 *  sequence.
 *
 *  We have to figure out where the new event goes, its new index into
 *  the container, and add the new table row in the corresponding place.
 */

void
qseqeventframe::slot_insert ()
{
    if (m_eventslots)
    {
        std::string ts = ui->entry_ev_timestamp->text().toStdString();
        std::string name = ui->combo_ev_name->currentText().toStdString();
        std::string d0 = ui->entry_ev_data_0->text().toStdString();
        std::string d1 = ui->entry_ev_data_1->text().toStdString();
        std::string ch = ui->channel_combo_box->currentText().toStdString();
        std::string linktime;                   /* empty, no link time yet  */
        bool has_events = m_eventslots->insert_event(ts, name, d0, d1, ch);
        set_seq_lengths(get_lengths());
        if (has_events)
        {
            std::string chan = m_eventslots->current_event().channel_string();
            int cr = m_eventslots->current_row();
            ui->eventTableWidget->insertRow(cr);
            set_row_height(cr, sc_event_row_height);
            set_event_line(cr, ts, name, chan, d0, d1, linktime);
            ui->button_del->setEnabled(true);
            ui->button_modify->setEnabled(true);
            set_dirty();
        }
    }
    set_selection_multi(false);
}

/**
 *  Passes the edited fields to the current editable event in the eventslot.
 *  Note that there are two cases to worry about.  If the timestamp has not
 *  changed, then we can simply modify the existing current event in place.
 *  Otherwise, we need to delete the old event and insert the new one.
 *  But that is done for us by eventslots::modify_current_event().
 */

void
qseqeventframe::slot_modify ()
{
    if (m_eventslots)
    {
        int row0 = current_row();
        const editable_event & ev0 = m_eventslots->current_event();
        std::string ts = ui->entry_ev_timestamp->text().toStdString();
        std::string name = ui->combo_ev_name->currentText().toStdString();
        std::string chan = ev0.channel_string();
        std::string d0 = ui->entry_ev_data_0->text().toStdString();
        std::string d1 = ui->entry_ev_data_1->text().toStdString();
        std::string ch = ui->channel_combo_box->currentText().toStdString();
        midipulse lt = c_null_midipulse;
        bool reload = false;                /* works, but why is it needed? */
        if (ev0.is_linked())
        {
            editable_event & ev1 = m_eventslots->lookup_link(ev0);
            if (ev1.valid_status())
            {
                int row1 = m_eventslots->count_to_link(ev0);
                if (row1 >= 0)
                {
                    lt = ev0.link_time();
                    m_eventslots->select_event(row1, false);
                    m_eventslots->modify_current_channel_event(row1, d0, d1, ch);
                    set_event_line(row1);
                }
                reload = true;              /* this is very krufty, mufti   */
            }
        }

        std::string ltstr = m_eventslots->time_string(lt);
        m_eventslots->select_event(row0, false);
        (void) m_eventslots->modify_current_event(row0, ts, name, d0, d1, ch);
        set_seq_lengths(get_lengths());
        set_event_line(row0, ts, name, ch, d0, d1, ltstr);
        if (reload)
            initialize_table();             /* this is very stilted, Milton */

        set_dirty();
    }
    set_selection_multi(false);
}

/**
 *  Handles saving the edited data back to the original sequence.
 *  The event list in the original sequence is cleared, and the editable
 *  events are converted to plain events, and added to the container, one by
 *  one.
 *
 *  Also tells performer to notify its subscribers.  This will cause
 *  qseqeventframe::on_sequence_change() to be called, so be careful of loops!
 *
 * \todo
 *      Could also support writing the events to a new sequence, for added
 *      flexibility.
 *
 * \todo
 *      Make sure that performer now calls the notification apparatus!
 */

void
qseqeventframe::slot_save ()
{
    if (m_eventslots)
    {
        bool ok = m_eventslots->save_events();
        if (ok)
        {
            seq::number seqno = track().seq_number();
            cb_perf().notify_sequence_change(seqno);
            ui->button_save->setEnabled(false);
            m_is_dirty = false;
        }
    }
    set_selection_multi(false);
}

void
qseqeventframe::slot_clear ()
{
    if (m_eventslots)
    {
        m_eventslots->clear();
        initialize_table();
        set_dirty();
    }
    set_selection_multi(false);
}

std::string
qseqeventframe::filename_prompt
(
    const std::string & prompt,
    const std::string & file
)
{
    std::string result = file;
    bool ok = show_file_dialog
    (
        this, result, prompt,
        "Text files (*.text *.txt);;All files (*)", SavingFile, NormalFile,
        ".text"
    );
    if (ok)
    {
        // nothing yet
    }
    else
        result.clear();

    return result;
}

void
qseqeventframe::slot_dump ()
{
    if (m_eventslots)
    {
        std::string dump = m_eventslots->events_to_string();
        if (! dump.empty())
        {
            std::string fspec = rc().midi_filename();
            std::string directory;
            std::string basename;
            bool ok = filename_split(fspec, directory, basename);
            if (ok)
            {
                basename = file_extension_set(basename);    /* strip .ext   */
                basename += "-pattern-";
                basename += std::to_string(track().seq_number());
                basename = file_extension_set(basename, ".text");
                fspec = filename_concatenate(directory, basename);

                /*
                 * Before writing, give the user a chance to save it
                 * elsewhere, or at least see where it will be saved.
                 */

                std::string prompt = "Dump events to text file";
                std::string filename = filename_prompt(prompt, fspec);
                if (filename.empty())
                {
                    // no code, the user cancelled
                }
                else
                {
                    if (! file_write_string(fspec, dump))
                        msgprintf(msglevel::status, "%s", dump.c_str());
                }
            }
            else
                msgprintf(msglevel::status, "%s", dump.c_str());
        }
    }
    set_selection_multi(false);
}

/**
 *  Cancels the edits and closes the dialog box.  In order for removing the
 *  current-highlighting in the mainwd or perfedit windows, some of the work
 *  of slot_close() needs to be done here as well.
 */

void
qseqeventframe::slot_cancel ()
{
    set_selection_multi(false);
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
qseqeventframe::keyPressEvent (QKeyEvent * event)
{
    event->accept();
}

void
qseqeventframe::keyReleaseEvent (QKeyEvent * event)
{
    event->accept();
}

}           // namespace seq66

/*
 * qseqeventframe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


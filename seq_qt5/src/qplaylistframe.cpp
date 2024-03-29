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
 * \file          qplaylistframe.cpp
 *
 *  This module declares/defines the base class for plastering
 *  pattern/sequence data information in the data area of the pattern
 *  editor.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-09-04
 * \updates       2023-09-07
 * \license       GNU GPLv2 or above
 *
 */

#include <QErrorMessage>                /* QErrorMessage                    */
#include <QKeyEvent>                    /* Needed for QKeyEvent::accept()   */
#include <QTimer>

#include "cfg/settings.hpp"             /* seq66::rc() and seq66::usr()     */
#include "play/performer.hpp"           /* seq66::performer                 */
#include "util/filefunctions.hpp"       /* seq66::filename_split()          */
#include "util/strfunctions.hpp"        /* seq66::string_to_int()           */
#include "qplaylistframe.hpp"           /* seq66::qplaylistframe child      */
#include "qsmainwnd.hpp"                /* seq66::qsmainwnd, a parent       */
#include "qt5_helpers.hpp"              /* seq66::qt_set_icon() etc.        */

#if defined SEQ66_QMAKE_RULES
#include "forms/ui_qplaylistframe.h"
#else
#include "forms/qplaylistframe.ui.h"
#endif

namespace seq66
{

/**
 *  For correcting the width of the play-list tables.  It tries to account for
 *  the width of the vertical scroll-bar, plus a bit more.
 */

static const int c_playlist_table_fix   = 24; // 48

/**
 *  Specifies the current hardwired value for set_row_heights().
 */

static const int c_playlist_row_height  = 18;

/**
 *  Principal constructor.
 */

qplaylistframe::qplaylistframe
(
    performer & p,
    qsmainwnd * window,
    QWidget * frameparent
) :
    QFrame                  (frameparent),
    ui                      (new Ui::qplaylistframe),
    m_timer                 (nullptr),
    m_performer             (p),
    m_parent                (window),
    m_current_list_index    (0),
    m_current_song_index    (0)
{
    ui->setupUi(this);

    QStringList playcolumns;
    playcolumns << "#" << "Playlist Names";
    ui->tablePlaylistSections->setHorizontalHeaderLabels(playcolumns);
    ui->tablePlaylistSections->setSelectionBehavior
    (
        QAbstractItemView::SelectRows   /* QAbstractItemView::SelectItems   */
    );
    ui->tablePlaylistSections->setSelectionMode
    (
        QAbstractItemView::SingleSelection
    );

    QStringList songcolumns;
    songcolumns << "#" << "Song Files in List";
    ui->tablePlaylistSongs->setHorizontalHeaderLabels(songcolumns);
    ui->tablePlaylistSongs->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tablePlaylistSongs->setSelectionMode
    (
        QAbstractItemView::SingleSelection
    );
    set_row_heights(c_playlist_row_height);
    set_column_widths();
    connect
    (
        ui->tablePlaylistSections, SIGNAL(currentCellChanged(int, int, int, int)),
        this, SLOT(slot_list_click_ex(int, int, int, int))
    );
    connect
    (
        ui->tablePlaylistSongs, SIGNAL(currentCellChanged(int, int, int, int)),
        this, SLOT(slot_song_click_ex(int, int, int, int))
    );
    connect
    (
        ui->buttonPlaylistCreate, SIGNAL(clicked(bool)),
        this, SLOT(slot_file_create_click())
    );
    connect
    (
        ui->buttonSelectSongDir, SIGNAL(clicked(bool)),
        this, SLOT(slot_list_dir_click())
    );
    connect
    (
        ui->buttonSelectPlaylist, SIGNAL(clicked(bool)),
        this, SLOT(slot_list_load_click())
    );
    connect
    (
        ui->buttonPlaylistAdd, SIGNAL(clicked(bool)),
        this, SLOT(slot_list_add_click())
    );
    connect
    (
        ui->buttonPlaylistModify, SIGNAL(clicked(bool)),
        this, SLOT(slot_list_modify_click())
    );
    connect
    (
        ui->buttonPlaylistRemove, SIGNAL(clicked(bool)),
        this, SLOT(slot_list_remove_click())
    );
    if (not_nullptr(parent()))
    {
        ui->buttonPlaylistSave->setText("Save Lists");
        if (parent()->use_nsm())
        {
            ui->buttonPlaylistSave->setToolTip
            (
                "Save playlists/MIDI files in NSM session."
            );
            ui->editPlaylistPath->setReadOnly(true);
            ui->editPlaylistPath->setEnabled(false);
            ui->editSongPath->setReadOnly(true);
            ui->editSongPath->setEnabled(false);
            ui->entry_playlist_file->setReadOnly(true);
            ui->entry_playlist_file->setEnabled(false);
        }
        else
        {
            ui->buttonPlaylistSave->setToolTip
            (
                "Save playlists (only) to current directory."
            );
            connect
            (
                ui->editPlaylistPath, SIGNAL(textEdited(QString)),
                this, SLOT(list_modify(QString))
            );
            connect
            (
                ui->editSongPath, SIGNAL(textEdited(QString)),
                this, SLOT(song_modify(QString))
            );
        }
    }
    connect
    (
        ui->buttonPlaylistSave, SIGNAL(clicked(bool)),
        this, SLOT(slot_list_save_click())
    );

#if defined USE_REDUNDANT_SONGLOAD_BUTTON
    connect
    (
        ui->buttonSongLoad, SIGNAL(clicked(bool)),
        this, SLOT(slot_song_load_click())
    );
#else
    ui->buttonSongLoad->setText(" ");
    ui->buttonSongLoad->setEnabled(false);
    ui->buttonSongLoad->hide();
#endif

    connect
    (
        ui->buttonSelectSong, SIGNAL(clicked(bool)),
        this, SLOT(slot_song_select_click())
    );
    connect
    (
        ui->buttonSongAdd, SIGNAL(clicked(bool)),
        this, SLOT(slot_song_add_click())
    );
    connect
    (
        ui->buttonSongModify, SIGNAL(clicked(bool)),
        this, SLOT(slot_song_modify_click())
    );
    connect
    (
        ui->buttonSongRemove, SIGNAL(clicked(bool)),
        this, SLOT(slot_song_remove_click())
    );
    connect
    (
        ui->checkBoxPlaylistActive, SIGNAL(clicked(bool)),
        this, SLOT(slot_playlist_active_click())
    );
    connect
    (
        ui->checkBoxAutoArm, SIGNAL(clicked(bool)),
        this, SLOT(slot_auto_arm_click())
    );
    connect
    (
        ui->checkBoxAutoPlay, SIGNAL(clicked(bool)),
        this, SLOT(slot_auto_play_click())
    );
    connect
    (
        ui->checkBoxAutoAdvance, SIGNAL(clicked(bool)),
        this, SLOT(slot_auto_advance_click())
    );
    connect
    (
        ui->entry_playlist_file, SIGNAL(textEdited(QString)),
        this, SLOT(list_modify(QString))
    );
#if defined USE_PLAYLIST_NUMBER_TEXTEDIT
    connect
    (
        ui->editPlaylistNumber, SIGNAL(textEdited(QString)),
        this, SLOT(list_modify(QString))
    );
#else
    ui->editPlaylistNumber->hide();
    set_spin_value(ui->spinPlaylistNumber, 0);
    connect
    (
        ui->spinPlaylistNumber, SIGNAL(valueChanged(int)),
        this, SLOT(list_modify(int))
    );
    connect
    (
        ui->spinPlaylistNumber, SIGNAL(editingFinished()),
        this, SLOT(list_modify())
    );
#endif
#if defined USE_SONG_NUMBER_TEXTEDIT
    connect
    (
        ui->editSongNumber, SIGNAL(textEdited(QString)),
        this, SLOT(song_modify(QString))
    );
#else
    ui->editSongNumber->hide();
    set_spin_value(ui->spinSongNumber, 0);
    connect
    (
        ui->spinSongNumber, SIGNAL(valueChanged(int)),
        this, SLOT(song_modify(int))
    );
    connect
    (
        ui->spinSongNumber, SIGNAL(editingFinished()),
        this, SLOT(song_modify())
    );
#endif

    /*
     * This field is now read-only, as is the base MIDI file name.
     * The new "load song" button (buttonSelectSong) must be used.
     */

#if defined SEQ66_ALLOW_FILENAME_EDIT
    connect
    (
        ui->editSongFilename, SIGNAL(textEdited(QString)),
        this, SLOT(song_modify(QString))
    );
#else
    ui->editSongFilename->setEnabled(false);    /* now stays disabled   */
#endif

    enable_midi_widgets(false);
    ui->midiBaseDirText->setReadOnly(true);
    ui->midiBaseDirText->setEnabled(false);
    ui->currentSongPath->setReadOnly(true);
    ui->currentSongPath->setEnabled(false);
    reset_playlist();                       /* if (perf().playlist_mode())  */
    m_timer = qt_timer(this, "qplaylistframe", 4, SLOT(conditional_update()));
}

/**
 *  Stops the timer and deletes the user interface.
 */

qplaylistframe::~qplaylistframe ()
{
    if (not_nullptr(m_timer))
        m_timer->stop();

    delete ui;
}

/**
 *  In an effort to reduce CPU usage when simply idling, this function calls
 *  update() only if necessary.  See qseqbase::needs_update().  We
 *  must check all sequences.
 */

void
qplaylistframe::conditional_update ()
{
    if (perf().needs_update())          /* potentially checks all sequences */
        update();
}

/**
 *  Sets the height of the rows in the list and song tables.
 */

void
qplaylistframe::set_row_heights (int height)
{
    const int prows = ui->tablePlaylistSections->rowCount();
    for (int pr = 0; pr < prows; ++pr)
        ui->tablePlaylistSections->setRowHeight(pr, height);

    const int rows = ui->tablePlaylistSongs->rowCount();
    for (int sr = 0; sr < rows; ++sr)
        ui->tablePlaylistSongs->setRowHeight(sr, height);
}

/**
 *  Scales the columns against the provided window width.
 */

void
qplaylistframe::set_column_widths ()
{
    int w = ui->tablePlaylistSections->width() - c_playlist_table_fix;
    ui->tablePlaylistSections->setColumnWidth(0, int(0.20f * w));
    ui->tablePlaylistSections->setColumnWidth(1, int(0.80f * w));

    w = ui->tablePlaylistSongs->width() - c_playlist_table_fix;
    ui->tablePlaylistSongs->setColumnWidth(0, int(0.20f * w));
    ui->tablePlaylistSongs->setColumnWidth(1, int(0.80f * w));
}

/**
 *  Enables/disables the MIDI-file buttons and edit controls to prevent
 *  trying to add a MIDI file when there is no playlist selected.
 */

void
qplaylistframe::enable_midi_widgets (bool enable)
{
    ui->editSongPath->setEnabled(enable);
#if defined USE_SONG_NUMBER_TEXTEDIT
    ui->editSongNumber->setEnabled(enable);
#else
    ui->spinSongNumber->setEnabled(enable);
#endif
    ui->buttonSelectSong->setEnabled(enable);
    ui->buttonSongLoad->setEnabled(enable);
}

/**
 *  Resets the play-list.  First, resets to the first (0th) play-list and the
 *  first (0th) song.  Then fills the play-list items and resets again.  Then
 *  fills in the play-list and song items for the current selection.
 *
 * \param listindex
 *      The play-list to reset to, defaulting to the 0th list.  However, when
 *      deleting a song, we want to reset to the current playlist.
 */

void
qplaylistframe::reset_playlist (int listindex)
{
    if (listindex >= 0)
    {
        bool usable = perf().playlist_reset();
        fill_playlists();
        fill_songs();
        if (listindex > 0)
            (void) perf().playlist_reset(listindex);

        set_current_playlist();
        ui->tablePlaylistSections->selectRow(listindex);
        ui->tablePlaylistSongs->selectRow(0);
        ui->buttonPlaylistRemove->setEnabled(usable);
        ui->buttonSongRemove->setEnabled(usable);
    }
    else
    {
        ui->buttonPlaylistRemove->setEnabled(false);
        ui->buttonSongRemove->setEnabled(false);
    }
}

void
qplaylistframe::reset_playlist_file_name ()
{
    QString fname = qt(rc().playlist_filespec());
}

void
qplaylistframe::set_current_playlist ()
{
    std::string temp;
    ui->checkBoxPlaylistActive->setChecked(perf().playlist_active());
    ui->checkBoxAutoArm->setChecked(perf().playlist_auto_arm());
    ui->checkBoxAutoPlay->setChecked(perf().playlist_auto_play());
    ui->checkBoxAutoAdvance->setChecked(perf().playlist_auto_advance());
    temp = perf().playlist_filename();
    ui->entry_playlist_file->setText(qt(temp));
    temp = perf().file_directory();
    ui->editPlaylistPath->setText(qt(temp));

    int midinumber = perf().playlist_midi_number();
    if (midinumber < 0)
        midinumber = 0;

#if defined USE_PLAYLIST_NUMBER_TEXTEDIT
    temp = std::to_string(midinumber);
    if (temp.empty())
        temp = "0";

    ui->editPlaylistNumber->setText(qt(temp));
#else
    set_spin_value(ui->spinPlaylistNumber, midinumber);
#endif

    temp = perf().playlist_midi_base();
    ui->midiBaseDirText->setText(qt(temp));
    temp = perf().playlist_name();
    ui->editPlaylistName->setText(qt(temp));
    set_current_song();
}

void
qplaylistframe::set_current_song ()
{
    int rows = perf().song_count();
    if (rows > 0)
    {

#if defined USE_SONG_NUMBER_TEXTEDIT
        std::string temp = std::to_string(perf().song_midi_number());
        ui->editSongNumber->setText(qt(temp));
        temp = perf().song_directory();
        if (temp.empty())
            temp = "None";
#else
        int midinumber = perf().song_midi_number();
        set_spin_value(ui->spinSongNumber, midinumber);

        std::string temp = perf().song_directory();
#endif

        ui->editSongPath->setText(qt(temp));

        bool embedded = perf().is_own_song_directory();
        temp = embedded ? "*" : " " ;
        ui->labelDirEmbedded->setText(qt(temp));
        temp = perf().song_filename();

        ui->editSongFilename->setText(qt(temp));
        temp = perf().song_filepath();
        ui->currentSongPath->setText(qt(temp));
    }
    else
    {
        ui->labelDirEmbedded->setText(" ");
    }
}

/**
 *  Retrieves the table cell at the given row and column.
 *
 * \param isplaylist
 *      If true, this call affects the play-list table.  Otherwise, it affects
 *      the song-list table.
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
qplaylistframe::cell (bool isplaylist, int row, column_id_t col)
{
    int column = int(col);
    QTableWidget * listptr = isplaylist ?
        ui->tablePlaylistSections : ui->tablePlaylistSongs ;

    QTableWidgetItem * result = listptr->item(row, column);
    if (is_nullptr(result))
    {
        /*
         * Will test row/column and maybe add rows on the fly later.
         */

        result = new QTableWidgetItem;
        listptr->setItem(row, column, result);
    }
    return result;
}

/**
 *  Adds the list of playlists to the tablePlaylistSections table-widget.  It
 *  does not select a play-list or select a song.  To be called only when
 *  loading a new play-list, as in reset_playlist().  Only acts if the list is
 *  non-empty.
 */

void
qplaylistframe::fill_playlists ()
{
    int rows = perf().playlist_count();
    if (rows > 0)
    {
        ui->tablePlaylistSections->clearContents();
        ui->tablePlaylistSections->setRowCount(rows);
        for (int r = 0; r < rows; ++r)
        {
            std::string temp;
            QTableWidgetItem * qtip = cell(true, r, CID_MIDI_NUMBER);
            ui->tablePlaylistSections->setRowHeight(r, c_playlist_row_height);
            if (not_nullptr(qtip))
            {
                int midinumber = perf().playlist_midi_number();
                temp = std::to_string(midinumber);
                qtip->setText(qt(temp));
            }
            qtip = cell(true, r, CID_ITEM_NAME);
            if (not_nullptr(qtip))
            {
                temp = perf().playlist_name();
                qtip->setText(qt(temp));
            }

            /*
             * Load the next list.  The false means "don't load the song", and
             * the true means "we're loading the playlist, so go to the next
             * playlist even if not active."
             */

            if (! perf().open_next_list(false, true))
                break;
        }

        /*
         * Only when list selected: ui->buttonPlaylistRemove->setEnabled(true);
         * Only when song selected: ui->buttonSongRemove->setEnabled(true);
         */
    }
    else
    {
        /*
         * This call cleans out the grid.
         *
         * ui->tablePlaylistSections->setRowCount(0);
         */

        ui->tablePlaylistSections->clearContents();
        ui->buttonPlaylistRemove->setEnabled(false);
        ui->buttonSongRemove->setEnabled(false);
    }
}

/**
 *  Adds the songs of the current playlist to the tablePlaylistSongs
 *  table-widget.  It does not select any song.
 */

void
qplaylistframe::fill_songs ()
{
    int rows = perf().song_count();
    if (rows > 0)
    {
        ui->tablePlaylistSongs->clearContents();
        ui->tablePlaylistSongs->setRowCount(rows);
        for (int r = 0; r < rows; ++r)
        {
            std::string temp;
            if (perf().open_select_song_by_index(r, false))
            {
                QTableWidgetItem * qtip = cell(false, r, CID_MIDI_NUMBER);
                ui->tablePlaylistSongs->setRowHeight
                (
                    r, c_playlist_row_height
                );
                if (not_nullptr(qtip))
                {
                    int midinumber = perf().song_midi_number();
                    temp = std::to_string(midinumber);
                    qtip->setText(qt(temp));
                }
                qtip = cell(false, r, CID_ITEM_NAME);
                if (not_nullptr(qtip))
                {
                    temp = perf().song_filename();
                    qtip->setText(qt(temp));
                }
            }
            else
                break;
        }
    }
    else
    {
        ui->tablePlaylistSongs->clearContents();
        ui->editSongPath->setText("");
#if defined USE_SONG_NUMBER_TEXTEDIT
        ui->editSongNumber->setText("0");
#else
        ui->spinSongNumber->setValue(0);
#endif
        ui->editSongFilename->setText("");
        ui->buttonSongRemove->setEnabled(false);
    }
}

bool
qplaylistframe::load_playlist (const std::string & fullfilespec)
{
    bool result = ! fullfilespec.empty();
    if (result)
    {
        bool opened = perf().open_playlist(fullfilespec);
        if (opened)
        {
            opened = perf().open_current_song();
            reset_playlist();
        }
        else
        {
            reset_playlist(); // reset_playlist_file_name();
        }
        update();                           /* refresh the user-interface   */
    }
    return result;
}

/**
 *  Weird, this function sometimes receives a row of -1, even when
 *  ui->tablePlaylistSections->clearContents() is called.  We ignore this
 *  happenstance without showing a message.
 */

void
qplaylistframe::slot_list_click_ex
(
    int row, int /*column*/, int /*prevrow*/, int /*prevcolumn*/
)
{
    if (row >= 0)
    {
        m_current_list_index = row;
        if (perf().open_select_list_by_index(row, false))
        {
            fill_songs();
            set_current_playlist();
            ui->tablePlaylistSongs->selectRow(0);
            ui->buttonPlaylistRemove->setEnabled(true);
            enable_midi_widgets(true);
        }
    }
}

/**
 *  Weird, this function sometimes receives a row of -1, even when
 *  ui->tablePlaylistSongs->clearContents() is called.  We ignore this
 *  happenstance without showing a message.
 */

void
qplaylistframe::slot_song_click_ex
(
    int row, int /*column*/, int /*prevrow*/, int /*prevcolumn*/
)
{
    if (row >= 0)
    {
        m_current_song_index = row;
        if (perf().open_select_song_by_index(row, true))
        {
            set_current_song();
            if (not_nullptr(parent()))
                parent()->recreate_all_slots();

            ui->buttonSongRemove->setEnabled(true);
        }
    }
}

void
qplaylistframe::slot_file_create_click ()
{
    if (not_nullptr(parent()))
    {
        if (parent()->specify_playlist())
            ui->buttonPlaylistSave->setEnabled(false);
        else
            ui->entry_playlist_file->setText("");
    }
}

/**
 *  Allows the user to select a directory to serve as the location for
 *  the MIDI files of the currently-selected play-list sub-list.
 */

void
qplaylistframe::slot_list_dir_click ()
{
    if (not_nullptr(parent()))
    {
        std::string folder = parent()->specify_playlist_folder();
        if (! folder.empty())
        {
            std::string listname;
            auto spos = folder.find_last_of("/");
            if (spos != std::string::npos)
            {
                listname = folder.substr(spos + 1);
                listname += " ";
            }
            listname += "MIDI Files";
            ui->editPlaylistPath->setText(qt(folder));
            ui->editPlaylistName->setText(qt(listname));
            ui->buttonPlaylistAdd->setEnabled(true);

            int highnumber = perf().next_available_list_number();
            ui->spinPlaylistNumber->setValue(highnumber);
            list_modify();
        }
    }
}

/**
 *  Calls qsmainwnd::open_playlist(), which then opens the file dialog and
 *  perform::open_playlist(), which resets the play-list pointer and opens the
 *  playlist.  It is reset to the first song and performer calls
 *  set_needs_update().
 */

void
qplaylistframe::slot_list_load_click ()
{
    if (not_nullptr(parent()))
    {
        if (parent()->open_playlist())
        {
            ui->buttonPlaylistSave->setEnabled(false);
            rc().auto_rc_save(true);
            parent()->enable_reload_button(true);
        }
    }
}

/**
 *  This function adds an empty list to the set of playlists in the playlist
 *  object.  The user is prompted for the characteristics of the list:
 *
 *      -   The MIDI control number that selects this list. It must not match
 *          any existing MIDI control number for list selection. This number
 *          is supplied by editing the left "Current" field,
 *          ui->editPlaylistNumber.
 *      -   The human-friendly name of the list. This name is supplied in the
 *          left unnamed field, ui->editPlaylistName.
 *      -   The optional directory where the contents (MIDI files) of the list
 *          are located.  The default is the same directory as the playlist.
 *          It is supplied in the left "Directory" field,
 *          ui->editPlaylistPath.  If in an NSM session, it is recommended to
 *          leave it as the session's "midi" directory.
 *
 *  Once this is done, then songs can be added.
 */

void
qplaylistframe::slot_list_add_click ()
{
    if (not_nullptr(parent()))
    {
        QString temp = ui->editPlaylistPath->text();
        std::string listpath = temp.toStdString();
        std::string listname;
        int index = perf().playlist_count();            /* useful number?   */
        if (index < 127)
        {
            int midinumber = index;                     /* useful number?   */
            temp = ui->editPlaylistName->text();
            listname = temp.toStdString();
#if defined USE_PLAYLIST_NUMBER_TEXTEDIT
            temp = ui->editPlaylistNumber->text();
            midinumber = string_to_int(temp.toStdString(), index);
#else
            midinumber = ui->spinPlaylistNumber->value();
#endif
            if (perf().add_list(index, midinumber, listname, listpath))
            {
                reset_playlist();
                fill_songs();                           /* just clears list */
                ui->buttonPlaylistSave->setEnabled(true);
            }
            else
            {
                /*
                 * TODO: report error
                 */
            }
        }
    }
}

void
qplaylistframe::slot_list_modify_click ()
{
    if (not_nullptr(parent()))
    {
        QString temp = ui->editPlaylistPath->text();
        std::string listpath = temp.toStdString();
        std::string listname;
        int index = m_current_list_index;
        int midinumber;
        temp = ui->editPlaylistName->text();
        listname = temp.toStdString();
#if defined USE_PLAYLIST_NUMBER_TEXTEDIT
        temp = ui->editPlaylistNumber->text();
        midinumber = string_to_int(temp.toStdString(), index);
#else
        midinumber = ui->spinPlaylistNumber->value();
#endif
        if (perf().modify_list(index, midinumber, listname, listpath))
        {
            reset_playlist(index);
            fill_songs();
            ui->buttonPlaylistSave->setEnabled(true);
        }
        else
        {
            /*
             * TODO: report error
             */
        }
    }
}

void
qplaylistframe::slot_list_remove_click ()
{
    if (not_nullptr(parent()))
    {
        int index = m_current_list_index;
        if (perf().remove_list(index))
        {
            reset_playlist();
            parent()->recreate_all_slots();
            parent()->enable_reload_button(true);
            ui->buttonPlaylistSave->setEnabled(true);
        }
    }
}

void
qplaylistframe::slot_list_save_click ()
{
    if (not_nullptr(parent()))
    {
        if (parent()->save_playlist())
        {
            rc().auto_rc_save(true);
            list_unmodify();
            song_unmodify();
            parent()->enable_reload_button(true);
        }
    }
}

/**
 *  This function supports loading a song from a file-selection dialog.
 *  Compare it to slot_song_add_click().
 */

void
qplaylistframe::slot_song_load_click ()
{
    if (not_nullptr(parent()))
    {
        std::string selectedfile = ui->editSongPath->text().toStdString();
        if (parent()->use_nsm())
        {
            bool ok = parent()->load_into_session(selectedfile);
            if (ok)
            {
                ok = perf().add_song(selectedfile);
                if (ok)
                {
                    fill_songs();           /* too much: reset_playlist();  */
                    parent()->recreate_all_slots();
                    parent()->enable_reload_button(true);
                    ui->buttonPlaylistSave->setEnabled(true);
                }
            }
        }
        else
        {
            if (show_open_midi_file_dialog(this, selectedfile))
            {
                bool ok = perf().add_song(selectedfile);
                if (ok)
                {
                    fill_songs();           /* too much: reset_playlist();  */
                    parent()->recreate_all_slots();
                    parent()->enable_reload_button(true);
                    ui->buttonPlaylistSave->setEnabled(true);
                }
            }
        }
    }
}

/**
 *  Lets one select a MIDI file from a file dialog to fill in the MIDI
 *  information.
 */

void
qplaylistframe::slot_song_select_click ()
{
    if (not_nullptr(parent()))
    {
        std::string selectedfile = ui->editSongPath->text().toStdString();
        if (show_open_midi_file_dialog(this, selectedfile))
        {
            std::string path;
            std::string basename;
            if (filename_split(selectedfile, path, basename))
            {
                ui->editSongPath->setText(qt(path));
                ui->editSongFilename->setText(qt(basename));
#if defined USE_SONG_NUMBER_TEXTEDIT
                ui->editSongNumber->setText("Unique MIDI #");
#else
                /*
                 * Find the highest song number and add one to it.
                 * Base it on playlist::select_song().
                 * Should this be done in fill_songs()???
                 * If so, what about fill_playlists() or
                 */

                int highnumber = perf().next_available_song_number();
                ui->spinSongNumber->setValue(highnumber);
#endif
                song_modify();
            }
        }
    }
}

/**
 *  These values depend on correct information edited into the Song text
 *  fields.  We support loading a song from a file-selection dialog, so that
 *  is the preferred method; see slot_song_load_click().
 */

void
qplaylistframe::slot_song_add_click ()
{
    if (not_nullptr(parent()))
    {
        std::string name = ui->editSongFilename->text().toStdString();
        std::string directory = ui->editSongPath->text().toStdString();
        name = filename_concatenate(directory, name);
        bool loadedfile = ! name.empty();
        if (loadedfile)
        {
            std::string directory;
            std::string basename;
            bool ok = filename_split(name, directory, basename);
            if (ok)
            {
#if defined USE_SONG_NUMBER_TEXTEDIT
                std::string nstr = ui->editSongNumber->text().toStdString();
                int midinumber = string_to_int(nstr);
#else
                int midinumber = ui->spinSongNumber->value();
#endif
                int index = perf().song_count() + 1;
                if (perf().add_song(index, midinumber, basename, directory))
                {
                    int listindex = m_current_list_index;
                    reset_playlist(listindex);
                    parent()->recreate_all_slots();
                    parent()->enable_reload_button(true);
                    song_unmodify();
                }
                else
                {
                    QErrorMessage * errbox = new QErrorMessage(this);
                    errbox->showMessage("Error adding song, fix MIDI #");
                    errbox->exec();
                }
            }
        }
    }
}

void
qplaylistframe::slot_song_modify_click ()
{
    if (not_nullptr(parent()))
    {
        std::string name = ui->editSongFilename->text().toStdString();
        std::string directory = ui->editSongPath->text().toStdString();
#if defined USE_SONG_NUMBER_TEXTEDIT
        std::string nstr = ui->editSongNumber->text().toStdString();
        int midinumber = string_to_int(nstr);
#else
        int midinumber = ui->spinSongNumber->value();
#endif
        int songindex = m_current_song_index;
        if (perf().modify_song(songindex, midinumber, name, directory))
        {
            int listindex = m_current_list_index;
            reset_playlist(listindex);
            parent()->recreate_all_slots();
            parent()->enable_reload_button(true);
        }
        else
        {
            /*
             * TODO: report error
             */
        }
    }
}

void
qplaylistframe::slot_song_remove_click ()
{
    if (not_nullptr(parent()))
    {
        int listindex = m_current_list_index;
        int songindex = m_current_song_index;
        if (perf().remove_song_by_index(songindex))
        {
            reset_playlist(listindex);
            parent()->recreate_all_slots();
            parent()->enable_reload_button(true);
            ui->buttonPlaylistSave->setEnabled(true);
        }
        else
        {
            /*
             * TODO: report error
             */
        }
    }
}

void
qplaylistframe::slot_playlist_active_click ()
{
    if (not_nullptr(parent()))
    {
        bool on = ui->checkBoxPlaylistActive->isChecked();
        bool success = perf().playlist_activate(on);
        if (success)
        {
            rc().auto_rc_save(true);
            if (on)                         /* leave patterns in if off     */
                parent()->recreate_all_slots();         /* why? */

            parent()->enable_reload_button(true);
        }
        ui->checkBoxPlaylistActive->setChecked(perf().playlist_active());
        ui->buttonPlaylistSave->setEnabled(true);
    }
}

void
qplaylistframe::slot_auto_arm_click ()
{
    if (not_nullptr(parent()))
    {
        bool on = ui->checkBoxAutoArm->isChecked();
        perf().playlist_auto_arm(on);
        ui->buttonPlaylistSave->setEnabled(true);
        parent()->enable_reload_button(true);
    }
}

void
qplaylistframe::slot_auto_play_click ()
{
    if (not_nullptr(parent()))
    {
        bool on = ui->checkBoxAutoPlay->isChecked();
        perf().playlist_auto_play(on);
        ui->buttonPlaylistSave->setEnabled(true);
        ui->checkBoxAutoArm->setChecked(true);          /* tricky   */
        slot_auto_arm_click();
    }
}

void
qplaylistframe::slot_auto_advance_click ()
{
    if (not_nullptr(parent()))
    {
        bool on = ui->checkBoxAutoAdvance->isChecked();
        perf().playlist_auto_advance(on);
        ui->buttonPlaylistSave->setEnabled(true);
        ui->checkBoxAutoPlay->setChecked(true);         /* tricky   */
        slot_auto_play_click();
    }
}

void
qplaylistframe::list_modify (const QString &)
{
    list_modify();
}

void
qplaylistframe::list_unmodify ()
{
    ui->buttonPlaylistAdd->setEnabled(false);
    ui->buttonPlaylistModify->setEnabled(false);
    ui->buttonPlaylistSave->setEnabled(false);
}

void
qplaylistframe::list_modify (int)
{
    list_modify();
}

void
qplaylistframe::list_modify ()
{
    ui->buttonPlaylistAdd->setEnabled(true);
    ui->buttonPlaylistModify->setEnabled(true);
    ui->buttonPlaylistSave->setEnabled(true);
    parent()->enable_reload_button(true);
}

void
qplaylistframe::song_modify (const QString &)
{
    song_modify();
}

void
qplaylistframe::song_unmodify ()
{
    ui->buttonSongAdd->setEnabled(false);
    ui->buttonSongModify->setEnabled(false);
    ui->buttonPlaylistSave->setEnabled(true);
}

void
qplaylistframe::song_modify (int)
{
    song_modify();
}

void
qplaylistframe::song_modify ()
{
    /*
     * Why this check? Makes no sense.
     *
     * bool addable = ! rc().midi_filename().empty();
     * if (addable)
     */

    ui->buttonSongAdd->setEnabled(true);
    ui->buttonSongModify->setEnabled(true);
    ui->buttonPlaylistSave->setEnabled(true);
    parent()->enable_reload_button(true);
}

/*
 *  We must accept() the key-event, otherwise even key-events in the QLineEdit
 *  items are propagated to the parent, where they then get passed to the
 *  performer as if they were keyboards controls (such as a pattern-toggle
 *  hot-key).
 */

void
qplaylistframe::keyPressEvent (QKeyEvent * event)
{
    event->accept();
}

void
qplaylistframe::keyReleaseEvent (QKeyEvent * event)
{
    event->accept();
}

}           // namespace seq66

/*
 * qplaylistframe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


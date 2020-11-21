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
 * \file          qplaylistframe.cpp
 *
 *  This module declares/defines the base class for plastering
 *  pattern/sequence data information in the data area of the pattern
 *  editor.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-09-04
 * \updates       2020-11-20
 * \license       GNU GPLv2 or above
 *
 */

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

/**
 *  For correcting the width of the play-list tables.  It tries to account for
 *  the width of the vertical scroll-bar, plus a bit more.
 */

#define SEQ66_PLAYLIST_TABLE_FIX        24  // 48

/**
 *  Specifies the current hardwired value for set_row_heights().
 */

#define SEQ66_PLAYLIST_ROW_HEIGHT       18

namespace seq66
{

/**
 *
 */

qplaylistframe::qplaylistframe
(
    performer & p,
    qsmainwnd * window,
    QWidget * parent
) :
    QFrame                  (parent),
    ui                      (new Ui::qplaylistframe),
    m_timer                 (nullptr),
    m_performer             (p),
    m_parent                (window),
    m_current_list_index    (0),
    m_current_song_index    (0)
{
    ui->setupUi(this);

    QStringList playcolumns;
    playcolumns << "#" << "List Names";
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
    set_row_heights(SEQ66_PLAYLIST_ROW_HEIGHT);
    set_column_widths();
    connect
    (
        ui->tablePlaylistSections, SIGNAL(currentCellChanged(int, int, int, int)),
        this, SLOT(handle_list_click_ex(int, int, int, int))
    );
    connect
    (
        ui->tablePlaylistSongs, SIGNAL(currentCellChanged(int, int, int, int)),
        this, SLOT(handle_song_click_ex(int, int, int, int))
    );
    connect
    (
        ui->buttonPlaylistLoad, SIGNAL(clicked(bool)),
        this, SLOT(handle_list_load_click())
    );
    connect
    (
        ui->buttonPlaylistAdd, SIGNAL(clicked(bool)),
        this, SLOT(handle_list_add_click())
    );
    connect
    (
        ui->buttonPlaylistRemove, SIGNAL(clicked(bool)),
        this, SLOT(handle_list_remove_click())
    );
    if (not_nullptr(m_parent))
    {
        if (m_parent->use_nsm())
        {
            ui->buttonPlaylistSave->setText("Save Sess");
            ui->buttonPlaylistSave->setToolTip
            (
                "Save playlists and MIDI to current NSM session."
            );
        }
        else
        {
            ui->buttonPlaylistSave->setText("Save Lists");
            ui->buttonPlaylistSave->setToolTip
            (
                "Save playlists (only) to current directory."
            );
        }
    }
    connect
    (
        ui->buttonPlaylistSave, SIGNAL(clicked(bool)),
        this, SLOT(handle_list_save_click())
    );
    connect
    (
        ui->buttonSongLoad, SIGNAL(clicked(bool)),
        this, SLOT(handle_song_load_click())
    );
    connect
    (
        ui->buttonSongAdd, SIGNAL(clicked(bool)),
        this, SLOT(handle_song_add_click())
    );
    connect
    (
        ui->buttonSongRemove, SIGNAL(clicked(bool)),
        this, SLOT(handle_song_remove_click())
    );
    connect
    (
        ui->checkBoxPlaylistActive, SIGNAL(clicked(bool)),
        this, SLOT(handle_playlist_active_click())
    );
    connect
    (
        ui->entry_playlist_file, SIGNAL(textEdited(QString)),
        this, SLOT(list_modify(QString))
    );
    connect
    (
        ui->editPlaylistNumber, SIGNAL(textEdited(QString)),
        this, SLOT(list_modify(QString))
    );
    connect
    (
        ui->editPlaylistPath, SIGNAL(textEdited(QString)),
        this, SLOT(list_modify(QString))
    );
    connect
    (
        ui->editPlaylistName, SIGNAL(textEdited(QString)),
        this, SLOT(list_modify(QString))
    );
    connect
    (
        ui->editSongNumber, SIGNAL(textEdited(QString)),
        this, SLOT(song_modify(QString))
    );
    connect
    (
        ui->editSongPath, SIGNAL(textEdited(QString)),
        this, SLOT(song_modify(QString))
    );
    connect
    (
        ui->editSongFilename, SIGNAL(textEdited(QString)),
        this, SLOT(song_modify(QString))
    );
    reset_playlist();                       /* if (perf().playlist_mode())  */

    m_timer = new QTimer(this);             /* timer for regular redraws    */
    m_timer->setInterval(4 * usr().window_redraw_rate());
    connect(m_timer, SIGNAL(timeout()), this, SLOT(conditional_update()));
    m_timer->start();
}

/**
 *  Stops the timer and deletes the user interface.
 */

qplaylistframe::~qplaylistframe ()
{
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
    int w = ui->tablePlaylistSections->width() - SEQ66_PLAYLIST_TABLE_FIX;
    ui->tablePlaylistSections->setColumnWidth(0, int(0.20f * w));
    ui->tablePlaylistSections->setColumnWidth(1, int(0.80f * w));

    w = ui->tablePlaylistSongs->width() - SEQ66_PLAYLIST_TABLE_FIX;
    ui->tablePlaylistSongs->setColumnWidth(0, int(0.20f * w));
    ui->tablePlaylistSongs->setColumnWidth(1, int(0.80f * w));
}

/**
 *  Resets the play-list.  First, resets to the first (0th) play-list and the
 *  first (0th) song.  Then fills the play-list items and resets again.  Then
 *  fills in the play-list and song items for the current selection.
 */

void
qplaylistframe::reset_playlist ()
{
    if (perf().playlist_reset())
    {
        fill_playlists();
        (void) perf().playlist_reset();     /* back to the 0th play-list    */
        fill_songs();
        set_current_playlist();
        ui->tablePlaylistSections->selectRow(0);
        ui->tablePlaylistSongs->selectRow(0);
    }
}

/**
 *
 */

void
qplaylistframe::set_current_playlist ()
{
    std::string temp;
    ui->checkBoxPlaylistActive->setChecked(perf().playlist_mode());
    temp = perf().playlist_filename();
    if (temp.empty())
        temp = "None";

    ui->entry_playlist_file->setText(QString::fromStdString(temp));
    temp = perf().file_directory();
    if (temp.empty())
        temp = "None";

    ui->editPlaylistPath->setText(QString::fromStdString(temp));

    int midinumber = perf().playlist_midi_number();
    temp = std::to_string(midinumber);
    if (temp.empty())
        temp = "0";

    ui->editPlaylistNumber->setText(QString::fromStdString(temp));
    temp = perf().playlist_midi_base();
    if (temp.empty())
        temp = "None";

    ui->midiBaseDirText->setText(QString::fromStdString(temp));
    temp = perf().playlist_name();
    if (temp.empty())
        temp = "None";

    ui->editPlaylistName->setText(QString::fromStdString(temp));
    set_current_song();
}

/**
 *
 */

void
qplaylistframe::set_current_song ()
{
    std::string temp = std::to_string(perf().song_midi_number());
    ui->editSongNumber->setText(QString::fromStdString(temp));

    temp = perf().song_directory();
    if (temp.empty())
        temp = "None";

    ui->editSongPath->setText(QString::fromStdString(temp));

    bool embedded = perf().is_own_song_directory();
    temp = embedded ? "*" : " " ;
    ui->labelDirEmbedded->setText(QString::fromStdString(temp));

    temp = perf().song_filename();
    if (temp.empty())
        temp = "None";

    ui->editSongFilename->setText(QString::fromStdString(temp));
    temp = perf().song_filepath();
    if (temp.empty())
        temp = "None";

    ui->currentSongPath->setText(QString::fromStdString(temp));
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
            ui->tablePlaylistSections->setRowHeight(r, SEQ66_PLAYLIST_ROW_HEIGHT);
            if (not_nullptr(qtip))
            {
                int midinumber = perf().playlist_midi_number();
                temp = std::to_string(midinumber);
                qtip->setText(QString::fromStdString(temp));
            }
            qtip = cell(true, r, CID_ITEM_NAME);
            if (not_nullptr(qtip))
            {
                temp = perf().playlist_name();
                qtip->setText(QString::fromStdString(temp));
            }
            if (! perf().open_next_list(false))     /* false = no load song */
                break;
        }
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
                    r, SEQ66_PLAYLIST_ROW_HEIGHT
                );
                if (not_nullptr(qtip))
                {
                    int midinumber = perf().song_midi_number();
                    temp = std::to_string(midinumber);
                    qtip->setText(QString::fromStdString(temp));
                }
                qtip = cell(false, r, CID_ITEM_NAME);
                if (not_nullptr(qtip))
                {
                    temp = perf().song_filename();
                    qtip->setText(QString::fromStdString(temp));
                }
            }
            else
                break;
        }
    }
}

/**
 *
 */

bool
qplaylistframe::load_playlist (const std::string & fullfilespec)
{
    if (! fullfilespec.empty())
    {
        bool playlistmode = perf().open_playlist(fullfilespec, rc().verbose());
        if (playlistmode)
            playlistmode = perf().open_current_song();
    }
    reset_playlist();                       /* if (perf().playlist_mode())  */
    return false;
}

/**
 *  Weird, this function sometimes receives a row of -1, even when
 *  ui->tablePlaylistSections->clearContents() is called.  We ignore this
 *  happenstance without showing a message.
 */

void
qplaylistframe::handle_list_click_ex
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
        }
    }
}

/**
 *  Weird, this function sometimes receives a row of -1, even when
 *  ui->tablePlaylistSongs->clearContents() is called.  We ignore this
 *  happenstance without showing a message.
 */

void
qplaylistframe::handle_song_click_ex
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
            if (not_nullptr(m_parent))
                m_parent->recreate_all_slots();
        }
    }
}

/**
 *  Calls qsmainwnd::open_playlist(), which then calls
 *  qsmainwnd::show_open_list_dialog(), then perform::open_playlist().  That
 *  function resets the play-list pointer, then opens the playlist.
 *  It is reset to the first song and performer calls set_needs_update().
 */

void
qplaylistframe::handle_list_load_click ()
{
    if (not_nullptr(m_parent))
        m_parent->open_playlist();
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
qplaylistframe::handle_list_add_click ()
{
    if (not_nullptr(m_parent))
    {
        QString temp = ui->editPlaylistPath->text();
        std::string listpath = temp.toStdString();
        std::string listname;
        int index = perf().playlist_count();        // useful???
        if (index < 127)
        {
            int midinumber = index;                     // useful???
            temp = ui->editPlaylistName->text();
            listname = temp.toStdString();
            temp = ui->editPlaylistNumber->text();
            midinumber = string_to_int(temp.toStdString(), index);
            if (perf().add_list(index, midinumber, listname, listpath))
            {
                reset_playlist();
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

/**
 *
 */

void
qplaylistframe::handle_list_remove_click ()
{
    if (not_nullptr(m_parent))
    {
        int index = m_current_list_index;
        if (perf().remove_list(index))
        {
            reset_playlist();
            m_parent->recreate_all_slots();
        }
    }
}

/**
 *
 */

void
qplaylistframe::handle_list_save_click ()
{
    if (not_nullptr(m_parent))
    {
        QString p = ui->entry_playlist_file->text();
        std::string plistname = p.toStdString();
        if (! plistname.empty())
        {
            if (perf().save_playlist(plistname))
            {
                list_unmodify();
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

/**
 *  These values depend on correct information edited into the Song text
 *  fields.  We should support loading a song from a file-selection dialog.
 *
 *  LOAD SONG: Add a song picked from a directory?
 *
 *  ADD SONG:  Add the currently loaded song?
 */

void
qplaylistframe::handle_song_load_click ()
{
    if (not_nullptr(m_parent))
    {
        std::string selectedfile = ui->editSongPath->text().toStdString();
        if (show_open_midi_file_dialog(this, selectedfile))
        {
            bool ok;
            std::string name;
            std::string directory = ui->editSongPath->text().toStdString();
            std::string nstr;   // = ui->editSongNumber->text().toStdString();
            int index = perf().song_count() + 1;
            int midinumber = (-1);  // std::stoi(nstr);
            (void) filename_split(selectedfile, nstr, name);
            if (nstr == directory)              /* i.e. the same file path  */
            {
                ok = perf().add_song(index, midinumber, name, directory);
            }
            else
            {
                std::string dir;
                ok = perf().add_song(index, midinumber, selectedfile, dir);
            }
            if (ok)
            {
                fill_songs();               /* too much: reset_playlist();  */
                m_parent->recreate_all_slots();
            }
        }
    }
}

/**
 *  These values depend on correct information edited into the Song text
 *  fields.  We should support loading a song from a file-selection dialog.
 */

void
qplaylistframe::handle_song_add_click ()
{
    if (not_nullptr(m_parent))
    {
        std::string name = ui->editSongFilename->text().toStdString();
        std::string directory = ui->editSongPath->text().toStdString();
        std::string nstr = ui->editSongNumber->text().toStdString();
        int midinumber = std::stoi(nstr);
        int index = perf().song_count() + 1;
        if (perf().add_song(index, midinumber, name, directory))
        {
            reset_playlist();
            m_parent->recreate_all_slots();
        }
        else
        {
            /*
             * TODO: report error
             */
        }
    }
}

/**
 *
 */

void
qplaylistframe::handle_song_remove_click ()
{
    if (not_nullptr(m_parent))
    {
        int index = m_current_song_index;
        if (perf().remove_song_by_index(index))
        {
            reset_playlist();
            m_parent->recreate_all_slots();
        }
        else
        {
            /*
             * TODO: report error
             */
        }
    }
}

/**
 *
 */

void
qplaylistframe::handle_playlist_active_click ()
{
    if (not_nullptr(m_parent))
    {
        //  m_parent->TODO();
    }
}

/**
 *
 */

void
qplaylistframe::list_modify (const QString &)
{
    ui->buttonPlaylistAdd->setEnabled(true);
    ui->buttonPlaylistModify->setEnabled(true);
    ui->buttonPlaylistSave->setEnabled(true);
}

/**
 *
 */

void
qplaylistframe::list_unmodify ()
{
    ui->buttonPlaylistAdd->setEnabled(false);
    ui->buttonPlaylistModify->setEnabled(false);
    ui->buttonPlaylistSave->setEnabled(false);
}

/**
 *
 */

void
qplaylistframe::song_modify (const QString &)
{
    ui->buttonSongAdd->setEnabled(true);
    ui->buttonSongModify->setEnabled(true);
}

/**
 *
 */

void
qplaylistframe::song_unmodify ()
{
    ui->buttonSongAdd->setEnabled(false);
    ui->buttonSongModify->setEnabled(false);
}

/**
 *
 */

void
qplaylistframe::keyPressEvent (QKeyEvent * event)
{
    QWidget::keyPressEvent(event);      // event->ignore();
}

/**
 *
 */

void
qplaylistframe::keyReleaseEvent (QKeyEvent * event)
{
    QWidget::keyReleaseEvent(event);    // event->ignore();
}

}           // namespace seq66

/*
 * qplaylistframe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


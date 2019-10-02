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
 * \file          qsmainwnd.cpp
 *
 *  This module declares/defines the base class for the main window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2019-09-18
 * \license       GNU GPLv2 or above
 *
 *  The main window is known as the "Patterns window" or "Patterns
 *  panel".  It holds the "Pattern Editor" or "Sequence Editor".  The main
 *  window consists of two object:  mainwnd, which provides the user-interface
 *  elements that surround the patterns, and mainwid, which implements the
 *  behavior of the pattern slots.
 */

#include <QDesktopWidget>               /* needed for screenGeometry() call */
#include <QErrorMessage>
#include <QFileDialog>
#include <QMessageBox>
#include <QResizeEvent>
#include <QTimer>

#include <sstream>                      /* std::ostringstream               */
#include <utility>                      /* std::make_pair()                 */

#include "cfg/settings.hpp"             /* seq66::usr().key_height(), etc.  */
#include "ctrl/keystroke.hpp"           /* seq66::keystroke class           */
#include "midi/wrkfile.hpp"             /* seq66::wrkfile class             */
#include "qliveframeex.hpp"
#include "qperfeditex.hpp"
#include "qperfeditframe64.hpp"
#include "qplaylistframe.hpp"
#include "qsmacros.hpp"                 /* QS_TEXT_CHAR() macro             */
#include "qsabout.hpp"
#include "qsbuildinfo.hpp"
#include "qseditoptions.hpp"
#include "qseqeditex.hpp"
#include "qseqeditframe.hpp"            /* Kepler34 version                 */
#include "qseqeventframe.hpp"           /* a new event editor for Qt        */
#include "qsetmaster.hpp"               /* shows a map of all sets          */
#include "qskeymaps.hpp"                /* mapping between Gtkmm and Qt     */
#include "qsmaintime.hpp"
#include "qsmainwnd.hpp"
#include "qsliveframe.hpp"
#include "qslivegrid.hpp"
#include "qt5_helpers.hpp"              /* seq66::qt_set_icon() etc.        */
#include "util/calculations.hpp"        /* pulse_to_measurestring(), etc.   */
#include "util/filefunctions.hpp"       /* seq66::file_extension_match()    */
#include "qmutemaster.hpp"              /* shows a map of mute-groups       */

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#if defined SEQ66_QMAKE_RULES
#include "forms/ui_qsmainwnd.h"
#else
#include "forms/qsmainwnd.ui.h"         /* generated btnStop, btnPlay, etc. */
#endif

#include "pixmaps/learn.xpm"
#include "pixmaps/learn2.xpm"
#include "pixmaps/live_mode.xpm"        /* #include "pixmaps/song_mode.xpm" */
#include "pixmaps/panic.xpm"
#include "pixmaps/pause.xpm"
#include "pixmaps/perfedit.xpm"
#include "pixmaps/play2.xpm"
#include "pixmaps/song_rec_on.xpm"      /* #include "pixmaps/song_rec.xpm" */
#include "pixmaps/stop.xpm"

/*
 * Don't document the namespace.
 */

namespace seq66
{

/**
 *  Manifest constant to indicate the location of each main-window tab.
 */

static const int Tab_Live           = 0;
static const int Tab_Song           = 1;
static const int Tab_Edit           = 2;
static const int Tab_Events         = 3;
static const int Tab_Playlist       = 4;
static const int Tab_Set_Master     = 5;
static const int Tab_Mute_Master    = 6;

/**
 *  Provides the main window for the application.
 *
 * \param p
 *      Provides the performer object to use for interacting with this sequence.
 *
 * \param midifilename
 *      Provides an optional MIDI file-name.  If provided, the file is opened
 *      immediately.
 *
 * \param ppqn
 *      Sets the desired PPQN value.  If 0 (SEQ66_USE_FILE_PPQN), then
 *      the PPQN to use is obtained from the file.  Otherwise, if a legal
 *      PPQN, any file read is scaled temporally to that PPQN, as applicable.
 *
 * \param parent
 *      Provides the parent window/widget for this container window.  Defaults
 *      to null.
 */

qsmainwnd::qsmainwnd
(
    performer & p,
    const std::string & midifilename,
    int ppqn,
    QWidget * parent
) :
    QMainWindow             (parent),
    performer::callbacks    (p),
    ui                      (new Ui::qsmainwnd),
    m_live_frame            (nullptr),
    m_perfedit              (nullptr),
    m_song_frame64          (nullptr),
    m_edit_frame            (nullptr),
    m_event_frame           (nullptr),
    m_playlist_frame        (nullptr),
    m_msg_error             (nullptr),
    m_msg_save_changes      (nullptr),
    m_timer                 (nullptr),
    m_menu_recent           (nullptr),
    m_recent_action_list    (),
    mc_max_recent_files     (10),
    m_import_dialog         (nullptr),
    m_main_perf             (p),
    m_beat_ind              (nullptr),
    m_dialog_prefs          (nullptr),
    m_dialog_about          (nullptr),
    m_dialog_build_info     (nullptr),
    m_set_master            (nullptr),
    m_mute_master           (nullptr),
    m_is_title_dirty        (true),
    m_ppqn                  (ppqn),     /* can specify 0 for file ppqn  */
    m_tick_time_as_bbt      (true),
    m_open_editors          (),
    m_open_live_frames      (),
    m_perf_frame_visible    (false),
    m_current_main_set      (0)
{
#if ! defined SEQ66_PLATFORM_CPP_11
    initialize_key_map();
#endif

    ui->setupUi(this);

    QRect screen = QApplication::desktop()->screenGeometry();
    int x = (screen.width() - width()) / 2;             // center on screen
    int y = (screen.height() - height()) / 2;
    move(x, y);

    /*
     * TODO.
     *  Combo-box for tweaking the PPQN.
     *  Hidden for now.
     */

    ui->cmbPPQN->hide();

    std::string ppqnstr = std::to_string(p.ppqn());
    ui->lineEditPpqn->setText(ppqnstr.c_str());

    /*
     * Fill options for beats per measure in the combo box, and set the
     * default.
     */

    for (int i = 0; i < 16; ++i)
    {
        QString combo_text = QString::number(i + 1);
        ui->cmb_beat_measure->insertItem(i, combo_text);
    }

    /*
     * Fill options for beat length (beat width) in the combo box, and set the
     * default.
     */

    for (int i = 0; i < 5; ++i)
    {
        QString combo_text = QString::number(pow(2, i));
        ui->cmb_beat_length->insertItem(i, combo_text);
    }

    m_msg_error = new QErrorMessage(this);
    m_msg_save_changes = new QMessageBox(this);
    m_msg_save_changes->setText(tr("Unsaved changes detected."));
    m_msg_save_changes->setInformativeText(tr("Do you want to save them?"));
    m_msg_save_changes->setStandardButtons
    (
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
    );
    m_msg_save_changes->setDefaultButton(QMessageBox::Save);

    m_import_dialog = new QFileDialog
    (
        this, tr("Import MIDI file to Current Set..."),
        rc().last_used_dir().c_str(),
        tr("MIDI files (*.midi *.mid);;WRK files (*.wrk);;All files (*)")
    );

    m_dialog_prefs = new qseditoptions(perf(), this);
    m_beat_ind = new qsmaintime(perf(), this, 4, 4);
    m_dialog_about = new qsabout(this);
    m_dialog_build_info = new qsbuildinfo(this);
    make_perf_frame_in_tab();           /* create m_song_frame64 pointer    */

    /*
     * LATER:  See if we can leverage redo_live_frame().
     */

    if (usr().grid_is_button())
        m_live_frame = new qslivegrid(perf(), this, ui->LiveTab);
    else
        m_live_frame = new qsliveframe(perf(), this, ui->LiveTab);

    if (not_nullptr(m_live_frame))
    {
        ui->LiveTabLayout->addWidget(m_live_frame);
        m_live_frame->setFocus();
    }

    m_playlist_frame = new qplaylistframe(perf(), this, ui->PlaylistTab);
    if (not_nullptr(m_playlist_frame))
        ui->PlaylistTabLayout->addWidget(m_playlist_frame);

    /*
     * File menu.  Connect the GUI elements to event handlers.
     */

    connect(ui->actionNew, SIGNAL(triggered(bool)), this, SLOT(new_file()));
    connect(ui->actionSave, SIGNAL(triggered(bool)), this, SLOT(save_file()));
    connect
    (
        ui->actionSave_As, SIGNAL(triggered(bool)),
        this, SLOT(save_file_as())
    );
    connect
    (
        ui->actionExportSong, SIGNAL(triggered(bool)),
        this, SLOT(export_song())
    );
    connect
    (
        ui->actionExportMIDI, SIGNAL(triggered(bool)),
        this, SLOT(export_file_as_midi())
    );
    connect
    (
        ui->actionImport_MIDI, SIGNAL(triggered(bool)),
        this, SLOT(show_import_dialog())
    );
    connect
    (
        ui->actionOpen, SIGNAL(triggered(bool)),
        this, SLOT(show_open_file_dialog())
    );
    connect
    (
        ui->actionOpenPlaylist, SIGNAL(triggered(bool)),
        this, SLOT(show_open_list_dialog())
    );
    connect(ui->actionQuit, SIGNAL(triggered(bool)), this, SLOT(quit()));

    /*
     * Help menu.
     */

    connect(ui->actionAbout, SIGNAL(triggered(bool)), this, SLOT(showqsabout()));
    connect
    (
        ui->actionBuildInfo, SIGNAL(triggered(bool)),
        this, SLOT(showqsbuildinfo())
    );

    /*
     * Edit Menu.  First connect the preferences dialog to the main window's
     * Edit / Preferences menu entry.  Then connect all the new Edit menu
     * entries.
     */

    if (not_nullptr(m_dialog_prefs))
    {
        connect
        (
            ui->actionPreferences, SIGNAL(triggered(bool)),
            m_dialog_prefs, SLOT(show())
        );
    }

    connect
    (
        ui->actionSongEditor, SIGNAL(triggered(bool)),
        this, SLOT(open_performance_edit())
    );
    connect
    (
        ui->actionSongTranspose, SIGNAL(triggered(bool)),
        this, SLOT(apply_song_transpose())
    );
    connect
    (
        ui->actionClearMuteGroups, SIGNAL(triggered(bool)),
        this, SLOT(clear_mute_groups())
    );
    connect
    (
        ui->actionReloadMuteGroups, SIGNAL(triggered(bool)),
        this, SLOT(reload_mute_groups())
    );
    connect
    (
        ui->actionMuteAllTracks, SIGNAL(triggered(bool)),
        this, SLOT(set_song_mute_on())
    );
    connect
    (
        ui->actionUnmuteAllTracks, SIGNAL(triggered(bool)),
        this, SLOT(set_song_mute_off())
    );
    connect
    (
        ui->actionToggleAllTracks, SIGNAL(triggered(bool)),
        this, SLOT(set_song_mute_toggle())
    );

    /*
     * Stop button.
     */

    connect(ui->btnStop, SIGNAL(clicked(bool)), this, SLOT(stop_playing()));
    qt_set_icon(stop_xpm, ui->btnStop);

    /*
     * Pause button.
     */

    connect(ui->btnPause, SIGNAL(clicked(bool)), this, SLOT(pause_playing()));
    qt_set_icon(pause_xpm, ui->btnPause);

    /*
     * Play button.
     */

    connect(ui->btnPlay, SIGNAL(clicked(bool)), this, SLOT(start_playing()));
    qt_set_icon(play2_xpm, ui->btnPlay);

    /*
     * Song Play (Live vs Song) button.
     */

    connect
    (
        ui->btnSongPlay, SIGNAL(clicked(bool)),
        this, SLOT(set_song_mode(bool))
    );
    if (usr().use_more_icons())
        qt_set_icon(live_mode_xpm, ui->btnSongPlay);

    set_song_mode(false);

    /*
     * Record-Song button.
     */

    connect
    (
        ui->btnRecord, SIGNAL(clicked(bool)),
        this, SLOT(song_recording(bool))
    );
    qt_set_icon(song_rec_on_xpm, ui->btnRecord);

    /*
     * Performance Editor button.
     */

    connect
    (
        ui->btnPerfEdit, SIGNAL(clicked(bool)),
        this, SLOT(load_qperfedit(bool))
    );
    qt_set_icon(perfedit_xpm, ui->btnPerfEdit);

    /*
     * B:B:T vs H:M:S button.
     */

    connect
    (
        ui->btn_set_HMS, SIGNAL(clicked(bool)),
        this, SLOT(toggle_time_format(bool))
    );

    /*
     * Set-Master window button.
     */

    connect
    (
        ui->setMasterButton, SIGNAL(clicked(bool)),
        this, SLOT(show_set_master())
    );


    /*
     * Group-learn ("L") button.
     */

    connect
    (
        ui->button_learn, SIGNAL(clicked(bool)),
        this, SLOT(learn_toggle())
    );
    qt_set_icon(learn_xpm, ui->button_learn);

    /*
     * Tap BPM button.
     */

    connect
    (
        ui->button_tap_bpm, SIGNAL(clicked(bool)),
        this, SLOT(tap())
    );

    /*
     * Keep Queue button.
     */

    connect
    (
        ui->button_keep_queue, SIGNAL(clicked(bool)),
        this, SLOT(queue_it())
    );
    ui->button_keep_queue->setCheckable(true);

    /*
     * BPM (beats-per-minute) spin-box.
     */

    ui->spinBpm->setDecimals(usr().bpm_precision());
    ui->spinBpm->setSingleStep(usr().bpm_step_increment());
    ui->spinBpm->setValue(perf().bpm());
    ui->spinBpm->setReadOnly(false);
    connect
    (
        ui->spinBpm, SIGNAL(valueChanged(double)),
        this, SLOT(update_bpm(double))
    );
    connect
    (
        ui->spinBpm, SIGNAL(editingFinished()),
        this, SLOT(edit_bpm())
    );

    /*
     * Beat Length (Width) combo-box.
     */

    connect
    (
        ui->cmb_beat_length, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_beat_length(int))
    );

    /*
     * Beats-Per-Measure combo-box.
     */

    connect
    (
        ui->cmb_beat_measure, SIGNAL(currentIndexChanged(int)),
        this, SLOT(updatebeats_per_measure(int))
    );

    /*
     * Tab-change callback.
     */

    connect
    (
        ui->tabWidget, SIGNAL(currentChanged(int)),
        this, SLOT(tabWidgetClicked(int))
    );

    /*
     * Record Snap button. Removed.  We always snap.
     *
     *  connect
     *  (
     *      ui->btnRecSnap, SIGNAL(clicked(bool)),
     *      this, SLOT(song_recording_snap(bool))
     *  );
     *  qt_set_icon(snap_xpm, ui->btnRecSnap);
     */

    /*
     * Pattern editor callbacks.  One for editing in the tab, and the other
     * for editing in an external pattern editor window.  Also added is a
     * signal/callback to create an external live-frame window.
     */

    if (not_nullptr(m_live_frame))
    {
        connect_editor_slots();
        connect
        (
            m_live_frame, SIGNAL(signal_live_frame(int)),
            this, SLOT(load_live_frame(int))
        );
    }

    /*
     * Panic button.
     */

    connect(ui->btnPanic, SIGNAL(clicked(bool)), this, SLOT(panic()));
    qt_set_icon(panic_xpm, ui->btnPanic);

    QString bname = perf().bank_name(0 /*m_bank_id*/).c_str();
    ui->txtBankName->setText(bname);
    ui->spinBank->setRange(0, usr().max_sets() - 1);

    /*
     * Set Number.
     */

    connect
    (
        ui->spinBank, SIGNAL(valueChanged(int)),
        this, SLOT(update_bank(int))
    );

    /*
     * Set Name.
     */

    connect
    (
        ui->txtBankName, SIGNAL(textEdited(QString)),
        this, SLOT(update_bank_name(QString))
    );

    /*
     * Other setups.
     */

    create_action_connections();
    create_action_menu();
    update_recent_files_menu();

    /*
     *  The MIDI file is now opened and read in the performer before the
     *  creation of this window, to avoid issues with unset PPQN values
     *  causing segfaults.
     *
     *      open_file(midifilename);
     */

    if (! midifilename.empty())
        m_is_title_dirty = true;

    if (usr().window_is_scaled())
    {
        /*
         * This scales the full GUI, cool!  However, it can be overridden by the
         * size of the new, larger, qseqeditframe64 frame.  We see the normal-size
         * window come up, and then it jumps to the larger size.
         */

#if defined SEQ66_PLATFORM_DEBUG_TMI
        int sh = SEQ66_QSMAINWND_WIDTH;
        int sw = SEQ66_QSMAINWND_HEIGHT;
        int width = usr().scale_size(sw);
        int height = usr().scale_size(sh);
#endif

        QSize s = size();
        int h = s.height();
        int w = s.width();
        int width = usr().scale_size(w);
        int height = usr().scale_size(h);
        resize(width, height);
        if (not_nullptr(m_live_frame))
            m_live_frame->repaint();
    }

    load_set_master();
    load_mute_master();
    ui->tabWidget->setCurrentIndex(Tab_Live);
    perf().enregister(this);            /* register this for notifications  */
    show();
    m_timer = new QTimer(this);         /* refresh GUI element every few ms */
    m_timer->setInterval(3 * usr().window_redraw_rate());   // was 2
    connect(m_timer, SIGNAL(timeout()), this, SLOT(refresh()));
    m_timer->start();
}

/**
 *  Destroys the user interface and removes any external qperfeditex that
 *  exists.
 */

qsmainwnd::~qsmainwnd ()
{
    perf().unregister(this);            /* unregister this immediately      */
    delete ui;
}

/**
 *  Handles closing this window by calling check(), and, if it returns false,
 *  ignoring the close event.
 *
 * \param event
 *      Provides a pointer to the close event to be checked.
 */

void
qsmainwnd::closeEvent (QCloseEvent * event)
{
    if (check())
    {
        remove_all_editors();
        remove_qperfedit();
        remove_all_live_frames();
        remove_set_master();
    }
    else
        event->ignore();
}

/**
 *  Pulls defaults from song frame.
 */

void
qsmainwnd::make_perf_frame_in_tab ()
{
    m_song_frame64 = new qperfeditframe64(perf(), ui->SongTab);
    if (not_nullptr(m_song_frame64))
    {
        int bpmeasure = m_song_frame64->get_beats_per_measure();
        int beatwidth = m_song_frame64->get_beat_width();
        ui->SongTabLayout->addWidget(m_song_frame64);
        ui->cmb_beat_length->setCurrentText(QString::number(beatwidth));
        ui->cmb_beat_measure->setCurrentText(QString::number(bpmeasure));
        if (not_nullptr(m_beat_ind))
        {
            ui->layout_beat_ind->addWidget(m_beat_ind);
            m_beat_ind->beat_width(beatwidth);
            m_beat_ind->beats_per_measure(bpmeasure);
        }
    }
}

/**
 *  Calls performer::stop_key() and unchecks the Play button.
 */

void
qsmainwnd::stop_playing ()
{
    perf().auto_stop();
    ui->btnPlay->setChecked(false);
}

/**
 *  Implements the pause button.
 */

void
qsmainwnd::pause_playing ()
{
    perf().auto_pause();
    ui->btnPlay->setChecked(perf().is_running());
}

/**
 *  Implements the play button.
 */

void
qsmainwnd::start_playing ()
{
    perf().auto_play();
    ui->btnPlay->setChecked(true);
}

/**
 *
 */

void
qsmainwnd::song_recording (bool record)
{
    perf().song_recording(record);
}

/**
 *  Sets the song mode, which is actually the JACK start mode.  If true, we
 *  are in playback/song mode.  If false, we are in live mode.  This
 *  function must be in the cpp module, where the button header file is
 *  included.
 */

void
qsmainwnd::set_song_mode (bool song_mode)
{
    if (song_mode)
    {
        ui->btnRecord->setEnabled(true);
        if (! usr().use_more_icons())
            ui->btnSongPlay->setText("Song");
    }
    else
    {
        song_recording(false);
        ui->btnRecord->setChecked(false);
        ui->btnRecord->setEnabled(false);
        if (! usr().use_more_icons())
            ui->btnSongPlay->setText("Live");
    }
    perf().song_mode(song_mode);    /* playback & song_start */
}

/**
 *  Toggles the song mode.  Note that calling this function will trigger the
 *  button signal callback, set_song_mode().  It only operates if the patterns
 *  are not playing.  This function must be in the cpp module, where the
 *  button header file is included.
 */

void
qsmainwnd::toggle_song_mode ()
{
    if (! perf().is_pattern_playing())
        ui->btnSongPlay->setEnabled(perf().toggle_song_mode());
}

/**
 *
 */

void
qsmainwnd::update_bpm (double bpm)
{
    perf().set_beats_per_minute(midibpm(bpm));
}

/**
 *
 */

void
qsmainwnd::edit_bpm ()
{
    double bpm = ui->spinBpm->value();
    perf().set_beats_per_minute(midibpm(bpm));
}

/**
 *
 */

void
qsmainwnd::show_open_file_dialog ()
{
    QString file;
    if (check())
    {
        file = QFileDialog::getOpenFileName
        (
            this, tr("Open MIDI/WRK file"), rc().last_used_dir().c_str(),
            tr
            (
                "MIDI files (*.midi *.mid *.MID);;"
                "WRK files (*.wrk *.WRK);;"
                "All files (*)"
            )
        );
    }
    if (! file.isEmpty())                   /* if the user did not cancel   */
        open_file(file.toStdString());
}

/**
 *  Opens the dialog to request a playlist.
 */

void
qsmainwnd::show_open_list_dialog ()
{
    QString file;
    if (check())
    {
        file = QFileDialog::getOpenFileName
        (
            this, tr("Open playlist file"), rc().last_used_dir().c_str(),
            tr("Playlist files (*.playlist);;All files (*)")
        );
    }
    if (! file.isEmpty())                           /* user did not cancel  */
    {
        std::string fname = file.toStdString();
        bool playlistmode = perf().open_playlist(fname, rc().verbose());
        if (playlistmode)
        {
            playlistmode = perf().open_current_song();
            m_playlist_frame->load_playlist();      /* update Playlist tab  */
#if defined SEQ66_PLATFORM_DEBUG_TMI
            perf().playlist_show();
#endif
        }
        else
        {
            QString msg_text = tr(perf().playlist_error_message().c_str());
            m_msg_error->showMessage(msg_text);
            m_msg_error->exec();
        }
    }
}

/**
 *  Also sets the current file-name and the last-used directory to the ones
 *  just loaded.
 *
 *  Compare this function to mainwnd::open_file() [the Gtkmm version]/
 */

void
qsmainwnd::open_file (const std::string & fn)
{
    std::string errmsg;
    int ppqn = m_ppqn;                      /* potential side-effect here   */
    bool result = perf().read_midi_file(fn, ppqn, errmsg);
    if (result)
    {
        redo_live_frame();

        /*
         * Update all of the children.  Doesn't seem to work for the edit
         * frames, may have to recreate them, or somehow hook in the new
         * sequence objects (as pointers, not references).  Probably an issue
         * to be ignored; the user will have to close and reopen the pattern
         * editor(s).
         */

        if (not_nullptr(m_song_frame64))
            m_song_frame64->update_sizes();

        if (not_nullptr(m_perfedit))
            m_perfedit->update_sizes();

#if defined USE_SEQEDIT_REDRAWING
        if (not_nullptr(m_edit_frame))
            m_edit_frame->update_draw_geometry();

        for (auto ei = m_open_editors.begin(); ei != m_open_editors.end(); ++ei)
        {
            qseqeditex * qep = ei->second;      /* save the pointer         */
            qep->update_draw_geometry();
        }
#else
        /*
         * The tabbed edit frame is automatically removed if no other seq-edit
         * is open.
         */

        remove_all_editors();
#endif

        update_recent_files_menu();
        m_is_title_dirty = true;
    }
    else
    {
        QString msg_text = tr(errmsg.c_str());
        m_msg_error->showMessage(msg_text);
        m_msg_error->exec();
    }
}

/*
 *  Reinitialize the "Live" frame.  Reconnect its signal, as we've made a new
 *  object.
 */

void
qsmainwnd::redo_live_frame ()
{
    ui->LiveTabLayout->removeWidget(m_live_frame);
    if (not_nullptr(m_live_frame))
        delete m_live_frame;

    if (usr().grid_is_button())
        m_live_frame = new qslivegrid(perf(), this, ui->LiveTab);
    else
        m_live_frame = new qsliveframe(perf(), this, ui->LiveTab);

    if (not_nullptr(m_live_frame))
    {
        ui->LiveTabLayout->addWidget(m_live_frame);
        connect_editor_slots();     /* external pattern editor window   */
        connect                     /* external live frame window       */
        (
            m_live_frame, SIGNAL(signal_live_frame(int)),
            this, SLOT(load_live_frame(int))
        );
        m_live_frame->show();
        m_live_frame->setFocus();

        /*
         * This is not necessary.  And it causes copies painter errors
         * since it bypasses paintEvent().  If we do need to redraw, call
         * m_live_frame->repaint() instead.
         *
         *  m_live_frame->redraw();
         */
    }
}

/**
 *
 */

void
qsmainwnd::update_window_title (const std::string & fn)
{
    std::string itemname = "unnamed";
    if (fn.empty())
    {
        itemname = perf().main_window_title(fn);
    }
    else
    {
        int pp = choose_ppqn();                 // choose_ppqn(ppqn());
        char temp[16];
        snprintf(temp, sizeof temp, " (%d ppqn) ", pp);
        itemname = fn;
        itemname += temp;
    }
    itemname += " [*]";                         // required by Qt 5

    QString fname = QString::fromLocal8Bit(itemname.c_str());
    setWindowTitle(fname);
}

/**
 *  Toggles the recording of the live song control done by the musician.
 *  This functionality currently does not have a key devoted to it, nor is it
 *  a saved setting.
 */

void
qsmainwnd::toggle_time_format (bool /*on*/)
{
    m_tick_time_as_bbt = ! m_tick_time_as_bbt;
    QString label = m_tick_time_as_bbt ? "B:B:T" : "H:M:S" ;
    ui->btn_set_HMS->setText(label);
}

/**
 *  Handles the external Set Master, the one instantiated with the Sets button
 *  on the main window.
 */

void
qsmainwnd::show_set_master ()
{
    if (is_nullptr(m_set_master))
    {
        /*
         * Show the set master in a separate window.  The "embedded" parameter
         * is set to false, and there is no parent widget, just this main window
         * as a parent.
         */

        m_set_master = new qsetmaster(perf(), false, this);
        if (not_nullptr(m_set_master))
            m_set_master->show();
    }
    else
        remove_set_master();
}

/**
 *
 */

void
qsmainwnd::remove_set_master ()
{
    if (not_nullptr(m_set_master))
    {
        delete m_set_master;
        m_set_master = nullptr;
    }
}

/**
 *  The debug statement shows us that the main-window size starts at
 *  920 x 680, goes to 800 x 480 (unscaled) briefly, and then back to
 *  920 x 680.
 */

void
qsmainwnd::refresh ()
{
    int active_screenset = int(perf().playscreen_number());
    std::string b = std::to_string(active_screenset);
    ui->entry_active_set->setText(b.c_str());
    if (ui->button_keep_queue->isChecked() != perf().is_keep_queue())
        ui->button_keep_queue->setChecked(perf().is_keep_queue());

    if (perf().is_pattern_playing())
    {
        /*
         * Calculate the current time, and display it.  Update beat indicator.
         */

        midipulse tick = perf().get_tick();
        midibpm bpm = perf().bpm();
        int ppqn = perf().ppqn();
        if (m_tick_time_as_bbt)
        {
            midi_timing mt
            (
                bpm, perf().get_beats_per_bar(), perf().get_beat_width(), ppqn
            );
            std::string t = pulses_to_measurestring(tick, mt);
            ui->label_HMS->setText(t.c_str());
        }
        else
        {
            std::string t = pulses_to_timestring(tick, bpm, ppqn, false);
            ui->label_HMS->setText(t.c_str());
        }
        if (not_nullptr(m_beat_ind))
            m_beat_ind->update();
    }
    else
    {
        /*
         * Use on_group_learn() to change this.
         *
        qt_set_icon
        (
            perf().is_group_learn() ? learn2_xpm : learn_xpm, ui->button_learn
        );
         */

        if (m_is_title_dirty)
        {
            if (not_nullptr(m_live_frame))
            {
                if (perf().playlist_mode())
                {
                    if (m_is_title_dirty)
                        m_live_frame->set_playlist_name(perf().playlist_song());
                }
                else
                    m_live_frame->set_playlist_name("");
            }
            m_is_title_dirty = false;
            update_window_title();
        }
    }
    if (perf().tap_bpm_timeout())
        set_tap_button(0);
}

/**
 *  Prompts the user to save the MIDI file.
 *
 * \return
 *      Returns true if the file was saved or the changes were "discarded" by
 *      the user.
 */

bool
qsmainwnd::check ()
{
    bool result = false;
    if (perf().modified())
    {
        int choice = m_msg_save_changes->exec();
        switch (choice)
        {
        case QMessageBox::Save:

            result = save_file();
            break;

        case QMessageBox::Discard:

            result = true;
            break;

        case QMessageBox::Cancel:
        default:

            break;
        }
    }
    else
        result = true;

    return result;
}

/**
 *  Prompts for a file-name, returning it as, what else, a C++ std::string.
 *
 * \param prompt
 *      The prompt to display.
 *
 * \return
 *      Returns the name of the file.  If empty, the user cancelled.
 */

std::string
qsmainwnd::filename_prompt (const std::string & prompt)
{
    std::string result;
    QString file = QFileDialog::getSaveFileName
    (
        this, tr(prompt.c_str()), rc().last_used_dir().c_str(),
        tr("MIDI files (*.midi *.mid);;All files (*)")
    );

    if (! file.isEmpty())
    {
        QFileInfo fileInfo(file);
        QString suffix = fileInfo.completeSuffix();
        if ((suffix != "midi") && (suffix != "mid"))
            file += ".midi";

        result = file.toStdString();
    }
    return result;
}

/**
 *
 * \todo
 *      Ensure proper reset on load.
 */

void
qsmainwnd::new_file ()
{
    if (check() && perf().clear_all(true))      /* also clear the playlist  */
    {
        m_is_title_dirty = true;
        redo_live_frame();
        remove_all_editors();
    }
}

/**
 *
 */

bool
qsmainwnd::save_file (const std::string & fname)
{
    bool result = false;
    std::string filename = fname.empty() ? rc().midi_filename() : fname ;
    if (filename.empty())
    {
        result = save_file_as();
    }
    else
    {
        bool is_wrk = file_extension_match(filename, "wrk");
        if (is_wrk)
        {
            std::ostringstream os;
            std::string wrkname = filename;
            filename = file_extension_set(filename, ".midi");
            os
                << "Will save the Cakewalk WRK file " << wrkname
                << " only in Seq66 format as " << filename
                ;
            report_message(os.str(), true);
        }

        std::string errmsg;
        result = write_midi_file(perf(), filename, errmsg);
        if (result)
        {
            rc().add_recent_file(filename);
            update_recent_files_menu();
        }
        else
        {
            m_msg_error->showMessage(errmsg.c_str());
            m_msg_error->exec();
        }
    }
    return result;
}

/**
 *
 */

bool
qsmainwnd::save_file_as ()
{
    bool result = false;
    std::string prompt = "Save MIDI file as...";
    std::string filename = filename_prompt(prompt);
    if (filename.empty())
    {
        // no code
    }
    else
    {
        result = save_file(filename);
        if (result)
        {
            rc().midi_filename(filename);
            m_is_title_dirty = true;
        }
    }
    return result;
}

/**
 *  Prompts for a file-name, then exports the current tune as a standard
 *  MIDI file, stripping out the Seq66 SeqSpec information.  Does not
 *  update the the current file-name, but does update the recent-file
 *  information at this time.
 *
 * \param fname
 *      The full path-name to the file to be written.  If empty (the default),
 *      then the user is prompted for the file-name.
 *
 * \return
 *      Returns true if the file was successfully written.
 */

bool
qsmainwnd::export_file_as_midi (const std::string & fname)
{
    bool result = false;
    std::string filename;
    if (fname.empty())
    {
        std::string prompt = "Export file as stock MIDI...";
        filename = filename_prompt(prompt);
    }
    else
        filename = fname;

    if (filename.empty())
    {
        /*
         * Maybe later, add some kind of warning dialog.
         */
    }
    else
    {
        midifile f(filename, choose_ppqn());
        bool result = f.write(perf(), false);           /* no SeqSpec   */
        if (result)
        {
            rc().add_recent_file(rc().midi_filename());
            update_recent_files_menu();
        }
        else
        {
            std::string errmsg = f.error_message();
            m_msg_error->showMessage(errmsg.c_str());
            m_msg_error->exec();
        }
    }
    return result;
}

/**
 *  Prompts for a file-name, then exports the current tune as a standard
 *  MIDI file, stripping out the Seq66 SeqSpec information.  Does not
 *  update the current file-name, but does update the the recent-file
 *  information at this time.
 *
 * \param fname
 *      The full path-name to the file to be written.  If empty (the default),
 *      then the user is prompted for the file-name.
 *
 * \return
 *      Returns true if the file was successfully written.
 */

bool
qsmainwnd::export_song (const std::string & fname)
{
    bool result = false;
    std::string filename;
    if (fname.empty())
    {
        std::string prompt = "Export Song as MIDI...";
        filename = filename_prompt(prompt);
    }
    else
        filename = fname;

    if (filename.empty())
    {
        /*
         * Maybe later, add some kind of warning dialog.
         */
    }
    else
    {
        midifile f(filename, choose_ppqn());
        bool result = f.write_song(perf());
        if (result)
        {
            rc().add_recent_file(filename);
            update_recent_files_menu();
        }
        else
        {
            std::string errmsg = f.error_message();
            m_msg_error->showMessage(errmsg.c_str());
            m_msg_error->exec();
        }
    }
    return result;
}

/**
 *
 */

void
qsmainwnd::show_import_dialog ()
{
    m_import_dialog->exec();
    QStringList filePaths = m_import_dialog->selectedFiles();

    /*
     * Rather than rely on the user remembering to set the destination
     * screen-set, prompt for the set/bank number.  Could make this a user
     * option at some point.
     *
     *  bool ok;
     *  int sset = QInputDialog::getInt
     *  (
     *      this, tr("Import to Bank"),
     *      tr("Destination screen-set/bank"), 1, 0, usr().max_sets() - 1, 1, &ok
     *  );
     */

    bool ok = filePaths.length() > 0;
    if (ok)
    {
        for (int i = 0; i < filePaths.length(); ++i)
        {
            QString path = m_import_dialog->selectedFiles()[i];
            if (! path.isEmpty())
            {
                try
                {
                    std::string fn = path.toStdString();
                    bool is_wrk = file_extension_match(fn, "wrk");
                    midifile * f = is_wrk ?
                        new wrkfile(fn) : new midifile(fn, ppqn()) ;

                    f->parse(perf(), int(perf().playscreen_number()));
                    ui->spinBpm->setValue(perf().bpm());
                    ui->spinBpm->setDecimals(usr().bpm_precision());
                    ui->spinBpm->setSingleStep(usr().bpm_step_increment());
                    if (not_nullptr(m_live_frame))
                        m_live_frame->set_bank(int(perf().playscreen_number()));
                }
                catch (...)
                {
                    QString msg_text =
                        "Error reading MIDI data from file: " + path;

                    m_msg_error->showMessage(msg_text);
                    m_msg_error->exec();
                }
            }
        }
    }
}

/**
 *
 */

void
qsmainwnd::showqsabout ()
{
    if (not_nullptr(m_dialog_about))
        m_dialog_about->show();
}

/**
 *
 */

void
qsmainwnd::showqsbuildinfo ()
{
    if (not_nullptr(m_dialog_build_info))
        m_dialog_build_info->show();
}

/**
 *  Loads the older Kepler34 pattern editor (qseqeditframe) for the selected
 *  sequence into the "Edit" tab.
 *
 *  We wanted to load the newer version, which has more functions, but it
 *  works somewhat differently, and cannot be fit into the current main window
 *  without enlarging it.  Therefore, we use the new version, qsegeditframe64,
 *  only in the external mode.
 *
 * \param seqid
 *      The slot value (0 to 1024) of the sequence to be edited.
 *
 * \warning
 *      Somehow, checking for not_nullptr(m_edit_frame) to determine whether
 *      to remove or add that widget causes the edit frame to not get created,
 *      and then the Edit tab is empty.
 *
 *  There are still some issues here:
 *
 *      -   Making sure that there is only one sequence editor window/tab
 *          per pattern.
 *      -   Removing the current song editor window/tab when another MIDI file
 *          is loaded.
 */

void
qsmainwnd::load_editor (int seqid)
{
    auto ei = m_open_editors.find(seqid);
    if (ei == m_open_editors.end())                         /* 1 editor/seq */
    {
        ui->EditTabLayout->removeWidget(m_edit_frame);      /* no ptr check */
        if (not_nullptr(m_edit_frame))
            delete m_edit_frame;

        m_edit_frame = new qseqeditframe(perf(), seqid, ui->EditTab);
        ui->EditTabLayout->addWidget(m_edit_frame);
        m_edit_frame->show();
        ui->tabWidget->setCurrentIndex(Tab_Edit);
    }
}

/**
 *
 */

void
qsmainwnd::load_event_editor (int seqid)
{
    auto ei = m_open_editors.find(seqid);
    if (ei == m_open_editors.end())                         /* 1 editor/seq */
    {
        ui->EventTabLayout->removeWidget(m_event_frame);    /* no ptr check */
        if (not_nullptr(m_event_frame))
            delete m_event_frame;

        /*
         * First, make sure the sequence exists.  Consider creating it if it
         * does not exist.
         */

        if (perf().is_seq_active(seqid))
        {
            m_event_frame = new qseqeventframe(perf(), seqid, ui->EventTab);
            ui->EventTabLayout->addWidget(m_event_frame);
            m_event_frame->show();
            ui->tabWidget->setCurrentIndex(Tab_Events);
        }
    }
}

/**
 *
 */

void
qsmainwnd::load_set_master ()
{
    qsetmaster * qsm = new qsetmaster(perf(), true, nullptr, ui->SetMasterTab);
    if (not_nullptr(qsm))
        ui->SetsTabLayout->addWidget(qsm);
}

/**
 *
 */

void
qsmainwnd::load_mute_master ()
{
    qmutemaster * qsm = new qmutemaster(perf(), nullptr, ui->MuteMasterTab);
    if (not_nullptr(qsm))
        ui->MutesTabLayout->addWidget(qsm);
}

/**
 *  Opens an external window for editing the sequence.  This window
 *  (qseqeditex with an embedded qseqeditframe64) is much more like the Gtkmm
 *  seqedit window, and somewhat more functional.  It has no parent widget,
 *  otherwise the whole big dialog will appear inside that parent.
 *
 * \warning
 *      We have to make sure the pattern ID is valid.  Somehow, we can
 *      double-click on the qsmainwnd's set/bank roller and get this function
 *      called!  For now, we work around that bug.
 *
 * \param seqid
 *      The slot value (0 to 1024) of the sequence to be edited.
 */

void
qsmainwnd::load_qseqedit (int seqid)
{
    if (perf().is_seq_active(seqid))
    {
        auto ei = m_open_editors.find(seqid);
        if (ei == m_open_editors.end())
        {
            /*
             * First, make sure the sequence exists.  We should consider
             * creating it if it does not exist.  So many features, so little
             * time.
             */

            if (perf().is_seq_active(seqid))
            {
                qseqeditex * ex = new qseqeditex(perf(), seqid, this);
                if (not_nullptr(ex))
                {
                    ex->show();
#if defined SEQ66_PLATFORM_CPP_11
                    std::pair<int, qseqeditex *> p = std::make_pair(seqid, ex);
#else
                    std::pair<int, qseqeditex *> p =
                        std::make_pair<int, qseqeditex *>(seqid, ex);
#endif
                    m_open_editors.insert(p);
                }
            }
        }
    }
}

/**
 *  Removes the editor window from the list.  This function is called by the
 *  editor window to tell its parent (this) that it is going away.  Note that
 *  this does not delete the editor, it merely removes the pointer to it.
 *
 * \param seqid
 *      The sequence number that the editor represented.
 */

void
qsmainwnd::remove_editor (int seqno)
{
    auto ei = m_open_editors.find(seqno);
    if (ei != m_open_editors.end())
        m_open_editors.erase(ei);
}

/**
 *  Uses the standard "associative-container erase-remove idiom".  Otherwise,
 *  the current iterator is invalid, and a segfault results in the top of the
 *  for-loop.  Another option with C++11 is "ci = m_open_editors.erase(ei)".
 */

void
qsmainwnd::remove_all_editors ()
{
    for (auto ei = m_open_editors.begin(); ei != m_open_editors.end(); /*++ei*/)
    {
        qseqeditex * qep = ei->second;      /* save the pointer             */
        m_open_editors.erase(ei++);         /* remove pointer, inc iterator */
        if (not_nullptr(qep))
            delete qep;                     /* delete the pointer           */
    }
}

/**
 * \param on
 *      Disabled for now.
 */

void
qsmainwnd::load_qperfedit (bool /*on*/)
{
    if (is_nullptr(m_perfedit))
    {
        qperfeditex * ex = new qperfeditex(perf(), this);
        if (not_nullptr(ex))
        {
            m_perfedit = ex;
            hide_qperfedit(false);

            /*
             * Leave it enabled now to do show versus hide to avoid a weird
             * segfault.
             *
             * ui->btnPerfEdit->setEnabled(false);
             */
        }
    }
    else
    {
        hide_qperfedit();
    }
}

/**
 *  Shows or hides the external performance editor window.  We use to just
 *  delete it, but somehow this started causing a segfault when X-ing
 *  (closing) that window.  So now we just keep it around until the
 *  application is exited.
 *
 * \param hide
 *      If true, the performance editor is unconditionally hidden.  Otherwise,
 *      it is shown if hidden, or hidden if showing.  The default value is
 *      false.
 */

void
qsmainwnd::hide_qperfedit (bool hide)
{
    if (not_nullptr(m_perfedit))
    {
        if (hide)
        {
            m_perfedit->hide();
            m_perf_frame_visible = false;
        }
        else
        {
            if (m_perf_frame_visible)
                m_perfedit->hide();
            else
                m_perfedit->show();

            m_perf_frame_visible = ! m_perf_frame_visible;
        }
    }
}

/**
 *  Removes the single song editor window.  This function is called by the
 *  editor window to tell its parent (this) that it is going away.
 */

void
qsmainwnd::remove_qperfedit ()
{
    if (not_nullptr(m_perfedit))
    {
        qperfeditex * tmp = m_perfedit;
        m_perfedit = nullptr;
        delete tmp;
        ui->btnPerfEdit->setEnabled(true);
    }
}

/**
 *  Opens an external live frame.  It has no parent widget, otherwise the whole
 *  big dialog will appear inside that parent.
 *
 * \param ssnum
 *      The screen-set value (0 to 31) of the live-frame to be displayed.
 */

void
qsmainwnd::load_live_frame (int ssnum)
{
    if (ssnum >= 0 && ssnum < usr().max_sets())
    {
        auto ei = m_open_live_frames.find(ssnum);
        if (ei == m_open_live_frames.end())
        {
            qliveframeex * ex = new qliveframeex(perf(), ssnum, this);
            if (not_nullptr(ex))
            {
                ex->show();
#if defined SEQ66_PLATFORM_CPP_11
                std::pair<int, qliveframeex *> p = std::make_pair(ssnum, ex);
#else
                std::pair<int, qliveframeex *> p =
                    std::make_pair<int, qliveframeex *>(ssnum, ex);
#endif
                m_open_live_frames.insert(p);
            }
        }
    }
}

/**
 *  Removes the live frame window from the list.  This function is called by
 *  the live frame window to tell its parent (this) that it is going away.
 *  Note that this does not delete the editor, it merely removes the pointer
 *  to it.
 *
 * \param ssnum
 *      The screen-set number that the live frame represented.
 */

void
qsmainwnd::remove_live_frame (int ssnum)
{
    auto ei = m_open_live_frames.find(ssnum);
    if (ei != m_open_live_frames.end())
        m_open_live_frames.erase(ei);
}

/**
 *  Uses the standard "associative-container erase-remove idiom".  Otherwise,
 *  the current iterator is invalid, and a segfault results in the top of the
 *  for-loop.  Another option with C++11 is "ci = m_open_editors.erase(ei)".
 */

void
qsmainwnd::remove_all_live_frames ()
{
    for
    (
        auto ei = m_open_live_frames.begin();
        ei != m_open_live_frames.end(); /*++ei*/
    )
    {
        qliveframeex * lep = ei->second;    /* save the pointer             */
        m_open_live_frames.erase(ei++);     /* remove pointer, inc iterator */
        if (not_nullptr(lep))
            delete lep;                     /* delete the pointer           */
    }
}

/**
 *
 */

void
qsmainwnd::update_beat_length (int blIndex)
{
    int bl;
    switch (blIndex)
    {
    case 0:
        bl = 1;
        break;

    case 1:
        bl = 2;
        break;

    case 2:
        bl = 4;
        break;

    case 3:
        bl = 8;
        break;

    case 4:
        bl = 16;
        break;

    default:
        bl = 4;
        break;
    }

    if (not_nullptr(m_song_frame64))
        m_song_frame64->set_beat_width(bl);

    if (not_nullptr(m_beat_ind))
        m_beat_ind->beat_width(bl);

    for (int i = 0; i < c_max_sequence; ++i) // set beat length, all sequences
    {
        seq::pointer seq = perf().get_sequence(i);
        if (seq)
        {
            /*
             * Set beat width, then reset the number of measures, causing
             * length to adjust to the new beats per measure.
             */

            seq->set_beat_width(bl);
            seq->set_measures(seq->get_measures());
        }
    }
    if (not_nullptr(m_edit_frame))
        m_edit_frame->update_draw_geometry();
}

/**
 *
 *  Also set beat length for all sequences.
 *  Reset number of measures, causing length to adjust to new b/m.
 */

void
qsmainwnd::updatebeats_per_measure(int bmindex)
{
    int bm = bmindex + 1;
    if (not_nullptr(m_beat_ind))
        m_beat_ind->beats_per_measure(bm);

    perf().set_beats_per_bar(bmindex + 1);
    for (int i = 0; i < c_max_sequence; ++i)
    {
        seq::pointer seq = perf().get_sequence(i);
        if (seq)
        {
            seq->set_beats_per_bar(bmindex + 1);
            seq->set_measures(seq->get_measures());
        }
    }
    if (not_nullptr(m_edit_frame))
        m_edit_frame->update_draw_geometry();
}

/**
 *  If we've selected the edit tab, make sure it has something to edit.
 *
 * \warning
 *      Somehow, checking for not_nullptr(m_edit_frame) to determine whether
 *      to remove or add that widget causes the edit frame to not get created,
 *      and then the Edit tab is empty.
 */

void
qsmainwnd::tabWidgetClicked (int newIndex)
{
    bool isnull = is_nullptr(m_edit_frame);
    seq::number seqid = perf().first_seq();         /* seq in playscreen?   */
    if (isnull)
    {
        if (newIndex == Tab_Edit)
        {
            bool ignore = false;
            if (seqid == seq::unassigned())         /* no, make a new one   */
            {
                /*
                 * This is too mysterious, and not sure we want to bother to
                 * update the live tab to show a new sequence.  Will think about
                 * it.
                 *
                 * seqid = perf().playscreen_offset();
                 * (void) perf().new_sequence(seqid);
                 */

                ignore = true;
            }

            if (! ignore)
            {
                seq::pointer seq = perf().get_sequence(seqid);
                if (seq)
                {
                    m_edit_frame = new qseqeditframe(perf(), seqid, ui->EditTab);
                    ui->EditTabLayout->addWidget(m_edit_frame);
                    m_edit_frame->show();
                    update();
                }
            }
        }
    }
    isnull = is_nullptr(m_event_frame);
    if (isnull)
    {
        if (newIndex == Tab_Events)
        {
            bool ignore = false;
            if (seqid == seq::unassigned())         /* no, make a new one   */
            {
                /*
                 * This is too mysterious, and not sure we want to bother to
                 * update the live tab to show a new sequence.  Will think about
                 * it.
                 *
                 * seqid = perf().playscreen_offset();
                 * (void) perf().new_sequence(seqid);
                 */

                ignore = true;
            }
            if (! ignore)
            {
                seq::pointer seq = perf().get_sequence(seqid);
                if (seq)
                {
                    m_event_frame = new qseqeventframe(perf(), seqid, ui->EditTab);
                    ui->EventTabLayout->addWidget(m_event_frame);
                    m_event_frame->show();
                    update();
                }
            }
        }
    }
}

/**
 *
 */

void
qsmainwnd::update_recent_files_menu ()
{
    int count = rc().recent_file_count();
    if (count > 0)
    {
        if (count > mc_max_recent_files)
            count = mc_max_recent_files;

        for (int f = 0; f < count; ++f)
        {
            std::string shortname = rc().recent_file(f);
            std::string longname = rc().recent_file(f, false);
            m_recent_action_list.at(f)->setText(shortname.c_str());
            m_recent_action_list.at(f)->setData(longname.c_str());
            m_recent_action_list.at(f)->setVisible(true);
        }
    }
    for (int fj = count; fj < mc_max_recent_files; ++fj)
        m_recent_action_list.at(fj)->setVisible(false);

    ui->menuFile->insertMenu(ui->actionSave, m_menu_recent);
}

/**
 *
 */

void
qsmainwnd::create_action_connections ()
{
    for (int i = 0; i < mc_max_recent_files; ++i)
    {
        QAction * action = new QAction(this);
        action->setVisible(false);
        QObject::connect
        (
            action, &QAction::triggered, this, &qsmainwnd::open_recent_file
        );
        m_recent_action_list.append(action);
    }
}

/**
 *  Deletes the recent-files menu and recreates it, insert it into the File
 *  menu.  Not sure why we picked create_action_menu() for the name of this
 *  function.
 */

void
qsmainwnd::create_action_menu ()
{
    if (not_nullptr(m_menu_recent) && m_menu_recent->isWidgetType())
        delete m_menu_recent;

    m_menu_recent = new QMenu(tr("&Recent MIDI Files..."), this);
    for (int i = 0; i < mc_max_recent_files; ++i)
    {
        m_menu_recent->addAction(m_recent_action_list.at(i));
    }
    ui->menuFile->insertMenu(ui->actionSave, m_menu_recent);
}

/**
 *  Opens the selected recent file.
 */

void
qsmainwnd::open_recent_file ()
{
    QAction * action = qobject_cast<QAction *>(sender());
    if (not_nullptr(action) && check())
    {
        QString fname = QVariant(action->data()).toString();
        std::string actionfile = fname.toStdString();
        if (! actionfile.empty())
            open_file(actionfile);
    }
}

/**
 *  Calls check(), and if it checks out (heh heh), remove all of the editor
 *  windows and then calls for an exit of the application.
 */

void
qsmainwnd::quit ()
{
    if (check())
    {
        remove_all_editors();
        QCoreApplication::exit();
    }
}

/**
 *  By experimenting, we see that qsliveframe gets all of the keystrokes.  So we
 *  moved much of the processing to that class.  If the event isn't handled
 *  there, then qsmainwnd::handle_key_press() is called.
 *
 *  If we reimplement this handler, it is very important to call
 * the base class implementation if the key is no acted on.
 */

void
qsmainwnd::keyPressEvent (QKeyEvent * event)
{
    QWidget::keyPressEvent(event);
}

/**
 *  Handles some hard-wired keystrokes.
 *
 *  The arrow keys support moving forward and backward through the playlists
 *  and the songs they specify.  These functions are also supported by the MIDI
 *  automation apparatus, but are not yet supported by the keystroke automation
 *  apparatus.
 *
 * \param k
 *      Provides a wrapper for the key event.
 */

bool
qsmainwnd::handle_key_press (const keystroke & k)
{
    bool result = false;
    if (k.is_right())
    {
        result = perf().open_next_song();
    }
    else if (k.is_left())
    {
        result = perf().open_previous_song();
    }
    else if (k.is_down())
    {
        result = perf().open_next_list();
    }
    else if (k.is_up())
    {
        result = perf().open_previous_list();
    }
    m_is_title_dirty = result;
    m_live_frame->set_playlist_name(perf().playlist_song());
    return result;
}

/**
 *  See qsliveframe::handle_key_release().
 */

bool
qsmainwnd::handle_key_release (const keystroke & /*k*/)
{
    return false;
}

/**
 *  Implements the "panic button".
 */

void
qsmainwnd::panic()
{
    perf().panic();
    ui->btnPlay->setChecked(perf().is_running());
}

/**
 *
 */

void
qsmainwnd::update_bank (int bankid)
{
#if defined USE_PRE_2019_09_02_CODE
    screenset::number setno = m_current_main_set; // perf().playscreen_number()
    if (bankid != int(setno))
    {
        setno = screenset::number(bankid);
        m_current_main_set = setno;
        bankid = int(perf().set_playing_screenset(setno));
        if (not_nullptr(m_live_frame))
            m_live_frame->update_bank(bankid);
    }
#else
        if (not_nullptr(m_live_frame))
            m_live_frame->update_bank(bankid);
#endif
}

/**
 *  Used to grab the std::string bank name and convert it to QString for
 *  display. Let performer set the modify flag, it knows when to do it.
 *  Otherwise, just scrolling to the next screen-set causes a spurious
 *  modification and an annoying prompt to a user exiting the application.
 */

void
qsmainwnd::update_bank_name (const QString &)
{
    if (not_nullptr(m_live_frame))
        m_live_frame->update_bank_name();
}

/**
 *
 */

void
qsmainwnd::show_message_box (const std::string & msg_text)
{
    if (not_nullptr(m_msg_error) && ! msg_text.empty())
    {
        QString msg = msg_text.c_str();     /* Qt still needs c_str()!! */
        m_msg_error->showMessage(msg);
        m_msg_error->exec();
    }
}

/**
 *
 */

void
qsmainwnd::connect_editor_slots ()
{
    connect         // connect to sequence-edit signal from the Live tab
    (
        m_live_frame, SIGNAL(signal_call_editor(int)),
        this, SLOT(load_editor(int))
    );
    connect         // new standalone sequence editor
    (
        m_live_frame, SIGNAL(signal_call_editor_ex(int)),
        this, SLOT(load_qseqedit(int))
    );

    /*
     * Event editor callback.  There is only one, for editing in the tab.
     * The event editor is meant for light use only at this time.
     */

    connect         // connect to sequence-edit signal from the Event tab
    (
        m_live_frame, SIGNAL(signal_call_edit_events(int)),
        this, SLOT(load_event_editor(int))
    );
}

/**
 *  Opens the Performance Editor (Song Editor).
 *
 *  We will let performer keep track of modifications, and not just set an
 *  is-modified flag just because we opened the song editor.  We're going to
 *  centralize the modification flag in the performer object, and see if it can
 *  work.
 */

void
qsmainwnd::open_performance_edit ()
{
    if (not_nullptr(m_perfedit))
    {
        if (m_perfedit->isVisible())
            m_perfedit->hide();
        else
            m_perfedit->show();
    }
    else
        load_qperfedit(true);
}

/**
 *  Apply full song transposition, if enabled.  Then reset the perfedit
 *  transpose setting to 0.
 */

void
qsmainwnd::apply_song_transpose ()
{
    if (perf().get_transpose() != 0)
    {
        perf().apply_song_transpose();
#if defined USE_THIS_CODE_IT_IS_READY
        m_perfedit->set_transpose(0);
#endif
    }
}

/**
 *  Reload all mute-group settings from the "rc" file.
 */

void
qsmainwnd::reload_mute_groups ()
{
    std::string errmessage;
    bool result = perf().reload_mute_groups(errmessage);
    if (! result)
    {
#if defined USE_THIS_CODE_IT_IS_READY
        Gtk::MessageDialog dialog           /* display the error-message    */
        (
            *this, "reload of mute groups", false,
            Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true
        );
        dialog.set_title("Mute Groups");
        dialog.set_secondary_text("Failed", false);
        dialog.run();
#endif
    }
}

/**
 *  Clear all mute-group settings.  Sets all values to false/zero.  Also,
 *  since the intent might be to clean up the MIDI file, the user is prompted
 *  to save.
 */

void
qsmainwnd::clear_mute_groups ()
{
    if (perf().clear_mute_groups())     /* did any mute statuses change?    */
    {
        if (check())
        {
            if (perf().is_running())
                stop_playing();
        }
    }
}

/**
 *  Sets the song-mute mode.
 */

void
qsmainwnd::set_song_mute_on ()
{
    perf().set_song_mute(mutegroups::muting::on);
    if (not_nullptr(m_live_frame))
        m_live_frame->set_needs_update();
}

/**
 *  Sets the song-mute mode.
 */

void
qsmainwnd::set_song_mute_off ()
{
    perf().set_song_mute(mutegroups::muting::off);
    if (not_nullptr(m_live_frame))
        m_live_frame->set_needs_update();
}

/**
 *  Sets the song-mute mode.
 */

void
qsmainwnd::set_song_mute_toggle ()
{
    perf().set_song_mute(mutegroups::muting::toggle);
    if (not_nullptr(m_live_frame))
        m_live_frame->set_needs_update();
}

/**
 *  Toggle the group-learn status.  Simply forwards the call to
 *  performer::learn_toggle().
 */

void
qsmainwnd::learn_toggle ()
{
    perf().learn_toggle();
    qt_set_icon
    (
        perf().is_group_learn() ? learn2_xpm : learn_xpm, ui->button_learn
    );
}

/**
 *  Implements the Tap button or Tap keystroke (defaults to F9).
 */

void
qsmainwnd::tap ()
{
    midibpm bpm = perf().update_tap_bpm();
    set_tap_button(perf().current_beats());
    if (perf().current_beats() > 1)             /* first one is useless */
    {
        ui->spinBpm->setValue(double(bpm));
    }
}

/**
 *  Sets the label in the Tap button to the given number of taps.
 *
 * \param beats
 *      The current number of times the user has clicked the Tap button/key.
 */

void
qsmainwnd::set_tap_button (int beats)
{
    char temp[8];
    snprintf(temp, sizeof temp, "%d", beats);
    ui->button_tap_bpm->setText(temp);
}

/**
 *  Implements the keep-queue button.
 */

void
qsmainwnd::queue_it ()
{
    bool is_active = ui->button_keep_queue->isChecked();
    perf().set_keep_queue(is_active);
}

/**
 *
 */

void
qsmainwnd::report_message (const std::string & msg, bool good)
{
    if (! msg.empty())
    {
        if (good)                           /* info message     */
        {
            QMessageBox * mbox = new QMessageBox(this);
            mbox->setText(tr(msg.c_str()));
            mbox->setInformativeText(tr("Click OK to continue."));
            mbox->setStandardButtons(QMessageBox::Ok);
            mbox->exec();
        }
        else                                /* error message    */
        {
            QErrorMessage * errbox = new QErrorMessage(this);
            errbox->showMessage(tr(msg.c_str()));
            errbox->exec();
        }
    }
}

/**
 *
 */

bool
qsmainwnd::on_group_learn (bool learning)
{
    qt_set_icon(learning ? learn2_xpm : learn_xpm, ui->button_learn);
    return true;
}

/**
 *
 */

bool
qsmainwnd::on_group_learn_complete (const keystroke & k, bool good)
{
    std::ostringstream os;
    if (good)
    {
        os
            << "MIDI mute group learn success, Mute group key '" << k.name()
            << "' (code = " << int(k.key()) << ") successfully mapped."
           ;
    }
    else
    {
        std::ostringstream os;
        os
            << "Key '" << k.name() << "' (code = " << int(k.key())
            << ") is not a configured mute-group key. "
            << "To add it, see the 'ctrl' file."
           ;
    }
    report_message(os.str(), good);
    return good;
}

/**
 *
 */

bool
qsmainwnd:: on_sequence_change (seq::number seqno)
{
    bool result = not_nullptr(m_live_frame);
    m_live_frame->update_sequence(seqno);
    return result;
}

/**
 *  This is not called when focus changes.  Instead, we have to call this from
 *  qliveframeex::changeEvent().
 */

void
qsmainwnd::changeEvent (QEvent * event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange)
    {
        if (isActiveWindow())
        {
            if (not_nullptr(m_live_frame))
                (void) perf().set_playing_screenset(m_live_frame->bank());
        }
        else
        {
            // widget is now inactive
        }
    }
}

/**
 *
 */

void
qsmainwnd::resizeEvent (QResizeEvent * /*r*/ )
{
#if defined SEQ66_PLATFORM_DEBUG_TMI
    printf("qsmainwnd::resizeEvent()\n");
#endif
}

/**
 *
 */

bool
qsmainwnd::recreate_all_slots ()
{
    bool result = not_nullptr(m_live_frame);
    if (result)
    {
        m_live_frame->set_playlist_name(perf().playlist_song());
        result = m_live_frame->recreate_all_slots();
    }
    return result;
}

}               // namespace seq66

/*
 * qsmainwnd.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


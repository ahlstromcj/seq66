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
 * \updates       2021-02-27
 * \license       GNU GPLv2 or above
 *
 *  The main window is known as the "Patterns window" or "Patterns
 *  panel".  It holds the "Pattern Editor" or "Sequence Editor".  The main
 *  window consists of two object:  mainwnd, which provides the user-interface
 *  elements that surround the patterns, and mainwid, which implements the
 *  behavior of the pattern slots.
 *
 *
 * Menu Entries for NSM:
 *
 *  New MIDI FIle   new_session()           Clear file/playlist, set new name
 *  Import [Open]   import_into_session()   Imports only a MIDI file
 *  Save session    save_session()          Save MIDI (and config?) in session
 *  Save As         HIDDEN                  See Export from Session
 *  Export from ... save_file_as()          Copy MIDI file outside of session
 *  Close           quit_session            Detach from session management
 *
 * Normal Menu Entries:
 *
 *  New             new_file()              Clear file/playlist
 *  Open            select_and_load_file()  Read file from anywhere
 *  Save            save_file()             Save file in original location
 *  Save As         save_file_as()          Save file anywhere, new name
 *  Recent files    open_recent_file()      Get a particular recent file
 *
 * Common Menu Entries:
 *
 *  Export Song     export_song()           Export song merging triggers
 *  Export as MIDI  export_file_as...()     Save as regular MIDI file
 *  Import MIDI     import_into_set()       Import MIDI into current set
 *  Quit/Exit       quit()                  Normal Qt application closing
 *  Help            showqsabout()           Show Help About (version info)
 *                  showqsbuildinfo()       Show features of the build
 */

#include <QErrorMessage>
#include <QFileDialog>                  /* prompt for full MIDI file's path */
#include <QInputDialog>                 /* prompt for NSM MIDI file-name    */
#include <QGuiApplication>              /* used for QScreen geometry() call */
#include <QMessageBox>
#include <QResizeEvent>
#include <QScreen>                      /* Qscreen                          */
#include <QStandardItemModel>           /* for disabling combobox entries   */
#include <QTimer>                       /* QTimer                           */
#include <sstream>                      /* std::ostringstream               */
#include <utility>                      /* std::make_pair()                 */

#include "cfg/settings.hpp"             /* seq66::usr() config functions    */
#include "ctrl/keystroke.hpp"           /* seq66::keystroke class           */
#include "midi/songsummary.hpp"         /* seq66::write_song_summary()      */
#include "midi/wrkfile.hpp"             /* seq66::wrkfile class             */
#include "qliveframeex.hpp"
#include "qmutemaster.hpp"              /* shows a map of mute-groups       */
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
#include "qsessionframe.hpp"            /* shows session information        */
#include "qsetmaster.hpp"               /* shows a map of all sets          */
#include "qskeymaps.hpp"                /* mapping between Gtkmm and Qt     */
#include "qsmaintime.hpp"
#include "qsmainwnd.hpp"
#include "qsliveframe.hpp"
#include "qslivegrid.hpp"
#include "qt5_helpers.hpp"              /* seq66::qt_set_icon() etc.        */
#include "sessions/smanager.hpp"        /* pulse_to_measurestring(), etc.   */
#include "util/calculations.hpp"        /* pulse_to_measurestring(), etc.   */
#include "util/filefunctions.hpp"       /* seq66::file_extension_match()    */

/*
 *  A signal handler is defined in daemonize.cpp, used for quick & dirty
 *  signal handling.  Thanks due to user falkTX!
 */

#include "os/daemonize.hpp"             /* session_close(), etc.            */

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

#define SEQ66_ERROR_BOX_WIDTH       600

/*
 * Don't document the namespace.
 */

namespace seq66
{

/**
 *  The default name of the current (if empty) tune.  Also refere to the
 *  function rc().session_midi_filename().
 */

const std::string s_default_tune = "newtune.midi";

/**
 *  Manifest constant to indicate the location of each main-window tab.
 */

static const int Tab_Live               =  0;
static const int Tab_Song               =  1;
static const int Tab_Edit               =  2;
static const int Tab_Events             =  3;
static const int Tab_Playlist           =  4;
static const int Tab_Set_Master         =  5;
static const int Tab_Mute_Master        =  6;
static const int Tab_Session            =  7;

/**
 *  Manifest constants for the beat-measure and beat-length combo-boxes.
 */

static const int s_beat_measure_count   = 16;
static const int s_beat_length_count    =  5;

/**
 *  Available PPQN values.  The default is 192, item #xx.  The first item uses the
 *  edit text for a "File" value, which means that whatever was read from the file
 *  is what holds.  The last item terminates the list.
 */

static const int s_ppqn_list [] =
{
       -1,      /* "Default" (SEQ66_USE_DEFAULT_PPQN), marked with asterisk */
        0,      /* "File" (SEQ66_USE_FILE_PPQN)                             */
       32,
       48,
       96,
      192,
      384,
      768,
      960,
     1920,
     3840,
     7680,
     9600,
    19200,
       -2       /* terminator   */
};

/**
 *  Given a display coordinate, looks up the screen and returns its geometry.
 *  If no screen was found, return the primary screen's geometry
 */

static QRect
desktop_rectangle (const QPoint & p)
{
    QList<QScreen *> screens = QGuiApplication::screens();
    Q_FOREACH(const QScreen *screen, screens)
    {
        if (screen->geometry().contains(p))
            return screen->geometry();
    }
    return QGuiApplication::primaryScreen()->geometry();
}

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
    bool usensm,
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
    m_menu_recent           (nullptr),          /* QMenu *                  */
    m_recent_action_list    (),                 /* QList<QAction *>         */
    mc_max_recent_files     (10),               /* constant                 */
    m_import_dialog         (nullptr),
    m_main_perf             (p),
    m_beat_ind              (nullptr),
    m_dialog_prefs          (nullptr),
    m_dialog_about          (nullptr),
    m_dialog_build_info     (nullptr),
    m_session_frame         (nullptr),
    m_set_master            (nullptr),
    m_mute_master           (nullptr),
    m_song_mode             (false),            /* perf().song_mode())      */
    m_use_nsm               (usensm),           /* use_nsm() accessor       */
    m_is_title_dirty        (true),
    m_tick_time_as_bbt      (true),
    m_previous_tick         (0),
    m_is_playing_now        (false),
    m_open_editors          (),
    m_open_live_frames      (),
    m_perf_frame_visible    (false),
    m_current_main_set      (0),
    m_session_mgr_ptr       (nullptr)
{
    ui->setupUi(this);

    QPoint pt;                                  /* default at (0, 0)        */
    QRect screen = desktop_rectangle(pt);       /* avoids deprecated func   */
    int x = (screen.width() - width()) / 2;     /* center on the screen     */
    int y = (screen.height() - height()) / 2;
    move(x, y);

    /*
     *  Combo-box for tweaking the PPQN.
     */

    bool ppqn_is_set = false;
    for (int i = 0; ; ++i)
    {
        int ppqn = s_ppqn_list[i];
        if (ppqn == SEQ66_USE_FILE_PPQN)
        {
            ui->cmb_ppqn->insertItem(i, "File");
        }
        else if (ppqn == SEQ66_USE_DEFAULT_PPQN)
        {
            std::string lbl = std::to_string(usr().midi_ppqn());
            lbl += "*";
            ui->cmb_ppqn->insertItem(i, lbl.c_str());
        }
        else if (ppqn >= SEQ66_MINIMUM_PPQN && ppqn <= SEQ66_MAXIMUM_PPQN)
        {
            QString combo_text = QString::number(ppqn);
            ui->cmb_ppqn->insertItem(i, combo_text);
            if (ppqn == perf().ppqn())
            {
                ui->cmb_ppqn->setCurrentIndex(i);
                ppqn_is_set = true;
            }
        }
        else if (ppqn == -2)
        {
            break;
        }
    }
    if (! ppqn_is_set)
        ui->cmb_ppqn->setCurrentIndex(0);

    std::string ppqnstr = std::to_string(perf().ppqn());
    ui->lineEditPpqn->setText(ppqnstr.c_str());
    ui->lineEditPpqn->setReadOnly(true);

    /*
     * Global output buss items.  Connected later on in this constructor.
     */

    const clockslist & opm = output_port_map();
    mastermidibus * mmb = perf().master_bus();
    ui->cmb_global_bus->addItem("None");
    if (not_nullptr(mmb))
    {
        int buses = opm.active() ?
            opm.count() : mmb->get_num_out_buses() ;

        for (int bus = 0; bus < buses; ++bus)
        {
            e_clock ec;
            std::string busname;
            if (perf().ui_get_clock(bussbyte(bus), ec, busname))
            {
                bool disabled = ec == e_clock::disabled;
                ui->cmb_global_bus->addItem(QString::fromStdString(busname));
                if (disabled)
                {
                    int index = bus + 1;
                    QStandardItemModel * model =
                        qobject_cast<QStandardItemModel *>
                        (
                            ui->cmb_global_bus->model()
                        );
                    QStandardItem * item = model->item(index);
                    item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
                }
            }
        }
    }

    /*
     * Fill options for beats per measure in the combo box, and set the
     * default.
     */

    for (int i = 0; i < s_beat_measure_count; ++i)
    {
        QString combo_text = QString::number(i + 1);
        ui->cmb_beat_measure->insertItem(i, combo_text);
    }

    /*
     * Fill options for beat length (beat width) in the combo box, and set the
     * default.  Note that the actual value is selected via a switch statement in
     * the update_beat_length() function.  See that function for the true story.
     */

    for (int i = 0; i < s_beat_length_count; ++i)
    {
        QString combo_text = QString::number(pow(2, i));
        ui->cmb_beat_length->insertItem(i, combo_text);
    }

    m_msg_save_changes = new QMessageBox(this);
    m_msg_save_changes->setText(tr("Unsaved changes detected."));
    m_msg_save_changes->setInformativeText(tr("Do you want to save them?"));
    m_msg_save_changes->setStandardButtons
    (
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
    );
    m_msg_save_changes->setDefaultButton(QMessageBox::Save);

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

    /*
     * File / Import MIDI to Current Set...
     */

    connect
    (
        ui->actionImport_MIDI, SIGNAL(triggered(bool)),
        this, SLOT(import_into_set())
    );
    m_import_dialog = new QFileDialog
    (
        this, tr("Import MIDI file to Current Set..."),
        rc().last_used_dir().c_str(),
        "MIDI files (*.midi *.mid);;WRK files (*.wrk);;All files (*)"
    );

    if (use_nsm())
        connect_nsm_slots();
    else
        connect_normal_slots();

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
    connect
    (
        ui->actionSongSummary, SIGNAL(triggered(bool)),
        this, SLOT(slot_summary_save())
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
    ui->btnSongPlay->setCheckable(false);
    if (usr().use_more_icons())
        qt_set_icon(live_mode_xpm, ui->btnSongPlay);

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
     * PPQN combo-box
     */

    connect
    (
        ui->cmb_ppqn, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_ppqn(int))
    );

    /*
     * Global buss combo-box
     */

    connect
    (
        ui->cmb_global_bus, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_midi_bus(int))
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
        this, SLOT(update_beats_per_measure(int))
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

    QString bname = perf().bank_name(0).c_str();
    ui->txtBankName->setText(bname);
    ui->spinBank->setRange(0, perf().screenset_max() - 1);

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
        this, SLOT(update_bank_text(QString))
    );

    connect
    (
        this, SIGNAL(signal_set_change(int)),
        this, SLOT(update_set_change(int))
    );

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
         * This scales the full GUI, cool!  However, it can be overridden by
         * the size of the new, larger, qseqeditframe64 frame.  We see the
         * normal-size window come up, and then it jumps to the larger size.
         */

        QSize s = size();
        int h = s.height();
        int w = s.width();
        int width = usr().scale_size(w);
        int height = usr().scale_size_y(h);
        resize(width, height);
        if (not_nullptr(m_live_frame))
            m_live_frame->repaint();
    }

    load_set_master();
    load_mute_master();
    load_session_frame();
    ui->tabWidget->setCurrentIndex(Tab_Live);
    ui->tabWidget->setTabEnabled(Tab_Events, false);    /* prevents issues  */

#if defined SEQ66_DISABLE_SESSION_TAB

    /*
     * We want to keep the session table accessible to show the current
     * configuration directory, etc.
     */

    if (! usr().wants_nsm_session())
    {
        /*
         * Which to do, remove or disable?
         * ui->tabWidget->removeTab(Tab_Session);
         */

        ui->tabWidget->setTabEnabled(Tab_Session, false);
    }

#endif

    /*
     * Test button.  This button supports whatever debugging we need to do at
     * any particular time.
     */


#if defined SEQ66_PLATFORM_DEBUG_SESSION_IMPORT
    ui->testButton->setToolTip("Developer test of MIDI 'Import into Session'.");
    ui->testButton->setEnabled(true);
    connect
    (
        ui->testButton, SIGNAL(clicked(bool)),
        this, SLOT(import_into_session())
    );
#else
    ui->testButton->setToolTip("Developer test button, disabled.");
    ui->testButton->setEnabled(false);
#endif

#if defined SEQ66_PLATFORM_DEBUG_PLAYLIST_SAVE
    ui->testButton->setToolTip("Test of saving/copying the current playlist.");
    ui->testButton->setEnabled(true);
    connect
    (
        ui->testButton, SIGNAL(clicked(bool)),
        this, SLOT(test_playlist_save())
    );
#endif

#if defined SEQ66_PLATFORM_DEBUG_NOTEMAP_SAVE
    ui->testButton->setToolTip("Test of saving the note-map.");
    ui->testButton->setEnabled(true);
    connect
    (
        ui->testButton, SIGNAL(clicked(bool)),
        this, SLOT(test_notemap_save())
    );
#endif

    if (use_nsm())
        rc().session_midi_filename(s_default_tune);

#if defined SEQ66_PORTMIDI_SUPPORT
    ui->alsaJackButton->setText("PortMidi");
    ui->jackTransportButton->hide();
#else
    QString midiengine = rc().with_jack_midi() ? "JACK" : "ALSA" ;
    QString jtrans = "None";
    if (cb_perf().is_jack_master())
        jtrans = "Master";
    else if (cb_perf().is_jack_slave())
        jtrans = "Slave";

    ui->alsaJackButton->setText(midiengine);
    ui->jackTransportButton->setText(jtrans);
#endif

    show();
    show_song_mode(m_song_mode);
    cb_perf().enregister(this);
    m_timer = new QTimer(this);
    m_timer->setInterval(3 * usr().window_redraw_rate());
    connect(m_timer, SIGNAL(timeout()), this, SLOT(refresh()));
    m_timer->start();
}

/**
 *  Destroys the user interface and removes any external qperfeditex that
 *  exists.
 */

qsmainwnd::~qsmainwnd ()
{
    if (not_nullptr(m_msg_error))
        delete m_msg_error;

    m_timer->stop();
    cb_perf().unregister(this);
    delete ui;
}

/*
 *  Note that the "use NSM" flag is set at construction time.  We don't need
 *  to do it here.
 *
 *      use_nsm(true);
 */

void
qsmainwnd::attach_session (smanager * sp)   // UNNECESSARY?
{
    if (not_nullptr(sp))
    {
        m_session_mgr_ptr = sp;

#if defined SEQ66_PLATFORM_DEBUG_CREATE_PROJECT_TEST
        std::string path("/home/ahlstrom/NSM Sessions/verbose/seq66.nSYPL");
        session()->create_project(path);
#endif
    }
    else
        use_nsm(false);
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
    ui->btnPause->setChecked(false);
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
    ui->btnPause->setChecked(false);
    ui->btnPlay->setChecked(true);
}

void
qsmainwnd::song_recording (bool record)
{
    perf().song_recording(record);
}

void
qsmainwnd::show_song_mode (bool songmode)
{
    if (songmode)
    {
        ui->btnRecord->setEnabled(false);
        if (! usr().use_more_icons())
            ui->btnSongPlay->setText("Song");
    }
    else
    {
        // ui->btnRecord->setChecked(false);
        ui->btnRecord->setEnabled(true);
        if (! usr().use_more_icons())
            ui->btnSongPlay->setText("Live");
    }
}


/**
 *  Sets the song mode, which is actually the JACK start mode.  If true, we
 *  are in playback/song mode.  If false, we are in live mode.  This
 *  function must be in the cpp module, where the button header file is
 *  included.
 */

void
qsmainwnd::set_song_mode (bool songmode)
{
    songmode = perf().toggle_song_mode();
    show_song_mode(songmode);
    song_recording(false);              /* unconditionally: if (!songmode) */
}

void
qsmainwnd::update_bpm (double bpm)
{
    perf().set_beats_per_minute(midibpm(bpm));
}

void
qsmainwnd::edit_bpm ()
{
    midibpm bpm = ui->spinBpm->value();
    perf().set_beats_per_minute(bpm);
}

void
qsmainwnd::slot_summary_save ()
{
    std::string fname = rc().midi_filename();       /* a full pathspec  */
    if (fname.empty())
    {
        // nothing to do yet
    }
    else
    {
        fname = file_extension_set(fname, ".text");
        if (show_text_file_dialog(this, fname))
            write_song_summary(perf(), fname);
    }
}

/**
 *  A test of playlist saving.
 */

#if defined SEQ66_PLATFORM_DEBUG_PLAYLIST_SAVE

void
qsmainwnd::test_playlist_save ()
{
    (void) perf().save_playlist();
    (void) perf().copy_playlist("~/tmp/playlists");
}

#endif

/**
 *  A test of note-map saving.
 */

#if defined SEQ66_PLATFORM_DEBUG_NOTEMAP_SAVE

void
qsmainwnd::test_notemap_save ()
{
    (void) perf().save_note_mapper();
}

#endif

/**
 *  For NSM usage, this function replaces the "Open" operation.  It will
 *  create a copy of the file which is then saved at the session path provided
 *  by NSM.  The question is, do we want to import configuration files as well
 *  as a MIDI file?  Well, no, because, when we first run qseq66 via NSM, we
 *  immediately create the $NSM_HOME/config directory and immediately copy the
 *  configuration files from $HOME/.config/seq66 there.
 *
 *  So here, we want to ask the user for the MIDI/WRK file, read it in, and
 *  then immediately save it to the $NSM_HOME/midi directory.
 */

void
qsmainwnd::import_into_session ()
{
    if (use_nsm())
    {
        std::string selectedfile;
        (void) load_into_session(selectedfile);
    }
}

bool
qsmainwnd::load_into_session (const std::string & selectedfile)
{
    bool result = false;
    std::string filename = selectedfile;
    if (show_open_file_dialog(filename))
    {
        if (open_file(filename))
        {
            /*
             * Change the filename to reflect the base NSM directory
             * and immediately save it.
             */

            std::string basename = filename_base(filename);
            rc().session_midi_filename(basename);   /* make NSM name  */

            std::string mfilename = rc().midi_filename();
            song_path(mfilename);
            std::string msg = save_file(mfilename, false) ?
                "Saved: " : "Failed to save: ";

            msg += rc().midi_filename();
            show_message_box(msg);
            m_is_title_dirty = false;
            result = true;
        }
        else
        {
            /* open_file() will show the error message. */
        }
    }
    return result;
}

/**
 *  Shows the "Open" file dialog.  If a file is selected, then the file is
 *  opened.
 */

void
qsmainwnd::select_and_load_file ()
{
    std::string selectedfile;
    if (show_open_file_dialog(selectedfile))
        (void) open_file(selectedfile);
}

/**
 *  Shows the "Open" file dialog, if not within an NSM session.  Otherwise,
 *  import_into_session() is called.
 */

bool
qsmainwnd::show_open_file_dialog (std::string & selectedfile)
{
    bool result = false;
    if (check())
        result = show_open_midi_file_dialog(this, selectedfile);

    return result;
}

/**
 *  Opens the dialog to request a playlist.  This action should be allowed
 *  in an NSM session.  This is a slot, which calls a member function that
 *  callers can call directly and get a boolean status, unlike this function.
 */

void
qsmainwnd::show_open_list_dialog ()
{
    if (check())
        (void) open_list_dialog();
}

bool
qsmainwnd::open_list_dialog ()
{
    std::string fname;
    bool result = show_playlist_dialog(this, fname, OpeningFile);
    if (result)
    {
        result = not_nullptr(m_playlist_frame);
        if (result)
        {
            result = m_playlist_frame->load_playlist(fname);
            if (! result)
                show_message_box(perf().playlist_error_message());
        }
    }
    return result;
}

/**
 *  Opens the dialog to save a playlist file.  This action should be allowed
 *  in an NSM session, but defaults to the configuration directory.
 */

void
qsmainwnd::show_save_list_dialog ()
{
    if (check())
        (void) save_list_dialog();
}

bool
qsmainwnd::save_list_dialog ()
{
    std::string fname;
    bool result = show_playlist_dialog(this, fname, SavingFile);
    if (result)
    {
        fname = file_extension_set(fname, ".playlist");
        result = perf().save_playlist(fname);
        if (result)
        {
            // performer will handle this: rc().playlist_filename(fname);
        }
        else
            show_message_box(perf().playlist_error_message());
    }
    return result;
}

/**
 *  Opens the dialog to request a mutegroups file.
 */

void
qsmainwnd::show_open_mutes_dialog ()
{
    (void) open_mutes_dialog();
}

bool
qsmainwnd::open_mutes_dialog ()
{
    std::string fname;
    bool result = show_file_dialog
    (
        this, fname, "Open mute-groups file",
        "Mutes-groups (*.mutes);;All (*)", OpeningFile, ConfigFile
    );
    if (result)
    {
        result = not_nullptr(m_mute_master);
        if (result)
        {
            result = m_mute_master->load_mutegroups(fname);
            if (! result)
                show_message_box("Mute-groups loading error");  // TODO
        }
        else
        {
            // what to do?
        }
    }
    return result;
}

void
qsmainwnd::show_save_mutes_dialog ()
{
    if (check())
        (void) save_mutes_dialog();
}

bool
qsmainwnd::save_mutes_dialog (const std::string & basename)
{
    std::string fname = basename;
    bool result = show_file_dialog
    (
        this, fname, "Save mute-groups file",
        "Mutes-groups (*.mutes);;All (*)", SavingFile, ConfigFile, ".mutes"
    );
    if (result)
    {
        result = not_nullptr(m_mute_master);
        if (result)
        {
            result = m_mute_master->save_mutegroups(fname);
            if (! result)
                show_message_box("Mute-groups saving error");  // TODO
        }
        else
        {
            // what to do?
        }
    }
    return result;
}

/**
 *  Also sets the current file-name and the last-used directory to the ones
 *  just loaded.
 */

bool
qsmainwnd::open_file (const std::string & fn)
{
    std::string errmsg;
    bool result = perf().read_midi_file(fn, errmsg);    /* updates recents  */
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

        if (! use_nsm())                        /* does this menu exist?    */
            update_recent_files_menu();

        m_is_title_dirty = true;
    }
    else
    {
        show_message_box(errmsg);
        update_recent_files_menu();
    }
    return result;
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
        int pp = choose_ppqn();
        char temp[16];
        snprintf(temp, sizeof temp, " (%d ppqn) ", pp);
        itemname = fn;
        itemname += temp;
    }
    itemname += " [*]";                             /* required by Qt 5 */

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

void
qsmainwnd::load_session_frame ()
{
    if (is_nullptr(m_session_frame))
    {
        qsessionframe * qsf = new qsessionframe(perf(), this, ui->SessionTab);
        if (not_nullptr(qsf))
        {
            ui->SessionTabLayout->addWidget(qsf);
            m_session_frame = qsf;
        }
    }
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
    if (session_close())
    {
        m_timer->stop();
        close();
        return;
    }
    if (session_save())
    {
        save_file();
    }

    int active_screenset = int(perf().playscreen_number());
    std::string b = "#";
    b += std::to_string(active_screenset);
    b += " of ";
    b += std::to_string(perf().screenset_count());
    ui->entry_active_set->setText(b.c_str());
    if (ui->button_keep_queue->isChecked() != perf().is_keep_queue())
        ui->button_keep_queue->setChecked(perf().is_keep_queue());

    if (m_song_mode != perf().song_mode())
    {
        m_song_mode = perf().song_mode();
        show_song_mode(m_song_mode);
    }

    midipulse tick = perf().get_tick();
    if (tick != m_previous_tick)
    {
        /*
         * Calculate the current time, and display it.  Update beat indicator.
         */

        m_previous_tick = tick;
        if (not_nullptr(m_beat_ind))
        {
            midibpm bpm = perf().bpm();
            int ppqn = perf().ppqn();
            if (m_tick_time_as_bbt)
            {
                midi_timing mt
                (
                    bpm, perf().get_beats_per_bar(),
                    perf().get_beat_width(), ppqn
                );
                std::string t = pulses_to_measurestring(tick, mt);
                ui->label_HMS->setText(t.c_str());
            }
            else
            {
                std::string t = pulses_to_timestring(tick, bpm, ppqn, false);
                ui->label_HMS->setText(t.c_str());
            }
            m_beat_ind->update();
        }
    }
    else
    {
        /*
         * We now use the on_group_learn() callback to change this.
         *
         *  qt_set_icon
         *  (
         *      perf().is_group_learn() ? learn2_xpm : learn_xpm,
         *      ui->button_learn
         *  );
         */

        if (m_is_title_dirty)
        {
            if (not_nullptr(m_live_frame))
            {
                if (perf().playlist_active())
                    m_live_frame->set_playlist_name(perf().playlist_song());
                else
                    m_live_frame->set_playlist_name(rc().midi_filename());
            }
            m_is_title_dirty = false;
            update_window_title();
        }
    }
    if (m_is_playing_now != perf().is_running())
    {
        m_is_playing_now = perf().is_running();
        ui->btnStop->setChecked(false);
        ui->btnPause->setChecked(false);
        ui->btnPlay->setChecked(m_is_playing_now);
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
    if (perf().modified() && ! use_nsm())
    {
        int choice = m_msg_save_changes->exec();
        switch (choice)
        {
        case QMessageBox::Save:

            result = save_file();
            break;

        case QMessageBox::Discard:

            perf().unmodify();          /* avoid saving in save_session()   */
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
 *      Returns the name of the file, which will include the path to the file.
 *      If empty, the user cancelled.
 */

std::string
qsmainwnd::filename_prompt (const std::string & prompt)
{
    std::string result = rc().last_used_dir();
    bool ok = show_file_dialog
    (
        this, result, prompt,
        "MIDI files (*.midi *.mid);;All files (*)", SavingFile, NormalFile,
        ".midi"
    );
    if (ok)
    {
        // nothing yet
    }
    return result;
}

/**
 *  We were passing true to performer::clear_all() to clear the playlist as
 *  well.  However, that doesn't clear the playlist frame, and the user may
 *  want to build a new tune to add to the playlist.
 */

void
qsmainwnd::new_file ()
{
    if (check() && perf().clear_all())              /* don't clear playlist */
    {
        m_is_title_dirty = true;
        redo_live_frame();
        remove_all_editors();
    }
}

/**
 *  This option will simply empty or reset the current file, after user
 *  confirmation.  According to NSM protocol, it cannot allow the user to
 *  create a new project/file in another location.  All this function does is
 *  set the file-name, if supplied, to be used later to save the MIDI file to
 *  the current NSM session.
 *
 *  After setting the file name, the code of the normal new_file() function is
 *  called, but without prompting to save the current MIDI file.  Any playlist
 *  loaded is also cleared from memory.
 *
 *  From the NSM API:
 *
 *      This option may empty/reset the current file or project (possibly
 *      after user confirmation). UNDER NO CIRCUMSTANCES should it allow the
 *      user to create a new project/file in another location.
 */

void
qsmainwnd::new_session ()
{
    if (use_nsm())
    {
        bool ok;
        std::string defname = rc().midi_filename();
        QString text = QInputDialog::getText
        (
            this, tr("New Session MIDI File"),      /* parent and title     */
            tr("MIDI FIle Base Name"),              /* input field label    */
            QLineEdit::Normal, defname.c_str(), &ok
        );
        if (ok)
        {
            if (perf().clear_all())                 /* like new_file()      */
            {
                m_is_title_dirty = true;
                redo_live_frame();
                remove_all_editors();
            }
            if (text.isEmpty())
            {
                file_message("Session MIDI file", "Cleared");
            }
            else
            {
                std::string filenamebase = text.toStdString();
                rc().session_midi_filename(filenamebase);
                file_message("Session MIDI file", rc().midi_filename());
            }
        }
    }
    else
    {
        // illegal
    }
}

/**
 *  This option, for NSM usage, saves the current file in the NSM location
 *  specified by the NSM "open" message.  According to NSM protocol, it cannot
 *  offer to play the file in another location.  For that purpose, see the
 *  "Export" menu entry.
 *
 *  The question here is what we want to save.  Here are the candidates:
 *
 *      -   The current MIDI file, if any.
 *      -   The "rc" file, along with any "ctrl", "mutes", and "playlist"
 *          files it specifies.
 *      -   The "usr" file.  Normally saved only if --user-save is specified.
 *
 *  I think this action should unconditionaly save them all.  No prompting.
 */

bool
qsmainwnd::save_session ()
{
    bool result = false;
    if (use_nsm())
    {
        if (not_nullptr(session()))
        {
            std::string msg;
            result = session()->save_session(msg);
            if (! result)
                show_message_box(msg);
        }
    }
    return result;
}

/**
 *  Not yet ready for prime time.
 */

bool
qsmainwnd::detach_session ()
{
    bool result = false;
    if (use_nsm())
    {
        if (not_nullptr(session()))
        {
            std::string msg;
            result = session()->detach_session(msg);
            if (result)
            {
                m_session_mgr_ptr = nullptr;
                use_nsm(false);
            }
            else
                show_message_box(msg);

        }
    }
    return result;
}

bool
qsmainwnd::save_file (const std::string & fname, bool updatemenu)
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
                << " in Seq66 format as " << filename
                ;
            report_message(os.str(), true);
        }

        std::string errmsg;
        result = write_midi_file(perf(), filename, errmsg);
        if (result)
        {
            if (updatemenu)                     /* or ! use_nsm()           */
                update_recent_files_menu();     /* add the recent file-name */
        }
        else
            show_message_box(errmsg);
    }
    return result;
}

bool
qsmainwnd::save_file_as ()
{
    bool result = false;
    std::string prompt = use_nsm() ?
        "Export MIDI file from NSM session as..." : "Save MIDI file as..." ;

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
 *  This function is equivalent to export_song(), except it calls midifile ::
 *  write() instead of midifile :: write_song().
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
        std::string prompt = "Export file as standard MIDI...";
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
        bool result = f.write(perf(), false);           /* no SeqSpec       */
        if (result)
        {
            rc().add_recent_file(rc().midi_filename()); /* not in write()   */
            update_recent_files_menu();
        }
        else
            show_message_box(f.error_message());
    }
    return result;
}

/**
 *  Prompts for a file-name, then exports the current tune as a standard
 *  MIDI file, stripping out the Seq66 SeqSpec information.  Does not
 *  update the current file-name, but does update the the recent-file
 *  information at this time.
 *
 *  This function is ESSENTIALLY EQUIVALENT to export_file_as_midi()!!!
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
            show_message_box(f.error_message());
    }
    return result;
}

void
qsmainwnd::import_into_set ()
{
    m_import_dialog->exec();

    QStringList filePaths = m_import_dialog->selectedFiles();
    if (filePaths.length() > 0)
    {
        for (int i = 0; i < filePaths.length(); ++i)
        {
            QString path = m_import_dialog->selectedFiles()[i];
            if (! path.isEmpty())
            {
                try
                {
                    int setno = int(perf().playscreen_number());
                    std::string fn = path.toStdString();
                    bool is_wrk = file_extension_match(fn, "wrk");
                    midifile * f = is_wrk ?
                        new wrkfile(fn) : new midifile(fn, choose_ppqn()) ;

                    if (f->parse(perf(), setno))
                    {
                        ui->spinBpm->setValue(perf().bpm());
                        ui->spinBpm->setDecimals(usr().bpm_precision());
                        ui->spinBpm->setSingleStep(usr().bpm_step_increment());
                        update_bank(setno);
                    }
                }
                catch (...)
                {
                    std::string p = path.toStdString();
                    std::string msg = "Error reading MIDI data from file: " + p;
                    show_message_box(msg);
                }
            }
        }
    }
}

void
qsmainwnd::showqsabout ()
{
    if (not_nullptr(m_dialog_about))
        m_dialog_about->show();
}

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
    seq::pointer seq = perf().get_sequence(seqid);
    bool ok = bool(seq);

#if defined DISALLOW_EDITOR_CONFLICT
    if (ok)
    {
        auto ei = m_open_editors.find(seqid);
        ok = ei == m_open_editors.end();                    /* 1 editor/seq */
    }
#endif

    if (ok)
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
 *  This function first looks to see if a piano roll editor is already open for
 *  this sequence.  If so, we will not open the event-editor frame, to avoid
 *  conflicts.
 */

void
qsmainwnd::load_event_editor (int seqid)
{
    seq::pointer seq = perf().get_sequence(seqid);
    bool ok = bool(seq);

#if defined DISALLOW_EDITOR_CONFLICT
    if (ok)
    {
        auto ei = m_open_editors.find(seqid);
        ok = ei == m_open_editors.end();                    /* 1 editor/seq */
    }
#endif

    if (ok)
    {
        if (make_event_frame(seqid))
        {
            ui->tabWidget->setTabEnabled(Tab_Events, true);
            ui->tabWidget->setCurrentIndex(Tab_Events);
            update();
        }
    }
}

void
qsmainwnd::load_set_master ()
{
    qsetmaster * qsm = new (std::nothrow)
        qsetmaster(perf(), true, this, ui->SetMasterTab);

    if (not_nullptr(qsm))
    {
        ui->SetsTabLayout->addWidget(qsm);
        m_set_master = qsm;
    }
}

void
qsmainwnd::load_mute_master ()
{
    qmutemaster * qsm = new (std::nothrow)
        qmutemaster(perf(), this, ui->MuteMasterTab);

    if (not_nullptr(qsm))
    {
        ui->MutesTabLayout->addWidget(qsm);
        m_mute_master = qsm;
    }
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
             * Make sure the sequence exists.  We should consider creating it if
             * it does not exist.  So many features, so little time.
             */

            if (perf().is_seq_active(seqid))
            {
                qseqeditex * ex = new (std::nothrow)
                    qseqeditex(perf(), seqid, this);

                if (not_nullptr(ex))
                {
                    ex->show();
                    std::pair<int, qseqeditex *> p = std::make_pair(seqid, ex);
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
    {
        m_open_editors.erase(ei);

        /*
         * Deleting this pointer makes qseq66 segfault, and valgrind doesn't
         * seem to show any leak.  Commented out.  This fixes issue #4.  It
         * also needs to be backported to Sequencer64.
         *
         * qseqeditex * qep = ei->second;   // save the pointer
         * if (not_nullptr(qep))
         *     delete qep;                  // delete the pointer
         */
    }
}

/**
 *  Uses the standard "associative-container erase-remove idiom".  Otherwise,
 *  the current iterator is invalid, and a segfault results in the top of the
 *  for-loop.  Another option with C++11 is "ci = m_open_editors.erase(ei)".
 */

void
qsmainwnd::remove_all_editors ()
{
    /*
     * New clause ca 2021-01-31.  Helps with File / New and Event Editor
     * interactions.
     */

    if (not_nullptr(m_event_frame))
    {
        delete m_event_frame;
        m_event_frame = nullptr;
        ui->EventTabLayout->removeWidget(m_event_frame);
        ui->tabWidget->setTabEnabled(Tab_Events, false);
    }
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
        qperfeditex * ex = new (std::nothrow) qperfeditex(perf(), this);
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
    if (ssnum >= 0 && ssnum < perf().screenset_max())
    {
        auto ei = m_open_live_frames.find(ssnum);
        if (ei == m_open_live_frames.end())
        {
            qliveframeex * ex = new (std::nothrow)
                qliveframeex(perf(), ssnum, this);

            if (not_nullptr(ex))
            {
                ex->show();
                std::pair<int, qliveframeex *> p = std::make_pair(ssnum, ex);
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
    for (auto i = m_open_live_frames.begin(); i != m_open_live_frames.end() ; )
    {
        qliveframeex * lep = i->second;     /* save the pointer             */
        m_open_live_frames.erase(i++);      /* remove pointer, inc iterator */
        if (not_nullptr(lep))
            delete lep;                     /* delete the pointer           */
    }
}

void
qsmainwnd::update_ppqn (int pindex)
{
    int p = s_ppqn_list[pindex];
    if (p > 0)
    {
        if (p == SEQ66_USE_FILE_PPQN)
        {
            p = usr().file_ppqn();
        }
        else if (p == SEQ66_USE_DEFAULT_PPQN)
        {
            p = usr().midi_ppqn();
        }
        if (perf().change_ppqn(p))
        {
            std::string ppqnstr = std::to_string(p);
            ui->lineEditPpqn->setText(ppqnstr.c_str());
        }
    }
}

/**
 *  Sets the MIDI bus to use for output for the current set and its sequences.
 *  A convenience when dealing with one MIDI output device.  Must be applied
 *  individually to each set; this allows each set to drive a different buss.
 *
 * \param index
 *      The index into the list of available MIDI buses.
 */

void
qsmainwnd::update_midi_bus (int index)
{
    mastermidibus * mmb = perf().master_bus();
    if (not_nullptr(mmb))
    {
        if (index == 0)
        {
            // Anything to do for the "None" entry?
        }
        else
        {
            --index;
            (void) perf().ui_change_set_bus(index);
        }
    }
}

/**
 *  We could set the value using pow(2, blindex).
 */

void
qsmainwnd::update_beat_length (int blindex)
{
    int bl;
    switch (blindex)
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

    for (int i = 0; i < perf().sequence_max(); ++i)
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
 *  Sets the beats-per-measure for the beat indicator and the performer.
 *  Also sets the beat length for all sequences, and
 *  resets the number of measures, causing length to adjust to new b/m.
 *
 * \param bmindex
 *      Provides the index into the list of beats.  This number is one less
 *      than the actual beats-per-measure represent by that index.
 */

void
qsmainwnd::update_beats_per_measure (int bmindex)
{
    int bm = bmindex + 1;
    if (perf().set_beats_per_measure(bm))
    {
        if (not_nullptr(m_beat_ind))
            m_beat_ind->beats_per_measure(bm);

        if (not_nullptr(m_edit_frame))
            m_edit_frame->update_draw_geometry();
    }
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
qsmainwnd::tabWidgetClicked (int newindex)
{
    bool isnull = is_nullptr(m_edit_frame);
    seq::number seqid = perf().first_seq();         /* seq in playscreen?   */
    if (isnull)
    {
        if (newindex == Tab_Edit)
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
                    m_edit_frame = new (std::nothrow)
                        qseqeditframe(perf(), seqid, ui->EditTab);

                    if (not_nullptr(m_edit_frame))
                    {
                        ui->EditTabLayout->addWidget(m_edit_frame);
                        m_edit_frame->show();
                        update();
                    }
                }
            }
        }
    }
    isnull = is_nullptr(m_event_frame);
    if (isnull)
    {
        if (newindex == Tab_Events)
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
                if (make_event_frame(seqid))
                {
                    ui->tabWidget->setCurrentIndex(Tab_Events);
                    update();
                }
            }
        }
    }
}

/**
 *  First, make sure the sequence exists.  Consider creating it if it does not
 *  exist.
 */

bool
qsmainwnd::make_event_frame (int seqid)
{
    seq::pointer seq = perf().get_sequence(seqid);
    bool result = bool(seq);
    if (result)
    {
        if (not_nullptr(m_event_frame))
        {
            ui->EventTabLayout->removeWidget(m_event_frame);
            delete m_event_frame;
        }
        m_event_frame = new (std::nothrow)
            qseqeventframe(perf(), seqid, ui->EventTab);

        if (not_nullptr(m_event_frame))
        {
            ui->EventTabLayout->addWidget(m_event_frame);
            m_event_frame->show();
        }
    }
    return result;
}

void
qsmainwnd::update_recent_files_menu ()
{
    int count = rc().recent_file_count();
    if (count > 0)
    {
        if (count > mc_max_recent_files)
            count = mc_max_recent_files;

        bool ok = true;
        for (int f = 0; f < count; ++f)
        {
            std::string shortname = rc().recent_file(f);
            if (! shortname.empty())
            {
                std::string longname = rc().recent_file(f, false);
                m_recent_action_list.at(f)->setText(shortname.c_str());
                m_recent_action_list.at(f)->setData(longname.c_str());
                m_recent_action_list.at(f)->setVisible(true);
            }
            else
            {
                ok = false;
                break;
            }
        }
        if (ok)
        {
            for (int fj = count; fj < mc_max_recent_files; ++fj)
                m_recent_action_list.at(fj)->setVisible(false);

            ui->menuFile->insertMenu(ui->actionSave, m_menu_recent);
        }
    }
}

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
            (void) open_file(actionfile);
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
 *  Calls check(), and if it checks out (heh heh), removes all of the editor
 *  windows and then calls for an exit of the application.  It "detaches" from
 *  the session.  To do that, we need to:
 *
 *      -   Tell the session manager that we are leaving the session.  Is this
 *          permanent or just temporary?
 *      -   Detach from the session:  nullify the pointer and reset the
 *          session flag.
 *      -   Recreate the main window menus.
 *
 *  From the NSM API:
 *
 *      This option MUST be disabled unless its meaning is to disconnect
 *      the application from session management.
 */

void
qsmainwnd::quit_session ()
{
    if (use_nsm())
    {
        if (check())
        {
            if (detach_session()) // currently causes an INCOMPLETE Quit later
            {
                use_nsm(false);
                disconnect_nsm_slots();
                connect_normal_slots();
            }
        }
    }
}

/**
 *  By experimenting, we see that qsliveframe gets all of the keystrokes.  So
 *  we moved much of the processing to that class.  If the event isn't handled
 *  there, then qsmainwnd::handle_key_press() is called.
 *
 *  If we reimplement this handler, it is very important to call the base class
 *  implementation if the key is no acted on.
 *
 *  ca 2020-11-22 Refactoring keystroke handling between the main window and the
 *  live grid/frame.
 */

void
qsmainwnd::keyPressEvent (QKeyEvent * event)
{
    keystroke k = qt_keystroke(event, SEQ66_KEYSTROKE_PRESS);
    bool done = handle_key_press(k);
    if (done)
        update();
    else
        QWidget::keyPressEvent(event);          /* event->ignore()?     */
}

void
qsmainwnd::keyReleaseEvent (QKeyEvent * event)
{
    keystroke k = qt_keystroke(event, SEQ66_KEYSTROKE_RELEASE);
    bool done = handle_key_release(k);
    if (done)
        update();
    else
        QWidget::keyReleaseEvent(event);            /* event->ignore()?     */
}

/**
 *  Handles some hard-wired keystrokes.  If they aren't handled, then MIDI
 *  control-key processing is performed.
 *
 *  The arrow keys support moving forward and backward through the playlists
 *  and the songs they specify.  These functions are also supported by the MIDI
 *  automation apparatus, but are not yet supported by the keystroke automation
 *  apparatus. [Is this still true???]
 *
 *  Note that changing the playlist name will cause a reupdate of all of the
 *  window, including the pattern buttons in qslivegrid, causing flickering of
 *  all armed pattern buttons.
 *
 * \param k
 *      Provides a wrapper for the key event.
 */

bool
qsmainwnd::handle_key_press (const keystroke & k)
{
    bool result = false;
    bool done = false;
    if (k.is_press())
    {
        if (k.is_right())
        {
            result = perf().open_next_song();
            done = true;
        }
        else if (k.is_left())
        {
            result = perf().open_previous_song();
            done = true;
        }
        else if (k.is_down())
        {
            result = perf().open_next_list();
            done = true;
        }
        else if (k.is_up())
        {
            result = perf().open_previous_list();
            done = true;
        }
        if (result)
        {
            if (perf().playlist_active())
                m_live_frame->set_playlist_name(perf().playlist_song());
        }
    }
    if (! done)
    {
        done = perf().midi_control_keystroke(k);
        if (perf().seq_edit_pending())
        {
            done = true;
        }
        else if (perf().event_edit_pending())
        {
            done = true;
        }
    }
    m_is_title_dirty = result;
    return done;
}

/**
 *  See qsliveframe::handle_key_release().
 */

bool
qsmainwnd::handle_key_release (const keystroke & k)
{
    bool result = ! k.is_press();
    if (result)
        result = perf().midi_control_keystroke(k);

    return result;
}

/**
 *  Implements the "panic button".
 */

void
qsmainwnd::panic()
{
    if (perf().panic())
    {
        ui->btnStop->setChecked(false);
        ui->btnPause->setChecked(false);
        ui->btnPlay->setChecked(false);
    }
}

/**
 *  The qslivebase::update_bank() function and its overrides do not take care
 *  of changing the performer's playing screenset; the live-frame does that.
 */

void
qsmainwnd::update_bank (int bankid)
{
    if (not_nullptr(m_live_frame))
    {
        m_live_frame->update_bank(bankid);

        /*
         * Done in the call above:
         * (void) perf().set_playing_screenset(m_live_frame->bank());
         */
    }
}

/**
 *  Used to grab the std::string bank name and convert it to QString for
 *  display. Let performer set the modify flag, it knows when to do it.
 *  Otherwise, just scrolling to the next screen-set causes a spurious
 *  modification and an annoying prompt to a user exiting the application.
 */

void
qsmainwnd::update_bank_text (const QString & newname)
{
    if (not_nullptr(m_live_frame))
    {
        std::string name = newname.toStdString();
        m_live_frame->update_bank_name(name);
    }
}

void
qsmainwnd::show_message_box (const std::string & msg_text)
{
    if (! msg_text.empty())
    {
        m_msg_error = new (std::nothrow) QErrorMessage(this);
        if (not_nullptr(m_msg_error))
        {
            QString msg = msg_text.c_str();             /* Qt needs c_str() */
            QSpacerItem * hspace = new QSpacerItem
            (
                SEQ66_ERROR_BOX_WIDTH, 0,
                QSizePolicy::Minimum, QSizePolicy::Expanding
            );
            QGridLayout * lay = (QGridLayout *) m_msg_error->layout();
            lay->addItem(hspace, lay->rowCount(), 0, 1, lay->columnCount());
            m_msg_error->showMessage(msg);
            m_msg_error->exec();
            delete m_msg_error;
            m_msg_error = nullptr;
        }
    }
}

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

void
qsmainwnd::connect_nsm_slots ()
{
    if (rc().verbose())
    {
        infoprint("Connecting NSM File menu entries");
    }

    /*
     * File / New.  NSM version.
     */

    ui->actionNew->setText("&New MIDI File...");
    ui->actionNew->setToolTip("Clear and set a new MIDI file in session.");
    connect
    (
        ui->actionNew, SIGNAL(triggered(bool)),
        this, SLOT(new_session())
    );

    /*
     * File / Import into Session. Do we need an "Open Session"?
     */

    ui->actionOpen->setText("&Import into Session...");
    ui->actionOpen->setToolTip
    (
        "Import a MIDI or Seq66 MIDI file into the current session."
    );
    connect
    (
        ui->actionOpen, SIGNAL(triggered(bool)),
        this, SLOT(import_into_session())
    );

    /*
     * File / Save Session.
     */

    ui->actionSave->setText("&Save");
    ui->actionSave->setToolTip
    (
        "Save the current MIDI file and the configuration in the session."
    );
    connect
    (
        ui->actionSave, SIGNAL(triggered(bool)),
        this, SLOT(save_session())
    );

    /*
     * File / Save As...
     */

    ui->actionSave_As->setVisible(false);

    /*
     * File / Export from Session
     */

    ui->actionSave_As->setText("&Export from Session...");
    ui->actionSave_As->setToolTip("Export as a Seq66 MIDI file.");
    connect
    (
        ui->actionSave_As, SIGNAL(triggered(bool)),
        this, SLOT(save_file_as())
    );

    /*
     * File / Close.
     */

    ui->actionClose->setText("&Detach Session");
    ui->actionClose->setToolTip("Detach from session management.");
    connect
    (
        ui->actionClose, SIGNAL(triggered(bool)),
        this, SLOT(quit_session())
    );
    ui->actionClose->setEnabled(false);
}

void
qsmainwnd::disconnect_nsm_slots ()
{
    disconnect
    (
        ui->actionNew, SIGNAL(triggered(bool)), this, SLOT(new_session())
    );
    disconnect
    (
        ui->actionOpen, SIGNAL(triggered(bool)),
        this, SLOT(import_into_session())
    );
    disconnect
    (
        ui->actionSave, SIGNAL(triggered(bool)), this, SLOT(save_session())
    );
    disconnect
    (
        ui->actionSave_As, SIGNAL(triggered(bool)), this, SLOT(save_file_as())
    );
    disconnect
    (
        ui->actionClose, SIGNAL(triggered(bool)), this, SLOT(quit_session())
    );
}

void
qsmainwnd::connect_normal_slots ()
{
    if (rc().verbose())
    {
        infoprint("Connecting normal File menu entries");
    }

    /*
     * File / New.  Connect the GUI elements to event handlers.
     */

    ui->actionNew->setText("&New");
    ui->actionNew->setToolTip("Clear MIDI data to make a new Seq66 tune.");
    connect(ui->actionNew, SIGNAL(triggered(bool)), this, SLOT(new_file()));

    /*
     * File / Open.
     */

#if defined SEQ66_PLATFORM_DEBUG_TEST_SESSION_IMPORT
    ui->actionOpen->setText("&Import into Session...");
    ui->actionOpen->setToolTip
    (
        "Import a MIDI or Seq66 MIDI file into the current session."
    );
    connect
    (
        ui->actionOpen, SIGNAL(triggered(bool)),
        this, SLOT(import_into_session())
    );
#else
    ui->actionOpen->setText("&Open...");
    ui->actionOpen->setToolTip("Open a standard or Seq66 MIDI file.");
    connect
    (
        ui->actionOpen, SIGNAL(triggered(bool)),
        this, SLOT(select_and_load_file())
    );
#endif

    /*
     * File / Save
     */

    ui->actionSave->setText("&Save");
    ui->actionSave->setToolTip("Save as a Seq66 MIDI file.");
    connect(ui->actionSave, SIGNAL(triggered(bool)), this, SLOT(save_file()));

    /*
     * File / Save As...
     */

    ui->actionSave_As->setText("Save &As...");
    ui->actionSave_As->setToolTip("Save as a different Seq66 MIDI file.");
    connect
    (
        ui->actionSave_As, SIGNAL(triggered(bool)),
        this, SLOT(save_file_as())
    );

    /*
     * File / Close.  Hide it.  Used only for NSM; otherwise the stock
     * Quit enty is used.
     */

    ui->actionClose->setVisible(false);

    /*
     * File / Recent MIDI files
     */

    create_action_connections();
    create_action_menu();
    update_recent_files_menu();
}

void
qsmainwnd::disconnect_normal_slots ()
{
    disconnect(ui->actionNew, SIGNAL(triggered(bool)), this, SLOT(new_file()));
    disconnect
    (
        ui->actionOpen, SIGNAL(triggered(bool)),
        this, SLOT(select_and_load_file())
    );
    disconnect(ui->actionSave, SIGNAL(triggered(bool)), this, SLOT(save_file()));
    disconnect
    (
        ui->actionSave_As, SIGNAL(triggered(bool)), this, SLOT(save_file_as())
    );

    /*
     *  The opposite of create_action_connections().  We hope clear()
     *  disconnects everything.  Similar for create_action_menu();
     */

    m_recent_action_list.clear();
    if (not_nullptr(m_menu_recent) && m_menu_recent->isWidgetType())
        delete m_menu_recent;
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
        m_live_frame->refresh();
}

/**
 *  Sets the song-mute mode.
 */

void
qsmainwnd::set_song_mute_off ()
{
    perf().set_song_mute(mutegroups::muting::off);
    if (not_nullptr(m_live_frame))
        m_live_frame->refresh();
}

/**
 *  Sets the song-mute mode.
 */

void
qsmainwnd::set_song_mute_toggle ()
{
    perf().set_song_mute(mutegroups::muting::toggle);
    if (not_nullptr(m_live_frame))
        m_live_frame->refresh();
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
    update_tap(bpm);
}

void
qsmainwnd::update_tap (midibpm bpm)
{
    set_tap_button(perf().current_beats());
    if (perf().current_beats() > 1)             /* first one is useless */
        ui->spinBpm->setValue(bpm);
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

bool
qsmainwnd::on_group_learn (bool learning)
{
    qt_set_icon(learning ? learn2_xpm : learn_xpm, ui->button_learn);
    return true;
}

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

bool
qsmainwnd::on_sequence_change (seq::number seqno, bool redo)
{
    bool result = not_nullptr(m_live_frame);
    if (result)
        m_live_frame->update_sequence(seqno, redo);

    return result;
}

bool
qsmainwnd::on_trigger_change (seq::number seqno)
{
    bool result = not_nullptr(m_live_frame);
    if (result)
        m_live_frame->refresh(seqno);

    return result;
}

bool
qsmainwnd::on_set_change (screenset::number setno, performer::change /*ctype*/)
{
    signal_set_change(int(setno));
    return true;
}

void
qsmainwnd::update_set_change (int setno)
{
    bool ok = not_nullptr(m_live_frame);
    if (ok)
    {
        if (setno != m_live_frame->bank())
        {
            m_live_frame->update_bank(setno);       /* updates current bank */
            ui->spinBank->setValue(setno);          /* shows it in spinbox  */
        }
        else
            m_live_frame->update_bank();            /* updates current bank */
    }
}

bool
qsmainwnd::on_resolution_change (int /*ppqn*/, midibpm bpm)
{
    ui->spinBpm->setValue(bpm);
    return true;
}

/**
 *  This is called when focus changes in the main window.
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
            {
                screenset::number bank = m_live_frame->bank();
                screenset::number setno = perf().playscreen_number();
                if (bank != setno)
                    (void) perf().set_playing_screenset(bank);
            }
        }
        else
        {
            // widget is now inactive
        }
    }
}

void
qsmainwnd::resizeEvent (QResizeEvent * /*r*/ )
{
#if defined SEQ66_PLATFORM_DEBUG_TMI
    printf("qsmainwnd::resizeEvent()\n");
#endif
}

bool
qsmainwnd::recreate_all_slots ()
{
    bool result = not_nullptr(m_live_frame);
    if (result)
    {
        if (perf().playlist_active())
        {
            m_live_frame->set_playlist_name(perf().playlist_song());
            update_window_title(perf().playlist_song());
        }
        result = m_live_frame->recreate_all_slots();
    }
    return result;
}

void
qsmainwnd::session_manager (const std::string & text)
{
    if (not_nullptr(m_session_frame))
        m_session_frame->session_manager(text);
}

void
qsmainwnd::session_path (const std::string & text)
{
    if (not_nullptr(m_session_frame))
        m_session_frame->session_path(text);
}

void
qsmainwnd::session_display_name (const std::string & text)
{
    if (not_nullptr(m_session_frame))
        m_session_frame->session_display_name(text);
}

void
qsmainwnd::session_client_id (const std::string & text)
{
    if (not_nullptr(m_session_frame))
        m_session_frame->session_client_id(text);
}

void
qsmainwnd::session_URL (const std::string & text)
{
    if (not_nullptr(m_session_frame))
        m_session_frame->session_URL(text);
}

void
qsmainwnd::session_log (const std::string & text)
{
    if (not_nullptr(m_session_frame))
        m_session_frame->session_log(text);
}

void
qsmainwnd::session_log_append (const std::string & text)
{
    if (not_nullptr(m_session_frame))
        m_session_frame->session_log_append(text);
}

void
qsmainwnd::song_path (const std::string & text)
{
    if (not_nullptr(m_session_frame))
        m_session_frame->song_path(text);
}

}               // namespace seq66

/*
 * qsmainwnd.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


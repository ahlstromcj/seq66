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
 * \file          qsmainwnd.cpp
 *
 *  This module declares/defines the base class for the main window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2021-11-24
 * \license       GNU GPLv2 or above
 *
 *  The main window is known as the "Patterns window" or "Patterns
 *  panel".  It holds the "Pattern Editor" or "Sequence Editor".  The main
 *  window consists of two object:  mainwnd, which provides the user-interface
 *  elements that surround the patterns, and the live-grid, which implements
 *  the behavior of the pattern slots.
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
 *
 * Removed:
 *
 *  We found a way to squeeze the much more functional qseqeditframe64 into
 *  the Edit tab, and so no longer need this version.
 *
 *      #include "qseqeditframe.hpp"    // Kepler34 version
 */

#include <QErrorMessage>                /* QErrorMessage                    */
#include <QFileDialog>                  /* prompt for full MIDI file's path */
#include <QInputDialog>                 /* prompt for NSM MIDI file-name    */
#include <QGuiApplication>              /* used for QScreen geometry() call */
#include <QMessageBox>                  /* QMessageBox                      */
#include <QResizeEvent>                 /* QResizeEvent                     */
#include <QScreen>                      /* Qscreen                          */
#include <QTimer>                       /* QTimer                           */
#include <iomanip>                      /* std::hex, std::setw()            */
#include <sstream>                      /* std::ostringstream               */
#include <utility>                      /* std::make_pair()                 */

#include "ctrl/keystroke.hpp"           /* seq66::keystroke class           */
#include "midi/songsummary.hpp"         /* seq66::write_song_summary()      */
#include "midi/wrkfile.hpp"             /* seq66::wrkfile class             */
#include "qliveframeex.hpp"             /* seq66::qliveframeex container    */
#include "qmutemaster.hpp"              /* shows a map of mute-groups       */
#include "qperfeditex.hpp"              /* seq66::qperfeditex container     */
#include "qperfeditframe64.hpp"         /* seq66::qperfeditframe64 class    */
#include "qplaylistframe.hpp"           /* seq66::qplaylistframe class      */
#include "qsabout.hpp"                  /* seq66::qsabout dialog class      */
#include "qsbuildinfo.hpp"              /* seq66::qsbuildinfo dialog class  */
#include "qseditoptions.hpp"            /* seq66::qseditoptions dialog      */
#include "qseqeditex.hpp"               /* seq66::qseqeditex container      */
#include "qseqeditframe64.hpp"          /* seq66::qseqeditframe64 class     */
#include "qseqeventframe.hpp"           /* a new event editor for Qt        */
#include "qsessionframe.hpp"            /* shows session information        */
#include "qsetmaster.hpp"               /* shows a map of all sets          */
#include "qsmaintime.hpp"               /* seq66::qsmaintime panel class    */
#include "qsmainwnd.hpp"                /* seq66::qsmainwnd main window     */
#include "qslivegrid.hpp"               /* seq66::qslivegrid panel          */
#include "qt5_helpers.hpp"              /* seq66::qt(), qt_set_icon() etc.  */
#include "util/filefunctions.hpp"       /* seq66::file_extension_match()    */
#include "sessions/smanager.hpp"        /* seq66::save_, attach_session()   */

/*
 *  A signal handler is defined in daemonize.cpp, used for quick & dirty
 *  signal handling.  Thanks due to user falkTX!
 */

#include "os/daemonize.hpp"             /* seq66::session_close(), etc.     */

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
#include "pixmaps/loop.xpm"
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
 *  The default name of the current (if empty) tune.  Also refer to the
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
static const int s_beat_length_count    = 16;

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
 * \param usensm
 *      If true, this changes the menu to be suitable for keeping work
 *      products inside an NSM session directory.
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
    m_initial_width         (0),
    m_initial_height        (0),
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
    m_main_perf             (p),
    m_beat_ind              (nullptr),
    m_dialog_prefs          (nullptr),
    m_dialog_about          (nullptr),
    m_dialog_build_info     (nullptr),
    m_session_frame         (nullptr),
    m_set_master            (nullptr),
    m_mute_master           (nullptr),
    m_ppqn_list             (default_ppqns()),  /* see the settings module  */
    m_control_status        (automation::ctrlstatus::none),
    m_song_mode             (false),
    m_is_looping            (false),
    m_use_nsm               (usensm),
    m_is_title_dirty        (true),
    m_tick_time_as_bbt      (false),            /* toggled in constructor   */
    m_previous_tick         (0),
    m_is_playing_now        (false),
    m_open_editors          (),
    m_open_live_frames      (),
    m_perf_frame_visible    (false),
    m_current_main_set      (0),
    m_shrunken              (usr().shrunken()),
    m_session_mgr_ptr       (nullptr)
{
    ui->setupUi(this);

    int w = usr().mainwnd_x();                  /* normal, maybe scaled     */
    int h = usr().mainwnd_y();
    resize(QSize(w, h));                        /* scaled values            */
    w = usr().mainwnd_x_min();                  /* scaled even smaller      */
    h = usr().mainwnd_y_min();
    setMinimumSize(QSize(w, h));                /* minimum values           */

    QPoint pt;                                  /* default at (0, 0)        */
    QRect screen = desktop_rectangle(pt);       /* avoids deprecated func   */
    int x = (screen.width() - width()) / 2;     /* center on the screen     */
    int y = (screen.height() - height()) / 2;
    move(x, y);

    if (usr().lock_main_window())
        setFixedSize(width(), height());

    /*
     *  Combo-box for tweaking the PPQN.
     */

    (void) set_ppqn_combo();

    int ppqn = perf().ppqn();
    std::string pstring = std::to_string(ppqn);
    set_ppqn_text(pstring);
    if (m_shrunken)
    {
        ui->lineEditPpqn->hide();
        ui->label_2->hide();        /* Hide the "PPQN" label too */
    }
    else
    {
        ui->lineEditPpqn->setReadOnly(true);
        connect
        (
            ui->cmb_ppqn, SIGNAL(currentTextChanged(const QString &)),
            this, SLOT(update_ppqn_by_text(const QString &))
        );
    }

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
                ui->cmb_global_bus->addItem(qt(busname));
                if (disabled)
                    enable_bus_item(bus, false);
            }
        }
    }

    /*
     * Fill options for beats per measure in the combo box, and set the
     * default.  For both the beat-measure and beat-length combo-boxes, we
     * tack on an additional entry for "32".
     */

    QString thirtytwo = QString::number(32);
    for (int i = 0; i < s_beat_measure_count; ++i)
    {
        QString combo_text = QString::number(i + 1);
        ui->cmb_beat_measure->insertItem(i, combo_text);
    }
    ui->cmb_beat_measure->insertItem(s_beat_measure_count, thirtytwo);

    /*
     * Fill options for beat length (beat width) in the combo box, and set the
     * default.  Note that the actual value is selected via a switch statement
     * in the update_beat_length() function.  See that function for the true
     * story.
     */

    for (int i = 0; i < s_beat_length_count; ++i)
    {
        QString combo_text = QString::number(i + 1);
        ui->cmb_beat_length->insertItem(i, combo_text);
    }
    ui->cmb_beat_length->insertItem(s_beat_length_count, thirtytwo);

    m_msg_save_changes = new QMessageBox(this);
    m_msg_save_changes->setText(tr("Unsaved changes detected."));
    m_msg_save_changes->setInformativeText(tr("Do you want to save them?"));
    m_msg_save_changes->setStandardButtons
    (
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
    );
    m_msg_save_changes->setDefaultButton(QMessageBox::Save);

    m_dialog_prefs = new (std::nothrow) qseditoptions(perf(), this);
    m_beat_ind = new (std::nothrow) qsmaintime(perf(), ui->verticalWidget /*this*/, 4, 4);
    m_dialog_about = new (std::nothrow) qsabout(this);
    m_dialog_build_info = new (std::nothrow) qsbuildinfo(this);
    make_perf_frame_in_tab();           /* create m_song_frame64 pointer    */
    m_live_frame = new (std::nothrow) qslivegrid
    (
        perf(), this, screenset::unassigned(), ui->LiveTab
    );
    if (not_nullptr(m_live_frame))
    {
        ui->LiveTabLayout->addWidget(m_live_frame);
        m_live_frame->setFocus();
    }

    m_playlist_frame = new (std::nothrow) qplaylistframe(perf(), this, ui->PlaylistTab);
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
    connect
    (
        ui->actionExportSMF0, SIGNAL(triggered(bool)),
        this, SLOT(export_file_as_smf_0())
    );
    if (perf().smf_format() != 0)
        ui->smf0Button->hide();
    else
        ui->smf0Button->show();

    /*
     * File / Import MIDI to Current Set...
     */

    connect
    (
        ui->actionImport_MIDI, SIGNAL(triggered(bool)),
        this, SLOT(import_into_set())
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
    connect
    (
        ui->actionCopyCurrentSet, SIGNAL(triggered(bool)),
        this, SLOT(set_playscreen_copy())
    );
    ui->actionCopyCurrentSet->setEnabled(true);
    connect
    (
        ui->actionPasteToCurrentSet, SIGNAL(triggered(bool)),
        this, SLOT(set_playscreen_paste())
    );
    ui->actionPasteToCurrentSet->setEnabled(false);

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
     * L/R Loop button.
     */

    if (m_shrunken)
    {
        ui->btnLoop->hide();
    }
    else
    {
        ui->btnLoop->setChecked(perf().looping());
        connect(ui->btnLoop, SIGNAL(clicked(bool)), this, SLOT(set_loop(bool)));
        qt_set_icon(loop_xpm, ui->btnLoop);
    }

    /*
     * Song Play (Live vs Song) button.
     */

    connect
    (
        ui->btnSongPlay, SIGNAL(clicked(bool)),
        this, SLOT(set_song_mode(bool))
    );
    ui->btnSongPlay->setCheckable(false);

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

    if (m_shrunken)
    {
        ui->btnPerfEdit->hide();
    }
    else
    {
        connect
        (
            ui->btnPerfEdit, SIGNAL(clicked(bool)),
            this, SLOT(load_qperfedit(bool))
        );
        qt_set_icon(perfedit_xpm, ui->btnPerfEdit);
    }

    /*
     * B:B:T vs H:M:S button.
     */

    toggle_time_format(true);           /* slightly tricky */
    connect
    (
        ui->btn_set_HMS, SIGNAL(clicked(bool)),
        this, SLOT(toggle_time_format(bool))
    );

    /*
     * Set-Reset button.
     */

    connect
    (
        ui->setResetButton, SIGNAL(clicked(bool)),
        this, SLOT(reset_sets())
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
     * Global buss combo-box.  If we set the buss override, we have to add 1
     * to it to allow for the "None" entry.
     */

    midibyte buss_override = usr().midi_buss_override();
    if (is_good_buss(buss_override))
        ui->cmb_global_bus->setCurrentIndex(int(buss_override) + 1);

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

    /*
     * Set Number and Name.
     */

    QString bname = qt(perf().set_name(0));
    ui->txtBankName->setText(bname);
    ui->spinBank->setRange(0, perf().screenset_max() - 1);
    connect
    (
        ui->setHomeButton, SIGNAL(clicked(bool)),
        this, SLOT(slot_set_home())
    );
    connect
    (
        ui->spinBank, SIGNAL(valueChanged(int)),
        this, SLOT(update_bank(int))
    );
    connect
    (
        ui->txtBankName, SIGNAL(editingFinished()),
        this, SLOT(update_bank_text())
    );
    connect
    (
        this, SIGNAL(signal_set_change(int)),
        this, SLOT(update_set_change(int))
    );
    connect
    (
        this, SIGNAL(signal_song_action(int)),
        this, SLOT(update_song_action(int))
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

    load_set_master();
    load_mute_master();
    load_session_frame();
    ui->tabWidget->setCurrentIndex(Tab_Live);
    ui->tabWidget->setTabEnabled(Tab_Events, false);    /* prevents issues  */

    /*
     * Test button.  This button supports whatever debugging we need to do at
     * any particular time.
     */

    if (rc().investigate_disabled())
    {
        /*
         * Enable to test SMF 0 conversion.
         */

        ui->testButton->setEnabled(true);
        ui->testButton->setToolTip("Converts patterns into one SMF 0 track.");
        connect(ui->testButton, SIGNAL(clicked(bool)), this, SLOT(slot_test()));
    }
    else
    {
        ui->testButton->setEnabled(false);
        ui->testButton->setToolTip("Developer test button, disabled.");
    }

    if (use_nsm())
    {
        std::string tune_name = s_default_tune;
        std::string fname = rc().midi_filename();
        if (! fname.empty())
            tune_name = fname;

        rc().session_midi_filename(tune_name);
    }
    (void) refresh_captions();

#if defined SEQ66_PORTMIDI_SUPPORT
    ui->alsaJackButton->setText("PortMidi");
    ui->jackTransportButton->hide();
#else
    QString midiengine = rc().with_jack_midi() ? "JACK" : "ALSA" ;
    if (cb_perf().is_jack_master())
        ui->jackTransportButton->setText("Master");
    else if (cb_perf().is_jack_slave())
        ui->jackTransportButton->setText("Slave");
    else
        ui->jackTransportButton->hide();

    ui->alsaJackButton->setText(midiengine);
#endif

    if (! rc().investigate())
        ui->label_test->hide();

    show();
    show_song_mode(m_song_mode);
    cb_perf().enregister(this);
    m_timer = qt_timer(this, "qsmainwnd", 3, SLOT(conditional_update()));
}

/**
 *  Destroys the user interface and removes any external qperfeditex that
 *  exists.
 */

qsmainwnd::~qsmainwnd ()
{
    /*
     * This should be done by Qt since the "this" parameter was used.
     *
     *  if (not_nullptr(m_msg_error))
     *      delete m_msg_error;
     */

    m_timer->stop();
    cb_perf().unregister(this);
    delete ui;
}

void
qsmainwnd::lock_main_window (bool lockit)
{
    if (lockit)
        setFixedSize(width(), height());
    else
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}

void
qsmainwnd::enable_bus_item (int bus, bool enabled)
{
    int index = bus + 1;
    enable_combobox_item(ui->cmb_global_bus, index, enabled);
}

void
qsmainwnd::set_ppqn_text (const std::string & text)
{
    QString ppqnstring = qt(text);
    ui->lineEditPpqn->setText(ppqnstring);
}

/*
 *  Note that the "use NSM" flag is set at construction time.
 */

void
qsmainwnd::attach_session (smanager * sp)   // UNNECESSARY?
{
    if (not_nullptr(sp))
        m_session_mgr_ptr = sp;
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
    m_song_frame64 = new (std::nothrow) qperfeditframe64(perf(), ui->SongTab);
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
qsmainwnd::set_loop (bool looping)
{
    perf().looping(looping);
    if (not_nullptr(m_perfedit))
        m_perfedit->set_loop_button(looping);
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
        ui->btnRecord->setEnabled(true);
        ui->btnSongPlay->setText("Song");
    }
    else
    {
        ui->btnRecord->setChecked(false);
        ui->btnRecord->setEnabled(false);
        ui->btnSongPlay->setText("Live");
    }
}


/**
 *  Sets the song mode, which is actually the JACK start mode.  If true, we
 *  are in playback/song mode.  If false, we are in live mode.
 *
 *  Removed: song_recording(false)
 */

void
qsmainwnd::set_song_mode (bool /*songmode*/)
{
    bool playmode = perf().toggle_song_mode();
    if (! playmode)
        song_recording(false);

    show_song_mode(playmode);
}

bool
qsmainwnd::set_ppqn_combo ()
{
    bool result = false;
    int count = m_ppqn_list.count();
    if (count > 0)
    {
        std::string p = std::to_string(perf().ppqn());
        QString combo_text = qt(p);
        ui->cmb_ppqn->clear();
        ui->cmb_ppqn->insertItem(0, combo_text);
        for (int i = 1; i < count; ++i)
        {
            p = m_ppqn_list.at(i);
            combo_text = qt(p);
            ui->cmb_ppqn->insertItem(i, combo_text);
            if (std::stoi(p) == perf().ppqn())
                result = true;
        }
        ui->cmb_ppqn->setCurrentIndex(0);
    }
    return result;
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
            rc().session_midi_filename(basename);       /* make NSM name    */

            std::string mfilename = rc().midi_filename();
            song_path(mfilename);
            std::string msg = save_file(mfilename, false) ?
                "Saved: " : "Failed to save: ";

            msg += rc().midi_filename();
            show_message_box(msg);
            m_is_title_dirty = false;
            result = true;
            if (not_nullptr(m_mute_master))
                m_mute_master->group_needs_update();
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
    {
        if (open_file(selectedfile))
        {
            if (! usr().is_buss_override())
                ui->cmb_global_bus->setCurrentIndex(0);

            if (not_nullptr(m_mute_master))
                m_mute_master->group_needs_update();
        }
    }
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
    {
        result = show_open_midi_file_dialog(this, selectedfile);
        if (result)
            stop_playing();
    }
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
            /* performer will handle rc().playlist_filename(fname) */
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
 *  Update all of the children.  Doesn't seem to work for the edit frames, may
 *  have to recreate them, or somehow hook in the new sequence objects (as
 *  pointers, not references).  Probably an issue to be ignored; the user will
 *  have to close and reopen the pattern editor(s).
 *
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
        if (not_nullptr(m_song_frame64))
            m_song_frame64->update_sizes();

        if (not_nullptr(m_perfedit))
            m_perfedit->update_sizes();

#if defined SEQ66_USE_SEQEDIT_REDRAWING
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
        {
            ui->actionSave->setEnabled(file_writable(fn));
            update_recent_files_menu();
        }
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

    m_live_frame = new (std::nothrow) qslivegrid
    (
        perf(), this, screenset::unassigned(), ui->LiveTab
    );
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
    std::string itemname = fn.empty() ? perf().main_window_title(fn) : fn ;
    itemname += " [*]";                             /* required by Qt 5     */

    QString fname = qt(itemname);
    setWindowTitle(fname);                          /* title must come 1st  */
    setWindowModified(perf().modified());           /* perhaps show the '*' */
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
        qsessionframe * qsf = new (std::nothrow) qsessionframe
        (
            perf(), this, ui->SessionTab
        );
        if (not_nullptr(qsf))
        {
            ui->SessionTabLayout->addWidget(qsf);
            m_session_frame = qsf;
        }
    }
}

/**
 *  Handles resetting the performer's play-set to the current set.
 */

void
qsmainwnd::reset_sets ()
{
    perf().reset_playset();
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
qsmainwnd::conditional_update ()
{
    if (session_close())
    {
        m_timer->stop();
        close();
        return;
    }
    if (session_save())
    {
        (void) save_session();      /* ca 2021-08-31 instead of save_file() */
    }

    int active_screenset = int(perf().playscreen_number());
    std::string b = "#";
    b += std::to_string(active_screenset);
    b += " of ";
    b += std::to_string(perf().screenset_count());
    ui->entry_active_set->setText(qt(b));
    if (ui->button_keep_queue->isChecked() != perf().is_keep_queue())
        ui->button_keep_queue->setChecked(perf().is_keep_queue());

    if (m_song_mode != perf().song_mode())
    {
        m_song_mode = perf().song_mode();
        show_song_mode(m_song_mode);
    }
    if (m_is_looping != perf().looping())
    {
        m_is_looping = perf().looping();
        ui->btnLoop->setChecked(m_is_looping);
    }
    if (m_control_status != perf().ctrl_status())
    {
        m_control_status = perf().ctrl_status();
        if (not_nullptr(m_live_frame))
            m_live_frame->set_mode_text(perf().ctrl_status_string());
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
            std::string t = m_tick_time_as_bbt ?
                perf().pulses_to_measure_string(tick) :
                perf().pulses_to_time_string(tick) ;

            ui->label_HMS->setText(qt(t));
            m_beat_ind->update();
        }
    }
    if (m_is_title_dirty)
    {
        (void) refresh_captions();
        m_is_title_dirty = false;
        update_window_title();          /* puts current MIDI file in title  */
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
qsmainwnd::filename_prompt
(
    const std::string & prompt,
    const std::string & file
)
{
    std::string result = file.empty() ? rc().last_used_dir() : file ;
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
    else
        result.clear();

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
        ui->actionSave->setEnabled(true);
        m_is_title_dirty = true;
        redo_live_frame();
        remove_all_editors();
        (void) perf().reset_mute_groups();          /* no modify() call     */
        if (! usr().is_buss_override())
            ui->cmb_global_bus->setCurrentIndex(0);

        if (not_nullptr(m_mute_master))
            m_mute_master->group_needs_update();
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
            QLineEdit::Normal, qt(defname), &ok
        );
        if (ok)
        {
            if (perf().clear_all())                 /* like new_file()      */
            {
                m_is_title_dirty = true;
                redo_live_frame();
                remove_all_editors();
                (void) perf().reset_mute_groups();  /* no modify() call     */
                if (not_nullptr(m_mute_master))
                    m_mute_master->group_needs_update();
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
    if (result)
        m_is_title_dirty = true;

    return result;
}

bool
qsmainwnd::save_file_as ()
{
    bool result = false;
    std::string prompt = use_nsm() ?
        "Export MIDI file from NSM session as..." : "Save MIDI file as..." ;

    std::string currentfile = rc().midi_filename();
    std::string filename = filename_prompt(prompt, currentfile);
    if (filename.empty())
    {
        // no code, the user merely cancelled
    }
    else
    {
        result = save_file(filename);           /* m_is_title_dirty = true  */
        if (result)
            rc().midi_filename(filename);
    }
    return result;
}

/**
 *  Prompts for a file-name, then exports the current tune as a standard
 *  MIDI file, stripping out the Seq66 SeqSpec information.  Does not
 *  update the the current file-name, but does update the recent-file
 *  information at this time.  It does not preserve the triggers.
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
        midifile f(filename, choose_ppqn(c_use_default_ppqn));
        result = f.write(perf(), false);           /* no SeqSpec       */
        if (! result)
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
        std::string prompt = "Export Song...";
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
        midifile f(filename, choose_ppqn(c_use_default_ppqn));
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
    std::string selectedfile;
    bool selected = show_import_midi_file_dialog(this, selectedfile);
    if (selected)
    {
        QString path = qt(selectedfile);
        if (! path.isEmpty())
        {
            try
            {
                int setno = int(perf().playscreen_number());
                std::string fn = path.toStdString();
                bool is_wrk = file_extension_match(fn, "wrk");
                midifile * f = is_wrk ?
                    new (std::nothrow) wrkfile(fn, choose_ppqn(c_use_default_ppqn)) :
                    new (std::nothrow) midifile(fn, choose_ppqn(c_use_default_ppqn))
                    ;

                if (f->parse(perf(), setno, true))  /* true --> importing   */
                {
                    ui->spinBpm->setValue(perf().bpm());
                    ui->spinBpm->setDecimals(usr().bpm_precision());
                    ui->spinBpm->setSingleStep(usr().bpm_step_increment());
                    update_bank(setno);
                    (void) refresh_captions();
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
 *  Loads a slightly less functional qseqeditframe64 for the selected
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

        m_edit_frame = new (std::nothrow) qseqeditframe64
        (
            perf(), seqid, ui->EditTab, true                /* short one    */
        );
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
 *  We make sure the sequence exists.  We should consider creating it if it
 *  does not exist.  So many features, so little time.
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
            qseqeditex * ex = new (std::nothrow)
                qseqeditex(perf(), seqid, this);

            if (not_nullptr(ex))
            {
                auto p = std::make_pair(seqid, ex);
                m_open_editors.insert(p);
                ex->show();
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
            qliveframeex * ex = new (std::nothrow) qliveframeex
            (
                perf(), ssnum, this
            );
            if (not_nullptr(ex))
            {
                auto p = std::make_pair(ssnum, ex);
                m_open_live_frames.insert(p);
                ex->show();
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
qsmainwnd::update_ppqn_by_text (const QString & text)
{
    std::string temp = text.toStdString();
    if (! temp.empty())
    {
        int p = std::stoi(temp);
        if (perf().change_ppqn(p))
        {
            set_ppqn_text(temp);
            m_ppqn_list.current(temp);
            ui->cmb_ppqn->setItemText(0, text);
            usr().file_ppqn(p);
        }
    }
}

/**
 *  Sets the MIDI bus to use for output for the current set and its sequences.
 *  A convenience when dealing with one MIDI output device.  Must be applied
 *  individually to each set; this allows each set to drive a different buss.
 *
 * \param index
 *      The index into the list of available MIDI buses.  The drop-down has
 *      already been remapped (if port-mapping is active).
 */

void
qsmainwnd::update_midi_bus (int index)
{
    mastermidibus * mmb = perf().master_bus();
    if (not_nullptr(mmb))
    {
        if (index == 0)
            usr().midi_buss_override(null_buss());  /* for the "None" entry */
        else
            (void) perf().ui_change_set_bus(index - 1);
    }
}

/**
 *  We could set the value using pow(2, blindex).
 */

void
qsmainwnd::update_beat_length (int blindex)
{
    int bl = blindex == s_beat_length_count ? 32 : blindex + 1 ;
    if (perf().set_beat_width(bl))
    {
        if (not_nullptr(m_song_frame64))
            m_song_frame64->set_beat_width(bl);

        if (not_nullptr(m_beat_ind))
            m_beat_ind->beat_width(bl);

        if (not_nullptr(m_edit_frame))
            m_edit_frame->update_draw_geometry();
    }
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
    int bm = bmindex == s_beat_measure_count ? 32 : bmindex + 1;
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
                 * update the live tab to show a new sequence.
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
                    /*
                     * This code is called when first clicking on the
                     * "Editor" tab. For the code called when selecting a
                     * pattern to open in this tab, see load_editor() above.
                     */

                    m_edit_frame = new (std::nothrow)
                        qseqeditframe64(perf(), seqid, ui->EditTab, true);

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
                 * update the live tab to show a new sequence.
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

/**
 * Let's see if we can distinguish files with the same base name.
 */

void
qsmainwnd::update_recent_files_menu ()
{
    int count = rc().recent_file_count();
    if (count > 0)
    {
        bool ok = true;
        bool shorten = ! rc().full_recent_paths();
        for (int f = 0; f < count; ++f)
        {
            std::string shortname = rc().recent_file(f, shorten);
            if (! shortname.empty())
            {
                std::string longname = rc().recent_file(f, false);
                m_recent_action_list.at(f)->setText(qt(shortname));
                m_recent_action_list.at(f)->setData(qt(longname));
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
            for (int fj = count; fj < rc().recent_file_max(); ++fj)
                m_recent_action_list.at(fj)->setVisible(false);

            ui->menuFile->insertMenu(ui->actionSave, m_menu_recent);
        }
    }
}

void
qsmainwnd::create_action_connections ()
{
    for (int i = 0; i < rc().recent_file_max(); ++i)
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
    for (int i = 0; i < rc().recent_file_max(); ++i)
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
        {
            if (open_file(actionfile))
            {
                if (! usr().is_buss_override())
                    ui->cmb_global_bus->setCurrentIndex(0);

                if (not_nullptr(m_mute_master))
                    m_mute_master->group_needs_update();
            }
        }
    }
}

void
qsmainwnd::enable_reload_button (bool flag)
{
    m_session_frame->enable_reload_button(flag);
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
 *  By experimenting, we see that the live frame gets all of the keystrokes.
 *  So we moved much of the processing to that class.  If the event isn't
 *  handled there, then qsmainwnd::handle_key_press() is called.
 *
 *  If we reimplement this handler, it is very important to call the base
 *  class implementation if the key is no acted on.
 *
 *  ca 2020-11-22 Refactoring keystroke handling between the main window and
 *  the live grid/frame.
 */

void
qsmainwnd::keyPressEvent (QKeyEvent * event)
{
    keystroke k = qt_keystroke(event, keystroke::action::press);
    bool done = handle_key_press(k);
    if (done)
        update();
    else
        QWidget::keyPressEvent(event);              /* event->ignore()?     */
}

void
qsmainwnd::keyReleaseEvent (QKeyEvent * event)
{
    keystroke k = qt_keystroke(event, keystroke::action::release);
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
 *  and the songs they specify.  These functions are also supported by the
 *  MIDI automation apparatus, but are not supported by the keystroke
 *  automation apparatus.  They are considered dedicated keys.
 *
 *  Note that changing the playlist name will cause a reupdate of all of the
 *  window, including the pattern buttons in qslivegrid, causing flickering of
 *  all armed pattern buttons.
 *
 *  Also note that we have switched from direct calls the performer playlist
 *  actions to using signals.  We have to use signals with MIDI control
 *  because of thread conflicts, so we might as well use them with the
 *  keystrokes as well.
 *
 * \param k
 *      Provides a wrapper for the key event.
 */

bool
qsmainwnd::handle_key_press (const keystroke & k)
{
    bool done = false;
    if (k.is_press())
    {
        playlist::action act = playlist::action::none;
        if (k.is_right())
        {
            act = playlist::action::next_song;
            done = true;
        }
        else if (k.is_left())
        {
            act = playlist::action::previous_song;
            done = true;
        }
        else if (k.is_down())
        {
            act = playlist::action::next_list;
            done = true;
        }
        else if (k.is_up())
        {
            act = playlist::action::previous_list;
            done = true;
        }

        int actcode = playlist::action_to_int(act);
        emit signal_song_action(actcode);
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
    return done;
}

/**
 *  See qslivegrid::handle_key_release().
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
 *  Quickly changes to set 0.
 */

void
qsmainwnd::slot_set_home ()
{
    ui->spinBank->setValue(0);          // update_bank(0);
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
         * (void) perf().set_playing_screenset(m_live_frame->bank_id());
         */

        std::string name = perf().set_name(bankid);
        QString newname = qt(name);
        ui->txtBankName->setText(newname);
    }
}

/**
 *  Used to grab the std::string bank name and convert it to QString for
 *  display. Let performer set the modify flag, it knows when to do it.
 *  Otherwise, just scrolling to the next screen-set causes a spurious
 *  modification and an annoying prompt to a user exiting the application.
 */

void
qsmainwnd::update_bank_text ()
{
    QString newname = ui->txtBankName->text();
    std::string name = newname.toStdString();
    if (not_nullptr(m_live_frame))
        m_live_frame->update_bank_name(name);

    ui->txtBankName->setText(newname);
}

void
qsmainwnd::show_message_box (const std::string & msg_text)
{
    if (! msg_text.empty())
    {
        if (not_nullptr(m_msg_error))
            delete m_msg_error;

        m_msg_error = new (std::nothrow) QErrorMessage(this);
        if (not_nullptr(m_msg_error))
        {
            QString msg = qt(msg_text);
            QSpacerItem * hspace = new QSpacerItem
            (
                SEQ66_ERROR_BOX_WIDTH, 0,
                QSizePolicy::Minimum, QSizePolicy::Expanding
            );
            QGridLayout * lay = (QGridLayout *) m_msg_error->layout();
            lay->addItem(hspace, lay->rowCount(), 0, 1, lay->columnCount());
            m_msg_error->showMessage(msg);
            m_msg_error->exec();
        }
    }
}

void
qsmainwnd::enable_save (bool flag)
{
    ui->actionSave->setEnabled(flag);
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

    ui->actionOpen->setText("&Open...");
    ui->actionOpen->setToolTip("Open a standard or Seq66 MIDI file.");
    connect
    (
        ui->actionOpen, SIGNAL(triggered(bool)),
        this, SLOT(select_and_load_file())
    );

    /*
     * File / Save
     */

    ui->actionSave->setText("&Save");
    ui->actionSave->setToolTip("Save as a Seq66 MIDI file.");
    if (! file_writable(rc().midi_filename()))
        ui->actionSave->setEnabled(false);

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
        show_message_box("Reload of mute-groups failed");
}

/**
 *  Clear all mute-group settings.  Sets all values to false/zero.  Also,
 *  since the intent might be to clean up the MIDI file, the user is prompted
 *  to save.
 *
 *  Should we use reset_mute_groups() instead?
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
    perf().set_song_mute(mutegroups::action::on);
    if (not_nullptr(m_live_frame))
        m_live_frame->refresh();
}

/**
 *  Sets the song-mute mode.
 */

void
qsmainwnd::set_song_mute_off ()
{
    perf().set_song_mute(mutegroups::action::off);
    if (not_nullptr(m_live_frame))
        m_live_frame->refresh();
}

/**
 *  Sets the song-mute mode.
 */

void
qsmainwnd::set_song_mute_toggle ()
{
    perf().set_song_mute(mutegroups::action::toggle);
    if (not_nullptr(m_live_frame))
        m_live_frame->refresh();
}

void
qsmainwnd::set_playscreen_copy ()
{
    if (perf().copy_playscreen())
        ui->actionPasteToCurrentSet->setEnabled(true);
}

void
qsmainwnd::set_playscreen_paste ()
{
    (void) perf().paste_playscreen(perf().playscreen_number());

    /*
     * We want to allow multiple pastes of the same screenset.
     *
     *  if (perf().paste_playscreen(perf().playscreen_number()))
     *      ui->actionPasteToCurrentSet->setEnabled(false);
     */
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
qsmainwnd::slot_test ()
{
    /*
     * No code at present
     */
}

bool
qsmainwnd::export_file_as_smf_0 (const std::string & fname)
{
    bool result = false;
    std::string filename;
    if (fname.empty())
    {
        std::string prompt = "Convert and export file as SMF 0...";
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
        if (perf().convert_to_smf_0())
        {
            midifile f(filename, choose_ppqn(c_use_default_ppqn));
            result = f.write(perf(), false);           /* no SeqSpec       */
            if (result)
            {
                rc().session_midi_filename(filename);
                m_is_title_dirty = true;
            }
            else
                show_message_box(f.error_message());
        }
        else
        {
            std::string msg =
                "Could not convert to SMF 0.  Make sure desired tracks are "
                "unmuted and have song triggers present."
                ;
            show_message_box(msg);
        }
        if (perf().smf_format() != 0)
            ui->smf0Button->hide();
        else
            ui->smf0Button->show();
    }
    return result;
}

void
qsmainwnd::report_message (const std::string & msg, bool good)
{
    if (! msg.empty())
    {
        if (good)                           /* info message     */
        {
            QMessageBox * mbox = new QMessageBox(this);
            mbox->setText(qt(msg));
            mbox->setInformativeText(tr("Click OK to continue."));
            mbox->setStandardButtons(QMessageBox::Ok);
            mbox->exec();
        }
        else                                /* error message    */
        {
            QErrorMessage * errbox = new QErrorMessage(this);
            errbox->showMessage(qt(msg));
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
            << "MIDI mute-group learn success, mute-group key '" << k.name()
            << "' (code = " << int(k.key())
            << " [0x" << std::hex << std::setw(2) << unsigned(k.key())
            << "]) mapped."
           ;
    }
    else
    {
        os
            << "Key '" << k.name()
            << "' (code = " << int(k.key())
            << " [0x" << std::hex << std::setw(2) << unsigned(k.key())
            << "]) is not a configured mute-group key. "
            << "To add it, edit the 'ctrl' file."
           ;
    }
    report_message(os.str(), good);
    return good;
}

bool
qsmainwnd::on_automation_change (automation::slot /* s */)
{
    bool result = not_nullptr(m_live_frame);
    if (result)
        m_live_frame->set_needs_update();       /* brute force */

    return result;
}

bool
qsmainwnd::on_sequence_change (seq::number seqno, bool redo)
{
    bool result = not_nullptr(m_live_frame);
    if (result)
    {
        m_live_frame->update_sequence(seqno, redo);
        for (auto ip : m_open_live_frames)
            ip.second->update_sequence(seqno, redo);

        ui->actionSave->setEnabled(perf().modified());
        if (redo)
            m_is_title_dirty = true;
    }
    return result;
}

bool
qsmainwnd::on_trigger_change (seq::number seqno)
{
    bool result = not_nullptr(m_live_frame);
    if (result)
    {
        m_live_frame->refresh(seqno);
        ui->actionSave->setEnabled(perf().modified());
    }
    return result;
}

bool
qsmainwnd::on_set_change (screenset::number setno, performer::change /*ctype*/)
{
    emit signal_set_change(int(setno));
    return true;
}

void
qsmainwnd::update_set_change (int setno)
{
    bool ok = not_nullptr(m_live_frame);
    if (ok)
    {
        if (setno != m_live_frame->bank_id())
        {
            QString bname = qt(perf().set_name(setno));
            m_live_frame->update_bank(setno);       /* updates current bank */
            ui->spinBank->setValue(setno);          /* shows it in spinbox  */
            ui->txtBankName->setText(bname);        /* show set/bank name   */
        }
        else
            m_live_frame->update_bank();            /* updates current bank */

        bool cancopy = perf().playscreen_active_count() > 0;
        ui->actionCopyCurrentSet->setEnabled(cancopy);

        if (not_nullptr(m_song_frame64))
            m_song_frame64->update_sizes();
    }
}

bool
qsmainwnd::on_resolution_change (int ppqn, midibpm bpm)
{
    std::string pstring = std::to_string(ppqn);
    set_ppqn_text(pstring);
    ui->spinBpm->setValue(bpm);
    return true;
}

bool
qsmainwnd::on_song_action (bool signal, playlist::action act)
{
    if (signal)
    {
        int a = playlist::action_to_int(act);
        emit signal_song_action(a);
    }
    else
    {
        m_is_title_dirty = true;
    }
    return true;
}

void
qsmainwnd::update_song_action (int playaction)
{
    bool result = false;
    playlist::action a = playlist::int_to_action(playaction);
    switch (a)
    {
    case playlist::action::next_list:

        result = perf().open_next_list();
        break;

    case playlist::action::next_song:

        result = perf().open_next_song();
        break;

    case playlist::action::previous_song:

        result = perf().open_previous_song();
        break;

    case playlist::action::previous_list:

        result = perf().open_previous_list();
        break;

    default:

        break;
    }
    if (result)
    {
        perf().next_song_mode();
        m_is_title_dirty = true;
        update_window_title(perf().playlist_song_basename());
    }
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
                screenset::number bank = m_live_frame->bank_id();
                screenset::number setno = perf().playscreen_number();
                if (bank != setno)
                {
                    (void) perf().set_playing_screenset(bank);
                }
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
    (void) usr().window_rescale(width(), height());
    recreate_all_slots();
}

bool
qsmainwnd::recreate_all_slots ()
{
    bool result = refresh_captions();
    if (result)
        result = m_live_frame->recreate_all_slots();

    return result;
}

bool
qsmainwnd::refresh_captions ()
{
    bool result = not_nullptr(m_live_frame);
    if (result)
    {
        std::string newname;
        if (perf().playlist_active())
            newname = perf().playlist_song();
        else
            newname = rc().midi_filename();

        m_live_frame->set_playlist_name(newname, perf().modified());
    }
    if (perf().playlist_active())
    {
        std::string newname = perf().playlist_song_basename();
        update_window_title(newname);
    }
    ui->actionSave->setEnabled(perf().modified());
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


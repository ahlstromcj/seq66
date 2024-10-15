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
 * \file          qseditoptions.cpp
 *
 *      The Qt version of the Options editor.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2024-10-15
 * \license       GNU GPLv2 or above
 *
 *      This version is located in Edit / Preferences.
 *
 *  Reset/Apply/Cancel/Ok and Restart-Session handling. The symbol 'x' means
 *  disabled, and 'o' means enabled.
 *
\verbatim
                Reset       Apply  Cancel  Ok      Restart
    First run:   x           x       o      x       x
    Change:      o           o       x      o       x
    Reset:       x           x       o      x       x
    Apply:       x           x       o      x       o
\endverbatim
 *
 *  When is a restart (reload) of the application needed? When the Apply or OK
 *  buttons are clicked, or the port statuses have changed.
 *
 *  When an 'rc' or 'usr' change is made, do we want to wait until the Apply/OK
 *  are clicked?  No, they directly affect rc() and usr() and a restart is
 *  needed/useful.
 */

#include <QButtonGroup>

#include "midi/jack_assistant.hpp"      /* seq66::jack_assistant statics    */
#include "os/daemonize.hpp"             /* seq66::signal_for_restart()      */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "util/filefunctions.hpp"       /* seq66::filename_base()           */
#include "util/strfunctions.hpp"        /* seq66::string_to_int()           */
#include "gui_palette_qt5.hpp"          /* seq66::global_palette()          */
#include "palettefile.hpp"              /* seq66::palettefile class         */
#include "qclocklayout.hpp"             /* seq66::qclocklayout class        */
#include "qinputcheckbox.hpp"           /* seq66::qinputcheckbox class      */
#include "qseditoptions.hpp"            /* seq66::qseditoptions dialog      */
#include "qsmainwnd.hpp"                /* seq66::qsmainwnd master class    */
#include "qt5_helpers.hpp"              /* seq66::qt() string conversion    */

#if defined SEQ66_NSM_SUPPORT
#include "nsm/nsmbase.hpp"              /* seq66::nsmbase's get_url()       */
#endif

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#if defined SEQ66_QMAKE_RULES
#include "forms/ui_qseditoptions.h"
#else
#include "forms/qseditoptions.ui.h"
#endif


/*
 *  Do not document the namespace, it breaks Doxygen.
 */

namespace seq66
{

const int Tab_MIDI_Clock     =  0;
const int Tab_MIDI_Input     =  1;
const int Tab_Display        =  2;
const int Tab_JACK           =  3;
const int Tab_Play_Options   =  4;
const int Tab_Metronome      =  5;
const int Tab_Pattern        =  6;
const int Tab_Session        =  7;

/**
 *  Button numbering for JACK Start Mode radio-buttons.
 */

enum playmode_button_t
{
    playmode_button_live,
    playmode_button_song
};

/**
 *  Button number for the Set Mode radio-buttons.
 */

enum setsmode_button_t
{
    setsmode_button_normal,
    setsmode_button_autoarm,
    setsmode_button_additive,
    setsmode_button_allsets,
};

/**
 *  Button number for the Song Start Mode radio-buttons.
 */

enum startmode_button_t
{
    startmode_button_live,
    startmode_button_song,
    startmode_button_auto
};

/**
 *  The main constructor.
 */

qseditoptions::qseditoptions (performer & p, QWidget * parent) :
    QDialog                 (parent),
    ui                      (new Ui::qseditoptions),
    m_live_song_buttons     (nullptr),
    m_parent_widget         (dynamic_cast<qsmainwnd *>(parent)),
    m_perf                  (p),
    m_ppqn_list             (default_ppqns(), true), /* add a blank slot    */
    m_buffer_size_list      (jack_buffer_size_list()),
    m_is_initialized        (false),
    m_backup_rc             (),
    m_backup_usr            (),
    m_reload_needed         (false)
{
    ui->setupUi(this);
    backup();
    setup_tab_midi_clock();
    setup_tab_midi_input();
    setup_tab_display();
    setup_tab_jack();
    setup_tab_play_options();
    setup_tab_metronome();
    setup_tab_pattern();
    setup_tab_session();

    /*
     * Reload/restart button.  Replaced by Apply --> Restart!
     */

    ui->pushButtonReload->setEnabled(false);
    ui->pushButtonReload->hide();

    /*
     * OK/Cancel Buttons
     */

    connect
    (
        ui->buttonBoxOptionsDialog->button(QDialogButtonBox::Ok),
        SIGNAL(clicked()), this, SLOT(okay())
    );
    connect
    (
        ui->buttonBoxOptionsDialog->button(QDialogButtonBox::Cancel),
        SIGNAL(clicked()), this, SLOT(cancel())
    );

    /*
     * Apply/Reset buttons.
     */

    set_text(QDialogButtonBox::Apply, "Restart Seq66!");
    connect
    (
        ui->buttonBoxOptionsDialog->button(QDialogButtonBox::Apply),
        SIGNAL(clicked()), this, SLOT(apply())
    );
    connect
    (
        ui->buttonBoxOptionsDialog->button(QDialogButtonBox::Reset),
        SIGNAL(clicked()), this, SLOT(reset())
    );
    sync();
    state_unchanged();
    ui->tabWidget->setCurrentIndex(Tab_MIDI_Clock);
    m_is_initialized = true;
}

/**
 *  A stock Qt GUI destructor.
 */

qseditoptions::~qseditoptions ()
{
    delete ui;
}

/*
 *---------------------------------------------------------------------
 *  MIDI Clock tab.
 *---------------------------------------------------------------------
 */

/*
 * Output MIDI control buss combo-box population.
 */

void
qseditoptions::setup_clock_combo_box (int buses, QComboBox * out)
{
    out->clear();
    for (int bus = 0; bus < buses; ++bus)
    {
        std::string busname;
        e_clock ec;
        bool good = perf().ui_get_clock(bussbyte(bus), ec, busname);
        if (good)
        {
            bool enabled = ec != e_clock::disabled;
            out->addItem(qt(busname));
            enable_combobox_item(out, bus, enabled);
        }
    }

    bool active = perf().midi_control_out().configure_enabled();
    int buss = perf().midi_control_out().configured_buss();
    out->setCurrentIndex(buss);
    ui->checkBoxMidiOutBuss->setChecked(active);
}

/*
 * Input MIDI control buss combo-box population. Note the (new) check
 * for the check-box being checked before enabling the items inside.
 */

void
qseditoptions::setup_input_combo_box (int buses, QComboBox * in)
{
    bool inctrlenabled = perf().midi_control_in().configure_enabled();
    in->clear();
    for (int bus = 0; bus < buses; ++bus)
    {
        std::string busname;
        bool inputing;
        bool good = perf().ui_get_input(bussbyte(bus), inputing, busname);
        if (good)
        {
            bool enabled = ! perf().is_input_system_port(bus);
            enabled = enabled && inctrlenabled;         /* ca 2023-09-21    */
            in->addItem(qt(busname));
            enable_combobox_item(in, bus, enabled);
        }
    }

    bool active = perf().midi_control_in().configure_enabled();
    int buss = perf().midi_control_in().configured_buss();
    in->setCurrentIndex(buss);
    ui->checkBoxMidiInBuss->setChecked(active);
}

void
qseditoptions::setup_tab_midi_clock ()
{
    ui->tabWidget->setTabToolTip
    (
        Tab_MIDI_Clock, "Output ports, port-mapping, and clock settings"
    );

    /*
     * Set up the MIDI Clock tab.  We use the qclocklayout class to hold
     * most of the complex code needed to handle the list of output ports and
     * clock radio-buttons.
     */

    QWidget * central = new QWidget;
    QVBoxLayout * vboxclocks = new QVBoxLayout(central);
    mastermidibus * mmb = perf().master_bus();
    const clockslist & opm = output_port_map();
    bool outportmap = opm.active();
    QComboBox * out = ui->comboBoxMidiOutBuss;
    if (not_nullptr(mmb))
    {
        int buses = outportmap ? opm.count() : mmb->get_num_out_buses() ;
        ui->clocksScrollArea->setWidget(central);
        ui->clocksScrollArea->setWidgetResizable(true);
        for (int bus = 0; bus < buses; ++bus)
        {
            qclocklayout * tempqc = new qclocklayout(this, perf(), bus);
            vboxclocks->addLayout(tempqc->layout());
        }

        /*
         * Output MIDI control buss combo-box population.
         */

        setup_clock_combo_box(buses, out);
        connect
        (
            out, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slot_output_bus(int))
        );
        connect
        (
            ui->checkBoxMidiOutBuss, SIGNAL(clicked(bool)),
            this, SLOT(slot_output_bus_enable())
        );
    }

    QSpacerItem * spacer = new QSpacerItem
    (
        40, 20, QSizePolicy::Expanding, QSizePolicy::Expanding
    );
    vboxclocks->addItem(spacer);
    connect
    (
        ui->spinBoxClockStartModulo, SIGNAL(valueChanged(int)),
        this, SLOT(slot_clock_start_modulo(int))
    );
    ui->lineEditTempoTrack->setEnabled(true);
    ui->lineEditTempoTrack->setReadOnly(false);

    std::string tn = std::to_string(rc().tempo_track_number());
    ui->lineEditTempoTrack->setText(qt(tn));
    connect
    (
        ui->lineEditTempoTrack, SIGNAL(editingFinished()),
        this, SLOT(slot_tempo_track())
    );

    /*
     * Buss override (simplistic for now)
     */

    int buss_override = usr().midi_buss_override();
    std::string botext = std::to_string(buss_override);
    ui->lineEditBussOverride->setText(qt(botext));
    connect
    (
        ui->lineEditBussOverride, SIGNAL(editingFinished()),
        this, SLOT(slot_buss_override())
    );

    /*
     * Tempo precision combo-box.
     */

    for (int i = 0; i <= 2; ++i)            /* c_max_bpm_precision */
    {
        QString s = QString::number(i);
        ui->comboBoxBpmPrecision->insertItem(i, s);
    }
    ui->comboBoxBpmPrecision->setCurrentIndex(usr().bpm_precision());
    connect
    (
        ui->comboBoxBpmPrecision, SIGNAL(currentIndexChanged(int)),
        this, SLOT(slot_bpm_precision(int))
    );
    ui->pushButtonTempoTrack->setEnabled(false);
    connect
    (
        ui->pushButtonTempoTrack, SIGNAL(clicked(bool)),
        this, SLOT(slot_tempo_track_set())
    );
    connect
    (
        ui->pushButtonStoreMap, SIGNAL(clicked(bool)),
        this, SLOT(slot_io_maps())
    );
    connect
    (
        ui->pushButtonRemoveMap, SIGNAL(clicked(bool)),
        this, SLOT(slot_remove_io_maps())
    );

    /*
     * The output and input port-map active check-boxes are both in the
     * Clocks tab.
     */

    bool inportmap = input_port_map().active();
    ui->ioPortsMappedCheck->setChecked(inportmap && outportmap);
    connect
    (
        ui->ioPortsMappedCheck, SIGNAL(clicked(bool)),
        this, SLOT(slot_activate_io_maps())
    );

    std::string clid = perf().client_id_string();
    ui->lineEditClientId->setText(qt(clid));
}

/*
 *---------------------------------------------------------------------
 *  MIDI Input tab.
 *---------------------------------------------------------------------
 */

void
qseditoptions::setup_tab_midi_input ()
{
    ui->tabWidget->setTabToolTip
    (
        Tab_MIDI_Input, "Input ports and I/O options"
    );

    /*
     * Set up the MIDI Input tab.  It is simpler, just a list of check-boxes
     * in the groupBoxInputs widget.  No need for a separate class.  However,
     * note that our qinputcheckbox class controls the activation, in the GUI,
     * of each input port, based on its buss status.
     */

    QWidget * central2 = new QWidget;
    QVBoxLayout * vboxinputs = new QVBoxLayout(central2);
    mastermidibus * mmb = perf().master_bus();
    const inputslist & ipm = input_port_map();
    bool inportmap = ipm.active();
    QComboBox * in = ui->comboBoxMidiInBuss;
    if (not_nullptr(mmb))
    {
        int buses = inportmap ? ipm.count() : mmb->get_num_in_buses() ;
        ui->inputsScrollArea->setWidget(central2);
        ui->inputsScrollArea->setWidgetResizable(true);
        for (int bus = 0; bus < buses; ++bus)
        {
            qinputcheckbox * tempqi = new qinputcheckbox(this, perf(), bus);
            vboxinputs->addWidget(tempqi->input_checkbox());
        }

        /*
         * Input MIDI control buss combo-box population.
         */

        setup_input_combo_box(buses, in);
        connect
        (
            in, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slot_input_bus(int))
        );
        connect
        (
            ui->checkBoxMidiInBuss, SIGNAL(clicked(bool)),
            this, SLOT(slot_input_bus_enable())
        );
    }

    /*
     * I/O Port Boolean options.
     */

    connect
    (
        ui->checkBoxRecordByBuss, SIGNAL(clicked(bool)),
        this, SLOT(slot_record_by_buss())
    );

    connect
    (
        ui->checkBoxRecordByChannel, SIGNAL(clicked(bool)),
        this, SLOT(slot_record_by_channel())
    );

    bool isvirtualports = rc().manual_ports();
    ui->checkBoxVirtualPorts->setChecked(isvirtualports);
    connect
    (
        ui->checkBoxVirtualPorts, SIGNAL(clicked(bool)),
        this, SLOT(slot_virtual_ports())
    );

    bool enablevirtualports = rc().manual_auto_enable();
    ui->checkBoxAutoEnableVirtual->setChecked(enablevirtualports);
    connect
    (
        ui->checkBoxAutoEnableVirtual, SIGNAL(clicked(bool)),
        this, SLOT(slot_enable_virtual_ports())
    );

    /*
     * The virtual port counts for input and output.
     */

    ui->lineEditOutputCount->setEnabled(isvirtualports);
    connect
    (
        ui->lineEditOutputCount, SIGNAL(editingFinished()),
        this, SLOT(slot_virtual_out_count())
    );
    ui->lineEditInputCount->setEnabled(isvirtualports);
    connect
    (
        ui->lineEditInputCount, SIGNAL(editingFinished()),
        this, SLOT(slot_virtual_in_count())
    );

    QSpacerItem * spacer2 = new QSpacerItem
    (
        40, 20, QSizePolicy::Expanding, QSizePolicy::Expanding
    );
    vboxinputs->addItem(spacer2);
}

/*
 *---------------------------------------------------------------------
 * Display tab.
 *---------------------------------------------------------------------
 */

void
qseditoptions::setup_tab_display ()
{
    ui->tabWidget->setTabToolTip
    (
        Tab_Display, "User-interface tweaks and boolean options"
    );

    ui->spinKeyHeight->setMinimum(usr().min_key_height());
    ui->spinKeyHeight->setMaximum(usr().max_key_height());
    connect
    (
        ui->spinKeyHeight, SIGNAL(valueChanged(int)),
        this, SLOT(slot_key_height())
    );
    connect
    (
        ui->lineEditUiScaling, SIGNAL(editingFinished()),
        this, SLOT(slot_ui_scaling())
    );
    connect
    (
        ui->lineEditUiScalingHeight, SIGNAL(editingFinished()),
        this, SLOT(slot_ui_scaling())
    );

    std::string spacingtext = int_to_string(usr().mainwnd_spacing());
    ui->lineEditGridSpacing->setText(qt(spacingtext));
    connect
    (
        ui->lineEditGridSpacing, SIGNAL(editingFinished()),
        this, SLOT(slot_grid_spacing())
    );
    connect
    (
        ui->lineEditSetSize, SIGNAL(editingFinished()),
        this, SLOT(slot_set_size_rows())
    );
    connect
    (
        ui->lineEditSetSizeColumns, SIGNAL(editingFinished()),
        this, SLOT(slot_set_size_columns())
    );
    connect
    (
        ui->lineEditProgressBox, SIGNAL(editingFinished()),
        this, SLOT(slot_progress_box_width())
    );
    connect
    (
        ui->lineEditProgressBoxHeight, SIGNAL(editingFinished()),
        this, SLOT(slot_progress_box_height())
    );
    connect
    (
        ui->checkBoxProgressBoxShown, SIGNAL(clicked(bool)),
        this, SLOT(slot_progress_box_shown())
    );
    connect
    (
        ui->lineEditFingerprintSize, SIGNAL(editingFinished()),
        this, SLOT(slot_fingerprint_size())
    );

#if defined USE_VERBOSE_CHECKBOX

    /*
     * Verbosity is meant to be only a temporary setting for one run.
     * It is set via --verbose or by editing the 'rc' file, but is
     * set to false at application exit. This control is read-only.
     */

     ui->checkBoxVerbose->setEnabled(true);
     connect
     (
         ui->checkBoxVerbose, SIGNAL(clicked(bool)),
         this, SLOT(slot_verbose_active_click())
     );

#endif

     ui->checkBoxQuiet->setChecked(rc().quiet());
     connect
     (
         ui->checkBoxQuiet, SIGNAL(clicked(bool)),
         this, SLOT(slot_quiet_active_click())
     );

    connect
    (
        ui->checkBoxLoadMostRecent, SIGNAL(clicked(bool)),
        this, SLOT(slot_load_most_recent_click())
    );
    connect
    (
        ui->checkBoxShowFullRecentPaths, SIGNAL(clicked(bool)),
        this, SLOT(slot_show_full_paths_click())
    );
    connect
    (
        ui->checkBoxLongBussNames, SIGNAL(clicked(bool)),
        this, SLOT(slot_long_buss_names_click())
    );
    connect
    (
        ui->checkBoxPairBussNames, SIGNAL(clicked(bool)),
        this, SLOT(slot_pair_buss_names_click())
    );
    connect
    (
        ui->checkBoxLockMainWindow, SIGNAL(clicked(bool)),
        this, SLOT(slot_lock_main_window_click())
    );
    ui->checkBoxDarkTheme->setChecked(usr().dark_theme());
    connect
    (
        ui->checkBoxDarkTheme, SIGNAL(clicked(bool)),
        this, SLOT(slot_dark_theme_click())
    );
    connect
    (
        ui->checkBoxSwapCoordinates, SIGNAL(clicked(bool)),
        this, SLOT(slot_swap_coordinates_click())
    );
    connect
    (
        ui->checkBoxBoldGridSlots, SIGNAL(clicked(bool)),
        this, SLOT(slot_bold_grid_slots_click())
    );
    ui->checkBoxElliptical->setChecked(usr().progress_box_elliptical());
    connect
    (
        ui->checkBoxElliptical, SIGNAL(clicked(bool)),
        this, SLOT(slot_elliptical_click())
    );
    ui->checkBoxFollowProgress->setChecked(usr().follow_progress());
    connect
    (
        ui->checkBoxFollowProgress, SIGNAL(clicked(bool)),
        this, SLOT(slot_follow_progress_click())
    );
    connect
    (
        ui->checkBoxDoubleClickEdit, SIGNAL(clicked(bool)),
        this, SLOT(slot_double_click_edit())
    );
    connect
    (
        ui->checkBoxGlobalSeqFeature, SIGNAL(clicked(bool)),
        this, SLOT(slot_global_seq_feature())
    );
}

/*
 *---------------------------------------------------------------------
 * Jack Sync tab.
 *---------------------------------------------------------------------
 */

void
qseditoptions::setup_tab_jack ()
{

#if defined SEQ66_JACK_SUPPORT

    ui->tabWidget->setTabToolTip
    (
        Tab_JACK, "JACK transport and server options"
    );

    /*
     * JACK Transport Connect/Disconnect buttons.
     */

    connect
    (
        ui->btnJackConnect, SIGNAL(clicked(bool)),
        this, SLOT(slot_jack_connect())
    );
    connect
    (
        ui->btnJackDisconnect, SIGNAL(clicked(bool)),
        this, SLOT(slot_jack_disconnect())
    );

    /*
     * JACK Transport/MIDI tab.
     */

    connect
    (
        ui->chkJackTransport, SIGNAL(stateChanged(int)),
        this, SLOT(slot_transport_support())
    );
    connect
    (
        ui->chkJackConditional, SIGNAL(stateChanged(int)),
        this, SLOT(slot_master_cond())
    );
    connect
    (
        ui->chkJackMaster, SIGNAL(stateChanged(int)),
        this, SLOT(slot_time_master())
    );
    connect
    (
        ui->chkJackNative, SIGNAL(stateChanged(int)),
        this, SLOT(slot_jack_midi())
    );
    connect
    (
        ui->chkJackAutoConnect, SIGNAL(stateChanged(int)),
        this, SLOT(slot_jack_auto_connect())
    );

    /*
     * Display of current JACK Server settings.
     */

    jack_assistant::parameters pos = jack_assistant::get_jack_parameters();
    std::string temp = std::to_string(int(pos.position.frame_rate));
    ui->lineEditFrameRate->setText(qt(temp));

    temp = std::to_string(pos.period_size);                     /* int      */
    ui->lineEditPeriod->setText(qt(temp));

    /*
     * Need a way to determine this.
     *
    temp = std::to_string(pos.alsa_nperiod);                    // int      //
    ui->lineEditNperiod->setText(qt(temp));
     */
    ui->lineEditNperiod->setText("?");

    char tmp[32];
    snprintf(tmp, sizeof tmp, "%.2f", pos.position.ticks_per_beat);
    ui->lineEditTicksPerBeat->setText(tmp);

    snprintf(tmp, sizeof tmp, "%.2f", pos.position.beats_per_minute);
    ui->lineEditBeatsPerMinute->setText(tmp);

    /*
     * Create a button group to manage the mutual status of the JACK Live and
     * Song Mode buttons.
     */

    m_live_song_buttons = new QButtonGroup(this);
    m_live_song_buttons->addButton(ui->radio_live_mode, playmode_button_live);
    m_live_song_buttons->addButton(ui->radio_song_mode, playmode_button_song);
    connect
    (
        m_live_song_buttons, SIGNAL(buttonClicked(int)),
        this, SLOT(slot_jack_mode(int))
    );

#else

    ui->tabWidget->setTabToolTip
    (
        Tab_JACK, "JACK not supported for this system"
    );

    ui->btnJackConnect->setEnabled(false);
    ui->btnJackDisconnect->setEnabled(false);
    ui->chkJackTransport->setEnabled(false);
    ui->chkJackConditional->setEnabled(false);
    ui->chkJackMaster->setEnabled(false);
    ui->chkJackNative->setEnabled(false);
    ui->chkJackAutoConnect->setEnabled(false);
    ui->tabWidget->setTabEnabled(Tab_JACK, false);

#endif  // defined SEQ66_JACK_SUPPORT
}

/*
 *---------------------------------------------------------------------
 * Play Options tab.
 *---------------------------------------------------------------------
 */

void
qseditoptions::setup_tab_play_options ()
{
    ui->tabWidget->setTabToolTip
    (
        Tab_Play_Options, "PPQN and playback options"
    );
    connect
    (
        ui->chkNoteResume, SIGNAL(stateChanged(int)),
        this, SLOT(slot_note_resume())
    );
    connect
    (
        ui->chkUseFilesPPQN, SIGNAL(stateChanged(int)),
        this, SLOT(slot_use_file_ppqn())
    );
    connect
    (
        ui->chkSongRecordSnap, SIGNAL(stateChanged(int)),
        this, SLOT(slot_song_record_snap())
    );

    /*
     *  Combo-box for changing the default PPQN.
     */

    (void) set_ppqn_combo();
    connect
    (
        ui->combo_box_ppqn, SIGNAL(currentTextChanged(const QString &)),
        this, SLOT(slot_ppqn_by_text(const QString &))
    );
    (void) set_buffer_size_combo();
    connect
    (
        ui->combo_box_buffer_size, SIGNAL(currentTextChanged(const QString &)),
        this, SLOT(slot_buffer_size_by_text(const QString &))
    );

    /*
     *  Group-box for changing the "sets mode".
     */

    QButtonGroup * rgroup = new QButtonGroup(this);
    rgroup->addButton(ui->radio_setsmode_normal, setsmode_button_normal);
    rgroup->addButton(ui->radio_setsmode_autoarm, setsmode_button_autoarm);
    rgroup->addButton(ui->radio_setsmode_additive, setsmode_button_additive);
    rgroup->addButton(ui->radio_setsmode_allsets, setsmode_button_allsets);
    connect
    (
        rgroup, SIGNAL(buttonClicked(int)),
        this, SLOT(slot_sets_mode(int))
    );

    /*
     *  Group-box for changing the "start mode".
     */

    QButtonGroup * rgroup2 = new QButtonGroup(this);
    rgroup2->addButton(ui->radio_startmode_live, startmode_button_live);
    rgroup2->addButton(ui->radio_startmode_song, startmode_button_song);
    rgroup2->addButton(ui->radio_startmode_auto, startmode_button_auto);
    connect
    (
        rgroup2, SIGNAL(buttonClicked(int)),
        this, SLOT(slot_start_mode(int))
    );
}

/*
 *---------------------------------------------------------------------
 * Metronome tab.
 *---------------------------------------------------------------------
 */

void
qseditoptions::setup_tab_metronome ()
{
    ui->tabWidget->setTabToolTip
    (
        Tab_Metronome, "Options for metronome and count-in"
    );

    int metrotemp = rc().metro_settings().beats_per_bar();
    QString qmetrotemp = qt(std::to_string(metrotemp));
    ui->lineedit_metro_beats_per_bar->setText(qmetrotemp);
    connect
    (
        ui->lineedit_metro_beats_per_bar, SIGNAL(editingFinished()),
        this, SLOT(slot_metro_beats_per_bar())
    );
    metrotemp = rc().metro_settings().beat_width();
    qmetrotemp = qt(std::to_string(metrotemp));
    ui->lineedit_metro_beat_width->setText(qmetrotemp);
    connect
    (
        ui->lineedit_metro_beat_width, SIGNAL(editingFinished()),
        this, SLOT(slot_metro_beat_width())
    );
    metrotemp = rc().metro_settings().main_patch();
    qmetrotemp = qt(std::to_string(metrotemp));
    ui->lineedit_metro_main_patch->setText(qmetrotemp);
    connect
    (
        ui->lineedit_metro_main_patch, SIGNAL(editingFinished()),
        this, SLOT(slot_metro_main_patch())
    );
    metrotemp = rc().metro_settings().main_note();
    qmetrotemp = qt(std::to_string(metrotemp));
    ui->lineedit_metro_main_note->setText(qmetrotemp);
    connect
    (
        ui->lineedit_metro_main_note, SIGNAL(editingFinished()),
        this, SLOT(slot_metro_main_note())
    );
    metrotemp = rc().metro_settings().main_note_velocity();
    qmetrotemp = qt(std::to_string(metrotemp));
    ui->lineedit_metro_main_velocity->setText(qmetrotemp);
    connect
    (
        ui->lineedit_metro_main_velocity, SIGNAL(editingFinished()),
        this, SLOT(slot_metro_main_velocity())
    );

    float metrofrac = rc().metro_settings().main_note_fraction();
    QString qmetrofrac = qt(double_to_string(metrofrac, 3));
    ui->lineedit_metro_main_fraction->setText(qmetrofrac);
    connect
    (
        ui->lineedit_metro_main_fraction, SIGNAL(editingFinished()),
        this, SLOT(slot_metro_main_fraction())
    );
    metrotemp = rc().metro_settings().sub_patch();
    qmetrotemp = qt(std::to_string(metrotemp));
    ui->lineedit_metro_sub_patch->setText(qmetrotemp);
    connect
    (
        ui->lineedit_metro_sub_patch, SIGNAL(editingFinished()),
        this, SLOT(slot_metro_sub_patch())
    );
    metrotemp = rc().metro_settings().sub_note();
    qmetrotemp = qt(std::to_string(metrotemp));
    ui->lineedit_metro_sub_note->setText(qmetrotemp);
    connect
    (
        ui->lineedit_metro_sub_note, SIGNAL(editingFinished()),
        this, SLOT(slot_metro_sub_note())
    );
    metrotemp = rc().metro_settings().sub_note_velocity();
    qmetrotemp = qt(std::to_string(metrotemp));
    ui->lineedit_metro_sub_velocity->setText(qmetrotemp);
    connect
    (
        ui->lineedit_metro_sub_velocity, SIGNAL(editingFinished()),
        this, SLOT(slot_metro_sub_velocity())
    );
    metrofrac = rc().metro_settings().sub_note_fraction();
    qmetrofrac = qt(double_to_string(metrofrac, 3));
    ui->lineedit_metro_sub_fraction->setText(qmetrofrac);
    connect
    (
        ui->lineedit_metro_sub_fraction, SIGNAL(editingFinished()),
        this, SLOT(slot_metro_sub_fraction())
    );
    ui->button_metro_reload->setEnabled(false);
    connect
    (
        ui->button_metro_reload, SIGNAL(clicked(bool)),
        this, SLOT(slot_metro_reload())
    );

    /*
     * combobox_metro_buss:
     *
     *  Code similar to that in qsmainwnd.  Output MIDI control
     *  buss combo-box population.
     */

    const clockslist & opm = output_port_map();
    mastermidibus * mmb = perf().master_bus();
    QComboBox * out = ui->combobox_metro_buss;
    out->clear();
    if (not_nullptr(mmb))
    {
        int metrobus = int(rc().metro_settings().buss());
        int buses = opm.active() ? opm.count() : mmb->get_num_out_buses() ;
        for (int bus = 0; bus < buses; ++bus)
        {
            e_clock ec;
            std::string busname;
            if (perf().ui_get_clock(bussbyte(bus), ec, busname))
            {
                bool enabled = ec != e_clock::disabled;
                out->addItem(qt(busname));
                enable_combobox_item(out, bus, enabled);
            }
        }
        ui->combobox_metro_buss->setCurrentIndex(metrobus);
        connect
        (
            ui->combobox_metro_buss, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slot_metro_buss(int))
        );
    }

    /*
     * combobox_metro_channel
     */

    repopulate_channel_menu(int(rc().metro_settings().buss()));

    /*
     *  Count-in and recorder settings.
     */

    bool count_in_active = rc().metro_settings().count_in_active();
    ui->checkbox_metro_count_in->setChecked(count_in_active);
    connect
    (
        ui->checkbox_metro_count_in, SIGNAL(clicked(bool)),
        this, SLOT(slot_metro_count_in())
    );
    metrotemp = rc().metro_settings().count_in_measures();
    qmetrotemp = qt(std::to_string(metrotemp));
    ui->lineedit_metro_count_in->setText(qmetrotemp);
    connect
    (
        ui->lineedit_metro_count_in, SIGNAL(editingFinished()),
        this, SLOT(slot_metro_count_in_measures())
    );
    count_in_active = rc().metro_settings().count_in_recording();
    ui->checkbox_metro_recording->setChecked(count_in_active);
    connect
    (
        ui->checkbox_metro_recording, SIGNAL(clicked(bool)),
        this, SLOT(slot_metro_recording())
    );
    metrotemp = rc().metro_settings().recording_measures();
    qmetrotemp = qt(std::to_string(metrotemp));
    ui->lineedit_metro_recording_measures->setText(qmetrotemp);
    connect
    (
        ui->lineedit_metro_recording_measures, SIGNAL(editingFinished()),
        this, SLOT(slot_metro_recording_measures())
    );

    /*
     * combobox_metro_record_buss:
     *
     *  Code similar to that in qsmainwnd.  Output MIDI control
     *  buss combo-box population.
     */

    const inputslist & ipm = input_port_map();
    QComboBox * in = ui->combobox_metro_record_buss;
    in->clear();
    if (not_nullptr(mmb))
    {
        int enabled_count = 0;
        int last_input = (-1);
        int buses = ipm.active() ? ipm.count() : mmb->get_num_in_buses() ;
        for (int bus = 0; bus < buses; ++bus)
        {
            std::string busname;
            bool inputing;
            bool good = perf().ui_get_input(bus, inputing, busname);
            if (good)
            {
                /*
                 * For this dialog, we want to allow the selection of
                 * a buss that is enabled for input, except for system
                 * ports, which should never be used.
                 */

                bool enabled = inputing && ! perf().is_input_system_port(bus);
                in->addItem(qt(busname));
                enable_combobox_item(in, bus, enabled);
                if (enabled)
                {
                    ++enabled_count;
                    last_input = bus;
                }
            }
        }
        ui->combobox_metro_record_buss->setCurrentIndex(last_input);
        if (enabled_count == 1)
            rc().metro_settings().recording_buss(midibyte(last_input));

        connect
        (
            ui->combobox_metro_record_buss, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slot_metro_record_buss(int))
        );
    }

    /*
     * combobox_metro_thru_buss
     */

    out = ui->combobox_metro_thru_buss;
    out->clear();
    if (not_nullptr(mmb))
    {
        int thrubus = int(rc().metro_settings().thru_buss());
        int buses = opm.active() ? opm.count() : mmb->get_num_out_buses() ;
        for (int bus = 0; bus < buses; ++bus)
        {
            e_clock ec;
            std::string busname;
            if (perf().ui_get_clock(bussbyte(bus), ec, busname))
            {
                bool enabled = ec != e_clock::disabled;
                out->addItem(qt(busname));
                enable_combobox_item(out, bus, enabled);
            }
        }
        out->setCurrentIndex(thrubus);
        connect
        (
            out, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slot_metro_thru_buss(int))
        );
    }

    /*
     * combobox_metro_thru_channel
     */

    repopulate_thru_channel_menu(int(rc().metro_settings().thru_buss()));
}

void
qseditoptions::repopulate_channel_menu (int buss)
{
    disconnect
    (
        ui->combobox_metro_channel, SIGNAL(currentIndexChanged(int)),
        this, SLOT(slot_metro_channel(int))
    );
    ui->combobox_metro_channel->clear();
    for (int channel = 0; channel < c_midichannel_max; ++channel)
    {
        char b[4];                                      /* 2 digits or less */
        snprintf(b, sizeof b, "%2d", channel + 1);      /* user-style no.   */
        std::string name = std::string(b);
        std::string s = usr().instrument_name(buss, channel);
        if (! s.empty())
        {
            name += " [";
            name += s;
            name += "]";
        }

        QString combo_text(qt(name));
        ui->combobox_metro_channel->insertItem(channel, combo_text);
    }

    int ch = rc().metro_settings().channel();
    if (is_null_channel(ch))
        ch = 0;

    ui->combobox_metro_channel->setCurrentIndex(ch);
    connect
    (
        ui->combobox_metro_channel, SIGNAL(currentIndexChanged(int)),
        this, SLOT(slot_metro_channel(int))
    );
}

void
qseditoptions::repopulate_thru_channel_menu (int buss)
{
    disconnect
    (
        ui->combobox_metro_thru_channel, SIGNAL(currentIndexChanged(int)),
        this, SLOT(slot_metro_thru_channel(int))
    );
    ui->combobox_metro_thru_channel->clear();
    for (int channel = 0; channel < c_midichannel_max; ++channel)
    {
        char b[4];                                      /* 2 digits or less */
        snprintf(b, sizeof b, "%2d", channel + 1);      /* user-style no.   */
        std::string name = std::string(b);
        std::string s = usr().instrument_name(buss, channel);
        if (! s.empty())
        {
            name += " [";
            name += s;
            name += "]";
        }

        QString combo_text(qt(name));
        ui->combobox_metro_thru_channel->insertItem(channel, combo_text);
    }

    int ch = rc().metro_settings().thru_channel();
    if (is_null_channel(ch))
        ch = 0;

    ui->combobox_metro_thru_channel->setCurrentIndex(ch);
    connect
    (
        ui->combobox_metro_thru_channel, SIGNAL(currentIndexChanged(int)),
        this, SLOT(slot_metro_thru_channel(int))
    );
}

void
qseditoptions::modify_metronome (bool enablereload)
{
    if (enablereload)
        ui->button_metro_reload->setEnabled(true);

    modify_rc();
}

void
qseditoptions::slot_metro_beats_per_bar ()
{
    QString text = ui->lineedit_metro_beats_per_bar->text();
    std::string b = text.toStdString();
    int bpb = string_to_int(b);
    if (bpb != rc().metro_settings().beats_per_bar())
    {
        rc().metro_settings().beats_per_bar(bpb);
        modify_metronome();
    }
}

void
qseditoptions::slot_metro_beat_width ()
{
    QString text = ui->lineedit_metro_beat_width->text();
    std::string b = text.toStdString();
    int bw = string_to_int(b);
    if (bw != rc().metro_settings().beat_width())
    {
        rc().metro_settings().beat_width(bw);
        modify_metronome();
    }
}

void
qseditoptions::slot_metro_main_patch ()
{
    QString text = ui->lineedit_metro_main_patch->text();
    std::string p = text.toStdString();
    int patch = string_to_int(p);
    if (patch != rc().metro_settings().main_patch())
    {
        rc().metro_settings().main_patch(patch);
        modify_metronome();
    }
}

void
qseditoptions::slot_metro_main_note ()
{
    QString text = ui->lineedit_metro_main_note->text();
    std::string n = text.toStdString();
    int note = string_to_int(n);
    if (note != rc().metro_settings().main_note())
    {
        rc().metro_settings().main_note(note);
        modify_metronome();
    }
}

void
qseditoptions::slot_metro_main_velocity ()
{
    QString text = ui->lineedit_metro_main_velocity->text();
    std::string v = text.toStdString();
    int vel = string_to_int(v);
    if (vel != rc().metro_settings().main_note_velocity())
    {
        rc().metro_settings().main_note_velocity(vel);
        modify_metronome();
    }
}

void
qseditoptions::slot_metro_main_fraction ()
{
    QString text = ui->lineedit_metro_main_fraction->text();
    std::string f = text.toStdString();
    float fraction = string_to_double(f);
    if (fraction != rc().metro_settings().main_note_fraction())
    {
        rc().metro_settings().main_note_fraction(fraction);
        modify_metronome();
    }
}

void
qseditoptions::slot_metro_sub_patch ()
{
    QString text = ui->lineedit_metro_sub_patch->text();
    std::string p = text.toStdString();
    int patch = string_to_int(p);
    if (patch != rc().metro_settings().sub_patch())
    {
        rc().metro_settings().sub_patch(patch);
        modify_metronome();
    }
}

void
qseditoptions::slot_metro_sub_note ()
{
    QString text = ui->lineedit_metro_sub_note->text();
    std::string n = text.toStdString();
    int note = string_to_int(n);
    if (note != rc().metro_settings().sub_note())
    {
        rc().metro_settings().sub_note(note);
        modify_metronome();
    }
}

void
qseditoptions::slot_metro_sub_velocity ()
{
    QString text = ui->lineedit_metro_sub_velocity->text();
    std::string v = text.toStdString();
    int vel = string_to_int(v);
    if (vel != rc().metro_settings().sub_note_velocity())
    {
        rc().metro_settings().sub_note_velocity(vel);
        modify_metronome();
    }
}

void
qseditoptions::slot_metro_sub_fraction ()
{
    QString text = ui->lineedit_metro_sub_fraction->text();
    std::string f = text.toStdString();
    float fraction = string_to_double(f);
    if (fraction != rc().metro_settings().sub_note_fraction())
    {
        rc().metro_settings().sub_note_fraction(fraction);
        modify_metronome();
    }
}

void
qseditoptions::slot_metro_reload ()
{
    ui->button_metro_reload->setEnabled(false);
    perf().reload_metronome();
}

void
qseditoptions::slot_metro_buss (int index)
{
    int b = int(rc().metro_settings().buss());
    if (index != b)
    {
        rc().metro_settings().buss(midibyte(index));
        repopulate_channel_menu(int(rc().metro_settings().buss()));
        modify_metronome();
    }
}

void
qseditoptions::slot_metro_channel (int index)
{
    int c = int(rc().metro_settings().channel());
    if (index != c)
    {
        rc().metro_settings().channel(midibyte(index));
        modify_metronome();
    }
}

void
qseditoptions::slot_metro_count_in ()
{
    bool on = ui->checkbox_metro_count_in->isChecked();
    rc().metro_settings().count_in_active(on);
    modify_metronome();
}

void
qseditoptions::slot_metro_count_in_measures ()
{
    QString text = ui->lineedit_metro_count_in->text();
    std::string m = text.toStdString();
    int measures = string_to_int(m);
    if (measures != rc().metro_settings().count_in_measures())
    {
        rc().metro_settings().count_in_measures(measures);
        modify_metronome();
    }
}

void
qseditoptions::slot_metro_recording ()
{
    bool on = ui->checkbox_metro_recording->isChecked();
    rc().metro_settings().count_in_recording(on);
    modify_metronome(true);                        /* no reload button */
}

void
qseditoptions::slot_metro_recording_measures ()
{
    QString text = ui->lineedit_metro_recording_measures->text();
    std::string m = text.toStdString();
    int measures = string_to_int(m);
    if (measures != rc().metro_settings().recording_measures())
    {
        rc().metro_settings().recording_measures(measures);
        modify_metronome(true);                    /* no reload button */
    }
}

void
qseditoptions::slot_metro_record_buss (int index)
{
    int b = int(rc().metro_settings().recording_buss());
    if (index != b)
    {
        rc().metro_settings().recording_buss(midibyte(index));
        modify_metronome(true);                    /* no reload button */
    }
}

void
qseditoptions::slot_metro_thru_buss (int index)
{
    int b = int(rc().metro_settings().thru_buss());
    if (index != b)
    {
        rc().metro_settings().thru_buss(midibyte(index));
        repopulate_thru_channel_menu(int(rc().metro_settings().thru_buss()));
        modify_metronome();
    }
}

void
qseditoptions::slot_metro_thru_channel (int index)
{
    int c = int(rc().metro_settings().thru_channel());
    if (index != c)
    {
        rc().metro_settings().thru_channel(midibyte(index));
        modify_metronome();
    }
}

/*
 *---------------------------------------------------------------------
 *  Pattern tab.
 *---------------------------------------------------------------------
 */

/*
 * Duty now for the Future!
 *
 *  checkBoxEscapePattern (for issue #117)
 *  checkBoxNewPatternArm
 *  checkBoxNewPatternRecord
 *  checkBoxNewPatternTighten
 *  checkBoxNewPatternQRecord
 *  checkBoxNewPatternNoteMap
 *  checkBoxNewPatternThru
 *  checkBoxNewPatternWrapAround
 *  comboBoxRecordStyle
 *
 *  spinBoxJitter
 *  spinBoxAmplitude
 */

void
qseditoptions::setup_tab_pattern ()
{
    ui->tabWidget->setTabToolTip
    (
        Tab_Pattern, "Options for new patterns and randomization"
    );

    /*
     * New-pattern boolean options.
     */

    ui->checkBoxEscapePattern->setChecked(usr().escape_pattern());
    connect
    (
        ui->checkBoxEscapePattern, SIGNAL(clicked(bool)),
        this, SLOT(slot_escape_pattern())
    );
    ui->checkBoxNewPatternArm->setChecked(usr().new_pattern_armed());
    connect
    (
        ui->checkBoxNewPatternArm, SIGNAL(clicked(bool)),
        this, SLOT(slot_new_pattern_arm())
    );
    ui->checkBoxNewPatternTighten->setChecked(usr().new_pattern_tighten());
    connect
    (
        ui->checkBoxNewPatternTighten, SIGNAL(clicked(bool)),
        this, SLOT(slot_new_pattern_tighten())
    );
    ui->checkBoxNewPatternQRecord->setChecked(usr().new_pattern_qrecord());
    connect
    (
        ui->checkBoxNewPatternQRecord, SIGNAL(clicked(bool)),
        this, SLOT(slot_new_pattern_qrecord())
    );
    ui->checkBoxNewPatternNoteMap->setChecked(usr().new_pattern_notemap());
    connect
    (
        ui->checkBoxNewPatternNoteMap, SIGNAL(clicked(bool)),
        this, SLOT(slot_new_pattern_notemap())
    );
    ui->checkBoxNewPatternRecord->setChecked(usr().new_pattern_record());
    connect
    (
        ui->checkBoxNewPatternRecord, SIGNAL(clicked(bool)),
        this, SLOT(slot_new_pattern_record())
    );
    ui->checkBoxNewPatternThru->setChecked(usr().new_pattern_thru());
    connect
    (
        ui->checkBoxNewPatternThru, SIGNAL(clicked(bool)),
        this, SLOT(slot_new_pattern_thru())
    );
    ui->checkBoxNewPatternWrapAround->setChecked
    (
        usr().new_pattern_wraparound()
    );
    connect
    (
        ui->checkBoxNewPatternWrapAround, SIGNAL(clicked(bool)),
        this, SLOT(slot_new_pattern_wraparound())
    );

    /*
     * New-pattern record-style options. These should be an array/function
     * in the settings module! Note the handling of this list is different
     * from PPQN and Buffer-size list handling.
     */

    const tokenization & items = rec_style_items();     // settings module
    ui->comboBoxRecordStyle->addItem(qt(items[0]));     // "Merge"
    ui->comboBoxRecordStyle->addItem(qt(items[1]));     // "Overwrite"
    ui->comboBoxRecordStyle->addItem(qt(items[2]));     // "Expand"
    ui->comboBoxRecordStyle->addItem(qt(items[3]));     // "Oneshot"
    ui->comboBoxRecordStyle->addItem(qt(items[4]));     // "Oneshot Reset"

    int r = usr().new_pattern_record_code();
    ui->comboBoxRecordStyle->setCurrentIndex(r);
    connect
    (
        ui->comboBoxRecordStyle, SIGNAL(currentIndexChanged(int)),
        this, SLOT(slot_new_record_style(int))
    );

    /*
     * Randomization options.
     */

    ui->spinBoxJitter->setValue(usr().jitter_divisor());
    connect
    (
        ui->spinBoxJitter, SIGNAL(valueChanged(int)),
        this, SLOT(slot_jitter(int))
    );
    ui->spinBoxAmplitude->setValue(usr().randomization_amount());
    connect
    (
        ui->spinBoxAmplitude, SIGNAL(valueChanged(int)),
        this, SLOT(slot_amplitude(int))
    );
}

void
qseditoptions::slot_escape_pattern ()
{
    bool enable = ui->checkBoxEscapePattern->isChecked();
    usr().escape_pattern(enable);
    modify_usr();
}

void
qseditoptions::slot_new_pattern_arm ()
{
    bool enable = ui->checkBoxNewPatternArm->isChecked();
    usr().new_pattern_armed(enable);
    modify_usr();
}

/**
 *  Note: currently tightening, quantizing, and note-mapping are mutually
 *  exclusive.  This GUI needs to enforce that. Or we need to allow
 *  note-mapping to occur along with one of the others.
 */

void
qseditoptions::slot_new_pattern_tighten ()
{
    bool enable = ui->checkBoxNewPatternTighten->isChecked();
    usr().new_pattern_tighten(enable);
    if (enable)
    {
        ui->checkBoxNewPatternQRecord->setChecked(false);
        ui->checkBoxNewPatternNoteMap->setChecked(false);
        usr().new_pattern_qrecord(false);
        usr().new_pattern_notemap(false);
    }
    modify_usr();
}

void
qseditoptions::slot_new_pattern_qrecord ()
{
    bool enable = ui->checkBoxNewPatternQRecord->isChecked();
    usr().new_pattern_qrecord(enable);
    if (enable)
    {
        ui->checkBoxNewPatternTighten->setChecked(false);
        ui->checkBoxNewPatternNoteMap->setChecked(false);
        usr().new_pattern_tighten(false);
        usr().new_pattern_notemap(false);
    }
    modify_usr();
}

void
qseditoptions::slot_new_pattern_notemap ()
{
    bool enable = ui->checkBoxNewPatternNoteMap->isChecked();
    usr().new_pattern_notemap(enable);
    if (enable)
    {
        ui->checkBoxNewPatternTighten->setChecked(false);
        ui->checkBoxNewPatternQRecord->setChecked(false);
        usr().new_pattern_tighten(false);
        usr().new_pattern_qrecord(false);
    }
    modify_usr();
}

void
qseditoptions::slot_new_pattern_record ()
{
    bool enable = ui->checkBoxNewPatternRecord->isChecked();
    usr().new_pattern_record(enable);
    modify_usr();
}

void
qseditoptions::slot_new_pattern_thru ()
{
    bool enable = ui->checkBoxNewPatternThru->isChecked();
    usr().new_pattern_thru(enable);
    modify_usr();
}

void
qseditoptions::slot_new_pattern_wraparound ()
{
    bool enable = ui->checkBoxNewPatternWrapAround->isChecked();
    usr().new_pattern_wraparound(enable);
    modify_usr();
}

void
qseditoptions::slot_new_record_style (int index)
{
    usr().new_pattern_record_style(index);
    modify_usr();
}

void
qseditoptions::slot_jitter (int jitr)
{
    usr().jitter_divisor(jitr);
    modify_usr();
}

void
qseditoptions::slot_amplitude (int amp)
{
    usr().randomization_amount(amp);
    modify_usr();
}

/*
 *---------------------------------------------------------------------
 *  Session tab.
 *---------------------------------------------------------------------
 */

void
qseditoptions::setup_tab_session ()
{
    ui->tabWidget->setTabToolTip
    (
        Tab_Session, "Options for session configuration files"
    );

    /*
     *  Group-box for changing the session.
     */

    QButtonGroup * sgroup = new QButtonGroup(this);
    sgroup->addButton
    (
        ui->radio_session_none, static_cast<int>(usrsettings::session::none)
    );
    sgroup->addButton
    (
        ui->radio_session_nsm, static_cast<int>(usrsettings::session::nsm)
    );
    sgroup->addButton
    (
        ui->radio_session_jack, static_cast<int>(usrsettings::session::jack)
    );
    connect
    (
        sgroup, SIGNAL(buttonClicked(int)),
        this, SLOT(slot_session(int))
    );

#if ! defined SEQ66_JACK_SESSION
    ui->radio_session_jack->setEnabled(false);
#endif

#if ! defined SEQ66_NSM_SUPPORT
    ui->radio_session_nsm->setEnabled(false);
#endif

    /*
     * The URL text-edit.
     */

    connect
    (
        ui->lineEditNsmUrl, SIGNAL(editingFinished()),
        this, SLOT(slot_nsm_url())
    );

    /*
     * The Configuration Files section.
     */

    /*
     * 'rc' file.  This file is ALWAYS active, so that check-box is read-only.
     */

    connect
    (
        ui->checkBoxSaveRc, SIGNAL(clicked(bool)),
        this, SLOT(slot_rc_save_click())
    );

#if defined USE_RC_NAME_CHANGE

    /*
     * There's no real way to change the 'rc' from the UI and have it
     * stick. It can be specified by the --config option only.
     * We could eventually support an environment variablie like
     * SEQ66_RC_FILE.
     */

    connect
    (
        ui->lineEditRc, SIGNAL(editingFinished()),
        this, SLOT(slot_rc_filename())
    );
    connect
    (
        ui->pushButtonLoadRc, SIGNAL(clicked(bool)),
        this, SLOT(slot_load_rc_filename())
    );
#else
    ui->lineEditRc->setReadOnly(true);
    ui->pushButtonLoadRc->hide();
#endif

    /*
     * 'usr' file. Making 'usr' inactive is no longer permitted, even
     * experimentally.
     */

#if defined SEQ66_CAN_DEACTIVATE_USR

    connect
    (
        ui->checkBoxActiveUsr, SIGNAL(clicked(bool)),
        this, SLOT(slot_usr_active_click())
    );

#else

    ui->checkBoxActiveUsr->setEnabled(false);

#endif

    connect
    (
        ui->checkBoxSaveUsr, SIGNAL(clicked(bool)),
        this, SLOT(slot_usr_save_click())
    );
    connect
    (
        ui->lineEditUsr, SIGNAL(editingFinished()),
        this, SLOT(slot_usr_filename())
    );

    /*
     * 'mutes' file
     */

    connect
    (
        ui->checkBoxActiveMutes, SIGNAL(clicked(bool)),
        this, SLOT(slot_mutes_active_click())
    );
    connect
    (
        ui->checkBoxSaveMutes, SIGNAL(clicked(bool)),
        this, SLOT(slot_mutes_save_click())
    );
    connect
    (
        ui->lineEditMutes, SIGNAL(editingFinished()),
        this, SLOT(slot_mutes_filename())
    );
    connect
    (
        ui->pushButtonLoadMutes, SIGNAL(clicked(bool)),
        this, SLOT(slot_load_mutes_filename())
    );

    /*
     * 'playlist' file
     */

    connect
    (
        ui->checkBoxActivePlaylist, SIGNAL(clicked(bool)),
        this, SLOT(slot_playlist_active_click())
    );
    connect
    (
        ui->checkBoxSavePlaylist, SIGNAL(clicked(bool)),
        this, SLOT(slot_playlist_save_click())
    );
    connect
    (
        ui->lineEditPlaylist, SIGNAL(editingFinished()),
        this, SLOT(slot_playlist_filename())
    );
    ui->pushButtonLoadPlaylist->hide();     /* hmmmmmm */
    connect
    (
        ui->pushButtonLoadPlaylist, SIGNAL(clicked(bool)),
        this, SLOT(slot_load_playlist_filename())
    );

    /*
     * 'ctrl' file.  Since this configuration is not editable while the
     * application is running, the "auto-save" check-box is read-only.
     */

    connect
    (
        ui->checkBoxActiveCtrl, SIGNAL(clicked(bool)),
        this, SLOT(slot_ctrl_active_click())
    );

#if defined SEQ66_CAN_SAVE_CTRL

    connect
    (
        ui->checkBoxSaveCtrl, SIGNAL(clicked(bool)),
        this, SLOT(slot_ctrl_save_click())
    );

#else

    ui->checkBoxSaveCtrl->setChecked(false);
    ui->checkBoxSaveCtrl->setEnabled(false);

#endif

    connect
    (
        ui->lineEditCtrl, SIGNAL(editingFinished()),
        this, SLOT(slot_ctrl_filename())
    );
    connect
    (
        ui->pushButtonLoadCtrl, SIGNAL(clicked(bool)),
        this, SLOT(slot_load_ctrl_filename())
    );

    /*
     * 'drums' file.  Since this configuration is not editable while the
     * application is running, the "auto-save" check-box is read-only.
     */

    connect
    (
        ui->checkBoxActiveDrums, SIGNAL(clicked(bool)),
        this, SLOT(slot_drums_active_click())
    );

#if defined SEQ66_CAN_SAVE_DRUMS

    connect
    (
        ui->checkBoxSaveDrums, SIGNAL(clicked(bool)),
        this, SLOT(slot_drums_save_click())
    );

#endif

    connect
    (
        ui->lineEditDrums, SIGNAL(editingFinished()),
        this, SLOT(slot_drums_filename())
    );
    connect
    (
        ui->pushButtonLoadDrums, SIGNAL(clicked(bool)),
        this, SLOT(slot_load_drums_filename())
    );

    /*
     * 'palette' file.
     */

#if defined SEQ66_CAN_SAVE_PALETTE

    connect
    (
        ui->checkBoxSavePalette, SIGNAL(clicked(bool)),
        this, SLOT(slot_palette_save_click())
    );

#endif

    connect
    (
        ui->checkBoxActivePalette, SIGNAL(clicked(bool)),
        this, SLOT(slot_palette_active_click())
    );
    connect
    (
        ui->lineEditPalette, SIGNAL(editingFinished()),
        this, SLOT(slot_palette_filename())
    );
    connect
    (
        ui->pushButtonLoadPalette, SIGNAL(clicked(bool)),
        this, SLOT(slot_load_palette_filename())
    );

    /*
     * A 'palette' extra.  Immediate saving of the palette.
     */

    connect
    (
        ui->pushButtonSavePalette, SIGNAL(clicked(bool)),
        this, SLOT(slot_palette_save_now_click())
    );

    /*
     * 'qss' file.  Since this configuration is not editable while the
     * application is running, the "auto-save" check-box is read-only.
     * Also note that this refers to a Qt Style Sheet.
     */

    connect
    (
        ui->checkBoxActiveStyleSheet, SIGNAL(clicked(bool)),
        this, SLOT(slot_stylesheet_active_click())
    );
    connect
    (
        ui->lineEditStyleSheet, SIGNAL(editingFinished()),
        this, SLOT(slot_stylesheet_filename())
    );
    connect
    (
        ui->pushButtonLoadStyleSheet, SIGNAL(clicked(bool)),
        this, SLOT(slot_load_stylesheet_filename())
    );

    /*
     * Viewers.
     */

    std::string viewer = usr().user_browser();
    if (viewer.empty())
        viewer = "Default";

    ui->lineEditBrowser->setText(qt(viewer));
    connect
    (
        ui->lineEditBrowser, SIGNAL(editingFinished()),
        this, SLOT(slot_browser_executable())
    );
    connect
    (
        ui->pushButtonSetBrowser, SIGNAL(clicked(bool)),
        this, SLOT(slot_load_browser_executable())
    );
    viewer = usr().user_pdf_viewer();
    if (viewer.empty())
        viewer = "Default";

    ui->lineEditPdfViewer->setText(qt(viewer));
    connect
    (
        ui->lineEditPdfViewer, SIGNAL(editingFinished()),
        this, SLOT(slot_pdf_executable())
    );
    connect
    (
        ui->pushButtonSetPdfViewer, SIGNAL(clicked(bool)),
        this, SLOT(slot_load_pdf_viewer_executable())
    );

    /*
     * This items is shown when certain files are changed.
     */

    ui->label_exit_required->hide();
}

void
qseditoptions::exit_required ()
{
    ui->label_exit_required->show();
}

bool
qseditoptions::set_ppqn_combo ()
{
    std::string p = std::to_string(int(perf().ppqn()));
    return fill_combobox(ui->combo_box_ppqn, ppqn_list(), p);
}

bool
qseditoptions::set_buffer_size_combo ()
{
    std::string sz = std::to_string(rc().jack_buffer_size());
    return fill_combobox(ui->combo_box_buffer_size, buffer_size_list(), sz);
}

void
qseditoptions::show_sets_mode (rcsettings::setsmode sm)
{
    switch (sm)
    {
        case rcsettings::setsmode::normal:
            ui->radio_setsmode_normal->setChecked(true);
            break;

        case rcsettings::setsmode::autoarm:
            ui->radio_setsmode_autoarm->setChecked(true);
            break;

        case rcsettings::setsmode::additive:
            ui->radio_setsmode_additive->setChecked(true);
            break;

        case rcsettings::setsmode::allsets:
            ui->radio_setsmode_allsets->setChecked(true);
            break;

        default:
            break;
    }
}

void
qseditoptions::show_start_mode (sequence::playback sm)
{
    switch (sm)
    {
        case sequence::playback::live:
            ui->radio_startmode_live->setChecked(true);
            break;

        case sequence::playback::song:
            ui->radio_startmode_song->setChecked(true);
            break;

        case sequence::playback::automatic:
            ui->radio_startmode_auto->setChecked(true);
            break;

        default:
            break;
    }
}

void
qseditoptions::slot_sets_mode (int buttonno)
{
    rcsettings::setsmode previous = rc().sets_mode();
    if (buttonno == setsmode_button_autoarm)
        rc().sets_mode("auto-arm");
    else if (buttonno == setsmode_button_additive)
        rc().sets_mode("additive");
    else if (buttonno == setsmode_button_allsets)
        rc().sets_mode("auto-arm");
    else
        rc().sets_mode("normal");

    if (rc().sets_mode() != previous)
        modify_rc();
}

void
qseditoptions::slot_start_mode (int buttonno)
{
    sequence::playback previous = rc().get_song_start_mode();
    if (buttonno == startmode_button_live)
        rc().song_start_mode_by_string("live");
    else if (buttonno == startmode_button_song)
        rc().song_start_mode_by_string("song");
    else
        rc().song_start_mode_by_string("auto");

    if (rc().get_song_start_mode() != previous)
        modify_rc();
}

/**
 *  A run-time-only setting.  No configuration modification.
 */

void
qseditoptions::slot_jack_mode (int buttonno)
{
    if (buttonno == playmode_button_live)
        perf().song_mode(false);
    else if (buttonno == playmode_button_song)
        perf().song_mode(true);
}

/**
 *  A run-time-only setting.  No configuration modification.
 */

void
qseditoptions::slot_jack_connect ()
{
    perf().set_jack_mode(true);
}

/**
 *  A run-time-only setting.  No configuration modification.
 */

void
qseditoptions::slot_jack_disconnect ()
{
    perf().set_jack_mode(false);
}

void
qseditoptions::slot_master_cond ()
{
    rc().with_jack_master_cond(ui->chkJackConditional->isChecked());
    modify_rc();
    sync();
}

void
qseditoptions::slot_time_master ()
{
    rc().with_jack_master(ui->chkJackMaster->isChecked());
    modify_rc();
    sync();
}

void
qseditoptions::slot_transport_support ()
{
    rc().with_jack_transport(ui->chkJackTransport->isChecked());
    modify_rc();
    sync();
}

void
qseditoptions::slot_jack_midi ()
{
    rc().with_jack_midi(ui->chkJackNative->isChecked());
    modify_rc();
    sync();
}

void
qseditoptions::slot_jack_auto_connect ()
{
    rc().jack_auto_connect(ui->chkJackAutoConnect->isChecked());
    modify_rc();
    sync();
}

void
qseditoptions::reload_needed (bool flag)
{
    if (flag)
        state_changed();
    else
        state_unchanged();

    sync();
}

/**
 *  State of buttons at startup and after Reset is clicked.  See the table
 *  at the top of this module.
 */

void
qseditoptions::state_unchanged ()
{
    set_enabled(QDialogButtonBox::Apply, false);
    set_enabled(QDialogButtonBox::Cancel, true);
    set_enabled(QDialogButtonBox::Reset, false);
    set_enabled(QDialogButtonBox::Ok, false);
    if (m_is_initialized)
        enable_reload_button(false);
}

void
qseditoptions::state_changed ()
{
    set_enabled(QDialogButtonBox::Apply, true);
    set_enabled(QDialogButtonBox::Cancel, false);
    set_enabled(QDialogButtonBox::Reset, true);
    set_enabled(QDialogButtonBox::Ok, true);
    enable_reload_button(true);
}

void
qseditoptions::state_applied ()
{
    set_enabled(QDialogButtonBox::Apply, false);
    set_enabled(QDialogButtonBox::Cancel, true);
    set_enabled(QDialogButtonBox::Reset, false);
    set_enabled(QDialogButtonBox::Ok, false);
    enable_reload_button(true);
}

void
qseditoptions::slot_io_maps ()
{
    bool ok = perf().store_io_maps();               /* sets 'rc' auto-save  */
    if (ok)
    {
        bool active = perf().port_maps_active();
        ui->ioPortsMappedCheck->setChecked(active);
        rc().portmaps_active(active);
        modify_rc();
    }
    else
    {
        ui->ioPortsMappedCheck->setChecked(false);
        rc().portmaps_active(false);
    }
}

void
qseditoptions::slot_remove_io_maps ()
{
    perf().clear_io_maps();                         /* sets save, inactive  */
    ui->ioPortsMappedCheck->setChecked(false);
    modify_rc();
}

void
qseditoptions::slot_activate_io_maps ()
{
    bool active = ui->ioPortsMappedCheck->isChecked();
    perf().activate_io_maps(active);                /* sets 'rc' auto-save  */
    active = perf().port_maps_active();
    rc().portmaps_active(active);
    modify_rc();
}

void
qseditoptions::show_session (usrsettings::session sm)
{
    bool url_modifiable = false;
    std::string tenturl;

#if defined SEQ66_NSM_SUPPORT
    tenturl = nsm::get_url();
#else
    ui->radio_session_nsm->setChecked(false);
    ui->radio_session_nsm->setEnabled(false);
#endif

    if (tenturl.empty())
    {
#if defined SEQ66_NSM_SUPPORT
        if (usr().want_nsm_session())
        {
            url_modifiable = true;
            tenturl = usr().session_url();
        }
#endif
#if defined SEQ66_JACK_SESSION
        if (usr().want_jack_session())
        {
            tenturl = rc().jack_session();          /* JACK session UUID    */
        }
#endif
    }
    ui->label_nsm_url->setEnabled(false);
    if (usr().in_nsm_session())
    {
        ui->radio_session_none->setChecked(false);
        ui->radio_session_none->setEnabled(false);
        ui->radio_session_jack->setChecked(false);
        ui->radio_session_jack->setEnabled(false);
#if defined SEQ66_NSM_SUPPORT
        ui->radio_session_nsm->setChecked(true);
        ui->radio_session_nsm->setEnabled(true);
        ui->label_nsm_url->setEnabled(true);
        ui->label_nsm_url->setText("NSM URL");
        ui->lineEditNsmUrl->setText(qt(tenturl));
#endif
    }
    else
    {
        switch (sm)
        {
            case usrsettings::session::none:
                ui->radio_session_none->setChecked(true);
                ui->label_nsm_url->setText("UUID N/A");
                break;

            case usrsettings::session::nsm:
#if defined SEQ66_NSM_SUPPORT
                ui->radio_session_nsm->setChecked(true);
                ui->label_nsm_url->setEnabled(true);
                ui->label_nsm_url->setText("NSM URL");
                ui->lineEditNsmUrl->setText(qt(tenturl));
#endif
                break;

            case usrsettings::session::jack:
#if defined SEQ66_JACK_SESSION
                ui->radio_session_jack->setChecked(true);
                ui->label_nsm_url->setEnabled(true);
                ui->label_nsm_url->setText("JACK UUID");
                ui->lineEditNsmUrl->setText(qt(tenturl));
#endif
                break;

            default:
                break;
        }
    }
    ui->lineEditNsmUrl->setEnabled(url_modifiable);
}

void
qseditoptions::slot_session (int buttonno)
{
    usrsettings::session current = usr().session_manager();
    if (buttonno == static_cast<int>(usrsettings::session::nsm))
    {
        usr().clear_option_bits();          /* see usrsettings::option_bits */
        usr().session_manager("nsm");
        ui->label_nsm_url->setEnabled(true);
        ui->label_nsm_url->setText("NSM URL");
        ui->lineEditNsmUrl->setEnabled(true);
    }
    else if (buttonno == static_cast<int>(usrsettings::session::jack))
    {
        usr().clear_option_bits();          /* see usrsettings::option_bits */
        usr().session_manager("jack");
        ui->label_nsm_url->setEnabled(true);
        ui->label_nsm_url->setText("JACK UUID");
    }
    else
    {
        usr().clear_option_bits();          /* see usrsettings::option_bits */
        usr().session_manager("none");
        ui->label_nsm_url->setEnabled(false);
        ui->label_nsm_url->setText("N/A");
        ui->lineEditNsmUrl->setEnabled(false);
    }
    if (usr().session_manager() != current)
        modify_usr();
}

void
qseditoptions::slot_ui_scaling ()
{
    QString qs = ui->lineEditUiScaling->text();                 /* w */
    QString qheight = ui->lineEditUiScalingHeight->text();      /* h */
    ui_scaling_helper(qs, qheight);
    modify_usr();
}

void
qseditoptions::slot_grid_spacing ()
{
    QString qs = ui->lineEditGridSpacing->text();
    std::string spacingtext = qs.toStdString();
    int spacing = string_to_int(spacingtext, 2);
    if (spacing != usr().mainwnd_spacing())
    {
        usr().mainwnd_spacing(spacing);
        modify_usr();
    }
}

/**
 *  Just closes.  Any modifications are already in the rc() and usr() objects.
 *  Also sets the reload buttons, but only the one in the main-window Session
 *  tab will be available.
 */

void
qseditoptions::okay ()
{
    enable_reload_button(true);
    close();
}

/**
 *  Restores the settings from the "backup" variables when the user cancels
 *  the edit.
 *
 *  Not necessary here: sync();
 */

void
qseditoptions::cancel ()
{
    reset();
    state_unchanged();
    close();
}

/**
 *  The Apply button now shows "Restart Seq66!" and serves to restart the whole
 *  application when clicked.
 *
 *  Aren't the backup calls useless at this point???
 */

void
qseditoptions::apply ()
{
    /*
     * backup() useless at this point, 'cause we're outa here!
     */

    state_unchanged();
    signal_for_restart();           /* warnprint("Session reload request"); */
}

void
qseditoptions::reset ()
{
    rc() = m_backup_rc;
    usr() = m_backup_usr;
    reload_needed(false);
}

void
qseditoptions::set_enabled (QDialogButtonBox::StandardButton bcode, bool on)
{
    QPushButton * button = ui->buttonBoxOptionsDialog->button(bcode);
    button->setEnabled(on);
}

void
qseditoptions::set_text
(
    QDialogButtonBox::StandardButton bcode,
    const std::string & text
)
{
    QPushButton * button = ui->buttonBoxOptionsDialog->button(bcode);
    button->setText(qt(text));
}

void
qseditoptions::show_button (QDialogButtonBox::StandardButton bcode, bool show)
{
    QPushButton * button = ui->buttonBoxOptionsDialog->button(bcode);
    if (show)
        button->show();
    else
        button->hide();
}

/**
 *  Backs up the JACK, Time, Key-height, and Note-Resume settings in case the
 *  user cancels. In that case, the cancel() function will put these settings
 *  back into the various settings objects.
 */

void
qseditoptions::backup ()
{
    m_backup_rc = rc();
    m_backup_usr = usr();
}

/**
 *  Sync with preferences.  In other words, the current values in the various
 *  settings objects are used to set the user-interface elements in this
 *  dialog.
 */

void
qseditoptions::sync ()
{
    sync_rc();
    sync_usr();

    /*
     * Other items to sync.
     */

    ui->checkBoxSaveMutes->setChecked(rc().auto_mutes_save());
    ui->checkBoxActiveMutes->setChecked(rc().mute_group_file_active());
}

/*
 *  Get all the settings from the 'rc' file and make the GUI controls match them.
 *
 *  Note that "foreach" is a Qt-specific keyword, not a C++ keyword.
 */

void
qseditoptions::sync_rc ()
{
    ui->chkJackTransport->setChecked(rc().with_jack_transport());
    ui->chkJackMaster->setChecked(rc().with_jack_master());
    ui->chkJackConditional->setChecked(rc().with_jack_master_cond());
    ui->chkJackNative->setChecked(rc().with_jack_midi());
#if defined SEQ66_JACK_SUPPORT
    ui->chkJackAutoConnect->setChecked(rc().jack_auto_connect());
#else
    rc().jack_auto_connect(false);
    ui->chkJackAutoConnect->setChecked(false);
#endif
    ui->chkJackMaster->setDisabled(! rc().with_jack_transport());
    ui->chkJackConditional->setDisabled(! rc().with_jack_transport());
    show_sets_mode(rc().sets_mode());
    show_start_mode(rc().get_song_start_mode());

#if defined SEQ66_JACK_SUPPORT
    int rbid = perf().song_mode() ? playmode_button_song : playmode_button_live ;
    foreach (QAbstractButton * button, m_live_song_buttons->buttons())
    {
        int bid = m_live_song_buttons->id(button);
        button->setChecked(bid == rbid);
    }
#endif

    /*
     * Sync all the settings for "UI Boolean Options.
     */

    ui->checkBoxVerbose->setChecked(rc().verbose());
    ui->checkBoxLoadMostRecent->setChecked(rc().load_most_recent());
    ui->checkBoxShowFullRecentPaths->setChecked(rc().full_recent_paths());
    ui->checkBoxLongBussNames->setChecked(rc().port_naming() == portname::full);
    ui->checkBoxPairBussNames->setChecked(rc().port_naming() == portname::pair);
    ui->checkBoxLockMainWindow->setChecked(usr().lock_main_window());
    ui->checkBoxSwapCoordinates->setChecked(usr().swap_coordinates());
    ui->checkBoxBoldGridSlots->setChecked(usr().progress_bar_thick());
    ui->checkBoxDoubleClickEdit->setChecked(rc().allow_click_edit());

    std::string filespec = rc().config_filespec();
    ui->checkBoxSaveRc->setChecked(rc().auto_rc_save());
    ui->checkBoxActiveRc->setChecked(true);         /* not always active    */
    tooltip_for_filename(ui->lineEditRc, filespec);

    filespec = rc().user_filespec();
    ui->checkBoxSaveUsr->setChecked(rc().auto_usr_save());
    ui->checkBoxActiveUsr->setChecked(rc().user_file_active());
    tooltip_for_filename(ui->lineEditUsr, filespec);

    filespec = rc().mute_group_filespec();
    ui->checkBoxSaveMutes->setChecked(rc().auto_mutes_save());
    ui->checkBoxActiveMutes->setChecked(rc().mute_group_file_active());
    tooltip_for_filename(ui->lineEditMutes, filespec);

    filespec = rc().playlist_filespec();
    ui->checkBoxSavePlaylist->setChecked(rc().auto_playlist_save());
    ui->checkBoxActivePlaylist->setChecked(rc().playlist_active());
    tooltip_for_filename(ui->lineEditPlaylist, filespec);

    filespec = rc().midi_control_filespec();
    ui->checkBoxSaveCtrl->setChecked(rc().auto_ctrl_save());    /* read-only */
    ui->checkBoxActiveCtrl->setChecked(rc().midi_control_active());
    tooltip_for_filename(ui->lineEditCtrl, filespec);

    filespec = rc().notemap_filespec();

#if defined SEQ66_CAN_SAVE_DRUMS

    /*
     * No way to edit drums, so no need to save them.
     */

    ui->checkBoxSaveDrums->setChecked(rc().auto_drums_save());  /* read-only */

#endif

    ui->checkBoxActiveDrums->setChecked(rc().notemap_active());
    tooltip_for_filename(ui->lineEditDrums, filespec);

#if defined SEQ66_CAN_SAVE_PALETTE

    /*
     *  No way to edit palette in the app, through the current palette can
     *  be written to the file via the Store Palette button.
     */

    ui->checkBoxSavePalette->setChecked(rc().auto_palette_save());

#endif

    filespec = rc().palette_filespec();
    ui->checkBoxActivePalette->setChecked(rc().palette_active());
    tooltip_for_filename(ui->lineEditPalette, filespec);

    /*
     * We will never save the style sheet from within Seq66.  We leave the
     * checkbox visible, unchecked, and disabled.
     *
     * ui->checkBoxSaveStyleSheet->hide();
     */

    filespec = rc().style_sheet_filespec();
    ui->checkBoxSaveStyleSheet->setChecked(false);
    ui->checkBoxActiveStyleSheet->setChecked(rc().style_sheet_active());
    tooltip_for_filename(ui->lineEditStyleSheet, filespec);

    bool outportmap = output_port_map().active();
    bool inportmap = input_port_map().active();
    ui->ioPortsMappedCheck->setChecked(outportmap && inportmap);
    ui->checkBoxVirtualPorts->setChecked(rc().manual_ports());

    std::string value = std::to_string(rc().manual_port_count());
    ui->lineEditOutputCount->setText(qt(value));
    value = std::to_string(rc().manual_in_port_count());
    ui->lineEditInputCount->setText(qt(value));

    bool bybuss = rc().record_by_buss();
    ui->checkBoxRecordByBuss->setChecked(bybuss);
    if (bybuss)
        ui->checkBoxRecordByChannel->setChecked(false);
    else
        ui->checkBoxRecordByChannel->setChecked(rc().record_by_channel());
}

void
qseditoptions::sync_usr ()
{
    char tmp[32];
    snprintf(tmp, sizeof tmp, "%.2f", usr().window_scale());
    ui->lineEditUiScaling->setText(tmp);
    snprintf(tmp, sizeof tmp, "%.2f", usr().window_scale_y());

    ui->lineEditUiScalingHeight->setText(tmp);
    ui->chkNoteResume->setChecked(usr().resume_note_ons());
    ui->chkUseFilesPPQN->setChecked(usr().use_file_ppqn());
    ui->chkSongRecordSnap->setChecked(perf().song_record_snap());
    ui->spinKeyHeight->setValue(usr().key_height());

    /*
     * New-pattern items
     */

    ui->checkBoxEscapePattern->setChecked(usr().escape_pattern());
    ui->checkBoxNewPatternArm->setChecked(usr().new_pattern_armed());
    ui->checkBoxNewPatternTighten->setChecked(usr().new_pattern_tighten());
    ui->checkBoxNewPatternQRecord->setChecked(usr().new_pattern_qrecord());
    ui->checkBoxNewPatternNoteMap->setChecked(usr().new_pattern_notemap());
    ui->checkBoxNewPatternRecord->setChecked(usr().new_pattern_record());
    ui->checkBoxNewPatternThru->setChecked(usr().new_pattern_thru());
    ui->checkBoxNewPatternWrapAround->setChecked
    (
        usr().new_pattern_wraparound()
    );

    int r = usr().new_pattern_record_code();
    ui->comboBoxRecordStyle->setCurrentIndex(r);
    show_session(usr().session_manager());
    set_scaling_fields();
    set_set_size_fields();
    set_progress_box_fields();

    snprintf(tmp, sizeof tmp, "%i", usr().fingerprint_size());
    ui->lineEditFingerprintSize->setText(tmp);
    ui->checkBoxGlobalSeqFeature->setChecked(usr().global_seq_feature());
}

/**
 *  Instead of this sequence of calls, we could send a Qt signal from
 *  qclocklayout to eventually call the qsmainwnd slot.  We need to make this
 *  item cause immediate action.
 *
 *  We also need to reconstruct the Control/Display combo-boxes. The
 *  code is a bit much, but seems to work well enough. The alternative
 *  is to keep even disabled devices enabled in the combo-box.
 */

void
qseditoptions::enable_bus_item (int bus, bool enabled)
{
    m_parent_widget->enable_bus_item(bus, enabled);

    mastermidibus * mmb = perf().master_bus();
    const clockslist & opm = output_port_map();
    bool outportmap = opm.active();
    QComboBox * out = ui->comboBoxMidiOutBuss;
    int buses = outportmap ? opm.count() : mmb->get_num_out_buses() ;
    setup_clock_combo_box(buses, out);

    const inputslist & ipm = input_port_map();
    bool inportmap = ipm.active();
    QComboBox * in = ui->comboBoxMidiInBuss;
    buses = inportmap ? ipm.count() : mmb->get_num_in_buses() ;
    setup_input_combo_box(buses, in);
    reload_needed(true);                                /* immediate action */
}

void
qseditoptions::enable_reload_button (bool flag)
{
    m_parent_widget->enable_reload_button(flag);
    ui->pushButtonReload->setEnabled(flag);
}

/**
 *  Updates the performer::result_note_ons() setting in accord with the
 *  user-interface, and then calls sync(), perhaps needlessly, to
 *  make sure the user-interface items correctly represent the settings.
 *  Resolves issue #5.
 */

void
qseditoptions::slot_note_resume ()
{
    if (m_is_initialized)
    {
        bool resumenotes = ui->chkNoteResume->isChecked();
        if (perf().resume_note_ons() != resumenotes)
        {
            usr().resume_note_ons(resumenotes);
            perf().resume_note_ons(resumenotes);
            modify_usr();
        }
    }
}

void
qseditoptions::slot_ppqn_by_text (const QString & text)
{
    std::string temp = text.toStdString();
    if (! temp.empty())
    {
        int p = string_to_int(temp);
        if (perf().change_ppqn(p))
        {
            ppqn_list().current(temp);
            m_parent_widget->set_ppqn_text(temp);
            ui->combo_box_ppqn->setItemText(0, text);
            usr().clear_option_bits();      /* see usrsettings::option_bits */
            usr().default_ppqn(p);
            modify_usr();
        }
    }
}

void
qseditoptions::slot_use_file_ppqn ()
{
    if (m_is_initialized)
    {
        bool ufppqn = ui->chkUseFilesPPQN->isChecked();
        bool status = usr().use_file_ppqn();
        if (ufppqn != status)
        {
            usr().clear_option_bits();      /* see usrsettings::option_bits */
            usr().use_file_ppqn(ufppqn);
            modify_usr();
        }
    }
}

void
qseditoptions::slot_buffer_size_by_text (const QString & text)
{
    std::string temp = text.toStdString();
    if (! temp.empty())
    {
        int bsize = string_to_int(temp);
        int oldsize = rc().jack_buffer_size();
        if (bsize != oldsize)
        {
            buffer_size_list().current(temp);
            rc().jack_buffer_size(bsize);
            if (rc().jack_buffer_size() == bsize)
                modify_rc();
        }
    }
}

/**
 *  Fixed issue #44 "Record live sequence changes" functionality, which
 *  was already in place, but had no runtime user-interface setting.
 */

void
qseditoptions::slot_song_record_snap ()
{
    if (m_is_initialized)
    {
        bool snappit = ui->chkSongRecordSnap->isChecked();
        perf().song_record_snap(snappit);
        set_enabled(QDialogButtonBox::Ok, true);    /* for appearances only */
    }
}

/**
 *  Updates the usrsettings::key_height() setting in accord with the
 *  user-interface, and then calls sync(), perhaps needlessly, to
 *  make sure the user-interface items correctly represent the settings.
 *  Also turns on the user-save setting, so that the settings will be written to
 *  the "usr" file upon exit.
 */

void
qseditoptions::slot_key_height ()
{
    usr().key_height(ui->spinKeyHeight->value());
    if (m_is_initialized)
    {
        modify_usr();
        sync();
    }
}

void
qseditoptions::set_scaling_fields ()
{
    char tmp[32];
    snprintf(tmp, sizeof tmp, "%.2f", usr().window_scale());
    ui->lineEditUiScaling->setText(tmp);
    snprintf(tmp, sizeof tmp, "%.2f", usr().window_scale_y());
    ui->lineEditUiScalingHeight->setText(tmp);
}

/*
 * The next function is weird.  That's because it uses the same parsing
 * method as the command-line option.
 */

void
qseditoptions::ui_scaling_helper
(
    const QString & widthtext,
    const QString & heighttext
)
{
    std::string wtext = widthtext.toStdString();
    std::string htext = heighttext.toStdString();
    if (! wtext.empty() && ! htext.empty())
    {
        std::string tuple = wtext + "x" + htext;
        usr().clear_option_bit(usrsettings::option_bits::option_scale);
        usr().clear_option_bits();          /* see usrsettings::option_bits */
        if (usr().parse_window_scale(tuple))
            modify_usr();
    }
}

void
qseditoptions::slot_nsm_url ()
{
    QString url = ui->lineEditNsmUrl->text();
    usr().session_url(url.toStdString());
    modify_usr();
}

void
qseditoptions::set_set_size_fields ()
{
    char tmp[32];
    snprintf(tmp, sizeof tmp, "%i", usr().mainwnd_rows());
    ui->lineEditSetSize->setText(tmp);
    snprintf(tmp, sizeof tmp, "%i", usr().mainwnd_cols());
    ui->lineEditSetSizeColumns->setText(tmp);
}

void
qseditoptions::slot_set_size_rows ()
{
    const QString qs = ui->lineEditSetSize->text();
    const std::string valuetext = qs.toStdString();
    if (! valuetext.empty())
    {
        int rows = string_to_int(valuetext);
        usr().clear_option_bits();          /* see usrsettings::option_bits */
        if (usr().mainwnd_rows(rows))
            modify_usr();
        else
            set_set_size_fields();
    }
}

void
qseditoptions::slot_set_size_columns ()
{
    const QString qs = ui->lineEditSetSizeColumns->text();
    const std::string valuetext = qs.toStdString();
    if (! valuetext.empty())
    {
        int columns = string_to_int(valuetext);
        usr().clear_option_bits();          /* see usrsettings::option_bits */
        if (usr().mainwnd_cols(columns))
            modify_usr();
        else
            set_set_size_fields();
    }
}

void
qseditoptions::set_progress_box_fields ()
{
    char tmp[32];
    snprintf(tmp, sizeof tmp, "%.2f", usr().progress_box_width());
    ui->lineEditProgressBox->setText(tmp);
    snprintf(tmp, sizeof tmp, "%.2f", usr().progress_box_height());
    ui->lineEditProgressBoxHeight->setText(tmp);
    ui->checkBoxProgressBoxShown->setChecked(usr().progress_box_shown());
}

void
qseditoptions::slot_progress_box_width ()
{
    const QString qs = ui->lineEditProgressBox->text();
    const std::string wtext = qs.toStdString();
    if (! wtext.empty())
    {
        double w = string_to_double(wtext, 0.0, 2);
        double h = usr().progress_box_height();
        if (usr().progress_box_size(w, h))
            modify_usr();
        else
            set_progress_box_fields();
    }
}

void
qseditoptions::slot_progress_box_height ()
{
    const QString qs = ui->lineEditProgressBoxHeight->text();
    const std::string htext = qs.toStdString();
    if (! htext.empty())
    {
        double w = usr().progress_box_width();
        double h = string_to_double(htext, 0.0, 2);
        if (usr().progress_box_size(w, h))
            modify_usr();
        else
            set_progress_box_fields();
    }
}

void
qseditoptions::slot_progress_box_shown ()
{
    bool on = ui->checkBoxProgressBoxShown->isChecked();
    usr().progress_box_shown(on);
    modify_usr();
}

void
qseditoptions::slot_fingerprint_size ()
{
    const QString qs = ui->lineEditFingerprintSize->text();
    std::string text = qs.toStdString();
    if (! text.empty())
    {
        double sz = string_to_int(text);
        if (usr().fingerprint_size(sz))
        {
            modify_usr();
        }
        else
        {
            char tmp[32];
            snprintf(tmp, sizeof tmp, "%i", usr().fingerprint_size());
            ui->lineEditFingerprintSize->setText(tmp);
        }
    }
}

/**
 *  Given a file-name, possibly with a path, get the base name and load it
 *  into the QLineEdit, and set the file-tooltip.
 *
 *  Currently useful for all files specified in the 'rc' file, but not
 *  for the style-sheet, which can have a full path and is also specified
 *  in the 'usr' file.
 *
 * \param lineedit
 *      The QLineEdit that must be updated with the new base-name and the
 *      full filespec tooltip.
 *
 * \param fileextension
 *      The desired file-extension for filtering in the file dialog.
 *      If empty, all files will be shown in the dialog.
 *
 * \return
 *      Returns true if the path/file selection succeeded.
 */

bool
qseditoptions::load_file_name
(
    QLineEdit * lineedit,
    const std::string & fileextension
)
{
    std::string selecteddir;
    std::string selectedfile;
    bool result = show_file_select_dialog   /* a simplified lookup dialog   */
    (
        this, fileextension, selecteddir, selectedfile
    );
    if (result)
    {
        tooltip_for_filename(lineedit, selecteddir);
        lineedit->setText(qt(selectedfile));
    }
    return result;
}

/**
 *  Opens a dialog to look for executables, starting with the filename
 *  provided.
 */

bool
qseditoptions::load_executable_name
(
    QLineEdit * lineedit,
    const std::string & fname
)
{
    std::string ncfname = fname;
    bool result = show_exe_file_dialog(lineedit, ncfname);
    if (result)
    {
        tooltip_for_filename(lineedit, ncfname);
        lineedit->setText(qt(ncfname));
    }
    return result;
}

/**
 *  The user can edit the filename instead of pressing the "..." button
 *  to get the name from the dialog. If a path is provided, it is stripped
 *  and the "home" configuration directory is used.
 */

void
qseditoptions::slot_palette_filename ()
{
    const QString qs = ui->lineEditPalette->text();
    std::string text = qs.toStdString();
    if (text != rc().palette_filename())
    {
        if (text.empty())
        {
            rc().palette_filename(text);
            ui->checkBoxActivePalette->setChecked(false);
            ui->lineEditPalette->setToolTip("No file");
        }
        else
        {
            std::string pathname = text;
            if (name_has_path(text))
                text = filename_base(text);

            rc().palette_filename(text);
            tooltip_for_filename(ui->lineEditPalette, rc().palette_filespec());
        }
        exit_required();
        modify_rc();

        /*
         * No saving the palette except via the Store Palette button.
         *
         * rc().auto_palette_save(true);
         * ui->checkBoxSavePalette->setChecked(true);
         */
    }
}

/**
 *  Note that load_file_name() sets the tooltip to the full path and
 *  the text to the base-name of the file selected.
 */

void
qseditoptions::slot_load_palette_filename ()
{
    if (load_file_name(ui->lineEditPalette, "palette"))
    {
        const QString qs = ui->lineEditPalette->text();
        std::string text = qs.toStdString();
        rc().palette_filename(text);
        if (! text.empty())
        {
            ui->checkBoxActivePalette->setChecked(true);
            rc().palette_active(true);
        }
        modify_rc();
    }
}

#if defined SEQ66_CAN_SAVE_PALETTE

void
qseditoptions::slot_palette_save_click ()
{
    if (is_empty(ui->lineEditPalette))
    {
        ui->checkBoxSavePalette->setChecked(false);
        rc().auto_palette_save(false);
    }
    else
    {
        bool on = ui->checkBoxSavePalette->isChecked();
        rc().auto_palette_save(on);
    }
    modify_rc();
}

#endif

/**
 *  Shows an alternative way to handle an empty file-name.
 */

void
qseditoptions::slot_palette_active_click ()
{
    bool on = ui->checkBoxActivePalette->isChecked();
    if (rc().palette_filename().empty())
    {
        on = false;
        ui->checkBoxActivePalette->setChecked(false);
    }
    rc().palette_active(on);
    if (! on)
        exit_required();

    modify_rc();
}

void
qseditoptions::slot_palette_save_now_click ()
{
    QString qs = ui->lineEditPalette->text();
    std::string palfile = qs.toStdString();
    if (! palfile.empty())
    {
        palfile = filename_base(palfile);
        rc().palette_filename(palfile);
    }
    palfile = rc().palette_filespec();
    if (! palfile.empty())
    {
        if (save_palette(global_palette(), palfile))
        {
            /*
             * TMI: file_message("Saved", palfile);
             *      exit_required();
             */

        }
        else
            file_error("Save failed", palfile);
    }
}

#if defined USE_VERBOSE_CHECKBOX

void
qseditoptions::slot_verbose_active_click ()
{
    bool on = ui->checkBoxVerbose->isChecked();
    rc().verbose(on);
    modify_rc();
}

#endif

void
qseditoptions::slot_quiet_active_click ()
{
    bool on = ui->checkBoxQuiet->isChecked();
    rc().quiet(on);
    modify_rc();
}

void
qseditoptions::slot_load_most_recent_click ()
{
    bool on = ui->checkBoxLoadMostRecent->isChecked();
    rc().load_most_recent(on);
    modify_rc();
}

void
qseditoptions::slot_show_full_paths_click ()
{
    bool on = ui->checkBoxShowFullRecentPaths->isChecked();
    rc().full_recent_paths(on);
    modify_rc();
}

void
qseditoptions::slot_long_buss_names_click ()
{
    bool on = ui->checkBoxLongBussNames->isChecked();
    rc().port_naming(on ? "long" : "short");
    modify_rc();
}

void
qseditoptions::slot_pair_buss_names_click ()
{
    bool on = ui->checkBoxPairBussNames->isChecked();
    rc().port_naming(on ? "pair" : "short");
    modify_rc();
}

void
qseditoptions::slot_lock_main_window_click ()
{
    bool on = ui->checkBoxLockMainWindow->isChecked();
    usr().lock_main_window(on);
    modify_usr();
    m_parent_widget->lock_main_window(on);
}

void
qseditoptions::slot_dark_theme_click ()
{
    bool on = ui->checkBoxDarkTheme->isChecked();
    usr().dark_theme(on);
    modify_usr();
}

void
qseditoptions::slot_swap_coordinates_click ()
{
    bool on = ui->checkBoxSwapCoordinates->isChecked();
    usr().swap_coordinates(on);
    modify_usr();
}

void
qseditoptions::slot_bold_grid_slots_click ()
{
    bool on = ui->checkBoxBoldGridSlots->isChecked();
    usr().progress_bar_thick(on);
    modify_usr();
}

void
qseditoptions::slot_elliptical_click ()
{
    bool on = ui->checkBoxElliptical->isChecked();
    usr().progress_box_elliptical(on);
    modify_usr();
}

void
qseditoptions::slot_follow_progress_click ()
{
    bool on = ui->checkBoxFollowProgress->isChecked();
    usr().follow_progress(on);
    modify_usr();
}

void
qseditoptions::slot_double_click_edit ()
{
    bool on = ui->checkBoxDoubleClickEdit->isChecked();
    rc().allow_click_edit(on);
    modify_rc();
}

void
qseditoptions::slot_global_seq_feature ()
{
    bool on = ui->checkBoxGlobalSeqFeature->isChecked();
    usr().global_seq_feature(on);
    modify_usr();
}

/**
 *  In the following functions, turning of the "auto" flag and the "modify"
 *  flag is somewhat redundant.
 *
 *  Also, we don't need to process the "active" check-box.  It is always
 *  checked and is read-only.  The name of the file can be changed, of course.
 */

void
qseditoptions::slot_rc_save_click ()
{
    bool on = ui->checkBoxSaveRc->isChecked();
    rc().auto_rc_save(on);
    reload_needed(true);
}

#if defined USE_RC_NAME_CHANGE

/**
 *  The previous file-name has either the "HOME" path, no path, or a different
 *  path specified the the 'rc' file, In the first two cases, the plain file
 *  base-name appears in the control. In the last one, the specified path
 *  appears. If there is no path, it can be obtained from the tooltip.
 *  If there is a path, it is stripped. Preserve the original path.
 */

void
qseditoptions::slot_rc_filename ()
{
    const QString qs = ui->lineEditRc->text();
    std::string text = qs.toStdString();
    if (text != rc().config_filename() && ! text.empty())
    {
        text = filename_base(text);
        rc().config_filename(text);
        modify_rc();
    }
}

void
qseditoptions::slot_load_rc_filename ()
{
    if (load_file_name(ui->lineEditRc, "rc"))
        modify_rc();
}

#endif

void
qseditoptions::modify_rc ()
{
    if (m_is_initialized)
    {
        ui->checkBoxSaveRc->setChecked(true);
        rc().auto_rc_save(true);
        rc().modify();
        reload_needed(true);
    }
}

void
qseditoptions::modify_ctrl ()
{
    if (m_is_initialized)
    {
        ui->checkBoxSaveCtrl->setChecked(true);
        rc().auto_ctrl_save(true);
        reload_needed(true);
    }
}

void
qseditoptions::modify_usr ()
{
    if (m_is_initialized)
    {
        ui->checkBoxSaveUsr->setChecked(true);
        rc().auto_usr_save(true);
        usr().modify();
        reload_needed(true);
    }
}

void
qseditoptions::slot_usr_save_click ()
{
    bool on = ui->checkBoxSaveUsr->isChecked();
    rc().auto_usr_save(on);
}

#if defined SEQ66_CAN_DEACTIVATE_USR

void
qseditoptions::slot_usr_active_click ()
{
    bool on = ui->checkBoxActiveUsr->isChecked();

    /*
     * usr().modify();
     */

    rc().user_file_active(on);
    rc().auto_rc_save(true);
}

#endif

/**
 *  There must always be a 'usr' file-name specified. If empty, that is
 *  ignored.
 */

void
qseditoptions::slot_usr_filename ()
{
    const QString qs = ui->lineEditUsr->text();
    std::string text = qs.toStdString();
    if (text != rc().user_filename())
    {
        if (text.empty())
        {
            text = rc().user_filename();
            ui->lineEditUsr->setText(qt(text)); /* restore previous value   */
        }
        else
        {
            if (name_has_path(text))
                text = filename_base(text);

            rc().user_filename(text);

            std::string pathname = rc().user_filespec();
            ui->checkBoxSaveUsr->setChecked(true);
            tooltip_for_filename(ui->lineEditUsr, pathname);
            modify_rc();
            modify_usr();                   /* guarantee saving this file   */
        }
    }
}

void
qseditoptions::slot_load_usr_filename ()
{
    if (load_file_name(ui->lineEditUsr, "usr"))
    {
        const QString qs = ui->lineEditPalette->text();
        std::string text = qs.toStdString();
        rc().user_filename(text);
        modify_rc();
    }
}

/**
 *  We could replace these is_empty(QLineEdit) checks with
 *  rc().xxxx_filename().empty() checks. Hmmmmmm.
 */

void
qseditoptions::slot_mutes_save_click ()
{
    if (is_empty(ui->lineEditMutes))
    {
        ui->checkBoxSaveMutes->setChecked(false);
        rc().auto_mutes_save(false);
    }
    else
    {
        bool on = ui->checkBoxSaveMutes->isChecked();
        rc().auto_mutes_save(on);
    }
    modify_rc();
}

void
qseditoptions::slot_mutes_active_click ()
{
    if (is_empty(ui->lineEditMutes))
    {
        ui->checkBoxActiveMutes->setChecked(false);
        rc().mute_group_file_active(false);
    }
    else
    {
        bool on = ui->checkBoxActiveMutes->isChecked();
        rc().mute_group_file_active(on);
    }
    modify_rc();
}

void
qseditoptions::slot_mutes_filename ()
{
    const QString qs = ui->lineEditMutes->text();
    std::string text = qs.toStdString();
    if (text != rc().mute_group_filename())
    {
        if (text.empty())
        {
            ui->checkBoxSaveMutes->setChecked(false);
            ui->checkBoxActiveMutes->setChecked(false);
            ui->lineEditMutes->setToolTip("No file");
            rc().mute_group_filename(text);
        }
        else
        {
            if (name_has_path(text))
                text = filename_base(text);

            rc().mute_group_filename(text);

            std::string pathname = rc().mute_group_filespec();
            ui->checkBoxSaveMutes->setChecked(true);
            tooltip_for_filename(ui->lineEditMutes, pathname);
            rc().auto_mutes_save(true);
        }
        modify_rc();
    }
}

void
qseditoptions::slot_load_mutes_filename ()
{
    if (load_file_name(ui->lineEditMutes, "mutes"))
    {
        const QString qs = ui->lineEditMutes->text();   /* ca 2024-10-14    */
        std::string text = qs.toStdString();
        rc().mute_group_filename(text);
        modify_rc();
    }
}

void
qseditoptions::slot_playlist_save_click ()
{
    if (is_empty(ui->lineEditPlaylist))
    {
        ui->checkBoxSavePlaylist->setChecked(false);
        rc().auto_playlist_save(false);
    }
    else
    {
        bool on = ui->checkBoxSavePlaylist->isChecked();
        rc().auto_playlist_save(on);
    }
    modify_rc();
}

void
qseditoptions::slot_playlist_active_click ()
{
    if (is_empty(ui->lineEditPlaylist))
    {
        ui->checkBoxActivePlaylist->setChecked(false);
        (void) perf().playlist_activate(false);
        rc().playlist_active(false);
    }
    else
    {
        bool on = ui->checkBoxActivePlaylist->isChecked();
        if (perf().playlist_activate(on))
            rc().playlist_active(on);
    }
    modify_rc();
}

void
qseditoptions::slot_playlist_filename ()
{
    const QString qs = ui->lineEditPlaylist->text();
    std::string text = qs.toStdString();
    if (text != rc().playlist_filename())
    {
        if (text.empty())
        {
            ui->checkBoxActivePlaylist->setChecked(false);
            ui->lineEditPlaylist->setToolTip("No file");
            perf().playlist_filename(text);
        }
        else
        {
            if (name_has_path(text))
                text = filename_base(text);

            perf().playlist_filename(text);     /* rc().playlist_filename(text) */

            std::string pathname = rc().playlist_filespec();
            ui->checkBoxSavePlaylist->setChecked(true);
            tooltip_for_filename(ui->lineEditPlaylist, pathname);
            rc().auto_playlist_save(true);
        }
        modify_rc();
    }
}

void
qseditoptions::slot_load_playlist_filename ()
{
    if (load_file_name(ui->lineEditPlaylist, "playlist"))
        modify_rc();
}

#if defined SEQ66_CAN_SAVE_CTRL

void
qseditoptions::slot_ctrl_save_click ()
{
    if (is_empty(ui->lineEditCtrl))
    {
        ui->checkBoxSaveCtrl->setChecked(false);
        rc().auto_ctrl_save(false);
    }
    else
    {
        bool on = ui->checkBoxSaveCtrl->isChecked();
        rc().auto_ctrl_save(on);
    }
    modify_rc();
}

#endif

void
qseditoptions::slot_ctrl_active_click ()
{
    if (is_empty(ui->lineEditCtrl))
    {
        ui->checkBoxActiveCtrl->setChecked(false);
        rc().midi_control_active(false);
    }
    else
    {
        bool on = ui->checkBoxActiveCtrl->isChecked();
        rc().midi_control_active(on);
    }
    modify_rc();
}

void
qseditoptions::slot_ctrl_filename ()
{
    const QString qs = ui->lineEditCtrl->text();
    std::string text = qs.toStdString();
    if (text != rc().midi_control_filename())
    {
        if (text.empty())
        {
            ui->checkBoxActiveCtrl->setChecked(false);
            ui->lineEditCtrl->setToolTip("No file");
            rc().midi_control_filename(text);
        }
        else
        {
            if (name_has_path(text))
                text = filename_base(text);

            rc().midi_control_filename(text);

            std::string pathname = rc().midi_control_filespec();
            ui->checkBoxSaveCtrl->setChecked(true);
            tooltip_for_filename(ui->lineEditCtrl, pathname);
            rc().auto_ctrl_save(true);
            modify_ctrl();
        }
        modify_rc();
    }
}

void
qseditoptions::slot_load_ctrl_filename ()
{
    if (load_file_name(ui->lineEditCtrl, "ctrl"))
    {
        const QString qs = ui->lineEditCtrl->text();
        std::string text = qs.toStdString();
        rc().midi_control_filename(text);
        modify_rc();
    }
}

#if defined SEQ66_CAN_SAVE_DRUMS

void
qseditoptions::slot_drums_save_click ()
{
    bool on = ui->checkBoxSaveDrums->isChecked();
    rc().auto_drums_save(on);
    modify_rc();
}

#endif

void
qseditoptions::slot_drums_active_click ()
{
    if (is_empty(ui->lineEditDrums))
    {
        ui->checkBoxActiveDrums->setChecked(false);
        rc().notemap_active(false);
    }
    else
    {
        bool on = ui->checkBoxActiveDrums->isChecked();
        rc().notemap_active(on);
    }
    modify_rc();
}

void
qseditoptions::slot_drums_filename ()
{
    const QString qs = ui->lineEditDrums->text();
    std::string text = qs.toStdString();
    if (text != rc().notemap_filename())
    {
        if (text.empty())
        {
            ui->checkBoxActiveDrums->setChecked(false);
            ui->lineEditDrums->setToolTip("No file");
            rc().notemap_filename(text);
        }
        else
        {
            if (name_has_path(text))
                text = filename_base(text);

            rc().notemap_filename(text);

            std::string pathname = rc().notemap_filespec();
            tooltip_for_filename(ui->lineEditDrums, pathname);
            ui->checkBoxSaveDrums->setChecked(true);
        }
        modify_rc();
    }
}

void
qseditoptions::slot_load_drums_filename ()
{
    if (load_file_name(ui->lineEditDrums, "drums"))
    {
        QString qtdrum = ui->lineEditDrums->text();
        std::string drum = qtdrum.toStdString();
        rc().notemap_filename(drum);
        if (! drum.empty())
        {
            ui->checkBoxActiveDrums->setChecked(true);
            rc().notemap_active(true);
        }
        modify_rc();
    }
}

void
qseditoptions::slot_stylesheet_active_click ()
{
    if (is_empty(ui->lineEditStyleSheet))
    {
        ui->checkBoxActiveStyleSheet->setChecked(false);
        rc().style_sheet_active(false);
        exit_required();
    }
    else
    {
        bool on = ui->checkBoxActiveStyleSheet->isChecked();
        rc().style_sheet_active(on);
        if (! on)
            exit_required();
    }
    modify_rc();
}

/**
 *  Here, we allow style-sheet files from anywhere in the directory tree.
 *  We should check for the existence of the file.
 */

void
qseditoptions::slot_stylesheet_filename ()
{
    const QString qs = ui->lineEditStyleSheet->text();
    std::string text = qs.toStdString();
    if (text != rc().style_sheet_filename())
    {
        rc().style_sheet_filename(text);
        if (text.empty())
        {
            ui->checkBoxActiveStyleSheet->setChecked(false);
            ui->lineEditStyleSheet->setToolTip("No file");
            rc().style_sheet_active(false);
        }
        else
        {
            ui->checkBoxActiveStyleSheet->setChecked(true);
            tooltip_for_filename(ui->lineEditStyleSheet, text);
        }
        exit_required();
        modify_rc();                        /* 'rc' now sets style-sheet    */
    }
}

void
qseditoptions::slot_load_stylesheet_filename ()
{
    if (load_file_name(ui->lineEditStyleSheet, "qss"))
    {
        QString qtqss = ui->lineEditStyleSheet->text();
        std::string qss = qtqss.toStdString();
        rc().style_sheet_filename(qss);
        if (! qss.empty())
        {
            ui->checkBoxActiveStyleSheet->setChecked(true);
            rc().style_sheet_active(true);
        }

        /*
         * exit_required();                 // why not required?
         */

        modify_rc();
    }
}

void
qseditoptions::slot_browser_executable ()
{
    QString qviewer = ui->lineEditBrowser->text();
    std::string viewer = qviewer.toStdString();
    if (viewer != usr().user_browser())
    {
        /*
         * viewer = strip_quotes(viewer);
         */

        usr().user_browser(viewer);         /* we could check existence     */
        modify_usr();
    }
}

void
qseditoptions::slot_load_browser_executable ()
{
    std::string current = usr().user_browser();
    if (load_executable_name(ui->lineEditBrowser, current))
    {
        const QString qs = ui->lineEditBrowser->text();
        std::string viewer = qs.toStdString();
        if (viewer != usr().user_browser())
        {
            usr().user_browser(viewer);
            modify_usr();
        }
    }
}

void
qseditoptions::slot_pdf_executable ()
{
    QString qviewer = ui->lineEditPdfViewer->text();
    std::string viewer = qviewer.toStdString();
    if (viewer != usr().user_pdf_viewer())
    {
        /*
         * viewer = strip_quotes(viewer);
         */

        usr().user_pdf_viewer(viewer);      /* we could check existence     */
        modify_usr();
    }
}

void
qseditoptions::slot_load_pdf_viewer_executable ()
{
    if (load_executable_name(ui->lineEditPdfViewer, ""))
    {
        const QString qs = ui->lineEditPdfViewer->text();
        std::string viewer = qs.toStdString();
        if (viewer != usr().user_pdf_viewer())
        {
            usr().user_pdf_viewer(viewer);
            modify_usr();
        }
    }
}

void
qseditoptions::slot_clock_start_modulo (int ticks)
{
    rc().set_clock_mod(ticks);
    modify_rc();
}

void
qseditoptions::slot_output_bus (int index)
{
    int oldindex = perf().midi_control_out().configured_buss();
    if (index != oldindex)
    {
        bool enable = index >= 0;
        if (enable)
        {
            /*
             * Don't call is_enabled(), as this will disable the control
             * display at exit.
             *
             * perf().midi_control_out().is_enabled(enable);
             * perf().midi_control_out().nominal_buss(index);
             * perf().midi_control_out().configure_enabled(true);
             */

            ui->checkBoxSaveCtrl->setChecked(true);
            perf().midi_control_out().configured_buss(index);
            perf().ui_set_clock(index, e_clock::off);           /* enabled! */
            rc().midi_control_active(true);
            reload_needed(true);
        }
        modify_ctrl();
    }
}

void
qseditoptions::activate_ctrl_file ()
{
    bool inenable = ui->checkBoxMidiInBuss->isChecked();
    bool outenable = ui->checkBoxMidiOutBuss->isChecked();
    bool active = inenable || outenable;
    rc().midi_control_active(active);
    ui->checkBoxActiveCtrl->setChecked(rc().midi_control_active());
}

/**
 *  The issue here is that disabling the display MIDI means it won't
 *  be acted on at exit/restart. We have rc().midi_control_active(), but
 *  may need settings outside the midi_control_in/out classes for in and out
 *  activity in the 'ctrl' file.
 */

void
qseditoptions::slot_output_bus_enable ()
{
    bool enable = ui->checkBoxMidiOutBuss->isChecked();
    modify_ctrl();
    perf().midi_control_out().configure_enabled(enable);
    activate_ctrl_file();
}

void
qseditoptions::slot_input_bus (int index)
{
    int oldindex = perf().midi_control_in().configured_buss();
    if (index != oldindex)
    {
        bool enable = index >= 0;
        if (enable)
        {
            ui->checkBoxSaveCtrl->setChecked(true);
            perf().midi_control_in().configured_buss(index);
            perf().ui_set_input(index, true);   /* auto-enable the input    */
            rc().midi_control_active(true);
            reload_needed(true);
        }
    }
    modify_ctrl();
}

void
qseditoptions::slot_input_bus_enable ()
{
    bool enable = ui->checkBoxMidiInBuss->isChecked();
    modify_ctrl();
    perf().midi_control_in().configure_enabled(enable);
}

void
qseditoptions::slot_tempo_track ()
{
    QString text = ui->lineEditTempoTrack->text();
    std::string t = text.toStdString();
    if (t.empty())
        ui->pushButtonTempoTrack->setEnabled(false);
    else
        ui->pushButtonTempoTrack->setEnabled(true);
}

void
qseditoptions::slot_buss_override ()
{
    QString text = ui->lineEditBussOverride->text();
    std::string t = text.toStdString();
    if (! t.empty())
    {
        bussbyte buss_override = bussbyte(string_to_int(t));
        if (is_valid_buss(buss_override))
        {
            usr().midi_buss_override(buss_override);
            modify_usr();
        }
    }
}

void
qseditoptions::slot_bpm_precision (int index)
{
    usr().bpm_precision(index);
    modify_usr();
}

void
qseditoptions::slot_tempo_track_set ()
{
    QString text = ui->lineEditTempoTrack->text();
    std::string t = text.toStdString();
    if (! t.empty())
    {
        int ttrack = string_to_int(t);
        bool ok = ttrack >= 0 && ttrack < seq::maximum();
        if (ok)
        {
            rc().tempo_track_number(ttrack);
            ui->pushButtonTempoTrack->setEnabled(false);
            modify_rc();

            /*
             * new ca 2024-05-20
             * Whether the new tempo track exists, it will be logged
             * in the last (Seqspec) track of the song.
             */

            m_perf.modify();
            m_perf.notify_sequence_change(ttrack);
        }
    }
}

void
qseditoptions::slot_record_by_buss ()
{
    bool on = ui->checkBoxRecordByBuss->isChecked();
    rc().record_by_buss(on);
    perf().record_by_buss(on);
    if (on)
        ui->checkBoxRecordByChannel->setChecked(false);

    modify_rc();
}

void
qseditoptions::slot_record_by_channel ()
{
    bool on = ui->checkBoxRecordByChannel->isChecked();
    rc().record_by_channel(on);
    perf().record_by_channel(on);
    if (on)
        ui->checkBoxRecordByBuss->setChecked(false);

    modify_rc();
}

void
qseditoptions::slot_virtual_ports ()
{
    bool on = ui->checkBoxVirtualPorts->isChecked();
    ui->lineEditOutputCount->setEnabled(on);
    ui->lineEditInputCount->setEnabled(on);
    rc().manual_ports(on);
    if (! on)
    {
        rc().default_manual_port_counts();
        sync_rc();                              /* too much, but awright    */
    }
    modify_rc();
}

void
qseditoptions::slot_enable_virtual_ports ()
{
    bool on = ui->checkBoxAutoEnableVirtual->isChecked();
    if (on)
    {
        ui->checkBoxVirtualPorts->setChecked(true);
        ui->lineEditOutputCount->setEnabled(true);
        ui->lineEditInputCount->setEnabled(true);
        rc().manual_ports(true);
    }
    rc().manual_auto_enable(on);
    modify_rc();
}

void
qseditoptions::slot_virtual_out_count ()
{
    QString text = ui->lineEditOutputCount->text();
    int count = string_to_int(text.toStdString());
    rc().manual_port_count(count);
    modify_rc();

    std::string value = std::to_string(rc().manual_port_count());
    ui->lineEditOutputCount->setText(qt(value));
}

void
qseditoptions::slot_virtual_in_count ()
{
    QString text = ui->lineEditInputCount->text();
    int count = string_to_int(text.toStdString());
    rc().manual_in_port_count(count);
    modify_rc();

    std::string value = std::to_string(rc().manual_in_port_count());
    ui->lineEditInputCount->setText(qt(value));
}

}           // namespace seq66

/*
 * qseditoptions.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

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
 * \updates       2022-03-29
 * \license       GNU GPLv2 or above
 *
 *      This version is located in Edit / Preferences.
 */

#include <QButtonGroup>

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

static const int Tab_MIDI_Clock     =  0;
static const int Tab_MIDI_Input     =  1;
static const int Tab_Display        =  2;
static const int Tab_JACK           =  3;
static const int Tab_Play_Options   =  4;
static const int Tab_Session        =  5;

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
    m_ppqn_list             (default_ppqns()),  /* see the settings module  */
    m_is_initialized        (false),
    m_backup_rc             (),
    m_backup_usr            (),
    m_reload_needed         (false)
{
    ui->setupUi(this);
    m_live_song_buttons = new QButtonGroup(this);
    backup();

#if defined SEQ66_JACK_SUPPORT

    /*
     * Jack Sync tab.  JACK Transport Connect/Disconnect buttons.
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
     * Jack Sync tab.  JACK Transport/MIDI tab.
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

#else

    ui->btnJackConnect->setEnabled(false);
    ui->btnJackDisconnect->setEnabled(false);
    ui->chkJackTransport->setEnabled(false);
    ui->chkJackConditional->setEnabled(false);
    ui->chkJackMaster->setEnabled(false);
    ui->chkJackNative->setEnabled(false);
    ui->chkJackAutoConnect->setEnabled(false);

#endif  // defined SEQ66_JACK_SUPPORT

    /*
     * Play Options tab
     */

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
     *  Combo-box for changing the default PPQN.
     */

    (void) set_ppqn_combo();
    connect
    (
        ui->combo_box_ppqn, SIGNAL(currentTextChanged(const QString &)),
        this, SLOT(slot_ppqn_by_text(const QString &))
    );

    /*
     * JACK tab
     */

    /*
     * Create a button group to manage the mutual status of the JACK Live and
     * Song Mode buttons.
     */

    m_live_song_buttons->addButton(ui->radio_live_mode, playmode_button_live);
    m_live_song_buttons->addButton(ui->radio_song_mode, playmode_button_song);
    connect
    (
        m_live_song_buttons, SIGNAL(buttonClicked(int)),
        this, SLOT(slot_jack_mode(int))
    );

    /*
     * Display tab.
     */

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
        ui->lineEditFingerprintSize, SIGNAL(editingFinished()),
        this, SLOT(slot_fingerprint_size())
    );
    connect
    (
        ui->checkBoxVerbose, SIGNAL(clicked(bool)),
        this, SLOT(slot_verbose_active_click())
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
        ui->checkBoxLockMainWindow, SIGNAL(clicked(bool)),
        this, SLOT(slot_lock_main_window_click())
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
    connect
    (
        ui->checkBoxDoubleClickEdit, SIGNAL(clicked(bool)),
        this, SLOT(slot_double_click_edit())
    );

    /*
     * Session tab.
     */

    /*
     * 'rc' file.  This file is always active, so that check-box is read-only.
     */

    connect
    (
        ui->checkBoxSaveRc, SIGNAL(clicked(bool)),
        this, SLOT(slot_rc_save_click())
    );
    connect
    (
        ui->lineEditRc, SIGNAL(editingFinished()),
        this, SLOT(slot_rc_filename())
    );

    /*
     * 'usr' file. Making 'usr' inactive is experimental.
     */

    connect
    (
        ui->checkBoxSaveUsr, SIGNAL(clicked(bool)),
        this, SLOT(slot_usr_save_click())
    );
    connect
    (
        ui->checkBoxActiveUsr, SIGNAL(clicked(bool)),
        this, SLOT(slot_usr_active_click())
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
        ui->checkBoxSaveMutes, SIGNAL(clicked(bool)),
        this, SLOT(slot_mutes_save_click())
    );
    connect
    (
        ui->checkBoxActiveMutes, SIGNAL(clicked(bool)),
        this, SLOT(slot_mutes_active_click())
    );
    connect
    (
        ui->lineEditMutes, SIGNAL(editingFinished()),
        this, SLOT(slot_mutes_filename())
    );

    /*
     * 'playlist' file
     */

    connect
    (
        ui->checkBoxSavePlaylist, SIGNAL(clicked(bool)),
        this, SLOT(slot_playlist_save_click())
    );
    connect
    (
        ui->checkBoxActivePlaylist, SIGNAL(clicked(bool)),
        this, SLOT(slot_playlist_active_click())
    );
    connect
    (
        ui->lineEditPlaylist, SIGNAL(editingFinished()),
        this, SLOT(slot_playlist_filename())
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
    connect
    (
        ui->lineEditCtrl, SIGNAL(editingFinished()),
        this, SLOT(slot_ctrl_filename())
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
    connect
    (
        ui->lineEditDrums, SIGNAL(editingFinished()),
        this, SLOT(slot_drums_filename())
    );

    /*
     * 'palette' file.
     */

    connect
    (
        ui->checkBoxSavePalette, SIGNAL(clicked(bool)),
        this, SLOT(slot_palette_save_click())
    );
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

    /*
     * For testing only
     *
    connect
    (
        ui->text_edit_key, SIGNAL(textChanged(const QString &)),
        this, SLOT(slot_key_test(const QString &))
    );
     *
     */

    ui->text_edit_key->hide();
    ui->label_key->hide();

    /*
     * Reload/restart button.
     */

    ui->pushButtonReload->setEnabled(false);
    connect
    (
        ui->pushButtonReload, SIGNAL(clicked(bool)),
        this, SLOT(slot_flag_reload())
    );

    /*
     * OK/Cancel Buttons
     */

    connect
    (
        ui->buttonBoxOptionsDialog->button(QDialogButtonBox::Ok),
        SIGNAL(clicked(bool)), this, SLOT(okay())
    );
    connect
    (
        ui->buttonBoxOptionsDialog->button(QDialogButtonBox::Cancel),
        SIGNAL(clicked(bool)), this, SLOT(cancel())
    );

    /*
     * Set up the MIDI Clock tab.  We use the new qclocklayout class to hold
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

        out->addItem("Disabled");
        for (int bus = 0; bus < buses; ++bus)
        {
            e_clock ec;
            std::string busname;
            if (perf().ui_get_clock(bussbyte(bus), ec, busname))
            {
                bool enabled = ec != e_clock::disabled;
                out->addItem(qt(busname));
                enable_combobox_item(out, bus + 1, enabled);
            }
        }

        bool active = perf().midi_control_out().is_enabled();
        int buss = perf().midi_control_out().nominal_buss();
        int activebuss = active ? buss + 1 : 0 ;
        out->setCurrentIndex(activebuss);
        connect
        (
            out, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slot_output_bus(int))
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
    connect
    (
        ui->lineEditTempoTrack, SIGNAL(editingFinished()),
        this, SLOT(slot_tempo_track())
    );

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
    connect
    (
        ui->inPortsMappedCheck, SIGNAL(clicked(bool)),
        this, SLOT(slot_activate_input_map())
    );
    connect
    (
        ui->outPortsMappedCheck, SIGNAL(clicked(bool)),
        this, SLOT(slot_activate_output_map())
    );


    /*
     * The virtual port counts for input and output.
     */

    connect
    (
        ui->lineEditOutputCount, SIGNAL(editingFinished()),
        this, SLOT(slot_virtual_out_count())
    );
    connect
    (
        ui->lineEditInputCount, SIGNAL(editingFinished()),
        this, SLOT(slot_virtual_in_count())
    );

    /*
     * Set up the MIDI Input tab.  It is simpler, just a list of check-boxes
     * in the groupBoxInputs widget.  No need for a separate class.  However,
     * note that our qinputcheckbox class controls the activation, in the GUI,
     * of each input port, based on its buss status.
     */

    QWidget * central2 = new QWidget;
    QVBoxLayout * vboxinputs = new QVBoxLayout(central2);
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

        in->addItem("Disabled");
        for (int bus = 0; bus < buses; ++bus)
        {
            std::string busname;
            bool inputing;
            bool good = perf().ui_get_input(bus, inputing, busname);
            bool enabled = ! perf().is_input_system_port(bus);
            in->addItem(qt(busname));
            if (good && enabled)
                enabled = true;

            enable_combobox_item(in, bus + 1, enabled);
        }

        bool active = perf().midi_control_in().is_enabled();
        int buss = perf().midi_control_in().nominal_buss();
        int activebuss = active ? buss  + 1 : 0 ;
        in->setCurrentIndex(activebuss);
        connect
        (
            in, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slot_input_bus(int))
        );
    }

    /*
     * I/O Port Boolean options.
     */

    connect
    (
        ui->checkBoxRecordByChannel, SIGNAL(clicked(bool)),
        this, SLOT(slot_record_by_channel())
    );
    connect
    (
        ui->checkBoxVirtualPorts, SIGNAL(clicked(bool)),
        this, SLOT(slot_virtual_ports())
    );

    QSpacerItem * spacer2 = new QSpacerItem
    (
        40, 20, QSizePolicy::Expanding, QSizePolicy::Expanding
    );
    vboxinputs->addItem(spacer2);
    sync();

    std::string clid = perf().client_id_string();
    ui->lineEditClientId->setText(qt(clid));
    m_is_initialized = true;

#if defined SEQ66_PLATFORM_WINDOWS
    ui->tabWidget->setTabEnabled(Tab_JACK, false);
#endif
}

/**
 *  A stock Qt GUI destructor.
 */

qseditoptions::~qseditoptions ()
{
    delete ui;
}

bool
qseditoptions::set_ppqn_combo ()
{
    bool result = false;
    int count = m_ppqn_list.count();
    if (count > 0)
    {
        std::string p = std::to_string(usr().default_ppqn());
        QString combo_text = qt(p);
        ui->combo_box_ppqn->clear();
        ui->combo_box_ppqn->insertItem(0, combo_text);
        for (int i = 1; i < count; ++i)
        {
            p = m_ppqn_list.at(i);
            combo_text = qt(p);
            ui->combo_box_ppqn->insertItem(i, combo_text);
            if (string_to_int(p) == perf().ppqn())
                result = true;
        }
        ui->combo_box_ppqn->setCurrentIndex(0);
    }
    return result;
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

void
qseditoptions::slot_jack_mode (int buttonno)
{
    if (buttonno == playmode_button_live)
        perf().song_mode(false);
    else if (buttonno == playmode_button_song)
        perf().song_mode(true);
}

void
qseditoptions::slot_jack_connect ()
{
    perf().set_jack_mode(true);
}

void
qseditoptions::slot_jack_disconnect ()
{
    perf().set_jack_mode(false);
}

void
qseditoptions::slot_master_cond ()
{
    rc().with_jack_master_cond(ui->chkJackConditional->isChecked());
    sync();
}

void
qseditoptions::slot_time_master ()
{
    rc().with_jack_master(ui->chkJackMaster->isChecked());
    sync();
}

void
qseditoptions::slot_transport_support ()
{
    rc().with_jack_transport(ui->chkJackTransport->isChecked());
    sync();
}

void
qseditoptions::slot_jack_midi ()
{
    rc().with_jack_midi(ui->chkJackNative->isChecked());
    sync();
}

void
qseditoptions::slot_jack_auto_connect ()
{
    rc().jack_auto_connect(ui->chkJackAutoConnect->isChecked());
    sync();
}

void
qseditoptions::reload_needed (bool flag)
{
    m_reload_needed = flag;
    if (flag)
        enable_reload_button(true);
}

void
qseditoptions::slot_io_maps ()
{
    perf().store_output_map();
    perf().store_input_map();

    bool outportmap = output_port_map().active();
    bool inportmap = input_port_map().active();
    ui->outPortsMappedCheck->setChecked(outportmap);
    ui->inPortsMappedCheck->setChecked(inportmap);
    rc().portmaps_active(outportmap && inportmap);
    reload_needed(true);
}

void
qseditoptions::slot_remove_io_maps ()
{
    perf().clear_output_map();
    perf().clear_input_map();
    ui->outPortsMappedCheck->setChecked(false);
    ui->inPortsMappedCheck->setChecked(false);
    rc().portmaps_active(false);
    reload_needed(true);
}

void
qseditoptions::slot_activate_input_map ()
{
    bool active = ui->inPortsMappedCheck->isChecked();
    perf().activate_input_map(active);

    bool outportmap = output_port_map().active();
    bool inportmap = input_port_map().active();
    rc().portmaps_active(outportmap && inportmap);
    modify_rc();
}

void
qseditoptions::slot_activate_output_map ()
{
    bool active = ui->outPortsMappedCheck->isChecked();
    perf().activate_output_map(active);

    bool outportmap = output_port_map().active();
    bool inportmap = input_port_map().active();
    rc().portmaps_active(outportmap && inportmap);
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
        else if (usr().want_jack_session())
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
        usr().session_manager("nsm");
        ui->label_nsm_url->setEnabled(true);
        ui->label_nsm_url->setText("NSM URL");
        ui->lineEditNsmUrl->setEnabled(true);
    }
    else if (buttonno == static_cast<int>(usrsettings::session::jack))
        usr().session_manager("jack");
    else
        usr().session_manager("none");

    if (usr().session_manager() != current)
        modify_usr();
}

void
qseditoptions::slot_ui_scaling ()
{
    QString qs = ui->lineEditUiScaling->text();                 /* w */
    QString qheight = ui->lineEditUiScalingHeight->text();      /* h */
    ui_scaling_helper(qs, qheight);
    reload_needed(true);
}

/**
 *  Backs up the current settings logged into the various settings object into
 *  the "backup" members, then calls close().
 */

void
qseditoptions::okay ()
{
    if (reload_needed())
        enable_reload_button(true);

    backup();
    close();
}

/**
 *  Restores the settings from the "backup" variables, then calls
 *  sync()
 */

void
qseditoptions::cancel ()
{
    rc() = m_backup_rc;
    usr() = m_backup_usr;
    reload_needed(false);
    enable_reload_button(false);
    sync();
    close();
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
 *
 *  Note that "foreach" is a Qt-specific keyword, not a C++ keyword.
 */

void
qseditoptions::sync ()
{
    sync_rc();
    sync_usr();
}

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

    int rbid = perf().song_mode() ? playmode_button_song : playmode_button_live ;
    foreach (QAbstractButton * button, m_live_song_buttons->buttons())
    {
        int bid = m_live_song_buttons->id(button);
        button->setChecked(bid == rbid);
    }
    ui->checkBoxVerbose->setChecked(rc().verbose());
    ui->checkBoxLoadMostRecent->setChecked(rc().load_most_recent());
    ui->checkBoxShowFullRecentPaths->setChecked(rc().full_recent_paths());
    ui->checkBoxLongBussNames->setChecked(rc().is_port_naming_long());
    ui->checkBoxLockMainWindow->setChecked(usr().lock_main_window());
    ui->checkBoxSwapCoordinates->setChecked(usr().swap_coordinates());
    ui->checkBoxBoldGridSlots->setChecked(usr().progress_bar_thick());

    QString filename = qt(rc().config_filename());
    ui->checkBoxSaveRc->setChecked(rc().auto_rc_save());
    ui->checkBoxActiveRc->setChecked(true);     /* ALWAYS active */
    ui->lineEditRc->setText(filename);

    filename = qt(rc().user_filename());
    ui->checkBoxSaveUsr->setChecked(rc().auto_usr_save());
    ui->checkBoxActiveUsr->setChecked(rc().user_file_active());
    ui->lineEditUsr->setText(filename);

    filename = qt(rc().mute_group_filename());
    ui->checkBoxSaveMutes->setChecked(rc().auto_mutes_save());
    ui->checkBoxActiveMutes->setChecked(rc().mute_group_active());
    ui->lineEditMutes->setText(filename);

    filename = qt(rc().playlist_filename());
    ui->checkBoxSavePlaylist->setChecked(rc().auto_playlist_save());
    ui->checkBoxActivePlaylist->setChecked(rc().playlist_active());
    ui->lineEditPlaylist->setText(filename);

    filename = qt(rc().midi_control_filename());
    ui->checkBoxSaveCtrl->setChecked(rc().auto_ctrl_save());    /* read-only */
    ui->checkBoxActiveCtrl->setChecked(rc().midi_control_active());
    ui->lineEditCtrl->setText(filename);

    filename = qt(rc().notemap_filename());
    ui->checkBoxSaveDrums->setChecked(rc().auto_drums_save());  /* read-only */
    ui->checkBoxActiveDrums->setChecked(rc().notemap_active());
    ui->lineEditDrums->setText(filename);

    filename = qt(rc().palette_filename());
    ui->checkBoxSavePalette->setChecked(rc().auto_palette_save());
    ui->checkBoxActivePalette->setChecked(rc().palette_active());
    ui->lineEditPalette->setText(filename);

    filename = qt(usr().style_sheet());

    /*
     * We will never save the style sheet from within Seq66.  We leave the
     * checkbox visible, unchecked, and disabled.
     *
     * ui->checkBoxSaveStyleSheet->hide();
     */

    ui->checkBoxSaveStyleSheet->setChecked(false);
    ui->checkBoxActiveStyleSheet->setChecked(usr().style_sheet_active());
    ui->lineEditStyleSheet->setText(filename);

    bool outportmap = output_port_map().active();
    bool inportmap = input_port_map().active();
    ui->outPortsMappedCheck->setChecked(outportmap);
    ui->inPortsMappedCheck->setChecked(inportmap);
    ui->checkBoxVirtualPorts->setChecked(rc().manual_ports());

    std::string value = std::to_string(rc().manual_port_count());
    ui->lineEditOutputCount->setText(qt(value));
    value = std::to_string(rc().manual_in_port_count());
    ui->lineEditInputCount->setText(qt(value));

    ui->checkBoxRecordByChannel->setChecked(rc().filter_by_channel());
}

void
qseditoptions::sync_usr ()
{
    char tmp[32];
    snprintf(tmp, sizeof tmp, "%g", usr().window_scale());
    ui->lineEditUiScaling->setText(tmp);
    snprintf(tmp, sizeof tmp, "%g", usr().window_scale_y());

    ui->lineEditUiScalingHeight->setText(tmp);
    ui->chkNoteResume->setChecked(usr().resume_note_ons());
    ui->chkUseFilesPPQN->setChecked(usr().use_file_ppqn());
    ui->spinKeyHeight->setValue(usr().key_height());

    show_session(usr().session_manager());
    set_scaling_fields();
    set_set_size_fields();
    set_progress_box_fields();

    snprintf(tmp, sizeof tmp, "%i", usr().fingerprint_size());
    ui->lineEditFingerprintSize->setText(tmp);

}

/**
 *  Instead of this sequence of calls, we could send a Qt signal from
 *  qclocklayout to eventually call the qsmainwnd slot.  We need to make this
 *  item cause immediate action.
 */

void
qseditoptions::enable_bus_item (int bus, bool enabled)
{
    m_parent_widget->enable_bus_item(bus, enabled);
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
            modify_usr();
            usr().resume_note_ons(resumenotes);
            perf().resume_note_ons(resumenotes);
            sync();
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
            m_parent_widget->set_ppqn_text(temp);
            m_ppqn_list.current(temp);
            ui->combo_box_ppqn->setItemText(0, text);
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
            usr().use_file_ppqn(ufppqn);
            sync();
            modify_usr();
        }
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
    sync();
    if (m_is_initialized)
        modify_usr();
}

void
qseditoptions::set_scaling_fields ()
{
    char tmp[32];
    snprintf(tmp, sizeof tmp, "%.1f", usr().window_scale());
    ui->lineEditUiScaling->setText(tmp);
    snprintf(tmp, sizeof tmp, "%.1f", usr().window_scale_y());
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
    snprintf(tmp, sizeof tmp, "%.1f", usr().progress_box_width());
    ui->lineEditProgressBox->setText(tmp);
    snprintf(tmp, sizeof tmp, "%.1f", usr().progress_box_height());
    ui->lineEditProgressBoxHeight->setText(tmp);
}

void
qseditoptions::slot_progress_box_width ()
{
    const QString qs = ui->lineEditProgressBox->text();
    const std::string wtext = qs.toStdString();
    if (! wtext.empty())
    {
        double w = std::stod(wtext);
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
        double h = std::stod(htext);
        if (usr().progress_box_size(w, h))
            modify_usr();
        else
            set_progress_box_fields();
    }
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

void
qseditoptions::slot_palette_filename ()
{
    const QString qs = ui->lineEditPalette->text();
    std::string text = qs.toStdString();
    if (! text.empty())
    {
        text = filename_base(text);
        if (text != rc().palette_filename())
        {
            rc().palette_filename(text);
            rc().auto_palette_save(true);
            ui->checkBoxSavePalette->setChecked(true);
            modify_rc();
        }
    }
}

void
qseditoptions::slot_palette_save_click ()
{
    bool on = ui->checkBoxSavePalette->isChecked();
    rc().auto_palette_save(on);
    modify_rc();
}

void
qseditoptions::slot_palette_save_now_click ()
{
    std::string palfile = rc().palette_filespec();
    if (palfile.empty())
    {
        QString qs = ui->lineEditPalette->text();
        palfile = qs.toStdString();
        rc().palette_filename(palfile);
        palfile = rc().palette_filespec();
    }
    if (! palfile.empty())
    {
        if (save_palette(global_palette(), palfile))
        {
            /*
             * TODO: report full file-path saved
             */
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
qseditoptions::slot_palette_active_click ()
{
    bool on = ui->checkBoxActivePalette->isChecked();
    rc().palette_active(on);
    modify_rc();
}

void
qseditoptions::slot_verbose_active_click ()
{
    bool on = ui->checkBoxVerbose->isChecked();
    rc().verbose(on);
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
qseditoptions::slot_lock_main_window_click ()
{
    bool on = ui->checkBoxLockMainWindow->isChecked();
    usr().lock_main_window(on);
    modify_usr();
    m_parent_widget->lock_main_window(on);
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
qseditoptions::slot_double_click_edit ()
{
    bool on = ui->checkBoxDoubleClickEdit->isChecked();
    rc().allow_click_edit(on);
    modify_rc();
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

void
qseditoptions::slot_rc_filename ()
{
    const QString qs = ui->lineEditRc->text();
    std::string text = qs.toStdString();
    if (text != rc().config_filename())
    {
        rc().config_filename(text);
        modify_rc();
    }
}

void
qseditoptions::modify_rc ()
{
    rc().auto_rc_save(true);
    rc().modify();
    reload_needed(true);
    ui->checkBoxSaveRc->setChecked(true);
}

void
qseditoptions::modify_usr ()
{
    rc().auto_usr_save(true);
    usr().modify();
    reload_needed(true);
    ui->checkBoxSaveUsr->setChecked(true);
}

void
qseditoptions::slot_usr_save_click ()
{
    bool on = ui->checkBoxSaveUsr->isChecked();
    rc().auto_usr_save(on);
}

void
qseditoptions::slot_usr_active_click ()
{
    bool on = ui->checkBoxActiveUsr->isChecked();
    usr().modify();
    rc().user_file_active(on);
    rc().auto_rc_save(true);
}

void
qseditoptions::slot_usr_filename ()
{
    const QString qs = ui->lineEditUsr->text();
    std::string text = qs.toStdString();
    if (text != rc().user_filename())
    {
        rc().user_filename(text);
        modify_rc();
        modify_usr();
    }
}

void
qseditoptions::slot_mutes_save_click ()
{
    bool on = ui->checkBoxSaveMutes->isChecked();
    rc().auto_mutes_save(on);
    modify_rc();
}

void
qseditoptions::slot_mutes_active_click ()
{
    bool on = ui->checkBoxActiveMutes->isChecked();
    rc().mute_group_active(on);
    modify_rc();
}

void
qseditoptions::slot_mutes_filename ()
{
    const QString qs = ui->lineEditMutes->text();
    std::string text = qs.toStdString();
    if (text != rc().mute_group_filename())
    {
        rc().mute_group_filename(text);
        rc().auto_mutes_save(true);
        modify_rc();
        ui->checkBoxSaveMutes->setChecked(true);
    }
}

void
qseditoptions::slot_playlist_save_click ()
{
    bool on = ui->checkBoxSavePlaylist->isChecked();
    rc().auto_playlist_save(on);
    modify_rc();
}

void
qseditoptions::slot_playlist_active_click ()
{
    bool on = ui->checkBoxActivePlaylist->isChecked();
    rc().playlist_active(on);
    modify_rc();
}

void
qseditoptions::slot_playlist_filename ()
{
    const QString qs = ui->lineEditPlaylist->text();
    std::string text = qs.toStdString();
    if (! text.empty())
    {
        if (text != rc().playlist_filename())
        {
            perf().playlist_filename(text); /* rc().playlist_filename(text) */
            rc().auto_playlist_save(true);
            rc().auto_rc_save(true);
            ui->checkBoxSavePlaylist->setChecked(true);
        }
    }
}

/*
 *  No ctrl-save slot needed here, saving is done only at first-exit time.
 */

void
qseditoptions::slot_ctrl_active_click ()
{
    bool on = ui->checkBoxActiveCtrl->isChecked();
    rc().midi_control_active(on);
    modify_rc();
}

void
qseditoptions::slot_ctrl_filename ()
{
    const QString qs = ui->lineEditCtrl->text();
    std::string text = qs.toStdString();
    if (! text.empty())
    {
        if (text != rc().midi_control_filename())
        {
            rc().midi_control_filename(text);
            rc().auto_ctrl_save(true);
            modify_rc();
            ui->checkBoxSaveCtrl->setChecked(true);
        }
    }
}

void
qseditoptions::slot_drums_active_click ()
{
    bool on = ui->checkBoxActiveDrums->isChecked();
    rc().notemap_active(on);
    modify_rc();
}

void
qseditoptions::slot_drums_filename ()
{
    const QString qs = ui->lineEditDrums->text();
    std::string text = qs.toStdString();
    if (! text.empty())
    {
        if (text != rc().notemap_filename())
        {
            rc().notemap_filename(text);
            modify_rc();
            ui->checkBoxSaveDrums->setChecked(true);
        }
    }
}

void
qseditoptions::slot_stylesheet_active_click ()
{
    bool on = ui->checkBoxActiveStyleSheet->isChecked();
    usr().style_sheet_active(on);
    modify_usr();
}

void
qseditoptions::slot_stylesheet_filename ()
{
    const QString qs = ui->lineEditStyleSheet->text();
    std::string text = qs.toStdString();
    if (! text.empty())
    {
        if (text != usr().style_sheet())
        {
            bool check = ! qs.isEmpty();
            usr().style_sheet(text);
            ui->checkBoxActiveStyleSheet->setChecked(check);
            modify_usr();

            /*
             * Now hidden: ui->checkBoxSaveStyleSheet->setChecked(true);
             */
        }
    }
}

void
qseditoptions::slot_flag_reload ()
{
    signal_for_restart();           /* warnprint("Session reload request"); */
}

void
qseditoptions::slot_key_test (const QString &)
{
    QString s = ui->text_edit_key->text();
    if (s.length() > 0)
    {
        QChar first = s.at(0);
        ushort us = first.unicode();
        msgprintf(msglevel::status, "0x%x", unsigned(us));
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
    int oldindex = perf().midi_control_out().nominal_buss() + 1;
    if (index != oldindex)
    {
        bool enable = index >= 0;
        perf().midi_control_out().is_enabled(enable);
        perf().midi_control_out().nominal_buss(index - 1);
        rc().auto_ctrl_save(true);
        reload_needed(true);
        ui->checkBoxSaveCtrl->setChecked(true);
    }
}

void
qseditoptions::slot_input_bus (int index)
{
    int oldindex = perf().midi_control_in().nominal_buss() + 1;
    if (index != oldindex)
    {
        bool enable = index >= 0;
        perf().midi_control_in().is_enabled(enable);
        perf().midi_control_in().nominal_buss(index - 1);
        rc().auto_ctrl_save(true);
        reload_needed(true);
        ui->checkBoxSaveCtrl->setChecked(true);
    }
}

void
qseditoptions::slot_tempo_track ()
{
    QString text = ui->lineEditTempoTrack->text();
    std::string t = text.toStdString();
    if (t.empty())
    {
        ui->pushButtonTempoTrack->setEnabled(false);
    }
    else
    {
        int track = string_to_int(t);
        bool ok = track >= 0 && track < seq::maximum();
        ui->pushButtonTempoTrack->setEnabled(ok);
        reload_needed(true);
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
        int track = string_to_int(t);
        rc().tempo_track_number(track);
        modify_rc();
    }
}

void
qseditoptions::slot_record_by_channel ()
{
    bool on = ui->checkBoxRecordByChannel->isChecked();
    rc().filter_by_channel(on);
    modify_rc();
}

void
qseditoptions::slot_virtual_ports ()
{
    bool on = ui->checkBoxVirtualPorts->isChecked();
    rc().manual_ports(on);
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

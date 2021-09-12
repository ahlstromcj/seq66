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
 * \updates       2021-09-11
 * \license       GNU GPLv2 or above
 *
 *      This version is located in Edit / Preferences.
 */

#include <QButtonGroup>

#include "play/performer.hpp"           /* seq66::performer class           */
#include "util/filefunctions.hpp"       /* seq66::filename_base()           */
#include "gui_palette_qt5.hpp"          /* seq66::global_palette()          */
#include "palettefile.hpp"              /* seq66::palettefile class         */
#include "qclocklayout.hpp"
#include "qinputcheckbox.hpp"
#include "qseditoptions.hpp"
#include "qsmainwnd.hpp"

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
 *  The main constructor.
 */

qseditoptions::qseditoptions (performer & p, QWidget * parent)
 :
    QDialog                 (parent),
    ui                      (new Ui::qseditoptions),
    m_parent_widget         (dynamic_cast<qsmainwnd *>(parent)),
    m_perf                  (p),
    m_ppqn_list             (default_ppqns()),  /* see the settings module  */
    m_is_initialized        (false),
    m_backup_JackTransport  (false),
    m_backup_TimeMaster     (false),
    m_backup_MasterCond     (false),
    m_backup_NoteResume     (false),
    m_backup_JackMidi       (false),
    m_backup_KeyHeight      (usr().key_height())
{
    ui->setupUi(this);
    backup();

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

    /*
     * Play Options tab
     */

    ui->chkNoteResume->setChecked(usr().resume_note_ons());
    connect
    (
        ui->chkNoteResume, SIGNAL(stateChanged(int)),
        this, SLOT(slot_note_resume())
    );

    ui->chkUseFilesPPQN->setChecked(usr().use_file_ppqn());
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
    show_sets_mode(rc().sets_mode());
    connect
    (
        rgroup, SIGNAL(buttonClicked(int)),
        this, SLOT(slot_sets_mode(int))
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
    show_session(usr().session_manager());
    connect
    (
        sgroup, SIGNAL(buttonClicked(int)),
        this, SLOT(slot_session(int))
    );

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

    QButtonGroup * bgroup = new QButtonGroup(this);
    bgroup->addButton(ui->radio_live_mode, playmode_button_live);
    bgroup->addButton(ui->radio_song_mode, playmode_button_song);

    /*
     * Note that "foreach" is a Qt-specific keyword, not a C++ keyword.
     */

    int rbid = perf().song_mode() ? playmode_button_song : playmode_button_live ;
    foreach (QAbstractButton * button, bgroup->buttons())
    {
        int bid = bgroup->id(button);
        if (bid == rbid)
        {
            button->setChecked(true);
            break;
        }
    }
    connect
    (
        bgroup, SIGNAL(buttonClicked(int)),
        this, SLOT(slot_jack_mode(int))
    );

    /*
     * Display tab.
     */

    ui->spinKeyHeight->setMinimum(usr().min_key_height());
    ui->spinKeyHeight->setMaximum(usr().max_key_height());
    ui->spinKeyHeight->setValue(usr().key_height());
    connect
    (
        ui->spinKeyHeight, SIGNAL(valueChanged(int)),
        this, SLOT(slot_key_height())
    );

    set_scaling_fields();
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

    set_set_size_fields();
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
    set_progress_box_fields();
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

    char tmp[32];
    snprintf(tmp, sizeof tmp, "%i", usr().fingerprint_size());
    ui->lineEditFingerprintSize->setText(tmp);
    connect
    (
        ui->lineEditFingerprintSize, SIGNAL(editingFinished()),
        this, SLOT(slot_fingerprint_size())
    );

    /*
     * Palette.
     */

    std::string palname = rc().palette_filename();
    if (palname.empty())
        palname = rc().application_name() + ".palette";

    QString qcn = QString::fromStdString(palname);
    ui->lineEditPaletteFile->setText(qcn);
    connect
    (
        ui->lineEditPaletteFile, SIGNAL(textEdited(const QString &)),
        this, SLOT(slot_palette_file(const QString &))
    );
    connect
    (
        ui->pushButtonSavePalette, SIGNAL(clicked(bool)),
        this, SLOT(slot_palette_save_click())
    );

    /*
     * UI Boolean options.
     */

    ui->checkBoxPaletteActive->setChecked(rc().palette_active());
    connect
    (
        ui->checkBoxPaletteActive, SIGNAL(clicked(bool)),
        this, SLOT(slot_palette_active_click())
    );

    ui->checkBoxVerbose->setChecked(rc().verbose());
    connect
    (
        ui->checkBoxVerbose, SIGNAL(clicked(bool)),
        this, SLOT(slot_verbose_active_click())
    );

    ui->checkBoxLoadMostRecent->setChecked(rc().load_most_recent());
    connect
    (
        ui->checkBoxLoadMostRecent, SIGNAL(clicked(bool)),
        this, SLOT(slot_load_most_recent_click())
    );

    ui->checkBoxShowFullRecentPaths->setChecked(rc().full_recent_paths());
    connect
    (
        ui->checkBoxShowFullRecentPaths, SIGNAL(clicked(bool)),
        this, SLOT(slot_show_full_paths_click())
    );

    bool longportnames = rc().is_port_naming_long();
    ui->checkBoxLongBussNames->setChecked(longportnames);
    connect
    (
        ui->checkBoxLongBussNames, SIGNAL(clicked(bool)),
        this, SLOT(slot_long_buss_names_click())
    );

    bool autosaverc = rc().auto_option_save();
    ui->checkBoxSaveRc->setChecked(autosaverc);
    connect
    (
        ui->checkBoxSaveRc, SIGNAL(clicked(bool)),
        this, SLOT(slot_rc_save_click())
    );

    bool autosaveusr = usr().save_user_config();
    ui->checkBoxSaveUsr->setChecked(autosaveusr);
    connect
    (
        ui->checkBoxSaveUsr, SIGNAL(clicked(bool)),
        this, SLOT(slot_usr_save_click())
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
    mastermidibus * masterbus = perf().master_bus();
    const clockslist & opm = output_port_map();
    bool outportmap = opm.active();
    if (not_nullptr(masterbus))
    {
        int buses = outportmap ? opm.count() : masterbus->get_num_out_buses() ;
        ui->clocksScrollArea->setWidget(central);
        ui->clocksScrollArea->setWidgetResizable(true);
        for (int bus = 0; bus < buses; ++bus)
        {
            qclocklayout * tempqc = new qclocklayout(this, perf(), bus);
            vboxclocks->addLayout(tempqc->layout());
        }
    }
    ui->outPortsMappedCheck->setChecked(outportmap);

    QSpacerItem * spacer = new QSpacerItem
    (
        40, 20, QSizePolicy::Expanding, QSizePolicy::Expanding
    );
    vboxclocks->addItem(spacer);

    connect
    (
        ui->pushButtonStoreMap, SIGNAL(clicked(bool)),
        this, SLOT(slot_io_maps())
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
    if (not_nullptr(masterbus))
    {
        int buses = inportmap ? ipm.count() : masterbus->get_num_in_buses() ;
        ui->inputsScrollArea->setWidget(central2);
        ui->inputsScrollArea->setWidgetResizable(true);
        for (int bus = 0; bus < buses; ++bus)
        {
            qinputcheckbox * tempqi = new qinputcheckbox(this, perf(), bus);
            vboxinputs->addWidget(tempqi->input_checkbox());
        }
    }
    ui->inPortsMappedCheck->setChecked(inportmap);

    /*
     * I/O Port Boolean options.
     */

    ui->checkBoxRecordByChannel->setChecked(rc().palette_active());
    connect
    (
        ui->checkBoxRecordByChannel, SIGNAL(clicked(bool)),
        this, SLOT(slot_record_by_channel())
    );

    ui->checkBoxVirtualPorts->setChecked(rc().palette_active());
    connect
    (
        ui->checkBoxVirtualPorts, SIGNAL(clicked(bool)),
        this, SLOT(slot_virtual_ports())
    );

    /*
     * Display tab.
     */

    ui->checkBoxPaletteActive->setChecked(rc().palette_active());
    connect
    (
        ui->checkBoxPaletteActive, SIGNAL(clicked(bool)),
        this, SLOT(slot_palette_active_click())
    );

    QSpacerItem * spacer2 = new QSpacerItem
    (
        40, 20, QSizePolicy::Expanding, QSizePolicy::Expanding
    );
    vboxinputs->addItem(spacer2);
    sync();

    std::string clid = perf().client_id_string();
    ui->plainTextEditClientId->setPlainText(QString::fromStdString(clid));
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
        QString combo_text = QString::fromStdString(p);
        ui->combo_box_ppqn->clear();
        ui->combo_box_ppqn->insertItem(0, combo_text);
        for (int i = 1; i < count; ++i)
        {
            p = m_ppqn_list.at(i);
            combo_text = QString::fromStdString(p);
            ui->combo_box_ppqn->insertItem(i, combo_text);
            if (std::stoi(p) == perf().ppqn())
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
qseditoptions::slot_sets_mode (int buttonno)
{
    if (buttonno == setsmode_button_autoarm)
        rc().sets_mode("auto-arm");
    else if (buttonno == setsmode_button_additive)
        rc().sets_mode("additive");
    else if (buttonno == setsmode_button_allsets)
        rc().sets_mode("auto-arm");
    else
        rc().sets_mode("normal");
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
qseditoptions::slot_io_maps ()
{
    perf().store_output_map();
    perf().store_input_map();
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
        if (usr().want_nsm_session())
        {
            url_modifiable = true;
            tenturl = usr().session_url();
        }
        else if (usr().want_jack_session())
        {
            tenturl = rc().jack_session();          /* JACK session UUID    */
        }
    }
    switch (sm)
    {
        case usrsettings::session::none:
            ui->radio_session_none->setChecked(true);
            ui->label_nsm_url->setText("N/A");
            break;

        case usrsettings::session::nsm:
#if defined SEQ66_NSM_SUPPORT
            ui->radio_session_nsm->setChecked(true);
            ui->label_nsm_url->setText("NSM URL");
            ui->lineEditNsmUrl->setText(QString::fromStdString(tenturl));
#endif
            break;

        case usrsettings::session::jack:
            ui->radio_session_jack->setChecked(true);
            ui->label_nsm_url->setText("JACK UUID");
            ui->lineEditNsmUrl->setText(QString::fromStdString(tenturl));
            break;

        default:
            break;
    }
    ui->lineEditNsmUrl->setEnabled(url_modifiable);
}

void
qseditoptions::slot_session (int buttonno)
{
    usrsettings::session current = usr().session_manager();
    if (buttonno == static_cast<int>(usrsettings::session::nsm))
        usr().session_manager("nsm");
    else if (buttonno == static_cast<int>(usrsettings::session::jack))
        usr().session_manager("jack");
    else
        usr().session_manager("none");

    usr().save_user_config(usr().session_manager() != current);
}

void
qseditoptions::slot_ui_scaling ()
{
    QString qs = ui->lineEditUiScaling->text();                 /* w */
    QString qheight = ui->lineEditUiScalingHeight->text();      /* h */
    ui_scaling_helper(qs, qheight);
}

/**
 *  Backs up the current settings logged into the various settings object into
 *  the "backup" members, then calls close().
 */

void
qseditoptions::okay ()
{
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
    rc().with_jack_transport(m_backup_JackTransport);
    rc().with_jack_master_cond(m_backup_MasterCond);
    rc().with_jack_master(m_backup_TimeMaster);
    rc().with_jack_midi(m_backup_JackMidi);
    usr().key_height(m_backup_KeyHeight);
    usr().resume_note_ons(m_backup_NoteResume);
    perf().resume_note_ons(m_backup_NoteResume);
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
    m_backup_JackTransport = rc().with_jack_transport();
    m_backup_JackMidi = rc().with_jack_midi();
    m_backup_MasterCond = rc().with_jack_master_cond();
    m_backup_TimeMaster = rc().with_jack_master();
    m_backup_KeyHeight = usr().key_height();
    m_backup_NoteResume = perf().resume_note_ons();
}

/**
 *  Sync with preferences.  In other words, the current values in the various
 *  settings objects are used to set the user-interface elements in this
 *  dialog.
 */

void
qseditoptions::sync ()
{
    ui->chkJackTransport->setChecked(rc().with_jack_transport());
    ui->chkJackNative->setChecked(rc().with_jack_midi());
    ui->chkJackMaster->setChecked(rc().with_jack_master());
    ui->chkJackConditional->setChecked(rc().with_jack_master_cond());

    /*
     * These JACK options are meaningless if JACK Transport is disabled.
     */

    ui->chkJackMaster->setDisabled(! rc().with_jack_transport());
    ui->chkJackConditional->setDisabled(! rc().with_jack_transport());

    /*
     * Both these options are usr() options now.
     */

    ui->chkNoteResume->setChecked(usr().resume_note_ons());
    ui->chkUseFilesPPQN->setChecked(usr().use_file_ppqn());
    ui->spinKeyHeight->setValue(usr().key_height());

    char tmp[32];
    snprintf(tmp, sizeof tmp, "%g", usr().window_scale());
    ui->lineEditUiScaling->setText(tmp);
    snprintf(tmp, sizeof tmp, "%g", usr().window_scale_y());
    ui->lineEditUiScalingHeight->setText(tmp);
}

/**
 *  Instead of this sequence of calls, we could send a Qt signal from
 *  qclocklayout to eventually call the qsmainwnd slot.
 */

void
qseditoptions::enable_bus_item (int bus, bool enabled)
{
    m_parent_widget->enable_bus_item(bus, enabled);
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
        int p = std::stoi(temp);
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
        int rows = std::stoi(valuetext);
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
        int columns = std::stoi(valuetext);
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
    const std::string text = qs.toStdString();
    if (! text.empty())
    {
        double sz = std::stoi(text);
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
qseditoptions::slot_palette_file (const QString & qs)
{
    std::string valuetext = qs.toStdString();
    valuetext = filename_base(valuetext);
    rc().palette_filename(valuetext);
}

void
qseditoptions::slot_palette_save_click ()
{
    std::string palfile = rc().palette_filespec();
    if (palfile.empty())
    {
        QString qs = ui->lineEditPaletteFile->text();
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
    bool on = ui->checkBoxPaletteActive->isChecked();
    rc().palette_active(on);
}

void
qseditoptions::slot_verbose_active_click ()
{
    bool on = ui->checkBoxVerbose->isChecked();
    rc().verbose(on);
}

void
qseditoptions::slot_load_most_recent_click ()
{
    bool on = ui->checkBoxLoadMostRecent->isChecked();
    rc().load_most_recent(on);
}

void
qseditoptions::slot_show_full_paths_click ()
{
    bool on = ui->checkBoxShowFullRecentPaths->isChecked();
    rc().full_recent_paths(on);
}

void
qseditoptions::slot_long_buss_names_click ()
{
    bool on = ui->checkBoxLongBussNames->isChecked();
    rc().port_naming(on ? "long" : "short");
}

void
qseditoptions::slot_rc_save_click ()
{
    bool on = ui->checkBoxSaveRc->isChecked();
    rc().auto_option_save(on);
    rc().modify();
}

void
qseditoptions::modify_usr ()
{
    usr().save_user_config(true);
    usr().modify();
    ui->checkBoxSaveUsr->setChecked(true);
}

void
qseditoptions::slot_usr_save_click ()
{
    bool on = ui->checkBoxSaveUsr->isChecked();
    usr().save_user_config(on);
    usr().modify();
}

void
qseditoptions::slot_key_test (const QString &)
{
    QString s = ui->text_edit_key->text();
    if (s.length() > 0)
    {
        QChar first = s.at(0);
        ushort us = first.unicode();
        printf("0x%x\n", unsigned(us));
    }
}

void
qseditoptions::slot_clock_start_modulo (int /*arg1*/)
{
    // TODO
}

void
qseditoptions::slot_tempo_track()
{
    // TODO
}

void
qseditoptions::slot_tempo_track_set()
{
    // TODO
}

void
qseditoptions::slot_record_by_channel ()
{
    bool on = ui->checkBoxRecordByChannel->isChecked();
    rc().filter_by_channel(on);
}

void
qseditoptions::slot_virtual_ports ()
{
    bool on = ui->checkBoxVirtualPorts->isChecked();
    rc().manual_ports(on);
}

void
qseditoptions::slot_virtual_out_count ()
{
    QString text = ui->lineEditOutputCount->text();
    int count = std::stoi(text.toStdString());
    rc().manual_port_count(count);
}

void
qseditoptions::slot_virtual_in_count ()
{
    QString text = ui->lineEditInputCount->text();
    int count = std::stoi(text.toStdString());
    rc().manual_in_port_count(count);
}

}           // namespace seq66

/*
 * qseditoptions.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

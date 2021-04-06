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
 * \file          qseditoptions.cpp
 *
 *      The Qt version of the Options editor.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2021-04-05
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

static const int Tab_MIDI_Clock         =  0;
static const int Tab_MIDI_Input         =  1;
static const int Tab_MIDI_Display       =  2;
static const int Tab_MIDI_JACK          =  3;
static const int Tab_MIDI_Play_Options  =  4;

/**
 *  Button numbering for JACK Start Mode radio-buttons.
 */

enum radio_button_t
{
    radio_button_live,
    radio_button_song
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
        this, SLOT(update_jack_connect())
    );
    connect
    (
        ui->btnJackDisconnect, SIGNAL(clicked(bool)),
        this, SLOT(update_jack_disconnect())
    );

    /*
     * Jack Sync tab.  JACK Transport/MIDI tab.
     */

    connect
    (
        ui->chkJackTransport, SIGNAL(stateChanged(int)),
        this, SLOT(update_transport_support())
    );
    connect
    (
        ui->chkJackConditional, SIGNAL(stateChanged(int)),
        this, SLOT(update_master_cond())
    );
    connect
    (
        ui->chkJackMaster, SIGNAL(stateChanged(int)),
        this, SLOT(update_time_master())
    );
    connect
    (
        ui->chkJackNative, SIGNAL(stateChanged(int)),
        this, SLOT(update_jack_midi())
    );

    ui->chkNoteResume->setChecked(usr().resume_note_ons());
    connect
    (
        ui->chkNoteResume, SIGNAL(stateChanged(int)),
        this, SLOT(update_note_resume())
    );

    ui->chkUseFilesPPQN->setChecked(usr().use_file_ppqn());
    connect
    (
        ui->chkUseFilesPPQN, SIGNAL(stateChanged(int)),
        this, SLOT(update_use_file_ppqn())
    );

    /*
     *  Combo-box for changing the default PPQN.
     */

    (void) set_ppqn_combo();
    connect
    (
        ui->combo_box_ppqn, SIGNAL(currentTextChanged(const QString &)),
        this, SLOT(update_ppqn_by_text(const QString &))
    );

    /*
     * Create a button group to manage the mutual status of the JACK Live and
     * Song Mode buttons.
     */

    QButtonGroup * bgroup = new QButtonGroup(this);
    bgroup->addButton(ui->radio_live_mode, radio_button_live);
    bgroup->addButton(ui->radio_song_mode, radio_button_song);

    /*
     * Note that "foreach" is a Qt-specific keyword, not a C++ keyword.
     */

    int rbid = perf().song_mode() ? radio_button_song : radio_button_live ;
    foreach (QAbstractButton * button, bgroup->buttons())
    {
        int bid = bgroup->id(button);
        if (bid == rbid)
            button->setChecked(true);
        else
            button->setChecked(false);
    }
    connect
    (
        bgroup, SIGNAL(buttonClicked(int)),
        this, SLOT(slot_jack_mode(int))
    );

    /*
     * Display tab.
     */

    ui->spinKeyHeight->setMinimum(SEQ66_SEQKEY_HEIGHT_MIN);
    ui->spinKeyHeight->setMaximum(SEQ66_SEQKEY_HEIGHT_MAX);
    ui->spinKeyHeight->setValue(usr().key_height());
    connect
    (
        ui->spinKeyHeight, SIGNAL(valueChanged(int)),
        this, SLOT(update_key_height())
    );
    connect
    (
        ui->lineEditUiScaling, SIGNAL(editingFinished()),
        this, SLOT(update_ui_scaling_width())
    );
    connect
    (
        ui->lineEditUiScalingHeight, SIGNAL(editingFinished()),
        this, SLOT(update_ui_scaling_height())
    );

#if defined USE_QSEDITOPTIONS_UPDATE_PATTERN_EDITOR
    connect
    (
        ui->checkBoxKeplerSeqedit, SIGNAL(stateChanged(int)),
        this, SLOT(update_pattern_editor())
    );
#endif

    std::string palname = rc().palette_filename();
    if (palname.empty())
        palname = rc().application_name() + ".palette";

    QString qcn = QString::fromStdString(palname);
    ui->lineEditPaletteFile->setText(qcn);
    connect
    (
        ui->lineEditPaletteFile, SIGNAL(textEdited(const QString &)),
        this, SLOT(update_palette_file(const QString &))
    );
    connect
    (
        ui->pushButtonSavePalette, SIGNAL(clicked(bool)),
        this, SLOT(handle_palette_save_click())
    );
    connect
    (
        ui->checkBoxPaletteActive, SIGNAL(clicked(bool)),
        this, SLOT(handle_palette_active_click())
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
        this, SLOT(update_io_maps())
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

    QSpacerItem * spacer2 = new QSpacerItem
    (
        40, 20, QSizePolicy::Expanding, QSizePolicy::Expanding
    );
    vboxinputs->addItem(spacer2);
    syncWithInternals();

    std::string clid = perf().client_id_string();
    ui->plainTextEditClientId->setPlainText(QString::fromStdString(clid));
    m_is_initialized = true;

#if defined SEQ66_PLATFORM_WINDOWS
    ui->tabWidget->setTabEnabled(Tab_MIDI_JACK, false);
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
qseditoptions::slot_jack_mode (int buttonno)
{
    if (buttonno == radio_button_live)
    {
        perf().song_mode(false);
    }
    else if (buttonno == radio_button_song)
    {
        perf().song_mode(true);
    }
}

void
qseditoptions::update_jack_connect ()
{
    perf().set_jack_mode(true);
}

void
qseditoptions::update_jack_disconnect ()
{
    perf().set_jack_mode(false);
}

void
qseditoptions::update_master_cond ()
{
    rc().with_jack_master_cond(ui->chkJackConditional->isChecked());
    syncWithInternals();
}

void
qseditoptions::update_time_master ()
{
    rc().with_jack_master(ui->chkJackMaster->isChecked());
    syncWithInternals();
}

void
qseditoptions::update_transport_support ()
{
    rc().with_jack_transport(ui->chkJackTransport->isChecked());
    syncWithInternals();
}

void
qseditoptions::update_jack_midi ()
{
    rc().with_jack_midi(ui->chkJackNative->isChecked());
    syncWithInternals();
}

void
qseditoptions::update_io_maps ()
{
    perf().store_output_map();
    perf().store_input_map();
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
 *  syncWithInternals()
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
    syncWithInternals();
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
qseditoptions::syncWithInternals ()
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
 *  user-interface, and then calls syncWithInternals(), perhaps needlessly, to
 *  make sure the user-interface items correctly represent the settings.
 *  Resolves issue #5.
 */

void
qseditoptions::update_note_resume ()
{
    if (m_is_initialized)
    {
        bool resumenotes = ui->chkNoteResume->isChecked();
        if (perf().resume_note_ons() != resumenotes)
        {
            usr().save_user_config(true);
            usr().resume_note_ons(resumenotes);
            perf().resume_note_ons(resumenotes);
            syncWithInternals();
        }
    }
}

void
qseditoptions::update_ppqn_by_text (const QString & text)
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
            usr().save_user_config(true);
        }
    }
}

void
qseditoptions::update_use_file_ppqn ()
{
    if (m_is_initialized)
    {
        bool ufppqn = ui->chkUseFilesPPQN->isChecked();
        bool status = usr().use_file_ppqn();
        if (ufppqn != status)
        {
            usr().use_file_ppqn(ufppqn);
            syncWithInternals();
            usr().save_user_config(true);
        }
    }
}

/**
 *  Updates the usrsettings::key_height() setting in accord with the
 *  user-interface, and then calls syncWithInternals(), perhaps needlessly, to
 *  make sure the user-interface items correctly represent the settings.
 *  Also turns on the user-save setting, so that the settings will be written to
 *  the "usr" file upon exit.
 */

void
qseditoptions::update_key_height ()
{
    usr().key_height(ui->spinKeyHeight->value());
    syncWithInternals();
    if (m_is_initialized)
        usr().save_user_config(true);
}

void
qseditoptions::update_ui_scaling_width ()
{
    const QString qs = ui->lineEditUiScaling->text();               /* w */
    const std::string valuetext = qs.toStdString();
    if (! valuetext.empty())
    {
        QString qheight = ui->lineEditUiScalingHeight->text();      /* h */
        std::string hheight = qheight.toStdString();
        if (! hheight.empty())
        {
            std::string tuple = valuetext + "x" + hheight;
            if (usr().parse_window_scale(tuple))
                usr().save_user_config(true);
        }
    }
}

void
qseditoptions::update_ui_scaling_height ()
{
    const QString qs = ui->lineEditUiScalingHeight->text();         /* h */
    const std::string valuetext = qs.toStdString();
    if (! valuetext.empty())
    {
        QString qwidth = ui->lineEditUiScaling->text();             /* w */
        std::string hwidth = qwidth.toStdString();
        if (! hwidth.empty())
        {
            std::string tuple = hwidth + "x" + valuetext;
            if (usr().parse_window_scale(tuple))
                usr().save_user_config(true);
        }
    }
}

void
qseditoptions::update_palette_file (const QString & qs)
{
    std::string valuetext = qs.toStdString();
    valuetext = filename_base(valuetext);
    rc().palette_filename(valuetext);
}

void
qseditoptions::handle_palette_save_click ()
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
qseditoptions::handle_palette_active_click ()
{
    bool on = ui->checkBoxPaletteActive->isChecked();
    rc().palette_active(on);
}

#if defined USE_QSEDITOPTIONS_UPDATE_PATTERN_EDITOR

/**
 *  Seq66 has a new pattern editor GUI for Qt that is larger and more
 *  functional than the Kepler34 pattern editor.  However, it has the
 *  side-effect, currently, of making the main window larger.
 *  This slot sets the new feature to off if the check-box is checked.
 *  Disabled.
 */

void
qseditoptions::update_pattern_editor ()
{
    bool use_kepler_seqedit = ui->checkBoxKeplerSeqedit->isChecked();
    usr().use_new_seqedit(! use_kepler_seqedit);
    usr().save_user_config(true);
}

#endif

/**
 *  Added for Seq66.  Not yet filled with functionality.
 */

void
qseditoptions::on_spinBoxClockStartModulo_valueChanged(int /*arg1*/)
{

}

/**
 *  Added for Seq66.  Not yet filled with functionality.
 */

void
qseditoptions::on_plainTextEditTempoTrack_textChanged()
{

}

/**
 *  Added for Seq66.  Not yet filled with functionality.
 */

void
qseditoptions::on_pushButtonTempoTrack_clicked()
{

}

/**
 *  Added for Seq66.  Not yet filled with functionality.
 */

void
qseditoptions::on_checkBoxRecordByChannel_clicked(bool /*checked*/)
{

}

/**
 *  Added for Seq66.  Not yet filled with functionality.
 */

void
qseditoptions::on_chkJackConditional_stateChanged(int /*arg1*/)
{

}

}           // namespace seq66

/*
 * qseditoptions.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

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
 * \updates       2020-07-20
 * \license       GNU GPLv2 or above
 *
 *      This version is located in Edit / Preferences.
 */

#include <QButtonGroup>

#include "cfg/settings.hpp"             /* seq66::usr().key_height(), etc.  */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "qclocklayout.hpp"
#include "qinputcheckbox.hpp"
#include "qseditoptions.hpp"

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

/**
 *  Button numbering for JACK Start Mode radio-buttons.
 */

enum radio_button_t
{
    radio_button_live,
    radio_button_song
};

/**
 *
 */

qseditoptions::qseditoptions (performer & p, QWidget * parent)
 :
    QDialog                 (parent),
    ui                      (new Ui::qseditoptions),
    m_perf                  (p),
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
    connect
    (
        ui->chkNoteResume, SIGNAL(stateChanged(int)),
        this, SLOT(update_note_resume())
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
        if (bgroup->id(button) == rbid)
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

    connect
    (
        ui->spinKeyHeight, SIGNAL(valueChanged(int)),
        this, SLOT(update_key_height())
    );
    connect
    (
        ui->lineEditUiScaling, SIGNAL(textEdited(const QString &)),
        this, SLOT(update_ui_scaling(const QString &))
    );

    connect
    (
        ui->checkBoxKeplerSeqedit, SIGNAL(stateChanged(int)),
        this, SLOT(update_pattern_editor())
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

    QVBoxLayout * vboxclocks = new QVBoxLayout;
    mastermidibus * masterbus = perf().master_bus();
    if (not_nullptr(masterbus))
    {
        int buses = masterbus->get_num_out_buses();
        for (int bus = 0; bus < buses; ++bus)
        {
            qclocklayout * tempqc = new qclocklayout(this, perf(), bus);
            vboxclocks->addLayout(tempqc->layout());
        }
    }

    QSpacerItem * spacer = new QSpacerItem
    (
        40, 20, QSizePolicy::Expanding, QSizePolicy::Expanding
    );

    vboxclocks->addItem(spacer);
    ui->groupBoxClocks->setLayout(vboxclocks);

    /*
     * Set up the MIDI Input tab.  It is simpler, just a list of check-boxes
     * in the groupBoxInputs widget.  No need for a separate class.  However,
     * note that our qinputcheckbox class controls the activation, in the GUI,
     * of each input port, based on its buss status.
     */

    QVBoxLayout * vboxinputs = new QVBoxLayout;
    if (not_nullptr(masterbus))
    {
        int buses = masterbus->get_num_in_buses();
        for (int bus = 0; bus < buses; ++bus)
        {
            qinputcheckbox * tempqi = new qinputcheckbox(this, perf(), bus);
            vboxinputs->addWidget(tempqi->input_checkbox());
        }
    }

    QSpacerItem * spacer2 = new QSpacerItem
    (
        40, 20, QSizePolicy::Expanding, QSizePolicy::Expanding
    );
    vboxinputs->addItem(spacer2);
    ui->groupBoxInputs->setLayout(vboxinputs);
    syncWithInternals();
    m_is_initialized = true;
}

/**
 *  A stock Qt GUI destructor.
 */

qseditoptions::~qseditoptions ()
{
    delete ui;
}

/**
 *
 */

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

/**
 *
 */

void
qseditoptions::update_jack_connect ()
{
    perf().set_jack_mode(true);             // perf().init_jack();
}

/**
 *
 */

void
qseditoptions::update_jack_disconnect ()
{
    perf().set_jack_mode(false);            // perf().deinit_jack();
}

/**
 *
 */

void
qseditoptions::update_master_cond ()
{
    rc().with_jack_master_cond(ui->chkJackConditional->isChecked());
    syncWithInternals();
}

/**
 *
 */

void
qseditoptions::update_time_master ()
{
    rc().with_jack_master(ui->chkJackMaster->isChecked());
    syncWithInternals();
}

/**
 *
 */

void
qseditoptions::update_transport_support ()
{
    rc().with_jack_transport(ui->chkJackTransport->isChecked());
    syncWithInternals();
}

/**
 *
 */

void
qseditoptions::update_jack_midi ()
{
    rc().with_jack_midi(ui->chkJackNative->isChecked());
    syncWithInternals();
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
    ui->spinKeyHeight->setValue(usr().key_height());

    char tmp[32];
    snprintf
    (
        tmp, sizeof tmp, "%gx%g",
        usr().window_scale(), usr().window_scale_y()
    );
    ui->lineEditUiScaling->setText(tmp);
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

/**
 *
 */

void
qseditoptions::update_ui_scaling (const QString & qs)
{
    const std::string valuetext = qs.toStdString();
    if (usr().parse_window_scale(valuetext))
        usr().save_user_config(true);
}

/**
 *  Seq66 has a new pattern editor GUI for Qt that is larger and more
 *  functional than the Kepler34 pattern editor.  However, it has the
 *  side-effect, currently, of making the main window larger.
 *
 *  This slot sets the new feature to off if the check-box is checked.
 */

void
qseditoptions::update_pattern_editor ()
{
    bool use_kepler_seqedit = ui->checkBoxKeplerSeqedit->isChecked();
    usr().use_new_seqedit(! use_kepler_seqedit);
    usr().save_user_config(true);
}

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

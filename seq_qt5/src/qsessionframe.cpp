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
 * \file          qsessionframe.cpp
 *
 *  This module declares/defines the base class for the main window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2020-08-24
 * \updates       2023-04-28
 * \license       GNU GPLv2 or above
 *
 */

#include <QKeyEvent>                    /* Needed for QKeyEvent::accept()   */

#include "seq66-config.h"               /* defines SEQ66_QMAKE_RULES        */
#include "os/daemonize.hpp"             /* seq66::signal_for_restart()      */
#include "play/performer.hpp"           /* seq66::performer                 */
#include "util/strfunctions.hpp"        /* seq66::int_to_string()           */
#include "qsessionframe.hpp"            /* seq66::qsessionframe, this class */
#include "qsmainwnd.hpp"                /* seq66::qsmainwnd                 */
#include "qt5_helpers.hpp"              /* seq66::qt(), qt_set_icon() etc.  */

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#if defined SEQ66_QMAKE_RULES
#include "forms/ui_qsessionframe.h"
#else
#include "forms/qsessionframe.ui.h"
#endif

/*
 * Don't document the namespace.
 */

namespace seq66
{

/**
 *  Limit for showing macro bytes in the combo-box.
 */

static const int c_macro_byte_max = 18;

/**
 *  Principle constructor.
 */

qsessionframe::qsessionframe
(
    performer & p,
    qsmainwnd * mainparent,
    QWidget * parent
) :
    QFrame          (parent),
    ui              (new Ui::qsessionframe),
    m_main_window   (mainparent),
    m_performer     (p)
{
    ui->setupUi(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->sessionManagerNameText->setEnabled(false);
    ui->sessionNameText->setEnabled(false);
    ui->sessionUrlText->setEnabled(false);
    ui->displayNameText->setEnabled(false);
    ui->clientIdText->setEnabled(false);
    ui->songPathText->setEnabled(false);
    session_log_file(usr().option_logfile());
    connect
    (
        ui->lineEditLogFile, SIGNAL(editingFinished()),
        this, SLOT(slot_log_file())
    );
    ui->pushButtonReload->setEnabled(false);
    connect
    (
        ui->pushButtonReload, SIGNAL(clicked(bool)),
        this, SLOT(slot_flag_reload())
    );
    populate_macro_combo();

    /*
     * New song-info edit control and the characters-remaining label..
     * Tricky, when getting the song info from the performer, it is already
     * in normal string format.
     *
     * std::string songinfo = midi_bytes_to_string(perf().song_info());
     */

    std::string songinfo = perf().song_info();
    size_t remainder = c_meta_text_limit - songinfo.size();
    std::string rem = int_to_string(int(remainder));
    ui->plainTextSongInfo->document()->setPlainText(qt(songinfo));
    ui->labelCharactersRemaining->setText(qt(rem));
    ui->spinBoxTrackInfo->hide();                   /* for the FUTURE   */
    connect
    (
        ui->plainTextSongInfo, SIGNAL(textChanged()),
        this, SLOT(slot_songinfo_change())
    );
    ui->pushButtonSaveInfo->setEnabled(false);
    connect
    (
        ui->pushButtonSaveInfo, SIGNAL(clicked(bool)),
        this, SLOT(slot_save_info())
    );
}

qsessionframe::~qsessionframe()
{
    delete ui;
}

void
qsessionframe::enable_reload_button (bool flag)
{
    ui->pushButtonReload->setEnabled(flag);
}

void
qsessionframe::slot_flag_reload ()
{
    signal_for_restart();           /* warnprint("Session reload request"); */
}

void
qsessionframe::slot_songinfo_change ()
{
    QString qtex = ui->plainTextSongInfo->toPlainText();
    std::string text = qtex.toStdString();
    size_t remainder = c_meta_text_limit - text.size();
    std::string rem = int_to_string(int(remainder));
    ui->labelCharactersRemaining->setText(qt(rem));
    ui->pushButtonSaveInfo->setEnabled(true);
}

void
qsessionframe::slot_save_info ()
{
    QString qtex = ui->plainTextSongInfo->toPlainText();
    std::string text = string_to_midi_bytes
    (
        qtex.toStdString(), c_meta_text_limit
    );
    perf().song_info(text);
    ui->pushButtonSaveInfo->setEnabled(false);
}

void
qsessionframe::populate_macro_combo ()
{
    tokenization t = perf().macro_names();
    bool macrosactive = perf().macros_active();
    if (macrosactive)
        macrosactive = ! t.empty();

    if (! t.empty())
    {
        int counter = 0;
        ui->macroComboBox->clear();
        for (const auto & name : t)
        {
            if (name.empty())
            {
                break;
            }
            else
            {
                midistring bytes = perf().macro_bytes(name);
                std::string bs = midi_bytes_string(bytes, c_macro_byte_max);
                std::string combined = name;
                combined += ": ";
                combined += bs;
                QString combotext(qt(combined));
                ui->macroComboBox->insertItem(counter++, combotext);
            }
        }
        connect
        (
            ui->macroComboBox, SIGNAL(currentTextChanged(const QString &)),
            this, SLOT(slot_macro_pick(const QString &))
        );
    }
    if (macrosactive)
    {
        ui->checkBoxMacrosActive->setChecked(true);
        connect
        (
            ui->checkBoxMacrosActive, SIGNAL(clicked(bool)),
            this, SLOT(slot_macros_active())
        );
    }
    else
    {
        ui->checkBoxMacrosActive->setChecked(false);
        ui->macroComboBox->setEnabled(false);
        if (t.empty())
            ui->checkBoxMacrosActive->setEnabled(false);
    }
}

void
qsessionframe::slot_macros_active()
{
    bool active = ui->checkBoxMacrosActive->isChecked();
    perf().macros_active(active);
    ui->macroComboBox->setEnabled(active);
    rc().auto_ctrl_save(true);
    ui->pushButtonReload->setEnabled(true);
}

void
qsessionframe::slot_macro_pick (const QString & name)
{
    if (! name.isEmpty())
    {
        std::string line = name.toStdString();
        size_t pos = line.find_first_of(":");
        line = line.substr(0, pos);
        perf().send_macro(line);
    }
}

void
qsessionframe::session_manager (const std::string & text)
{
    ui->sessionManagerNameText->setText(qt(text));
}

void
qsessionframe::session_path (const std::string & text)
{
    ui->sessionNameText->setText(qt(text));
}

void
qsessionframe::session_display_name (const std::string & text)
{
    ui->displayNameText->setText(qt(text));
}

void
qsessionframe::session_client_id (const std::string & text)
{
    ui->clientIdText->setText(qt(text));
}

void
qsessionframe::session_URL (const std::string & text)
{
    ui->sessionUrlText->setText(qt(text));
}

void
qsessionframe::session_log_file (const std::string & text)
{
    ui->lineEditLogFile->setText(qt(text));
}

void
qsessionframe::slot_log_file ()
{
    QString text = ui->lineEditLogFile->text();
    std::string temp = text.toStdString();
    if (temp != usr().option_logfile())
    {
        usr().option_logfile(temp);
        usr().option_use_logfile(! temp.empty());
        rc().auto_usr_save(true);
        usr().modify();
    }
}

void
qsessionframe::song_path (const std::string & text)
{
    ui->songPathText->setText(qt(text));
}

/*
 *  We must accept() the key-event, otherwise even key-events in the QLineEdit
 *  items are propagated to the parent, where they then get passed to the
 *  performer as if they were keyboards controls (such as a pattern-toggle
 *  hot-key).
 */

void
qsessionframe::keyPressEvent (QKeyEvent * event)
{
    event->accept();
}

void
qsessionframe::keyReleaseEvent (QKeyEvent * event)
{
    event->accept();
}

}               // namespace seq66

/*
 * qsessionframe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


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
 * \updates       2023-10-16
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
    QFrame                  (parent),
    ui                      (new Ui::qsessionframe),
    m_main_window           (mainparent),
    m_performer             (p),
    m_current_track         (0),
    m_current_text_number   (0),
    m_track_high            (p.sequence_high())
{
    ui->setupUi(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->sessionManagerNameText->setEnabled(false);
    ui->sessionNameText->setEnabled(false);
    ui->sessionUrlText->setEnabled(false);
    ui->displayNameText->setEnabled(false);
    ui->clientIdText->setEnabled(false);
    ui->songPathText->setEnabled(false);
    ui->lineEditLastUsedDir->setEnabled(false);
    ui->lineEditLastUsedDir->setText(qt(rc().last_used_dir()));
    session_log_file(usr().option_logfile());
    connect
    (
        ui->lineEditLogFile, SIGNAL(editingFinished()),
        this, SLOT(slot_log_file())
    );

    QIcon icon = QIcon::fromTheme("edit-clear", QIcon(":/images/icon"));
    ui->pushButtonLogFileClear->setIcon(icon);
    connect
    (
        ui->pushButtonLogFileClear, SIGNAL(clicked(bool)),
        this, SLOT(slot_log_file_clear())
     );
    ui->pushButtonReload->setEnabled(false);
    connect
    (
        ui->pushButtonReload, SIGNAL(clicked(bool)),
        this, SLOT(slot_flag_reload())
    );
    populate_macro_combo();
    reload_song_info();
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
    sync_track_label();

    /*
     * Track (pattern) spin-box.
     */

    ui->spinBoxTrackNumber->setEnabled(true);
    ui->spinBoxTrackNumber->setReadOnly(false);
    ui->spinBoxTrackNumber->setRange(0, m_track_high - 1);
    ui->spinBoxTrackNumber->setValue(0);
    connect
    (
        ui->spinBoxTrackNumber, SIGNAL(valueChanged(int)),
        this, SLOT(slot_track_number(int))
    );

#if defined ALLOW_TRACK_NUMBER_EDIT         // maybe later
    connect
    (
        ui->spinBoxTrackNumber, SIGNAL(textChanged(const QString &)),
        this, SLOT(slot_edit_track_number())
    );
#endif
}

qsessionframe::~qsessionframe()
{
    delete ui;
}

void
qsessionframe::sync_track_label ()
{
    std::string tlabel = "Song Info ";
    if (m_current_track > 0)
    {
        tlabel = "Track Info ";
        tlabel += std::to_string(int(m_current_track));
    }
    ui->labelTrackInfo->setText(qt(tlabel));
}

void
qsessionframe::sync_track_high ()
{
    int high = int(perf().sequence_high());
    if (m_track_high != high)
    {
        bool reduced = m_track_high < high;
        m_track_high = high;
        if (reduced)
        {
            m_current_track = high - 1;
            ui->spinBoxTrackNumber->setValue(high - 1);
            sync_track_label();
        }
        ui->spinBoxTrackNumber->setMaximum(high - 1);
    }
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

/**
 *  Also gets the characters remaining after translation to encoded
 *  MIDI bytes.  Too slow?
 */

void
qsessionframe::slot_songinfo_change ()
{
    QString qtex = ui->plainTextSongInfo->toPlainText();
    std::string text = string_to_midi_bytes(qtex.toStdString());
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
qsessionframe::slot_track_number (int trk)
{
    sync_track_high();                  /* adjust the maximum value         */
    if (trk != m_current_track)
    {
        if (trk == 0)
        {
            reload_song_info();         /* track zero is treated specially  */
        }
        else
        {
            bool nextmatch = false;
            seq66::event e = perf().get_track_info(trk, nextmatch);
            std::string trkinfo = e.get_text();
            if (trkinfo.empty())
            {
                ui->plainTextSongInfo->document()->setPlainText("*No text*");
                ui->pushButtonSaveInfo->setEnabled(false);
            }
            else
            {
                size_t remainder = c_meta_text_limit - trkinfo.size();
                std::string rem = int_to_string(int(remainder));
                midipulse ts = e.timestamp();
                std::string tstr = long_to_string(long(ts));
                ui->plainTextSongInfo->document()->setPlainText(qt(trkinfo));
                ui->labelCharactersRemaining->setText(qt(rem));
                ui->pushButtonSaveInfo->setEnabled(false);
            }
        }
    }
    m_current_track = trk;
    sync_track_label();
}

/*
 * New song-info edit control and the characters-remaining label..
 * Tricky, when getting the song info from the performer, it is already
 * in normal string format.
 *
 * std::string songinfo = midi_bytes_to_string(perf().song_info());
 */

void
qsessionframe::reload_song_info ()
{
    std::string songinfo = perf().song_info();
    size_t remainder = c_meta_text_limit - songinfo.size();
    std::string rem = int_to_string(int(remainder));
    ui->plainTextSongInfo->document()->setPlainText(qt(songinfo));
    ui->labelCharactersRemaining->setText(qt(rem));
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
        usr().option_logfile(temp);     /* sets usr().option_use_logfile()  */
        session_log_file(usr().option_logfile());
        rc().auto_usr_save(true);
        usr().modify();
        enable_reload_button(true);
        ui->pushButtonLogFileClear->setEnabled(! temp.empty());
    }
}

void
qsessionframe::slot_log_file_clear()
{
    ui->lineEditLogFile->clear();
    slot_log_file();
}

void
qsessionframe::song_path (const std::string & text)
{
    ui->songPathText->setText(qt(text));
}

void
qsessionframe::last_used_dir (const std::string & text)
{
    ui->lineEditLastUsedDir->setText(qt(text));
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


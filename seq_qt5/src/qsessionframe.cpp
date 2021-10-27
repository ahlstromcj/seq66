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
 * \updates       2021-10-27
 * \license       GNU GPLv2 or above
 *
 */

#include <QKeyEvent>                    /* Needed for QKeyEvent::accept()   */

#include "seq66-config.h"               /* defines SEQ66_QMAKE_RULES        */
#include "os/daemonize.hpp"             /* seq66::signal_for_restart()      */
#include "play/performer.hpp"           /* seq66::performer                 */
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

qsessionframe::qsessionframe
(
    performer & p,
    qsmainwnd * mainparent,
    QWidget * parent
) :
    QFrame                  (parent),
    ui                      (new Ui::qsessionframe),
    m_main_window           (mainparent),
    m_performer             (p)
{
    ui->setupUi(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->sessionManagerNameText->setEnabled(false);
    ui->sessionNameText->setEnabled(false);
    ui->sessionUrlText->setEnabled(false);
    ui->displayNameText->setEnabled(false);
    ui->clientIdText->setEnabled(false);
    ui->sessionLogText->setEnabled(false);
    ui->sessionLogText->setEnabled(false);
    ui->songPathText->setEnabled(false);
    ui->pushButtonReload->setEnabled(false);
    connect
    (
        ui->pushButtonReload, SIGNAL(clicked(bool)),
        this, SLOT(slot_flag_reload())
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
    signal_for_restart();
    warnprint("Session reload request");
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
qsessionframe::session_log (const std::string & text)
{
    ui->sessionLogText->setText(qt(text));
}

void
qsessionframe::session_log_append (const std::string & text)
{
    ui->sessionLogText->append("<br>");            // need a newline?
    ui->sessionLogText->append(qt(text));
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


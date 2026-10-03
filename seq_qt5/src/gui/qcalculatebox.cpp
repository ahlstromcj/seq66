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
 * \file          qcalculatebox.cpp
 *
 *  This module declares/defines the base class for the tiny command window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2026-10-02
 * \updates       2026-10-03
 * \license       GNU GPLv2 or above
 *
 *  The Command dialog provides a way to make a simple supported calculation
 *  and to present the result. It is meant as a quick-and-dirty tool
 *  to help in trouble-shooting. The implementation of calculations
 *  is provided in the calculations module.
 *
 *  Currently supported:
 *
 *      -   Conversions between variable-length values (VLV) and
 *          long integers.
 */

#include "midi/calculations.hpp"        /* seq66::vlv_to_long(), etc.       */
#include "qcalculatebox.hpp"            /* seq66::qcalculatebox             */
#include "qt5_helpers.hpp"              /* seq66::qt()                      */
#include "ui_qcalculatebox.h"

namespace seq66
{

qcalculatebox::qcalculatebox (QWidget * parent) :
    QDialog (parent),
    ui      (new Ui::qcalculatebox)
{
    ui->setupUi(this);
    connect
    (
        ui->line_edit_command, SIGNAL(editingFinished()),
        this, SLOT(slot_command_changed())
    );
    ui->line_edit_result->setReadOnly(true);    /* any use besides viewing? */
}

qcalculatebox::~qcalculatebox ()
{
    delete ui;
}

void
qcalculatebox::slot_command_changed ()
{
#if defined SEQ66_USE_QCALCULATEBOX

    QString qcmd { ui->line_edit_command->text() };
    std::string cmd { qcmd.toStdString() };
    std::string result { cmd_calculate(cmd) };
    ui->line_edit_result->setText(qt(result));

#endif  // defined SEQ66_USE_QCALCULATEBOX
}

}               // namespace seq66

/*
 * qcalculatebox.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

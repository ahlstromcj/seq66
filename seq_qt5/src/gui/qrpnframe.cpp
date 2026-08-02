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
 * \file          qrpnframe.cpp
 *
 *  This module declares/defines the base class for the LFO window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2026-07-30
 * \updates       2026-07-30
 * \license       GNU GPLv2 or above
 *
 *  The RPN dialog provides a way to enter RPN and NRPN controller events.
 *  It is easier than trying to add them in the event editor.
 *
 */

#include "qrpnframe.hpp"                /* seq66::qrpnframe                 */
#include "ui_qrpnframe.h"

namespace seq66
{

qrpnframe::qrpnframe
(
    QWidget * parent
) :
    QFrame(parent),
    ui(new Ui::qrpnframe)
{
    ui->setupUi(this);

    /*
     * Obsolete in Qt 6:
     */

#if defined QT_VERSION_5


    GroupBox rgroup { ui->group_box_rpn };
    connect
    (
        rgroup, SIGNAL(buttonClicked(int)),
        this, SLOT(slot_select_control(int))
    );

#elif defined QT_VERSION_6 || defined QT_VERSION_7

    auto lambdafunc = [this, rgroup] (QAbstractButton * abutton)
    {
        slot_select_control(rgroup->id(abutton));
    };
    connect(rgroup, &QButtonGroup::buttonClicked, lambdafunc);

#endif
}

qrpnframe::~qrpnframe()
{
    delete ui;
}

void
qrpnframe::slot_select_control (int x)
{
    printf("Selected %d\n");
}

}               // namespace seq66

/*
 * qrpnframe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

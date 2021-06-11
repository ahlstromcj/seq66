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
 * \file          qsbuildinfo.cpp
 *
 *  The time bar shows markers and numbers for the measures of the song,
 *  and also depicts the left and right markers.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-05-30
 * \updates       2019-09-01
 * \license       GNU GPLv2 or above
 *
 */

#include "cfg/cmdlineopts.hpp"          /* for build info function          */
#include "qsbuildinfo.hpp"

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#if defined SEQ66_QMAKE_RULES
#include "forms/ui_qsbuildinfo.h"
#else
#include "forms/qsbuildinfo.ui.h"
#endif

namespace seq66
{

/**
 *  Principal constructor.
 */

qsbuildinfo::qsbuildinfo (QWidget * parent)
 :
    QDialog (parent),
    ui      (new Ui::qsbuildinfo)
{
    ui->setupUi(this);

    QString name(SEQ66_PACKAGE_NAME);
    QString version(SEQ66_VERSION);
    QString comment("\n");
    comment += seq_build_details().c_str();

    ui->buildProgramLabel->setText(name);
    ui->buildVersionLabel->setText(version);
    ui->buildInfoTextEdit->setPlainText(comment);
}

qsbuildinfo::~qsbuildinfo ()
{
    delete ui;
}

}               // namespace seq66

/*
 * qsbuildinfo.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


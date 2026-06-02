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
 * \file          qsabout.cpp
 *
 *  The time bar shows markers and numbers for the measures of the song,
 *  and also depicts the left and right markers.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2026-06-02
 * \license       GNU GPLv2 or above
 *
 */

#include "seq66_features.hpp"           /* version information functions    */
#include "cfg/settings.hpp"             /* usr().dark_mode()                */
#include "qsabout.hpp"                  /* your basic developer spoor       */
#include "qt5_helpers.hpp"              /* seq66::qt(), qt_set_icon() etc.  */
#include "ui_qsabout.h"

namespace seq66
{

/**
 *  Principal constructor. Now sets the links to be opened by clicking on
 *  them.
 *
 *  <a href="https://github.com/ahlstromcj/seq66">
 *      <span style=" text-decoration: underline; color:#0000ff;">
 *      GitHub/ahlstromcj/seq66</span></a>
 */

qsabout::qsabout (QWidget * parent) : QDialog (parent), ui (new Ui::qsabout)
{
    ui->setupUi(this);
    std::string apptag { seq_app_name() + " " + seq_version() };
    std::string vertag { seq_version_text() };
    ui->topLabel->setText(qt(apptag));
    ui->versionLabel->setText(qt(vertag));

    if (usr().dark_theme())
    {
        (void) qlabel_change_color(ui->label_github_seq66, "#00ffff;");
        (void) qlabel_change_color(ui->label_filter24_seq24, "#00ffff;");
        (void) qlabel_change_color(ui->label_github_seq32, "#00ffff;");
        (void) qlabel_change_color(ui->label_github_kepler34, "#00ffff;");
        (void) qlabel_change_color(ui->label_gmail_ahlstromcj, "#00ffff;");
    }
    ui->label_github_seq66->setOpenExternalLinks(true);
    ui->label_filter24_seq24->setOpenExternalLinks(true);
    ui->label_github_seq32->setOpenExternalLinks(true);
    ui->label_github_kepler34->setOpenExternalLinks(true);
    ui->label_gmail_ahlstromcj->setOpenExternalLinks(true);
    connect(ui->OkButton, &QPushButton::clicked, this, &QDialog::accept);
}

qsabout::~qsabout()
{
    delete ui;
}

}               // namespace seq66

/*
 * qsabout.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

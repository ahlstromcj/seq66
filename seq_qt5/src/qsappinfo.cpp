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
 * \file          qsappinfo.cpp
 *
 *  The time bar shows markers and numbers for the measures of the song,
 *  and also depicts the left and right markers.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2023-08-21
 * \updates       2025-01-25
 * \license       GNU GPLv2 or above
 *
 *  This module supports a task similar to that of the Help / Tutorial menu
 *  entry. The qsappinfo dialog is similar to the qsbuildinfo dialog. There
 *  are a couple of differences:
 *
 *      -   The dialog uses a rich-text browser widget, QTextBrowser.
 *      -   The data to be shown will reside in small files stored in
 *          the build directory seq66/data/share/doc/info, and installed to
 *          -   /usr/local/share/doc/seq66-0.99/info
 *          -   C:/Program Files/Seq66/data/share/doc/info
 *
 *  The plan is to generalize the tutorial_folder_list() function to more
 *  flexibly select the document folder, and then write a function
 *  to read the desired file into a string.
 *
 */

#include "cfg/settings.hpp"             /* seq66::open_share_doc_file()     */
#include "qsappinfo.hpp"                /* seq66::qsappinfo dialog class    */
#include "qt5_helpers.hpp"              /* seq66::qt() string conversion    */

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#if defined SEQ66_QMAKE_RULES
#include "forms/ui_qsappinfo.h"
#else
#include "forms/qsappinfo.ui.h"
#endif

namespace seq66
{

static const std::string s_error_html
{
"<html>"
"<head>"
"<meta name=\"qrichtext\" content=\"1\" />"
"<meta charset=\"utf-8\" />"
"</head>"
"<body>"
"<i>Could not find the HTML file in installation directories</i>"
"</body>"
"</html>"
};

/**
 *  Principal constructor.
 */

qsappinfo::qsappinfo (QWidget * parent) :
    QDialog (parent),
    ui      (new Ui::qsappinfo)
{
    ui->setupUi(this);
    ui->buttonReserved_1->hide();
    ui->buttonReserved_2->hide();
    slot_common_keys();
    connect
    (
        ui->buttonCommonKeys, SIGNAL(clicked(bool)),
        this, SLOT(slot_common_keys())
    );
    connect
    (
        ui->buttonAutomation, SIGNAL(clicked(bool)),
        this, SLOT(slot_automation_keys())
    );
    connect
    (
        ui->buttonSeqroll, SIGNAL(clicked(bool)),
        this, SLOT(slot_seqroll_keys())
    );
    connect
    (
        ui->buttonPerfroll, SIGNAL(clicked(bool)),
        this, SLOT(slot_songroll_keys())
    );
    connect
    (
        ui->buttonHotKeys, SIGNAL(clicked(bool)),
        this, SLOT(slot_hot_keys())
    );
    connect
    (
        ui->buttonMutesKeys, SIGNAL(clicked(bool)),
        this, SLOT(slot_mutes_keys())
    );
}

qsappinfo::~qsappinfo ()
{
    delete ui;
}

void
qsappinfo::open_html
(
    const std::string & basename,
    const std::string & comment
)
{
    std::string filename = basename + ".html";
    std::string html = open_share_doc_file(filename, "info");
    if (html.empty())
        html = s_error_html;

    ui->textBrowser->setHtml(qt(html));
    ui->appPanelLabel->setText(qt(comment));
}

void
qsappinfo::slot_common_keys ()
{
    open_html("common_keys", "Common Piano Roll Keys");
}

void
qsappinfo::slot_automation_keys ()
{
    open_html("automation_keys", "Automation Defaults");
}

void
qsappinfo::slot_seqroll_keys ()
{
    open_html("seqroll_keys", "Pattern Editor Keys");
}

void
qsappinfo::slot_songroll_keys ()
{
    open_html("songroll_keys", "Song Editor Keys");
}

void
qsappinfo::slot_hot_keys ()
{
    open_html("pattern_hotkeys", "Grid Hot Keys");
}

void
qsappinfo::slot_mutes_keys ()
{
    open_html("mute_group_keys", "Grid Mute-Group Keys");
}

}               // namespace seq66

/*
 * qsappinfo.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


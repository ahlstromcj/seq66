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
 * \file          qslogview.cpp
 *
 *  The time bar shows markers and numbers for the measures of the song,
 *  and also depicts the left and right markers.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2025-01-25
 * \updates       2025-01-25
 * \license       GNU GPLv2 or above
 *
 *  This module supports a task similar to that of the Help / Tutorial menu
 *  entry. The qslogview dialog is similar to the qsbuildinfo dialog. There
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

#include <QScrollBar>

#include "cfg/settings.hpp"             /* seq66::open_share_doc_file()     */
#include "util/filefunctions.hpp"       /* seq66::file_read_string()        */
#include "qslogview.hpp"                /* seq66::qslogview dialog class    */
#include "qt5_helpers.hpp"              /* seq66::qt() string conversion    */

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#if defined SEQ66_QMAKE_RULES
#include "forms/ui_qslogview.h"
#else
#include "forms/qslogview.ui.h"
#endif

namespace seq66
{

static const std::string s_error_text
{
    "No active log. Set it up in main Session tab."
};
static const std::string s_empty_text
{
    "No log entries yet. Strange."
};

/**
 *  Principal constructor.
 */

qslogview::qslogview (QWidget * parent) :
    QDialog (parent),
    ui      (new Ui::qslogview)
{
    ui->setupUi(this);
    connect
    (
        ui->buttonRefresh, SIGNAL(clicked(bool)),
        this, SLOT(slot_refresh_log_view())
    );
}

qslogview::~qslogview ()
{
    delete ui;
}

void
qslogview::refresh ()
{
    std::string text{s_error_text};
    if (usr().option_use_logfile())
    {
        std::string logfilename = usr().option_logfile();
        text = file_read_string(logfilename);
        if (text.empty())
            text = s_empty_text;
    }
    ui->textBrowser->setText(qt(text));

    QScrollBar * sb = ui->textBrowser->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void
qslogview::slot_refresh_log_view ()
{
    refresh();
}

}               // namespace seq66

/*
 * qslogview.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


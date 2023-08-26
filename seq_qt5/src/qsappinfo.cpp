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
 * \updates       2023-08-21
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

/**
 * Currently just test code.
 */

const std::string s_main_html
{
"<html>"
"<head>"
"<meta name=\"qrichtext\" content=\"1\" />"
"<meta charset=\"utf-8\" />"
"<style type=\"text/css\">"
"p, li { white-space: pre-wrap; }"
"hr { height: 1px; border-width: 0; }"
"li.unchecked::marker { content: \"\2610\"; }"
"li.checked::marker { content: \"\2612\"; }"
"</style>"
"</head>"
"<body>"
"   <p> <b> Common Keystrokes</b> </p>"
"   <p>"
"      Each piano roll panel has keystrokes separate from the automation keystrokes, and thus not configurable. The arrow keys and some other characters provide functions that are common to the pattern edit and song \"piano rolls\"."
"   <br>"
"   <table>"
"      <tr>"
"         <td> Standard movement keys</td>"
"         <td> Arrow-left, -right, -up, -down</td>"
"      </tr>"
"      <tr>"
"         <td> vi-like movement keys</td>"
"         <td> h, j, k, l</rd>"
"      </tr>"
"      <tr>"
"         <td> Standard large movement keys</td>"
"         <td> Page-Up, Page-Down, and Ctrl-Home, Ctrl-End</rd>"
"      </tr>"
"      <tr>"
"         <td> Horizontal zoom keys</td>"
"         <td> Zoom out 'z', zoom in 'Z', reset '0'</rd>"
"      </tr>"
"      <tr>"
"         <td> Vertical zoom keys</td>"
"         <td> Zoom out 'v', zoom in 'V', reset '0'</rd>"
"      </tr>"
"   </table>"
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
    ui->textBrowser->setHtml(qt(s_main_html));
}

qsappinfo::~qsappinfo ()
{
    delete ui;
}

}               // namespace seq66

/*
 * qsappinfo.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


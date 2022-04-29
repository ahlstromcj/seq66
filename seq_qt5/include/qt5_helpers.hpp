#if ! defined SEQ66_QT5_HELPERS_HPP
#define SEQ66_QT5_HELPERS_HPP

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
 * \file          qt5_helpers.hpp
 *
 *  This module declares/defines some helpful free functions to support Qt and
 *  C++ integration.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-03-14
 * \updates       2022-04-28
 * \license       GNU GPLv2 or above
 *
 */

#include "cfg/settings.hpp"             /* seq66::rc().home_config_dir...() */
#include "ctrl/keymap.hpp"              /* seq66::qt_modkey_ordinal()       */
#include "ctrl/keystroke.hpp"           /* seq66::keystroke wrapper class   */

class QAction;
class QComboBox;
class QIcon;
class QKeyEvent;
class QPushButton;
class QTimer;
class QWidget;

/*
 * Don't document the namespace.
 */

namespace seq66
{

/*
 *  Free constants in the seq66 namespace.  These values are simply visible
 *  booleans for using file dialogs.
 */

const bool SavingFile = true;
const bool OpeningFile = false;
const bool ConfigFile = true;
const bool NormalFile = false;

/*
 * Free functions in the seq66 namespace.
 */

extern void qt_set_icon (const char * pixmap_array [], QPushButton * button);
extern std::string qt_icon_theme ();
extern bool qt_prompt_ok
(
    const std::string & text,
    const std::string & info
);
extern keystroke qt_keystroke
(
    QKeyEvent * event,
    keystroke::action rp,
    bool testing = false
);
extern QString qt (const std::string & text);
extern QTimer * qt_timer
(
    QObject * self,
    const std::string & name,
    int redraw_factor,
    const char * slotname
);
extern void enable_combobox_item (QComboBox * box, int index, bool enabled);
extern bool fill_combobox
(
    QComboBox * box,
    const combolist & clist,
    std::string value           = "",
    const std::string & prefix  = "",
    const std::string & suffix  = ""
);
extern QAction * create_menu_action
(
    const std::string & text,
    const QIcon & micon
);
extern bool show_open_midi_file_dialog (QWidget * parent, std::string & file);
extern bool show_import_midi_file_dialog (QWidget * parent, std::string & file);
extern bool show_import_project_dialog
(
    QWidget * parent,
    std::string & selecteddir,
    std::string & selectedfile
);
extern bool show_playlist_dialog
(
    QWidget * parent,
    std::string & file,
    bool saving
);
extern bool show_text_file_dialog (QWidget * parent, std::string & file);
extern bool show_file_dialog
(
    QWidget * parent,
    std::string & selectedfile,
    const std::string & prompt = "",
    const std::string & filterlist = "",
    bool saving = false,
    bool forceconfig = false,
    const std::string & extension = ""
);

}               // namespace seq66

#endif          // SEQ66_QT5_HELPERS_HPP

/*
 * qt5_helpers.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


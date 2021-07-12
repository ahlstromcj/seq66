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
 * \file          qt5_helpers.cpp
 *
 *  This module declares/defines some helpful macros or functions.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-03-14
 * \updates       2021-05-04
 * \license       GNU GPLv2 or above
 *
 */

#include <QFileDialog>                  /* prompt for full MIDI file's path */
#include <QKeyEvent>
#include <QPushButton>

#include "cfg/settings.hpp"             /* seq66::rc().home_config_dir...() */
#include "util/filefunctions.hpp"       /* seq66 file-name manipulations    */
#include "qt5_helpers.hpp"

/*
 * Don't document the namespace.
 */

namespace seq66
{

/**
 *  This code is used in qslivekeys in order to get the list(s) of extended
 *  characters in the keymap module.
 */

static keystroke
qt_keystroke_test (QKeyEvent * event, keystroke::action act)
{
    unsigned kmods = static_cast<unsigned>(event->modifiers());
    eventkey k = event->key();
    ctrlkey ordinal = qt_modkey_ordinal(k, kmods);
    bool press = act == keystroke::action::press;
    keystroke t = keystroke(ordinal, press);
    std::string ktext = event->text().toStdString();
    std::string kname = t.name();
    std::string modifiers = modifier_names(kmods);
    unsigned scode = unsigned(event->nativeScanCode());     /* scan code    */
    unsigned kcode = unsigned(event->nativeVirtualKey());   /* key sym      */
    if (ktext.empty())
        ktext = kname;

    printf
    (
        "Event key #0x%02x mod %s '%s' %s: scan 0x%x key 0x%x ord 0x%x\n",
        k, modifiers.c_str(), ktext.c_str(), press ? "press" : "release",
        scode, kcode, ordinal
    );
    return keystroke(0, press);                 /* disable the key action   */
}

/**
 *  Given a keystroke from a Qt 5 GUI, this function returns an "ordinal"
 *  version of the keystroke.  Note that there are many keystrokes that can
 *  have the same event key value.  For example: Ctrl-a, Shift-a, and a.  In
 *  cases like that, we have to check the modifiers.
 *
 *  But the QKeyEvent::modifiers() function cannot always be trusted. The user
 *  can confuse it by pressing both Shift keys simultaneously and releasing
 *  one of them, for example.
 *
 *  We can also check the nativeVirtualKey() result for the event.  Even that
 *  can be fooled by a change in the keyboard encoding.  Yeesh!
 *
 *  The qt_modkey_ordinal() function in the keymap module can use all these
 *  codes to try to figure out the proper ordinal to return.
 *
 * \param event
 *      The putative Qt 5 keystroke event.
 *
 * \param act
 *      Indicates if the keystroke action was a press or a release.
 *
 * \param testing
 *      If true (the default is false), then the lookup results are shown and
 *      the true keystroke is returned to be acted on.  We have a feeling
 *      we'll be looking at a more international key-maps in the future.
 *      To avoid problematic actions getting in the way of the test, call
 *      qt_keystroke_test() directly, as done in the qslivegrid class when
 *      SEQ66_KEY_TESTING is defined.
 *
 * \return
 *      Returns an object that makes the key event easier to use.  It needs to
 *      hold only the ordinal and whether the key was pressed or released.
 */

keystroke
qt_keystroke (QKeyEvent * event, keystroke::action act, bool testing)
{
    if (testing)
        (void) qt_keystroke_test(event, act);           /* show key details */

    unsigned kmods = static_cast<unsigned>(event->modifiers());
    eventkey k = event->key();
    eventkey v = event->nativeVirtualKey();
    ctrlkey ordinal = qt_modkey_ordinal(k, kmods, v);
    bool press = act == keystroke::action::press;
    return keystroke(ordinal, press, kmods);
}

/**
 *  Clears the text of the QPushButton, and sets its icon to the pixmap given
 *  by the pixmap character array.
 *
 * \param pixmap_array
 *      Provides the character array representing the XPM pixmap.
 *
 * \param button
 *      Provides a pointer to the button to be cleared and to have its icon
 *      set.
 */

void
qt_set_icon (const char * pixmap_array [], QPushButton * button)
{
    QPixmap pixmap(pixmap_array);
    QIcon icon;
    icon.addPixmap(pixmap, QIcon::Normal, QIcon::On);
    button->setText("");
    button->setIcon(icon);
}

/**
 *  Shows the "Open" file dialog.
 *
 * \param [inout] selectedfile
 *      A return value for the chosen file and path.  If not-empty when the
 *      call is made, show the user that directory instead of the last-used
 *      directory.
 *
 * \return
 *      Returns true if the returned path can be used.
 */

bool
show_open_midi_file_dialog (QWidget * parent, std::string & selectedfile)
{
    return show_file_dialog
    (
        parent, selectedfile, "Open MIDI/WRK file",
        "MIDI/WRK (*.midi *.mid *.MID *.wrk *.WRK);;"
        "MIDI (*.midi *.mid *.MID);;WRK (*.wrk *.WRK);;All (*)",
        OpeningFile, NormalFile
    );
}

/**
 *  Shows the "Open" play-list dialog.
 *
 *  Was starting from the rc().last_used_dir(), but should be the home
 *  directory for both normal and NSM sessions.
 *
 * \param [inout] selectedfile
 *      A return value for the chosen file and path.  If not-empty when the
 *      call is made, show the user that directory instead of the home
 *      configuration directory.  Callers should just provide the base-name of
 *      the file when saving a file under session management!  For opening a
 *      file, not a big deal.
 *
 * \return
 *      Returns true if the returned path can be used.
 */

bool
show_playlist_dialog (QWidget * parent, std::string & selectedfile, bool saving)
{
    std::string filter = "Playlist (*.playlist);;All files (*)";
    std::string caption = saving ?
        "Save play-lists file" : "Open play-lists file";

    return show_file_dialog
    (
        parent, selectedfile, caption, filter, saving, ConfigFile, ".playlist"
    );
}

bool
show_text_file_dialog (QWidget * parent, std::string & selectedfile)
{
    return show_file_dialog
    (
        parent, selectedfile, "Save text file",
        "Text (*.txt *.text);;All (*)", SavingFile, NormalFile, ".text"
    );
}

/**
 *  Meant to handle many more situations.
 */

bool
show_file_dialog
(
    QWidget * parent,
    std::string & selectedfile,
    const std::string & prompt,
    const std::string & filterlist,
    bool saving,
    bool forceconfig,
    const std::string & extension
)
{
    bool result = false;
    std::string d = forceconfig ? rc().home_config_directory() : selectedfile ;
    if (selectedfile.empty())
    {
        /* nothing to do (yet) */
    }
    else
    {
        if (name_has_path(selectedfile) && forceconfig)
        {
            if (file_is_directory(selectedfile))
            {
                /* Keep the home configuration directory */
            }
            else
            {
                /*
                 *  Pull out the file-name and prepend the home configuration
                 *  directory.
                 */

                std::string fullpath = selectedfile;
                std::string path;
                std::string basename;
                (void) filename_split(fullpath, path, basename);
                d = filename_concatenate(d, basename);
            }
        }
    }

    std::string p = prompt;
    if (p.empty())
        p = saving ? "Save file" : "Open file" ;

    std::string f = filterlist;
    if (f.empty())
        f = "All files (*)";

    QString folder = QString::fromStdString(d);
    QString caption = QString::fromStdString(p);
    QString filters = QString::fromStdString(f);
    QString file = saving ?
        QFileDialog::getSaveFileName(parent, caption, folder, filters) :
        QFileDialog::getOpenFileName(parent, caption, folder, filters) ;

    result = ! file.isEmpty();
    if (result)
    {
        selectedfile = file.toStdString();
        if (saving && ! extension.empty())
            selectedfile = file_extension_set(selectedfile, extension);

        file_message(saving ? "Saving" : "Opening", selectedfile);
    }
    return result;
}

}               // namespace seq66

/*
 * qt5_helpers.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


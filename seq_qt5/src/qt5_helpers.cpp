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
 * \updates       2023-08-12
 * \license       GNU GPLv2 or above
 *
 *  The items provided externally are:
 *
 *      -   qt_keystroke(). Returns an "ordinal" for Qt keystroke.
 *      -   qt_set_icon(). Sets an icon for a push-button.
 *      -   qt_icon_theme(). Returns the name of the icon theme.
 *      -   qt_prompt_ok(). Does an OK/Cancel QMessageBox.
 *      -   qt().  Converts an std::sring to a QString.
 *      -   qt_timer(). Encapsulates creating and starting a timer, with a
 *          callback given by a Qt slot-name.
 *      -   enable_combobox_item(). Handles the appearance of a combo box.
 *      -   fill_combobox(). Fills a combo box from a combolist.
 *      -   create_menu_action(). Creates a menu action from text and an icon.
 *      -   show_open_midi_file_dialog()
 *      -   show_import_midi_file_dialog()
 *      -   show_import_project_dialog()
 *      -   show_playlist_dialog()
 *      -   show_text_file_dialog()
 *      -   show_file_dialog()
 */

#include <QAction>
#include <QComboBox>
#include <QErrorMessage>
#include <QFileDialog>                  /* prompt for full MIDI file's path */
#include <QIcon>
#include <QKeyEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QTimer>

#include "cfg/settings.hpp"             /* seq66::rc().home_config_dir...() */
#include "util/filefunctions.hpp"       /* seq66 file-name manipulations    */
#include "qt5_helpers.hpp"              /* these cool helper functions!     */

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

    char tmp[128];
    snprintf
    (
        tmp, sizeof tmp,
        "Event key #0x%02x mod %s '%s' %s: scan 0x%x key 0x%x ord 0x%x",
        k, modifiers.c_str(), ktext.c_str(), press ? "press" : "release",
        scode, kcode, ordinal
    );
    (void) info_message(tmp);
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
 *  can be fooled by a change in the keyboard encoding.  Yeesh! And on
 *  Windows, the native value is different from that of Linux for the arrow
 *  keys.
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
#if defined SEQ66_PLATFORM_WINDOWS
    ctrlkey ordinal = qt_modkey_ordinal(k, kmods, 0);
#else
    eventkey v = event->nativeVirtualKey();
    ctrlkey ordinal = qt_modkey_ordinal(k, kmods, v);
#endif
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
 *  Gets the icon theme name.
 */

std::string
qt_icon_theme ()
{
    QString qs = QIcon::themeName();
    return qs.toStdString();
}

/**
 *  Encapsulates a common conversion from C++ string to QString.
 */

QString
qt (const std::string & text)
{
    return QString::fromStdString(text);
}

/**
 *  Provide an OK/Cancel prompt and return the result, true if Ok.
 */

bool
qt_prompt_ok
(
    const std::string & text,
    const std::string & info
)
{
    QMessageBox temp;
    temp.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    temp.setText(qt(text));
    temp.setInformativeText(qt(info));

    int choice = temp.exec();
    bool result = choice == QMessageBox::Ok;
    return result;
}

/**
 *  Provides an informative message box, no decision necessary.
 *
 *  Do we need to delete the message box here, or risk them accumulating?`
 */

void
qt_info_box (QWidget * self, const std::string & msg)
{
    QMessageBox * mbox = new QMessageBox(self);
    if (not_nullptr(mbox))
    {
        mbox->setText(qt(msg));
        mbox->setStandardButtons(QMessageBox::Ok);
        (void) mbox->exec();
        delete mbox;
    }
}

/**
 *  Provides an informative error message box, no decision necessary.
 *  Qt::WA_DeleteOnClose?
 */

void
qt_error_box (QWidget * self, const std::string & msg)
{
    QErrorMessage * mbox = new QErrorMessage(self);
    if (not_nullptr(mbox))
    {
        mbox->showMessage(qt(msg));
        (void) mbox->exec();
        delete mbox;
    }
}

/**
 *  Creates a QTimer in a consistent manner.
 */

QTimer *
qt_timer
(
    QObject * self,
    const std::string & name,
    int redraw_factor,
    const char * slotname
)
{
    QTimer * result = new QTimer(self);
    if (not_nullptr(result))
    {
        int interval = redraw_factor * usr().window_redraw_rate();

#if defined SHOW_TIMER_CREATION         /* this has been well vetted, quiet */
        if (rc().investigate())
        {
            std::string msg = "Timer '";
            msg += name;
            msg += "' created at rate ";
            msg += std::to_string(interval);
            msg += " for slot ";
            msg += slotname;
            (void) debug_message(msg);
        }
#endif

        result->setInterval(interval);
        QMetaObject::Connection c =
            QObject::connect(result, SIGNAL(timeout()), self, slotname);

        if (bool(c))
            result->start();
        else
            error_message("Connection invalid");
    }
    else
    {
        std::string msg = "Could not create timer '";
        msg += name;
        msg += "'";
        error_message(msg);
    }
    return result;
}

/**
 *  Helper for handling enabled/disabled items in a combo-box
 */

void
enable_combobox_item (QComboBox * box, int index, bool enabled)
{
    QStandardItemModel * m = qobject_cast<QStandardItemModel *>(box->model());
    QStandardItem * item = m->item(index);
    if (not_nullptr(item))
    {
        if (enabled)
            item->setFlags(item->flags() | Qt::ItemIsEnabled);
        else
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    }
    else
    {
        std::string data = "index ";
        data += std::to_string(index);
        error_message("null combobox item", data);
    }
}

/**
 *  Helper to fill a combo-box.  Also sets the current index, and can bracket
 *  each line-item with optional prefix and suffix text.
 */

bool
fill_combobox
(
    QComboBox * box,
    const combolist & clist,
    std::string value,
    const std::string & prefix,
    const std::string & suffix
)
{
    bool result = false;
    int count = clist.count();
    if (count > 0)
    {
        box->clear();
        for (int i = 0; i < count; ++i)
        {
            std::string item = clist.at(i);
            if (! item.empty())
            {
                bool nopadding = clist.use_current() && i == 0;
                bool addpadding = (item != "-") && ! nopadding;
                if (addpadding)
                {
                    if (! prefix.empty())   /* for example, "1/" for widths */
                        item = prefix + item;

                    if (! suffix.empty())
                        item = item + suffix;
                }
                result = true;
                if (item == "-")
                {
                    box->insertSeparator(8);
                }
                else
                {
                    QString text = qt(item);
                    box->insertItem(i, text);
                }
            }
        }
        if (result)
        {
            if (! value.empty())
            {
                std::string fullvalue = prefix;
                fullvalue += value;
                fullvalue += suffix;
                clist.current(fullvalue);
                box->setCurrentText(qt(fullvalue));
            }
            else
                box->setCurrentIndex(0);
        }
    }
    return result;
}

/**
 *  Sets the value in an integer spin-box without causing it to trigger
 *  an event.
 */

void
set_spin_value (QSpinBox * spin, int value)
{
    spin->blockSignals(true);
    spin->setValue(value);
    spin->blockSignals(false);
}

/**
 *  Handles versioning issue in creating a menu action.
 */

QAction *
create_menu_action
(
    const std::string & text,
    const QIcon & micon
)
{
    QString mlabel(qt(text));
#if QT_VERSION < QT_VERSION_CHECK(5, 8, 0)
    QAction * result = new QAction(micon, mlabel, nullptr);
#else
    QAction * result = new QAction(micon, mlabel);
#endif
    return result;
}

/**
 *  To be called by the monitoring object.
 *
 *      QWidget * monitor = parentWidget(); // ui->dataScrollArea
 *      m_scrollArea->viewport()->installEventFilter(monitor);
 *      m_scrollArea->horizontalScrollBar()->installEventFilter(monitor);
 *      if event->type() == QEvent::ShortcutOverride &&
 *          obj == myparent->verticalScrollBar()
 *
 *          QKeyEvent keyEvent = static_cast<QKeyEvent>(event);
 *          if(obj == keyEvent->key() == Qt::Key_Up)
 *          {
 *              m_scrollArea->verticalScrollBar()->setEnabled(false);
 *              std::cout<<" Up "<<std::endl;
 *          }
 */

bool
install_scroll_filter (QWidget * monitor, QScrollArea * target)
{
    bool result = not_nullptr(monitor);
    if (result)
    {
        target->viewport()->installEventFilter(monitor);
        target->horizontalScrollBar()->installEventFilter(monitor);
        target->verticalScrollBar()->installEventFilter(monitor);
    }
    return result;
}

/**
 *  For internal usage.
 */

static const char *
midi_wrk_wildcards ()
{
    static const char * const s_wildcards =
        "MIDI/WRK (*.midi *.mid *.MID *.wrk *.WRK);;"
        "MIDI (*.midi *.mid *.MID);;WRK (*.wrk *.WRK);;All (*)"
        ;
    return s_wildcards;
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
        parent, selectedfile, "Open MIDI/WRK file", midi_wrk_wildcards(),
        OpeningFile, NormalFile
    );
}

bool
show_import_midi_file_dialog (QWidget * parent, std::string & selectedfile)
{
    std::string caption = "Import MIDI File into Current Set";
    return show_file_dialog
    (
        parent, selectedfile, caption, midi_wrk_wildcards(),
        OpeningFile, NormalFile
    );
}

bool
show_import_project_dialog
(
    QWidget * parent,
    std::string & selecteddir,
    std::string & selectedfile
)
{
    std::string filter = "Config (*.rc);;All files(*)";
    std::string caption = "Import Project Configuration";
    std::string selection;
    bool result = show_file_dialog
    (
        parent, selection, caption, filter, OpeningFile, ConfigFile
    );
    if (result)
    {
        std::string dir;
        std::string file;
        result = filename_split(selection, dir, file);
        if (result)
        {
            selecteddir = dir;
            selectedfile = file;
        }
    }
    if (! result)
    {
        selecteddir.clear();
        selectedfile.clear();
    }
    return result;
}

/**
 *  Shows the "Open" play-list dialog.
 *
 * \param parent
 *      Provides the parent widget of this dialog.
 *
 * \param [inout] selectedfile
 *      A return value for the chosen file and path.  If not-empty when the
 *      call is made, show the user that directory instead of the home
 *      configuration directory.  Callers should just provide the base-name of
 *      the file when saving a file under session management!  For opening a
 *      file, not a big deal.
 *
 * \param saving
 *      Indicates saving versus opening the file. The two values are:
 *
 *          -   SavingFile = true
 *          -   OpeningFile = false
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
 *
 * QString QFileDialog::getSaveFileName
 * (
 *     QWidget * parent = nullptr,
 *     const QString & caption = QString(),
 *     const QString & dir = QString(),
 *     const QString & filter = QString(),
 *     QString * selectedFilter = nullptr,
 *     QFileDialog::Options options = Options()
 * )
 *
 * Useful QFileDialog::Options:
 *
 * QFileDialog::ShowDirsOnly
 * QFileDialog::DontConfirmOverwrite
 *
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
    const std::string & extension,
    bool promptoverwrite
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

    QString file;
    QString folder = qt(d);
    QString caption = qt(p);
    QString filters = qt(f);
    QString * selfilter = nullptr;
    QFileDialog::Options options;   /* what's the default? Options(); */
    if (saving)
    {
        if (! promptoverwrite)
            options = QFileDialog::DontConfirmOverwrite;

        file = QFileDialog::getSaveFileName
        (
            parent, caption, folder, filters, selfilter, options
        );
    }
    else
    {
        file = QFileDialog::getOpenFileName
        (
            parent, caption, folder, filters, selfilter, options
        );
    }

    result = ! file.isEmpty();
    if (result)
    {
        selectedfile = file.toStdString();

        std::string ext = file_extension(selectedfile);
        if (saving && ext.empty() && ! extension.empty())
            selectedfile = file_extension_set(selectedfile, extension);

        file_message(saving ? "Saving" : "Opening", selectedfile);
    }
    return result;
}

/**
 *  This dialog is meant to allow the user to select a directory instead
 *  of typing it.
 */

bool
show_folder_dialog
(
    QWidget * parent,
    std::string & selectedfolder,
    const std::string & prompt,
    bool forcehome
)
{
    bool result = false;
    std::string d = forcehome ? rc().home_config_directory() : selectedfolder ;
    if (d.empty())
    {
        /* nothing extra to do (yet) */
    }
    else
    {
        bool badpath = ! name_has_path(d);
        if (! badpath)
            badpath = ! file_is_directory(d);

        if (badpath)
            d.clear();
    }

    QString qprompt = qt(prompt);
    QString folder = qt(d);
    folder = QFileDialog::getExistingDirectory(parent, qprompt, folder);
    result = ! folder.isEmpty();
    if (result)
    {
        selectedfolder = folder.toStdString();
        file_message("Choosing", selectedfolder);
    }
    return result;
}

/**
 *  For issue #114, we want to append the keystroke as configured to the
 *  tool-tip.
 *
 * \param widget
 *      The QWidget whose tool-tip we want to modify.
 *
 * \param keyname
 *      The name of the key as provided by the caller. (We could change
 *      this parameter to an automation action that can be looked up here
 *      instead.)
 *
 * \param duration
 *      Provides the tool-tip duration in milliseconds.  If -1 (the default),
 *      then duration depends on the length of the tool-tip (and this is still
 *      a bit too short.
 */

void
tooltip_with_keystroke
(
    QWidget * widget,
    const std::string & keyname,
    int duration
)
{
    if (! keyname.empty())
    {
        QString qtip = widget->toolTip();
        std::string tip = qtip.toStdString();
        if (tip.empty())
        {
            tip = "Key " + keyname;
        }
        else
        {
            tip += " [";
            tip += keyname;
            tip += "]";
        }
        qtip = qt(tip);
        widget->setToolTip(qtip);
        widget->setToolTipDuration(duration);
    }
}

}               // namespace seq66

/*
 * qt5_helpers.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


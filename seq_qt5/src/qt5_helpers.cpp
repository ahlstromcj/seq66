/*
 *  This file is part of seq66.
 *
 *  seq66 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  seq66 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq66; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          qt5_helpers.cpp
 *
 *  This module declares/defines some helpful macros or functions.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-03-14
 * \updates       2019-03-23
 * \license       GNU GPLv2 or above
 *
 */

#include <QKeyEvent>
#include <QPushButton>

#include "qt5_helpers.hpp"

/*
 * Don't document the namespace.
 */

namespace seq66
{

/**
 *  Given a keystroke from a Qt 5 GUI, this function returns an "ordinal"
 *  version of the keystroke.
 *
 * \param event
 *      The putative Qt 5 keystroke event.
 *
 * \param press
 *      Set to true if the keystroke event is a key-press, and false if it is a
 *      key-release.
 *
 * \return
 *      Returns an object that makes the key event easier to use.
 */

keystroke
qt_keystroke (QKeyEvent * event, bool press)
{
    seq66::ctrlkey k = event->key();
    unsigned kmods = static_cast<unsigned>(event->modifiers());
    seq66::ctrlkey ordinal = seq66::qt_modkey_ordinal(k, kmods);
    return keystroke(ordinal, press);
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

}               // namespace seq66

/*
 * qt5_helpers.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


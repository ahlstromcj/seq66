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
 * \file          qtestframe.cpp
 *
 *  This module declares/defines the base class for the keystroke test
 *  window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-16
 * \updates       2018-11-23
 * \license       GNU GPLv2 or above
 *
 */

#include <QKeyEvent>

#include "ctrl/keymap.hpp"              /* free functions for keystrokes    */
#include "qt/qsmacros.hpp"
#include "qtestframe.hpp"
#include "util/basic_macros.hpp"

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#if defined SEQ66_QMAKE_RULES
#include "forms/ui_qtestframe.h"        /* for building via qmake-to-make   */
#else
#include "forms/qtestframe.ui.h"        /* for building via GNU autotools   */
#endif

/**
 *
 * \param p
 *      Provides the performer object to use for interacting with this live
 *      frame.
 *
 * \param ssnum
 *      Provides the screen-set number.
 *
 * \param parent
 *      Provides the parent window/widget for this container window.  Defaults
 *      to null.  Note that this parameter does not link this class to the
 *      parent as a QWidget, because that would make this class appear inside
 *      the qsmainwnd user-interface.
 */

qtestframe::qtestframe (QWidget * parent)
 :
    QWidget             (parent),
    ui                  (new Ui::qtestframe),
    m_fake_performer    ()
{
    ui->setupUi(this);
    setFocusPolicy(Qt::ClickFocus);     /* so that we can press keys    */

    // show();                          // the caller can do this
}

/**
 *  Deletes the user interface, then tells the editor parent to remove
 *  this object.
 */

qtestframe::~qtestframe ()
{
    delete ui;
}

/**
 *  This code is most just a test of the keymap module.
 *  It casts Qt::KeyboardModifier to an unsigned value.
 *
 * \param event
 *      Provides a pointer to the key event.
 */

void
qtestframe::keyPressEvent (QKeyEvent * event)
{
    seq66::ctrlkey kkey = event->key();
    unsigned kmods = static_cast<unsigned>(event->modifiers());
    seq66::ctrlkey ordinal = seq66::qt_modkey_ordinal(kkey, kmods);

#if defined SEQ66_PLATFORM_DEBUG_NOT_NEEDED
    seq66::ctrlkey ktext = QS_TEXT_CHAR(event->text());
    std::string kname = seq66::qt_modkey_name(kkey, kmods);
    std::string modstring = "Mods: ";
    if (kmods & Qt::ShiftModifier)
        modstring += "S";

    if (kmods & Qt::ControlModifier)
        modstring += "C";

    if (kmods & Qt::AltModifier)
        modstring += "A";

    if (kmods & Qt::MetaModifier)
        modstring += "M";

    if (kmods & Qt::KeypadModifier)
        modstring += "K";

    if (kmods & Qt::GroupSwitchModifier)
        modstring += "G";

    printf
    (
        "Key: name = %s; ordinal = 0x%x; key = 0x%x; text = 0x%x; %s\n",
        kname.c_str(), ordinal, kkey, ktext, modstring.c_str()
    );

#else       // SEQ66_PLATFORM_DEBUG

    m_fake_performer.handle_keystroke(ordinal);

#endif      // SEQ66_PLATFORM_DEBUG

    QWidget::keyPressEvent(event);              /* event->ignore()      */
}

/*
 * qtestframe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


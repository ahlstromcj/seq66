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
 * \file          qtcore_task.cpp
 * \library       Seqtool (from the Seq66 project)
 * \author        Chris Ahlstrom
 * \date          2018-11-14
 * \updates       2018-11-23
 * \version       $Revision$
 * \license       $XPC_SUITE_GPL_LICENSE$
 *
 */

#include <qnamespace.h>

#include "seq66-config.h"
#include "ctrl/keymap.hpp"              /* free functions for keystrokes    */
#include "qt/qsmacros.hpp"              /* QS_TEXT_CHAR(), etc.             */
#include "qtcore_task.hpp"

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.  Not needed, since qtcore_task has no
 *  UI.
 *

#if defined SEQ66_QMAKE_RULES
#include "forms/ui_qtcore_task.h"
#else
#include "forms/qtcore_task.ui.h"
#endif
 *
 */

qtcore_task::qtcore_task (QObject * parent) :

    QObject         (parent),
    m_time_to_go    (false)
{
    // no code
}

qtcore_task::~qtcore_task ()
{
    // QObject::~QObject() automatically called
}

void
qtcore_task::run ()
{
    // Any processing?

    emit finished();
}

/*
 * qtcore_task.cpp
 *
 * vim: ts=4 sw=4 et ft=cpp
 */


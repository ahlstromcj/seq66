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
 *  Place, Suite 330, Boston, MA  02111-1307  USA.
 */

/**
 * \file          qscrollslave.cpp
 *
 *  This module declares/defines a class for ignoring arrow events.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2023-05-23
 * \updates       2023-05-24
 * \license       GNU GPLv2 or above
 *
 *  Compare using qscrollslave for a QScrollArea versus the method used in
 *  qperfnames::keyPressEvent().
 */

#include <QApplication>
#include <QResizeEvent>
#include <QScrollBar>
#include <QWheelEvent>

#include "qscrollslave.h"               /* ::qscrollslave class             */
#include "qscrollmaster.h"              /* ::qscrollmaster class            */
#include "util/basic_macros.h"          /* not_nullptr() test macro         */

/*
 * Note that there is no namespace; the Qt uic specification does not seem to
 * support them.
 */

/**
 *  Constructor.
 *
 * \param qf
 *      Provides the "parent" of this object.
 */

qscrollslave::qscrollslave (QWidget * qf) :
    QScrollArea (qf),
    m_master    (nullptr)
{
    // Done!
}

qscrollslave::~qscrollslave ()
{
    // no code
}

void
qscrollslave::attach_master (qscrollmaster * qsm)
{
    m_master = qsm;
}

static bool
is_direction_key (int key)
{
    return
    (
        key == Qt::Key_Down || key == Qt::Key_Up ||
        key == Qt::Key_Left || key == Qt::Key_Right ||
        key == Qt::Key_PageUp || key == Qt::Key_PageDown
    );
}

/**
 *  Arrow and Page events are accepted so they don't go to the QScrollArea,
 *  and then forwarded to the qscrollmaster (e.g. the piano roll pane in
 *  the pattern editor.
 */

void
qscrollslave::keyPressEvent (QKeyEvent * event)
{
    int key = event->key();
    bool isarrow = is_direction_key(key);
    if (isarrow)
    {
        event->accept();                            // event->ignore();
        if (not_nullptr(m_master))
        {
            QKeyEvent keyevent = *event;
            QApplication::sendEvent(m_master, &keyevent);
        }
    }
    else
        QScrollArea::keyPressEvent(event);          // event->accept();
}

void
qscrollslave::keyReleaseEvent (QKeyEvent * event)
{
    int key = event->key();
    bool isarrow = is_direction_key(key);
    if (isarrow)
        event->accept();                            // event->ignore();
    else
        QScrollArea::keyReleaseEvent(event);        // event->accept();
}

/**
 *  The event forward here does not work, not sure why.
 */

void
qscrollslave::wheelEvent (QWheelEvent * qwep)
{
    qwep->accept();                                 // qwep->ignore();
    if (not_nullptr(m_master))
    {
        QWheelEvent wheelevent = *qwep;
        QApplication::sendEvent(m_master, &wheelevent);
    }
}

/*
 * qscrollslave.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


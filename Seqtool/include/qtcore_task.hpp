#if ! defined QTCORE_TASK_HPP
#define QTCORE_TASK_HPP

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
 * \file          qtcore_task.hpp
 * \library       Seqtool (from the Seq66 project)
 * \author        Chris Ahlstrom
 * \date          2018-11-14
 * \updates       2019-01-08
 * \version       $Revision$
 * \license       $XPC_SUITE_GPL_LICENSE$
 *
 *    This module provides helper functions for the unit tests of
 *    the midicontrol module of the libseq66 library.
 */

#include <QObject>

/*
 * When compiling on Ubuntu 18.04 (as opposed to Debian Sid), this symbol
 * is not found.  Ubuntu uses Qt 5.9, while this symbol was introduced in Qt
 * 5.10.  However, removing all headers but QObject seems to have fixed this
 * issue.  Odd.
 */

#if defined USE_THIS_MACRO
#if ! defined QT_INIT_METAOBJECT
#if defined Q_CC_GNU
#define QT_INIT_METAOBJECT  __attribute((init_priority(101)))
#else
#define QT_INIT_METAOBJECT              /* not present pre-Qt 10!           */
#endif
#endif
#endif

#include "util/basic_macros.hpp"        /* nullptr                          */

/**
 *
 */

class qtcore_task : public QObject
{
    Q_OBJECT

public:

    qtcore_task (QObject * parent = nullptr);

    virtual ~qtcore_task ();

public slots:

    void run ();

signals:

    void finished ();           /* implementation created by moc    */

private:

    bool m_time_to_go;

};              // class qtcore_task

#endif          // QTCORE_TASK_HPP

/*
 * qtcore_task.hpp
 *
 * vim: ts=4 sw=4 et ft=cpp
 */

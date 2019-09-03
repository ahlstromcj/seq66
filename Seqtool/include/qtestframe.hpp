#if ! defined QTESTFRAME_HPP
#define QTESTFRAME_HPP

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
 * \file          qtestframe.hpp
 *
 *  This module declares/defines the base class for the external
 *  keystroke test window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-16
 * \updates       2019-01-08
 * \license       GNU GPLv2 or above
 *
 *  The sequence editing window is known as the "Pattern Editor".  Kepler34
 *  provides an editor embedded within a tab, but we supplement that with a
 *  more sophisticated external editor, which works a lot more like the Gtkmm
 *  seqedit class.
 */

#include <QWidget>

/*
 * When compiling on Ubuntu 18.04 (as opposed to Debian Sid), this symbol
 * is not found.  Ubuntu uses Qt 5.9, while this symbol was introduced in Qt
 * 5.10.  But now the problem has disappeared.  Odd.
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

#include "faker.hpp"                    /* proof-of-concept class faker     */

class QKeyEvent;

namespace Ui
{
    class qtestframe;
}

/**
 *
 */

class qtestframe : public QWidget
{
    Q_OBJECT

public:

    explicit qtestframe (QWidget * parent = nullptr);
    virtual ~qtestframe ();

protected:

    virtual void keyPressEvent (QKeyEvent * event);

private:

    Ui::qtestframe * ui;

private:

    faker m_fake_performer;

};              // class qtestframe

#endif          // QTESTFRAME_HPP

/*
 * qtestframe.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


#ifndef SEQ66_QSCROLLSLAVE_H
#define SEQ66_QSCROLLSLAVE_H

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
 * \file          qscrollslave.h
 *
 *  This module declares/defines a class for ignoring arrow events.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2023-05-23
 * \updates       2023-05-23
 * \license       GNU GPLv2 or above
 *
 */

#include <QScrollArea>
#include <QSize>
#include <list>                         /* std::list container class        */

#define SEQ66_ENABLE_SCROLL_WHEEL       /* EXPERIMENTAL for issue #3        */
#undef SEQ66_ENABLE_SCROLL_WHEEL_ALL    /* leave this macro undefined       */

/*
 *  Forward declarations.  The Qt header files are moved into the cpp file.
 */

class QFrame;
class QScrollBar;

/*
 * Note that there is no namespace; the Qt uic specification does not seem to
 * support them well.  Also note the lack of namespace seq66 for this class.
 */

/**
 *  Derived from QScrollArea, this class provides a way to pass any horizontal
 *  or vertical scrollbar value changes on to one or more other QScrollBars.
 *  Any number (even 0) of horizontal or vertical scrollbars can be added to
 *  this object.  See the qseqroll class and the class that creates it,
 *  qseqeditframe64.
 */

class qscrollslave : public QScrollArea
{

private:

public:

    qscrollslave (QWidget * qf);
    virtual ~qscrollslave ();

protected:      // QWidget overrides

    virtual void keyPressEvent (QKeyEvent *) override;
    virtual void keyReleaseEvent (QKeyEvent *) override;
    virtual void wheelEvent (QWheelEvent *) override;
//  virtual void scrollContentsBy (int dx, int dy) override;

};              // class qscrollslave

#endif          // SEQ66_QSCROLLSLAVE_H

/*
 * qscrollslave.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


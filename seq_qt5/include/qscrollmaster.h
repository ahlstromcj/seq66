#ifndef SEQ66_QSCROLLMASTER_H
#define SEQ66_QSCROLLMASTER_H

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
 * \file          qscrollmaster.h
 *
 *  This module declares/defines a class for controlling other QScrollAreas
 *  from this one.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-06-18
 * \updates       2023-09-12
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

class qscrollmaster : public QScrollArea
{

public:

    enum class dir
    {
        left,
        right,
        up,
        down
    };

private:

    using container = std::list<QScrollBar *>;

private:

    /**
     *  Holds a list of external vertical scroll bars to me maintained.
     */

    container m_v_scrollbars;

    /**
     *  Holds a list of external horizontal scroll bars to me maintained.
     */

    container m_h_scrollbars;

    /**
     *  Holds a pointer to this scroll-area's vertical scrollbar.
     */

    QScrollBar * m_self_v_scrollbar;

    /**
     *  Holds a pointer to this scroll-area's horizontal scrollbar.
     */

    QScrollBar * m_self_h_scrollbar;

public:

    qscrollmaster (QWidget * qf);
    virtual ~qscrollmaster ();

    void add_v_scroll (QScrollBar * qsb)
    {
        m_v_scrollbars.push_back(qsb);
    }

    void add_h_scroll (QScrollBar * qsb)
    {
        m_h_scrollbars.push_back(qsb);
    }

    QScrollBar * v_scroll ()
    {
        return m_self_v_scrollbar;
    }

    QScrollBar * h_scroll ()
    {
        return m_self_h_scrollbar;
    }

    QSize viewport_size () const
    {
        return viewportSizeHint();
    }

    void scroll_to_x (int x);
    void scroll_x_by_factor (float f);
    void scroll_x_by_step (dir d);
    void scroll_to_y (int y);
    void scroll_y_by_factor (float f);
    void scroll_y_by_step (dir d);

protected:      // QWidget overrides

    virtual void wheelEvent (QWheelEvent *) override;
    virtual void scrollContentsBy (int dx, int dy) override;

};              // class qscrollmaster

#endif          // SEQ66_QSCROLLMASTER_H

/*
 * qscrollmaster.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


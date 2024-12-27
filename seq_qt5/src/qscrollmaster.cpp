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
 * \file          qscrollmaster.cpp
 *
 *  This module declares/defines a class for controlling other QScrollAreas
 *  from this one.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-06-18
 * \updates       2024-12-27
 * \license       GNU GPLv2 or above
 *
 *  When inheriting QAbstractScrollArea, you need to do the following:
 *
 *  -#  Control the scroll bars by setting their range, value, page step, and
 *      tracking their movements.
 *  -#  Draw the contents of the area in the viewport according to the values
 *      of the scroll bars.
 *  -#  Handle events received by the viewport in viewportEvent() - notably
 *      resize events.
 *  -#  Use viewport->update() to update the contents of the viewport instead
 *      of update() as all painting operations take place on the viewport.
 *
 *  In order to track scroll bar movements, reimplement the virtual function
 *  scrollContentsBy().
 *
 *  For convenience, QAbstractScrollArea makes all viewport events available
 *  in the virtual viewportEvent() handler. QWidget's specialized handlers are
 *  remapped to viewport events in the cases where this makes sense. The
 *  remapped specialized handlers are:
 *
 *      -   paintEvent()
 *      -   mousePressEvent()
 *      -   mouseReleaseEvent()
 *      -   mouseDoubleClickEvent()
 *      -   mouseMoveEvent()
 *      -   wheelEvent()
 *      -   dragEnterEvent()
 *      -   dragMoveEvent()
 *      -   dragLeaveEvent()
 *      -   dropEvent()
 *      -   contextMenuEvent()
 *      -   resizeEvent()
 *
 * Other useful QScrollBar functions:
 *
 *  -   setPageStep()
 *  -   setSingleStep()
 */

#include <QResizeEvent>
#include <QScrollBar>
#include <QWheelEvent>

#include "qscrollmaster.h"              /* ::qscrollmaster class            */
#include "util/basic_macros.hpp"        /* nullptr and other macros         */

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

qscrollmaster::qscrollmaster (QWidget * qf) :
    QScrollArea         (qf),
    m_v_scrollbars      (),
    m_h_scrollbars      (),
    m_self_v_scrollbar  (verticalScrollBar()),
    m_self_h_scrollbar  (horizontalScrollBar())
{
    // Done!
}

qscrollmaster::~qscrollmaster ()
{
    // no code
}

/**
 *  This override of a QScrollArea virtual member function modifies any
 *  attached/listed scrollbars and then calls the base-class version of this
 *  function.
 *
 * \param dx
 *      The change in the x position value of the scrollbar.  Simply passed
 *      on to the base-class version of this function.
 *
 * \param dy
 *      The change in the y position value of the scrollbar.  Simply passed
 *      on to the base-class version of this function.
 */

void
qscrollmaster::scrollContentsBy (int dx, int dy)
{
    if (! m_v_scrollbars.empty())
    {
        int vvalue = m_self_v_scrollbar->value();
        scroll_to_y(vvalue);
    }
    if (! m_h_scrollbars.empty())
    {
        int hvalue = m_self_h_scrollbar->value();
        scroll_to_x(hvalue);
    }
    QScrollArea::scrollContentsBy(dx, dy);
}

void
qscrollmaster::scroll_to_x (int x)
{
    if (! m_h_scrollbars.empty())
    {
        for (auto hit : m_h_scrollbars)
            hit->setValue(x);

        m_self_h_scrollbar->setValue(x);
    }
}

void
qscrollmaster::scroll_x_to_factor (float f)
{
    if (! m_h_scrollbars.empty())
    {
        int hmin = m_self_h_scrollbar->minimum();
        int hmax = m_self_h_scrollbar->maximum();
        int newh = int((hmax - hmin) * f) + hmin;
        int dx = hmax - newh;
        scroll_to_x(newh);
        QScrollArea::scrollContentsBy(dx, 0);
    }
}

void
qscrollmaster::scroll_x_by_factor (float f)
{
    if (! m_h_scrollbars.empty())
    {
        int hvalue = m_self_h_scrollbar->value();
        int newh = int(hvalue * f);
        int dx = hvalue - newh;
        scroll_to_x(newh);
        QScrollArea::scrollContentsBy(dx, 0);
    }
}

void
qscrollmaster::scroll_x_by_step (dir d)
{
    if (! m_h_scrollbars.empty())
    {
        int dx = m_self_h_scrollbar->singleStep();
        if (d == dir::left)                         /* else right assumed   */
            dx = -dx;

        int hvalue = m_self_h_scrollbar->value();
        int newh = hvalue + dx;
        scroll_to_x(newh);
        QScrollArea::scrollContentsBy(dx, 0);
    }
}

void
qscrollmaster::scroll_to_y (int y)
{
    if (! m_v_scrollbars.empty())
    {
        for (auto vit : m_v_scrollbars)
            vit->setValue(y);

        m_self_v_scrollbar->setValue(y);
    }
}

/**
 *  Scrolls vertically by a factor, plus a bit to try to center in the roll.
 *  TODO!
 *
 *  Used in qseqeditframe64::scroll_to_note().
 *
 * \param f
 *      Provides the "average" value of the note(s) found at the beginning
 *      of the pattern.
 */

void
qscrollmaster::scroll_y_to_factor (float f)
{
    if (! m_v_scrollbars.empty() && f > 0.1)
    {
        int vmin = m_self_v_scrollbar->minimum();
        int vmax = m_self_v_scrollbar->maximum();
        int newv = int((vmax - vmin) * f) + vmin;
        scroll_to_y(newv);

        /*
         * int dy = newv - vmax;
         * QScrollArea::scrollContentsBy(0, dy);
         */
    }
}

void
qscrollmaster::scroll_y_by_factor (float f)
{
    if (! m_v_scrollbars.empty())
    {
        int vvalue = m_self_v_scrollbar->value();
        int newv = int(vvalue * f);
        int dy = vvalue - newv;
        scroll_to_y(newv);
        QScrollArea::scrollContentsBy(0, dy);
    }
}

void
qscrollmaster::scroll_y_by_step (dir d)
{
    if (! m_v_scrollbars.empty())
    {
        int dy = m_self_v_scrollbar->singleStep();
        if (d == dir::up)                           /* else down assumed    */
            dy = -dy;

        int vvalue = m_self_v_scrollbar->value();
        int newv = vvalue + dy;
        scroll_to_y(newv);
        QScrollArea::scrollContentsBy(dy, 0);
    }
}

void
qscrollmaster::show_values () const
{
    int xv = m_self_h_scrollbar->value();
    int yv = m_self_v_scrollbar->value();
    printf("Scrollbars at (%d, %d)\n", xv, yv);
}

void
qscrollmaster::wheelEvent (QWheelEvent * qwep)
{
    QScrollArea::wheelEvent(qwep);
}

/*
 * qscrollmaster.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


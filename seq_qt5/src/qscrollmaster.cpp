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
 *  along with seq66; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
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
 * \updates       2019-07-27
 * \license       GNU GPLv2 or above
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

qscrollmaster::qscrollmaster (QWidget * qf)
 :
    QScrollArea         (qf),
    m_v_scrollbars      (),
    m_h_scrollbars      (),
    m_self_v_scrollbar  (verticalScrollBar()),
    m_self_h_scrollbar  (horizontalScrollBar())
{
    // Done!
}

/**
 *
 */

qscrollmaster::~qscrollmaster ()
{
    // no code
}

/**
 *  This override of a QScrollArea virtual member function
 *  modifies any attached/listed scrollbars and then calls the base-class
 *  version of this function.
 *
 * \param dx
 *      The change in the x position value of the scrollbar.  Simply passed on
 *      to the base-class version of this function.
 *
 * \param dy
 *      The change in the y position value of the scrollbar.  Simply passed on
 *      to the base-class version of this function.
 */

void
qscrollmaster::scrollContentsBy (int dx, int dy)
{
    if (! m_v_scrollbars.empty())
    {
        int vvalue = m_self_v_scrollbar->value();
        for
        (
            iterator vit = m_v_scrollbars.begin();
            vit != m_v_scrollbars.end(); ++vit
        )
        {
            (*vit)->setValue(vvalue);
        }
    }

    if (! m_h_scrollbars.empty())
    {
        int hvalue = m_self_h_scrollbar->value();
        for
        (
            iterator hit = m_h_scrollbars.begin();
            hit != m_h_scrollbars.end(); ++hit
        )
        {
            (*hit)->setValue(hvalue);
        }
    }
    QScrollArea::scrollContentsBy(dx, dy);
}

/**
 *
 */

void
qscrollmaster::paintEvent (QPaintEvent * qpep)
{

#if defined SEQ66_PLATFORM_DEBUG_TMI
    static int s_count = 0;
    printf("qscrollmaster::paintEvent(%d)\n", s_count++);
#endif

    QScrollArea::paintEvent(qpep);
}

/**
 *
 */

void
qscrollmaster::resizeEvent (QResizeEvent * qrep)
{

#if defined SEQ66_PLATFORM_DEBUG_TMI
    static int s_count = 0;
    printf("qscrollmaster::resizeEvent(%d)\n", s_count++);
#endif

    qrep->ignore();                         /* QScrollArea::resizeEvent(qrep) */
}

/**
 *
 */

void
qscrollmaster::wheelEvent (QWheelEvent * qwep)
{

#if defined SEQ66_PLATFORM_DEBUG_TMI
    static int s_count = 0;
    printf("qscrollmaster::wheelEvent(%d)\n", s_count++);
#endif

    qwep->ignore();                         /* QScrollArea::wheelEvent(qwep)  */
}

/**
 *  When inheriting QAbstractScrollArea, you need to do the following:
 *
 *  -#  Control the scroll bars by setting their range, value, page step, and
 *      tracking their movements.
 *  -#  Draw the contents of the area in the viewport according to the values of
 *      the scroll bars.
 *  -#  Handle events received by the viewport in viewportEvent() - notably
 *      resize events.
 *  -#  Use viewport->update() to update the contents of the viewport instead of
 *      update() as all painting operations take place on the viewport.
 *
 *  In order to track scroll bar movements, reimplement the virtual function
 *  scrollContentsBy().
 *
 *  For convenience, QAbstractScrollArea makes all viewport events available in
 *  the virtual viewportEvent() handler. QWidget's specialized handlers are
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
 */

void
qscrollmaster::adjust_for_resize ()
{
   QSize view = viewport()->size();
   QSize widg = widget()->size();

#if defined SEQ66_PLATFORM_DEBUG_TMI

    /*
     * Typical: viewport = 208x796 and widget = 1538x38790.
     */

    printf("viewport size (w, h) = (%d, %d)\n", view.height(), view.width());
    printf("widget size (w, h)   = (%d, %d)\n", widg.height(), widg.width());

#endif

   verticalScrollBar()->setPageStep(view.height());
   horizontalScrollBar()->setPageStep(view.width());
   verticalScrollBar()->setRange(0, widg.height() - view.height());
   horizontalScrollBar()->setRange(0, widg.width() - view.width());

   // updateWidgetPosition();
}

/*
 * qscrollmaster.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


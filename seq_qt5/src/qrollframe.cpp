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
 * \file          qrollframe.cpp
 *
 *  This module declares/defines the base class for drawing on the piano
 *  roll of the patterns editor for the Qt 5 implementation.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-07-29
 * \updates       2019-08-03
 * \license       GNU GPLv2 or above
 *
 *  We need a way to grab part of the pixmap so that we can restore it when the
 *  progress bar ("progbar" or "playhead") leaves that part.  Here are the
 *  operations we need to do:
 *
 *  -#  Paint the whole sequence.  This is not part of rollframe's
 *      functionality.
 *  -#  Get the new pixmap when it changes:
 *      -#  Upon the first paintEvent() call. This will require the whole
 *          sequence to be redrawn from scratch.  This will also occur when a
 *          note is added to or deleted from the sequence.
 *      -#  When a note is added or deleted from the sequence.
 *      -#  When the frame number changes during follow-progress.
 *      -#  When a resize occurs.  This also causes the pixmap to be
 *          regenerated.
 *  -#  Just before drawing the progress bar, get the x coordinate of the
 *      current tick, where the progress bar will be drawn.
 *  -#  Draw the progress bar.
 *  -#  Just before drawing the next progress bar, erase the previous one by
 *      covering it with the portion of the saved pixmap from before.
 *
 *
 *          <----------- m_frame_size -------------->
 *          t0            Frame 0                   t1
 *          |                                       |   Frame
 *          :       .       .       .       :       :   Bars
 *          |                                       |   Window
 *           ---------------------------------------
 *
 *  void QWidget::render
 *  (
 *      QPaintDevice * target,
 *      const QPoint & targetOffset = QPoint(),
 *      const QRegion & sourceRegion = QRegion(),
 *      QWidget::RenderFlags renderFlags =
 *          RenderFlags(DrawWindowBackground | DrawChildren)
 *  )
 *
 *  But QPixmap doesn't have a render() function.
 *
 *  QPainter painter(this);
 *  ...
 *  painter.end();
 *  myWidget->render(this);
 *
 *  Ensure that you call QPainter::end() for the target device's active
 *  painter (if any) before rendering.
 *
 */

#include <QPainter>
#include <QPixmap>
#include <QRegion>
#include <QSize>
#include <QWidget>

#include "basic_macros.hpp"             /* provides the not_nullptr() macro */
#include "qrollframe.hpp"               /* seq66::qrollframe class          */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Creates a small management object for a pixmap.  The caller creates the
 *  provided pixmap... too many parameters to pass along.
 *
 *  All parameters must be vetted by the caller.
 */

qrollframe::qrollframe (int barwidth) :
    m_grid_pixmap           (nullptr),
    m_rendering             (false),
    m_frame_number          (-1),
    m_frame_width           (0),
    m_frame_height          (0),
    m_bar_width             (barwidth),
    m_x_0                   (0),
    m_x_current             (0),
    m_x_1                   (0)
{
    // no code
}

/**
 *
 */

qrollframe::~qrollframe ()
{
    if (not_nullptr(m_grid_pixmap))
        delete m_grid_pixmap;
}

/**
 *
 */

bool
qrollframe::regenerate (const QRect & r, QWidget * widget)
{
    int w = r.width();
    int h = r.height();
    bool result = resize(w, h);
    if (result)
    {
        QSize gridsize(w, h);
        if (not_nullptr(m_grid_pixmap))
            delete m_grid_pixmap;

        m_grid_pixmap = new (std::nothrow) QPixmap(gridsize);
        result = not_nullptr(m_grid_pixmap);
        if (result)
        {
            QRegion region(r);
            m_x_0 = m_x_current = r.x();
            m_x_1 = r.right();
            m_grid_pixmap->fill();          /* fills a white background     */
            m_rendering = true;
            widget->render(m_grid_pixmap, QPoint(), region);
            m_rendering = false;
        }
    }
    return result;
}

/**
 *
 */

bool
qrollframe::resize (int w, int h)
{
    bool result = w != m_frame_width || h != m_frame_height;
    if (result)
    {
        m_frame_width = w;
        m_frame_height = h;
    }
    return result;
}

/**
 *
 */

void
qrollframe::restore_bar_area (QPainter & painter, int progx, int progy)
{
    if (not_nullptr(m_grid_pixmap) && progx >= 0)
    {
        int dx = progx;
        int dy = progy;
        int dw = bar_width();
        int dh = height();
        int sx = progx - m_x_0;
        int sy = 0;
        int sw = bar_width();
        int sh = height();
        painter.drawPixmap(dx, dy, dw, dh, *m_grid_pixmap, sx, sy, sw, sh);
    }
}

/**
 *  Writes (dumps) the image to a file, useful for trouble-shooting.
 */

void
qrollframe::dump () const
{
    if (not_nullptr(m_grid_pixmap) && frame() == 0)
    {
        char tmp[16];
        snprintf(tmp, sizeof tmp, "grid%02d.png", m_frame_number);
        m_grid_pixmap->save(tmp);
    }
}

}           // namespace seq66

/*
 * qrollframe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


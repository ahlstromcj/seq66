/*
 *  This file is part of seq66.
 *
 *  seq24 is free software; you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option) any later
 *  version.
 *
 *  seq24 is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with seq24; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          rect.cpp
 *
 *  This module declares/defines the concrete class for a rectangle.
 *
 * \library       seq66 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2017-09-16
 * \updates       2020-11-25
 * \license       GNU GPLv2 or above
 *
 */

#include "util/rect.hpp"                /* seq66::rect                      */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 * \defaultctor
 */

rect::rect () :
    m_x         (0),                            /* also known as x0()       */
    m_y         (0),                            /* also known as y0()       */
    m_width     (0),                            /* used to calculate x1()   */
    m_height    (0)                             /* used to calculate y1()   */
{
    // Empty body
}

/**
 *  Principal constructor.
 *
 * \param x
 *      The x coordinate.
 *
 * \param y
 *      The y coordinate.
 *
 * \param width
 *      The width value.
 *
 * \param height
 *      The width value.
 */

rect::rect (int x, int y, int width, int height) :
    m_x         (x),
    m_y         (y),
    m_width     (width),
    m_height    (height)
{
    // Empty body
}

/**
 *  Gets the rectangle values for primitive callers that don't store them as
 *  a rectangle.
 *
 * \param [out] x0
 *      The destination x coordinate.
 *
 * \param [out] y0
 *      The destination y coordinate.
 *
 * \param [out] width
 *      The destination width value.
 *
 * \param [out] height
 *      The destination width value.
 */

void
rect::get (int & x, int & y, int & width, int & height) const
{
    x = m_x;
    y = m_y;
    width = m_width;
    height = m_height;
}

void
rect::get_coordinates (int & x0, int & y0, int & x1, int & y1) const
{
    x0 = m_x;
    y0 = m_y;
    x1 = x0 + m_width;
    y1 = y0 + m_height;
}

/**
 *  Sets all of the members of the rectangle directly.
 *
 * \param x
 *      The x coordinate.
 *
 * \param y
 *      The y coordinate.
 *
 * \param width
 *      The width value.
 *
 * \param height
 *      The width value.
 */

void
rect::set (int x, int y, int width, int height)
{
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;
}

void
rect::set_coordinates (int x0, int y0, int x1, int y1)
{
    m_x = x0;
    m_y = y0;
    m_width = x1 - x0;
    m_height = y1 - y0;
}

/**
 *  Converts rectangle corner coordinates to a rect object, which includes
 *  width and height.
 *
 * \param x0
 *      The x value of the first corner.
 *
 * \param y0
 *      The y value of the first corner.
 *
 * \param x1
 *      The x value of the second corner.
 *
 * \param y1
 *      The y value of the second corner.
 *
 * \param [out] r
 *      The destination for the coordinate, width, and height.
 */

void
rect::xy_to_rect (int x0, int y0, int x1, int y1, rect & r)
{
    if (x0 < x1)
    {
        r.m_x = x0;
        r.m_width = x1 - x0;
    }
    else
    {
        r.m_x = x1;
        r.m_width = x0 - x1;
    }
    if (y0 < y1)
    {
        r.m_y = y0;
        r.m_height = y1 - y0;
    }
    else
    {
        r.m_y = y1;
        r.m_height = y0 - y1;
    }
}

/**
 *  Converts rectangle corner coordinates to a starting coordinate, plus a
 *  width and height.  This function checks the mins / maxes, and then fills
 *  in the x, y, width, and height values.  It picks the lowest x and y
 *  coordinate to use as the corner coordinate, so that the width and height
 *  are always positive.
 *
 * \param x0
 *      The x value of the first corner.
 *
 * \param y0
 *      The y value of the first corner.
 *
 * \param x1
 *      The x value of the second corner.
 *
 * \param y1
 *      The y value of the second corner.
 *
 * \param [out] x
 *      The destination for the x value in pixels.
 *
 * \param [out] y
 *      The destination for the y value in pixels.
 *
 * \param [out] w
 *      The destination for the rectangle width in pixels.
 *
 * \param [out] h
 *      The destination for the rectangle height value in pixels.
 */

void
rect::xy_to_rect_get
(
    int x0, int y0, int x1, int y1,
    int & x, int & y, int & w, int & h
)
{
    if (x0 < x1)
    {
        x = x0;
        w = x1 - x0;
    }
    else
    {
        x = x1;
        w = x0 - x1;
    }
    if (y0 < y1)
    {
        y = y0;
        h = y1 - y0;
    }
    else
    {
        y = y1;
        h = y0 - y1;
    }
}

}           // namespace seq66

/*
 * rect.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


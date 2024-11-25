#if ! defined SEQ66_RECT_HPP
#define SEQ66_RECT_HPP

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
 * \file          rect.hpp
 *
 *  This module declares/defines the base class for a Seq66 rectangle.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2017-09-16
 * \updates       2024-11-25
 * \license       GNU GPLv2 or above
 *
 *  Our version of the rectangle provides specific functionality not necessary
 *  found in, say, GUI rectangle classes.
 */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Supports a simple rectangle and some common manipulations needed by the
 *  user-interface.  Will eventually replace the gui_drawingarea_gtk2::rect
 *  structure.
 *
 *  One minor issue that may crop up in the transition from Gtkmm to Qt 5 is
 *  the exact meaning of the coordinates.  To be clarified later.  For now, it
 *  uses the current Gtkmm conventions.
 */

class rect
{

private:

    int m_x;        /**< The x coordinate of the first corner or x0.        */
    int m_y;        /**< The y coordinate of the first corner or y0.        */
    int m_width;    /**< The width of the rectangle.                        */
    int m_height;   /**< The height of the rectangle.                       */

public:

    rect ();
    rect (int x, int y, int width, int height);

    void get (int & x, int & y, int & width, int & height) const;
    void get_coordinates (int & x0, int & y0, int & x1, int & y1) const;
    void set (int x, int y, int width, int height);
    void set_coordinates (int x0, int y0, int x1, int y1);

    void clear ()
    {
        m_x = m_y = m_width = m_height = 0;
    }

    static void xy_to_rect (int x0, int y0, int x1, int y1, rect & r);
    static void xy_to_rect_get
    (
        int x0, int y0, int x1, int y1,
        int & x, int & y, int & w, int & h
    );

    void xy_to_rect (int x0, int y0, int x1, int y1)
    {
        xy_to_rect(x0, y0, x1, y1, *this);
    }

    int x () const
    {
        return m_x;
    }

    void x (int v)
    {
        m_x = v;
    }

    int x0 () const
    {
        return m_x;
    }

    void x0 (int v)
    {
        m_x = v;
    }

    /**
     *  Provides a setter that uses the parameter to increment the member.
     *  The width is assumed to be unchanged by this function.
     */

    void x_incr (int v)
    {
        m_x += v;
    }

    int y () const
    {
        return m_y;
    }

    void y (int v)
    {
        m_y = v;
    }

    int y0 () const
    {
        return m_y;
    }

    void y0 (int v)
    {
        m_y = v;
    }

    /**
     *  Provides a setter that uses the parameter to increment the member.
     *  The height is assumed to be unchanged by this function.
     */

    void y_incr (int v)
    {
        m_y += v;
    }

    int width () const
    {
        return m_width;
    }

    void width (int w)
    {
        m_width = w;
    }

    int x1 () const
    {
        return m_x + m_width;
    }

    void x1 (int x)
    {
        m_width = x - m_x;
    }

    void width_incr (int w)
    {
        m_width += w;
    }

    int height () const
    {
        return m_height;
    }

    void height (int h)
    {
        m_height = h;
    }

    int y1 () const
    {
        return m_y + m_height;
    }

    void y1 (int h)
    {
        m_height = h - m_y;
    }

    void height_incr (int h)
    {
        m_height += h;
    }

    void xy_incr (int xv, int yv)
    {
        m_x += xv;
        m_y += yv;
    }

private:

    /**
     *  The calculated width is always positive.  Follows the conventions of
     *  the xy_to_rect_get() function.
     */

    static int calculated_width (int x1, int x2)
    {
        return (x1 < x2) ? (x2 - x1) : (x1 - x2) ;
    }

    /**
     *  The calculated height is always positive.  Follows the conventions of
     *  the xy_to_rect_get() function.
     */

    static int calculated_height (int y1, int y2)
    {
        return (y1 < y2) ? (y2 - y1) : (y1 - y2) ;
    }

};          // class rect

}           // namespace seq66

#endif      // SEQ66_RECT_HPP

/*
 * rect.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


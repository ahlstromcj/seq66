#if ! defined SEQ66_QROLLFRAME_HPP
#define SEQ66_QROLLFRAME_HPP

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
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          qrollframe.hpp
 *
 *  This module declares/defines the base class for drawing on the piano
 *  roll of the patterns editor.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-07-29
 * \updates       2019-08-03
 * \license       GNU GPLv2 or above
 *
 *  We are currently moving toward making this class a base class.
 */

/*
 * Forward references
 */

class QPixmap;

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  A class that manages the information for a single pixmap frame.
 */

class qrollframe
{
private:

    QPixmap * m_grid_pixmap;                /* not std::unique_ptr<QPixmap> */
    bool m_rendering;
    int m_frame_number;
    int m_frame_width;
    int m_frame_height;
    int m_bar_width;
    int m_x_0;
    int m_x_current;
    int m_x_1;

public:

    qrollframe (int barwidth = 1);
    qrollframe (const qrollframe & rhs) = delete;
    ~qrollframe ();

    QPixmap * grid ()
    {
        return m_grid_pixmap;
    }

    const QPixmap * grid () const
    {
        return m_grid_pixmap;
    }

    int width () const
    {
        return m_frame_width;
    }

    int height () const
    {
        return m_frame_height;
    }

    int bar_width () const
    {
        return m_bar_width;
    }

    int x0 () const
    {
        return m_x_0;
    }

    int x () const
    {
        return m_x_current;
    }

    int x1 () const
    {
        return m_x_1;
    }

    int frame () const
    {
        return m_frame_number;
    }

    bool rendering () const
    {
        return m_rendering;
    }

    bool change_frame (int f)
    {
#if defined SEQ66_SEQROLL_PLAYHEAD_RENDER
        bool result = f != m_frame_number || m_rendering;
#else
        bool result = f != m_frame_number;
#endif
        m_frame_number = f;
        return result;
    }

    void set_x (int x)
    {
        m_x_current = x;
    }

    bool regenerate (const QRect & r, QWidget * widget);
    bool resize (int w, int h);
    void restore_bar_area (QPainter & painter, int progx, int progy);
    void dump () const;

};          // class qrollframe

}           // namespace seq66

#endif      // SEQ66_QROLLFRAME_HPP

/*
 * qrollframe.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


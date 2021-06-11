#if ! defined SEQ66_VICTOR_HPP
#define SEQ66_VICTOR_HPP

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
 * \file          victor.hpp
 *
 *  This module declares a two dimensional vector class.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-30
 * \updates       2018-11-30
 * \license       GNU GPLv2 or above
 *
 *  What's your vector, Victor?
 *
 *  Smells like <sniff sniff> ... victory!
 *
 *  This class is a wrapper for a vector of vectors.  However, it also
 *  enforces some restrictions on the vectors:
 *
 *      -   The number of rows is specified at construction time; the size of
 *          the row vector is set at construction time; and the row vector's
 *          size stays constant throughout its lifetime.
 *      -   The number of columns is specified at construction time; the size
 *          of the all column vector is set at construction time; the column
 *          vector's size stays constant throughout its lifetime; and all
 *          columns are the same size.
 *
 *  Think of seq66::victor as an easier implementation of a two-dimensional
 *  array.
 */

#include <vector>

#include "util/basic_macros.hpp"        /* insure build macros defined      */

/*
 *  This namespace is not documented because it screws up the document
 *  processing done by Doxygen.
 */

namespace seq66
{

/**
 *  This 2-D vector class is meant only for storing and retrieving values.
 *  It does not support math operations. As per the following link, it uses
 *  operator () rather than trying to cobble up an operator [][].
 *
 *  https://isocpp.org/wiki/faq/operator-overloading#matrix-array-of-array
 *
 *  Also, out-of-range indices are simply ignored without error.  The only
 *  indication possible is to use an impossible value (for the caller's usage)
 *  and pass it in as the default value at construction time.
 */

template <typename NUMERIC>
class victor
{

private:

    std::vector < std::vector<NUMERIC> > m_vector_2d;

    const NUMERIC m_default_value;

    const size_t m_rows;

    const size_t m_columns;

public:

    /**
     *  Creates the 2-D array of values, setting them all to 0.
     */

    victor (size_t rows, size_t columns, NUMERIC dvalue = 0) :
        m_vector_2d
        (
            rows, std::vector<NUMERIC>(columns, dvalue) /* SIMPLE WAY */
        ),
        m_default_value (dvalue),
        m_rows          (rows),
        m_columns       (columns)
    {
#if defined SIMPLE_WAY_DOESNT_WORK
        std::vector columnvector(columns, dvalue);
        m_vector_2d.resize(rows);
        for (size_t i = 0; i < rows; ++i)
        {
            m_vector_2d[i] = columnvector;
        }
#endif
    }

    bool set (size_t r, size_t c, NUMERIC value)
    {
        bool result = (r < m_rows) && (c < m_columns);
        if (result)
            m_vector_2d[r].operator[](c) = value;

        return result;
    }

    NUMERIC & operator () (size_t r, size_t c)
    {
        static NUMERIC s_dummy = m_default_value;
        return(r < m_rows) && (c < m_columns) ?
            m_vector_2d[r].operator[](c) : s_dummy ;
    }

    NUMERIC operator () (size_t r, size_t c) const
    {
        return (r < m_rows) && (c < m_columns) ?
            m_vector_2d[r].operator[](c) : m_default_value ;
    }

};              // class victor

}               // namespace seq66

#endif          // SEQ66_VICTOR_HPP

/*
 * victor.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


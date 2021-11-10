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
 * \file          mutegroup.cpp
 *
 *  This module declares a two dimensional vector class solely to hold the
 *  mute status of a number of sequences in a set.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-12-01
 * \updates       2021-11-01
 * \license       GNU GPLv2 or above
 *
 *  This class manages one of the lines in the "[mute-group]" section of the
 *  new "rc" files:
 *
\verbatim
     ----- Screenset/bank number
    |
    |  --- Row 0         --- Row 1         --- Row 2         --- Row 3
    | |                 |                 |                 |
    v v                 v                 v                 v
    0 [0 0 0 0 0 0 0 0] [0 0 0 0 0 0 0 0] [0 0 0 0 0 0 0 0] [0 0 0 0 0 0 0 0]
       ^ ^ ^ . . .       ^                 ^                 ^             ^
       | | |             |                 |                 |             |
       | | |              --- Loop 8        --- Loop 16       --- Loop 24  |
       | | |                                                      Loop 31 -
       | |  --- Loop 2
       |  ----- Loop 1
        ------- Loop 0
\endverbatim
 *
 *  The stanza above describes the default Seq66/Seq64/Seq24 setup, where a
 *  set consists of 4 rows and 8 columns of 32 patterns.
 *
 *  How can we handle using sets of a larger size?
 *
 *  The following might work for a 8-column x 8 row = 64 screen-set.
 *
\verbatim
    0 [0 0 0 0 0 0 0 0] [0 0 0 0 0 0 0 0] [0 0 0 0 0 0 0 0] [0 0 0 0 0 0 0 0]
    0 [0 0 0 0 0 0 0 0] [0 0 0 0 0 0 0 0] [0 0 0 0 0 0 0 0] [0 0 0 0 0 0 0 0]
\endverbatim
 *
 *  The following might work for a 16-column x 4 row = 64 screen-set.
 *
\verbatim
    0 [0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0] [0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0]
    0 [0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0] [0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0]
\endverbatim
 *
 *  What if we want to swap rows and columns?
 *
\verbatim
    0 [0 0 0 0 ] [ 0 0 0 0] [0 0 0 0 ] [ 0 0 0 0]
    0 [0 0 0 0 ] [ 0 0 0 0] [0 0 0 0 ] [ 0 0 0 0]
\endverbatim
 */

#include <iostream>                     /* std::cout                        */

#include "cfg/settings.hpp"             /* seq66::usr()                     */
#include "play/mutegroup.hpp"           /* seq66::mutegroup class           */
#include "util/strfunctions.hpp"        /* seq66::write_stanza_bits()       */

/*
 *  This namespace is not documented because it screws up the document
 *  processing done by Doxygen.
 */

namespace seq66
{

/**
 *  Principal constructor.
 */

mutegroup::mutegroup (mutegroup::number group, int rows, int columns) :
    m_name              ("Group"),
    m_group_state       (false),
    m_group_size        (int(rows * columns)),          /* order important   */
    m_mutegroup_vector  (m_group_size, midibool(false)),
    m_rows              (rows),
    m_columns           (columns),
    m_swap_coordinates  (usr().swap_coordinates()),
    m_group             (group >= 0 ? group : 0),
    m_group_offset      (m_group * m_group_size)
{
    m_name += " ";
    m_name += std::to_string(int(group));
}

/**
 *  Copies the given bits-vector into the mute-group vector.
 *
 * \param bits
 *      The vector of bits to be copied.  The length of this vector is checked.
 *
 * \return
 *      Returns true if the \a bits parameter was of the proper length.
 */

bool
mutegroup::set (const midibooleans & bits)
{
    bool result = bits.size() == size_t(m_group_size);
    if (result)
        m_mutegroup_vector = bits;

    return result;
}

/**
 *  Clears the mute-group vector and refills it with values of "unarmed"
 *  (false).
 */

void
mutegroup::clear ()
{
    m_mutegroup_vector.clear();
    m_mutegroup_vector.reserve(m_group_size);
    for (auto & mg : m_mutegroup_vector)
        mg = midibool(false);
}

/**
 *  Checks to see if the mute-group is worth being saved.
 *
 *  \return
 *      Returns true if any of the "bits" in the mutegroup is set to armed
 *      (true).
 */

bool
mutegroup::any () const
{
    bool result = false;
    for (auto mg : m_mutegroup_vector)
    {
        if (bool(mg))
        {
            result = true;
            break;
        }
    }
    return result;
}

/**
 *  Counts the number of armed mute settings in the vector.
 *
 *  \return
 *      Returns true if any of the "bits" in the mutegroup is set to armed
 *      (true).
 */

int
mutegroup::armed_count () const
{
    int result = 0;
    for (auto mg : m_mutegroup_vector)
    {
        if (bool(mg))
            ++result;
    }
    return result;
}

/**
 *  Calculates the row and column for a given index.
 *
 *  Compare to screenset::index_to_grid().
 *
 * \param index
 *      This is the group or pattern number which, by default, can range
 *      from 0 to 31 (same as the size of a mute group is set to).
 *
 * \return
 *      Returns true if the calculation is valid and the result can be
 *      used.
 */

bool
mutegroup::mute_to_grid (int group, int & row, int & column) const
{
    int offset = group - int(m_group_offset);
    bool result = offset >= 0 && offset < int(m_group_size);
    if (result)
    {
        if (swap_coordinates())
        {
            row = group / m_columns;
            column = group % m_columns;
        }
        else
        {
            row = group % m_rows;
            column = group / m_rows;
        }
    }
    return result;
}

int
mutegroup::grid_to_mute (int row, int column)
{
    if (row < m_rows && column < m_columns)
    {
        if (swap_coordinates())
            return m_group_offset + column + m_columns * row;
        else
            return m_group_offset + row + m_rows * column;
    }
    else
        return 0;
}

/**
 *  Gets the muting value, actually whether the loop is to be armed or not,
 *  for the given index.  It is used in controlling the active set.
 *
 * \param index
 *      The offset into the mute-group vector of booleans.
 *
 * \return
 *      Returns true if the index was valid and the mute-group bit was set.
 */

bool
mutegroup::armed (int index) const
{
    bool result = index >= 0 && index < m_group_size;
    if (result)
        return bool(m_mutegroup_vector[index]);

    return result;
}

void
mutegroup::armed (int index, bool flag)
{
    if (index >= 0 && index < m_group_size)
        m_mutegroup_vector[index] = flag;
}

/**
 *  Just a simple display of a mute group.
 */

void
mutegroup::show () const
{
    std::string stanzabits = write_stanza_bits(get());  /* get midibooleans */
    std::cout
        << "Group #" << group()
        << " " << stanzabits
        << " " << name() << std::endl
        ;
}

}               // namespace seq66

/*
 * mutegroup.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


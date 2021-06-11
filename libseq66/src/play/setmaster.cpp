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
 * \file          setmaster.cpp
 *
 *  This module declares a class to manage a number of screensets.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2020-08-10
 * \updates       2021-04-15
 * \license       GNU GPLv2 or above
 *
 *  Implements setmaster.
 */

#include <iostream>                     /* std::cout                        */
#include <sstream>                      /* std::ostringstream               */

#include "play/setmaster.hpp"           /* seq66::setmaster class           */

/*
 *  This namespace is not documented because it screws up the document
 *  processing done by Doxygen.
 */

namespace seq66
{

/**
 *  Creates a manager for all of the sets in a tune, at set level.  It also
 *  provides access to the container of sets and to the currently-selected set,
 *  called the "play-screen".
 *
 *  After creation, screenset 0 is created and set as the play-screen.
 */

setmaster::setmaster (int setrows, int setcolumns) :
    m_screenset_rows        (setrows),
    m_screenset_columns     (setcolumns),
    m_rows                  (c_rows),                   /* constant         */
    m_columns               (c_columns),                /* constant         */
    m_set_count             (m_rows * m_columns),
    m_highest_set           (-1),
    m_container             ()                          /* screensets map   */
{
    (void) reset();
}

/**
 *  Resets back to the constructor set.  This means we have one set, the empty
 *  play-screen, plus a "dummy" set.
 */

bool
setmaster::reset ()
{
    clear();
    auto setp = add_set(0);
    bool result = setp != m_container.end();
    if (result)
        (void) add_set(screenset::limit());         /* create the dummy set */

    return result;
}

/**
 *  Returns the set number for the given row and column.  Remember that the
 *  layout of sets matches that of sequences.  See the top banner of the
 *  setmaster.cpp file.  This function is useful for picking the correct set in
 *  the qsetmaster button array.
 *
 * Set Layout:
 *
 *  Like the sequences in the main (live) window, the set numbers are
 *  transposed, so that the set number increments vertically, not horizontally:
 *
\verbatim
    0   4   8   12  16  20  24  28
    1   5   9   13  17  21  25  29
    2   6   10  14  18  22  26  30
    3   7   11  15  19  23  27  31
\endverbatim
 *
 *  However, this grid never changes.  There's a strong dependency on the
 *  number of keys we can devote to this function (32) and the 4-rows by
 *  8-columns heritage of Seq24.
 *
 * \param row
 *      Provides the desired row, clamped to a legal value.
 *
 * \param row
 *      Provides the desired row, clamped to a legal value.
 *
 * \return
 *      Returns the calculated set number, which will range from 0 to
 *      (m_rows * m_columns) - 1 = m_set_count.  If out of range,
 *      set 0 is returned.
 */

screenset::number
setmaster::calculate_set (int row, int column) const
{
    if (row < 0 || row >= m_rows || column < 0 || column >= m_columns)
        return 0;
    else
        return m_rows * column + row;
}

bool
setmaster::calculate_coordinates
(
    screenset::number setno, int & row, int & column
)
{
    bool result = is_screenset_valid(setno);
    if (result)
    {
        row = setno % m_screenset_rows;
        column = setno / m_screenset_rows;
    }
    return result;
}

/**
 *  Creates and adds a screenset to the container.
 *
 *  This code revealed that, when items (screenset, seq, etc.) are to be
 *  stored in containers, it is best to not have const data member.  These
 *  make the assignment operator or constructor ill-formed, so that the
 *  compiler deletes these functions!  Then, calling std::make_pair() results
 *  in very mysterious error messages.
 *
 *  Auto types:
 *
 *      -   std::pair<screenset::number, screenset>
 *      -   std::pair<container::iterator, bool>
 */

setmaster::container::iterator
setmaster::add_set (screenset::number setno)
{
    screenset newset(setno, m_screenset_rows, m_screenset_columns);
    auto setpair = std::make_pair(setno, newset);
    auto resultpair = m_container.insert(setpair);
    if (resultpair.second)
    {
        if (setno > m_highest_set && setno != screenset::limit())
            m_highest_set = setno;
    }
    return resultpair.first;
}

/**
 *  Given a set number, counts through the container until it finds the matching
 *  set number.  We have to brute-force the lookup because there may be
 *  unoccupied set-slots in between occupied set-slots.
 */

int
setmaster::screenset_index (screenset::number setno) const
{
    int result = int(seq::unassigned());
    int index = 0;
    for (const auto & sset : m_container)
    {
        if (sset.second.set_number() == setno)
        {
            result = index;
            break;
        }
        ++index;
    }
    return result;
}

/**
 *  For each screenset that exists, execute a set-handler function.
 */

bool
setmaster::set_function (screenset::sethandler s)
{
    bool result = false;
    screenset::number index = 0;
    for (auto & sset : m_container)                 /* screenset reference  */
    {
        if (sset.second.usable())
        {
            result = sset.second.set_function(s, index++);
            if (! result)
                break;
        }
    }
    return result;
}

/**
 *  Runs a set-handler and a slot-handler for each set.
 */

bool
setmaster::set_function (screenset::sethandler s, screenset::slothandler p)
{
    bool result = false;
    for (auto & sset : m_container)                 /* screenset reference  */
    {
        if (sset.second.usable())
        {
            result = sset.second.set_function(s, p);
            if (! result)
                break;
        }
    }
    return result;
}

/**
 *  Runs only a slot-handler for each slot (pattern) in each set.
 */

bool
setmaster::set_function (screenset::slothandler p)
{
    bool result = false;
    for (auto & sset : m_container)                 /* screenset reference  */
    {
        if (sset.second.usable())
        {
            result = sset.second.slot_function(p);
            if (! result)
                break;
        }
    }
    return result;
}

/**
 *  Does a brute-force lookup of the given set number, obtained by
 *  screenset::set_number().  We must use the long form of the for loop here.
 *  as far as we can tell.
 */

setmaster::container::iterator
setmaster::find_by_value (screenset::number setno)
{
    auto result = m_container.end();
    for (auto it = m_container.begin(); it != m_container.end(); ++it)
    {
        if (it->second.set_number() == setno)
        {
            result = it;
            break;
        }
    }
    return result;
}

/**
 * \tricky
 *      For use in the qsetmaster set-list, we need to look up the iterator to
 *      a set by value, not by key, because after the first swap there is no
 *      longer a correspondence between the key and the actual set-number
 *      value.
 */

bool
setmaster::swap_sets (seq::number set0, seq::number set1)
{
    auto item0 = find_by_value(set0);
    auto item1 = find_by_value(set1);
    bool result = item0 != m_container.end() && item1 != m_container.end();
    if (result)
    {
        std::swap(item0->second, item1->second);
        item0->second.change_set_number(set1);      /* also changes seq #s  */
        item1->second.change_set_number(set0);      /* also changes seq #s  */
    }
    return result;
}

std::string
setmaster::sets_to_string (bool showseqs) const
{
    std::ostringstream result;
    result << "Sets" << (showseqs ? " and Sequences:" : ":") << std::endl;
    for (auto & s : m_container)
    {
        int keyvalue = int(s.first);
        if (keyvalue < screenset::limit())
        {
            result << "  Key " << int(s.first) << ": ";
            if (s.second.usable())
                result << s.second.to_string(showseqs);
            else
                result << std::endl;
        }
    }
    return result.str();
}

void
setmaster::show (bool showseqs) const
{
    std::cout << sets_to_string(showseqs);
}

}               // namespace seq66

/*
 * setmaster.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


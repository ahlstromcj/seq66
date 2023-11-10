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
 * \updates       2023-11-10
 * \license       GNU GPLv2 or above
 *
 *  Implements setmaster.  The difference between the setmaster and setmapper
 *  is that setmaster provides a constant number of sets (4 x 8) used for the
 *  managerment of sets, while setmapper translates between the performer and
 *  the playing (current) screenset.
 */

#include <iostream>                     /* std::cout                        */
#include <sstream>                      /* std::ostringstream               */

#include "cfg/settings.hpp"             /* seq66::usr()                     */
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
    m_swap_coordinates      (usr().swap_coordinates()),
    m_set_count             (m_rows * m_columns),
    m_highest_set           (-1),
    m_container             ()                          /* #, screenset map */
{
    if (! reset())
        error_message("setmaster", "reset() failed()");
}

/**
 *  Resets back to the constructor set.  This means we have one set, the empty
 *  play-screen, plus a "dummy" set.
 */

bool
setmaster::reset ()
{
    bool result = false;
    clear();
    for (int i = 0; i < Size(); ++i)
    {
        auto setp = add_set(screenset::number(i));
        result = setp != m_container.end();
        if (! result)
            break;
    }
    if (result)
    {
        auto setp = add_set(screenset::limit());    /* create the dummy set */
        result = setp != m_container.end();
    }
    return result;
}

/**
 *  Returns the set number for the given row and column.  Remember that the
 *  layout of sets matches that of sequences.  See the top banner of the
 *  setmaster.cpp file.  This function is useful for picking the correct set in
 *  the qsetmaster button array.
 *
 *  Compare to screenset::grid_to_seq().
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
 *  This layout can also be "swapped" so that increasing pattern numbers go from
 *  left to right instead of from to bottom.
 *
 *  However, this grid never changes.  There's a strong dependency on the number
 *  of keys we can devote to this function (32) and the 4-rows by 8-columns
 *  heritage of Seq24.
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
setmaster::grid_to_set (int row, int column) const
{
    if (row < 0 || row >= m_rows || column < 0 || column >= m_columns)
        return 0;
    else if (swap_coordinates())
        return column + m_columns * row;
    else
        return row + m_rows * column;
}

/**
 *  Compare to screenset::index_to_grid().  Fixes found while fixing
 *  issue #87.
 */

bool
setmaster::index_to_grid (screenset::number setno, int & row, int & column)
{
    bool result = is_screenset_valid(setno);
    if (result)
    {
        if (swap_coordinates())
        {
            row = setno / m_columns;                // not m_screenset_columns
            column = setno % m_columns;             // not m_screenset_columns
        }
        else
        {
            row = setno % m_rows;                   // not m_screenset_rows
            column = setno / m_rows;                // not m_screenset_rows
        }
    }
    return result;
}

/**
 *  Creates and adds an "empty" (i.e. no activated sequences) screenset to
 *  the container.
 *
 *  This code revealed that, when items (screenset, seq, etc.) are to be
 *  stored in containers, it is best to not have a const data member.  These
 *  make the assignment operator or constructor ill-formed, so that the
 *  compiler deletes these functions!  Then, calling std::make_pair() results
 *  in very mysterious error messages.
 *
 *  If remove any existing set at the given set number, and add a new one.
 *
 *  Auto types:
 *
 *      -   std::pair<screenset::number, screenset>
 *      -   std::pair<container::iterator, bool>
 */

setmaster::container::iterator
setmaster::add_set (screenset::number setno)
{
    (void) m_container.erase(setno);            /* remove the existing item */

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
 *  Removes a set, replacing it with an empty screenset.
 *  This basically just clears the set, and should (must check) remove
 *  it's sequences.
 */

bool
setmaster::remove_set (screenset::number setno)
{
    auto item = find_by_value(setno);
    bool result = item != m_container.end();
    if (result)
    {
        (void) m_container.erase(item);

        screenset newset(setno, m_screenset_rows, m_screenset_columns);
        auto setpair = std::make_pair(setno, newset);
        auto resultpair = m_container.insert(setpair);
        result = resultpair.second;
    }
    return result;
}

screenset &
setmaster::screen (screenset::number setno)
{
    auto sp = m_container.find(setno);          /* look it up in the map    */
    return sp != m_container.end() ? sp->second : dummy_screenset();
}

const screenset &
setmaster::screen (screenset::number setno) const
{
    auto sp = m_container.find(setno);          /* look it up in the map    */
    return sp != m_container.end() ? sp->second : dummy_screenset();
}

bool
setmaster::any_in_edit () const
{
    bool result = false;
    for (const auto & sset : m_container)
    {
        if (sset.second.any_in_edit())
        {
            result = true;
            break;
        }
    }
    return result;
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
setmaster::exec_set_function (screenset::sethandler s)
{
    bool result = false;
    screenset::number index = 0;
    for (auto & sset : m_container)                 /* screenset reference  */
    {
        if (sset.second.usable())
        {
            result = sset.second.exec_set_function(s, index++);
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
setmaster::exec_set_function (screenset::sethandler s, screenset::slothandler p)
{
    bool result = false;
    for (auto & sset : m_container)                 /* screenset reference  */
    {
        if (sset.second.usable())
        {
            result = sset.second.exec_set_function(s, p);
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
setmaster::exec_set_function (screenset::slothandler p)
{
    bool result = false;
    for (auto & sset : m_container)                 /* screenset reference  */
    {
        if (sset.second.usable())
        {
            result = sset.second.exec_slot_function(p);
            if (! result)
                break;
        }
    }
    return result;
}

/**
 *  Does a brute-force lookup of the given set number, obtained by screenset
 *  :: set_number().  We must use the long form of the for loop here, as far
 *  as we can tell.
 *
 *  ca 2023-11-09 Why the heck not just use std::map::find()?
 *
 *  We could also do a find() on the map and return a dummy screenset, but
 *  the setmaster::swap_sets() function requires this brute-force lookup.
 *
 *  ca 2023-11-09 Not true!
 */

setmaster::container::iterator
setmaster::find_by_value (screenset::number setno)
{
    return m_container.find(setno);
}

/**
 * \tricky
 *      For use in the qsetmaster set-list, we need to look up the iterator to
 *      a set by value, not by key, because after the first swap there is no
 *      longer a correspondence between the key and the actual set-number
 *      value.
 *
 * TODO: copy the screensets, delete them, then reinsert them with their new
 * keys.
 */

bool
setmaster::swap_sets (screenset::number set0, screenset::number set1)
{
    auto item0 = find_by_value(set0);
    auto item1 = find_by_value(set1);
    bool result = item0 != m_container.end() && item1 != m_container.end();
    if (result)
    {
#if defined SEQ66_PLATFORM_DEBUG_TMI
        printf("BEFORE swap\n");
        show(true, 8);
#endif
        screenset copy0{item0->second};
        screenset copy1{item1->second};
        copy0.change_set_number(set1);              /* also changes seq #s  */
        copy1.change_set_number(set0);              /* also changes seq #s  */
        (void) m_container.erase(item0);
        (void) m_container.erase(item1);

        auto setpair = std::make_pair(set1, copy0);
        auto resultpair = m_container.insert(setpair);
        result = resultpair.second;
        if (result)
        {
            setpair = std::make_pair(set0, copy1);
            resultpair = m_container.insert(setpair);
            result = resultpair.second;
        }
#if defined SEQ66_PLATFORM_DEBUG_TMI
        printf("AFTER swap\n");
        show(true, 8);
#endif
    }
    return result;
}

std::string
setmaster::sets_to_string (bool showseqs, int limit) const
{
    std::ostringstream result;
    result << "Sets" << (showseqs ? " and Sequences:" : ":") << std::endl;
    int counter = 0;
    for (auto & s : m_container)
    {
        int keyvalue = int(s.first);
        if (keyvalue < screenset::limit())
        {
            result << "  Key " << int(s.first) << ": ";
            if (s.second.usable())
                result << s.second.to_string(showseqs, limit);
            else
                result << std::endl;
        }
        if (++counter == limit)
            break;
    }
    return result.str();
}

void
setmaster::show (bool showseqs, int limit) const
{
    std::cout << sets_to_string(showseqs, limit);
}

}               // namespace seq66

/*
 * setmaster.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


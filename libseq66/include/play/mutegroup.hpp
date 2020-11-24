#if ! defined SEQ66_MUTEGROUP_HPP
#define SEQ66_MUTEGROUP_HPP

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
 * \file          mutegroup.hpp
 *
 *  This module declares a linear vector class solely to hold the
 *  mute status of a number of sequences in a set, while also able to access
 *  patterns/loops by row, column.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-12-01
 * \updates       2020-11-24
 * \license       GNU GPLv2 or above
 *
 */

#include <functional>                   /* std::function, function objects  */
#include <vector>

#include "app_limits.h"                 /* default rows and columns macros  */
#include "midi/midibytes.hpp"           /* seq66::midibool                  */

/**
 *  Mutes sizes.
 */

#define SEQ66_MUTE_ROWS                    4
#define SEQ66_MIN_MUTE_ROWS                4
#define SEQ66_MAX_MUTE_ROWS                4
#define SEQ66_MUTE_COLUMNS                 8
#define SEQ66_MIN_MUTE_COLUMNS             8
#define SEQ66_MAX_MUTE_COLUMNS             8

/*
 *  This namespace is not documented because it screws up the document
 *  processing done by Doxygen.
 */

namespace seq66
{

/**
 *  Provides a class that represents an array the same size as a screenset, but
 *  holding armed statuses that can be saved an applied later.
 */

class mutegroup
{

public:

    /**
     *  A revealing alias for mutegroup numbers.
     */

    using number = int;

    /**
     *  Provides an alias for functions that can be called on all groups.
     *  We might end up not using this, letting the settmapper do the work.
     */

    using grouphandler = std::function<bool (mutegroup &, mutegroup::number)>;

    /**
     *  Provides an alias for a group number
     *
     * static const int MUTEGROUP_DEFAULT = ROWS_DEFAULT * COLS_DEFAULT;
     */

    static const int ROWS_DEFAULT = SEQ66_DEFAULT_SET_ROWS;
    static const int COLS_DEFAULT = SEQ66_DEFAULT_SET_COLUMNS;

private:

    /**
     *  Provides a mnemonic name for the group.  By default, it is of the
     *  format "Group 1", and currently not modifiable.
     */

    std::string m_name;

    /**
     *  Indicates the current state of the mute-group, either on or off.
     *  Useful in toggling.
     */

    mutable bool m_group_state;

    /**
     *  The number of loops/patterns in the mute-group.  Saves a calculation
     *  of row x column.  It is important to note the the size of the group is
     *  constant throughout its lifetime (and the lifetime of the
     *  application).
     */

    const int m_group_size;

    /**
     *  Holds a set of boolean values in a 1-D vector, but can be virtually
     *  arranged by row and column.  Note that we use midibool rather than
     *  bool, to avoid bitset issues.
     */

    midibooleans m_mutegroup_vector;

    /**
     *  Indicates the number of virtual rows in a screen-set (bank), which is
     *  also the same number of virtual rows as a mute-group.  This value will
     *  generally be the same as the size used in the rest of the application.
     *  The default value is the historical value of 4 rows per set or
     *  mute-group.
     */

    const int m_rows;

    /**
     *  Indicates the number of virtual columns in a screen-set (bank), which
     *  is also the same number of virtual columns as a mute-group.  This
     *  value will generally be the same as the size used in the rest of the
     *  application.  The default value is the historical value of 8 columns
     *  per set or mute-group.
     */

    const int m_columns;

    /**
     *  Indicates the group (akin to the set or bank number) represented by
     *  this mutegroup object.
     */

    const number m_group;

    /**
     *  Indicates the screen-set offset (the number of the first loop/pattern
     *  in the screen-set).  This value is m_group_count * m_group.  This
     *  saves a calculation.
     */

    const int m_group_offset;

public:

    /*
     *  Creates the vector of values, setting them all to 0 (false).
     */

    mutegroup
    (
        number group    = 0,
        int rows        = SEQ66_DEFAULT_SET_COLUMNS,
        int columns     = SEQ66_DEFAULT_SET_ROWS
    );

    /*
     * The move and copy constructors, the move and copy assignment operators,
     * and the destructors are all compiler generated.
     */

    mutegroup (const mutegroup &) = default;
    mutegroup & operator = (const mutegroup &) = default;
    mutegroup (mutegroup &&) = default;
    mutegroup & operator = (mutegroup &&) = default;
    ~mutegroup () = default;

    /**
     *  Checks if a the sequence number is an assigned one, i.e. not equal to
     *  -1.
     */

    static bool none (number group)
    {
        return group == (-1);
    }

    /**
     *  Indicates that a sequence number has not been assigned.
     */

    static number unassigned ()
    {
        return (-1);
    }

    bool group_state () const
    {
        return m_group_state;
    }

    void group_state (bool f)
    {
        m_group_state = f;
    }

    int count () const
    {
        return int(m_mutegroup_vector.size());
    }

    int armed_count () const;
    bool armed (int index) const;
    void armed (int index, bool flag);

    bool muted (int index) const
    {
        return ! armed(index);
    }

    bool set (const midibooleans & bits);

    const midibooleans & zeroes () const
    {
        static midibooleans s_bits(m_group_size, midibool(false));
        return s_bits;
    }

    const midibooleans & get () const
    {
        return m_mutegroup_vector;
    }

    const std::string & name () const
    {
        return m_name;
    }

    void name (const std::string & n)
    {
        m_name = n;
    }

    number group () const
    {
        return m_group;
    }

    int rows () const
    {
        return m_rows;
    }

    int columns () const
    {
        return m_columns;
    }

    bool any () const;
    void clear ();
    void show () const;

private:

    bool coordinate (int index, int & row, int & column) const;

    /**
     *  Calculates the group index (i.e. a pattern number) given by the rows,
     *  columns, and the group-offset value.
     *
     * \return
     *      Returns 0 if the row or column were illegal.  But 0 is also a
     *      legal value.
     */

    int index (int r, int c)
    {
        return (r < m_rows) && (c < m_columns) ?
            int(m_group_offset + r * c) : 0 ;
    }

};              // class mutegroup

}               // namespace seq66

#endif          // SEQ66_MUTEGROUP_HPP

/*
 * mutegroup.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


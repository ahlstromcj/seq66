#if ! defined SEQ66_SETMASTER_HPP
#define SEQ66_SETMASTER_HPP

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
 * \file          setmaster.hpp
 *
 *  This module declares a small manager for a set of sets.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2020-08-10
 * \updates       2021-11-13
 * \license       GNU GPLv2 or above
 *
 *  The setmaster class is meant to encapsulate the sets and their layout,
 *  without performing any functions related to patterns.  This new class was
 *  created because we found some confusion, in the setmapper, between the
 *  size of a set versus the size of the set of sets.
 *
 *  The size of a pattern set can vary widely based on user preferences, but
 *  the size of the set of sets managed by the setmaster is hard-wired to 4 x
 *  8.
 */

#include <map>                          /* std::map<>                       */

#include "play/screenset.hpp"           /* seq66::screenset and seq         */

/*
 *  This namespace is not documented because it screws up the document
 *  processing done by Doxygen.
 */

namespace seq66
{

/**
 *  Provides a class for managing screensets.  Much of the action will occur
 *  in the selected play-screen.
 */

class setmaster
{
    friend class performer;             /* a very good friend to have   */
    friend class setmapper;             /* also a good friend/buddy     */

private:

    using container = std::map<screenset::number, screenset>;

    /**
     *  Holds the number of rows to use when creating a new set.  We could use
     *  the value in setmapper, but we might want the user-interface to create
     *  sets directly at some point.  This value along with m_screenset_columns
     *  provides the size of a screenset, which can vary from the default 4 x 8
     *  via configuration options.
     */

    int m_screenset_rows;

    /**
     *  Holds the number of columns to use when creating a new set.  We could
     *  use the value in setmapper, but we might want the user-interface to
     *  create sets directly at some point.
     */

    int m_screenset_columns;

    /**
     *  Storage for the number of rows in the layout of the set-master. It
     *  defaults to 4 rows and is actually considered to be a constant.
     *  We removed the const qualifier to avoid issues with containers.
     */

    int m_rows;         /* const */

    /**
     *  Storage for the number of columns in the layout of the set-master. It
     *  defaults to 8 columns is actually considered to be a constant.
     *  We removed the const qualifier to avoid issues with containers.
     */

    int m_columns;      /* const */

    /**
     *  Experimental option to swap rows and columns for sets; see the similar
     *  swappage for screensets and its patterns.
     */

    bool m_swap_coordinates;

    /**
     *  The maximum number of sets supported.  The main purpose for this value
     *  is as a sanity check for set lookup, not necessarily for limiting the
     *  number of sets.
     */

    int m_set_count;

    /**
     *  The highest-numbered set that currently exists, whether empty or not.
     *  Does not include the dummy set.
     */

    int m_highest_set;

    /**
     *  Holds a vector of screenset objects.  This container starts out empty.
     */

    container m_container;

private:

    /**
     *  The base (or default) number of rows in a set, useful in handling the
     *  slot-shift feature and the set-master user-interface. Returned by the
     *  static Rows() function.
     */

    static const int c_rows = screenset::c_default_rows;

    /**
     *  The canonical and default set size.  Used in relation to the
     *  keystrokes used to access sequences (and mute-groups). Returned by the
     *  static Columns() function.
     */

    static const int c_columns = screenset::c_default_columns;

public:

    setmaster (int setrows = c_rows, int setcolumns = c_columns);

    /*
     * The move and copy constructors, the move and copy assignment operators,
     * and the destructors are all compiler generated.
     */

    setmaster (const setmaster &) = default;
    setmaster & operator = (const setmaster &) = default;
    setmaster (setmaster &&) = default;
    setmaster & operator = (setmaster &&) = default;
    ~setmaster () = default;

    static int Rows ()
    {
        return c_rows;
    }

    static int Columns ()
    {
        return c_columns;
    }

    static int Size ()
    {
        return c_rows * c_columns;
    }

    bool swap_coordinates () const
    {
        return m_swap_coordinates;
    }

    std::string set_to_string (screenset::number setno) const;
    std::string sets_to_string (bool showseqs = true, int limit = 0) const;
    void show (bool showseqs = true, int limit = 0) const;

    bool name (screenset::number setno, const std::string & nm)
    {
        return m_container.find(setno) != m_container.end() ?
            m_container.at(setno).name(nm) : false ;
    }

    bool is_screenset_active (screenset::number setno) const
    {
        return is_screenset_available(setno) ?
            m_container.at(setno).active() : false ;
    }

    bool is_screenset_available (screenset::number setno) const
    {
        return m_container.find(setno) != m_container.end();
    }

    bool is_screenset_valid (screenset::number setno) const
    {
        return setno >= 0 && setno < m_set_count;
    }

    int screenset_count () const
    {
        return int(m_container.size()) - 1;     /* ignore the dummy set */
    }

    int screenset_active_count () const;

    int highest_set () const
    {
        return m_highest_set;
    }

    int screenset_max () const
    {
        return m_set_count;
    }

    int screenset_index (screenset::number setno) const;
    bool swap_sets (screenset::number set0, screenset::number set1);
    bool any_in_edit () const;

private:

    screenset::number grid_to_set (int row, int column) const;
    bool index_to_grid (screenset::number setno, int & row, int & column);

    bool inside_set (int row, int column) const
    {
        return (row >= 0) && (row < m_rows) &&
            (column >= 0) && (column < m_columns);
    }

    void clear ()
    {
        m_container.clear();                    /* unconditional zappage!   */
    }

    int rows () const
    {
        return m_rows;
    }

    int columns () const
    {
        return m_columns;
    }

    /*
     * exec_set_function(s) executes a set-handler for each set.
     * exec_set_function(s,p) runs a set-handler and a slot-handler for each
     * set.  exec_set_function(p) runs the slot-handler for all patterns in
     * all sets.  exec_slot_function() uses the play-screen, and so is in
     * setmapper, not here.
     */

    bool exec_set_function (screenset::sethandler s);
    bool exec_set_function (screenset::sethandler s, screenset::slothandler p);
    bool exec_set_function (screenset::slothandler p);

private:

    bool reset ();
    container::iterator add_set (screenset::number setno);
    container::iterator find_by_value (screenset::number setno);
    bool remove_set (screenset::number setno);
    bool clear_set (screenset::number setno);

    /**
     *  Clamps a screenset number to the range of 0 to one less than
     *  m_set_count.
     */

    screenset::number clamp (screenset::number offset) const
    {
        if (offset < 0)
            return 0;
        else if (offset >= m_set_count)
            return m_set_count - 1;

        return offset;
    }

    screenset & screen (screenset::number setno);
    const screenset & screen (screenset::number setno) const;

    screenset & dummy_screenset ()
    {
        return m_container.at(screenset::limit());
    }

    const screenset & dummy_screenset () const
    {
        return m_container.at(screenset::limit());
    }

    container & set_container ()        /* for setmapper and performer  */
    {
        return m_container;
    }

    const container & set_container () const
    {
        return m_container;
    }

};              // class setmaster

}               // namespace seq66

#endif          // SEQ66_SETMASTER_HPP

/*
 * setmaster.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


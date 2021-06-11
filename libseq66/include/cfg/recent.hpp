#if ! defined SEQ66_RECENT_HPP
#define SEQ66_RECENT_HPP

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
 * \file          recent.hpp
 *
 *  This module declares/defines a container for "recent" entries.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-03-29
 * \updates       2018-11-24
 * \license       GNU GPLv2 or above
 *
 */

#include <deque>
#include <string>

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  This class provides a standard container and operations for a recent-files
 *  list.  The container should, if possible, contain only unique strings...
 *  no file-path should be included more than once.
 */

class recent
{

private:

    /**
     *  Provide the type definition for the recent-files container.  It is
     *  currently a standard deque.  A deque (pronounced like "deck") is an
     *  irregular acronym of double-ended queue. Double-ended queues are
     *  sequence containers with dynamic sizes that can be expanded or
     *  contracted on both ends (either its front or its back), like a deck of
     *  cards where the dirty dealer can deal from the bottom.
     */

    using container = std::deque<std::string>;

private:

    /**
     *  Holds the list of recent files.
     */

    container m_recent_list;

    /**
     *  Holds the constraint on the number of recent files.  Usually a value
     *  like 10.
     */

    const int m_maximum_size;

public:

    recent ();
    recent (const recent & source) = default;
    recent & operator = (const recent & source);
    ~recent ();

    void clear ()
    {
        m_recent_list.clear();
    }

    int count () const
    {
        return int(m_recent_list.size());
    }

    int maximum () const
    {
        return m_maximum_size;
    }

    std::string get (int index) const;
    bool append (const std::string & item);
    bool add (const std::string & item);
    bool remove (const std::string & item);

};          // class recent

}           // namespace seq66

#endif      // SEQ66_RECENT_HPP

/*
 * recent.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


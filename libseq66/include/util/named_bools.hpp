#if ! defined SEQ66_NAMED_BOOLS_HPP
#define SEQ66_NAMED_BOOLS_HPP

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
 * \file          named_bools.hpp
 *
 *  This module provides a map of booleans using a string as a key value.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2021-09-13
 * \updates       2021-09-13
 * \license       GNU GPLv2 or above
 *
 *  This seems to be much easier for small sets of booleans that using an
 *  enumeration.
 */

#include <map>                          /* std::map container class         */
#include <string>                       /* std::string class                */

namespace seq66
{

/**
 *  A handy class to make it easy to look up and set a "small" number of
 *  boolean values by name.
 */

class named_bools
{
    using container = std::map<std::string, bool>;

private:

    /**
     *  Provides an associative container of booleans.
     */

    container m_container;

public:

    named_bools () : m_container ()
    {
        // no code
    }

    bool add (const std::string & name, bool value)
    {
        auto p = std::make_pair(name, value);
        auto r = m_container.insert(p);
        return r.second;
    }

    bool get (const std::string & name) const
    {
        auto r = m_container.find(name);
        return r != m_container.end() ? r->second : false ;
    }

    void set (const std::string & name, bool value = true)
    {
        auto r = m_container.find(name);
        if (r != m_container.end())
            r->second = value;
        else
            (void) add(name, value);
    }

    void clear ()
    {
        m_container.clear();
    }

    int count ()
    {
        return int(m_container.size());
    }

};          // class named_bools

}           // namespace seq66

#endif      // SEQ66_NAMED_BOOLS_HPP

/*
 * named_bools.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


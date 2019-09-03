#if ! defined SEQ66_COMMENTS_HPP
#define SEQ66_COMMENTS_HPP

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
 * \file          comments.hpp
 *
 *  This module declares/defines an object to hold "[comments]", which are
 *  present in three different classes now.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-26
 * \updates       2018-11-26
 * \license       GNU GPLv2 or above
 *
 */

#include <string>

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  This class is a wrapper for comments-management.
 */

class comments
{

private:

    /**
     *  [comments]
     *
     *  Provides a way to embed comments in the "rc" file and not lose
     *  them when the "rc" file is auto-saved.
     */

    std::string m_comments_block;

public:

    comments (const std::string & comtext = "");
    comments (const comments & rhs);
    comments & operator = (const comments & rhs);

    /**
     * \getter m_comments_block
     */

    const std::string & text () const
    {
        return m_comments_block;
    }

    /**
     * \setter m_comments_block
     */

    void clear ()
    {
        m_comments_block.clear();
    }

    /**
     * \setter m_comments_block
     */

    void set (const std::string & block)
    {
        m_comments_block = block;
    }

    /**
     * \setter m_comments_block
     */

    void append (const std::string & line)
    {
        m_comments_block += line;
    }

};          // class comments

}           // namespace seq66

#endif      // SEQ66_COMMENTS_HPP

/*
 * comments.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


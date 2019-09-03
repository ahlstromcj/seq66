/*
 *  This file is part of seq24/sequencer64.
 *
 *  seq24 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  seq24 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq24; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          comments.cpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  in this application.
 *
 * \library       sequencer64 application
 * \author        Chris Ahlstrom
 * \date          2018-11-26
 * \updates       2018-11-26
 * \license       GNU GPLv2 or above
 *
 */

#include "cfg/comments.hpp"             /* seq66::comments class            */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Default constructor.
 */

comments::comments (const std::string & comtext) :
    m_comments_block    (comtext)
{
    if (comtext.empty())
    {
        set
        (
    "(Comments added to this section are preserved.  Lines starting with\n"
    " a '#' or '[', or that are blank, are ignored.  Start lines that must\n"
    " be blank with a space.)\n"
        );
    }
}

/**
 *  Copy constructor.
 *
 * \param rhs
 *      The source of the data for the copy.
 */

comments::comments (const comments & rhs) :
    m_comments_block    (rhs.m_comments_block)
{
    // Empty body
}

/**
 *  Principal assignment operator.
 *
 * \param rhs
 *      The source of the data for the assignment.
 *
 * \return
 *      Returns a reference to the destination for use in serial assignments.
 */

comments &
comments::operator = (const comments & rhs)
{
    if (this != &rhs)
    {
        m_comments_block = rhs.m_comments_block;
    }
    return *this;
}

}           // namespace seq66

/*
 * comments.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


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
 * \file          midioperation.cpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  and functions for the extended MIDI control feature.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-13
 * \updates       2019-03-20
 * \license       GNU GPLv2 or above
 *
 */

#include <iostream>                     /* std::cout for show()             */

#include "ctrl/midioperation.hpp"       /* seq66::midioperation class       */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  This default constructor creates a "zero" object.  Every member is
 *  either false or some other form of zero.
 */

midioperation::midioperation () :
    m_op_name           (),
    m_op_category       (automation::category::none),
    m_op_number         (automation::slot::none),
    m_parent_function   ()
{
    // Empty body
}

/**
 *  This constructor assigns the basic values of control name, number, and
 *  action code.  The rest of the members can be set via the set() function.
 */

midioperation::midioperation
(
    const std::string & opname,
    automation::category opcategory,
    automation::slot opnumber,
    functor pfunction
) :
    m_op_name           (opname),
    m_op_category       (opcategory),
    m_op_number         (opnumber),
    m_parent_function   (pfunction)
{
    // Empty body
}

/**
 *
 */

void
midioperation::show () const
{
    std::cout
        << "Op  " << name()
        << " Cat  " << cat_name()
        << " Slot " << slot_name()
        << std::endl
        ;
}

}           // namespace seq66

/*
 * midioperation.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


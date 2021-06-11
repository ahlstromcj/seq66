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
 * \file          opcontainer.cpp
 *
 *  This module declares/defines a container for operation numbers and
 *  midioperation objects.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-18
 * \updates       2021-02-11
 * \license       GNU GPLv2 or above
 *
 */

#include <iomanip>                      /* std::setw() manipulator          */
#include <iostream>                     /* std::cout (using namespace std)  */

#include "cfg/settings.hpp"             /* seq66::rc() rcsettings getter    */
#include "ctrl/opcontainer.hpp"         /* seq66::opcontainer class         */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  This default constructor creates a "zero" object.  Every member is
 *  either false or some other form of zero.
 */

opcontainer::opcontainer () :
    m_container         (),
    m_container_name    ()
{
    // Empty body
}

/**
 *  This constructor assigns the basic values of control name, number, and
 *  action code.  The rest of the members can be set via the set() function.
 */

opcontainer::opcontainer (const std::string & name) :
    m_container         (),
    m_container_name    (name)
{
    // Empty body
}

/**
 *  Adds a midioperation to the container, it it is not the automation slot.
 *
 * \param op
 *      The operation's object, which provides the op-number, the automation
 *      slot number to use.  The "max" value will never be encounterd, and
 *      the "automation" value is ignored.  The "reserved" numbers should use
 *      a no-op midioperation.
 *
 *      What about just checking "none"?
 */

bool
opcontainer::add (const midioperation & op)
{
    bool result = false;
    automation::slot opnumber = op.number();
    if
    (
        opnumber != automation::slot::max ||
        opnumber != automation::slot::automation
    )
    {
        opmap::size_type sz = m_container.size();

        /*
         * auto --> std::pair<automation::slot, midioperation>;
         */

        auto p = std::make_pair(opnumber, op);
        (void) m_container.insert(p);
        result = m_container.size() == (sz + 1);
    }
    if (! result)
    {
#if defined SEQ66_PLATFORM_DEBUG
        int snumber = static_cast<int>(opnumber);
        printf("opcontainer::add() failed for slot #%d\n", snumber);
#endif
    }
    return result;
}

const midioperation &
opcontainer::operation (automation::slot s) const
{
    static midioperation sm_midioperation_dummy;
    const auto & coi = m_container.find(s);
    return (coi != m_container.end()) ? coi->second : sm_midioperation_dummy;
}

void
opcontainer::show () const
{
    int index = 0;
    std::cout << "Op container size: " << m_container.size() << std::endl;
    for (const auto & oc : m_container)
    {
        std::cout
            << "[" << std::setw(2) << std::right << index << "] "
            << opcontrol::slot_name(oc.first) << ": "
            ;

        oc.second.show();
        ++index;
    }
}

}           // namespace seq66

/*
 * opcontainer.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


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
 * \file          clockslist.cpp
 *
 *  This module defines some of the more complex functions of the clockslist.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2020-12-10
 * \updates       2020-12-11
 * \license       GNU GPLv2 or above
 *
 */

#include "play/clockslist.hpp"          /* seq66::clockslist class          */

/*
 *  This namespace is not documented because it screws up the document
 *  processing done by Doxygen.
 */

namespace seq66
{

/**
 *  Default and principal constructor defined in the header.
 */

/**
 *  Saves the clock settings read from the "rc" file so that they can be
 *  passed to the mastermidibus after it is created.
 *
 * \param clocktype
 *      The clock value read from the "rc" file.
 */

void
clockslist::add (e_clock clocktype, const std::string & name)
{
    io ioitem;
    ioitem.io_enabled = clocktype != e_clock::disabled;
    ioitem.out_clock = clocktype;
    if (! name.empty())
    {
        ioitem.io_name = name;
        ioitem.io_nick_name = name;
    }
    m_master_io.push_back(ioitem);
}

/**
 *  Sets a single clock item, if in the currently existing range.
 *  Mostly meant for use by the Options / MIDI Input tab and configuration
 *  files.
 */

bool
clockslist::set (bussbyte bus, e_clock clocktype)
{
    bool result = bus < count();
    if (result)
    {
        bool enabled = clocktype != e_clock::disabled;
        m_master_io[bus].io_enabled = enabled;
        m_master_io[bus].out_clock = clocktype;
    }
    return result;
}

e_clock
clockslist::get (bussbyte bus) const
{
    return bus < count() ? m_master_io[bus].out_clock : e_clock::off ;
}

}               // namespace seq66

/*
 * clockslist.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


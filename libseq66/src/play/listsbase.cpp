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
 * \file          listsbase.cpp
 *
 *  This module defines some of the more complex functions of the listsbase.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2020-12-10
 * \updates       2020-12-10
 * \license       GNU GPLv2 or above
 *
 */

#include "play/listsbase.hpp"          /* seq66::listsbase class          */

/*
 *  This namespace is not documented because it screws up the document
 *  processing done by Doxygen.
 */

namespace seq66
{

/*
 *  The simple constructor and destructor defined in the header file.
 *  A few functions included here for better debugging.
 */

void
listsbase::set_name (bussbyte bus, const std::string & name)
{
    if (bus < count())
        m_master_io[bus].io_name = name;
}

void
listsbase::set_nick_name (bussbyte bus, const std::string & name)
{
    if (bus < count())
        m_master_io[bus].io_nick_name = name;
}

std::string
listsbase::get_name (bussbyte bus) const
{
    static std::string s_dummy;
    return bus < count() ? m_master_io[bus].io_name : s_dummy ;
}

std::string
listsbase::get_nick_name (bussbyte bus) const
{
    static std::string s_dummy;
    return bus < count() ? m_master_io[bus].io_nick_name : s_dummy ;
}

}               // namespace seq66

/*
 * listsbase.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


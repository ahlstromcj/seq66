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
 * \file          automutex.cpp
 *
 *  This module declares/defines the base class for mutexes.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2019-03-16
 * \license       GNU GPLv2 or above
 *
 *  Seq66 needs a recursive mutex and a condition-variable for sequencer
 *  operations.  This module defines the recursive mutex needed.
 */

#include "util/automutex.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

    /*
     * Currently, the header defines all the code needed for this class.
     */

}           // namespace seq66

/*
 * automutex.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


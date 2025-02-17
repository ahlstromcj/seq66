#if ! defined SEQ66_PATCHES_HPP
#define SEQ66_PATCHES_HPP

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
 * \file          patches.hpp
 *
 *  This module declares the array of MIDI controller names.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2025-02-17
 * \updates       2025-02-17
 * \license       GNU GPLv2 or above
 *
 */

#include <string>

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

extern std::string gm_program_name (int index);
extern std::string program_name (int index);

}           // namespace seq66

#endif      // SEQ66_PATCHES_HPP

/*
 * patches.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


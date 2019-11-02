#if ! defined SEQ66_CONTROLLERS_HPP
#define SEQ66_CONTROLLERS_HPP

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
 * \file          controllers.hpp
 *
 *  This module declares the array of MIDI controller names.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2015-12-06
 * \license       GNU GPLv2 or above
 *
 *  This file used to define the array itself, but now it just declares it,
 *  since more than one module now uses this array.
 */

#include <string>

#include "midibytes.hpp"                /* seq66::c_midibyte_data_max (128) */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 *  Provides the default names of MIDI controllers, which a specified in the
 *  controllers.cpp module.  This array is used
 *  only by the seqedit/qseqedit classes.
 *
 *  We could make this list a configuration option.  Overkill?
 */

extern const std::string c_controller_names[c_midibyte_data_max];

}           // namespace seq66

#endif      // SEQ66_CONTROLLERS_HPP

/*
 * controllers.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


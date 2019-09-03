#if ! defined SEQ66_SETTINGS_HPP
#define SEQ66_SETTINGS_HPP

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
 * \file          settings.hpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  and functions used in this application.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2016-05-17
 * \updates       2019-03-14
 * \license       GNU GPLv2 or above
 *
 *  A couple of universal helper functions remain as inline functions in the
 *  module.  The rest have been moved to the calculations module.
 *
 *  Also note that this file really is a C++ header file, and should have
 *  the "hpp" file extension.  We will fix that Real Soon Now.
 */

#include <string>

#include "cfg/rcsettings.hpp"           /* seq66::rcsettings                */
#include "cfg/usrsettings.hpp"          /* seq66::usrsettings               */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 *  Returns a reference to the global rcsettings and usrsettings objects.
 *  Why a function instead of direct variable access?  Encapsulation.  We are
 *  then free to change the way "global" settings are accessed, without
 *  changing client code.
 */

extern rcsettings & rc ();
extern usrsettings & usr ();
extern int choose_ppqn (int ppqn = SEQ66_USE_DEFAULT_PPQN);
extern void set_defaults ();

/**
 *  Shows a message if in verbose mode.
 */

inline void
verbose_message (const std::string & msg)
{
    if (rc().verbose() && ! msg.empty())
        printf("%s\n", msg.c_str());
}

}           // namespace seq66

#endif      // SEQ66_SETTINGS_HPP

/*
 * settings.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


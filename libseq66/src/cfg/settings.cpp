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
 * \file          settings.cpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  and functions in this application.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2016-05-17
 * \updates       2021-03-18
 * \license       GNU GPLv2 or above
 *
 *  The first part of this file defines a couple of global structure
 *  instances, followed by the global variables that these structures
 *  completely replace.
 */

#include "cfg/settings.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Returns a reference to the global rcsettings object.  Why a function
 *  instead of direct variable access?  Encapsulation.  We are then free to
 *  change the way "global" settings are accessed, without changing client
 *  code.
 *
 * \return
 *      Returns the global object g_rcsettings.
 */

rcsettings &
rc ()
{
    static rcsettings s_rcsettings;
    return s_rcsettings;
}

/**
 *  Provides the replacement for all of the other settings in the "user"
 *  configuration file, plus some of the "constants" in the globals module.
 *  Returns a reference to the global usrsettings object, for better
 *  encapsulation.
 *
 * \return
 *      Returns the global object g_usrsettings.
 */

usrsettings &
usr ()
{
    static usrsettings g_usrsettings;
    return g_usrsettings;
}

/**
 *  Call set_defaults() on the "rc" and "usr" objects.
 */

void
set_configuration_defaults ()
{
    rc().set_defaults();
    usr().set_defaults();
}

/**
 *  Available PPQN values.  The default is 192, item #xx.  The first item uses
 *  the edit text for a "File" value, which means that whatever was read from
 *  the file is what holds.  The last item terminates the list.  However, note
 *  that the default PPQN can be edited by the user to be any value within
 *  this range.  Also note this list is wrapped in an accessor function.
 *
 * \param index
 *      Provides the index into the PPQN list.  If set to below 0, then
 *      it represents a request for the size of the list.
 *
 * \return
 *      Returns either the desired PPQN value, or the length of the list.
 */

int
ppqn_list_value (int index)
{
    static int s_ppqn_list [] =
    {
        0,                          /* place-holder for default PPQN    */
        32, 48, 96, 192,
        384, 768, 960, 1920,
        3840, 7680, 9600, 19200,
    };
    static const int s_count = sizeof(s_ppqn_list) / sizeof(int);
    int result = s_count;
    if (index >= 0 && (index < (s_count - 1)))
    {
        s_ppqn_list[0] = usr().default_ppqn();
        result = s_ppqn_list[index];
    }
    return result;
}

/**
 *  Common code for handling PPQN settings.  Putting it here means we can
 *  reduce the reliance on the global PPQN, and have a lot more flexibility in
 *  changing the PPQN.
 *
 *  However, this function works completely only if the "user" configuration
 *  file has already been read.  In some cases we may need to retrofit the
 *  desired PPQN value!
 *
 * \param ppqn
 *      Provides the PPQN value to be used. The default value is
 *      SEQ66_USE_DEFAULT_PPQN.
 *
 * \return
 *      Returns the ppqn parameter, unless that parameter is one of the
 *      special values above, or is illegal, as noted above.
 */

int
choose_ppqn (int ppqn)
{
    int result = ppqn;
    if (result == SEQ66_USE_DEFAULT_PPQN)
        result = usr().default_ppqn();
    else if (result == SEQ66_USE_FILE_PPQN)
        result = usr().file_ppqn();

    if (result < SEQ66_MINIMUM_PPQN || result > SEQ66_MAXIMUM_PPQN)
    {
        result = usr().default_ppqn();
        if (rc().verbose())
        {
            if (result != SEQ66_USE_FILE_PPQN)      /* "usr" was 0          */
            {
                warnprintf("PPQN %d bad, setting it to default", result);
            }
        }
    }
    return result;
}

}           // namespace seq66

/*
 * settings.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


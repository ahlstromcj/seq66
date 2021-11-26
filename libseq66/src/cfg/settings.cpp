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
 * \file          settings.cpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  and functions in this application.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2016-05-17
 * \updates       2021-11-22
 * \license       GNU GPLv2 or above
 *
 *  The first part of this file defines a couple of global structure
 *  instances, followed by the global variables that these structures
 *  completely replace.  The second part includes some convenience functions.
 */

#include "cfg/settings.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  The combo default constructor.
 */

combo::combo () : m_list_items ()
{
    m_list_items.push_back("");
}

combo::combo (const tokenization & slist) : m_list_items ()
{
    m_list_items.push_back("");
    for (const auto & s : slist)
        m_list_items.push_back(s);
}

std::string
combo::at (int index) const
{
    std::string result;
    if (index >= 0 && index < int(m_list_items.size()))
        result = m_list_items[index];

    return result;
}

int
combo::ctoi (int index) const
{
    int result = (-1);
    std::string s = at(index);
    if (! s.empty())
        result = std::stoi(s);

    return result;
}

/**
 *  This list is useful in the user-interface.  Also see ppqn_list_value()
 *  below for internal integer versions.
 */

const tokenization &
default_ppqns ()
{
    static tokenization s_default_ppqn_list =
    {
        "32", "48", "96", "192", "240",
        "384", "768", "960", "1920", "2400",
        "3840", "7680", "9600", "19200"
    };
    return s_default_ppqn_list;
}

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
 *  Available PPQN values.  The default is 192.  The first item uses the edit
 *  text for a non-standard default PPQN (like 333).  However, note that the
 *  default PPQN can be edited by the user to be any value within this range.
 *  Also note this list is wrapped in an accessor function.
 *
 * \param index
 *      Provides the index into the PPQN list.  If set to below 0, then
 *      it represents a request for the size of the list.
 *
 * \return
 *      Returns either the desired PPQN value, or the length of the list.
 *      If the index is not in the range, and is not (-1), then 0 is returned,
 *      and the result should be ignored.
 */

int
ppqn_list_value (int index)
{
    static int s_ppqn_list [] =
    {
        0,                          /* place-holder for default PPQN    */
        32, 48, 96, 192, 240,
        384, 768, 960, 1920, 2400,
        3840, 7680, 9600, 19200
    };
    static const int s_count = sizeof(s_ppqn_list) / sizeof(int);
    int result = 0;
    s_ppqn_list[0] = usr().default_ppqn();
    if (index >= 0 && (index < s_count))
        result = s_ppqn_list[index];
    else if (index == (-1))
        result = s_count;

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
 *      c_use_default_ppqn.
 *
 * \return
 *      Returns the ppqn parameter, unless that parameter is one of the
 *      special values above, or is illegal, as noted above.
 */

int
choose_ppqn (int ppqn)
{
    int result = ppqn;
    if (result == c_use_default_ppqn)
        result = usr().midi_ppqn();                 /* usr().default_ppqn() */
    else if (result == c_use_file_ppqn)
        result = usr().file_ppqn();
    else if (! ppqn_in_range(result))               /* file, in-range PPQN  */
        result = usr().midi_ppqn();                 /* usr().default_ppqn() */

    return result;
}

}           // namespace seq66

/*
 * settings.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


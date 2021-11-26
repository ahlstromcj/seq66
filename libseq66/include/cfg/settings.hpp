#if ! defined SEQ66_SETTINGS_HPP
#define SEQ66_SETTINGS_HPP

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
 * \file          settings.hpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  and functions used in this application.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2016-05-17
 * \updates       2021-11-22
 * \license       GNU GPLv2 or above
 *
 *  A couple of universal helper functions remain as inline functions in the
 *  module.  The rest have been moved to the calculations module.
 *
 *  Also note that this file really is a C++ header file, and should have
 *  the "hpp" file extension.  We will fix that Real Soon Now.
 */

#include "cfg/rcsettings.hpp"           /* seq66::rcsettings, std::string   */
#include "cfg/usrsettings.hpp"          /* seq66::usrsettings, std::vector  */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Provides an encapsulation of editable combo-box support.  Currently used
 *  only for PPQN combo-boxes.  The 0th item in this combo list always
 *  contains the default or current value.
 */

class combo
{

private:

    /**
     *  The list of values.  Starts out at size 1.  Items can only be added,
     *  not changed or removed.  However, the 0th item can be changed by the
     *  caller.
     */

    tokenization m_list_items;

public:

    combo ();
    combo (const combo &) = default;
    combo & operator = (const combo &) = default;
    combo (const tokenization & slist);
    ~combo () = default;

    int count () const
    {
        return int(m_list_items.size());
    }

    std::string current () const
    {
        return m_list_items[0];
    }

    void current (const std::string & s)
    {
        m_list_items[0] = s;
    }

    void current (int v)
    {
        m_list_items[0] = std::to_string(v);
    }

    std::string at (int index) const;
    int ctoi (int index) const;

    void add (const std::string & s)
    {
        m_list_items.push_back(s);
    }

    void add (int v)
    {
        std::string s = std::to_string(v);
        add(s);
    }

};          // class combo

/*
 *  Returns a reference to the global rcsettings and usrsettings objects.
 *  Why a function instead of direct variable access?  Encapsulation.  We are
 *  then free to change the way "global" settings are accessed, without
 *  changing client code.
 */

extern rcsettings & rc ();
extern usrsettings & usr ();
extern int choose_ppqn (int ppqn);
extern int ppqn_list_value (int index = (-1));
extern const tokenization & default_ppqns();
extern void set_configuration_defaults ();

/**
 *  Indicates if the PPQN value is in the legal range of usable PPQN values.
 */

inline bool
ppqn_in_range (int ppqn)
{
    return usr().use_file_ppqn() ||
        (ppqn >= c_minimum_ppqn && ppqn <= c_maximum_ppqn);
}

}           // namespace seq66

#endif      // SEQ66_SETTINGS_HPP

/*
 * settings.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


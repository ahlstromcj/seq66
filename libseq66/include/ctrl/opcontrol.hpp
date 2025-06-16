#if ! defined SEQ66_OPCONTROL_HPP
#define SEQ66_OPCONTROL_HPP

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
 * \file          opcontrol.hpp
 *
 *  This module declares/defines the class for handling MIDI control data for
 *  the application.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-12-04
 * \updates       2023-09-05
 * \license       GNU GPLv2 or above
 *
 *  This module defines a number of constants relating to control of pattern
 *  unmuting, group control, and a number of additional controls to make Seq66
 *  controllable without a graphical user interface.  Requires C++11 and
 *  above.
 */

#include <string>                       /* std::string                      */
#include <vector>                       /* std::vector<>                    */

#include "ctrl/automation.hpp"          /* namespace seq66::automation      */

namespace seq66
{

/**
 *  Provides an object that supports some enumerations to indicate what kind
 *  of control this is.  This class is a base class for the key and MIDI control
 *  classes.
 */

class opcontrol
{

public:

    /**
     *  Indicates that a category or other integer operation is not valid.
     */

    static const int INVALID = (-1);

private:

    /**
     *  The name of the opcontrol object.
     */

    std::string m_name;

    /**
     *  Which section of the "rc" control file is this operation in?
     *  Pattern, Mute_group, or Automation?
     */

    automation::category m_category;

    /**
     *  Indicates if the automation operation is a toggle, an on, or an off.
     */

    automation::action m_action;

    /**
     *  Provides the operation number.  For a pattern control, this is the
     *  slot number to obtain the loop-control midioperation object.  For a
     *  mute-group control, this is the group number to obtain the
     *  mute-group-control midioperation object.  For an automation control,
     *  this is the number of the performer operation to call, of type
     *  automation::slot.  The values above automation::slot::max are used
     *  for pattern and mute-group function selection.
     */

    automation::slot m_slot_number;

public:

    opcontrol ();
    opcontrol
    (
        const std::string & opname,
        automation::category opcategory,
        automation::action opaction,
        automation::slot opnumber,
        int index = 0
    );
    opcontrol (const opcontrol &) = default;
    opcontrol & operator = (const opcontrol &) = default;
    opcontrol (opcontrol &&) = default;
    opcontrol & operator = (opcontrol &&) = default;
    virtual ~opcontrol () = default;

    static std::string category_name (automation::category c);
    static std::string action_name (automation::action a);
    static std::string automation_slot_name (automation::slot s);
    static automation::slot set_slot (int opcode);

    bool is_usable () const
    {
        return
        (
            m_category != automation::category::none &&
            m_action != automation::action::none &&
            m_slot_number != automation::slot::none
        );
    }

    bool is_glearn_control () const                     /* a special case   */
    {
        return
        (
            m_category == automation::category::automation &&
            m_slot_number == automation::slot::mod_glearn
        );
    }

    /**
     *  An operation is allowed if it is either not a keystroke (d0 == -1) or
     *  is not inverse (true for keystroke release).
     */

    static bool allowed (int d0, bool inverse)
    {
        return d0 >= 0 || ! inverse;
    }

    /**
     *  Simplifies this check for the callers.
     */

    static bool is_automation (automation::category cat)
    {
        return cat == automation::category::automation;
    }

    /**
     *  Simplifies this check for the callers.
     */

    static bool is_sequence_control (automation::category cat)
    {
        return cat == automation::category::loop ||
            cat == automation::category::mute_group;
    }

    const std::string & name () const
    {
        return m_name;
    }

    automation::category category_code () const
    {
        return m_category;
    }

    std::string category_name () const
    {
        return category_name(m_category);
    }

    automation::action action_code () const
    {
        return m_action;
    }

    std::string action_name () const
    {
        return action_name(m_action);
    }

    automation::slot slot_number () const
    {
        return m_slot_number;
    }

    std::string automation_slot_name () const
    {
        return automation_slot_name(m_slot_number);
    }

    std::string build_slot_name (int index) const;

};              // class automation

/*
 * ----------------------------------------------------------------------
 *  Free functions in the seq66 namespace
 * ----------------------------------------------------------------------
 */

extern std::string auto_name (automation::slot s);

}               // namespace seq66

#endif          // SEQ66_OPCONTROL_HPP

/*
 * opcontrol.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


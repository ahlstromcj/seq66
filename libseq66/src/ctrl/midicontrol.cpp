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
 * \file          midicontrol.cpp
 *
 *  This module declares/defines a class for extended and flexible MIDI control,
 *  unified with keyboard control.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-09
 * \updates       2019-02-24
 * \license       GNU GPLv2 or above
 *
 *  The idea behind the MIDI control automation setup is that an incoming event
 *  can be looked up in a midicontrolin object, based on its event status byte,
 *  the first data byte, and a range for values for the second data byte.
 *
 *  The control also contains an automation slot number with indicates which of
 *  roughly 50 functions is to be called.  Two of these functions handle pattern
 *  toggling and mute-groups, and use a third value to determine which
 *  sequences in a set or mutes in a group are to be operated on.
 *
 *  A MIDI control object can appear more than once in the container, to affect,
 *  for example, multiple patterns.
 */

#include <iomanip>                      /* std::setw manipulator            */
#include <iostream>                     /* std::cout  (using namespace std) */

#include "ctrl/midicontrol.hpp"         /* seq66::midicontrol class         */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  This default constructor creates a "zero" object.  Every member is
 *  either false or some other form of zero.  This object is useful as a
 *  return value for container lookups that do not succeed.
 */

midicontrol::midicontrol () :
    keycontrol          (),
    m_active            (false),
    m_inverse_active    (false),
    m_status            (0),
    m_d0                (0),
    m_d1                (0),
    m_min_value         (0),
    m_max_value         (0)
{
    // Empty body
}

/**
 *  This constructor assigns the basic values of control name, number, and
 *  action code.  The rest of the members can be set via the set() function.
 *
 * \param keyname
 *      Provides the name of the key associated with the control, such as
 *      "KP_Home".
 *
 * \param opcategory
 *      Indicates if this keystroke is meant for pattern control, mute-group
 *      (mutes) control, or general automation control.  It's value is determine
 *      by the section-name in the "ctrl" file.
 *
 * \param actioncode
 *      One of the values of automation::action::none,
 *      automation::action::toggle, automation::action::on, or
 *      automation::action::off.  The "none" action generally won't be used.
 *      Instead, we just don't create the midicontrol.  This value is
 *      implicit in the positioning of a sub-stanza in the stanza line
 *      consisting of three sub-stanzas.
 *
 * \param opnumber
 *      Provides the slot number of the control, which determines which slot
 *      function (see the automation::slot enumeration) is called.  This
 *      specifies the pattern/loop function, mute-group function, or one of the
 *      roughly 48 automation functions.
 *
 * \param opcode
 *      This value is useful in when calling the pattern and mute-group slot
 *      functions.  It contains the pattern number, mute-group number, or,
 *      redundantly re the opnumber parameter, the automation-slot number, that
 *      the control is supposed to operate.  This number is the first number on
 *      each MIDI-control stanza line in the "ctrl" files.
 */

midicontrol::midicontrol
(
    const std::string & keyname,
    automation::category opcategory,
    automation::action actioncode,
    automation::slot opnumber,
    int opcode
) :
    keycontrol
    (
        "MIDI", keyname, opcategory, actioncode, opnumber, opcode
    ),
    m_active            (false),
    m_inverse_active    (false),
    m_status            (0),
    m_d0                (0),
    m_d1                (0),
    m_min_value         (0),
    m_max_value         (0)
{
    // Empty body
}

/**
 *  Checks to see if this control matches the given category and slot.
 *  For the pattern category, the slot should be the pattern number. For the
 *  mute_group category, the slot should be the group number.  For both, this
 *  is the control_code() value.  For the automation category, the number to
 *  check is the slot_number().
 */

bool
midicontrol::merge_key_match (automation::category c, int opslot) const
{
    if (c == category_code())
    {
        if (opcontrol::is_automation(c))
        {
            automation::slot s = static_cast<automation::slot>(opslot);
            return s == slot_number();
        }
        else                            /* assumes a valid category here    */
        {
            return opslot == control_code();
        }
    }
    else
        return false;
}

/**
 *
 */

void
midicontrol::show (bool add_newline) const
{
    using namespace std;
    cout << "Key: ";

    keycontrol::show(false);

    cout
        << " [ " << active() << " " << inverse_active()
        << " "   << "0x" << setw(2)
        << setfill('0') << hex <<status() << setfill(' ')
        << " "   << setw(3) << d0()
        << " "   << setw(3) << min_value()
        << " "   << setw(3) << max_value()
        << " ]"
        ;

    if (add_newline)
        cout << endl;
}

}           // namespace seq66

/*
 * midicontrol.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


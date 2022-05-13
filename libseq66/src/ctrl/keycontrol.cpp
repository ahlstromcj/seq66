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
 * \file          keycontrol.cpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  and functions for the extended MIDI control feature.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-09
 * \updates       2021-12-31
 * \license       GNU GPLv2 or above
 *
 */

#include <iomanip>                      /* std::setw() manipulator          */
#include <iostream>                     /* std::cout (using namespace std)  */

#include "ctrl/keycontrol.hpp"          /* seq66::keycontrol class          */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  The name for keys that are unused.  They should not be added to a
 *  keycontainer, but they are needed in the base class of midicontrol.
 */

const std::string keycontrol::scm_dead_key_name = "Blank";

/**
 *  This constructor assigns the basic values of control name, number, and
 *  action code.  The rest of the members can be set via the set() function.
 *
 * \param opname
 *      Provides the name of the control, such as "BPM Up/Down".  It is not
 *      used as part of the control information, but it can be used in
 *      messages, or as a label in the user-interface.
 *
 * \param keyname
 *      Provides the name of the keystroke that triggers this control.
 *      This name is one of the values in the keymap.cpp/hpp module's
 *      static qt_keycodes array, sq_qt_keys[].  This name is not part of the
 *      control information, but it does allow for conversion to an ordinal
 *      value using the keymap function qt_keyname_ordinal().
 *
 * \param opcategory
 *      Indicates if this keystroke is meant for pattern control, mute-group
 *      (mutes) control, or general automation control.  Like the \a opname
 *      and the \a keyname parameters, this one is used for informational
 *      purposes.
 *
 * \param actioncode
 *      One of the values of keycontrol::action::none,
 *      keycontrol::action::toggle, keycontrol::action::on, or
 *      keycontrol::action::off.  The "none" action generally won't be used.
 *      Instead, we just don't create the keycontrol.
 *
 * \param opslot
 *      Provides the slot number of the control, which determines which slot
 *      function (see the automation::slot enumeration) is called.
 *
 * \param index
 *      Provides an index for use by the keystroke control.  For pattern
 *      control, this number is the pattern offset into the active screen-set
 *      (e.g. 0 to 31).  For mute-group control, this number is the mute-group
 *      to toggle (e.g. 0 to 31).  For automation control, the specific slot
 *      function determines if and how this parameter is used.
 */

keycontrol::keycontrol
(
    const std::string & opname,
    const std::string & keyname,
    automation::category opcategory,
    automation::action actioncode,
    automation::slot opslot,
    int index
) :
    opcontrol       (opname, opcategory, actioncode, opslot, index),
    m_key_name      (keyname),
    m_control_code  (index),
    m_ordinal       (qt_keyname_ordinal(keyname))
{
    if (is_invalid_ordinal(m_ordinal))
    {
        m_key_name = scm_dead_key_name;
    }
}

/**
 *  Man, this would be a lot easier with printf()!
 */

void
keycontrol::show (bool add_newline) const
{
    using namespace std;
    cout
        <<         setw(7) << left << key_name()
        << " "  << setw(4) << left << category_name()
        << " "  << setw(6) << left << action_name()
        << " "  << setw(2) << dec  << right << int(slot_number())
        << "/"  << setw(2) << dec  << right << int(control_code())
        << " '" << name() << "'"
        ;
    if (add_newline)
        cout << endl;
}

}           // namespace seq66

/*
 * keycontrol.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


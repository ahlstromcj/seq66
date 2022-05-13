#if ! defined SEQ66_KEYCONTROL_HPP
#define SEQ66_KEYCONTROL_HPP

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
 * \file          keycontrol.hpp
 *
 *  This module declares/defines the class for handling key control data for
 *  the application.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-18
 * \updates       2022-05-13
 * \license       GNU GPLv2 or above
 *
 *  This class is similar in intent to the midicontrol class, but is simpler
 *  because keystrokes don't have data parameters the way a MIDI event does.
 *  It requires C++11 and above.
 */

#include "ctrl/keymap.hpp"              /* seq66::qt_ordinal_keyname()      */
#include "ctrl/opcontrol.hpp"           /* seq66::opcontrol & automation    */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  This class contains the control information for sequences that make up a
 *  live set.
 *
 *  Note that, although we've converted this to a full-fledged class, the
 *  ordering of variables and the data arrays used to fill them is very
 *  signifcant.  See the midifile and optionsfile modules.
 */

class keycontrol : public opcontrol
{

    friend class keycontainer;

private:

    /**
     *  The name used to represent any key that cannot be used.
     */

    static const std::string scm_dead_key_name;

    /**
     *  Provides the name of keystroke associated with this control.  This
     *  item is useful for displaying the assigned keystroke for debugging or
     *  in the user interface.
     */

    std::string m_key_name;

    /**
     *  Provides the particular index for this keystroke control.  This number
     *  supplements the operation number, and applies to pattern controls and
     *  mute-group controls, where the operation (slot) number covers a number
     *  of controls: automation::slot::loop and automation::slot::mute_group.
     *
     *  The operation (slot) number is used to choose the correct performance
     *  function for the control.  The index number is used to choose the
     *  correct pattern or mute-group number.
     *
     *  Provides the operation number, ranging from 0 to the maximum number of
     *  keys supported, say 96 to 127.  For a pattern control, this is the
     *  pattern number.  For a mute-group control, this is the group number.
     *  For an automation control, this COULD BE the number of the performer
     *  operation to call, of type automation::slot.
     */

    int m_control_code;       // pattern or mute-group number

    /**
     *  The ordinal of this key-control.  This is an index into the
     *  keymap, and might be useful in the future to filter out certain ordinals
     *  when processing the keys.  For example, we might want to allow control
     *  codes to be used in order to gain extra slots for automation controls to
     *  which we will never map keystrokes, but need to provide for MIDI
     *  control.
     *
     *  Side note:  Seq66 will never automate more than 254 functions.
     */

    ctrlkey m_ordinal;

public:

    /*
     *  A default constructor is needed to provide a dummy object to return
     *  when the desired one cannot be found.
     */

    keycontrol () = default;

    /*
     * The move and copy constructors, the move and copy assignment operators,
     * and the destructors are all compiler generated.
     */

    keycontrol
    (
        const std::string & name,
        const std::string & keyname,
        automation::category opcategory,
        automation::action actioncode,
        automation::slot opnumber,
        int index
    );

    keycontrol (const keycontrol &) = default;
    keycontrol & operator = (const keycontrol &) = default;
    keycontrol (keycontrol &&) = default;
    keycontrol & operator = (keycontrol &&) = default;
    virtual ~keycontrol () = default;

    std::string key_name () const
    {
        return m_key_name;
    }

    int control_code () const
    {
        return m_control_code;
    }

    /**
     *  Performs a common test and returns the appropriate number, either the
     *  control-code (for loop/pattern and mute-groups) or the slot-number
     *  (for the automation group).
     */

    int slot_control () const
    {
        return category_code() == automation::category::automation ?
            static_cast<int>(slot_number()) : control_code() ;
    }

public:

    void key_name (const std::string & kn)
    {
        m_key_name = kn;
    }

    /**
     *  Builds a label for the key/MIDI control, which will include the loop
     *  or group number if appropriate for the category of the control.
     */

    std::string label () const
    {
        return build_slot_name(m_control_code);
    }

    void show (bool add_newline = true) const;

    ctrlkey ordinal () const
    {
        return m_ordinal;
    }

    bool is_ctrl_ordinal () const
    {
        return m_ordinal < 0x1f;
    }

private:

    void ordinal (ctrlkey ck)
    {
        m_ordinal = ck;
    }

};              // class keycontrol

}               // namespace seq66

#endif          // SEQ66_KEYCONTROL_HPP

/*
 * keycontrol.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


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
 * \file          midicontrolin.cpp
 *
 *  This module declares/defines a container for key-ordinals and MIDI
 *  operation information.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-23
 * \updates       2020-08-13
 * \license       GNU GPLv2 or above
 *
 * MIDI control container:
 *
 *  Key:
 *
 *      The MIDI container is sorted by a key := { event status, d0 } with d1
 *      used for range-checking.
 *
 *  Value:
 *
 *      The MIDI control value stored in the container consists of:
 *
 *          opcontrol:      name, category, action, slot
 *          keycontrol:     key-name, control-code (pattern or group number)
 *          midicontrol:    active, inverse, status, d0, d1, min, max
 *
 *  Run-time lookup:
 *
 *      For run-time lookup, a key is created from the incoming status and d0,
 *      and used to find the (possibly multiple) midicontrols for that event.
 *      If found, then the d1 value is checked for the proper range.  If
 *      satisfied, then the corresponding midicontrol operation is invoked.
 *
 *      In the new-style "rc"/"ctrl" file, the configured keystroke is also
 *      added to the midicontrol object, as well as to the keycontainer.
 *
 *  Key-control setup from legacy "rc" file:
 *
 *      After setting up the MIDI control container as above (via the defaults
 *      or the "ctrl" file), the keystrokes are backfilled into the existing
 *      midicontrol container.
 *
 *      In this reading, all MIDI control stanzas are added to the container,
 *      even if not active, so that all can be written out to the new file
 *      format.
 *
 *      The order of the keystroke values in the MIDI container depends on
 *      whether the keystroke matches a non-zero MIDI event or not.
 *
 *  Key/MIDI control setup written to new "ctrl" file:
 *
 *      Writing of the midicontrolin occurs only during conversion from
 *      old-style "rc" file to new-style "ctrl" file using the seqtool
 *      application.  In that case, the container needs iteration in the
 *      following order:
 *
 *          -   Category value: pattern section, then mute-group section, then
 *              the automation section.
 *          -   Slot value (pattern number, mute-group number, or automation
 *              slot.
 *          -   Action: toggle, on, and off.
 */

#include <iomanip>                      /* std::setw manipulator            */
#include <iostream>                     /* std::cerr                        */

#include "cfg/settings.hpp"             /* seq66::rc() rcsettings getter    */
#include "ctrl/keycontainer.hpp"        /* seq66::keycontainer class       */
#include "ctrl/keymap.hpp"              /* seq66::qt_keyname_ordinal()      */
#include "ctrl/midicontrolin.hpp"       /* seq66::midicontrolin class       */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  This default constructor creates a "zero" object.  Every member is
 *  either false or some other form of zero.
 */

midicontrolin::midicontrolin
(
    int /*buss*/,       /* NOT YET USED */
    int rows,
    int columns
) :
    midicontrolbase
    (
        SEQ66_MIDI_CONTROL_IN_BUSS,         /* allows all busses */
        rows, columns
    ),
    m_container         (),
    m_container_name    ("Default MIDI Controls"),
    m_comments_block    (),
    m_inactive_allowed  (false),
    m_loaded_from_rc    (false),
    m_control_status    (automation::ctrlstatus::none),
    m_have_controls     (false)
{
    is_enabled(true);                   /* by default */
}

/**
 *  This constructor assigns the basic values of control name, number, and
 *  action code.  The rest of the members can be set via the set() function.
 */

midicontrolin::midicontrolin (const std::string & name) :
    m_container         (),
    m_container_name    (name),
    m_comments_block    (),
    m_loaded_from_rc    (false),
    m_control_status    (automation::ctrlstatus::none),
    m_have_controls     (false)
{
   // no code
}

/**
 *  Adds the midicontrol object to the container, keyed by the midicontrol ::
 *  key object, which represents the basic event data the control must match.
 *
 * \param mc
 *      The midicontrol value to add.  The key is constructed from the data in
 *      this object.
 *
 * \return
 *      Returns true if the control was added.  That is, the size of the
 *      container increased by 1.  It is an std::multimap, so there shouldn't
 *      be any failures.  However, the caller is responsible for inserting
 *      only what controls are needed.
 */

bool
midicontrolin::add (const midicontrol & mc)
{
    bool result = false;
    auto sz = m_container.size();
    midicontrol::key k = mc.make_key();

    /*
     * auto --> std::pair<int, midicontrol>;
     */

    auto p = std::make_pair(k, mc);
    (void) m_container.insert(p);
    result = m_container.size() == (sz + 1);
    if (result)
    {
        if (! mc.blank())
            m_have_controls = true;
    }
    else
    {
        std::cerr
            << "Duplicate or invalid opslot for '" << mc.name()
            << "' Category " << mc.category_name()
            << " Slot " << mc.slot_name()
            << std::endl
            ;
    }
    return result;
}

/**
 *  This function is needed for running the application for the first time, when
 *  there is no "rc" or "ctrl" file.  We want to be able to write out the full
 *  set of stanzas, with the keystrokes, even if the MIDI controls are all zero.
 *  Controls are written only for defined keystrokes in the keycontainer.
 *
 * \param kc
 *      Provides the key setup, so that the keystroke can be added to the
 *      output.
 */

void
midicontrolin::add_blank_controls (const keycontainer & kc)
{
    for (const auto & kpair : kc.container())
    {
        const keycontrol & k = kpair.second;
        midicontrol blank
        (
            k.key_name(), k.category_code(), k.action_code(),
            k.slot_number(), k.control_code()
        );
        (void) add(blank);
    }
#if defined SEQ66_PLATFORM_DEBUG
    if (rc().verbose())
        show();
#endif
}

/**
 *  Looks for the given automation category and operation slot number, and
 *  inserts the key code if found.
 *
 *  Used in converting from the old-style "rc" file, where the MIDI controls
 *  and the key controls are in separate sections.  We have to check the whole
 *  container every damn time, since the container is not keyed by category or
 *  slot.  However, this function is called only at startup.
 *
 * \param opcat
 *      The category of control (pattern, mute-group, or automation/opcontrol)
 *      to which the keystroke has been assigned.  This must be matched by the
 *      midicontrols' category.
 *
 * \param keyname
 *      The name of the key, which replaces the "Null_ff" value assigned when
 *      the midi-control section was read.
 *
 * \param opslot
 *      Provides the control-code (for pattern and mute-groups) or the
 *      slot-code (for automation/opcontrol).
 */

bool
midicontrolin::merge_key
(
    automation::category opcat,
    const std::string & keyname,
    int opslot
)
{
    bool result = false;
    for (auto & mcpair : m_container)
    {
        midicontrol & mc = mcpair.second;
        if (mc.merge_key_match(opcat, opslot))
        {
            if (mc.key_name() != "Null_ff")
            {
                if (mc.key_name().empty())
                {
                    errprint("merge_key() finds empty key-name");
                }
                else
                {
                    std::string msg = " merge_key() finds valid key-name '" +
                         mc.key_name() + "' already present for category " +
                        opcontrol::category_name(opcat) + ", '" + keyname +
                        "', slot #'" + std::to_string(opslot)
                         ;

                    warnprint(msg);
                }
            }
            mc.key_name(keyname);
            result = true;
        }
    }
    return result;
}

/**
 *  Looks up the MIDI-control object matching the given key value. Remeber that
 *  the key is match on the MIDI event status and the d0 value, and
 *  range-checked on the d1 value.
 */

const midicontrol &
midicontrolin::control (const midicontrol::key & k) const
{
    static midicontrol sm_midicontrol_dummy;
    if (have_controls())
    {
        const auto & cki = m_container.find(k);
        return (cki != m_container.end()) ? cki->second : sm_midicontrol_dummy;
    }
    else
        return sm_midicontrol_dummy;
}

/**
 *
 */

void
midicontrolin::show () const
{
    using namespace std;
    int index = 0;
    cout << "MIDI container size: " << m_container.size() << endl;
    for (const auto & mcpair : m_container)
    {
        const midicontrol::key & k = mcpair.first;
        const midicontrol & mc = mcpair.second;
        int status = k.status();
        int d0 = k.d0();
        int d1 = k.d1();
        cout
            << "["   << setw(3) << hex << right << index << "] "
            << "ev " << "0x" << setw(2)
            << setfill('0') << hex << status << setfill(' ')
            << " "   << setw(2) << hex << d0 << " " << setw(2) << hex << d1
            << " "
            ;

        mc.show();
        ++index;
    }
}

}           // namespace seq66

/*
 * midicontrolin.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


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
 * \file          midicontrolin.cpp
 *
 *  This module declares/defines a container for key-ordinals and MIDI
 *  operation information.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-23
 * \updates       2021-12-02
 * \license       GNU GPLv2 or above
 *
 * MIDI control container:
 *
 *  Key:
 *
 *      The MIDI container is sorted by a key := { event status, d0 }.
 *
 *  Value:
 *
 *      The MIDI control value stored in the container consists of:
 *
 *          opcontrol:      name, category, action, slot
 *          keycontrol:     key-name, control-code (pattern or group number)
 *          midicontrol:    active, inverse, status, d0, d1, min d1, max d1
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
 *      Writing of the MIDI control setting takes place every time Seq66
 *      exits.
 */

#include <iomanip>                      /* std::setw() manipulator          */
#include <iostream>                     /* std::cerr                        */

#include "cfg/settings.hpp"             /* seq66::rc() rcsettings getter    */
#include "ctrl/keycontainer.hpp"        /* seq66::keycontainer class        */
#include "ctrl/midicontrolin.hpp"       /* seq66::midicontrolin class       */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  This constructor assigns the basic values of control name, number, and
 *  action code.  The rest of the members can be set via the set() function.
 */

midicontrolin::midicontrolin (const std::string & name) :
    midicontrolbase     (name),
    m_container         (),
    m_comments_block    (),
    m_control_status    (automation::ctrlstatus::none),
    m_have_controls     (false)
{
   // no code
}

bool
midicontrolin::initialize (int buss, int rows, int columns)
{
    bool result = midicontrolbase::initialize(buss, rows, columns);
    is_enabled(result);     // master bus???
    return result;
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
    auto k = mc.make_key();
    auto p = std::make_pair(k, mc);         /* std::pair<int, midicontrol>  */
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
            << " Slot " << mc.automation_slot_name()
            << std::endl
            ;
    }
    return result;
}

/**
 *  This function is needed for running the application for the first time,
 *  when there is no "rc" or "ctrl" file.  We want to be able to write out the
 *  full set of stanzas, with the keystrokes, even if the MIDI controls are
 *  all zero.  Controls are written only for defined keystrokes in the
 *  keycontainer.
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
}

/**
 *  Looks up the MIDI-control object matching the given key value. Remember
 *  that the key is match on the MIDI event status and the d0 value, and
 *  range-checked on the d1 value.  And now, the source bus of the event is
 *  now part of the key, not for operator <, but for checking the source of
 *  the event.  The source should match this container's true buss, if it
 *  isn't the "null" buss (0xFF).
 */

const midicontrol &
midicontrolin::control (const midicontrol::key & k) const
{
    static midicontrol sm_midicontrol_dummy;
    bool ok = have_controls();
    if (ok)
    {
        const auto & cki = m_container.find(k);
        ok = cki != m_container.end();
        if (ok)
            ok = is_null_buss(nominal_buss()) || k.buss() == true_buss();

        return ok ? cki->second : sm_midicontrol_dummy;
    }
    else
        return sm_midicontrol_dummy;
}

/**
 *  The possible status are contained in automation::ctrlstatus, and consist
 *  of none, replace, snapshot, queue, keep_queue, oneshot, and learn. There
 *  is a combo-status solo == queue + replace.
 *
 */

std::string
midicontrolin::status_string () const
{
    std::string result;
    if (is_solo())
        result = "Solo";
    else if (is_keep_queue())
        result = "Keep Q";
    else if (is_queue())
        result = "Queued";
    else if (is_replace())
        result = "Replace";
    else if (is_snapshot())
        result = "Snapshot";
    else if (is_oneshot())
        result = "One-shot";
    else if (is_learn())
        result = "Learn";

    return result;
}

void
midicontrolin::show () const
{
    using namespace std;
    int index = 0;
    cout
        << "MIDI-In container (size " << m_container.size() << "): " << endl
        << "Index; MIDI key; Keystroke (name, action, slot, code); stanza"
        << endl
        ;
    for (const auto & mcpair : m_container)
    {
        const midicontrol::key & k = mcpair.first;
        const midicontrol & mc = mcpair.second;
        int status = k.status();
        int d0 = k.d0();                            // key::d1() removed
        cout
            << "["   << setw(3) << hex << right << index << "] "
            << "0x" << setw(2)
            << setfill('0') << hex << status << setfill(' ')
            << " "   << setw(2) << hex << d0 << " " //  << setw(2) << hex << d1
            // << " "
            ;
        mc.show();                      /* shows the key and midi control   */
        ++index;
    }
}

}           // namespace seq66

/*
 * midicontrolin.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


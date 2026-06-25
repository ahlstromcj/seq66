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
 * \file          midilearn.cpp
 *
 *  This module declares/defines containers and management for a
 *  MIDI Learn function.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2026-06-09
 * \updates       2026-06-25
 * \license       GNU GPLv2 or above
 *
 */

#include <iomanip>                      /* std::setw() manipulator          */
#include <iostream>                     /* std::cerr                        */

#include "cfg/settings.hpp"             /* seq66::rc()                      */
#include "ctrl/midilearn.hpp"           /* seq66::midilearn class           */
#include "play/performer.hpp"           /* seq66::performer class           */

namespace seq66
{

/**
 *  This constructor assigns members based on information from the
 *  performer object. The rest are initialized in the class declaration.
 */

midilearn::midilearn
(
    performer & p,
    bool clearcontrols
) :
    m_perf              (p),
    m_original_controls (p.midi_control_in()),
    m_current_controls  (p.midi_control_in()),
    m_loop_count_max    (usr().set_size())
{
    if (clearcontrols)
        clear_all();

    (void) active_counts();
}

bool
midilearn::active_counts
(
    int & loopcount,
    int & mutescount,
    int & autocount
) const
{
    bool result = active_counts();
    if (result)
    {
        loopcount = m_loops_ctrl_count;
        mutescount = m_mutes_ctrl_count;
        autocount = m_automation_ctrl_count;
    }
    return result;
}

/*
 * const midicontrolin & mci { perf().midi_control_in() };
 */

bool
midilearn::clear_all ()
{
    const keycontainer & kc { perf().key_controls() };
    m_current_controls.clear_all();
    m_current_controls.add_blank_controls(kc);
    clear_current_index();
    return true;
}

/**
 *  Initializes the index, used when selecting an automation
 *  category.
 */

void
midilearn::initialize_current_index ()
{
    automation::category opcat { automation_category() };
    clear_current_index();
    if (opcat == automation::category::loop)
    {
        if (m_loops_ctrl_count < m_loop_count_max)
            m_current_index = m_loops_ctrl_count;
    }
    else if (opcat == automation::category::mute_group)
    {
        if (m_mutes_ctrl_count < m_mute_count_max)
            m_current_index = m_mutes_ctrl_count;
    }
}

/**
 *  Copies the original controls into the current controls.
 */

bool
midilearn::reset ()
{
    m_current_controls = m_original_controls;
    return true;
}

/**
 *  Tells the perform to redirect the incoming MIDI to the
 *  midilearn object.
 */

bool
midilearn::start ()
{
    return false; // TODO
}

/**
 *  Copies the current controls to the performer.
 *
 *  A flag for saving the 'ctrl' file needs to be raised.
 */

bool
midilearn::save ()
{
    bool result { perf().save_midi_learn(m_current_controls) };
    if (result)
    {
        // TODO ?
    }
    return result;
}

bool
midilearn::learn_control
(
    const event & ev,
    const std::string & keyname,
    bool isinverse,
    int d1min,
    int d1max,
    automation::action altaction
)
{
    bool result { true };

    /*
     * Code similar to parse_control_stanza() in the midicontrolfile
     * module.
     */

    automation::category opcat { automation_category() };
    automation::slot opslot { automation::slot::none };
    if (opcat == automation::category::loop)
    {
        opslot = automation::slot::loop;
    }
    else if (opcat == automation::category::mute_group)
    {
        opslot = automation::slot::mute_group;
    }
    else if (opcat == automation::category::automation)
    {
        result = automation_slot_active();
        opslot = opcontrol::set_slot(current_index());
    }
    if (result)
    {
        /*
         * Prevent next event (e.g. a Note Off) from being used
         * until another slot is selected in the user-interface.
         *
         * We're in the middle of some refactoring, and this
         * breaks the process:
         *
         *      clear_automation_slot();
         */

        automation::action a
        {
            altaction == automation::action::none ?
                automation_action() : altaction
        };
        midicontrol mc(keyname, opcat, a, opslot, current_index());
        mc.set(isinverse, ev.get_status(), ev.d0(), d1min, d1max);

        result = m_current_controls.replace(mc);
        if (result)
        {
#if defined SEQ66_PLATFORM_DEBUG
            if (rc().investigate())
            {
                printf
                (
                    "Learned event %2d: 0x%02x %d\n",
                    m_current_index, unsigned(ev.get_status()), int(ev.d0())
                );
            }
#endif
            set_dirty(true);
            if (m_automation_category != automation::category::automation)
                ++m_current_index;
        }
    }
    return result;
}

}           // namespace seq66

/*
 * midilearn.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

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
 * \updates       2026-06-09
 * \license       GNU GPLv2 or above
 *
 */

#include <iomanip>                      /* std::setw() manipulator          */
#include <iostream>                     /* std::cerr                        */

#include "ctrl/midilearn.hpp"           /* seq66::midilearn class           */
#include "play/performer.hpp"           /* seq66::performer class           */

namespace seq66
{

/**
 *  This constructor assigns the basic values of control name, number, and
 *  action code.  The rest of the members can be set via the set() function.
 */

midilearn::midilearn
(
    performer & p,
    bool clearcontrols
) :
    m_perf              (p),
    m_original_controls (p.midi_control_in()),
    m_current_controls  (p.midi_control_in()),
    m_control_status    (automation::ctrlstatus::none)
{
    if (clearcontrols)
        clear();
}

/**
 *  Copies the original controls into the current controls.
 */

void
midilearn::reset ()
{
    m_current_controls = m_original_controls;
}

/**
 *  Tells the perform to redirect the incoming MIDI to the
 *  midilearn object.
 */

void
midilearn::start ()
{
    // TODO
}

/**
 *  Copies the current controls to the performer.
 */

bool
midilearn::save ()
{
    bool result { true };
    if (result)
    {
        // TODO
    }
    return result;
}

}           // namespace seq66

/*
 * midilearn.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

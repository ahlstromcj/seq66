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
 * \file          rpn.cpp
 *
 *  This module declares/defines a class for handling RPN/NPRN/Data
 *  controls.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2026-07-31
 * \updates       2026-07-31
 * \license       GNU GPLv2 or above
 *
 */

#include "midi/rpn.hpp"                 /* seq66::rpn for ALSA         */

namespace seq66
{

rpn::rpn
(
    control control_type,
    parameter parameter_type,
    midipulse time_stamp,
    midishort rpn_parameter_value,
    midishort rpn_parameter_number,
    bool append_data,
    bool append_reset
) :
    m_control_type          { control_type },
    m_parameter_type        { parameter_type },
    m_time_stamp            { time_stamp },
    m_rpn_parameter_number  { rpn_parameter_number },
    m_rpn_parameter_value   { rpn_parameter_value },
    m_append_data           { append_data },
    m_append_reset          { append_reset }
{
    // no code yet
}

/**
 *  This function makes adjustments based on the settings the
 *  constructor made.
 *
 *  Note that the names of the RPNs are provided, at present, in
 *  the controllers module in the s_rpn_names[] array.
 */

void
rpn::fix_settings ()
{
    switch (m_control_type)
    {
    case control::rpn:

        m_rpn_parameter_number = parameter_to_short(m_parameter_type);
        break;

    case control::nrpn:

        break;

    case control::slider:

        break;

    case control::increment:

        break;

    case control::decrement:

        break;

    default:

        break;
    }
}

}           // namespace seq66

/*
 * rpn.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

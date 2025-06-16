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
 * \file          usermidibus.cpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  in this application.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-24
 * \updates       2021-01-21
 * \license       GNU GPLv2 or above
 *
 *  Note that this module also sets the legacy global variables, so that
 *  they can be used by modules that have not yet been cleaned up.
 */

#include "cfg/settings.hpp"             /* seq66::usr() accessor            */
#include "cfg/usermidibus.hpp"          /* seq66::usermidibus structure     */
#include "cfg/userinstrument.hpp"       /* seq66::userinstrument structure  */

namespace seq66
{

/**
 *  This constant indicates that a configuration file numeric value is
 *  the default value for specifying that an instrument is a GM
 *  instrument.  Used in the "user" configuration-file processing.
 */

static const int c_gm_instrument_flag = (-1);

/**
 *  Default constructor.
 *
 * \param name
 *      The name of the buss, valid only if it is not empty.
 */

usermidibus::usermidibus (const std::string & name) :
    m_is_valid          (false),
    m_channel_count     (0),
    m_midi_bus_def      ()
{
    clear();
    set_name(name);
}

/**
 *  Copy constructor.
 *
 * \param rhs
 *      The sources of the data for the copy.
 */

usermidibus::usermidibus (const usermidibus & rhs) :
    m_is_valid          (rhs.m_is_valid),
    m_channel_count     (rhs.m_channel_count),
    m_midi_bus_def      ()                      // a constant-size array
{
    copy_definitions(rhs);
}

/**
 *  Principal assignment operator.
 *
 * \param rhs
 *      The sources of the data for the assignment.
 *
 * \return
 *      Returns a reference to this object.
 */

usermidibus &
usermidibus::operator = (const usermidibus & rhs)
{
    if (this != &rhs)
    {
        m_is_valid = rhs.m_is_valid;
        m_channel_count = rhs.m_channel_count;
        copy_definitions(rhs);
    }
    return *this;
}

/**
 *  Sets the default values.  Also invalidates the object.  All 16 of the
 *  channels are set to (-1).
 */

void
usermidibus::clear ()
{
    m_is_valid = false;
    m_channel_count = 0;
    m_midi_bus_def.alias.clear();
    for (int channel = 0; channel < c_midichannel_max; ++channel)
        m_midi_bus_def.instrument[channel] = c_gm_instrument_flag;
}

/**
 * \getter m_midi_bus_def.instrument[channel]
 *
 * \param channel
 *      Provides the desired buss channel number.
 *
 * \return
 *      The instrument number of the desired buss channel is returned.  If
 *      the channel number is out of range, or the object is not valid,
 *      then c_gm_instrument_flag (-1) is returned.
 */

int
usermidibus::instrument (int channel) const
{
    if (m_is_valid && channel >= 0 && channel < c_midichannel_max)
        return m_midi_bus_def.instrument[channel];
    else
        return c_gm_instrument_flag;
}

std::string
usermidibus::instrument_name (int channel) const
{
    std::string result;
    if (m_is_valid && channel >= 0 && channel < c_midichannel_max)
    {
        int inumber = m_midi_bus_def.instrument[channel];
        const userinstrument & uin = usr().instrument(inumber);
        result = uin.name();
    }
    else
        result = "GM";

    return result;
}

/**
 * \getter m_midi_bus_def.instrument[channel]
 *
 *      Does not alter the validity flag, just checks it.
 *
 * \param channel
 *      Provides the desired buss channel number.
 *
 * \param instrum
 *      Provides the instrument number to set that channel to.
 */

bool
usermidibus::set_instrument (int channel, int instrum)
{
    bool result = m_is_valid && channel >= 0 && channel < c_midichannel_max;
    if (result)
    {
        m_midi_bus_def.instrument[channel] = instrum;
        if (instrum != c_gm_instrument_flag)
            ++m_channel_count;
    }
    return result;
}

/**
 *  Copies the member fields from one instance of usermidibus to this
 *  one.  Does not include the validity flag.
 */

void
usermidibus::copy_definitions (const usermidibus & rhs)
{
    m_midi_bus_def.alias = rhs.m_midi_bus_def.alias;
    for (int channel = 0; channel < c_midichannel_max; ++channel)
    {
        m_midi_bus_def.instrument[channel] =
            rhs.m_midi_bus_def.instrument[channel];
    }
}

}           // namespace seq66

/*
 * usermidibus.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


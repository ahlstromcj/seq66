#if ! defined SEQ66_USERMIDIBUS_HPP
#define SEQ66_USERMIDIBUS_HPP

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
 * \file          usermidibus.hpp
 *
 *  This module declares/defines the user MIDI-buss section of the "user"
 *  configuration file.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-24
 * \updates       2018-11-24
 * \license       GNU GPLv2 or above
 *
 *  This class replaces an global_usermidibus_definitions[] array element
 *  with a wrapper class for better safety.
 */

#include <string>

#include "app_limits.h"                 /* SEQ66_GM_INSTRUMENT_FLAG         */
#include "midi/midibytes.hpp"           /* seq66::c_midichannel_max         */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  This structure corresponds to <code> [user-midi-bus-0] </code>
 *  definitions in the <code> ~/.seq66usr </code> ("user") file
 *  (<code> ~/.config/seq66.usr </code> in the latest
 *  version of the application).
 */

struct usermidibus_t
{
    /**
     *  Provides the user's desired name for the MIDI bus.  For example,
     *  "2x2 A" for some kind of MIDI card or USB MIDI cable.  If
     *  manual-ports is enabled, this could be something like
     *  "[0] seq66 0", and that is what should be shown in that case.
     */

    std::string alias;

    /**
     *  Provides an implicit list of MIDI channels from 0 to 15 (1 to 16) and
     *  the "instrument" number assigned to each channel.  Note that the
     *  "instrument" is not a MIDI program number.  Instead, it is the number
     *  associated with a [user-instrument-definitions] section in the "user"
     *  configuration file.
     */

    int instrument[c_midichannel_max];
};

/**
 *  Provides data about the MIDI busses, readable from the "user"
 *  configuration file.  Will later make the size adjustable, if it
 *  makes sense to do so.
 *
 */

class usermidibus
{

    /**
     *  Provides a validity flag, useful in returning a reference to a
     *  bogus object for internal error-check.  Callers should check
     *  this flag via the is_valid() accessor before using this object.
     *  This flag is set to true when any valid member assignment occurs
     *  via a public setter call.
     */

    bool m_is_valid;

    /**
     *  Provides the actual number of non-default buss channels actually
     *  set.  Often, the "user" configuration file has only a few out of
     *  the 16 assigned explicitly.
     */

    int m_channel_count;

    /**
     *  The instance of the structure that this class wraps.
     */

    usermidibus_t m_midi_bus_def;

public:

    usermidibus (const std::string & name = "");
    usermidibus (const usermidibus & rhs);
    usermidibus & operator = (const usermidibus & rhs);

    /**
     * \getter m_is_valid
     */

    bool is_valid () const
    {
        return m_is_valid;
    }

    void clear ();

    /**
     * \getter m_midi_bus_def.alias (name of alias)
     */

    const std::string & name () const
    {
        return m_midi_bus_def.alias;
    }

    /**
     * \getter m_channel_count
     * \return
     *      This function returns the actual number of channels.  This is
     *      different from before, when the maximum number was always
     *      returned.
     */

    int channel_count () const
    {
        return m_channel_count;
    }

    /**
     * \setter m_channel_count
     */

    void channel_count (int count)
    {
        m_channel_count = count;
    }

    /**
     * \getter c_midichannel_max
     * \return
     *      Returns the maximum number of MIDI channels.
     *      Remember that the instrument channels for each MIDI buss
     *      range from 0 to 15 (c_midichannel_max).
     */

    int channel_max () const
    {
        return c_midichannel_max;
    }

    int instrument (int channel) const;                     // getter
    bool set_instrument (int channel, int instrum);         // setter
    std::string instrument_name (int channel) const;

private:

    /**
     * \setter m_midi_bus_def.alias (name of alias)
     *      Also sets the validity flag according to the emptiness of the
     *      name parameter.
     */

    void set_name (const std::string & name)
    {
        m_midi_bus_def.alias = name;
        m_is_valid = ! name.empty();
    }

    void copy_definitions (const usermidibus & rhs);

};

}           // namespace seq66

#endif      // SEQ66_USERMIDIBUS_HPP

/*
 * usermidibus.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


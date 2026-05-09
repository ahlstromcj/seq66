#if ! defined SEQ66_BUSARRAY_HPP
#define SEQ66_BUSARRAY_HPP

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
 * \file          busarray.hpp
 *
 *  This module declares/defines the Master MIDI Bus base class.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2026-05-09
 * \updates       2026-05-09
 * \license       GNU GPLv2 or above
 *
 *  The busarray module defines busarray classes so that we can
 *  start avoiding arrays and explicit access to them.
 *
 *  The businfo class holds a pointer to its midibus object.
 *
 *  The busarray class holds a number of businfo classes, and two busarrays
 *  are maintained, one for input and one for output.
 */

// #include <memory>                       /* std::shared_ptr<>                */
#include <vector>                       /* for containing the bus objects   */

#include "midi/businfo.hpp"             /* seq66::bussinfo                  */

namespace seq66
{
    class event;

/**
 *  Holds a number of businfo objects.
 */

class busarray
{

private:

    /**
     *  The full set of businfo objects, only some of which will actually be
     *  used.
     */

    std::vector<businfo> m_container;

public:

    busarray ();
    ~busarray ();

    bool add (midibus * bus, e_clock clock);
    bool add (midibus * bus, bool inputing);
    bool initialize ();

    int count () const
    {
        return int(m_container.size());
    }

    midibus * bus (bussbyte b)
    {
        return b < bussbyte(count()) ? m_container[b].bus() : nullptr ;
    }

    int client_id (bussbyte b)
    {
        return b < bussbyte(count()) ? m_container[b].bus()->client_id() : 0 ;
    }

    /**
     *  Starts all of the busses; used for output busses only, but no check is
     *  made at present.
     */

    void start ()
    {
        for (auto & bi : m_container)       /* vector of businfo copies     */
            bi.start();
    }

    /**
     *  Stops all of the busses; used for output busses only, but no check is
     *  made at present.
     */

    void stop ()
    {
        for (auto & bi : m_container)       /* vector of businfo copies     */
            bi.stop();
    }

    /**
     *  Continues from the given tick for all of the busses; used for output
     *  busses only.
     *
     * \param tick
     *      Provides the tick value for all busses to continue from.
     */

    void continue_from (midipulse tick)
    {
        for (auto & bi : m_container)       /* vector of businfo copies     */
            bi.continue_from(tick);
    }

    /**
     *  Initializes the clocking at the given tick for all of the busses; used
     *  for output busses only.
     *
     * \param tick
     *      Provides the tick value for all busses use as the clock tick.
     */

    void init_clock (midipulse tick)
    {
        for (auto & bi : m_container)       /* vector of businfo copies     */
            bi.init_clock(tick);
    }

    /**
     *  Clocks at the given tick for all of the busses; used for output busses
     *  only.
     *
     * \param tick
     *      Provides the tick value for all busses use as the clock tick.
     */

    void clock (midipulse tick)
    {
        for (auto & bi : m_container)       /* vector of businfo copies     */
            bi.clock(tick);
    }

    void play (bussbyte bus, const event * e24, midibyte channel);
    void sysex (bussbyte bus, const event * ev);
    bool set_clock (bussbyte bus, e_clock clocktype);

    /**
     *  Sets the clock type for all busses, usually the output buss.  Note that
     *  the settings to apply are added when the add() call is made.  This is a
     *  bit ugly.
     */

    void set_all_clocks ()
    {
        for (auto & bi : m_container)           /* vector of businfo copies */
            bi.bus()->set_clock(bi.init_clock());
    }

    e_clock get_clock (bussbyte bus) const;
    std::string get_midi_bus_name (int bus) const;  /* full display name!   */
    std::string get_midi_port_name (int bus) const; /* without the client   */
    std::string get_midi_alias (int bus) const;
    void print () const;
    void port_exit (int client, int port);
    bool set_input (bussbyte bus, bool inputing);

    /**
     *  Set the status of all input busses.  There's no implementation-specific
     *  API function here.  This function should be used only for the input
     *  busarray, obviously.  Note that the input settings used here were
     *  stored when the add() function was called.  They can be changed by the
     *  user via the Options / MIDI Input tab.
     */

    void set_all_inputs ()
    {
        for (auto & bi : m_container)       /* vector of businfo copies     */
            bi.bus()->set_input(bi.init_input());
    }

    bool get_input (bussbyte bus) const;
    bool is_system_port (bussbyte bus) const;
    bool is_port_unavailable (bussbyte bus) const;
    bool is_port_locked (bussbyte bus) const;
    int poll_for_midi ();
    bool get_midi_event (event * inev);
    int replacement_port (int bus, int port);

};          // class busarray

/*
 * Free functions
 */

extern void swap (busarray & buses0, busarray & buses1);

}           // namespace seq66

#endif      // SEQ66_BUSARRAY_HPP

/*
 * businfo.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

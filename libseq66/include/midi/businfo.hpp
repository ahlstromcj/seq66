#if ! defined SEQ66_BUSINFO_HPP
#define SEQ66_BUSINFO_HPP

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
 * \file          businfo.hpp
 *
 *  This module declares/defines the Master MIDI Bus base class.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2016-12-31
 * \updates       2022-02-23
 * \license       GNU GPLv2 or above
 *
 *  The businfo module defines the businfo and busarray classes so that we can
 *  start avoiding arrays and explicit access to them.
 *
 *  The businfo class holds a pointer to its midibus object.
 *
 *  The busarray class holds a number of businfo classes, and two busarrays
 *  are maintained, one for input and one for output.
 */

#include <memory>                       /* std::shared_ptr<>                */
#include <vector>                       /* for containing the bus objects   */

#include "midi/midibus_common.hpp"      /* enum class e_clock               */
#include "midi/midibus.hpp"             /* seq66::midibus                   */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class event;
    class midibus;

/**
 *  A new class to consolidate a number of bus-related arrays into one array.
 *  There will be in input instances and an output instance of this object
 *  contained by mastermidibus.  Inputs will be in one container, and output in
 *  another container.
 */

class businfo
{
    friend class busarray;

private:

    /**
     *  Points to an existing midibus object.
     */

    std::shared_ptr<midibus> m_bus;

    /**
     *  Indicates if the existing bus is active.
     */

    bool m_active;

    /**
     *  Indicates if the existing bus is initialized.
     */

    bool m_initialized;

    /**
     *  Clock initialization, if this businfo is stored in an output container.
     */

    e_clock m_init_clock;

    /**
     *  Input initialization, if the businfo is stored in an output container.
     */

    bool m_init_input;

public:

    businfo () = delete;
    businfo (midibus * bus);
    businfo (const businfo & rhs);
    ~businfo () = default;              // the bus pointer is self-deleting

    /**
     *  Deletes and nullifies the m_bus pointer.
     */

    void remove ()
    {
        if (bool(m_bus))
            m_bus.reset();
    }

    const midibus * bus () const
    {
        return m_bus.get();
    }

    midibus * bus ()
    {
        return m_bus.get();
    }

    bool active () const
    {
        return m_active;
    }

    bool initialize ();

    bool initialized () const
    {
        return m_initialized;
    }

    e_clock init_clock () const
    {
        return m_init_clock;
    }

    bool init_input () const
    {
        return m_init_input;
    }

public:

    void activate ()
    {
        m_active = m_initialized = true;
    }

    void deactivate ()
    {
        m_active = m_initialized = false;
    }

    void init_clock (e_clock clocktype)
    {
        m_init_clock = clocktype;
        if (not_nullptr(bus()))
            bus()->set_clock(clocktype);
    }

    void init_input (bool flag)
    {
        m_init_input = flag;

        /*
         * When clicking on the MIDI Input item, this is not needed...
         * it disables the detection of a change, so that init() and deinit()
         * do not get called.
         *
         * When starting up we need to honor the init-input flag if it is
         * set, and init() the bus.  But we don't need to call deinit() at
         * startup if it is false, since init() hasn't been called yet.
         */

        if (not_nullptr(bus()))
            bus()->set_input_status(flag);
    }

private:

    void start ()
    {
        bus()->start();
    }

    void stop ()
    {
        bus()->stop();
    }

    void continue_from (midipulse tick)
    {
        bus()->continue_from(tick);
    }

    void init_clock (midipulse tick)
    {
        bus()->init_clock(tick);
    }

    void clock (midipulse tick)
    {
        bus()->clock(tick);
    }

    void sysex (const event * ev)
    {
        bus()->sysex(ev);
    }

private:

    void print () const;

};          // class businfo

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
     *  busarray, obviously.  Note that the input settings used here were stored
     *  when the add() function was called.  They can be changed by the user via
     *  the Options / MIDI Input tab.
     */

    void set_all_inputs ()
    {
        for (auto & bi : m_container)       /* vector of businfo copies     */
            bi.bus()->set_input(bi.init_input());
    }

    bool get_input (bussbyte bus) const;
    bool is_system_port (bussbyte bus);
    int poll_for_midi ();
    bool get_midi_event (event * inev);
    int replacement_port (int bus, int port);

};          // class busarray

/*
 * Free functions
 */

extern void swap (busarray & buses0, busarray & buses1);

}           // namespace seq66

#endif      // SEQ66_BUSINFO_HPP

/*
 * businfo.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


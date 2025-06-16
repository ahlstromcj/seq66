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
 * \file          businfo.cpp
 *
 *  This module declares/defines the base class for handling MIDI I/O via
 *  the ALSA system.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2016-12-31
 * \updates       2023-09-20
 * \license       GNU GPLv2 or above
 *
 *  This file provides a base-class implementation for various master MIDI
 *  buss support classes.  There is a lot of common code between these MIDI
 *  buss classes.
 *
 * Bus Info:
 *
 *      -#  For each MIDI I/O port, a new midibus object and pointer is created
 *          in one of the following seq_rtmidi functions:
 *          -   midi_alsa_info::api_port_start()
 *          -   mastermidibus::api_init()
 *          Both function pass in a mastermidibus object as well.
 *      -#  In busarray::add (), this pointer is passed to the constructor of
 *          businfo.  There, it is managed by an std::shared<> pointer. The
 *          local pointer in step 1 goes out of scope.
 *      -#  This copy of businfo is init'ed and then pushed onto the busarray's
 *          businfo container.
 */

#include "cfg/settings.hpp"             /* seq66::rc() and seq66::usr()     */
#include "midi/businfo.hpp"             /* seq66::businfo class             */
#include "midi/event.hpp"               /* seq66::event class               */

namespace seq66
{

/*
 * -------------------------------------------------------------------------
 * class businfo
 * -------------------------------------------------------------------------
 */

/**
 *  Principal constructor.
 *
 * is_input_port():
 *
 *      Indicates if the midibus represents an input port (true) versus an
 *      output port (false).
 *
 * is_virtual_port():
 *
 *      Indicates if the midibus represents a virtual port (true) versus a
 *      normal port (false).
 *
 * \param bus
 *      Provides a pointer to the MIDI buss object to be represented by this
 *      object.
 */

businfo::businfo (midibus * bus)
 :
    m_bus           (),                 /* this is an std::shared_ptr<>     */
    m_active        (false),
    m_initialized   (false),
    m_init_clock    (e_clock::off),     /* could end up disabled as well    */
    m_init_input    (false)
{
    m_bus.reset(bus);                   /* also see initialize()            */
}

/**
 *  Copy constructor.
 *
 * \param rhs
 *      The source object to be copied.
 */

businfo::businfo (const businfo & rhs)
 :
    m_bus           (rhs.m_bus),
    m_active        (rhs.m_active),
    m_initialized   (rhs.m_initialized),
    m_init_clock    (rhs.m_init_clock),
    m_init_input    (rhs.m_init_input)
{
    // no other code needed
}

/**
 *  This function is called when the businfo object is added to the busarray.
 *  It relies on the performer::launch() function to actually activate() all of
 *  the ports that have been flagged as "activated" here.
 *
 * is_input_port():
 *
 *      Indicates if the midibus represents an input port (true) versus an
 *      output port (false).  The way the mastermidibus currently works, it
 *      creates the API MIDI input objects there, so it does not need to be
 *      done here.  This falls under the heading of "tricky code".
 *
 * is_virtual_port():
 *
 *      Indicates if the midibus represents a manual/virtual port (true)
 *      versus a normal port (false).
 *
 *  The rules for port initialization follow those of seq66 for MIDI busses:
 *
 *      -   Manual (virtual) input and output ports always get their init
 *          functions called.  They are unconditionally marked as "active"
 *          and "initialized".
 *      -   Normal output ports are marked as "active" and "initialized" if
 *          init_out() succeeds.
 *      -   Normal input ports don't have init_in() called, but are marked
 *          as "active" and "initialized" anyway.  The settings from the "rc"
 *          file determine which inputs will operate.
 *
 *  We still have a potential conflict between "active", "initialized", and
 *  "disabled".
 *
 *      "Active" is used for:  enabling play(), set_clock(), get_clock(),
 *      get_midi_bus_name(), set_input(), port_enabled(), is_system_port(),
 *      replacement_port().
 *
 *      "Initialized" is used for: . . .
 *
 *      "Disabled" is currently used to making an OS-disabled, non-openable port
 *      a non-fatal error.
 *
 * Unavailable ports:
 *
 *      In Windows, a port can be present on the system, but be unavailable
 *      because the MIDI Mapper has taken control of it. Can a port be
 *      unavailable for a similar reason (e.g. grabbed by another application)
 *      in Linux?
 *
 * \return
 *      Returns true if the buss is value, and it could be initialized (as an
 *      output port or a virtual output port.  If bus has been "disabled"
 *      (e_clock::disabled), skip this port and return true.
 */

bool
businfo::initialize ()
{
    bool result = not_nullptr(bus());           /* a bit too tricky now     */
    if (result)
    {
        result = bus()->initialize(rc().init_disabled_ports());
        if (result)
        {
            activate();                         /* "initialized" & "active" */
        }
        else
        {
#if defined SEQ66_PLATFORM_WINDOWS_DISABLED     /* don't allow it1!!        */
            result = true;                      /* allow it for Windoze     */
#endif
            bus()->set_port_unavailable();      /* currently permanent      */
        }
    }
    else
    {
        errprint("businfo(): null midibus pointer provided");
    }
    return result;
}

/**
 *  Print some information about the MIDI bus.
 */

void
businfo::print () const
{
    std::string flags;
    if (bus()->is_virtual_port())
        flags += " virtual";
    else if (bus()->is_system_port())
        flags += " system";
    else
        flags += " normal";

    if (bus()->is_input_port())
        flags += " input";
    else
        flags += " output";

    if (active())
        flags += " active";
    else
        flags += bus()->port_unavailable() ? "unavailable" : " inactive" ;

    if (initialized())
        flags += " initialized";
    else
        flags += " uninitialized";

    if (bus()->is_input_port())
    {
        flags += " ";
        flags += init_input() ? "inputting" : "not inputting" ;
    }
    else
    {
        flags += " clock ";
        if (init_clock() == e_clock::off)
            flags += "Off";
        else if (init_clock() == e_clock::pos)
            flags += "Pos";
        else if (init_clock() == e_clock::mod)
            flags += "Mod";
        else if (init_clock() == e_clock::disabled)
            flags += "Disabled";
        else
            flags += "illegal!";
    }
    printf
    (
        "  %s:%s %s\n",
        bus()->bus_name().c_str(), bus()->port_name().c_str(), flags.c_str()
    );
}

/*
 * -------------------------------------------------------------------------
 * class busarray
 * -------------------------------------------------------------------------
 */

/**
 *  A new class to hold a vector of MIDI busses and flags for more controlled
 *  access than using arrays of booleans and pointers.
 */

busarray::busarray () : m_container()
{
    // Empty body
}

/**
 *  Removes components from the container.
 *
 * \question
 *  However, now that we swap containers, we cannot call this functionality,
 *  because it deletes the bus's midibus pointer and nullifies it.
 *  But we do call it, and it seems to work.
 */

busarray::~busarray ()
{
    for (auto & bi : m_container)       /* vector of businfo copies         */
        bi.remove();                    /* deletes the businfo's midibus    */
}

/**
 *  Creates and adds a new midibus object to the list.  Then the clock value
 *  is set.  This function is meant for output ports.
 *
 *  We need to belay the initialization until later, when we know the
 *  configured clock settings for the output ports.  So initialization has
 *  been removed from the constructor and moved to the initialize() function.
 *
 * \param bus
 *      The midibus to be hooked into the array of busses.
 *
 * \param clock
 *      The clocking value for the bus.
 *
 * \return
 *      Returns true if the bus was added successfully, though, really, it
 *      cannot fail.
 */

bool
busarray::add (midibus * bus, e_clock clock)
{
    bool result = not_nullptr(bus);
    if (result)
    {
        size_t count = m_container.size();
        businfo b(bus);
        b.init_clock(clock);
        m_container.push_back(b);                       /* creates a copy   */
        result = m_container.size() == (count + 1);
    }
    return result;
}

/**
 *  Adds a new midibus object to the list.  Then the inputing value
 *  is set.  This function is meant for input ports.
 *
 *  We need to belay the initialization until later, when we know the
 *  configured inputing settings for the input ports.  So initialization has
 *  been removed from the constructor and moved to the initialize() function.
 *  However, now we know the configured status and can apply it right away.
 *
 * \param bus
 *      The midibus to be hooked into the array of busses.
 *
 * \param inputing
 *      The input flag value for the bus.  If true, this value indicates that
 *      the user has selected this bus to be the input MIDI bus.
 *
 * \return
 *      Returns true if the bus was added successfully, though, really, it
 *      cannot fail.
 */

bool
busarray::add (midibus * bus, bool inputing)
{
    bool result = not_nullptr(bus);
    if (result)
    {
        size_t count = m_container.size();
        businfo b(bus);
        b.init_input(inputing);                 /* sets the flag, important */
        m_container.push_back(b);               /* now we can push a copy   */
        result = m_container.size() == (count + 1);
    }
    return result;
}

/**
 *  Initializes all busses.  Not sure we need this function.
 *
 * \return
 *      Returns true if all busses initialized successfully.  It currently keeps
 *      going even after a failure, though.
 */

bool
busarray::initialize ()
{
    bool result = true;
    for (auto & bi : m_container)           /* vector of businfo copies     */
    {
        if (! bi.initialize())
            result = false;
    }
    return result;
}

/**
 *  Plays an event, if the bus is proper.
 *
 * \param bus
 *      The MIDI buss on which to play the event.  The buss number must be
 *      valid (in range) and the bus must be active.
 *
 * \param e24
 *      A pointer to the event to be played.  Currently we don't bother to check
 *      it!
 *
 * \param channel
 *      The MIDI channel on which to play the event.  Seq66 controls
 *      the actual channel of playback, no matter what the channel specified
 *      in the event.
 */

void
busarray::play (bussbyte bus, const event * e24, midibyte channel)
{
    if (bus < count() && m_container[bus].active())
        m_container[bus].bus()->play(e24, channel);
}

/**
 *  Handles SysEx events; used for output busses.
 *
 * \param ev
 *      Provides the SysEx event to handle.
 */

void
busarray::sysex (bussbyte bus, const event * e24)
{
    if (bus < count() && m_container[bus].active())
        m_container[bus].bus()->sysex(e24);
}

/**
 *  Sets the clock type for the given bus, usually the output buss.
 *  This code is a bit more restrictive than the original code in
 *  mastermidibus::set_clock().
 *
 *  Getting the current clock setting is essentially equivalent to:
 *
 *      m_container[bus].bus()->get_clock();
 *
 * ca  2023-05-18
 *
 *  The check for a change in status is commented out because it
 *  can disable setting values for the same item as stored in the portmap.
 *  Need to investigate this at some point.
 *
 * \param bus
 *      The MIDI bus for which the clock is to be set.
 *
 * \param clocktype
 *      Provides the type of clocking for the buss.
 *
 * \return
 *      Returns true if the change was made.  It is made only if needed.
 */

bool
busarray::set_clock (bussbyte bus, e_clock clocktype)
{
    e_clock current = get_clock(bus);
    bool result = bus < count();
    if (result)
    {
        businfo & bi = m_container[bus];
        result = bi.active() || current == e_clock::disabled;
        if (result)
            bi.init_clock(clocktype);           /* also handles set_clock() */
    }
    return result;
}

/**
 *  Gets the clock type for the given bus, usually the output buss.
 *
 * \param bus
 *      The MIDI bus for which the clock is to be set.
 *
 * \return
 *      Returns the clock value set for the desired buss.  If the buss is
 *      invalid, e_clock::off is returned.  If the buss is not active, we still
 *      get the existing clock value.  The theory here is that we don't want
 *      to junk the current clock value; it could alter what was read from the
 *      "rc" file.
 */

e_clock
busarray::get_clock (bussbyte bus) const
{
    if (bus < count())
    {
        const businfo & bi = m_container[bus];
        return bi.bus()->get_clock();
    }
    else
        return e_clock::off;
}

/**
 *  Get the MIDI output buss name (i.e. the full display name) for the given
 *  (legal) buss number.
 *
 *  This function adds the retrieval of client and port numbers that are not
 *  needed in the portmidi implementation, but seem generally useful to
 *  support in all implementations.  It's main use is to display the
 *  full portname in one of two forms:
 *
 *      -   "[0] 0:0 clientname:portname"
 *      -   "[0] 0:0 portname"
 *
 *  The second version is chosen if "clientname" is already included in the
 *  port name, as many MIDI clients do that.  However, the name gets
 *  modified to reflect the remote system port to which it will connect.
 *
 * \param bus
 *      Provides the output buss number.  Checked before usage.
 *      Actually should now be an index number
 *
 * \return
 *      Returns the buss name as a standard C++ string, truncated to 80-1
 *      characters.  Also contains an indication that the buss is disconnected
 *      or unconnected.  If the buss number is illegal, this string is empty.
 */

std::string
busarray::get_midi_bus_name (int bus) const
{
    std::string result;
    if (bus < count())
    {
        const businfo & bi = m_container[bus];
        const midibus * buss = bi.bus();
        e_clock current = buss->get_clock();
        if (bi.active() || current == e_clock::disabled)
        {
            std::string busname = buss->bus_name();
            std::string portname = buss->port_name();
            std::size_t len = busname.size();
            int test = busname.compare(0, len, portname, 0, len);
            if (test == 0)
            {
                char tmp[80];
                snprintf
                (
                    tmp, sizeof tmp, "[%d] %d:%d %s",
                    bus, buss->bus_id(),
                    buss->port_id(), portname.c_str()
                );
                result = tmp;
            }
            else
                result = buss->display_name();
        }
        else
        {
            /*
             * The display name gets saved, and so must be used unaltered
             * here.
             */

            result = buss->display_name();
        }
    }
    return result;
}

/**
 *  The function gets the port name.  We're trying to keep our own client name
 *  (normally "seq66") out of the 'rc' input and clock sections.
 */


std::string
busarray::get_midi_port_name (int bus) const
{
    std::string result;
    if (bus < count())
    {
        const businfo & bi = m_container[bus];
        const midibus * buss = bi.bus();
        result = buss->port_name();
    }
    return result;
}

/**
 *  This function gets only the alias name of the port, if it exists.
 *  Some (later) versions of JACK support getting the alias in the manner
 *  of "a2jmidid --export-hw", which can be used to use the device's model name
 *  rather that some generic name.
 */

std::string
busarray::get_midi_alias (int bus) const
{
    std::string result;
    if (bus < count())
    {
        const businfo & bi = m_container[bus];
        const midibus * buss = bi.bus();
        result = buss->port_alias();
    }
    return result;
}

/**
 *  Print some information about the available MIDI input or output busses.
 */

void
busarray::print () const
{
    printf("Available busses:\n");
    for (const auto & bi : m_container)         /* vector of businfo copies */
        bi.print();
}

/**
 *  Turn off the given port for the given client.  Both the busses for the given
 *  client are stopped: that is, set to inactive.
 *
 *  This function is called by api_get_midi_event() when the ALSA event
 *  SND_SEQ_EVENT_PORT_EXIT is received.  Since port_exit() has no direct
 *  API-specific code in it, we do not need to create a virtual
 *  api_port_exit() function to implement the port-exit event.
 *
 * \param client
 *      The client to be matched and acted on.  This value is actually an ALSA
 *      concept.
 *
 * \param port
 *      The port to be acted on.  Both parameter must be matched before the
 *      buss is made inactive.  This value is actually an ALSA concept.
 */

void
busarray::port_exit (int client, int port)
{
    for (auto & bi : m_container)               /* vector of businfo copies */
    {
        if (bi.bus()->match(client, port))
           bi.deactivate();
    }
}

/**
 *  Set the status of the given input buss, if a legal buss number.  There's
 *  currently no implementation-specific API function called directly here.
 *  What happens is that midibase::set_input() uses the \a inputing parameter
 *  to decide whether to call init_in() or deinit_in(), and these functions
 *  ultimately lead to an API specific called.
 *
 *  Note that the call to midibase::set_input() will set its m_inputing flag,
 *  and then call init_in() or deinit_in() if that flag changed. This change
 *  is important, so we have to call midibase::set_input() first. Then the
 *  call to businfo::init_input() will set that flag again (plus another
 *  flag).  A bit confusing in sequence and in function naming.
 *
 *  This function should be used only for the input busarray, obviously.
 *
 * ca  2023-05-18
 *
 *  The check for a change in status is commented out because it
 *  can disable setting values for the same item as stored in the portmap.
 *  Need to investigate this at some point.
 *
 * \threadsafe
 *
 * \param bus
 *      Provides the buss number.
 *
 * \param inputing
 *      True if the input bus will be inputting MIDI data.
 *
 * \return
 *      Returns true if the buss number is valid and was active, and so could
 *      be set.
 */

bool
busarray::set_input (bussbyte bus, bool inputing)
{
    bool current = get_input(bus);                          /* see below    */
    bool result = bus < count();
    if (result)
    {
        businfo & bi = m_container[bus];

        /*
         *  The init_input() call here first sets the m_init_input flag in
         *  businfor.  Then it sets the I/O status flag of the midibus. It
         *  does this only if the businfo is active (i.e. initialized) and the
         *  status has changed.
         */

        result = bi.active() || ! current;
        if (result)
            bi.init_input(inputing);
    }
    return result;
}

/**
 *  Get the input for the given (legal) buss number.
 *
 *  There's currently no implementation-specific API function here.
 *
 * \param bus
 *      Provides the buss number.
 *
 * \return
 *      If the buss is a system buss, always returns true.  Otherwise, if the
 *      buss is inactive, returns false. Otherwise, the buss's port_enabled()
 *      status is returned.
 */

bool
busarray::get_input (bussbyte bus) const
{
    bool result = false;
    if (bus < count())
    {
        const businfo & bi = m_container[bus];
        if (bi.active())
            result = bi.bus()->is_system_port() ?
                true : bi.bus()->port_enabled();
    }
    return result;
}

/**
 *  Get the system-port status for the given (legal) buss number.
 *
 * \param bus
 *      Provides the buss number.
 *
 * \return
 *      Returns the selected buss's is-system-port status.  If the buss number
 *      is out of range, then false is returned.
 */

bool
busarray::is_system_port (bussbyte bus) const
{
    bool result = false;
    if (bus < count())
    {
        const businfo & bi = m_container[bus];
        if (bi.active())
            result = bi.bus()->is_system_port();
    }
    return result;
}

bool
busarray::is_port_unavailable (bussbyte bus) const
{
    bool result = true;
    if (bus < count())
    {
        const businfo & bi = m_container[bus];
        result = bi.bus()->port_unavailable();
    }
    return result;
}

/**
 *  Modified to account for the fact that if a port has not been
 *  added to the array, it cannot be locked.  This check is made by
 *  comparing the bus to the count().
 *
 * \param bus
 *      The buss/port number.
 */

bool
busarray::is_port_locked (bussbyte bus) const
{
    bool result = false;
    if (bus < count())
    {
        const businfo & bi = m_container[bus];
        result = bi.bus()->is_port_locked();
    }
    return result;
}

/**
 *  Initiate a poll() on the existing poll descriptors.  This is a primitive
 *  poll, which exits when some data is obtained.  It also applies only to the
 *  input busses.
 *
 *  One issue is that we have no way of knowing here which MIDI input device
 *  has MIDI input events waiting.  Should we randomize the order of polling
 *  in order to avoid starving some input devices?
 *
 * \return
 *      Returns the number of MIDI events detected on one of the busses.  Note
 *      that this is no longer a boolean value.
 */

int
busarray::poll_for_midi ()
{
    int result = 0;
    for (auto & bi : m_container)               /* vector of businfo copies */
    {
        result = bi.bus()->poll_for_midi();     /* works if I/O active      */
        if (result > 0)
            break;
    }
    return result;
}

/**
 *  Gets the first MIDI event in finds on an input bus.
 *
 *  Note that this function risks starving the second input device if more
 *  than one is enabled in Seq66.  We will figure that one out later.
 *
 * \param inev
 *      A pointer to the event to be modified by incoming data, if any.
 *
 * \return
 *      Returns true if an event's data was copied into the event pointer.
 */

bool
busarray::get_midi_event (event * inev)
{
    for (auto & bi : m_container)               /* vector of businfo copies */
    {
        if (bi.bus()->get_midi_event(inev))
        {
            bussbyte b = bussbyte(bi.bus()->bus_index());
            inev->set_input_bus(b);
#if defined SEQ66_PLATFORM_DEBUG_TMI
            printf("[seq66] input event on bus %d\n", int(b));
#endif
            return true;
        }
    }
    return false;
}

/**
 *  Provides a function to use in api_port_start(), to determine if the port
 *  is to be a "replacement" port.  This function is meant only for the output
 *  buss (so far).
 *
 *  Still need to determine exactly what this function needs to do.
 *
 * \param bus
 *      The buss to be affected.
 *
 * \param port
 *      The prot to be affected.
 *
 * \return
 *      Returns -1 if no matching port is found, otherwise it returns the
 *      replacement-port number.
 */

int
busarray::replacement_port (int bus, int port)
{
    int result = -1;
    int counter = 0;
    for (auto bi = m_container.begin(); bi != m_container.end(); ++bi)
    {
        if (bi->bus()->match(bus, port) && ! bi->active())
        {
            result = counter;
            if (bool(bi->bus()))
            {
                (void) m_container.erase(bi);   /* deletes m_bus as well    */
                errprintf("port_start(): bus out %d not null\n", result);
            }
            break;
        }
        ++counter;
    }
    return result;
}

/**
 *  This free function swaps the contents of two busarray objects.
 *
 * \param buses0
 *      Provides the first buss in the swap.
 *
 * \param buses1
 *      Provides the second buss in the swap.
 */

void
swap (busarray & buses0, busarray & buses1)
{
    busarray temp = buses0;
    buses0 = buses1;
    buses1 = temp;
}

}           // namespace seq66

/*
 * businfo.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


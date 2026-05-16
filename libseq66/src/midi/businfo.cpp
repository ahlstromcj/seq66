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
 * \updates       2026-05-16
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

/*
 * Weird issue with incomplete type "seq66::midibus" in the Windows
 * build. The first macro is defined a 1 in a meson build, and the
 * latter is defined in a qmake build.
 */

#if SEQ66_QMAKE_RULES
#include "midibus_pm.hpp"               /* seq66::midibus class             */
#else
#include "midi/midibus.hpp"             /* seq66::midibus                   */
#endif

namespace seq66
{

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
    m_bus           (bus),              /* this is an std::shared_ptr<>     */
    m_active        (false),
    m_initialized   (false),
    m_init_clock    (e_clock::none),    /* could end up disabled as well    */
    m_init_input    (false)
{
    // m_bus.reset(bus);                /* also see initialize()            */
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

/*
 * Moved from the header to here to avoid "incomplete type" error.
 */

const midibus *
businfo::bus () const
{
    return m_bus;
}

midibus *
businfo::bus ()
{
    return m_bus;
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
 * Accessor functions formerly inlined in the header.
 */

void
businfo::init_clock (e_clock clocktype)
{
    m_init_clock = clocktype;
    if (not_nullptr(bus()))
        bus()->set_clock(clocktype);
}

/*
 * When clicking on the MIDI Input item, this is not needed...  it disables
 * the detection of a change, so that init() and deinit() do not get called.
 *
 * When starting up we need to honor the init-input flag if it is set, and
 * init() the bus.  But we don't need to call deinit() at startup if it is
 * false, since init() hasn't been called yet.
 */

void
businfo::init_input (bool flag)
{
    m_init_input = flag;
    if (not_nullptr(bus()))
        bus()->set_io_status(flag);
}

void
businfo::start ()
{
    bus()->start();
}

void
businfo::stop ()
{
    bus()->stop();
}

void
businfo::continue_from (midipulse tick)
{
    bus()->continue_from(tick);
}

void
businfo::init_clock (midipulse tick)
{
    bus()->init_clock(tick);
}

void
businfo::clock (midipulse tick)
{
    bus()->clock(tick);
}

void
businfo::sysex (const event * ev)
{
    bus()->sysex(ev);
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
        if (init_clock() == e_clock::none)
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

}           // namespace seq66

/*
 * businfo.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

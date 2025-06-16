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
 * \file          midibus.cpp (rtmidi)
 *
 *  This module declares/defines the base class for MIDI I/O under one of
 *  the rtmidi frameworks.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2016-11-21
 * \updates       2025-01-20
 * \license       GNU GPLv2 or above
 *
 *  This file provides a cross-platform implementation of the midibus class.
 *  Based on our super-heavily refactored version of the RtMidi project
 *  included in this library.  Currently only ALSA and JACK are supported.
 */

#include "cfg/settings.hpp"             /* seq66::rc()                      */
#include "midi/event.hpp"               /* seq66::event and macros          */
#include "midibus_rm.hpp"               /* seq66::midibus for rtmidi        */
#include "rtmidi.hpp"                   /* RtMidi updated API header file   */
#include "rtmidi_info.hpp"              /* seq66::rtmidi_info (new)         */

namespace seq66
{

/**
 *  Normal-port and virtual-port constructor.
 *
 * \param rt
 *      Provides the rtmidi_info object to use to obtain the
 *      client ID (buss ID), port ID, and port name, as obtained via calls to
 *      the ALSA, JACK, Core MIDI, or Windows MM subsystems.
 *      We need it to provide the single ALSA "handle" needed in the in
 *      Seq66 buss model, where the master MIDI buss provides it to be
 *      used by all the MIDI buss objects.
 *
 * \param index
 *      This is the index into the rtmidi object, and is used to get the
 *      desired client and port information.  It is an index into the
 *      info container held by the rtmidi object.
 *
 * \param iotype
 *      Indicates that the port is an input port, as opposed to an output port.
 *      However, in JACK we need to swap the I/O semantics. For now we must
 *      let the caller (mastermidibus) do this.
 *
 * \param porttype
 *      Indicates that the port is also a system port (i.e. always present) or
 *      indicates that the port is virtual, as opposed to normal.
 *
 * \param bussoverride
 *      Optional buss ID, if not equal to the index parameter.
 */

midibus::midibus
(
    rtmidi_info & rt,
    int index,
    midibase::io iotype,
    midibase::port porttype,
    int bussoverride
) :
    midibase
    (
        rt.app_name(),
        rt.get_bus_name(index),
        rt.get_port_name(index),
        index,
        is_good_buss(bussoverride) ? bussoverride : rt.get_bus_id(index),
        rt.get_port_id(index),          /* ca 2023-05-07 was "index"        */
        rt.global_queue(),
        rt.ppqn(), rt.bpm(),
        iotype,                         /* perhaps I/O is swapped (JACK)    */
        porttype,
        rt.get_port_alias(index)
    ),
    m_rt_midi       (nullptr),
    m_master_info   (rt)                /* master_info() accessor           */
{
    if (porttype == port::manual)
    {
        /*
         * Set the buss ID for virtual ports to 0.  We might consider another
         * number.
         */

        bus_name(rc().app_client_name());
        if (is_null_buss(bus_id()))
            set_bus_id(0);

        bool isinput = iotype == midibase::io::input;
        std::string pname = "midi ";
        pname += isinput ? "in" : "out";
        if (index >= 0)
        {
            pname += " ";
            pname += std::to_string(index);
            port_name(pname);
            set_port_id(index);
            set_bus_id(index);
            set_name(rt.app_name(), bus_name(), port_name());
        }
    }
    else
    {
        int portcount = rt.get_port_count();
        if (index < portcount)
        {
            int id = rt.get_port_id(index);
            if (id >= 0)
                set_port_id(id);

            id = rt.get_bus_id(index);
            if (id >= 0)
                set_bus_id(id);

            /*
             * This is called with identical parameters in the midibase class.
             *
             *   set_name
             *   (
             *      rt.app_name(), rt.get_bus_name(index),
             *      rt.get_port_name(index)
             *   );
             */
        }
    }
}

/**
 *  The destructor closes out the RtMidi MIDI infrastructure.
 */

midibus::~midibus ()
{
    if (not_nullptr(m_rt_midi))
    {
        delete m_rt_midi;
        m_rt_midi = nullptr;
    }
}

/**
 *  We want to see if we can get by without this full check:
 *
 *      return not_nullptr(m_rt_midi) && m_rt_midi->have_api();
 */

bool
midibus::good_api () const
{
    return not_nullptr(m_rt_midi);
}

/**
 *  Connects to another port.  If the port is an input port, but is not
 *  configured (by the user or the "rc" configuration file), then it is not
 *  connected, and this is not an error.  Output ports are always connected.
 *
 *  WHY IS THE ABOVE THE CASE?
 *
 *  Note that the api_connect() function will errprint is own errors.  But if
 *  we expected to be able to connect, and have a null rt_midi pointer, then
 *  this is a reported error.
 *
 * \return
 *      Returns true if the connection was made successfully.
 */

bool
midibus::api_connect ()
{
    bool result = good_api();
    if (result)
    {
        result = m_rt_midi->api_connect();
    }
    else
    {
        char temp[80];
        snprintf
        (
            temp, sizeof temp, "null pointer port '%s'",
            display_name().c_str()
        );
        errprintfunc(temp);
    }
    return result;
}

/**
 *  Polls for MIDI events.  This is the API implementation for RtMidi.
 *
 *  This should work only for input busses, so we need to insure this at some
 *  point.  Currently, this is the domain of the master bus.  We also should
 *  make this routine just check the input queue size and then read the queue.
 *  Note that the ALSA handle checks incoming MIDI events and either passes
 *  them to the callback function or pushes them onto the input queue.
 *
 * \return
 *      Returns 0 if the polling succeeded, and 1 if it failed.  If the buss
 *      hasn't been initialized, it has a null m_rt_midi pointer, and will
 *      return 0.  This can happen normally when a MIDI input port is
 *      configured to be disabled.
 */

int
midibus::api_poll_for_midi ()
{
    if (port_enabled())
        return good_api() ? m_rt_midi->api_poll_for_midi() : 0 ;
    else
        return 0;
}

/**
 *  Gets a MIDI event.
 *
 * \param inev
 *      The location to deposit the MIDI event data.
 *
 * \return
 *      Returns true if a MIDI event was obtained.
 */

bool
midibus::api_get_midi_event (event * inev)
{
    if (port_enabled())
    {
        return good_api() ?
            m_rt_midi->api_get_midi_event(inev) : false ;
    }
    else
        return false;
}

/**
 *  Initializes the MIDI output port.
 *
 *  Currently, we use the default values for the rtmidi API, the queue number,
 *  and the queue size.
 *
 * \return
 *      Returns true if the output port was successfully opened.
 */

bool
midibus::api_init_out ()
{
    bool result = false;
    try
    {
        m_rt_midi = new rtmidi_out(*this, master_info());
        result = m_rt_midi->api_init_out();
    }
    catch (const rterror & err)
    {
        err.print_message();
    }
    return result;
}

/**
 *  Initializes the MIDI virtual output port.
 *
 * \return
 *      Returns true if the output port was successfully opened.
 */

bool
midibus::api_init_out_sub ()
{
    bool result = false;
    try
    {
        m_rt_midi = new rtmidi_out(*this, master_info());
        result = m_rt_midi->api_init_out_sub();
    }
    catch (const rterror & err)
    {
        err.print_message();
    }
    return result;
}

/**
 *  Initializes the MIDI input port.  Note that this port already exists if
 *  we're coming back from suspending the port.
 *
 * \return
 *      Returns true if the input port was successfully opened.
 */

bool
midibus::api_init_in ()
{
    bool result = false;
    try
    {
        if (is_nullptr(m_rt_midi))
            m_rt_midi = new rtmidi_in(*this, master_info());

        result = good_api();
        if (result)
            result = m_rt_midi->api_init_in();
    }
    catch (const rterror & err)
    {
        err.print_message();
    }
    return result;
}

/**
 *  Initializes the MIDI virtual input port.
 *
 * \return
 *      Returns true if the input port was successfully opened.
 */

bool
midibus::api_init_in_sub ()
{
    bool result = false;
    try
    {
        m_rt_midi = new rtmidi_in(*this, master_info());
        result = good_api();
        if (result)
            result = m_rt_midi->api_init_in_sub();
    }
    catch (const rterror & err)
    {
        err.print_message();
    }
    return result;
}

/**
 *  Seems to cause no ill effects. :-D
 */

bool
midibus::api_deinit_out ()
{
    return good_api() ? m_rt_midi->api_deinit_out() : false ;
}


/**
 *  Forwards the de-initialization call to the API object that implements
 *  it.  We don't bother checking the m_rt_midi pointer.  If it is null,
 *  it is the programmer's fault.
 *
 * \return
 *      Returns the result of the m_rt_midi->api_deinit_in() call.
 */

bool
midibus::api_deinit_in ()
{
    return good_api() ? m_rt_midi->api_deinit_in() : false ;
}

/**
 *  Takes a native event, and encodes to a Windows message, and writes it to
 *  the queue.  It fills a small byte buffer, sets the MIDI channel, make a
 *  message of it, and writes the message.
 *
 *  Again, DO WE NEED to distinguish between input and output here?
 *
 *  Note that we're doing a double-forwarding here, which may lower
 *  throughput.
 *
 * \param e24
 *      The MIDI event to play.
 *
 * \param channel
 *      The channel on which to play the event.
 */

void
midibus::api_play (const event * e24, midibyte channel)
{
    if (good_api())
        m_rt_midi->api_play(e24, channel);
}

void
midibus::api_sysex (const event * e24)
{
    if (good_api())
        m_rt_midi->api_sysex(e24);
}

/**
 *  Continue from the given tick.  This function implements only the
 *  RtMidi-specific code.
 *
 *  Note that, unlike in PortMidi, here we do not deal with zeroing the event
 *  timestamp.
 *
 * \param tick
 *      The tick to continue from; unused in the RtMidi API implementation.
 *
 * \param beats
 *      The calculated beats.  This calculation is made in the
 *      midibase::continue_from() function.
 */

void
midibus::api_continue_from (midipulse tick, midipulse beats)
{
    if (good_api())
        m_rt_midi->api_continue_from(tick, beats);
}

/**
 *  Sets the MIDI clock a-runnin', if the clock type is not e_clock::off.
 *  This function is called by midibase::start().  No timestamp handling.
 */

void
midibus::api_start ()
{
    if (good_api())
        m_rt_midi->api_start();
}

/**
 *  Stops the MIDI clock, if the clock-type is not e_clock::off.
 *  This function is called by midibase::stop().  No timestamp handling.
 */

void
midibus::api_stop ()
{
    if (good_api())
        m_rt_midi->api_stop();
}

/**
 *  Generates MIDI clock.  This function is called by midibase::clock().  No
 *  timestamp handling.
 *
 * \param tick
 *      The clock tick value.
 */

void
midibus::api_clock (midipulse tick)
{
    if (good_api())
        m_rt_midi->api_clock(tick);
}

}           // namespace seq66

/*
 * midibus.cpp (rtmidi)
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

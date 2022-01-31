/*
 *  This file is part of seq66.
 *
 *  seq24 is free software; you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option) any later
 *  version.
 *
 *  seq24 is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with seq24; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          midibus.cpp
 *
 *  This module declares/defines the base class for MIDI I/O under one of
 *  Windows' audio frameworks.
 *
 * \library       seq66 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2022-01-29
 * \license       GNU GPLv2 or above
 *
 *  This file provides a Windows-only implementation of the midibus class.
 *  It differs from the ALSA implementation in the following particulars:
 *
 *      -   No concept of a buss-name or a port-name, though it does have a
 *          client-name.  The ALSA version has an ID, a client address, a
 *          client port, and a user-configurable alias.
 *      -   It has a poll_for_midi() function.
 *      -   It does not have the following functions:
 *          -   init_out_sub()
 *          -   init_in_sub()
 *          -   deinit_out()
 *          -   deinit_in()
 */

#include "cfg/settings.hpp"             /* seq66::rc_settings               */
#include "midi/event.hpp"               /* seq66::event and macros          */
#include "os/timing.hpp"                /* seq66::microsleep()              */
#include "midibus_pm.hpp"               /* seq66::midibus for PortMIDI      */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

static std::string
s_portname (const std::string & name, int index)
{
    return name + " " + std::to_string(index);
}

/**
 *  Principal constructor.
 *
 *  There's a little confusion with the port ID parameter(s).  Also, the
 *  default values of queue, ppqn, bpm, and makevirtual are passed to the
 *  midibase constructor.  PortMidi does not support those constructs.
 *
 * Question:
 *
 *      Should we call s_portname with "clientname, index")?
 */

midibus::midibus
(
    int index, int bus_id, int port_id,
    const std::string & clientname
) :
    midibase
    (
//      EXPERIMENTAL as noted in the banner.
//      rc().application_name(), s_portname("PortMidi", index), clientname,
        rc().application_name(),            // appname
        s_portname(clientname, index),      // busname
        clientname,                         // portname
        index,
        bus_id,
        port_id,
        port_id,                            // queue number
        usr().use_default_ppqn(),           // PPQN flag value (-1)
        usr().bpm_default(),                // 120.0,
        midibase::io::output,               // false,
        midibase::port::normal              // false,
//      false
        /* PM uses 'queue' still */
    ),
    m_pms (nullptr)
{
    // Empty body
}

/**
 *  The destructor closes out the Windows MIDI infrastructure.
 */

midibus::~midibus ()
{
    if (not_nullptr(m_pms))
    {
        Pm_Close(m_pms);
        m_pms = nullptr;
    }
}

/**
 *  Polls for MIDI events.  This is the API implementation for PortMidi.
 *  It tests that the queue number (formerly m_pm) is valid first.  It does
 *  not assume that the PortMidiStream pointer m_pms is valid.
 *
 *  The original error-checking was too simplistic.  The PmError values of
 *  PmNoError, pmNoData, and pmGotData are actually "no error" codes, if you
 *  read /usr/include/portmidi.h, so we should not print a message if they
 *  occur.  FALSE and TRUE are just too limiting.  FALSE == pmNoError and
 *  pmNoData, and TRUE == any other value.
 *
 *  EXPERIMENTAL for Windows CPU usage. Sleep for one millisecond,
 *  not 10 microseconds.
 *
 * \return
 *      Returns 0 if the polling succeeded, and 1 if it failed.
 */

int
midibus::api_poll_for_midi ()
{
    int result = 0;
    if (not_nullptr(m_pms) && queue_number() >= 0)      /* buss number      */
    {
        PmError err = Pm_Poll(m_pms);
        if (err == pmGotData)
            result = 1;
    }
    if (result == 0)
         (void) microsleep(1000);                       /* std_sleep_us()   */

    return result;
}

/**
 *  Initializes the MIDI output port, for PortMidi.
 *
 *  If there is an error, we set the clocking to e_clock::disable to indicate
 *  we should not bother to use the port.
 *
 * \return
 *      Returns true if the output port was successfully opened.
 */

bool
midibus::api_init_out ()
{
    PmError err = Pm_OpenOutput
    (
        &m_pms, queue_number(), NULL, 100, NULL, NULL, 0
    );
    bool result = err == pmNoError;
    if (! result)
    {
        errprintf
        (
            "Pm_OpenOutput(): %s; MIDI clock disabled\n",
            Pm_GetErrorText(err)
        );
        (void) set_clock(e_clock::disabled);
    }
    return result;
}

/**
 *  Initializes the MIDI input port, for PortMidi.
 *
 * \return
 *      Returns true if the input port was successfully opened.
 */

bool
midibus::api_init_in ()
{
    PmError err = Pm_OpenInput(&m_pms, queue_number(), NULL, 100, NULL, NULL);
    bool result = err == pmNoError;
    if (! result)
    {
        errprintf("Pm_OpenInput(): %s\n", Pm_GetErrorText(err));
        set_input(false);
    }
    return result;
}

/**
 *  Takes a native event, and encodes to a Windows message, and writes it to
 *  the queue.  It fills a small byte buffer, sets the MIDI channel, makes a
 *  message of it, and writes the message.
 *
 * \question
 *      The subatomic glue (Windows/PortMidi) implementation of Seq24 uses a
 *      mutex to lock this function.  Do we need to do that? Done in the
 *      wrapper.
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
    midibyte buffer[4];                /* temp for midi data */
    buffer[0] = e24->get_status(channel);
    e24->get_data(buffer[1], buffer[2]);

    PmEvent event;
    event.timestamp = 0;
    event.message = Pm_Message(buffer[0], buffer[1], buffer[2]);
    /* PmError err = */ Pm_Write(m_pms, &event, 1);
}

/**
 *  Continue from the given tick.  This function implements only the
 *  PortMidi-specific code.
 *
 * \param tick
 *      The tick to continue from; unused in the PortMidi API implementation.
 *
 * \param beats
 *      The calculated beats.  This calculation is made in the
 *      midibase::continue_from() function.
 */

void
midibus::api_continue_from (midipulse /* tick */, midipulse beats)
{
    PmEvent event;
    event.timestamp = 0;
    event.message = Pm_Message(EVENT_MIDI_CONTINUE, 0, 0);
    Pm_Write(m_pms, &event, 1);
    event.message = Pm_Message
    (
        EVENT_MIDI_SONG_POS, (beats & 0x3F80 >> 7), (beats & 0x7F)
    );
    Pm_Write(m_pms, &event, 1);
}

/**
 *  Sets the MIDI clock a-runnin', if the clock type is not e_clock::off.
 *  This function is called by midibase::start().
 */

void
midibus::api_start ()
{
    if (not_nullptr(m_pms) && ! port_disabled())
    {
        PmEvent event;
        event.timestamp = 0;
        event.message = Pm_Message(EVENT_MIDI_START, 0, 0);
        Pm_Write(m_pms, &event, 1);
    }
}

/**
 *  Stops the MIDI clock, if the clock-type is not e_clock::off.
 *  This function is called by midibase::stop().
 */

void
midibus::api_stop ()
{
    if (not_nullptr(m_pms) && ! port_disabled())
    {
        PmEvent event;
        event.timestamp = 0;
        event.message = Pm_Message(EVENT_MIDI_STOP, 0, 0);
        Pm_Write(m_pms, &event, 1);
    }
}

/**
 *  Generates MIDI clock.  This function is called by midibase::clock().
 *
 * \question
 *      The subatomic glue (Windows/PortMidi) implementation of Seq24 uses a
 *      mutex to lock this function.  Do we need to do that?  We do that, in
 *      midibase::clock().
 *
 * \param tick
 *      The clock tick value, not used in the API implementation of this
 *      function for PortMidi.
 */

void
midibus::api_clock (midipulse /* tick */)
{
    if (not_nullptr(m_pms) && ! port_disabled())
    {
        PmEvent event;
        event.timestamp = 0;                /* WHY NOT use 'tick' here? */
        event.message = Pm_Message(EVENT_MIDI_CLOCK, 0, 0);
        Pm_Write(m_pms, &event, 1);
    }
}

}           // namespace seq66

/*
 * midibus.cpp for PortMIDI
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


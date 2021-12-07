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
 * \file          mastermidibus.cpp
 *
 *  This module declares/defines the base class for MIDI I/O under the
 *  refactored RtMidi framework.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2021-12-07
 * \license       GNU GPLv2 or above
 *
 *  This file provides a Windows-only implementation of the mastermidibus
 *  class.  There is a lot of common code between these two versions!
 */

#include "util/basic_macros.hpp"

#if defined SEQ66_HAVE_LIBASOUND
#include <sys/poll.h>
#endif

#include "cfg/settings.hpp"             /* seq66::rc()                      */
#include "midi/event.hpp"               /* seq66::event                     */
#include "mastermidibus_rm.hpp"         /* seq66::mastermidibus, RtMIDI     */
#include "midibus_rm.hpp"               /* seq66::midibus, RtMIDI           */

#define SEQ66_USE_JACK_POLLING_FLAG     /* until we reconcile ALSA/JACK     */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  The base-class constructor fills the array for our busses.
 *
 * \param ppqn
 *      Provides the PPQN value for this object.  However, in most cases, the
 *      default value, (-1) to use the default PPQN, should be specified.
 *
 * \param bpm
 *      Provides the beats per minute value, which defaults to
 *      c_beats_per_minute.
 */

mastermidibus::mastermidibus (int ppqn, midibpm bpm)
 :
    mastermidibase      (ppqn, bpm),
    m_midi_master
    (
        rc().with_jack_midi() ? RTMIDI_API_UNIX_JACK : RTMIDI_API_LINUX_ALSA,
        rc().app_client_name(), ppqn, bpm
    ),
    m_use_jack_polling  (rc().with_jack_midi())
{
    // Empty body
}

/**
 *  The destructor deletes all of the output busses, and terminates the
 *  Windows MIDI manager.
 */

mastermidibus::~mastermidibus ()
{
    // Empty body
}

/**
 *  Initializes the RtMidi implementation.  Two different styles are
 *  supported.  If the --manual-ports option is in force, then 16 virtual
 *  output ports and one virtual input port are created.  They are given names
 *  that make it clear which application (seq66) has set them up.  They are
 *  not connected to anything.  The user will have to use a connection GUI
 *  (such as qjackctl) or a session manager to make the connections.
 *
 *  Otherwise, the system MIDI input and output ports are scanned (via the
 *  rtmidi_info member) and passed to the midibus constructor calls.  For
 *  every MIDI input port found on the system, this function creates a
 *  corresponding output port, and connects to the system MIDI input.  For
 *  example, for an input port found called "qmidiarp:in 1", we want to create
 *  a "shadow" output port called "seq66:qmidiarp in 1".
 *
 *  Also, as a new feature for 0.98.0, we extract a port alias (JACK only) for
 *  system port names that do not contain the name of the ALSA USB device in
 *  them; the alias might contain that more human-readable name.
 *
 *  For every MIDI output found on the system this function creates a
 *  corresponding input port, and connects it to the system MIDI output.  For
 *  For example, for an output port found called "qmidiarp:out 1", we want to
 *  create a "shadow" input port called "seq66:qmidiarp out 1".
 *
 *  This code creates a midibus in the conventional manner.  Then the
 *  busarray::add() function makes a new businfo object with the desired
 *  "output" and "isvirtual" parameters; the businfo object then decides
 *  whether to call init_in(), init_out(), init_in_sub(), or init_out_sub().
 *
 *  Also, the midibus pointers created here are local, but the busarray::add()
 *  function manages them, using the std::unique_ptr<> template.  We could use
 *  std::unique_ptr<> here, and even std::make_unique() if we wanted to require
 *  C++14 at this time.
 *
 *  Are these good conventions, or potentially confusing to users?  They
 *  match what the legacy seq66 and seq66 do for ALSA.
 *
 * \param ppqn
 *      Provides the (possibly new) value of PPQN to set.  ALSA has a function
 *      that sets its idea of the PPQN.  JACK, as far as we know, does not.
 *
 * \param bpm
 *      Provides the (possibly new) value of BPM (beats per minute) to set.
 *      ALSA has a function that sets its idea of the BPM.  JACK, as far as we
 *      know, does not.
 */

void
mastermidibus::api_init (int ppqn, midibpm bpm)
{
    m_midi_master.api_set_ppqn(ppqn);
    m_midi_master.api_set_beats_per_minute(bpm);
    if (rc().manual_ports())                            /* virtual ports    */
    {
        m_midi_master.clear();

        int num_buses = rc().manual_port_count();
        for (int bus = 0; bus < num_buses; ++bus)       /* output busses    */
        {
            midibus * m = new (std::nothrow) midibus
            (
                m_midi_master, bus,
                midibase::c_virtual_port,
                midibase::c_output_port,
                bus /* bussoverride */                  /* breaks ALSA?     */
            );
            if (not_nullptr(m))
            {
                m_outbus_array.add(m, clock(bus));      /* must come 1st    */
                m_midi_master.add_output(m);            /* must come 2nd    */
            }
        }
        num_buses = rc().manual_in_port_count();
        for (int bus = 0; bus < num_buses; ++bus)       /* input busses     */
        {
            midibus * m = new (std::nothrow) midibus    /* input buss       */
            (
                m_midi_master, bus,
                midibase::c_virtual_port,
                midibase::c_input_port,
                bus
            );
            if (not_nullptr(m))
            {
                // m_inbus_array.add(m, input(0)); ???  /* must come 1st    */
                m_inbus_array.add(m, input(bus));       /* must come 1st    */
                m_midi_master.add_input(m);             /* must come 2nd    */
            }
        }
    }
    else
    {
        unsigned nports = m_midi_master.full_port_count();
        bool swap_io = rc().with_jack_midi();
        bool isinput = swap_io ?
            midibase::c_output_port : midibase::c_input_port;
        bool isoutput = swap_io ?
            midibase::c_input_port : midibase::c_output_port;

        if (nports > 0)
        {
            m_midi_master.midi_mode(midibase::c_input_port);     /* ugh! */
            unsigned inports = m_midi_master.get_port_count();
            for (unsigned bus = 0; bus < inports; ++bus)
            {
                bool isvirtual = m_midi_master.get_virtual(bus);
                bool issystem = m_midi_master.get_system(bus);
                midibus * m = new (std::nothrow) midibus
                (
                    m_midi_master, bus, isvirtual, isinput,
                    null_buss(), issystem
                );
                if (not_nullptr(m))
                {
                    if (swap_io)
                    {
                        set_midi_out_alias(bus, m->port_alias());
                        m_outbus_array.add(m, clock(bus));
                    }
                    else
                    {
                        set_midi_in_alias(bus, m->port_alias());
                        m_inbus_array.add(m, input(bus));
                    }
                    m_midi_master.add_bus(m);           /* must come 2nd    */
                }
            }
            m_midi_master.midi_mode(midibase::c_output_port);    /* ugh! */

            unsigned outports = m_midi_master.get_port_count();
            for (unsigned bus = 0; bus < outports; ++bus)
            {
                bool isvirtual = m_midi_master.get_virtual(bus);
                bool issystem = m_midi_master.get_system(bus);
                midibus * m = new (std::nothrow) midibus
                (
                    m_midi_master, bus, isvirtual, isoutput,
                    null_buss(), issystem
                );
                if (not_nullptr(m))
                {
                    if (swap_io)
                    {
                        set_midi_in_alias(bus, m->port_alias());
                        m_inbus_array.add(m, input(bus));
                    }
                    else
                    {
                        set_midi_out_alias(bus, m->port_alias());
                        m_outbus_array.add(m, clock(bus));
                    }
                    m_midi_master.add_bus(m);           /* must come 2nd    */
                }
            }
        }
    }
    set_beats_per_minute(bpm);
    set_ppqn(ppqn);
}

/**
 *  Activates the mastermidibase code and the rtmidi_info object via its
 *  api_connect() function.
 */

bool
mastermidibus::activate ()
{
    bool result = mastermidibase::activate();
    if (result)
        result = m_midi_master.api_connect();      /* activates, too    */

    return result;
}

/**
 *  Initiate a poll() on the existing poll descriptors.  This is a
 *  primitive poll, which exits when some data is obtained, or sleeps a
 *  millisecond in note data is obtained.
 *
 *  For JACK polling, call the base-class implementation:
 *
 *      -   mastermidibase::api_poll_for_midi()
 *      -   busarray::poll_for_midi()
 *      -   businfo::poll_for_midi()
 *      -   midibus::poll_for_midi() [midibase::poll_for_midi()]
 *      -   midibase::api_poll_for_midi(), a virtual function overridden
 *          for JACK (and ALSA).
 *
 *  Otherwise, the call sequence is:
 *
 *      -   rtmidi_info::api_poll_for_midi()
 *      -   rtmidi_info::get_api_info()->api_poll_for_midi()
 *      -   midi_alsa_info::api_poll_for_midi()
 *      -   poll() on the ALSA descriptors; a return > 0 means that number of
 *          events are ready
 *
 *  Because of some reasons long forgotten, the ALSA "rtmidi" framework here
 *  handles MIDI via the midi_alsa_info object.
 *
 * \return
 *      Returns the number of input MIDI events waiting.
 */

int
mastermidibus::api_poll_for_midi ()
{
#if defined SEQ66_USE_JACK_POLLING_FLAG
    if (m_use_jack_polling)                             /* run-time option  */
        return mastermidibase::api_poll_for_midi();     /* inbus-array poll */
    else
        return m_midi_master.api_poll_for_midi();       /* ALSA poll        */
#else
    return mastermidibase::api_poll_for_midi();         /* inbus-array poll */
#endif
}

/**
 *  Grab a MIDI event.  For the ALSA implementation, this call
 *
 * \threadsafe
 */

bool
mastermidibus::api_get_midi_event (event * inev)
{
#if defined SEQ66_USE_JACK_POLLING_FLAG
    if (m_use_jack_polling)
        return m_inbus_array.get_midi_event(inev);
    else
        return m_midi_master.api_get_midi_event(inev);
#else
    return m_inbus_array.get_midi_event(inev);
#endif
}

}           // namespace seq66

/*
 * mastermidibus.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


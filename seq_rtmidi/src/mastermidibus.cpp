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
 * \updates       2024-06-04
 * \license       GNU GPLv2 or above
 *
 *  This file provides a Windows-only implementation of the mastermidibus
 *  class.  There is a lot of common code between these two versions!
 */

#include "cfg/settings.hpp"             /* seq66::rc()                      */
#include "midi/event.hpp"               /* seq66::event                     */
#include "mastermidibus_rm.hpp"         /* seq66::mastermidibus, rtmidi     */
#include "midibus_rm.hpp"               /* seq66::midibus, rtmidi           */

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

mastermidibus::mastermidibus (int ppqn, midibpm bpm) :
    mastermidibase      (ppqn, bpm),
    m_midi_master                                           /* rtmidi_info  */
    (
        rc().with_jack_midi() ? rtmidi_api::jack : rtmidi_api::alsa,
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
 *  std::unique_ptr<> here, and even std::make_unique() if we wanted to
 *  require C++14 at this time.
 *
 *  Are these good conventions, or potentially confusing to users?  They match
 *  what the legacy seq66 and seq66 do for ALSA.
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
    midi_master().api_set_ppqn(ppqn);
    midi_master().api_set_beats_per_minute(bpm);
    if (rc().manual_ports())                            /* virtual ports    */
    {
        bool enable = rc().manual_auto_enable();
        int num_buses = rc().manual_port_count();       /* output count     */
        midi_master().clear();
        for (int bus = 0; bus < num_buses; ++bus)       /* output busses    */
        {
           midibus * m = make_virtual_bus(bus, midibase::io::output);
            if (not_nullptr(m))
            {
                if (rc().manual_auto_enable())
                    m->set_io_status(enable);

                midi_master().add_output(m);            /* must come 2nd    */
            }
        }
        num_buses = rc().manual_in_port_count();        /* input count      */
        for (int bus = 0; bus < num_buses; ++bus)       /* input busses     */
        {
            midibus * m = make_virtual_bus(bus, midibase::io::input);
            if (not_nullptr(m))
            {
                if (rc().manual_auto_enable())
                    m->set_io_status(enable);

                midi_master().add_input(m);             /* must come 2nd    */
            }
        }
    }
    else
    {
        bool swap_io = midi_master().selected_api() == rtmidi_api::jack;
        unsigned nports = midi_master().full_port_count();
        if (nports > 0)
        {
            midibase::io iodirection = swap_io ?
                midibase::io::output : midibase::io::input ;

            debug_message("Adding midibus port objects");
            midi_master().midi_mode(midibase::io::input);     /* ugh! mode! */

            unsigned inports = midi_master().get_port_count();
            for (unsigned bus = 0; bus < inports; ++bus)
            {
                midibus * m = make_normal_bus(bus, iodirection);
                if (not_nullptr(m))
                    midi_master().add_bus(m);               /* rtmidi_info  */
            }
            iodirection = swap_io ? midibase::io::input : midibase::io::output ;
            midi_master().midi_mode(midibase::io::output);    /* ugh! mode! */

            unsigned outports = midi_master().get_port_count();
            for (unsigned bus = 0; bus < outports; ++bus)
            {
                midibus * m = make_normal_bus(bus, iodirection);
                if (not_nullptr(m))
                    midi_master().add_bus(m);               /* rtmidi_info  */
            }
        }
    }
    set_beats_per_minute(bpm);
    set_ppqn(ppqn);
}

midibus *
mastermidibus::make_virtual_bus (int bus, midibase::io iotype)
{
    midibus * m = new (std::nothrow) midibus
    (
        midi_master(), bus, iotype, midibase::port::manual, bus
    );
    if (not_nullptr(m))
    {
        if (iotype == midibase::io::input)
            m_inbus_array.add(m, input(bus));
        else
            m_outbus_array.add(m, clock(bus));
    }
    return m;
}

midibus *
mastermidibus::make_normal_bus (int bus, midibase::io iotype)
{
    midibase::port porttype = midibase::port::normal;
    if (midi_master().get_virtual(bus))
        porttype = midibase::port::manual;
    else if (midi_master().get_system(bus))
        porttype = midibase::port::system;

    midibus * m = new (std::nothrow) midibus
    (
        midi_master(), bus, iotype, porttype, null_buss()
    );
    if (not_nullptr(m))
    {
#if defined SEQ66_SHOW_BUS_VALUES
        m->show_bus_values();
#endif
        set_midi_alias(bus, iotype, m->port_alias());
        if (iotype == midibase::io::input)
            m_inbus_array.add(m, input(bus));
        else
            m_outbus_array.add(m, clock(bus));
    }
    return m;
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
        result = midi_master().api_connect();      /* activates, too    */

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
    if (m_use_jack_polling)                             /* --jack-midi set  */
        return mastermidibase::api_poll_for_midi();     /* inbus-array poll */
    else
        return midi_master().api_poll_for_midi();       /* ALSA poll        */
#else
    return mastermidibase::api_poll_for_midi();         /* inbus-array poll */
#endif
}

/**
 *  Grab a MIDI event.  For the ALSA implementation, this call is ...???
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
        return midi_master().api_get_midi_event(inev);
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


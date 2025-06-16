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
 * \file          mastermidibase.cpp
 *
 *  This module declares/defines the base class for handling MIDI I/O via
 *  the ALSA system.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2016-11-23
 * \updates       2025-06-10
 * \license       GNU GPLv2 or above
 *
 *  This file provides a base-class implementation for various master MIDI
 *  buss support classes.  There is a lot of common code between these MIDI
 *  buss classes.
 */

#include "cfg/settings.hpp"             /* seq66::rc()                      */
#include "midi/event.hpp"               /* seq66::event                     */
#include "midi/mastermidibase.hpp"      /* seq66::mastermidibase            */
#include "play/sequence.hpp"            /* seq66::sequence                  */
#include "os/timing.hpp"                /* seq66::microsleep()              */

namespace seq66
{

/**
 *  The mastermidibase default constructor fills the array with our busses.
 *
 * \param ppqn
 *      Provides the PPQN value for this object.  However, in most cases, the
 *      default baseline PPQN should be specified.  Then the caller of this
 *      constructor should call mastermidibase::set_ppqn() to set up the
 *      proper PPQN value.
 *
 * \param bpm
 *      Provides the beats per minute value, which defaults to
 *      c_beats_per_minute.
 */

mastermidibase::mastermidibase
(
    int ppqn,
    midibpm bpm
) :
    m_client_id         (0),
    m_max_busses        (c_busscount_max),
    m_bus_announce      (nullptr),      /* used only for ALSA announce bus  */
    m_inbus_array       (),
    m_outbus_array      (),
    m_master_clocks     (),
    m_master_inputs     (),
    m_queue             (0),
    m_ppqn              (choose_ppqn(ppqn)),
    m_beats_per_minute  (bpm),          /* beats per minute                 */
    m_dumping_input     (false),
    m_vector_sequence   (),             /* stazed feature                   */
    m_record_by_buss    (false),        /* set based on configuration       */
    m_record_by_channel (false),        /* ditto, but mutually exclusive    */
    m_seq               (nullptr),
    m_mutex             ()
{
    // Empty body now
}

/**
 *  The virtual destructor deletes all of the output busses, clears out the
 *  ALSA events, stops and frees the queue, and closes ALSA for this
 *  application.
 *
 *  Valgrind indicates we might have issues caused by the following functions:
 *
 *      -   snd_config_hook_load()
 *      -   snd_config_update_r() via snd_seq_open()
 *      -   _dl_init() and other GNU function
 *      -   init_gtkmm_internals() [version 2.4]
 */

mastermidibase::~mastermidibase ()
{
    if (not_nullptr(m_bus_announce))
    {
        delete m_bus_announce;
        m_bus_announce = nullptr;
    }
}

/**
 *  Initializes and activates the busses, in a partly API-dependent manner.
 *  Currently re-implemented only in the rtmidi JACK API.
 */

bool
mastermidibase::activate ()
{
    bool result = m_inbus_array.initialize();
    if (result)
        result = m_outbus_array.initialize();

    if (result)
        set_client_id(m_outbus_array.client_id(0));

    return result;
}

/**
 *  Starts all of the configured output busses up to m_num_out_buses.  Calls
 *  the implementation-specific API function for starting.
 *
 * \threadsafe
 */

void
mastermidibase::start ()
{
    automutex locker(m_mutex);
    api_start();
    m_outbus_array.start();
}

/**
 *  Gets the MIDI output busses running again.  This function calls the
 *  implementation-specific API function, and then calls
 *  midibus::continue_from() for all of the MIDI output busses.
 *
 * \threadsafe
 *
 * \param tick
 *      Provides the tick value to continue from.
 */

void
mastermidibase::continue_from (midipulse tick)
{
    automutex locker(m_mutex);
    api_continue_from(tick);
    m_outbus_array.continue_from(tick);
}

/**
 *  Initializes the clock of each of the MIDI output busses.  Calls the
 *  implementation-specific API function, and then calls midibus::init_clock()
 *  for each of the MIDI output busses.
 *
 * \threadsafe
 *
 * \param tick
 *      Provides the tick value with which to initialize the buss clock.
 */

void
mastermidibase::init_clock (midipulse tick)
{
    automutex locker(m_mutex);
    api_init_clock(tick);
    m_outbus_array.init_clock(tick);
}

/**
 *  Stops each of the MIDI output busses.  Then calls the
 *  implementation-specific API function to finalize the stoppage. (See the
 *  ALSA implementation in the seq_alsamidi library, for example.  It is the
 *  original Seq66 implementation.)
 *
 * \threadsafe
 */

void
mastermidibase::stop ()
{
    automutex locker(m_mutex);
    m_outbus_array.stop();
    api_stop();
}

/**
 *  Generates the MIDI clock for each of the output busses.  Also calls the
 *  api_clock() function, which does nothing for the <i> original </i> ALSA
 *  implementation and the PortMidi implementation.
 *
 *  api_clock() doesn't do anything, so it is not called here. The clock()
 *  call here does flush as well.
 *
 * \threadsafe
 *
 * \param tick
 *      Provides the tick value with which to set the buss clock.
 */

void
mastermidibase::emit_clock (midipulse tick)
{
    automutex locker(m_mutex);
    m_outbus_array.clock(tick);
}

/**
 *  Set the PPQN value (parts per quarter note). Then call the
 *  implementation-specific API function to complete the PPQN setting.
 *
 * \threadsafe
 *
 * \param ppqn
 *      The PPQN value to be set.
 */

void
mastermidibase::set_ppqn (int ppqn)
{
    automutex locker(m_mutex);
    m_ppqn = choose_ppqn(ppqn);                     /* m_ppqn = ppqn        */
    api_set_ppqn(ppqn);
}

/**
 *  Set the BPM value (beats per minute).  Then call the
 *  implementation-specific API function to complete the BPM setting.
 *
 * \threadsafe
 *
 * \param bpm
 *      Provides the beats-per-minute value to set.
 */

void
mastermidibase::set_beats_per_minute (midibpm bpm)
{
    automutex locker(m_mutex);
    m_beats_per_minute = bpm;
    api_set_beats_per_minute(bpm);
}

/**
 *  Flushes our local queue events out  The implementation-specific API
 *  function is called.  For example, ALSA provides a function to "drain" the
 *  output.
 *
 * \threadsafe
 */

void
mastermidibase::flush ()
{
    automutex locker(m_mutex);
    api_flush();
}

/**
 *  Stops all notes on all channels on all busses.  Adapted from Oli Kester's
 *  Kepler34 project.  Whether the buss is active or not is ultimately checked
 *  in the busarray::play() function.  A bit wasteful, but do we really care?
 */

void
mastermidibase::panic (int displaybuss)
{
    automutex locker(m_mutex);
    for (int bus = 0; bus < c_busscount_max; ++bus)
    {
        if (bus == displaybuss)             /* do not clear the Launchpad   */
            continue;

        for (int channel = 0; channel < c_midichannel_max; ++channel)
        {
            for (int note = 0; note < c_midibyte_data_max; ++note)
            {
                event e(0, EVENT_NOTE_OFF, channel, note, 0);
                m_outbus_array.play(bus, &e, channel);
            }
        }
    }
    api_flush();
}

/**
 *  Handle the sending of SYSEX events.  There's currently no
 *  implementation-specific API function for this call.
 *
 * \threadsafe
 *
 * \param ev
 *      Provides the event pointer to be set.
 */

void
mastermidibase::sysex (bussbyte bus, const event * ev)
{
    automutex locker(m_mutex);
    m_outbus_array.sysex(bus, ev);
}

/**
 *  Handle the playing of MIDI events on the MIDI buss given by the parameter,
 *  as long as it is a legal buss number.  There's currently no
 *  implementation-specific API function here.
 *
 * \threadsafe
 *
 * \param bus
 *      The actual system buss to start play on.  The caller is expected to
 *      make sure this buss is the correct buss.
 *
 * \param e24
 *      The seq66 event to play on the buss.  For speed, we don't bother to
 *      check the pointer.
 *
 * \param channel
 *      The channel on which to play the event.
 */

void
mastermidibase::play (bussbyte bus, event * e24, midibyte channel)
{
    automutex locker(m_mutex);
    m_outbus_array.play(bus, e24, channel);
}

void
mastermidibase::play_and_flush (bussbyte bus, event * e24, midibyte channel)
{
    automutex locker(m_mutex);
    m_outbus_array.play(bus, e24, channel);
    api_flush();
}

/**
 *  Set the clock for the given (legal) buss number.  The legality checks
 *  are a little loose, however.
 *
 *  There's currently no implementation-specific API function here.
 *
 * \threadsafe
 *
 * \param bus
 *      The actual system buss to start play on.  Checked before usage.
 *
 * \param clocktype
 *      The type of clock to be set, either "off", "pos", or "mod", as noted
 *      in the midibus_common module.
 */

bool
mastermidibase::set_clock (bussbyte bus, e_clock clocktype)
{
    automutex locker(m_mutex);
    bool result = m_outbus_array.set_clock(bus, clocktype);
    if (result)
    {
        flush();
        result = save_clock(bus, clocktype);    /* save into the vector */
    }
    return result;
}

/**
 *  Saves the given clock value in m_master_clocks[bus].
 *
 * \param bus
 *      Provides the desired buss to be set.  This must be an actual system
 *      buss, not a buss number from the output-port-map.
 *
 * \param clock
 *      Provides the clocking value to set.
 *
 * \return
 *      Returns true if the buss value is valid.
 */

bool
mastermidibase::save_clock (bussbyte bus, e_clock clock)
{
    bool result = m_master_clocks.set(bus, clock);
    if (! result)
    {
        int currentcount = m_master_clocks.count();
        errprint("mmb::save_clock(): missing bus");
        for (int i = currentcount; i <= bus; ++i)
        {
            e_clock value = e_clock::disabled;
            if (i == int(bus))
            {
                value = clock;
                m_master_clocks.add(i, false, value, "Null clock");
            }
        }
    }
    return result;
}

/**
 *  Gets the clock setting for the given (legal) buss number.
 *
 *  There's currently no implementation-specific API function here.
 *
 * \param bus
 *      Provides an actual system buss number to read.  Checked before usage.
 *
 * \return
 *      If the buss number is legal, and the buss is active, then its clock
 *      setting is returned.  Otherwise, e_clock::disabled is returned.
 */

e_clock
mastermidibase::get_clock (bussbyte bus) const
{
    return m_outbus_array.get_clock(bus);
}

/**
 *  This function copies the buss values from the input and output busarrays
 *  (see the businfo module). They are then added to the masterbus's input and
 *  output containers.
 *
 *  Question: do we really want to add unavailable ports here?
 */

void
mastermidibase::copy_io_busses ()
{
    int buses = m_inbus_array.count();              /* get_num_in_buses()   */
    m_master_inputs.clear();                        /* inputslist container */
    for (int bus = 0; bus < buses; ++bus)
    {
        bool available = ! m_inbus_array.is_port_unavailable(bus);
        bool inputflag = m_inbus_array.get_input(bus);
        std::string name = m_inbus_array.get_midi_bus_name(bus);
        std::string alias = m_inbus_array.get_midi_alias(bus);
        m_master_inputs.add(bus, available, inputflag, name, "", alias);
    }
    buses = m_outbus_array.count();                 /* get_num_out_buses()  */
    m_master_clocks.clear();                        /* clockslist container */
    for (int bus = 0; bus < buses; ++bus)
    {
        bool available = ! m_outbus_array.is_port_unavailable(bus);
        e_clock clk = m_outbus_array.get_clock(bus);
        std::string name = m_outbus_array.get_midi_bus_name(bus);
        std::string alias = m_outbus_array.get_midi_alias(bus);
        m_master_clocks.add(bus, available, clk, name, "", alias);
    }
}

/**
 *  Used in the performer class to pass the settings read from the "rc" file
 *  to here.  There is an converse function defined above. This function gets
 *  the I/O port-map ports and copies them into the provided I/O lists.
 *
 *  How it works:  The portslist::match_system_to_map() function here takes the
 *  master I/O lists, which come from the I/O section in the 'rc' file, and
 *  sets the matching statuses of the I/O settings to the statuses of the
 *  matching internal I/O port-maps.
 */

void
mastermidibase::get_port_statuses (clockslist & outs, inputslist & ins)
{
    get_out_port_statuses(outs);
    get_in_port_statuses(ins);
}

/**
 *  Copies the port statuses between the master clockslist and the output
 *  port-map.
 *
 *  The direction is dependent on whether the port-map is active or not.
 *  If active, then the port-map is copied to the master clockslist.
 *  If inactive, then the master clockslist is copied to the port-map.
 *  Finally, the (possibly update) master clockslist is copied to the \a
 *  outs parameter.
 *
 * \param outs
 *      The destination for the current master clockslist port statuses.
 */

void
mastermidibase::get_out_port_statuses (clockslist & outs)
{
    clockslist & opm = output_port_map();
    if (opm.active())
    {
        if (! opm.empty())                          /* master clocks = opm  */
            opm.match_system_to_map(m_master_clocks);
    }
    else
        opm.match_map_to_system(m_master_clocks);   /* opm = master clocks  */

    outs = m_master_clocks;         /* system clocks information to store   */
}

void
mastermidibase::get_in_port_statuses (inputslist & ins)
{
    inputslist & ipm = input_port_map();
    if (ipm.active())
    {
        if (! ipm.empty())                          /* master inputs = ipm  */
            ipm.match_system_to_map(m_master_inputs);
    }
    else
        ipm.match_map_to_system(m_master_inputs);   /* ipm = master inputs  */

    ins = m_master_inputs;          /* system inputs information to store   */
}

/**
 *  Set the status of the given input buss, if a legal buss number.
 *  Why is another buss-count constant, and a global one at that, being
 *  used?  And I thought there was only one input buss anyway!  Well,
 *  there is only one ALSA input buss, but more can be used with JACK,
 *  apparently.
 *
 *  There's currently no implementation-specific API function here.
 *
 * \threadsafe
 *
 * \param bus
 *      Provides the actual system buss number.
 *
 * \param inputing
 *      True if the input bus will be inputting MIDI data.
 *
 * \return
 *      Returns true if the input buss array item could be set and then saved
 *      into the status container.
 */

bool
mastermidibase::set_input (bussbyte bus, bool inputing)
{
    automutex locker(m_mutex);
    bool result = m_inbus_array.set_input(bus, inputing);
    if (result)
    {
        flush();
        result = save_input(bus, inputing);     /* save into the vector */
    }
    return result;
}

/**
 *  Saves the input status (as selected in the MIDI Input tab).  Now, we were
 *  checking this bus number against the size of the vector as gotten from the
 *  performer object, which it got the from the "rc" file's [midi-input]
 *  section.  However, the "rc" file won't necessarily match what is on the
 *  system now.  So we might have to adjust.
 *
 *  Do we also have to adjust the performer's vector?  What about the name of
 *  the buss?
 *
 * \param bus
 *      Provides the actual system buss number.
 *
 * \param inputing
 *      True if the input bus will be inputting MIDI data.
 *
 * \return
 *      Returns true, always.
 */

bool
mastermidibase::save_input (bussbyte bus, bool inputing)
{
    bool result = m_master_inputs.set(bus, inputing);
    if (! result)
    {
        int currentcount = m_master_inputs.count();
        errprint("mmb::save_input(): missing bus");
        for (int i = currentcount; i <= bus; ++i)
        {
            bool value = false;
            if (i == int(bus))
            {
                value = inputing;
                m_master_inputs.add(i, false, value, "Null input");
            }
        }
    }
    return result;
}

/**
 *  Get the input for the given (legal) buss number.
 *
 *  There's currently no implementation-specific API function here.
 *
 * \param bus
 *      Provides the actual system buss number.
 *
 * \return
 *      Returns the value of the busarray::get_input(bus) call.
 */

bool
mastermidibase::get_input (bussbyte bus) const
{
    return m_inbus_array.get_input(bus);
}

/**
 *  Get the system-buss status for the given (legal) buss number.
 *
 * \param bus
 *      Provides the actual system buss number.
 *
 * \return
 *      Returns the value of the busarray::get_input(bus) call.
 */

bool
mastermidibase::is_input_system_port (bussbyte bus) const
{
    return m_inbus_array.is_system_port(bus);
}

/**
 *  There is an issue where this function is indirectly used in creating
 *  the lists of I/O ports in the user-interface. There will, in general,
 *  be more ports shown in the map than actually exist on the system.
 *  For that purpose, we need to use the port maps, not the real port lists.
 */

bool
mastermidibase::is_port_unavailable (bussbyte bus, midibase::io iotype) const
{
    if (iotype == midibase::io::input)
        return m_inbus_array.is_port_unavailable(bus);
    else
        return m_outbus_array.is_port_unavailable(bus);
}

bool
mastermidibase::is_port_locked (bussbyte bus, midibase::io iotype) const
{
    if (iotype == midibase::io::input)
        return m_inbus_array.is_port_locked(bus);
    else
        return m_outbus_array.is_port_locked(bus);
}

/**
 *
 *  This function adds the retrieval of client and port numbers that are not
 *  needed in the portmidi implementation, but seem generally useful to
 *  support in all implementations.
 *
 *  We first try a port-mapper lookup, to get the short-name for the port. If
 *  we can't get one, then we return the value obtained from the output
 *  busarray, which has extra information over and above.....
 *
 * \param bus
 *      Provides the actual system output buss number.  Checked before usage.
 *
 * \return
 *      Returns the buss name as a standard C++ string.
 */

/**
 *  Get the MIDI input/output buss name for the given (legal) buss number.
 *  This function is used for display purposes, and is also written to the
 *  options ("rc") file.
 *
 *  This function adds the retrieval of client and port numbers that are not
 *  needed in the portmidi implementation, but seem generally useful to
 *  support in all implementations.
 *
 * \param bus
 *      Provides the I/O buss number.
 *
 * \param iotype
 *      Indicates which I/O list is used for the lookup.
 *
 * \return
 *      Returns the buss name as a standard C++ string.  Also contains an
 *      indication that the buss is disconnected or unconnected.  If the buss
 *      number is illegal, this string is empty.
 */

std::string
mastermidibase::get_midi_bus_name (bussbyte bus, midibase::io iotype) const
{
    portname p = rc().port_naming();
    if (iotype == midibase::io::input)
        return m_master_inputs.get_display_name(bus, p);
    else
        return m_master_clocks.get_display_name(bus, p);
}

/**
 *  Print some information about the available MIDI input and output busses.
 */

void
mastermidibase::print () const
{
    m_inbus_array.print();
    m_outbus_array.print();
}

/**
 *  Initiate a poll() on the existing poll descriptors.  This base-class
 *  implementation could be made identical to portmidi's poll_for_midi()
 *  function, maybe.  But currently it is better just call the
 *  implementation-specific API function.
 *
 * \warning Do we need to use a mutex lock? No! It causes a deadlock!!!
 *
 * \return Returns the result of the poll, or 0 if the API is not supported.
 */

int
mastermidibase::poll_for_midi ()
{
    return api_poll_for_midi();
}

/**
 *  Provides a default implementation of api_poll_for_midi().  This
 *  implementation adds a millisecond of sleep time unless more than two
 *  events are pending.  Some input devices may need to override this
 *  function.
 *
 *  For a quick threadsafe check, call is_more_input() instead.  But see the
 *  warning in the non-API poll_for_midi() function.
 *
 * \return
 *      Returns the number of events found by the first successful poll of the
 *      array of input busses (m_inbus_array).
 */

int
mastermidibase::api_poll_for_midi ()
{
    int result = m_inbus_array.poll_for_midi();
    if (result > 0)
    {
        if (result <= 2)
            (void) microsleep(std_sleep_us());      /* is this sensible?    */
    }
    else
    {
        (void) microsleep(std_sleep_us());
    }
    return result;
}

/**
 *  Test the sequencer to see if any more input is pending.  Calls the
 *  implementation-specific API function.
 *
 *  Note that the ALSA implementation calls a single "input-pending" function,
 *  while the PortMidi implementation loops through all of the input midibus
 *  objects, calling the poll_for_midi() function of each.
 *
 * \threadsafe
 *
 * \return
 *      Returns true if ALSA is supported, and the returned size is greater
 *      than 0, or false otherwise.
 */

bool
mastermidibase::is_more_input ()
{
    automutex locker(m_mutex);
    return m_inbus_array.poll_for_midi() > 0;
}

/**
 *  Start the given MIDI port.  This function is called by
 *  api_get_midi_event() when the ALSA event SND_SEQ_EVENT_PORT_START is
 *  received.  Unlike port_exit(), the port_start() function does rely on
 *  API-specific code, so we do need to create a virtual api_port_start()
 *  function to implement the port-start event.
 *
 *  \threadsafe
 *      Quite a lot is done during the lock for the ALSA implimentation.
 *
 * \param client
 *      Provides the client number, which is actually an ALSA concept.
 *
 * \param port
 *      Provides the client port, which is actually an ALSA concept.
 */

void
mastermidibase::port_start (int client, int port)
{
    automutex locker(m_mutex);
    api_client_port_start(client, port);
}

/**
 *  Turn off the given port for the given client.  Both the input and output
 *  busses for the given client are stopped: that is, set to inactive.
 *
 *  This function is called by api_get_midi_event() when the ALSA event
 *  SND_SEQ_EVENT_PORT_EXIT is received.  Since port_exit() has no direct
 *  API-specific code in it, we do not need to create a virtual
 *  api_port_exit() function to implement the port-exit event.
 *
 * \threadsafe
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
mastermidibase::port_exit (int client, int port)
{
    automutex locker(m_mutex);
    m_outbus_array.port_exit(client, port);
    m_inbus_array.port_exit(client, port);
}

/**
 *  Set the input sequence object, and set the m_dumping_input value to
 *  the given state.
 *
 *  The portmidi version only sets m_seq and m_dumping_input, but it seems
 *  like all the code below would apply to any mastermidibus.
 *
 *  What if m_seq is already set? Do we want to undo that one and set the
 *  new one (which requires being able to remove the red recording circle),
 *  or just ignore the new click? The latter is easier.
 *
 * Usages:
 *
 *  -   portmidi mastermidibus::api_init()
 *  -   qseqeditframe64::toggle_midi_rec() and _thru()
 *  -   sequence::set_input_recording() and _thru()
 *  -   performer::set_recording() and _thru()
 *
 * \threadsafe
 *
 * \param state
 *      Provides the dumping-input (recording) state to be set.  This value,
 *      as used in seqedit, can represent the state of the thru button or the
 *      record button.
 *
 * \param seqp
 *      Provides the sequence pointer to be logged as the mastermidibase's
 *      current sequence.  Can also be used to set a null pointer, to disable
 *      the sequence setting. For issue #129, if we're asking again to make
 *      a sequence the inputing sequence (i.e. to add quantization recording),
 *      we accept the seqp if it matches m_seq.
 *
 * \return
 *      Returns true if the sequence pointer is not null.
 */

bool
mastermidibase::set_sequence_input (bool state, sequence * seqp)
{
    automutex locker(m_mutex);
    bool result = not_nullptr(seqp);
    if (m_record_by_buss)
    {
        m_dumping_input = state;
    }
    else if (m_record_by_channel)
    {
        if (result)
        {
            if (state)                  /* add sequence if not already in   */
            {
                bool have_seq_already = false;
                for (size_t i = 0; i < m_vector_sequence.size(); ++i)
                {
                    if (m_vector_sequence[i] == seqp)
                    {
                        have_seq_already = true;
                        break;
                    }
                }
                if (! have_seq_already)
                    m_vector_sequence.push_back(seqp);
            }
            else                        /* remove sequence if already in    */
            {
                for (size_t i = 0; i < m_vector_sequence.size(); ++i)
                {
                    if (m_vector_sequence[i] == seqp)
                    {
                        m_vector_sequence.erase(m_vector_sequence.begin() + i);
                        break;
                    }
                }
            }
            if (m_vector_sequence.size() != 0)
                m_dumping_input = true;
        }
        else if (! state)
            m_vector_sequence.clear();  /* don't record, and clear vector   */
    }
    else
    {
        if (state)
        {
            if (not_nullptr(m_seq))
            {
                if (seqp != m_seq)
                    result = false;     /* We already got one! Is very nice */
            }
            else
            {
                m_dumping_input = state;
                m_seq = seqp;
            }
        }
        else
        {
            m_dumping_input = false;
            m_seq = nullptr;
        }
    }
    return result;
}

/**
 *  This function augments the recording functionality by looking for a
 *  sequence that has a matching channel number, logging the event to that
 *  sequence, and then immediately exiting.  It should be called only if
 *  m_record_by_channel is set.
 *
 *  If we have more than one sequence recording, and the channel-match feature
 *  [the sequence::channels_match() function] is disabled, then only the first
 *  sequence will get the events.  So now we add an additional call to the new
 *  sequence::channel_match() function.
 *
 * \param ev
 *      The event that was recorded, passed as a copy.  (Do we really need a
 *      copy?)
 */

bool
mastermidibase::dump_midi_input (event ev)
{
    bool result = false;
    size_t sz = m_vector_sequence.size();
    for (size_t i = 0; i < sz; ++i)
    {
        if (is_nullptr(m_vector_sequence[i]))          // error check
        {
            errprint("dump_midi_input(): bad sequence");
            continue;
        }
        else if (m_vector_sequence[i]->stream_event(ev))
        {
            /*
             * Did we find a match to the sequence channel?  Then don't
             * bother with the remaining sequences.  Otherwise, pass the
             * event to any other recording sequences.
             */

            if (m_vector_sequence[i]->channel_match())
            {
                result = true;
                break;
            }
        }
    }
    return result;
}

}           // namespace seq66

/*
 * mastermidibase.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


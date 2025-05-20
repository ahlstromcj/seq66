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
 * \file          midi_alsa.cpp
 *
 *  This module declares/defines the base class for handling MIDI I/O via
 *  the ALSA system.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2016-12-18
 * \updates       2025-05-20
 * \license       GNU GPLv2 or above
 *
 *  This file provides a Linux-only implementation of ALSA MIDI support.
 *  It is derived from the seq_alsamidi implementation in that midibus module.
 *  Note that we are changing the MIDI API somewhat from the original RtMidi
 *  midi_api class; that interface didn't really fit the seq66 model,
 *  and it was getting very painful to warp RtMidi to fit.
 *
 * Examples of subscription:
 *
 *  In ALSA library, subscription is done via snd_seq_subscribe_port()
 *  function. It takes the argument of snd_seq_port_subscribe_t record
 *  pointer. Suppose that you have a client which will receive data from a
 *  MIDI input device. The source and destination addresses are like the
 *  below:
 *
 *	Capture from keyboard:
 *
 *		Assume MIDI input port = 64:0, application port = 128:0, and queue for
 *		timestamp = 1 with real-time stamp. The application port must have
 *		capability SND_SEQ_PORT_CAP_WRITE.
 *
\verbatim
    void capture_keyboard (snd_seq_t * seq)
    {
        snd_seq_addr_t sender, dest;
        snd_seq_port_subscribe_t *subs;
        sender.client = 64;
        sender.port = 0;
        dest.client = 128;
        dest.port = 0;
        snd_seq_port_subscribe_alloca(&subs);
        snd_seq_port_subscribe_set_sender(subs, &sender);
        snd_seq_port_subscribe_set_dest(subs, &dest);
        snd_seq_port_subscribe_set_queue(subs, 1);
        snd_seq_port_subscribe_set_time_update(subs, 1);
        snd_seq_port_subscribe_set_time_real(subs, 1);
        snd_seq_subscribe_port(seq, subs);
    }
\endverbatim
 *
 *      Question: can we simply replace the above with the following code, which
 *      seems to work.
 *
\verbatim
 *      snd_seq_connect_from(seq, 0, 65, 1);    // myport, srcclient, srcport
\endverbatim
 *
 *  Output to MIDI device:
 *
 *      Assume MIDI output port = 65:1 and application port = 128:0. The
 *      application port must have capability SND_SEQ_PORT_CAP_READ.
 *
\verbatim
    void subscribe_output(snd_seq_t *seq)
    {
        snd_seq_addr_t sender, dest;
        snd_seq_port_subscribe_t *subs;
        sender.client = 128;
        sender.port = 0;
        dest.client = 65;
        dest.port = 1;
        snd_seq_port_subscribe_alloca(&subs);
        snd_seq_port_subscribe_set_sender(subs, &sender);
        snd_seq_port_subscribe_set_dest(subs, &dest);
        snd_seq_subscribe_port(seq, subs);
    }
\endverbatim
 *
 *      This example can be simplified by using the snd_seq_connect_to()
 *      function (as done in the RtMidi library).
 *
\verbatim
    void subscribe_output(snd_seq_t *seq)
    {
        snd_seq_connect_to(seq, 0, 65, 1);  // myport, destclient, destport
    }
\endverbatim
 *
 *  See http://www.alsa-project.org/alsa-doc/alsa-lib/seq.html for a wealth of
 *  information on ALSA sequencing.
 */

#include "cfg/settings.hpp"             /* seq66::rc()                      */
#include "midi/event.hpp"               /* seq66::event (MIDI event)        */
#include "midibus_rm.hpp"               /* seq66::midibus for rtmidi        */
#include "midi_alsa.hpp"                /* seq66::midi_alsa for ALSA        */
#include "midi_info.hpp"                /* seq66::midi_info                 */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 * --------------------------------------------------------------------------
 *  midi_alsa
 * --------------------------------------------------------------------------
 */

/**
 *  Provides a constructor with client number, port number, ALSA sequencer
 *  support, name of client, name of port, etc., mostly contained within an
 *  already-initialized midi_info object.
 *
 *  This constructor is the only one that is used for the MIDI input and
 *  output busses, whether the [manual-ports] option is in force or not.
 *  The actual setup of a normal or virtual port is done in the api_*_init_*()
 *  routines.
 *
 *  Also used for the announce buss, and in the mastermidi_alsa::port_start()
 *  function.  There's currently some overlap between local/dest client and
 *  port numbers and the buss and port numbers of the midibase interface.
 *  Also, note that the rcfile module uses the master buss to get the
 *  buss names when it writes the file.
 *
 *  We get the actual user-client ID from ALSA, then rebuild the descriptive
 *  name for this port. Also have to do it for the parent midibus.  We'd like
 *  to use seq_client_name(), but it comes up unresolved by the damned GNU
 *  linker!  The obvious fixes don't work!
 *
 *  ALSA returns "130" as the client ID.  That is our ALSA ID, not the ID of
 *  the client we are representing.  Thus, we should not set the buss ID and
 *  name of the parent-bus; these have already been determined.  ALSA assigns
 *  client IDs from the sequencer handle, and they range from 128 to 191.
 *
 * \param parentbus
 *      Provides much of the infor about this ALSA buss.
 *
 * \param masterinfo
 *      Provides the information about the desired port, and more.
 */

midi_alsa::midi_alsa (midibus & parentbus, midi_info & masterinfo) :
    midi_api            (parentbus, masterinfo),
    m_seq
    (
        reinterpret_cast<snd_seq_t *>(masterinfo.midi_handle())
    ),
    m_dest_addr_client  (parentbus.bus_id()),
    m_dest_addr_port    (parentbus.port_id()),
    m_local_addr_client (snd_seq_client_id(m_seq)),     /* our client ID    */
    m_local_addr_port   (-1)
{
    set_client_id(m_local_addr_client);
    set_name(SEQ66_CLIENT_NAME, bus_name(), port_name());
#if defined SEQ66_SHOW_BUS_VALUES
    parentbus.show_bus_values();
#endif
}

/**
 *  A rote empty virtual destructor.
 */

midi_alsa::~midi_alsa ()
{
    // empty body
}

/**
 *  Initialize the MIDI output port.  This initialization is done when the
 *  "manual-ports" option is not in force.
 *
 *  This initialization is like the "open_port()" function of the RtMidi
 *  library, with the addition of the snd_seq_connect_to() call involving
 *  the local and destination ports.
 *
 * \tricky
 *      One important thing to note is that this output port is initialized
 *      with the SND_SEQ_PORT_CAP_READ flag, which means this is really an
 *      input port.  We connect this input port with a system output port that
 *      was discovered.  This is backwards of the way RtMidi does it.
 *
 * \return
 *      Returns true unless setting up ALSA MIDI failed in some way.
 */

bool
midi_alsa::api_init_out ()
{
    static unsigned s_port_caps =
        SND_SEQ_PORT_CAP_NO_EXPORT | SND_SEQ_PORT_CAP_READ ;

    static unsigned s_port_types =
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION ;

    std::string busname = parent_bus().bus_name();
    int rcode = snd_seq_create_simple_port                 /* create ports     */
    (
        m_seq, busname.c_str(), s_port_caps, s_port_types
    );
    if (rcode < 0)
    {
        error_message("ALSA create output port failed");
        return false;
    }
    else
        m_local_addr_port = rcode;

    rcode = snd_seq_connect_to
    (
        m_seq, m_local_addr_port, m_dest_addr_client, m_dest_addr_port
    );
    if (rcode < 0)
    {
        msgprintf
        (
            msglevel::error, "ALSA connect to %d:%d error",
            m_dest_addr_client, m_dest_addr_port
        );
        return false;
    }
    else
        set_port_open();

    return true;
}

/**
 *  Initialize the MIDI input port.
 *
 *      SND_SEQ_PORT_CAP_SUBS_WRITE:    Allow write subscription.
 *      SND_SEQ_PORT_CAP_NO_EXPORT:     Routing not allowed.
 *
 * \tricky
 *      One important thing to note is that this input port is initialized
 *      with the SND_SEQ_PORT_CAP_WRITE flag, which means this is really an
 *      output port.  We connect this output port with a system input port
 *      that was discovered.  This is backwards of the way RtMidi does it.
 *
 * \return
 *      Returns true unless setting up ALSA MIDI failed in some way.
 */

bool
midi_alsa::api_init_in ()
{
    static unsigned s_port_caps =
        SND_SEQ_PORT_CAP_NO_EXPORT | SND_SEQ_PORT_CAP_WRITE ;

    static unsigned s_port_types =
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION ;

    std::string portname = parent_bus().port_name();
    int rcode = snd_seq_create_simple_port
    (
        m_seq, portname.c_str(), s_port_caps, s_port_types
    );
    if (rcode < 0)
    {
        error_message("ALSA create input port failed");
        return false;
    }
    else
    {
        m_local_addr_port = rcode;
    }
    rcode = snd_seq_connect_from
    (
        m_seq, m_local_addr_port, m_dest_addr_client, m_dest_addr_port
    );
    if (rcode < 0)
    {
        msgprintf
        (
            msglevel::error, "ALSA connect from %d:%d error",
            m_dest_addr_client, m_dest_addr_port
        );
        return false;
    }
    else
        set_port_open();

    return true;
}

/**
 *  Gets information directly from ALSA.  The problem this function solves is
 *  that the midibus constructor for a virtual ALSA port doesn't not have all
 *  of the information it needs at that point.  Here, we can get this
 *  information and get the actual data we need to rename the port to
 *  something accurate.
 *
 * \return
 *      Returns true if all of the information could be obtained.  If false is
 *      returned, then the caller should not use the side-effects.
 *
 * \sideeffect
 *      Passes back the values found.
 */

bool
midi_alsa::set_virtual_name (int portid, const std::string & portname)
{
    bool result = not_nullptr(m_seq);
    if (result)
    {
        snd_seq_client_info_t * cinfo;                  /* client info      */
        snd_seq_client_info_alloca(&cinfo);             /* will fill cinfo  */
        snd_seq_get_client_info(m_seq, cinfo);          /* filled!          */

        int cid = snd_seq_client_info_get_client(cinfo);
        const char * cname = snd_seq_client_info_get_name(cinfo);
        result = not_nullptr(cname);
        if (result)
        {
            std::string clientname = cname;
            set_port_id(portid);
            port_name(portname);
            set_bus_id(cid);
            set_name(rc().app_client_name(), clientname, portname);
            parent_bus().set_name(rc().app_client_name(), clientname, portname);
        }
    }
    return result;
}

/**
 *  Initialize the output in a different way.  This version of initialization
 *  is used by mastermidi_alsa in the "manual-ports" clause.  This code
 *  is also very similar to the same function in the
 *  midibus::api_init_out_sub() function of midibus::api_init_out_sub().
 *
 * \return
 *      Returns true unless setting up the ALSA port failed in some way.
 */

bool
midi_alsa::api_init_out_sub ()
{
    static unsigned s_port_caps =
        SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ ;

    static unsigned s_port_types =
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION ;

    std::string portname = port_name();
    if (portname.empty())
        portname = rc().app_client_name() + " out";

    int result = snd_seq_create_simple_port             /* create ports */
    (
        m_seq, portname.c_str(), s_port_caps, s_port_types
    );
    m_local_addr_port = result;
    if (result < 0)
    {
        error_message("ALSA create output virtual port failed");
        return false;
    }
    else
    {
        set_virtual_name(result, portname);
        set_port_open();
    }
    return true;
}

/**
 *  Initialize the output in a different way. This creates a "virtual"
 *  (i.e. "manual") input port. The meanings of the CAP flags are:
 *
 *      SND_SEQ_PORT_CAP_WRITE
 *
 *          The (input) port can be written to by other clientsh.
 *
 *      SND_SEQ_PORT_CAP_SUBS_WRITE
 *
 *          Subscription to the input port is allowed.
 *
 * \return
 *      Returns true unless setting up the ALSA port failed in some way.
 */

bool
midi_alsa::api_init_in_sub ()
{
    static unsigned s_port_caps =
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE ;

    static unsigned s_port_types =
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION ;

    std::string portname = port_name();
    if (portname.empty())
        portname = rc().app_client_name() + " midi in";

    int result = snd_seq_create_simple_port             /* create ports */
    (
        m_seq, portname.c_str(), s_port_caps, s_port_types
    );
    m_local_addr_port = result;
    if (result < 0)
    {
        error_message("ALSA create input virtual port failed");
        return false;
    }
    else
    {
        set_virtual_name(result, portname);
        set_port_open();
    }
    return true;
}

/**
 *  Not used.
 */

bool
midi_alsa::api_deinit_out ()
{
    return true;
}

/**
 *  Deinitialize the MIDI input.  Set the input and the output ports.
 *  The destination port is actually our local port.
 *
 * \return
 *      Returns true, unless an error occurs.
 */

bool
midi_alsa::api_deinit_in ()
{
    snd_seq_port_subscribe_t * subs;
    snd_seq_port_subscribe_alloca(&subs);

    snd_seq_addr_t sender;                                  /* output       */
    sender.client = m_dest_addr_client;
    sender.port = m_dest_addr_port;
    snd_seq_port_subscribe_set_sender(subs, &sender);

    snd_seq_addr_t dest;                                    /* input        */
    dest.client = m_local_addr_client;
    dest.port = m_local_addr_port;
    snd_seq_port_subscribe_set_dest(subs, &dest);

    /*
     * This would seem to unsubscribe all ports.  True?  Danger?
     */

    int queue = parent_bus().queue_number();
    snd_seq_port_subscribe_set_queue(subs, queue);
    snd_seq_port_subscribe_set_time_update(subs, queue);    /* get ticks    */

    int rc = snd_seq_unsubscribe_port(m_seq, subs);         /* unsubscribe  */
    if (rc < 0)
    {
        msgprintf
        (
            msglevel::error, "ALSA unsubscribe port %d:%d error",
            m_dest_addr_client, m_dest_addr_port
        );
        return false;
    }
    return true;
}

/**
 *  Trying to rearrange the ALSA code to be more like the JACK code.
 *  However, currently midi_alsa_info::api_connect() is what is called, and
 *  it currently doesn't do anything.
 */

bool
midi_alsa::api_connect ()
{
    return true;
}

/**
 *  Defines the size of the MIDI event buffer, which should be large enough to
 *  accomodate the largest MIDI message to be encoded.  A local define for
 *  visibility.  Also provided, but not yet used, is a size for SysEx events,
 *  which we don't handle, but want to note here.  Inspired by Qtractor code.
 *  Also in Qtractor, the same snd_midi_event_t object is used over and over,
 *  rather than being recreated/destroyed for every event-play by
 *  snd_midi_event_new() and snd_midi_event_free().
 *
 * Unused:
 *
 *      static const size_t s_sysex_size_max = 512; // Hydrogen uses 32
 *      w/input
 */

static const size_t s_event_size_max =  12;

/**
 *  This play() function takes a native event, encodes it to an ALSA MIDI
 *  sequencer event, sets the broadcasting to the subscribers, sets the
 *  direct-passing mode to send the event without queueing, and puts it in the
 *  queue.
 *
 * \threadsafe
 *
 * \param e24
 *      The event to be played on this bus.  For speed, we don't bother to
 *      check the pointer.
 *
 * \param channel
 *      The channel of the playback.  This channel is either the global MIDI
 *      channel of the sequence, or the channel of the event.  Either way, we
 *      mask it into the event status.
 */

void
midi_alsa::api_play (const event * e24, midibyte channel)
{
    if (parent_bus().port_enabled())
    {
        snd_midi_event_t * midi_ev;                         /* MIDI parser  */
        int rc = snd_midi_event_new(s_event_size_max, &midi_ev);
        if (rc == 0)
        {
            snd_seq_event_t ev;                             /* event memory */
            midibyte buffer[4];                             /* temp data    */
            buffer[0] = e24->get_status(channel);           /* status+chan  */
            e24->get_data(buffer[1], buffer[2]);            /* set the data */
            snd_seq_ev_clear(&ev);                          /* clear event  */
            snd_midi_event_encode(midi_ev, buffer, 3, &ev); /* 3 raw bytes  */
            snd_midi_event_free(midi_ev);                   /* free parser  */
            snd_seq_ev_set_source(&ev, m_local_addr_port);  /* set source   */
            snd_seq_ev_set_subs(&ev);                       /* subscriber   */
            snd_seq_ev_set_direct(&ev);                     /* immediate    */
            snd_seq_event_output(m_seq, &ev);               /* pump to que  */
        }
        else
        {
            errprint("ALSA out-of-memory");
        }
    }
}

/**
 *  min() for long values.
 *
 * \param a
 *      First operand.
 *
 * \param b
 *      Second operand.
 *
 * \return
 *      Returns the minimum value of a and b.
 */

inline long
min (long a, long b)
{
    return (a < b) ? a : b ;
}

/**
 *  Defines the value used for sleeping, in microseconds.  Defined locally
 *  simply for visibility.  Why 80000?
 */

static const int c_sysex_sleep_us   = 80000;
static const int c_sysex_chunk      =   256;

/**
 *  Takes a native SYSEX event, encodes it to an ALSA event, and then
 *  puts it in the queue.
 *
 * \param e24
 *      The event to be handled.
 */

void
midi_alsa::api_sysex (const event * e24)
{
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);                              /* clear event      */
    snd_seq_ev_set_priority(&ev, 1);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);                         /* it's immediate   */
    snd_seq_ev_set_source(&ev, m_local_addr_port);      /* set source       */

    /*
     *  Replaced by a vector of midibytes:
     *
     *      midibyte * data = e24->get_sysex();
     *
     *  This tack relies on the standard property of std::vector, where all n
     *  elements of the vector are guaranteed to be stored contiguously (in
     *  order to be accessible via random-access iterators).
     *
     *  We send the sysex data in chunks, with a sleep, for slower devices.
     */

    event::sysex & data = const_cast<event::sysex &>(e24->get_sysex());
    int data_size = e24->sysex_size();
    if (data_size < c_sysex_chunk)
    {
        snd_seq_ev_set_sysex(&ev, data_size, &data[0]);

        int rc = snd_seq_event_output_direct(m_seq, &ev);
        if (rc >= 0)
        {
            api_flush();
        }
        else
        {
            errprint("Sending complete SysEx failed");
        }
    }
    else
    {
        for (int offset = 0; offset < data_size; offset += c_sysex_chunk)
        {
            int data_left = data_size - offset;
            snd_seq_ev_set_sysex
            (
                &ev, min(data_left, c_sysex_chunk), &data[offset]
            );

            int rc = snd_seq_event_output_direct(m_seq, &ev);
            if (rc >= 0)
            {
                usleep(c_sysex_sleep_us);
                api_flush();
            }
            else
            {
                errprint("Sending SysEx failed");
            }
        }
    }
}

/**
 *  Flushes our local queue events out into ALSA.  This is also a
 *  midi_alsa_info function.
 */

void
midi_alsa::api_flush ()
{
    snd_seq_drain_output(m_seq);
}

/**
 *  Continue from the given tick.
 *
 *  Also defined in midi_alsa_info.
 *
 * \param tick
 *      The continuing tick, unused in the ALSA implementation here.
 *      The midibase::continue_from() function uses it.
 *
 * \param beats
 *      The beats value calculated by midibase::continue_from().
 */

void
midi_alsa::api_continue_from (midipulse /* tick */, midipulse beats)
{
    if (parent_bus().port_enabled())
    {
        snd_seq_event_t ev;
        snd_seq_ev_clear(&ev);                          /* clear event      */
        ev.type = SND_SEQ_EVENT_CONTINUE;

        snd_seq_event_t evc;
        snd_seq_ev_clear(&evc);                         /* clear event      */
        evc.type = SND_SEQ_EVENT_SONGPOS;
        evc.data.control.value = beats;
        snd_seq_ev_set_fixed(&ev);
        snd_seq_ev_set_fixed(&evc);
        snd_seq_ev_set_priority(&ev, 1);
        snd_seq_ev_set_priority(&evc, 1);
        snd_seq_ev_set_source(&evc, m_local_addr_port); /* set the source   */
        snd_seq_ev_set_subs(&evc);
        snd_seq_ev_set_source(&ev, m_local_addr_port);
        snd_seq_ev_set_subs(&ev);
        snd_seq_ev_set_direct(&ev);                     /* it's immediate   */
        snd_seq_ev_set_direct(&evc);
        snd_seq_event_output(m_seq, &evc);              /* pump into queue  */
        api_flush();
        snd_seq_event_output(m_seq, &ev);
    }
}

/**
 *  This function gets the MIDI clock a-runnin', if the clock type is not
 *  e_clock::off.  The clock-enabled status is checked in midibase::start().
 */

void
midi_alsa::api_start ()
{
    if (parent_bus().port_enabled())
    {
        snd_seq_event_t ev;
        snd_seq_ev_clear(&ev);                          /* memsets it to 0  */
        ev.type = SND_SEQ_EVENT_START;
        snd_seq_ev_set_fixed(&ev);
        snd_seq_ev_set_priority(&ev, 1);
        snd_seq_ev_set_source(&ev, m_local_addr_port);  /* set the source   */
        snd_seq_ev_set_subs(&ev);
        snd_seq_ev_set_direct(&ev);                     /* it's immediate   */
        snd_seq_event_output(m_seq, &ev);               /* pump into queue  */
    }
}

/**
 *  Stop the MIDI buss.
 */

void
midi_alsa::api_stop ()
{
    if (parent_bus().port_enabled())
    {
        snd_seq_event_t ev;
        snd_seq_ev_clear(&ev);                          /* memsets it to 0  */
        ev.type = SND_SEQ_EVENT_STOP;
        snd_seq_ev_set_fixed(&ev);
        snd_seq_ev_set_priority(&ev, 1);
        snd_seq_ev_set_source(&ev, m_local_addr_port);  /* set the source   */
        snd_seq_ev_set_subs(&ev);
        snd_seq_ev_set_direct(&ev);                     /* it's immediate   */
        snd_seq_event_output(m_seq, &ev);               /* pump into queue  */
    }
}

/**
 *  Generates the MIDI clock, starting at the given tick value.
 *  Also sets the event tag to 127 so the sequences won't remove it.
 *
 * \threadsafe
 *
 * \param tick
 *      Provides the starting tick, unused in the ALSA implementation.
 */

void
midi_alsa::api_clock (midipulse /*tick*/)
{
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);                          /* clear event          */
    ev.type = SND_SEQ_EVENT_CLOCK;
    ev.tag = 127;
    snd_seq_ev_set_fixed(&ev);
    snd_seq_ev_set_priority(&ev, 1);
    snd_seq_ev_set_source(&ev, m_local_addr_port);  /* set source           */
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);                     /* it's immediate       */
    snd_seq_event_output(m_seq, &ev);               /* pump it into queue   */
}

/**
 * Currently, this code is implemented in the midi_alsa_info module, since
 * it is a mastermidibus function.  Note the implementation here, though.
 * Which actually gets used?
 */

void
midi_alsa::api_set_ppqn (int ppqn)
{
    int queue = parent_bus().queue_number();
    snd_seq_queue_tempo_t * tempo;
    snd_seq_queue_tempo_alloca(&tempo);

    int rc = snd_seq_get_queue_tempo(m_seq, queue, tempo);
    if (rc == 0)
    {
        snd_seq_queue_tempo_set_ppq(tempo, ppqn);
        snd_seq_set_queue_tempo(m_seq, queue, tempo);
    }
}

/**
 *  Set the BPM value (beats per minute).  This is done by creating
 *  an ALSA tempo structure, adding tempo information to it, and then
 *  setting the ALSA sequencer object with this information.
 *
 *  We fill the ALSA tempo structure (snd_seq_queue_tempo_t) with the current
 *  tempo information, set the BPM value, put it in the tempo structure, and
 *  give the tempo value to the ALSA queue.
 *
 * \note
 *      Consider using snd_seq_change_queue_tempo() here if the ALSA queue has
 *      already been started.  It's arguments would be m_alsa_seq, m_queue,
 *      tempo (microseconds), and null.
 *
 * \threadsafe
 *
 * \param bpm
 *      Provides the beats-per-minute value to set.
 */

void
midi_alsa::api_set_beats_per_minute (midibpm bpm)
{
    int queue = parent_bus().queue_number();
    snd_seq_queue_tempo_t * tempo;
    snd_seq_queue_tempo_alloca(&tempo);          /* allocate tempo struct */

    int rc = snd_seq_get_queue_tempo(m_seq, queue, tempo);
    if (rc == 0)
    {
        snd_seq_queue_tempo_set_tempo(tempo, unsigned(tempo_us_from_bpm(bpm)));
        snd_seq_set_queue_tempo(m_seq, queue, tempo);
    }
}

#if defined REMOVE_QUEUED_ON_EVENTS_CODE

/**
 *  Deletes events in the queue.  This function is not used anywhere, and
 *  there was no comment about the intent/context of this function.
 */

void
midi_alsa::remove_queued_on_events (int tag)
{
    snd_seq_remove_events_t * remove_events;
    snd_seq_remove_events_malloc(&remove_events);
    snd_seq_remove_events_set_condition
    (
        remove_events, SND_SEQ_REMOVE_OUTPUT | SND_SEQ_REMOVE_TAG_MATCH |
            SND_SEQ_REMOVE_IGNORE_OFF
    );
    snd_seq_remove_events_set_tag(remove_events, tag);
    snd_seq_remove_events(m_seq, remove_events);
    snd_seq_remove_events_free(remove_events);
}

#endif      // REMOVE_QUEUED_ON_EVENTS_CODE

/*
 * --------------------------------------------------------------------------
 *  midi_in_alsa
 * --------------------------------------------------------------------------
 */

/**
 *  ALSA MIDI input normal port or virtual port constructor.  The kind of port
 *  is determine by which port-initialization function the mastermidibus
 *  calls.
 */

midi_in_alsa::midi_in_alsa (midibus & parentbus, midi_info & masterinfo) :
    midi_alsa   (parentbus, masterinfo)
{
    // Empty body
}

int
midi_in_alsa::api_poll_for_midi ()
{
    return 0;                       /* master_info().api_poll_for_midi();   */
}

/*
 * --------------------------------------------------------------------------
 *  midi_out_alsa
 * --------------------------------------------------------------------------
 */

/**
 *  ALSA MIDI output normal port or virtual port constructor.  The kind of
 *  port is determine by which port-initialization function the mastermidibus
 *  calls.
 */

midi_out_alsa::midi_out_alsa (midibus & parentbus, midi_info & masterinfo) :
    midi_alsa   (parentbus, masterinfo)
{
    // Empty body
}

}           // namespace seq66

/*
 * midi_alsa.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


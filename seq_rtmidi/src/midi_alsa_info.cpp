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
 * \file          midi_alsa_info.cpp
 *
 *    A class for obtaining ALSA information.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2022-06-20
 * \license       See above.
 *
 *  API information found at:
 *
 *      - http://www.alsa-project.org/documentation.php#Library
 *
 *  This class is meant to collect a whole bunch of ALSA information about
 *  client number, port numbers, and port names, and hold them for usage when
 *  creating ALSA midibus objects and midi_alsa API objects.
 *
 *  This was to be a function to create an ALSA "announce" bus.  But it turned
 *  out to be feasible and simpler to add it as a special input port in the
 *  get_all_port_info() function.  Still, the discussion here is useful.
 *
 *  A sequencer core has two pre-defined system ports on the system client
 *  SND_SEQ_CLIENT_SYSTEM: SND_SEQ_PORT_SYSTEM_TIMER and
 *  SND_SEQ_PORT_SYSTEM_ANNOUNCE. The SND_SEQ_PORT_SYSTEM_TIMER is the system
 *  timer port, and SND_SEQ_PORT_SYSTEM_ANNOUNCE is the system announce port.
 *
 * Timer:
 *
 *  In order to control a queue from a client, client should send a
 *  queue-control event like start, stop and continue queue, change tempo,
 *  etc. to the system timer port. Then the sequencer system handles the queue
 *  according to the received event. This port supports subscription. The
 *  received timer events are broadcasted to all subscribed clients.  From
 *  SND_SEQ_PORT_SYSTEM_TIMER, one may receive SND_SEQ_EVENT_START events.
 *
 * Announce:
 *
 *  The SND_SEQ_PORT_SYSTEM_ANNOUNCE port does not receive messages, but
 *  supports subscription. When each client or port is attached, detached or
 *  modified, an announcement is sent to subscribers from this port.  From
 *  SND_SEQ_PORT_SYSTEM_ANNOUNCE, one may receive
 *  SND_SEQ_EVENT_PORT_SUBSCRIBED events.
 *
 * Capability bits (FYI):
 *
 \verbatim
        SND_SEQ_PORT_CAP_READ           0x01
        SND_SEQ_PORT_CAP_WRITE          0x02
        SND_SEQ_PORT_CAP_SYNC_READ      0x04
        SND_SEQ_PORT_CAP_SYNC_WRITE     0x08
        SND_SEQ_PORT_CAP_DUPLEX         0x10
        SND_SEQ_PORT_CAP_SUBS_READ      0x20
        SND_SEQ_PORT_CAP_SUBS_WRITE     0x40
        SND_SEQ_PORT_CAP_NO_EXPORT      0x80
 \endverbatim
 */

#include "cfg/settings.hpp"             /* seq66::rc() configuration object */
#include "midi/event.hpp"               /* seq66::event and other tokens    */
#include "midi/midibus_common.hpp"      /* from the libseq66 sub-project    */
#include "midi_alsa_info.hpp"           /* seq66::midi_alsa_info            */
#include "util/basic_macros.hpp"        /* C++ version of easy macros       */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  We tried opening the ALSA port in non-blocking mode.  Didn't seem to
 *  offer any benefit.
 *
 *  -   0                       Blocking mode.
 *  -   SND_SEQ_NONBLOCK        Non-blocking mode.
 *
 *  We did reduce the polling timeout from 1000 milliseconds (in Seq24) to 100
 *  milliseconds, and now, after testing, 10 milliseconds, and removed the
 *  additional 100 microsecond wait.
 */

static const int c_poll_wait_ms     = 10;
static const int c_open_block_mode  = SND_SEQ_NONBLOCK;

/*
 * Initialization of static members.
 */

unsigned midi_alsa_info::sm_input_caps =
    SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ;

unsigned midi_alsa_info::sm_output_caps =
    SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE;

/**
 *  Principal constructor.
 *
 * \param appname
 *      Provides the name of the application.
 *
 * \param ppqn
 *      Provides the PPQN value needed by this object.
 *
 * \param bpm
 *      Provides the beats/minute value needed by this object.
 */

midi_alsa_info::midi_alsa_info
(
    const std::string & appname,
    int ppqn,
    midibpm bpm
) :
    midi_info               (appname, ppqn, bpm),
    m_alsa_seq              (nullptr),
    m_num_poll_descriptors  (0),            /* from ALSA mastermidibus      */
    m_poll_descriptors      (nullptr)       /* ditto                        */
{
    snd_seq_t * seq;                        /* point to member              */
    int rcode = snd_seq_open                /* set up ALSA sequencer client */
    (
        &seq, "default", SND_SEQ_OPEN_DUPLEX, c_open_block_mode
    );
    if (rcode < 0)
    {
        m_error_string = "error opening ALSA seq client";
        error(rterror::kind::driver_error, m_error_string);
    }
    else
    {
        /*
         * Save the ALSA "handle".  Set the client's name for ALSA.  Then set up
         * the ALSA client queue.  rc().app_client_name() gets a name that is not
         * necessarily on the executable name.
         */

        m_alsa_seq = seq;
        midi_handle(seq);
        snd_seq_set_client_name(m_alsa_seq, rc().app_client_name().c_str());
        global_queue(snd_seq_alloc_queue(m_alsa_seq));
        get_poll_descriptors();
    }
}

/**
 *  Destructor.  Closes a connection if it exists, shuts down the input
 *  thread, and then cleans up any API resources in use.
 */

midi_alsa_info::~midi_alsa_info ()
{
    if (not_nullptr(m_alsa_seq))
    {
        snd_seq_event_t ev;
        snd_seq_ev_clear(&ev);                          /* memset it to 0   */
        snd_seq_stop_queue(m_alsa_seq, global_queue(), &ev);
        snd_seq_free_queue(m_alsa_seq, global_queue());
        snd_seq_close(m_alsa_seq);                      /* close client     */
        (void) snd_config_update_free_global();         /* more cleanup     */
        m_alsa_seq = nullptr;
        remove_poll_descriptors();
    }
}

/**
 *  Get the number of MIDI input poll file descriptors.  Allocate the
 *  poll-descriptors array.  Then get the input poll-descriptors into the
 *  array.  Finally, set the input and output buffer sizes.  Can we do this
 *  before creating all the MIDI busses?  If not, we'll put them in a separate
 *  function to call later.
 *
 *  This function is called in the constructor and in api_port_start().
 */

void
midi_alsa_info::get_poll_descriptors ()
{
    m_num_poll_descriptors = snd_seq_poll_descriptors_count(m_alsa_seq, POLLIN);
    if (m_num_poll_descriptors > 0)
    {
        m_poll_descriptors = new (std::nothrow) pollfd[m_num_poll_descriptors];
        if (not_nullptr(m_poll_descriptors))
        {
            snd_seq_poll_descriptors                /* get input descriptors */
            (
                m_alsa_seq, m_poll_descriptors, m_num_poll_descriptors, POLLIN
            );
            snd_seq_set_output_buffer_size(m_alsa_seq, c_midibus_output_size);
            snd_seq_set_input_buffer_size(m_alsa_seq, c_midibus_input_size);
        }
    }
    else
    {
        errprint("No ALSA poll descriptors found");
    }
}

/**
 *  Removes the poll descriptors.
 */

void
midi_alsa_info::remove_poll_descriptors ()
{
    if (not_nullptr(m_poll_descriptors))
    {
        struct pollfd * pds = m_poll_descriptors;
        m_poll_descriptors = nullptr;
        m_num_poll_descriptors = 0;
        delete [] pds;
    }
}

/**
 *  Checks the port type for not being the "generic" types
 *  SND_SEQ_PORT_TYPE_MIDI_GENERIC and SND_SEQ_PORT_TYPE_SYNTH.
 */

bool
midi_alsa_info::check_port_type (snd_seq_port_info_t * pinfo) const
{
    unsigned alsatype = snd_seq_port_info_get_type(pinfo);
    return
    (
        ((alsatype & SND_SEQ_PORT_TYPE_MIDI_GENERIC) == 0) &&
        ((alsatype & SND_SEQ_PORT_TYPE_SYNTH) == 0)
    );
}

/**
 *  Gets information on ALL ports, putting input data into one midi_info
 *  container, and putting output data into another container.  For ALSA
 *  input, the first item added is the ALSA MIDI system "announce" buss.
 *  It has the client:port value of "0:1", denoted by the ALSA macros
 *  SND_SEQ_CLIENT_SYSTEM:SND_SEQ_PORT_SYSTEM_ANNOUNCE.
 *  The information obtained is:
 *
 *      -   Client name
 *      -   Port number
 *      -   Port name
 *      -   Port capabilities
 *
 * \return
 *      Returns the total number of ports found.  For an ALSA setup, finding
 *      no ALSA ports can be considered an error.  However, finding no ports
 *      for other APIS may be fine.  So, we set the result to -1 to flag a
 *      true error.
 */

int
midi_alsa_info::get_all_port_info
(
    midi_port_info & inputports,
    midi_port_info & outputports
)
{
    int result = 0;
    if (not_nullptr(m_alsa_seq))
    {
        snd_seq_port_info_t * pinfo;                /* point to member      */
        snd_seq_client_info_t * cinfo;
        snd_seq_client_info_alloca(&cinfo);
        snd_seq_client_info_set_client(cinfo, -1);
        inputports.clear();
        outputports.clear();
        inputports.add
        (
            SND_SEQ_CLIENT_SYSTEM, "system",
            SND_SEQ_PORT_SYSTEM_ANNOUNCE, "announce",
            midibase::io::input, midibase::port::system,
            global_queue()
        );
        ++result;
        while (snd_seq_query_next_client(m_alsa_seq, cinfo) >= 0)
        {
            int client = snd_seq_client_info_get_client(cinfo);
            if (client == SND_SEQ_CLIENT_SYSTEM)    /* i.e. 0 in alsa/seq.h */
            {
                /*
                 * Client 0 won't have ports (timer and announce) that match
                 * the MIDI-generic and Synth types checked below.
                 */

                continue;
            }

            snd_seq_port_info_alloca(&pinfo);
            snd_seq_port_info_set_client(pinfo, client); /* reset query info */
            snd_seq_port_info_set_port(pinfo, -1);
            while (snd_seq_query_next_port(m_alsa_seq, pinfo) >= 0)
            {
                if (check_port_type(pinfo))
                    continue;

                unsigned caps = snd_seq_port_info_get_capability(pinfo);
                std::string clientname = snd_seq_client_info_get_name(cinfo);
                std::string portname = snd_seq_port_info_get_name(pinfo);
                int portnumber = snd_seq_port_info_get_port(pinfo);
                if ((caps & sm_input_caps) == sm_input_caps)
                {
                    inputports.add
                    (
                        client, clientname, portnumber, portname,
                        midibase::io::input, midibase::port::normal,
                        global_queue()
                    );
                    ++result;
                }
                if ((caps & sm_output_caps) == sm_output_caps)
                {
                    outputports.add
                    (
                        client, clientname, portnumber, portname,
                        midibase::io::input, midibase::port::normal
                    );
                    ++result;
                }
                else
                {
                    /*
                     * When VMPK is running, we get this message for a
                     * client-name of 'VMPK Output'.
                     */

                    warnprintf("Non-I/O port '%s'", clientname.c_str());
                }
            }
        }
    }
    if (result == 0)
        result = (-1);

    return result;
}

/**
 *  Flushes our local queue events out into ALSA.  This is also a midi_alsa
 *  function.
 */

void
midi_alsa_info::api_flush ()
{
    snd_seq_drain_output(m_alsa_seq);
}

bool
midi_alsa_info::api_connect ()
{
    return true;
}

/**
 *  Sets the PPQN numeric value, then makes ALSA calls to set up the PPQN
 *  tempo.  The steps are:
 *
 *      -   Allocate tempo structure.
 *      -   Fill tempo structure with current tempo info.
 *      -   Set the PPQN.
 *      -   Give tempo structure to the queue.
 *
 * \param p
 *      The desired new PPQN value to set.
 */

void
midi_alsa_info::api_set_ppqn (int p)
{
    int queue = global_queue();
    midi_info::api_set_ppqn(p);
    snd_seq_queue_tempo_t * tempo;
    snd_seq_queue_tempo_alloca(&tempo);                 /* allocate struct  */
    snd_seq_get_queue_tempo(m_alsa_seq, queue, tempo);
    snd_seq_queue_tempo_set_ppq(tempo, p);
    snd_seq_set_queue_tempo(m_alsa_seq, queue, tempo);
}

/**
 *  Sets the BPM numeric value, then makes ALSA calls to set up the BPM
 *  tempo.  The steps are:
 *
 *      -   Allocate tempo structure.
 *      -   Fill tempo structure with current tempo info.
 *      -   Set the tempo.
 *      -   Give tempo structure to the queue.
 *
 * \param b
 *      The desired new BPM value to set.
 */

void
midi_alsa_info::api_set_beats_per_minute (midibpm b)
{
    int queue = global_queue();
    midi_info::api_set_beats_per_minute(b);
    snd_seq_queue_tempo_t * tempo;
    snd_seq_queue_tempo_alloca(&tempo);          /* allocate tempo struct */
    snd_seq_get_queue_tempo(m_alsa_seq, queue, tempo);
    snd_seq_queue_tempo_set_tempo(tempo, unsigned(tempo_us_from_bpm(b)));
    snd_seq_set_queue_tempo(m_alsa_seq, queue, tempo);
}

/**
 *  Polls for any ALSA MIDI information using a timeout value of 10
 *  milliseconds.
 *
 * \return
 *      Returns the result of the call to poll() on the global ALSA poll
 *      descriptors.
 */

int
midi_alsa_info::api_poll_for_midi ()
{
    int result = poll
    (
        m_poll_descriptors, m_num_poll_descriptors, c_poll_wait_ms
    );
    return result;
}

/*
 * Definitions copped from the seq_alsamidi/src/mastermidibus.cpp module.
 */

/**
 *  Macros to make capabilities-checking more readable.
 */

#define CAP_READ(cap)       (((cap) & SND_SEQ_PORT_CAP_SUBS_READ) != 0)
#define CAP_WRITE(cap)      (((cap) & SND_SEQ_PORT_CAP_SUBS_WRITE) != 0)

/**
 *  These checks need both bits to be set.  Intermediate macros used for
 *  readability.
 */

#define CAP_R_BITS      (SND_SEQ_PORT_CAP_SUBS_READ | SND_SEQ_PORT_CAP_READ)
#define CAP_W_BITS      (SND_SEQ_PORT_CAP_SUBS_WRITE | SND_SEQ_PORT_CAP_WRITE)

#define CAP_FULL_READ(cap)  (((cap) & CAP_R_BITS) == CAP_R_BITS)
#define CAP_FULL_WRITE(cap) (((cap) & CAP_W_BITS) == CAP_W_BITS)

#define ALSA_CLIENT_CHECK(pinfo) \
    (snd_seq_client_id(m_alsa_seq) != snd_seq_port_info_get_client(pinfo))

/**
 *  Start the given ALSA MIDI port.  This function is called by
 *  api_get_midi_event() when an ALSA event SND_SEQ_EVENT_PORT_START is
 *  received.
 *
 *  -   Get the API's client and port information.
 *  -   Do some capability checks.
 *  -   Find the client/port combination among the set of input/output busses.
 *      If it exists and is not active, then mark it as a replacement.  If it
 *      is not a replacement, it will increment the number of input/output
 *      busses.
 *
 *  We can simplify this code a bit by using elements already present in
 *  midi_alsa_info.
 *
 *  Also, the midibus pointers created here are local, but the busarray::add()
 *  function manages them, using the std::unique_ptr<> template.  We could use
 *  std::unique_ptr<> here, and even std::make_unique() if we wanted to require
 *  C++14 at this time.
 *
 * \param masterbus
 *      Provides the object that is need to get access to the busses that need
 *      to be started.
 *
 * \param bus
 *      Provides the ALSA bus/client number.
 *
 * \param port
 *      Provides the ALSA client port.
 */

void
midi_alsa_info::api_port_start (mastermidibus & masterbus, int bus, int port)
{
    snd_seq_client_info_t * cinfo;                          /* get bus info  */
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_get_any_client_info(m_alsa_seq, bus, cinfo);
    snd_seq_port_info_t * pinfo;                            /* get port info */
    snd_seq_port_info_alloca(&pinfo);
    snd_seq_get_any_port_info(m_alsa_seq, bus, port, pinfo);

    int cap = snd_seq_port_info_get_capability(pinfo);      /* get caps      */
    if (ALSA_CLIENT_CHECK(pinfo))
    {
        if (CAP_FULL_WRITE(cap) && ALSA_CLIENT_CHECK(pinfo)) /* outputs      */
        {
            int bus_slot = masterbus.m_outbus_array.count();
            int test = masterbus.m_outbus_array.replacement_port(bus, port);
            if (test >= 0)
                bus_slot = test;

            midibus * m = new (std::nothrow) midibus
            (
                masterbus.m_midi_master, bus_slot
            );
            if (not_nullptr(m))
            {
                m->is_virtual_port(false);
                m->is_input_port(false);
                masterbus.m_outbus_array.add(m, e_clock::off);
            }
        }
        if (CAP_FULL_READ(cap) && ALSA_CLIENT_CHECK(pinfo)) /* inputs */
        {
            int bus_slot = masterbus.m_inbus_array.count();
            int test = masterbus.m_inbus_array.replacement_port(bus, port);
            if (test >= 0)
                bus_slot = test;

            midibus * m = new (std::nothrow) midibus
            (
                masterbus.m_midi_master, bus_slot
            );
            if (not_nullptr(m))
            {
                m->is_virtual_port(false);
                m->is_input_port(true);
                masterbus.m_inbus_array.add(m, false);
            }
        }
    }                                               /* end loop for clients */

    /*
     * Get the number of MIDI input poll file descriptors.  This is done in the
     * constructor, too!
     */

    remove_poll_descriptors();
    get_poll_descriptors();
}

/**
 *  For debugging, we may expose the following static function for use for
 *  normal (and usually copious) incoming MIDI events.  For less common
 *  events, like port/client subscription, the debugging can be enabled by the
 *  "--verbose" command-line option.
 */

bool
midi_alsa_info::show_event (snd_seq_event_t * ev, const char * tag)
{
    if (rc().verbose())
    {
        int c = int(ev->source.client);
        int p = int(ev->source.port);
        int b = int(input_ports().get_port_index(c, p));
        char tmp[80];
        snprintf
        (
            tmp, sizeof tmp, "[%s event[%d] = 0x%x: client %d port %d]",
            tag, b, unsigned(ev->type), c, p
        );
        info_message(tmp);
    }
    return true;
}


/**
 *  Grab a MIDI event.  First, a rather large buffer is allocated on the stack
 *  to hold the MIDI event data.  Next, if the --alsa-manual-ports option is
 *  not in force, then we check to see if the event is a port-start,
 *  port-exit, or port-change event, and we prcess it, and are done.
 *
 *  Otherwise, we create a "MIDI event parser" and decode the MIDI event.
 *
 *  We've beefed up the error-checking in this function due to crashes we got
 *  when connected to VMPK and suddenly getting a rush of ghost notes, then a
 *  seqfault.  This also occurs in legacy seq66.  To reproduce, run VMPK and
 *  make it the input source.  Open a new pattern, turn on recording, and
 *  start the ALSA transport.  Record one note.  Then activate the button for
 *  "dump input to MIDI bus".  You will here the note through VMPK, then ghost
 *  notes start appearing and seq66/seq66 eventually crash.  A bug in VMPK, or
 *  our processing?  At any rate, we catch the bug now, and don't crash, but
 *  eventually processing gets swamped until we kill VMPK.  And we now have a
 *  note sounding even though neither app is running.  Really screws up ALSA!
 *
 * ALSA events:
 *
 *      The ALSA events are listed in the snd_seq_event_type enumeration in
 *      /usr/lib/alsa/seq_event.h, where the "normal" MIDI events (from Note
 *      On to Key Signature) have values ranging from 5 to almost 30.  But
 *      there are some special ALSA events we need to handle in a different
 *      manner (currently by ignoring them):
 *
 *      -  0x3c: SND_SEQ_EVENT_CLIENT_START
 *      -  0x3d: SND_SEQ_EVENT_CLIENT_EXIT
 *      -  0x3e: SND_SEQ_EVENT_CLIENT_CHANGE
 *      -  0x3f: SND_SEQ_EVENT_PORT_START
 *      -  0x40: SND_SEQ_EVENT_PORT_EXIT
 *      -  0x41: SND_SEQ_EVENT_PORT_CHANGE
 *      -  0x42: SND_SEQ_EVENT_PORT_SUBSCRIBED
 *      -  0x43: SND_SEQ_EVENT_PORT_UNSUBSCRIBED
 *
 *  We will add more special events here as we find them.
 *
 * \todo
 *      Also, we need to consider using the new remcount return code to loop
 *      on receiving events as long as we are getting them.
 *
 * \param inev
 *      The event to be set based on the found input event.  It is the
 *      destination for the incoming event.
 *
 * \return
 *      This function returns false if we are not using virtual/manual ports
 *      and the event is an ALSA port-start, port-exit, or port-change event.
 *      It also returns false if there is no event to decode.  Otherwise, it
 *      returns true.
 */

bool
midi_alsa_info::api_get_midi_event (event * inev)
{
    bool result = false;
    snd_seq_event_t * ev;
    int remcount = snd_seq_event_input(m_alsa_seq, &ev);
    if (remcount < 0 || is_nullptr(ev))
    {
        if (remcount == -EAGAIN)
        {
            // no input in non-blocking mode
        }
        else if (remcount == -ENOSPC)
        {
            errprint("input FIFO overrun");
        }
        else
            errprint("snd_seq_event_input() failure");

        return false;
    }
    if (! rc().manual_ports())
    {
        switch (ev->type)
        {
        case SND_SEQ_EVENT_CLIENT_START:

            result = show_event(ev, "Client start");
            break;

        case SND_SEQ_EVENT_CLIENT_EXIT:

            result = show_event(ev, "Client exit");
            break;

        case SND_SEQ_EVENT_CLIENT_CHANGE:

            result = show_event(ev, "Client change");
            break;

        case SND_SEQ_EVENT_PORT_START:
        {
            /*
             * Figure out how to best do this.  It has way too many parameters
             * now, and is currently meant to be called from mastermidibus.
             * See mastermidibase::port_start().
             *
             * port_start(masterbus, ev->data.addr.client, ev->data.addr.port);
             * api_port_start (mastermidibus & masterbus, int bus, int port)
             */

            result = show_event(ev, "Port start");
            break;
        }
        case SND_SEQ_EVENT_PORT_EXIT:
        {
            /*
             * The port_exit() function is defined in mastermidibase and in
             * businfo.  They seem to cover this functionality.
             *
             * port_exit(masterbus, ev->data.addr.client, ev->data.addr.port);
             */

            result = show_event(ev, "Port exit");
            break;
        }
        case SND_SEQ_EVENT_PORT_CHANGE:
        {
            result = show_event(ev, "Port change");
            break;
        }
        case SND_SEQ_EVENT_PORT_SUBSCRIBED:

            result = show_event(ev, "Port subscribed");
            break;

        case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:

            result = show_event(ev, "Port unsubscribed");
            break;

        default:
#if defined SEQ66_PLATFORM_DEBUG_TMI
            result = show_event(ev, "Port other");
#endif
            break;
        }
    }
    if (result)
        return false;

    midibyte buffer[0x1000];                    /* 4096 buffer for data     */
    snd_midi_event_t * midi_ev;                 /* make ALSA MIDI parser    */
    int rc = snd_midi_event_new(sizeof buffer, &midi_ev);
    if (rc < 0 || is_nullptr(midi_ev))
    {
        errprint("snd_midi_event_new() failed");
        return false;
    }

    /*
     *  Note that ev->time.tick is always 0.  (Same in Seq32).  Not sure about
     *  this handling of SysEx data. Apparently one can get only up to ALSA
     *  buffer size (4096) of data.  Also, the snd_seq_event_input() function
     *  is said to block!
     */

    long bytes = snd_midi_event_decode(midi_ev, buffer, sizeof buffer, ev);
    if (bytes > 0)
    {
        result = inev->set_midi_event(ev->time.tick, buffer, bytes);
        if (result)
        {
            bussbyte b = input_ports().get_port_index
            (
                int(ev->source.client), int(ev->source.port)
            );
            bool sysex = inev->is_sysex();
            inev->set_input_bus(b);
#if defined SEQ66_PLATFORM_DEBUG_TMI
            warnprintf("Input on buss %d\n", int(b));
#endif
            while (sysex)           /* sysex might be more than one message */
            {
                int remcount = snd_seq_event_input(m_alsa_seq, &ev);
                long bytes = snd_midi_event_decode
                (
                    midi_ev, buffer, sizeof buffer, ev
                );
                if (bytes > 0)
                {
                    sysex = inev->append_sysex(buffer, bytes);
                    if (remcount == 0)
                        sysex = false;
                }
                else
                    sysex = false;
            }
        }
        snd_midi_event_free(midi_ev);
        return true;
    }
    else
    {
        /*
         * This happens even at startup, before anything is really happening.
         */

        snd_midi_event_free(midi_ev);

#if defined SEQ66_PLATFORM_DEBUG_TMI
        errprintf("snd_midi_event_decode() returned %ld", bytes);
#endif

        return false;
    }
}

}           // namespace seq66

/*
 * midi_alsa_info.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


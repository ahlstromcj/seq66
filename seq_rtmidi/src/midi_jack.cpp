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
 * \file          midi_jack.cpp
 *
 *    A class for realtime MIDI input/output via JACK.
 *
 * \library       seq66 application
 * \author        Gary P. Scavone; severe refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2022-03-13
 * \license       See above.
 *
 *  Written primarily by Alexander Svetalkin, with updates for delta time by
 *  Gary Scavone, April 2011.
 *
 *  In this Seq66 refactoring of RtMidi, we have to warp the RtMidi
 *  model, where ports are opened directly by the application, to the
 *  Seq66 midibus model, where the port object is created, but
 *  initialized after creation.  This proved very challenging -- it took a
 *  long time to get the midi_alsa implementation working, and still more time
 *  to get the midi_jack implementation solid.  So to call this code "rtmidi"
 *  code is slightly misleading.
 *
 *  There is an additional issue with JACK ports.  First, think of our ALSA
 *  implementation.  We have two modes:  manual (virtual) and real (normal)
 *  ports.  In ALSA, the manual mode exposes Seq66 ports (1 input port,
 *  16 output ports) to which other applications can connect.  The real/normal
 *  mode, via a midi_alsa_info object, determines the list of existing ALSA
 *  ports in the system, and then Seq66 ports are created (via the
 *  midibus) that are local, but connected to these "destination" system
 *  ports.
 *
 *  In JACK, we can do something similar.  Use the manual/virtual mode to
 *  allow Sequencer64 (seq66) to be connected manually via something like
 *  QJackCtl or a session manager.  Use the real/normal mode to connect
 *  automatically to whatever is already present.  Currently, though, new
 *  devices that appear in the system won't be accessible until a restart.
 *  (Or perhaps a reconnection using a JACK manager like QJackCtl.)
 *
 * Random JACK notes:
 *
 *      jack_activate() tells the JACK server that the program is ready to
 *      process JACK events.  jack_deactivate() removes the JACK client from
 *      the process graph and disconnects all ports belonging to it.
 *
 * Callbacks:
 *
 *      The input JACK callback can call an rtmidi input callback of the form
 *
 *          void callback (midi_message & message, void * userdata)
 *
 *      This callback is wired in by calling rtmidi_in_data ::
 *      user_callback().  Unlike RtMidi, the delta time is stored as part of
 *      the message.
 *
 * JackPortFlags:
 *
\verbatim
        JackPortIsInput     = 0x01
        JackPortIsOutput    = 0x02
        JackPortIsPhysical  = 0x04
        JackPortCanMonitor  = 0x08
        JackPortIsTerminal  = 0x10
\endverbatim
 *
 * I/O Issues:
 *
 *      The nomenclature for JACK input/output ports seems to be backwards of
 *      that for ALSA.  Note the confusing (but necessary) orientation of the
 *      driver backend ports: playback ports are "input" to the backend, and
 *      capture ports are "output" from the backend.  Here are the properties
 *      we have gleaned for JACK I/O ports:
 *
 *      -#  JACK Input Port.
 *          -#  A writable client.
 *          -#  Accepts input from an application or device.
 *          -#  We create an "output" port and connect it to this input port,
 *              so that we can send data to the port.
 *          -#  We set up an "output" JACK process callback that hands off the
 *              data to JACK, so that it can be played.
 *      -#  JACK Output Port.
 *          -#  A readable client.
 *          -#  Provides output to an application or device.
 *          -#  We create an "input" port and connect it to this output port,
 *              so that we can receive data from the port.
 *          -#  We set up an "input" JACK process callback that provides us
 *              with the data collected by JACK, so that we can record the
 *              data.
 *
 *  jack_port_get_buffer() returns a pointer to the memory area associated with
 *  the specified port. For an output port, it will be a memory area that can be
 *  written to; for an input port, it will be an area containing the data from
 *  the port's connection(s), or zero-filled. If there are multiple inbound
 *  connections, the data will be mixed appropriately.  Do not cache the
 *  returned address across process() callbacks. Port buffers have to be
 *  retrieved in each callback for proper functionning.
 *
 *  jack_midi_clear_buffer() clears the buffer, which must be an output buffer.
 *  It must be called before calling jack_midi_event_reserve() or
 *  jack_midi_event_write().  This function may not be called on an input
 *  port's buffer! The function jack_midi_reset_buffer() is deprecated.
 *
 *  jack_midi_event_reserve() allocates space for an event to be written to an
 *  event port buffer.  Clients must write the event data to the pointer
 *  returned by this function. Clients must not write more than data_size
 *  bytes into this buffer. Clients must write normalized MIDI data to the
 *  port - no running status and no (1-byte) realtime messages interspersed
 *  with other messages (realtime messages are fine when they occur on their
 *  own).  The events must be written in order, sorted by their offsets.  JACK
 *  will not sort the events, and will refuse to store out-of-order events.
 *  The offset ranges from 0 to nframes, where nframes is a parameter passed
 *  to the callback.
 *
 *  jack_ringbuffer_read() reads data from the ring-buffer and advances the data
 *  pointer.  The first parameter is the pointer to the ring-buffer.  The second
 *  parameter is the destination for the data that is read from the ring-buffer.
 *  The third parameter is the number of bytes to read.  It returns the number
 *  of bytes actually read.
 *
 *  jack_midi_event_get() gets a MIDI event from an event port buffer.  JACK
 *  MIDI is normalized; the MIDI event returned by this function is guaranteed
 *  to be a complete MIDI event (the status byte is always present, and no
 *  realtime events are interspersed with the event).  It returns 0 on
 *  success, or ENODATA if buffer is empty.
 *
 *	jack_nframes_t last_frame_time = jack_last_frame_time(jack_client) gets
 *  the precise time at the start of the current process cycle.
 *  It may only be used from the process callback, and can be used to
 *  interpret timestamps generated by `frame_time` in other threads with
 *  respect to the current process cycle.  This is the only JACK time function
 *  that returns exact time: when used during the process callback it always
 *  returns the same value (until the next process callback, where it will
 *  return that value + `blocksize`, etc).  The return value is guaranteed to
 *  be monotonic and linear in this fashion unless an XRUN occurs.  If an XRUN
 *  occurs, clients must check this value again, as time may have advanced in
 *  a non-linear way (e.g. cycles may have been skipped).
 *
 * MIDI clock:
 *
 *      We need to study the source code to the jack_midi_clock application to
 *      make sure we're doing this correctly.
 *
 *  GitHub issue #165: enabled a build and run with no JACK support.  Weird is
 *  that removing or moving the calculations.hpp header into the support macro
 *  section causes midi_jack member functions to be unresolved!
 */

#include "seq66-config.h"               /* SEQ66_JACK_SUPPORT                */

#if defined SEQ66_JACK_SUPPORT

#include <cstring>                      /* std::strcpy(), std::strcat()     */
#include <sstream>

#include <jack/midiport.h>
#include <jack/ringbuffer.h>

#if defined SEQ66_JACK_METADATA_TEST
#include <jack/metadata.h>
#endif

#include "cfg/settings.hpp"             /* seq66::rc() accessor function    */
#include "midi/event.hpp"               /* seq66::event from main library   */
#include "midi/jack_assistant.hpp"      /* seq66::jack_status_pair_t        */
#include "midibus_rm.hpp"               /* seq66::midibus for rtmidi        */
#include "midi_jack.hpp"                /* seq66::midi_jack                 */
#include "os/timing.hpp"                /* seq66::microsleep()              */

/**
 *  Delimits the size of the JACK ringbuffer.
 */

static const size_t c_jack_ringbuffer_size = 16384;

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Defines the JACK input process callback.  It is the JACK process callback
 *  for a MIDI output port (e.g. "system:midi_capture_1", which gives us the
 *  output of the Korg nanoKEY2 MIDI controller), also known as a "Readable
 *  Client" by qjackctl.  This callback receives data from JACK and gives it
 *  to our application's input port:
 *
 *      -#  Get the JACK port buffer and the MIDI event-count in this buffer.
 *      -#  For each MIDI event, get the event from JACK and push it into a
 *          local midi_message object.
 *      -#  Get the event time, converting it to a delta time if possible.
 *      -#  If it is not a SysEx continuation, then:
 *          -#  If we're using a callback, pass the data to that callback.  Do
 *              we need this callback to interface with the midibus-based
 *              code?
 *          -#  Otherwise, add the midi_message container to the rtmidi input
 *              queue.  One can then grab this data in a midibase ::
 *              poll_for_midi() call.  We still ought to check the add
 *              success.
 *
 *  The ALSA code polls for events, and that model is also available here.
 *  We're still working exactly how it will work best.
 *
 *  Since this is an input port, buff is the area that contains data from the
 *  "remote" (i.e. outside our application) port.  We do not check
 *  midi_jack_data::m_jack_port here, it should be good, or else.
 *
 *  This function used to be static, but now we make it available to
 *  midi_jack_info.  Also note the s_null_detected flag.
 *
 * Question:
 *
 *      When MIDI input comes in, does it accumulate if we don't call
 *      jack_midi_get_event()? Should we get it even if the port is
 *      disabled, and then just not use it?
 *
 * Error codes:
 *
 *      ENODATA = 61: No data available.
 *      ENOBUFS = 105: No buffer space available can happen.
 *
 * \param frameno
 *    The frame number to be processed.
 *
 * \param arg
 *    A pointer to the midi_jack_data structure to be processed.
 *
 * \return
 *    Returns 0 unless overflow occurs, then -1 is returned.
 */

int
jack_process_rtmidi_input (jack_nframes_t frameno, void * arg)
{
    midi_jack_data * jackdata = reinterpret_cast<midi_jack_data *>(arg);
    rtmidi_in_data * rtindata = jackdata->m_jack_rtmidiin;
    void * buf = ::jack_port_get_buffer(jackdata->m_jack_port, frameno);
    int evcount = ::jack_midi_get_event_count(buf);
    bool overflow = false;
    for (int j = 0; j < evcount; ++j)
    {
        jack_midi_event_t jmevent;
        int rc = ::jack_midi_event_get(&jmevent, buf, j);
        if (rc == 0)                                /* ENODATA if buf empty */
        {
            jack_time_t jtime = ::jack_get_time();  /* compute delta time   */
            jack_time_t delta_jtime;                /* uint64_t             */
            if (rtindata->first_message())
            {
                rtindata->first_message(false);
                delta_jtime = 0;
            }
            else
            {
                jtime -= jackdata->m_jack_lasttime;
                delta_jtime = jack_time_t(jtime * 0.000001);
            }
            jackdata->m_jack_lasttime = jtime;

            midi_message message(delta_jtime);
            int eventsize = int(jmevent.size);
            for (int i = 0; i < eventsize; ++i)
                message.push(jmevent.buffer[i]);

            if (! rtindata->continue_sysex())
            {
                if (! rtindata->queue().add(message))
                {
                    async_safe_strprint("~");
                    overflow = true;
                    break;
                }
            }
        }
        else
        {
            const char * errmsg = "rtmidi input error";
            if (rc == ENODATA)
                errmsg = "rtmidi input: ENODATA";
            else if (rc == ENOBUFS)
                errmsg = "rtmidi input: ENOBUFS";

            async_safe_errprint(errmsg);
        }
    }
    if (overflow)
    {
        async_safe_errprint(" Message overflow ");
        return (-1);
    }
    return 0;
}

/**
 *  Defines the JACK process output callback.  It is the JACK process callback
 *  for a MIDI output port (a midi_out_jack object associated with, for
 *  example, "system:midi_playback_1", representing, for example, a Korg
 *  nanoKEY2 to which we can send information), also known as a "Writable
 *  Client" by qjackctl.  Here's how it works:
 *
 *      -#  Get the JACK port buffer, for our local jack port.  Clear it.
 *      -#  Loop while the number of bytes available for reading [via
 *          jack_ringbuffer_read_space()] is non-zero.  Note that the second
 *          parameter is where the data is copied.
 *      -#  Get the size of each event, and allocate space for an event to be
 *          written to an event port buffer (the JACK "reserve" function).
 *      -#  Read the data from the ringbuffer into this port buffer.  JACK
 *          should then send it to the remote port.
 *
 *  Since this is an output port, "buff" is the area to which we can write
 *  data, to send it to the "remote" (i.e. outside our application) port.  The
 *  data is written to the ringbuffer in api_init_out(), and here we read the
 *  ring buffer and pass it to the output buffer.
 *
 *  We were wondering if, like the JACK midiseq example program, we need to
 *  wrap the out-process in a for-loop over the number of frames.  In our
 *  tests, we are getting 1024 frames, and the code seems to work without that
 *  loop.
 *
 * \param frameno
 *    The frame number to be processed.
 *
 * \param arg
 *    A pointer to the JackMIDIData structure to be processed.
 *
 * \return
 *    Returns 0.
 */

int
jack_process_rtmidi_output (jack_nframes_t frameno, void * arg)
{
    const size_t s_offset = 0;
    midi_jack_data * jackdata = reinterpret_cast<midi_jack_data *>(arg);
    void * buf = ::jack_port_get_buffer(jackdata->m_jack_port, frameno);
    ::jack_midi_clear_buffer(buf);                  /* no nullptr test      */

    /*
     * A for-loop over the number of frames?  See discussion above.
     * Why are we reading here?  That's where our app has dumped the next set
     * of MIDI events to output.
     */

    while (jack_ringbuffer_read_space(jackdata->m_jack_buffsize) > 0)
    {
        int space;
        (void) ::jack_ringbuffer_read
        (
            jackdata->m_jack_buffsize, (char *) &space, sizeof space
        );

        /*
         * s_offset is always 0. Using frameno instead of s_offset causes
         * notes not to be played.  Because this is a write operation?
         */

        jack_midi_data_t * md = ::jack_midi_event_reserve(buf, s_offset, space);
        if (not_nullptr(md))
        {
            char * mididata = reinterpret_cast<char *>(md);
            (void) ::jack_ringbuffer_read           /* copy into mididata   */
            (
                jackdata->m_jack_buffmessage, mididata, size_t(space)
            );
        }
        else
            async_safe_errprint("JACK Event Reserve null pointer");
    }
    return 0;
}

/**
 *  This callback is to shut down JACK by clearing the jack_assistant ::
 *  m_jack_running flag.
 *
 * \param arg
 *      Points to the jack_assistant in charge of JACK support for the
 *      performer object.
 */

void
jack_shutdown_callback (void * arg)
{
    midi_jack_info * jack = reinterpret_cast<midi_jack_info *>(arg);
    if (not_nullptr(jack))
        async_safe_strprint("JACK Shutdown");
    else
        async_safe_errprint("JACK Shutdown null pointer");
}

#if defined SEQ66_JACK_PORT_CONNECT_CALLBACK

/**
 *  Port-connect and port-register callbacks seem to get interleaved, making
 *  for confusing output.  We currently want to know only about registration,
 *  anwyay.
 *
 * \param a
 *      One of two ports connected/disconnected.
 *
 * \param b
 *      One of two ports connected/disconnected.
 *
 * \param connect
 *      Non-zero if ports were connected, zero if ports were disconnected.
 *
 * \param arg
 *      Pointer to a client supplied data (a midi_jack_info pointer).
 */

void
jack_port_connect_callback
(
    jack_port_id_t port_a, jack_port_id_t port_b,
    int connect, void * arg
)
{
    midi_jack_info * jack = reinterpret_cast<midi_jack_info *>(arg);
    if (not_nullptr(jack))
    {
        if (rc().investigate())
        {
            char value[c_async_safe_utoa_size];
            char temp[2 * c_async_safe_utoa_size + 48];
            std::strcpy(temp, "port-connect(");
            async_safe_utoa(unsigned(port_a), value);
            std::strcat(temp, value);
            async_safe_utoa(unsigned(port_b), value);
            std::strcat(temp, value);
            async_safe_utoa(unsigned(connect), value);
            std::strcat(temp, connect != 0 ? " connect" : " disconnect");
            std::strcat(temp, value);
            std::strcat(temp, not_nullptr(arg) ? " arg)" : " nullptr)");
            async_safe_strprint(temp);
        }
    }
}

#endif

/**
 *  Provides a callback for registering a port, so that we can detect when a
 *  port appears or disappears in the system.  We use jack_port_by_id() to
 *  get the port, then jack_port_is_mine() so that we can avoid processing our
 *  own port.
 *
 *  The original port handle is set via the port_handle() setter called in the
 *  member function register_port(). If this port_handle can be found in the
 *  list of ports, that should mean that it is a Seq66 port, and nothing should
 *  be done.  Otherwise, the I/O port containers should be updated.
 *
 *  Also see the api_get_port_name() function and the midi_jack_data class.
 *
 * Notes:
 *
 *  -   For a2j ports, the long and short names are like this:
 *      -   Long: 'a2j:nanoKEY2 [20] (playback): nanoKEY2 nanoKEY2 _ CTRL'
 *      -   Short: 'nanoKEY2 [20] (playback): nanoKEY2 nanoKEY2 _ CTRL'
 *  -   For Seq66's connections to a2j ports, the long and short names are
 *      like this:
 *      -   Long: 'seq66:a2j nanoKEY2 [20] (playback): nanoKEY2 nanoKEY2 _ CTRL'
 *      -   Short: 'a2j nanoKEY2 [20] (playback): nanoKEY2 nanoKEY2 _ CTRL'
 *  -   FluidSynth (on Ubuntu 20):
 *      -   Long: 'fluidsynth-midi:midi_00'
 *      -   Short: 'midi_00'.
 *
 * Desired activity steps:
 *
 *      -#  Startup.
 *          -#  As done now, gather the input and output ports into these
 *              containers (midi_port_info).
 *          -#  Also make copies of these containers.
 *      -#  MORE
 *
 * Rules for handling port registration and unregistration:
 *
 *      -#  Port change detection.
 *          -#  Changes should only be detected in non-Seq66 ("not mine") ports.
 *          -#  This occurs when a registratio or un-registration causes the
 *              callback to occur.
 *      -#  No special processing unless the user has enabled port-mapping.
 *          Otherwise, Seq66 port numbers, which are simply indexes into the
 *          port vectors, would change, breaking the song.
 *      -#  Ports are never removed from the port map.
 *          -#  Unregistered ports are simply disabled.
 *          -#  New ports are appended to the port map.
 *          -#  Ports accumulate.  The user must (currently) edit the 'rc' file
 *              to remove ports from the port-map, and restart the application.
 *      -#  Port removal.
 *          -#  Detection. Examine the active ports and the port map.
 *              -#  If the port is not in the active port list, do nothing.
 *              -#  If the port is not in the port map, do nothing.
 *          -#  Disable or de-initialize the port in:
 *              -#  In midi_jack_info/midi_info.
 *              -#  The port map.
 *              -#  The mastermidibus. ???
 *              -#  The busarray.
 *      -#  Port addition.
 *          -#  Detection. Examine the active ports and the port map.
 *              -#  If the port is in the active port list, do nothing.
 *              -#  If the port is in the port map, enable it.
 *          -#  If the port is new:
 *              -#  Append it to the portmap.
 *              -#  Append it to midi_jack_info/midi_info.
 *              -#  Add it to the mastermidis. ???
 *              -#  Add it to the busarray and initialize it.
 *      -#  User interface.  It must be updated to reflect the new port map.
 *          -#  Edit / Preferences MIDI Clock and MIDI Input.
 *          -#  Main window port list.
 *          #   The pattern editor(s) (if output).
 *
 * \param portid
 *      The ID of the port. It is a 32-bit unsigned integer.
 *
 * \param regv
 *      The registration value. It is non-zero if the port is being registered,
 *      and zero if the port is being unregistered.
 *
 * \param arg
 *      Pointer to a client supplied data, the midi_jack_info object.
 */

void
jack_port_register_callback (jack_port_id_t portid, int regv, void * arg)
{
    midi_jack_info * jackinfo = reinterpret_cast<midi_jack_info *>(arg);
    if (not_nullptr(jackinfo))
    {
        jack_client_t * handle = jackinfo->client_handle();
        jack_port_t * portptr = nullptr;
        if (not_nullptr(handle))
        {
            bool mine = false;
            int flags = 0;
            midibase::io iotype = midibase::io::indeterminate;
            std::string longname;
            std::string shortname;
            std::string porttype;
            portptr = ::jack_port_by_id(handle, portid);
            if (not_nullptr(portptr))
            {
                const char * ln = ::jack_port_name(portptr);
                const char * sn = ::jack_port_short_name(portptr);
                if (not_nullptr(ln))
                    longname = std::string(ln);

                if (not_nullptr(sn))
                    shortname = std::string(sn);

                mine = ::jack_port_is_mine(handle, portptr) != 0;
                flags = ::jack_port_flags(portptr);
                porttype = ::jack_port_type(portptr);
                if (flags & JackPortIsInput)
                    iotype = midibase::io::input;
                else if (flags & JackPortIsOutput)
                    iotype = midibase::io::output;
            }
            else
                return;                             /* fatal error, bug out */

            if (rc().investigate())
            {
                /*
                 * JACK docs say this function doesn't need to be suitable for
                 * real-time execution. But what about async-safety? It seems
                 * to be necessary; debug_message() yields intermixed output.
                 */

                const char * iot = "TBD";
                char value[c_async_safe_utoa_size];
                char temp[128];
                if (iotype == midibase::io::input)
                    iot = " In";
                else if (iotype == midibase::io::output)
                    iot = "Out";

                async_safe_utoa(unsigned(portid), &value[0], false);
                std::strcpy(temp, "Port ");
                std::strcat(temp, value);
                std::strcat(temp, ": ");
                std::strcat(temp, regv != 0 ? "Reg" : "Unreg");
                std::strcat(temp, " ");
                std::strcat(temp, iot);
                std::strcat(temp, " ");
                std::strncat(temp, shortname.c_str(), 30);   /* truncate it  */
                std::strcat(temp, "/ ");
                if (is_nullptr(arg))
                    std::strcat(temp, "nullptr! ");

                std::strcat(temp, porttype.c_str());
                if (mine)
                    std::strcat(temp, " seq66");

                async_safe_strprint(temp);
#if defined SEQ66_PLATFORM_DEBUG_TMI
                show_details();
#endif
            }
            jackinfo->update_port_list
            (
                mine, int(portid), regv, shortname, longname
            );
        }
    }
}

/*
 * MIDI JACK base class
 */

/**
 *  Note that this constructor also adds its object to the midi_jack_info port
 *  list, so that the JACK callback functions can iterate through all of the
 *  JACK ports in use by this application, performing work on them.
 *
 *  In midi_jack::api_init_[in/out](), which occurs in midibus::api_init_out()
 *  and results in a call to midi_jack::api_init_out(), the call to
 *  set_alt_name() changes the port name from something like "midi" to
 *  "fluidsynth-midi:midi".  That is, it makes the name out of the client name
 *  and (minimal) port name.
 *
 *  This is the short name we can look up.
 *
 * \param parentbus
 *      Provides a reference to the midibus that represents this object.
 *
 * \param masterinfo
 *      Provides a reference to the midi_jack_info object that may provide
 *      extra informatino that is needed by this port.  Too many entities!
 */

midi_jack::midi_jack (midibus & parentbus, midi_info & masterinfo) :
    midi_api            (parentbus, masterinfo),
    m_remote_port_name  (),
    m_jack_info         (dynamic_cast<midi_jack_info &>(masterinfo)),
    m_jack_data         ()
{
    client_handle(reinterpret_cast<jack_client_t *>(masterinfo.midi_handle()));
    (void) jack_info().add(*this);
}

/**
 *  This could be a rote empty destructor if we offload this destruction to the
 *  midi_jack_data structure.  However, other than the initialization, that
 *  structure is currently "dumb".
 */

midi_jack::~midi_jack ()
{
    if (not_nullptr(jack_data().m_jack_buffsize))
        ::jack_ringbuffer_free(jack_data().m_jack_buffsize);

    if (not_nullptr(jack_data().m_jack_buffmessage))
        ::jack_ringbuffer_free(jack_data().m_jack_buffmessage);
}

/**
 *  Initialize the MIDI output port.  This initialization is done when the
 *  "manual ports" option is not in force.  This code is basically what was
 *  done by midi_out_jack::open_port() in RtMidi.
 *
 *  For jack_connect(), the first port-name is the source port, and the source
 *  port must include the JackPortIsOutput flag.  The second port-name is the
 *  destination port, and the destination port must include the
 *  JackPortIsInput flag.  For this function, the source/output port is this
 *  port, and the destination/input is....
 *
 *  Note that connect_port() [which calls jack_connect()] cannot usefully be
 *  called until jack_activate() has been called.  We cannot connect ports
 *  until we are activated, and we cannot activate until all ports are
 *  properly set up.
 *
 * \return
 *      Returns true unless setting up JACK MIDI failed in some way.
 */

bool
midi_jack::api_init_out ()
{
    bool result = true;
    std::string remoteportname = connect_name();    /* "bus:port"   */
    remote_port_name(remoteportname);
    result = create_ringbuffer(c_jack_ringbuffer_size);
    if (result)
    {
        set_alt_name(rc().application_name(), rc().app_client_name());
        result = register_port(midibase::io::output, port_name());
    }
    return result;
}

/**
 *  This function is called when we are processing the list of system input
 *  ports. We want to create an output port of a similar name, but with the
 *  application as client, and connect it to this sytem input port.  We want to
 *  follow the model we got from seq24, rather than the RtMidi model, so that we
 *  do not have to rework (and probably break) the seq24 model.
 *
 *  We can't use the API port name here at this time, because it comes up
 *  empty.  It comes up empty because we haven't yet registered the ports,
 *  including the source ports.  So we register it first; connect_port() will
 *  not try to register it again.
 *
 *  Based on the comments in the jack.txt note file, here is what we need to do:
 *
 *      -#  open_client_impl(INPUT).
 *          -#  Get port name via master_info().connect_name().
 *          -#  Call jack_client_open() with or without a UUID.
 *          -#  Call jack_set_process_callback() for input/output.
 *          -#  Call jack_activate().
 *      -#  register_port(INPUT...).  The flag is JackPortIsInput.
 *      -#  connect_port(INPUT...).  Call jack_connect(srcport, destport).
 *
 *  Unlike the corresponding virtual port, this input port is actually an
 *  output port.
 *
 *  Note that we cannot connect ports until we are activated, and we cannot be
 *  activated until all ports are properly set up.  We also need to fill in
 *  the jack_data() member here.
 *
 * \return
 *      Returns true if the function was successful, and sets the flag
 *      indicating that the port is open.
 */

bool
midi_jack::api_init_in ()
{
    std::string remoteportname = connect_name();    /* "bus:port"       */
    remote_port_name(remoteportname);
    set_alt_name(rc().application_name(), rc().app_client_name());
    return register_port(midibase::io::input, port_name());
}

/**
 *  Assumes that the port has already been registered, and that JACK
 *  activation has already occurred.
 *
 * \return
 *      Returns true if all steps of the connection succeeded.
 */

bool
midi_jack::api_connect ()
{
    std::string remotename = remote_port_name();
    std::string localname = connect_name();     /* modified!    */
    bool result;
    if (is_input_port())
        result = connect_port(midibase::io::input, remotename, localname);
    else
        result = connect_port(midibase::io::output, localname, remotename);

    if (result)
        set_port_open();

    return result;
}

/**
 *  Gets information directly from JACK.  The problem this function solves is
 *  that the midibus constructor for a virtual JACK port doesn't not have all
 *  of the information it needs at that point.  Here, we can get this
 *  information and get the actual data we need to rename the port to
 *  something accurate.
 *
 *      const char * pname = jack_port_name(const jack_port_t *);
 *
 * \return
 *      Returns true if all of the information could be obtained.  If false is
 *      returned, then the caller should not use the side-effects.
 *
 * \sideeffect
 *      Passes back the values found.
 */

bool
midi_jack::set_virtual_name (int portid, const std::string & portname)
{
    bool result = not_nullptr(client_handle());
    if (result)
    {
        char * cname = ::jack_get_client_name(client_handle());
        result = not_nullptr(cname);
        if (result)
        {
            std::string clientname = cname;
            set_port_id(portid);
            port_name(portname);
            set_name(rc().app_client_name(), clientname, portname);

#if defined USE_REDUNDANT_SET_NAME_CALL
            parent_bus().set_name
            (
                 rc().app_client_name(), clientname, portname
            );
#endif
        }
    }
    return result;
}

/**
 *  Useful for trouble-shooting and exploration. We find that the port name (of
 *  client:port) is, as expected, the JACK short name. For example,
 *  "fluidsynth-midi midi_00".  The remote name obtained from midibase ::
 *  connect_name() is just the constructed "bus:port" name:
 *  "fulidsynth-midi:midi_00".  For a2jmidi-supplied ports, though, the
 *  port/short and remote names are the same.
 */

std::string
midi_jack::details () const
{
    /*
     * TMI:
     *      std::string result = parent_bus().display_name();
     *      result += ", ";
     *      result += parent_bus().bus_name();
     */

    std::string result = parent_bus().bus_name();
    result += ":";
    result += parent_bus().port_name();
    result += "--->";
    result += m_remote_port_name;
    return result;
}

/**
 *  This initialization is like the "open_virtual_port()" function of the
 *  RtMidi library.
 *
 * \return
 *      Returns true if all steps of the initialization succeeded.
 */

bool
midi_jack::api_init_out_sub ()
{
    master_midi_mode(midibase::io::output);      /* this is necessary */
    int portid = parent_bus().port_id();
    bool result = portid >= 0;
    if (! result)
    {
        portid = bus_index();
        result = portid >= 0;
    }
    if (result)
        result = create_ringbuffer(c_jack_ringbuffer_size);

    if (result)
    {
        std::string portname = parent_bus().port_name();
        if (portname.empty())
        {
            portname = "midi out";
            portname += " ";
            portname += std::to_string(portid);
        }
        result = register_port(midibase::io::output, portname);
        if (result)
        {
            set_virtual_name(portid, portname);
            set_port_open();
        }
    }
    return result;
}

/**
 *  Initializes a virtual/manual input port.
 *
 * \return
 *      Returns true if all steps of the initialization succeeded.
 */

bool
midi_jack::api_init_in_sub ()
{
    master_midi_mode(midibase::io::input);
    int portid = parent_bus().port_id();
    bool result = portid >= 0;
    if (! result)
    {
        portid = bus_index();
        result = portid >= 0;
    }
    if (result)
    {
        std::string portname = master_info().get_port_name(bus_index());
        std::string portname2 = parent_bus().port_name();
        if (portname.empty())
        {
            portname = "midi in";
            portname += " ";
            portname += std::to_string(portid);
        }
        result = register_port(midibase::io::input, portname);
        if (result)
        {
            set_virtual_name(portid, portname);
            set_port_open();
        }
    }
    return result;
}

/**
 *  Commented out.  See the discussion at api_deinit_in().
 */

bool
midi_jack::api_deinit_out ()
{
    /*
     *  Seems, in retrospect, to be ill-advised.
     *
     *      set_port_suspended(true);
     */

    return true;
}

/**
 *  We could define these in the opposite order.
 *
 *  This function is mis-named at this time; it actually leaves the port open
 *  now, and simply disables the reading of MIDI data.  We need to improve on
 *  the life-cycle.  Also, this suspension seems to cause an issue where
 *  the application hangs on exit.
 */

bool
midi_jack::api_deinit_in ()
{
    /*
     *  Do not close.  We still have callbacks responding, and they bitch about
     *  exercising a closed port.
     *
     * close_port();
     *
     *  Seems to cause an application hang upon exit.
     *
     *      set_port_suspended(true);
     */

    return true;
}

/**
 *  We could push the bytes of the event into a midibyte vector, as done in
 *  send_message().  The ALSA code (seq_alsamidi/src/midibus.cpp) sticks the
 *  event bytes in an array, which might be a little faster than using
 *  push_back(), but let's try the vector first.  The rtmidi code here is from
 *  midi_out_jack::send_message().
 */

void
midi_jack::api_play (const event * e24, midibyte channel)
{
    midibyte status = e24->get_status(channel);
    midibyte d0, d1;
    e24->get_data(d0, d1);

    midi_message message;
    message.push(status);
    message.push(d0);
    if (e24->is_two_bytes())
        message.push(d1);

    if (jack_data().valid_buffer())
    {
        if (! send_message(message))
        {
            errprint("JACK Play failed");
        }
    }
}

/**
 *  Sends a JACK MIDI output message.  It writes the full message size and
 *  the message itself to the JACK ring buffer.
 *
 * \param message
 *      Provides the MIDI message object, which contains the bytes to send.
 *
 * \return
 *      Returns true if the buffer message and buffer size seem to be written
 *      correctly.
 */

bool
midi_jack::send_message (const midi_message & message)
{
    int nbytes = message.count();
    bool result = nbytes > 0;
    if (result)
    {
        int count1 = ::jack_ringbuffer_write    /* write the message bytes  */
        (
            jack_data().m_jack_buffmessage, message.array(), nbytes // message.count()
        );
        int count2 = ::jack_ringbuffer_write    /* write raw message size ! */
        (
            jack_data().m_jack_buffsize, (char *) &nbytes, sizeof nbytes
        );
        result = (count1 > 0) && (count2 > 0);
    }
    return result;
}

/**
 *  Work on this routine now in progress.  Unlike the ALSA implementation, we
 *  do not try to send large messages (greater than 255 bytes) in chunks.
 *
 *  The event::sysex data type is a vector of midibytes.  Also note that both
 *  Meta and Sysex messages are covered by the event :: is_ex_data() function
 *  and via the sysex() function to send data.
 *
 *  This SysEx capability is also used in the midicontrolout class to create
 *  an event that contains the MIDI bytes of a control macro, which can then
 *  be sent out via mastermidibus :: sysex().
 */

void
midi_jack::api_sysex (const event * e24)
{
    midi_message message;
    const event::sysex & data = e24->get_sysex();
    int data_size = e24->sysex_size();
    for (int offset = 0; offset < data_size; ++offset)
        message.push(data[offset]);

    if (jack_data().valid_buffer())
    {
        if (! send_message(message))
        {
            errprint("JACK SysEx failed");
        }
    }
}

/**
 *  It seems like JACK doesn't have the concept of flushing events.
 *  The actual function called right now is midi_jack_info::api_flush(), via
 *  rtmidi_info::api_flush(), and it is also an empty function.
 */

void
midi_jack::api_flush ()
{
    // No code needed
}

/**
 *  jack_transport_locate(), jack_transport_reposition(), or something else?
 *  What is used by jack_assistant?
 *
 * \param tick
 *      Provides the tick value to continue from.
 *
 * \param beats
 *      The parameter "beats" is currently unused.
 */

void
midi_jack::api_continue_from (midipulse tick, midipulse /*beats*/)
{
    int beat_width = 4;                                 // no m_beat_width !!!
    int ticks_per_beat = ppqn() * 10;                   // why 10 !!!
    midibpm beats_per_minute = bpm();
    uint64_t tick_rate =
    (
        uint64_t(::jack_get_sample_rate(client_handle())) * tick * 60.0
    );
    long tpb_bpm = long(ticks_per_beat * beats_per_minute * 4.0 / beat_width);
    uint64_t jack_frame = tick_rate / tpb_bpm;
    if (::jack_transport_locate(client_handle(), jack_frame) != 0)
        (void) info_message("JACK Continue failed");

    /*
     * New code to work like the ALSA version, needs testing.  Related to
     * issue #67.  However, the ALSA version sends Continue, flushes, and
     * then sends Song Position, so we will match that here.
     */

    send_byte(EVENT_MIDI_CONTINUE);
    api_flush();                                /* currently does nothing   */
    send_byte(EVENT_MIDI_SONG_POS);
}

/**
 *  Starts this JACK client and sends MIDI start.   Note that the
 *  jack_assistant code (which implements JACK transport) checks if JACK is
 *  running, but a check of the JACK client handle here should be enough.
 */

void
midi_jack::api_start ()
{
    ::jack_transport_start(client_handle());
    send_byte(EVENT_MIDI_START);
}

/**
 *  Stops this JACK client and sends MIDI stop.   Note that the jack_assistant
 *  code (which implements JACK transport) checks if JACK is running, but a
 *  check of the JACK client handle here should be enough.
 */

void
midi_jack::api_stop ()
{
    ::jack_transport_stop(client_handle());
    send_byte(EVENT_MIDI_STOP);
}

/**
 *  Sends a MIDI clock event.
 *
 * \param tick
 *      The value of the tick to use.
 */

void
midi_jack::api_clock (midipulse tick)
{
    if (tick >= 0)
    {
#if defined SEQ66_PLATFORM_DEBUG_TMI
        midibase::show_clock("JACK", tick);
#endif
        send_byte(EVENT_MIDI_CLOCK);
    }
}

/**
 *  An internal helper function for sending MIDI clock bytes.  This function
 *  ought to be called "send_realtime_message()".
 *
 *  Based on the GitHub project "jack_midi_clock", we could try to bypass the
 *  ringbuffer used here and convert the ticks to jack_nframes_t, and use the
 *  following code:
 *
 *      uint8_t * buffer = jack_midi_event_reserve(port_buf, time, 1);
 *      if (buffer)
 *          buffer[0] = rt_msg;
 *
 *  We generally need to send the (realtime) MIDI clock messages Start, Stop,
 *  and Continue if the JACK transport state changed.
 *
 * \param evbyte
 *      Provides one of the following values (though any byte can be sent):
 *
 *          -   EVENT_MIDI_SONG_POS
 *          -   EVENT_MIDI_CLOCK. The tick value is not needed.
 *          -   EVENT_MIDI_START.  The tick value is not needed.
 *          -   EVENT_MIDI_CONTINUE.  The tick value is needed if...
 *          -   EVENT_MIDI_STOP.  The tick value is not needed.
 */

void
midi_jack::send_byte (midibyte evbyte)
{
    midi_message message;
    message.push(evbyte);
    if (jack_data().valid_buffer())
    {
        if (! send_message(message))
        {
            errprint("JACK Send byte failed");
        }
    }
}

/**
 *  Empty body for setting PPQN.
 */

void
midi_jack::api_set_ppqn (int /*ppqn*/)
{
    // No code needed yet
}

/**
 *  Empty body for setting BPM.
 */

void
midi_jack::api_set_beats_per_minute (midibpm /*bpm*/)
{
    // No code needed yet
}

/**
 *  Gets the name of the current port via jack_port_name().  This is different
 *  from get_port_name(index), which simply looks up the port-name in the
 *  attached midi_info object.
 *
 *  This function is never called!
 *
 * \return
 *      Returns the full port name ("clientname:portname") if the port has
 *      already been opened/registered; otherwise an empty string is returned.
 */

std::string
midi_jack::api_get_port_name ()
{
    std::string result;
    if (not_nullptr(port_handle()))
        result = std::string(jack_port_name(port_handle()));

    return result;
}

/**
 *  Closes the JACK client handle.
 *
 *  This function is currently not called!
 */

void
midi_jack::close_client ()
{
    if (not_nullptr(client_handle()))
    {
        int rc = ::jack_client_close(client_handle());
        client_handle(nullptr);
        if (rc != 0)
        {
            int index = bus_index();
            int id = parent_bus().port_id();
            m_error_string = "JACK Close port ";
            m_error_string += std::to_string(index);
            m_error_string += " (id ";
            m_error_string += std::to_string(id);
            m_error_string += ")";
            error(rterror::kind::driver_error, m_error_string);
        }
    }
}

/**
 *  Connects two named JACK ports, but only if they are not virtual/manual
 *  ports.  First, we register the local port.  If this is nominally a local
 *  input port, it is really doing output, and this is the source-port name.
 *  If this is nominally a local output port, it is really accepting input,
 *  and this is the destination-port name.
 *
 * \param input
 *      Indicates true if the port to register and connect is an input port,
 *      and false if the port is an output port.  Useful macros for
 *      readability: midibase::io::input and midibase::io::output.
 *
 * \param srcportname
 *      Provides the destination port-name for the connection.  For input,
 *      this should be the name associated with the JACK client handle; it is
 *      the port that gets registered.  For output, this should be the full
 *      name of the port that was enumerated at start-up.  The JackPortFlags
 *      of the source port must include JackPortIsOutput.
 *
 * \param destportname
 *      For input, this should be full name of port that was enumerated at
 *      start-up.  For output, this should be the name associated with the
 *      JACK client handle; it is the port that gets registered.  The
 *      JackPortFlags of the destination port must include JackPortIsInput.
 *      Now, if this name is empty, that basically means that there is no such
 *      port, and we create a virtual port in this case.  So jack_connect() is
 *      not called in this case.  We do not treat this case as an error.
 *
 * \return
 *      If the jack_connect() call succeeds, true is returned.  If the port is
 *      a virtual (manual) port, then it is not connected, and true is
 *      returned without any action.
 */

bool
midi_jack::connect_port
(
    midibase::io iotype,
    const std::string & srcportname,
    const std::string & destportname
)
{
    bool result = true;
    if (! is_virtual_port())
    {
        result = ! srcportname.empty() && ! destportname.empty();
        if (result)
        {
            int rc = ::jack_connect
            (
                client_handle(), srcportname.c_str(), destportname.c_str()
            );
            result = rc == 0;
            if (! result)
            {
                if (rc == EEXIST)
                {
                    /*
                     * Probably not worth emitting a warning to the console.
                     */
                }
                else
                {
                    bool input = iotype == midibase::io::input;
                    m_error_string = "JACK Connect error";
                    m_error_string += input ? "input '" : "output '";
                    m_error_string += srcportname;
                    m_error_string += "' to '";
                    m_error_string += destportname;
                    m_error_string += "'";
                    error(rterror::kind::driver_error, m_error_string);
                }
            }
        }
    }
    return result;
}

/**
 *  Registers a named JACK port.  We made this function to encapsulate some
 *  otherwise cut-and-paste functionality.
 *
 *  For jack_port_register(), the port-name must be the actual port-name, not
 *  the full-port name ("busname:portname").
 *
 *  Note that the buffer size of non-built-in port type is 0, and so it is
 *  ignored.
 *
 * JackPortIsTerminal:
 *
 *      For an input port, the data received by the port will not be passed on
 *      or made available at any other port.  For an output port, the data
 *      available at the port does not originate from any other port. Audio
 *      synthesizers, I/O hardware interface clients, HDR systems are examples
 *      of clients that would set this flag for their ports.
 *
 * \tricky
 *      If we are registering an input port, this means that we got the input
 *      port from the system.  In order to connect to that port, we have
 *      to register as an output port, even though the application calls it in
 *      input port (midi_in_jack).  Confusing, but the same thing was implicit
 *      in the ALSA implementation, and so we have to apply that same setup
 *      here.
 *
 * \param input
 *      Indicates if the port to register is an input or output port
 *
 * \param portname
 *      Provides the local name of the port.  This is the full name
 *      ("clientname:portname"), but the "portname" part is extracted to fit
 *      the requirements of the jack_port_register() function.  There is an
 *      issue here when a2jmidid is running.  We may see a client name of
 *      "seq66", but a port name of "a2j Midi Through [1] capture: Midi
 *      Through Port-0", which has a colon in it.  What to do?  Just not
 *      extract the port name from the portname parameter.  If we have an
 *      issue here, we'll have to fix it in the caller.
 */

bool
midi_jack::register_port (midibase::io iotype, const std::string & portname)
{
    bool result = not_nullptr(port_handle());
    if (! result)
    {
        unsigned long buffsize = 0;
        unsigned long flag = iotype == midibase::io::input ?
            JackPortIsInput : JackPortIsOutput ;
        const char * porttype = JACK_DEFAULT_MIDI_TYPE;

        /*
         * At least in the version of JACK on our developer laptop,
         * using our own port type string fails. This is contrary
         * to the current JACK documentation for jack_port_register().
         *
         * if (port_type() == midibase::port::manual)
         *      porttype = "virtual midi";
         */

        jack_port_t * p = ::jack_port_register
        (
            client_handle(), portname.c_str(), porttype, flag, buffsize
        );
        if (not_nullptr(p))
        {
            port_handle(p);
            result = true;

#if defined SEQ66_JACK_METADATA_TEST
#if defined USE_METADATA_TEST_1
            (void) set_jack_port_property                   /* it works! */
            (
                client_handle(), p, JACK_METADATA_PRETTY_NAME, "Pretty Name"
            );
#else
            (void) set_jack_port_property                   /* no good! */
            (
                client_handle(), portname, JACK_METADATA_PRETTY_NAME,
                "Pretty Name"
            );
#endif
#endif
            if (rc().investigate())
            {
                std::string longname = portname;
                std::string shortname = ::jack_port_short_name(p);
                if (shortname != portname)
                {
                    longname += " \"";
                    longname += shortname;
                    longname += "\"";
                }
                debug_message("Registered", longname);
            }
        }
        else
        {
            m_error_string = "JACK Register error";
            m_error_string += " ";
            m_error_string += portname;
            error(rterror::kind::driver_error, m_error_string);
        }
    }
    return result;
}

/**
 *  Closes the MIDI port by calling jack_port_unregister() and
 *  nullifying the port pointer.
 *
 *  This function is not called at application exit!
 */

void
midi_jack::close_port ()
{
    if (not_nullptr(client_handle()) && not_nullptr(port_handle()))
    {
        ::jack_port_unregister(client_handle(), port_handle());
        port_handle(nullptr);
    }
}

/**
 *  Creates the JACK ring-buffers.
 */

bool
midi_jack::create_ringbuffer (size_t rbsize)
{
    bool result = rbsize > 0;
    if (result)
    {
        jack_ringbuffer_t * rb = ::jack_ringbuffer_create(rbsize);
        if (not_nullptr(rb))
        {
            jack_data().m_jack_buffsize = rb;
            rb = ::jack_ringbuffer_create(rbsize);
            if (not_nullptr(rb))
                jack_data().m_jack_buffmessage = rb;
            else
                result = false;
        }
        else
            result = false;

        if (! result)
        {
            m_error_string = "JACK Ringbuffer error";
            error(rterror::kind::warning, m_error_string);
        }
    }
    return result;
}

/*
 * MIDI JACK input class.
 */

/**
 *  Principal constructor.  For Seq66, we don't current need to create
 *  a midi_in_jack object; all that is needed is created via the
 *  api_init_in*() functions.  Also, this constructor still needs to do
 *  something with queue size.
 *
 * \param parentbus
 *      Provides the buss object that determines buss-specific parameters of
 *      this class.
 *
 * \param masterinfo
 *      Provides information about the JACK system as found on this machine.
 */

midi_in_jack::midi_in_jack (midibus & parentbus, midi_info & masterinfo)
 :
    midi_jack       (parentbus, masterinfo),
    m_client_name   ()
{
    /*
     * Currently, we cannot initialize here because the clientname is empty.
     * It is retrieved in api_init_in().
     *
     * Hook in the input data.  The JACK port pointer will get set in
     * api_init_in() or api_init_out() when the port is registered.
     */

    jack_data().m_jack_rtmidiin = input_data();
}

/**
 *  Checks the rtmidi_in_data queue for the number of items in the queue.
 *
 * \return
 *      Returns the value of rtindata->queue().count(), unless the caller is
 *      using an rtmidi callback function, in which case 0 is always returned.
 */

int
midi_in_jack::api_poll_for_midi ()
{
    rtmidi_in_data * rtindata = jack_data().m_jack_rtmidiin;
    (void) microsleep(std_sleep_us());
    return rtindata->queue().count();
}

/**
 *  Gets a MIDI event.  This implementation gets a midi_message off the front
 *  of the queue and converts it to a Seq66 event.
 *
 * \change ca 2017-11-04
 *      Issue #4 "Bug with Yamaha PSR in JACK native mode" in the
 *      seq66-packages project has been fixed.  For now, we ignore
 *      system messages.  Yamaha keyboards like my PSS-790 constantly emit
 *      active sensing messages (0xfe) which are not logged, and the previous
 *      event (typically pitch wheel 0xe0 0x0 0x40) is continually emitted.
 *      One result (we think) is odd artifacts in the seqroll when recording
 *      and passing through.
 *
 * \param inev
 *      Provides the destination for the MIDI event.
 *
 * \return
 *      Returns true if a MIDI event was obtained, indicating that the return
 *      parameter can be used.  Note that we force a value of false for all
 *      system messages at this time; they cannot yet be handled gracefully in
 *      the native JACK implementation.
 */

bool
midi_in_jack::api_get_midi_event (event * inev)
{
    rtmidi_in_data * rtindata = jack_data().m_jack_rtmidiin;
    bool result = ! rtindata->queue().empty();
    if (result)
    {
        midi_message mm = rtindata->queue().pop_front();
        result = inev->set_midi_event(mm.timestamp(), mm.data(), mm.count());
        if (result)
        {
            /*
             * For now, ignore certain messages; they're not handled by the
             * perform object.  Could be handled there, but saves some
             * processing time if done here.  Also could move the output code
             * to perform so it is available for frameworks beside JACK.
             */

            midibyte st = mm[0];
            if (st >= EVENT_MIDI_REALTIME && rc().verbose())
            {
                static int s_count = 0;
                char c;
                switch (st)
                {
                case EVENT_MIDI_CLOCK:          c = 'C';    break;
                case EVENT_MIDI_ACTIVE_SENSE:   c = 'S';    break;
                case EVENT_MIDI_RESET:          c = 'R';    break;
                case EVENT_MIDI_START:          c = '>';    break;
                case EVENT_MIDI_CONTINUE:       c = '|';    break;
                case EVENT_MIDI_STOP:           c = '<';    break;
                case EVENT_MIDI_SYSEX:          c = 'X';    break;
                default:                        c = '.';    break;
                }
                (void) putchar(c);
                if (++s_count == 80)
                {
                    s_count = 0;
                    (void) putchar('\n');
                }
                fflush(stdout);
            }
            if (event::is_sense_or_reset(st))
                result = false;

            /*
             * Issue #55 (ignoring the channel).  The status is already set,
             * with channel nybble, above, no need to set it here.
             */
        }
    }
    return result;
}

/**
 *  Destructor.  Currently the base class closes the port, closes the JACK
 *  client, and cleans up the API data structure.
 */

midi_in_jack::~midi_in_jack()
{
    // No code yet
}

/*
 * API: JACK Class Definitions: midi_out_jack
 */

/**
 *  Principal constructor.  For Seq66, we don't current need to create
 *  a midi_out_jack object; all that is needed is created via the
 *  api_init_out*() functions.
 *
 * \param parentbus
 *      Provides the buss object that determines buss-specific parameters of
 *      this class.
 *
 * \param masterinfo
 *      Provides information about the JACK system as found on this machine.
 */

midi_out_jack::midi_out_jack (midibus & parentbus, midi_info & masterinfo) :
    midi_jack       (parentbus, masterinfo)
{
    /*
     * Currently, we cannot initialize here because the clientname is empty.
     * It is retrieved in api_init_out().
     */
}

/**
 *  Destructor.  Currently the base class closes the port, closes the JACK
 *  client, and cleans up the API data structure.
 */

midi_out_jack::~midi_out_jack ()
{
    // No code yet
}

}           // namespace seq66

#endif      // SEQ66_JACK_SUPPORT

/*
 * midi_jack.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


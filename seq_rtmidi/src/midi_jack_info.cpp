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
 * \file          midi_jack_info.cpp
 *
 *    A class for obtaining JACK port information.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2017-01-01
 * \updates       2022-02-27
 * \license       See above.
 *
 *  This class is meant to collect a whole bunch of JACK information about
 *  client number, port numbers, and port names, and hold them for usage when
 *  creating JACK midibus objects and midi_jack API objects.
 */

#include <cstring>                      /* std::strcpy(), std::strcat()     */

#include "seq66-config.h"

#if defined SEQ66_JACK_SUPPORT

#if defined SEQ66_JACK_METADATA
#include <jack/metadata.h>
#include "base64_images.hpp"
#endif

#include "cfg/settings.hpp"             /* seq66::rc() configuration object */
#include "midi/event.hpp"               /* seq66::event and other tokens    */
#include "midi/jack_assistant.hpp"      /* seq66::create_jack_client()      */
#include "midi/midibus_common.hpp"      /* from the libseq66 sub-project    */
#include "midi_jack.hpp"                /* seq66::midi_jack_info            */
#include "midi_jack_data.hpp"           /* seq66::midi_jack_data            */
#include "midi_jack_info.hpp"           /* seq66::midi_jack_info            */
#include "os/timing.hpp"                /* seq66::microsleep()              */
#include "util/basic_macros.hpp"        /* C++ version of easy macros       */
#include "util/calculations.hpp"        /* seq66::extract_port_names()      */
#include "util/strfunctions.hpp"        /* seq66::contains()                */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 * Defined in midi_jack.cpp; used to be static.
 */

extern int jack_process_rtmidi_input (jack_nframes_t nframes, void * arg);
extern int jack_process_rtmidi_output (jack_nframes_t nframes, void * arg);
extern void jack_shutdown_callback (void * arg);

#if defined SEQ66_JACK_PORT_CONNECT_CALLBACK

extern void jack_port_connect_callback
(
    jack_port_id_t a, jack_port_id_t b, int connect, void * arg
);

#endif

extern void jack_port_register_callback
(
    jack_port_id_t port, int ev_value, void * arg
);

/**
 *  Provides a JACK callback function that uses the callbacks defined in the
 *  midi_jack module.  This function calls both the input callback and
 *  the output callback, depending on the port type.  This may lead to
 *  delays, depending on the size of the JACK MIDI buffer.
 *
 * \param nframes
 *      The frame number from the JACK API.
 *
 * \param arg
 *      The putative pointer to the midi_jack_info structure.
 *
 * \return
 *      Always returns 0.
 */

int
jack_process_io (jack_nframes_t nframes, void * arg)
{
    if (nframes > 0)
    {
        midi_jack_info * self = reinterpret_cast<midi_jack_info *>(arg);
        if (not_nullptr(self))
        {
            /*
             * Here we want to go through the I/O ports and route the data
             * appropriately.
             */

            for (auto mj : self->jack_ports())      /* midi_jack pointers   */
            {
                if (mj->enabled())
                {
                    midi_jack_data * mjp = &mj->jack_data();
                    if (mj->parent_bus().is_input_port())
                    {
                        int rc = jack_process_rtmidi_input(nframes, mjp);
                        if (rc == (-1))
                            continue;   /* COMMENTED FOR EXPERIMENT break; */
                    }
                    else
                        (void) jack_process_rtmidi_output(nframes, mjp);
                }
            }
        }
    }
    return 0;
}

/**
 *  Principal constructor.
 *
 * \param appname
 *      Provides the name of the application.
 *
 * \param ppqn
 *      Provides the desired value of the PPQN (pulses per quarter note).
 *
 * \param bpm
 *      Provides the desired value of the BPM (beats per minute).
 */

midi_jack_info::midi_jack_info
(
    const std::string & appname,
    int ppqn,
    midibpm bpm
) :
    midi_info               (appname, ppqn, bpm),
    m_jack_ports            (),
    m_jack_client           (nullptr)               /* inited for connect() */
{
    silence_jack_info();
    m_jack_client = connect();
    if (not_nullptr(m_jack_client))                 /* created by connect() */
    {
        midi_handle(m_jack_client);                 /* void version         */
        client_handle(m_jack_client);               /* jack version         */
    }
}

/**
 *  Destructor.  Deactivates (disconnects and closes) any ports maintained by
 *  the JACK client, then closes the JACK client, shuts down the input
 *  thread, and then cleans up any API resources in use.
 */

midi_jack_info::~midi_jack_info ()
{
    disconnect();
}

/**
 *  Local JACK connection for enumerating the ports.  Note that this name will
 *  be used for normal ports, so we make sure it reflects the application
 *  name.
 *
 *  Note that this function does not call jack_connect().
 *
 *  We need to add a call to jack_on_shutdown() to set up a shutdown callback.
 *  We also need to wait on the activation call until we have registered all
 *  the ports.  Then we (actually the mastermidibus) can call the
 *  api_connect() function to activate this JACK client and connect all the
 *  ports.
 */

jack_client_t *
midi_jack_info::connect ()
{
    jack_client_t * result = m_jack_client;
    if (is_nullptr(result))
    {
        const char * clientname = seq_client_name().c_str();
        result = create_jack_client(clientname);    /* see jack_assistant   */
        if (not_nullptr(result))
        {
            int r = ::jack_set_process_callback(result, jack_process_io, this);
            m_jack_client = result;
            if (r == 0)
            {
                std::string uuid = rc().jack_session();
                if (uuid.empty())
                    uuid = get_jack_client_uuid(result);

                if (! uuid.empty())
                    rc().jack_session(uuid);

                ::jack_on_shutdown
                (
                    m_jack_client, jack_shutdown_callback, (void *) this
                );

#if defined SEQ66_JACK_PORT_CONNECT_CALLBACK
                r = ::jack_set_port_connect_callback
                (
                    m_jack_client, jack_port_connect_callback, (void *) this
                );
                if (r != 0)
                {
                    m_error_string = "JACK cannot set port-connect callback";
                    error(rterror::kind::warning, m_error_string);
                }
#endif
                r = ::jack_set_port_registration_callback
                (
                    m_jack_client, jack_port_register_callback,
                    (void *) this
                );
                if (r != 0)
                {
                    m_error_string =
                        "JACK cannot set port-register callback";

                    error(rterror::kind::warning, m_error_string);
                }

#if defined SEQ66_JACK_METADATA
                std::string n = seq_icon_name();
                bool ok = set_jack_client_property
                (
                    m_jack_client, JACK_METADATA_ICON_NAME, n
                );
                if (ok)
                {
                    debug_message("Set 32x32 icon", n);
                    ok = set_jack_client_property
                    (
                        m_jack_client, JACK_METADATA_ICON_SMALL,
                        qseq66_32x32, "image/png;base64"
                    );
                    if (! ok)
                        error_message("Failed to set 32x32 icon");
                }
                else
                    error_message("Failed to set client icon", n);

                if (ok)
                {
                    debug_message("Set 128x128 icon", n);
                    ok = set_jack_client_property
                    (
                        m_jack_client, JACK_METADATA_ICON_LARGE,
                        qseq66_128x128, "image/png;base64"
                    );
                    if (! ok)
                        error_message("Failed to set 128x128 icon");
                }
#endif
            }
            else
            {
                m_error_string = "JACK cannot set I/O callback";
                error(rterror::kind::warning, m_error_string);
            }
        }
        else
        {
            m_error_string = "JACK server not running";
            error(rterror::kind::warning, m_error_string);
        }
    }
    return result;
}

/**
 *  The opposite of connect().
 */

void
midi_jack_info::disconnect ()
{
    if (not_nullptr(m_jack_client))
    {
        ::jack_deactivate(m_jack_client);
        ::jack_client_close(m_jack_client);
        m_jack_client = nullptr;
    }
}

/**
 *  Extracts the two names from the JACK port-name format,
 *  "clientname:portname".
 */

void
midi_jack_info::extract_names
(
    const std::string & fullname,
    std::string & clientname,
    std::string & portname
)
{
    (void) extract_port_names(fullname, clientname, portname);
}

/**
 *  Gets information on ALL ports, putting input data into one midi_info
 *  container, and putting output data into another midi_info container.
 *
 * \tricky
 *      When we want to connect to a system input port, we want to use an
 *      output port to do that.  When we want to connect to a system output
 *      port, we want to use an input port to do that.  Therefore, we search
 *      for the <i> opposite </i> kind of port.
 *
 *  If there is no system input port, or no system output port, then we add a
 *  virtual port of that type so that the application has something to work
 *  with.
 *
 *  Note that, at some pointer, we ought to consider how to deal with
 *  transitory system JACK clients and ports, and adjust for it.  A kind of
 *  miniature form of session management.  Also, don't forget about the
 *  usefulness of jack_get_port_by_id() and jack_get_port_by_name().
 *
 * Error handling:
 *
 *  Not having any JACK input ports present isn't necessarily an error.  There
 *  may not be any, and there may still be at least one output port.  Also, if
 *  there are none, we try to make a virtual port so that the application has
 *  something to work with.  The only issue is the client number.  Currently
 *  all virtual ports we create have a client number of 0.
 *
 * JackPortIsPhysical:
 *
 *  If this flag is added, then only ports corresponding to a physical device
 *  are get detected and connected.  This might be a useful option to add at a
 *  later date.
 *
 * \return
 *      Returns the total number of ports found.  Note that 0 ports is not
 *      necessarily an error; there may be no JACK apps running with exposed
 *      ports.  If there is no JACK client, then -1 is returned.
 */

int
midi_jack_info::get_all_port_info
(
    midi_port_info & inputports,
    midi_port_info & outputports
)
{
    int result = (-1);
    if (not_nullptr(m_jack_client))
    {
        const char ** inports = ::jack_get_ports  /* list of the JACK ports */
        (
            m_jack_client, NULL,
            JACK_DEFAULT_MIDI_TYPE,
            JackPortIsInput                       /* tricky JACK code       */
        );
        inputports.clear();
        result = 0;
        if (is_nullptr(inports))                  /* check port validity    */
        {
            warnprint("No JACK in-ports; making a virtual port");
            int clientnumber = 0;
            int portnumber = 0;
            std::string clientname = seq_client_name();
            std::string portname = "midi in 0";
            inputports.add
            (
                clientnumber, clientname, portnumber, portname,
                midibase::io::input, midibase::port::manual
            );
            ++result;
        }
        else
        {
            tokenization client_name_list;
            int client = -1;
            int count = 0;
            while (not_nullptr(inports[count]))
            {
                std::string fullname = inports[count];
                std::string clientname;
                std::string portname;
                std::string alias = get_port_alias(fullname);
                if (alias == fullname)
                    alias.clear();

                /*
                 * TODO:  somehow get the 32-bit ID of the port and add it as a
                 * system port ID to use for lookup when detecting newly
                 * registered or unregistered ports.
                 */

                extract_names(fullname, clientname, portname);
                if (client == -1 || clientname != client_name_list.back())
                {
                    client_name_list.push_back(clientname);
                    ++client;
                }
                inputports.add
                (
                    client, clientname, count, portname,
                    midibase::io::input, midibase::port::normal,
                    0, alias
                );
                ++count;
            }
            ::jack_free(inports);
            result += count;
        }

        const char ** outports = ::jack_get_ports  /* list of JACK ports   */
        (
            m_jack_client, NULL,
            JACK_DEFAULT_MIDI_TYPE,
            JackPortIsOutput                       /* tricky   */
        );
        outputports.clear();
        if (is_nullptr(outports))                  /* check port validity  */
        {
            /*
             * Not really an error, though perhaps we want to warn about it.
             * As with the input port, we create a virtual port.
             */

            warnprint("No JACK out-ports, making a virtual port");
            int client = 0;
            std::string clientname = seq_client_name();
            std::string portname = "midi out 0";
            outputports.add
            (
                client, clientname, 0, portname,
                midibase::io::output, midibase::port::manual
            );
            ++result;
        }
        else
        {
            tokenization client_name_list;
            int client = -1;
            int count = 0;
            while (not_nullptr(outports[count]))
            {
                std::string fullname = outports[count];
                std::string clientname;
                std::string portname;
                std::string alias = get_port_alias(fullname);
                if (alias == fullname)
                    alias.clear();

                extract_names(fullname, clientname, portname);
                if (client == -1 || clientname != client_name_list.back())
                {
                    client_name_list.push_back(clientname);
                    ++client;
                }
                outputports.add
                (
                    client, clientname, count, portname,
                    midibase::io::output, midibase::port::normal,
                    0, alias
                );
                ++count;
            }
            ::jack_free(outports);
            result += count;
        }
    }
    return result;
}

/**
 *  If the name includes "system:", try getting an alias instead via
 *  jack_port_by_name() and jack_port_get_aliases().
 *
 *  The cast of the result of malloc() is needed in C++, but not C.
 *
 *  The jack_port_t pointer type is actually an opaque value equivalent to an
 *  integer greater than 1, which is a port index.
 *
 *  Examples of aliases retrieved:
 *
\verbatim
      Out-port: "system:midi_capture_2":

        alsa_pcm:Launchpad-Mini/midi_capture_1
        Launchpad-Mini:midi/capture_1
\endverbatim
 *
 *  and
 *
\verbatim
      In-port "system:midi_playback_2":

        alsa_pcm:Launchpad-Mini/midi_playback_1
        Launchpad-Mini:midi/playback_1
\endverbatim
 *
 *  Ports created by "a2jmidid --export-hw" do not have JACK aliases.  Ports
 *  created by Seq66 do not have JACK aliases.  Ports created by qsynth do not
 *  have JACK aliases.
 *
 * \param name
 *      Provides the name of the port as retrieved by jack_get_ports(). This
 *      name is used to look up the JACK port handle.
 *
 * \return
 *      Returns the alias, if it exists, for a system port.  That is, one with
 *      "system:" in its name.  Brittle.  And some JACK systems do not provide
 *      an alias.
 */

std::string
midi_jack_info::get_port_alias (const std::string & name)
{
    bool is_system_port = contains(name, "system:");        /* brittle code */
    std::string result;
    if (is_system_port)
    {
        jack_port_t * p = ::jack_port_by_name(m_jack_client, name.c_str());
        if (not_NULL(p))
        {
            char * aliases[2];
            const int sz = ::jack_port_name_size();
            aliases[0] = reinterpret_cast<char *>(malloc(sz));  /* alsa_pcm */
            aliases[1] = reinterpret_cast<char *>(malloc(sz));  /* dev name */
            if (is_nullptr_2(aliases[0], aliases[1]))
                return result;                                  /* bug out  */

            aliases[0][0] = aliases[1][0] = 0;                  /* 0 length */
            int rc = ::jack_port_get_aliases(p, aliases);
            if (rc > 1)
            {
                std::string nick = std::string(aliases[1]);     /* brittle  */
                auto colonpos = nick.find_first_of(":");        /* brittle  */
                if (colonpos != std::string::npos)
                    result = nick.substr(0, colonpos);

                /*
                 * Another bit of brittleness:  the name generated via the
                 * a2jmidid program uses spaces, but the system alias returned
                 * by JACK uses a hyphen.  Convert them to spaces.
                 */

                auto hyphenpos = result.find_first_of("-");        /* brittle  */
                while (hyphenpos != std::string::npos)
                {
                    result[hyphenpos] = ' ';
                    hyphenpos = result.find_first_of("-", hyphenpos);
                }
            }
            else
            {
                if (rc < 0)
                    errprint("JACK port aliases error");
                else
                    infoprint("JACK aliases unavailable");
            }
            free(aliases[0]);
            free(aliases[1]);
        }
    }
    return result;
}

/**
 *  Handle port registration and unregistration.  A feature for the future!
 *
 * \param is_my_port
 *      Provides the result of the call to the jack_port_is_mine() function.
 *
 * \param portid
 *      Provides the port number provided to the port-registration callback.
 *
 * \param registration
 *      True if the port is being registered, and false if it is being
 *      unregistered with JACK.
 *
 * \param portname
 *      Provides the name obtained by calling jack_port_short_name().
 */

void
midi_jack_info::update_port_list
(
    bool is_my_port,
    int portid,
    bool registration,
    const std::string & shortname,
    const std::string & longname
)
{
#if defined USE_PORTSMAP_ACTIVE_STATUS
    bool permitted = rc().portmaps_active();
#else
    bool permitted = true;
#endif

    if (permitted)
    {
#if defined SEQ66_MIDI_PORT_REFRESH
        midi_jack * mj = lookup_midi_jack(shortname, longname);
        bool is_new = is_nullptr(mj);
        if (is_new)
        {
            /*
             *  Add the port to this list and to the appropriate busarray.
             *  This begs the question, how to detect input versus output?
             *  If all is established, we can query the midibus associated
             *  this midi_jack_info.
             */

            if (is_my_port)
            {
                /*
                 * Should not do anything.  This port will be added to the
                 * useful ports for Seq66 in the normal course of handling.
                 */
            }
            else
            {
                /*
                 * A new external port.  Add it to the busarray, midi_jack_info
                 * list, and to the portmap as "disabled". Then it can be
                 * checked for later registrations and unregistrations.
                 */
            }
        }
        else
        {
            if (is_my_port)
            {
                /*
                 * The port exists.  If unregistered, simply disable the port in
                 * this list and in the busarray. If registered, make sure the
                 * port status is unchanged.
                 */
            }
            else
            {
                /*
                 * Same as above?
                 */
            }
        }
#endif  // defined SEQ66_MIDI_PORT_REFRESH

    }
    else
    {
        if (! rc().investigate() && ! is_my_port)
        {
            std::string pinfo = shortname;
            pinfo += " #";
            pinfo += std::to_string(portid);
            if (! longname.empty())
            {
                pinfo += "(";
                pinfo += longname;
                pinfo += ")";
            }
            pinfo += " ";
            pinfo += registration ? "registered" : "unregistered";
            pinfo += "; ignored";
            info_message("External port", pinfo);
        }
    }
}

#if defined SEQ66_MIDI_PORT_REFRESH

/**
 *  By this time, the midi_jack::port_name() returns the short name of the
 *  port as registered, so we can look up port_name() here.
 *
 * Important:
 *
 *      At present, the list of midi_jacks contains only the ones that are
 *      enabled!
 */

midi_jack *
midi_jack_info::lookup_midi_jack
(
    const std::string & shortname,
    const std::string & longname
)
{
    midi_jack * result = nullptr;
    if (! shortname.empty())
    {
        for (const auto mj : jack_ports())          /* midi_jack pointers   */
        {
#if defined SEQ66_PLATFORM_DEBUG_TMI
            printf
            (
                "!! Short name:     '%s'\n"
                "   Long name:      '%s'\n"
                "   MJ Port name:   '%s'\n"
                "   MJ Remote name: '%s'\n"
                ,
                shortname.c_str(),
                longname.c_str(),
                mj->port_name().c_str(),
                mj->remote_port_name().c_str()
            );
#endif
            bool match = mj->port_name() == shortname;
            if (! match && ! longname.empty())
                match = mj->port_name() == longname;

            if (match)
            {
                if (rc().investigate())
                    async_safe_strprint(mj->port_name().c_str());

                result = mj;
                break;
            }
        }
    }
    return result;
}

#endif  // defined SEQ66_MIDI_PORT_REFRESH

/**
 *  Useful for trouble-shooting and exploration.
 */

void
midi_jack_info::show_details () const
{
    int count = 0;
    for (const auto mj : jack_ports())              /* midi_jack pointers   */
    {
        std::string d = "Index ";
        d += std::to_string(count);
        d += ": ";
        d += mj->details();
        printf("%s\n", d.c_str());
        ++count;
    }
}

/**
 *  Sets up all of the ports, represented by midibus objects, that have
 *  been created.
 *
 *  The main JACK client is activated, and then all non-virtual ports are
 *  simply connected.
 *
 *  Each JACK port's midi_jack::api_connect() function decides whether or not
 *  to activate before making the connection.
 *
 * Issue #60: GUI option to disable auto midi port creation/connection.
 *
 * \return
 *      Returns true if activation succeeds.
 */

bool
midi_jack_info::api_connect ()
{
    bool result = not_nullptr(client_handle());
    if (result)
    {
        int rc = ::jack_activate(client_handle());
        result = rc == 0;
    }
    if (result && rc().jack_auto_connect())         /* issue #60        */
    {
        for (auto m : bus_container())              /* midibus pointers */
        {
            if (m->is_port_connectable())
            {
                result = m->api_connect();
                if (! result)
                    break;
            }
        }
    }
    if (! result)
    {
        m_error_string = "JACK cannot activate/connect I/O";
        error(rterror::kind::warning, m_error_string);
    }
    return result;
}

/**
 *  Sets the PPQN numeric value, then makes JACK calls to set up the PPQ
 *  tempo.
 *
 * \param p
 *      The desired new PPQN value to set.
 */

void
midi_jack_info::api_set_ppqn (int p)
{
    midi_info::api_set_ppqn(p);
}

/**
 *  Sets the BPM numeric value, then makes JACK calls to set up the BPM
 *  tempo.  These calls might need to be done in a JACK callback.
 *  Why is this called twice?
 *
 * \param b
 *      The desired new BPM value to set.
 */

void
midi_jack_info::api_set_beats_per_minute (midibpm b)
{
    midi_info::api_set_beats_per_minute(b);

    // Need JACK specific tempo-setting here if applicable.
}

/**
 *  Start the given JACK MIDI port.  This function is called by
 *  api_get_midi_event() when an JACK event SND_SEQ_EVENT_PORT_START is
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
 *  midi_jack_info.
 *
 * \param masterbus
 *      Provides the object needed to get access to the array of input and
 *      output buss objects.
 *
 * \param bus
 *      Provides the JACK bus/client number.
 *
 * \param port
 *      Provides the JACK client port.
 */

void
midi_jack_info::api_port_start
(
    mastermidibus & /*masterbus*/, int /*bus*/, int /*port*/
)
{
    // no code
}

/**
 *  We might be able to eliminate this function.
 */

int
midi_jack_info::api_poll_for_midi ()
{
    (void) microsleep(std_sleep_us());
    return 0;
}

/**
 *  Grab a MIDI event.
 *
 * \todo
 *      Return a bussbyte or c_bussbyte_max value instead of a boolean.
 *
 * \param inev
 *      The event to be set based on the found input event.  We should make
 *      this value a reference someday.  Not used here.
 *
 * \return
 *      Always returns false.  Will eventually delete this function.
 */

bool
midi_jack_info::api_get_midi_event (event * /*inev*/)
{
    return false;
}

/**
 *  This function merely eats the string passed as a parameter.
 */

static void
jack_message_bit_bucket (const char *)
{
    // Into the bit-bucket with ye ya scalliwag!
}

/**
 *  This function silences JACK error output to the console.  Probably not
 *  good to silence this output, but let's provide the option, for the sake of
 *  symmetry, consistency, what have you.
 */

void
silence_jack_errors (bool silent)
{
    if (silent)
        ::jack_set_error_function(jack_message_bit_bucket);
}

/**
 *  This function silences JACK info output to the console.  We were getting
 *  way too many informational message, to the point of obscuring the debug
 *  and error output.
 */

void
silence_jack_info (bool silent)
{
    if (silent)
    {
#if ! defined SEQ66_SHOW_API_CALLS
        ::jack_set_info_function(jack_message_bit_bucket);
#endif
    }
}

}           // namespace seq66

#endif      // SEQ66_JACK_SUPPORT

/*
 * midi_jack_info.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


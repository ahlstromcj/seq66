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
 * \file          midi_info.cpp
 *
 *    A class for obrtaining system MIDI information
 *
 * \library       seq66 application
 * \author        Gary P. Scavone; severe refactoring by Chris Ahlstrom
 * \date          2016-12-06
 * \updates       2022-02-21
 * \license       See above.
 *
 * Classes defined:
 *
 *      -   port_info. A structure class holding data about a port.
 *      -   midi_port_info. Holds multiple port_infos and provides accessors.
 *      -   midi_info. Holds input and output midi_portinfos.
 *
 *  These classes are meant to collect a whole bunch of system MIDI information
 *  about client/buss number, port numbers, and port names, and hold it
 *  for usage when creating midibus objects and midi_api objects.
 *
 * Port Refresh Idea:
 *
 *      -#  When midi_info :: get_all_port_info() is called, copy midi_info ::
 *          input_ports() and output_ports() midi_info :: m_previous_input and
 *          midi_info :: m_previous_output.
 *      -#  Detect when a MIDI port registers or unregisters.
 *      -#  Compare the old set of ports to the new set found by
 *          get_all_port_info() to find new ports or missing ports.
 */

#include <sstream>                      /* std::ostringstream               */

#include "cfg/settings.hpp"             /* access to rc() configuration     */
#include "midi/midibus.hpp"             /* select portmidi/rtmidi headers   */
#include "midi_info.hpp"                /* seq66::midi_info etc.            */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 * class midi_port_info
 */

port_info::port_info
(
    int clientnumber,
    const std::string & clientname,
    int portnumber,
    const std::string & portname,
    midibase::io iotype,
    midibase::port porttype,
    int queuenumber,
    const std::string & alias
) :
    m_client_number (clientnumber),
    m_client_name   (clientname),
    m_port_number   (portnumber),
    m_port_name     (portname),
    m_queue_number  (queuenumber),
    m_io_type       (iotype),
    m_port_type     (porttype),
    m_port_alias    (alias),
    m_internal_id   (null_system_port_id())
{
    // No other code
}

/*
 * class midi_port_info
 */

/**
 *  Default constructor.  Instantiated just for visibility.
 */

midi_port_info::midi_port_info () :
    m_port_count        (0),
    m_port_container    ()
{
    // Empty body
}

/**
 *  Adds a set of port information to the port container.
 *
 * \param clientnumber
 *      Provides the client or buss number for the port.  This is a value like
 *
 * \param clientname
 *      Provides the system or user-supplied name for the client or buss.
 *
 * \param portnumber
 *      Provides the port number, usually re 0.
 *
 * \param portname
 *      Provides the system or user-supplied name for the port.
 *
 * \param iotype
 *      Indicates if the port is an input port or an output port.
 *
 * \param porttype
 *      If the system currently has no input or output port available, then we
 *      want to create a virtual port so that the application has something to
 *      work with.  In some systems, we need to create and activate a system
 *      port, such as a timer port or an ALSA announce port.  For all other
 *      ports, this value is note used.
 *
 * \param queuenumber
 *      Provides the optional queue number, if applicable.  For example, the
 *      seq66 application grabs the client number (normally valued at 1)
 *      from the ALSA subsystem.
 *
 * \param alias
 *      In some JACK configurations an alias is available.  This lets one see
 *      the device's model name without running the a2jmidid daemon.
 */

void
midi_port_info::add
(
    int clientnumber,
    const std::string & clientname,
    int portnumber,
    const std::string & portname,
    midibase::io iotype,
    midibase::port porttype,
    int queuenumber,
    const std::string & alias
)
{
    port_info temp
    (
        clientnumber, clientname, portnumber, portname,
        iotype, porttype, queuenumber, alias
    );
    m_port_container.push_back(temp);
    m_port_count = int(m_port_container.size());
    if (rc().verbose() && ! rc().investigate())
    {
        bool makevirtual = porttype == midibase::port::manual;
        bool makesystem = porttype == midibase::port::system;
        bool makeinput =  iotype == midibase::io::input;
        const char * vport = makevirtual ? "virtual" : "auto" ;
        const char * iport = makeinput ? "input" : "output" ;
        const char * sport = makesystem ? "system" : "device" ;
        char tmp[128];
        snprintf
        (
            tmp, sizeof tmp, "\"%s:%s\" %s (%s %s %s)",
            clientname.c_str(), portname.c_str(), alias.c_str(),
            vport, iport, sport
        );
        info_message(tmp);
    }
}

/**
 *  Adds values from a midibus (actually a midibase-derived class).
 */

void
midi_port_info::add (const midibus * m)
{
    add
    (
        m->bus_id(), m->bus_name(),
        m->port_id(), m->port_name(),
        m->io_type(), m->port_type()
    );
}

/**
 *  Retrieve the index of a client:port combination (e.g. in ALSA, the output
 *  of the "aplaymidi -l" or "arecordmidi -l" commands) in the port-container.
 *
 * \param client
 *      Provides the client number.
 *
 * \param port
 *      Provides the port number, the number of a sub-port of the client.
 *
 * \return
 *      Returns the index of the pair in the port container, which will match
 *      up with the listing one sees in the "MIDI Input" or "MIDI Clocks"
 *      pages in the "Preferences" dialog.  If not found, a -1 is returned.
 */

bussbyte
midi_port_info::get_port_index (int client, int port)
{
    bussbyte result = null_buss();
    for (int i = 0; i < m_port_count; ++i)
    {
        if (m_port_container[i].m_client_number != client)
            continue;

        if (m_port_container[i].m_port_number == port)
        {
            result = bussbyte(i);
            break;
        }
    }
    return bussbyte(result);
}

/*
 * class midi_info
 */

/**
 *  Principal constructor.
 */

midi_info::midi_info
(
    const std::string & appname,
    int ppqn,
    midibpm bpm
) :
    m_midi_mode_input   (true),
    m_input             (),                 /* midi_port_info for inputs    */
    m_output            (),                 /* midi_port_info for outputs   */
    m_bus_container     (),
    m_global_queue      (c_bad_id),         /* a la mastermidibase; created */
    m_midi_handle       (nullptr),          /* usually looked up or created */
    m_app_name          (appname),
    m_ppqn              (ppqn),
    m_bpm               (bpm),
    m_midi_port_refresh (false),
    m_error_string      ()
{
    // No code
}

/**
 *  Provides an error handler.  Unlike the midi_api version, it cannot support
 *  an error callback.
 *
 * \throw
 *      If the error is not just a warning, then an rterror object is thrown.
 *
 * \param type
 *      The type of the error.
 *
 * \param errorstring
 *      The error message, which gets copied if this is the first error.
 */

void
midi_info::error (rterror::kind errtype, const std::string & errorstring)
{
    /*
     * Not a big fan of throwing errors, especially since we currently log
     * errors in rtmidi to the console.  Might make this a build option.
     *
     * throw rterror(errorstring, type);
     */

    if (errtype != rterror::kind::max)
        errprint(errorstring);
}

/**
 *  Generates a string listing all of the ports present in the port container.
 *  Useful for debugging and probing.
 *
 * \return
 *      Returns a multi-line ASCII string enumerating all of the ports.
 */

std::string
midi_info::port_list () const
{
    int inportcount = m_input.get_port_count();
    int outportcount = m_output.get_port_count();
    std::ostringstream os;
    midi_info * nc_this = const_cast<midi_info *>(this);

    nc_this->midi_mode(midibase::io::input);
    os << "Input ports (" << inportcount << "):" << std::endl;
    for (int i = 0; i < inportcount; ++i)
    {
        std::string annotation;
        if (nc_this->get_virtual(i))
            annotation = "virtual";
        else if (nc_this->get_system(i))
            annotation = "system";

        os
            << "  [" << i << "] "
            << nc_this->get_bus_id(i) << ":" << nc_this->get_port_id(i) << " "
            << nc_this->get_bus_name(i) << ":" << nc_this->get_port_name(i)
            ;

        if (! annotation.empty())
            os << " (" << annotation << ")";

        os << std::endl;
    }

    nc_this->midi_mode(midibase::io::input);
    os << "Output ports (" << outportcount << "):" << std::endl;
    for (int o = 0; o < outportcount; ++o)
    {
        os
            << "  [" << o << "] "
            << nc_this->get_bus_id(o) << ":" << nc_this->get_port_id(o)
            << " " << nc_this->get_bus_name(o) << ":"
            << nc_this->get_port_name(o)
            << (nc_this->get_virtual(o) ? " (virtual)" : " ")
            << std::endl
            ;
    }
    return os.str();
}

}           // namespace seq66

/*
 * midi_info.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


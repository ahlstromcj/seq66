#if ! defined SEQ66_MIDI_JACK_INFO_HPP
#define SEQ66_MIDI_JACK_INFO_HPP

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
 * \file          midi_jack_info.hpp
 *
 *    A class for holding the current status of the JACK system on the host.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2017-01-01
 * \updates       2022-02-18
 * \license       See above.
 *
 *    We need to have a way to get all of the JACK information of
 *    the midi_jack module.  This module provides that information.
 *
 *  GitHub issue #165: enabled a build and run with no JACK support.
 */

#include "seq66-config.h"

#if defined SEQ66_JACK_SUPPORT

#include <jack/jack.h>                  /* JACK (2) API                     */

#include "mastermidibus_rm.hpp"         /* adds a rtmidi_info "MIDI master" */
#include "midi_info.hpp"                /* seq66::midi_port_info etc.       */
#include "midi/midibus.hpp"             /* seq66::midibus                   */

/*
 * This feature works, but if both the port-connect and port-registration
 * callbacks are defined, they interfere with each other.  We really need to
 * know only that a new port has registered with the system.
 */

#undef  SEQ66_JACK_PORT_CONNECT_CALLBACK

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq66
{
    class mastermidibus;
    class midi_jack;

/**
 *  The class for handling JACK MIDI port enumeration.
 */

class midi_jack_info final : public midi_info
{
    friend class midi_jack;
    friend int jack_process_io (jack_nframes_t nframes, void * arg);

private:

    using portlist = std::vector<midi_jack *>;

    /**
     *  Holds the port data.  This list is iterated in the input and output
     *  portions of the JACK process callback.  This class does not own the
     *  pointers.
     */

    portlist m_jack_ports;

    /**
     *  Holds the JACK sequencer client pointer so that it can be used
     *  by the midibus objects.  This is actually an opaque pointer; there is
     *  no way to get the actual fields in this structure; they can only be
     *  accessed through functions in the JACK API.  Note that it is also
     *  stored as a void pointer in midi_info::m_midi_handle.
     */

    jack_client_t * m_jack_client;

public:

    midi_jack_info () = delete;
    midi_jack_info (const std::string & appname, int ppqn, midibpm bpm);
    virtual ~midi_jack_info ();

    /**
     *  This is the platform-specific version of midi_handle().
     */

    jack_client_t * client_handle ()
    {
        return m_jack_client;
    }

    virtual bool api_get_midi_event (event * inev) override;
    virtual bool api_connect () override;
    virtual int api_poll_for_midi () override;
    virtual void api_set_ppqn (int p) override;
    virtual void api_set_beats_per_minute (midibpm b) override;
    virtual void api_port_start
    (
        mastermidibus & masterbus,
        int bus, int port
    ) override;

    /**
     *  Flushes our local queue events out into JACK.  This is also a
     *  midi_jack function that's never called.  This function is called a
     *  lot.
     */

    virtual void api_flush () override
    {
        // no code
    }

private:


    virtual int get_all_port_info
    (
        midi_port_info & inports,
        midi_port_info & outports
    ) override;

    std::string get_port_alias (const std::string & name);

    /**
     * \getter m_jack_client
     *      This is the platform-specific version of midi_handle().
     */

    void client_handle (jack_client_t * j)
    {
        m_jack_client = j;
    }

    jack_client_t * connect ();
    void disconnect ();
    void extract_names
    (
        const std::string & fullname,
        std::string & clientname,
        std::string & portname
    );

private:

    const portlist & jack_ports () const
    {
        return m_jack_ports;
    }

    portlist & jack_ports ()
    {
        return m_jack_ports;
    }

    /**
     *  Adds a pointer to a JACK port.
     */

    bool add (midi_jack & mj)
    {
        jack_ports().push_back(&mj);
        return true;
    }

};          // midi_jack_info

/*
 * Free functions in the seq66 namespace
 */

void silence_jack_errors (bool silent = true);
void silence_jack_info (bool silent = true);

}           // namespace seq66

#endif      // SEQ66_JACK_SUPPORT

#endif      // SEQ66_MIDI_JACK_INFO_HPP

/*
 * midi_jack_info.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


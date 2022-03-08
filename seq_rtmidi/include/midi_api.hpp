#if ! defined SEQ66_MIDI_API_HPP
#define SEQ66_MIDI_API_HPP

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
 * \file          midi_api.hpp
 *
 *  An abstract base class for realtime MIDI input/output.
 *
 * \library       seq66 application
 * \author        Gary P. Scavone; modifications by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2022-03-08
 * \license       See above.
 *
 *  Declares the following classes:
 *
 *      -   seq66::midi_api
 *      -   seq66::midi_in_api
 *      -   seq66::midi_out_api
 */

#include "midibus_rm.hpp"
#include "rterror.hpp"
#include "rtmidi_types.hpp"             /* SEQ66_NO_INDEX               */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq66
{
    class event;
    class midi_info;

/**
 *  Subclasses of midi_in_api and midi_out_api contain all API- and
 *  OS-specific code necessary to fully implement the rtmidi API.
 *
 *  Note that midi_in_api and midi_out_api are abstract base classes and
 *  cannot be explicitly instantiated.  rtmidi_in and rtmidi_out will
 *  create instances of a midi_in_api or midi_out_api subclass.
 */

class midi_api
{

private:

    /**
     *  Contains information about the ports (system or client) enumerated by
     *  the API. Currently has child classes midi_alsa_info and midi_jack_info.
     */

    midi_info & m_master_info;

    /**
     *  Contains a reference to the parent midibus/midibase object.  This
     *  object is needed to get parameters that are peculiar to the port as it
     *  is actually set up, rather than information from the midi_info object.
     */

    midibus & m_parent_bus;

    /**
     *  Although this really is useful only for MIDI input objects, the split
     *  of the midi_api is not as convenient for re-use as is the split for
     *  derived classes like midi_in_jack/midi_out_jack.
     */

    rtmidi_in_data m_input_data;

    /**
     *  Set to true if the port was opened, activated, and connected without
     *  issue.
     */

    bool m_connected;

protected:

    /**
     *  Holds the last error message, if in force.  This is an original RtMidi
     *  concept.
     */

    std::string m_error_string;

    /**
     *  Holds the error callback function pointer, if any.  This is an
     *  original RtMidi concept.
     */

    rterror_callback m_error_callback;

    /**
     *  Indicates that the first error has happened. This is an original
     *  RtMidi concept.  I have to confess I am not sure how it is/should be
     *  used, yet.
     */

    bool m_first_error_occurred;

    /**
     *  Holds data needed by the error-callback. This is an original RtMidi
     *  concept.  I have to confess I am not sure how it is/should be used,
     *  yet.
     */

    void * m_error_callback_user_data;

public:

    midi_api (midibus & parentbus, midi_info & masterinfo);
    virtual ~midi_api ();

public:

    /**
     *  No code; only midi_jack overrides this function at present.
     */

    virtual bool api_connect ()
    {
        return true;
    }

    virtual int api_poll_for_midi () = 0;
    virtual bool api_init_out () = 0;
    virtual bool api_init_out_sub () = 0;
    virtual bool api_init_in () = 0;
    virtual bool api_init_in_sub () = 0;
    virtual bool api_deinit_out () = 0;
    virtual bool api_deinit_in () = 0;
    virtual bool api_get_midi_event (event *) = 0;
    virtual void api_play (const event * e24, midibyte channel) = 0;
    virtual void api_sysex (const event * e24) = 0;
    virtual void api_continue_from (midipulse tick, midipulse beats) = 0;
    virtual void api_start () = 0;
    virtual void api_stop () = 0;
    virtual void api_flush () = 0;
    virtual void api_clock (midipulse tick) = 0;
    virtual void api_set_ppqn (int ppqn) = 0;
    virtual void api_set_beats_per_minute (midibpm bpm) = 0;

    /*
     * The next two functions are provisional.  Currently useful only in the
     * midi_jack module.
     */

    virtual std::string api_get_bus_name ()
    {
        std::string sm_empty;
        return sm_empty;
    }

    virtual std::string api_get_port_name ()
    {
        std::string sm_empty;
        return sm_empty;
    }

public:

    bool is_port_open () const
    {
        return m_connected;
    }

    midi_info & master_info ()
    {
        return m_master_info;
    }

    const midi_info & master_info () const
    {
        return m_master_info;
    }

    midibus & parent_bus ()
    {
        return m_parent_bus;
    }

    const midibus & parent_bus () const
    {
        return m_parent_bus;
    }

    void master_midi_mode (midibase::io iotype);

    /*
     *  A basic error reporting function for rtmidi classes.
     */

    void error (rterror::kind errtype, const std::string & errorstring);

#if defined SEQ66_USER_CALLBACK_SUPPORT

    /*
     * Moved from the now-removed midi_in_api class.
     */

    void user_callback (rtmidi_callback_t callback, void * userdata);
    void cancel_callback ();

#endif

    /*
     * Pass-alongs to the midibus representing this object's generic data.
     *
     * Ones not passed along at this time:
     *
     *      display_name()
     *      client_id()
     *      set_clock() [useful in disabling a midi_jack/midi_alsa port?]
     */

    bool is_input_port () const
    {
        return parent_bus().is_input_port();
    }

    /**
     *  A virtual port is what Seq24 called a "manual" port.  It is a MIDI
     *  port that an application can create as if it is a real ALSA or
     *  JACK port.
     */

    bool is_virtual_port () const
    {
        return parent_bus().is_virtual_port();
    }

    /**
     *  A system port is one that is independent of the devices and
     *  applications that exist.  In the ALSA subsystem, the only system port
     *  is the "announce" port (and the unused "timer" port).
     */

    bool is_system_port () const
    {
        return parent_bus().is_system_port();
    }

    const std::string & bus_name () const
    {
        return parent_bus().bus_name();
    }

    const std::string & port_name () const
    {
        return parent_bus().port_name();
    }

    const std::string & port_alias () const
    {
        return parent_bus().port_alias();
    }

    midibase::port port_type () const
    {
        return parent_bus().port_type();
    }

    bool enabled () const
    {
        return parent_bus().port_enabled();
    }

    std::string connect_name () const
    {
        return parent_bus().connect_name();
    }

    int bus_index () const
    {
        return parent_bus().bus_index();
    }

    /*
     * The next two functions are useful in debugging.
     */

    int bus_id () const
    {
        return parent_bus().bus_id();
    }

    int port_id () const
    {
        return parent_bus().port_id();
    }

    int ppqn () const
    {
        return parent_bus().ppqn();
    }

    midibpm bpm () const
    {
        return parent_bus().bpm();
    }

protected:

    /*
     * Pass-alongs to the midibus representing this object's generic data.
     */

    void set_client_id (int id)
    {
        parent_bus().set_client_id(id);
    }

    void set_bus_id (int id)
    {
        parent_bus().set_bus_id(id);
    }

    void set_port_id (int id)
    {
        parent_bus().set_port_id(id);
    }

    void bus_name (const std::string & name)
    {
        parent_bus().bus_name(name);
    }

    void port_name (const std::string & name)
    {
        parent_bus().port_name(name);
    }

    /*
     *  Is this ever called?
     */

    void set_name
    (
        const std::string & appname,
        const std::string & busname,
        const std::string & portname
    )
    {
        parent_bus().set_name(appname, busname, portname);
    }

    void set_alt_name
    (
        const std::string & appname,
        const std::string & busname
    )
    {
        parent_bus().set_alt_name(appname, busname);
    }

    /*
     * API-related functions.
     */

    void set_port_open ()
    {
        m_connected = true;
    }

    rtmidi_in_data * input_data ()
    {
        return &m_input_data;
    }

};          // class midi_api

}           // namespace seq66

#endif      // SEQ66_MIDI_API_HPP

/*
 * midi_api.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


#if ! defined SEQ66_RTMIDI_HPP
#define SEQ66_RTMIDI_HPP

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
 * \file          rtmidi.hpp
 *
 *  An abstract base class for realtime MIDI input/output.
 *
 * \library       seq66 application
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2022-03-13
 * \license       See above.
 *
 *  The big difference between this class (seq66::rtmidi) and
 *  seq66::rtmidi_info is that it gets information via midi_api-derived
 *  functions, while the latter gets if via midi_api_info-derived functions.
 */

#include <string>

#include "seq66_rtmidi_features.h"          /* defines what's implemented   */
#include "midi_api.hpp"                     /* seq66::midi[_in][_out]_api   */
#include "rtmidi_types.hpp"                 /* seq66::rtmidi_api etc.       */
#include "rtmidi_info.hpp"                  /* seq66::rtmidi_info           */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  The main class of the rtmidi API.  We moved the enum Api definition into
 *  the new rtmidi_types.hpp module to make refactoring the code easier.
 */

class rtmidi : public midi_api
{
    friend class midibus;

private:

    /**
     *  Holds a reference to the "global" midi_info wrapper object.
     *  Unlike the original RtMidi library, this library separates the
     *  port-enumeration code ("info") from the port-usage code ("api").
     *
     *  We might make it a static object at some point.
     */

    rtmidi_info & m_rtmidi_info;

    /**
     *  Points to the API I/O object (e.g. midi_alsa or midi_jack) for which
     *  this class is a wrapper.
     */

    midi_api * m_midi_api;

protected:

    rtmidi (midibus & parentbus, rtmidi_info & info);
    virtual ~rtmidi ();

public:

    virtual bool api_connect () override
    {
        return get_api()->api_connect();
    }

    virtual void api_play (const event * e24, midibyte channel) override
    {
        get_api()->api_play(e24, channel);
    }

    virtual void api_continue_from (midipulse tick, midipulse beats) override
    {
        get_api()->api_continue_from(tick, beats);
    }

    virtual void api_start () override
    {
        get_api()->api_start();
    }

    virtual void api_stop () override
    {
        get_api()->api_stop();
    }

    virtual void api_clock (midipulse tick) override
    {
        get_api()->api_clock(tick);
    }

    virtual void api_set_ppqn (int ppqn) override
    {
        get_api()->api_set_ppqn(ppqn);
    }

    virtual void api_set_beats_per_minute (midibpm bpm) override
    {
        get_api()->api_set_beats_per_minute(bpm);
    }

    virtual bool api_init_out () override
    {
        return get_api()->api_init_out();
    }

    virtual bool api_init_out_sub () override
    {
        return get_api()->api_init_out_sub();
    }

    virtual bool api_init_in () override
    {
        return get_api()->api_init_in();
    }

    virtual bool api_init_in_sub () override
    {
        return get_api()->api_init_in_sub();
    }

    virtual bool api_deinit_out () override
    {
        return get_api()->api_deinit_out();
    }

    virtual bool api_deinit_in () override
    {
        return get_api()->api_deinit_in();
    }

    virtual bool api_get_midi_event (event * inev) override
    {
        return get_api()->api_get_midi_event(inev);
    }

    virtual int api_poll_for_midi () override
    {
        return get_api()->api_poll_for_midi();
    }

    virtual void api_sysex (const event * e24) override
    {
        get_api()->api_sysex(e24);
    }

    virtual void api_flush () override
    {
        get_api()->api_flush();
    }

public:

    /**
     *  Returns true if a port is open and false if not.
     */

    virtual bool is_port_open () const
    {
       return get_api()->is_port_open();
    }

    virtual std::string get_port_name ()
    {
        return parent_bus().port_name();    /* get_api()->port_name()   */
    }

    virtual std::string get_port_alias ()
    {
        return parent_bus().port_name();    /* get_api()->port_alias()  */
    }

    int get_port_count ()
    {
        return m_rtmidi_info.get_port_count();
    }

    /**
     *  \return
     *      Returns the sum of the number of input and output ports.
     */

    int full_port_count ()
    {
        return m_rtmidi_info.full_port_count();
    }

    const midi_api * get_api () const
    {
        return m_midi_api;
    }

    midi_api * get_api ()
    {
        return m_midi_api;
    }

    /*
     * Pass-alongs to the parent bus for this midi_api-derived object.
     * More are already defined above, as well.
     */

    void set_bus_id (int id)
    {
        parent_bus().set_bus_id(id);
    }

    void set_port_id (int id)
    {
        parent_bus().set_port_id(id);
    }

    std::string connect_name () const
    {
        return parent_bus().connect_name();
    }

protected:

    void set_api (midi_api * ma)
    {
        if (not_nullptr(ma))
            m_midi_api = ma;
    }

    void delete_api ()
    {
        if (not_nullptr(m_midi_api))
        {
            delete m_midi_api;
            m_midi_api = nullptr;
        }
    }

};          // class rtmidi

/**
 *  A realtime MIDI input class.  This class provides a common,
 *  platform-independent API for realtime MIDI input.  It allows access to a
 *  single MIDI input port.  Incoming MIDI messages are either saved to a
 *  queue for retrieval using the get_message() function or immediately passed
 *  to a user-specified callback function.  Create multiple instances of this
 *  class to connect to more than one MIDI device at the same time.  With the
 *  OS-X, Linux ALSA, and JACK MIDI APIs, it is also possible to open a
 *  virtual input port to which other MIDI software clients can connect.
 */

class rtmidi_in : public rtmidi
{

public:

    rtmidi_in (midibus & parentbus, rtmidi_info & info);
    virtual ~rtmidi_in ();

protected:

    void openmidi_api (rtmidi_api api, rtmidi_info & info);

};

/**
 *  A realtime MIDI output class.
 *
 *  This class provides a common, platform-independent API for MIDI output.
 *  It allows one to probe available MIDI output ports, to connect to one such
 *  port, and to send MIDI bytes immediately over the connection.  Create
 *  multiple instances of this class to connect to more than one MIDI device
 *  at the same time.  With the OS-X, Linux ALSA and JACK MIDI APIs, it is
 *  also possible to open a virtual port to which other MIDI software clients
 *  can connect.
 */

class rtmidi_out : public rtmidi
{

public:

    rtmidi_out (midibus & parentbus, rtmidi_info & info);

    /**
     *  The destructor closes any open MIDI connections.
     */

    virtual ~rtmidi_out ();

protected:

    void openmidi_api
    (
        rtmidi_api api, rtmidi_info & info // , int index = SEQ66_NO_INDEX
    );

};          // class rtmidi_out

}           // namespace seq66

#endif      // SEQ66_RTMIDI_HPP

/*
 * rtmidi.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


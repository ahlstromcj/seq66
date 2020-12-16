#if ! defined SEQ66_RTMIDI_HPP
#define SEQ66_RTMIDI_HPP

/**
 * \file          rtmidi.hpp
 *
 *  An abstract base class for realtime MIDI input/output.
 *
 * \library       seq66 application
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2020-12-16
 * \license       See the rtexmidi.lic file.  Too big for a header file.
 *
 *  The big difference between this class (seq66::rtmidi) and
 *  seq66::rtmidi_info is that it gets information via midi_api-derived
 *  functions, while the latter gets if via midi_api_info-derived functions.
 */

#include <string>

#include "seq66_rtmidi_features.h"          /* defines what's implemented   */
#include "midi_api.hpp"                     /* seq66::midi[_in][_out]_api   */
#include "rterror.hpp"                      /* seq66::rterror               */
#include "rtmidi_types.hpp"                 /* seq66::rtmidi_api etc.       */
#include "rtmidi_info.hpp"                  /* seq66::rtmidi_info           */
#include "util/basic_macros.hpp"            /* platform macros for compiler */

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

    rtmidi_info & m_midi_info;

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

    virtual void api_play (event * e24, midibyte channel) override
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

    virtual void api_sysex (event * e24) override
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

    /**
     *  Gets the buss/client ID for a MIDI interfaces.  This is the left-hand
     *  side of a X:Y pair (such as 128:0).
     *
     *  This function is a new part of the RtMidi interface.
     *
     * \return
     *      Returns the buss/client value as provided by the selected API.
     */

    virtual int get_bus_id () const
    {
        return parent_bus().bus_id();       /* get_api()->bus_id()      */
    }

    /**
     * \return
     *      Returns the buss name from the selected API subsystem.
     */

    virtual std::string bus_name () const
    {
        return parent_bus().bus_name();     /* get_api()->bus_name()    */
    }

    /**
     * \return
     *      Returns the port ID number from the selected API subsystem.
     */

    virtual int port_id () const
    {
        return parent_bus().port_id();      /* get_api()->port_id()     */
    }

    /**
     * \return
     *      Returns the port name from the selected API subsystem.
     */

    virtual std::string get_port_name ()
    {
        return parent_bus().port_name();    /* get_api()->port_name()   */
    }

    /**
     *  \return
     *      This value depends on the MIDI mode setting (input versus output).
     */

    int get_port_count ()
    {
        return m_midi_info.get_port_count();
    }

    /**
     *  \return
     *      This value is the sum of the number of input and output ports.
     */

    int full_port_count ()
    {
        return m_midi_info.full_port_count();
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
 *  A realtime MIDI input class.
 *
 *  This class provides a common, platform-independent API for realtime MIDI
 *  input.  It allows access to a single MIDI input port.  Incoming MIDI
 *  messages are either saved to a queue for retrieval using the get_message()
 *  function or immediately passed to a user-specified callback function.
 *  Create multiple instances of this class to connect to more than one MIDI
 *  device at the same time.  With the OS-X, Linux ALSA, and JACK MIDI APIs,
 *  it is also possible to open a virtual input port to which other MIDI
 *  software clients can connect.
 */

class rtmidi_in : public rtmidi
{

public:

    rtmidi_in (midibus & parentbus, rtmidi_info & info);
    virtual ~rtmidi_in ();

#if defined SEQ66_USER_CALLBACK_SUPPORT

    /**
     *  Set a callback function to be invoked for incoming MIDI messages.
     *
     *  The callback function will be called whenever an incoming MIDI
     *  message is received.  While not absolutely necessary, it is best
     *  to set the callback function before opening a MIDI port to avoid
     *  leaving some messages in the queue.
     *
     * \param callback
     *      A callback function must be given.
     *
     * \param userdata
     *      Optionally, a pointer to additional data can be passed to the
     *      callback function whenever it is called.
     */

    void user_callback (rtmidi_callback_t callback, void * userdata = nullptr)
    {
       dynamic_cast<midi_api *>(get_api())->user_callback(callback, userdata);
    }

    /**
     *  Cancel use of the current callback function (if one exists).
     *
     *  Subsequent incoming MIDI messages will be written to the queue
     *  and can be retrieved with the \e get_message function.
     */

    void cancel_callback ()
    {
       dynamic_cast<midi_api *>(get_api())->cancel_callback();
    }

#endif

protected:

    void openmidi_api
    (
        rtmidi_api api, rtmidi_info & info //, int index = SEQ66_NO_INDEX
    );

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


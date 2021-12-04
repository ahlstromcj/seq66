#if ! defined SEQ66_MIDIBASE_HPP
#define SEQ66_MIDIBASE_HPP

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
 * \file          midibase.hpp
 *
 *  This module declares/defines the base class for MIDI I/O under Linux.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2016-11-24
 * \updates       2021-12-04
 * \license       GNU GPLv2 or above
 *
 *  The midibase module is the new base class for the various implementations
 *  of the midibus module.  There is enough commonality to be worth creating a
 *  base class for all such classes.
 */

#include "midi/midibus_common.hpp"      /* values and e_clock enumeration   */
#include "midi/midibytes.hpp"           /* seq66::midibyte alias            */
#include "util/automutex.hpp"           /* seq66::recmutex recursive mutex  */
#include "util/basic_macros.h"          /* not_nullptr() macro              */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class event;

/**
 *  This class implements with ALSA version of the midibase object.
 */

class midibase
{
    /**
     *  The master MIDI bus sets up the buss.
     */

    friend class mastermidibus;

public:

    /**
     *  Constants for selecting input versus output ports in a more obvious
     *  way.  These items are needed for the midi_mode() setter function.
     *  Note that midi_mode() has no functionality in the midi_api base class,
     *  which has a number of such stub functions so that we can use the
     *  midi_info and midi_api derived classes.  Tested by the is_input_port()
     *  functions.
     */

static const bool c_output_port  = false;   /* the MIDI mode is not input   */
static const bool c_input_port   = true;    /* the MIDI mode is input       */

    /**
     *  Constants for selecting virtual versus normal ports in a more obvious
     *  way.  Used in the rtmidi midibus constructors.  Tested by the
     *  is_virtual_port() functions.
     */

static const bool c_normal_port  = false;   /* the MIDI port is not virtual */
static const bool c_virtual_port = true;    /* the MIDI port is virtual     */

    /**
     *  Constants for indicating if the port is a built-in system port versus
     *  a port that exists because a MIDI device is plugged in or some
     *  application has set up a virtual port.  Tested by the is_system_port()
     *  functions.
     */

static const bool c_system_port  = true;    /* API always exposes this port */

private:

    /**
     *  This is another name for "16 * 4".
     */

    static int m_clock_mod;

    /**
     *  Provides the index of the midibase object in either the input list or
     *  the output list.  Otherwise, it is currently -1.
     */

    const int m_bus_index;

    /**
     *  The buss ID of the Seq66 application as determined by the ALSA
     *  subsystem.  It is set in the midi_alsa constructor.
     *
     *  For JACK, this is currently set to the same value as the buss ID.
     */

    int m_client_id;

    /**
     *  The buss ID of the midibase object.  For example, on one system the
     *  IDs are 14 (MIDI Through), 128 (TiMidity), and 129 (Yoshimi).
     */

    int m_bus_id;

    /**
     *  The port ID of the midibase object.
     */

    int m_port_id;

    /**
     *  The type of clock to use.  The special value e_clock::disabled means
     *  we will not be using the port, so that a failure in setting up the
     *  port is not a "fatal error".  (We could have added an "m_outputing"
     *  boolean as an alternative.)
     */

    e_clock m_clock_type;

    /**
     *  This flag indicates if an input bus has been selected for action as an
     *  input device (such as a MIDI controller).  It is turned on if the user
     *  selects the port in the Options / MIDI Input tab.
     */

    bool m_inputing;

    /**
     *  Provides the PPQN value in force, currently a constant.
     *  Some APIs can control or use this value.
     */

    int m_ppqn;

    /**
     *  Provides the PPQN value in force, currently a constant.
     *  Some APIs can control or use this value.
     */

    midibpm m_bpm;

    /**
     *  Another ID of the MIDI queue?  This is an implementation-dependent
     *  value.  For ALSA, it is the ALSA queue number.  For PortMidi, this is
     *  the old "m_pm_num" value.  For RtMidi, it is not currently used.
     */

    int m_queue;

    /**
     *  Holds the full display name of the bus, index, ID numbers, and item
     *  names.  Assembled by the set_name() function.
     */

    std::string m_display_name;

    /**
     *  The name of the MIDI buss.  This should be something like a major device
     *  name or the name of a subsystem such as Timidity.
     */

    std::string m_bus_name;

    /**
     *  The name of the MIDI port.  This should be the name of a specific device
     *  or port on a major device.
     */

    std::string m_port_name;

    /**
     *  The last (most recent? final?) tick.
     */

    midipulse m_lasttick;

    /**
     *  Indicates if the port is to be a virtual port.  The default is to
     *  create a system port (true).
     */

    bool m_is_virtual_port;

    /**
     *  Indicates if the port is to be an input (versus output) port.
     *  It matters when we are creating the name of the port, where we don't
     *  want an input virtual port to have the same name as an output virtual
     *  port... one of them will fail.
     */

    bool m_is_input_port;

    /**
     *  Indicates if the port is a system port.  Two examples are the ALSA
     *  System Timer buss and the ALSA System Announce bus, the latter being
     *  necessary for input subscription and notification.  For most ports,
     *  this value will be false.  A restricted setter is provided.
     *  Only the rtmidi ALSA implementation sets this flag.
     */

    bool m_is_system_port;

    /**
     *  Locking mutex. This one is based on std:::recursive_mutex.
     */

    recmutex m_mutex;

public:

    midibase
    (
        const std::string & appname,        /* usually the app name         */
        const std::string & busname,        /* subsystem name               */
        const std::string & portname,
        int index,                          /* 0, a display ordinal         */
        int bus_id,                         /* null_buss()                  */
        int port_id,                        /* bad_id()                     */
        int queue,                          /* bad_id()                     */
        int ppqn,                           /* use_default_ppqn             */
        midibpm bpm,                        /* default_bpm                  */
        bool makevirtual,                   /* false                        */
        bool isinput,                       /* false                        */
        bool makesystem                     /* false                        */
    );

    virtual ~midibase ();

    void show_bus_values ();

    static void show_clock (const std::string & context, midipulse tick);

    const std::string & display_name () const
    {
        return m_display_name;
    }

    const std::string & bus_name () const
    {
        return m_bus_name;
    }

    const std::string & port_name () const
    {
        return m_port_name;
    }

    std::string connect_name () const;

    int bus_index () const
    {
        return m_bus_index;
    }

    int client_id () const
    {
        return m_client_id;
    }

    int bus_id () const
    {
        return m_bus_id;
    }

    int port_id () const
    {
        return m_port_id;
    }

    int ppqn () const
    {
        return m_ppqn;
    }

    midibpm bpm () const
    {
        return m_bpm;
    }

    /**
     *  Checks if the given parameters match the current bus and port numbers.
     */

    bool match (int bus, int port)
    {
        return (m_port_id == port) && (m_bus_id == bus);
    }

    bool is_virtual_port () const
    {
        return m_is_virtual_port;
    }

    /**
     * \setter m_is_virtual_port
     *      This function is needed in the rtmidi library to set the
     *      is-virtual flag in the api_init_*_sub() functions, so that
     *      midi_alsa, midi_jack (and any other additional APIs that end up
     *      supported by our heavily-refactored rtmidi library), as well as
     *      the original midibus, can know that they represent a virtual port.
     */

    void is_virtual_port (bool flag)
    {
        m_is_virtual_port = flag;
    }

    bool is_input_port () const
    {
        return m_is_input_port;
    }

    bool is_output_port () const
    {
        return ! m_is_input_port;
    }

    void is_input_port (bool flag)
    {
        m_is_input_port = flag;
    }

    bool is_system_port () const
    {
        return m_is_system_port;
    }

    bool is_port_connectable () const
    {
        return ! port_disabled() && ! is_virtual_port();
    }

    void set_system_port_flag ()
    {
        m_is_system_port = true;
    }

    bool set_clock (e_clock clocktype);

    e_clock get_clock () const
    {
        return m_clock_type;
    }

    bool port_disabled () const
    {
        return m_clock_type == e_clock::disabled;
    }

    bool clock_enabled () const
    {
        return m_clock_type != e_clock::off && m_clock_type != e_clock::disabled;
    }

    bool get_input () const
    {
        return m_inputing;
    }

    void set_input_status (bool flag)
    {
        m_inputing = flag;
    }

    int queue_number () const
    {
        return m_queue;
    }

    /**
     *  Useful for setting the buss ID when using the rtmidi_info object to
     *  create a list of busses and ports.  Would be protected, but midi_alsa
     *  needs to change this value to reflect the user-client ID actually
     *  assigned by ALSA.  (That value ranges from 128 to 191.)
     */

    void set_bus_id (int id)
    {
        m_bus_id = id;
    }

    void set_client_id (int id)
    {
        m_client_id = id;
    }

    void set_name
    (
        const std::string & appname,
        const std::string & busname,
        const std::string & portname
    );
    void set_alt_name
    (
        const std::string & appname,
        const std::string & busname,
        const std::string & portname
    );
    void set_multi_name
    (
        const std::string & appname,
        const std::string & localbusname,
        const std::string & remoteportname
    );

    /**
     *  Set the clock mod to the given value, if legal.
     *
     * \param clockmod
     *      If this value is not equal to 0, it is used to set the static
     *      member m_clock_mod.
     */

    static void set_clock_mod (int clockmod)
    {
        if (clockmod != 0)
            m_clock_mod = clockmod;
    }

    /**
     *  Get the clock mod value.
     */

    static int get_clock_mod ()
    {
        return m_clock_mod;
    }

    int poll_for_midi ();
    bool get_midi_event (event * inev);
    bool init_out ();
    bool init_in ();
    bool deinit_out ();
    bool deinit_in ();
    bool init_out_sub ();
    bool init_in_sub ();
    void play (const event * e24, midibyte channel);
    void sysex (const event * e24);
    void flush ();
    void start ();
    void stop ();
    void clock (midipulse tick);
    void continue_from (midipulse tick);
    void init_clock (midipulse tick);
    void print ();
    bool set_input (bool inputing);

public:             // protected: public for seq_rtmidi midi_api

    void display_name (const std::string & name)
    {
        m_display_name = name;
    }

    void bus_name (const std::string & name)
    {
        m_bus_name = name;
    }

    void port_name (const std::string & name)
    {
        m_port_name = name;
    }

    /**
     * \setter m_port_id
     *      Useful for setting the port ID when using the rtmidi_info object
     *      to inspect and create a list of busses and ports.
     */

    void set_port_id (int id)
    {
        m_port_id = id;
    }

    /**
     *  Now defined in the ALSA implementation, and used by mastermidibus.
     *  Also used in the JACK implementation.
     */

    virtual int api_poll_for_midi ()
    {
        return 0;
    }

    /**
     *  Used in the JACK implementation.
     */

    virtual bool api_get_midi_event (event * inev)
    {
        return not_nullptr(inev);
    }

    /**
     *  Not defined in the PortMidi implementation.
     */

    virtual bool api_init_in_sub ()
    {
        return false;                   /* no code for portmidi */
    }

    /**
     *  Not defined in the PortMidi implementation.
     */

    virtual bool api_init_out_sub ()
    {
        return false;                   /* no code for portmidi */
    }

    /**
     *  Not defined in the PortMidi implementation.
     */

    virtual bool api_deinit_out ()
    {
        return false;
    }

    virtual bool api_deinit_in ()
    {
        return false;
    }

    virtual void api_play (const event * e24, midibyte channel) = 0;

    /**
     *  Handles implementation details for SysEx messages.
     *
     *  The \a e24 parameter, the SysEx event pointer, is unused here.
     */

    virtual void api_sysex (const event * /* e24 */)
    {
        // no code for portmidi
    }

    /**
     *  Handles implementation details for the flush() function.
     */

    virtual void api_flush ()
    {
        // no code for portmidi
    }

protected:

    virtual bool api_init_in () = 0;
    virtual bool api_init_out () = 0;
    virtual void api_continue_from (midipulse tick, midipulse beats) = 0;
    virtual void api_start () = 0;
    virtual void api_stop () = 0;
    virtual void api_clock (midipulse tick) = 0;

};          // class midibase

}           // namespace seq66

#endif      // SEQ66_MIDIBASE_HPP

/*
 * midibase.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


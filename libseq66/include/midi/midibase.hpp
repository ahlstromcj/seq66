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
 * \updates       2023-05-14
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

#undef  SEQ66_SHOW_BUS_VALUES           /* ca 2024-06-04 for investigation  */

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
     *
     *  We could add "max" to this enumeration.
     */

    enum class io
    {
        input,          /**< The port is an input MIDI port.                */
        output,         /**< The port is an output MIDI port.               */
        indeterminate   /**< Cannot determine the type of the port.         */
    };

    /**
     *  Constants for selecting virtual versus normal versus built-in system
     *  ports.  Used in the rtmidi midibus constructors.  Tested by the
     *  is_virtual_port() and is_system_port() functions.
     *
     *  We could add "indeterminate" and "max" to this enumeration.
     */

    enum class port
    {
        normal,         /**< Able to be automatically connected.            */
        manual,         /**< A virtual port (virtual is a keyword, though). */
        system          /**< A system port (ALSA only).                     */
    };

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
     *  subsystem.  It is set in the midi_alsa constructor. If there are
     *  no other MIDI *applications* running this value will end up being
     *  129.
     *
     *  For JACK, this is currently set to the same value as the buss ID of
     *  Seq66.
     */

    int m_client_id;

    /**
     *  The buss ID of the midibase object represents *other* MIDI devices and
     *  applications (besides Seq66) present at Seq66 startup.  For example,
     *  on one system the IDs are 14 (MIDI Through), 20 (LaunchPad Mini), 128
     *  (TiMidity), and 129 (Yoshimi).
     */

    int m_bus_id;

    /**
     *  The port ID of the midibase object. Numbering startes at 0.
     */

    int m_port_id;

    /**
     *  The type of clock to use.  The special value e_clock::disabled means
     *  we will not be using the port, so that a failure in setting up the
     *  port is not a "fatal error".  We could have added an "m_outputing"
     *  boolean as an alternative. However, we can overload m_inputing instead.
     */

    e_clock m_clock_type;

    /**
     *  This flag indicates if an input or output bus has been selected for
     *  action as an input device (such as a MIDI controller).  It is turned
     *  on if the user selects the port in the Options / MIDI Input tab.
     */

    bool m_io_active;

    /**
     *   Indicates if the port is unavailable. For example, when the Windows
     *   MIDI Mapper grabs the GS wave-table synthesizer.  The port is not
     *   just disabled... it cannot be enabled.
     */

    bool m_unavailable;

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
     *  or port on a major device.  This value, for JACK is reconstructed by
     *  set_alt_name() so that it is essentially the "short" port name that JACK
     *  recognizes.
     */

    std::string m_port_name;

    /**
     *  The alias of the MIDI port.  This item is specific to JACK, and is
     *  empty for other APIs.
     */

    std::string m_port_alias;

    /**
     *  The last (most recent? final?) tick.
     */

    midipulse m_lasttick;

    /**
     *  Indicates if the port is to be an input (versus output) port.
     *  It matters when we are creating the name of the port, where we don't
     *  want an input virtual port to have the same name as an output virtual
     *  port... one of them will fail.
     */

    io m_io_type;

    /**
     *  Indicates if the port is a system port.  Two examples are the ALSA
     *  System Timer buss and the ALSA System Announce bus, the latter being
     *  necessary for input subscription and notification.  For most ports,
     *  this value will be port::normal.  A restricted setter is provided.
     *  Only the rtmidi ALSA implementation sets up system ports.
     */

    port m_port_type;

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
        io iotype,
        port porttype,
        const std::string & portalias = ""  /* JACK only                    */
    );

    virtual ~midibase ();

#if defined SEQ66_SHOW_BUS_VALUES
    void show_bus_values ();
#endif

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

    const std::string & port_alias () const
    {
        return m_port_alias;
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

    port port_type () const
    {
        return m_port_type;
    }

    bool is_virtual_port () const
    {
        return m_port_type == port::manual;
    }

    /**
     *  This function is needed in the rtmidi library to set the is-virtual
     *  flag in the api_init_*_sub() functions, so that midi_alsa, midi_jack
     *  (and any other additional APIs that end up supported by our
     *  heavily-refactored rtmidi library), as well as the original midibus,
     *  can know that they represent a virtual port.
     */

    void is_virtual_port (bool flag)
    {
        if (! is_system_port())
            m_port_type = flag ? port::manual : port::normal;
    }

    io io_type () const
    {
        return m_io_type;
    }

    bool is_input_port () const
    {
        return m_io_type == io::input;
    }

    bool is_output_port () const
    {
        return m_io_type == io::output;
    }

    void is_input_port (bool flag)
    {
        m_io_type = flag ? io::input : io::output ;
    }

    bool is_system_port () const
    {
        return m_port_type == port::system;
    }

    bool is_port_connectable () const;
    bool set_clock (e_clock clocktype);

    e_clock get_clock () const
    {
        return m_clock_type;
    }

    bool port_enabled () const                  /* replaces get_input() */
    {
        return m_io_active;
    }

    bool port_unavailable () const
    {
        return m_unavailable;
    }

    bool clock_enabled () const
    {
        return clocking_enabled(m_clock_type);  /* pos and mod enabled  */
    }

    void set_io_status (bool flag)
    {
        m_io_active = flag;
    }

    void set_port_unavailable ()
    {
        m_unavailable = true;
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
        const std::string & busname
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

    /**
     *  Obtains a MIDI event.
     *
     * \param inev
     *      Points the event to be filled with the MIDI event data.
     *
     * \return
     *      Returns true if an event was found, thus making the return parameter
     *      useful.
     */

    bool get_midi_event (event * inev)
    {
        return api_get_midi_event(inev);
    }

    /**
     *  Polls for MIDI events.  This is a fix for a PortMidi bug, but it is
     *  needed for all.
     *
     * \return
     *      Returns a value greater than 0 if MIDI events are available.
     *      Otherwise 0 is returned, or -1 for some APIs (ALSA) when an internal
     *      error occurs.
     */

    int poll_for_midi ()
    {
        return m_io_active ? api_poll_for_midi() : 0 ;
    }

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
    bool initialize (bool initdisabled);

private:

    bool init_out ()
    {
        return api_init_out();
    }

    bool init_in ()
    {
        return api_init_in();
    }

    bool init_out_sub ()
    {
        return api_init_out_sub();      // no portmidi implementation
    }

    bool init_in_sub ()
    {
        return api_init_in_sub();       // no portmidi implementation
    }

    bool deinit_in ()
    {
        return api_deinit_out();
    }

    bool deinit_out ()
    {
        return api_deinit_in();
    }

public:

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
     *  Useful for setting the port ID when using the rtmidi_info object to
     *  inspect and create a list of busses and ports.
     */

    void set_port_id (int id)
    {
        m_port_id = id;
    }

    virtual bool is_port_locked () const
    {
        return false;               /* not supported in Linux, just Windows */
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


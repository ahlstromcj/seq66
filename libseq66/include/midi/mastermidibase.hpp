#if ! defined SEQ66_MASTERMIDIBASE_HPP
#define SEQ66_MASTERMIDIBASE_HPP

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
 * \file          mastermidibase.hpp
 *
 *  This module declares/defines the Master MIDI Bus base class.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2016-11-23
 * \updates       2025-05-20
 * \license       GNU GPLv2 or above
 *
 *  The mastermidibase module is the base-class version of the mastermidibus
 *  module.  There's a lot of common code needed by the various
 *  implementations of the seq66::mastermidibus class:  ALSA, RtMidi, and
 *  PortMidi.
 */

#include <vector>                       /* for channel-filtered recording   */

#include "midi/businfo.hpp"             /* seq66::businfo & busarray        */
#include "midi/midibase.hpp"            /* seq66::midibase::io & recmutex   */
#include "play/clockslist.hpp"          /* list of seq66::e_clock settings  */
#include "play/inputslist.hpp"          /* list of boolean input settings   */

namespace seq66
{
    class event;
    class midibus;
    class sequence;

/**
 *  The class that "supervises" all of the midibus objects?
 */

class mastermidibase
{

    friend class performer;
    friend class midi_alsa_info;

protected:

    /**
     *  The ALSA/JACK MIDI client ID.
     */

    int m_client_id;

    /**
     *  The maximum number of busses supported.  Set to c_busscount_max for
     *  now.
     */

    int m_max_busses;

    /**
     *  MIDI buss announcer. Only for ALSA.
     */

    midibus * m_bus_announce;

    /**
     *  Encapsulates information about the input busses.
     */

    busarray m_inbus_array;

    /**
     *  Encapsulates information about the output busses.
     */

    busarray m_outbus_array;

    /**
     *  Saves the clock settings obtained from the "rc" (options) file so that
     *  they can be loaded into the mastermidibus once it is created.
     */

    clockslist m_master_clocks;

    /**
     *  Saves the input settings obtained from the "[midi-input] section of
     *  the "rc" (options) file, so that they can be loaded into the
     *  mastermidibus once it is created.  However, these items will be
     *  modified if the actual enumerated input ports do not match the ports
     *  read from the "rc" file.
     */

    inputslist m_master_inputs;

    /**
     *  The ID of the MIDI queue.
     */

    int m_queue;

    /**
     *  Resolution in parts per quarter note.
     */

    int m_ppqn;

    /**
     *  BPM (beats per minute).  We had to lengthen this name; way too easy to
     *  confuse it with "bpm" for "beats per measure".
     */

    midibpm m_beats_per_minute;

    /**
     *  For dumping MIDI input to a sequence for recording.  This value is set
     *  to true when a sequence editor window is open and the user has
     *  clicked the "record MIDI" or "thru MIDI" button.  See the
     *  set_sequence_input() function.
     */

    bool m_dumping_input;

    /**
     *  Used for the new "stazed" feature of filtering MIDI channels so that
     *  a sequence gets only the channels meant for it.  We want to make this
     *  a run-time, non-legacy option.
     */

    std::vector<sequence *> m_vector_sequence;

    /**
     *  If true, incoming data to the sequence that has the buss it is meant
     *  for.
     */

    bool m_record_by_buss;

    /**
     *  If true, the m_vector_sequence container is used to divert incoming
     *  data to the sequence that has the channel it is meant for.
     */

    bool m_record_by_channel;

    /**
     *  Points to the sequence object.  Set in set_sequence_input().  See that
     *  function's description.
     */

    sequence * m_seq;

    /**
     *  The locking mutex.  This object is passed to an automutex object that
     *  lends exception-safety to the mutex locking.
     */

    recmutex m_mutex;

public:

    mastermidibase () = delete;
    mastermidibase (int ppqn, midibpm bpm);
    virtual ~mastermidibase ();

    /**
     *  Initialize the mastermidibus using the implementation-specific API
     *  function. A return value would be nice.
     *
     * \param ppqn
     *      The PPQN value to which to initialize the master MIDI buss.
     *
     * \param bpm
     *      The beats/minute value to which to initialize the master MIDI
     *      buss.
     */

    virtual void init (int ppqn, midibpm bpm)
    {
        m_ppqn = ppqn;
        m_beats_per_minute = bpm;
        api_init(ppqn, bpm);
    }

    int client_id () const
    {
        return m_client_id;
    }

    int get_num_out_buses () const
    {
        return m_outbus_array.count();
    }

    int get_num_in_buses () const
    {
        return m_inbus_array.count();
    }

    bool record_by_buss () const
    {
        return m_record_by_buss;
    }

    void record_by_buss (bool flag)
    {
        m_record_by_buss = flag;
    }

    bool record_by_channel () const
    {
        return m_record_by_channel;
    }

    void record_by_channel (bool flag)
    {
        m_record_by_channel = flag;
    }

    midibpm get_beats_per_minute () const
    {
        return m_beats_per_minute;
    }

    int get_ppqn () const
    {
        return m_ppqn;
    }

    bool is_dumping_input () const
    {
        return m_dumping_input;
    }

    /**
     *  Used only in performer::input_func() when not filtering MIDI input by
     *  channel.
     */

    sequence * get_sequence () const
    {
        return m_seq;
    }

    void start ();
    void stop ();
    void port_start (int client, int port);
    void port_exit (int client, int port);
    void play (bussbyte bus, event * e24, midibyte channel);
    void play_and_flush (bussbyte bus, event * e24, midibyte channel);
    void sysex (bussbyte bus, const event * event);
    void continue_from (midipulse tick);
    void init_clock (midipulse tick);
    void emit_clock (midipulse tick);
    void print () const;
    void flush ();
    void panic (int displaybuss = c_bussbyte_max);          /* kepler34 func  */
    bool dump_midi_input (event in);                        /* seq32 function */
    std::string get_midi_bus_name (bussbyte bus, midibase::io iotype) const;

    void set_midi_alias
    (
        bussbyte bus, midibase::io iotype, const std::string & alias
    )
    {
        if (iotype == midibase::io::input)
            m_master_inputs.set_alias(bus, alias);
        else
            m_master_clocks.set_alias(bus, alias);
    }

    std::string get_midi_alias (bussbyte bus, midibase::io iotype) const
    {
        return iotype == midibase::io::input ?
            m_master_inputs.get_alias(bus, portname::brief) :
            m_master_clocks.get_alias(bus, portname::brief) ;
    }

    int poll_for_midi ();
    bool set_sequence_input (bool state, sequence * seq);
    bool is_more_input ();

    /**
     *  Grab a MIDI event via the currently-selected MIDI API.
     *  No locking, so we make it an inline function.
     *
     * \param ev
     *      The event to be set based on the found input event.
     */

    bool get_midi_event (event * in)
    {
        return api_get_midi_event(in);
    }

    e_clock get_clock (bussbyte bus) const;
    bool set_clock (bussbyte bus, e_clock clock_type);
    bool get_input (bussbyte bus) const;
    bool set_input (bussbyte bus, bool inputing);
    bool is_input_system_port (bussbyte bus) const;
    bool is_port_unavailable (bussbyte bus, midibase::io iotype) const;
    bool is_port_locked (bussbyte bus, midibase::io iotype) const;
    void copy_io_busses ();
    void set_ppqn (int ppqn);
    void set_beats_per_minute (midibpm bpm);

protected:

    void set_client_id (int id)
    {
        m_client_id = id;
    }

    /**
     *  Used in the performer class to pass the settings read from the "rc"
     *  file to here.  There is an converse function defined below.
     */

    void set_port_statuses (const clockslist & outs, const inputslist & ins)
    {
        m_master_clocks = outs;
        m_master_inputs = ins;
    }

    void get_port_statuses (clockslist & outs, inputslist & ins);
    void get_out_port_statuses (clockslist & outs);
    void get_in_port_statuses (inputslist & ins);

    e_clock clock (bussbyte bus)
    {
        return m_master_clocks.get(bus);
    }

    bool input (bussbyte bus)
    {
        return m_master_inputs.get(bus);
    }

    virtual bool activate ();
    virtual void api_init (int ppqn, midibpm bpm) = 0;

    /**
     *  Provides MIDI API-specific functionality for the start() function.
     */

    virtual void api_start ()
    {
        // no code for base or portmidi
    }

    /**
     *  Provides MIDI API-specific functionality for the continue_from()
     *  function.
     */

    virtual void api_continue_from (midipulse /* tick */)
    {
        // no code for base or portmidi
    }

    /**
     *  Provides MIDI API-specific functionality for the init_clock()
     *  function.
     */

    virtual void api_init_clock (midipulse /* tick */)
    {
        // no code for base, alsa, or portmidi
    }

    /**
     *  Provides MIDI API-specific functionality for the stop() function.
     */

    virtual void api_stop ()
    {
        // no code for base or portmidi
    }

    /**
     *  Provides MIDI API-specific functionality for the set_ppqn() function.
     */

    virtual void api_set_ppqn (int /* ppqn */)
    {
        // no code for base or portmidi
    }

    /**
     *  Provides MIDI API-specific functionality for the
     *  set_beats_per_minute() function.
     */

    virtual void api_set_beats_per_minute (midibpm /* bpm */)
    {
        // no code for base
    }

    /**
     *  Provides MIDI API-specific functionality for the flush() function.
     */

    virtual void api_flush ()
    {
        // no code for base or portmidi
    }

    /**
     *  Provides MIDI API-specific functionality for the clock() function.
     */

    virtual void api_clock ()
    {
        // no code for base, alsmidi, or portmidi
    }

    virtual void api_client_port_start (int /* client */, int /* port */)
    {
        // no code for portmidi
    }

    virtual bool api_get_midi_event (event * inev) = 0;
    virtual int api_poll_for_midi ();

/*
 *  So far, there is no need for these API-specific functions.
 *
 *  virtual void api_sysex (const event * ev) = 0;
 *  virtual void api_play (bussbyte bus, const event * e24, midibyte channel) = 0;
 *  virtual void api_set_clock (bussbyte bus, e_clock clocktype) = 0;
 *  virtual void api_get_clock (bussbyte bus) = 0;
 *  virtual void api_set_input (bussbyte bus, bool inputting) = 0;
 *  virtual void api_get_input (bussbyte bus) = 0;
 */

private:

    bool save_clock (bussbyte bus, e_clock clock);
    bool save_input (bussbyte bus, bool inputing);

};          // class mastermidibase

}           // namespace seq66

#endif      // SEQ66_MASTERMIDIBASE_HPP

/*
 * mastermidibase.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


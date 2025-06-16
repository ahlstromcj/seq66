#if ! defined SEQ66_JACK_ASSISTANT_HPP
#define SEQ66_JACK_ASSISTANT_HPP

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
 * \file          jack_assistant.hpp
 *
 *  This module declares/defines the base class for handling many facets
 *  of performering (playing) a full MIDI song using JACK.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-23
 * \updates       2022-09-26
 * \license       GNU GPLv2 or above
 *
 *  This class contains a number of functions that used to reside in the
 *  still-large performer module.
 */

#include "cfg/rcsettings.hpp"           /* seq66::enum class timebase       */
#include "midi/midibytes.hpp"           /* seq66::midipulse alias           */

#if defined SEQ66_JACK_SUPPORT

#include <jack/jack.h>
#include <jack/transport.h>

#if defined SEQ66_JACK_SESSION
#include <jack/session.h>
#endif

/**
 *  This item exists in the JACK 2 source code, but not in the installed JACK
 *  headers on our development system.
 */

#if defined SEQ66_JACK_METADATA

#if defined __cplusplus
extern "C"
{
#endif

extern const char * JACK_METADATA_ICON_NAME;

#if defined __cplusplus
} /* namespace */
#endif

#endif

#else
#undef SEQ66_JACK_SESSION
#endif

namespace seq66
{
    class performer;                    /* forward reference                */

/**
 *  Provide a temporary structure for passing data and results between a
 *  performer and jack_assistant object.  The jack_assistant class already
 *  has access to the members of performer, but it needs access to and
 *  modification of "local" variables in performer::output_func().  This
 *  scratchpad structure is useful even if JACK support is not enabled.
 */

class jack_scratchpad
{

public:

    double js_current_tick;             /**< Holds current location.        */
    double js_total_tick;               /**< Current location ignoring L/R. */
    double js_clock_tick;               /**< Identical to js_total_tick.    */
    bool js_jack_stopped;               /**< Flags performer::inner_stop(). */
    bool js_dumping;                    /**< Non-JACK playback in progress? */
    bool js_init_clock;                 /**< We now have a good JACK lock.  */
    bool js_looping;                    /**< seqedit loop button is active. */
    bool js_playback_mode;              /**< Song mode (versus live mode).  */
    double js_ticks_converted;          /**< Keeps track of ...?            */
    double js_ticks_delta;              /**< Minor difference in tick.      */
    double js_ticks_converted_last;     /**< Keeps track of position?       */
    long js_delta_tick_frac;            /**< More precision for seq66 0.9.3 */

public:

    jack_scratchpad ();
    void initialize (midipulse currenttick, bool islooping, bool songmode);
    void set_current_tick (midipulse curtick);
    void add_delta_tick (midipulse deltick);
    void set_current_tick_ex (midipulse curtick);

};

#if defined SEQ66_JACK_SUPPORT

/**
 *  Provides an internal type to make it easier to display a specific and
 *  accurate human-readable message when a JACK operation fails.
 */

using jack_status_pair_t = struct
{
    /**
     *  Holds one of the bit-values from jack_status_t, which is defined as an
     *  "enum JackStatus" type.
     */

    unsigned jf_bit;

    /**
     *  Holds a textual description of the corresponding status bit.
     */

    std::string jf_meaning;
};

/**
 *  This class provides the performance mode JACK support.
 */

class jack_assistant
{
    friend int jack_transport_callback (jack_nframes_t nframes, void * arg);
    friend void jack_transport_shutdown (void * arg);

#if defined SEQ66_USE_JACK_SYNC_CALLBACK
    friend int jack_sync_callback
    (
        jack_transport_state_t state,
        jack_position_t * pos,
        void * arg
    );
#endif

    friend void jack_timebase_callback
    (
        jack_transport_state_t state,
        jack_nframes_t nframes,
        jack_position_t * pos,
        int new_pos,
        void * arg
    );

#if defined SEQ66_JACK_SESSION
    friend void jack_session_callback (jack_session_event_t * ev, void * arg);
#endif

public:

    /**
     *  p_position_structure holds frame_rate, ticks_per_beat, and
     *  beats/minute.
     */

    using parameters = struct
    {
        jack_position_t position;
        int period_size;                        /* frames per cycle */
        int alsa_nperiod;                       /* usually 2 or 3   */
    };

private:

    /**
     *  Pairs the JACK status bits with human-readable descriptions of each
     *  one.
     */

    static jack_status_pair_t sm_status_pairs [];

    /**
     *  For issue #100, storage for the true JACK transport position, etc.
     *  Store the current JACK parameters, currently for display only.
     *  Tired of being fooled about the actual parameters.
     */

    static parameters sm_jack_parameters;

    /**
     *  Provides the performer object that needs this JACK assistant/scratchpad
     *  class.
     */

    performer & m_jack_parent;

    /**
     *  Provides a handle into JACK, so that the application, as a JACK
     *  client, can issue commands and retrieve status information from JACK.
     */

    mutable jack_client_t * m_jack_client;

    /**
     *  A new member to hold the actual name of the client assigned by JACK.
     *  We might show this in the user-interface at some point.
     */

    std::string m_jack_client_name;

    /**
     *  A new member to hold the actual UUID of the client assigned by JACK.
     *  We might show this in the user-interface at some point.
     */

    std::string m_jack_client_uuid;

    /**
     *  Holds the current frame number obtained from JACK transport, via a
     *  call to jack_get_current_transport_frame().
     */

    jack_nframes_t m_frame_current;

    /**
     *  Holds the last frame number we got from JACK, so that progress can be
     *  tracked.  Also used in incrementing m_jack_tick.
     */

    jack_nframes_t m_frame_last;

    /**
     *  Provides positioning information on JACK playback.  This structure is
     *  filled via a call to jack_transport_query().  It holds, among other
     *  items, the frame rate (often 48000), the ticks/beat, and the
     *  beats/minute.
     */

    jack_position_t m_jack_pos;

    /**
     *  Holds the JACK transport state.  Common values are
     *  JackTransportStopped, JackTransportRolling, and JackTransportLooping.
     */

    jack_transport_state_t m_transport_state;

    /**
     *  Holds the last JACK transport state.
     */

    jack_transport_state_t m_transport_state_last;

    /**
     *  The tick/pulse value derived from the current frame number, the
     *  ticks/beat value, the beats/minute value, and the frame rate.
     */

    double m_jack_tick;

    /**
     *  Indicates if JACK Sync has been enabled successfully.
     */

    bool m_jack_running;

    /**
     *  Indicates if JACK Sync has been enabled successfully, with the
     *  application running as JACK Master.
     */

    timebase m_timebase;

    /**
     *  Holds the current frame rate.  Just in case.  QJackCtl does not always
     *  set pos.frame_rate, so we get garbage and some strange BBT
     *  calculations displayed in qjackctl.
     */

    jack_nframes_t m_frame_rate;

    /**
     *  Ostensibly a toggle, the functions that access this member are called
     *  "jack_mode" functions.
     */

    bool m_toggle_jack;

    /**
     *  Used in jack_process_callback() to reposition when JACK transport is
     *  not rolling or starting.  Repositions the transport marker.
     */

    midipulse m_jack_stop_tick;

    /**
     *  Indicates to follow JACK transport.
     */

    bool m_follow_transport;

    /**
     *  Holds the global PPQN value for the Seq66 session.  It is used
     *  for calculating ticks/beat (pulses/beat) and for setting the tick
     *  position.
     *
     */

    int m_ppqn;

    /**
     *  Holds the song's beats/measure value for using in setting JACK
     *  position.
     */

    int m_beats_per_measure;

    /**
     *  Holds the song's beat width value (denominator of the time signature)
     *  for using in setting JACK position.
     */

    int m_beat_width;

    /**
     *  Holds the song's beats/minute (BPM) value for using in setting JACK
     *  position.
     */

    midibpm m_beats_per_minute;

public:

    jack_assistant
    (
        performer & parent,
        midibpm bpminute, int ppqn,
        int bpm, int beatwidth
    );
    ~jack_assistant ();

    static void show_position (const jack_position_t & pos);
    static bool save_jack_parameters
    (
        const jack_position_t & p,
        int periodsize  = 0,
        int alsanperiod = 0
    );
    static const parameters & get_jack_parameters ();

    performer & parent ()       /* getter needed for external callbacks.    */
    {
        return m_jack_parent;
    }

    const performer & parent () const
    {
        return m_jack_parent;
    }

    bool is_running () const
    {
        return m_jack_running;
    }

    bool is_master () const
    {
        return m_timebase == timebase::master;
    }

    bool is_slave () const
    {
        return m_timebase == timebase::slave;
    }

    bool no_transport () const
    {
        return m_timebase == timebase::none;
    }

    int get_ppqn () const
    {
        return m_ppqn;
    }

    int beat_width () const
    {
        return m_beat_width;
    }

    void set_beat_width (int bw)
    {
        m_beat_width = bw;
    }

    int beats_per_measure () const
    {
        return m_beats_per_measure;
    }

    void set_beats_per_measure (int bpm)
    {
        m_beats_per_measure = bpm;
    }

    midibpm get_beats_per_minute () const
    {
        return m_beats_per_minute;
    }

    void set_beats_per_minute (midibpm bpminute);

    jack_transport_state_t transport_state () const
    {
        return m_transport_state;
    }

    /**
     *  Returns true if the JACK transport state is not JackTransportStarting.
     */

    bool transport_not_starting () const
    {
        return m_transport_state != JackTransportStarting;
    }

    bool transport_rolling_now () const
    {
        return
        (
            m_transport_state_last == JackTransportStarting &&
            m_transport_state == JackTransportRolling
        );
    }

    bool transport_stopped_now () const
    {
        return
        (
            m_transport_state_last == JackTransportRolling &&
            m_transport_state == JackTransportStopped
        );
    }

    bool init ();
    bool deinit ();

#if defined SEQ66_JACK_SESSION
    void session_event (jack_session_event_t * ev);
#endif

    bool activate ();
    void start ();
    void stop (bool rewind = false);
    void position (bool state, midipulse tick = 0);
    bool output (jack_scratchpad & pad);

    /**
     * \setter m_ppqn
     *      For the future, changing the PPQN internally.  We should consider
     *      adding validation.  But it is used by performer.
     *
     * \param ppqn
     *      Provides the PPQN value to set.
     */

    void set_ppqn (int ppqn)
    {
        m_ppqn = ppqn;
    }

    double get_jack_tick () const
    {
        return m_jack_tick;
    }

    const jack_position_t & jack_pos () const
    {
        return m_jack_pos;
    }

    jack_position_t & jack_pos ()
    {
        return m_jack_pos;
    }

    void toggle_jack_mode ()
    {
        set_jack_mode(! m_jack_running);
    }

    void set_jack_mode (bool mode)
    {
        m_toggle_jack = mode;
    }

    /**
     * \getter m_toggle_jack
     *      Seems misnamed.
     */

    bool get_jack_mode () const
    {
        return m_toggle_jack;
    }

    midipulse jack_stop_tick () const
    {
        return m_jack_stop_tick;
    }

    void jack_stop_tick (long tick)
    {
        m_jack_stop_tick = tick;
    }

    jack_nframes_t jack_frame_rate () const
    {
        return m_frame_rate;
    }

    bool get_follow_transport () const
    {
        return m_follow_transport;
    }

    void set_follow_transport (bool aset)
    {
        m_follow_transport = aset;
    }

    void toggle_follow_transport ()
    {
        set_follow_transport(! m_follow_transport);
    }

    jack_client_t * client () const
    {
        return m_jack_client;
    }

    const std::string & client_name () const
    {
        return m_jack_client_name;
    }

    const std::string & client_uuid () const
    {
        return m_jack_client_uuid;
    }

private:

    /**
     * \setter m_jack_running
     *
     * \param flag
     *      Provides the is-running value to set.
     */

    void set_jack_running (bool flag)
    {
        m_jack_running = flag;
    }

    /**
     *  Convenience function for internal use.  Should we change 4.0 to a
     *  member value?  What does it mean?
     *
     *  The old tick_multiplier() matches what seq24 does, but it
     *  makes the tick delta dependent on "beatwidth / 4". It is was a bug,
     *  Dave.
     *
     * \return
     *      Returns the multiplier to convert a JACK tick value according to
     *      the PPQN, ticks/beat, but not the beat-type.
     */

    double tick_multiplier () const
    {
        return double(m_ppqn) / jack_pos().ticks_per_beat;
    }

    jack_client_t * client_open (const std::string & clientname);
    void get_jack_client_info ();
    long current_jack_position () const;

#if defined SEQ66_USE_JACK_SYNC_CALLBACK
    int sync (jack_transport_state_t state = (jack_transport_state_t)(-1));
#endif
#if defined USE_TIMEBASE_MASTER
    void update_timebase_master (jack_transport_state_t s);
#endif
    void set_position (midipulse currenttick);

};          // class jack_assistant

/**
 *  Global functions for JACK support and JACK sessions.
 */

#if defined SEQ66_USE_JACK_SYNC_CALLBACK
extern int jack_sync_callback
(
    jack_transport_state_t state,
    jack_position_t * pos,
    void * arg
);
#endif

extern void jack_transport_shutdown (void * arg);
extern void jack_timebase_callback
(
    jack_transport_state_t state,
    jack_nframes_t nframes,
    jack_position_t * pos,
    int new_pos,
    void * arg
);

/*
 *  Implemented second patch for JACK Transport from freddix/seq66 GitHub
 *  project.  Added the following functions.
 */

extern int jack_transport_callback (jack_nframes_t nframes, void * arg);
extern jack_client_t * create_jack_client
(
    std::string clientname,
    std::string uuid = ""       /* deprecated */
);
extern void jack_set_position
(
    jack_client_t * client,
    jack_position_t & pos,
    midipulse tick
);
extern std::string get_jack_client_uuid (jack_client_t * jc);

#if defined SEQ66_JACK_METADATA
extern bool set_jack_client_property
(
    jack_client_t * jc,
    const std::string & key,
    const std::string & value,
    const std::string & type = "text/plain"
);
extern bool set_jack_port_property
(
    jack_client_t * jc,
    jack_port_t * jp,
    const std::string & key,
    const std::string & value,
    const std::string & type = "text/plain"
);
extern bool set_jack_port_property
(
    jack_client_t * jc,
    const std::string & portname,
    const std::string & key,
    const std::string & value,
    const std::string & type = "text/plain"
);
#endif

extern void show_jack_statuses (unsigned bits);
extern std::string jack_state_name (const jack_transport_state_t & state);

#if defined SEQ66_JACK_SESSION
extern void jack_session_callback (jack_session_event_t * ev, void * arg);
#endif

#endif      // SEQ66_JACK_SUPPORT

}           // namespace seq66

#endif      // SEQ66_JACK_ASSISTANT_HPP

/*
 * jack_assistant.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


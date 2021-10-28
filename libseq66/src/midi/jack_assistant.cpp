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
 * \file          jack_assistant.cpp
 *
 *  This module defines the helper class for using JACK in the performance
 *  mode.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-09-14
 * \updates       2021-10-26
 * \license       GNU GPLv2 or above
 *
 *  This module was created from code that existed in the performer object.
 *  Moving it into is own module makes it easier to maintain and makes the
 *  performer object a little easier to grok.
 *
 *  For the summaries of the JACK functions used in this module, and how
 *  the code is supposed to operate, see the Seq66 developer's reference
 *  manual.  It discusses the following items:
 *
 *  -   JACK Position Bits
 *  -   jack_transport_reposition()
 *
 *  Only JackPositionBBT is supported so far.
 *
 * JACK clients and BPM:
 *
 *  Does a JACK client need to be JACK Master before it can foist BPM changes
 *  on other clients?  What are the conventions?
 *
 *      -   https://linuxmusicians.com/viewtopic.php?t=14913&start=15
 *      -   http://jackaudio.org/api/transport-design.html
 *
 *  One might be curious as to the origin of the name "jack_assistant".  Well,
 *  it is simply so this class can be called "jack_ass" for short :-D.
 *
 *  Note the confusing (but necessary) orientation of the JACK driver backend
 *  ports: playback ports are "input" to the backend, and capture ports are
 *  "output" from it.
 *
 * NSM Support:
 *
 *      Seq66 supports NSM; see the code in libseq66/ * /sessions and in
 *      libsessions.  Also see the Session Management chapter in the PDF user
 *      manual.
 *
 * JACK Session (JS) Support:
 *
 *  A JS-aware application must:
 *
 *      -   Register with a JS manager.
 *      -   Respond to messages from the JS manager.
 *      -   Be startable with session information.
 *
 *  Response to a JS message will generally:
 *
 *      -   Save the application's state into a file, where the directory is
 *          given by the session manager.
 *      -   Reply to the session manager with a command that can be used to
 *          restart the application, with enough information that it can
 *          restore its state (e.g. the name of the state file).
 *
 *  JS aware clients identify themselves to the session manager by a UUID
 *  The client makes it up as an integer represented as a string; it is passed
 *  to the session manager when registering, and passed back to the client
 *  when it is restarted by the session manager. This is done by a command
 *  line argument to the application, and the format of the command line is
 *  also up to the client. For Seq66, these are the arguments:
 *
 *      -   --jack-session-uuid <uuid>      (-U for short)
 *      -   --home <directory>              (-H for short)
 *      -   --config <basename>             (-c for short) (Maybe???)
 *
 *      -   Saving a session will save the state of all supported applications.
 *          The application can be told where to save the state.
 *          -   The 'rc', 'usr', and other files that need to be saved.
 *          -   The current MIDI file, if modified.
 *          -   The JACK connections that are active.
 *      -   Opening a session will launch the included and supported
 *          applications.  The application can be told where to load the
 *          state.
 *      -   The 'rc' file should hold the connections to be restored.  It can
 *          also specify that the most recent MIDI file be loaded.
 */

#include <stdio.h>
#include <string.h>                     /* strdup() <gasp!>                 */

#include "midi/jack_assistant.hpp"      /* this seq66::jack_ass class       */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "cfg/settings.hpp"             /* "rc" and "user" settings         */

#if defined SEQ66_JACK_SESSION          /* deprecated, use Non Session Mgr. */
#include "midi/midifile.hpp"            /* seq66::midifile class            */
#endif

/*
 *  All library code in the Seq66 project is in the seq66 namespace.
 */

namespace seq66
{

#if defined SEQ66_JACK_SUPPORT

/*
 * -------------------------------------------------------------------------
 *  JACK Transport Callbacks
 * -------------------------------------------------------------------------
 */

/**
 *  Apparently, MIDI pulses are 10 times the size of JACK ticks. So we need,
 *  in some places, to convert pulses (ticks) to JACK ticks by multiplying by
 *  10.
 */

static const int c_jack_factor = 10;

#if defined USE_JACK_DEBUG_PRINT

/**
 *  Debugging code for JACK.  We made this static so that we can hide it in
 *  this module and enable it without enabling all of the debug code available
 *  in the Seq66 code base.  As a side-effect, we added a couple of
 *  const accessors to jack_assistant so that outsiders can monitor some of
 *  its status.
 *
 *  Now, this is really too much output, so you'll have to enable it
 *  separately by defining USE_JACK_DEBUG_PRINT.  The difference in output
 *  between this function and what jack_assitant::show_position() report may
 *  be instructive.
 *
 * \param jack
 *      The jack_assistant object for which to show debugging data.
 *
 * \param current_tick
 *      The current time location.
 *
 * \param tick_delta
 *      The change in ticks to show.
 */

static void
jack_debug_print
(
    const jack_assistant & jack,
    double current_tick,
    double ticks_delta
)
{
    static long s_output_counter = 0;
    if ((s_output_counter++ % 100) == 0)
    {
        const jack_position_t & p = jack.get_jack_pos();
        double jtick = jack.get_jack_tick();

        /*
         * double jack_tick = (p.bar-1) * (p.ticks_per_beat * p.beats_per_bar ) +
         *  (p.beat-1) * p.ticks_per_beat + p.tick;
         */

        long pbar = long(jtick) / long(p.ticks_per_beat * p.beats_per_bar);
        long pbeat = long(jtick) % long(p.ticks_per_beat * p.beats_per_bar);
        pbeat /= long(p.ticks_per_beat);
        long ptick = long(jtick) % long(p.ticks_per_beat);
        printf
        (
            "* curtick=%4.2f delta=%4.2f BBT=%ld:%ld:%ld "
            "jbbt=%d:%d:%d jtick=%4.2f\n"   //  mtick=%4.2f cdelta=%4.2f\n"
            ,
            current_tick, ticks_delta, pbar+1, pbeat+1, ptick,
            p.bar, p.beat, p.tick, jtick    // , jack_tick, jtick - jack_tick
        );
    }
}

#endif  // USE_JACK_DEBUG_PRINT

/**
 *  Provides a dummy callback.
 *
 * \param nframes
 *      An unused parameter.
 *
 * \param arg
 *      Provides the jack_assistant pointer.
 *
 * \return
 *      Does nothing, but returns nframes.  If the arg parameter is null, then
 *      0 is returned.
 */

int
jack_dummy_callback (jack_nframes_t nframes, void * arg)
{
    jack_assistant * j = static_cast<jack_assistant *>(arg);
    if (is_nullptr(j))
        nframes = 0;

    return nframes;
}

/**
 *  On 2021-05-02 we reverted some of the code in this function to the code
 *  present in the Seq64 version.  This seems to solve issue #51 where running
 *  JACK transport under Arch/KXStudio environments causes very iffy playback,
 *  with either multiple repetitions of Note Ons or notes being dropped from
 *  complex tunes.
 *
 *  Implemented second patch for JACK Transport from freddix/seq66 GitHub
 *  project, to allow seq66 to follow JACK transport.  For more advanced
 *  ideas, see the MetronomeJack::process_callback() function in the "klick"
 *  project.  It plays a metronome tick after calculating if it needs to or
 *  not.  (Maybe we could use it to provide our own tick for recording
 *  patterns.)
 *
 *  The code sets the JACK position field bbt_offset to 0.  It doesn't seem to
 *  have any effect, though it can be seen when calling show_position() in the
 *  jack_transport_callback() function.
 *
 *  This callback is called by JACK whether stopped or rolling. JACK calls it
 *  continually with state JackTransportStopped when the (external) Master is
 *  not running, but only once with state JackTransportStarting.
 *
 *  Issue #34 "seq66 doesn't follow jack_transport tempo changes": we no
 *  longer make this conditional upon performer not running.
 *
 *  Be vewwy vewwy carweful!  This code is really prickly and touchy!
 *
 * \param nframes
 *      Unused.
 *
 * \param arg
 *      Used for debug output now.  Note that this function will be called
 *      very often, and this pointer will currently not be used unless
 *      debugging is turned on.
 *
 * \return
 *      Returns 0 on success, non-zero on error.
 */

int
jack_transport_callback (jack_nframes_t /*nframes*/, void * arg)
{
    jack_assistant * j = static_cast<jack_assistant *>(arg);
    if (not_nullptr(j))
    {
        jack_position_t pos;
        jack_transport_state_t s = jack_transport_query(j->client(), &pos);
        performer & p = j->parent();
        if (p.is_running())
        {
            if (j->is_slave())
            {
                if (pos.beats_per_minute > 1.0)         /* a sanity check   */
                {
                    static double s_old_bpm = 0.0;
                    if (pos.beats_per_minute != s_old_bpm)
                    {
                        s_old_bpm = pos.beats_per_minute;
                        j->parent().set_beats_per_minute(pos.beats_per_minute);
                    }
                }
            }

            long tick = j->current_jack_position();
            p.jack_reposition(tick, j->jack_stop_tick());
        }
        else
        {
            /*
             * At start or FF/RW when not running, start or reposition the
             * transport marker.  Using "! j->is_master()" may lead to a hang
             * at exit [in ~QApplication()].  Not sure, very tricky, sporadic.
             */

            if (j->is_slave())
            {
                if (pos.beats_per_minute > 1.0)         /* a sanity check   */
                {
                    static double s_old_bpm = 0.0;
                    if (pos.beats_per_minute != s_old_bpm)
                    {
                        s_old_bpm = pos.beats_per_minute;
                        j->parent().set_beats_per_minute(pos.beats_per_minute);
                    }
                }
            }
            if (s == JackTransportRolling || s == JackTransportStarting)
            {
                j->m_transport_state_last = JackTransportStarting;
                p.inner_start();
            }
            else                            /* reposition transport marker  */
            {
                long tick = j->current_jack_position();
                p.jack_reposition(tick, j->jack_stop_tick());
            }
        }
    }
    return 0;
}

/**
 *  A more full-featured initialization for a JACK client, which is meant to
 *  be called by the init() function.  Do not call this function if the JACK
 *  client handle is already open.
 *
 * Status bits for jack_status_t return pointer:
 *
 *      JackNameNotUnique means that the client name was not unique. With
 *      JackUseExactName, this is fatal. Otherwise, the name was modified by
 *      appending a dash and a two-digit number in the range "-01" to "-99".
 *      The jack_get_client_name() function returns the exact string used. If
 *      the specified client_name plus these extra characters would be too
 *      long, the open fails instead.
 *
 *      JackServerStarted means that the JACK server was started as a result
 *      of this operation. Otherwise, it was running already. In either case
 *      the caller is now connected to jackd, so there is no race condition.
 *      When the server shuts down, the client will find out.
 *
 * JackOpenOptions:
 *
 *      JackSessionID | JackServerName | JackNoStartServer | JackUseExactName
 *
 * helgrind:
 *
 *      Valgrind's helgrind tool shows
 *
\verbatim
          Possible data race during read of size 4 at 0xF854E58 by thread #1
             by 0x267602: seq66::create_jack_client(...)
          This conflicts with a previous write of size 4 by thread #2
             by 0x267602: seq66::create_jack_client(...)
\endverbatim
 *
 *      So we add a static mutex to use with our automutex.  Does not prevent
 *      that message..... WHY? Now (2021-05-23) we're making the string
 *      parameters copies.
 *
 * \param clientname
 *      Provides the name of the client, used in the call to
 *      jack_client_open().  By default, this name is the macro SEQ66_PACKAGE
 *      (i.e.  "seq66").  The name scope is local to each server. Unless
 *      forbidden by the JackUseExactName option, the server will modify this
 *      name to create a unique variant, if needed.
 *
 * \param uuid
 *      The optional UUID to assign to the new client.  If empty, there is no
 *      UUID.
 *
 * \return
 *      Returns a pointer to the JACK client if JACK has opened the client
 *      connection successfully.  Otherwise, a null pointer is returned.
 */

jack_client_t *
create_jack_client (std::string clientname, std::string uuid)
{
    jack_client_t * result = nullptr;
    const char * name = clientname.c_str();
    jack_status_t status;
    jack_status_t * ps = &status;
    jack_options_t options = JackNoStartServer;
    if (uuid.empty())
    {
        result = jack_client_open(name, options, ps);
    }
    else
    {
        const char * uid = uuid.c_str();
        options = static_cast<jack_options_t>(JackNoStartServer|JackSessionID);
        result = jack_client_open(name, options, ps, uid);
        if (not_nullptr(result) && rc().investigate())
        {
            char t[80];
            snprintf(t, sizeof t, "client opened with UUID %s", uid);
            (void) info_message(t);
        }
    }
    if (not_nullptr(result))
    {
        if (status & JackServerStarted)
            (void) info_message("JACK server started");
        else
            (void) info_message("JACK server already started");

        if (status & JackNameNotUnique)
        {
            char t[80];
            snprintf(t, sizeof t, "JACK client name '%s' not unique", name);
            (void) info_message(t);
        }
        else
            show_jack_statuses(status);
    }
    else
        (void) error_message("JACK server not running");

    return result;                      /* bad result handled by caller     */
}

std::string
get_jack_client_uuid (jack_client_t * jc)
{
    std::string result;
    char * lname = jack_get_client_name(jc);
    if (not_nullptr(lname))
    {
        char * luuid = jack_get_uuid_for_client_name(jc, lname);
        if (not_nullptr(luuid))
        {
            result = luuid;
            jack_free(luuid);
        }
    }
    return result;
}

/**
 *  Provides a list of JACK status bits, and a brief string to explain the
 *  status bit.  Terminated by a 0 value and an empty string.
 */

jack_status_pair_t
s_status_pairs [] =
{
    {
        JackFailure,
        "JackFailure: overall operation failed"
    },
    {
        JackInvalidOption,
        "JackInvalidOption: operation used an invalid/unsupported option"
    },
    {
        JackNameNotUnique,
        "JackNameNotUnique: client name was not unique"
    },
    {
        JackServerStarted,
        "JackServerStarted: JACK started by this operation"
    },
    {
        JackServerFailed,
        "JackServerFailed: unable to connect to JACK server"
    },
    {
        JackServerError,
        "JackServerError: communication error with JACK server"
    },
    {
        JackNoSuchClient,
        "JackNoSuchClient: requested client does not exist"
    },
    {
        JackLoadFailure,
        "JackLoadFailure: cannot load internal client"
    },
    {
        JackInitFailure,
        "JackInitFailure: cannot initialize client"
    },
    {
        JackShmFailure,
        "JackShmFailure: cannot access shared memory"
    },
    {
        JackVersionError,
        "JackVersionError: client protocol version mismatch"
    },
    {
        JackBackendError,
        "JackBackendError: JACK back-end error"
    },
    {
        JackClientZombie,
        "JackClientZombie: JACK zombie process!"
    },
    {                                   /* terminator */
        0, ""
    }
};

/**
 *  Loops through the full set of JACK bits, showing the information for any
 *  bits that are set in the given parameter.  For reference, here are the
 *  enumeration values from /usr/include/jack/types.h:
 *
\verbatim
        JackFailure         = 0x01
        JackInvalidOption   = 0x02
        JackNameNotUnique   = 0x04
        JackServerStarted   = 0x08
        JackServerFailed    = 0x10
        JackServerError     = 0x20
        JackNoSuchClient    = 0x40
        JackLoadFailure     = 0x80
        JackInitFailure     = 0x100
        JackShmFailure      = 0x200
        JackVersionError    = 0x400
        JackBackendError    = 0x800
        JackClientZombie    = 0x1000
\endverbatim
 *
 * \param bits
 *      The mask of the bits to be shown in the output.
 */

void
show_jack_statuses (unsigned bits)
{
    jack_status_pair_t * jsp = &s_status_pairs[0];
    while (jsp->jf_bit != 0)
    {
        if (bits & jsp->jf_bit)
            (void) info_message(jsp->jf_meaning);

        ++jsp;
    }
}

/*
 * -------------------------------------------------------------------------
 *  JACK helper functions
 * -------------------------------------------------------------------------
 */

static double
jack_ticks (const jack_position_t & pos)
{
    return
    (
        pos.frame * pos.ticks_per_beat *
        pos.beats_per_minute / (pos.frame_rate * 60.0)
    );
}

static double
jack_ticks_delta (int framediff, const jack_position_t & pos)
{
    return
    (
        framediff * pos.ticks_per_beat *
        pos.beats_per_minute / (pos.frame_rate * 60.0)
    );
}

/*
 * -------------------------------------------------------------------------
 *  JACK Assistant
 * -------------------------------------------------------------------------
 */

/**
 *  This constructor initializes a number of member variables, some
 *  of them public!
 *
 *  Note that the performer object currently calls jack_assistant::init(), but
 *  that call could be made here instead.
 *
 * \param parent
 *      Provides a reference to the main performer object that needs to
 *      control JACK event.
 *
 * \param bpminute
 *      The beats/minute to set up JACK to use (applies to Master setup).
 *
 * \param ppqn
 *      The parts-per-quarter-note setting in force for the present tune.
 *
 * \param bpmeasure
 *      The beats/measure (time signature numerator) in force for the present
 *      tune.
 *
 * \param beatwidth
 *      The beat-width (time signature denominator)  in force for the present
 *      tune.
 */

jack_assistant::jack_assistant
(
    performer & parent,
    midibpm bpminute,
    int ppqn,
    int bpmeasure,
    int beatwidth
) :
    m_jack_parent               (parent),
    m_jack_client               (nullptr),
    m_jack_client_name          (),
    m_jack_client_uuid          (),
    m_frame_current             (0),
    m_frame_last                (0),
    m_jack_pos                  (),
    m_transport_state           (JackTransportStopped),
    m_transport_state_last      (JackTransportStopped),
    m_jack_tick                 (0.0),
    m_jack_running              (false),
    m_timebase                  (timebase::none),   /* or slave, master...  */
#if defined ENABLE_PROPOSED_FUNCTIONS
    m_timebase_tracking         (-1),
#endif
    m_frame_rate                (0),
    m_toggle_jack               (false),
    m_jack_stop_tick            (0),
    m_follow_transport          (true),
    m_ppqn                      (choose_ppqn(ppqn)),
    m_beats_per_measure         (bpmeasure),
    m_beat_width                (beatwidth),
    m_beats_per_minute          (bpminute)
{
    /*
     * Do this in the rtmidi constructor.
     *
     * const char * jv = jack_get_version_string();
     * if (not_nullptr(jv) && strlen(jv) > 0)
     *     set_jack_version(std::string(jv));
     */
}

/**
 *  The destructor doesn't need to do anything yet.  The performer object
 *  currently calls jack_assistant::deinit(), but that call could be made here
 *  instead.
 */

jack_assistant::~jack_assistant ()
{
    /*
     * Anything to do?  Call deinit()?
     */
}

/**
 *  Tries to obtain the best information on the JACK client and the UUID
 *  assigned to this client.  Sets m_jack_client_name and m_jack_client_info
 *  as side-effects.
 *
 * Only store the transport UUID in the rc().jack_session() if not already
 * filled by opening the with-jack MIDI client.
 */

void
jack_assistant::get_jack_client_info ()
{
    char * actualname = jack_get_client_name(m_jack_client);
    if (not_nullptr(actualname))
    {
        m_jack_client_uuid = get_jack_client_uuid(m_jack_client);
        if (! m_jack_client_uuid.empty())               /* this test okay?? */
        {
            if (rc().jack_session().empty())
                rc().jack_session(m_jack_client_uuid);
        }
        m_jack_client_name = actualname;
    }

    std::string jinfo = "JACK transport client:uuid ";
    jinfo += m_jack_client_name;
    if (! m_jack_client_uuid.empty())
    {
        jinfo += ":";
        jinfo += m_jack_client_uuid;
    }
    (void) info_message(jinfo);
}

/**
 *  Initializes JACK support.  Then we become a new client of the JACK server.
 *
 *  A sync callback is needed for polling of slow-sync clients.  But
 *  seq66 are not slow-sync clients.  We don't really need to be a
 *  slow-sync client, as far as we can tell.  We can't get JACK working
 *  exactly the way it does in seq66 without the callback in place.  Plus, it
 *  does things important to the setup of JACK.  So now this setup is
 *  permanent.
 *
 * Jack transport settings:
 *
 *      There are three settings:  On, Master, and Master Conditional.
 *      Currently, they can all be selected in the user-interface's File /
 *      Options / JACK page.  We really want only the proper combinations
 *      to be set, for clarity (the user-interface now takes care of this.  We
 *      need to initialize if any of them are set, and the
 *      rcsettings::with_jack_transport() function tells us that.
 *
 * jack_set_process_callback() patch:
 *
 *      Implemented first patch from freddix/seq66 GitHub project, to fix JACK
 *      transport.  One line of code.  Well, we added some error-checking. :-)
 *      Found some old notes on the Web the this patch really only works (to
 *      prevent seq66 freeze) if seq66 is set as JACK Master, or if another
 *      client application, such as Qtractor, is running as JACK Master (and
 *      then seq66 will apparently follow it).
 *
 * Stazed:
 *
 *      The call to jack_timebase_callback() to supply jack with BBT, etc.
 *      would occasionally fail when the *pos information had zero or some
 *      garbage in the pos.frame_rate variable. This would occur when there
 *      was a rapid change of frame position by another client... i.e.
 *      qjackctl.  From the jack API:
 *
 *          "pos address of the position structure for the next cycle;
 *          pos->frame will be its frame number. If new_pos is FALSE, this
 *          structure contains extended position information from the current
 *          cycle.  If TRUE, it contains whatever was set by the requester.
 *          The timebase_callback's task is to update the extended information
 *          here."
 *
 *      The "If TRUE" line seems to be the issue. It seems that qjackctl does
 *      not always set pos.frame_rate so we get garbage and some strange BBT
 *      calculations that display in qjackctl. So we need to set it here and
 *      just use m_frame_rate for calculations instead of pos.frame_rate.
 *
 *      Stazed JACK support uses only the jack_transport_callback().  Makes
 *      sense, since seq66/32/64 are not "slow-sync" clients.
 *
 * \return
 *      Returns true if JACK is now considered to be running (or if it was
 *      already running.)
 */

bool
jack_assistant::init ()
{
    bool result = rc().with_jack_transport() && ! m_jack_running;
    if (result)
    {
        std::string kind = rc().with_jack_master() ? "master" : "slave" ;
        std::string package = rc().app_client_name() + kind;
        m_timebase = timebase::none;
        m_jack_client = client_open(package);
        if (m_jack_client == NULL)
        {
            result = false;
            return error_message("No JACK server");
        }
        else
        {
            m_frame_rate = jack_get_sample_rate(m_jack_client);
            get_jack_client_info();
            jack_on_shutdown            /* no return value, surprisingly    */
            (
                m_jack_client, jack_transport_shutdown, (void *) this
            );

            int jackcode = jack_set_process_callback    /* notes in banner  */
            (
                m_jack_client, jack_transport_callback, (void *) this
            );
            if (jackcode != 0)
            {
                result = false;
                return error_message("JACK set callback failed");
            }
        }

#if defined SEQ66_JACK_SESSION
        if (result && usr().want_jack_session())
        {
            int jackcode = jack_set_session_callback
            (
                m_jack_client, jack_session_callback, (void *) this
            );
            if (jackcode != 0)
            {
                result = false;
                (void) error_message("jack_set_session_callback() failed]");
            }
            else
            {
                if (rc().investigate_disabled())
                    info_message("JACK session callback set");
            }
        }
#endif

        if (result)
        {
            bool master_is_set = false;         /* flag to handle trickery  */
            if (rc().with_jack_master())        /* OR with 'cond' removed   */
            {
                /*
                 * 'cond' is true if we want to fail if there is already a JACK
                 * master, i.e. it is a conditional attempt to be JACK master.
                 */

                bool cond = rc().with_jack_master_cond();
                int jackcode = jack_set_timebase_callback
                (
                    m_jack_client, cond, jack_timebase_callback, (void *) this
                );
                if (jackcode == 0)
                {
                    (void) info_message("JACK transport master");
                    m_timebase = timebase::master;
                    master_is_set = true;
                }
                else
                {
                    result = false;
                    (void) error_message("jack_set_timebase_callback() failed");
                }
            }
            if (! master_is_set)
            {
                m_timebase = timebase::slave;
                (void) info_message("JACK transport slave");
            }
        }
        if (result)
        {
            result = activate();
            if (result)
            {
                (void) info_message("JACK transport enabled");
                m_jack_running = true;
            }
            else
                (void) info_message("Running without JACK transport");
        }
    }
    return result;
}

/**
 *  Tears down the JACK infrastructure.
 *
 * \todo
 *      Note that we still need a way to call jack_release_timebase()  when
 *      the user turns off the "JACK Master" status of Seq66.
 *
 * \return
 *      Returns the value of m_jack_running, which should be false.
 */

bool
jack_assistant::deinit ()
{
    bool result = true;
    if (m_jack_running)
    {
        m_jack_running = false;
        if (is_master())
        {
            m_timebase = timebase::none;
            if (jack_release_timebase(m_jack_client) != 0)
                (void) error_message("Cannot release JACK timebase");
        }

        /*
         * New:  Simply to be symmetric with the startup flow.  Not yet sure
         * why jack_activate() was needed, but assume that jack_deactivate() is
         * thus important as well.
         */

        if (jack_deactivate(m_jack_client) != 0)
            result = error_message("Can't deactivate JACK transport");

        if (jack_client_close(m_jack_client) != 0)
            result = error_message("Can't close JACK transport");
    }
    return result;
}

/**
 *  Activate JACK here.  This function is called by performer::activate() after
 *  the master bus is activated successfully.
 *
 * \return
 *      Returns true if the m_jack_client pointer is null, which means only
 *      that we're not running JACK.  Also returns true if the pointer exists
 *      and the jack_active() call succeeds.
 *
 * \sideeffect
 *      The m_jack_running and JACK master flags are falsified if
 *      jack_activate() fails.
 */

bool
jack_assistant::activate ()
{
    bool result = true;
    if (not_nullptr(m_jack_client))
    {
        int rc = jack_activate(m_jack_client);
        result = rc == 0;
        if (result)
        {
            (void) info_message("JACK activated");
        }
        else
        {
            m_timebase = timebase::none;
            (void) error_message("Can't activate JACK transport client");
        }
    }
    return result;
}

/**
 *  If JACK is supported, starts the JACK transport.  This function assumes
 *  that m_jack_client is not null, if m_jack_running is true.
 */

void
jack_assistant::start ()
{
    if (m_jack_running)
    {
        jack_transport_start(m_jack_client);
        if (is_master())
            set_position(parent().get_tick());
    }
    else if (rc().with_jack())
        (void) error_message("Sync start: JACK not running");
}

/**
 *  If JACK is supported, stops the JACK transport.  This function assumes
 *  that m_jack_client is not null, if m_jack_running is true.
 */

void
jack_assistant::stop ()
{
    if (m_jack_running)
        jack_transport_stop(m_jack_client);
    else if (rc().with_jack())
        (void) error_message("Sync stop: JACK not running");
}

/**
 *  performer::set_beats_per_minute() validates the BPM.  Also, since
 *  jack_transport_reposition() can be "called at any time by any client", we
 *  have removed the check for "is master".  We do seem to see more "bad
 *  position structure" messages, though.
 *
 * \param bpminute
 *      Provides the beats/minute value to set.
 */

void
jack_assistant::set_beats_per_minute (midibpm bpminute)
{
    if (bpminute != m_beats_per_minute)
    {
        m_beats_per_minute = bpminute;
        if (not_nullptr(m_jack_client))
        {
            (void) jack_transport_query(m_jack_client, &m_jack_pos);
            m_jack_pos.beats_per_minute = bpminute;
            int jackcode = jack_transport_reposition(m_jack_client, &m_jack_pos);
            if (jackcode != 0)
            {
                errprint("jack_transport_reposition(): bad position structure");
            }
        }
    }
}

/**
 *  If JACK is supported and running, sets the position of the transport to
 *  the new frame number, frame 0.  This new position takes effect in two
 *  process cycles. If there are slow-sync clients and the transport is
 *  already rolling, it will enter the JackTransportStarting state and begin
 *  invoking their sync_callbacks until ready. This function is realtime-safe.
 *
 *      http://jackaudio.org/files/docs/html/transport-design.html
 *
 *  This position() function is called via performer::position_jack() in the
 *  mainwnd, perfedit, perfroll, and seqroll graphical user-interface support
 *  objects.
 *
 * Stazed:
 *
 *      The jack_frame calculation is all that is needed to change JACK
 *      position. The BBT calculation can be sent, but will be overridden by the
 *      first call to jack_timebase_callback() of any Master set. If no Master
 *      is set, then the BBT will display the new position but will not change
 *      it, even if the transport is rolling. There is no need to send BBT on
 *      position change -- the fact that jack_transport_locate() exists and only
 *      uses the frame position is proof that BBT is not needed! Upon further
 *      reflection, why not send BBT?  Because other programs do not... let's
 *      follow convention.  The calculation for jack_transport_locate(), works,
 *      is simpler, and does not send BBT. The calculation for
 *      jack_transport_reposition() will be commented out again.
 *      jack_BBT_position() is not necessary to change jack position!
 *
 *  Let's follow the example of Stazed's tick_to_jack_frame() function.  One odd
 *  effect we want to solve is why Seq66 as JACK slave is messing up the playback
 *  in Hydrogen (it oscillates around the 0 marker).  Note that there are
 *  potentially a couple of divide-by-zero opportunities in this function.
 *  Helgrind complains about a possible data race involving
 *  jack_transport_locate() when starting playing.
 *
 * \param songmode
 *      True if the caller wants to position while in Song mode.
 *
 * Alternate parameter to_left_tick (non-seq32 version):
 *
 *      If true, the current tick is set to the leftmost tick, instead of the
 *      0th tick.  Now used, but only if relocate is true.  One question is,
 *      do we want to performer this function if rc().with_jack_transport() is
 *      true?  Seems like we should be able to do it only if JACK master is
 *      true.
 *
 * \param tick
 *      If using Song mode for this call then this value is set as the
 *      "current tick" value.  If it's value is bad (null_midipulse),
 *      then this parameter is set to 0 before being used.
 */

void
jack_assistant::position (bool songmode, midipulse tick)
{
#if defined SEQ66_JACK_SUPPORT
    if (songmode)                               /* master in song mode  */
        tick = is_null_midipulse(tick) ? 0 : tick * c_jack_factor ;
    else
        tick = 0;

    int ticks_per_beat = m_ppqn * c_jack_factor;
    int beats_per_minute = parent().get_beats_per_minute();
    uint64_t tick_rate = (uint64_t(m_frame_rate) * tick * 60.0);
    long tpb_bpm = ticks_per_beat * beats_per_minute * 4.0 / m_beat_width;
    uint64_t jack_frame = tick_rate / tpb_bpm;
    if (is_master())
    {
        /*
         * We don't want to do this unless we are JACK Master.  Otherwise,
         * other JACK clients never advance if Seq66 won't advance.
         * However, according to JACK docs, "Any client can start or stop
         * playback, or seek to a new location."
         */

        if (jack_transport_locate(m_jack_client, jack_frame) != 0)
            (void) info_message("jack_transport_locate() failed");
    }
    if (parent().is_running())
        parent().set_reposition(false);
#endif
}

/**
 *  This function is currently unused, and has been macroed out.
 *
 *  Provides the code that was effectively commented out in the
 *  performer::position_jack() function.  We might be able to use it in other
 *  functions.
 *
 *  Computing the BBT information from the frame number is relatively simple
 *  here, but would become complex if we supported tempo or time signature
 *  changes at specific locations in the transport timeline.
 *
 \verbatim
        ticks * 10 = jack ticks;
        jack ticks / ticks per beat = num beats;
        num beats / beats per minute = num minutes
        num minutes * 60 = num seconds
        num secords * frame_rate  = frame
 \endverbatim
 *
 * \param tick
 *      Provides the current position to be set.
 */

void
jack_assistant::set_position (midipulse tick)
{
    jack_position_t pos;
    pos.valid = JackPositionBBT;                // flag what will be modified
    pos.beats_per_bar = m_beats_per_measure;
    pos.beat_type = m_beat_width;
    pos.ticks_per_beat = m_ppqn * c_jack_factor;
    pos.beats_per_minute = get_beats_per_minute();

    /*
     * pos.frame = frame;
     */

    tick *= c_jack_factor;          /* compute BBT info from frame number */
    pos.bar = int32_t(tick / long(pos.ticks_per_beat) / pos.beats_per_bar);
    pos.beat = int32_t(((tick / long(pos.ticks_per_beat)) % m_beat_width));
    pos.tick = int32_t((tick % (m_ppqn * c_jack_factor)));
    pos.bar_start_tick = pos.bar * pos.beats_per_bar * pos.ticks_per_beat;
    ++pos.bar;
    ++pos.beat;

    /*
     * Modifying frame rate and frame cannot be set from clients, the server
     * sets them; see transport.h of JACK.
     *
     *  jack_nframes_t rate = jack_get_sample_rate(m_jack_client);
     *  pos.frame_rate = rate;
     *  pos.frame = (jack_nframes_t)
     *  (
     *      (tick * rate * 60.0) /
     *      (pos.ticks_per_beat * pos.beats_per_minute)
     *  );
     */

    pos.valid = (jack_position_bits_t)(pos.valid | JackBBTFrameOffset);
    pos.bbt_offset = 0;

    int jackcode = jack_transport_reposition(m_jack_client, &pos);
    if (jackcode != 0)
    {
        errprint("jack_assistant::set_position(): bad position structure");
    }
}

#if defined SEQ66_USE_JACK_SYNC_CALLBACK

/**
 *  A helper function for syncing up with JACK parameters.  Seq66 is not a
 *  slow-sync client (and Stazed support doesn't use it), so that callback is
 *  not really needed, but we probably need this sub-function here to start out
 *  with the right values for interacting with JACK.
 *
 *  Note the call to jack_transport_query().  This call but seems to be needed
 *  because we put m_jack_pos in the initializer list, which sets all its fields
 *  to 0.  Seq24 accesses m_jack_pos before it ever gets set, but its fields
 *  have values.  These values are bogus, but are consistent from run to run on
 *  my computer, and allow seq66 to follow another JACK Master, on some
 *  computers.  It explains why people had different experiences with JACK
 *  transport.
 *
 *  If we explicity call jack_transport_query() here, without changing the \a
 *  state parameter, then seq66 also can follow another JACK Master.
 *  (CURRENTLY BUGGY!)
 *
 *  Note that we should consider massaging the following jack_position_t
 *  members to set them to 0 (or 0.0) if less than 1.0 or 0.5:
 *
 *      -   bar_start_tick
 *      -   ticks_per_beat
 *      -   beats_per_minute
 *      -   frame_time
 *      -   next_time
 *      -   audio_frames_per_video_frame
 *
 *  Also, why does bbt_offset start at 2128362496?
 *
 * \param state
 *      The JACK transport state to be set.
 */

int
jack_assistant::sync (jack_transport_state_t state)
{
    int result = 0;                     /* seq66 always returns 1   */
    m_frame_current = jack_get_current_transport_frame(m_jack_client);
    (void) jack_transport_query(m_jack_client, &m_jack_pos);

    jack_nframes_t rate = m_jack_pos.frame_rate;
    if (rate == 0)
    {
        /*
         * The actual frame rate might be something like 48000.  Try to make
         * it work somehow, for now.
         */

        errprint("jack_assistant::sync(): zero frame rate");
        rate = 48000;
    }
    else
        result = 1;

    m_jack_tick = m_frame_current * m_jack_pos.ticks_per_beat *
        m_jack_pos.beats_per_minute / (rate * 60.0) ;

    m_frame_last = m_frame_current;
    m_transport_state_last = m_transport_state = state;
    if (state == JackTransportStarting)
        parent().inner_start();

    return result;
}

#endif  // defined SEQ66_USE_JACK_SYNC_CALLBACK

#if defined SEQ66_USE_JACK_SYNC_CALLBACK

/**
 *  This JACK synchronization callback informs the specified performer
 *  object of the current state and parameters of JACK.
 *
 *  The transport state will be:
 *
 *      -   JackTransportStopped when a new position is requested.
 *      -   JackTransportStarting when the transport is waiting to start.
 *      -   JackTransportRolling when the timeout has expired, and the
 *          position is now a moving target.
 *
 *  This is the slow-sync callback, which the stazed code replaces with
 *  jack_transport_callback().
 *
 * \param state
 *      The JACK Transport state.
 *
 * \param pos
 *      The JACK position value.
 *
 * \param arg
 *      The pointer to the jack_assistant object.  Currently not checked for
 *      nullity, nor dynamic-casted.
 *
 * \return
 *      Returns 1 if the function works, and 0 if something was wrong.
 */

int
jack_sync_callback
(
    jack_transport_state_t state,
    jack_position_t * pos,
    void * arg
)
{
    int result = 0;
    jack_assistant * jack = static_cast<jack_assistant *>(arg);
    if (not_nullptr(jack))
    {
        result = jack->sync(state);         /* use the new member function  */
    }
    else
    {
        errprint("jack_sync_callback(): null JACK pointer");
    }
    return result;
}

#endif  // SEQ66_USE_JACK_SYNC_CALLBACK

#if defined SEQ66_JACK_SESSION          /* "deprecated" alternative to NSM  */

static std::string
session_event_name (jack_session_event_t * ev)
{
    std::string result = "JACK Session Event: '";
    if (ev->type == JackSessionSave)
        result += "Save";
    else if (ev->type == JackSessionSaveAndQuit)
        result += "Save and quit";
    else if (ev->type == JackSessionSaveTemplate)
        result += "Save template";

    result += "'";
    return result;
}

/**
 *  Writes the state information quits if told to by JACK, and can free the JACK
 *  session event.
 *
 *  The job of the callback is to save state information, pass information back
 *  to the session manager and perhaps exit.
 *
 * Command line:
 *
 *      qseq66 --jack-session-uuid UUID --home SESSION_DIR
 *
 * Event types:
 *
 *      -   JackSessionSave. Save the session completely.  The client may save
 *          references to data outside the provided directory, but only so by
 *          creating a link inside the provided directory.  The client must
 *          not refer to data files outside the provided directory directly in
 *          save files; this makes it impossible for the session manager to
 *          create a session archive for distribution or archival.
 *      -   JackSessionSaveAndQuit. Save the session completely, then quit.
 *          The rules for saving are exactly the same as for JackSessionSave.
 *      -   JackSessionSaveTemplate. Save a session template.  A session
 *          template is a "skeleton" of the session without any data. Clients
 *          must save a session that, when restored, will create the same
 *          ports as a full save would have. However, the actual data
 *          contained in the session may not be saved (e.g. a DAW would create
 *          the necessary tracks, but not save the actual recorded data).
 *
 * Flags:
 *
 *      -   JackSessionSaveError.  An error occurred while saving.
 *      -   JackSessionNeedTerminal.  Client needs to run in a terminal.
 *
 * TODO:
 *
 *      Move this code into libsessions/ * / jack, create a base class
 *      [sessionbase] to use with a new jack_session class and the existing
 *      nsmbase class. The functions needed in the session base class might be
 *
 *      -   is_active() and m_active
 *      -   is_a_client() and not_a_client()
 *      -   qsessionmanager
 *          -   session_manager
 *          -   session_path
 *          -   session_display_name
 *          -   session_client_id
 */

void
jack_assistant::session_event (jack_session_event_t * ev)
{
    bool quit = false;
    std::string uuid = std::string(ev->client_uuid);
    std::string filepath = std::string(ev->session_dir);
    std::string cmd = seq_app_name();                   /* e.g. "qseq66"    */
    cmd += (" --jack-midi");
    cmd += (" --jack-");
    cmd += rc().with_jack_master() ? "master" : "slave" ;
    cmd += (" --jack-session ");
    cmd += uuid;
    cmd += " --home ${SESSION_DIR}";
    ev->command_line = strdup(cmd.c_str());

    std::string clientname = rc().app_client_name();    /* seq_client_id()  */
    clientname += ":";
    clientname += uuid;
    rc().app_client_name(clientname);                   /* set_client_id()  */
    rc().home_config_directory(filepath);

    /*
     * Not sure it is the business of this particular session manager (as
     * opposed to NSM) to determine where the user's data resides.
     *
     *      rc().midi_filepath(filepath);
     */

    if (jack_session_reply(m_jack_client, ev) != 0)
    {
        errprint("JACK session reply failed");
    }
    if (ev->type == JackSessionSave)
    {
        parent().signal_save();                     /* session_save()   */
    }
    else if (ev->type == JackSessionSaveAndQuit)
    {
        quit = true;
    }
    else if (ev->type == JackSessionSaveTemplate)
    {
        /*
         * Not yet directly supported. Seq66 creates bare-bones files only if
         * no configuration files yet exist in the "home" location.
         */
    }
    if (rc().investigate_disabled())
    {
        info_message(session_event_name(ev));
        file_message("Session command", cmd);
        file_message("Session path", filepath);
    }
    jack_session_event_free(ev);
    if (quit)
        parent().signal_quit();                     /* session_close()      */
    else
        rc().jack_session_activate();               /* really have session! */
}

/**
 *  Glib is then used to connect in performer::jack_session_event().  However,
 *  the performer object's GUI-support interface is used instead of the
 *  following, so that the libseq66 library can be independent of a specific
 *  GUI framework:
 *
 *      Glib::signal_idle().
 *          connect(sigc::mem_fun(*jack, &jack_assistant::session_event));
 *
 * \param ev
 *      The JACK event to be set.
 *
 * \param arg
 *      The pointer to the jack_assistant object.  Currently not checked
 *      for nullity.
 */

void
jack_session_callback (jack_session_event_t * ev, void * arg)
{
    jack_assistant * jack = static_cast<jack_assistant *>(arg);
    if (not_nullptr(jack))
        jack->session_event(ev);

    /*
     * jack->parent().gui().jack_idle_connect(*jack);   // see note above
     */
}

#endif  // SEQ66_JACK_SESSION

/**
 *  Performance output function for JACK, called by the performer function
 *  of the same name.  This code comes from performer::output_func() from seq24.
 *
 * \note
 *      Follow up on this note found "out there":  "Maybe I'm wrong but if I
 *      understood correctly, recent jack1 transport no longer goes into
 *      Jack_Transport_Starting state before going to Jack_Transport_Rolling
 *      (this was deliberately dropped), but seq24 currently needs this to
 *      start off with JACK transport."  On the other hand, some people have
 *      no issues.  This may have been due to the lack of m_jack_pos
 *      initialization.
 *
 * Stazed:
 *
 *      Another note about JACK.  If another JACK client supplies tempo/BBT
 *      different from seq42 (as Master), the perfroll grid will be incorrect.
 *      Perfroll uses internal temp/BBT and cannot update on the fly. Even if
 *      seq42 could support tempo/BBT changes, all info would have to be
 *      available before the transport start, to work.  For this reason, the
 *      tempo/BBT info will be plugged from the seq42 internal settings here,
 *      always. This is the method used by probably all other JACK clients
 *      with some sort of time-line. The JACK API indicates that BBT is
 *      optional and AFIK, other sequencers only use frame & frame_rate from
 *      JACK for internal calculations. The tempo and BBT info is always
 *      internal. Also, if there is no Master set, then we would need to plug
 *      it here to follow the JACK frame anyways.
 *
 * Issue #48 "Non-JACK-Master p'back not working if built for non-seq32 JACK":
 *
 *      Using the seq32 code here works to solve issue #48, We scrapped the
 *      old code entirely.  As for the setting of beats/minute, we had thought
 *      that we wanted to force a change in BPM only if we are JACK Master,
 *      but this is not true, and prevents Seq66 from playing back when not
 *      the Master.
 *
 * \param pad
 *      Provides a JACK scratchpad for sharing certain items between the
 *      performer object and the jack_assistant object.
 *
 * \return
 *      Returns true if JACK is running.
 */

bool
jack_assistant::output (jack_scratchpad & pad)
{
    if (m_jack_running)
    {
        pad.js_init_clock = false;              /* no init until a good lock */
        m_transport_state = jack_transport_query(m_jack_client, &m_jack_pos);

        /* See Issue #48 above */

        m_jack_pos.beats_per_bar = m_beats_per_measure;
        m_jack_pos.beat_type = m_beat_width;
        m_jack_pos.ticks_per_beat = m_ppqn * c_jack_factor;
        m_jack_pos.beats_per_minute = parent().get_beats_per_minute();
        if (transport_rolling_now())
        {
            midipulse midi_ticks;
            m_frame_current = jack_get_current_transport_frame(m_jack_client);
            m_frame_last = m_frame_current;
            pad.js_dumping = true;              /* "[Start JACK Playback]"  */

            /*
             * Here, Seq32 uses the tempo map if in song mode, instead of
             * making these calculations.
             */

            m_jack_tick = jack_ticks(m_jack_pos);
            midi_ticks = midipulse(m_jack_tick * tick_multiplier() + 0.5);
            parent().set_last_ticks(midi_ticks);
            pad.set_current_tick_ex(midi_ticks);
            pad.js_init_clock = true;
            if (pad.js_looping && pad.js_playback_mode)
            {
                if (pad.js_current_tick >= parent().get_right_tick())
                {
                    /*
                     * Let C = pad.js_current_tick, R and L be the looping
                     * ticks.  This loop should continue about C / (R-L)
                     * times, if R and L remain the same.
                     */

                    double r_minus_l = parent().left_right_size();
                    while (pad.js_current_tick >= parent().get_right_tick())
                        pad.js_current_tick -= r_minus_l;

                    parent().off_sequences();
                    parent().set_last_ticks(midipulse(pad.js_current_tick));
                }
            }
        }
        if (transport_stopped_now())
        {
            m_transport_state_last = JackTransportStopped;
            pad.js_jack_stopped = true;
        }

        /*
         * Jack Transport is Rolling Now!??  Transport is in a sane state if
         * dumping == true.
         */

        if (pad.js_dumping)
        {
            m_frame_current = jack_get_current_transport_frame(m_jack_client);
            if (m_frame_current > m_frame_last)         /* moving ahead?    */
            {
                /*
                 * Seq32 uses tempo map if in song mode here, instead.
                 */

                if (m_jack_pos.frame_rate > 0)          /* usually 48000    */
                {
                    int diff = int(m_frame_current - m_frame_last);
                    m_jack_tick += jack_ticks_delta(diff, m_jack_pos);
                }
                else
                    info_message("jack_assistant::output() 2: zero frame rate");

                m_frame_last = m_frame_current;
            }

            double midi_ticks = m_jack_tick * tick_multiplier();
            double delta = midi_ticks - pad.js_ticks_converted_last;
            if (delta != 0.0)
            {
                /*
                 * On one system, a PPQN of 192 yields deltas around 1.5,
                 * and PPQN yields deltas of around 0.8, leading to many more
                 * "replays" of the same tick value.
                 */

                pad.js_clock_tick += delta;
                pad.js_current_tick += delta;
                pad.js_total_tick += delta;
            }
            m_transport_state_last = m_transport_state;
            pad.js_ticks_converted_last = midi_ticks;

#if defined USE_JACK_DEBUG_PRINT
            jack_debug_print(*this, pad.js_current_tick, delta);
#endif
        }                               /* if dumping (sane state)  */
    }                                   /* if m_jack_running        */
    return m_jack_running;
}

#if defined ENABLE_PROPOSED_FUNCTIONS

/**
 *  Adapted from Hydrogen source code.  If transport is not stopped, check the
 *  timebase tracking count.  If 0, the JackTimeBaseCallback isn't called
 *  anymore, so we're the client (slave). Otherwise, if there is no timebase
 *  master anymore, we become a regular client, or there is a timebase master,
 *  so we are a slave.
 *
 *      jack_set_timebase_callback(..., , jack_timebase_callback, ...);
 */

void
jack_assistant::update_timebase_master (jack_transport_state_t s)
{
    if (s != JackTransportStopped)
    {
        if (m_timebase_tracking > 0)
        {
            --m_timebase_tracking;
            if (m_timebase_tracking == 0)
                m_timebase = timebase::slave;
        }
    }
    if (m_timebase_tracking == 0 && ! (m_jack_pos.valid & JackPositionBBT))
    {
        m_timebase_tracking = -1;
        m_timebase = timebase::none;            /* i.e. a regular client    */
    }
    else if (m_timebase_tracking < 0 && (m_jack_pos.valid & JackPositionBBT))
    {
        m_timebase_tracking = 0;
        m_timebase = timebase::slave;           /* external master exists   */
    }
}

#endif  // defined ENABLE_PROPOSED_FUNCTIONS

/**
 *  Shows a one-line summary of a JACK position structure.  This function is
 *  meant for experimenting and learning.
 *
 *  The fields of this structure are as follows.  Only the fields we care
 *  about are shown.
 *
\verbatim
    jack_nframes_t      frame_rate:     current frame rate (per second)
    jack_nframes_t      frame:          frame number, always present
    jack_position_bits_t valid:         which other fields are valid
\endverbatim
 *
\verbatim
JackPositionBBT:
    int32_t             bar:            current bar
    int32_t             beat:           current beat-within-bar
    int32_t             tick:           current tick-within-beat
    double              bar_start_tick
    float               beats_per_bar:  time signature "numerator"
    float               beat_type:      time signature "denominator"
    double              ticks_per_beat
    double              beats_per_minute
\endverbatim
 *
\verbatim
JackBBTFrameOffset:
    jack_nframes_t      bbt_offset;     frame offset for the BBT fields
\endverbatim
 *
 *  Only the most "important" and time-varying fields are shown. The format
 *  output is brief and inscrutable unless you read this format example:
 *
\verbatim
    nnnnn frame B:B:T N/D TPB BPM BBT
      ^     ^     ^   ^ ^  ^   ^   ^
      |     |     |   | |  |   |   |
      |     |     |   | |  |   |    -------- bbt_offset (frame), even if invalid
      |     |     |   | |  |    ------------ beats_per_minute
      |     |     |   | |   ---------------- ticks_per_beat (PPQN * 10?)
      |     |     |   |  ------------------- beat_type (denominator)
      |     |     |    --------------------- beats_per_bar (numerator)
      |     |      ------------------------- bar : beat : tick
      |      ------------------------------- frame (number)
       ------------------------------------- the "valid" bits
\endverbatim
 *
 *  The "valid" field is shown as bits in the same bit order as shown here, but
 *  represented as a five-character string, "nnnnn", n = 0 or 1:
 *
\verbatim
    JackVideoFrameOffset = 0x100
    JackAudioVideoRatio  = 0x080
    JackBBTFrameOffset   = 0x040
    JackPositionTimecode = 0x020
    JackPositionBBT      = 0x010
\endverbatim
 *
 *  We care most about nnnnn = "00101" in our experiments (the most common
 *  output will be "00001").  And we don't worry about non-integer
 *  measurements... we truncate them to integers.  Change the output format if
 *  you want to play with non-Western timings.
 *
 * \param pos
 *      The JACK position structure to dump.
 */

void
jack_assistant::show_position (const jack_position_t & pos)
{
    char temp[80];
    std::string nnnnn = "00000";
    if (pos.valid & JackVideoFrameOffset)
        nnnnn[0] = '1';

    if (pos.valid & JackAudioVideoRatio)
        nnnnn[1] = '1';

    if (pos.valid & JackBBTFrameOffset)
        nnnnn[2] = '1';

    if (pos.valid & JackPositionTimecode)
        nnnnn[3] = '1';

    if (pos.valid & JackPositionBBT)
        nnnnn[4] = '1';

    snprintf
    (
        temp, sizeof temp, "%s %8ld %03d:%d:%04d %d/%d %5d %3d %d",
        nnnnn.c_str(), long(pos.frame),
        int(pos.bar), int(pos.beat), int(pos.tick),
        int(pos.beats_per_bar), int(pos.beat_type),
        int(pos.ticks_per_beat), int(pos.beats_per_minute),
        int(pos.bbt_offset)
    );
    infoprint(temp);                    /* no output in release mode */
}

/**
 *  A member wrapper function for the new free function create_jack_client().
 *
 * \param name
 *      Provides the name of the client, used in the call to
 *      create_jack_client().  By default, this name is the macro SEQ66_PACKAGE
 *      (i.e.  "seq66").
 *
 * \return
 *      Returns a pointer to the JACK client if JACK has opened the client
 *      connection successfully.  Otherwise, a null pointer is returned.
 *      The caller must handle a bad result.
 */

jack_client_t *
jack_assistant::client_open (const std::string & name)
{
    return create_jack_client(name, rc().jack_session());
}

/**
 *  This function gets the current JACK position.  The Seq32 version also uses
 *  its tempo map to adjust this, but Seq66 currently does not.
 *
 *  The original equation is:
 *
\verbatim
            frame * Tpb * Bpbar * ppqn * 4
    tick = ---------------------------------
                 60 * FR * Tpb * Bw
\endverbatim
 *
 *  which simplifies to:
 *
\verbatim
            frame * Bpbar * ppqn
    tick = -----------------------
                15 * FR * Bw
\endverbatim
 *
 * \warning
 *      Currently valgrind flags j->client() as uninitialized.
 *
 * \param arg
 *      Provides the putative jack_assistant pointer, assumed to be not null.
 *
 * \return
 *      Returns the calculated tick position if no errors occur.  Otherwise,
 *      returns 0.
 */

long
jack_assistant::current_jack_position () const
{
    if (not_nullptr(client()))
    {
        double ppqn = double(get_ppqn());
        double Bpbar = beats_per_measure();
        double Bw = beat_width();
        jack_nframes_t frame = jack_get_current_transport_frame(client());
        double tick2 = frame * Bpbar * ppqn / (15.0 * jack_frame_rate() * Bw);

        /*
         * double Tpb = ppqn * c_jack_factor;          // ticks/beat
         * double tick = frame * Tpb * Bpbar / (jack_frame_rate() * 60.0);
         * tick *= (ppqn / (Tpb * Bw / 4.0));
         * printf("tick = %f; tick2 = %f\n", tick, tick2);
         */

        return tick2;
    }
    else
    {
        error_message("Null JACK transport client");
        return 0;
    }
}

/*
 *  JACK callbacks.
 */

/**
 *  The JACK timebase function defined here sets the JACK position structure.
 *  The original version of the function worked properly with Hydrogen, but
 *  not with Klick.  The new code seems to work with both.  More testing and
 *  clarification is needed.  This new code was "discovered" in the
 *  "SooperLooper" project: http://essej.net/sooperlooper/
 *
 *  The first difference with the new code is that it handles the case where
 *  the JACK position is moved (new_pos == true).  If this is true, and the
 *  JackPositionBBT bit is off in pos->valid, then the new BBT value is set.
 *
 *  The second set of differences are in the "else" clause.  In the new code,
 *  it is very simple: calculate the new tick value, back it off by the number
 *  of ticks in a beat, and perhaps go to the first beat of the next bar.
 *
 *  In the old code (complex!), the simple BBT adjustment is always made.
 *  This changes (perhaps) the beats_per_bar, beat_type, etc.  We
 *  need to make these settings use the actual global values for beats set for
 *  Seq66.  Then, if transitioning from JackTransportStarting to
 *  JackTransportRolling (instead of checking new_pos!), the BBT values (bar,
 *  beat, and tick) are finally adjusted.  Here are the steps, with old and new
 *  steps noted:
 *
 *      -#  Calculate the "delta" ticks based on the current frame, the
 *          ticks_per_beat, the beats_per_minute, and the frame_rate.  The old
 *          code saves this in a local, the new code assigns it to pos->tick.
 *      -#  Old code: save this delta as a positive value.
 *      -#  Figure out the settings and modify bar, beat, tick, and
 *          bar_start_tick.  The old and new code seem to have the same intent,
 *          but it seems like the new code is faster and also correct.
 *          -   Old code:  Calculations are made by division and mod
 *              operations.
 *          -   New code:  Calculations are made by increments and decrements
 *              in a while loop.
 *
 * Stazed:
 *
 *  The call to jack_timebase_callback() to supply JACK with BBT, etc. would
 *  occasionally fail when the pos information had zero or some garbage in the
 *  pos.frame_rate variable. This would occur when there was a rapid change of
 *  frame position by another client... i.e. qjackctl.  From the JACK API:
 *
 *      pos: address of the position structure for the next cycle; pos->frame
 *      will be its frame number. If new_pos is FALSE, this structure contains
 *      extended position information from the current cycle.  If TRUE, it
 *      contains whatever was set by the requester.  The timebase_callback's
 *      task is to update the extended information here."
 *
 *  The "If TRUE" line seems to be the issue. It seems that qjackctl does not
 *  always set pos.frame_rate so we get garbage and some strange BBT
 *  calculations that display in qjackctl. So we need to set it here and just
 *  use m_frame_rate for calculations instead of pos.frame_rate.
 *
 * \param state
 *      Indicates the current state of JACK transport.
 *
 * \param nframes
 *      The number of JACK frames in the current time period.
 *
 * \param pos
 *      Provides the position structure to be filled in, the
 *      address of the position structure for the next cycle; pos->frame will
 *      be its frame number. If new_pos is FALSE, this structure contains
 *      extended position information from the current cycle. If TRUE, it
 *      contains whatever was set by the requester. The timebase_callback's
 *      task is to update the extended information here.
 *
 * \param new_pos
 *      TRUE (non-zero) for a newly requested pos, or for the first cycle
 *      after the timebase_callback is defined.  This is usually 0 in
 *      Seq66 at present, and 1 if one, say, presses "rewind" in
 *      qjackctl.
 *
 * \param arg
 *      Provides the jack_assistant pointer, currently unchecked for nullity.
 */

void
jack_timebase_callback
(
    jack_transport_state_t /*state*/,
    jack_nframes_t nframes,
    jack_position_t * pos,
    int new_pos,
    void * arg
)
{
    jack_assistant * jack = static_cast<jack_assistant *>(arg);
    pos->beats_per_minute = jack->get_beats_per_minute();   /* sooperlooper */
    pos->beats_per_bar = jack->beats_per_measure();
    pos->beat_type = jack->beat_width();
    pos->ticks_per_beat = jack->get_ppqn() * double(c_jack_factor);

    long ticks_per_bar = long(pos->ticks_per_beat * pos->beats_per_bar);
    long ticks_per_minute = long(pos->beats_per_minute * pos->ticks_per_beat);
    double framerate = double(pos->frame_rate * 60.0);
    bool not_bbt = ! (pos->valid & JackPositionBBT);
    if (new_pos || not_bbt)
    {
        /*
         * This code is hit at Start and Stop actions from all clients.
         */

        double minute = pos->frame / framerate;
        long abs_tick = long(minute * ticks_per_minute);
        long abs_beat = long(abs_tick / pos->ticks_per_beat);
        pos->bar = int(abs_beat / pos->beats_per_bar);
        pos->beat = int(abs_beat - (pos->bar * pos->beats_per_bar) + 1);
        pos->tick = int(abs_tick - (abs_beat * pos->ticks_per_beat));
        pos->bar_start_tick = int(pos->bar * ticks_per_bar);
        pos->bar++;                             /* adjust start to bar 1 */
    }
    else
    {
        /*
         * With this code, computing the BBT based on the previous period,
         * "klick -j -P" follows Seq66 when it is JACK Master!
         */

        int delta_tick = int(nframes * ticks_per_minute / framerate);
        pos->tick += delta_tick;
        while (pos->tick >= pos->ticks_per_beat)
        {
            pos->tick -= int(pos->ticks_per_beat);
            if (++pos->beat > pos->beats_per_bar)
            {
                pos->beat = 1;
                ++pos->bar;
                pos->bar_start_tick += ticks_per_bar;
            }
        }
        if (jack->is_master())
            pos->beats_per_minute = jack->parent().get_beats_per_minute();
    }
    pos->bbt_offset = 0;
    pos->valid = (jack_position_bits_t)
    (
        pos->valid | JackBBTFrameOffset | JackPositionBBT
    );
}

/**
 *  This callback is to shut down JACK by clearing the jack_assistant ::
 *  m_jack_running flag.
 *
 * \param arg
 *      Points to the jack_assistant in charge of JACK support for the performer
 *      object.
 */

void
jack_transport_shutdown (void * arg)
{
    jack_assistant * jack = static_cast<jack_assistant *>(arg);
    if (not_nullptr(jack))
    {
        jack->set_jack_running(false);
        info_message("JACK transport shutdown");
    }
    else
    {
        (void) error_message("null JACK transport pointer");
    }
}

/**
 *  Converts a JACK transport value to a human-readable string.
 *
 * \param state
 *      Provides the transport state value.
 *
 * \return
 *      Returns the state name.
 */

std::string
jack_state_name (const jack_transport_state_t & state)
{
    std::string result;
    switch (state)
    {
    case JackTransportStopped:

        result = "JackTransportStopped";
        break;

    case JackTransportRolling:

        result = "JackTransportRolling";
        break;

    case JackTransportStarting:

        result = "JackTransportStarting";
        break;

    case JackTransportLooping:

        result = "JackTransportLooping";
        break;

    default:

        errprint("JackTransportUnknown");
        break;
    }
    return result;
}

#endif      // SEQ66_JACK_SUPPORT

/*
 * -------------------------------------------------------------------------
 *  JACK scratch-pad
 * -------------------------------------------------------------------------
 */

jack_scratchpad::jack_scratchpad () :
    js_current_tick         (0.0),
    js_total_tick           (0.0),
    js_clock_tick           (0.0),
    js_jack_stopped         (false),
    js_dumping              (false),
    js_init_clock           (true),
    js_looping              (false),
    js_playback_mode        (false),
    js_ticks_converted      (0.0),
    js_ticks_delta          (0.0),
    js_ticks_converted_last (0.0),
    js_delta_tick_frac      (0L)
{
    // No other code
}

void
jack_scratchpad::initialize
(
    midipulse currenttick,
    bool islooping,
    bool songmode
)
{
    js_current_tick         = double(currenttick);
    js_total_tick           = 0.0;
    js_clock_tick           = 0.0;
    js_jack_stopped         = false;
    js_dumping              = false;
    js_init_clock           = true;
    js_looping              = islooping;
    js_playback_mode        = songmode;
    js_ticks_converted      = 0.0;
    js_ticks_delta          = 0.0;
    js_ticks_converted_last = 0.0;
    js_delta_tick_frac      = 0L;
}

void
jack_scratchpad::set_current_tick (midipulse curtick)
{
    double ct = double(curtick);
    js_current_tick = js_total_tick = js_clock_tick = ct;
}

void
jack_scratchpad::set_current_tick_ex (midipulse curtick)
{
    double ct = double(curtick);
    js_current_tick = js_total_tick = js_clock_tick =
        js_ticks_converted_last = ct;
}

void
jack_scratchpad::add_delta_tick (midipulse deltick)
{
    double dt = double(deltick);
    js_current_tick += dt;
    js_total_tick += dt;
    js_clock_tick += dt;
    js_dumping = true;
}

}           // namespace seq66

/*
 * jack_assistant.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


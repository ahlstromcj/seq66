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
 * \file          performer.cpp
 *
 *  This module defines the base class for the performer of MIDI patterns.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom and others
 * \date          2018-11-12
 * \updates       2022-07-27
 * \license       GNU GPLv2 or above
 *
 *  Also read the comments in the Seq64 version of this module, perform.
 *  This class is probably the single most important class in Seq66, as it
 *  supports sequences, mute-groups, sets, playback, JACK, and more.
 *
 *  The automation slots supported are defined in the enumeration seq66 ::
 *  automation :: slot. Their human readable names are defined in opcontrol ::
 *  automation_slot_name (). Their default keystrokes are defined in
 *  keycontainer :: keys_automation (). Their internal name are defined in the
 *  automation.cpp module, in the static array s_slotnamelist[]. The
 *  automation call-back functions are defined in this module, the performer
 *  module.  More information is included in the user's manual, in section
 *  9.5.7 "'ctrl' File / Keyboard / Default Assignments" and in the
 *  Libreoffice spreadsheets in the "doc" directory, which may be out-of-date.
 *
 *  Keystrokes versus MIDI controls:  MIDI can support toggle, on, and off
 *  actions.  Keystrokes can only be pressed and released. Each keystroke can
 *  be used for a toggle, which should be triggered on a press event or a
 *  release event, but not both.  A keystroke's press event can alos be used
 *  for an on, and the release event can be used for an off.  These two modes
 *  of operation depend on the slot(s) involved.
 *
 *  Playscreen vs screenset in Seq24:
 *
 *      m_playing_screen is used in:
 *
 *      -   select_group_mute().  Sets the selected mute group number and stores
 *          the mute group if learn is active.  Used in midifile, optionsfile,
 *          and userfile.
 *      -   select_mute_group(). Almost the same in a stilted way, but also
 *          saves the state of the mute group in a small set array, "tracks
 *          mute".  Used indirectly in mainwnd to "activate" the desired
 *          mute-group.
 *      -   mute_group_tracks(). If in group mode, sets the sequences according
 *          to the state in "tracks mute".
 *      -   set_playing_screenset(). Sets "tracks mute" per the current
 *          playing screen.  Changes the playing screen to the current screen
 *          set, then calls mute_group_tracks().  Called by MIDI control
 *          "play_ss", and the Home key handling in mainwnd.
 *      -   sequence_playing_on() and _off(). If in group mode and the playing
 *          set is the screen set, sets "tracks mute" for that sequence.
 *
 * Playback/recording coordination via condition variables:
 *
 *      -   performer().  Create an seq66:condition instance "c".
 *      -   inner_start().
 *          -#  c.lock(), which locks the mutex.
 *          -#  Start running and flag it.
 *          -#  c.signal(), which calls notify_one().
 *          -#  c.unlock(), which unlocks the mutex.
 *      -   output_func().
 *          -#  c.lock(), which locks the mutex.
 *          -#  While not running, c.wait() [on the signal].
 *          -#  Once signalled, if not outputing, we are stopping.
 *          -#  c.unlock(), which unlocks the mutex.
 *      -   ~ performer().
 *          -#  c.signal()
 *          -#  Join the output and input threads.
 *
 * MIDI CLOCK Support:
 *
 *    MIDI beat clock (MIDI timing clock or MIDI clock) is a clock signal that
 *    is broadcast via MIDI to ensure that several MIDI-enabled devices such as
 *    a synthesizer or music sequencer stay in synchronization.  MIDI beat clock
 *    is tempo-dependent. Clock events are sent at a rate of 24 times every
 *    quarter note. Those pulses maintain a synchronized tempo for synthesizers
 *    with BPM-dependent voices, and for arpeggiator synchronization. Location
 *    information is specified using the Song Position Pointer (SPP) although
 *    many simple MIDI devices ignore this message.  Because of limitations in
 *    MIDI and synthesizers, devices driven by MIDI beat clock are often subject
 *    to clock drift.
 *
 *    On output:
 *
 *    -   perform::m_usemidiclock starts at false;
 *    -   It is set to false in pause_playing().
 *    -   It is set to the midiclock parameter of inner_stop().
 *    -   If m_usemidiclock is true:
 *        -   It affects m_midiclocktick in output.
 *        -   The position in output cannot be repositioned.
 *        -   The tick location cannot be changed.
 *
 *    On input:
 *
 *    -   If MIDI Start is received, m_midiclockrunning and m_usemidiclock
 *        become true, and m_midiclocktick and m_midiclockpos become 0.
 *    -   If MIDI Continue is received, m_midiclockrunning is set to true and
 *        we start according to song-mode.
 *    -   If MIDI Stop is received, m_midiclockrunning is set to false,
 *        m_midiclockpos is set to the current tick (!), all_notes_off(), and
 *        inner_stop(true) [sets m_usemidiclock = true].
 *    -   If MIDI Clock is received, and m_midiclockrunning is true, then
 *        m_midiclocktick += m_midiclockincrement.
 *    -   If MIDI Song Position is received, then m_midiclockpos is set as per
 *        in data in this event.
 *    -   MIDI Active Sense and MIDI Reset are currently filtered by the JACK
 *        implementation.
 *
 * Locking:
 *
 *      -#  The flags m_inputing and m_outputing start out true.
 *      -#  When the perform starts, the input thread starts.
 *      -#  When the perform starts, the output thread then starts.
 *      -#  The output thread then waits on the condition variable for
 *          inner_start() to set is_running() to true. It then proceeds to run
 *          forever.
 *      -#  In the destructor, the flags m_inputing and m_outputing are
 *          replaced with m_io_active,  set to false, and the condition
 *          variable is signalled.  This causes the output thread to exit.
 *          The input thread detects that m_io_active is false and exits.
 *      -#  The two threads are then joined.
 *
 * Threads:
 *
 *  https://eli.thegreenplace.net/2016/c11-threads-affinity-and-hyperthreading/
 *
 *  https://www.acodersjourney.com/top-20-cplusplus-multithreading-mistakes/
 *
 *      $ taskset -c 5,6 ./Seq66qt/qseq66 ...
 *
 *  https://www.glennklockwood.com/hpc-howtos/process-affinity.html
 *
 *      Assuming your executable is called application.x, see what cores each
 *      thread is using by issuing the following command in bash:
 *
 *      $ for i in $(pgrep application.x);
 *          do ps -mo pid,tid,fname,user,psr -p $i; done
 *
 *      The PSR field is the OS identifier for the core each TID (thread id)
 *      is utilizing.
 *
 * pthreads:
 *
 *      We were using sched_setscheduler(), but user gresade reported issue
 *      #179, with some jitter in playback/recording, and we noticed that the
 *      man page indicated to use the pthread_setsched_param function instead
 *      when using pthreads, which Sequencer64 does use in Linux.
 *
 *      http://ccrma.stanford.edu/planetccrma/software/understandlowlat.html:
 *
 *          Summarizing, you need tuned drivers that do not disable interrupts
 *          for long, low latency patches in the kernel so that the scheduler
 *          runs often enough and your application itself has to run with the
 *          SCHED_FIFO scheduling policy so that it gets the best chance of
 *          grabbing the processor when it needs it.
 *
 *          When everything is in place things work incredibly well. The
 *          system can be running an audio task with no dropouts and a few
 *          milliseconds of latency while the computer is being loaded with
 *          disk accesses, screen refreshes and whatnot. The mouse gets jerky,
 *          windows update very slowly but not a dropout to be heard.
 *
 *      http://www.informit.com/articles/article.aspx?p=101760&seqNum=4:
 *
 *          Two or more SCHED_FIFO tasks at the same priority run round robin.
 *          If a SCHED_FIFO task is runnable, all tasks at a lower priority
 *          cannot run until it finishes.
 *
 *          SCHED_RR is identical to SCHED_FIFO except that each process can
 *          only run until it exhausts a predetermined timeslice. That is,
 *          SCHED_RR is SCHED_FIFO with timeslices—it is a real-time
 *          round-robin scheduling algorithm.
 *
 *          Real-time priorities range inclusively from one to MAX_RT_PRIO
 *          minus one. By default, MAX_RT_PRIO is 100—therefore, the default
 *          real-time priority range is one to 99. This priority space is
 *          shared with the nice values of SCHED_OTHER tasks; they use the
 *          space from MAX_RT_PRIO to (MAX_RT_PRIO + 40). By default, this
 *          means the –20 to +19 nice range maps directly onto the 100 to 140
 *          priority range.
 *
 * Settings lifecycle:
 *
 *      First, note that performer will use only the "global" rcsettings
 *      object, as retrieved by the seq66::rc() function.  The same is true
 *      for seq66::usr().
 *
 *      -#  The static rcsettings value creates its own copy of the key and
 *          MIDI control containers.
 *          -#  In the keycontainer constructor, it calls its set_defaults()
 *              function to set up the default keystrokes.
 *          -#  The midicontrolin does not do this.  It remains empty.
 *      -#  The performer constructor creates its own key and MIDI control
 *          containers.  Again, only the keycontainer has default values in
 *          it.
 *      -#  In main(), we run parse_options_files(), which creates an rcfile
 *          object. If there is a "[midi-control-file]" section, it is parsed,
 *          otherwise the control data is parsed from the "rc" file.  This
 *          data goes into the "global" settings object, rc().
 *      -#  If parse_options_file() succeeds, then the performer gets the
 *          settings from rc(), and launches.
 *      -#  After ending, we get the latest settings from the performer, and
 *          copy them into the "global" rc().
 *      -#  The options are then written.
 *
 * Modify action:
 *
 *      A modify action is any change that would require the current MIDI tune
 *      to be saved before closing the application or loading a new MIDI tune.
 *      These actions include: a change in a song/pattern parameter setting;
 *      modification of the triggers in the Song editor; a change in output
 *      buss (though selection of the input buss is saved in the "rc" file);
 *      and anything else? When they occur, performer :: modify() is called.
 *
 *      One issue with modification is that we don't have comprehensive
 *      tracking of all "undo" operations, so that, once the modify flag is
 *      set, only saving the MIDI tune will unset it.  See the calls to
 *      performer :: unmodify().
 *
 *      Also note that some of the GUI windows have their own, unrelated,
 *      modify() function.
 */

#include <algorithm>                    /* std::find() for std::vector      */
#include <cmath>                        /* std::round()                     */
#include <iostream>                     /* std::cout                        */
#include <sstream>                      /* std::ostringstream               */

#include "cfg/mutegroupsfile.hpp"       /* seq66::mutegroupsfile            */
#include "cfg/notemapfile.hpp"          /* seq66::notemapfile               */
#include "cfg/playlistfile.hpp"         /* seq66::playlistfile              */
#include "cfg/settings.hpp"             /* seq66::rcsettings rc(), etc.     */
#include "ctrl/keystroke.hpp"           /* seq66::keystroke class           */
#include "midi/midifile.hpp"            /* seq66::read_midi_file()          */
#include "play/notemapper.hpp"          /* seq66::notemapper                */
#include "play/performer.hpp"           /* seq66::performer, this class     */
#include "os/daemonize.hpp"             /* seq66::signal_for_exit()         */
#include "os/timing.hpp"                /* seq66::microsleep(), microtime() */
#include "util/filefunctions.hpp"       /* seq66::filename_base(), etc.     */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  This value is the "trigger width" in microseconds, a Seq24 concept.
 *  It also had a "lookahead" time of 2 ms, not used however.
 */

static const int c_thread_trigger_width_us = 4 * 1000;

/**
 *  This high-priority value is used if the --priorit option is specified.
 *  Needs more testing, we really haven't needed it yet.
 */

static const int c_thread_priority = 1;

/**
 *  When operating a playlist, especially from a headless seq66cli run, and
 *  with JACK transport active, the change from a playing tune to the next
 *  tune would really jack up JACK, crashing the app (corrupted double-linked
 *  list, double frees in destructors, etc.) and sometimes leaving a loud tone
 *  buzzing.  So after we stop the current tune, we delay a little bit to
 *  allow JACK playback to exit.  See the delay_stop() member function and its
 *  usages herein.
 */

static const int c_delay_stop_ms = 100;

/**
 *  Indicates how much of a long file-path we will show using the
 *  shorten_file_spec() function.
 */

static const int c_long_path_max = 56;

/**
 *  Principal constructor.
 */

performer::performer (int ppqn, int rows, int columns) :
    m_smf_format            (1),
    m_error_pending         (false),
    m_play_set              (),
    m_play_list             (),
    m_note_mapper           (new notemapper()),
    m_song_start_mode       (sequence::playback::automatic),
    m_reposition            (false),
    m_excell_FF_RW          (1.0),
    m_FF_RW_button_type     (ff_rw::none),
    m_old_seqno             (seq::unassigned()),
    m_current_seqno         (seq::unassigned()),
    m_moving_seq            (),
    m_seq_clipboard         (),
    m_screenset_to_copy     (screenset::unassigned()),
    m_clocks                (),                 /* vector wrapper class     */
    m_inputs                (),                 /* vector wrapper class     */
    m_key_controls          ("Key controls"),
    m_midi_control_in       ("Perf ctrl in"),
    m_midi_control_out      ("Perf ctrl out"),
    m_mute_groups           ("Mute groups", rows, columns),
    m_operations            ("Performer ops"),
    m_set_master            (rows, columns),    /* 32 row x column sets     */
    m_set_mapper                                /* accessed via mapper()    */
    (
        m_set_master, m_mute_groups, rows, columns
    ),
    m_queued_replace_slot   (-1),               /* REFACTOR                 */
    m_no_queued_solo        (-1),               /* constant                 */
    m_transpose             (0),
    m_out_thread            (),
    m_in_thread             (),
    m_out_thread_launched   (false),
    m_in_thread_launched    (false),
    m_io_active             (false),            /* !done(), set in launch() */
    m_is_running            (false),
    m_is_pattern_playing    (false),
    m_needs_update          (true),
    m_is_busy               (false),            /* try this flag for now    */
    m_looping               (false),
    m_song_recording        (false),
    m_song_record_snap      (true),
    m_resume_note_ons       (usr().resume_note_ons()),
    m_ppqn                  (choose_ppqn(ppqn)),
    m_file_ppqn             (0),
    m_bpm                   (usr().midi_beats_per_minute()),
    m_resolution_change     (true),
    m_current_beats         (0),
    m_delta_us              (0),
    m_base_time_ms          (0),
    m_last_time_ms          (0),
    m_beats_per_bar         (usr().midi_beats_per_bar()),
    m_beat_width            (usr().midi_beat_width()),
    m_clocks_per_metronome  (24),
    m_32nds_per_quarter     (0),
    m_us_per_quarter_note   (0),
    m_master_bus            (),                 /* this is a shared pointer */
    m_filter_by_channel     (false),
    m_one_measure           (0),
    m_left_tick             (0),
    m_right_tick            (0),
    m_start_tick            (0),
    m_tick                  (0),
    m_max_extent            (0),
    m_jack_pad              (),                 /* data for JACK... & ALSA  */
    m_jack_tick             (0),
    m_usemidiclock          (false),            /* MIDI Clock support       */
    m_midiclockrunning      (false),
    m_midiclocktick         (0),
    m_midiclockincrement    (clock_ticks_from_ppqn(m_ppqn)),
    m_midiclockpos          (0),
    m_dont_reset_ticks      (false),            /* support for pausing      */
    m_is_modified           (false),
    m_selected_seqs         (),
    m_condition_var         (*this),            /* private access via cv()  */
#if defined SEQ66_JACK_SUPPORT
    m_jack_asst
    (
        *this, usr().bpm_default(),             /* beats per minute         */
        m_ppqn, usr().bpb_default(),            /* beats per bar (measure)  */
        usr().bw_default()                      /* beat width (denominator) */
    ),
#endif
    m_have_undo             (false),
    m_undo_vect             (),
    m_have_redo             (false),
    m_redo_vect             (),
    m_notify                (),
    m_signalled_changes     (! usr().app_is_headless()),
    m_seq_edit_pending      (false),
    m_event_edit_pending    (false),
    m_pending_loop          (seq::unassigned()),
    m_slot_shift            (0),
    m_hidden                (false),
    m_show_hide_pending     (false)
{
    /*
     * Generally will be parsing the 'rc' files after creating the performer.
     * (void) get_settings(rc(), usr());
     */

    (void) populate_default_ops();
}

/**
 *  The destructor sets some running flags to false, signals this condition,
 *  then joins the input and output threads if the were launched.
 *
 *  A thread that has finished executing code, but has not yet been joined is
 *  still considered an active thread of execution and is therefore joinable.
 */

performer::~performer ()
{
    (void) finish();                    /* sets m_io_active to false, etc.  */
}

/**
 *  Register a class that inherits from performer::callbacks to be notified of
 *  various happenings.
 */

void
performer::enregister (callbacks * pfcb)
{
    if (not_nullptr(pfcb))
    {
        auto it = std::find(m_notify.begin(), m_notify.end(), pfcb);
        if (it == m_notify.end())
            m_notify.push_back(pfcb);
    }
}

/**
 *  Removes a class from the notification list.  Used in transitory windows
 *  and frames that need notification.
 */

void
performer::unregister (callbacks * pfcb)
{
    if (not_nullptr(pfcb))
    {
        auto it = std::find(m_notify.begin(), m_notify.end(), pfcb);
        if (it != m_notify.end())
            (void) m_notify.erase(it);
    }
}

void
performer::set_error_message (const std::string & msg)
{
    m_error_pending = true;
    error_message("performer", msg);
}

void
performer::unmodify ()
{
    m_is_modified = false;                  /* m_needs_update = false;  */
    mapper().unmodify_all_sequences();
}

/**
 *  This improved version checks all of the sequences. This allow the user to
 *  unmodify a sequence without using performer::modify(). First usage of
 *  sequence::modify() and unmodify() is in qpatternfix.
 */

bool
performer::modified () const
{
    bool result = m_is_modified;
    if (result)
    {
        /* printf("MODIFIED\n");   // just for debugging */
    }
    else
        result = mapper().any_modified_sequences();

    return result;
}

void
performer::notify_automation_change (automation::slot s)
{
    for (auto notify : m_notify)
        (void) notify->on_automation_change(s);
}

/*
 *  Note that we need to call modify() before telling the subscribers, so that
 *  they can check the status of the performer.  This is not strictly
 *  necessary, but some subscribers still call performer::modified() instead
 *  of using the parameter.
 */

void
performer::notify_set_change (screenset::number setno, change mod)
{
    if (changed(mod))
        modify();

    for (auto notify : m_notify)
        (void) notify->on_set_change(setno, mod);
}

void
performer::notify_mutes_change (mutegroup::number mutesno, change mod)
{
    for (auto notify : m_notify)
        (void) notify->on_mutes_change(mutesno);

    if (mod == change::yes)
        modify();
}

/**
 *  Called by qseqeventframe.  This function will eventually cause a call to
 *  recreate all the slot buttons in qslivegrid, and when qslivegrid ::
 *  refresh() is called, it can find all the buttons deleted.
 */

void
performer::notify_sequence_change (seq::number seqno, change mod)
{
    bool redo = mod == change::recreate;
    if (mod == change::yes || redo)
        modify();

    for (auto notify : m_notify)
        (void) notify->on_sequence_change(seqno, mod);
}

/**
 *  This notification currently does not cause a modify action.
 */

void
performer::notify_ui_change (seq::number seqno, change /*mod*/)
{
    for (auto notify : m_notify)
        (void) notify->on_ui_change(seqno);
}

void
performer::notify_trigger_change (seq::number seqno, change mod)
{
    for (auto notify : m_notify)
        (void) notify->on_trigger_change(seqno);

    if (mod == change::yes)
    {
        modify();
    }
    else if (mod == change::no)
    {
        if (seq_in_playing_screen(seqno))
        {
            const seq::pointer s = get_sequence(seqno);
            seqno %= screenset_size();
            announce_sequence(s, seqno);
        }
    }
}

/**
 *  Allows notification of changes in the PPQN and tempo (beats-per-minute,
 *  BPM).
 */

void
performer::notify_resolution_change (int ppqn, midibpm bpm, change mod)
{
    m_resolution_change = true;
    for (auto notify : m_notify)
        (void) notify->on_resolution_change(ppqn, bpm);

    if (mod == change::yes)
        modify();
}

/**
 *  Notifies when the use selects a new song or playlist.
 *
 * \param signalit
 *      If true, emit a signal, to avoid conflict with the GUI.
 */

void
performer::notify_song_action (bool signalit, playlist::action act)
{
    for (auto notify : m_notify)
        (void) notify->on_song_action(signalit, act);
}

/*
 * -------------------------------------------------------------------------
 *  Settings Get/Put
 * -------------------------------------------------------------------------
 */

/**
 *  Gets the settings and applies them to the performer.  The clocks and input
 *  settings will eventually be copied to the mastermidibus, which might change
 *  them due to changes in plugged devices.
 *
 *  The clocks and inputs values will later be updated wth the masterbus
 *  clocks and inputs as retrieved at run-time.  Generally, we need at least
 *  one output device, or we will fail.
 *
 *  WHAT ABOUT THE PLAYLIST?
 *
 * \note
 *      Playlist filename is handled by rcsettings, but the playlist itself is
 *      handled by the performer.
 *
 * \param rcs
 *      Provides the source of the settings.
 *
 * \return
 *      Returns true if all of the settings were obtained.  However, it isn't
 *      necessarily an error.  Something to think about.
 */

bool
performer::get_settings (const rcsettings & rcs, const usrsettings & usrs)
{
    int buses = rcs.clocks().count();
    int inputs = rcs.inputs().count();
    bool result = buses > 0;                        /* at least 1 output    */
    if (result)
    {
        m_clocks = rcs.clocks();
        if (inputs > 0)
            m_inputs = rcs.inputs();

        /*
         * At this point, the names are not yet set in the clocks and inputs.
         *
         *      result = build_output_port_map(m_clocks);
         */
    }
    if (rcs.manual_ports())
    {
        inputslist & ipm = input_port_map();
        clockslist & opm = output_port_map();
        ipm.active(false);
        opm.active(false);
    }

    int kcount = rcs.key_controls().count();
    int micount = rcs.midi_control_in().count();
    int moacount = rcs.midi_control_out().action_count();
    int momcount = rcs.midi_control_out().macro_count();
    if (kcount > 0)
        m_key_controls = rcs.key_controls();

    msgprintf
    (
        msglevel::status,
        "Controls: %d keys; %d MIDI in; %d automation displays; %d macros",
        kcount, micount, moacount, momcount
    );

    /*
     * We need to copy the MIDI input controls whether the user has enabled
     * them or not.  Otherwise, the controls are replaced by the defaults
     * during the 'ctrl' file save at exit, which is surprising to the poor
     * user.  See issue #47.
     */

    m_midi_control_in = rcs.midi_control_in();
    if (micount == 0 && kcount > 0)
        m_midi_control_in.add_blank_controls(m_key_controls);

    m_midi_control_out = rcs.midi_control_out();

    const std::string & mgf = rc().mute_group_filespec();
    (void) open_mutegroups(mgf);
    if (! rc().song_start_auto())
        song_start_mode(rcs.get_song_start_mode());      /* force the mode   */

    filter_by_channel(rcs.filter_by_channel());

    /*
     * tempo_track_number(rcs.tempo_track_number());    // [midi-meta-events]
     */

    m_resume_note_ons = usrs.resume_note_ons();
    return result;
}

/**
 *  Copies the settings to an external settings object.
 *
 *  The clocks and input settings might be modified by mastermidibus.
 *  Therefore, we refill these containers before passing them back to
 *  rcsettings.
 *
 *  Please note that we will upgrade mastermidibus to use the clockslist and
 *  inputslist classes, rather than accessing the vectors directly.
 *
 * \param rcs
 *      Provides the destination for the settings.
 *
 * \return
 *      Returns true if the settings were proper and were copied.
 */

bool
performer::put_settings (rcsettings & rcs, usrsettings & usrs)
{
    /*
     * We cannot allow changes made outside of the Preferences GUI to
     * be saved (e.g the Live/Song button in the main window).
     *
     *  bool pb = song_mode();
     *  rcs.song_start_mode(pb);
     */

    if (m_master_bus)
    {
        m_master_bus->get_port_statuses(m_clocks, m_inputs);
        rcs.clocks() = m_clocks;
        rcs.inputs() = m_inputs;
    }
    rcs.key_controls() = m_key_controls;
    rcs.midi_control_in() = m_midi_control_in;
    rcs.midi_control_out() = m_midi_control_out;
    if (mutes().is_modified())
    {
        const std::string & mgf = rc().mute_group_filespec();
        (void) save_mutegroups(mgf);
    }
    rcs.filter_by_channel(m_filter_by_channel);
    usrs.resume_note_ons(m_resume_note_ons);

    /*
     * We also need to update the playlist file-name in case the user loaded
     * or removed the playlist.
     */

    rcs.playlist_filename(playlist_filename());
    rcs.playlist_active(playlist_active());
    return true;
}

void
performer::playlist_filename (const std::string & basename)
{
    if (name_has_path(basename))
    {
        m_play_list->file_name(basename);
    }
    else
    {
        rc().playlist_filename(basename);
        m_play_list->file_name(rc().playlist_filespec());
    }
}

/**
 *  Reloads the mute groups from the "mutes" file.
 *
 * \param errmessage
 *      A pass-back parameter for any error message the file-processing might
 *      cause.
 *
 * \return
 *      Returns true if the reload succeeded.
 */

bool
performer::reload_mute_groups (std::string & errmessage)
{
    const std::string filename = rc().mute_group_filespec();
    bool result = open_mutegroups(filename);
    if (result)
    {
        result = get_settings(rc(), usr());     /* re-investigate usage     */
    }
    else
    {
        std::string msg = filename;
        msg += ": reading mutes failed";
        errmessage = msg;
        set_error_message(errmessage);          /* show it on the console   */
    }
    return result;
}

bool
performer::ui_get_input (bussbyte bus, bool & active, std::string & n) const
{
    const inputslist & ipm = input_port_map();
    bool disabled = false;
    std::string name;
    std::string alias;
    if (ipm.active())
    {
        name = ipm.get_name(bus);       // , rc().port_naming());
        alias = ipm.get_alias(bus, rc().port_naming());
        active = ipm.get(bus);
        disabled = ipm.is_disabled(bus);
    }
    else if (master_bus())
    {
        /*
         * Should we do this in one call?
         */

        name = master_bus()->get_midi_bus_name(bus, midibase::io::input);
        alias = master_bus()->get_midi_alias(bus, midibase::io::input);
        active = master_bus()->get_input(bus);
    }
    if (! alias.empty())
    {
        name += " (";
        name += alias;
        name += ")";
    }
    n = name;
    return ! name.empty() && ! disabled;
}

/**
 *  Sets the main input bus, and handles the special "key labels on sequence"
 *  and "sequence numbers on sequence" functionality.  This function is called
 *  by qinputcheckbox :: input_callback_clicked().  Note that the
 *  mastermidibus :: set_input() function passes the setting along to the
 *  input busarray.
 *
 * \param bus
 *      Provides the buss number, less than c_busscount_max, not checked.
 *
 * \param active
 *      Indicates whether the buss or the user-interface feature is active or
 *      inactive.
 */

bool
performer::ui_set_input (bussbyte bus, bool active)
{
    bussbyte truebus = true_input_bus(bus);
    bool result = m_master_bus->set_input(truebus, active);
    if (result)
    {
        inputslist & ipm = input_port_map();
        if (ipm.active())
            result = ipm.set(bus, active);

        set_input(bus, active);
        mapper().set_dirty();
    }
    return result;
}

bool
performer::ui_get_clock (bussbyte bus, e_clock & e, std::string & n) const
{
    const clockslist & opm = output_port_map();
    std::string name;
    std::string alias;
    if (opm.active())
    {
        name = opm.get_name(bus);       // , rc().port_naming());
        alias = opm.get_alias(bus, rc().port_naming());
        e = opm.get(bus);
    }
    else if (master_bus())
    {
        name = master_bus()->get_midi_bus_name(bus, midibase::io::output);
        alias = master_bus()->get_midi_alias(bus, midibase::io::output);
        e = master_bus()->get_clock(bus);
    }
    if (! alias.empty())
    {
        name += " (";
        name += alias;
        name += ")";
    }
    n = name;
    return ! name.empty();
}

/**
 *  Sets the clock value, as specified in the Preferences / MIDI Clocks tab.
 *  Note that the call to mastermidibus::set_clock() also sets the clock in
 *  the output busarray.
 *
 * \param bus
 *      The bus index to be set.  It is converted to the actual bus number, if
 *      necessary.
 *
 * \param clocktype
 *      Indicates whether the buss or the user-interface feature is
 *      e_clock_off, e_clock_pos, e_clock_mod, or (new) e_clock_disabled.
 */

bool
performer::ui_set_clock (bussbyte bus, e_clock clocktype)
{
    bussbyte truebus = true_output_bus(bus);
    bool result = m_master_bus->set_clock(truebus, clocktype);
    if (result)
    {
        clockslist & opm = output_port_map();
        if (opm.active())
            result = opm.set(bus, clocktype);

        set_clock(bus, clocktype);
        mapper().set_dirty();
    }
    return result;
}

/*
 * -------------------------------------------------------------------------
 *  Labeling Functions
 * -------------------------------------------------------------------------
 */

/**
 *  Provides a way to format the sequence parameters string for display in the
 *  mainwnd or perfnames modules.  This string goes on the bottom-left of
 *  those user-interface elements.
 *
 *  The format of this string is something like the following example.  The
 *  values shown are: sequence number, buss number, channel number, beats per
 *  bar, and beat width.
 *
\verbatim
        9  31-16 4/4
\endverbatim
 *
 *  The sequence number and buss number are re 0, while the channel number is
 *  displayed re 1, unless it is an SMF 0 null channel (0xFF), in which case
 *  it is 0.
 *
 *          "%-3d%d-%d %d/%d"  (old)
 *
 * \note
 *      Later, we could add the sequence hot-key to this string, though
 *      showing that is not much use in perfnames.  Also, this function is a
 *      stilted mix of direct access and access through sequence number.
 *
 * \param seq
 *      Provides the reference to the sequence, use for getting the sequence
 *      parameters to be written to the label string.
 *
 * \return
 *      Returns the filled in label if the sequence is active.
 *      Otherwise, an empty string is returned.
 */

std::string
performer::sequence_label (seq::cref seq) const
{
    std::string result;
    int sn = seq.seq_number();
    if (is_seq_active(sn))
    {
        bussbyte bus = seq.seq_midi_bus();
        int chan = seq.is_smf_0() ? 0 : int(seq.seq_midi_channel()) + 1;
        int bpb = int(seq.get_beats_per_bar());
        int bw = int(seq.get_beat_width());
        char tmp[32];
        snprintf(tmp, sizeof tmp, "%-3d %d-%d %d/%d", sn, bus, chan, bpb, bw);
        result = std::string(tmp);
    }
    return result;
}

/**
 *  A pass-through to the other sequence_label() function.
 *
 * \param seq
 *      Provides the reference to the sequence, use for getting the sequence
 *      parameters to be written to the label string.
 *
 * \return
 *      Returns the filled in label if the sequence is active.
 *      Otherwise, an empty string is returned.
 */

std::string
performer::sequence_label (seq::number seqno) const
{
    const seq::pointer s = get_sequence(seqno);
    return s ? sequence_label(*s) : std::string("") ;
}

/**
 *  Creates the sequence title, adjusting it for scaling down.  This title is
 *  used in the slots to show the (possibly shortened) pattern title. Note
 *  that the sequence title will also show the sequence length, in measures.
 *
 * \param seq
 *      Provides the reference to the sequence, use for getting the sequence
 *      parameters to be written to the label string.
 *
 * \return
 *      Returns the filled in label if the sequence is active.
 *      Otherwise, an empty string is returned.
 */

std::string
performer::sequence_title (seq::cref seq) const
{
    std::string result;
    int sn = seq.seq_number();
    if (is_seq_active(sn))
    {
        char temp[16];
        const char * fmt = usr().window_scaled_down() ? "%.11s" : "%.14s" ;
        snprintf(temp, sizeof temp, fmt, seq.title().c_str());
        result = std::string(temp);
    }
    return result;
}

/**
 *  Creates a sequence ("seqedit") window title, a longer version of
 *  sequence_title().
 *
 * \param seq
 *      Provides the reference to the sequence, use for getting the sequence
 *      parameters to be written to the string.
 *
 * \return
 *      Returns the filled in label if the sequence is active.
 *      Otherwise, an incomplete string is returned.
 *
 */

std::string
performer::sequence_window_title (seq::cref seq) const
{
    std::string result = seq_app_name();
    int sn = seq.seq_number();
    if (is_seq_active(sn))
    {
        int ppqn = seq.get_ppqn();
        char temp[32];
        snprintf(temp, sizeof temp, " (%d ppqn)", ppqn);
        result += " #";
        result += seq.seq_number_string();
        result += " \"";
        result += sequence_title(seq);
        result += "\"";
        result += temp;
    }
    else
    {
        result += "[inactive]";
    }
    return result;
}

/**
 *  Creates the main window title.  Unlike the disabled code in
 *  mainwnd::update_window_title(), this code does not (yet) handle
 *  conversions to UTF-8.
 *
 * \return
 *      Returns the filled-in main window title.
 */

std::string
performer::main_window_title (const std::string & filename) const
{
    std::string result = seq_package_name() + std::string(" ");
    std::string itemname = rc().no_name();
    if (filename.empty())
    {
        std::string fn = rc().midi_filename();
        if (! fn.empty())
        {

            std::string path;
            std::string name;
            if (filename_split(fn, path, name))
                itemname = name;
            else
                itemname = shorten_file_spec(fn, c_long_path_max);
        }
    }
    else
        itemname = filename;

    result += itemname;
    return result;
}

std::string
performer::pulses_to_measure_string (midipulse tick) const
{
    midi_timing mt(bpm(), get_beats_per_bar(), get_beat_width(), ppqn());
    return seq66::pulses_to_measurestring(tick, mt);
}

std::string
performer::pulses_to_time_string (midipulse tick) const
{
    return seq66::pulses_to_time_string(tick, bpm(), ppqn());
}

std::string
performer::client_id_string () const
{
    std::string result = seq_client_name();
    result += ':';
    if (rc().with_jack_midi() && ! rc().jack_session().empty())
        result += rc().jack_session();
    else
        result += std::to_string(m_master_bus->client_id());

    return result;
}

/*
 * -------------------------------------------------------------------------
 *  Sequence Creation/Installation
 * -------------------------------------------------------------------------
 */

/**
 *  A private helper function for add_sequence() and new_sequence().  It is
 *  common code and using it prevents inconsistencies.  It assumes values have
 *  already been checked.  It does not set the "is modified" flag, since
 *  adding a sequence by loading a MIDI file should not set it.  Compare
 *  new_sequence(), used by mainwnd and seqmenu, with add_sequence(), used by
 *  midifile.  This function *does not* delete the sequence already present
 *  with the given sequence number; instead, it keeps incrementing the
 *  sequence number until an open slot is found.
 *
 * \param seq
 *      The pointer to the pattern/sequence to add.
 *
 * \param seqno
 *      The sequence number of the pattern to be added.  Not validated, to
 *      save some time.  This is only the starting value; if already filled,
 *      then next open slot is used, and this value will be updated to the
 *      actual number.
 *
 * \param fileload
 *      If true (the default is false), the modify flag will not be set.
 *
 * \return
 *      Returns true if the sequence was successfully added.
 */

bool
performer::install_sequence (sequence * s, seq::number & seqno, bool fileload)
{
    bool result = mapper().install_sequence(s, seqno);
    if (result)
    {
        s->set_parent(this);                    /* also sets a lot of stuff */
        if (rc().is_setsmode_clear())           /* i.e. normal or auto-arm  */
        {
            /*
             * This code is wasteful.  It clears the playset and refills it
             * with the latest set of patterns in the screenset.
             */

            if (is_running())
                result = mapper().add_to_play_set(m_play_set, s);
            else
                result = mapper().fill_play_set(m_play_set);
        }
        else if (rc().is_setsmode_allsets())
        {
            /*
             * This code covers only allsets; the additive mode is in play when
             * changing the current set.
             */

            result = mapper().add_to_play_set(m_play_set, s);
        }
        if (! fileload)
            modify();
    }
    return result;
}

/**
 *  Creates a new pattern/sequence for the given slot, and sets the new
 *  pattern's master MIDI bus address.  Then it activates the pattern [this is
 *  done in the install_sequence() function].  It doesn't deal with thrown
 *  exceptions.
 *
 *  This function is called by the seqmenu and mainwnd objects to create a new
 *  sequence.  We now pass this sequence to install_sequence() to better
 *  handle potential memory leakage, and to make sure the sequence gets
 *  counted.  Also, adding a new sequence from the user-interface is a
 *  significant modification, so the "is modified" flag gets set.
 *
 *  If enabled, wire in the MIDI buss override.
 *
 * \param seq
 *      The prospective sequence number of the new sequence.  If not set to
 *      seq::unassigned() (-1), then the sequence is also installed, and this
 *      value will be updated to the actual number.
 *
 * \return
 *      Returns true if the sequence is valid.  Do not use the
 *      sequence if false is returned, it will be null.
 */

bool
performer::new_sequence (seq::number & finalseq, seq::number seq)
{
    sequence * seqptr = new (std::nothrow) sequence(ppqn());
    bool result = not_nullptr(seqptr);
    if (result && seq != seq::unassigned())
    {
        result = install_sequence(seqptr, seq);
        if (result)                                 /* new 2021-10-01   */
        {
            const seq::pointer s = get_sequence(seq);
            result = not_nullptr(s);
            if (result)
            {
                s->set_dirty();
                finalseq = s->seq_number();
                notify_sequence_change(finalseq, change::recreate);
            }
        }
    }
    return result;
}

/**
 *  Deletes a pattern/sequence by number.  We now also solidify the deletion
 *  by setting the pointer to null after deletion, so it will blow up if
 *  accidentally accessed.  The final act is to raise the "is modified" flag,
 *  since deleting an existing sequence is always a significant modification.
 *
 *  Now, this function obviously sets the "active" flag for the sequence to
 *  false.  But there are a few other flags that are not modified; shouldn't
 *  we also falsify them here?
 *
 *  What about notify_sequence_change()?  Checking in-edit status?
 *
 * \param seq
 *      The sequence number of the sequence to be deleted.  It is validated.
 *
 * \return
 *      Returns true if the sequence was removed.
 */

bool
performer::remove_sequence (seq::number seqno)
{
    bool result = mapper().remove_sequence(seqno);
    if (result)
    {
        seq::number buttonno = seqno - playscreen_offset();
        send_seq_event(buttonno, midicontrolout::seqaction::removed);
        notify_sequence_change(seqno, change::recreate);            /* NEW */
        modify();
    }
    return result;
}

bool
performer::copy_sequence (seq::number seqno)
{
    bool result = is_seq_active(seqno);
    if (result)
    {
        const seq::pointer s = get_sequence(seqno);
        result = bool(s);
        if (result)
            m_seq_clipboard.partial_assign(*s, true);
    }
    return result;
}

bool
performer::cut_sequence (seq::number seqno)
{
    bool result = is_seq_active(seqno) && ! is_seq_in_edit(seqno);
    if (result)
    {
        seq::pointer s = get_sequence(seqno);
        result = bool(s);
        if (result)
        {
            m_seq_clipboard.partial_assign(*s);
            result = remove_sequence(seqno);
        }
    }
    return result;
}

bool
performer::paste_sequence (seq::number seqno)
{
    bool result = ! is_seq_active(seqno);
    if (result)
    {
        static seq::number s_dummy;
        if (new_sequence(s_dummy, seqno))
        {
            seq::pointer s = get_sequence(seqno);
            s->partial_assign(m_seq_clipboard);

            /*
             * Already done in new_sequence().
             *
             * s->set_dirty();
             */
        }
    }
    return result;
}

bool
performer::merge_sequence (seq::number seqno)
{
    bool result = false;
    if (! is_seq_active(seqno))
    {
        result = paste_sequence(seqno);
    }
    else
    {
        seq::pointer s = get_sequence(seqno);
        result = s->merge_events(m_seq_clipboard);
        if (result)
        {
            s->set_dirty();
            notify_sequence_change(seqno, change::recreate);        /* NEW */
        }
    }
    return result;
}

/**
 *  Takes the given sequence number, makes sure the sequence is active, copies
 *  it to m_moving_seq via a partial-assign, and then removes it.
 */

bool
performer::move_sequence (seq::number seqno)
{
    bool result = is_seq_active(seqno);
    if (result)
    {
        seq::pointer s = get_sequence(seqno);
        m_old_seqno = seqno;
        m_moving_seq.partial_assign(*s);
        result = remove_sequence(seqno);
    }
    return result;
}

bool
performer::finish_move (seq::number seqno)
{
    static seq::number s_dummy;
    bool result = false;
    if (! is_seq_active(seqno))
    {
        if (new_sequence(s_dummy, seqno))
        {
            get_sequence(seqno)->partial_assign(m_moving_seq);
            result = true;
        }
    }
    else
    {
        if (new_sequence(s_dummy, m_old_seqno))
        {
            get_sequence(m_old_seqno)->partial_assign(m_moving_seq);
            result = true;
        }
    }
    return result;
}

/*
 * -------------------------------------------------------------------------
 *  More settings
 * -------------------------------------------------------------------------
 */

/**
 *  Sets the PPQN for the master buss, JACK assistant, and the performer.
 *  Note that we do not set the modify flag or do notification notification
 *  here.  See the change_ppqn() function instead.
 *
 * \setter ppqn
 *      Also sets other related members.
 *
 *      While running it is better to call change_ppqn(), in order to run
 *      though ALL patterns and user-interface objects to fix them.
 *
 * \param p
 *      Provides a PPQN that should be different from the current value and be
 *      in the legal range of PPQN values.
 */

bool
performer::set_ppqn (int p)
{
    bool result = m_ppqn != p && ppqn_in_range(p);
    if (result)
    {
        if (m_master_bus)
        {
            m_ppqn = p;
            m_one_measure = 0;
            (void) jack_set_ppqn(p);
            m_master_bus->set_ppqn(p);
        }
        else
        {
            set_error_message("set_ppqn() null master bus");
            result = false;
        }
    }
    if (m_one_measure == 0)
    {
        m_one_measure = p * 4;                  /* simplistic!  */
        m_right_tick = m_one_measure * 4;       /* ditto        */
    }
    return result;
}

/**
 *  Goes through all sets and sequences, updating the PPQN of the events and
 *  triggers.  It also, via notify_resolution_change(), sets the modify flag.
 *
 *  Currently operates only on the current screenset.
 */

bool
performer::change_ppqn (int p)
{
    bool result = set_ppqn(p);                  /* performer & master bus   */
    if (result)
    {
        mapper().exec_set_function
        (
            [p] (seq::pointer sp, seq::number /*sn*/)
            {
                bool result = bool(sp);
                if (result)
                    sp->change_ppqn(p);

                return result;
            }
        );
        if (result)
        {
            change ch = rc().midi_filename().empty() ?
                change::no : change:: yes;

            notify_resolution_change(get_ppqn(), get_beats_per_minute(), ch);
        }
    }
    return result;
}

/**
 *  Goes through all the sequences in the current play-set, updating the buss
 *  to the same (global) buss number.
 *
 * \param buss
 *      Provides the number of the buss to be set.  Note that this buss number
 *      has already effectively been remapped if port-mapping is in place.
 */

bool
performer::ui_change_set_bus (int buss)
{
    bussbyte b = bussbyte(buss);
    bool result = is_good_buss(b);
    if (result)
    {
        for (auto seqi : m_play_set.seq_container())
        {
            if (seqi)
                seqi->set_midi_bus(b, true);    /* calls notify function    */
            else
                set_error_message("set bus on null sequence");
        }

        screenset::number setno = mapper().playscreen_number();
        notify_set_change(setno, change::yes);
    }
    return result;
}

/**
 *  This function provides a way to set the song-mode depending on if the
 *  loaded song has triggers or not.  If there are no triggers, then all
 *  tracks are unmuted automatically.  This feature is useful for headless
 *  play.
 */

void
performer::next_song_mode ()
{
    bool has_triggers = mapper().trigger_count() > 0;
    (void) set_playing_screenset(screenset::number(0)); /* ca 2021-12-13    */
    if (rc().song_start_auto())
    {
        song_mode(has_triggers);

        /*
         * No, the user has to turn this on!
         *
         * song_recording(has_triggers);
         */

        if (has_triggers)
            set_song_mute(mutegroups::action::off);
    }
    else
    {
        bool mutem = rc().is_setsmode_normal();
        bool songmode = rc().song_start_mode();     /* song vs live here    */
        mute_all_tracks(mutem);
        song_mode(songmode);

        /*
         * No, the user has to turn this on!
         *
         * song_recording(songmode && has_triggers);
         */
    }
}

/**
 *  Locks on m_condition_var [accessed by function cv()].  Then, if not
 *  is_running(), the playback mode is set to the given state.  If that state
 *  is true, call off_sequences().  Set the running status, unlock, and signal
 *  the condition.
 *
 *  Note that we reverse unlocking/signalling from what Seq64 does (BUG!!!)
 *  Manual unlocking should be done before notifying, to avoid waking waking
 *  up the waiting thread, only to lock again.  See the notify_one() notes for
 *  details.
 *
 *  This function should be considered the "second thread", that is the thread
 *  that starts after the worker thread is already working.
 *
 *  In ALSA mode, restarting the sequence moves the progress bar to the
 *  beginning of the sequence, even if just pausing.  This is fixed by
 *  disabling calling off_sequences() when starting playback from the song
 *  editor / performance window.
 *
 * \param songmode
 *      Sets the playback mode, and, if true, turns off all of the sequences
 *      before setting the is-running condition.
 */

void
performer::inner_start ()
{
    if (! done())                               /* won't start when exiting */
    {
        if (! is_running())
        {
            /*
             * Issue #89.  This happens all the time! Thus announce_pattern()
             * spews events! However, the cause is not here.
             */

            if (song_mode())
                off_sequences();                /* mute for song playback   */

            is_running(true);                   /* part of cv()'s predicate */
            pad().js_jack_stopped = false;
            cv().signal();                      /* signal we are running    */
            send_onoff_event(midicontrolout::uiaction::play, true);
            send_onoff_event(midicontrolout::uiaction::panic, false);
        }
    }
}

/**
 *  Unconditionally, and without locking, clears the running status and resets
 *  the sequences.  Sets m_usemidiclock to the given value.  Note that we do
 *  need to set the running flag to false here, even when JACK is running.
 *  Otherwise, JACK starts ping-ponging back and forth between positions under
 *  some circumstances.
 *
 *  However, if JACK is running, we do not want to reset the sequences... this
 *  causes the progress bar for each sequence to move to near the end of the
 *  sequence.
 *
 * \param midiclock
 *      If true, indicates that the MIDI clock should be used.  The default
 *      value is false.
 */

void
performer::inner_stop (bool midiclock)
{
    is_running(false);
    reset_sequences();                  /* resets, and flushes the buss     */
    m_usemidiclock = midiclock;
    send_onoff_event(midicontrolout::uiaction::stop, true);
    send_onoff_event(midicontrolout::uiaction::panic, true);
}

/**
 *  Copies the given string into the desired set's name.
 *
 * \param sset
 *      The ID number of the screen-set, an index into the setmapper.
 *
 * \param name
 *      Provides the string data to copy into the name.
 *
 * \param is_load_modification
 *      If true (the default is false), we do not want to set the modify flag,
 *      otherwise the user is prompted to save even if no changes have
 *      occurred.
 */

void
performer::screenset_name
(
    screenset::number sn,
    const std::string & name,
    bool is_load_modification
)
{
    bool changed = mapper().name(sn, name);
    if (changed)
    {
        change mod = is_load_modification ? change::no : change::yes ;
        notify_set_change(sn, mod);
    }
}

/**
 *  New for the Qt 5 version, to stop endless needless redraws upon
 *  ticking of the redraw timer.  Also see (for Qt 5) the qseqbase ::
 *  needs_update() function.  Most useful in seqedit or qseqedit.
 *
 * \param seq
 *      The sequence to check.  If set to seq::all(), the default, check
 *      them all, exiting when the first dirty one is found.
 *
 * \return
 *      Returns true if the performer is running or if a sequence is found
 *      to be dirty, and in need of refreshing in the user interface.
 */

bool
performer::needs_update (seq::number seqno) const
{
    bool result = false;
    if (m_is_busy)
    {
        (void) warn_message("performer busy!");
    }
    else
    {
        result = is_running();
        if (! result)
        {
            if (m_needs_update)
            {
                m_needs_update = false;                     /* mutable      */
                result = true;
            }
            else
            {
                if (seqno == seq::all())
                    result = mapper().needs_update();       /* check all    */
                else
                    result = is_dirty_main(seqno);          /* check one    */
            }
        }
    }
    return result;
}

/**
 *  Sets the value of the BPM into the master MIDI buss, after making
 *  sure it is squelched to be between 20 and 500.  Replaces
 *  performer::set_bpm() from seq24.
 *
 *  The value is set only if neither JACK nor this performer object are
 *  running.
 *
 *  It's not clear that we need to set the "is modified" flag just because we
 *  changed the beats per minute.  This setting does get saved to the MIDI
 *  file, with the c_bpmtag.
 *
 *  Do we need to adjust the BPM of all of the sequences, including the
 *  potential tempo track???  It is "merely" the putative main tempo of the
 *  MIDI tune.  Actually, this value can now be recorded as a Set Tempo event
 *  by user action in the main window (and, later, by incoming MIDI Set Tempo
 *  events).
 *
 * \param bpm
 *      Provides the beats/minute value to be set.  Checked for validity.  It
 *      is a wide range of speeds, well beyond what normal music needs.
 */

bool
performer::set_beats_per_minute (midibpm bpm)
{
    bool result = usr().bpm_is_valid(int(bpm));
    if (result)
    {
        bpm = fix_tempo(bpm);
        result = jack_set_beats_per_minute(bpm);    /* not just JACK though */
    }
    return result;
}

/**
 *  This is a faster version, meant for jack_assistant to call.  This logic
 *  matches the original seq24, but is it really correct?  Well, we fixed it
 *  so that, whether JACK transport is in force or not, we can modify the BPM
 *  and have it stick.  No test for JACK Master or for JACK and normal running
 *  status needed.
 *
 *  Note that the JACK server, especially when transport is stopped,
 *  sends some artifacts (really low BPM), so we avoid dealing with low
 *  values.
 */

bool
performer::jack_set_beats_per_minute (midibpm bpm)
{
    bool result = bpm != m_bpm && usr().bpm_is_valid(bpm);
    if (result)
    {

#if defined SEQ66_JACK_SUPPORT
        m_jack_asst.set_beats_per_minute(bpm);  /* see banner note */
#endif

        if (m_master_bus)
            m_master_bus->set_beats_per_minute(bpm);

        m_us_per_quarter_note = tempo_us_from_bpm(bpm);
        m_bpm = bpm;
        change ch = rc().midi_filename().empty() ?  change::no : change:: yes;
        notify_resolution_change(get_ppqn(), bpm, ch);
    }
    return result;
}

/**
 *  Encapsulates some calls used in mainwnd.  Actually does a lot of
 *  work in those function calls.
 *
 * \return
 *      Returns the resultant BPM, as a convenience.
 */

midibpm
performer::decrement_beats_per_minute ()
{
    midibpm result = get_beats_per_minute() - usr().bpm_step_increment();
    set_beats_per_minute(result);
    return result;
}

/**
 *  Encapsulates some calls used in mainwnd.  Actually does a lot of
 *  work in those function calls.
 *
 * \return
 *      Returns the resultant BPM, as a convenience.
 */

midibpm
performer::increment_beats_per_minute ()
{
    midibpm result = get_beats_per_minute() + usr().bpm_step_increment();
    set_beats_per_minute(result);
    return result;
}

/**
 *  Provides additional coarse control over the BPM value, which comes into
 *  force when the Page-Up/Page-Down keys are pressed.
 *
 *  Encapsulates some calls used in mainwnd.  Actually does a lot of
 *  work in those function calls.
 *
 * \return
 *      Returns the resultant BPM, as a convenience.
 */

midibpm
performer::page_decrement_beats_per_minute ()
{
    midibpm result = get_beats_per_minute() - usr().bpm_page_increment();
    set_beats_per_minute(result);
    return result;
}

/**
 *  Provides additional coarse control over the BPM value, which comes into
 *  force when the Page-Up/Page-Down keys are pressed.
 *
 *  Encapsulates some calls used in mainwnd.  Actually does a lot of
 *  work in those function calls.
 *
 * \return
 *      Returns the resultant BPM, as a convenience.
 */

midibpm
performer::page_increment_beats_per_minute ()
{
    midibpm result = get_beats_per_minute() + usr().bpm_page_increment();
    set_beats_per_minute(result);
    return result;
}

/**
 *  Should we pass the current value of BPM to the set_beats_per_minute()
 *  function?
 */

midibpm
performer::update_tap_bpm ()
{
    midibpm bpm = 0.0;
    long ms = millitime();
    if (m_current_beats == 0)
    {
        m_base_time_ms = ms;
        m_last_time_ms = 0;
    }
    else if (m_current_beats >= 1)
    {
        int diffms = ms - m_base_time_ms;
        if (diffms > 0)
            bpm = m_current_beats * 60000.0 / diffms;
        else
            bpm = m_bpm;                        /* where do we set this?    */

        m_last_time_ms = ms;
    }
    ++m_current_beats;
    return bpm;
}

bool
performer::tap_bpm_timeout ()
{
    bool result = false;
    if (m_current_beats > 0 && m_last_time_ms > 0)
    {
        long ms = millitime();
        long difference = ms - m_last_time_ms;
        if (difference > usr().tap_button_timeout())
        {
            clear_current_beats();
            result = true;
        }
    }
    return result;
}

/**
 *  Used by callers to insert tempo events.  Note that, if the current tick
 *  position is past the end of pattern 0's length, then the length of the
 *  tempo track pattern (0 by default) is increased in order to hold the tempo
 *  event.
 *
 *  Note that we allow the user to change the tempo track from the default of
 *  0 to any pattern from 0 to 1023.  This is done in the File / Options /
 *  MIDI Clock tab, and is saved to the "rc" file.
 *
 * \return
 *      Returns true if the tempo-track sequence exists.
 */

bool
performer::log_current_tempo ()
{
    seq::pointer s = get_sequence(rc().tempo_track_number());
    bool result = bool(s);
    if (result)
    {
        midipulse tick = get_tick();
        midibpm bpm = get_beats_per_minute();
        seq66::event e = create_tempo_event(tick, bpm);     /* event.cpp    */
        if (s->add_event(e))                                /* sorts too    */
        {
            s->set_dirty();
            if (tick > s->get_length())
                s->set_length(tick);

            modify();   /* notify_sequence_change(seqno) too problematic    */
        }
    }
    return result;
}

/**
 *  Also calls mapper().set_playscreen(), and notifies any performer::callbacks
 *  subscribers. Note that the setsmode values of normal and autoarm indicate
 *  to clear the play-set before adding the next set to it.
 */

screenset::number
performer::set_playing_screenset (screenset::number setno)
{
    bool ok = m_io_active;                          /* setno != current     */
    if (ok)
        ok = mapper().set_playing_screenset(setno);

    if (ok)
    {
        bool clearit = rc().is_setsmode_clear();    /* remove all patterns? */
        announce_exit(false);                       /* blank the device     */
        unset_queued_replace();                     /* clear queueing       */
        mapper().fill_play_set(m_play_set, clearit);
        if (rc().is_setsmode_autoarm())
        {
            set_song_mute(mutegroups::action::off); /* unmute them all      */
        }
        else if (rc().is_setsmode_allsets())
        {
            /*
             * Nothing to do?
             */
        }
        announce_playscreen();                      /* inform control-out   */
        notify_set_change(setno, change::signal);   /* change::no           */
    }
    return mapper().playscreen_number();
}

/**
 *  Clears the whole play-set and refills it with the current playscreen.
 *  If auto-arm is in force, will unmute them.  Does not signal a set-change,
 *  because the playing set hasn't changed.
 */

void
performer::reset_playset ()
{
    announce_exit(false);                           /* blank the device     */
    unset_queued_replace();                         /* clear queueing       */
    mapper().fill_play_set(m_play_set, true);       /* true: clear it first */
    if (rc().is_setsmode_autoarm())
        set_song_mute(mutegroups::action::off);     /* unmute them all      */

    announce_playscreen();                          /* inform control-out   */
}

bool
performer::copy_playscreen ()
{
    screenset::number pscreen = playscreen_number();
    bool result = pscreen != screenset::unassigned();
    if (result)
        m_screenset_to_copy = pscreen;

    return result;
}

bool
performer::paste_playscreen (screenset::number destination)
{
    bool result = m_screenset_to_copy != screenset::unassigned();
    if (result)
        result = destination != screenset::unassigned();

    if (result)
        result = destination != m_screenset_to_copy;

    if (result)
    {
        result = mapper().copy_screenset(m_screenset_to_copy, destination);
        if (result)
            notify_set_change(destination, change::yes);
    }
    return result;
}

/**
 *  Removes the given screenset, then notifies all subscribers.
 *
 * \return
 *      Returns true if the set was found (and removed).
 */

bool
performer::remove_set (screenset::number setno)
{
    bool result = mapper().remove_set(setno);
    if (result)
        notify_set_change(setno, change::removed);

    return result;
}

/**
 *  Swaps the sets, useful in moving sets around in the set-master (class
 *  qsetmaster).
 */

bool
performer::swap_sets (seq::number set0, seq::number set1)
{
    bool result = mapper().swap_sets(set0, set1);
    if (result)
    {
        notify_set_change(set0);
        notify_set_change(set1);
    }
    return result;
}

/**
 *  Clears all of the patterns/sequences. Attempts to reset the performer to
 *  its startup condition when no MIDI file is loaded
 *
 *  The mainwnd module calls this function, and the midifile and wrkfile
 *  classes, if a play-list is being loaded and verified.  Note that perform
 *  now handles the "is modified" flag on behalf of all external objects, to
 *  centralize and simplify the dirtying of a MIDI tune.
 *
 *  Anything else to clear?  What about all the other sequence flags?  We can
 *  beef up remove_sequence() for them, at some point.
 *
 *  Added stazed code from 1.0.5 to abort clearing if any of the sequences are
 *  in editing.
 *
 * \warning
 *      We have an issue.  Each GUIs conditional_update() function can call
 *      this one, potentially telling later GUIs that they do not need to
 *      update.  This affect the needs_update() function when not running
 *      playback.
 *
 * \param clearplaylist
 *      Defaults to false. If true, the playlist is cleared completely, which
 *      also clears the original playlist file. TODO: if true, get the
 *      playlist tab to clear itself.
 *
 * \return
 *      Returns true if the clear-all operation could be performed.  If false,
 *      then at least one active sequence was in editing mode.
 */

bool
performer::clear_all (bool /* clearplaylist */ )
{
    bool result = clear_song();
    usr().clear_global_seq_features();
    if (result)
    {
        m_play_set.clear();             /* dump active patterns             */

#if defined WE_REALLY_NEED_TO_RESET_PLAYLIST
        if (m_play_list)
        {
            m_is_busy = true;
            (void) m_play_list->reset_list(clearplaylist);
            m_is_busy = false;
        }
#endif

        set_needs_update();             /* tell all GUIs to refresh. BUG!   */
        announce_exit();
        announce_playscreen();
        announce_mutes();
        announce_automation();
    }
    return result;
}

bool
performer::clear_song ()
{
    bool result = ! mapper().any_in_edit() && ! m_is_busy;
    if (result)
    {
        m_is_busy = true;
        reset_sequences();
        rc().midi_filename("");
        set_have_undo(false);
        m_undo_vect.clear();
        set_have_redo(false);
        m_redo_vect.clear();
        mapper().reset();               /* clears and recreates empty set   */
        m_is_busy = false;
        unmodify();                     /* new, we start afresh             */
        set_needs_update();             /* tell all GUIs to refresh. BUG!   */
    }
    return result;
}

/**
 *  For all active patterns/sequences, get its playing state, turn off the
 *  playing notes, set playing to false, zero the markers, and, if not in
 *  playback mode, restore the playing state.  Note that these calls are
 *  folded into one member function of the sequence class.  Finally, flush the
 *  master MIDI buss.
 *
 *  Could use a member function pointer to avoid having to code two loops.
 *  We did it.  Note that std::shared_ptr does not support operator::->*, so
 *  we have to get() the pointer.
 *
 *  Another option is to call mapper().reset_sequences(pause, playback_mode()).
 *  This would result in a call to screenset::reset_sequences(), which does
 *  the same thing but also checks the sequence for being active.  Is it worth
 *  it?
 *
 * \param p
 *      Try to prevent notes from lingering on pause if true.  By default, it
 *      is false.
 */

void
performer::reset_sequences (bool p)
{
    void (sequence::* f) (bool) = p ? &sequence::pause : &sequence::stop ;
    bool songmode = song_mode();
    for (auto & seqi : m_play_set.seq_container())
        (seqi.get()->*f)(songmode);

    /*
     * Alread flushed in the loop above.
     *
     * if (m_master_bus)
     *     m_master_bus->flush();
     */
}

bool
performer::repitch_all (const std::string & nmapfile, seq::ref s)
{
    bool result = open_note_mapper(nmapfile);
    if (result)
        result = s.repitch(*m_note_mapper, true);

    if (result)
        modify();

    return result;
}

bool
performer::repitch_selected (const std::string & nmapfile, seq::ref s)
{
    bool result = open_note_mapper(nmapfile);
    if (result)
        result = s.repitch(*m_note_mapper);

    if (result)
        modify();

    return result;
}

/**
 *  Provides for various settings of the song-mute status of all sequences in
 *  the song. The sequence::set_song_mute() and toggle_song_mute() functions
 *  do all the work, including mp-dirtying the sequence.
 *
 *  We've modified this function to call mute_all_tracks() and
 *  toggle_all_tracks() in order to consolidate the code and (cough cough) fix
 *  a bug in this functionality from the mainwnd menu.
 *
 * \question
 *      Do we want to replace the call to toggle_all_tracks() with a call to
 *      toggle_playing_tracks()?
 *
 * \param op
 *      Provides the "flag" that indicates if this function is to set mute on,
 *      off, or to toggle the mute status.
 */

void
performer::set_song_mute (mutegroups::action op)
{
    switch (op)
    {
    case mutegroups::action::on:

        mute_all_tracks(true);
        break;

    case mutegroups::action::off:

        mute_all_tracks(false);
        break;

    case mutegroups::action::toggle:

        toggle_all_tracks();
        break;

    case mutegroups::action::toggle_active:
    default:

        break;
    }
}

/**
 *  Creates the mastermidibus.  We need to delay creation until launch time,
 *  so that settings can be obtained before determining just how to set up the
 *  application.
 *
 *  Once the master buss is created, we then copy the clocks and input setting
 *  that were read from the "rc" file, via the mastermidibus ::
 *  set_port_statuses() function, to use in determining whether to initialize
 *  and connect the input ports at start-up.  Seq24 wouldn't connect
 *  unconditionally, and Sequencer64 shouldn't, either.
 *
 *  However, the devices actually on the system at start time might be
 *  different from what was saved in the "rc" file after the last run of
 *  Sequencer64.
 *
 *  For output, both apps have always connected to all ports automatically.
 *  But we want to support disabling some output ports, both in the "rc"
 *  file and via the operating system indicating that it cannot open an
 *  output port.  So how do we get the port-settings from the OS?  Probably
 *  at initialization time.  See the mastermidibus constructor for PortMidi.
 *
 * \return
 *      Returns true if the creation succeeded, or if the buss already exists.
 */

bool
performer::create_master_bus ()
{
    bool result = false;
    if (! m_master_bus)                 /* no master buss yet?  */
    {
        /*
         * \todo Use std::make_shared()
         *
         *  Also, at this point, do we have the actual complement of inputs and
         *  clocks, as opposed to what's in the rc file?
         */

        m_master_bus.reset(new (std::nothrow) mastermidibus(m_ppqn, m_bpm));
        if (m_master_bus)
        {
            mastermidibus * mmb = m_master_bus.get();
            mmb->filter_by_channel(m_filter_by_channel);
            mmb->set_port_statuses(m_clocks, m_inputs);
            midi_control_out().set_master_bus(mmb);
            result = true;
        }
    }
    return result;
}

/**
 *  Calls the MIDI buss and JACK initialization functions and the input/output
 *  thread-launching functions.  This function is called in main().  We
 *  collected all the calls here as a simplification, and renamed it because
 *  it is more than just initialization.  This function must be called after
 *  the perform constructor and after the configuration file and command-line
 *  configuration overrides.  The original implementation, where the master
 *  buss was an object, was too inflexible to handle a JACK implementation.
 *
 * \param ppqn
 *      Provides the PPQN value, which is determined by the caller and assumed
 *      to be valid.
 *
 * \todo
 *      We probably need a bpm parameter for consistency at some point.
 */

bool
performer::launch (int ppqn)
{
    bool result = create_master_bus();      /* calls set_port_statuses()    */
    if (result)
    {
        (void) init_jack_transport();
        m_master_bus->init(ppqn, m_bpm);    /* calls api_init() per API     */
        result = activate();
        if (result)
        {
            /*
             * Get and store the clocks and inputs created (disabled or not)
             * by the mastermidibus during api_init().  After this call, the
             * clocks and inputs now have names.  These calls are necessary to
             * populate the port lists the first time Seq66 is run.
             *
             * m_master_bus->get_port_statuses(m_clocks, m_inputs); the
             * statuses from e.g. midi_jack_info are already obtained in the
             * call stack of create_master_bus().
             */

            m_master_bus->copy_io_busses();
            m_master_bus->get_port_statuses(m_clocks, m_inputs);

            /*
             * Moved from get_settings() so that aliases, if present, are
             * obtained by this point.
             */

            bussbyte namedbus = m_midi_control_in.nominal_buss();
            bussbyte truebus = true_input_bus(namedbus);
            m_midi_control_in.true_buss(truebus);
            namedbus = m_midi_control_out.nominal_buss();
            truebus = true_output_bus(namedbus);
            m_midi_control_out.true_buss(truebus);
            m_io_active = true;
            launch_input_thread();
            launch_output_thread();
            midi_control_out().send_macro(midimacros::startup);
            announce_playscreen();
            announce_mutes();
            announce_automation();
            (void) set_playing_screenset(screenset::number(0));
        }
        else
            m_error_pending = true;
    }
    return result;
}

/**
 *  Announces the current mute states of the now-current play-screen.  This
 *  function is handled by creating a slothandler that calls the
 *  announce_sequence() function.  The proper working of this function depends
 *  on announce_sequence() returning true for all slots, even empty ones.
 */

void
performer::announce_playscreen ()
{
    if (midi_control_out().is_enabled())
    {
        screenset::slothandler sh = std::bind
        (
            &performer::announce_sequence, this,
            std::placeholders::_1, std::placeholders::_2
        );
        exec_slot_function(sh, false);          /* do not use set-offset    */
        m_master_bus->flush();
    }
}

/**
 *  This action is similar to announce_playscreen(), but it unconditionally
 *  turns off (removes) all of the sequences in the MIDI status device (e.g.
 *  the Launchpad Mini).
 *
 *  It also optionally turns off all of the automation buttons and
 *  mute-group buttons as well.
 *
 * \param playstatesoff
 *      If true, also blank the automation and mute-group buttons.
 *      Defaults to true.
 */

void
performer::announce_exit (bool playstatesoff)
{
    if (midi_control_out().is_enabled())
    {
        midi_control_out().clear_sequences();
        if (playstatesoff)
        {
            announce_automation(false);
            midi_control_out().clear_mutes();
        }
    }
}

/**
 *  Announces the initial and ending statues of the automation output display.
 *
 * \param activate
 *      If true (the default), then we try to set the status of each control to
 *      "off", which means active, but not yet used.  If false, all are set to
 *      the "del" state, which normally display as an unilluminated button.
 *      Defaults to true, to be used at start-up.
 */

void
performer::announce_automation (bool activate)
{
    midi_control_out().send_automation(activate);
}

/**
 *  This function sets the buttons of all mutes_groups that have mute settings
 *  to red, and the rest to off.
 */

void
performer::announce_mutes ()
{
    for (int g = 0; g < mutegroups::Size(); ++g)
    {
        bool hasany = mutes().any(mutegroup::number(g));
        if (hasany)
            send_mutes_event(g, false);                 /* should turn red  */
        else
            send_mutes_inactive(g);                     /* should turn off  */
    }
}

/**
 *  Provides a screenset::slothandler function to announce the current status
 *  of a sequence to an external device via the midicontrolout container.
 *  This function has to have both the sequence and its number as parameters,
 *  and must return a boolean value.
 *
 *  Also note that exec_slot_function() must be called with the use_set_offset
 *  parameter set to false, in order to keep the slot number below the
 *  set-size, otherwise a crash occurs.
 *
 * Issue #89:
 *
 *      Had not added code to send the "queue" status!  Fixed.
 *
 * \param s
 *      Provides the pointer to the sequence.
 *
 * \param sn
 *      Provides the slot number to be used for display, and should range from
 *      0 to the set-size.
 *
 * \return
 *      Returns true all the time, because we want to be able to handle empty
 *      slots as well, and screenset::slot_function() is meant to use a false
 *      result only under abnormal conditions.
 */

bool
performer::announce_sequence (seq::pointer s, seq::number sn)
{
    bool ok = not_nullptr(s);
    midicontrolout::seqaction what;
    if (ok)
    {
        if (s->playing())
        {
            what = s->get_queued() ?
                midicontrolout::seqaction::queued :     // unqueueing pending
                midicontrolout::seqaction::armed ;
        }
#if 0
        else if (s->get_queued())
            what = midicontrolout::seqaction::queued;
        else if (s->one_shot())
            what = midicontrolout::seqaction::queued;
        else
            what = midicontrolout::seqaction::muted;
#else
        what = midicontrolout::seqaction::muted;
        if (s->get_queued() || s->one_shot())
            what = midicontrolout::seqaction::queued;
#endif
    }
    else
        what = midicontrolout::seqaction::removed;

    send_seq_event(sn, what);
    return true;
}

bool
performer::announce_pattern (seq::number seqno)
{
    seq::pointer s = get_sequence(seqno);
    return announce_sequence(s, mapper().seq_to_offset(*s));
}

/**
 *  Sets the beats per measure.
 *
 *  Note that a lambda function is used to make the changes.
 */

bool
performer::set_beats_per_measure (int bpm, bool user_change)
{
    bool result = bpm != m_beats_per_bar;
    if (result)
    {
        set_beats_per_bar(bpm);         /* also sets in jack_assistant  */
        mapper().exec_set_function
        (
            [bpm, user_change] (seq::pointer sp, seq::number /*sn*/)
            {
                bool result = bool(sp);
                if (result)
                {
                    sp->set_beats_per_bar(bpm, user_change);
                    sp->set_measures(sp->get_measures());
                }
                return result;
            }
        );
    }
    return result;
}

/**
 * \setter m_beat_width
 *
 * \param bw
 *      Provides the value for beat-width.  Also used to set the
 *      beat-width in the JACK assistant object.
 */

bool
performer::set_beat_width (int bw, bool user_change)
{
    bool result = bw != m_beat_width;
    if (result)
    {
        set_beat_length(bw);            /* also sets in jack_assistant  */
        mapper().exec_set_function
        (
            [bw, user_change] (seq::pointer sp, seq::number /*sn*/)
            {
                bool result = bool(sp);
                if (result)
                {
                    sp->set_beat_width(bw, user_change);
                    sp->set_measures(sp->get_measures());
                }
                return result;
            }
        );
    }
    return result;
}

/**
 *  Creates the output thread using output_thread_func().  This might be a
 *  good candidate for a small thread class derived from a small base class.
 *
 *  -   We may want more control over lifetime of object, for example to
 *      initialize it "lazily". The std::thread already supports it. It can be
 *      made in "not representing a thread" state, assigned a real thread
 *      later when needed, and it has to be explicitly joined or detacheded
 *      before destruction.
 *  -   We may want a member to be transferred to/from some other ownership.
 *      No need of pointer for that since std::thread already supports move
 *      and swap.
 *  -   We may want opaque pointer for to hide implementation details and
 *      reduce dependencies, but std::thread is standard library class so we
 *      can't make it opaque.
 *  -   We may want to have shared ownership. That is fragile scenario with
 *      std::thread. Multiple owners who can assign, swap, detach, or join it
 *      (and there are no much other operations with thread) can cause
 *      complications.
 *  -   There is mandatory cleanup before destruction like if
 *      (xthread.joinable()) xthread.join(); or xthread.detach();. That is
 *      also more robust and easier to read in destructor of owning class
 *      instead of code that instruments same thing into deleter of a smart
 *      pointer.
 *
 *  So unless there is some uncommon reason we should use thread as data
 *  member directly.
 */

void
performer::launch_output_thread ()
{
    if (rc().verbose())
    {
        unsigned num_cpus = std::thread::hardware_concurrency();
        infoprintf("%u CPUs detected", num_cpus);
    }
    if (! m_out_thread_launched)
    {
        m_out_thread = std::thread(&performer::output_func, this);
        m_out_thread_launched = true;
        if (rc().priority())                        /* Not in MinGW RCB     */
        {
            bool ok = set_thread_priority(m_out_thread, c_thread_priority);
            if (ok)
            {
#if defined SEQ66_PLATFORM_LINUX
                warn_message("Output priority elevated");
#endif
            }
            else
            {
                errprint
                (
                    "output thread: couldn't set scheduler to FIFO, "
                    "need root priviledges."
                );
                pthread_exit(0);
                m_out_thread_launched = false;
            }
        }
    }
}

/**
 *  Creates the input thread using input_thread_func().  This might be a good
 *  candidate for a small thread class derived from a small base class.
 */

void
performer::launch_input_thread ()
{
    if (!  m_in_thread_launched)
    {
        m_in_thread = std::thread(&performer::input_func, this);
        m_in_thread_launched = true;
        if (rc().priority())                        /* Not in MinGW RCB     */
        {
            bool ok = set_thread_priority(m_in_thread, c_thread_priority);
            if (ok)
            {
#if defined SEQ66_PLATFORM_LINUX
                warn_message("Input priority elevated");
#endif
            }
            else
            {
                errprint
                (
                    "input thread: couldn't set scheduler to FIFO, "
                    "need root priviledges."
                );
                pthread_exit(0);
                m_in_thread_launched = false;
            }
        }
    }
}

/**
 *  The rough opposite of launch(); it doesn't stop the threads.  A minor
 *  simplification for the main() routine, hides the JACK support macro.
 *  We might need to add code to stop any ongoing outputing.
 *
 *  Also gets the settings made/changed while the application was running from
 *  the mastermidibase class to here.  This action is the converse of calling
 *  the set_port_statuses() function defined in the mastermidibase module.
 *
 *  Note that we call stop_playing().  This will stop JACK transport. If we
 *  restart Seq66 without doing this, transport keeps running (as can be seen
 *  in QJackCtl).  So playback starts while loading a MIDI file while Seq66
 *  starts.  Not only is this kind of surprising, it can lead to a seqfault at
 *  random times.
 *
 *  Also note that m_is_running and m_io_active are both used in the
 *  performer::synch::predicate() override.
 */

bool
performer::finish ()
{
    bool result = true;
    if (! done())                           /* m_io_active is true          */
    {
        stop_playing();                     /* see notes in banner          */
        reset_sequences();                  /* stop all output upon exit    */
        announce_exit(true);                /* blank device completely      */
        midi_control_out().send_macro(midimacros::shutdown);
        m_io_active = false;                /* set done() for predicate     */
        m_is_running = false;               /* set is_running() off         */
        cv().signal();                      /* signal the end of play       */
        if (m_out_thread_launched && m_out_thread.joinable())
        {
            m_out_thread.join();
            m_out_thread_launched = false;
        }
        if (m_in_thread_launched && m_in_thread.joinable())
        {
            m_in_thread.join();
            m_in_thread_launched = false;
        }

        bool ok = deinit_jack_transport();
        bool result = bool(m_master_bus);
        if (result)
            m_master_bus->get_port_statuses(m_clocks, m_inputs);

        result = ok && result;
    }
    return result;
}

/**
 *  Performs a controlled activation of the jack_assistant and other JACK
 *  modules. Currently does work only for JACK; the activate() calls for other
 *  APIs just return true without doing anything.  However...
 *
 * ca 2021-07-14 Move this. Why doing it even if no JACK transport specified?
 */

bool
performer::activate ()
{
    bool result = m_master_bus && m_master_bus->activate();

#if defined SEQ66_JACK_SUPPORT_ACTIVATE_HERE // init_jack_transport() instead
    if (result)
        result = m_jack_asst.activate();
#endif

    return result;
}

/*
 * -------------------------------------------------------------------------
 *  Tick Support
 * -------------------------------------------------------------------------
 */

void
performer::set_tick (midipulse tick, bool dontreset)
{
    m_tick = tick;
    if (dontreset)
    {
        m_dont_reset_ticks = true;
        set_start_tick(tick);
        set_needs_update();
    }
}

/**
 *  Set the left marker at the given tick.  We let the caller determine if
 *  this setting is a modification.  If the left tick is later than the right
 *  tick, the right tick is move to one measure past the left tick.
 *
 * \todo
 *      The performer::m_one_measure member is currently hardwired to m_ppqn*4.
 *
 * \param tick
 *      The tick (MIDI pulse) at which to place the left tick.  If the left
 *      tick is greater than or equal to the right tick, then the right ticked
 *      is moved forward by one "measure's length" (m_ppqn * 4) past the left
 *      tick.
 */

void
performer::set_left_tick (midipulse tick)
{
    m_left_tick = tick;
    set_start_tick(tick);
    m_reposition = false;
    if (is_jack_master())                       /* don't use in slave mode  */
    {
        position_jack(true, tick);
        set_tick(tick);
    }
    else if (! is_jack_running())
        set_tick(tick);

    if (m_left_tick >= m_right_tick)
        m_right_tick = m_left_tick + m_one_measure;
}

/**
 *  Set the right marker at the given tick.  This setting is made only if the
 *  tick parameter is at or beyond the first measure.  We let the caller
 *  determine is this setting is a modification.
 *
 * \param tick
 *      The tick (MIDI pulse) at which to place the right tick.  If less than
 *      or equal to the left tick setting, then the left tick is backed up by
 *      one "measure's worth" (m_ppqn * 4) worth of ticks from the new right
 *      tick.
 */

void
performer::set_right_tick (midipulse tick)
{
    if (tick == 0)
        tick = m_one_measure;

    if (tick >= m_one_measure)
    {
        m_right_tick = tick;
        if (m_right_tick <= m_left_tick)
        {
            m_left_tick = m_right_tick - m_one_measure;
            set_start_tick(m_left_tick);
            m_reposition = false;
            if (is_jack_master())
                position_jack(true, m_left_tick);
            else
                set_tick(m_left_tick);
        }
    }
}

void
performer::set_left_tick_seq (midipulse tick, midipulse snap)
{
    midipulse remainder = tick % snap;
    if (remainder > (snap / 2))
        tick += snap - remainder;               /* move up to next snap     */
    else
        tick -= remainder;                      /* move down to next snap   */

    if (m_right_tick <= tick)
        set_right_tick_seq(tick + 4 * snap, snap);

    m_left_tick = tick;
    set_start_tick(tick);
    m_reposition = false;
    if (is_jack_master())                       /* don't use in slave mode  */
        position_jack(true, tick);
    else if (! is_jack_running())
        set_tick(tick);
}

void
performer::set_right_tick_seq (midipulse tick, midipulse snap)
{
    midipulse remainder = tick % snap;
    if (remainder > (snap / 2))
        tick += snap - remainder;               /* move up to next snap     */
    else
        tick -= remainder;                      /* move down to next snap   */

    if (tick > m_left_tick)
    {
        m_right_tick = tick;
        set_start_tick(m_left_tick);
        m_reposition = false;
        if (is_jack_master())
            position_jack(true, m_left_tick);
        else
            set_tick(m_left_tick);
    }
}

bool
performer::set_midi_bus (seq::number seqno, int buss)
{
    seq::pointer s = get_sequence(seqno);
    bool result = bool(s);
    if (result)
        result = s->set_midi_bus(buss, true);           /* a user change    */

    return result;
}

/**
 *  The only legal values for channel are 0 through 15, and null_channel(),
 *  which is 0x80, and indicates a "Free" channel (i.e. the pattern is
 *  channel-free.
 *
 *  The live-grid popup-menu calls this function, while the pattern-editor
 *  dropdown calls sequence::set_midi_channel() directly.  Both calls set the
 *  user-change flag.
 *
 * \param seqno
 *      Provides the sequence number for the channel setting.
 *
 * \paramm channel
 *      Provides the channel setting, 0 to 15.  If greater than that, it is
 *      coerced to the null-channel (0x80).
 */

bool
performer::set_midi_channel (seq::number seqno, int channel)
{
    seq::pointer s = get_sequence(seqno);
    bool result = bool(s);
    if (result)
    {
        if (channel >= c_midichannel_max)               /* 0 to 15, Free    */
            channel = null_channel();                   /* Free             */

        result = s->set_midi_channel(midibyte(channel), true);  /* user ch. */
    }
    return result;
}

/**
 *  Also modify()'s.
 */

bool
performer::set_sequence_name (seq::ref s, const std::string & name)
{
    bool result = name != s.name();
    if (result)
    {
        seq::number seqno = s.seq_number();
        s.set_name(name);
        notify_sequence_change(seqno, performer::change::recreate);
        set_needs_update();             /* tell GUIs to refresh. FIXME  */
    }
    return result;
}

/*
 * -------------------------------------------------------------------------
 *  Recording
 * -------------------------------------------------------------------------
 */

/**
 *  Encapsulates code used by the sequence editing frames' recording-change
 *  callbacks.
 *
 * \param recordon
 *      Provides the current status of the Record button.
 *
 * \param thruon  CHANGING TO TOGGLE!
 *      Provides the current status of the Thru button.
 *
 * \param s
 *      The sequence that the seqedit window represents.  This pointer is
 *      checked.
 */

bool
performer::set_recording (seq::ref s, bool recordon, bool toggle)
{
    return s.set_recording(recordon, toggle);
}

/**
 *  Encapsulates code used internally by performer's automation mechanism. This
 *  is a private function.
 *
 * \param seqno
 *      The sequence number; the resulting pointer is checked.
 *
 * \param recordon
 *      Provides the current status of the Record button.
 *
 * \param toggle
 *      If true, ignore the first flag and let the sequence toggle its
 *      setting.  Passed along to sequence::input_recording().
 */

bool
performer::set_recording (seq::number seqno, bool recordon, bool toggle)
{
    sequence * s = get_sequence(seqno).get();
    bool result = not_nullptr(s);
    if (result)
        result = set_recording(*s, recordon, toggle);

    return result;
}

/**
 *  Sets quantized recording in the way used by seqedit.
 *
 * \param recordon
 *      The setting desired for the quantized-recording flag.
 *
 * \param s
 *      Provides the pointer to the sequence to operate upon.  Checked for
 *      validity.
 */

bool
performer::set_quantized_recording (seq::ref s, bool recordon, bool toggle)
{
    return s.set_quantized_recording(recordon, toggle);
}

/**
 *  Sets quantized recording.  This isn't quite consistent with setting
 *  regular recording, which uses sequence::input_recording().
 *
 * \param recordon
 *      Provides the current status of the Record button.
 *
 * \param seq
 *      The sequence number; the resulting pointer is checked.
 *
 * \param toggle
 *      If true, ignore the first flag and let the sequence toggle its
 *      setting.  Passed along to sequence::set_recording().
 */

bool
performer::set_quantized_recording (seq::number seqno, bool recordon, bool toggle)
{
    sequence * s = get_sequence(seqno).get();
    bool result = not_nullptr(s);
    if (result)
        result = set_quantized_recording(*s, recordon, toggle);

    return result;
}

bool
performer::set_tightened_recording
(
    seq::number seqno, bool recordon, bool toggle
)
{
    sequence * s = get_sequence(seqno).get();
    bool result = not_nullptr(s);
    if (result)
        result = set_tightened_recording(*s, recordon, toggle);

    return result;
}

bool
performer::set_tightened_recording (seq::ref s, bool recordon, bool toggle)
{
    return s.set_tightened_recording(recordon, toggle);
}

/**
 *  Set recording for overwrite.  This feature was obtained from jfrey-xx on
 *  GitHub.
 *
 *  Pull request #150: Ask for a reset explicitly upon toggle-on, since we
 *  don't have the GUI to control for progress.  This is implemented in
 *  sequence's version of this function.
 *
 * \param s
 *      The sequence pointer, which is checked.
 *
 * \param oactive
 *      Provides the current status of the overwrite mode.
 *
 * \param toggle
 *      If true, ignore the first flag and let the sequence toggle its
 *      setting.  Passed along to sequence::set_overwrite_rec().
 */

bool
performer::set_overwrite_recording (seq::ref s, bool oactive, bool toggle)
{
    return s.set_overwrite_recording(oactive, toggle);
}

/**
 *  Set recording for overwrite.  This feature was obtained from jfrey-xx on
 *  GitHub.
 *
 * \param oactive
 *      Provides the current status of the overwrite mode.
 *
 * \param seq
 *      The sequence number; the resulting pointer is checked.
 *
 * \param toggle
 *      If true, ignore the first flag and let the sequence toggle its
 *      setting.  Passed along to sequence::set_overwrite_rec().
 */

bool
performer::set_overwrite_recording
(
    seq::number seqno, bool oactive, bool toggle
)
{
    sequence * s = get_sequence(seqno).get();
    bool result = not_nullptr(s);
    if (result)
        result = set_overwrite_recording(*s, oactive, toggle);

    return result;
}

/**
 *  Encapsulates code used by seqedit::thru_change_callback().
 *
 * \param thruon
 *      Provides the current status of the Thru button.
 *
 * \param toggle
 *      Indicates to toggle the status.
 *
 * \param s
 *      The sequence that the seqedit window represents.  This pointer is
 *      checked.
 */

bool
performer::set_thru (seq::ref s, bool thruon, bool toggle)
{
    return s.set_thru(thruon, toggle);
}

/**
 *  Encapsulates code used by seqedit::thru_change_callback().  However, this
 *  function depends on the sequence, not the seqedit, for obtaining the
 *  recording status.
 *
 * \param thruon
 *      Provides the current status of the Thru button.
 *
 * \param seq
 *      The sequence number; the resulting pointer is checked.
 *
 * \param toggle
 *      If true, ignore the first flag and let the sequence toggle its
 *      setting.  Passed along to sequence::set_input_thru().
 */

bool
performer::set_thru (seq::number seqno, bool thruon, bool toggle)
{
    sequence * s = get_sequence(seqno).get();
    bool result = not_nullptr(s);
    if (result)
        result = set_thru(*s, thruon, toggle);

    return result;
}

/*
 * -------------------------------------------------------------------------
 *  JACK Transport
 * -------------------------------------------------------------------------
 */

/**
 *  Encapsulates behavior needed by perfedit.  Note that we moved some of the
 *  code from perfedit::set_jack_mode() [the seq32 version] to this function.
 *
 * \param connect
 *      Indicates if JACK is to be connected versus disconnected.
 *
 * \return
 *      Returns true if JACK is running currently, and false otherwise.
 */

bool
performer::set_jack_mode (bool connect)
{
    if (! is_running())
    {
        if (connect)
            (void) init_jack_transport();
        else
            (void) deinit_jack_transport();
    }
#if defined SEQ66_JACK_SUPPORT
    m_jack_asst.set_jack_mode(is_jack_running());    /* seqroll keybinding  */
#endif

    /*
     *  For setting the transport tick to display in the correct location.
     *  FIXME: does not work for slave from disconnected; need JACK position.
     */

    if (song_mode())
    {
        set_reposition(false);
        set_start_tick(get_left_tick());
    }
    else
        set_start_tick(get_tick());

    return is_jack_running();
}

/**
 *
 * \param tick
 *      The current JACK position in ticks.
 *
 * \param stoptick
 *      The current JACK stop-tick.
 */

void
performer::jack_reposition (midipulse tick, midipulse stoptick)
{
    midipulse diff = tick - stoptick;
    if (diff != 0)
    {
        set_reposition(true);
        set_start_tick(tick);
        jack_stop_tick(tick);
    }
}

/**
 *  Set up the performance and start the thread.  This function should be
 *  considered the "worker thread".  We rely on C++11's thread handling to set
 *  up the thread properly on Linux and Windows.  It runs while m_io_active is
 *  true, which is set in the constructor, stays that way basically for the
 *  duration of the application.  We do not use std::unique_lock<std::mutex>,
 *  because we want a recursive mutex.
 *
 * \warning
 *      Valgrind shows that output_func() is being called before the JACK
 *      client pointer is being initialized!!!
 *
 *  See the old global output_thread_func() in Sequencer64.  This locking is
 *  similar to that of inner_start(), except that signalling (notification) is
 *  not done here. While running, we:
 *
 *      -#  Before the "is-running" loop:  If in any view (song, grid, or pattern
 *          editor), we care about starting from the m_start_tick offset.
 *          However, if the pause key is what resumes playback, we do not want
 *          to reset the position.  So how to detect that situation, since
 *          m_is_pause is now false?
 *      -#  At the top of the "is-running" loop:
 *          -# Get delta time (current - last).
 *          -# Get delta ticks from time.
 *          -# Add to current_ticks.
 *          -# Compute prebuffer ticks.
 *          -# Play from current tick to prebuffer.
 *      -#  Delta time to ticks; get delta ticks.  seq24 0.9.3 changes
 *          delta_tick's type and adds code -- delta_ticks_frac is in 1000th of
 *          a tick.  This code is meant to correct for clock drift.  However,
 *          this code breaks the MIDI clock speed.  So we use the "Stazed"
 *          version of the code, from seq32.  We get delta ticks, delta_ticks_f
 *          is in 1000th of a tick.
 *
 * microsleep() call:
 *
 *      Figure out how much time we need to sleep, and do it.  Then we want to
 *      trigger every c_thread_trigger_width_us; it took delta_us microseconds
 *      to play().  Also known as the "sleeping_us".  Check the MIDI clock
 *      adjustment:  60000000.0f / m_ppqn * / bpm.
 *
 *      With the long patterns in a rendition of Kraftwerk's "Europe Endless",
 *      we found that that the Qt thread was getting starved, as evidenced by
 *      a great slow-down in counting in the timer function in qseqroll. And
 *      many other parts of the user interface were slow.  This bug did not
 *      appear in version 0.91.3, but we never found out the difference that
 *      caused it.  However, a microsleep(1) call mitigated the problem
 *      without causing playback issues.
 *
 *      What might be happening is the the call on line 3144 is not being
 *      made.  But it's not being called in 0.91.3 either!  However, we found
 *      we were using milliseconds, not microseconds!  Once corrected, we
 *      were getting sleep deltas from 3800, down to around 1000 as the load
 *      level (number of playing patterns) increased.
 *
 *      Now, why the 2.0 factor in this?
 *
 *          if (next_clock_delta_us < (c_thread_trigger_width_us * 2.0))
 *
 * Stazed code (when ready):
 *
 *      If we reposition key-p, FF, rewind, adjust delta_tick for change then
 *      reset to adjusted starting.  We have to grab the clock tick if looping
 *      is unchecked while we are running the performance; we have to
 *      initialize the MIDI clock (send EVENT_MIDI_SONG_POS); we have to
 *      restart at the left marker; and reset the tempo list (which Seq64
 *      doesn't have).
 */

void
performer::output_func ()
{
    if (! set_timer_services(true))         /* wrapper for Win-only func.   */
    {
        (void) set_timer_services(false);
        return;
    }
    show_cpu();
    while (m_io_active)                     /* this variable is now atomic  */
    {
        cv().wait();                        /* lock mutex, predicate wait   */
        if (done())                         /* if stopping, kill the thread */
            break;

        pad().initialize(0, looping(), song_mode());

        /*
         * If song-mode Master, then start the left tick marker if the Stazed
         * "key-p" position was set.  If live-mode master, start at 0.  This
         * code is also present at about line #3209, and covers more
         * complexities.
         */

        if (! m_dont_reset_ticks)           /* no repositioning in pause    */
        {
            if (song_mode())
            {
                if (is_jack_master() && m_reposition)
                    position_jack(true, get_left_tick());
            }
            else
                position_jack(false, 0);
        }

        /*
         *  See note 1 in the function banner.
         */

        midipulse startpoint;
        if (m_dont_reset_ticks)
            startpoint = get_tick();
        else if (looping())
            startpoint = get_left_tick();
        else
            startpoint = get_start_tick();

        pad().set_current_tick(startpoint);
        set_last_ticks(startpoint);

        /*
         * We still need to make sure the BPM and PPQN changes are airtight!
         * Check jack_set_beats_per_minute() and change_ppqn()
         */

        double bwdenom = 4.0 / get_beat_width();
        midibpm bpmfactor = m_master_bus->get_beats_per_minute() * bwdenom;
        int ppqn = m_master_bus->get_ppqn();
        int bpm_times_ppqn = bpmfactor * ppqn;
        double dct = double_ticks_from_ppqn(ppqn);
        double pus = pulse_length_us(bpmfactor, ppqn);
        long current;                           /* current time             */
        long elapsed_us, delta_us;              /* current - last           */
        long last = microtime();                /* beginning time           */
        m_resolution_change = false;            /* BPM/PPQN                 */
        while (is_running())
        {
            if (m_resolution_change)            /* an atomic boolean        */
            {
                bwdenom = 4.0 / get_beat_width();
                bpmfactor = m_master_bus->get_beats_per_minute() * bwdenom;
                ppqn = m_master_bus->get_ppqn();
                bpm_times_ppqn = bpmfactor * ppqn;
                dct = double_ticks_from_ppqn(ppqn);
                pus = pulse_length_us(bpmfactor, ppqn);
                m_resolution_change = false;
            }

            /**
             *  See note 2 and the microsleep() note in the function banner.
             *  See note 3 in the function banner.
             */

            current = microtime();
            delta_us = elapsed_us = current - last;

            long long delta_tick_num = bpm_times_ppqn * delta_us +
                pad().js_delta_tick_frac;

            long delta_tick = long(delta_tick_num / 60000000LL);
            pad().js_delta_tick_frac = long(delta_tick_num % 60000000LL);
            if (m_usemidiclock)
            {
                delta_tick = m_midiclocktick;       /* int to long          */
                m_midiclocktick = 0;
                if (m_midiclockpos >= 0)            /* was after this if    */
                {
                    delta_tick = 0;
                    pad().set_current_tick(midipulse(m_midiclockpos));
                    m_midiclockpos = -1;
                }
            }

            bool jackrunning = jack_output(pad());
            if (jackrunning)
            {
                // No additional code needed besides the output() call above.
            }
            else
            {
#if defined USE_THIS_STAZED_CODE_WHEN_READY
                if (song_mode() && ! m_usemidiclock && m_reposition)
                {
                    current_tick = clock_tick;
                    delta_tick = m_start_tick - clock_tick;
                    init_clock = true;
                    m_start_tick = get_left_tick();
                    m_reposition = false;
                    m_reset_tempo_list = true;
                }
#endif
                pad().add_delta_tick(delta_tick);   /* add to current ticks */
            }

            /*
             * pad().js_init_clock will be true when we run for the first time,
             * or as soon as JACK gets a good lock on playback.
             */

            if (pad().js_init_clock)
            {
                m_master_bus->init_clock(midipulse(pad().js_clock_tick));
                pad().js_init_clock = false;
            }
            if (pad().js_dumping)
            {
                if (looping())
                {
                    /*
                     * This stazed JACK code works better than the original
                     * code, so it is now permanent code.
                     */

                    static bool jack_position_once = false;
                    midipulse rtick = get_right_tick();     /* can change? */
                    if (pad().js_current_tick >= rtick)
                    {
                        if (is_jack_master() && ! jack_position_once)
                        {
                            position_jack(true, get_left_tick());
                            jack_position_once = true;
                        }

                        double leftover_tick = pad().js_current_tick - rtick;
                        if (jack_transport_not_starting())  /* no FF/RW xrun */
                        {
                            play(rtick - 1);
                        }
                        reset_sequences();

                        midipulse ltick = get_left_tick();
                        set_last_ticks(ltick);
                        pad().js_current_tick = double(ltick) + leftover_tick;
                    }
                    else
                        jack_position_once = false;
                }

                /*
                 * Don't play during JackTransportStarting to avoid xruns on
                 * FF or RW.
                 */

                if (jack_transport_not_starting())
                {
                    play(midipulse(pad().js_current_tick));
                }

                /*
                 * The next line enables proper pausing in both old and seq32
                 * JACK builds.
                 */

                set_jack_tick(pad().js_current_tick);
                m_master_bus->emit_clock(midipulse(pad().js_clock_tick));
            }

            /*
             *  See "microsleep() call" in banner.  Code is similar to line
             *  3096 above.
             */

            last = current;
            current = microtime();
            elapsed_us = current - last;
            delta_us = c_thread_trigger_width_us - elapsed_us;

            double next_clock_delta = dct - 1;
            double next_clock_delta_us = next_clock_delta * pus;
            if (next_clock_delta_us < (c_thread_trigger_width_us * 2.0))
                delta_us = long(next_clock_delta_us);

            if (delta_us > 0)
            {
                (void) microsleep(int(delta_us));           /* timing.hpp   */
                m_delta_us = 0;
            }
            else
            {
#if defined SEQ66_PLATFORM_DEBUG
                if (delta_us != 0)
                {
                    print_client_tag(msglevel::warn);
                    fprintf
                    (
                        stderr, "Play underrun %ld us          \r",
                        delta_us
                    );
                    /*
                     * 2022-03-03 Why do we sleep here?
                     *
                     * (void) microsleep(1);
                     */
                }
#endif
                m_delta_us = delta_us;
            }
            if (pad().js_jack_stopped)
                inner_stop();
        }

        /*
         * Disabling this setting allows all of the progress bars (seqroll,
         * perfroll, and the slots in the mainwnd) to stay visible where
         * they paused.  However, the progress still restarts when playback
         * begins again, without some other changes.  m_tick is the progress
         * play tick that determines the progress bar location.
         */

        if (! m_dont_reset_ticks)
        {
            midipulse start = song_mode() ? get_left_tick() : 0 ;
            if (is_jack_master())
                position_jack(song_mode(), start);
            else if (! m_usemidiclock && ! is_jack_running())
                set_tick(start);
        }

        /*
         * This means we leave m_tick at stopped location if in slave mode or
         * if m_usemidiclock == true.
         */

        m_master_bus->flush();
        m_master_bus->stop();
    }
    (void) set_timer_services(false);
}

/**
 *  This function is called by input_thread_func().  It handles certain MIDI
 *  input events.  Many of them are now handled by functions for easier reading
 *  and trouble-shooting (of MIDI clock).
 *
 *  For events less than or equal to SysEx, we call midi_control_event() to
 *  handle the MIDI controls that Sequencer64 supports.  (These are
 *  configurable in the "rc" configuration file.) We test for MIDI control
 *  events even if "dumping".  Otherwise, we cannot handle any more control
 *  events once recording is turned on.  Warning:  This can slow down
 *  recording.
 *
 *  "Dumping" means that we are dumping MIDI input events into a sequence.
 *  It means "recording".
 *
 *  We currently ignore these events on input.  MIGHT NOT BE VALID.  STILL
 *  INVESTIGATING.  EVENT_MIDI_ACTIVE_SENSE and EVENT_MIDI_RESET are filtered
 *  in midi_jack.  Send out the current event, if "dumping".
 *
 *  ev.get_status() ==
 *
 *      EVENT_MIDI_ACTIVE_SENSE  handled elsewhere
 *      EVENT_MIDI_RESET handled elsewhere
 *      EVENT_MIDI_QUARTER_FRAME
 *      EVENT_MIDI_SONG_SELECT
 *      EVENT_MIDI_SONG_F4
 *      EVENT_MIDI_SONG_F5
 *      EVENT_MIDI_TUNE_SELECT
 *      EVENT_MIDI_SYSEX_END
 *      EVENT_MIDI_SYSEX_CONTINUE
 *      EVENT_MIDI_SONG_F9
 *      EVENT_MIDI_SONG_FD
 */

void
performer::input_func ()
{
    if (set_timer_services(true))       /* wrapper for a Windows-only func. */
    {
        while (! done())
        {
            if (! poll_cycle())
                break;
        }
        set_timer_services(false);
    }
}

/**
 *  A helper function for input_func().
 */

bool
performer::poll_cycle ()
{
    bool result = ! done();
    if (result && m_master_bus->poll_for_midi() > 0)
    {
        do
        {
            if (done())
            {
                result = false;
                break;                              /* spurious exit events */
            }

            event ev;
            if (m_master_bus->get_midi_event(&ev))
            {
                if (ev.below_sysex())                       /* below 0xF0   */
                {
#if defined SEQ66_PLATFORM_DEBUG_TMI
                    std::string estr = ev.to_string();
                    status_message("MIDI event", estr);
#endif
                    if (m_master_bus->is_dumping())         /* see banner   */
                    {
                        if (midi_control_event(ev, true))   /* quick check  */
                        {
                            // No code at this time
                        }
                        else
                        {
                            ev.set_timestamp(get_tick());
                            if (m_filter_by_channel)
                                m_master_bus->dump_midi_input(ev);
                            else
                                m_master_bus->get_sequence()->stream_event(ev);
                        }
                    }
                    else
                        (void) midi_control_event(ev);
                }
                else if (ev.is_midi_start())
                {
                    midi_start();
                }
                else if (ev.is_midi_continue())
                {
                    midi_continue();
                }
                else if (ev.is_midi_stop())
                {
                    midi_stop();
                }
                else if (ev.is_midi_clock())
                {
                    midi_clock();
                }
                else if (ev.is_midi_song_pos())
                {
                    midi_song_pos(ev);
                }
                else if (ev.is_tempo())             /* added for issue #76  */
                {
                    /*
                     * Should we do this only if JACK transport is not
                     * enabled?
                     */

                    if (is_jack_master() || ! is_jack_running())
                        (void) set_beats_per_minute(ev.tempo());
                }
                else if (ev.is_sysex())
                {
                    midi_sysex(ev);
                }
#if defined USE_ACTIVE_SENSE_AND_RESET
                else if (ev.is_sense_reset())
                {
                    return false;                   /* see note in banner   */
                }
#endif
                else
                {
                    /* ignore the event */
                }
            }
        } while (m_master_bus->is_more_input());
    }
    return result;
}

/**
 * http://www.blitter.com/~russtopia/MIDI/~jglatt/tech/midispec/ssp.htm
 *
 *      Example: If a Song Position value of 8 is received, then a sequencer
 *      (or drum box) should cue playback to the third quarter note of the
 *      song.  Since there are 24 MIDI Clocks in a quarter note, the first
 *      quarter occurs on a time of 0 MIDI Clocks, the second quarter note
 *      occurs upon the 24th MIDI Clock, and the third quarter note occurs on
 *      the 48th MIDI Clock).
 *
 *      8 MIDI beats * 6 MIDI clocks per MIDI beat = 48 MIDI Clocks.
 *
 *      The MIDI-control check will limit the controls to start, stop and
 *      record only. The function returns a a bool flag indicating whether the
 *      event was used or not. The flag is used to exclude from recording the
 *      events that are used for control purposes and should not be recorded
 *      (dumping).  If the used event is a note on, then the linked note off
 *      will also be excluded.
 *
 * http://midi.teragonaudio.com/tech/midispec/seq.htm
 *
 *      Provides a description of how the following events and Song Position
 *      work.
 *
 * EVENT_MIDI_START:
 *
 *      Starts the MIDI Time Clock.  The Master sends this message, which
 *      alerts the slave that, upon receipt of the very next MIDI Clock
 *      message, the slave should start playback.  MIDI Start puts the slave
 *      in "play mode", and the receipt of that first MIDI Clock marks the
 *      initial downbeat of the song.  MIDI B
 *
 *      Kepler34 does "stop(); set_playback_mode(false); start();" in its
 *      version of this event.  This sets the playback mode to Live mode. This
 *      behavior seems reasonable, though the function names Seq66 uses are
 *      different.  Used when starting from the beginning of the song.  Obey
 *      the MIDI time clock.
 *
 * Issue #76:
 *
 *      Somehow auto_stop() was placed after auto_play().  Brain fart?
 */

void
performer::midi_start ()
{
    auto_stop();
    song_start_mode(sequence::playback::live);
    auto_play();
    m_midiclockrunning = m_usemidiclock = true;
    m_midiclocktick = m_midiclockpos = 0;
    if (rc().verbose())
        infoprint("MIDI Start");
}

/**
 * EVENT_MIDI_CONTINUE:
 *
 *      MIDI continue: start from current position.  Sent immediately
 *      after EVENT_MIDI_SONG_POS, and is used for starting from other than
 *      beginning of the song, or for starting from previous location at
 *      EVENT_MIDI_STOP. Again, converted to Kepler34 mode of setting the
 *      playback mode to Live mode.
 */

void
performer::midi_continue ()
{
    song_start_mode(sequence::playback::live);
    m_midiclockpos = get_tick();
    m_dont_reset_ticks = true;
    m_midiclockrunning = m_usemidiclock = true;

    /*
     * Not sure why, but doing this twice works.
     */

    auto_pause(); auto_play();
    auto_pause(); auto_play();
    if (rc().verbose())
        infoprint("MIDI Continue");
}

/**
 * EVENT_MIDI_STOP:
 *
 *      A master stops the slave simultaneously by sending a MIDI Stop
 *      message. The master may then continue to send MIDI Clocks at the rate
 *      of its tempo, but the slave should ignore these, and not advance its
 *      song position.
 *
 *      Do nothing, just let the system pause.  Since we're not getting ticks
 *      after the stop, the song won't advance when start is received, we'll
 *      reset the position. Or, when continue is received, we won't reset the
 *      position.  We do an inner_stop(); the m_midiclockpos member holds the
 *      stop position in case the next event is "continue".  This feature is
 *      not in Kepler34.
 */

void
performer::midi_stop ()
{
    all_notes_off();
    m_usemidiclock = true;
    m_midiclockrunning = false;
    m_midiclockpos = get_tick();
    auto_stop();
    if (rc().verbose())
        infoprint("MIDI Stop");
}

/**
 * EVENT_MIDI_CLOCK:
 *
 *      MIDI beat clock (MIDI timing clock or simply MIDI clock) is a clock
 *      signal broadcast via MIDI to ensure that MIDI-enabled devices stay in
 *      synchronization. It is not MIDI timecode.  Unlike MIDI timecode, MIDI
 *      beat clock is tempo-dependent. Clock events are sent at a rate of 24
 *      ppqn (pulses per quarter note). Those pulses maintain a synchronized
 *      tempo for synthesizers that have BPM-dependent voices and for
 *      arpeggiator synchronization.  Location information can be specified
 *      using MIDI Song Position Pointer.  Many simple MIDI devices ignore
 *      this message.
 */

void
performer::midi_clock ()
{
#if defined SEQ66_PLATFORM_DEBUG
    if (rc().verbose())
    {
        infoprint("MIDI Clock");
        if (m_midiclockrunning)
            m_midiclocktick += m_midiclockincrement;
        else
            infoprint("Clock not running");
    }
    else
#endif
    if (m_midiclockrunning)
        m_midiclocktick += m_midiclockincrement;
}

/**
 * EVENT_MIDI_SONG_POS:
 *
 *      MIDI song position pointer message tells a MIDI device to cue to a
 *      point in the MIDI sequence to be ready to play.  This message consists
 *      of three bytes of data. The first byte, the status byte, is 0xF2 to
 *      flag a song position pointer message. Two bytes follow the status
 *      byte.  These two bytes are combined in a 14-bit value to show the
 *      position in the song to cue to. The top bit of each of the two bytes
 *      is not used.  Thus, the value of the position to cue to is between
 *      0x0000 and 0x3FFF. The position represents the MIDI beat, where a
 *      sequence always starts on beat zero and each beat is a 16th note.
 *      Thus, the device will cue to a specific 16th note.  Also see the
 *      combine_bytes() function.
 */

void
performer::midi_song_pos (const event & ev)
{
    midibyte d0, d1;
    ev.get_data(d0, d1);
    m_midiclockpos = combine_bytes(d0, d1);
}

/**
 * EVENT_MIDI_SYSEX:
 *
 *      These messages are system-wide messages.  We filter system-wide
 *      messages.  If the master MIDI buss is dumping, set the timestamp of
 *      the event and stream it on the sequence.  Otherwise, use the event
 *      data to control the sequencer, if it is valid for that action.
 *
 *      "Dumping" is set when a seqedit window is open and the user has
 *      clicked the "record MIDI" or "thru MIDI" button.  In this case, if
 *      seq32 support is in force, dump to it, else stream the event, with
 *      possibly multiple sequences set.  Otherwise, handle an incoming MIDI
 *      control event.
 *
 *      Also available (but macroed out) is Stazed's parse_sysex() function.
 *      It seems specific to certain Yamaha devices, but might prove useful
 *      later.
 *
 *      Not sure what to do with this code, so we just show the data if
 *      allowed to.  We would need to add back the non-buss version of the
 *      various sysex() functions.
 */

void
performer::midi_sysex (const event & ev)
{
    if (rc().show_midi())
        ev.print();

    /*
     *  if (rc().pass_sysex())
     *  {
     *      m_master_bus->sysex(&ev);
     *  }
     */
}

/**
 *  Encapsulates a series of calls used in mainwnd.  We've reversed the
 *  start() and start_jack() calls so that JACK is started first, to match all
 *  of the other use-cases for playing that we've found in the code.  Note
 *  that the complementary function, stop_playing(), is an inline function
 *  defined in the header file.
 *
 *  The performer::start() function passes its boolean flag to
 *  performer::inner_start(), which sets the playback mode to that flag; if
 *  that flag is false, that turns off "song" mode.  So that explains why
 *  mute/unmute is disabled.
 *
 * Playback use cases:
 *
 *      These use cases are meant to apply to either a Seq32 or a regular
 *      build of Sequencer64, eventually.  Currently, the regular build does
 *      not have a concept of a "global" perform song-mode flag.
 *
 *      -#  mainwnd.
 *          -#  Play.  If the perform song-mode is "Song", then use that mode.
 *              Otherwise, use "Live" mode.
 *          -#  Stop.  This action is modeless here.  In ALSA, it will cause
 *              a rewind (but currently seqroll doesn't rewind until Play is
 *              clicked, a minor bug).
 *          -#  Pause.  Same processing as Play or Stop, depending on current
 *              status.  When stopping, the progress bars in seqroll and
 *              perfroll remain at their current point.
 *      -#  perfedit.
 *          -#  Play.  Override the current perform song-mode to use "Song".
 *          -#  Stop.  Revert the perfedit setting, in case play is restarted
 *              or resumed via mainwnd.
 *          -#  Pause.  Same processing as Play or Stop, depending on current
 *              status.
 *       -# ALSA versus JACK.  One issue here is that, if JACK isn't "running"
 *          at all (i.e. we are in ALSA mode), then we cannot be JACK Master.
 *
 *  Helgrind shows a read/write race condition in m_start_from_perfedit
 *  bewteen jack_transport_callback() and start_playing() here.  Is inline
 *  function access of a boolean atomic?
 *
 *  song_mode() indicates if the caller wants to start the playback in Song
 *  mode (sometimes erroneously referred to as "JACK mode").  In the seq32
 *  code at GitHub, this flag was identical to the "global_jack_start_mode"
 *  flag, which is true for Song mode, and false for Live mode.  False
 *  disables Song mode, and is the default, which matches seq24.  Generally,
 *  we pass true in this parameter if we're starting playback from the
 *  perfedit window.  It alters the m_start_from_perfedit member, not the
 *  m_song_start_mode member (which replaces the global flag now).
 */

void
performer::start_playing ()
{
    if (song_mode())
    {
       /*
        * Allow to start at key-p position if set; for cosmetic reasons,
        * to stop transport line flicker on start, position to the left
        * tick.
        *
        *   m_jack_asst.position(true, m_left_tick);        // position_jack()
        *
        * The "! m_reposition" doesn't seem to make sense.
        */

        if (! song_recording())
            m_max_extent = get_max_extent();

       if (is_jack_master() && ! m_reposition)
           position_jack(true, get_left_tick());
    }
    else
    {
        if (is_jack_master() && ! m_dont_reset_ticks)       // ca 2021-05-20
            position_jack(false, 0);

        if (resume_note_ons())                              /* for issue #5 */
        {
            for (auto seqi : m_play_set.seq_container())
                seqi->resume_note_ons(get_tick());
        }
    }
    start_jack();
    start();
    for (auto notify : m_notify)
        (void) notify->on_automation_change(automation::slot::start);
}

/**
 *  Pause playback, so that progress bars stay where they are, and playback
 *  always resumes where it left off, at least in ALSA mode, which doesn't
 *  have to worry about being a "slave".
 *
 *  Currently almost the same as stop_playing(), but expanded as noted in the
 *  comments so that we ultimately have more granular control over what
 *  happens.  We're researching the whole sequence of stopping and starting,
 *  and it can be tricky to make correct changes.
 *
 *  We still need to make restarting pick up at the same place in ALSA mode;
 *  in JACK mode, JACK transport takes care of that feature.
 *
 *  User layk noted this call, and it makes sense to not do this here, since
 *  it is unknown at this point what the actual status is.  Note that we STILL
 *  need to FOLLOW UP on calls to pause_playing() and stop_playing() in
 *  perfedit, mainwnd, etc.
 *
 *      is_pattern_playing(false);
 *
 *  But what about is_running()?
 *
 * \param songmode
 *      Indicates that, if resuming play, it should play in Song mode (true)
 *      or Live mode (false).  See the comments for the start_playing()
 *      function.
 */

void
performer::pause_playing ()
{
    m_dont_reset_ticks = true;
    is_running(! is_running());
    stop_jack();
    if (! is_jack_running())
        m_usemidiclock = false;

    reset_sequences(true);                      /* don't reset "last-tick"  */
    send_onoff_play_states(midicontrolout::uiaction::pause);
}

/**
 *  Encapsulates a series of calls used in mainwnd.  Stops playback, turns off
 *  the (new) m_dont_reset_ticks flag, and set the "is-pattern-playing" flag
 *  to false.  With stop, reset the start-tick to either the left-tick or the
 *  0th tick (to be determined, currently resets to 0).  If looping, act like
 *  pause_playing(), but allow reset to the left tick (as opposed to 0).
 */

void
performer::stop_playing ()
{
    m_max_extent = 0;
    if (looping())
    {
        pause_playing();
        m_dont_reset_ticks = false;
    }
    else
    {
        stop_jack();
        stop();
        m_dont_reset_ticks = false;
        for (auto notify : m_notify)
            (void) notify->on_automation_change(automation::slot::stop);
    }
}

void
performer::auto_play ()
{
    bool onekey = false;                /* keys().start() == keys().stop(); */
    bool isplaying = false;
    if (onekey)
    {
        if (is_running())
        {
            stop_playing();
        }
        else
        {
            start_playing();
            isplaying = true;
        }
    }
    else if (! is_running())
    {
        start_playing();
        isplaying = true;
    }
    is_pattern_playing(isplaying);
}

void
performer::auto_pause ()
{
    bool isplaying = false;
    if (is_running())
    {
        pause_playing();
        send_onoff_event(midicontrolout::uiaction::play, false);
        send_onoff_event(midicontrolout::uiaction::panic, true);
    }
    else
    {
        start_playing();
        isplaying = true;
        send_onoff_event(midicontrolout::uiaction::play, true);
        send_onoff_event(midicontrolout::uiaction::panic, false);
    }
    is_pattern_playing(isplaying);
}

void
performer::auto_stop ()
{
    stop_playing();
    is_pattern_playing(false);
}

void
performer::delay_stop ()
{
    auto_stop();
    millisleep(c_delay_stop_ms);
}

/**
 *  Starts the playing of all the patterns/sequences.  This function just runs
 *  down the list of sequences and has them dump their events.  It skips
 *  sequences that have no playable MIDI events.
 *
 *  Note how often the "sp" (sequence) pointer was used.  It was worth
 *  offloading all these calls to a new sequence function.  Hence the new
 *  sequence::play_queue() function.
 *
 *  This function is called twice in a row with the same tick value, causing
 *  notes to be played twice. This happens because JACK "ticks" are 10 times
 *  as fast as MIDI ticks, and the conversion can result in the same MIDI tick
 *  value consecutively, especially at lower PPQN.  However, it also can play
 *  notes twice when the tick changes by a small amount.  Not yet sure what to
 *  do about this.
 *
 * \param tick
 *      Provides the tick at which to start playing.  This value is also
 *      copied to m_tick.
 */

void
performer::play (midipulse tick)
{
    if (tick != get_tick() || tick == 0)                /* avoid replays    */
    {
        bool songmode = song_mode();
        if (m_max_extent > 0 && tick > m_max_extent)
        {
            auto_stop();
            return;
        }
        set_tick(tick);
        for (auto seqi : m_play_set.seq_container())
        {
            if (seqi)
                seqi->play_queue(tick, songmode, resume_note_ons());
            else
                set_error_message("play() on null sequence");
        }
        m_master_bus->flush();                          /* flush MIDI buss  */
    }
}

void
performer::play_all_sets (midipulse tick)
{
    if (tick > get_tick() || tick == 0)                 /* avoid replays    */
    {
        set_tick(tick);
        sequence::playback songmode = song_start_mode();
        mapper().play_all_sets(tick, songmode, resume_note_ons());
        m_master_bus->flush();                          /* flush MIDI buss  */
    }
}

int
performer::count_exportable () const
{
    int result = 0;
    for (int i = 0; i < sequence_high(); ++i)   /* count exportable tracks  */
    {
        if (is_exportable(i))                   /* unmuted, has triggers    */
            ++result;
    }
    return result;
}

/**
 *  Seq66 can split an SMF 0 file into multiple tracks, effectively converting
 *  it to SMF 1, via midi_splitter.  This fucntion performers the opposite
 *  process, creating an SMF 0 track from all the other tracks, for saving as
 *  an SMF 0 file.
 *
 * Prerequisites:
 *
 *  -#  The same prequisites for exporting a song:
 *      -#  Events in each track to be part of the export.
 *      -#  Each track unmuted.
 *      -#  Trigger(s) in the tracks to combine.
 *  -#  At least one valid pattern slot available.  This will normally not be
 *      an issue.
 *
 * Process:
 *
 *  -#  If slot 0 has a pattern, move it to the first open slot.
 *  -#  Set up the destination pattern in slot 0 to be channel-free.
 *  -#  For all other patterns, no matter the set (or in the playset):
 *  -#  Check the export of that pattern for validity.
 *  -#  Make sure all channel events have the desired channel.
 *  -#  Copy that pattern to the performers's pattern clipboard using
 *      performer::copy_sequence(), which replaces the clipboard's
 *      contents.  Alternative:  use cut_sequence().
 *  -#  Merge the clipboard pattern into the destination pattern.
 *  -#  Finalize the file:
 *      -#  Make sure the midifile class gets the SMF value (0) and provides
 *          it to write_midi_file(), for one track.  The performer can store
 *          this format.
 *      -#  midifile::write_song(perf()) is called by the Song Export menu
 *          item in qsmainwnd.
 *      -#  write_header()
 *
 *  We start with slot 0, and search for the first open slot (as a side-effect
 *  of new_sequence() and install_sequence() to put the SMF 0 data.
 */

bool
performer::convert_to_smf_0 (bool remove_old)
{
    int numtracks = count_exportable();
    bool result = numtracks > 0;
    seq::number newslot = seq::unassigned();
    if (result)
    {
        result = new_sequence(newslot, 0);
        if (result)
        {
            seq::pointer s = get_sequence(newslot);
            (void) s->set_name("SMF 0");
            result = s->set_midi_channel(null_channel(), true);
        }
    }
    if (result)
    {
        for (seq::number track = 0; track < sequence_high(); ++track)
        {
            if (track == newslot)
                continue;

            if (is_exportable(track))
            {
                bool ok = copy_sequence(track);         /* to the clipboard */
                if (ok)
                    (void) merge_sequence(newslot);
            }
        }
        if (result)
        {
            /*
             * Remove the exported sequences, then move the SMF 0 track to
             * slot 0.  We use remove_sequence() though it modifies too many
             * times. We don't check for failure because removing any
             * intervening empty slots will fail.
             */

            if (remove_old)
            {
                for (seq::number track = 0; track < sequence_high(); ++track)
                {
                    if (track == newslot)
                        continue;

                    (void) remove_sequence(track);
                }
            }
            if (newslot > 0)
            {
                result = move_sequence(newslot);
                if (result)
                    result = finish_move(0);
            }
            if (result)
            {
                /*
                 * Find the actual last timestamp and use that as the new
                 * length of the sequence, since the user will forget to
                 * modify that.
                 */

                seq::pointer s = get_sequence(newslot);
                if (s)
                {
                    (void) s->extend_length();
                    smf_format(0);
                    notify_sequence_change(newslot, change::recreate);
                }
            }
        }
    }
    return result;
}

/**
 *  For all active patterns/sequences, turn off its playing notes.
 *  Then flush the master MIDI buss.
 */

void
performer::all_notes_off ()
{
    mapper().all_notes_off();
    if (m_master_bus)
        m_master_bus->flush();                      /* flush MIDI buss  */
}

/**
 *  Similar to all_notes_off(), but also sends Note Off events directly to the
 *  active busses.  Adapted from Oli Kester's Kepler34 project.
 */

bool
performer::panic ()
{
    bool result = bool(m_master_bus);
    stop_playing();
    inner_stop();                                   /* force inner stop     */
    mapper().panic();
    if (result)
    {
        /*
         * Works, but can cause lights to remain on at exit. Weird.
         */

        int displaybuss = int(midi_control_out().true_buss());
        m_master_bus->panic(displaybuss);           /* flush the MIDI buss  */
    }
    set_tick(0);
    return result;
}

/**
 *  Toggles the m_hidden flag and sets m_show_hide_pending.  The latter will
 *  be toggled off by the qt5nsmanager, which is the only class that cares
 *  about the pending flag.
 */

bool
performer::visibility (automation::action a)
{
    if (a == automation::action::toggle)
        m_hidden = ! m_hidden;
    else if (a == automation::action::on)
        m_hidden = true;
    else if (a == automation::action::off)
        m_hidden = false;

    m_show_hide_pending = true;
    return true;
}

/*
 * -------------------------------------------------------------------------
 *  Box Selection
 * -------------------------------------------------------------------------
 */

/**
 *  A prosaic implementation of calling a function on the set of stored
 *  sequences.  Used for redrawing selected sequences in the graphical user
 *  interface.
 *
 * \param func
 *      The (bound) function to call for each sequence in the set.  It has two
 *      parameters, the sequence number and a pulse value.  The sequence
 *      number parameter is a place-holder and it obtained here.  The pulse
 *      parameter is bound by the caller to create func().
 *
 * \return
 *      Returns true if at least one set item was found to operate on.
 */

bool
performer::selection_operation (SeqOperation func)
{
    bool result = false;
    for (auto s : m_selected_seqs)
        func(s);                        /* not "*s" */

    return result;
}

/**
 *  Selects the desired trigger for this sequence.  If this is the first
 *  selection, then the sequence is inserted into the box container.
 *
 * \param dropseq
 *      The sequence to operate on.
 *
 * \param droptick
 *      Indicates the trigger to be selected.
 */

void
performer::box_insert (seq::number dropseq, midipulse droptick)
{
    seq::pointer s = get_sequence(dropseq);
    if (s)
    {
        bool can_add_seq = s->selected_trigger_count() == 0;
        if (s->select_trigger(droptick))            /* able to select?      */
        {
            if (can_add_seq)
                m_selected_seqs.insert(dropseq);
        }
    }
}

/**
 *  Unselects only the desired trigger for this sequence.  If there are no
 *  more selected triggers for this sequence, then the sequence is erased from
 *  the box container.
 *
 * \param dropseq
 *      The sequence to operate on.
 *
 * \param droptick
 *      Indicates the trigger to be unselected.
 */

void
performer::box_delete (seq::number dropseq, midipulse droptick)
{
    seq::pointer s = get_sequence(dropseq);
    if (s)
    {
        s->unselect_trigger(droptick);
        if (s->trigger_count() == 0)
            m_selected_seqs.erase(dropseq);
    }
}

/**
 *  If the sequence is not in the "box set", add it.  Otherwise, we are
 *  "reselecting" the sequence, so remove it from the list of selections.
 *  Used in the performerance window's on-button-press event.
 *
 * \param dropseq
 *      The number of the sequence where "the mouse was clicked", in the
 *      performerance roll.
 */

void
performer::box_toggle_sequence (seq::number dropseq, midipulse droptick)
{
    auto s = m_selected_seqs.find(dropseq);
    if (s != m_selected_seqs.end())
        box_delete(*s, droptick);
    else
        box_insert(dropseq, droptick);
}

/**
 *  If the current sequence is not part of the selection, then we need to
 *  unselect all sequences.
 */

void
performer::box_unselect_sequences (seq::number dropseq)
{
    if (m_selected_seqs.find(dropseq) == m_selected_seqs.end())
    {
         unselect_all_triggers();
         m_selected_seqs.clear();
    }
}

/**
 *  Moves the box-selected set of triggers to the given tick.
 *
 * \param tick
 *      The destination location for the trigger.
 */

void
performer::box_move_triggers (midipulse tick)
{
    for (auto s : m_selected_seqs)
    {
        seq::pointer selseq = get_sequence(s);
        if (selseq)                                 /* needlessly safe  */
            selseq->move_triggers(tick, true);
    }
}

/**
 *  Offset the box-selected set of triggers by the given tick amount.
 *
 * \param tick
 *      The destination location for the trigger.
 */

void
performer::box_offset_triggers (midipulse offset)
{
    for (auto s : m_selected_seqs)
    {
        seq::pointer selseq = get_sequence(s);
        if (selseq)                                 /* needlessly safe  */
            selseq->offset_triggers(offset);
    }
}

/*
 * -------------------------------------------------------------------------
 *  Trigger handling
 * -------------------------------------------------------------------------
 */

midipulse
performer::get_max_extent () const
{
    midipulse timelen = get_max_timestamp();
    midipulse triglen = get_max_trigger();
    midipulse result = mapper().max_extent();
    if (triglen > result)
        result = triglen;

    if (timelen > result)
        result = timelen;

    return result;
}

std::string
performer::duration (bool dur) const
{
    midipulse tick = get_max_extent();
    return dur ? pulses_to_time_string(tick) : pulses_to_measure_string(tick) ;
}

/**
 *  Selectes a trigger for the given sequence.
 *
 * \param dropseq
 *      The sequence number that was in play for the location of the mouse in
 *      the (for example) perfedit roll.
 *
 * \param droptick
 *      The location of the mouse horizonally in the perfroll.
 *
 * \return
 *      Returns true if a trigger was there to select, and was selected.
 */

bool
performer::select_trigger (seq::number dropseq, midipulse droptick)
{
    seq::pointer s = get_sequence(dropseq);
    bool result = bool(s);
    if (result)
        result = s->select_trigger(droptick);

    return result;
}

/**
 *  Encapsulates getting the trigger limits without putting the burden on the
 *  caller.  The more code moved out of the user-interface, the better.
 *
 * \param seqno
 *      The number of the sequence of interest.
 *
 * \param droptick
 *      The tick location, basically where the mouse was clicked.
 *
 * \param [out] tick0
 *      The output location for the start of the trigger.
 *
 * \param [out] tick1
 *      The output location for the end of the trigger.
 *
 * \return
 *      Returns true if the sequence is valid and we can select the trigger.
 */

bool
performer::selected_trigger
(
    seq::number seqno, midipulse droptick,
    midipulse & tick0, midipulse & tick1
)
{
    bool result = false;
    seq::pointer s = get_sequence(seqno);
    if (s)
        result = s->selected_trigger(droptick, tick0, tick1);

    return result;
}

/**
 *  Adds a trigger on behalf of a sequence. The Seq24 behavior is that
 *  the beginning of the sequence is snapped to the nearest value that is a
 *  multiple of the sequence length. It grows forward or backward by one whole
 *  sequence length.
 *
 *  With song-record-snap off, we
 *  allow the user to place the trigger anywhere in tick-time, and provide the
 *  whole sequence at that time, which can then be grown in either direction.
 *
 *  With song-record-snap on, we want the beginning of the trigger to go to
 *  the nearest snap, but offer a snap length of 0 to indicate to snap to the
 *  sequence length.
 *
 * \param seqno
 *      Indicates the sequence that needs to have its trigger handled.
 *
 * \param tick
 *      The MIDI pulse number at which the trigger should be handled.
 */

void
performer::add_trigger (seq::number seqno, midipulse tick, midipulse snap)
{
    seq::pointer s = get_sequence(seqno);
    if (s)
    {
        midipulse seqlength = s->get_length();
        if (song_record_snap())
        {
            if (snap == 0)
                snap = seqlength;

            tick -= tick % snap;            // seqlength;
        }

        push_trigger_undo(seqno);
        s->add_trigger(tick, seqlength);    /* offset = 0, fixoffset = true */
        notify_trigger_change(seqno);
    }
}

/**
 *    Delete the existing specified trigger.
 *
 * \param seqno
 *      Indicates the sequence that needs to have its trigger handled.
 *
 * \param tick
 *      The MIDI pulse number at which the trigger should be handled.
 */

void
performer::delete_trigger (seq::number seqno, midipulse tick)
{
    seq::pointer s = get_sequence(seqno);
    if (s)
    {
        push_trigger_undo(seqno);
        s->delete_trigger(tick);
        notify_trigger_change(seqno);
    }
}

/**
 *    Add a new trigger if nothing is selected, otherwise delete the existing
 *    trigger.
 *
 * \param seqno
 *      Indicates the sequence that needs to have its trigger handled.
 *
 * \param tick
 *      The MIDI pulse number at which the trigger should be handled.
 */

void
performer::add_or_delete_trigger (seq::number seqno, midipulse tick)
{
    seq::pointer s = get_sequence(seqno);
    if (s)
    {
        bool state = s->get_trigger_state(tick);
        push_trigger_undo(seqno);
        if (state)
        {
            s->delete_trigger(tick);
        }
        else
        {
            midipulse seqlength = s->get_length();
            s->add_trigger(tick, seqlength);
        }
        notify_trigger_change(seqno);
    }
}

/**
 *  Convenience function for perfroll's split-trigger functionality.
 *
 * \param seqno
 *      Indicates the sequence that needs to have its trigger split.
 *
 * \param tick
 *      The MIDI pulse number at which the trigger should be split.
 *
 * \param splittype
 *      The type of split to perform.
 *
 * \return
 *      Returns true if a split was able to be made.
 */

bool
performer::split_trigger
(
    seq::number seqno,
    midipulse tick,
    trigger::splitpoint splittype
)
{
    bool result = false;
    seq::pointer s = get_sequence(seqno);
    if (s)
    {
        push_trigger_undo(seqno);
        result = s->split_trigger(tick, splittype);
        if (result)
            notify_trigger_change(seqno);
    }
    return result;
}

/**
 *  Convenience function for perfroll's paste-trigger functionality.
 *
 * \param seqno
 *      Indicates the sequence that needs to have its trigger pasted.
 *
 * \param tick
 *      The MIDI pulse number at which the trigger should be pasted.
 */

bool
performer::paste_trigger (seq::number seqno, midipulse tick)
{
    bool result = false;
    seq::pointer s = get_sequence(seqno);
    if (s)
    {
        result = true;
        push_trigger_undo(seqno);
        s->paste_trigger(tick);
        notify_trigger_change(seqno);
    }
    return result;
}

/**
 *  Convenience function for perfroll's paste-or-split-trigger functionality.
 *
 * \param seqno
 *      Indicates the sequence that needs to have its trigger handled.
 *
 * \param tick
 *      The MIDI pulse number at which the trigger should be handled.
 */

bool
performer::paste_or_split_trigger (seq::number seqno, midipulse tick)
{
    bool result = false;
    seq::pointer s = get_sequence(seqno);
    if (s)
    {
        bool state = s->get_trigger_state(tick);
        push_trigger_undo(seqno);
        if (state)
        {
            result = s->split_trigger(tick, trigger::splitpoint::exact);
        }
        else
        {
            result = true;
            s->paste_trigger(tick);
        }
        if (result)
            notify_trigger_change(seqno);
    }
    return result;
}

#if defined USE_INTERSECT_FUNCTIONS

/**
 *  Finds the trigger intersection.
 *
 * \param seqno
 *      The number of the sequence in question.
 *
 * \param tick
 *      The time-location desired.
 *
 * \return
 *      Returns true if the sequence exists and the
 *      sequence::intersect_triggers() function returned true.
 */

bool
performer::intersect_triggers (seq::number seqno, midipulse tick)
{
    bool result = false;
    seq::pointer s = get_sequence(seqno);
    if (s)
        result = s->intersect_triggers(tick);

    return result;
}

#endif  // defined USE_INTERSECT_FUNCTIONS

/**
 *  For every active sequence, call that sequence's push_trigger_undo()
 *  function.  Too bad we cannot yet keep track of all the undoes for the sake
 *  of properly handling the "is modified" flag.
 *
 *  This function now has a new parameter.  Not added to this function is the
 *  seemingly redundant undo-push the seq32 code does; is this actually a
 *  seq42 thing?
 *
 *  Also, there is still an issue with our undo-handling for a single track.
 *  See pop_trigger_undo().
 *
 * \param track
 *      A new parameter (found in the stazed seq32 code) that allows this
 *      function to operate on a single track.  A parameter value of
 *      seq::all() (-2, the default) implements the original behavior.
 */

void
performer::push_trigger_undo (int track)
{
    m_undo_vect.push_back(track);                       /* stazed   */
    if (track == seq::all())
    {
        mapper().push_trigger_undo();
    }
    else
    {
        seq::pointer s = get_sequence(track);
        if (s)
            s->push_trigger_undo();
    }
    set_have_undo(true);                                /* stazed   */
}

/**
 *  For every active sequence, call that sequence's pop_trigger_undo()
 *  function.
 *
 * \todo
 *      Look at seq32/src/perform.cpp and the perform ::
 *      push_trigger_undo(track) function, which has a track parameter that
 *      has a -1 value that supports all tracks.  It requires two new vectors
 *      (one for undo, one for redo), two new flags (likewise).  We've put
 *      this code in place, no longer macroed out, now permanent.
 */

void
performer::pop_trigger_undo ()
{
    if (! m_undo_vect.empty())
    {
        int track = m_undo_vect.back();
        m_undo_vect.pop_back();
        m_redo_vect.push_back(track);
        if (track == seq::all())
        {
            mapper().pop_trigger_undo();
        }
        else
        {
            seq::pointer s = get_sequence(track);
            if (s)
                s->pop_trigger_undo();
        }
        set_have_undo(! m_undo_vect.empty());
        set_have_redo(! m_redo_vect.empty());
    }
}

/**
 *  For every active sequence, call that sequence's pop_trigger_redo()
 *  function.
 */

void
performer::pop_trigger_redo ()
{
    if (! m_redo_vect.empty())
    {
        int track = m_redo_vect.back();
        m_redo_vect.pop_back();
        m_undo_vect.push_back(track);
        if (track == seq::all())
        {
            mapper().pop_trigger_redo();
        }
        else
        {
            seq::pointer s = get_sequence(track);
            if (s)
                s->pop_trigger_redo();
        }
        set_have_undo(! m_undo_vect.empty());
        set_have_redo(! m_redo_vect.empty());
    }
}

/*
 * -------------------------------------------------------------------------
 *  Other handling
 * -------------------------------------------------------------------------
 */

void
performer::show_cpu ()
{
#if defined SEQ66_PLATFORM_LINUX
    if (rc().verbose())
        infoprintf("Output function on CPU #%d", sched_getcpu());
#endif
}

/**
 *  Simple error reporting for debugging. We have to cast the ordinal back to
 *  unsigned, otherwise odd or empty strings are emitted.
 */

void
performer::show_key_error (const keystroke & k, const std::string & tag)
{
    ctrlkey ordinal = k.key();
    std::string name = qt_ordinal_keyname(ordinal);
    std::string pr = k.is_press() ? "Press" : "Release" ;
    std::string mods = modifier_names(unsigned(k.modifiers()));
    std::cerr
        << "Key '" << name
        << "' Ordinal 0x" << std::hex << unsigned(ordinal)
        << " Modifier(s) " << mods
        << ": " << pr << ": "<< tag << std::endl
        ;
}

/**
 *  This static function merely prints the parameters passed to it.
 *  This function, enabled by the --verbose flag (and in the 'rc' file),
 *  allows one to see the incoming automation commands.
 */

void
performer::print_parameters
(
    const std::string & tag, automation::action a,
    int d0, int d1, int index, bool inverse
)
{
    if (rc().verbose())
    {
        std::ostringstream os;
        os << tag << " '" << opcontrol::action_name(a) << "'; "
            << "d0 = " << d0 << "; "
            << "d1 = " << d1 << "; "
            << "index = " << index << "; "
            << "inv = " << inverse
            ;
        (void) info_message(os.str());
    }
}

/*
 * -------------------------------------------------------------------------
 *  Control
 * -------------------------------------------------------------------------
 */

/**
 *  Set the MIDI control output object
 */

void
performer::set_midi_control_out ()
{
    if (m_master_bus)
    {
        mastermidibus * temp = m_master_bus.get();
        midi_control_out().set_master_bus(temp);
    }
}

/**
 *  Sets or unsets the keep-queue functionality, used by the "Q"
 *  button in the main window to set keep-queue.  Also see the
 *  automation_keep_queue() function.
 */

void
performer::set_keep_queue (bool activate)
{
    automation::action a = activate ?
        automation::action::on : automation::action::off;

    (void) set_ctrl_status(a, automation::ctrlstatus::keep_queue);
}

/**
 *  If the given status is present in the automation::ctrlstatus::snapshot,
 *  the playing state is saved.  Then the given status is OR'd into the
 *  control-status.
 *
 *  If the given status is present in the automation::ctrlstatus::snapshot,
 *  the playing state is restored.  Then the given status is reversed in
 *  control-status.
 *
 *  If the given status includes automation::ctrlstatus::queue, this is a
 *  signal to stop queuing (which is already in place elsewhere).  It also
 *  unsets the new queue-replace feature.
 *
 * \param a
 *      The action to be applied.  On sets the status, and Off unsets
 *      the status.
 *
 * \param status
 *      The status item to be applied.
 */

bool
performer::set_ctrl_status
(
    automation::action a,
    automation::ctrlstatus status
)
{
    bool on = a == automation::action::on || a == automation::action::toggle;
    if (on && midi_control_in().is_set(status))
        on = false;

    if (on)
    {
        if (midi_control_in().is_snapshot(status))
            save_snapshot();

        /*
         * Anything to do for solo (queue + replace)?
         */

        midi_control_in().add_status(status);
    }
    else
    {
        bool q = midi_control_in().is_keep_queue(status) ||
            midi_control_in().is_queue(status);

        if (midi_control_in().is_snapshot(status))
            restore_snapshot();

        if (q)
            unset_queued_replace();

        midi_control_in().remove_status(status);
    }
    display_ctrl_status(status, on);
    return true;
}

bool
performer::toggle_ctrl_status (automation::ctrlstatus status)
{
    bool on = ! midi_control_in().is_set(status);
    automation::action a = on ?
        automation::action::on : automation::action::off ;

    return set_ctrl_status(a, status);
}

void
performer::display_ctrl_status (automation::ctrlstatus s, bool on)
{
    if (midi_control_in().is_keep_queue(s))
        send_onoff_event(midicontrolout::uiaction::queue, on);

    if (midi_control_in().is_oneshot(s))
        send_onoff_event(midicontrolout::uiaction::oneshot, on);

    if (midi_control_in().is_replace(s))
        send_onoff_event(midicontrolout::uiaction::replace, on);

    if (midi_control_in().is_snapshot(s))
        send_onoff_event(midicontrolout::uiaction::snap, on);
}

/**
 *  A helper function to make the code a tad more readable.
 */

void
performer::send_onoff_event (midicontrolout::uiaction a, bool on)
{
    midicontrolout::actionindex ai = on ?
        midicontrolout::action_on : midicontrolout::action_off ;

    midi_control_out().send_event(a, ai);
}

/**
 *  A helper function to make the code a tad more readable.
 */

void
performer::send_mutes_event (int group, bool on)
{
    midicontrolout::actionindex a = on ?
        midicontrolout::action_on : midicontrolout::action_off ;

    midi_control_out().send_mutes_event(group, a);
}

void
performer::send_mutes_events (int groupon, int groupoff)
{
    bool wasactive = mutes().group_valid(groupoff);
    if (wasactive && (groupoff != groupon))
    {
        midi_control_out().send_mutes_event
        (
            groupoff, midicontrolout::action_off
        );
    }
    midi_control_out().send_mutes_event(groupon, midicontrolout::action_on);
}

void
performer::send_mutes_inactive (int group)
{
    midi_control_out().send_mutes_event(group, midicontrolout::action_del);
}

/**
 *  Sets the state of the Start, Stop, and Play button(s) as configured in the
 *  "ctrl" file.  It first turns off all of the states (which might be mapped to
 *  one button or to three buttons), then turns on the desired state.
 *
 * \param a
 *      Provides the desired state to set, which is one of the following values
 *      of uiaction: play, stop, and pause.  The corresponding event is sent.
 *      If another value (max is the best one to use), then all are off.
 */

void
performer::send_onoff_play_states (midicontrolout::uiaction a)
{
    if (a < midicontrolout::uiaction::max)
        send_onoff_event(a, true);
    else
        announce_automation();
}

/**
 *  Helper function that clears the queued-replace feature.  This also clears
 *  the queue mode; we shall see if this disrupts any user's workflow.
 *
 * \param clearbits
 *      If true (the default), then clear the queue and replace status bits.  If
 *      the user is simply replacing the current replace pattern with another
 *      pattern, we pass false for this parameter.
 */

void
performer::unset_queued_replace (bool clearbits)
{
    if (m_queued_replace_slot != m_no_queued_solo)
    {
        m_queued_replace_slot = m_no_queued_solo;
        clear_snapshot();
        if (clearbits)
            midi_control_in().remove_queued_replace();
    }
}

/**
 *  Sets the group-mute mode, then the group-learn mode, then notifies all of
 *  the notification subscribers.  This function is called via a MIDI control
 *  c_midi_control_mod_glearn and via the group-learn keystroke.
 *
 * \param learning
 *      If true, sets group-learn mode, otherwise, unsets it.  If false, the
 *      "good" status will refer to the success of memorizing the mute status.
 *
 * \param good
 *      If true, either the learning or the mute-setting succeed.  Otherwise,
 *      the operation failed.
 *
 * \param k
 *      Inidates the keystroke involved in the transaction, useful for
 *      reporting.
 */

void
performer::group_learn (bool learning)
{
    automation::action a = learning ?
        automation::action::on : automation::action::off;

    (void) set_ctrl_status(a, automation::ctrlstatus::learn);
    mutes().group_learn(learning);
    midi_control_out().send_learning(learning);
    for (auto notify : m_notify)
        (void) notify->on_group_learn(learning);
}

void
performer::group_learn_complete (const keystroke & k, bool good)
{
    group_learn(false);
    for (auto notify : m_notify)
        (void) notify->on_group_learn_complete(k, good);
}

/**
 *  If the given sequence is active, then it is toggled as per the current
 *  value of control-status.  If control-status is
 *  automation::ctrlstatus::queue, then the sequence's toggle_queued()
 *  function is called.  This is the "mod queue" implementation.
 *
 *  Otherwise, if it is automation::ctrlstatus::replace, then the status is
 *  unset, and all sequences are turned off.  Then the sequence's
 *  toggle-playing() function is called, which should turn it back on.  This is
 *  the "mod replace" implementation; it is like a Solo.  But can it be undone?
 *
 *  This function is called in loop_control() to implement a toggling of the
 *  sequence of the pattern slot in the current screen-set that is represented
 *  by the keystroke.
 *
 *  This function is also called in midi_control_event() if the control number
 *  represents a sequence number in a screen-set, that is, it ranges from 0 to
 *  31 (by default).  This function also supports the queued-replace
 *  (queued-solo) feature.
 *
 *  One-shots are allowed only if we are not playing this sequence.
 *
 * \param seq
 *      The sequence number of the sequence to be potentially toggled.
 *      This value must be a valid and active sequence number. If in
 *      queued-replace mode, and if this pattern number is different from the
 *      currently-stored number (m_queued_replace_slot), then we clear the
 *      currently stored set of patterns and set new stored patterns.
 */

bool
performer::sequence_playing_toggle (seq::number seqno)
{
    seq::pointer s = get_sequence(seqno);
    bool result = bool(s);
    if (result)
    {
        bool is_queue = midi_control_in().is_queue();
        bool is_keep_q = midi_control_in().is_keep_queue();
        bool is_replace = midi_control_in().is_replace();
        bool is_oneshot = midi_control_in().is_oneshot();
        if (is_oneshot && ! s->playing())
        {
            s->toggle_one_shot();                   /* why not just turn on */
        }
        else if (is_queue || is_keep_q)
        {
            if (is_replace)                         /* not in Seq32         */
            {
                if (m_queued_replace_slot != m_no_queued_solo)
                {
                    if (seqno != m_queued_replace_slot)
                    {
                        unset_queued_replace(false);    /* don't clear bits */
                        save_queued(seqno);
                    }
                }
                else
                {
                    save_queued(seqno);
                }
                unqueue_sequences(seqno);
                m_queued_replace_slot = seqno;
            }
            else
            {
                s->toggle_queued();
            }
        }
        else
        {
            if (is_replace)
            {
                (void) set_ctrl_status
                (
                    automation::action::off,
                    automation::ctrlstatus::replace
                );
                off_sequences();
            }
            s->toggle_playing(get_tick(), resume_note_ons());   /* kepler34 */
        }

        /*
         * For issue #89, sequence::toggle_playing() already announces the
         * sequence change, so don't do it here.
         *
         * announce_sequence(s, mapper().seq_to_offset(*s));
         */

        /*
         * If we're in song playback, temporarily block the events until the
         * next sequence boundary. And if we're recording, add "Live" sequence
         * playback changes to the Song/Performance data as triggers.
         *
         * \todo
         *      Would be nice to delay song-recording start to the next queue,
         *      if queuing is active for this sequence.
         */

        if (song_mode())
            s->song_playback_block(true);

        if (song_recording())
        {
            midipulse seq_length = s->get_length();
            midipulse tick = get_tick();
            bool trigger_state = s->get_trigger_state(tick);
            if (trigger_state)              /* if sequence already playing  */
            {
                /*
                 * If this play is us recording live, end the new trigger
                 * block here.
                 */

                if (s->song_recording())
                {
                    s->song_recording_stop(tick);
                }
                else        /* ...else need to trim block already in place  */
                {
                    /*
                     * Hmmm, for issue #44, can we make the splitpoint an
                     * option?
                     */

                    s->split_trigger(tick, trigger::splitpoint::exact);
                    s->delete_trigger(tick);
                }
            }
            else            /* if not playing, start recording a new strip  */
            {
                if (song_record_snap())         /* see Seq64 issue #171 !   */
                    tick -= tick % seq_length;

                /*
                 * Why is this needed? It leads to a modify() when just
                 * double-clicking on an existing pattern.
                 *
                 * push_trigger_undo();
                 */

                s->song_recording_start(tick, song_record_snap());
            }
        }
    }
    return result;
}

bool
performer::replace_for_solo (seq::number seqno)
{
    seq::pointer s = get_sequence(seqno);
    bool result = bool(s);
    if (result)
    {
        (void) set_ctrl_status
        (
            automation::action::off,
            automation::ctrlstatus::replace
        );
        off_sequences();
        s->toggle_playing(get_tick(), resume_note_ons());   /* kepler34 */
        announce_sequence(s, mapper().seq_to_offset(*s));
    }
    return result;
}

sequence::playback
performer::toggle_song_start_mode ()
{
    song_start_mode
    (
        live_mode() ? sequence::playback::song : sequence::playback::live
    );
    if (song_mode())
        (void) unapply_mutes(mutes().null_mute_group());

    infoprint(live_mode() ? "Live Mode" : "Song Mode");
    return m_song_start_mode;
}

void
performer::song_recording (bool f)
{
    if (f != m_song_recording)
    {
        m_song_recording = f;
        if (! f)
            song_recording_stop();      /* stops recording on all sequences */
    }
}

/**
 *  This code handles the use of the Shift key to toggle the mute state of all
 *  other sequences.  See perfnames::on_button_press_event().  If the Shift
 *  key is pressed, toggle the mute state of all other sequences.
 *  Inactive sequences are skipped.  Compare it to toggle_other_seqs().
 *
 * \param seqno
 *      The sequence that is being clicked on.  It must be active in order to
 *      allow toggling.
 *
 * \param isshiftkey
 *      Indicates if the shift-key functionality for toggling all of the other
 *      sequences is active.
 *
 * \return
 *      Returns true if the toggling was able to be performed.
 */

bool
performer::toggle_other_names (seq::number seqno, bool isshiftkey)
{
    bool result = is_seq_active(seqno);
    if (result)
    {
        if (isshiftkey)
            mapper().toggle_song_mute();
        else
            mapper().toggle_song_mute(seqno);
    }
    return result;
}

/**
 *  Changes the play-state of the given sequence.  This does not cause a modify
 *  action.
 *
 * \param seqno
 *      The number of the sequence to be turned on or off.
 *
 * \param on
 *      The state (true = armed) to which to set the sequence.
 */

bool
performer::sequence_playing_change (seq::number seqno, bool on)
{
    bool qinprogress = midi_control_in().is_queue();
    mapper().sequence_playscreen_change(seqno, on, qinprogress);

    /*
     * Too much maybe: notify_trigger_change(seqno, change::no);
     */

    return true;
}

/*
 * Seq/Event-edit pending flag support.
 */

/**
 *  Sets the edit-pending flags to false, and disabled the pending sequence
 *  number.
 */

void
performer::clear_seq_edits ()
{
    m_seq_edit_pending = m_event_edit_pending = false;
    m_pending_loop = seq::unassigned();
}

/*
 * End of Seq/Event-edit pending flag support.
 */

/**
 *  Handle a control key.  The caller (e.g. a Qt key-press event handler)
 *  grabs the event text and modifiers and converts it to an ctrlkey value
 *  (ranging from 0x00 to 0xFE).  We show the caller code here for reference:
 *
\verbatim
        ctrlkey kkey = event->key();
        unsigned kmods = static_cast<unsigned>(event->modifiers());
        ctrlkey ordinal = qt_modkey_ordinal(kkey, kmods);
        keystroke ks = keystroke(ordinal, press);
\endverbatim
 *
 *  We made a Qt function for this, qt_keystroke(), in the qt5_helpers.cpp/hpp
 *  module.
 *
 *  Next, we look up the keycontrol based on the ordinal value.  If this
 *  keycontrol is usable (it is not a default-constructed keycontrol), then we
 *  can use its slot value to look up the midioperation associated with this
 *  slot.
 *
 *  Also part of keystroke is whether the key was pressed or released.  A
 *  press sets inverse = false, while a release sets inverse = true.  For some
 *  keystrokes, this difference matters.  For most, we want to ignore the
 *  release.
 *
 *  Note that the default action for most keys is automation::action::toggle,
 *  but some keys are configured to do automation::action::on during a
 *  key-press, and automation::action::off during a key-release.  To
 *  summarize:
 *
 *      -   Pattern keys. The action is always automation::action::toggle
 *          upon a key-press. Key-release is ignored.
 *      -   Mute-group keys. The action is always automation::action::toggle
 *          upon a key-press. Key-release is ignored.
 *      -   Automation keys. The action is specified by the location in the
 *          list, as defined in keycontainer::sm_keys_automation.
 *          -   on.  Causes an action, such as BPM Up.  Operate only upon
 *              key-press.
 *          -   toggle.  Operate a toggling operation.  Operate only upon
 *              key-press.
 *          -   off.  Not used with keystrokes, just with MIDI control.
 *      -   Modal keys.  The action is automation::action::on.  But the state
 *          of the key-press is used.  If a press, the mode is activated.  If
 *          a release, the mode is deactivated. This operating mode is
 *          determined by the automation callback function.
 *
 * Example:
 *
 *      -   BPM Up. Uses automation::action::on, and increments BPM at each
 *          key-press, with key-release being ignored.
 *      -   Pause.  Uses automation::action::toggle.  At each key-press, the
 *          playing state is toggled.  Key-release is ignored.
 *      -   Replace, queue, snapshot, one-shot, and group-learn.  Uses
 *          automation::action::on.  On key-press, enter the given mode. On
 *          key-release, leave the given mode.
 *
 *  If the midioperation is usable, then we can call the midioperation
 *  function, passing it the parameters based on the keystroke.
 *
 * Notes:
 *
 *  1.  Note that the "inverse" parameter is based on key press versus release.
 *      Not all automation functions care about this setting.  The
 *      opcontrol::allowed(int, bool) function checks for the
 *      non-keystroke-release status. Too tricky. Also, the index is meant
 *      only for pattern and mute-group control.
 *
 *  2.  If we start group-learn mode, and then press the group-learn key,
 *      we're in learning mode, but should not learn that key.  We still need
 *      to report it, though.
 *
 * \param key
 *      Provides the ordinal number of the keystroke.  See above for how to
 *      get it (in Qt 5).
 *
 * \return
 *      Returns true if the action was handled.  Returns false if the action
 *      failed or was not handled.  The caller has to know what the context
 *      is.
 */

bool
performer::midi_control_keystroke (const keystroke & k)
{
    bool result = true;
    keystroke kkey = k;
    if (is_group_learn())
    {
        if (kkey.is_press())
        {
            if (m_key_controls.use_auto_shift())
                kkey.shift_lock();      /* employ the auto-shift feature    */
        }
        else
            result = false;             /* ignore the control-key release   */
    }
    if (result)
    {
        const keycontrol & kc = m_key_controls.control(kkey.key());
        result = kc.is_usable();
        if (result)
        {
            automation::slot s = kc.slot_number();
            const midioperation & mop = m_operations.operation(s);
            if (mop.is_usable())
            {
                /*
                 * See Notes 1 (inverse) and 2 (group-learn) in the banner.
                 */

                automation::action a = kc.action_code();
                bool invert = ! kkey.is_press();
                int d0 = (-1);                                  /* key flag */
                int d1 = 0;                                     /* no d1    */
                int index = kc.control_code();
                bool learning = is_group_learn();               /* before   */
                if (kc.is_glearn_control())                     /* ignore?  */
                {
                    if (invert)
                    {
                        result = false;
                    }
                    else if (learning)
                    {
                        group_learn_complete(kkey, false);      /* fail     */
                        group_learn(false);
                        result = false;
                    }
                }
                if (result)
                    result = mop.call(a, d0, d1, index, invert);

                if (result)
                {
                    if (learning)
                        group_learn_complete(kkey, ! is_group_learn());
                }
                else
                {
                    /*
                     * Using the "=" or "-" keys deliberately returns false.
                     */

                    if (! m_seq_edit_pending && ! m_event_edit_pending)
                        show_key_error(kkey, "call returned false");
                }
            }
            else
                show_key_error(kkey, "call unusable");
        }
    }
    return result;
}

/**
 *  Looks up the MIDI event and calls the corresponding function, if any.
 *
 * Note:
 *
 *      Pattern-edit can turn recording on, potentially disabling the next
 *      pattern-edit, so we check for it here.  Anything else to check???  We
 *      actually need to see if there is any control that CANNOT occur while
 *      recording, otherwise loop-control etc. is disabled as well!
 *
 * \param ev
 *      The MIDI event to process.
 *
 * \param recording
 *      This parameter, if true, restricts the handled controls to start,
 *      stop, and record.
 *
 * \return
 *      Returns true if the event was valid and usable, and the call to the
 *      automation function returned true. Also returns true if the event
 *      came in on the control buss, so that it will not be recorded. (A fix
 *      for issue #80.) Returns false if the action was not on the control
 *      bus.
 */

bool
performer::midi_control_event (const event & ev, bool recording)
{
    bool result = m_midi_control_in.is_enabled();
    if (result)
        result = ev.input_bus() == m_midi_control_in.true_buss();

    if (result)
    {
        midicontrol::key k(ev);
        const midicontrol & incoming = m_midi_control_in.control(k);
        bool good = incoming.is_usable();
        if (good)
        {
            automation::slot s = incoming.slot_number();
            const midioperation & mop = m_operations.operation(s);
            if (mop.is_usable())
            {
                bool process_the_action = incoming.in_range(ev.d1());
                if (recording)
                {
                    /*
                     * See Note above.
                     */
                }
                if (process_the_action)
                {
                    automation::action a = incoming.action_code();
                    bool invert = incoming.inverse_active();
                    int d0 = incoming.d0();
                    int d1 = incoming.d1();
                    int index = incoming.control_code(); /* in lieu of d1() */
                    good = mop.call(a, d0, d1, index, invert);
                }
                else
                    good = false;
            }
        }
#if defined SEQ66_PLATFORM_DEBUG
        if (! good)
            printf("Control event not processed\n");
#endif
    }
    return result;
}

void
performer::signal_save ()
{
    stop_playing();
    signal_for_save();              /* provided by the daemonize module */
}

void
performer::signal_quit ()
{
    stop_playing();
    signal_for_exit();              /* provided by the daemonize module */
}

/**
 *  Adds a member function to an automation slot.
 */

bool
performer::add_automation (automation::slot s, automation_function f)
{
    std::string name = opcontrol::category_name
    (
        automation::category::automation
    );
    midioperation func
    (
        name, automation::category::automation, s,
        [this, f]
        (
            automation::action a, int d0, int d1,
            int index, bool inverse
        )
        {
            return (this->*f) (a, d0, d1, index, inverse);
        }
    );
    return m_operations.add(func);
}

/**
 *  Tries to populate the opcontainer with simulated versions of a pattern
 *  control function, a mute-group control function, and functions to handle
 *  each of the automation controls.
 *
 *  Pattern:
 *
 *      A single loop-control function, a lambda function that calls
 *      performer::loop_control().
 *
 *  Mute-group:
 *
 *      A single mute-control function, a lambda function that calls
 *      performer::mute_group_control().
 *
 *  Automation:
 *
 *      A number of performer functions stuffed into lambdas.  See the
 *      sm_auto_func_list[] array.  Works like a champ and a lot fewer lines of
 *      code.  See the bottom of this file for the function table.
 */

bool
performer::populate_default_ops ()
{
    midioperation patmop
    (
        opcontrol::category_name(automation::category::loop),   /* name     */
        automation::category::loop,                             /* category */
        automation::slot::loop,                                 /* opnumber */
        [this]
        (
            automation::action a, int d0, int d1,
            int index, bool inverse
        )
        {
            return loop_control(a, d0, d1, index, inverse);
        }
    );
    bool result = m_operations.add(patmop);
    if (result)
    {
        midioperation mutmop
        (
            opcontrol::category_name(automation::category::mute_group),
            automation::category::mute_group,
            automation::slot::mute_group,                       /* opnumber */
            [this]
            (
                automation::action a, int d0, int d1,
                int index, bool inverse
            )
            {
                return mute_group_control(a, d0, d1, index, inverse);
            }
        );
        result = m_operations.add(mutmop);
    }
    for (int index = 0; /* breaker */ ; ++index)
    {
        if (sm_auto_func_list[index].ap_slot != automation::slot::max)
        {
            result = add_automation
            (
                sm_auto_func_list[index].ap_slot,
                sm_auto_func_list[index].ap_function
            );
            if (! result)
            {
                std::string errmsg = "Failed to insert automation function #";
                errmsg += std::to_string(index);
                set_error_message(errmsg);
                break;
            }
        }
        else
            break;
    }
    return result;
}

/*
 * -------------------------------------------------------------------------
 *  Mutes / Mute-groups
 * -------------------------------------------------------------------------
 */

bool
performer::group_name (mutegroup::number gmute, const std::string & n)
{
    bool result = (mutes().group_save_to_midi() &&
        n != mutes().group_name(gmute));

    mutes().group_name(gmute, n);
    if (result)
        modify();

    return result;
}

void
performer::group_format_hex (bool flag)
{
    if (flag != mutes().group_format_hex())
        modify();

    mutes().group_format_hex(flag);
}

bool
performer::group_save (bool bmidi, bool bmutes)
{
    bool result = bmidi != group_save_to_midi();
    mutes().group_save(bmidi, bmutes);
    if (result)
        modify();

    return result;
}

bool
performer::strip_empty (bool flag)
{
    bool result = flag != mutes().strip_empty();
    mutes().strip_empty(flag);
    if (result)
        modify();

    return result;
}

/**
 *  Sets the given mute group.  If there is a change,  then the subscribers are
 *  notified.  If the 'rc' "save-mutes-to" setting indicates saving it to the
 *  MIDI file, then this becomes a modify action.  Associated with the "Update
 *  Group" button in the qmutemaster tab.
 *
 * \param gmute
 *      Provides the number of the mute-group to be updated.
 *
 * \param bits
 *      Provides the bits representing the layout of the mute-group's
 *      armed/unarmed statuses.
 *
 * \param putmutes
 *      If true, then the mute-group in the rc() mute-groups object is
 *      updated.
 *
 * \return
 *      Returns true if the mutes were able to be set.
 */

bool
performer::set_mutes
(
    mutegroup::number gmute,
    const midibooleans & bits,
    bool putmutes
)
{
    midibooleans original = get_mutes(gmute);
    bool result = bits != original;
    if (result)
    {
        result = mapper().set_mutes(gmute, bits);
        if (result)
        {
            change c = mutes().group_save_to_midi() ?
                change::yes : change:: no ;

            notify_mutes_change(mutegroup::unassigned(), c);
            if (putmutes)
                mutes().set(gmute, bits);
        }
    }
    return result;
}

/**
 *  Clears the mute groups.  If there any to clear, then the subscribers are
 *  notified.  If the "mutes" "save-mutes-to" setting indicates saving it to
 *  the MIDI file, then this becomes a modify action. Compare to the
 *  clear_mute_groups() function.
 */

bool
performer::clear_mutes ()
{
    bool result = mutes().any();
    if (result)
    {
        result = mutes().reset_defaults();      /* clears and adds all 0s   */
        if (result)
        {
            change c = mutes().group_save_to_midi() ?
                change::yes : change::no ;

            notify_mutes_change(mutegroup::unassigned(), c);
        }
    }
    return result;
}

bool
performer::clear_mute_groups ()
{
    bool result = mutes().clear();              /* clears mutes container   */
    if (result)
        modify();

    return result;
}

bool
performer::apply_mutes (mutegroup::number group)
{
    mutegroup::number oldgroup = mutes().group_selected();
    bool result = mapper().apply_mutes(group);
    if (result)
        send_mutes_events(group, oldgroup);

    return result;
}

bool
performer::unapply_mutes (mutegroup::number group)
{
    bool result = mapper().unapply_mutes(group);
    if (result)
        midi_control_out().send_mutes_event(group, midicontrolout::action_off);

    return result;
}

bool
performer::toggle_mutes (mutegroup::number group)
{
    mutegroup::number oldgroup = mutes().group_selected();
    bool result = mapper().toggle_mutes(group);
    if (result)
    {
        mutegroup::number newgroup = mutes().group_selected();
        send_mutes_events(newgroup, oldgroup);
    }
    return result;
}

bool
performer::toggle_active_mutes (mutegroup::number group)
{
    mutegroup::number oldgroup = mutes().group_selected();
    bool result = mapper().toggle_active_mutes(group);
    if (result)
    {
        mutegroup::number newgroup = mutes().group_selected();
        send_mutes_events(newgroup, oldgroup);
    }
    return result;
}

/**
 *  Provides a solution to "SM: pattern state isn't recalled with session
 *  (#27).  It actually applies to normal operation as well.
 */

bool
performer::apply_session_mutes ()
{
    bool result = mutes().any() && mutes().group_valid();
    if (rc().song_start_auto())
        result = mapper().trigger_count() == 0; /* triggers imply song mode */
    else
        result = ! rc().song_start_mode();      /* don't apply in song mode */

    if (result)
        result = apply_mutes(mutes().group_selected());

    return result;
}

bool
performer::learn_mutes (mutegroup::number group)
{
    bool result =  mapper().learn_mutes(true, group);   /* true == learn */
    if (result)
    {
        change c = mutes().group_save_to_midi() ?
            change::yes : change:: no ;

        notify_mutes_change(group, c);
    }
    return result;
}

/*
 * -------------------------------------------------------------------------
 *  Automation "slots"
 * -------------------------------------------------------------------------
 */

/*
 *  For the next two functions, compare to performer::set_record_style().
 */

void
performer::next_grid_record_style ()
{
    (void) usr().next_grid_record_style();
    notify_automation_change(automation::slot::record_style);
}

void
performer::previous_grid_record_style ()
{
    (void) usr().previous_grid_record_style();
    notify_automation_change(automation::slot::record_style);
}

void
performer::next_record_mode ()
{
    (void) usr().next_record_mode();
    notify_automation_change(automation::slot::quan_record);
}

void
performer::previous_record_mode ()
{
    (void) usr().previous_record_mode();
    notify_automation_change(automation::slot::quan_record);
}

void
performer::record_mode (recordmode rm)
{
    (void) usr().record_mode(rm);
    notify_automation_change(automation::slot::quan_record);
}

/**
 *  Provides the pattern-control function... hot-keys that toggle the patterns
 *  in the current set.
 *
 * \param a
 *      Provides the action code: toggle, on, or off.  Keystrokes that use this
 *      function will always provide automation::action::toggle.
 *
 * \param d0
 *      Provides the first MIDI data byte's value.  For a keystroke, this value
 *      is currently always 0.
 *
 * \param loopnumber
 *      Provides the second MIDI data byte's value.  For keystrokes, this value
 *      provides the sequence number (an offset in the active set), the group
 *      number (an offset into the mutesgroups), or is ignored, and is set
 *      via the keycontrol constructor.  See seq66 :: keycontainer ::
 *      add_defaults() for an example of this setup.
 *
 * \param inverse
 *      If true, no action is performed, as this means that it is a likely to be
 *      a key release, which would then undo the key press.
 *
 * \return
 *      Returns true if \a loopnumber was valid.
 */

bool
performer::loop_control
(
    automation::action a, int d0, int d1,
    int loopnumber, bool inverse
)
{
    std::string name = "Pattern ";
    name += std::to_string(loopnumber);
    print_parameters(name, a, d0, d1, loopnumber, inverse);

    /*
     * We need to enforce a rule to use the playscreen offset when needed.
     */

    seq::number seqno = mapper().play_seq(loopnumber);
    bool result = seqno >= 0;
    if (result && ! inverse)
    {
        if (slot_shift() > 0)
        {
            if (columns() == setmaster::Columns())
            {
                if (rows() > setmaster::Rows())
                    seqno += slot_shift() * rows();     /* move down x rows */
            }
            else
                seqno += slot_shift() * screenset_size();

            clear_slot_shift();
        }
        m_pending_loop = seqno;
        if (m_seq_edit_pending || m_event_edit_pending)
        {
            result = false;                             /* let caller do it */
        }
        else
        {
            if (usr().no_grid_record())
            {
                gridmode gm = usr().grid_mode();
                if (gm == gridmode::loop)
                {
                    if (a == automation::action::toggle)
                        (void) sequence_playing_toggle(seqno);
                    else if (a == automation::action::on)
                        (void) sequence_playing_change(seqno, true);
                    else if (a == automation::action::off)
                        (void) sequence_playing_change(seqno, false);
                }
                else if (gm == gridmode::copy)
                {
                    result = copy_sequence(seqno);
                }
                else if (gm == gridmode::paste)
                {
                    result = paste_sequence(seqno);
                }
                else if (gm == gridmode::clear)
                {
                    // TODO:  need a clear_sequence() function and an
                    //        entry in qslivebase and the qslivegrid macro
                }
                else if (gm == gridmode::remove)
                {
                    result = remove_sequence(seqno);
                }
                else if (gm == gridmode::thru)
                {
                    result = set_thru(seqno, false, true);  /* true --> toggle  */
                }
                else if (gm == gridmode::solo)
                {
                    result = replace_for_solo(seqno);   /* is this valid?   */
                }
                else if (gm == gridmode::cut)
                {
                    result = cut_sequence(seqno);
                }
                else if (gm == gridmode::double_length)
                {
                    // TODO
                }
            }
            else
            {
                bool rec = false;                   /* !s->recording()  */
                bool toggle = false;
                if (a == automation::action::toggle)
                    toggle = true;
                else if (a == automation::action::on)
                    rec = true;

                if (usr().record_mode() == recordmode::normal)
                    result = set_recording(seqno, rec, toggle);
                else if (usr().record_mode() == recordmode::quantize)
                    result = set_quantized_recording(seqno, rec, toggle);
                else if (usr().record_mode() == recordmode::tighten)
                    result = set_tightened_recording(seqno, rec, toggle);
            }
        }
        if (result)
            notify_sequence_change(seqno, change::no);
    }
    return result;
}

/**
 *  A boolean setter for the setmapper's mode-group value.  If in group-learn
 *  mode, this function will memorize the state of the current (play) screen and
 *  save it in the desired mute group.  Otherise, it will grab the desired
 *  mute-group and apply it to the current play-screen (and mute all other
 *  screens).
 *
 * \param a
 *      Provides the action code: toggle, on, or off.  Keystrokes that use this
 *      function will always provide automation::action::toggle.
 *
 * \param d0
 *      Provides the first MIDI data byte's value.  For a keystroke, this value
 *      is currently always 0.
 *
 * \param groupnumber
 *      Provides the second MIDI data byte's value.  This value provides the
 *      group number (an offset into the mutesgroups).
 *
 * \param inverse
 *      If true, then the mute-group key is being released.  In this case,
 *      nothing is done.
 *
 * \return
 *      Returns true if \a groupnumber was valid.
 */

bool
performer::mute_group_control
(
    automation::action a, int d0, int d1,
    int groupnumber, bool inverse
)
{
    std::string name = is_group_learn() ? "Mute Learn " : "Mutes " ;
    name += std::to_string(d0);
    print_parameters(name, a, d0, d1, groupnumber, inverse);

    mutegroup::number gn = static_cast<mutegroup::number>(groupnumber);
    bool result = gn >= 0;
    if (result && ! inverse)
    {
        if (is_group_learn())
        {
            if (a == automation::action::toggle)
            {
                result = learn_mutes(gn);
            }
            else if (a == automation::action::on)
            {
                result = learn_mutes(gn);
            }
            else if (a == automation::action::off)
            {
                result = learn_mutes(gn);
            }
            group_learn(false);
        }
        else
        {
            /*
             * Treat all mute-group controls the same for now.  We might
             * eventually be able to somehow "toggle" mute groups.
             */

            if (a == automation::action::toggle)
            {
                if (mutes().toggle_active_only())
                    (void) toggle_active_mutes(gn);
                else
                    (void) toggle_mutes(gn);            /* apply_mutes(gn); */
            }
            else if (a == automation::action::on)
            {
                select_and_mute_group(gn);
            }
            else if (a == automation::action::off)
            {
                select_and_mute_group(gn);
            }
        }
    }
    return true;
}

screenset::number
performer::decrement_screenset (int amount)
{
    screenset::number newnumber = playscreen_number() - amount;
    return set_playing_screenset(newnumber);
}

screenset::number
performer::increment_screenset (int amount)
{
    screenset::number newnumber = playscreen_number() + amount;
    return set_playing_screenset(newnumber);
}

/*
 * -------------------------------------------------------------------------
 *  Automation functions
 * -------------------------------------------------------------------------
 */

/**
 *  Implements a no-op function for reserved slots not yet implemented, or it
 *  can can serve as an indication that the caller (e.g. a user-interface)
 *  should handle the functionality itself.
 *
 * \return
 *      Because the slot is not implemented, false is returned.
 */

bool
performer::automation_no_op
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "No-op";
    print_parameters(name, a, d0, d1, index, inverse);
    return false;
}

/**
 *  Implements BPM Up and BPM Down for MIDI control.  There would be no need
 *  for two BPM configuration lines for MIDI control, except that we need two
 *  different keystrokes, one for up, and one for down.
 *
 *  All keystrokes are handled such that the key-press sets inverse to
 *  "false", and the key-release sets inverse to "true".  For most keystrokes,
 *  then, we have to ignore inverse == true.
 *
 *  For the configured BPM Up keystroke, this function is called with an action
 *  of "on", to implement BPM Up.  But a second function, automation_bpm_dn(),
 *  is provided to implement BPM Down for keystrokes.  It can also be
 *  configured for MIDI usage, and it will work like Seq24/Sequencer64, which
 *  just checks for the event irregardless of whether it is toggle, on, or off.
 *
 * \param a
 *      Provides the action code: toggle, on, or off.  Keystrokes that use this
 *      function will always provide automation::action::toggle.
 *
 * \param d0
 *      Unused.
 *
 * \param d1
 *      Unused.
 *
 * \param inverse
 *      Unused.
 *
 * \return
 *      Always returns true.
 */

bool
performer::automation_bpm_up_dn
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "BPM";
    print_parameters(name, a, d0, d1, index, inverse);
    if (inverse)
    {
        if (opcontrol::allowed(d0, inverse))        /* not a key-release    */
        {
            if (a == automation::action::on)
                decrement_beats_per_minute();
            else if (a == automation::action::off)
                increment_beats_per_minute();
        }
    }
    else
    {
        if (a == automation::action::toggle)        /* for key-presses      */
            increment_beats_per_minute();
        else if (a == automation::action::on)
            increment_beats_per_minute();
        else if (a == automation::action::off)
            decrement_beats_per_minute();
    }
    return true;
}

/**
 *  No matter how BPM Down is configured for MIDI control, if present and the
 *  MIDI event matches, it will act like a BPM Down.  This matches the behavior
 *  of Seq24/Sequencer64. Remember that d0 < 0 flags a keystroke, and when
 *  true, we ignore inverse == true (a key-release) via a call to
 *  opcontrol::allowed(d0, inverse).
 */

bool
performer::automation_bpm_dn
(
    automation::action /*a*/, int d0, int d1,
    int index, bool inverse
)
{
    if (opcontrol::allowed(d0, inverse))
    {
        return automation_bpm_up_dn
        (
            automation::action::off, d0, d1, index, inverse
        );
    }
    else
        return true;                    /* pretend the key release worked   */
}

/**
 *  Implements screenset Up and Down.  The default keystrokes are "]" for up
 *  and "[" for down.  Note that all keystrokes generate toggles, and the
 *  release sets "inverse" to true.
 */

bool
performer::automation_ss_up_dn
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Screenset";
    print_parameters(name, a, d0, d1, index, inverse);
    if (inverse)
    {
        if (opcontrol::allowed(d0, inverse))        /* not a key-release    */
        {
            if (a == automation::action::on)
                decrement_screenset();
            else if (a == automation::action::off)
                increment_screenset();
        }
    }
    else
    {
        if (a == automation::action::toggle)        /* for key-presses      */
            increment_screenset();
        else if (a == automation::action::on)
            increment_screenset();
        else if (a == automation::action::off)
            decrement_screenset();
    }
    return true;
}

/**
 *  No matter how Screenset Down is configured for MIDI control, if present and
 *  the MIDI event matches, it will act like a Screenset Down.  This matches the
 *  behavior of Seq24/Sequencer64.
 */

bool
performer::automation_ss_dn
(
    automation::action /*a*/, int d0, int d1,
    int index, bool inverse
)
{
    if (opcontrol::allowed(d0, inverse))
    {
        return automation_ss_up_dn
        (
            automation::action::off, d0, d1, index, inverse
        );
    }
    else
        return true;                    /* pretend the key release worked   */
}

/**
 *  Implements mod_replace.  For MIDI control, there should be no support for
 *  toggle, but we're not sure how to implement this feature.
 *
 *  For keystrokes, the user-interface's key-press callback should set the
 *  inverse flag to false, and the key-release callback should set it to true.
 *  The action will always be toggle for keystrokes.
 */

bool
performer::automation_replace
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Mod Replace";
    print_parameters(name, a, d0, d1, index, inverse);
    if (opcontrol::allowed(d0, inverse))
        return set_ctrl_status(a, automation::ctrlstatus::replace);
    else
        return true;                    /* pretend the key release worked   */
}

/**
 *  Implements mod_snapshot.
 */

bool
performer::automation_snapshot
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Mod Snapshot";
    print_parameters(name, a, d0, d1, index, inverse);
    if (opcontrol::allowed(d0, inverse))
        return set_ctrl_status(a, automation::ctrlstatus::snapshot);
    else
        return true;                    /* pretend the key release worked   */
}

/**
 *  Implements mod_queue.
 */

bool
performer::automation_queue
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Mod Queue";
    print_parameters(name, a, d0, d1, index, inverse);
    if (opcontrol::allowed(d0, inverse))
        return set_ctrl_status(a, automation::ctrlstatus::queue);
    else
        return true;                    /* pretend the key release worked   */
}

/**
 *  Implements mod_gmute.  When set, this sets the group-mute mode that will
 *  take the current screenset and memorize its statuses to the mute-group
 *  specified via the next key that is struck.
 *
 *  Does not quite capture the keyboard group-on key versus group-off key of
 *  Sequencer64.
 */

bool
performer::automation_gmute
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Mod Group Mute";
    print_parameters(name, a, d0, d1, index, inverse);
    if (opcontrol::allowed(d0, inverse))
    {
        if (a == automation::action::toggle)
            mapper().toggle_group_mode();
        else if (a == automation::action::on)
            mapper().group_mode(true);
        else if (a == automation::action::off)
            mapper().group_mode(false);
    }
    return true;
}

/**
 *  Implements mod_glearn. This function is related to automation_gmute().
 *  Like that one, it sets the group-mute mode.  In addition, it sets
 *  group-learn mode, and then notifies all of the subscribers to this event.
 *
 *  Another avenue for this is the learn_toggle() function, which a GUI like
 *  qsmainwnd can call directly, either via the "Learn" button or a keystroke
 *  like Ctrl-L.
 */

bool
performer::automation_glearn
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Mod Group Learn";
    print_parameters(name, a, d0, d1, index, inverse);
    if (opcontrol::allowed(d0, inverse))
    {
        if (a == automation::action::toggle)
            learn_toggle();                     /* also notifies clients    */
        else if (a == automation::action::on)
            group_learn(true);                  /* also notifies clients    */
        else if (a == automation::action::off)
            group_learn(false);                 /* also notifies clients    */
    }
    return true;
}

/**
 *  Implements play_ss.  This function saves the current state of the
 *  screenset, then sets the play-screen to it, then it seems to redundantly
 *  set the states again.  NEEDS FURTHER INVESTIGATION.
 *
 *  See automation_ss_set(), which seems to be a duplicate?
 */

bool
performer::automation_play_ss
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Play Screen-Set";
    print_parameters(name, a, d0, d1, index, inverse);
    if (! inverse)
        (void) set_playing_screenset(screenset::number(d1));

    return true;
}

/**
 *  Implements playback.  That is, start, pause, and stop.  These are MIDI
 *  controls. However, note that we also support separate actions for the
 *  sake of keyboard control:
 *
 *      -   automation_start()
 *      -   automation_stop()
 *
 *  If the action is a toggle (as happens with the "pause" key), then the
 *  toggling is ignored if \a inverse is true.
 *
 * \param a
 *      Provides the action to perform.  Toggle = pause; On = start; and Off =
 *      stop.
 *
 * \return
 *      Returns true if the action was handled.
 */

bool
performer::automation_playback
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Playback";
    print_parameters(name, a, d0, d1, index, inverse);
    if (a == automation::action::toggle)            /* key "." press  */
    {
        if (! inverse)
            auto_pause();
    }
    else if (a == automation::action::on)
    {
        if (inverse)
            auto_stop();
        else
            auto_play();
    }
    else if (a == automation::action::off)
    {
        if (inverse)
            auto_play();
        else
            auto_stop();
    }
    return true;
}

/**
 *  Implements song_record, which sets the status to recording live events
 *  into song triggers.  If \a inverse is true, nothing is done.
 */

bool
performer::automation_song_record
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Song Record";
    print_parameters(name, a, d0, d1, index, inverse);
    if (! inverse)
    {
        if (a == automation::action::toggle)
            song_recording(! song_recording());
        else if (a == automation::action::on)
            song_recording(true);
        else if (a == automation::action::off)
            song_recording(false);
    }
    return true;
}

/**
 *  Implements solo.  This isn't clear even in Sequencer64.  We have
 *  queued-replace and queued-solo which seem to be the same thing.
 *  Replace, though, is not queued, while solo is queued.
 */

bool
performer::automation_solo
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Solo";
    bool result = true;
    print_parameters(name, a, d0, d1, index, inverse);
    if (opcontrol::allowed(d0, inverse))
    {
        automation::ctrlstatus c =
            automation::ctrlstatus::queue | automation::ctrlstatus::replace;

        if (a == automation::action::toggle)
            result = toggle_ctrl_status(c);
        else
            result = set_ctrl_status(a, c);
    }
    return result;
}

/**
 *  Implements thru.  If \a inverse is true, nothing is done.
 */

bool
performer::automation_thru
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Thru";
    print_parameters(name, a, d0, d1, index, inverse);
    if (! inverse)
    {
        seq::number seqno = seq::number(d1);
        if (a == automation::action::toggle)
            set_thru(seqno, false, true);                       /* toggles  */
        else if (a == automation::action::on)
            set_thru(seqno, true, false);                       /* on       */
        else if (a == automation::action::off)
            set_thru(seqno, false, false);                      /* off      */
    }
    return true;
}

/**
 *  Implements BPM Up and BPM Down.  If \a inverse is true, nothing is done.  We
 *  need this behavior to support keystrokes.
 */

bool
performer::automation_bpm_page_up_dn
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "BPM Page";
    print_parameters(name, a, d0, d1, index, inverse);
    if (inverse)
    {
        if (opcontrol::allowed(d0, inverse))            /* not a keystroke  */
        {
            if (a == automation::action::on)
                page_decrement_beats_per_minute();
            else if (a == automation::action::off)
                page_increment_beats_per_minute();
        }
    }
    else
    {
        if (a == automation::action::toggle)            /* for keystroke */
            page_increment_beats_per_minute();
        else if (a == automation::action::on)
            page_increment_beats_per_minute();
        else if (a == automation::action::off)
            page_decrement_beats_per_minute();
    }

    return true;
}

/**
 *  No matter how BPM Down is configured for MIDI control, if present and the
 *  MIDI event matches, it will act like a BPM Down.  This matches the behavior
 *  of Seq24/Sequencer64.
 */

bool
performer::automation_bpm_page_dn
(
    automation::action /*a*/, int d0, int d1,
    int index, bool inverse
)
{
    if (opcontrol::allowed(d0, inverse))
    {
        return automation_bpm_page_up_dn
        (
            automation::action::off, d0, d1, index, inverse
        );
    }
    else
        return true;                    /* pretend the key release worked   */
}

/**
 *  Sets the screen by number.  Needs to be clarified.  If \a inverse is true,
 *  nothing is done, to support keystroke-release properly.
 */

bool
performer::automation_ss_set
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Screen-Set Set";
    print_parameters(name, a, d0, d1, index, inverse);
    if (! inverse)
        (void) set_playing_screenset(screenset::number(d1));

    return true;
}

/**
 *  Implements the recording control.  This function sets the recording status
 *  of incoming MIDI events.  If \a inverse is true, nothing is done. The same
 *  operation occurs for the actions of toggle, on, and off (the latter two are
 *  not supported by keystrokes.
 *
 *  As of 2021-11-13, this function sets the loop-control mode.  Normally, this
 *  is "none", which means that loop-control toggles the mute status of the
 *  specified pattern.  This function increments the mode to the next one,
 *  looping back to none.  See usrsettings :: grid_record_style() and the
 *  usrsettings :: recordstyle enumeration.
 */

bool
performer::automation_record_style
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Record Style";
    print_parameters(name, a, d0, d1, index, inverse);
    if (! inverse)
    {
        if (a == automation::action::toggle)
            next_grid_record_style();
        else if (a == automation::action::on)
            next_grid_record_style();
        else if (a == automation::action::off)
            previous_grid_record_style();

        /*
         *  Done in the functions called above:
         *
         *      notify_automation_change(automation::slot::record_style);
         */
    }
    return true;
}

/**
 *  Like record, but quantized.
 */

bool
performer::automation_quan_record
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Q Record";
    print_parameters(name, a, d0, d1, index, inverse);
    if (! inverse)
    {
        if (a == automation::action::toggle)
            next_record_mode();
        else if (a == automation::action::on)
            next_record_mode();
        else if (a == automation::action::off)
            previous_record_mode();

        notify_automation_change(automation::slot::quan_record);
    }
    return true;
}

/**
 *  We now use it for a call to reset_sequences() and reset_playset().
 */

bool
performer::automation_reset_sets
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Reset Sequences";
    print_parameters(name, a, d0, d1, index, inverse);
    if (! inverse)
    {
        reset_sequences();
        reset_playset();
    }
    return true;
}

/**
 *  Handle one-shot mode, in a manner similar to queue, replace, etc.  The
 *  \a inverse parameter, if true, indicates exiting the mode, as upon
 *  keystroke-release.
 */

bool
performer::automation_oneshot
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "One-shot Queue";
    print_parameters(name, a, d0, d1, index, inverse);
    if (opcontrol::allowed(d0, inverse))
        return set_ctrl_status(a, automation::ctrlstatus::oneshot);
    else
        return true;                    /* pretend the key release worked   */
}

bool
performer::automation_FF
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Fast-forward";
    print_parameters(name, a, d0, d1, index, inverse);

    /*
     * TO BE DETERMINED
     */

    return true;
}

bool
performer::automation_rewind
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Rewind";
    print_parameters(name, a, d0, d1, index, inverse);

    /*
     * TO BE DETERMINED
     */

    return true;
}

/**
 *  Sets the time to the song beginning or the L marker.
 *  Needs work.
 */

bool
performer::automation_top
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Top";
    print_parameters(name, a, d0, d1, index, inverse);

    /*
     * TO BE DETERMINED
     */

    return true;
}

/**
 *  Implements playlist control.  If \a inverse is true, nothing is done.  Note
 *  that currently the GUI may hardwire the usage of the arrow keys for this
 *  functionality.
 */

bool
performer::automation_playlist
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    bool result = false;
    std::string name = "Playlist";
    print_parameters(name, a, d0, d1, index, inverse);
    if (! inverse)
    {
        if (a == automation::action::toggle)            /* select-by-value  */
            result = open_select_list_by_midi(d1);
        else if (a == automation::action::on)           /* select-next      */
        {
            if (signalled_changes())
            {
                notify_song_action(true, playlist::action::next_list);
                result = true;
            }
            else
                result = open_next_list();
        }
        else if (a == automation::action::off)          /* select-previous  */
        {
            if (signalled_changes())
            {
                notify_song_action(true, playlist::action::previous_list);
                result = true;
            }
            else
                result = open_previous_list();
        }
    }
    return result;
}

/*
 * ----------------------------------------------------------------------
 *  More performer::automation_xxx() functions follow this group of
 *  functions.
 * ----------------------------------------------------------------------
 */

/**
 *  This function calls the seq66::read_midi_file() free function, and then
 *  sets the PPQN value.
 *
 * \param fn
 *      Provides the full path file-specification for the MIDI file.
 *
 * \param pp
 *      Provides the destination for the effective PPQN, generally read from the
 *      file.
 *
 * \param [out] errmsg
 *      Provides the destination for an error message, if any.
 *
 * \return
 *      Returns true if the function succeeded.  If false is returned, there
 *      should be an errmsg to display.
 */

bool
performer::read_midi_file
(
    const std::string & fn,
    std::string & errmsg,
    bool addtorecent
)
{
    errmsg.clear();
    usr().clear_global_seq_features();
    bool result = seq66::read_midi_file(*this, fn, ppqn(), errmsg, addtorecent);
    if (result)
    {
        next_song_mode();
    }
    return result;
}

bool
performer::open_note_mapper (const std::string & notefile)
{
    bool result = false;
    m_note_mapper.reset(new notemapper());
    if (m_note_mapper)
    {
        if (notefile.empty())
        {
            // anything to do?
        }
        else
        {
            notemapfile nmf(*m_note_mapper, notefile, rc());
            result = nmf.parse();
            if (! result)
                set_error_message(nmf.get_error_message());
        }
    }
    return result;
}

bool
performer::save_note_mapper (const std::string & notefile)
{
    bool result = bool(m_note_mapper);
    if (result)
    {
        std::string nfname = rc().notemap_filespec();
        if (! notefile.empty())
            nfname = notefile;

        if (nfname.empty())
        {
            result = false;
        }
        else
        {
            notemapfile nmf(*m_note_mapper, nfname, rc());
            result = nmf.write();
            if (! result)
                set_error_message(nmf.get_error_message());
        }
    }
    return result;
}

std::string
performer::playlist_song_basename () const
{
    return filename_base(playlist_song());
}

void
performer::playlist_activate (bool on)
{
    if (on)
    {
        if (m_play_list)
        {
            if (m_play_list->mode())            /* loaded successfully? */
                rc().playlist_active(true);
        }
    }
    else
        rc().playlist_active(false);
}

bool
performer::open_next_list (bool opensong, bool loading)
{
    bool result;
    if (signalled_changes())
    {
        delay_stop();
        result = m_play_list->open_next_list(opensong, loading);
    }
    else
    {
        delay_stop();
        result = m_play_list->open_next_list(opensong, loading);
        if (result)
        {
            if (opensong)
                next_song_mode();

            notify_song_action(false);
        }
    }
    return result;
}

bool
performer::open_previous_list (bool opensong)
{
    bool result;
    if (signalled_changes())
    {
        delay_stop();
        result = m_play_list->open_previous_list(opensong);
    }
    else
    {
        delay_stop();
        result = m_play_list->open_previous_list(opensong);
        if (result)
        {
            if (opensong)
                next_song_mode();

            notify_song_action(false);
        }
    }
    return result;
}

bool
performer::open_select_song_by_index (int index, bool opensong)
{
    bool result;
    if (signalled_changes())
    {
        if (opensong)
            delay_stop();

        result = m_play_list->open_select_song(index, opensong);
    }
    else
    {
        delay_stop();
        result = m_play_list->open_select_song(index, opensong);
        if (result)
        {
            if (opensong)
                next_song_mode();

            notify_song_action(false);
        }
    }
    return result;
}

bool
performer::open_select_song_by_midi (int ctrl, bool opensong)
{
    bool result;
    if (signalled_changes())
    {
        delay_stop();
        result = m_play_list->open_select_song_by_midi(ctrl, opensong);
    }
    else
    {
        delay_stop();
        result = m_play_list->open_select_song_by_midi(ctrl, opensong);
        if (result)
        {
            if (opensong)
                next_song_mode();

            notify_song_action(false);
        }
    }
    return result;
}

bool
performer::open_next_song (bool opensong)
{
    bool result;
    if (signalled_changes())
    {
        delay_stop();
        result = m_play_list->open_next_song(opensong);
    }
    else
    {
        delay_stop();
        result = m_play_list->open_next_song(opensong);
        if (result)
        {
            if (opensong)
                next_song_mode();

            notify_song_action(false);
        }
    }
    return result;
}

bool
performer::open_previous_song (bool opensong)
{
    bool result;
    if (signalled_changes())
    {
        delay_stop();
        result = m_play_list->open_previous_song(opensong);
    }
    else
    {
        delay_stop();
        result = m_play_list->open_previous_song(opensong);
        if (result)
        {
            if (opensong)
                next_song_mode();

            notify_song_action(false);
        }
    }
    return result;
}

bool
performer::open_mutegroups (const std::string & mgf)
{
    bool result = false;
    std::string mgfname = mgf;
    if (mgf.empty())
        mgfname = rc().mute_group_filespec();

    if (mgfname.empty())
    {
        set_error_message("no mute-group filename");
    }
    else
    {
        result = seq66::open_mutegroups(mgfname, mutes());
        if (result)
            mutes().group_save(rc().mute_group_save());
    }
    return result;
}

bool
performer::save_mutegroups (const std::string & mgf)
{
    bool result = false;
    std::string mgfname = mgf;
    if (mgf.empty())
        mgfname = rc().mute_group_filespec();

    if (mgfname.empty())
    {
        // what to do?
    }
    else
    {
        result = seq66::save_mutegroups(mgfname, mutes());
        if (result)
        {
            // Anything to do?
        }
    }
    return result;
}

/**
 *  Imports a play-list from one directory to another.
 *
 *      -#  Provide the full path to the source playlist file.  This path is
 *          normally acquired from a playlist dialog box that starts in
 *          the current home configuration directory; but it can reside
 *          anywhere in the file system.
 *      -#  Copy the playlist file to the session configuration directory:
 *          -#  Normal: home config directory
 *          -#  NSM: session_directory/config
 *      -#  Load the playlist to set its filename and to get its parameters.
 *      -#  Copy the playlist's MIDI files to session MIDI directory:
 *          -#  Normal: home config directory/midi
 *          -#  NSM: session_directory/midi
 *      -#  Adjust the playlist base directory and its internal directory
 *          hierarchy to match the new location of files.
 *      -#  Make playlist active and official in the 'rc' file.
 *          -#  The 'active' flag.
 *          -#  The base 'name' of the file.
 *          -#  The 'base-directory'
 *
 *  What to do about absolute directories?  Leave them be?
 *
 * \param sourcefile
 *      Provides the full path to the playlist file to import.
 *
 * \param cfgpath
 *      The destination where the caller wants to put the playlist. For NSM
 *      session destinations, this path will end with "config".
 *
 * \param midipath
 *      The destination where the caller wants to put the MIDI file. For NSM
 *      session destinations or for normal home destinations, this path will
 *      end with "midi".  Or maybe something more specific?  Should we ignore
 *      absolute paths and leave them be?
 */

bool
performer::import_playlist
(
    const std::string & sourcefile,
    const std::string & cfgpath,
    const std::string & midipath
)
{
    bool result = file_readable(sourcefile);
    if (result)
        result = ! cfgpath.empty() && ! midipath.empty();

    if (result)
    {
        result = make_directory_path(cfgpath);      /* make it exist    */
        if (result)
            result = make_directory_path(midipath);

        if (result)
        {
            std::string sourcebase = filename_base(sourcefile);
            std::string filespec = filename_concatenate(cfgpath, sourcebase);
            result = file_copy(sourcefile, cfgpath);
            if (result)
                result = open_playlist(filespec);

            if (result)
                result = copy_playlist_songs(*m_play_list, filespec, midipath);
        }
    }
    return result;
}

/**
 *  Creates a playlist object and opens it.  If there is a playlist object
 *  already in existence, it is replaced. If there is no playlist file-name,
 *  then an "empty" playlist object is created.
 *
 *  The perform object needs to own the playlist.
 *
 * \param pl
 *      Provides the full path file-specification for the play-list file to be
 *      opened.  If empty, a single playlist with only one play-list and no
 *      songs is created.
 *
 * \param show_on_stdout
 *      If true (the default is false), the playlist is opened to show
 *      song selections on stdout.  This is useful for trouble-shooting or for
 *      making the CLI version of Sequencer64 easier to follow when running.
 *
 * \return
 *      Returns true if the playlist object was able to be created. If the
 *      file-name is not empty, this also means that it was opened, and the
 *      play-list read.  If false is returned, then the previous playlist, if
 *      any, still exists, but is marked as inactive.
 */

bool
performer::open_playlist (const std::string & pl, bool show_on_stdout)
{
    if (m_play_list)
        m_play_list->mode(false);                           /* just in case */

    m_play_list.reset
    (
        new (std::nothrow) playlist(this, pl, show_on_stdout)
    );

    bool result = bool(m_play_list);
    if (result)
    {
        result = seq66::open_playlist(*m_play_list, pl, show_on_stdout);
        if (result)
        {
            clear_all();                    /* reset, not clear, playlist   */
        }
        else
        {
            /*
             * set_error_message(m_play_list->error_message());
             */
        }
    }
    else
    {
        errprint("null playlist pointer");
    }
    return result;
}

/**
 *  Writes the play-list, whether it is active or not, as long as it exists.
 *
 * \param pl
 *      Provides the full path file-specification for the play-list file to be
 *      saved.  If empty, the file-name with which the play-list was created
 *      is used.
 *
 * \return
 *      Returns true if the write operation succeeded.
 */

bool
performer::save_playlist (const std::string & pl)
{
    bool result = bool(m_play_list);
    if (result)
    {
        result = seq66::save_playlist(*m_play_list, pl);
        /*
         * TODO
         *
        if (! result)
            set_error_message(m_play_list->error_message());
         */
    }
    else
    {
        errprint("null playlist pointer");
    }
    return result;
}

/*
 * ----------------------------------------------------------------------
 *  More performer::automation_xxx() functions:
 * ----------------------------------------------------------------------
 */

/**
 *  Implements playlist control.  If \a inverse is true, nothing is done.
 *  Note that currently the GUI may hardwire the usage of the arrow keys for
 *  this functionality.
 */

bool
performer::automation_playlist_song
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    bool result = false;
    std::string name = "Playlist Song";
    print_parameters(name, a, d0, d1, index, inverse);
    if (! inverse)
    {
        if (a == automation::action::toggle)            /* select-by-value  */
            result = open_select_song_by_midi(d1);
        else if (a == automation::action::on)           /* select-next      */
        {
            if (signalled_changes())
            {
                notify_song_action(true, playlist::action::next_song);
                result = true;
            }
            else
                result = open_next_song();
        }
        else if (a == automation::action::off)          /* select-previous  */
        {
            if (signalled_changes())
            {
                notify_song_action(true, playlist::action::previous_song);
                result = true;
            }
            else
                result = open_previous_song();
        }
    }
    return result;
}

/**
 *  Implements setting the BPM by tapping a key.
 */

bool
performer::automation_tap_bpm
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Tap BPM";
    print_parameters(name, a, d0, d1, index, inverse);
    if (! inverse)
    {
        midibpm bpm = update_tap_bpm();
        if (bpm != get_beats_per_minute())
            set_beats_per_minute(bpm);
    }
    return true;
}

/**
 *  Starts playback if not playing, or stops playback, with auto-rewind, if
 *  already playing.  This "one-key" feature is now hard-wired, so that one
 *  does not need to keep remembering to reach for the Esc key.  Also, this is
 *  consistent with the hard-wired role of the Space key in the seqroll and
 *  the perfroll.
 *
 *  The \a inverse parameter, if true, does nothing.  We don't want a
 *  double-clutch on start when a keystroke is released.
 */

bool
performer::automation_start
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Start";
    print_parameters(name, a, d0, d1, index, inverse);
    if (! inverse)
    {
        if (is_pattern_playing())
            auto_stop();
        else
            auto_play();
    }
    return true;
}

/**
 *  Stops playback.  The \a inverse parameter, if true, does nothing.  We don't
 *  want a double-clutch on start when a keystroke is released.
 */

bool
performer::automation_stop
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Stop";
    print_parameters(name, a, d0, d1, index, inverse);
    if (! inverse)
        auto_stop();

    return true;
}

bool
performer::automation_reserved_29
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Reserved 29";
    print_parameters(name, a, d0, d1, index, inverse);
    return false;
}

bool
performer::automation_toggle_mutes
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Toggle Mutes";
    print_parameters(name, a, d0, d1, index, inverse);
    if (a == automation::action::toggle)
    {
        if (! inverse)
            set_song_mute(mutegroups::action::toggle);
    }
    else if (a == automation::action::on)
    {
        if (inverse)
            set_song_mute(mutegroups::action::off);
        else
            set_song_mute(mutegroups::action::on);
    }
    else if (a == automation::action::off)
    {
        if (inverse)
            set_song_mute(mutegroups::action::on);
        else
            set_song_mute(mutegroups::action::off);
    }
    return true;
}

bool
performer::automation_song_pointer
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Song Pointer";
    print_parameters(name, a, d0, d1, index, inverse);

    /*
     * TO BE DETERMINED TODO
     */

    return true;
}

/**
 *  I think we don't want to support inverse for this one.  See the support for
 *  the "Q" button and the set_keep_queue() function.
 */

bool
performer::automation_keep_queue
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Keep queue";
    print_parameters(name, a, d0, d1, index, inverse);
    if (opcontrol::allowed(d0, inverse))
    {
        if (a == automation::action::toggle)
            return toggle_ctrl_status(automation::ctrlstatus::keep_queue);
        else
            return set_ctrl_status(a, automation::ctrlstatus::keep_queue);
    }
    else
        return true;
}

/**
 * \return
 *      Returns false so that the caller can take action on it.
 */

bool
performer::automation_edit_pending
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Seq edit pending";
    print_parameters(name, a, d0, d1, index, inverse);
    if (! inverse)
        m_seq_edit_pending = true;

    return true;
}

/**
 * \return
 *      Returns false so that the caller can take action on it.
 */

bool
performer::automation_event_pending
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Event edit pending";
    print_parameters(name, a, d0, d1, index, inverse);
    if (! inverse)
        m_event_edit_pending = true;

    return true;
}

/**
 * \return
 *      Returns false so that the caller can take action on it, unless the user
 *      has pressed the key more than twice.
 */

bool
performer::automation_slot_shift
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Slot shift ";
    bool result = false;
    name += std::to_string(slot_shift() + 1);
    print_parameters(name, a, d0, d1, index, inverse);
    if (! inverse)
    {
        (void) increment_slot_shift();
        result = true;
    }
    return result;
}

/**
 * \return
 *      Returns false so that the caller can take action on it, unless the user
 *      has pressed the key more than twice.
 */

bool
performer::automation_mutes_clear
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Mutes clear";
    bool result = false;
    name += std::to_string(slot_shift() + 1);
    print_parameters(name, a, d0, d1, index, inverse);
    if (! inverse)
    {
        clear_mutes();
        result = true;
    }
    return result;
}

/**
 *  Signals that the application should exit.
 */

bool
performer::automation_quit
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Exit";
    print_parameters(name, a, d0, d1, index, inverse);
    if (a == automation::action::on && ! inverse)
        signal_quit();

    return true;
}

/**
 *  Toggles the Song/Live mode, but only on a key press, not on a key release.
 *
 * \return
 *      Always returns true.
 */

bool
performer::automation_song_mode
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Song mode toggle";
    print_parameters(name, a, d0, d1, index, inverse);
    if (! inverse)
        (void) toggle_song_start_mode();

    return true;
}

/**
 *  Toggles the JACK transport mode, but only on a key press, not on a key
 *  release.
 *
 * \return
 *      Always returns true.
 */

bool
performer::automation_toggle_jack
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Toggle JACK Transport";
    print_parameters(name, a, d0, d1, index, inverse);
    if (! inverse)
    {
        std::string mode("JACK Transport ");
        toggle_jack_mode();
        mode += get_jack_mode() ? "On" : "Off" ;
        infoprint(mode);
    }
    return true;
}

/**
 *  TODO TODO TODO
 */

bool
performer::automation_menu_mode
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Menu mode toggle TODO";
    bool result = false;
    print_parameters(name, a, d0, d1, index, inverse);
    if (! inverse)
    {
        // TODO
    }
    return result;
}

/**
 *  Toggles the following of JACK Transport upon a "key" press.
 */

bool
performer::automation_follow_transport
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Follow JACK Transport";
    bool result = true;
    print_parameters(name, a, d0, d1, index, inverse);
    if (! inverse)
    {
        std::string mode("Follow JACK Transport ");
        toggle_follow_transport();
        mode += get_follow_transport() ? "On" : "Off" ;
        infoprint(mode);
    }
    return result;
}

bool
performer::automation_panic
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Panic!";
    bool result = true;
    print_parameters(name, a, d0, d1, index, inverse);
    if (! inverse)
        result = panic();

    return result;
}

bool
performer::automation_visibility
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Visibility";
    bool result = true;
    print_parameters(name, a, d0, d1, index, inverse);
    if (! inverse)
        result = visibility(a);

    return result;
}

bool
performer::automation_save_session
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Save Session";
    bool result = true;
    print_parameters(name, a, d0, d1, index, inverse);
    if (a == automation::action::on && ! inverse)
        signal_save();                     /* actually just raises a flag  */

    return result;
}

/**
 *  Values are none, merge, overwrite, expand, and one-shot.
 */

void
performer::set_record_style (recordstyle rs)
{
    if (rs < recordstyle::max)
    {
        usr().grid_record_style(rs);
        notify_automation_change(automation::slot::record_style);
    }
}

bool
performer::automation_record_style_select
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Record Style"; // opcontrol::slot_name(index)
    bool result = true;
    print_parameters(name, a, d0, d1, index, inverse);
    if (a == automation::action::on && ! inverse)
    {
        automation::slot s = int_to_slot_cast(index);
        recordstyle rs;
        switch (s)
        {
            case automation::slot::record_overdub:
                rs = recordstyle::merge;
                break;

            case automation::slot::record_overwrite:
                rs = recordstyle::overwrite;
                break;

            case automation::slot::record_expand:
                rs = recordstyle::expand;
                break;

            case automation::slot::record_oneshot:
                rs = recordstyle::oneshot;
                break;

            default:
                rs = recordstyle::max;
                break;
        }
        set_record_style(rs);
    }
    return result;
}

/**
 *  Values are loop, record, copy, ... double (length).
 */

void
performer::set_grid_mode (gridmode gm)
{
    if (gm < gridmode::max)
    {
        usr().grid_mode(gm);
        if (gm != gridmode::record)
        {
            usr().record_mode(recordmode::normal);
            usr().grid_record_style(recordstyle::merge);
        }
        notify_automation_change(automation::slot::grid_loop);
    }
}

bool
performer::automation_grid_mode
(
    automation::action a, int d0, int d1,
    int index, bool inverse
)
{
    std::string name = "Grid Mode";
    bool result = true;
    print_parameters(name, a, d0, d1, index, inverse);
    if (a == automation::action::on && ! inverse)
    {
        automation::slot s = int_to_slot_cast(index);
        gridmode gm;
        switch (s)
        {
            case automation::slot::grid_loop:
                gm = gridmode::loop;
                break;

            case automation::slot::grid_record:
                gm = gridmode::record;
                break;

            case automation::slot::grid_copy:
                gm = gridmode::copy;
                break;

            case automation::slot::grid_paste:
                gm = gridmode::paste;
                break;

            case automation::slot::grid_clear:
                gm = gridmode::clear;
                break;

            case automation::slot::grid_delete:
                gm = gridmode::remove;
                break;

            case automation::slot::grid_thru:
                gm = gridmode::thru;
                break;

            case automation::slot::grid_solo:
                gm = gridmode::solo;
                break;

            case automation::slot::grid_cut:
                gm = gridmode::cut;
                break;

            case automation::slot::grid_double:
                gm = gridmode::double_length;
                break;

            default:
                gm = gridmode::max;
                break;
        }
        set_grid_mode(gm);
    }
    return result;
}

/**
 *  Provides a list of all the functions that can be configured to be called
 *  upon configured keystrokes or incoming MIDI messages.
 */

performer::automation_pair
performer::sm_auto_func_list [] =
{
    { automation::slot::bpm_up, &performer::automation_bpm_up_dn         },
    { automation::slot::bpm_dn, &performer::automation_bpm_dn            },
    { automation::slot::ss_up, &performer::automation_ss_up_dn           },
    { automation::slot::ss_dn, &performer::automation_ss_dn              },
    { automation::slot::mod_replace, &performer::automation_replace      },
    { automation::slot::mod_snapshot, &performer::automation_snapshot    },
    { automation::slot::mod_queue, &performer::automation_queue          },
    { automation::slot::mod_gmute, &performer::automation_gmute          },
    { automation::slot::mod_glearn, &performer::automation_glearn        },
    { automation::slot::play_ss, &performer::automation_play_ss          },
    { automation::slot::playback, &performer::automation_playback        },
    { automation::slot::song_record, &performer::automation_song_record  },
    { automation::slot::solo, &performer::automation_solo                },
    { automation::slot::thru, &performer::automation_thru                },
    {
        automation::slot::bpm_page_up,
        &performer::automation_bpm_page_up_dn
    },
    { automation::slot::bpm_page_dn, &performer::automation_bpm_page_dn  },
    { automation::slot::ss_set, &performer::automation_ss_set            },
    {
        automation::slot::record_style,
        &performer::automation_record_style
    },
    { automation::slot::quan_record, &performer::automation_quan_record  },
    { automation::slot::reset_sets, &performer::automation_reset_sets    },
    { automation::slot::mod_oneshot, &performer::automation_oneshot      },
    { automation::slot::FF, &performer::automation_FF                    },
    { automation::slot::rewind, &performer::automation_rewind            },
    { automation::slot::top, &performer::automation_top                  },
    { automation::slot::playlist, &performer::automation_playlist        },
    {
        automation::slot::playlist_song,
        &performer::automation_playlist_song
    },
    { automation::slot::tap_bpm, &performer::automation_tap_bpm          },
    { automation::slot::start, &performer::automation_start              },
    { automation::slot::stop, &performer::automation_stop                },
    {
        automation::slot::reserved_29,
        &performer::automation_reserved_29
    },
    {
        automation::slot::toggle_mutes,
        &performer::automation_toggle_mutes
    },
    {
        automation::slot::song_pointer,
        &performer::automation_song_pointer
    },
    { automation::slot::keep_queue, &performer::automation_keep_queue    },
    { automation::slot::slot_shift, &performer::automation_slot_shift    },
    { automation::slot::mutes_clear, &performer::automation_mutes_clear  },
    { automation::slot::quit, &performer::automation_quit                },
    {
        automation::slot::pattern_edit,
        &performer::automation_edit_pending
    },
    { automation::slot::event_edit, &performer::automation_event_pending },
    { automation::slot::song_mode, &performer::automation_song_mode      },
    { automation::slot::toggle_jack, &performer::automation_toggle_jack  },
    { automation::slot::menu_mode, &performer::automation_menu_mode      },
    {
        automation::slot::follow_transport,
        &performer::automation_follow_transport
    },
    { automation::slot::panic,       &performer::automation_panic        },
    { automation::slot::visibility,  &performer::automation_visibility   },
    {
        automation::slot::save_session,
        &performer::automation_save_session
    },
    { automation::slot::reserved_45, &performer::automation_no_op        },
    { automation::slot::reserved_46, &performer::automation_no_op        },
    { automation::slot::reserved_47, &performer::automation_no_op        },
    { automation::slot::reserved_48, &performer::automation_no_op        },

    /*
     * Proposed massive expansion in automation. Grid mode selection.
     */

    {
        automation::slot::record_overdub,
        &performer::automation_record_style_select
    },
    {
        automation::slot::record_overwrite,
        &performer::automation_record_style_select
    },
    {
        automation::slot::record_expand,
        &performer::automation_record_style_select
    },
    {
        automation::slot::record_oneshot,
        &performer::automation_record_style_select
    },
    {
        automation::slot::grid_loop,
        &performer::automation_grid_mode
    },
    {
        automation::slot::grid_record,
        &performer::automation_grid_mode
    },
    {
        automation::slot::grid_copy,
        &performer::automation_grid_mode
    },
    {
        automation::slot::grid_paste,
        &performer::automation_grid_mode
    },
    {
        automation::slot::grid_clear,
        &performer::automation_grid_mode
    },
    {
        automation::slot::grid_delete,
        &performer::automation_grid_mode
    },
    {
        automation::slot::grid_thru,
        &performer::automation_grid_mode
    },
    {
        automation::slot::grid_solo,
        &performer::automation_grid_mode
    },
    {
        automation::slot::grid_cut,
        &performer::automation_grid_mode
    },
    {
        automation::slot::grid_double,
        &performer::automation_grid_mode
    },

    /*
     * Grid quantization type selection.
     */

    { automation::slot::grid_quant_none,    &performer::automation_no_op },
    { automation::slot::grid_quant_full,    &performer::automation_no_op },
    { automation::slot::grid_quant_tighten, &performer::automation_no_op },
    { automation::slot::grid_quant_random,  &performer::automation_no_op },
    { automation::slot::grid_quant_jitter,  &performer::automation_no_op },
    { automation::slot::grid_quant_68,      &performer::automation_no_op },

    /*
     * A few more likely candidates.
     */

    { automation::slot::mod_bbt_hms,        &performer::automation_no_op },
    { automation::slot::mod_LR_loop,        &performer::automation_no_op },
    { automation::slot::mod_undo_recording, &performer::automation_no_op },
    { automation::slot::mod_redo_recording, &performer::automation_no_op },
    { automation::slot::mod_transpose_song, &performer::automation_no_op },
    { automation::slot::mod_copy_set,       &performer::automation_no_op },
    { automation::slot::mod_paste_set,      &performer::automation_no_op },
    { automation::slot::mod_toggle_tracks,  &performer::automation_no_op },

    /*
     * Set playing modes.
     */

    { automation::slot::set_mode_normal,    &performer::automation_no_op },
    { automation::slot::set_mode_auto,      &performer::automation_no_op },
    { automation::slot::set_mode_additive,  &performer::automation_no_op },
    { automation::slot::set_mode_all_sets,  &performer::automation_no_op },

    /*
     * Terminator
     */

    { automation::slot::max, &performer::automation_no_op               }
};

}           // namespace seq66

/*
 * performer.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


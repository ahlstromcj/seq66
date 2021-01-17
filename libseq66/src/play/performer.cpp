/*
 *  This file is part of seq66.
 *
 *  seq66 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  seq66 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq66; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          performer.cpp
 *
 *  This module defines the base class for the performer of MIDI patterns.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom and others
 * \date          2018-11-12
 * \updates       2020-12-29
 * \license       GNU GPLv2 or above
 *
 *  Also read the comments in the Sequencer64 version of this module,
 *  perform.
 *
 *  This class is probably the single most important class in Seq66, as
 *  it supports sequences, playback, JACK, and more.
 *
 *  Here are the slots supported, and what they are suppose to do for
 *  keystrokes versus MIDI controls.  MIDI can support toggle, on, and off
 *  actions.  Keystrokes can only be pressed and release. Each keystroke can
 *  be used for a toggle, which should be triggered on a press event or a
 *  release event, but not both.  A keystroke's press event can be used for an
 *  on, and the release event can be used for an off.  These two modes of
 *  operation depend on the slot(s) involved.
 *
 *  bpm_up:             Key: Toggle = Up.  MIDI: Toggle/On/Off = Up/Up/Down.
 *  bpm_dn:             Passes Off to bpm_up to decrement it.
 *  ss_up:              Key: Toggle = Up.  MIDI: Toggle/On/Off = Up/Up/Down.
 *  ss_dn:              Passes Off to ss_up to decrement it.
 *  mod_replace:        Key: Press-On/Release-Off. MIDI: Toggle = On.
 *  mod_snapshot:       Key: Press-On/Release-Off. MIDI: Toggle = On.
 *  mod_queue:          Key: Press-On/Release-Off. MIDI: Toggle = On.
 *  mod_gmute:          Key: Toggle.  MIDI: Toggle/On/Off.  Group On/Off.
 *  mod_glearn:         Key: Press-On/Release-Off.
 *  play_ss:            Key, MIDI:  All events set the screenset.
 *  playback:    T      Key pause, and MIDI for pause/start/stop.
 *  song_record:        Key, MIDI: Toggle/On/Off song_recording() status.
 *  solo:               TODO, intended to solo track.
 *  thru:               Key: Toggle.  MIDI: Toggle/On/Off.
 *  bpm_page_up:        Key: Toggle = Up.  MIDI: Toggle/On/Off = Up/Up/Down.
 *  bpm_page_dn:        Passes Off to bpm_page_up to decrement it.
 *  ss_set:             Key, MIDI: Set current set as playing screen-set.
 *  record:             Key: Toggle.  MIDI: Toggle/On/Off.
 *  quan_record:        Key: Toggle.  MIDI: Toggle/On/Off.
 *  reset_seq:          Key: Toggle.  MIDI: Toggle/On/Off.
 *  mod_oneshot:        Key: Press-On/Release-Off. MIDI: Toggle = On.
 *  FF:                 TODO.
 *  rewind:             TODO.
 *  top:                TODO.
 *  playlist:           MIDI only, arrow keys hardwired.
 *  playlist_song:      MIDI only, arrow keys hardwired.
 *  tap_bpm,            Tap key for estimating BPM.
 *  start:              TODO.
 *  stop:               TODO.
 *  mod_snapshot_2      TODO.
 *  toggle_mutes        TODO.
 *  song_pointer        TODO.
 *  keep_queue:         Key: Toggle (compare to "queue").
 *  slot_shift:         Each instance of this control add the set size to
 *                      the key's configured slot/pattern value.
 *  mutes_clear:        Set all mute groups to unarmed.
 *  reserved_35:        Reserved for expansion.
 *  pattern_edit:       GUI action, bring up pattern for editing.
 *  event_edit:         GUI action, bring up the event editor.
 *  song_mode:          GUI. Toggle between Song Mode and Live Mode.
 *  toggle_jack:        GUI. Toggle between JACK and ALSA support.
 *  menu_mode:          GUI. Switch menu between enabled/disabled.
 *  follow_transport:   GUI. Toggle between following JACK or not.
 *  panic:              Provides a panic button to stop all notes.
 *  reserved_43:        Reserved for expansion.
 *  reserved_44:        Reserved for expansion.
 *  reserved_45:        Reserved for expansion.
 *  reserved_46:        Reserved for expansion.
 *  reserved_47:        Reserved for expansion.
 *  reserved_48:        Reserved for expansion.
 *  maximum,            Used only for termination/range-checking.
 *  loop:               Key: Toggle-only.  MIDI: Toggle/On/Off.
 *  mute_group:         Key: Toggle-only.  MIDI: Toggle/On/Off.
 *  automation:         See the items above.
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
 *          mute-group. Also called by default in handle_midi_control().
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
 *    Note that 24 is represented by SEQ66_MIDI_CLOCK_IN_PPQN in the
 *    calculations.cpp module.
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
 *      -#  In the destructor, the flags m_inputing and m_outputing are set to
 *          false, and the condition variable is signalled.  This causes the
 *          output thread to exit.  The input thread detects that m_inputing is
 *          false and exits.
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
 *          When everything is in place things work incredibly well. The system
 *          can be running an audio task with no dropouts and a few milliseconds
 *          of latency while the computer is being loaded with disk accesses,
 *          screen refreshes and whatnot. The mouse gets jerky, windows update
 *          very slowly but not a dropout to be heard.
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
 *      First, note that performer will use only the "global" rcsettings object,
 *      as retrieved by the seq66::rc() function.  The same is true for
 *      seq66::usr().
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
 *      modification of the triggers in the Song editor; a change in output buss
 *      (though selection of the input buss is saved in the "rc" file); and
 *      anything else? When they occur, performer :: modify() is called.
 *
 *      One issue with modification is that we don't have comprehensive tracking
 *      of all "undo" operations, so that, once the modify flag is set, only
 *      saving the MIDI tune will unset it.  See the calls to performer ::
 *      unmodify().
 *
 *      Also note that some of the GUI windows have their own, unrelated,
 *      modify() function.
 */

#include <algorithm>                    /* std::find() for std::vector      */
#include <iostream>                     /* std::cout                        */
#include <cmath>                        /* std::round()                     */
#include <cstring>                      /* std::memset()                    */

#include "cfg/cmdlineopts.hpp"          /* cmdlineopts::parse_mute_groups   */
#include "cfg/notemapfile.hpp"          /* seq66::notemapfile               */
#include "cfg/playlistfile.hpp"         /* seq66::playlistfile              */
#include "cfg/settings.hpp"             /* seq66::rcsettings rc(), etc.     */
#include "ctrl/keystroke.hpp"           /* seq66::keystroke class           */
#include "midi/midifile.hpp"            /* seq66::read_midi_file()          */
#include "play/notemapper.hpp"          /* seq66::notemapper                */
#include "play/performer.hpp"           /* seq66::performer, this class     */
#include "os/timing.hpp"                /* seq66::microsleep(), microtime() */
#include "util/strfunctions.hpp"        /* seq66::shorten_file_spec()       */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Need to document this.
 */

static int c_thread_trigger_width_us = SEQ66_DEFAULT_TRIGWIDTH_MS;

/**
 *  This constructor...
 *
 * PPQN and choose_ppqn(p):
 *
 *      -   If p is SEQ66_USE_FILE_PPQN, that is the potential result.
 *      -   If p is SEQ66_USE_DEFAULT_PPQN (-1, the default), then
 *          usr().midi_ppqn() is checked. If SEQ66_USE_FILE_PPQN, then
 *          usr().file_ppqn() is returned.
 */

performer::performer (int ppqn, int rows, int columns) :
    m_error_pending         (false),
    m_play_set              (),
    m_play_list             (),
    m_note_mapper           (new notemapper()),
    m_song_start_mode       (sequence::playback::live),
    m_start_from_perfedit   (false),
    m_reposition            (false),
    m_excell_FF_RW          (1.0),
    m_FF_RW_button_type     (ff_rw::none),
    m_old_seqno             (seq::unassigned()),
    m_current_seqno         (seq::unassigned()),
    m_moving_seq            (),
    m_seq_clipboard         (),
    m_clocks                (),                 /* vector wrapper class     */
    m_inputs                (),                 /* vector wrapper class     */
    m_key_controls          ("Key controls"),
    m_midi_control_in       ("MIDI input controls"),
    m_midi_control_buss     (c_bussbyte_max),   /* any buss can control app */
    m_midi_control_out      (),
    m_mute_groups           ("Mute groups", rows, columns),
    m_operations            ("Performer Operations"),
    m_set_master            (rows, columns),    /* 32 row x column sets     */
    m_set_mapper                                /* accessed via mapper()    */
    (
        m_set_master, m_mute_groups, rows, columns
    ),
    m_queued_replace_slot   (-1),               /* REFACTOR                 */
    m_transpose             (0),
    m_out_thread            (),
    m_in_thread             (),
    m_out_thread_launched   (false),
    m_in_thread_launched    (false),
    m_io_active             (true),             /* must start out true      */
    m_is_running            (false),
    m_is_pattern_playing    (false),
    m_needs_update          (true),
    m_is_busy               (false),            /* try this flag for now    */
    m_looping               (false),
    m_song_recording        (false),
    m_song_record_snap      (false),
    m_resume_note_ons       (usr().resume_note_ons()),
    m_current_tick          (0.0),
    m_ppqn                  (choose_ppqn(ppqn)),
    m_file_ppqn             (0),
    m_bpm                   (SEQ66_DEFAULT_BPM),
    m_current_beats         (0),
    m_base_time_ms          (0),
    m_last_time_ms          (0),
    m_beats_per_bar         (4),
    m_beat_width            (4),
    m_tempo_track_number    (0),
    m_clocks_per_metronome  (24),
    m_32nds_per_quarter     (0),
    m_us_per_quarter_note   (0),
    m_master_bus            (),                 /* this is a shared pointer */
    m_filter_by_channel     (false),
    m_one_measure           (0),
    m_left_tick             (0),
    m_right_tick            (0),
    m_starting_tick         (0),
    m_tick                  (0),
    m_jack_tick             (0),
    m_usemidiclock          (false),            /* MIDI Clock support       */
    m_midiclockrunning      (false),
    m_midiclocktick         (0),
    m_midiclockincrement    (clock_ticks_from_ppqn(m_ppqn)),
    m_midiclockpos          (0),
    m_dont_reset_ticks      (false),            /* support for pausing      */
    m_is_modified           (false),
    m_selected_seqs         (),
    m_condition_var         (),                 /* private access via cv()  */
#if defined SEQ66_JACK_SUPPORT
    m_jack_asst             (*this, SEQ66_DEFAULT_BPM, m_ppqn),
#endif
    m_have_undo             (false),
    m_undo_vect             (),
    m_have_redo             (false),
    m_redo_vect             (),
    m_notify                (),
    m_seq_edit_pending      (false),
    m_event_edit_pending    (false),
    m_pending_loop          (seq::unassigned()),
    m_slot_shift            (0)
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
    m_io_active = m_is_running = false;
    reset_sequences();                      /* stop all output upon exit    */
    announce_exit();
    cv().signal();                          /* signal the end of play       */
    if (m_out_thread_launched && m_out_thread.joinable())
        m_out_thread.join();

    if (m_in_thread_launched && m_in_thread.joinable())
        m_in_thread.join();
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
 *  Removes a class from the notification list.  Used in transitory windows and
 *  frames that need notification.
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
performer::notify_set_change (screenset::number setno, change mod)
{
    for (auto notify : m_notify)
        (void) notify->on_set_change(setno, mod);

    if (mod == change::yes || mod == change::removed)
        modify();
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
    for (auto notify : m_notify)
        (void) notify->on_sequence_change(seqno, redo);

    seq::pointer s = get_sequence(seqno);
    if (mod == change::yes || redo)
        modify();
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

/**
 *  This call is currently done only when adding, deleting, or modifying a
 *  trigger.  NOT TRUE
 */

void
performer::notify_trigger_change (seq::number seqno, change mod)
{
    for (auto notify : m_notify)
        (void) notify->on_trigger_change(seqno);

    if (mod == change::yes)
        modify();
    else if (mod == change::no)
    {
        const seq::pointer s = get_sequence(seqno);
        seqno %= screenset_size();
        announce_sequence(s, seqno);
    }
}

/**
 *  Allows notification of changes in the PPQN and tempo (beats-per-minute,
 *  BPM).
 */

void
performer::notify_resolution_change (int ppqn, midibpm bpm, change mod)
{
    for (auto notify : m_notify)
        (void) notify->on_resolution_change(ppqn, bpm);

    if (mod == change::yes)
        modify();
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
    if (rcs.key_controls().count() > 0)             /* could be 0-sized     */
        m_key_controls = rcs.key_controls();

    if (rcs.midi_control_in().count() > 0)          /* could be 0-sized     */
        m_midi_control_in = rcs.midi_control_in();
    else if (rcs.key_controls().count() > 0)
        m_midi_control_in.add_blank_controls(m_key_controls);

    m_mute_groups = rcs.mute_groups();              /* could be 0-sized     */
    song_mode(rcs.song_start_mode());               /* boolean setter       */
    filter_by_channel(rcs.filter_by_channel());
    tempo_track_number(rcs.tempo_track_number());   /* [midi-meta-events]   */
    m_midi_control_out = rcs.midi_control_out();
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
 *  WHAT ABOUT THE PLAYLIST?
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
    bool pb = song_mode();
    m_master_bus->get_port_statuses(m_clocks, m_inputs);
    rcs.clocks() = m_clocks;
    rcs.inputs() = m_inputs;

#if defined SEQ66_PLATFORM_DEBUG
    m_clocks.show("Clocks");
    m_inputs.show("Inputs");
#endif

    rcs.key_controls() = m_key_controls;
    rcs.midi_control_in() = m_midi_control_in;
    rcs.midi_control_out() = m_midi_control_out;
    rcs.mute_groups() = m_mute_groups;
    rcs.song_start_mode(pb);
    rcs.filter_by_channel(m_filter_by_channel);
    rcs.tempo_track_number(m_tempo_track_number);
    usrs.resume_note_ons(m_resume_note_ons);
    return true;
}

/**
 *  Reloads the mute groups from the "rc" file.
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
    bool result = cmdlineopts::parse_mute_groups(rc(), errmessage);
    if (result)
    {
        result = get_settings(rc(), usr());
    }
    else
    {
        error_message(errmessage);      /* at least show it on the console  */
    }
    return result;
}

bool
performer::ui_get_input (bussbyte bus, bool & active, std::string & n) const
{
    const inputslist & ipm = input_port_map();
    std::string busname;
    bool disabled = false;
    if (ipm.active())
    {
        n = ipm.get_name(bus);
        active = ipm.get(bus);
        disabled = ipm.is_disabled(bus);
    }
    else if (not_nullptr(master_bus()))
    {
        n = master_bus()->get_midi_in_bus_name(bus);
        active = master_bus()->get_input(bus);
    }
    return ! n.empty() && ! disabled;
}

/**
 *  Sets the main input bus, and handles the special "key labels on sequence"
 *  and "sequence numbers on sequence" functionality.  This function is called
 *  by qinputcheckbox :: input_callback_clicked().  Note that the
 *  mastermidibus :: set_input() function passes the setting along to the
 *  input busarray.
 *
 * \param bus
 *      Provides the buss number, less than c_busscount_max (32).
 *
 * \param active
 *      Indicates whether the buss or the user-interface feature is active or
 *      inactive.
 */

bool
performer::ui_set_input (bussbyte bus, bool active)
{
    bool result = m_master_bus->set_input(bus, active);
    if (result)
    {
        inputslist & ipm = input_port_map();
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
    std::string busname;
    if (opm.active())
    {
        n = opm.get_name(bus);
        e = opm.get(bus);
    }
    else if (not_nullptr(master_bus()))
    {
        n = master_bus()->get_midi_out_bus_name(bus);
        e = master_bus()->get_clock(bus);
    }
    return ! n.empty();
}

/**
 *  Sets the clock value, as specified in the Options / MIDI Clocks tab.
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
 *  mainwid or perfnames modules.  This string goes on the bottom-left of
 *  those user-interface elements.
 *
 *  The format of this string is something like the following example,
 *  depending on the "show sequence numbers" option.  The values shown are, in
 *  this order, sequence number (if allowed), buss number, channel number,
 *  beats per bar, and beat width.
 *
\verbatim
        No sequence number:     31-16 4/4
        Sequence number:        9  31-16 4/4
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
performer::sequence_label (const sequence & seq)
{
    std::string result;
    int sn = seq.seq_number();
    if (is_seq_active(sn))
    {
        bussbyte bus = seq.get_midi_bus();
        int chan = seq.is_smf_0() ? 0 : seq.get_midi_channel() + 1;
        int bpb = int(seq.get_beats_per_bar());
        int bw = int(seq.get_beat_width());
        char tmp[32];
        if (rc().show_ui_sequence_number())                  /* new feature! */
            snprintf(tmp, sizeof tmp, "%-3d %d-%d %d/%d", sn, bus, chan, bpb, bw);
        else
            snprintf(tmp, sizeof tmp, "%d-%d %d/%d", bus, chan, bpb, bw);

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
performer::sequence_label (seq::number seqno)
{
    const seq::pointer s = get_sequence(seqno);
    return s ? sequence_label(*s) : std::string("") ;
}

/**
 *  Creates the sequence title, adjusting it for scaling down.  This title is
 *  used in the slots to show the (possibly shortened) pattern title. Note
 *  that the sequence title will also show the sequence length, in measures,
 *  if the rc().show_ui_sequence_key() option is active.
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
performer::sequence_title (const sequence & seq)
{
    std::string result;
    int sn = seq.seq_number();
    if (is_seq_active(sn))
    {
        char temp[16];
        const char * fmt =  usr().window_scaled_down() ? "%.11s" : "%.14s" ;
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
performer::sequence_window_title (const sequence & seq)
{
    std::string result = seq_app_name();
    int sn = seq.seq_number();
    if (is_seq_active(sn))
    {
        int ppqn = seq.get_ppqn();                    /* choose_ppqn(m_ppqn);    */
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
performer::main_window_title (const std::string & file_name)
{
    std::string result = seq_app_name() + std::string(" - ");
    std::string itemname = "unnamed";
    int ppqn = choose_ppqn(m_ppqn);
    char temp[32];
    snprintf(temp, sizeof temp, " (%d ppqn) ", ppqn);
    if (file_name.empty())
    {
        if (! rc().midi_filename().empty())
        {
            std::string name = shorten_file_spec(rc().midi_filename(), 56);
#if defined USE_UTF8_CONVERSION
            itemname = Glib::filename_to_utf8(name);
#else
            itemname = name;
#endif
        }
    }
    else
    {
        itemname = file_name;
    }
    result += itemname + std::string(temp);
    return result;
}

/*
 * -------------------------------------------------------------------------
 *  Sequence Creation/Installation
 * -------------------------------------------------------------------------
 */

/**
 *  A private helper function for add_sequence() and new_sequence().  It is
 *  common code and using it prevents inconsistences.  It assumes values have
 *  already been checked.  It does not set the "is modified" flag, since
 *  adding a sequence by loading a MIDI file should not set it.  Compare
 *  new_sequence(), used by mainwid and seqmenu, with add_sequence(), used by
 *  midifile.  This function deletes the sequence already present with the given
 *  sequence number, and adds the current sequence.
 *
 * \param seq
 *      The pointer to the pattern/sequence to add.
 *
 * \param seqno
 *      The sequence number of the pattern to be added.  Not validated, to
 *      save some time.
 *
 * \param fileload
 *      If true (the default is false), the modify flag will not be set.
 *
 * \return
 *      Returns true if the sequence was
 *      successfully added.
 */

bool
performer::install_sequence (sequence * s, seq::number seqno, bool fileload)
{
    bool result = mapper().install_sequence(s, seqno);
    if (result)
    {
        /*
         * Add the buss override, if specified.  We can't set it until after
         * assigning the master MIDI buss, otherwise we get a segfault.
         */

        midipulse barlength = s->get_ppqn() * s->get_beats_per_bar();
        bussbyte buss_override = usr().midi_buss_override();
        s->set_parent(this);                /* also sets true buss value    */
        s->set_master_midi_bus(m_master_bus.get());
        s->sort_events();                   /* sort the events now          */
        s->set_length();                    /* final verify_and_link()      */
        s->empty_coloring();                /* yellow color if no events    */
        if (s->get_length() < barlength)    /* pad sequence to a measure    */
            s->set_length(barlength, false);

        if (buss_override != c_bussbyte_max)
            s->set_midi_bus(buss_override);

        if (! fileload)
            modify();

        mapper().fill_play_set(m_play_set);     /* clear it and refill it   */
    }
    return result;
}

/**
 *  Creates a new pattern/sequence for the given slot, and sets the new
 *  pattern's master MIDI bus address.  Then it activates the pattern [this is
 *  done in the install_sequence() function].  It doesn't deal with thrown
 *  exceptions.
 *
 *  This function is called by the seqmenu and mainwid objects to create a new
 *  sequence.  We now pass this sequence to install_sequence() to better
 *  handle potential memory leakage, and to make sure the sequence gets
 *  counted.  Also, adding a new sequence from the user-interface is a
 *  significant modification, so the "is modified" flag gets set.
 *
 *  If enabled, wire in the MIDI buss override.
 *
 * \param seq
 *      The prospective sequence number of the new sequence.  If not set to
 *      seq::unassigned() (-1), then the sequence is also installed.
 *
 * \return
 *      Returns true if the sequence is valid.  Do not use the
 *      sequence if false is returned, it will be null.
 */

bool
performer::new_sequence (seq::number seq)
{
    sequence * seqptr = new (std::nothrow) sequence(ppqn());
    bool result = not_nullptr(seqptr);
    if (result && seq != seq::unassigned())
        result = install_sequence(seqptr, seq);

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
        modify();

    midi_control_out().send_seq_event(seqno, midicontrolout::seqaction::remove);
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
            m_seq_clipboard.partial_assign(*s);
    }
    return result;
}

bool
performer::cut_sequence (seq::number seq)
{
    bool result = is_seq_active(seq) && ! is_seq_in_edit(seq);
    if (result)
    {
        seq::pointer s = get_sequence(seq);
        result = bool(s);
        if (result)
        {
            m_seq_clipboard.partial_assign(*s);
            result = remove_sequence(seq);
        }
    }
    return result;
}

bool
performer::paste_sequence (seq::number seq)
{
    bool result = ! is_seq_active(seq);
    if (result)
    {
        if (new_sequence(seq))
        {
            get_sequence(seq)->partial_assign(m_seq_clipboard);
            get_sequence(seq)->set_dirty();
        }
    }
    return result;
}

/**
 *  Takes the given sequence number, makes sure the sequence is active, copies
 *  it to m_moving_seq via a partial-assign, and then removes it.
 */

bool
performer::move_sequence (seq::number seq)
{
    bool result = is_seq_active(seq);
    if (result)
    {
        seq::pointer s = get_sequence(seq);
        m_old_seqno = seq;
        m_moving_seq.partial_assign(*s);
        result = remove_sequence(seq);
    }
    return result;
}

bool
performer::finish_move (seq::number seq)
{
    bool result = false;
    if (! is_seq_active(seq))             /* is_mseq_available(seq)   */
    {
        if (new_sequence(seq))
        {
            get_sequence(seq)->partial_assign(m_moving_seq);
            result = true;
        }
    }
    else
    {
        if (new_sequence(m_old_seqno))
        {
            get_sequence(m_old_seqno)->partial_assign(m_moving_seq);
            result = true;
        }
    }
    return result;
}

/*
 * -------------------------------------------------------------------------
 *  NEXT
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
        m_ppqn = p;
        if (m_master_bus)
            m_master_bus->set_ppqn(p);
        else
            (void) error_message("performer::set_ppqn(): master bus is null");

#if defined SEQ66_JACK_SUPPORT
        m_jack_asst.set_ppqn(p);
#endif

        m_one_measure = 0;
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
 *  triggers.
 *
 *  Currently operates only on the current screenset.  We will fix this using a
 *  lamdba function.
 *
 *  Note the it also, via notify_resolution_change(), sets the modify flag.
 */

bool
performer::change_ppqn (int p)
{
    bool result = set_ppqn(p);                  /* performer & master bus   */
    if (result)
    {
        mapper().set_function
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
            notify_resolution_change(get_ppqn(), get_beats_per_minute());
    }
    return result;
}

/**
 *  Goes through all the sequences in the current set, updating the buss to
 *  the same (global) buss number.
 *
 *  Currently operates only on the current screenset.
 *
 * \param buss
 */

bool
performer::ui_change_set_bus (int buss)
{
    bussbyte b = bussbyte(buss);
    bool result = is_good_bussbyte(b);
    if (result)
    {
        for (auto seqi : m_play_set.seq_container())
            seqi->set_midi_bus(b, true);    /* calls notification funcion   */

        screenset::number setno = mapper().playscreen_number();
        notify_set_change(setno, change::yes);
    }
    return result;
}

/**
 *  Locks on m_condition_var [accessed by function cv()].  Then, if not
 *  is_running(), the playback mode is set to the given state.  If that state
 *  is true, call off_sequences().  Set the running status, unlock, and signal
 *  the condition.
 *
 *  Note that we reverse unlocking/signalling from what Seq64 does (BUG!!!)
 *  Manual unlocking should be done before notifying, to avoid waking waking up
 *  the waiting thread, only to lock again.  See the notify_one() notes for
 *  details.
 *
 *  This function should be considered the "second thread", that is the thread
 *  that starts after the worker thread is already working.
 *
 * Minor issue:
 *
 *      In ALSA mode, restarting the sequence moves the progress bar to the
 *      beginning of the sequence, even if just pausing.  This is fixed by
 *      compiling with pause support, which disables calling off_sequences()
 *      when starting playback from the song editor / performance window.
 *
 * \param songmode
 *      Sets the playback mode, and, if true, turns off all of the sequences
 *      before setting the is-running condition.
 */

void
performer::inner_start (bool songmode)
{
    if (! is_running())
    {
        song_mode(songmode);            /* playback_mode(p)             */
        if (songmode)
            off_sequences();

        is_running(true);

#if ! defined SEQ66_PLATFORM_WINDOWS
        automutex lk(cv().locker());    /* use the condition's recmutex */
#endif
        cv().signal();
        send_play_states(midicontrolout::uiaction::play);
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
    start_from_perfedit(false);
    m_is_running = false;
    reset_sequences();                  /* resets, and flushes the buss     */
    m_usemidiclock = midiclock;
    send_play_states(midicontrolout::uiaction::stop);
}

/**
 *  Copies the given string into the desired set's name.
 *
 * \param sset
 *      The ID number of the screen-set, an index into the setmapper.
 *
 * \param notepad
 *      Provides the string date to copy into the notepad.
 *
 * \param is_load_modification
 *      If true (the default is false), we do not want to set the modify flag,
 *      otherwise the user is prompted to save even if no changes have
 *      occurred.
 */

void
performer::set_screenset_notepad
(
    screenset::number sn,
    const std::string & notepad,
    bool is_load_modification
)
{
    bool changed = mapper().name(sn, notepad);
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
 * \param bpm
 *      Provides the beats/minute value to be set.  It is clamped, if
 *      necessary, between the values SEQ66_MINIMUM_BPM to SEQ66_MAXIMUM_BPM.
 *      They provide a wide range of speeds, well beyond what normal music
 *      needs.
 */

void
performer::set_beats_per_minute (midibpm bpm)
{
    if (bpm < SEQ66_MINIMUM_BPM)
        bpm = SEQ66_MINIMUM_BPM;
    else if (bpm > SEQ66_MAXIMUM_BPM)
        bpm = SEQ66_MAXIMUM_BPM;
    else
        bpm = fix_tempo(bpm);

    if (bpm != m_bpm)
    {

#if defined SEQ66_JACK_SUPPORT

        /*
         * This logic matches the original seq24, but is it really correct?
         * Well, we fixed it so that, whether JACK transport is in force or
         * not, we can modify the BPM and have it stick.  No test for JACK
         * Master or for JACK and normal running status needed.
         */

        m_jack_asst.set_beats_per_minute(bpm);

#endif

        if (m_master_bus)
            m_master_bus->set_beats_per_minute(bpm);

        m_us_per_quarter_note = tempo_us_from_bpm(bpm);
        m_bpm = bpm;

        /*
         * Do we need to adjust the BPM of all of the sequences, including the
         * potential tempo track???  It is "merely" the putative main tempo of
         * the MIDI tune.  Actually, this value can now be recorded as a Set
         * Tempo event by user action in the main window (and, later, by
         * incoming MIDI Set Tempo events).
         */
    }
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

midibpm
performer::update_tap_bpm ()
{
    midibpm bpm = 0.0;
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    long ms = long(spec.tv_sec) * 1000;         /* seconds to milliseconds  */
    ms += std::round(spec.tv_nsec * 1.0e-6);    /* nanosecs to milliseconds */
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

#if defined SEQ66_PLATFORM_DEBUG_TMI
        printf("BPM(%d) = %g\n", m_current_beats, bpm);
#endif

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
        struct timespec spec;
        clock_gettime(CLOCK_REALTIME, &spec);
        long ms = long(spec.tv_sec) * 1000;         /* seconds to ms        */
        ms += round(spec.tv_nsec * 1.0e-6);         /* nanoseconds to ms    */
        long difference = ms - m_last_time_ms;
        if (difference > usr().tap_bpm_timeout())
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
    seq::pointer s = get_sequence(tempo_track_number());
    bool result = bool(s);
    if (result)
    {
        midipulse tick = get_tick();
        midibpm bpm = get_beats_per_minute();
        seq66::event e = create_tempo_event(tick, bpm);     /* event.cpp    */
        if (s->add_event(e))                                /* sorts too    */
        {
            s->link_tempos();
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
 *  subscribers.
 */

screenset::number
performer::set_playing_screenset (screenset::number setno)
{
    screenset::number current = mapper().playscreen_number();
    bool ok = setno != current;
    if (ok)
        ok = mapper().set_playing_screenset(setno);

    if (ok)
    {
        bool clearitfirst = rc().is_setsmode_to_clear();
        announce_exit(false);                       /* blank the device     */
        announce_playscreen();                      /* inform control-out   */
        unset_queued_replace();
        mapper().fill_play_set(m_play_set, clearitfirst);
        if (rc().is_setsmode_autoarm())
        {
            set_song_mute(mutegroups::muting::off);
        }
        notify_set_change(setno, change::no);
    }
    return mapper().playscreen_number();
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
performer::clear_all (bool clearplaylist)
{
    bool result = clear_song();
    if (result)
    {
        if (m_play_list)
        {
            m_is_busy = true;
            (void) m_play_list->reset_list(clearplaylist);
        }
        m_is_busy = false;
        set_needs_update();             /* tell all GUIs to refresh. BUG!   */
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
        unmodify();                     /* new, we start afresh             */
        m_is_busy = false;
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
 * \param pause
 *      Try to prevent notes from lingering on pause if true.  By default, it
 *      is false.
 */

void
performer::reset_sequences (bool pause)
{
    void (sequence::* f) (bool) = pause ? &sequence::pause : &sequence::stop ;
    bool songmode = song_mode();
    for (auto & seqi : m_play_set.seq_container())
        (seqi.get()->*f)(songmode);

    if (m_master_bus)
        m_master_bus->flush();                          /* flush MIDI buss  */
}

bool
performer::repitch_selected (const std::string & nmapfile, sequence & s)
{
    bool result = open_note_mapper(nmapfile);
    if (result)
        result = s.repitch_selected(*m_note_mapper);

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
performer::set_song_mute (mutegroups::muting op)
{
    switch (op)
    {
    case mutegroups::muting::on:

        mute_all_tracks(true);
        break;

    case mutegroups::muting::off:

        mute_all_tracks(false);
        break;

    case mutegroups::muting::toggle:

        toggle_all_tracks();
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

        m_master_bus.reset(new(std::nothrow) mastermidibus(m_ppqn, m_bpm));
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
    bool result = create_master_bus();  /* also calls set_port_statuses()   */
    if (result)
    {
        (void) init_jack_transport();
        m_master_bus->init(ppqn, m_bpm);    /* calls api_init() per API     */

        bool ok = activate();
        if (ok)
        {
            launch_input_thread();
            launch_output_thread();
            (void) set_playing_screenset(0);    // ca 2020-08-11
        }
        else
            m_error_pending = true;

        /*
         * Get and store the clocks and inputs created (disabled or not) by
         * the mastermidibus during api_init().  After this call, the clocks
         * and inputs now have name.
         */

        m_master_bus->copy_io_busses();
        m_master_bus->get_port_statuses(m_clocks, m_inputs);
        if (ok)
            announce_playscreen();
    }
    return result;
}

/**
 *  Announces the current mute states of the now-current play-screen.  This
 *  function is handled by creating a slothandler that calls the
 *  announce_sequence() function.
 *
 *  This version works only for the first screen-set!  The slot-handler
 *  increments the seq-number beyond the size of a set automatically.
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
        slot_function(sh, false);           /* do not use the set-offset    */
        m_master_bus->flush();
    }
}

/**
 *  This action is similar to announce_playscreen(), but it unconditionally
 *  turns off (removes) all of the sequences.
 */

void
performer::announce_exit (bool playstatesoff)
{
    if (midi_control_out().is_enabled())
    {
        int setsize = midi_control_out().screenset_size();
        for (int i = 0; i < setsize; ++i)
        {
            midi_control_out().send_seq_event
            (
                i, midicontrolout::seqaction::remove
            );
        }
        if (playstatesoff)
            send_play_states(midicontrolout::uiaction::max);
    }
}

/**
 *  Provides a screenset::slothandler function to announce the current status
 *  of a sequence to an external device via the midicontrolout container.
 *  This function has to have both the sequence and its number as parameters,
 *  and must return a boolean value.
 *
 *  Also note that slot_function() must be called with the use_set_offset
 *  parameter set to false, in order to keep the slot number below the
 *  set-size, otherwise a crash occurs.
 *
 * \param s
 *      Provides the pointer to the sequence.
 *
 * \param sn
 *      Provides the slot number to be used for display, and should range from
 *      0 to the set-size.
 *
 * \return
 *      Returns true if the sequence pointer exists.
 */

bool
performer::announce_sequence (seq::pointer s, seq::number sn)
{
    bool result = not_nullptr(s);
    if (result)
    {
        if (s->playing())
        {
            midi_control_out().send_seq_event
            (
                sn, midicontrolout::seqaction::arm
            );
        }
        else
        {
            midi_control_out().send_seq_event
            (
                sn, midicontrolout::seqaction::mute
            );
        }
    }
    else
    {
        midi_control_out().send_seq_event
        (
            sn, midicontrolout::seqaction::remove
        );
    }
    return result;
}

/**
 *  Sets the beats per measure.
 *
 *  Note that a lambda function is used to make the changes.
 */

bool
performer::set_beats_per_measure (int bpm)
{
    bool result = bpm != m_beats_per_bar;
    if (result)
    {
        set_beats_per_bar(bpm);
        mapper().set_function
        (
            [bpm] (seq::pointer sp, seq::number /*sn*/)
            {
                bool result = bool(sp);
                if (result)
                {
                    sp->set_beats_per_bar(bpm);
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
    m_out_thread = std::thread(&performer::output_func, this);
    m_out_thread_launched = true;
    if (rc().priority())                        /* Not in MinGW RCB     */
    {
        struct sched_param schp;
        memset(&schp, 0, sizeof(sched_param));
        schp.sched_priority = 1;                /* Linux range: 1 to 99 */
#if defined SEQ66_PLATFORM_PTHREADS
        int rc = pthread_setschedparam
        (
            m_out_thread.native_handle(), SCHED_FIFO, &schp
        );
#else
        int rc = sched_setscheduler
        (
            m_out_thread.native_handle(), SCHED_FIFO, &schp
        );
#endif
        if (rc != 0)
        {
            errprint
            (
                "output_thread: couldn't set scheduler to FIFO, "
                "need root priviledges."
            );
            pthread_exit(0);
        }
        else
        {
            infoprint("Output priority set to 1");
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
    m_in_thread = std::thread(&performer::input_func, this);
    m_in_thread_launched = true;
    if (rc().priority())                        /* Not in MinGW RCB     */
    {
        struct sched_param schp;
        memset(&schp, 0, sizeof(sched_param));
        schp.sched_priority = 1;                /* Linux range: 1 to 99 */
#if defined SEQ66_PLATFORM_PTHREADS
        int rc = pthread_setschedparam
        (
            m_in_thread.native_handle(), SCHED_FIFO, &schp
        );
#else
        int rc = sched_setscheduler
        (
            m_in_thread.native_handle(), SCHED_FIFO, &schp
        );
#endif
        if (rc != 0)
        {
            errprint
            (
                "output_thread: couldn't set scheduler to FIFO, "
                "need root priviledges."
            );
            pthread_exit(0);
        }
        else
        {
            infoprint("Input priority set to 1");
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
 */

bool
performer::finish ()
{
    bool ok = deinit_jack_transport();
    bool result = bool(m_master_bus);
    if (result)
        m_master_bus->get_port_statuses(m_clocks, m_inputs);

    return ok && result;
}

/**
 *  Performs a controlled activation of the jack_assistant and other JACK
 *  modules. Currently does work only for JACK; the activate() calls for other
 *  APIs just return true without doing anything.
 */

bool
performer::activate ()
{
    bool result = m_master_bus && m_master_bus->activate();

#if defined SEQ66_JACK_SUPPORT
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

/**
 *  This version for song-recording not only logs m_tick, it also does JACK
 *  positioning (if applicable), calls the master bus's continue_from()
 *  function, and sets m_current_tick as well.
 *
 *  For debugging: Display the tick values; normally this is too much
 *  information.
 *
 * \todo
 *      Do we really need m_current_tick???
 */

void
performer::set_tick (midipulse tick)
{
#if defined SEQ66_PLATFORM_DEBUG_TMI
    static midipulse s_last_tick = 0;
    midipulse difference = tick - s_last_tick;
    if (difference > 100)
    {
        s_last_tick = tick;
        infoprintf("perform tick = %ld", m_tick);
        fflush(stdout);
    }
    if (tick == 0)
        s_last_tick = 0;
#endif

    m_tick = m_current_tick = tick;
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
 *
 * \param setstart
 *      If true (the default, and long-standing implicit setting), then the
 *      starting tick is also set to the left tick.
 */

void
performer::set_left_tick (midipulse tick, bool setstart)
{
    m_left_tick = tick;
    if (setstart)
        set_start_tick(tick);

#if defined SEQ66_JACK_SUPPORT
    if (is_jack_master())                       /* don't use in slave mode  */
        position_jack(true, tick);
    else if (! is_jack_running())
        set_tick(tick);
#else
    set_tick(tick);
#endif

    m_reposition = false;
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
 *
 * \param setstart
 *      If true (the default, and long-standing implicit setting), then the
 *      starting tick is also set to the left tick, if that got changed.
 */

void
performer::set_right_tick (midipulse tick, bool setstart)
{
    if (tick == 0)
        tick = m_one_measure;

    if (tick >= m_one_measure)
    {
        m_right_tick = tick;
        if (m_right_tick <= m_left_tick)
        {
            m_left_tick = m_right_tick - m_one_measure;
            if (setstart)
                set_start_tick(m_left_tick);

            /*
             * Do this no matter the value of setstart, to match stazed's
             * implementation.
             */

            if (is_jack_master())
                position_jack(true, m_left_tick);
            else
                set_tick(m_left_tick);

            m_reposition = false;
        }
    }
}

/**
 *  Also modify()'s.
 */

bool
performer::set_sequence_name (seq::pointer s, const std::string & name)
{
    bool result = bool(s) && (name != s->name());
    if (result)
    {
        seq::number seqno = s->seq_number();
        s->set_name(name);
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
performer::set_recording (seq::pointer s, bool recordon, bool toggle)
{
    bool result = bool(s);
    if (result)
        result = s->set_recording(recordon, toggle);

    return result;
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
    seq::pointer s = get_sequence(seqno);
    return set_recording(s, recordon, toggle);
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
performer::set_quantized_recording (seq::pointer s, bool recordon, bool toggle)
{
    bool result = bool(s);
    if (result)
        result = s->set_quantized_recording(recordon, toggle);

    return result;
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
    seq::pointer s = get_sequence(seqno);
    return set_quantized_recording(s, recordon, toggle);
}

/**
 *  Set recording for overwrite.  This feature was obtained from jfrey-xx on
 *  GitHub.
 *
 *  Pull request #150: Ask for a reset explicitly upon toggle-on, since we don't
 *  have the GUI to control for progress.  This is implemented in sequence's
 *  version of this function.
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
performer::set_overwrite_recording (seq::pointer s, bool oactive, bool toggle)
{
    bool result = bool(s);
    if (result)
        result = s->set_overwrite_recording(oactive, toggle);

    return result;
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
performer::set_overwrite_recording (seq::number seqno, bool oactive, bool toggle)
{
    seq::pointer s = get_sequence(seqno);
    return set_overwrite_recording(s, oactive, toggle);
}

/**
 *  Encapsulates code used by seqedit::thru_change_callback().
 *
 * \param recordon
 *      Provides the current status of the Record button.
 *
 * \param thruon
 *      Provides the current status of the Thru button.
 *
 * \param s
 *      The sequence that the seqedit window represents.  This pointer is
 *      checked.
 */

bool
performer::set_thru (seq::pointer s, bool thruon, bool toggle)
{
    bool result = bool(s);
    if (result)
        result = s->set_thru(thruon, toggle);

    return result;
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
    seq::pointer s = get_sequence(seqno);
    return set_thru(s, thruon, toggle);
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
 * \param jack_button_active
 *      Indicates if the perfedit JACK button shows it is active.
 *
 * \return
 *      Returns true if JACK is running currently, and false otherwise.
 */

bool
performer::set_jack_mode (bool jack_button_active)
{
    if (! is_running())                         /* was global_is_running    */
    {
        if (jack_button_active)
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
 *  Initializes JACK support, if SEQ66_JACK_SUPPORT is defined.  Who calls
 *  this routine?  The main() routine of the application [via launch()],
 *  and the options module, when the Connect button is pressed.
 *
 * \return
 *      Returns the result of the init() call; true if JACK sync is now
 *      running.  If JACK support is not built into the application, then
 *      this function returns false, to indicate that JACK is (definitely)
 *      not running.
 */

bool
performer::init_jack_transport ()
{
#if defined SEQ66_JACK_SUPPORT
    return m_jack_asst.init();
#else
    return false;
#endif
}

/**
 *  Tears down the JACK infrastructure.  Called by launch() and in the
 *  options module, when the Disconnect button is pressed.  This function
 *  operates only while Sequencer64 is not outputing, otherwise we have a
 *  race condition that can lead to a crash.
 *
 * \return
 *      Returns the result of the init() call; false if JACK sync is now
 *      no longer running.  If JACK support is not built into the
 *      application, then this function returns true, to indicate that
 *      JACK is (definitely) not running.
 */

bool
performer::deinit_jack_transport ()
{
#if defined SEQ66_JACK_SUPPORT
    return m_jack_asst.deinit();
#else
    return true;
#endif
}

/**
 *  If JACK is supported and running, sets the position of the transport.
 *
 * \param songmode
 *      If true, playback is to be in Song mode.  Otherwise, it is to be in
 *      Live mode.
 *
 * \param tick
 *      Provides the pulse position to be set.  The default value is 0.
 */

#if defined SEQ66_JACK_SUPPORT
void
performer::position_jack (bool songmode, midipulse tick)
{
    m_jack_asst.position(songmode, tick);
}
#else
void
performer::position_jack (bool /*songmode*/, midipulse /*tick*/)
{
    // No code
}
#endif

/**
 *  Set up the performance and start the thread.  We rely on C++11's thread
 *  handling to set up the thread properly on Linux and Windows.
 *  Here's how it works:
 *
 *      -   It runs while m_io_active is true, which is set in the constructor,
 *          stays that way basically for the duration of the application.
 *
 *  We do not use std::unique_lock<std::mutex>, because we want a recursive
 *  mutex.
 *
 * \warning
 *      Valgrind shows that output_func() is being called before the JACK
 *      client pointer is being initialized!!!
 *
 *  See the old global output_thread_func() in Sequencer64.
 *  This locking is similar to that of inner_start(), except that
 *  signalling (notification) is not done here.
 *
 *  This function should be considered the "worker thread".
 *
 *  While running, we:
 *
 *      -#  Before the "is-running" loop:  If in the performance view (song
 *          editor), we care about starting from the m_starting_tick offset.
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
 */

void
performer::output_func ()
{
    while (m_io_active)                     /* should we LOCK this variable */
    {
        SEQ66_SCOPE_LOCK                    /* only a marker macro          */
        {
            automutex lk(cv().locker());    /* deadlock?                    */
            while (! is_running())
            {
                cv().wait();
                if (done())                 /* if stopping, kill the thread */
                    break;
            }
        }

        long last;                          /* beginning time               */
        long current;                       /* current time                 */
        long delta;                         /* current - last               */

#if defined SEQ66_PLATFORM_DEBUG && defined SEQ66_PLATFORM_LINUX
        if (rc().verbose())
            infoprintf("output_func() running on CPU #%d", sched_getcpu());
#endif

        jack_scratchpad pad;
        pad.js_total_tick = 0.0;            /* double... long probably ...  */
        pad.js_clock_tick = 0;              /* ... offers more ticks        */
        if (m_dont_reset_ticks)
        {
            pad.js_current_tick = get_jack_tick();
        }
        else
        {
            pad.js_current_tick = 0.0;      /* tick and tick fraction       */
            pad.js_total_tick = 0.0;
            m_current_tick = 0.0;
        }

        pad.js_jack_stopped = false;
        pad.js_dumping = false;
        pad.js_init_clock = true;
        pad.js_looping = m_looping;
        pad.js_playback_mode = song_mode();
        pad.js_ticks_converted_last = 0.0;
        pad.js_ticks_converted = 0.0;
        pad.js_ticks_delta = 0.0;
        pad.js_delta_tick_frac = 0L;        /* seq24 0.9.3; long value      */

        /*
         *  See note 1 in the function banner.
         */

        bool ok = jack_song_mode() && ! m_dont_reset_ticks;
        m_dont_reset_ticks = false;
        if (ok)
        {
            m_current_tick = double(m_starting_tick);
            pad.js_current_tick = long(m_starting_tick);
            pad.js_clock_tick = m_starting_tick;
            set_last_ticks(m_starting_tick);
        }

        int ppqn = m_master_bus->get_ppqn();
        last = microtime();                     /* depends on OS            */
        while (is_running())
        {
            /**
             *  See note 2 in the function banner.
             */

            current = microtime();
            delta = current - last;
            long delta_us = delta;
            midibpm bpm  = m_master_bus->get_beats_per_minute();

            /*
             *  See note 3 in the function banner.
             */

            long long delta_tick_num = bpm * ppqn * delta_us +
                pad.js_delta_tick_frac;

            long delta_tick = long(delta_tick_num / 60000000LL);
            pad.js_delta_tick_frac = long(delta_tick_num % 60000000LL);
            if (m_usemidiclock)
            {
                delta_tick = m_midiclocktick;       /* int to double */
                m_midiclocktick = 0;
            }
            if (m_midiclockpos >= 0)
            {
                delta_tick = 0;
                m_current_tick = double(m_midiclockpos);
                pad.js_clock_tick = pad.js_current_tick = pad.js_total_tick =
                    m_midiclockpos;

                m_midiclockpos = -1;
            }

#if defined SEQ66_JACK_SUPPORT
            bool jackrunning = m_jack_asst.output(pad);
            if (jackrunning)
            {
                // No additional code needed besides the output() call above.
            }
            else
            {
#endif

#if defined USE_THIS_STAZED_CODE_WHEN_READY
                /*
                 * If we reposition key-p, FF, rewind, adjust delta_tick for
                 * change then reset to adjusted starting.  We have to grab
                 * the clock tick if looping is unchecked while we are
                 * running the performance; we have to initialize the MIDI
                 * clock (send EVENT_MIDI_SONG_POS); we have to restart at
                 * the left marker; and reset the tempo list (which Seq64
                 * doesn't have).
                 */

                if (song_mode() && && ! m_usemidiclock && m_reposition)
                {
                    current_tick = clock_tick;
                    delta_tick = m_starting_tick - clock_tick;
                    init_clock = true;
                    m_starting_tick = m_left_tick;
                    m_reposition = false;
                    m_reset_tempo_list = true;
                }
#endif  // USE_THIS_STAZED_CODE_WHEN_READY

                /*
                 * The default if JACK is not compiled in, or is not
                 * running.  Add the delta to the current ticks.
                 */

                pad.js_clock_tick += delta_tick;
                pad.js_current_tick += delta_tick;
                pad.js_total_tick += delta_tick;
                pad.js_dumping = true;
                m_current_tick = double(pad.js_current_tick);
#if defined SEQ66_JACK_SUPPORT
            }
#endif

            /*
             * If we reposition key-p from perfroll, reset to adjusted
             * start.
             */

            bool change_position = jack_song_mode() && ! m_usemidiclock;
            if (change_position)
                change_position = m_reposition;

            if (change_position)
            {
                set_last_ticks(m_starting_tick);
                m_starting_tick = m_left_tick;      /* restart at L marker  */
                m_reposition = false;
            }

            /*
             * pad.js_init_clock will be true when we run for the first time,
             * or as soon as JACK gets a good lock on playback.
             */

            if (pad.js_init_clock)
            {
                m_master_bus->init_clock(midipulse(pad.js_clock_tick));
                pad.js_init_clock = false;
            }
            if (pad.js_dumping)
            {
                /*
                 * THIS IS A MESS WE WILL HAVE TO SORT OUT.  If looping, then
                 * we ought to play if any of the tested flags are true.
                 */

                bool perfloop = m_looping;
                if (perfloop)
                    perfloop = song_mode() || start_from_perfedit();

                if (perfloop)
                {
                    /*
                     * This stazed JACK code works better than the original
                     * code, so it is now permanent code.
                     */

                    static bool jack_position_once = false;
                    midipulse rtick = get_right_tick();     /* can change? */
                    if (pad.js_current_tick >= rtick)
                    {
                        if (is_jack_master() && ! jack_position_once)
                        {
                            position_jack(true, m_left_tick);
                            jack_position_once = true;
                        }
                        double leftover_tick = pad.js_current_tick - rtick;

                        /*
                         * Do not play during starting to avoid xruns on
                         * fast-forward or rewind.
                         */

                        if (is_jack_running())
                        {
#if defined SEQ66_JACK_SUPPORT
                            if (m_jack_asst.transport_not_starting())
                                play(rtick - 1);
#endif
                        }
                        else
                            play(rtick - 1);

                        midipulse ltick = get_left_tick();
                        set_last_ticks(ltick);
                        m_current_tick = double(ltick) + leftover_tick;
                        pad.js_current_tick = double(ltick) + leftover_tick;
                    }
                    else
                        jack_position_once = false;
                }

                /*
                 * Don't play during JackTransportStarting to avoid xruns on
                 * FF or RW.
                 */

                if (is_jack_running())
                {
#if defined SEQ66_JACK_SUPPORT
                    if (m_jack_asst.transport_not_starting())
                    {
#endif
                        midipulse jackrtick = pad.js_current_tick;
                        play(midipulse(jackrtick));
#if defined SEQ66_JACK_SUPPORT
                    }
#endif
                }
                else
                {
                    play(midipulse(pad.js_current_tick));
                }

                /*
                 * The next line enables proper pausing in both old and seq32
                 * JACK builds.
                 */

                set_jack_tick(pad.js_current_tick);
                m_master_bus->emit_clock(midipulse(pad.js_clock_tick));
            }

            /**
             *  Figure out how much time we need to sleep, and do it.
             */

            last = current;
            current = microtime();
            delta = current - last;

            /**
             * Now we want to trigger every c_thread_trigger_width_us, and it
             * took us delta_us to play().  Also known as the "sleeping_us".
             */

            delta_us = c_thread_trigger_width_us - delta;

            /**
             * Check MIDI clock adjustment.  We replaced "60000000.0f / m_ppqn
             * / bpm" with a function.
             */

            double dct = double_ticks_from_ppqn(m_ppqn);
            double next_total_tick = pad.js_total_tick + dct;
            double next_clock_delta = next_total_tick - pad.js_total_tick - 1;
            double next_clock_delta_us =
                next_clock_delta * pulse_length_us(bpm, m_ppqn);

            if (next_clock_delta_us < (c_thread_trigger_width_us * 2.0))
                delta_us = long(next_clock_delta_us);

            if (delta_us > 0)
                (void) microsleep(delta_us);            /* timing.hpp       */

            if (pad.js_jack_stopped)
                inner_stop();
        }

        /*
         * Disabling this setting allows all of the progress bars (seqroll,
         * perfroll, and the slots in the mainwid) to stay visible where
         * they paused.  However, the progress still restarts when playback
         * begins again, without some other changes.  m_tick is the progress
         * play tick that displays the progress bar.
         */

        if (song_mode())
        {
            if (is_jack_master())                       // running Song Master
                position_jack(song_mode(), m_left_tick);
        }
        else
        {
            if (is_jack_master())                       // running Live Master
                position_jack(song_mode(), 0);
        }
        if (! m_usemidiclock)                           // stop by MIDI event?
        {
            if (! is_jack_running())
            {
                if (song_mode())
                    set_tick(m_left_tick);              // song mode default
                else if (! m_dont_reset_ticks)
                    set_tick(0);                        // live mode default
            }
        }

        /*
         * This means we leave m_tick at stopped location if in slave mode or
         * if m_usemidiclock == true.
         */

        m_master_bus->flush();
        m_master_bus->stop();
    }
}

/**
 *  This function is called by input_thread_func().  It handles certain MIDI
 *  input events.
 *
 * Stazed:
 *
 *      http://www.blitter.com/~russtopia/MIDI/~jglatt/tech/midispec/ssp.htm
 *
 *      Example: If a Song Position value of 8 is received, then a sequencer
 *      (or drum box) should cue playback to the third quarter note of the
 *      song.  (8 MIDI beats * 6 MIDI clocks per MIDI beat = 48 MIDI Clocks.
 *      Since there are 24 MIDI Clocks in a quarter note, the first quarter
 *      occurs on a time of 0 MIDI Clocks, the second quarter note occurs upon
 *      the 24th MIDI Clock, and the third quarter note occurs on the 48th
 *      MIDI Clock).
 *
 *      8 MIDI beats * 6 MIDI clocks per MIDI beat = 48 MIDI Clocks.
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
 *      the MIDI time clock.  Comments in the banner.
 *
 * EVENT_MIDI_CONTINUE:
 *
 *      MIDI continue: start from current position.  This is sent immediately
 *      after EVENT_MIDI_SONG_POS, and is used for starting from other than
 *      beginning of the song, or for starting from previous location at
 *      EVENT_MIDI_STOP. Again, converted to Kepler34 mode of setting the
 *      playback mode to Live mode.
 *
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
 *
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
 *
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
 *
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
 *  For events less than or equal to SysEx, we call midi_control_event() to
 *  handle the MIDI controls that Sequencer64 supports.  (These are
 *  configurable in the "rc" configuration file.) We test for MIDI control
 *  events even if "dumping".  Otherwise, we cannot handle any more control
 *  events once recording is turned on.  Warning:  This can slow down
 *  recording.
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
    while (m_io_active)                 /* should we lock this variable?    */
    {
        if (! poll_cycle())
            return;
    }
}

/**
 *  A helper function for input_func().
 */

bool
performer::poll_cycle ()
{
    bool result = true;
    if (m_master_bus->poll_for_midi() > 0)
    {
        do
        {
            event ev;
            if (m_master_bus->get_midi_event(&ev))
            {
                if (ev.below_sysex())                       /* below 0xF0   */
                {
                    if (m_master_bus->is_dumping())         /* see banner   */
                    {
                        if (midi_control_event(ev, true))   /* quick check  */
                        {
#if defined SEQ66_PLATFORM_DEBUG_TMI
                            std::string estr = to_string(ev);
                            infoprintf("MIDI ctrl event %s", estr);
#endif
                        }
                        else
                        {
                            ev.set_timestamp(get_tick());
#if defined SEQ66_PLATFORM_DEBUG
                            if (rc().verbose())
                                ev.print_note();

                            if (rc().show_midi())
                                ev.print();
#endif

                            if (m_filter_by_channel)
                                m_master_bus->dump_midi_input(ev);
                            else
                                m_master_bus->get_sequence()->stream_event(ev);
                        }
                    }
                    else
                    {
                        if (rc().show_midi())
                            ev.print();

                        (void) midi_control_event(ev);
                    }
                }
                else if (ev.is_midi_start())
                {
                    song_start_mode(sequence::playback::live);
                    m_midiclockrunning = m_usemidiclock = true;
                    m_midiclocktick = m_midiclockpos = 0;
                    auto_stop();
                    auto_play();
                    if (rc().verbose())
                        infoprint("MIDI Start");
                }
                else if (ev.is_midi_continue())
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
                else if (ev.is_midi_stop())
                {
                    all_notes_off();
                    m_usemidiclock = true;
                    m_midiclockrunning = false;
                    m_midiclockpos = get_tick();
                    auto_stop();
                    if (rc().verbose())
                        infoprint("MIDI Stop");
                }
                else if (ev.is_midi_clock())
                {
                    if (m_midiclockrunning)
                        m_midiclocktick += m_midiclockincrement;
                }
                else if (ev.is_midi_song_pos())
                {
                    midibyte d0, d1;                /* see note in banner   */
                    ev.get_data(d0, d1);
                    m_midiclockpos = combine_bytes(d0, d1);
                }
                else if (ev.is_sysex())             /* what about channel?  */
                {
                    if (rc().show_midi())
                        ev.print();

                    if (rc().pass_sysex())
                        m_master_bus->sysex(&ev);
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
 *      These use cases are meant to apply to either a Seq32 or a regular build
 *      of Sequencer64, eventually.  Currently, the regular build does not have
 *      a concept of a "global" perform song-mode flag.
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
 * \param songmode
 *      Indicates if the caller wants to start the playback in Song mode
 *      (sometimes erroneously referred to as "JACK mode").  In the seq32 code
 *      at GitHub, this flag was identical to the "global_jack_start_mode"
 *      flag, which is true for Song mode, and false for Live mode.  False
 *      disables Song mode, and is the default, which matches seq24.
 *      Generally, we pass true in this parameter if we're starting playback
 *      from the perfedit window.  It alters the m_start_from_perfedit member,
 *      not the m_song_start_mode member (which replaces the global flag now).
 */

void
performer::start_playing (bool songmode)
{
    m_start_from_perfedit = songmode;
    songmode = songmode || song_mode();
    if (songmode)
    {
       /*
        * Allow to start at key-p position if set; for cosmetic reasons,
        * to stop transport line flicker on start, position to the left
        * tick.
        *
        *   m_jack_asst.position(true, m_left_tick);    // position_jack()
        *
        * The "! m_repostion" doesn't seem to make sense.
        */

       if (is_jack_master() && ! m_reposition)
           position_jack(true, m_left_tick);
    }
    else
    {
        if (is_jack_master())
            position_jack(false);

        if (resume_note_ons())                          /* for issue #5     */
        {
            for (auto seqi : m_play_set.seq_container())
                seqi->resume_note_ons(get_tick());
        }
    }
    start_jack();
    start(songmode);                                    /* Song vs Live     */
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
 *  User layk noted this call, and it makes sense to not do this here, since it
 *  is unknown at this point what the actual status is.  Note that we STILL
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
performer::pause_playing (bool songmode)
{
    m_dont_reset_ticks = true;
    is_running(! is_running());
    stop_jack();
    if (is_jack_running())
    {
        m_start_from_perfedit = songmode;   /* act like start_playing()     */
    }
    else
    {
        reset_sequences(true);              /* don't reset "last-tick"      */
        m_usemidiclock = false;
        m_start_from_perfedit = false;      /* act like stop_playing()      */
    }
    send_play_states(midicontrolout::uiaction::pause);
}

/**
 *  Encapsulates a series of calls used in mainwnd.  Stops playback, turns off
 *  the (new) m_dont_reset_ticks flag, and set the "is-pattern-playing" flag
 *  to false.  With stop, reset the start-tick to either the left-tick or the
 *  0th tick (to be determined, currently resets to 0).
 */

void
performer::stop_playing ()
{
    stop_jack();
    stop();
    m_dont_reset_ticks = m_start_from_perfedit = false;
    for (auto notify : m_notify)
        (void) notify->on_automation_change(automation::slot::stop);
}

/**
 *
 */

void
performer::auto_play ()
{
    bool songmode = song_mode();
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
            start_playing(songmode);
            isplaying = true;
        }
    }
    else if (! is_running())
    {
        start_playing(songmode);
        isplaying = true;
    }
    is_pattern_playing(isplaying);
}

/**
 *
 */

void
performer::auto_pause ()
{
    bool songmode = song_mode();
    bool isplaying = false;
    if (is_running())
    {
        pause_playing(songmode);
    }
    else
    {
        start_playing(songmode);
        isplaying = true;
    }
    is_pattern_playing(isplaying);
}

/**
 *
 */

void
performer::auto_stop ()
{
    stop_playing();
    is_pattern_playing(false);
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
 * \param tick
 *      Provides the tick at which to start playing.  This value is also
 *      copied to m_tick.
 */

void
performer::play (midipulse tick)
{
    set_tick(tick);
    bool songmode = song_mode();
#if defined SEQ66_PLATFORM_DEBUG
    int count = 0;
    for (auto seqi : m_play_set.seq_container())
    {
        if (not_nullptr(seqi))
        {
            seqi->play_queue(tick, songmode, resume_note_ons());
        }
        else
        {
            printf("Null sequence at play() count = %d\n", count);
            break;
        }
        ++count;
    }
#else
    for (auto seqi : m_play_set.seq_container())
        seqi->play_queue(tick, songmode, resume_note_ons());
#endif

    if (not_nullptr(m_master_bus))
        m_master_bus->flush();                      /* flush MIDI buss  */
}

void
performer::play_all_sets (midipulse tick)
{
    set_tick(tick);
    sequence::playback songmode = song_start_mode();
    mapper().play_all_sets(tick, songmode, resume_note_ons());
    if (not_nullptr(m_master_bus))
        m_master_bus->flush();                      /* flush MIDI buss  */
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

void
performer::panic ()
{
    stop_playing();
    inner_stop();                                   /* force inner stop     */
    mapper().panic();
    if (m_master_bus)
        m_master_bus->panic();                      /* flush the MIDI buss  */

    set_tick(0);
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
 *  Adds a trigger on behalf of a sequence.
 *
 * \param seqno
 *      Indicates the sequence that needs to have its trigger handled.
 *
 * \param tick
 *      The MIDI pulse number at which the trigger should be handled.
 */

void
performer::add_trigger (seq::number seqno, midipulse tick)
{
    seq::pointer s = get_sequence(seqno);
    if (s)
    {
        midipulse seqlength = s->get_length();
        tick -= tick % seqlength;
        push_trigger_undo(seqno);
        s->add_trigger(tick, seqlength);
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

/**
 *  Simple error reporting for debugging.
 */

void
performer::show_ordinal_error (ctrlkey ordinal, const std::string & tag)
{
    std::cerr << "Ordinal 0x" << std::hex << ordinal << " " << tag << std::endl;
}

/**
 *  This static function merely prints the parameters passed to it.
 */

void
performer::print_parameters
(
    const std::string & tag, automation::action a,
    int d0, int d1, bool inverse
)
{
    if (rc().verbose())
    {
        std::cout
            << tag << " '" << opcontrol::action_name(a) << "'; "
            << "d0 = " << d0 << "; "
            << "d1 = " << d1 << "; "
            << "inv = " << inverse
            << std::endl
            ;
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
 *  Sets or unsets the keep-queue functionality, to be used by the new "Q"
 *  button in the main window.
 */

void
performer::set_keep_queue (bool activate)
{
    automation::action a = activate ?
        automation::action::off : automation::action::on;

    if (activate)
        set_sequence_control_status(a, automation::ctrlstatus::queue);
}

/**
 *  If the given status is present in the automation::ctrlstatus::snapshot, the
 *  playing state is saved.  Then the given status is OR'd into the
 *  control-status.
 *
 *  If the given status is present in the automation::ctrlstatus::snapshot, the
 *  playing state is restored.  Then the given status is reversed in
 *  control-status.
 *
 *  If the given status includes automation::ctrlstatus::queue, this is a signal
 *  to stop queuing (which is already in place elsewhere).  It also unsets the
 *  new queue-replace feature.
 *
 * \param a
 *      The action to be applied.  Toggle and On set the status, and Off unsets
 *      the status.
 *
 * \param status
 *      The status item to be applied.
 *
 * \param inverse
 *      If true (the default is false), the status to be applied is inverted.
 */

void
performer::set_sequence_control_status
(
    automation::action a,
    automation::ctrlstatus status,
    bool inverse
)
{
    bool set_it = a == automation::action::toggle || a == automation::action::on;
    if (inverse)
        set_it = ! set_it;

    if (set_it)
    {
        if (midi_control_in().is_snapshot(status))
            save_snapshot();

        midi_control_in().add_status(status);
    }
    else
    {
        if (midi_control_in().is_snapshot(status))
            restore_snapshot();

        if (midi_control_in().is_queue(status))
            unset_queued_replace();

        midi_control_in().remove_status(status);
    }

    if (midi_control_in().is_queue(status))
        send_event(midicontrolout::uiaction::queue, set_it);

    if (midi_control_in().is_oneshot(status))
        send_event(midicontrolout::uiaction::oneshot, set_it);

    if (midi_control_in().is_replace(status))
        send_event(midicontrolout::uiaction::replace, set_it);

    if (midi_control_in().is_snapshot(status))
        send_event(midicontrolout::uiaction::snap1, set_it);
}

/**
 *  A help function to make the code a tad more readable.
 */

void
performer::send_event (midicontrolout::uiaction a, bool on)
{
    midi_control_out().send_event(a, on);
}

/**
 *  Sets the state of the Start, Stop, and Play button(s) as configured in the
 *  "ctrl" file.  It first turns off all of the states (which might be mapped
 *  to one button or to three buttons), then turns on the desired state.
 *
 * \param a
 *      Provides the desired state to set, which is one of the following
 *      values of uiaction: play, stop, and pause.  The corresponding event is
 *      sent.  If another value (max is the best one to use), then all are
 *      off.
 */

void
performer::send_play_states (midicontrolout::uiaction a)
{
    bool play_on = false;
    bool stop_on = false;
    bool pause_on = false;
    switch (a)
    {
        case midicontrolout::uiaction::play:

            play_on = true;
            break;

        case midicontrolout::uiaction::stop:

            stop_on = true;
            break;

        case midicontrolout::uiaction::pause:

            pause_on = true;
            break;

        default:

            /* leave them all off */
            break;
    }
    send_event(midicontrolout::uiaction::play, false);
    send_event(midicontrolout::uiaction::stop, false);
    send_event(midicontrolout::uiaction::pause, false);
    if (play_on)
        send_event(midicontrolout::uiaction::play, true);
    else if (stop_on)
        send_event(midicontrolout::uiaction::stop, true);
    else if (pause_on)
        send_event(midicontrolout::uiaction::pause, true);
}

/**
 *  Helper function that clears the queued-replace feature.  This also clears
 *  the queue mode; we shall see if this disrupts any user's workflow.
 *
 * \param clearbits
 *      If true (the default), then clear the queue and replace status bits.
 *      If the user is simply replacing the current replace pattern with
 *      another pattern, we pass false for this parameter.
 */

void
performer::unset_queued_replace (bool clearbits)
{
    if (m_queued_replace_slot != sm_no_queued_solo)
    {
        m_queued_replace_slot = sm_no_queued_solo;
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
    mutes().group_learn(learning);
    midi_control_out().send_learning(learning);
    for (auto notify : m_notify)
        (void) notify->on_group_learn(learning);
}

void
performer::group_learn_complete (const keystroke & k, bool good)
{
    mutes().group_learn(false);
    for (auto notify : m_notify)
    {
        (void) notify->on_group_learn(false);
        (void) notify->on_group_learn_complete(k, good);
    }
}

/**
 *  If the given sequence is active, then it is toggled as per the current value
 *  of control-status.  If control-status is automation::ctrlstatus::queue, then
 *  the sequence's toggle_queued() function is called.  This is the "mod queue"
 *  implementation.
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

void
performer::sequence_playing_toggle (seq::number seqno)
{
    seq::pointer s = get_sequence(seqno);
    if (s)
    {
        bool is_queue = midi_control_in().is_queue();
        bool is_replace = midi_control_in().is_replace();
        bool is_oneshot = midi_control_in().is_oneshot();
        if (is_oneshot && ! s->playing())
        {
            s->toggle_one_shot();                   /* why not just turn on */
        }
        else if (is_queue && is_replace)
        {
            if (m_queued_replace_slot != sm_no_queued_solo)
            {
                if (seqno != m_queued_replace_slot)
                {
                    unset_queued_replace(false);    /* do not clear bits    */
                    save_queued(seqno);
                }
            }
            else
                save_queued(seqno);

            unqueue_sequences(seqno);
            m_queued_replace_slot = seqno;
        }
        else if (is_queue)
        {
            s->toggle_queued();
            announce_sequence(s, mapper().seq_to_offset(s));
        }
        else
        {
            if (is_replace)
            {
                set_sequence_control_status
                (
                    automation::action::off,
                    automation::ctrlstatus::replace
                );
                off_sequences();
            }
            s->toggle_playing();
            announce_sequence(s, mapper().seq_to_offset(s));
        }

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
                    s->split_trigger(tick, trigger::splitpoint::exact);
                    s->delete_trigger(tick);
                }
            }
            else            /* if not playing, start recording a new strip  */
            {
                /*
                 * ca 2019-02-06 Issue #171, fixed by removing the check.
                 * Always snap.  Make the snap smaller if desired.
                 */

                tick -= tick % seq_length;
                push_trigger_undo();
                s->song_recording_start(tick, false);   /* no snap */
            }
        }
    }
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
    m_song_recording = f;
    if (! f)
        song_recording_stop();
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

void
performer::sequence_playing_change (seq::number seqno, bool on)
{
    bool qinprogress = midi_control_in().is_queue();
    mapper().sequence_playscreen_change(seqno, on, qinprogress);
    notify_trigger_change(seqno, change::no);
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
 *  Handle a control key.  The caller (e.g. a Qt key-press event handler) grabs
 *  the event text and modifiers and converts it to an ctrlkey value
 *  (ranging from 0x00 to 0xFE).  We show the caller code here for reference:
 *
\verbatim
        ctrlkey kkey = event->key();
        unsigned kmods = static_cast<unsigned>(event->modifiers());
        ctrlkey ordinal = qt_modkey_ordinal(kkey, kmods);
        keystroke ks =keystroke(ordinal, press);
\endverbatim
 *
 *  We made a Qt function for this, qt_keystroke(), in the qt5_helpers.cpp/hpp
 *  module.
 *
 *  Next, we look up the keycontrol based on the ordinal value.  If this
 *  keycontrol is usable (it is not a default-constructed keycontrol),
 *  then we can use its slot value to look up the midioperation associated with
 *  this slot.
 *
 *  Also part of keystroke is whether the key was pressed or released.
 *  For some keystrokes, this difference matters.
 *  If the keycontrol::is_toggle() function returns false, then press will
 *  be handled differently from release.  Furthermore, if the keystroke is
 *  release, the "inverse" flag of the event will be set to true.
 *
 *  Note that the default action for most keys is automation::action::on,
 *  but some keys are configured to do automation::action::on during a
 *  key-press, and automation::action::off during a key-release, while a few
 *  keys do automation::action::toggle.
 *
\verbatim
 *      Action:             On            Toggle          Off
 *     -----------------|--------------|--------------|--------------
 *    | Toggle:         |              |              |              |
 *    | Press = true    |              |              |              |
 *    | Press = false   |              |              |              |
 *    |-----------------|--------------|--------------|--------------
 *    | Non-Toggle:     |              |              |              |
 *    | Press = true    |              |              |              |
 *    | Press = false   |              |              |              |
 *     -----------------|--------------|--------------|--------------
\endverbatim
 *
 *  To summarize:
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
 *      -   Modal keys.  The action is automation::action::on.  But the state of
 *          the key-press is used.  If a press, the mode is activated.  If a
 *          release, the mode is deactivated. This operating mode is determined
 *          by the automation callback function.
 *
 *  Example:
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
 * \param key
 *      Provides the ordinal number of the keystroke.  See above for how to
 *      get it (in Qt 5).
 *
 * \return
 *      Returns true if the action was handled.  Returns false if the action
 *      failed or was not handled.  The caller has to know what the context is.
 */

bool
performer::midi_control_keystroke (const keystroke & k)
{
    keystroke kkey = k;
    if (is_group_learn())
        kkey.shift_lock();

    const keycontrol & kc = m_key_controls.control(kkey.key());
    bool result = kc.is_usable();
    if (result)
    {
        automation::slot s = kc.slot_number();
        const midioperation & mop = m_operations.operation(s);
        if (mop.is_usable())
        {
            /*
             * Note that the "inverse" parameter is based on key press versus
             * release.  Not all automation functions care about this setting.
             */

            automation::action a = kc.action_code();
            bool invert = ! kkey.is_press();
            int d0 = 0;
            int index = kc.control_code();
            bool learning = is_group_learn();               /* before   */
            result = mop.call(a, d0, index, invert);
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
                    show_ordinal_error(kkey.key(), "call returned false");
            }
        }
        else
            show_ordinal_error(kkey.key(), "call unusable");
    }
    return result;
}

/**
 *  Looks up the MIDI event and calls the corresponding function, if any.
 *
 * \param ev
 *      The MIDI event to process.
 *
 * \param recording
 *      This parameter, if true, restricts the handled controls to start, stop, and
 *      record.
 *
 * \return
 *      Returns true if the event was valid and usable, and the call to the
 *      automation function returned true.  Returns false if the action was
 *      not processed.
 */

bool
performer::midi_control_event (const event & ev, bool recording)
{
    bool result = m_midi_control_in.is_enabled();
    if (result)
    {
        midicontrol::key k(ev);
        const midicontrol & incoming = m_midi_control_in.control(k);
        result = incoming.is_usable();
        if (result && m_midi_control_buss < c_bussbyte_max)
            result = ev.input_bus() == m_midi_control_buss;

        if (result)
        {
            automation::slot s = incoming.slot_number();
            const midioperation & mop = m_operations.operation(s);
            if (mop.is_usable())
            {
                bool process_the_action = incoming.in_range(ev.d1());
                if (recording)
                {
                    process_the_action = s == automation::slot::start ||
                        s == automation::slot::stop ||
                        s == automation::slot::record ;
                }
                if (process_the_action)
                {
                    automation::action a = incoming.action_code();
                    bool invert = incoming.inverse_active();
                    int d0 = incoming.d0();
                    int index = incoming.control_code(); /* in lieu of d1() */
                    result = mop.call(a, d0, index, invert);
                }
                else
                    result = false;
            }
        }
    }
    return result;
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
        [this, f] (automation::action a, int d0, int d1, bool inverse)
        {
            return (this->*f) (a, d0, d1, inverse);
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
        [this] (automation::action a, int d0, int d1, bool inverse)
        {
            return loop_control(a, d0, d1, inverse);
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
            [this] (automation::action a, int d0, int d1, bool inverse)
            {
                return mute_group_control(a, d0, d1, inverse);
            }
        );
        result = m_operations.add(mutmop);
    }

    for (int index = 0; /* breaker */ ; ++index)
    {
        if (sm_auto_func_list[index].ap_slot != automation::slot::maximum)
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
                error_message(errmsg);
                break;
            }
        }
        else
            break;
    }
#if defined SEQ66_PLATFORM_DEBUG
    if (rc().verbose())
        m_operations.show();
#endif

    return result;
}

/**
 *  Sets the given mute group.  If there is a change,  then the subscribers are
 *  notified.  If the 'rc' "save-mutes-to" setting indicates saving it to the
 *  MIDI file, then this becomes a modify action.
 */

bool
performer::set_mutes (mutegroup::number gmute, const midibooleans & bits)
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
        }
    }
    return result;
}

/**
 *  Clears the mute groups.  If there any to clear, then the subscribers are
 *  notified.  If the "mutes" "save-mutes-to" setting indicates saving it to
 *  the MIDI file, then this becomes a modify action.
 */

bool
performer::clear_mutes ()
{
    bool result = mutes().any();
    mutes().reset_defaults();               /* clears and adds all zeros    */
    if (result)
    {
        change c = mutes().group_save_to_midi() ?
            change::yes : change:: no ;

        notify_mutes_change(mutegroup::unassigned(), c);
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
    if (result)
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
    automation::action a, int d0, int loopnumber, bool inverse
)
{
    std::string name = "Pattern ";
    name += std::to_string(loopnumber);
    print_parameters(name, a, d0, loopnumber, inverse);

    seq::number sn = mapper().play_seq(loopnumber);
    bool result = sn >= 0;
    if (result && ! inverse)
    {
        if (slot_shift() > 0)
        {
#if defined USE_OLD_STYLE_SLOT_SHIFT
            sn += slot_shift() * screenset_size();
#else
            if (columns() == setmaster::Columns())
            {
                if (rows() > setmaster::Rows())
                    sn += slot_shift() * rows();        /* move down x rows */
            }
            else
            {
                sn += slot_shift() * screenset_size();
            }
#endif
            clear_slot_shift();
        }
        m_pending_loop = sn;
        if (m_seq_edit_pending || m_event_edit_pending)
        {
            result = false;             /* let the caller handle it */
#if defined SEQ66_PLATFORM_DEBUG
            infoprint("loop_control(): edit pending");
#endif
        }
        else
        {
            if (a == automation::action::toggle)
                sequence_playing_toggle(sn);
            else if (a == automation::action::on)
                sequence_playing_change(sn, true);
            else if (a == automation::action::off)
                sequence_playing_change(sn, false);
        }
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
    automation::action a, int d0, int groupnumber, bool inverse
)
{
    std::string name = is_group_learn() ? "Mute Learn " : "Mutes " ;
    name += std::to_string(d0);
    print_parameters(name, a, d0, groupnumber, inverse);

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

/**
 *  Implements a no-op function for reserved slots not yet implemented, or it
 *  can can serve as an indication that the caller (e.g. a user-interface)
 *  should handle the functionality itself.
 *
 * \return
 *      Because the slot is not implemented, false is returned.
 */

bool
performer::automation_no_op (automation::action a, int d0, int d1, bool inverse)
{
    std::string name = "No-op";
    print_parameters(name, a, d0, d1, inverse);
    return false;
}

/**
 *  Implements BPM Up and BPM Down for MIDI control.  There is really no need
 *  for two BPM configuration lines for MIDI control, since the configured MIDI
 *  event can specify which is needed.
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
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "BPM";
    print_parameters(name, a, d0, d1, inverse);
    if (a == automation::action::toggle)            /* for keystroke */
        increment_beats_per_minute();
    else if (a == automation::action::on)
        increment_beats_per_minute();
    else if (a == automation::action::off)
        decrement_beats_per_minute();

    return true;
}

/**
 *  No matter how BPM Down is configured for MIDI control, if present and the
 *  MIDI event matches, it will act like a BPM Down.  This matches the behavior
 *  of Seq24/Sequencer64.
 */

bool
performer::automation_bpm_dn
(
    automation::action /*a*/, int d0, int d1, bool inverse
)
{
    return automation_bpm_up_dn(automation::action::off, d0, d1, inverse);
}

/**
 *  Implements screenset Up and Down.  The default keystrokes are "]" for up
 *  and "[" for down.  Note that all keystrokes generate toggles, and the
 *  release sets "inverse" to true.
 */

bool
performer::automation_ss_up_dn
(
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Screenset";
    print_parameters(name, a, d0, d1, inverse);
    if (! inverse)
    {
        if (a == automation::action::toggle)            /* for keystroke */
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
    automation::action /*a*/, int d0, int d1, bool inverse
)
{
    return automation_ss_up_dn(automation::action::off, d0, d1, inverse);
}

/**
 *  Implements mod_replace.
 *
 *  For MIDI control, there should be no support for toggle, but we're not sure
 *  how to implement this feature.
 *
 *  For keystrokes, the user-interface's key-press callback should set the
 *  inverse flag to false, and the key-release callback should set it to true.
 *  The action will always be toggle for keystrokes.
 */

bool
performer::automation_replace
(
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Mod Replace";
    print_parameters(name, a, d0, d1, inverse);
    set_sequence_control_status(a, automation::ctrlstatus::replace, inverse);
    return true;
}

/**
 *  Implements mod_snapshot.
 */

bool
performer::automation_snapshot
(
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Mod Snapshot";
    print_parameters(name, a, d0, d1, inverse);
    set_sequence_control_status(a, automation::ctrlstatus::snapshot, inverse);
    return true;
}

/**
 *  Implements mod_queue.
 */

bool
performer::automation_queue
(
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Mod Queue";
    print_parameters(name, a, d0, d1, inverse);
    set_sequence_control_status(a, automation::ctrlstatus::queue, inverse);
    return true;
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
    automation::action a, int d0, int d1, bool inverse
)
{
    bool result = true;
    std::string name = "Mod Group Mute";
    print_parameters(name, a, d0, d1, inverse);
    if (a == automation::action::toggle)
        mapper().toggle_group_mode();
    else if (a == automation::action::on)
        mapper().group_mode(true);
    else if (a == automation::action::off)
        mapper().group_mode(false);
    else
        result = false;

    return result;
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
    automation::action a, int d0, int d1, bool inverse
)
{
    bool result = true;
    std::string name = "Mod Group Learn";
    print_parameters(name, a, d0, d1, inverse);
    if (a == automation::action::toggle)
        learn_toggle();                         /* also notifies clients    */
    else if (a == automation::action::on)
        group_learn(true);                      /* also notifies clients    */
    else if (a == automation::action::off)
        group_learn(false);                     /* also notifies clients    */
    else
        result = false;

    return result;
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
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Play Screen-Set";
    print_parameters(name, a, d0, d1, inverse);
    set_playing_screenset(screenset::number(d1));
    return true;
}

/**
 *  Implements playback.  That is, start, pause, and stop.  These are MIDI
 *  controls. However, note that we also support separate actions for the
 *  keyboard:
 *
 *      -   automation_start()
 *      -   automation_stop()
 *
 *  If the action is a toggle (as happens with the "pause" key), then the
 *  toggling is ignored if \a inverse is true.  NEEDS FURTHER INVESTIGATION.
 *
 * \param a
 *      Provides the action to perform.  Toggle = pause; On = start; and off =
 *      stop.
 *
 * \return
 *      Returns true if the action was handled.
 */

bool
performer::automation_playback
(
    automation::action a, int d0, int d1, bool inverse
)
{
    bool result = false;
    std::string name = "Playback";
    print_parameters(name, a, d0, d1, inverse);
    if (a == automation::action::toggle)
    {
        if (! inverse)
            auto_pause();

        result = true;
    }
    else if (a == automation::action::on)
    {
        auto_play();
        result = true;
    }
    else if (a == automation::action::off)
    {
        auto_stop();
        result = true;
    }
    return result;
}

/**
 *  Implements song_record, which sets the status to recording live events into
 *  song triggers.  If \a inverse is true, nothing is done.
 */

bool
performer::automation_song_record
(
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Song Record";
    print_parameters(name, a, d0, d1, inverse);
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
 *  Implements solo.  This isn't clear even in Sequencer64.
 */

bool
performer::automation_solo
(
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Solo";
    print_parameters(name, a, d0, d1, inverse);
    if (a == automation::action::toggle)
    {
        // TODO
    }
    else if (a == automation::action::on)
    {
        // TODO
    }
    else if (a == automation::action::off)
    {
        // TODO
    }
    return true;
}

/**
 *  Implements thru.  If \a inverse is true, nothing is done.
 */

bool
performer::automation_thru
(
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Thru";
    print_parameters(name, a, d0, d1, inverse);
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
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "BPM Page";
    print_parameters(name, a, d0, d1, inverse);
    if (! inverse)
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
    automation::action /*a*/, int d0, int d1, bool inverse
)
{
    return automation_bpm_page_up_dn(automation::action::off, d0, d1, inverse);
}

/**
 *  Sets the screen by number.  Needs to be clarified.  If \a inverse is true,
 *  nothing is done, to support keystroke-release properly.
 */

bool
performer::automation_ss_set
(
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Screen-Set Set";
    print_parameters(name, a, d0, d1, inverse);
    if (! inverse)
        (void) set_playing_screenset(d1);

    return true;
}

/**
 *  Implements the recording control.  This function sets the recording status
 *  of incoming MIDI events.  If \a inverse is true, nothing is done.
 */

bool
performer::automation_record
(
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Record";
    print_parameters(name, a, d0, d1, inverse);
    if (! inverse)
    {
        seq::number seqno = seq::number(d1);
        if (a == automation::action::toggle)
            set_recording(seqno, false, true);                  /* toggles  */
        else if (a == automation::action::on)
            set_recording(seqno, true, false);                  /* on       */
        else if (a == automation::action::off)
            set_recording(seqno, false, false);                 /* off      */
    }
    return true;
}

/**
 *  Like record, but quantized.
 */

bool
performer::automation_quan_record
(
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Quantized Record";
    print_parameters(name, a, d0, d1, inverse);
    if (! inverse)
    {
        seq::number seqno = seq::number(d1);
        if (a == automation::action::toggle)
            set_quantized_recording(seqno, false, true);        /* toggles  */
        else if (a == automation::action::on)
            set_quantized_recording(seqno, true, false);        /* on       */
        else if (a == automation::action::off)
            set_quantized_recording(seqno, false, false);       /* off      */
    }
    return true;
}

/**
 *  Implements reset_seq.  It determines if pattern recording merges notes or
 *  overwrites them upon loop-return.
 *
 *  What about the "extend sequence" mode?  What about the return codes?
 */

bool
performer::automation_reset_seq
(
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Reset Sequence";
    print_parameters(name, a, d0, d1, inverse);
    if (! inverse)
    {
        seq::number seqno = seq::number(d1);
        if (a == automation::action::toggle)
            set_overwrite_recording(seqno, false, true);        /* toggles  */
        else if (a == automation::action::on)
            set_overwrite_recording(seqno, true, false);        /* on       */
        else if (a == automation::action::off)
            set_overwrite_recording(seqno, false, false);       /* off      */
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
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "One-shot Queue";
    print_parameters(name, a, d0, d1, inverse);
    set_sequence_control_status(a, automation::ctrlstatus::oneshot, inverse);
    return true;
}

bool
performer::automation_FF
(
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Fast-forward";
    print_parameters(name, a, d0, d1, inverse);

    /*
     * TO BE DETERMINED
     */

    return true;
}

bool
performer::automation_rewind
(
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Rewind";
    print_parameters(name, a, d0, d1, inverse);

    /*
     * TO BE DETERMINED
     */

    return true;
}

bool
performer::automation_top
(
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Top";
    print_parameters(name, a, d0, d1, inverse);

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
    automation::action a, int d0, int d1, bool inverse
)
{
    bool result = false;
    std::string name = "Playlist";
    print_parameters(name, a, d0, d1, inverse);
    if (! inverse)
    {
        if (a == automation::action::toggle)            /* select-by-value  */
            result = open_select_list_by_midi(d1);
        else if (a == automation::action::on)           /* select-next      */
            result = open_next_list();
        else if (a == automation::action::off)          /* select-previous  */
            result = open_previous_list();
    }

    return result;
}

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
    std::string & errmsg
)
{
    errmsg.clear();
    bool result = seq66::read_midi_file(*this, fn, ppqn(), errmsg);
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
                (void) error_message(nmf.error_message());
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
        std::string nfname = rc().notemap_filename();
        if (! notefile.empty())
            nfname = notefile;

        if (nfname.empty())
        {
            // TODO?
        }
        else
        {
            notemapfile nmf(*m_note_mapper, nfname, rc());
            result = nmf.write();
            if (! result)
                (void) error_message(nmf.error_message());
        }
    }
    return result;
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
    {
        rc().playlist_active(false);
    }
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
             * (void) error_message(m_play_list->error_message());
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
            (void) error_message(m_play_list->error_message());
         */
    }
    else
    {
        errprint("null playlist pointer");
    }
    return result;
}

/**
 *  Implements playlist control.  If \a inverse is true, nothing is done.  Note
 *  that currently the GUI may hardwire the usage of the arrow keys for this
 *  functionality.
 */

bool
performer::automation_playlist_song
(
    automation::action a, int d0, int d1, bool inverse
)
{
    bool result = false;
    std::string name = "Playlist Song";
    print_parameters(name, a, d0, d1, inverse);
    if (! inverse)
    {
        if (a == automation::action::toggle)            /* select-by-value  */
            result = open_select_song_by_midi(d1);
        else if (a == automation::action::on)           /* select-next      */
            result = open_next_song();
        else if (a == automation::action::off)          /* select-previous  */
            result = open_previous_song();
    }
    return result;
}

/**
 *  Implements setting the BPM by tapping a key.
 */

bool
performer::automation_tap_bpm
(
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Tap BPM";
    print_parameters(name, a, d0, d1, inverse);
    if (! inverse)
    {
        midibpm bpm = update_tap_bpm();
        if (bpm != get_beats_per_minute())
            set_beats_per_minute(bpm);
    }
    return true;
}

/**
 *  Starts playback.  The \a inverse parameter, if true, does nothing.  We don't
 *  want a double-clutch on start when a keystroke is released.
 */

bool
performer::automation_start
(
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Start";
    print_parameters(name, a, d0, d1, inverse);
    if (! inverse)
        auto_play();

    return true;
}

/**
 *  Stops playback.  The \a inverse parameter, if true, does nothing.  We don't
 *  want a double-clutch on start when a keystroke is released.
 */

bool
performer::automation_stop
(
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Stop";
    print_parameters(name, a, d0, d1, inverse);
    if (! inverse)
        auto_stop();

    return true;
}

/**
 *
 */

bool
performer::automation_snapshot_2
(
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Snapshot 2";
    print_parameters(name, a, d0, d1, inverse);

    /*
     * TO BE DETERMINED
     */

    return true;
}

bool
performer::automation_toggle_mutes
(
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Toggle Mutes";
    print_parameters(name, a, d0, d1, inverse);

    /*
     * TO BE DETERMINED
     */

    return true;
}

bool
performer::automation_song_pointer
(
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Song Pointer";
    print_parameters(name, a, d0, d1, inverse);

    /*
     * TO BE DETERMINED TODO
     */

    return true;
}

/**
 *  I think we don't want to support inverse for this one.  See the support for
 *  the "Q" button.  We might need to make this a toggle for the keystroke
 *  support.  TODO TODO TODO!!!
 */

bool
performer::automation_keep_queue
(
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Keep queue";
    print_parameters(name, a, d0, d1, inverse);
    if (! inverse)
        set_sequence_control_status(a, automation::ctrlstatus::queue, inverse);

    return true;
}

/**
 * \return
 *      Returns false so that the caller can take action on it.
 */

bool
performer::automation_edit_pending
(
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Seq edit pending";
    print_parameters(name, a, d0, d1, inverse);
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
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Event edit pending";
    print_parameters(name, a, d0, d1, inverse);
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
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Slot shift ";
    bool result = false;
    name += std::to_string(slot_shift() + 1);
    print_parameters(name, a, d0, d1, inverse);
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
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Mutes clear";
    bool result = false;
    name += std::to_string(slot_shift() + 1);
    print_parameters(name, a, d0, d1, inverse);
    if (! inverse)
    {
        clear_mutes();
        result = true;
    }
    return result;
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
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Song mode toggle";
    print_parameters(name, a, d0, d1, inverse);
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
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Toggle JACK Transport";
    print_parameters(name, a, d0, d1, inverse);
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
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Menu mode toggle TODO";
    bool result = false;
    print_parameters(name, a, d0, d1, inverse);
    if (! inverse)
    {
        // TODO
    }
    return result;
}

/**
 *  TODO TODO TODO
 */

bool
performer::automation_follow_transport
(
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Follow JACK Transport TODO";
    bool result = false;
    print_parameters(name, a, d0, d1, inverse);
    if (! inverse)
    {
        // TODO
    }
    return result;
}

/**
 *  TODO TODO TODO
 */

bool
performer::automation_panic
(
    automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Panic!";
    bool result = false;
    print_parameters(name, a, d0, d1, inverse);
    if (! inverse)
    {
        panic();
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
    { automation::slot::record, &performer::automation_record            },
    { automation::slot::quan_record, &performer::automation_quan_record  },
    { automation::slot::reset_seq, &performer::automation_reset_seq      },
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
        automation::slot::mod_snapshot_2,
        &performer::automation_snapshot_2
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
    { automation::slot::reserved_35, &performer::automation_no_op        },
    {
        automation::slot::pattern_edit, &performer::automation_edit_pending
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
    { automation::slot::reserved_43, &performer::automation_no_op        },
    { automation::slot::reserved_44, &performer::automation_no_op        },
    { automation::slot::reserved_45, &performer::automation_no_op        },
    { automation::slot::reserved_46, &performer::automation_no_op        },
    { automation::slot::reserved_47, &performer::automation_no_op        },
    { automation::slot::reserved_48, &performer::automation_no_op        },

    /*
     * Terminator
     */

    { automation::slot::maximum, &performer::automation_no_op            }
};

}           // namespace seq66

/*
 * performer.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


#if ! defined  SEQ66_PERFORMER_HPP
#define SEQ66_PERFORMER_HPP

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
 * \file          performer.hpp
 *
 *  This module declares/defines the base class for handling many facets
 *  of performing (playing) a full MIDI song.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-12
 * \updates       2021-09-23
 * \license       GNU GPLv2 or above
 *
 *  The main player!  Coordinates sets, patterns, mutes, playlists, you name
 *  it!
 */

#include <set>                          /* std::set, arbitary selection     */
#include <memory>                       /* std::shared_ptr<>, unique_ptr<>  */
#include <vector>                       /* std::vector<>                    */
#include <thread>                       /* std::thread                      */

#include "cfg/usrsettings.hpp"          /* PPQN constants                   */
#include "ctrl/keycontainer.hpp"        /* class seq66::keycontainer        */
#include "ctrl/midicontrolin.hpp"       /* class seq66::midicontrolin       */
#include "ctrl/midicontrolout.hpp"      /* seq66::midicontrolout            */
#include "ctrl/opcontainer.hpp"         /* class seq66::opcontainer         */
#include "midi/jack_assistant.hpp"      /* optional seq66::jack_assistant   */
#include "midi/mastermidibus.hpp"       /* seq66::mastermidibus ALSA/JACK   */
#include "play/clockslist.hpp"          /* list of seq66::e_clock settings  */
#include "play/inputslist.hpp"          /* list of boolean input settings   */
#include "play/mutegroups.hpp"          /* class seq66::mutegroups          */
#include "play/playlist.hpp"            /* seq66::playlist                  */
#include "play/sequence.hpp"            /* seq66::sequence                  */
#include "play/setmapper.hpp"           /* seq66::seqmanager and seqstatus  */
#include "util/condition.hpp"           /* seq66::condition (variable)      */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 * Offloads from the app limits header that provide a sanity check for
 * transposition values.  Also see the tranposition functions in the trigger
 * class.
 */

const int c_transpose_down_limit = c_num_keys / 2;
const int c_transpose_up_limit = -c_transpose_down_limit;

/*
 * Forward references.
 */

class keystroke;
class notemapper;
class rcsettings;
class usrsettings;

/**
 *  This class supports the performance mode.
 */

class performer
{
    friend class jack_assistant;
    friend class midifile;
    friend class rcfile;
    friend class playlist;
    friend class qperfeditframe64;
    friend class qplaylistframe;
    friend class qt5nsmanager;
    friend class qsmainwnd;
    friend class sequence;
    friend class wrkfile;

#if defined SEQ66_JACK_SUPPORT

    friend int jack_sync_callback
    (
        jack_transport_state_t state,
        jack_position_t * pos,
        void * arg
    );

    friend int jack_transport_callback (jack_nframes_t nframes, void * arg);
    friend void jack_shutdown (void * arg);
    friend void jack_timebase_callback
    (
        jack_transport_state_t state, jack_nframes_t nframes,
        jack_position_t * pos, int new_pos, void * arg
    );
    friend long get_current_jack_position (void * arg);

#endif  // SEQ66_JACK_SUPPORT

public:

    /**
     *  Provides settings for tempo recording.  Currently not used, though the
     *  functionality of logging and recording tempo is in place.
     */

    enum class record_tempo
    {
        log_event,
        on,
        off,
        max
    };

    /**
     *  Provides a setting for the fast-forward and rewind functionality.
     */

    enum class ff_rw
    {
        rewind  = -1,
        none    =  0,
        forward =  1,
        max
    };

    /**
     *  A visible representation of whether to "modify" the tune.  Some changes
     *  do not require the tune to be saved before closing. The "recreate"
     *  value is a stronger form of "yes", and additionally requests that key
     *  elements of the notified object need to be recreated.
     */

    enum class change
    {
        no,             /**< Do not set the modify-flag.                    */
        yes,            /**< Do set the modify-flag.                        */
        undo,           /**< Unset the modify-flag????                      */
        recreate,       /**< Recreate the user-interface(s).                */
        removed,        /**< Change was a removal; more specific than yes.  */
        signal,         /**< Could alter the UI from a different thread.    */
        max
    };

    /**
     *  A nested class used for notification of group-learn and other changes.
     *  The easiest way to use this class is by inheriting from it, then
     *  overriding the virtual functions defined within.  If that leads to
     *  multiple inheritance, well, for our use cases, that is not an issue,
     *  as we will not copy the user-interface classes anyway.  For an
     *  example, see one of the user-interface classes, such as qsmainwnd.
     *
     *  In each of the callbacks declared/defined below, the \a state
     *  parameter indicates the state to which the object is transitioning.
     */

    class callbacks
    {

    private:

        /**
         *  Provides a reference to the main performer object.
         */

        performer & m_performer;

    public:

        /**
         *  Provides a type definition for a list of pointers to objects
         *  supporting the performer::callbacks "interface".
         */

        using clients = std::vector<callbacks *>;

        /**
         *  Provides a way to indicate via a value what callback function is
         *  in force.
         */

        enum class index
        {
            group_learn,            /**< Group-learn turned on.             */
            group_learn_complete,   /**< Group-learn turned off.            */
            mutes_change,           /**< Change in the mute-state.          */
            set_change,             /**< Change in the active screen-set.   */
            sequence_change,        /**< New, deleted, or pasted pattern.   */
            automation_change,      /**< A start or stop control occurred.  */
            ui_change,              /**< Indicates a user-interface action. */
            trigger_change,         /**< A trigger changed pattern muting.  */
            resolution_change,      /**< A change in PPQN or BPM.           */
            song_change             /**< A different MIDI tune was loaded.  */
        };

    public:

        callbacks (performer & p) : m_performer(p)
        {
            /* Empty body */
        }

        /**
         *  Derived classes should override these function to perform work, if
         *  needed, and to return true if the work was done successfully.
         */

        virtual bool on_group_learn (bool /* learning */)
        {
            return false;
        }

        virtual bool on_group_learn_complete
        (
            const keystroke & /* k */, bool /* good */
        )
        {
            return false;
        }

        virtual bool on_mutes_change (mutegroup::number /* group */)
        {
            return false;
        }

        virtual bool on_set_change (screenset::number, performer::change)
        {
            return false;
        }

        virtual bool on_sequence_change (seq::number, bool /* recreate */)
        {
            return false;
        }

        virtual bool on_automation_change (automation::slot /* s */)
        {
            return false;
        }

        virtual bool on_ui_change (seq::number /* seqno */)
        {
            return false;
        }

        virtual bool on_trigger_change (seq::number /* seqno */)
        {
            return false;
        }

        virtual bool on_resolution_change (int /* ppqn */, midibpm /* bpm */)
        {
            return false;
        }

        virtual bool on_song_action (bool /* sig */, playlist::action /* a */)
        {
            return false;
        }

        performer & cb_perf ()
        {
            return m_performer;
        }

        const performer & cb_perf () const
        {
            return m_performer;
        }

    };          // class callbacks

public:

    /**
     *  Provides a type to hold the unique shift-selected sequence numbers.
     *  Although this can be considered a GUI function, it makes sense to
     *  let performer manage it and encapsulate it.
     */

    using selection = std::set<int>;

    /**
     *  Provides a function type that can be applied to each sequence number
     *  in a selection.  Generally, the caller will bind a member function to
     *  use in operate_on_set().  The first parameter is a sequence number
     *  (obtained from the selection).  The caller can bind additional
     *  placeholders or parameters, if desired.
     *
     *  See the old seq64 perfroll module.
     */

    using SeqOperation = std::function<void(int)>;

private:

    /**
     *  Defines a pointer to a member automation function.
     */

    using automation_function = bool (performer::*)
    (
        automation::action a, int d0, int d1, bool inverse
    );

    /**
     *  Provides a type alias useful in creating a function table to make the
     *  loading of member op/slot functions much easier.
     */

    using automation_pair = struct
    {
        automation::slot ap_slot;
        automation_function ap_function;
    };

    static automation_pair sm_auto_func_list [];

    /**
     *  Indicates that an internal setup error occurred (e.g. a device could
     *  not be set up in PortMidi).  In this case, we will eventually want to
     *  emit an error prompt, though we keep going in order to populate the
     *  "rc" file correctly.
     */

    bool m_error_pending;

    /**
     *  When the screenset changes, we put only the existing sequences in this
     *  vector to try to save time in the play() function.  This "play-set"
     *  feature offloads the performer::play() work to a special short vector
     *  only active sequences.  We're desperately trying to reduce the CPU
     *  usage of this program when playing.  Without being connected to a
     *  synthesizer, playing the "b4uacuse" MIDI file in Live mode, with no
     *  pattern armed, the program eats up one whole CPU on an i7.  Setting
     *  this macro cuts that roughly in half... except when a pattern is
     *  armed.
     */

    playset m_play_set;

    /**
     *  Provides an optional play-list, loosely patterned after Stazed's Seq32
     *  play-list. Important: This object is now owned by perform.
     */

    std::unique_ptr<playlist> m_play_list;

    /**
     *  Provides an optional note-mapper or drum-mapper, read from a ".drums"
     *  file.
     */

    std::unique_ptr<notemapper> m_note_mapper;

    /**
     *  If true, playback is done in Song mode, not Live mode.  This option is
     *  saved to and restored from the "rc" configuration file.  Sometimes
     *  called "JACK start mode", it used to be a JACK setting, but now
     *  applies to any playback.
     */

    sequence::playback m_song_start_mode;

    /**
     *  It seems that this member, if true, forces a repositioning to the left
     *  (L) tick marker.
     */

    bool m_reposition;

    /**
     *  Provides an "acceleration" factor for the fast-forward and rewind
     *  functionality.  It starts out at 1.0, and can range up to 60.0, being
     *  multiplied by 1.1 by the FF/RW timeout function.
     */

    float m_excell_FF_RW;

    /**
     *  Indicates whether the fast-forward or rewind key is in effect in the
     *  perfedit window.  It has values of rewind, none, or forward.  This was a
     *  free (global in a namespace) int in perfedit.
     */

    ff_rw m_FF_RW_button_type;

private:

    /**
     *  From the liveframe/grid classes, these values make performer the boss
     *  of pattern cut-and-paste.
     */

    seq::number m_old_seqno;
    seq::number m_current_seqno;
    sequence m_moving_seq;
    sequence m_seq_clipboard;

private:

    /**
     *  Set to screenset::none().
     */

    screenset::number m_screenset_to_copy;

private:                            /* key, midi, and op container section  */

    /**
     *  The list of output clocks.
     */

    clockslist m_clocks;

    /**
     *  The list of input bus statuses.
     */

    inputslist m_inputs;

    /**
     *  Provides a default-filled keycontrol container.
     */

    keycontainer m_key_controls;

    /**
     *  Provides a default-filled midicontrol container.
     */

    midicontrolin m_midi_control_in;

    /**
     *  Specifies a single buss that is allowed to apply MIDI control to the
     *  application.  If set to c_bussbyte_max, any buss can control the
     *  application.
     */

    bussbyte m_midi_control_buss;

    /**
     *  Provides the class encapsulating MIDI control output.
     */

    midicontrolout m_midi_control_out;

    /**
     *  Provides a default-filled mutegroups container.  It is a copy of the
     *  data read into the global rcsettings object.
     */

    mutegroups m_mute_groups;

    /**
     *  Holds a map of midioperation functors to be used to control patterns,
     *  mute-groups, and automation functions.
     */

    opcontainer m_operations;

    /**
     *  Pulls out the set-specific manipulations needed by the qsetmaster
     *  user-interface class.  These are moved out of setmapper for increased
     *  clarity.  The performer uses some of its functions directly, while the
     *  setmapper can iterate over the container of sets in the set-master.
     */

    setmaster m_set_master;

    /**
     *  Manages extra sequence items formerly in separate arrays.
     */

    setmapper m_set_mapper;

    /**
     *  A value not equal to -1 (it ranges from 0 to 31) indicates we're now
     *  using the saved screen-set state to control the queue-replace
     *  (queue-solo) status of sequence toggling.  This value is set to -1
     *  when queue mode is exited.  See the m_no_queued_solo value.
     */

    int m_queued_replace_slot;

    /**
     *  This value is used to indicated that the queued-replace (queued-solo)
     *  feature is reset and not in force.
     */

    const int m_no_queued_solo;

    /**
     *  Holds the global MIDI transposition value.
     */

    int m_transpose;

    /**
     *  Provides information for managing threads.  Provides a "handle" to
     *  the output thread.
     */

    std::thread m_out_thread;

    /**
     *  Provides a "handle" to the input thread.
     */

    std::thread m_in_thread;

    /**
     *  Indicates that the output thread has been started.
     */

    bool m_out_thread_launched;

    /**
     *  Indicates that the input thread has been started.
     */

    bool m_in_thread_launched;

    /**
     *  Indicates merely that the input and output thread functions can keep
     *  running.  Replaces m_inputing and m_outputing.
     */

    std::atomic<bool> m_io_active;

    /**
     *  Indicates that playback is running.  However, this flag is conflated
     *  with some JACK support, and we have to supplement it with another
     *  flag, m_is_pattern_playing.
     */

    bool m_is_running;

    /**
     *  Indicates that a pattern is playing.  It replaces rc_settings ::
     *  is_pattern_playing(), which is gone, since the performer is now
     *  visible to all classes that care about it.
     */

    bool m_is_pattern_playing;

    /**
     *  Also, there are circumstance where client GUIs need to update, such as
     *  when File / New is selected.
     *
     *  Perhaps this needs to be a counter?
     */

    mutable bool m_needs_update;

    /**
     *  Indicates to belay updates during critical work.
     */

    bool m_is_busy;

    /**
     *  Indicates that status of the "loop" button in the performance editor.
     *  If true, the performance will loop between the L and R markers in the
     *  performance editor.
     */

    bool m_looping;

    /**
     *  Indicates to record live sequence-trigger changes into the Song data.
     */

    bool m_song_recording;

    /**
     *  Snap recorded playback changes to the sequence length.
     */

    bool m_song_record_snap;

    /**
     *  Indicates to resume notes if the sequence is toggled after a Note On.
     *  Note that this setting is settable in the user interface, and is a
     *  usrsettings value.
     */

    bool m_resume_note_ons;

    /**
     *  Holds the current PPQN for usage in various actions.  If 0 is the
     *  value, then m_file_ppqn will be used.
     */

    int m_ppqn;

    /**
     *  Holds the current PPQN from a MIDI file that has been read.  It might
     *  be 0.
     */

    int m_file_ppqn;

    /**
     *  Holds the current BPM (beats per minute) for later usage.
     */

    midibpm m_bpm;

    /**
     *  Indicates if the BPM or PPQN value has changed, for internal handling in
     *  output_func().
     */

    std::atomic<bool> m_resolution_change;

    /**
     *  Indicates the number of beats considered in calculating the BPM via
     *  button tapping.  This value is displayed in the button.
     */

    int m_current_beats;

    /**
     *  Indicates the first time the tap button was ... tapped.
     */

    long m_base_time_ms;

    /**
     *  Indicates the last time the tap button was tapped.  If this button
     *  wasn't tapped for awhile, we assume the user has been satisfied with
     *  the tempo he/she tapped out.
     */

    long m_last_time_ms;

    /**
     *  Holds the beats/bar value as obtained from the MIDI file.
     *  The default value is 4. See usrsettings.
     */

    int m_beats_per_bar;

    /**
     *  Holds the beat width value as obtained from the MIDI file.
     *  The default value is 4.  See usrsettings.
     */

    int m_beat_width;

    /**
     *  Holds the number of the official tempo track for this performance.
     *  Normally 0, it can be changed to any value from 1 to 1023 via the
     *  tempo-track-number setting in the "rc" file, and that can be overriden
     *  by the c_tempo_track SeqSpec possibly present in the song's MIDI file.

    seq::number m_tempo_track_number;
     */

    /**
     *  Augments the beats/bar and beat-width with the additional values
     *  included in a Time Signature meta event.  This value provides the
     *  number of MIDI clocks between metronome clicks.  The default value of
     *  this item is 24.  It can also be read from some SMF 1 files, such as
     *  our hymne.mid example.
     */

    int m_clocks_per_metronome;

    /**
     *  Augments the beats/bar and beat-width with the additional values
     *  included in a Time Signature meta event.  Useful in export.  A
     *  duplicate of the same member in the sequence class.
     */

    int m_32nds_per_quarter;

    /**
     *  Augments the beats/bar and beat-width with the additional values
     *  included in a Tempo meta event.  Useful in export.  A duplicate of the
     *  same member in the sequence class.
     */

    long m_us_per_quarter_note;

    /**
     *  Provides our MIDI buss.  We changed this item to a pointer so that we
     *  can delay the creation of this object until after all settings have
     *  been read.  Use a smart pointer!
     */

    std::shared_ptr<mastermidibus> m_master_bus;

    /**
     *  Provides storage for this "rc" configuration option so that the
     *  performer can set it in the master buss once that has been
     *  created.
     */

    bool m_filter_by_channel;

    /**
     *  Holds the "one measure's worth" of pulses (ticks), which is normally
     *  m_ppqn * 4.  We can save some multiplications, and, more importantly,
     *  later define a more flexible definition of "one measure's worth" than
     *  simply four quarter notes.
     */

    midipulse m_one_measure;

    /**
     *  Holds the position of the left (L) marker, and it is first defined as
     *  0.  Note that "tick" is actually "pulses".
     */

    midipulse m_left_tick;

    /**
     *  Holds the position of the right (R) marker, and it is first defined as
     *  the end of the fourth measure.  Note that "tick" is actually "pulses".
     */

    midipulse m_right_tick;

    /**
     *  Holds the starting tick for playing.  By default, this value is always
     *  reset to the value of the "left tick".  We want to eventually be able
     *  to leave it at the last playing tick, to support a "pause"
     *  functionality. Note that "tick" is actually "pulses".
     */

    midipulse m_start_tick;

    /**
     *  MIDI Clock support.  The m_tick member holds the tick to be used in
     *  displaying the progress bars and the maintime pill.  It is mutable
     *  because sometimes we want to adjust it in a const function for pause
     *  functionality.
     */

    mutable midipulse m_tick;

    /**
     *  Holds a bunch of jack_assistant settings.
     */

    jack_scratchpad m_jack_pad;

    /**
     *  Let's try to save the last JACK pad structure tick for re-use with
     *  resume after pausing.
     */

    midipulse m_jack_tick;

    /**
     *  More MIDI clock support.
     */

    bool m_usemidiclock;

    /**
     *  More MIDI clock support.  Indicates if the MIDI clock is stopped or
     *  started.
     */

    bool m_midiclockrunning;

    /**
     *  More MIDI clock support.
     */

    int m_midiclocktick;

    /**
     *  We need to adjust the clock increment for the PPQN that is in force.
     *  Higher PPQN need a longer increment than 8 in order to get 24 clocks
     *  per quarter note.
     */

    int m_midiclockincrement;

    /**
     *  More MIDI clock support.
     */

    int m_midiclockpos;

    /**
     *  Support for pause, which does not reset the "last tick" when playback
     *  stops/starts.  All this member is used for is keeping the last tick
     *  from being reset.
     */

    bool m_dont_reset_ticks;

    /**
     *  It may be a good idea to eventually centralize all of the dirtiness of
     *  a performance here.  All the GUIs use a performer.
     */

    bool m_is_modified;

    /**
     *  Provides a set holding all of the sequences numbers that have been
     *  shift-selected.  If we ever enable box-selection, this container will
     *  support that as well.
     */

    selection m_selected_seqs;

    /**
     *  A condition variable to protect playback.  It is signalled if playback
     *  has been started.  The output thread function waits on this variable
     *  until m_is_running and m_io_active are false.  This variable is also
     *  signalled in the performer destructor.
     */

    condition m_condition_var;

#if defined SEQ66_JACK_SUPPORT

    /**
     *  A wrapper object for the JACK support of this application.  It
     *  implements most of the JACK stuff.  Not used on Windows (we use
     *  PortMidi instead).
     */

    jack_assistant m_jack_asst;

#endif

    /*
     * Not sure that we need this code; we'll think about it some more.  One
     * issue with it is that we really can't keep good track of the modify
     * flag in this case, in general.
     *
     * Used for undo track modification support.
     * Is is worth creating an "undo" class????
     */

    bool m_have_undo;

    /**
     *  Holds the "track" numbers or the "all tracks" values for undo
     *  operations.  See the push_trigger_undo() function.
     */

    std::vector<seq::number> m_undo_vect;

    /**
     * Used for redo track modification support.
     */

    bool m_have_redo;

    /**
     *  Holds the "track" numbers or the "all tracks" values for redo
     *  operations.  See the pop_trigger_undo() function.
     */

    std::vector<seq::number> m_redo_vect;

    /**
     *  Can register here for events.  Used in mainwnd and perform.
     *  Now wrapped in the enregister() function, so no longer public.
     *
     *  Actually, currently the main window in Qt relies on checking the learn
     *  status in a timer.  We will rethink this eventually.
     */

    callbacks::clients m_notify;

    /**
     *  If true, indicate certain events, like song-changes, occur via a
     *  signal.  In a headless run, there's no conflict with Qt's threads, but
     *  when Qt is running, hoo boy!
     */

    bool m_signalled_changes;

    /**
     *  Set to true if automation_edit_pending() is called.  It is reset by the
     *  caller as a side-effect.  The usual (but configurable) keystroke for
     *  this function is "=". In Sequencer64 this was m_call_seq_edit.
     */

    mutable bool m_seq_edit_pending;

    /**
     *  Set to true if automation_event_pending() is called.  It is reset by the
     *  caller as a side-effect.  The usual (but configurable) keystroke for
     *  this function is "-". In Sequencer64 this was m_call_seq_eventedit.
     */

    mutable bool m_event_edit_pending;

    /**
     *  Holds the loop number in the case of using the edit keys.  It is
     *  reset when the slot-shift key is struck. In Sequencer64 this was
     *  m_call_seq_number.
     */

    mutable seq::number m_pending_loop;

    /**
     *  Incremented when automation_slot_shift() is called.  It is reset by the
     *  caller once the keystroke is handled.  It is used for toggling patterns
     *  from 32 to 63 and 64 to 95. The usual (but configurable) keystroke for
     *  this function is "/". In Sequencer64 this was m_call_seq_shift.
     */

    mutable int m_slot_shift;

    /**
     *  Indicates if the graphical user-interface is visible.  Currently
     *  applies only to the main window.  This item can be toggled by the
     *  automation::visibility automation control or by the (Non) session
     *  manager.
     */

    std::atomic<bool> m_hidden;
    std::atomic<bool> m_show_hide_pending;

public:

    performer () = delete;
    performer (int ppqn, int rows, int columns);
    ~performer ();
    performer (const performer &) = delete;
    performer & operator = (const performer &) = delete;

    void enregister (callbacks * pfcb);             /* for notifications    */
    void unregister (callbacks * pfcb);
    void notify_set_change (screenset::number setno, change mod = change::yes);
    void notify_mutes_change (screenset::number setno, change mod = change::yes);
    void notify_sequence_change (seq::number seqno, change mod = change::yes);
    void notify_ui_change (seq::number seqno, change mod = change::yes);
    void notify_trigger_change (seq::number seqno, change mod = change::yes);
    void notify_resolution_change
    (
        int ppqn, midibpm bpm, change mod = change::yes
    );
    void notify_song_action
    (
        bool signalit = true,
        playlist::action act = playlist::action::none
    );

    bool error_pending () const
    {
        return m_error_pending;
    }

    bool modified () const
    {
        return m_is_modified;
    }

    /**
     * \setter m_is_modified
     *      This setter only sets the modified-flag to true.
     *      The setter that can falsify it, unmodify(), is private.  No one
     *      but performer and its friends should falsify this flag.
     */

    void modify ()
    {
        m_is_modified = true;
        m_needs_update = true;              /* kind of iffy */
    }

    bool get_settings (const rcsettings & rcs, const usrsettings & usrs);
    bool put_settings (rcsettings & rcs, usrsettings & usrs);

    std::string sets_to_string () const
    {
        return master().sets_to_string();
    }

    void show_patterns () const
    {
        master().show();
    }

    bool read_midi_file
    (
        const std::string & fn,
        std::string & errmsg,
        bool addtorecent = true
    );

    bool notemap_exists () const
    {
        return bool(m_note_mapper);
    }

    const playset & play_set () const
    {
        return m_play_set;
    }

    playset & play_set ()
    {
        return m_play_set;
    }

    /*
     * Start of playlist accessors.  Playlist functionality.  Note that we
     * ensure that a playlist object exists, even if empty.  Saves a lot of
     * pointer checks.
     */

    int playlist_count () const
    {
        return m_play_list->list_count();
    }

    int song_count () const
    {
        return m_play_list->song_count();
    }

    bool playlist_reset (int listindex = 0)
    {
        return m_play_list->reset_list(listindex);
    }

    bool open_note_mapper (const std::string & notefile);
    bool save_note_mapper (const std::string & notefile = "");
    bool open_mutegroups (const std::string & mfg);
    bool save_mutegroups (const std::string & mfg = "");
    bool open_playlist (const std::string & pl, bool show_on_stdout = false);
    bool save_playlist (const std::string & pl = "");

    bool remove_playlist ()
    {
        return m_play_list->reset_list(true);
    }

    void playlist_show ()
    {
        m_play_list->show();
    }

    void playlist_test ()
    {
        m_play_list->test();
    }

    std::string playlist_filename () const
    {
        return m_play_list->file_name();
    }

    void playlist_filename (const std::string & name);

    std::string playlist_midi_base () const
    {
        return m_play_list->midi_base_directory();
    }

    int playlist_midi_number () const
    {
        return m_play_list->list_midi_number();
    }

    std::string playlist_name () const
    {
        return m_play_list->list_name();
    }

    bool playlist_active () const
    {
        return m_play_list->active();
    }

    bool playlist_mode () const
    {
        return m_play_list->mode();
    }

    void playlist_mode (bool on)
    {
        m_play_list->mode(on);
    }

    const std::string & playlist_error_message () const
    {
        return m_play_list->error_message();
    }

    std::string file_directory () const
    {
        return m_play_list->file_directory();
    }

    std::string song_directory () const
    {
        return m_play_list->song_directory();
    }

    bool is_own_song_directory () const
    {
        return m_play_list->is_own_song_directory();
    }

    std::string song_filename () const
    {
        return m_play_list->song_filename();
    }

    std::string song_filepath () const
    {
        return m_play_list->song_filepath();
    }

    int song_midi_number () const
    {
        return m_play_list->song_midi_number();
    }

    std::string playlist_song () const
    {
        return m_play_list->current_song();
    }

    std::string playlist_song_basename () const;

    bool open_current_song ()
    {
        return m_play_list->open_current_song();
    }

    bool open_select_list_by_index (int index, bool opensong = true)
    {
        return m_play_list->open_select_list(index, opensong);
    }

    bool open_select_list_by_midi (int ctrl, bool opensong = true)
    {
        return m_play_list->select_list_by_midi(ctrl, opensong);
    }

    bool add_list
    (
        int index, int midinumber,
        const std::string & name,
        const std::string & directory
    )
    {
        return m_play_list->add_list(index, midinumber, name, directory);
    }

    bool modify_list
    (
        int index, int midinumber,
        const std::string & name,
        const std::string & directory
    )
    {
        return m_play_list->modify_list(index, midinumber, name, directory);
    }

    bool remove_list (int index)
    {
        return m_play_list->remove_list(index);
    }

    bool add_song
    (
        int index, int midinumber,
        const std::string & name,
        const std::string & directory
    )
    {
        return m_play_list->add_song(index, midinumber, name, directory);
    }

    bool add_song (const std::string & fullpath)
    {
        return m_play_list->add_song(fullpath);
    }

    bool modify_song
    (
        int index, int midinumber,
        const std::string & name,
        const std::string & directory
    )
    {
        return m_play_list->modify_song(index, midinumber, name, directory);
    }

    bool remove_song_by_index (int index)
    {
        return m_play_list->remove_song(index);
    }

    bool open_next_list (bool opensong = true, bool loading = false);
    bool open_previous_list (bool opensong = true);
    bool open_select_song_by_index (int index, bool opensong = true);
    bool open_select_song_by_midi (int ctrl, bool opensong = true);
    bool open_next_song (bool opensong = true);
    bool open_previous_song (bool opensong = true);

    /*
     * End of playlist accessors.
     */

public:

    bool repitch_all (const std::string & nmapfile, sequence & s);
    bool repitch_selected (const std::string & nmapfile, sequence & s);

    setmapper & mapper ()
    {
        return m_set_mapper;
    }

    const setmapper & mapper () const
    {
        return m_set_mapper;
    }

    setmaster & master ()
    {
        return m_set_master;
    }

    const setmaster & master () const
    {
        return m_set_master;
    }

    int screenset_count () const
    {
        return master().screenset_count();
    }

    int highest_set () const
    {
        return master().highest_set();
    }

    int screenset_max () const
    {
        return master().screenset_max();
    }

    int screenset_index (screenset::number setno) const
    {
        return master().screenset_index(setno);
    }

    int screenset_size () const
    {
        return mapper().screenset_size();
    }

    int sequences_in_sets () const
    {
        return mapper().sequences_in_sets();
    }

    int ppqn () const
    {
        return m_ppqn == c_use_file_ppqn ? m_file_ppqn : m_ppqn ;
    }

    void file_ppqn (int p)
    {
        m_file_ppqn = p;
    }

    /**
     *  Only a nominal value.  The mastermidibus could be considered the true
     *  value of BPM (and PPQN).
     */

    midibpm bpm () const
    {
        return m_bpm;                   /* only a nominal value */
    }

    int rows () const
    {
        return mapper().rows();
    }

    int columns () const
    {
        return mapper().columns();
    }

    int seqs_in_set () const
    {
        return rows() * columns();
    }

    int mute_rows () const
    {
        return mutes().rows();
    }

    int mute_columns () const
    {
        return mutes().columns();
    }

    screenset::number master_calculate_set (int row, int column) const
    {
        return master().calculate_set(row, column);
    }

    bool master_calculate_coordinates
    (
        screenset::number setno, int & row, int & column
    )
    {
        return master().calculate_coordinates(setno, row, column);
    }

    bool master_inside_set (int row, int column) const
    {
        return master().inside_set(row, column);
    }

    seq::number grid_to_seq (int row, int column) const
    {
        return mapper().grid_to_seq(row, column);
    }

    bool seq_to_grid (seq::number seqno, int & row, int & column) const
    {
        return mapper().seq_to_grid(seqno, row, column);
    }

    /**
     *  It is better to call this getter before bothering to even try to use a
     *  sequence.  In many cases at startup, or when loading a file, there are
     *  no sequences yet, and still the code calls functions that try to access
     *  them.
     */

    int sequence_count () const
    {
        return mapper().sequence_count();
    }

    seq::number sequence_high () const
    {
        return mapper().sequence_high();
    }

    seq::number sequence_max () const
    {
        return mapper().sequence_max();
    }

    int get_beats_per_bar () const
    {
        return m_beats_per_bar;
    }

    void set_beats_per_bar (int bpm)
    {
        m_beats_per_bar = bpm;
#if defined SEQ66_JACK_SUPPORT
        m_jack_asst.set_beats_per_measure(bpm);
#endif
    }

    bool set_beats_per_measure (int bpm);

    int get_beat_width () const
    {
        return m_beat_width;
    }

    void set_beat_length (int bl)
    {
        m_beat_width = bl;
#if defined SEQ66_JACK_SUPPORT
        m_jack_asst.set_beat_width(bl);
#endif
    }

    bool set_beat_width (int bw);

    /**
     *
    seq::number tempo_track_number () const
    {
        return m_tempo_track_number;
    }

     * \setter m_tempo_track_number
     *
     * \param tempotrack
     *      Provides the value for beat-width.  Also used to set the
     *      beat-width in the JACK assistant object.

    void tempo_track_number (seq::number tempotrack)
    {
        if (tempotrack >= 0 && tempotrack < sequence_max())
            m_tempo_track_number = tempotrack;
    }
     */

    void clocks_per_metronome (int cpm)
    {
        m_clocks_per_metronome = cpm;
    }

    int clocks_per_metronome () const
    {
        return m_clocks_per_metronome;
    }

    void set_32nds_per_quarter (int tpq)
    {
        m_32nds_per_quarter = tpq;
    }

    int get_32nds_per_quarter () const
    {
        return m_32nds_per_quarter;
    }

    void us_per_quarter_note (long upqn)
    {
        m_us_per_quarter_note = upqn;
    }

    long us_per_quarter_note () const
    {
        return m_us_per_quarter_note;
    }

    mastermidibus * master_bus ()
    {
        return m_master_bus.get();
    }

    const mastermidibus * master_bus () const
    {
        return m_master_bus.get();
    }

    std::string client_id_string () const;

    int client_id () const
    {
        return m_master_bus->client_id();
    }

    void filter_by_channel (bool flag)
    {
        m_filter_by_channel = flag;
        if (m_master_bus)
            m_master_bus->filter_by_channel(flag);
    }

    bool is_running () const            /* is_playing()!!!                  */
    {
        return m_is_running;
    }

    bool is_pattern_playing () const
    {
        return m_is_pattern_playing;
    }

    void is_pattern_playing (bool flag)
    {
        m_is_pattern_playing = flag;
    }

    bool done () const
    {
        return ! m_io_active;
    }

    /*
     * ---------------------------------------------------------------------
     *  JACK Transport
     * ---------------------------------------------------------------------
     */

    jack_scratchpad & pad ()
    {
        return m_jack_pad;
    }

#if defined SEQ66_JACK_SUPPORT
    bool jack_output (jack_scratchpad & pad)
    {
        return m_jack_asst.output(pad);
    }
#else
    bool jack_output (jack_scratchpad & /*pad*/)
    {
        return false;
    }
#endif

    /**
     * \getter m_jack_asst.is_running()
     *      This function is useful for announcing the status of JACK in
     *      user-interface items that only have access to the performer.
     */

    bool is_jack_running () const
    {
#if defined SEQ66_JACK_SUPPORT
        return m_jack_asst.is_running();
#else
        return false;
#endif
    }

    /**
     *  Also now includes is_jack_running(), since one cannot be JACK Master
     *  if JACK is not running.
     */

    bool is_jack_master () const
    {
#if defined SEQ66_JACK_SUPPORT
        return m_jack_asst.is_running() && m_jack_asst.is_master();
#else
        return false;
#endif
    }

    bool is_jack_slave () const
    {
#if defined SEQ66_JACK_SUPPORT
        return m_jack_asst.is_running() && m_jack_asst.is_slave();
#else
        return false;
#endif
    }

    bool no_jack_transport () const
    {
#if defined SEQ66_JACK_SUPPORT
        return ! m_jack_asst.is_running() || m_jack_asst.no_transport();
#else
        return true;
#endif
    }

    bool jack_transport_not_starting () const
    {
#if defined SEQ66_JACK_SUPPORT
        return ! is_jack_running() || m_jack_asst.transport_not_starting();
#else
        return true;
#endif
    }

    /**
     *  If JACK is supported, starts the JACK transport.
     */

    void start_jack ()
    {
#if defined SEQ66_JACK_SUPPORT
        m_jack_asst.start();
#endif
    }

    void stop_jack ()
    {
#if defined SEQ66_JACK_SUPPORT
        m_jack_asst.stop();
#endif
    }

    /**
     *  Initializes JACK support, if defined.  The launch() function and
     *  options module (when Connect is pressed) call this.
     *
     * \return
     *      running, false if not or if JACK support is note defined.
     */

    bool init_jack_transport ()
    {
#if defined SEQ66_JACK_SUPPORT
        return m_jack_asst.init();
#else
        return false;
#endif
    }

    /**
     *  Tears down the JACK infrastructure.  Called by launch() and the
     *  options module (when Disconnect is pressed).  This function operates
     *  only while we are not outputing, otherwise we have a race condition
     *  that can lead to a crash.
     *
     * \return
     *      Returns the result of the init() call; true if JACK sync is now
     *      no longer running or JACK is not supported.
     */

    bool deinit_jack_transport ()
    {
#if defined SEQ66_JACK_SUPPORT
        return m_jack_asst.deinit();
#else
        return true;
#endif
    }

#if defined SEQ66_JACK_SUPPORT
    void position_jack (bool songmode, midipulse tick)
    {
        m_jack_asst.position(songmode, tick);
    }
#else
    void position_jack (bool, midipulse) { /* no code */ }
#endif

    bool set_jack_mode (bool connect);

    void toggle_jack_mode ()
    {
#if defined SEQ66_JACK_SUPPORT
        m_jack_asst.toggle_jack_mode();
#endif
    }

    bool get_jack_mode () const
    {
#if defined SEQ66_JACK_SUPPORT
        return m_jack_asst.get_jack_mode();
#else
        return false;
#endif
    }

    midipulse jack_stop_tick () const
    {
#if defined SEQ66_JACK_SUPPORT
        return m_jack_asst.jack_stop_tick();
#else
        return 0;
#endif
    }

    bool jack_set_beats_per_minute (midibpm bpm);

    bool jack_set_ppqn (int p)
    {
#if defined SEQ66_JACK_SUPPORT
        m_jack_asst.set_ppqn(p);
        return true;
#else
        return p > 0;
#endif
    }

#if defined SEQ66_JACK_SUPPORT
    void jack_stop_tick (midipulse tick)
    {
        m_jack_asst.jack_stop_tick(tick);
    }
#else
    void jack_stop_tick (midipulse)
    {
        /* no code needed */
    }
#endif

    midipulse get_jack_tick () const
    {
        return m_jack_tick;
    }

    void set_jack_tick (midipulse tick)
    {
        m_jack_tick = tick;             /* current JACK tick/pulse value    */
    }

#if defined SEQ66_JACK_SUPPORT
    void set_follow_transport (bool flag)
    {
        m_jack_asst.set_follow_transport(flag);
    }
#else
    void set_follow_transport (bool)
    {
        /* no code needed */
    }
#endif

    bool get_follow_transport () const
    {
#if defined SEQ66_JACK_SUPPORT
        return m_jack_asst.get_follow_transport();
#else
        return false;
#endif
    }

    void toggle_follow_transport ()
    {
#if defined SEQ66_JACK_SUPPORT
        m_jack_asst.toggle_follow_transport();
#endif
    }

    /**
     *  Convenience function for following progress in seqedit.
     */

    bool follow_progress () const
    {
#if defined SEQ66_JACK_SUPPORT
        return m_is_running && m_jack_asst.get_follow_transport();
#else
        return m_is_running;
#endif
    }

    /*
     * ---------------------------------------------------------------------
     *  Song versus Live mode
     * ---------------------------------------------------------------------
     */

    bool jackless_song_mode () const
    {
        return song_mode() && ! is_jack_running();
    }

    sequence::playback toggle_song_start_mode ();

    bool song_mode (sequence::playback p) const
    {
        return p == sequence::playback::song;
    }

    bool live_mode () const
    {
        return m_song_start_mode == sequence::playback::live;
    }

    bool song_mode () const
    {
        return m_song_start_mode == sequence::playback::song;
    }

    bool live_mode (sequence::playback p) const
    {
        return p == sequence::playback::live;
    }

    void song_start_mode (sequence::playback p)
    {
        m_song_start_mode = p;
    }

    /**
     *  Note that there are a lot of existing boolean comparisons, which now
     *  use the song_mode() function.  A little confusing.
     */

    sequence::playback song_start_mode () const
    {
        return m_song_start_mode;
    }

    void next_song_mode ();

    void song_mode (bool flag)
    {
        m_song_start_mode = flag ?
            sequence::playback::song : sequence::playback::live ;
    }

    bool toggle_song_mode ()
    {
        return toggle_song_start_mode() == sequence::playback::song;
    }

    void FF_rewind ();
    bool FF_RW_timeout ();          /* called by free-function of same name */
    void jack_reposition (midipulse tick, midipulse stoptick);

    void set_reposition (bool postype)
    {
        m_reposition = postype;
    }

    ff_rw ff_rw_type ()
    {
        return m_FF_RW_button_type;
    }

    void ff_rw_type (ff_rw button_type)
    {
        m_FF_RW_button_type = button_type;
    }

    /**
     *  Sets the rewind status.
     *
     * \param press
     *      If true, the status is set to FF_RW_REWIND, otherwise it is set to
     *      FF_RW_NONE.
     */

    void rewind (bool press)
    {
        ff_rw_type(press ? ff_rw::rewind : ff_rw::none);
    }

    /**
     *  Sets the fast-forward status.
     *
     * \param press
     *      If true, the status is set to ff_rw::forward, otherwise it is set
     *      to ff_rw::none.
     */

    void fast_forward (bool press)
    {
        ff_rw_type(press ? ff_rw::forward : ff_rw::none);
    }

    void reposition (midipulse tick);

public:

    bool set_midi_bus (seq::number seqno, int buss);
    bool set_midi_channel (seq::number seqno, int channel);
    bool set_sequence_name (seq::pointer s, const std::string & name);
    bool set_recording (seq::pointer s, bool active, bool toggle);
    bool set_quantized_recording (seq::pointer s, bool active, bool toggle);
    bool set_overwrite_recording (seq::pointer s, bool active, bool toggle);
    bool set_thru (seq::pointer s, bool active, bool toggle);
    bool selected_trigger
    (
        seq::number seqno, midipulse droptick,
        midipulse & tick0, midipulse & tick1
    );

    bool selection_operation (SeqOperation func);
    void box_insert (seq::number dropseq, midipulse droptick);
    void box_delete (seq::number dropseq, midipulse droptick);
    void box_toggle_sequence (seq::number dropseq, midipulse droptick);
    void box_unselect_sequences (seq::number dropseq);
    void box_move_triggers (midipulse tick);
    void box_offset_triggers (midipulse offset);

    bool box_selection_empty () const
    {
        return m_selected_seqs.empty();
    }

    void box_selection_clear ()
    {
        m_selected_seqs.clear();
    }

    bool clear_all (bool clearplaylist = false);
    bool clear_song ();
    bool launch (int ppqn);
    bool finish ();
    bool activate ();
    bool new_sequence (seq::number seq = seq::unassigned());
    bool remove_sequence (seq::number seq);
    bool copy_sequence (seq::number seq);
    bool cut_sequence (seq::number seq);
    bool paste_sequence (seq::number seq);
    bool merge_sequence (seq::number seq);
    bool move_sequence (seq::number seq);
    bool finish_move (seq::number seq);
    bool remove_set (screenset::number setno);
    bool swap_sets (seq::number set0, seq::number set1);

    bool can_paste () const
    {
        return m_seq_clipboard.event_count() > 0;
    }

    bool is_seq_in_edit (int seq) const
    {
        return mapper().is_seq_in_edit(seq);
    }

    /**
     *  Shows all the triggers of all the sequences.
     */

    void print_busses () const
    {
        if (m_master_bus)
            m_master_bus->print();
    }

    void delay_stop ();
    void auto_stop ();
    void auto_pause ();
    void auto_play ();
    void play_all_sets (midipulse tick);
    void play (midipulse tick);
    void all_notes_off ();

    void unqueue_sequences (int hotseq)
    {
        mapper().unqueue(hotseq);
    }

    bool panic ();                                      /* from kepler43    */
    bool visibility (automation::action a);             /* for NSM/Live use */
    void set_tick (midipulse tick, bool dontreset = false);
    void set_left_tick (midipulse tick);
    void set_left_tick_seq (midipulse tick, midipulse snap);

    /**
     *  For every pattern/sequence that is active, sets the "original tick"
     *  value for the pattern.  This is really the "last tick" value, so we
     *  renamed sequence::set_orig_tick() to sequence::set_last_tick().
     *
     * \param tick
     *      Provides the last-tick value to be set for each sequence that is
     *      active.
     */

    void set_last_ticks (midipulse tick)
    {
        mapper().set_last_ticks(tick);
    }

    midipulse get_left_tick () const
    {
        return m_left_tick;
    }

    void set_start_tick (midipulse tick)
    {
        m_start_tick = tick;         /* starting JACK tick/pulse value   */
    }

    midipulse get_start_tick () const
    {
        return m_start_tick;
    }

    void set_right_tick (midipulse tick);
    void set_right_tick_seq (midipulse tick, midipulse snap);

    midipulse get_right_tick () const
    {
        return m_right_tick;
    }

    /**
     *  Convenience function for JACK support when loop in song mode.
     *
     * \return
     *      Returns the difference between the right and left tick, cast to
     *      double.
     */

    double left_right_size () const
    {
        return double(m_right_tick - m_left_tick);
    }

public:

    /*
     * Functions to move into sequence management.
     */

    /**
     *  Checks the pattern/sequence for activity.  Uses the setmapper to get the
     *  actually screenset (internally).
     *
     * \param seq
     *      The pattern number.  It is checked for invalidity.  This can
     *      lead to "too many" (i.e. redundant) checks, but we're trying to
     *      centralize such checks in this function.
     *
     * \return
     *      Returns the value of the active-flag, or false if the sequence was
     *      invalid or null.
     */

    bool is_seq_active (seq::number seqno) const
    {
        return mapper().is_seq_active(seqno);
    }

    seq::number first_seq () const
    {
        return mapper().first_seq();
    }

public:

    void apply_song_transpose ()
    {
        mapper().apply_song_transpose();
    }

    /**
     * \setter m_transpose
     *      For sanity's sake, the values are restricted to +-64.
     */

    void set_transpose (int t)
    {
        if (t >= c_transpose_down_limit && t <= c_transpose_up_limit)
            m_transpose = t;
    }

    int get_transpose () const
    {
        return m_transpose;
    }

    /**
     * \getter m_master_bus.get_beats_per_minute
     *      Retrieves the BPM setting of the master MIDI buss.
     *
     *  This result should be the same as the value of the m_bpm member.
     *  This function returns that value in a roundabout way.
     *
     * \return
     *      Returns the value of beats/minute from the master buss.
     */

    midibpm get_beats_per_minute () const
    {
        return m_master_bus ? m_master_bus->get_beats_per_minute() : bpm() ;
    }

    int get_ppqn () const
    {
        return m_master_bus ? m_master_bus->get_ppqn() : ppqn() ;
    }

    midibpm update_tap_bpm ();
    bool tap_bpm_timeout ();

    int current_beats () const
    {
        return m_current_beats;
    }

    void clear_current_beats ()
    {
        m_current_beats = m_base_time_ms = m_last_time_ms = 0;
    }

    bool reload_mute_groups (std::string & errmessage);
    bool set_ctrl_status
    (
        automation::action a,
        automation::ctrlstatus status
    );
    bool toggle_ctrl_status (automation::ctrlstatus s);
    void display_ctrl_status (automation::ctrlstatus s, bool on);
    void unset_queued_replace (bool clearbits = true);
    bool sequence_playing_toggle (seq::number seqno);
    bool sequence_playing_change (seq::number seqno, bool on);
    void set_keep_queue (bool activate);

    bool is_keep_queue () const
    {
        return midi_control_in().is_keep_queue();
    }

    /*
     * ---------------------------------------------------------------------
     *  Pattern/track control
     * ---------------------------------------------------------------------
     */

    /**
     *  Calls sequence_playing_change() with a value of true.
     *
     * \param seq
     *      The sequence number of the sequence to turn on.
     */

    void sequence_playing_on (seq::number seqno)
    {
        sequence_playing_change(seqno, true);
    }

    /**
     *  Calls sequence_playing_change() with a value of false.
     *
     * \param seq
     *      The sequence number of the sequence to turn off.
     */

    void sequence_playing_off (seq::number seqno)
    {
        sequence_playing_change(seqno, false);
    }

    /**
     *  Mutes/unmutes all tracks in the current set of active patterns/sequences.
     *  Covers tracks from 0 to m_sequence_max.
     *
     *  We have to also set the sequence's playing status, in opposition to the
     *  mute status, in order to see the sequence status change on the
     *  user-interface.   HMMMMMM.
     *
     * \param flag
     *      If true (the default), the song-mutes of the sequences are turned
     *      on.  Otherwise, they are turned off.
     */

    void mute_all_tracks (bool flag = true)
    {
        mapper().mute_all_tracks(flag);
    }

    /**
     *  Toggles the mutes status of all tracks in the current set of active
     *  patterns/sequences.  Covers tracks from 0 to m_sequence_max.
     *
     *  Note that toggle_playing() now has two default parameters used by the
     *  new song-recording feature, which are currently not used here.
     */

    void toggle_all_tracks ()
    {
        mapper().toggle();
    }

    void set_song_mute (mutegroups::action op);
    void mute_screenset (int ss, bool flag = true);

    /**
     *  Toggles the mutes status of all playing (currently unmuted) tracks in
     *  the current set of active patterns/sequences on all screen-sets.
     *
     *  Note that this function operates only in Live mode; it is too confusing
     *  to use in Song mode.  Do we need to call performer ::
     *  sequence_playing_toggle() for all tracks instead, to enable recording
     *  of those kinds of song performance changes?
     */

    void toggle_playing_tracks ()
    {
        if (! song_mode())
            mapper().toggle_playing_tracks();
    }

    bool any_group_unmutes () const
    {
        return mutes().any();
    }

    bool install_sequence
    (
        sequence * seq, seq::number seqno, bool fileload = false
    );
    void inner_start ();
    void inner_stop (bool midiclock = false);

    /**
     *  If JACK is not running, call inner_start() with the given state.
     *
     * \question
     *      Should we also call song_start_mode(songmode) here?
     *
     * \param songmode
     *      If true, playback is to be in Song mode.  Otherwise, it is to be
     *      in Live mode.
     */

    void start ()
    {
        if (! is_jack_running())
            inner_start();
    }

    /**
     *  If JACK is not running, call inner_stop().
     */

    void stop ()
    {
        if (! is_jack_running())
            inner_stop();
    }

    int clamp_track (int track) const;
    int clamp_group (int group) const;
    void save_playing_state ();
    void restore_playing_state ();

    void save_queued (int repseq)
    {
        mapper().save_queued(repseq);
    }

public:

    void start_playing ();
    void pause_playing ();
    void stop_playing ();
    void group_learn (bool flag);
    void group_learn_complete (const keystroke & k, bool good = true);
    bool needs_update (seq::number seqno = seq::all()) const;

    midipulse get_tick () const
    {
        return m_tick;
    }

    void learn_toggle ()
    {
        group_learn(! is_group_learn());
    }

    /**
     *  Does a learn-action if in group-learn mode, followed by
     *  mute_group_tracks.
     */

    void select_and_mute_group (mutegroup::number mg)
    {
        mapper().select_and_mute_group(mg);
    }

    int count_mutes (mutegroup::number group)
    {
        return mutes().armed_count(group);
    }

    midibooleans get_mutes (mutegroup::number gmute) const
    {
        return mutes().get(gmute);
    }

    midibooleans get_active_groups () const
    {
        return mutes().get_active_groups();
    }

    bool set_mutes
    (
        mutegroup::number gmute,
        const midibooleans & bits,
        bool putmutes = false
    );
    bool put_mutes ();
    bool learn_mutes (mutegroup::number group);
    bool clear_mutes ();                /* can cause a modify()             */
    bool apply_session_mutes ();
    bool apply_mutes (mutegroup::number group);
    bool unapply_mutes (mutegroup::number group);
    bool toggle_mutes (mutegroup::number group);
    bool toggle_active_mutes (mutegroup::number group);

    midibpm decrement_beats_per_minute ();
    midibpm increment_beats_per_minute ();
    midibpm page_decrement_beats_per_minute ();
    midibpm page_increment_beats_per_minute ();
    screenset::number decrement_screenset (int amount = 1);
    screenset::number increment_screenset (int amount = 1);
    bool copy_playscreen ();
    bool paste_playscreen (screenset::number destination);

    screenset::number playscreen_number () const
    {
        return mapper().playscreen_number();    // not in setmaster purview!
    }

    seq::number playscreen_offset () const
    {
        return mapper().playscreen_offset();
    }

    int playscreen_active_count () const
    {
        return mapper().playscreen_active_count();
    }

    /**
     *  True if a sequence is empty and should be highlighted.  This setting
     *  is currently a build-time option, but could be made a run-time option
     *  later.
     *
     * \param seq
     *      Provides a reference to the desired sequence.
     */

    bool highlight (const sequence & seq) const
    {
        return seq.event_count() == 0;
    }

    /**
     *  True if the sequence is an SMF 0 sequence.
     *
     * \param seq
     *      Provides a reference to the desired sequence.
     */

    bool is_smf_0 (const sequence & seq) const
    {
        return seq.is_smf_0();
    }

    /**
     *  Retrieves the actual sequence, based on the pattern / sequence / loop
     *  / track number.  This is the non-const version.  Note that it is more
     *  efficient to call this function and check the result than to call
     *  is_active() and then call this function.
     *
     *  Note that it gets the sequence / loop from the play-screen.
     *
     * \param seqno
     *      The prospective sequence number.
     *
     * \return
     *      Returns the sequence pointer if seq is valid.  Otherwise, a
     *      null pointer is returned.
     */

    seq::pointer loop (seq::number seqno)
    {
        return mapper().loop(seqno);
    }

    /**
     *  Retrieves the actual sequence. This is the const version.
     */

    const seq::pointer loop (seq::number seqno) const
    {
        return mapper().loop(seqno);
    }

    void off_sequences ()
    {
        mapper().off_sequences();
    }

    std::string sequence_label (const sequence & seq) const;
    std::string sequence_label (seq::number seqno) const;
    std::string sequence_title (const sequence & seq) const;
    std::string sequence_window_title (const sequence & seq) const;
    std::string main_window_title (const std::string & fn = "") const;
    std::string pulses_to_measure_string (midipulse tick) const;
    std::string pulses_to_time_string (midipulse tick) const;

    bool ui_set_input (bussbyte bus, bool active);
    bool ui_get_input (bussbyte bus, bool & active, std::string & n) const;
    bool ui_set_clock (bussbyte bus, e_clock clocktype);
    bool ui_get_clock (bussbyte bus, e_clock & e, std::string & n) const;

    void store_output_map ()
    {
        (void) build_output_port_map(m_clocks);
    }

    bussbyte true_output_bus (bussbyte nominalbuss) const
    {
        return seq66::true_output_bus(m_clocks, nominalbuss);
    }

    void store_input_map ()
    {
        (void) build_input_port_map(m_inputs);
    }

    bussbyte true_input_bus (bussbyte nominalbuss) const
    {
        return seq66::true_input_bus(m_inputs, nominalbuss);
    }

    /**
     *  Sets a single clock item, if in the currently existing range.
     *  Mostly meant for use by the Options / MIDI Input tab.
     */

    void set_clock (bussbyte bus, e_clock clocktype)
    {
        bus = true_output_bus(bus);
        m_clocks.set(bus, clocktype);
    }

    e_clock get_clock (bussbyte bus) const
    {
        bus = true_output_bus(bus);
        return m_clocks.get(bus);
    }

    /**
     *  Saves the input settings read from the "rc" file so that they can be
     *  passed to the mastermidibus after it is created.
     *
     * \param flag
     *      The input flag read from the "rc" file.

    void add_input (int bus, bool flag, const std::string & name)
    {
        m_inputs.add(bus flag, name);
    }
     */

    /**
     *  Sets a single input item, if in the currently existing range.
     *  Mostly meant for use by the Options / MIDI Input tab.
     */

    void set_input (bussbyte bus, bool inputing)
    {
        bus = true_input_bus(bus);
        m_inputs.set(bus, inputing);
    }

    bool get_input (bussbyte bus) const
    {
        bus = true_input_bus(bus);
        return m_inputs.get(bus);
    }

    bool is_input_system_port (bussbyte bus)
    {
        return not_nullptr(m_master_bus) ?
            m_master_bus->is_input_system_port(bus) : false ;
    }

    bool mainwnd_key_event (const keystroke & k);
    bool keyboard_control_press (unsigned key);
    bool keyboard_group_c_status_press (unsigned key);
    bool keyboard_group_c_status_release (unsigned key);
    bool keyboard_group_press (unsigned key);
    bool keyboard_group_release (unsigned key);
    bool perfroll_key_event (const keystroke & k, int drop_sequence);

    /*
     * More trigger functions.
     */

    void clear_sequence_triggers (int seq);
    void print_triggers () const;

    void move_triggers (bool direction)
    {
        mapper().move_triggers(m_left_tick, m_right_tick, direction);
    }

    void copy_triggers ()
    {
        mapper().copy_triggers(m_left_tick, m_right_tick);
    }

    void push_trigger_undo (seq::number track = seq::all());
    void pop_trigger_undo ();
    void pop_trigger_redo ();

    /**
     *  Pass-along to sequence::get_trigger_state().
     *
     * \param seqno
     *      The index to the desired sequence.
     *
     * \param tick
     *      The time-location at which to get the trigger state.
     *
     * \return
     *      Returns true if the sequence indicated by \a seqno exists and it's
     *      trigger state is true.
     */

    bool get_trigger_state (seq::number seqno, midipulse tick) const
    {
        const seq::pointer s = get_sequence(seqno);
        return not_nullptr(s) ? s->get_trigger_state(tick) : false ;
    }

    void add_trigger (seq::number seqno, midipulse tick, midipulse snap);
    void delete_trigger (seq::number seqno, midipulse tick);
    void add_or_delete_trigger (seq::number seqno, midipulse tick);
    bool split_trigger
    (
        seq::number seqno,
        midipulse tick,
        trigger::splitpoint splittype
    );
    bool paste_trigger (seq::number seqno, midipulse tick);
    bool paste_or_split_trigger (seq::number seqno, midipulse tick);

#if defined USE_INTERSECT_FUNCTIONS
    bool intersect_triggers (seq::number seqno, midipulse tick);
#endif

    midipulse get_max_timestamp () const
    {
        return mapper().max_timestamp();
    }

    midipulse get_max_trigger () const
    {
        return mapper().max_trigger();
    }

    midipulse get_max_extent () const;
    std::string duration () const;

    /**
     *  Indicates that the desired sequence is active, unmuted, and has
     *  a non-zero trigger count.
     *
     * \param seq
     *      The index of the desired sequence.
     *
     * \return
     *      Returns true if the sequence has the three properties noted above.
     */

    bool is_exportable (seq::number seqno) const
    {
        return mapper().is_exportable(seqno);
    }

    /**
     *  Checks the pattern/sequence for main-dirtiness.  See the
     *  sequence::is_dirty_main() function.
     *
     * \param seq
     *      The pattern number.  It is converted to a set number and an offset
     *      into the set.  It is checked for validity?
     *
     * \return
     *      Returns the was-active-main flag value, before setting it to
     *      false.  Returns false if the pattern was invalid.
     */

    bool is_dirty_main (seq::number seqno) const
    {
        return mapper().is_dirty_main(seqno);
    }

    bool is_dirty_edit (seq::number seqno) const
    {
        return mapper().is_dirty_edit(seqno);
    }

    bool is_dirty_perf (seq::number seqno) const
    {
        return mapper().is_dirty_perf(seqno);
    }

    bool is_dirty_names (seq::number seqno) const
    {
        return mapper().is_dirty_names(seqno);
    }

    void send_onoff_event (midicontrolout::uiaction a, bool on);
    void send_play_states
    (
        midicontrolout::uiaction a,
        midicontrolout::actionindex ai = midicontrolout::action_del
    );
    void send_onoff_play_states (midicontrolout::uiaction a);
    void send_mutes_event (int group, bool on);
    void send_mutes_events (int groupon, int groupoff);
    void send_mutes_inactive (int group);
    void announce_playscreen ();
    void announce_automation (bool activate = true);
    void announce_exit (bool playstatesoff = true);
    bool announce_sequence (seq::pointer s, seq::number sn);
    void announce_mutes ();
    void set_midi_control_out ();

    const midicontrolout & midi_control_out () const
    {
        return m_midi_control_out;
    }

    midicontrolout & midi_control_out ()
    {
        return m_midi_control_out;
    }

    void set_needs_update (bool flag = true)
    {
        m_needs_update = flag;
    }

    void send_seq_event (int seqno, midicontrolout::seqaction what)
    {
        midi_control_out().send_seq_event(seqno, what);
    }

    bool slot_function (screenset::slothandler p, bool use_set_offset = true)
    {
        return mapper().slot_function(p, use_set_offset);
    }

    bool set_function (screenset::sethandler s)
    {
        return mapper().set_function(s);
    }

    bool set_function (screenset::sethandler s, screenset::slothandler p)
    {
        return mapper().set_function(s, p);
    }

    screenset::number set_playing_screenset (screenset::number setno);
    void reset_playset ();

    bool is_screenset_valid (screenset::number sset) const
    {
        return master().is_screenset_valid(sset);
    }

    bool toggle_other_seqs (seq::number seqno, bool isshiftkey);
    bool toggle_other_names (seq::number seqno, bool isshiftkey);

    /**
     *  Toggles sequences.  Useful in perfnames, taken from perfnames ::
     *  on_button_press_event() so that it can be re-used in qperfnames.
     */

    bool toggle_sequences (seq::number seqno, bool isshiftkey)
    {
        return toggle_other_names(seqno, isshiftkey);
    }

    bool are_any_armed ();

    /*
     * This is a long-standing request from user's, adapted from Kepler34.
     */

    bool song_recording () const
    {
        return m_song_recording;
    }

    bool song_record_snap () const
    {
        return m_song_record_snap;
    }

    bool resume_note_ons () const
    {
        return m_resume_note_ons;
    }

    void resume_note_ons (bool f)
    {
        m_resume_note_ons = f;
    }

    void select_triggers_in_range
    (
        seq::number seqlow, seq::number seqhigh,
        midipulse tickstart, midipulse tickfinish
    )
    {
        mapper().select_triggers_in_range
        (
            seqlow, seqhigh, tickstart, tickfinish
        );
    }

    bool select_trigger (seq::number dropseq, midipulse droptick);

    void unselect_all_triggers ()
    {
        mapper().unselect_triggers();
    }

public:

    /**
     *  A better name for get_screen_set_notepad(), adapted from Kepler34.
     *  However, we will still refer to them as "sets".
     */

    std::string bank_name (int bank) const
    {
        return get_screenset_notepad(bank);
    }

    std::string screenset_notepad (screenset::number sset) const
    {
        return mapper().name(sset);
    }

    bool looping () const
    {
        return m_looping;
    }

    void looping (bool looping)
    {
        m_looping = looping;
    }

    /**
     *  Deals with the colors used to represent specific sequences.  We don't
     *  want performer knowing the details of the palette color, just treat it
     *  as an integer.
     */

    int color (seq::number seqno) const
    {
        return mapper().color(seqno);
    }

    bool color (seq::number seqno, int c)
    {
        bool result = mapper().color(seqno, c);
        if (result)
            modify();

        return result;
    }

    bool have_undo () const
    {
        return m_have_undo;
    }

    /**
     * \setter m_have_undo
     *      Note that, if the \a undo parameter is true, then we mark the
     *      performance as modified.  Once it is set, it remains set, unless
     *      cleared by saving the file.
     */

    void set_have_undo (bool undo)
    {
        m_have_undo = undo;
        if (undo)
            modify();
    }

    bool have_redo () const
    {
        return m_have_redo;
    }

    void set_have_redo (bool redo)
    {
        m_have_redo = redo;
    }

    /**
     *  Retrieves the actual sequence, based on the pattern/sequence number.
     *  This is the const version.  Note that it is more efficient to call
     *  this function and check the result than to call is_active() and then
     *  call this function.
     *
     *  \deprecated
     *      We will replace this with loop() eventually.
     *
     * \param seq
     *      The prospective sequence number.
     *
     * \return
     *      Returns the value of "m_seqs[seq]" if seq is valid.  Otherwise, a
     *      null pointer is returned.
     */

    const seq::pointer get_sequence (seq::number seqno) const
    {
        return mapper().loop(seqno);
    }

    /**
     *  The non-const version of get_sequence().  Let the caller beware!
     *
     *  \deprecated
     *      We will replace this with loop() eventually.
     */

    seq::pointer get_sequence (seq::number seqno)
    {
        return mapper().loop(seqno);
    }

    const seq::pointer sequence_pointer (seq::number seqno) const
    {
        return mapper().loop(seqno);
    }

    seq::pointer sequence_pointer (seq::number seqno)
    {
        return mapper().loop(seqno);
    }

public:         /* GUI-support functions */

    /*
     * Deals with the editing mode of the specific sequence.
     */

    sequence::editmode edit_mode (seq::number seqno) const
    {
        const seq::pointer sp = loop(seqno);
        return sp ? sp->edit_mode() : sequence::editmode::note ;
    }

    /*
     * This overload deals with the editing mode of the specific sequence,
     * but the seqeuence ID is replaced with a reference to the sequence
     * itself.
     */

    sequence::editmode edit_mode (const sequence & s) const
    {
        return s.edit_mode();
    }

    /**
     *  A pass-along function to set the edit-mode of the given sequence.
     *  Was private, but a class can have too many friends.
     *
     * \param seq
     *      Provides the sequence number.  If the sequence is not active
     *      (available), then nothing is done.
     *
     * \param ed
     *      Provides the edit mode, which is "note" or "drum", and which
     *      determines if the duration of events matters (note) or not (drum).
     */

    void edit_mode (seq::number seqno, sequence::editmode ed)
    {
        seq::pointer sp = loop(seqno);
        if (sp)
            sp->edit_mode(ed);
    }

    void edit_mode (sequence & s, sequence::editmode ed)
    {
        s.edit_mode(ed);
    }

    /**
     *  Returns the notepad text for the current screen-set.
     */

    std::string current_screenset_notepad () const
    {
        return mapper().name();
    }

    std::string get_screenset_notepad (screenset::number sn) const
    {
        return mapper().name(sn);
    }

    void set_screenset_notepad
    (
        screenset::number sset,
        const std::string & note,
        bool is_load_modification = false
    );

    /**
     *  Tests to see if the screen-set is active.  By "active", we mean that
     *  the screen-set has at least one active pattern.
     *
     * \param screenset
     *      The number of the screen-set to check, re 0.
     *
     * \return
     *      Returns true if the screen-set has an active pattern.
     */

    bool is_screenset_active (screenset::number sset)
    {
        return mapper().is_screenset_active(sset);
    }

    /**
     *  Tests to see if the screen-set is available... does it exist?
     *
     * \param screenset
     *      The number of the screen-set to check, re 0.
     *
     * \return
     *      Returns true if the screen-set is found in the std::map container.
     */

    bool is_screenset_available (screenset::number sset)
    {
        return mapper().is_screenset_available(sset);
    }

    /**
     *  Sets the notepad text for the current screen-set.
     *
     * \param note
     *      The string value to set into the notepad text.
     */

    void set_screenset_notepad (const std::string & note)
    {
        mapper().name(note);
    }

    void song_recording_start ()
    {
        mapper().song_recording_start(pad().js_current_tick);
    }

    void song_recording_stop ()
    {
        mapper().song_recording_stop(pad().js_current_tick);
    }

    bool seq_in_playing_screen (int seq)
    {
        return mapper().seq_in_playscreen(seq);
    }

    void song_recording (bool f);

    void song_record_snap (bool f)
    {
        m_song_record_snap = f;
    }

    void toggle_record_snap ()
    {
        m_song_record_snap = ! m_song_record_snap;
    }

    mutegroup::number group_selected () const
    {
        return mutes().group_selected();
    }

    bool midi_mute_group_present () const
    {
        return mutes().group_present();
    }

    bool is_group_learn () const
    {
        return mutes().is_group_learn();
    }

    int group_size () const
    {
        return mutes().group_size();
    }

    bool group_event () const
    {
        return mutes().group_event();
    }

    bool group_error () const
    {
        return mutes().group_error();
    }

    /**
     *  group_mode() starts out true, and allows mute_group_tracks() to work.
     *  It is set and unset via the "gmute" MIDI control and the group-on/off
     *  keys.  m_mode_group_learn starts out false, and is set and unset via the
     *  "glearn" MIDI control and the group-learn press and release actions.
     */

    bool group_mode () const
    {
        return mutes().group_mode();
    }

    void group_mode (bool flag)
    {
        mutes().group_mode(flag);
    }

    void toggle_group_mode ()
    {
        mutes().toggle_group_mode();
    }

    bool set_beats_per_minute (midibpm bpm);        /* more than a setter   */
    bool set_ppqn (int p);
    bool change_ppqn (int p);
    bool ui_change_set_bus (int b);

private:

    void hidden (bool flag)
    {
        m_hidden = flag;                            /* qt5nsmanager         */
        m_show_hide_pending = false;                /* tricky code          */
    }

    void show_cpu ();
    void playlist_activate (bool on);
    bool set_recording (seq::number seqno, bool active, bool toggle);
    bool set_quantized_recording (seq::number seqno, bool active, bool toggle);
    bool set_overwrite_recording (seq::number seqno, bool active, bool toggle);
    bool set_thru (seq::number seqno, bool active, bool toggle);
    bool log_current_tempo ();
    bool create_master_bus ();
    void reset_sequences (bool pause = false);

#if defined USE_STAZED_PARSE_SYSEX
    void parse_sysex (event ev);
#endif

    /**
     *  Convenience function for perfedit's collapse functionality.
     */

    void collapse ()
    {
        push_trigger_undo();
        move_triggers(false);
        modify();
    }

    /**
     *  Convenience function for perfedit's copy functionality.
     */

    void copy ()
    {
        push_trigger_undo();
        copy_triggers();
    }

    /**
     *  Convenience function for perfedit's expand functionality.
     */

    void expand ()
    {
        push_trigger_undo();
        move_triggers(true);
        modify();
    }

public:                                 /* access functions for the containers */

    const keycontainer & key_controls () const
    {
        return m_key_controls;
    }

    keycontainer & key_controls ()
    {
        return m_key_controls;
    }

    bool midi_control_keystroke (const keystroke & k);
    bool midi_control_event (const event & ev, bool recording = false);
    void signal_save ();
    void signal_quit ();

    /*
     * Looks up the slot-key (hot-key) for the given pattern number.
     */

    std::string lookup_slot_key (int seqno) const
    {
        return m_key_controls.slot_key(seqno % screenset_size());
    }

    std::string lookup_mute_key (int mute_number) const
    {
        return m_key_controls.mute_key(mute_number);
    }

    const midicontrolin & midi_control_in () const
    {
        return m_midi_control_in;
    }

    midicontrolin & midi_control_in ()
    {
        return m_midi_control_in;
    }

    automation::ctrlstatus ctrl_status () const
    {
        return midi_control_in().status();
    }

    std::string ctrl_status_string () const
    {
        return midi_control_in().status_string();
    }

    /*
     * Start of mute-groups accessors.
     */

    int mutegroup_count () const
    {
        return mutes().count();
    }

    const std::string & group_name (mutegroup::number group) const
    {
        return mutes().group_name(group);
    }

    const mutegroups & mutes () const
    {
        return m_mute_groups;
    }

    mutegroups & mutes ()
    {
        return m_mute_groups;
    }

    bool clear_mute_groups ()
    {
        return mutes().clear();
    }

    bool reset_mute_groups ()
    {
        return mutes().reset_defaults();
    }

private:

    void clear_snapshot ()
    {
        mapper().clear_snapshot();
    }

    void save_snapshot ()
    {
        mapper().save_snapshot();
    }

    void restore_snapshot ()
    {
        mapper().restore_snapshot();
    }

    void is_running (bool flag)
    {
        m_is_running = flag;
    }

    void unmodify ()
    {
        m_is_modified = false;                  /* m_needs_update = false;  */
    }

private:

    void output_func ();
    void input_func ();
    bool poll_cycle ();
    void launch_input_thread ();
    void launch_output_thread ();
    void midi_start ();
    void midi_continue ();
    void midi_stop ();
    void midi_clock ();
    void midi_song_pos (const event & ev);
    void midi_sysex (const event & ev);

    condition & cv ()
    {
        return m_condition_var;
    }

private:

    void show_key_error (const keystroke & k, const std::string & tag);

    static void print_parameters
    (
        const std::string & tag,
        automation::action a,
        int d0, int d1, bool inverse
    );

public:

    bool signalled_changes () const
    {
        return m_signalled_changes;
    }

    void clear_seq_edits ();

    void toggle_seq_edit ()
    {
        m_seq_edit_pending = ! m_seq_edit_pending;
    }

    void toggle_event_edit ()
    {
        m_event_edit_pending = ! m_event_edit_pending;
    }

    bool seq_edit_pending () const
    {
        return m_seq_edit_pending;
    }

    bool event_edit_pending () const
    {
        return m_event_edit_pending;
    }

    bool call_seq_edits () const
    {
        return m_seq_edit_pending || m_event_edit_pending;
    }

    seq::number pending_loop () const
    {
        return m_pending_loop;
    }

    void pending_loop (seq::number n) const
    {
        m_pending_loop = n;
    }

    int slot_shift () const
    {
        return m_slot_shift;
    }

    int increment_slot_shift () const
    {
        if (++m_slot_shift > 2)
            clear_slot_shift();

        return slot_shift();
    }

    void clear_slot_shift () const
    {
        m_slot_shift = 0;               /* mutable */
    }

    bool hidden () const
    {
        return m_hidden;
    }

    bool show_hide_pending () const
    {
        return m_show_hide_pending;
    }

    /*
     * This is just a very fast check meant for use in some GUI timers.
     */

    bool got_seqno (seq::number & s) const
    {
        bool result = seq::assigned(pending_loop());
        if (result)
            s = pending_loop();

        return result;
    }

    bool loop_control                   /* [loop-control]       */
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool mute_group_control             /* [mute-group-control] */
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool populate_default_ops ();
    bool add_automation                 /* [automation-control] */
    (
        automation::slot s,
        automation_function f
    );
    bool automation_no_op
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_bpm_up_dn
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_bpm_dn
    (
        automation::action a,int d0, int d1, bool inverse
    );
    bool automation_ss_up_dn
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_ss_dn
    (
        automation::action a,int d0, int d1, bool inverse
    );
    bool automation_replace
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_snapshot
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_queue
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_gmute
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_glearn
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_play_ss
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_playback
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_song_record
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_solo
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_thru
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_bpm_page_up_dn
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_bpm_page_dn
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_ss_set
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_record
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_quan_record
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_reset_seq
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_oneshot
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_FF
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_rewind
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_top
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_playlist
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_playlist_song
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_tap_bpm
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_start
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_stop
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_reserved_29
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_toggle_mutes
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_song_pointer
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_keep_queue
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_edit_pending
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_event_pending
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_slot_shift
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_mutes_clear
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_quit
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_song_mode
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_toggle_jack
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_menu_mode
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_follow_transport
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_panic
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_visibility
    (
        automation::action a, int d0, int d1, bool inverse
    );
    bool automation_save_session
    (
        automation::action a, int d0, int d1, bool inverse
    );

};          // class performer

}           // namespace seq66

#endif      // SEQ66_PERFORMER_HPP

/*
 * performer.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


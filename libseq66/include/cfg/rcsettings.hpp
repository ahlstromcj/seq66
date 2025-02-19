#if ! defined SEQ66_RCSETTINGS_HPP
#define SEQ66_RCSETTINGS_HPP

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
 * \file          rcsettings.hpp
 *
 *  This module declares/defines a settings class that is also exposed
 *  globally in this application.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-09-22
 * \updates       2025-02-19
 * \license       GNU GPLv2 or above
 *
 *  This collection of variables describes the options of the application,
 *  accessible from the command-line or from the 'rc' file.
 */

#include <string>                       /* std::string class                */

#include "cfg/basesettings.hpp"         /* seq66::basesettings class        */
#include "cfg/recent.hpp"               /* seq66::recent class              */
#include "ctrl/keycontainer.hpp"        /* seq66::keycontainer class        */
#include "ctrl/midicontrolin.hpp"       /* seq66::midicontrolin class       */
#include "ctrl/midicontrolout.hpp"      /* seq66::midicontrolout class      */
#include "play/clockslist.hpp"          /* list of seq66::e_clock settings  */
#include "play/inputslist.hpp"          /* list of boolean input settings   */
#include "play/metro.hpp"               /* seq66::metrosettings class       */
#include "play/mutegroups.hpp"          /* map of seqq66::mutes stanzas     */
#include "util/named_bools.hpp"         /* map of booleans keyed by strings */

/**
 *  EXPERIMENTAL.
 *  Keep a list of the full file-specifications of each of the configureation
 *  files that can be set up in the 'rc' file. We can eliminate the dependency
 *  on configuration file-extensions in copying and deleting a configuration.
 */

#if defined SEQ66_KEEP_RC_FILE_LIST
#include <map>                          /* std::map class                   */
#endif

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  This high-priority value is used if the --priority option is specified.
 *  Needs more testing, we really haven't needed it yet.
 */

static const int c_thread_priority = 10;

/**
 *  These control sizes.  We'll try changing them and see what happens.
 *  Increasing these value spreads out the pattern grids a little bit and
 *  makes the Patterns panel slightly bigger.  Seems like it would be
 *  useful to make these values user-configurable.
 */

/**
 *  The number of default virtual ALSA input busses supported in the
 *  manual-ports mode.  This value used to implicitly be 1, but it would be
 *  useful to allow a few more.  Now expanded per user request.  Let the user
 *  beware!  See issue #42.
 */

const int c_input_buss_max      = 48;
const int c_input_buss_default  =  4;

/**
 *  The number of ALSA I/O busses supported.  See mastermidibus::init().
 *  Currently, this is also the default number of "manual" (virtual) output
 *  ports created in the manual-ports mode.  Now expanded per user request.
 *  Let the user beware!  See issue #42.
 */

const int c_output_buss_max     = 48;
const int c_output_buss_default =  8;

/**
 *  Maximum number of groups that can be supported.  Basically, the number of
 *  groups set in the 'rc' file.  32 groups can be filled.  This is a permanent
 *  maximum because we really can't support more than 32 keystrokes to support
 *  selecting a mute-group.
 */

const int c_max_groups = 32;

/**
 *  Maximum number of screen sets that can be supported.  Basically, the number
 *  of times the Patterns Panel can be filled.  Up to 32 sets can be created.
 *  This is a permanent maximum because we really can't support more than 32
 *  keystrokes to support selecting a screenset.
 */

const int c_max_sets   = 32;

/**
 *  Default value of number of slot toggle keys (shortcut keys) that can be
 *  defined.  Even if we end up adding more slots to a set, this would be about
 *  the maximum number of keys we could really support.  Maximum number of set
 *  keys that can be supported.  32 keys can be assigned in the Options /
 *  Keyboard tab and 'rc' file.  This value applies to the "[keyboard-group]"
 *  and "[keyboard-control]" sections.
 */

const int c_max_set_keys = 32;

/**
 *  Indicates whether Seq66 or another program is the JACK timebase master.
 *
 * \var none
 *      JACK transport is not being used.
 *
 * \var slave
 *      An external program is timebase master and we disregard all local
 *      tempo information. Instead, we use onl the BPM provided by JACK.
 *
 * \var master
 *      Whether by force or conditionally, this program is JACK master.
 *
 * \var conditional
 *      This value is just for requesting conditional master in the 'rc' file.
 */

enum class timebase
{
    none,
    slave,
    master,
    conditional
};

/**
 *  We need to offer some options for handling running-status issues in
 *  some MIDI files.  See the midifile class.
 *
 * \var recover
 *      Try to recover the running-status value.
 *
 * \var skip
 *      Skip the rest of the track.
 *
 * \var proceed
 *      Allow running status errors to cascade.
 *
 * \var abort
 *      Stop processing the rest of the tracks.
 */

enum class rsaction
{
    recover,    /* Try to recover the running-status value.                 */
    skip,       /* Skip the rest of the track.                              */
    proceed,    /* Allow running status errors to cascade.                  */
    abort       /* Stop processing the rest of the tracks.                  */
};

/**
 *  This class contains "global" options that can be read from the 'rc' file
 *  (class rcfile) and its related "mutes" and "ctrl" files.
 */

class rcsettings final : public basesettings
{

public:

    /**
     *  Provides mutually-exclusive codes for the mouse-handling used by the
     *  application.  Moved here from the globals.h module.  The fruity mode
     *  will probably never be supported in seq66, though.
     */

    enum class interaction
    {
        seq24,          /**< Use the normal mouse interactions.             */
        fruity,         /**< The "fruity" mouse interactions. To do.        */
        max             /**< Keep this last... a size value.                */
    };

    /**
     *  Experimental to change how set changes work in regard to muting/arming
     *  of sequences in a set.
     */

    enum class setsmode
    {
        normal,         /**< Set change mutes current, loads new set.       */
        autoarm,        /**< Mute current set, load and unmute new set.     */
        additive,       /**< Keep current set armed when changing sets.     */
        allsets,        /**< Arm all sets at once.                          */
        max             /**< Keep this last... a size value.                */
    };

#if defined SEQ66_KEEP_RC_FILE_LIST

    /**
     *  Provides a map of file-specification strings keyed by the type of
     *  configuration file:
     *
     *      -   ctrl
     *      -   drums (also covers .notemap, the same kind of configuration)
     *      -   mutes
     *      -   palette
     *      -   patches
     *      -   playlist
     *      -   qss
     *      -   rc
     *      -   usr
     */

    using files = std::map<std::string, std::string>;

    /**
     *  Holds a map of full file specifications, for use in copying a
     *  configuration without caring about the file-extension.
     */

    files m_config_files;

#endif

private:

    /**
     *  The list of output clocks.
     */

    clockslist m_clocks;

    /**
     *  The list of input bus statuses.
     */

    inputslist m_inputs;

    /**
     *  Settings for the metronome.
     */

    metrosettings m_metro_settings;

    /**
     *  Holds the saving type on behalf of the mutegroups, which is now
     *  owned by performer.
     */

    mutegroups::saving m_mute_group_save;

    /**
     *  Holds the key-container.
     */

    keycontainer m_keycontainer;

    /**
     *  New.  If true, leave empty (inactive) MIDI control entries out of the
     *  "[midi-control-settings]" container.  This has the side effect of
     *  causing empty entries to not be written to the 'ctrl' file,
     *  potentially confusing. However, it can reduce the size of the control
     *  container dramatically, saving look-up time and memory.
     */

    bool m_drop_empty_in_controls;

    /**
     *  Provides a way to pick which input buss will be used as the MIDI
     *  controller device.
     */

    bussbyte m_midi_control_buss;

    /**
     *  Holds all of the MIDI controls stanzas, even one that are inactive.
     *  This is necessary for being able to write out a full section of
     *  keystroke controls and MIDI controls.
     */

    midicontrolin m_midi_control_in;

    /**
     *  Holds the MIDI control out stanzas.
     */

    midicontrolout m_midi_control_out;

    /**
     *  Provides the Song Position, in 16th notes, at which MIDI clocking will
     *  begin if a MIDI buss is set the the MIDI Clock Mode setting.
     *
     *  Held for midibase::set_clock_mod().
     */

    bool m_clock_mod;

    bool m_verbose;                 /**< Console message showing setting.   */
    bool m_quiet;                   /**< Disables startup error prompts.    */
    bool m_investigate;             /**< An option for the test of the day. */
    std::string m_session_tag;      /**< Picks an alternate configuration.  */

    /**
     *  A replacement for m_auto_option_save and all "save" options except for
     *  MIDI files.
     */

    named_bools m_save_list;

    bool m_save_old_triggers;       /**< Save c_triggers_ex, no transpose.  */
    bool m_save_old_mutes;          /**< Save mutes as bytes, not longs.    */
    bool m_allow_mod4_mode;         /**< Allow Mod4 to hold drawing mode.   */
    bool m_allow_snap_split;        /**< Allow snap-split of a trigger.     */
    bool m_allow_click_edit;        /**< Allow double-click edit pattern.   */
    bool m_show_midi;               /**< Show MIDI events to console.       */
    bool m_priority;                /**< Run at high priority (Linux only). */
    int m_thread_priority;          /**< The desired priority (Linux only). */
    bool m_pass_sysex;              /**< Pass SysEx to outputs, not ready.  */
    bool m_with_jack_transport;     /**< Enable synchrony with JACK.        */
    bool m_with_jack_master;        /**< Serve as a JACK transport Master.  */
    bool m_with_jack_master_cond;   /**< Serve as JACK Master if possible.  */
    bool m_with_jack_midi;          /**< Use JACK MIDI.                     */
    bool m_with_alsa_midi;          /**< Use ALSA MIDI.                     */
    bool m_jack_auto_connect;       /**< Connect JACK ports in normal mode. */
    bool m_jack_use_offset;         /**< Try to calculate output offset.    */
    int m_jack_buffer_size;         /**< The desired power-of-2 size, or 0. */
    sequence::playback m_song_start_mode; /**< Song mode versus Live mode.  */
    bool m_song_start_is_auto;      /**< True if "auto" read from 'rc'.     */
    bool m_record_by_buss;          /**< Record into sequence w/input-buss. */
    bool m_record_by_channel;       /**< Record into sequence with channel. */
    bool m_manual_ports;            /**< [manual-ports] setting.            */
    bool m_manual_auto_enable;      /**< [manual-port] auto-enable.         */
    int m_manual_port_count;        /**< [manual-ports] output port count.  */
    int m_manual_in_port_count;     /**< [manual-ports] input port count.   */
    bool m_reveal_ports;            /**< [reveal-ports] setting.            */
    bool m_init_disabled_ports;     /**< A new test option. EXPERIMENTAL.   */
    bool m_print_keys;              /**< Show hot-key in main window slot.  */
    interaction m_interaction_method; /**< Interaction method: no support.  */
    setsmode m_sets_mode;           /**< How to handle set changes.         */
    portname m_port_naming;         /**< How to display port names.         */

    /**
     *  Provides the name of current MIDI file.  Under normal usage, it is the
     *  full file specification, including the path to the file.  Under session
     *  management, it is the base name (e.g. "song.midi") of the file with
     *  the new m_midi_filepath variable prepended to the base name.
     */

    std::string m_midi_filename;

    /**
     *  Provides the base name for MIDI files.  This value is meant to be used
     *  only under session management, where all files must be read and written
     *  from the same non-standard (get the pun?) directory.  It will be empty
     *  under normal operation.
     */

    std::string m_midi_filepath;

    /**
     *  Indicates what to do with running-status irregularities.
     */

    rsaction m_running_status_action;

    /**
     *  Holds the JACK UUID value that makes this JACK connection unique.
     */

    std::string m_jack_session_uuid;

    /**
     *  Indicates if the JACK session callback was invoked.
     */

    bool m_jack_session_active;

    /**
     *  Holds the directory from which the last MIDI file was opened (or
     *  saved).
     */

    std::string m_last_used_dir;

    /**
     *  Holds the current 'rc' and 'usr' configuration base directory.  This
     *  value is ".config/seq66" by default.  For usage, it is normally
     *  expanded to a full path.
     *
     *  For NSM usage, it is the full path returned by the NSM daemon.
     */

    std::string m_session_directory;

    /**
     *  An optional appendage to the base configuration directory. It is
     *  appended to the default session/configuration path. The setter,
     *  config_subdirectory, is meant to work only once. It is set only
     *  by the --home option. The boolean makes sure the appending is done only
     *  once (due to processing command-line options multiple times, :-(.)
     */

    mutable bool m_config_subdirectory_set;
    std::string m_config_subdirectory;

    /**
     *  Holds the current 'rc' configuration filename.  This value is
     *  "qseq66.rc" by default, and is always a basename.
     */

    std::string m_config_filename;

    /**
     *  The full expanded path to the configuration directory.  This value is
     *  created, by default, by concatenating $HOME and ".config/seq66".
     *  However it can be reset completely by the full_config_directory()
     *  function to whatever the user needs (e.g. for usage with the Non/New
     *  Session Manager, RaySession, and Agordejo).
     */

    mutable std::string m_full_config_directory;

    /**
     *  Indicates if the 'usr' file is actually to be used.
     *  Useful for temporarily disabling a radically modified file.
     */

    bool m_user_file_active;

    /**
     *  Holds the current 'usr' configuration filename.  This value is
     *  "qseq66.usr" by default.
     */

    std::string m_user_filename;

    /**
     *  Indicates if the user wants to use a [midi-control] section from a
     *  separate file, for the convenience of changing the MIDI control setup
     *  without a lot of editing.  This value is now permanently set to true.
     *
     *      bool m_use_midi_control_file;
     */

    /**
     *  A new flag for indicating if MIDI Control I/O is to be active.
     *  Useful for temporarily disabling a 'ctrl' file.
     */

    bool m_midi_control_active;

    /**
     *  The base name of the MIDI control file, if applicable.  This file is
     *  located only in the specific 'rc'/'usr' HOME directory,
     *  m_session_directory.
     */

    std::string m_midi_control_filename;

    /**
     *  Indicates if the user wants to use a [mute-group] section from a
     *  separate file, for the convenience of changing the setup
     *  without a lot of editing.  This value is now permanently
     *  set to true.  A user with an old setup will have to adapt manually if
     *  necessary.
     *
     *      bool m_use_mute_group_file;
     */

    /**
     *  Indicates if the mute-group file is actually to be used.
     *  Useful for enabling/disabling a 'mutes' file as opposed to
     *  using the mutes stored in a Seq66 file.
     */

    bool m_mute_group_file_active;

    /**
     *  The base name of the mute-group file, if applicable.  This file is
     *  located only in the specific 'rc'/'usr' HOME directory,
     *  m_session_directory.
     */

    std::string m_mute_group_filename;

    /**
     *  Indicates if the user wants to use the play-list stored in the 'rc'
     *  file.  This value is stored as well.  It is cleared if there was a
     *  problem such as the play-list file-name not existing.
     */

    bool m_playlist_active;

    /**
     *  Provides the full name of a play-list file, such as "tunes.playlist"
     *  or "/home/dude/.config/seq66/tunes.playlist".
     *
     *  It is used only if playlist mode is active.  This file is always
     *  located in the configuration directory (which can be modified from the
     *  command-line).
     */

    std::string m_playlist_filename;

    /**
     *  Holds the base directory of all the MIDI files in all the playlists.
     *  Normally useful when MIDI files are in an NSM session directory or a
     *  directory separate from where Seq66 is run.  Normally empty.
     */

    std::string m_playlist_midi_base;

    /**
     *  Indicates if the user wants to use the note-mapper stored in the 'rc'
     *  file.  This value is stored as well.
     */

    bool m_notemap_active;

    /**
     *  Provides the name of a note-mapping file to use.  This is a feature
     *  adapted and modified from our "midicvt" project.
     */

    std::string m_notemap_filename;

    /**
     *  Indicates if the user wants to use the patches stored in the 'patches'
     *  file.  This value is stored as well.
     */

    bool m_patches_active;

    /**
     *  Provides the name of a patches file to use.  This is a feature
     *  adapted and modified from our "midicvt" project.
     */

    std::string m_patches_filename;

    /**
     *  Indicates if the user wants to use the palette file stored in the 'rc'
     *  file value.
     */

    bool m_palette_active;

    /**
     *  Provides the base name of a palette file to use.
     */

    std::string m_palette_filename;

    /**
     *  Indicates if the style-sheet will be used. Move from 'usr' to the 'rc'
     *  file.
     */

    bool m_style_sheet_active;

    /**
     *  Provides the name of an optional Qt style-sheet, located in the active
     *  Seq66 configuration directory.  By default, this name is empty and not
     *  used.  It present, it contains the base name of the sheet (e.g.
     *  "qseq66.qss".  It can also contain a path, in order to support a
     *  universal style-sheet. Move from 'usr' to the 'rc' file.
     */

    std::string m_style_sheet_filename;

    /**
     *  Holds the application name, e.g. "seq66", "qpseq66", "seq66cli", or,
     *  most commonly, "qseq66".  This is a constant, set to SEQ66_APP_NAME,
     *  but obtained via the seq_app_name() function.  Do not confuse it with
     *  the client name, which defaults to "seq66" no matter what the
     *  application name.
     */

    std::string m_application_name;

    /**
     *  New value to allow the user to violate the MIDI specification and use a
     *  track other than the first track (#0) as the MIDI tempo track.
     *  Holds the number of the official tempo track for this performance.
     *  Normally 0, it can be changed to any value from 1 to 1023 via the
     *  tempo-track-number setting in the 'rc' file, and that can be overriden
     *  by the c_tempo_track SeqSpec possibly present in the song's MIDI file.
     */

    int m_tempo_track_number;

    /**
     *  Holds a few MIDI file-names most recently used.  Although this is a
     *  vector, we do not let it grow past SEQ66_RECENT_FILES_MAX.
     *  New feature from Oli Kester's kepler34 project.
     */

    recent m_recent_files;

    /**
     *  If true, this flag indicates to open the most recent MIDI file, which is
     *  the first in the list.  This flag is set and used only at startup time,
     *  after the "session" is created.
     */

    bool m_load_most_recent;

    /**
     *  If true, show the full directory path in the most-recent-file list, in
     *  order to distinguish identical tunes in different sub-directories.
     *  Defaults to false.
     */

    bool m_full_recent_paths;

    /**
     *  Indicates that the "[midi-input-map]" and "[midi-clock-map]" sections
     *  were found, which indicates they should not be recreated again,
     *  regardless of the status of "portmaps active".
     */

    bool m_portmaps_present;

    /**
     *  Indicates if both the input and output port-maps are active. Needed as
     *  a convenience for callers that need to know the statuses of both
     *  ports.
     */

    bool m_portmaps_active;

public:

    rcsettings ();
    rcsettings (const rcsettings & rhs) = default;
    rcsettings & operator = (const rcsettings & rhs) = default;
    virtual ~rcsettings () = default;

    std::string no_name () const
    {
        return std::string("No name");
    }

    std::string make_config_filespec
    (
        const std::string & base,
        const std::string & ext = ""
    ) const;

    std::string config_filespec () const;
    std::string config_filespec (const std::string & altname) const;
    std::string user_filespec () const;
    std::string user_filespec (const std::string & altname) const;
    std::string midi_control_filespec () const;
    std::string mute_group_filespec () const;
    std::string playlist_filespec () const;
    void clear_playlist (bool disable = false);
    std::string notemap_filespec () const;
    std::string patches_filespec () const;
    std::string palette_filespec () const;
    std::string style_sheet_filespec () const;
    virtual void set_defaults () override;

#if defined SEQ66_KEEP_RC_FILE_LIST

    const files & config_files () const
    {
        return m_config_files;
    }

    bool add_config_filespec
    (
        const std::string & key,
        const std::string & fspec
    );

#endif

    const clockslist & clocks () const
    {
        return m_clocks;
    }

    clockslist & clocks ()
    {
        return m_clocks;
    }

    const inputslist & inputs () const
    {
        return m_inputs;
    }

    inputslist & inputs ()
    {
        return m_inputs;
    }

    metrosettings & metro_settings ()
    {
        return m_metro_settings;
    }

    const metrosettings & metro_settings () const
    {
        return m_metro_settings;
    }

    mutegroups::saving mute_group_save () const
    {
        return m_mute_group_save;
    }

    const keycontainer & key_controls () const
    {
        return m_keycontainer;
    }

    keycontainer & key_controls ()
    {
        return m_keycontainer;
    }

    bool drop_empty_in_controls () const
    {
        return m_drop_empty_in_controls;
    }

    bussbyte midi_control_buss () const
    {
        return m_midi_control_buss;
    }

    const midicontrolin & midi_control_in () const
    {
        return m_midi_control_in;
    }

    midicontrolin & midi_control_in ()
    {
        return m_midi_control_in;
    }

    const midicontrolout & midi_control_out () const
    {
        return m_midi_control_out;
    }

    midicontrolout & midi_control_out ()
    {
        return m_midi_control_out;
    }

    int get_clock_mod () const
    {
        return m_clock_mod;
    }

    bool verbose () const
    {
        return m_verbose;
    }

    bool quiet () const
    {
        return m_quiet;
    }

    bool investigate () const
    {
        return m_investigate;
    }

    bool investigate_disabled () const
    {
        return false;
    }

    const std::string & session_tag () const
    {
        return m_session_tag;
    }

    bool alt_session () const
    {
        return ! m_session_tag.empty();
    }

    bool auto_options_save () const;

    bool auto_rc_save () const
    {
        return m_save_list.get("rc");
    }

    bool auto_usr_save () const
    {
        return m_save_list.get("usr");
    }

    bool auto_mutes_save () const
    {
        return m_save_list.get("mutes");
    }

    bool auto_playlist_save () const
    {
        return m_save_list.get("playlist");
    }

    /**
     *  Actually, since we cannot edit keystroke/MIDI control, drums
     *  (note-mapping), and style-sheets in the application, these functions
     *  are moot. Although the palettes can be saved by a button in
     *  the session preferences tab.
     */

    bool auto_ctrl_save () const
    {
        return m_save_list.get("ctrl");
    }

    bool auto_drums_save () const
    {
        return m_save_list.get("drums");
    }

    /**
     *  Unused. Style-sheet not used by default, so not saved, even at
     *  first-start. Kept for :-) consistency.
     */

    bool auto_qss_save () const
    {
        return m_save_list.get("qss");
    }

    bool auto_palette_save () const
    {
        return m_save_list.get("palette");
    }

    bool save_old_triggers () const
    {
        return m_save_old_triggers;
    }

    bool save_old_mutes () const
    {
        return m_save_old_mutes;
    }

    bool allow_mod4_mode () const
    {
        return m_allow_mod4_mode;
    }

    bool allow_snap_split () const
    {
        return m_allow_snap_split;
    }

    bool allow_click_edit () const
    {
        return m_allow_click_edit;
    }

    bool show_midi () const
    {
        return m_show_midi;
    }

    bool priority () const
    {
        return m_priority;
    }

    int thread_priority () const
    {
        return m_thread_priority;
    }

    bool pass_sysex () const
    {
        return m_pass_sysex;
    }

    bool with_jack_transport () const
    {
        return m_with_jack_transport;
    }

    bool with_jack_master () const
    {
        return m_with_jack_master;
    }

    bool with_jack_master_cond () const
    {
        return m_with_jack_master_cond;
    }

    bool with_jack_midi () const
    {
        return m_with_jack_midi;
    }

    bool with_alsa_midi () const
    {
        return m_with_alsa_midi;
    }

    bool with_port_midi () const
    {
#if defined SEQ66_PORTMIDI_SUPPORT
        return true;
#else
        return false;
#endif
    }

    bool sequence_lookup_support () const;

    bool jack_auto_connect () const
    {
        return m_jack_auto_connect;
    }

    bool jack_use_offset () const
    {
        return m_jack_use_offset;
    }

    int jack_buffer_size () const
    {
        return m_jack_buffer_size;
    }

    bool song_start_mode () const
    {
        return m_song_start_mode == sequence::playback::song;
    }

    sequence::playback get_song_start_mode () const
    {
        return m_song_start_mode;
    }

    /**
     *  Was returning m_song_start_mode == sequence::playback::automatic, but
     *  this conflates run-time mode with desired initial mode.
     */

    bool song_start_auto () const
    {
        return m_song_start_is_auto;
    }

    std::string song_mode_string () const;
    void set_jack_transport (const std::string & value);

    void with_jack_transport (bool flag)
    {
        m_with_jack_transport = flag;
    }

    void with_jack_master (bool flag)
    {
        m_with_jack_master = flag;
        if (flag)
            m_with_jack_transport = true;
    }

    void with_jack_master_cond (bool flag)
    {
        m_with_jack_master_cond = flag;
        if (flag)
            m_with_jack_transport = true;
    }

    void with_jack_midi (bool flag)
    {
        m_with_jack_midi = flag;
    }

    void with_alsa_midi (bool flag)
    {
        m_with_alsa_midi = flag;
    }

    void jack_auto_connect (bool flag)
    {
        m_jack_auto_connect = flag;
    }

    void jack_use_offset (bool flag)
    {
        m_jack_use_offset = flag;
    }

    /*
     * This check is the same as is_power_of_2() in the calculations module.
     */

    void jack_buffer_size (int sz)
    {
        if (((sz != 0) && (! (sz & (sz - 1)))) || (sz == 0))
            m_jack_buffer_size = sz;
    }

    /**
     * \getter m_with_jack_transport m_with_jack_master, and
     * m_with_jack_master_cond, to save client code some trouble.  Do not
     * confuse these original options with the new "no JACK MIDI" option.
     */

    bool with_jack () const
    {
        return
        (
            m_with_jack_transport || m_with_jack_master ||
                m_with_jack_master_cond
        );
    }

    bool record_by_buss () const
    {
        return m_record_by_buss;
    }

    bool record_by_channel () const
    {
        return m_record_by_channel;
    }

    bool manual_ports () const
    {
        return m_manual_ports;
    }

    bool manual_auto_enable () const
    {
        return m_manual_auto_enable;
    }

    int manual_port_count () const
    {
        return m_manual_port_count;
    }

    int manual_in_port_count () const
    {
        return m_manual_in_port_count;
    }

    bool reveal_ports () const
    {
        return m_reveal_ports;
    }

    bool init_disabled_ports () const
    {
        return m_init_disabled_ports;
    }

    bool print_keys () const
    {
        return m_print_keys;
    }

    /*
     * Currently not supported in Seq66.
     */

    interaction interaction_method () const
    {
        return m_interaction_method;
    }

    setsmode sets_mode () const
    {
        return m_sets_mode;
    }

    bool is_setsmode_normal () const
    {
        return m_sets_mode == setsmode::normal;
    }

    bool is_setsmode_autoarm () const
    {
        return m_sets_mode == setsmode::autoarm;
    }

    bool is_setsmode_additive () const
    {
        return m_sets_mode == setsmode::additive;
    }

    bool is_setsmode_allsets () const
    {
        return m_sets_mode == setsmode::allsets;
    }

    bool is_setsmode_clear () const
    {
        return m_sets_mode == setsmode::normal ||
            m_sets_mode == setsmode::autoarm;
    }

    std::string sets_mode_string () const;
    std::string sets_mode_string (setsmode v) const;

    portname port_naming () const
    {
        return m_port_naming;
    }

    std::string port_naming_string () const;
    std::string port_naming_string (portname v) const;

    const std::string & midi_filename () const
    {
        return m_midi_filename;
    }

    void midi_filename (const std::string & value)
    {
        m_midi_filename = value;
    }

    void clear_midi_filename ()
    {
        m_midi_filename.clear();
    }

    void session_midi_filename (const std::string & value);

    const std::string & midi_filepath () const
    {
        return m_midi_filepath;
    }

    void midi_filepath (const std::string & value)
    {
        m_midi_filepath = value;
    }

    void running_status_action (const std::string & value);
    std::string running_status_action_name () const;

    rsaction running_status_action () const
    {
        return m_running_status_action;
    }

    const std::string & jack_session () const
    {
        return m_jack_session_uuid;
    }

    bool jack_session_active () const
    {
        return m_jack_session_active;
    }

    const std::string & last_used_dir () const
    {
        return m_last_used_dir;
    }

    void last_used_dir (const std::string & value, bool userchange = true);

    /**
     * \setter m_recent_files
     *
     *  First makes sure the filename is not already present, and removes
     *  the back entry from the list, if it is full (SEQ66_RECENT_FILES_MAX)
     *  before adding it.  Now the full pathname is added.
     *
     * \param fname
     *      Provides the full path to the MIDI file that is to be added to
     *      the recent-files list.
     *
     * \return
     *      Returns true if the file-name was able to be added.
     */

    bool add_recent_file (const std::string & filename)
    {
        bool result = m_recent_files.add(filename);
        if (result)
            auto_rc_save(true);                 /* fix on 2023-04-09 by ca  */

        return result;
    }

    bool append_recent_file (const std::string & filename)
    {
        return m_recent_files.append(filename);
    }

    bool remove_recent_file (const std::string & filename)
    {
        return m_recent_files.remove(filename);
    }

    void clear_recent_files ()
    {
        m_recent_files.clear();
    }

    bool load_most_recent () const
    {
        return m_load_most_recent;
    }

    bool full_recent_paths () const
    {
        return m_full_recent_paths;
    }

    bool portmaps_present () const
    {
        return m_portmaps_present;
    }

    bool portmaps_active () const
    {
        return m_portmaps_active;
    }

    const std::string & session_directory () const
    {
        return m_session_directory;
    }

    void set_config_files (const std::string & value);
    bool has_home_config_path (const std::string & name);
    std::string default_session_path () const;
    std::string home_config_directory () const;
    std::string trim_home_directory (const std::string & filepath);

    const std::string & config_filename () const
    {
        return m_config_filename;
    }

    bool playlist_active () const
    {
        return m_playlist_active;
    }

    bool notemap_active () const
    {
        return m_notemap_active;
    }

    bool patches_active () const
    {
        return m_patches_active;
    }

    bool palette_active () const
    {
        return m_palette_active;
    }

    bool style_sheet_active () const
    {
        return m_style_sheet_active;
    }

    const std::string & style_sheet_filename () const
    {
        return m_style_sheet_filename;
    }

    const std::string & playlist_filename () const
    {
        return m_playlist_filename;
    }

    const std::string & midi_base_directory () const
    {
        return m_playlist_midi_base;
    }

    const std::string & notemap_filename () const
    {
        return m_notemap_filename;
    }

    const std::string & patches_filename () const
    {
        return m_patches_filename;
    }

    const std::string & palette_filename () const
    {
        return m_palette_filename;
    }

    bool user_file_active () const
    {
        return m_user_file_active;
    }

    const std::string & user_filename () const
    {
        return m_user_filename;
    }

     bool use_midi_control_file () const
     {
        return true;
     }

     bool midi_control_active () const
     {
        return m_midi_control_active;
     }

     const std::string & midi_control_filename () const
     {
        return m_midi_control_filename;
     }

     bool mute_group_file_active () const
     {
        return m_mute_group_file_active;
     }

     const std::string & mute_group_filename () const
     {
        return m_mute_group_filename;
     }

     bool use_mute_group_file () const
     {
        return true;
     }

    const std::string application_name () const
    {
        return m_application_name;
    }

    const std::string & app_client_name () const;
    void app_client_name (const std::string & n) const;

    int tempo_track_number () const
    {
        return m_tempo_track_number;
    }

    std::string recent_file (int index, bool shorten = true) const;

    int recent_file_count () const
    {
        return m_recent_files.count();
    }

    int recent_file_max () const
    {
        return m_recent_files.maximum();
    }

public:

    void mute_group_save (mutegroups::saving ms)
    {
        m_mute_group_save = ms;
    }

    void drop_empty_in_controls (bool flag)
    {
        m_drop_empty_in_controls = flag;
    }

    void midi_control_buss (bussbyte b)
    {
        m_midi_control_buss = b;
    }

    /**
     *  Set the clock mod to the given value, if legal.
     *
     * \param clockmod
     *      If this value is not equal to 0, it is used to set the static
     *      member m_clock_mod.
     */

    void set_clock_mod (int clockmod)
    {
        if (clockmod != 0)
            m_clock_mod = clockmod;
    }

    void session_tag (const std::string & t)
    {
        m_session_tag = t;
    }

    void quiet (bool flag)
    {
        m_quiet = flag;
    }

    void verbose (bool flag);
    void investigate (bool flag);
    void set_imported_playlist
    (
        const std::string & sourcepath,
        const std::string & midipath
    );
    void auto_rc_save (bool flag);

    void auto_usr_save (bool flag)
    {
        m_save_list.set("usr", flag);
    }

    void auto_mutes_save (bool flag)
    {
        m_save_list.set("mutes", flag);
    }

    void auto_playlist_save (bool flag)
    {
        m_save_list.set("playlist", flag);
    }

    void auto_ctrl_save (bool flag)
    {
        m_save_list.set("ctrl", flag);
    }

    /*
     * Used in smanager and set in qseditoptions.
     */

    void auto_drums_save (bool flag)
    {
        m_save_list.set("drums", flag);
    }

    /*
     * Used in qt5nsmanager, but not set anywhere.
     */

    void auto_palette_save (bool flag)
    {
        m_save_list.set("palette", flag);
    }

    /*
     *  void auto_palette_save (bool flag)
     *  {
     *      m_save_list.set("palette", flag);
     *  }
     */

    void save_old_triggers (bool flag)
    {
        m_save_old_triggers = flag;
    }

    void save_old_mutes (bool flag)
    {
        m_save_old_mutes = flag;
    }

    void allow_mod4_mode (bool /*flag*/)
    {
        m_allow_mod4_mode = false;
    }

    void allow_snap_split (bool flag)
    {
        m_allow_snap_split = flag;
    }

    void allow_click_edit (bool flag)
    {
        m_allow_click_edit = flag;
    }

    void show_midi (bool flag)
    {
        m_show_midi = flag;
    }

    void priority (bool flag)
    {
        m_priority = flag;
    }

    void thread_priority (int p)
    {
        if (p == 0)
            p = c_thread_priority;

        m_thread_priority = p;
    }

    void pass_sysex (bool flag)
    {
        m_pass_sysex = flag;
    }

    void song_start_mode (bool flag)
    {
        m_song_start_mode = flag ?
            sequence::playback::song : sequence::playback::live ;
    }

    void song_start_mode_by_string (const std::string & s);
    void record_by_buss (bool flag);
    void record_by_channel (bool flag);

    void manual_ports (bool flag)
    {
        m_manual_ports = flag;
    }

    void  manual_auto_enable (bool flag)
    {
        m_manual_auto_enable = flag;
    }

    void manual_port_count (int count)
    {
        if (count <= 0 || count > c_output_buss_max)
            count = c_output_buss_default;

        m_manual_port_count = count;
    }

    void default_manual_port_counts ()
    {
        m_manual_port_count = c_output_buss_default;
        m_manual_in_port_count = c_input_buss_default;
    }

    void manual_in_port_count (int count)
    {
        if (count <= 0 || count > c_input_buss_max)
            count = c_input_buss_default;

        m_manual_in_port_count = count;
    }

    void reveal_ports (bool flag)
    {
        m_reveal_ports = flag;
    }

    void init_disabled_ports (bool flag)
    {
        m_init_disabled_ports = flag;
    }

    void print_keys (bool flag)
    {
        m_print_keys = flag;
    }

    void midi_control_active (bool flag)
    {
        m_midi_control_active = flag;
    }

    void midi_control_filename (const std::string & name);

    void mute_group_file_active (bool flag)
    {
        m_mute_group_file_active = flag;
    }

    void mute_group_filename (const std::string & name);

    void playlist_active (bool flag)
    {
        m_playlist_active = flag;
    }

    void notemap_active (bool flag)
    {
        m_notemap_active = flag;
    }

    void patches_active (bool flag)
    {
        m_patches_active = flag;
    }

    void palette_active (bool flag)
    {
        m_palette_active = flag;
    }

    void style_sheet_active (bool flag)
    {
        m_style_sheet_active = flag;
    }

    void  midi_base_directory (const std::string & mbd)
    {
        m_playlist_midi_base = mbd;
    }

    void load_most_recent (bool f)
    {
        m_load_most_recent = f;
    }

    void full_recent_paths (bool f)
    {
        m_full_recent_paths = f;
    }

    void portmaps_present (bool f)
    {
        m_portmaps_present = f;
    }

    void portmaps_active (bool f)
    {
        m_portmaps_active = f;
    }

    bool interaction_method (int v)
    {
        return interaction_method(static_cast<interaction>(v));
    }

    void sets_mode (setsmode sm)
    {
        m_sets_mode = sm;
    }

    void sets_mode (const std::string & v);
    void port_naming (const std::string & v);

    /*
     * The setters for non-bool values, defined in the cpp file because
     * they do some heavier validation.
     */

    void tempo_track_number (int track);
    bool interaction_method (interaction value);
    void jack_session (const std::string & uuid);

    void jack_session_activate ()
    {
        m_jack_session_active = true;
    }

    void set_config_directory (const std::string & value);
    void full_config_directory (const std::string & value);
    void session_directory (const std::string & value);
    void config_subdirectory (const std::string & value);
    void config_filename (const std::string & value);
    void playlist_filename (const std::string & value);
    bool playlist_filename_checked (const std::string & value);
    void user_filename (const std::string & value);
    void notemap_filename (const std::string & value);
    void patches_filename (const std::string & value);
    void palette_filename (const std::string & value);
    void style_sheet_filename (const std::string & value);
    void create_config_names (const std::string & base = "");
    void set_save_list (bool state);
    void disable_save_list ();
    void set_save (const std::string & name, bool value);
    std::string filespec_helper (const std::string & baseext) const;

    void home_config_directory (const std::string & hcd)
    {
        m_full_config_directory = hcd;
    }

    void user_file_active (bool flag)
    {
        m_user_file_active = flag;
    }

private:

    std::string filename_base_fix
    (
        const std::string & filename,
        const std::string & ext
    ) const;

};          // class rcsettings

}           // namespace seq66

#endif      // SEQ66_RCSETTINGS_HPP

/*
 * rcsettings.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


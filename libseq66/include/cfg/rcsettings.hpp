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
 * \updates       2021-10-05
 * \license       GNU GPLv2 or above
 *
 *  This collection of variables describes the options of the application,
 *  accessible from the command-line or from the "rc" file.
 */

#include <string>

#include "cfg/basesettings.hpp"         /* seq66::basesettings class        */
#include "cfg/recent.hpp"               /* seq66::recent class              */
#include "ctrl/keycontainer.hpp"        /* seq66::keycontainer class        */
#include "ctrl/midicontrolin.hpp"       /* seq66::midicontrolin class       */
#include "ctrl/midicontrolout.hpp"      /* seq66::midicontrolout class      */
#include "play/mutegroups.hpp"          /* list of mute groups              */
#include "play/clockslist.hpp"          /* list of seq66::e_clock settings  */
#include "play/inputslist.hpp"          /* list of boolean input settings   */
#include "play/mutegroups.hpp"          /* map of seqq66::mutes stanzas     */
#include "util/named_bools.hpp"         /* map of booleans keyed by strings */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

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
 *  groups set in the "rc" file.  32 groups can be filled.  This is a permanent
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
 *  Keyboard tab and "rc" file.  This value applies to the "[keyboard-group]"
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
 *  This class contains "global" options that can be read from the "rc" file
 *  (class rcfile) and its related "mutes" and "ctrl" files.
 */

class rcsettings final : public basesettings
{
    /*
     * Too many friends need access to the protected setters.
     *
     *      friend class cmdlineopts;
     *      friend class clinsmanager;
     *      friend class converter;
     *      friend class midicontrolfile;
     *      friend class mutegroupsfile;
     *      friend class optionsfile;
     *      friend class performer;
     *      friend class rcfile;
     *      friend class rtmidi_info;
     */

public:

    /**
     *  Provides mutually-exclusive codes for the mouse-handling used by the
     *  application.  Moved here from the globals.h module.  The fruity mode
     *  will probably never be supported in seq66, though.
     */

    enum class interaction
    {
        seq24,          /**< Use the normal mouse interactions.         */
        fruity,         /**< The "fruity" mouse interactions. To do.    */
        maximum         /**< Keep this last... a size value.            */
    };

    /**
     *  Experimental to change how set changes work in regard to muting/arming
     *  of sequences in a set.
     */

    enum class setsmode
    {
        normal,         /**< Set change mutes current, loads new set.   */
        autoarm,        /**< Mute current set, load and unmute new set. */
        additive,       /**< Keep current set armed when changing sets. */
        allsets,        /**< Arm all sets at once.                      */
        maximum         /**< Keep this last... a size value.            */
    };

    /**
     *  Indicates whether to use the short (internal) or the long (normal)
     *  port names in visible user-interface elements.  If there is no
     *  internal map, this option is forced to "long".
     */

    enum class portnaming
    {
        shortnames,     /**< Use short names: "[0] MIDI Through Port".  */
        longnames,      /**< Long names: "[0] 36:0 MIDI Through Port".  */
        maximum         /**< Keep this last... a size value.            */
    };

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
     *  Holds a set of mute-groups ("mutes") read from a configuration file.
     */

    mutegroups m_mute_groups;

    /**
     *  Indicates if we want to load the key controls.  Generally, we do.
     *  This setting is made by the [midi-control-flags] load-key-control
     *  entry in the MIDI control section.
     */

    bool m_load_key_controls;

    /**
     *  Holds the key-container.
     */

    keycontainer m_keycontainer;

    /**
     *  Indicates if we want to load the MIDI controls.  Generally, we do.
     *  This setting is made by the [midi-control-flags] load-midi-control
     *  entry in the MIDI control section.
     */

    bool m_load_midi_controls;

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

    /*
     * Much more complete descriptions of these options can be found in the
     * seq66.rc file.
     */

    bool m_show_ui_sequence_key;    /**< Moved from key_performers.         */
    bool m_show_ui_sequence_number; /**< Moved from key_performers.         */

    /**
     *  Provides the Song Position, in 16th notes, at which MIDI clocking will
     *  begin if a MIDI buss is set the the MIDI Clock Mode setting.
     *
     *  Held for midibase::set_clock_mod().
     */

    bool m_clock_mod;

    bool m_verbose;                 /**< Message-showing setting.           */
    bool m_investigate;             /**< An option for the test of the day. */

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
    bool m_pass_sysex;              /**< Pass SysEx to outputs, not ready.  */
    bool m_with_jack_transport;     /**< Enable synchrony with JACK.        */
    bool m_with_jack_master;        /**< Serve as a JACK transport Master.  */
    bool m_with_jack_master_cond;   /**< Serve as JACK Master if possible.  */
    bool m_with_jack_midi;          /**< Use JACK MIDI.                     */
    sequence::playback m_song_start_mode; /**< Song mode versus Live mode.  */
    bool m_song_start_is_auto;      /**< True if "auto" read from 'rc'.     */
    bool m_filter_by_channel;       /**< Record only sequence channel data. */
    bool m_manual_ports;            /**< [manual-ports] setting.            */
    int m_manual_port_count;        /**< [manual-ports] outputjport count.  */
    int m_manual_in_port_count;     /**< [manual-ports] inputjport count.   */
    bool m_reveal_ports;            /**< [reveal-ports] setting.            */
    bool m_print_keys;              /**< Show hot-key in main window slot.  */
    interaction m_interaction_method; /**< Interaction method: no support.  */
    setsmode m_sets_mode;           /**< How to handle set changes.         */
    portnaming m_port_naming;       /**< How to display port names.         */

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
     *  Holds the current "rc" and "user" configuration base directory.  This
     *  value is ".config/seq66" by default.  For usage, it is normally
     *  expanded.
     */

    std::string m_config_directory;

    /**
     *  Holds the current "rc" configuration filename.  This value is
     *  "qseq66.rc" by default.
     */

    std::string m_config_filename;

    /**
     *  The full expanded path to the configuration directory.  This value is
     *  created, by default, by concatenating $HOME and ".config/seq66".
     *  However it can be reset completely by the full_config_directory()
     *  function to what ever the user needs (e.g. for usage with the Non
     *  Session Manager).
     */

    mutable std::string m_full_config_directory;

    /**
     *  Indicates if the 'usr' file is actually to be used.
     *  Useful for temporarily disabling a radically modified file.
     */

    bool m_user_file_active;

    /**
     *  Holds the current "user" configuration filename.  This value is
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
     *  located only in the specific "rc"/"usr" HOME directory,
     *  m_config_directory.
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
     *  Useful for temporarily disabling a 'mutes' file.
     */

    bool m_mute_group_active;

    /**
     *  The base name of the mute-group file, if applicable.  This file is
     *  located only in the specific "rc"/"usr" HOME directory,
     *  m_config_directory.
     */

    std::string m_mute_group_filename;

    /**
     *  Indicates if the user wants to use the play-list stored in the "rc"
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
     *  Indicates if the user wants to use the note-mapper stored in the "rc"
     *  file.  This value is stored as well.
     */

    bool m_notemap_active;

    /**
     *  Provides the name of a note-mapping file to use.  This is a feature
     *  adapted and modified from our "midicvt" project.
     */

    std::string m_notemap_filename;

    /**
     *  Indicates if the user wants to use the palette file stored in the "rc"
     *  file value.
     */

    bool m_palette_active;

    /**
     *  Provides the name of a palette file to use.
     */

    std::string m_palette_filename;

    /**
     *  Holds the application name, e.g. "seq66", "qpseq66", "qrseq66",
     *  "seq66cli", or, most commonly, "qseq66".  This is a constant, set to
     *  SEQ66_APP_NAME, but obtained via the seq_app_name() function.  Do not
     *  confuse it with the client name, which defaults to "seq66" no matter
     *  what the application name.
     */

    const std::string m_application_name;

    /**
     *  New value to allow the user to violate the MIDI specification and use a
     *  track other than the first track (#0) as the MIDI tempo track.
     *  Holds the number of the official tempo track for this performance.
     *  Normally 0, it can be changed to any value from 1 to 1023 via the
     *  tempo-track-number setting in the "rc" file, and that can be overriden
     *  by the c_tempo_track SeqSpec possibly present in the song's MIDI file.
     */

    int m_tempo_track_number;

    /**
     *  Holds a few MIDI file-names most recently used.  Although this is a
     *  vector, we do not let it grow past SEQ66_RECENT_FILES_MAX.
     *
     *  New feature from Oli Kester's kepler34 project.
     *
     * std::vector<std::string> m_recent_files;
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

public:

    rcsettings ();
    rcsettings (const rcsettings & rhs) = default;
    rcsettings & operator = (const rcsettings & rhs) = default;
    virtual ~rcsettings () = default;

    std::string make_config_filespec
    (
        const std::string & base,
        const std::string & ext
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
    std::string palette_filespec () const;
    std::string style_sheet_filespec () const;
    virtual void set_defaults () override;

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

    const mutegroups & mute_groups () const
    {
        return m_mute_groups;
    }

    mutegroups & mute_groups ()
    {
        return m_mute_groups;
    }

    bool load_key_controls () const
    {
        return m_load_key_controls;
    }

    const keycontainer & key_controls () const
    {
        return m_keycontainer;
    }

    keycontainer & key_controls ()
    {
        return m_keycontainer;
    }

    bool load_midi_control_in () const
    {
        return m_load_midi_controls;
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

    bool show_ui_sequence_key () const
    {
        return m_show_ui_sequence_key;
    }

    bool show_ui_sequence_number () const
    {
        return m_show_ui_sequence_number;
    }

    int get_clock_mod () const
    {
        return m_clock_mod;
    }

    bool verbose () const
    {
        return m_verbose;
    }

    bool investigate () const
    {
        return m_investigate;
    }

    bool investigate_disabled () const
    {
        return false;
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

    bool auto_ctrl_save () const
    {
        return m_save_list.get("ctrl");
    }

    bool auto_drums_save () const
    {
        return m_save_list.get("drums");
    }

    bool auto_qss_save () const
    {
        return m_save_list.get("drums");
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

    bool song_start_mode () const
    {
        return m_song_start_mode == sequence::playback::song;
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

    bool filter_by_channel () const
    {
        return m_filter_by_channel;
    }

    bool manual_ports () const
    {
        return m_manual_ports;
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

    portnaming port_naming () const
    {
        return m_port_naming;
    }

    bool is_port_naming_long () const
    {
        return m_port_naming == portnaming::longnames;
    }

    std::string port_naming_string () const;
    std::string port_naming_string (portnaming v) const;

    const std::string & midi_filename () const
    {
        return m_midi_filename;
    }

    void midi_filename (const std::string & value)
    {
        m_midi_filename = value;
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

    void last_used_dir (const std::string & value);

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
        return m_recent_files.add(filename);
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

    const std::string & config_directory () const
    {
        return m_config_directory;
    }

    void set_config_files (const std::string & value);
    bool has_home_config_path (const std::string & name);
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

    bool palette_active () const
    {
        return m_palette_active;
    }

    const std::string & playlist_filename () const;

    const std::string & midi_base_directory () const
    {
        return m_playlist_midi_base;
    }

    const std::string & notemap_filename () const
    {
        return m_notemap_filename;
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

     bool mute_group_active () const
     {
        return m_mute_group_active;
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

    void load_key_controls (bool flag)
    {
        m_load_key_controls = flag;
    }

    void load_midi_control_in (bool flag)
    {
        m_load_midi_controls = flag;
    }

    void midi_control_buss (bussbyte b)
    {
        m_midi_control_buss = b;
    }

    void show_ui_sequence_key (bool flag)
    {
        m_show_ui_sequence_key = flag;
    }

    void show_ui_sequence_number (bool flag)
    {
        m_show_ui_sequence_number = flag;
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

    void verbose (bool flag)
    {
        m_verbose = flag;
    }

    void investigate (bool flag)
    {
        m_investigate = flag;
    }

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

    void auto_drums_save (bool flag)
    {
        m_save_list.set("drums", flag);
    }

    void auto_palette_save (bool flag)
    {
        m_save_list.set("palette", flag);
    }

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

    void pass_sysex (bool flag)
    {
        m_pass_sysex = flag;
    }

    void song_start_mode (bool flag)
    {
        m_song_start_mode = flag ?
            sequence::playback::song : sequence::playback::live ;
    }

    void song_start_mode (const std::string & s);

    void filter_by_channel (bool flag)
    {
        m_filter_by_channel = flag;
    }

    void manual_ports (bool flag)
    {
        m_manual_ports = flag;
    }

    void manual_port_count (int count)
    {
        if (count <= 0 || count > c_output_buss_max)
            count = c_output_buss_default;

        m_manual_port_count = count;
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

    void print_keys (bool flag)
    {
        m_print_keys = flag;
    }

    void midi_control_active (bool flag)
    {
        m_midi_control_active = flag;
    }

    void midi_control_filename (const std::string & name)
    {
       m_midi_control_filename = name;
    }

    void mute_group_active (bool flag)
    {
        m_mute_group_active = flag;
    }

    void mute_group_filename (const std::string & name)
    {
       m_mute_group_filename = name;
    }

    void playlist_active (bool flag)
    {
        m_playlist_active = flag;
    }

    void notemap_active (bool flag)
    {
        m_notemap_active = flag;
    }

    void palette_active (bool flag)
    {
        m_palette_active = flag;
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

    bool interaction_method (int v)
    {
        return interaction_method(static_cast<interaction>(v));
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

    void full_config_directory (const std::string & value, bool addhome = false);
    void config_directory (const std::string & value);
    void config_filename (const std::string & value);
    void playlist_filename (const std::string & value);
    bool playlist_filename_checked (const std::string & value);

    void home_config_directory (const std::string & hcd)
    {
        m_full_config_directory = hcd;
    }

    void user_file_active (bool flag)
    {
        m_user_file_active = flag;
    }

    void user_filename (const std::string & value);

    void notemap_filename (const std::string & fn)
    {
        m_notemap_filename = fn;
    }

    void palette_filename (const std::string & fn)
    {
        m_palette_filename = fn;
    }

    void set_save_list (bool state);
    void set_save (const std::string & name, bool value);

private:

    std::string filespec_helper (const std::string & baseext) const;

};          // class rcsettings

}           // namespace seq66

#endif      // SEQ66_RCSETTINGS_HPP

/*
 * rcsettings.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


/*
 *  This file is part of seq24/seq66.
 *
 *  seq24 is free software; you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option) any later
 *  version.
 *
 *  seq24 is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with seq24; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          rcsettings.cpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  in this application.
 *
 * \library       seq66 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2015-09-22
 * \updates       2025-05-31
 * \license       GNU GPLv2 or above
 *
 *  Note that this module also sets the legacy global variables, so that
 *  they can be used by modules that have not yet been cleaned up.
 *
 * \warning
 *      No more "statistics" support.
 */

#include <algorithm>                    /* std::find()                      */

#include "cfg/settings.hpp"             /* seq66::rc(), seq66::usr()        */
#include "play/seq.hpp"                 /* seq66::seq::maximum()            */
#include "util/filefunctions.hpp"       /* make_directory(), etc.           */
#include "util/strfunctions.hpp"        /* seq66::strncompare()             */

#if defined SEQ66_KEEP_RC_FILE_LIST
#include <utility>                      /* std::make_pair()                 */
#endif

namespace seq66
{

/**
 *  Default constructor.
 */

rcsettings::rcsettings () :
    basesettings                (),
#if defined SEQ66_KEEP_RC_FILE_LIST
    m_config_files              (),         /* vector of file-names         */
#endif
    m_clocks                    (),         /* vector wrapper class         */
    m_inputs                    (),         /* vector wrapper class         */
    m_metro_settings            (),
    m_mute_group_save           (mutegroups::saving::midi),
    m_keycontainer              ("rc"),
    m_drop_empty_in_controls    (false),    /* the legacy value             */
    m_midi_control_buss         (null_buss()),
    m_midi_control_in           ("rc"),
    m_midi_control_out          ("rc"),
    m_clock_mod                 (64),
    m_verbose                   (false),
    m_quiet                     (false),
    m_investigate               (false),
    m_session_tag               (),
    m_save_list                 (),         /* std::map<string, bool>       */
    m_save_old_triggers         (false),
    m_save_old_mutes            (false),
    m_allow_mod4_mode           (false),
    m_allow_snap_split          (false),
    m_allow_click_edit          (true),
    m_show_midi                 (false),
    m_priority                  (false),
    m_thread_priority           (0),        /* c_thread_priority            */
    m_pass_sysex                (false),
    m_with_jack_transport       (false),
    m_with_jack_master          (false),
    m_with_jack_master_cond     (false),
#if defined SEQ66_RTMIDI_SUPPORT
    m_with_jack_midi            (true),     /* tentative                    */
#else
    m_with_jack_midi            (false),
#endif
    m_with_alsa_midi            (false),    /* unless ALSA gets selected    */
    m_jack_auto_connect         (true),
    m_jack_use_offset           (true),
    m_jack_buffer_size          (0),
    m_song_start_mode           (sequence::playback::automatic),
    m_song_start_is_auto        (true),
    m_record_by_buss            (false),
    m_record_by_channel         (false),
    m_manual_ports              (false),
    m_manual_auto_enable        (false),
    m_manual_port_count         (c_output_buss_default),
    m_manual_in_port_count      (c_input_buss_default),
    m_reveal_ports              (false),
    m_init_disabled_ports       (false),
    m_print_keys                (false),
    m_interaction_method        (interaction::seq24),
    m_sets_mode                 (setsmode::normal),
    m_port_naming               (portname::brief),
    m_midi_filename             (),
    m_midi_filepath             (),
    m_running_status_action     (rsaction::recover),
    m_jack_session_uuid         (),
    m_jack_session_active       (false),
    m_last_used_dir             (),                     /* double_quotes()  */
    m_session_directory         (),
    m_config_subdirectory_set   (false),
    m_config_subdirectory       (),
    m_config_filename           (seq_config_name()),    /* updated in body  */
    m_full_config_directory     (),
    m_user_file_active          (true),                 /* keep it true     */
    m_user_filename             (seq_config_name()),    /* updated in body  */
    m_midi_control_active       (false),
    m_midi_control_filename     (seq_config_name()),    /* updated in body  */
    m_mute_group_file_active    (false),
    m_mute_group_filename       (seq_config_name()),    /* updated in body  */
    m_playlist_active           (false),
    m_playlist_filename         (seq_config_name()),    /* updated in body  */
    m_playlist_midi_base        (),
    m_notemap_active            (false),
    m_notemap_filename          (seq_config_name()),    /* updated in body  */
    m_patches_active            (false),
    m_patches_filename          (seq_config_name()),    /* updated in body  */
    m_palette_active            (false),
    m_palette_filename          (seq_config_name()),    /* updated in body  */
    m_style_sheet_active        (false),
    m_style_sheet_filename      (seq_config_name()),
    m_application_name          (seq_app_name()),
    m_tempo_track_number        (0),
    m_recent_files              (),
    m_load_most_recent          (true),
    m_full_recent_paths         (false),
    m_portmaps_present          (false),
    m_portmaps_active           (false)
{
    m_session_directory = user_session(seq_config_dir_name());
    m_midi_control_in.inactive_allowed(true);
    m_config_filename += ".rc";
    m_user_filename += ".usr";
    m_midi_control_filename += ".ctrl";
    m_mute_group_filename += ".mutes";
    m_playlist_filename += ".playlist";
    m_notemap_filename += ".drums";
    m_patches_filename += ".patches";
    m_palette_filename += ".palette";
    m_style_sheet_filename += ".qss";
    set_config_files(seq_config_name());                /* ca 2023-05-11    */
}

/**
 *  Sets the default values.
 *
 * Not altered:
 *
 *      m_clocks.clear();
 *      m_inputs.clear();
 *      m_mute_groups.clear();
 *      m_keycontainer.clear();              // what is best?
 *      m_midi_control_in.clear();           // what is best?
 *      m_midi_control_out.clear();          // does not exist
 */

void
rcsettings::set_defaults ()
{
    /*
     * basesettings
     * m_clocks
     * m_inputs
     * m_keycontainer
     * m_midi_control_in
     * m_midi_control_out
     */

    m_metro_settings.set_defaults();
    m_mute_group_save           = mutegroups::saving::midi;
    m_drop_empty_in_controls    = false;
    m_midi_control_buss         = null_buss();
    m_clock_mod                 = 64;
    m_verbose                   = false;
    m_quiet                     = false;
    m_investigate               = false;
    m_session_tag.clear();
    m_save_old_triggers         = false;
    m_save_old_mutes            = false;
    m_allow_mod4_mode           = false;
    m_allow_snap_split          = false;
    m_allow_click_edit          = true;
    m_show_midi                 = false;
    m_priority                  = false;
    m_thread_priority           = 0;        /* c_thread_priority            */
    m_pass_sysex                = false;
    m_with_jack_transport       = false;
    m_with_jack_master          = false;
    m_with_jack_master_cond     = false;
#if defined SEQ66_RTMIDI_SUPPORT
    m_with_jack_midi            = true;
#else
    m_with_jack_midi            = false;
#endif
    m_with_alsa_midi            = false;    /* unless ALSA gets selected    */
    m_jack_auto_connect         = true;
    m_jack_use_offset           = true;
    m_jack_buffer_size          = 0;
    m_song_start_mode           = sequence::playback::automatic;
    m_song_start_is_auto        = true;
    m_record_by_buss            = false;
    m_record_by_channel         = false;
    m_manual_ports              = false;
    m_manual_auto_enable        = false;
    m_manual_port_count         = c_output_buss_default;
    m_manual_in_port_count      = c_input_buss_default;
    m_reveal_ports              = false;
    m_init_disabled_ports       = false;
    m_print_keys                = false;
    m_interaction_method        = interaction::seq24;
    m_sets_mode                 = setsmode::normal;
    m_port_naming               = portname::brief;
    m_midi_filename.clear();
    m_midi_filepath.clear();
    m_running_status_action     = rsaction::recover;
    m_jack_session_uuid.clear();
    m_jack_session_active       = false;
    m_last_used_dir.clear();                /* double_quotes()              */
    m_session_directory         = user_session(seq_config_dir_name());
    m_config_subdirectory_set   = false;
    m_config_subdirectory.clear();
    m_config_filename           = seq_config_name();
    m_full_config_directory.clear();
    m_user_file_active = true;
    m_user_filename = seq_config_name();
    m_midi_control_active = false;
    m_midi_control_filename = seq_config_name();
    m_mute_group_file_active = false;
    m_mute_group_filename = seq_config_name();
    m_playlist_active = false;
    m_playlist_filename = seq_config_name();
    m_playlist_midi_base.clear();
    m_notemap_active = false;
    m_notemap_filename = seq_config_name();
    m_patches_active = false;
    m_patches_filename = seq_config_name();
    m_palette_active = false;
    m_palette_filename = seq_config_name();
    m_style_sheet_active = false;
    m_style_sheet_filename = seq_config_name();
    m_config_filename += ".rc";
    m_user_filename += ".usr";
    m_midi_control_filename += ".ctrl";
    m_mute_group_filename += ".mutes";
    m_playlist_filename += ".playlist";
    m_notemap_filename += ".drums";
    m_patches_filename += ".patches";
    m_palette_filename += ".palette";
    m_style_sheet_filename += ".qss";

    /*
     * const: m_application_name = seq_app_name();
     */

    m_tempo_track_number = 0;
    m_recent_files.clear();
    m_load_most_recent = true;
    m_full_recent_paths = false;
    m_portmaps_present = false;
    m_portmaps_active = false;
    set_config_files(seq_config_name());
    set_save_list(false);
}

/**
 *  We no longer save the 'rc' every damn time.
 */

void
rcsettings::set_save_list (bool state)
{
    m_save_list.clear();
    m_save_list.add("rc", state);               /* can be edited in UI  */
    m_save_list.add("usr", state);              /* can be edited in UI  */
    m_save_list.add("mutes", state);            /* can be edited in UI  */
    m_save_list.add("playlist", state);         /* can be edited in UI  */

    /*
     * The following are saved only after the first run.  Thereafter, they
     * are managed by the user. Seq66 offers no way to edit these
     * files, nor the 'palette' file. The palette in place can be saved
     * via a special button in Edit / Preferences / Session.
     */

    m_save_list.add("palette", state);          /* can save via button  */
    m_save_list.add("drums", state);
    m_save_list.add("ctrl", state);
    m_save_list.add("qss", state);
}

/**
 *  This function is useful when importing a session into NSM. It prevents
 *  any saves when exiting the application.
 *
 *  What about the "session" file?
 */

void
rcsettings::disable_save_list ()
{
    m_save_list.clear();
    m_save_list.add("rc", false);
    m_save_list.add("usr", false);
    m_save_list.add("mutes", false);
    m_save_list.add("playlist", false);
    m_save_list.add("palette", false);
    m_save_list.add("drums", false);
    m_save_list.add("ctrl", false);
    m_save_list.add("qss", false);
}

void
rcsettings::set_save (const std::string & name, bool value)
{
    bool status = m_save_list.get(name);
    if (status != value)
        m_save_list.set("rc", true);        /* 'rc' holds all the status    */

    m_save_list.set(name, value);
}

bool
rcsettings::auto_options_save () const
{
    return
    (
        auto_rc_save() || auto_usr_save() ||
        is_modified() || usr().is_modified()
    );
}

void
rcsettings::verbose (bool flag)
{
    m_verbose = flag;
    set_verbose(flag);                      /* from the basic_macros module */
}

void
rcsettings::investigate (bool flag)
{
    m_investigate = flag;
    set_investigate(flag);                  /* from the basic_macros module */
}

/**
 *  After importing a playlist, call this function to make it permanent (but
 *  will still need to reload the sesson).
 */

void
rcsettings::set_imported_playlist
(
    const std::string & sourcepath,
    const std::string & midipath
)
{
    playlist_active(true);
    playlist_filename(filename_base(sourcepath));
    midi_base_directory(midipath);
    auto_playlist_save(true);
    auto_rc_save(true);
}

void
rcsettings::auto_rc_save (bool flag)
{
    m_save_list.set("rc", flag);
    if (flag)
        modify();
}

void
rcsettings::set_jack_transport (const std::string & value)
{
    if (value == "slave")
    {
        with_jack_transport(true);
    }
    else if (value == "master")
    {
        with_jack_transport(true);
        with_jack_master(true);
    }
    else if (value == "conditional")
    {
        with_jack_transport(true);
        with_jack_master_cond(true);
    }
    else
    {
        with_jack_transport(false);
        with_jack_master(false);
        with_jack_master_cond(false);
    }
}

/**
 *  Song-start mode.  Was boolean, but now can be set to a value that
 *  determines the mode based on the file being loaded having triggers, or
 *  not. This should be used only by the rcfile class.
 */

std::string
rcsettings::song_mode_string () const
{
    std::string result;
    switch (m_song_start_mode)
    {
        case sequence::playback::live:      result = "live";    break;
        case sequence::playback::song:      result = "song";    break;
        case sequence::playback::automatic: result = "auto";    break;
        default:                            result = "unknown"; break;
    }
    if (m_song_start_is_auto)                   /* "auto" read from 'rc'?   */
        result = "auto";

    return result;
}

void
rcsettings::song_start_mode_by_string (const std::string & s)
{
    if (s == "song" || s == "true")
    {
        m_song_start_mode = sequence::playback::song;
        m_song_start_is_auto = false;
    }
    else if (s == "live" || s == "false")
    {
        m_song_start_mode = sequence::playback::live;
        m_song_start_is_auto = false;
    }
    else
    {
        m_song_start_mode = sequence::playback::automatic;
        m_song_start_is_auto = true;
    }
}

/**
 *  The record-by settings are loaded from the 'rc' file at startup.
 *  See sequence_lookup_support() as well.
 */

void
rcsettings::record_by_buss (bool flag)
{
    m_record_by_buss = flag;
    if (flag)
        m_record_by_channel = false;
}

void
rcsettings::record_by_channel (bool flag)
{
    if (record_by_buss())
        m_record_by_channel = false;
    else
        m_record_by_channel = flag;
}

/**
 *  Holds the client name for the application.  This is much like the
 *  application name, but in the future will be a configuration option.
 *  For now it is just the value returned by the seq_client_name()
 *  function.  However, note that, under session management, we replace it
 *  with the NSM "client_id" value.
 */

const std::string &
rcsettings::app_client_name () const
{
    return seq_client_name();
}

void
rcsettings::app_client_name (const std::string & n) const
{
    set_client_name(n);
}

std::string
rcsettings::default_session_path () const
{
    std::string result = user_home();
    if (is_empty_string(result))                // if (result.empty())
    {
        result += session_directory();          /* seq66 directory      */
    }
    else
    {
        if (name_has_root_path(session_directory()))
            result = session_directory();       /* seq66 directory      */
        else
            result = pathname_concatenate(result, session_directory());
    }
    return result;
}

/**
 *  Provides the directory for the configuration file, and also creates the
 *  directory if necessary.
 *
 *  The home directory is (in Linux) "/home/username/.config/seq66", and
 *  the configuration file is "qseq66.rc".
 *
 *  This function should also adapt to Windows conventions automatically.
 *  No, it does not. But all we have to do is replace Window's HOMEPATH with
 *  its LOCALAPPDATA value.
 *
 * \return
 *      Returns the selected home configuration directory.  If it does not
 *      exist, or could not be created, then an empty string is returned.
 */

std::string
rcsettings::home_config_directory () const
{
    if (m_full_config_directory.empty())
    {
        std::string home = default_session_path();
        std::string result = home;
        if (home.empty())
        {
            std::string temp = "Cannot find HOME!";
            result = set_error_message(temp);
        }
        else
        {
            if (m_config_subdirectory_set)
                result = pathname_concatenate(result, m_config_subdirectory);

            bool ok = make_directory_path(result);
            if (ok)
            {
                result = normalize_path(result);
                m_full_config_directory = result;
            }
            else
            {
                file_error("Create fail", result);
                result.clear();
            }
        }
        return result;
    }
    else
        return normalize_path(m_full_config_directory);
}

/**
 *  Checks to see if the provided string matches the home configuration
 *  directory name, up to the length of that directory name.
 *
 * \param name
 *      Provides the name to be checked.
 *
 * \return
 *      Returns true if the name matches the home directory path.
 */

bool
rcsettings::has_home_config_path (const std::string & name)
{
    return strncompare(name, home_config_directory());
}

/**
 *  Conditionally trims a file-specification of its home-directory path.  This
 *  function is meant to be used for more flexible specification of the
 *  specified 'ctrl', 'mutes', and 'playlist' files.  If there is no
 *  directory, then the file is in the current configuration directory.  The
 *  same is true if the home directory is present, but we don't need it.  If
 *  there is another directory, whether relative or not, we need to keep it.
 */

std::string
rcsettings::trim_home_directory (const std::string & filepath)
{
    if (has_home_config_path(filepath))
    {
        std::string path;
        std::string result;
        (void) filename_split(filepath, path, result);
        return result;
    }
    else
        return filepath;
}

/**
 *  Moved here from cmdlineopts.
 *
 *  If no "rc" file exists, then, of course, we will create them upon
 *  exit.  But we also want to force the new style of output, where the
 *  key/MIDI controls and the mute-groups are in separate files ending
 *  with the extensions "ctrl" and "mutes", respectively.  Also, the
 *  mute-groups container is populated with zero values.  Finally, the
 *  user may have specified an altername configuration-file name on
 *  the command-line (e.g. "myseq66" versus the default, "qseq66",
 *  which is the application name.
 *
 *      std::string cfgname = rc().application_name();
 */

void
rcsettings::create_config_names (const std::string & base)
{
    std::string cfgname = base.empty() ? config_filename() : base ;
    cfgname = filename_base(cfgname, true);             /* strip extension  */

    std::string cc = file_extension_set(cfgname, ".rc");
    std::string uf = file_extension_set(cfgname, ".usr");
    std::string cf = file_extension_set(cfgname, ".ctrl");
    std::string mf = file_extension_set(cfgname, ".mutes");
    std::string pl = file_extension_set(cfgname, ".playlist");
    std::string nm = file_extension_set(cfgname, ".drums");
    std::string pa = file_extension_set(cfgname, ".palette");
    std::string af = cfgname + "rc,ctrl,midi,mutes,drums,playlist,palette";
    config_filename(cc);
    user_filename(uf);
    midi_control_filename(cf);
    mute_group_filename(mf);
    playlist_filename(pl);
    notemap_filename(nm);
    palette_filename(pa);
    file_message("Configuration files", af);
}

/**
 *  We need a way to handle the following configuration-file paths:
 *
 *      -   "base.ext":              Prepend the HOME directory.
 *      -   "subdirectory/base.ext": Prepend the HOME directory
 *      -   "/path/.../base.ext":    Use the path as is.
 *      -   "C:/path/.../base.ext":  Use the path as is.
 *
 *  Compare to make_config_filespec(), used only by rcfile and this class.
 *  That one treats the base and extension separately.
 */

std::string
rcsettings::filespec_helper (const std::string & baseext) const
{
    std::string result = baseext;
    if (! result.empty())
    {
        bool use_as_is = false;
        if (name_has_path(baseext))
        {
            if (name_has_root_path(baseext))
                use_as_is = true;
        }
        if (! use_as_is)
            result = filename_concatenate(home_config_directory(), baseext);;

        result = normalize_path(result);            /* change to UNIX slash */
    }
    return result;
}

/**
 *  Guarantees that a file-name is of the simple form "basename.ext".
 *
 * \param filename
 *      The full file-specification of a file, or the basename, or a basename
 *      without any extension.
 */

std::string
rcsettings::filename_base_fix
(
    const std::string & filename,
    const std::string & ext
) const
{
    std::string result = filename_base(filename, true); /* strip the .ext   */
    result = file_extension_set(result, ext);
    return result;
}

/**
 *  Constructs a full path and file specification for the caller.
 *
 * \todo
 *      Need to add support for "C:" and Windows backslash, but let's work out
 *      the other problems first.
 *
 * \param base
 *      The part of the filename after the path but before the extension.
 *      If it has a path, then a full file-name is assumed.  If the path is
 *      relative (the first character is not a slash), then the home
 *      configuration directory is still prepended.
 *
 * \param ext
 *      The file extension, including the period (which is optional anyway).
 *      If the \a base parameter has a period, then the extension is ignored.
 *
 * \return
 *      Returns the full path/base.ext file specification.  If the base name
 *      contains a "/" and a ".", it is returned as is, presumably already valid.
 *      This feature allows the user to use a full-path to the file, and
 *      relative paths.  It is not recommended to use relative paths.  Maybe we
 *      can make the relative to the configuration directory.
 */

std::string
rcsettings::make_config_filespec
(
    const std::string & base,
    const std::string & ext
) const
{
    std::string result = base;                          /* bugfix 23-07-17  */
    auto pos = base.find_last_of(".");
    if (pos != std::string::npos)                       /* base.extension   */
    {
        pos = result.find_first_of("/");
        if (pos != std::string::npos)                   /* slash in base    */
        {
            if (pos > 0)                                /* not at start     */
                result = filename_concatenate(home_config_directory(), base);
        }
        else
            result = filename_concatenate(home_config_directory(), base);
    }
    else
    {
        std::string extension = ext;
        auto pos = ext.find_first_of(".");
        bool add_ext_dot =
        (
            pos == std::string::npos ||                 /* ext has no dot   */
            pos > 0                                     /* or not at start  */
        );
        if (add_ext_dot)
        {
            extension = ".";                            /* add the dot      */
            extension += ext;
        }
        result = home_config_directory();
        result += base;
        result += extension;
    }
    return result;
}

/**
 *  Constructs the full path and file specification for the "rc" file.
 *  If the file-name includes a path, then that is returned as is.
 *  See filespec_helper().
 *
 * \return
 *      If home_config_directory() returns a non-empty string, then the
 *      normal "rc" configuration file-name is appended to that result, and
 *      returned.  Otherwise, an empty string is returned.
 */

std::string
rcsettings::config_filespec () const
{
    return filespec_helper(config_filename());
}

/**
 *  Constructs an alternate full path and file specification for the "rc"
 *  file.  This function is useful in writing to an alternate "rc" file when a
 *  fatal error occurs.  Also note that the configuration file specification
 *  can never be empty or blank (equal to this string: "").  See the
 *  strfunctions module's is_empty_string() function.
 *
 * \param altname
 *      Provides the base-name of the alternate file, including the extension.
 *      Examples are "erroneous.rc" or "mydumpfile.log".
 *
 * \return
 *      If home_config_directory() returns a non-empty string, the alternate
 *      normal "rc" configuration file-name is appended to that result, and
 *      returned.  Otherwise, an empty string is returned.
 */

std::string
rcsettings::config_filespec (const std::string & altname) const
{
    return filespec_helper(altname);
}

/**
 *  Constructs the full path and file specification for the "user" file.
 *
 * \return
 *      If home_config_directory() returns a non-empty string, then the
 *      normal "user" configuration file-name is appended to that result,
 *      and returned.  Otherwise, an empty string is returned.
 */

std::string
rcsettings::user_filespec () const
{
    return filespec_helper(user_filename());
}

/**
 *  Constructs an alternate full path and file specification for the "usr"
 *  file.  This function is useful in writing to an alternate "usr" file when
 *  a fatal error occurs.
 *
 * \param altname
 *      Provides the base-name of the alternate file, including the extension.
 *      Example:  "erroneous.usr".
 *
 * \return
 *      If home_config_directory() returns a non-empty string, alternate
 *      normal "usr" configuration file-name is appended to that result, and
 *      returned.  Otherwise, an empty string is returned.
 */

std::string
rcsettings::user_filespec (const std::string & altname) const
{
    return filespec_helper(altname);
}

/**
 *  Constructs the full path and file specification for the "playlist" file,
 *  if necessary, if the playlist file-name exists.  If the playlist file-name
 *  already has a directory as part of its name, then it is used as is.
 *
 * \return
 *      Returns either the filename itself, if it already has a directory in
 *      it.  Otherwise, home_config_directory() is retrieved, the
 *      "playlist" file-name is appended to that result, and returned.
 *      Otherwise, an empty string is returned.  Don't use it!
 */

std::string
rcsettings::playlist_filespec () const
{
    return filespec_helper(playlist_filename());
}

/**
 *  Constructs the note-mapper filespec.
 */

std::string
rcsettings::notemap_filespec () const
{
    return filespec_helper(notemap_filename());
}

/**
 *  Constructs the patches configuration filespec.
 */

std::string
rcsettings::patches_filespec () const
{
    return filespec_helper(patches_filename());
}

/**
 *  Constructs the palette configuration filespec.
 */

std::string
rcsettings::palette_filespec () const
{
    return filespec_helper(palette_filename());
}

/**
 *  Constructs the style-sheet filespec.  The base name is, however, part of
 *  the 'usr' configuration.  If empty, then this function returns and empty
 *  string.
 */

std::string
rcsettings::style_sheet_filespec () const
{
    return filespec_helper(style_sheet_filename());
}

/**
 *  Constructs the full path and file specification for a "MIDI control" file.
 *
 * \return
 *      Returns either the filename itself, if it already has a directory in
 *      it.  Otherwise, home_config_directory() is retrieved, the
 *      "MIDI control" file-name is appended to that result, and returned.
 *      Otherwise, an empty string is returned.  Don't use it!
 */

std::string
rcsettings::midi_control_filespec () const
{
    return filespec_helper(midi_control_filename());
}

/**
 *  Constructs the full path and file specification for a "mute group" file.
 *
 * \return
 *      Returns either the filename itself, if it already has a directory in
 *      it.  Otherwise, home_config_directory() is retrieved, the
 *      "mute group" file-name is appended to that result, and returned.
 *      Otherwise, an empty string is returned.  Don't use it!
 */

std::string
rcsettings::mute_group_filespec () const
{
    return filespec_helper(mute_group_filename());
}

void
rcsettings::tempo_track_number (int track)
{
    if (track < 0 || track >= seq::maximum())
        track = 0;

    m_tempo_track_number = track;
}

/**
 * \getter m_recent_files
 *
 *  Gets the desired recent MIDI file-name, if present.
 *
 * \param index
 *      Provides the desired index into the recent-files vector.
 *
 * \param shorten
 *      If true, remove the path-name from the file-name.  True by default.
 *      It needs to be short for the menu entry (though now using the full
 *      path is an option), but the full path-name for the "rc" file.
 *
 * \return
 *      Returns m_recent_files[index], perhaps shortened.  An empty string is
 *      returned if there is no such animal.
 */

std::string
rcsettings::recent_file (int index, bool shorten) const
{
    std::string result = m_recent_files.get(index);
    if (shorten && ! result.empty())
    {
        auto slashpos = result.find_last_of("/\\");
        if (slashpos != std::string::npos)
            result = result.substr(slashpos + 1, std::string::npos);
    }
    return result;
}

/**
 * \setter m_recent_files
 *
 *  First makes sure the filename is not already present, and removes
 *  the back entry from the list, if it is full (SEQ66_RECENT_FILES_MAX)
 *  before adding it.  Now the full pathname is added.
 *
 *  ca 2025-05-31. Even if the file is already in the list, we
 *  want it to be moved to the top, which recent::add() does for us.
 *
 * \param fname
 *      Provides the full path to the MIDI file that is to be added to
 *      the recent-files list.
 *
 * \return
 *      Returns true if the file-name was able to be added.
 *      If false, the file-name might already be in the list, so no need
 *      to update the UI representing the list.
 */

bool
rcsettings::add_recent_file (const std::string & filename)
{
    std::string path = get_full_path(normalize_path(filename));
    bool result = ! filename.empty();  /* ! m_recent_files.is_in_list(path) */
    if (result)
    {
        result = m_recent_files.add(filename);
        if (result)
            auto_rc_save(true);                 /* fix on 2023-04-09 by ca  */
    }
    return result;
}

/**
 * \setter m_interaction_method
 *
 * \param value
 *      The value to use to make the setting.
 *
 * \return
 *      Returns true if the value was legal.
 */

bool
rcsettings::interaction_method (interaction value)
{
    bool result = false;
    switch (value)
    {
    case interaction::seq24:
    case interaction::fruity:

        m_interaction_method = value;
        result = true;
        break;

    default:

        errprint("illegal interaction-method value");
        break;
    }
    return result;
}

/**
 *  Prepends the exisiting m_midi_filepath value (if not empty) to the current
 *  value and stores it as the new m_midi_filename value.  This function is
 *  meant to be used mainly under session management, but remember that the
 *  session MIDI file-name is the same entity as the regular MIDI file-name.
 *
 * \param value
 *      Provides the base name for the MIDI file, such as "mytune.midi".  If
 *      the ".midi" isn't provided (only the "." is searched), it will be
 *      appended.  If empty, the m_midi_filename value is cleared.
 */

void
rcsettings::session_midi_filename (const std::string & value)
{
    if (value.empty())
    {
        m_midi_filename.clear();
    }
    else
    {
        std::string base = file_extension_set(value, ".midi");
        if (! m_midi_filepath.empty())
        {
            std::string path = filename_concatenate(m_midi_filepath, base);
            m_midi_filename = path;
        }
        else
            m_midi_filename = base;
    }
}

/**
 * \setter m_jack_session_uuid
 *
 *  This is an FYI-only data item that is controlled by JACK, and should not
 *  be modified by the user. See https://jackaudio.org/metadata/ for more
 *  information.
 *
 * \param uuid
 *      The value to use to make the setting. If set to "on", it can enable
 *      JACK session usage.  "off" disables it.  The session manager will
 *      supply a long integer value (as a string).
 */

void
rcsettings::jack_session (const std::string & uuid)
{
    bool save_config = true;
    bool clear_uuid = true;
    if (uuid.empty())
    {
        save_config = false;
    }
    else
    {
        if (uuid == "on")
        {
            usr().session_manager("jack");
        }
        else if (uuid == "off")
        {
            usr().session_manager("none");
        }
        else
        {
            usr().session_manager("jack");
            m_jack_session_uuid = uuid;
            save_config = clear_uuid = false;
        }
    }
    if (save_config)
        auto_usr_save(true);

    if (clear_uuid)
        m_jack_session_uuid.clear();
}

/**
 *  Sets the method to use for handling running status errors.
 */

void
rcsettings::running_status_action (const std::string & v)
{
    if (v == "skip")
        m_running_status_action = rsaction::skip;
    else if (v == "proceed")
        m_running_status_action = rsaction::proceed;
    else if (v == "abort")
        m_running_status_action = rsaction::abort;
    else
        m_running_status_action = rsaction::recover;
}

std::string
rcsettings::running_status_action_name () const
{
    std::string result = "recover";
    if (m_running_status_action == rsaction::skip)
        result = "skip";
    else if (m_running_status_action == rsaction::proceed)
        result = "proceed";
    else if (m_running_status_action == rsaction::abort)
        result = "abort";

    return result;
}

/**
 * \param value
 *      The value to use to make the setting.  It needs to be a directory, not
 *      a file.  Also, we now expand a relative directory to the full path to
 *      that directory, to avoid ambiguity should the application be run from
 *      a different directory.
 *
 * \param userchange
 *      If true (the default), then the 'rc' file needs to be save.
 *      If false, we are loading the 'rc' file, so no change in status
 *      is necessary.
 */

void
rcsettings::last_used_dir (const std::string & value, bool userchange)
{
    if (value.empty())
        m_last_used_dir = empty_string();           /* "" from strfunctions */
    else
    {
#if defined USE_OLD_CODE
        std::string last = get_full_path(value);    /* might end up empty   */
        if (last != m_last_used_dir)                /* new directory?       */
        {
            m_last_used_dir = get_full_path(value); /* might end up empty   */
            if (userchange)
                auto_rc_save(true);                 /* need to write it     */
        }
#else
        std::string last = filename_path(value);    /* might end up empty   */
        if (last != m_last_used_dir)                /* new directory?       */
        {
            m_last_used_dir = last;
            if (userchange)
                auto_rc_save(true);                 /* need to write it     */
        }
#endif
    }
}

/**
 * \setter m_session_directory
 *
 *      The default value of this member is that returned by the
 *      user_session() function, either ".config" or "AppData/Local", with
 *      the "seq66" or the result of seq_client_name() appended.
 *
 * \param value
 *      The value to use to make the setting.  Currently, we do not handle
 *      relative paths.  To do so seems... iffy.
 */

void
rcsettings::session_directory (const std::string & value)
{
    if (! value.empty())
        m_session_directory = value;
}

/**
 * \setter m_config_subdirectory
 *
 *  This value is one of the ways to implement the --home option. It
 *  works if there is no root to the value parameter. It should only
 *  work once (when m_config_subdirectory has not yet been set).
 *
 * \param value
 *      The value to use to make the setting. This should be a relative path.
 *      If empty (should be a rare use case), then the config subdirectory
 *      is cleared.
 */

void
rcsettings::config_subdirectory (const std::string & value)
{
    if (value.empty())
    {
        m_config_subdirectory_set = false;
        m_config_subdirectory.clear();
    }
    else
    {
        if (! m_config_subdirectory_set)
        {
            m_config_subdirectory_set = true;
            m_config_subdirectory = value;
        }
    }
}

/**
 *  New processing (2023-03-31) of the -H, --home option and of setting the
 *  sub-directory for a session manager.
 *
 *  If the name has a root path (either the user's home directory or a full
 *  path from root), then the configuration directory becomes the value
 *  parameter. Otherwise, we merely set the sub-directory for later use.
 */

void
rcsettings::set_config_directory (const std::string & value)
{
    bool rooted = name_has_root_path(value);
    if (rooted)
    {
        /*
         * Incorrect: full_config_directory(value) reads, but doesn't set
         * home_config_directory(). So we set it here. Note that
         * home_config_directory() normalizes the path it returns.
         */

        m_full_config_directory = value;;

        std::string homedir = home_config_directory();
        if (make_directory_path(homedir))
        {
            /*
             * This setting will convert the relative session directory
             * to a full path.
             */

            file_message("Config directory", homedir);
            session_directory(homedir);
        }
        else
            file_error("Could not create", homedir);
    }
    else
    {
        config_subdirectory(value);
    }
}

/**
 * \setter m_full_config_directory
 *
 *  Provides an alternate value to be returned by the
 *  home_config_directory() function. Please note that all configuration
 *  locates are relative to "home".
 *
 *  This causes double concatenation. But we need to call it just
 *  once in the case where NSM has changed the default configuration
 *  directory.
 *
 * \param value
 *      Provides the directory name, which should be an actual full path.
 */

void
rcsettings::full_config_directory (const::std::string & value)
{
    if (! value.empty())
    {
        std::string tv = value;
        if (m_config_subdirectory_set)              /* see the banner note  */
        {
            tv = pathname_concatenate(tv, m_config_subdirectory);
            m_config_subdirectory_set = false;
            m_full_config_directory = normalize_path(tv, true, true);
        }

        std::string homedir = home_config_directory();
        if (make_directory_path(homedir))                   // REDUNDANT
        {
            /*
             * This setting will convert the relative session directory
             * to a full path.
             */

            file_message("Config directory", homedir);
            session_directory(homedir);
        }
        else
            file_error("Could not create", homedir);
    }
}

/**
 * \setter m_config_filename and m_user_filename
 *
 *      Implements the --config option to change both configuration files
 *      ("rc" and "usr") with one option.
 *
 *      What about the "ctrl", "playlist", "mutes", "notemap", and "palette"
 *      files?  They are defined in the "rc" file.
 *
 * \param value
 *      The value to use to make the setting, if the string is not empty.
 *      If the value has an extension, it is stripped first.
 *
 *      TODO: use value = file_extension_set(value);
 */

void
rcsettings::set_config_files (const std::string & value)
{
    if (! value.empty())
    {
        size_t ppos = value.rfind(".");
        std::string basename;
        if (ppos != std::string::npos)
            basename = value.substr(0, ppos);   /* strip after first period */
        else
            basename = value;

        config_filename(basename);
        user_filename(basename);
    }
}

/**
 * \setter m_config_filename ("rc"), which is the base-name of this file.
 *
 * \param value
 *      The value to use to make the setting, if the string is not empty.
 *      If there is no period in the string, then ".rc" is appended to the
 *      end of the filename.
 */

void
rcsettings::config_filename (const std::string & value)
{
    if (value.empty())                /* an 'rc' file must always be used */
    {
        // This should never happen
    }
    else
    {
        m_config_filename = filename_base_fix(value, ".rc");
#if defined SEQ66_KEEP_RC_FILE_LIST
        (void) add_config_filespec
        (
            "rc", filespec_helper(m_config_filename)
        );
#endif
    }
}

/**
 * \setter m_playlist_filename ("playlist")
 *
 *  Let the caller take care of this: m_playlist_active = true;
 *
 * \param value
 *      The value to use to make the setting, if the string is not empty.
 *      If there is no period in the string, then ".playlist" is appended to
 *      the end of the filename.
 */

void
rcsettings::playlist_filename (const std::string & value)
{
    if (is_empty_string(value))         /* TODO: use this check elsewhere!! */
    {
        clear_playlist();               /* clears file-name and active flag */
    }
    else
    {
        m_playlist_filename = filename_base_fix(value, ".playlist");
#if defined SEQ66_KEEP_RC_FILE_LIST
        (void) add_config_filespec
        (
            "playlist", filespec_helper(m_playlist_filename)
        );
#endif
    }
}

/**
 *  Same as the playlist_filename() setter, but also checks for file existence
 *  to help the caller decide if the playlist is active.
 *
 * \param value
 *      The base-name of the playlist.
 *
 * \return
 *      Returns true if the file-name was valid and the file (in the seq66
 *      configuration directory) exists.  Otherwise returns false, but the
 *      filename is still set if value.
 */

bool
rcsettings::playlist_filename_checked (const std::string & value)
{
    bool result = false;
    if (is_empty_string(value))
    {
        playlist_filename(value);       /* will inactivate & clear playlist */
    }
    else
    {
        std::string fname = make_config_filespec(value, ".playlist");
        result = file_exists(fname);
        playlist_filename(value);       /* set playlist name no matter what */
    }
    return result;
}

/**
 *  Clears the play-list file-name and flags that the play-list is not active.
 *
 * \param disable
 *      If true, the file-name is instead set to a value that indicates there
 *      was a file-name, but the file does not exist.
 */

void
rcsettings::clear_playlist (bool disable)
{
    playlist_active(false);
    if (disable)
        m_playlist_filename = questionable_string();
    else
        m_playlist_filename.clear();
}

/**
 * \setter m_user_filename ("usr")
 *
 * \param value
 *      The value to use to make the setting, if the string is not empty.
 *      If there is no period in the string, then ".usr" is appended to the
 *      end of the filename.
 */

void
rcsettings::user_filename (const std::string & value)
{
    if (value.empty())                  /* always need a 'usr' file to read */
    {
        user_file_active(false);        /* this should never happen         */
    }
    else
    {
        m_user_filename = filename_base_fix(value, ".usr");
#if defined SEQ66_KEEP_RC_FILE_LIST
        (void) add_config_filespec
        (
            "usr", filespec_helper(m_user_filename)
        );
#endif
    }
}

/**
 *  If the 'ctrl' file-name is empty, the internal default keystrokes are
 *  used.
 */

void
rcsettings::midi_control_filename (const std::string & value)
{
    if (value.empty())
    {
        midi_control_active(false);
    }
    else
    {
        m_midi_control_filename = filename_base_fix(value, ".ctrl");
#if defined SEQ66_KEEP_RC_FILE_LIST
        (void) add_config_filespec
        (
            "ctrl", filespec_helper(m_midi_control_filename)
        );
#endif
    }
}

void
rcsettings::mute_group_filename (const std::string & value)
{
    if (value.empty())
    {
        mute_group_file_active(false);
    }
    else
    {
        m_mute_group_filename = filename_base_fix(value, ".mutes");
#if defined SEQ66_KEEP_RC_FILE_LIST
        (void) add_config_filespec
        (
            "mutes", filespec_helper(m_mute_group_filename)
        );
#endif
    }
}

/**
 *  We want to be able to use the .notemap extension as well.
 */

void
rcsettings::notemap_filename (const std::string & value)
{
    if (value.empty())
    {
        notemap_active(false);
    }
    else
    {
        bool has_ext = name_has_extension(value);
        if (has_ext)
            m_notemap_filename = value;
        else
            m_notemap_filename = filename_base_fix(value, ".drums");

#if defined SEQ66_KEEP_RC_FILE_LIST
        (void) add_config_filespec
        (
            "drums", filespec_helper(m_notemap_filename)
        );
#endif
    }
}

void
rcsettings::patches_filename (const std::string & value)
{
    if (value.empty())
    {
        patches_active(false);
    }
    else
    {
        m_patches_filename = filename_base_fix(value, ".patches");

#if defined SEQ66_KEEP_RC_FILE_LIST
        (void) add_config_filespec
        (
            "patches", filespec_helper(m_patches_filename)
        );
#endif
    }
}

void
rcsettings::palette_filename (const std::string & value)
{
    if (value.empty())
    {
        palette_active(false);
    }
    else
    {
        m_palette_filename = filename_base_fix(value, ".palette");
#if defined SEQ66_KEEP_RC_FILE_LIST
        (void) add_config_filespec
        (
            "palette", filespec_helper(m_palette_filename)
        );
#endif
    }
}

void
rcsettings::style_sheet_filename (const std::string & value)
{
    if (value.empty())
    {
        style_sheet_active(false);
    }
    else
    {
        m_style_sheet_filename = filename_base_fix(value, ".qss");
#if defined SEQ66_KEEP_RC_FILE_LIST
        (void) add_config_filespec
        (
            "qss", filespec_helper(m_style_sheet_filename)
        );
#endif
    }
}

void
rcsettings::sets_mode (const std::string & v)
{
    if (v == "normal")
        m_sets_mode = setsmode::normal;
    else if (v == "auto-arm" || (v == "autoarm"))
        m_sets_mode = setsmode::autoarm;
    else if (v == "additive")
        m_sets_mode = setsmode::additive;
    else if ((v == "all-sets") || (v == "allsets"))
        m_sets_mode = setsmode::allsets;
    else
        m_sets_mode = setsmode::normal;
}

std::string
rcsettings::sets_mode_string () const
{
    return sets_mode_string(sets_mode());
}

std::string
rcsettings::sets_mode_string (setsmode v) const
{
    std::string result;
    switch (v)
    {
        case setsmode::normal:   result = "normal";     break;
        case setsmode::autoarm:  result = "auto-arm";    break;
        case setsmode::additive: result = "additive";   break;
        case setsmode::allsets:  result = "all-sets";    break;
        default:                 result = "unknown";    break;
    }
    return result;
}

void
rcsettings::port_naming (const std::string & v)
{
    if (v == "long" || v == "full")
        m_port_naming = portname::full;
    else if (v == "pair")
        m_port_naming = portname::pair;
    else
        m_port_naming = portname::brief;
}

std::string
rcsettings::port_naming_string () const
{
    return port_naming_string(port_naming());
}

std::string
rcsettings::port_naming_string (portname v) const
{
    std::string result;
    switch (v)
    {
        case portname::brief:    result = "short";       break;
        case portname::pair:     result = "pair";        break;
        case portname::full:     result = "long";        break;
        default:                 result = "unknown";     break;
    }
    return result;
}

/**
 *  In ALSA (and PortMidi), there is only one input (POLLIN) descriptor.
 *  As far as we can tell, this allows only 1 input.  For example, when
 *  running two instances of VMPK, without recording, only one instance
 *  yields a note.  What about with two real devices? It works.
 *
 *  So, don't bother trying to use two instances of VMPK for testing.
 */

bool
rcsettings::sequence_lookup_support () const
{
    bool result = record_by_buss();
    if (result)
        result =  with_jack_midi() || with_alsa_midi() || with_port_midi();

    return result;
}

#if defined SEQ66_KEEP_RC_FILE_LIST

bool
rcsettings::add_config_filespec
(
    const std::string & key,
    const std::string & fspec
)
{
    bool result = false;
    if (! fspec.empty())
    {
        auto valueit = m_config_files.find(key);
        if (valueit != m_config_files.end())
            m_config_files.erase(valueit);

        auto p = std::make_pair(key, fspec);
        auto fi = m_config_files.insert(p);
        result = fi.second;
    }
    return result;
}

#endif      // defined SEQ66_KEEP_RC_FILE_LIST

}           // namespace seq66

/*
 * rcsettings.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


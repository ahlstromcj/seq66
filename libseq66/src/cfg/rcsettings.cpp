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
 * \updates       2021-11-14
 * \license       GNU GPLv2 or above
 *
 *  Note that this module also sets the legacy global variables, so that
 *  they can be used by modules that have not yet been cleaned up.
 *
 * \warning
 *      No more "statistics" support.
 *
 * \todo
 *      Kepler34 has two more settings values: [midi-clock-mod-ticks],
 *      [note-resume] and [key-height].  The latter sounds more like a "usr"
 *      setting.
 */

#include <algorithm>                    /* std::find()                      */
#include <cstdlib>                      /* std::getenv()                    */

#include "seq66_features.hpp"           /* seq66::set_app_name(), etc.      */
#include "cfg/settings.hpp"             /* seq66::rc(), seq66::usr()        */
#include "play/seq.hpp"                 /* seq66::seq::maximum()            */
#include "util/filefunctions.hpp"       /* make_directory(), etc.           */
#include "util/strfunctions.hpp"        /* seq66::strncompare()             */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Default constructor.
 */

rcsettings::rcsettings () :
    basesettings                (),
    m_clocks                    (),         /* vector wrapper class     */
    m_inputs                    (),         /* vector wrapper class     */
    m_mute_groups               (),
    m_load_key_controls         (true),
    m_keycontainer              ("rc"),
    m_load_midi_controls        (true),
    m_midi_control_buss         (null_buss()),
    m_midi_control_in           ("rc"),
    m_midi_control_out          ("rc"),
    m_clock_mod                 (64),
    m_verbose                   (false),
    m_investigate               (false),
    m_save_list                 (),         /* std::map<string, bool>   */
    m_save_old_triggers         (false),
    m_save_old_mutes            (false),
    m_allow_mod4_mode           (false),
    m_allow_snap_split          (false),
    m_allow_click_edit          (true),
    m_show_midi                 (false),
    m_priority                  (false),
    m_pass_sysex                (false),
    m_with_jack_transport       (false),
    m_with_jack_master          (false),
    m_with_jack_master_cond     (false),
#if defined SEQ66_RTMIDI_SUPPORT
    m_with_jack_midi            (true),
#else
    m_with_jack_midi            (false),
#endif
    m_jack_auto_connect         (true),
    m_song_start_mode           (sequence::playback::automatic),
    m_song_start_is_auto        (true),
    m_manual_ports              (false),
    m_manual_port_count         (c_output_buss_default),
    m_manual_in_port_count      (c_input_buss_default),
    m_reveal_ports              (false),
    m_print_keys                (false),
    m_interaction_method        (interaction::seq24),
    m_sets_mode                 (setsmode::normal),
    m_port_naming               (portnaming::shortnames),
    m_midi_filename             (),
    m_midi_filepath             (),
    m_jack_session_uuid         (),
    m_jack_session_active       (false),
    m_last_used_dir             (double_quotes()),
#if defined SEQ66_PLATFORM_WINDOWS      /* but see home_config_directory()  */
    m_config_directory          (SEQ66_CLIENT_NAME),
#else
    m_config_directory
    (
        std::string(".config/") + std::string(SEQ66_CLIENT_NAME)
    ),
#endif
    m_config_filename           (SEQ66_CONFIG_NAME),    /* updated in body  */
    m_full_config_directory     (),
    m_user_file_active          (true),
    m_user_filename             (SEQ66_CONFIG_NAME),    /* updated in body  */
    m_midi_control_active       (false),
    m_midi_control_filename     (SEQ66_CONFIG_NAME),    /* updated in body  */
    m_mute_group_active         (false),
    m_mute_group_filename       (SEQ66_CONFIG_NAME),    /* updated in body  */
    m_playlist_active           (false),
    m_playlist_filename         (SEQ66_CONFIG_NAME),    /* updated in body  */
    m_playlist_midi_base        (),
    m_notemap_active            (false),
    m_notemap_filename          (SEQ66_CONFIG_NAME),    /* updated in body  */
    m_palette_active            (false),
    m_palette_filename          (SEQ66_CONFIG_NAME),    /* updated in body  */
    m_application_name          (seq_app_name()),
    m_tempo_track_number        (0),
    m_recent_files              (),
    m_load_most_recent          (true),
    m_full_recent_paths         (false)
{
    m_midi_control_in.inactive_allowed(true);
    m_config_filename += ".rc";
    m_user_filename += ".usr";
    m_midi_control_filename += ".ctrl";
    m_mute_group_filename += ".mutes";
    m_playlist_filename += ".playlist";
    m_notemap_filename += ".drums";
    m_palette_filename += ".palette";
}

/**
 *  Sets the default values.
 */

void
rcsettings::set_defaults ()
{
    /*
     * m_clocks.clear();
     * m_inputs.clear();
     * m_mute_groups.clear();
     */

    m_load_key_controls         = true;

    /*
     * m_keycontainer.clear();              // what is best?
     */

    m_load_midi_controls        = true;
    m_midi_control_buss         = null_buss();

    /*
     * m_midi_control_in.clear();           // what is best?
     * m_midi_control_out.clear();          // does not exist
     */

    m_clock_mod                 = 64;
    m_verbose                   = false;
    m_investigate               = false;
    m_save_old_triggers         = false;
    m_save_old_mutes            = false;
    m_allow_mod4_mode           = false;
    m_allow_snap_split          = false;
    m_allow_click_edit          = true;
    m_show_midi                 = false;
    m_priority                  = false;
    m_pass_sysex                = false;
    m_with_jack_transport       = false;
    m_with_jack_master          = false;
    m_with_jack_master_cond     = false;
#if defined SEQ66_RTMIDI_SUPPORT
    m_with_jack_midi            = true;
#else
    m_with_jack_midi            = false;
#endif
    m_jack_auto_connect         = true;
    m_song_start_mode           = sequence::playback::automatic;
    m_song_start_is_auto        = true;
    m_manual_ports              = false;
    m_manual_port_count         = c_output_buss_default;
    m_manual_in_port_count      = c_input_buss_default;
    m_reveal_ports              = false;
    m_print_keys                = false;
    m_interaction_method        = interaction::seq24;
    m_sets_mode                 = setsmode::normal;
    m_port_naming               = portnaming::shortnames;
    m_midi_filename.clear();
    m_midi_filepath.clear();
    m_jack_session_uuid.clear();
    m_jack_session_active       = false;
    m_last_used_dir             = double_quotes();
#if defined SEQ66_PLATFORM_WINDOWS    /* but see home_config_directory()  */
    m_config_directory          = SEQ66_CLIENT_NAME;
#else
    m_config_directory = std::string(".config/") + std::string(SEQ66_CLIENT_NAME);
#endif
    m_config_filename           = SEQ66_CONFIG_NAME;
    m_config_filename           += ".rc";
    m_full_config_directory.clear();
    m_user_file_active = true;
    m_user_filename = SEQ66_CONFIG_NAME;
    m_midi_control_active = false;
    m_midi_control_filename = SEQ66_CONFIG_NAME;
    m_mute_group_active = false;
    m_mute_group_filename = SEQ66_CONFIG_NAME;
    m_playlist_active = false;
    m_playlist_filename = SEQ66_CONFIG_NAME;
    m_playlist_midi_base.clear();
    m_notemap_active = false;
    m_notemap_filename = SEQ66_CONFIG_NAME;
    m_palette_active = false;
    m_palette_filename = SEQ66_CONFIG_NAME;
    m_config_filename += ".rc";
    m_user_filename += ".usr";
    m_midi_control_filename += ".ctrl";
    m_mute_group_filename += ".mutes";
    m_playlist_filename += ".playlist";
    m_notemap_filename += ".drums";
    m_palette_filename += ".palette";

    /*
     * const: m_application_name = seq_app_name();
     */

    m_tempo_track_number = 0;
    m_recent_files.clear();
    m_load_most_recent = true;
    m_full_recent_paths = false;
    set_config_files(SEQ66_CONFIG_NAME);
    set_save_list(false);
}

void
rcsettings::set_save_list (bool state)
{
    m_save_list.clear();
    m_save_list.add("rc", true);                /* can be edited in UI  */
    m_save_list.add("usr", state);              /* can be edited in UI  */
    m_save_list.add("mutes", state);            /* can be edited in UI  */
    m_save_list.add("playlist", state);         /* can be edited in UI  */
    m_save_list.add("palette", state);          /* can be saved via UI  */

    /*
     * The following are saved only after the first run.  Thereafter, they
     * are managed by the user.
     */

    m_save_list.add("drums", state);
    m_save_list.add("ctrl", state);
    m_save_list.add("qss", state);
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
        rc().is_modified() || usr().is_modified()
    );
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
rcsettings::song_start_mode (const std::string & s)
{
    if (s == "song" || s == "true" || s == "1")
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

/**
 *  Provides the directory for the configuration file, and also creates the
 *  directory if necessary.
 *
 *  The home directory is (in Linux) "/home/username/.config/seq66", and
 *  the configuration file is "qseq66.rc".
 *
 *  This function should also adapt to Windows conventions automatically.
 *  We shall see.  No, it does not.  But all we have to do is replace
 *  Window's HOMEPATH with its LOCALAPPDATA value.
 *
 * getenv(HOME):
 *
 *      -   Linux returns "/home/ahlstrom".  Append "/.config/seq66".
 *      -   Windows returns "\Users\ahlstrom".  A better value than HOMEPATH
 *          is LOCALAPPDATA, which gives us most of what we want:
 *          "C:\Users\ahlstrom\AppData\local", and then we append simply
 *          "seq66".
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
        std::string result;
        std::string home = user_home();
        if (! home.empty())
        {
            result = home + path_slash();           /* e.g. /home/username/ */
            result += config_directory();           /* seq66 directory      */
#if defined SEQ66_PLATFORM_UNIX
            result += path_slash();
#endif
            bool ok = make_directory_path(result);
            if (ok)
            {
#if defined SEQ66_PLATFORM_WINDOWS
                result += path_slash();
#endif
                m_full_config_directory = normalize_path(result);
            }
            else
            {
                file_error("Create fail", result);
                result.clear();
            }
        }
        else
        {
            std::string temp = "std::getenv(HOME/LOCALAPPDATA) failed";
            result = set_error_message(temp);
        }
        return result;
    }
    else
        return os_normalize_path(m_full_config_directory);
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
    std::string result;
    auto pos = base.find_last_of(".");
    if (pos != std::string::npos)                       /* base.extension   */
    {
        pos = result.find_first_of("/");
        if (pos != std::string::npos)                   /* slash in base    */
        {
            if (pos > 0)                                /* not at start     */
            {
                result = home_config_directory();       /* relative path    */
                result += base;
            }
            else
                result = base;                          /* a full path      */
        }
        else
        {
            result = home_config_directory();           /* HOME/base.exten  */
            result += base;
        }
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
 *  We need a way to handle the following configuration-file paths:
 *
 *      -   "base.ext": Prepend the HOME directory.
 *      -   "subdirectory/base.ext": Prepend the HOME directory
 *      -   "/path/.../base.ext": Use the path as is.
 *      -   "C:/path/.../base.ext": Use the path as is.
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
        {
            result = home_config_directory();
            result += baseext;
        }
        result = os_normalize_path(result);         /* change to OS's slash */
    }
    return result;
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
    return filespec_helper(usr().style_sheet());
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
        rc().auto_usr_save(true);

    if (clear_uuid)
        m_jack_session_uuid.clear();
}

/**
 * \setter m_last_used_dir
 *
 * \param value
 *      The value to use to make the setting.  It needs to be a directory, not
 *      a file.  Also, we now expand a relative directory to the full path to
 *      that directory, to avoid ambiguity should the application be run from
 *      a different directory.
 */

void
rcsettings::last_used_dir (const std::string & value)
{
    if (value.empty())
        m_last_used_dir = empty_string();       /* "" from strfunctions */
    else
        m_last_used_dir = get_full_path(value); /* might end up empty   */
}

/**
 * \setter m_config_directory
 *
 * \param value
 *      The value to use to make the setting.  Currently, we do not handle
 *      relative paths.  To do so seems... iffy.
 */

void
rcsettings::config_directory (const std::string & value)
{
    if (! value.empty())
        m_config_directory = value;
}

/**
 * \setter m_full_config_directory
 *
 *      Provides an alternate value to be returned by the
 *      home_config_directory() function. Please note that all configuration
 *      locates are relative to home.
 *
 * \param value
 *      Provides the directory name, either an actual full path, or a path
 *      meant to be relative to the $HOME directory.
 *
 * \param addhome
 *      If true, the user's $HOME is prepended to the path, if not already
 *      there.
 */

void
rcsettings::full_config_directory (const::std::string & value, bool addhome)
{
    std::string tv = value;
    if (name_has_root_path(tv))
        addhome = false;

    if (addhome)
    {
        tv = trim(tv, SEQ66_TRIM_CHARS_PATHS);
        config_directory(tv);
        m_full_config_directory.clear();
        tv = home_config_directory();
    }
    m_full_config_directory = normalize_path(tv, true, true);
}

/**
 * \setter m_config_filename and m_user_filename
 *
 *      Implements the --config option to change both configuration files
 *      ("rc" and "usr") with one option.
 *
 * \param value
 *      The value to use to make the setting, if the string is not empty.
 *      If the value has an extension, it is stripped first.
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
    if (! value.empty())
        m_config_filename = value;

    if (m_config_filename.find(".") == std::string::npos)
        m_config_filename += ".rc";
}

/**
 * \setter m_playlist_filename ("playlist")
 *
 * \param value
 *      The value to use to make the setting, if the string is not empty.
 *      If there is no period in the string, then ".playlist" is appended to
 *      the end of the filename.
 */

void
rcsettings::playlist_filename (const std::string & value)
{
    if (is_empty_string(value))
    {
        clear_playlist();               /* clears file-name and active flag */
    }
    else
    {
        /*
         * Let the caller take care of this: m_playlist_active = true;
         */

        m_playlist_filename = value;
        if (m_playlist_filename.find(".") == std::string::npos)
            m_playlist_filename += ".playlist";
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

const std::string &
rcsettings::playlist_filename () const
{
    return m_playlist_filename;
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
    if (! value.empty())
        m_user_filename = value;

    if (m_user_filename.find(".") == std::string::npos)
        m_user_filename += ".usr";
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
    if (v == "short")
        m_port_naming = portnaming::shortnames;
    else if (v == "long")
        m_port_naming = portnaming::longnames;
    else
        m_port_naming = portnaming::shortnames;
}

std::string
rcsettings::port_naming_string () const
{
    return port_naming_string(port_naming());
}

std::string
rcsettings::port_naming_string (portnaming v) const
{
    std::string result;
    switch (v)
    {
        case portnaming::shortnames:    result = "short";       break;
        case portnaming::longnames:     result = "long";        break;
        default:                        result = "unknown";     break;
    }
    return result;
}

}           // namespace seq66

/*
 * rcsettings.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


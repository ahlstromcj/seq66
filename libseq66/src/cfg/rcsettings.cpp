/*
 *  This file is part of seq24/seq66.
 *
 *  seq24 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  seq24 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq24; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
 * \updates       2020-11-23
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
#include "util/strfunctions.hpp"        /* strncompare()                    */

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
    m_mute_groups
    (
        SEQ66_DEFAULT_SET_ROWS, SEQ66_DEFAULT_SET_COLUMNS
    ),                                      /* a map wrapper class      */
    m_load_key_controls         (true),
    m_keycontainer              ("rc"),
    m_load_midi_controls        (true),
    m_midi_control_buss         (c_bussbyte_max),
    m_midi_control_in           ("rc"),
    m_midi_control_out          (),
    m_show_ui_sequence_key      (true),
    m_show_ui_sequence_number   (true),
    m_clock_mod                 (64),
    m_verbose                   (false),
    m_auto_option_save          (true),     /* legacy seq24 behavior    */
    m_lash_support              (false),
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
    m_song_start_mode           (false),
    m_manual_ports              (false),
    m_manual_port_count         (SEQ66_OUTPUT_BUSS_MAX),
    m_reveal_ports              (false),
    m_print_keys                (false),
    m_device_ignore             (false),
    m_device_ignore_num         (0),
    m_interaction_method        (interaction::seq24),
    m_midi_filename             (),
    m_midi_filepath             (),
    m_jack_session_uuid         (),
#if defined SEQ66_PLATFORM_WINDOWS      /* but see home_config_directory()  */
    m_last_used_dir             (),
#else
    m_last_used_dir             ("~/"),
#endif
    m_config_directory          (SEQ66_CLIENT_NAME),
    m_config_filename           (SEQ66_CONFIG_NAME),
    m_full_config_directory     (),
    m_user_filename             (SEQ66_CONFIG_NAME),
    m_use_midi_control_file     (false),
    m_midi_control_filename     (),
    m_use_mute_group_file       (false),
    m_mute_group_filename       (),
    m_playlist_active           (false),
    m_playlist_filename         (),
    m_playlist_midi_base        (),
    m_notemap_active            (false),
    m_notemap_filename          (),
    m_application_name          (seq_app_name()),
    m_tempo_track_number        (0),
    m_recent_files              (),
    m_load_most_recent          (true)
{
    m_midi_control_in.inactive_allowed(true);
    m_config_filename += ".rc";
    m_user_filename += ".usr";

#if ! defined SEQ66_PLATFORM_WINDOWS    /* but see home_config_directory()  */
    m_config_directory = std::string(".config/") + m_config_directory;
#endif

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
    m_midi_control_buss         = c_bussbyte_max;

    /*
     * m_midi_control_in.clear();           // what is best?
     * m_midi_control_out.clear();          // does not exist
     */

    m_show_ui_sequence_key      = true;
    m_show_ui_sequence_number   = true;
    m_clock_mod                 = 64;
    m_verbose                   = false;
    m_auto_option_save          = true;     /* legacy seq24 setting */
    m_lash_support              = false;
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
    m_song_start_mode           = false;
    m_manual_ports              = false;
    m_manual_port_count         = SEQ66_OUTPUT_BUSS_MAX;
    m_reveal_ports              = false;
    m_print_keys                = false;
    m_device_ignore             = false;
    m_device_ignore_num         = 0;
    m_interaction_method        = interaction::seq24;
    m_midi_filename.clear();
    m_midi_filepath.clear();
    m_jack_session_uuid.clear();
    m_config_directory          = SEQ66_CLIENT_NAME;
#if defined SEQ66_PLATFORM_WINDOWS    /* but see home_config_directory()  */
    m_last_used_dir.clear();
#else
    m_last_used_dir             = "~/";
    m_config_directory          = std::string(".config/") + m_config_directory;
#endif
    m_config_filename           = SEQ66_CONFIG_NAME;
    m_config_filename           += ".rc";
    m_full_config_directory.clear();
    m_user_filename = SEQ66_CONFIG_NAME; m_user_filename += ".usr";
    m_use_midi_control_file = false;
    m_midi_control_filename = "";
    m_use_mute_group_file = false;
    m_mute_group_filename.clear();
    m_playlist_active = false;
    m_playlist_filename.clear();
    m_playlist_midi_base.clear();
    m_notemap_active = false;
    m_notemap_filename.clear();

    /*
     * const: m_application_name = seq_app_name();
     */

    m_tempo_track_number = 0;
    m_recent_files.clear();
    m_load_most_recent = true;
    set_config_files(SEQ66_CONFIG_NAME);
}

/**
 *  Holds the client name for the application.  This is much like the
 *  application name, but in the future will be a configuration option.
 *  For now it is just the value returned by the seq_client_name()
 *  function.  However, note that, under session management, we replace it
 *  with the NSM "client_id" value.
 */

std::string
rcsettings::app_client_name () const
{
    return seq_client_name();
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
    return strncompare
    (
        name, home_config_directory(), home_config_directory().length()
    );
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
    std::string::size_type pos = base.find_last_of(".");
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
        std::string::size_type pos = ext.find_first_of(".");
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
    std::string result = home_config_directory();
    if (! result.empty())
    {
        result += config_filename();
        result = os_normalize_path(result);     /* change to OS's slash     */
    }
    return result;
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
 *  Constructs an alternate full path and file specification for the "rc"
 *  file.  This function is useful in writing to an alternate "rc" file when a
 *  fatal error occurs.  Also note that the configuration file specification
 *  can never be empty or blank (equal to this string: "").  See the
 *  strfunction module's is_empty_string() function.
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
    std::string result;
    if (! altname.empty())
    {
        result = home_config_directory();
        if (! result.empty())
            result += altname;
    }
    return result;
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
    std::string result = home_config_directory();
    if (! result.empty())
    {
        result += user_filename();
        result = os_normalize_path(result);     /* change to OS's slash     */
    }
    return result;
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
    std::string result;
    if (! altname.empty())
    {
        result = home_config_directory();
        if (! result.empty())
            result += altname;
    }
    return result;
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
    std::string result;
    std::string listname = playlist_filename();
    if (! listname.empty())
    {
        if (name_has_directory(listname))
            result = listname;
        else
            result = home_config_directory() + listname;

        result = os_normalize_path(result);
    }
    return result;
}

/**
 *  Constructs the note-mapper filespec.
 */

std::string
rcsettings::notemap_filespec () const
{
    std::string result;
    std::string notemap_name = notemap_filename();
    if (! notemap_name.empty())
    {
        if (name_has_directory(notemap_name))
            result = notemap_name;
        else
            result = home_config_directory() + notemap_name;

        result = os_normalize_path(result);
    }
    return result;
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
    std::string result;
    std::string ctlfilename = midi_control_filename();
    if (! ctlfilename.empty())
    {
        if (name_has_directory(ctlfilename))
            result = ctlfilename;
        else
            result = home_config_directory() + ctlfilename;

        result = os_normalize_path(result);
    }
    return result;
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
    std::string result;
    std::string mutefilename = mute_group_filename();
    if (! mutefilename.empty())
    {
        if (name_has_directory(mutefilename))
            result = mutefilename;
        else
            result = home_config_directory() + mutefilename;

        result = os_normalize_path(result);
    }
    return result;
}

/**
 * \setter m_device_ignore_num
 *      However, please note that this value, while set in the options
 *      processing of the main module, does not appear to be used anywhere
 *      in the code in seq24, Sequencer24, and this application.
 *
 * \param value
 *      The value to use to make the setting.
 */

void
rcsettings::device_ignore_num (int value)
{
    if (value >= 0)
        m_device_ignore_num = value;
}

void
rcsettings::tempo_track_number (int track)
{
    if (track < 0)
        track = 0;
    else if (track >= seq::maximum())
        track = seq::maximum() - 1;

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
 *      It needs to be short for the menu entry, but the full path-name for
 *      the "rc" file.
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
        std::string::size_type slashpos = result.find_last_of("/\\");
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
 * \param value
 *      The value to use to make the setting.
 */

void
rcsettings::jack_session_uuid (const std::string & value)
{
    if (! value.empty())
        m_jack_session_uuid = value;
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
        m_last_used_dir = get_full_path(value);
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
    if (is_root_path(tv))
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
        std::string fname = make_config_filespec(fname, ".playlist");
        result = file_exists(fname);
        playlist_filename(value);       /* set playlist name no matter what */
    }
    return result;
}

/**
 *
 */

const std::string &
rcsettings::playlist_filename () const
{
    return m_playlist_filename;
}

/**
 *
 */

void
rcsettings::clear_playlist ()
{
    playlist_active(false);
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

/**
 *  Adds three midicontrol objects to the midicontrolin, one each for the
 *  toggle, on, and off actions.
 *
 *  Compare this function, rcsettings::add_midicontrol_stanza(), as called in
 *  optionsfile::parse_midi_control_section() with
 *  midicontrolfile::parse_control_stanza() as called in
 *  midicontrolfile::parse().  Not sure yet if it is worth unifying them; the
 *  midicontrolfile parsing is more flexible.
 *
 *  Please note:
 *
 *      -   These items are keyed by the status values and the d0
 *          values, which are at indices 2 and 3 in the array parameters.
 *      -   They are inserted into an std::multimap.
 *      -   The category, action, and slot number are part of the data value,
 *          not the key.
 *
 *  Therefore, when writing these items back out to an "rc" file, we must:
 *
 *      -   Iterate through an equal_range.
 *      -   Use the category to pick the right section into which to output
 *          the control information.
 *      -   Use the action to pick the right group in the stanza to output it.
 *      -   Output either the midicontrol::control_code() or the slot value as
 *          the slot number.
 *      -   Remember to grab the corresponding key-name from the keycontainer
 *          while writing the section.
 *
 * \param kn
 *      The official name of the related keystroke.
 *
 * \param cat
 *      Indicates the section of the "rc" MIDI control block that is in force.
 *
 * \param nslot
 *      The slot number, already adjusted to be in the range of 0 to 31.  For
 *      the automation::loop category, this is the pattern number to be
 *      controlled.  For the automation::mute_group category, this is the
 *      mute-group number to be controlled.  For the automation category, this
 *      is the integer version of the automation::slot number, which can range
 *      above 31.
 *
 * \param a
 *      Provide the array of toggle-control values.
 *
 * \param b
 *      Provide the array of on-control values.
 *
 * \param c
 *      Provide the array of off-control values.
 *
 * \return
 *      Returns true if no error occurred.
 */

bool
rcsettings::add_midicontrol_stanza
(
    const std::string & kn,
    automation::category cat, int nslot,
    int a[6], int b[6], int c[6]
)
{
    bool result;
    if (opcontrol::is_sequence_control(cat))
    {
        automation::slot s = cat == automation::category::loop ?
            automation::slot::loop : automation::slot::mute_group;

        midicontrol mctoggle(kn, cat, automation::action::toggle, s, nslot);
        mctoggle.set(a);

        midicontrol mcon(kn, cat, automation::action::on, s, nslot);
        mcon.set(b);

        midicontrol mcoff(kn, cat, automation::action::off, s, nslot);
        mcoff.set(c);
        result = m_midi_control_in.add(mctoggle);
        if (result)
            result = m_midi_control_in.add(mcon);

        if (result)
            result = m_midi_control_in.add(mcoff);
    }
    else if (opcontrol::is_automation(cat))
    {
        automation::slot s = static_cast<automation::slot>(nslot);
        midicontrol mctoggle(kn, cat, automation::action::toggle, s, 0);
        mctoggle.set(a);

        midicontrol mcon(kn, cat, automation::action::on, s, 0);
        mcon.set(b);

        midicontrol mcoff(kn, cat, automation::action::off, s, 0);
        mcoff.set(c);
        result = m_midi_control_in.add(mctoggle);
        if (result)
            result = m_midi_control_in.add(mcon);

        if (result)
            result = m_midi_control_in.add(mcoff);
    }
    else
        result = false;

    return result;
}

/**
 *  Adds three blank midicontrol objects to the midicontrolin, one each for
 *  the toggle, on, and off actions.  These are always category::automation
 *  objects, and cover the cases (usually GUI or mode controls) where there
 *  are no associated midicontrols.
 *
 * \param kn
 *      The official name of the related keystroke.
 *
 * \param cat
 *      Indicates the section of the "rc" MIDI control block that is in force.
 *
 * \param nslot
 *      The slot number, already adjusted to be in the range of 0 to 31 or
 *      thereabouts.
 *
 * \return
 *      Returns true if no error occurred.
 */

bool
rcsettings::add_blank_stanza
(
    const std::string & kn,
    automation::category cat,
    int nslot
)
{
    static int s_v[6] = {0, 0, 0, 0, 0, 0};
    return add_midicontrol_stanza(kn, cat, nslot, s_v, s_v, s_v);
}

}           // namespace seq66

/*
 * rcsettings.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


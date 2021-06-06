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
 * \file          rcfile.cpp
 *
 *  This module declares/defines the base class for managing the
 *  the ~/.config/seq66.rc ("rc") configuration file.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-23
 * \updates       2021-06-06
 * \license       GNU GPLv2 or above
 *
 *  The <code> ~/.config/seq66.rc </code> configuration file is fairly simple
 *  in layout.  The documentation for this module is supplemented by the
 *  following GitHub projects:
 *
 *      -   https://github.com/ahlstromcj/seq66-doc.git
 *
 *  Those documents also relate these file settings to the application's
 *  command-line options.
 *
 *  Note that the parse() and write() functions process sections in a
 *  different order!  The reason this does not mess things up is that the
 *  line_after() function, by default, rescans from the beginning of the file.
 *  As long as each section's sub-values are read and written in the same
 *  order, there will be no problem.
 *
 *  This file now (again) supports LASH setup.  Eventually, we will add more
 *  modern session management.
 *
 *  Finally, note that seq66 no longer supports the Seq24 file format;
 *  too much has changed.
 */

#include "cfg/midicontrolfile.hpp"      /* seq66::midicontrolfile class     */
#include "cfg/mutegroupsfile.hpp"       /* seq66::mutegroupsfile class      */
#include "cfg/rcfile.hpp"               /* seq66::rcfile class              */
#include "cfg/settings.hpp"             /* seq66::rc() accessor             */
#include "midi/midibus.hpp"             /* seq66::midibus class             */
#include "util/calculations.hpp"        /* seq66::current_date_time()       */
#include "util/filefunctions.hpp"       /* seq66::file_extension_set()      */
#include "util/strfunctions.hpp"        /* seq66::strip_quotes() function   */

#if defined SEQ66_USE_FRUITY_CODE         /* will not be supported in seq66   */

/**
 *  Provides names for the mouse-handling used by the application.  The fruity
 *  mode is currently not supported in Seq66, but maybe someone will want it
 *  back.
 */

static const std::string c_interaction_method_names[3] =
{
    "seq66",
    "fruity",
    ""
};

/**
 *  Provides descriptions for the mouse-handling used by the application.
 */

static const std::string c_interaction_method_descs[3] =
{
    "original seq66 method",
    "similar to a certain fruity sequencer",
    ""
};

#endif // defined SEQ66_USE_FRUITY_CODE

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Principal constructor.
 *
 * Versions:
 *
 *      1:  2021-05-16
 *      2:  2021-06-04  Transition of get-variable for booleans.
 *
 * \param rcs
 *      The source/destination for the configuration information.
 *
 * \param name
 *      Provides the name of the options file; this is usually a full path
 *      file-specification.
 */

rcfile::rcfile (const std::string & name, rcsettings & rcs) :
    configfile  (name, rcs)
{
    version("2");
}

/**
 *  Parse the ~/.config/seq66/qseq66.rc file.  After this function is
 *  called, the performer::get_settings() function can be used to populate the
 *  performer with the settings it needs.
 *
 *  [midi-control]
 *
 *      See midicontrolfile::parse_stream().  Replaced by [loop-control],
 *      [mute-group-control], and [automation-control].
 *
 *  [midi-control-file]
 *
 *      If this section is present, the [midi-control] section is ignored, even
 *      if present, in favor of reading the MIDI control information from a
 *      separate file.  This allows the user to switch between different setups
 *      without having to mess with editing the "rc" file much.  The next data
 *      line after section should be a filename.  If there is none, or
 *      if it is set to "", then the [midi-control] section is used, if present.
 *      If neither are present, this is a fatal error.
 *
 *  [mute-group]
 *
 *      See mutegroupsfile::parse_stream().
 *
 *  [midi-clock]
 *
 *      The MIDI-clock section defines the clocking value for up to 16 output
 *      busses.  The first number indicates how many busses are specified.
 *      Generally, these busses are shown to the user with names such as "[1]
 *      seq66 1".
 *
 *  [keyboard-control]
 *  [keyboard-group]
 *  [extended-keys]
 *  [New-keys]
 *
 *      The [keyboard-control] section, and related sections, of Seq64 has been
 *      replaced by moving the keystroke settings to the MIDI control section,
 *      thus unifying keyboard and MIDI control of patterns/loops, mute-groups,
 *      and application automation.
 *
 *  [jack-transport]
 *
 *      This section covers various JACK settings, one setting per line.  In
 *      order, the following numbers are specfied:
 *
 *      -   jack_transport - Enable sync with JACK Transport.
 *      -   jack_master - Seq24 will attempt to serve as JACK Master.
 *      -   jack_master_cond - Seq24 will fail to be Master if there is
 *          already a Master set.
 *      -   song_start_mode:
 *          -   0 = Playback will be in Live mode.  Use this to allow
 *              muting and unmuting of loops.
 *          -   1 = Playback will use the Song Editor's data.
 *
 *  [midi-input]
 *
 *      This section covers the MIDI input busses, and has a format similar to
 *      "[midi-clock]".  Generally, these busses are shown to the user with names
 *      such as "[1] seq66 1", and currently there is only one input buss.  The
 *      first field is the port number, and the second number indicates whether
 *      it is disabled (0), or enabled (1).
 *
 *  [midi-clock-mod-ticks]
 *
 *      This section covers....  One common value is 64.
 *
 *  [manual-ports]
 *
 *      Set to 1 if you want seq66 to create its own virtual ports and not
 *      connect to other clients.
 *
 *  [last-used-dir]
 *
 *      This section simply holds the last path-name that was used to read or
 *      write a MIDI file.  We still need to add a check for a valid path, and
 *      currently the path must start with a "/", so it is not suitable for
 *      Windows.
 *
 *  [interaction-method] (obsolete)
 *
 *      This section specified the kind of mouse interaction.  We're keeping
 *      the code, macro'd out, in case someone clamors for it eventually.
 *
 *      -   0 = 'seq66' (original Seq24 method).
 *      -   1 = 'fruity' (similar to a certain fruity sequencer we like).
 *
 *      The second data line is set to "1" if Mod4 can be used to keep seq66 in
 *      note-adding mode even after the right-click is released, and "0"
 *      otherwise.
 *
 * \return
 *      Returns true if the file was able to be opened for reading.
 *      Currently, there is no indication if the parsing actually succeeded.
 */

bool
rcfile::parse ()
{
    std::ifstream file(name(), std::ios::in | std::ios::ate);
    if (! set_up_ifstream(file))            /* verifies [Seq66]: version */
        return false;

    bool verby = get_boolean(file, "[Seq66]", "verbose");
    rc().verbose(verby);
    (void) parse_version(file);

    std::string s = get_variable(file, "[Seq66]", "sets-mode");
    rc().sets_mode(s);
    s = get_variable(file, "[Seq66]", "port-naming");
    rc().port_naming(s);

    /*
     * [comments] Header comments (hash-tag lead) is skipped during parsing.
     * However, we now try to read an optional comment block.
     */

    std::string comments = parse_comments(file);
    if (! comments.empty())
        rc_ref().comments_block().set(comments);

    bool ok = true;                                     /* start hopefully! */
    if (file_version_number() < 2)
    {
        if (line_after(file, "[midi-control-file]"))
        {
            ok = ! is_missing_string(line());               /* "", "?", empty   */
            if (ok)
            {
                std::string mcfname = strip_quotes(line());
                rc_ref().midi_control_filename(mcfname);    /* set base name    */

                std::string fullpath = rc_ref().midi_control_filespec();
                file_message("Reading 'ctrl'", fullpath);
                ok = parse_midi_control_section(fullpath, true);
                if (! ok)
                {
                    std::string info = "'";
                    info += fullpath;
                    info += "'";
                    return make_error_message("midi-control-file", info);
                }
            }
        }
#if defined USE_OBSOLETE_CODE
        else
        {
            /*
             * This call causes parsing to skip all of the header material.  Note
             * that line_after() starts from the beginning of the file every time,
             * a lot a rescanning!  But it goes fast these days.
             */

            ok = parse_midi_control_section(name());
        }
#endif
    }
    else
    {
        std::string pfname;
        bool active = get_file_status(file, "[midi-control-file]", pfname);
        rc_ref().midi_control_active(active);
        rc_ref().midi_control_filename(pfname);             /* base name    */
    }

    if (line_after(file, "[mute-group-file]"))
    {
        ok = ! is_missing_string(line());
        if (ok)
        {
            std::string mgfname = strip_quotes(line());
            rc_ref().mute_group_filename(mgfname);      /* base name        */

            std::string fullpath = rc_ref().mute_group_filespec();
            file_message("Reading 'mutes'", fullpath);
            ok = parse_mute_group_section(fullpath, true);
            if (! ok)
            {
                std::string info = "cannot parse file '";
                info += fullpath;
                info += "'";
                return make_error_message("mute-group-file", info);
            }
        }
    }
    else
    {
        /*
         * After parsing the mute-groups, see if there is another value for
         * the mute_group_handling enumeration.  One little issue...  the
         * parse_mute_group_section() function actually re-opens the file
         * itself, and once it exits, it's as if the section never existed.
         * So we also have to parse the new mute-group handling feature there
         * as well.
         */

        ok = parse_mute_group_section(name());
    }

    if (line_after(file, "[usr-file]"))
    {
        ok = ! is_missing_string(line());
        if (ok)
        {
            std::string mgfname = strip_quotes(line());
            rc_ref().user_filename(mgfname);             /* base name        */
        }
    }

    int flag = 0;

    if (file_version_number() < 2)
    {
        if (line_after(file, "[palette-file]"))
        {
            sscanf(scanline(), "%d", &flag);
            rc_ref().palette_active(flag != 0);
            if (next_data_line(file))
            {
                ok = ! is_missing_string(line());
                if (ok)
                {
                    std::string pfname = strip_quotes(line());
                    rc_ref().palette_filename(pfname);      /* base name    */
                }
            }
        }
    }
    else
    {
        std::string pfname;
        bool active = get_file_status(file, "[palette-file]", pfname);
        rc_ref().palette_active(active);
        rc_ref().palette_filename(pfname);              /* base name    */
    }

    /*
     * JACK transport settings are currently accessed only via the rcsetting's
     * rc() accessor function.  Also note that these setting must occur in the
     * given order in the 'rc' file: (1) Transport; (2) Master; (3)
     * Master-conditional.  Otherwise the coordination of these settings gets
     * messed up.
     */

    s = get_variable(file, "[jack-transport]", "transport-type");
    if (s.empty())
    {
        if (line_after(file, "[jack-transport]"))
        {
            sscanf(scanline(), "%d", &flag);                /* 1 */
            rc_ref().with_jack_transport(bool(flag));
            next_data_line(file);
            sscanf(scanline(), "%d", &flag);                /* 2 */
            rc_ref().with_jack_master(bool(flag));
            next_data_line(file);
            sscanf(scanline(), "%d", &flag);                /* 3 */
            rc_ref().with_jack_master_cond(bool(flag));
            next_data_line(file);
            sscanf(scanline(), "%d", &flag);                /* 4 */
            rc_ref().song_start_mode(bool(flag));
            if (next_data_line(file))
            {
                sscanf(scanline(), "%d", &flag);            /* 5 */
                rc_ref().with_jack_midi(bool(flag));
            }
        }
    }
    else
    {
        rc_ref().set_jack_transport(s);
        s = get_variable(file, "[jack-transport]", "song-start-mode");
        if (s == "live")
            s = "false";
        else if (s == "song")
            s = "true";

        rc_ref().song_start_mode(string_to_bool(s, false));

        bool flag = get_boolean(file, "[jack-transport]", "jack-midi");
        rc_ref().with_jack_midi(flag);
    }

    bool use_manual_ports = false;
    if (line_after(file, "[manual-ports]"))
    {
        sscanf(scanline(), "%d", &flag);
        use_manual_ports = bool(flag);
        rc_ref().manual_ports(use_manual_ports);
        if (next_data_line(file))
        {
            int count;
            sscanf(scanline(), "%d", &count);
            rc().manual_port_count(count);
        }
        if (next_data_line(file))
        {
            int count;
            sscanf(scanline(), "%d", &count);
            rc().manual_in_port_count(count);
        }
    }
    else
        (void) make_error_message("manual-ports", "data line missing");

    /*
     *  We are taking a slightly different approach to this section.  When
     *  Seq66 exits, it saves all of the inputs it has.  If an input is
     *  removed from the system (e.g. unplugging a MIDI controller), then
     *  there will be too many entries in this section.  The user might remove
     *  one, and forget to update the buss count.  So we basically ignore the
     *  buss count.  But we also have to read the  channel-filter boolean.  If
     *  an error occurs, we abort... the user must fix the "rc" file.
     */

    if (! use_manual_ports && line_after(file, "[midi-input]"))
    {
        int inbuses = 0;
        int count = sscanf(scanline(), "%d", &inbuses);
        if (count > 0 && inbuses > 0)
        {
            int b = 0;
            rc_ref().inputs().clear();
            while (next_data_line(file))
            {
                int bus, bus_on;
                count = sscanf(scanline(), "%d %d", &bus, &bus_on);
                if (count == 2)
                {
                    rc_ref().inputs().add(bus, bool(bus_on), line());
                    ++b;
                }
                else if (count == 1)
                {
                    bool flag = bool(bus);
                    rc_ref().filter_by_channel(flag);
                    toggleprint("Filter-by-channel", flag);
                }
            }
            if (b < inbuses)
                return make_error_message("midi-input", "too few buses");
        }
    }
    else
        return make_error_message("midi-input", "data lines missing");

    /*
     *  Check for an optional input port map section.
     */

    if (line_after(file, "[midi-input-map]"))
    {
        inputslist & inputref = input_port_map();
        int activeflag;
        inputref.clear();
        (void) sscanf(scanline(), "%d", &activeflag);
        inputref.active(activeflag != 0);
        for ( ; next_data_line(file); )
        {
            if (! inputref.add_list_line(line()))
            {
                inputref.clear();
                inputref.active(false);
                break;
            }
        }
    }
    if (ok)
        ok = ! use_manual_ports && line_after(file, "[midi-clock]");

    int outbuses = 0;
    if (ok)
    {
        sscanf(scanline(), "%d", &outbuses);
        ok = next_data_line(file) && is_good_busscount(outbuses);
    }
    if (ok)
    {
        /**
         * One thing about MIDI clock values.  If a device (e.g. Korg
         * nanoKEY2) is present in a system when Seq66 is exited, it will be
         * saved in the [midi-clock] list.  When unplugged, it will be read
         * here at startup, but won't be shown.  The next exit will find it
         * removed from this list.
         *
         * Also, we want to pre-allocate the number of clock entries needed,
         * and then use the buss number to populate the list of clocks, in the
         * odd event that the user changed the bus-order of the entries.
         */

        rc_ref().clocks().clear();
        for (int i = 0; i < outbuses; ++i)
        {
            int bus, bus_on;
            int count = sscanf(scanline(), "%d %d", &bus, &bus_on);
            ok = count == 2;
            if (ok)
            {
                e_clock e = int_to_clock(bus_on);
                rc_ref().clocks().add(bus, e, line());
                ok = next_data_line(file);
                if (! ok && i < (outbuses-1))
                    return make_error_message("midi-clock", "missing data line");
            }
            else
                return make_error_message("midi-clock", "data line error");
        }
    }
    else
    {
        /*
         *  If this is zero, we need to fake it to have 1 buss with a 0 clock,
         *  rather than make the poor user figure out how to fix it.
         *
         *      return make_error_message("midi-clock");
         *
         *  And let's use the new e_clock::disabled code instead of
         *  e_clock::off.  LATER?
         */

        rc_ref().clocks().add(0, e_clock::off, "Bad clock count");
    }

    /*
     *  Check for an optional output port map section.
     */

    if (line_after(file, "[midi-clock-map]"))
    {
        clockslist & clocsref = output_port_map();
        int activeflag;
        clocsref.clear();
        (void) sscanf(scanline(), "%d", &activeflag);
        clocsref.active(activeflag != 0);
        for ( ; next_data_line(file); )
        {
            if (! clocsref.add_list_line(line()))
            {
                clocsref.clear();
                clocsref.active(false);
                break;
            }
        }
    }

    if (line_after(file, "[midi-clock-mod-ticks]"))
    {
        int ticks = 64;
        sscanf(scanline(), "%d", &ticks);
        rc_ref().set_clock_mod(ticks);
    }
    else
        return make_error_message("midi-clock-mod-ticks", "data line missing");

    if (line_after(file, "[midi-meta-events]"))
    {
        int track = 0;
        sscanf(scanline(), "%d", &track);
        rc_ref().tempo_track_number(track); /* MIDI file can override this  */
    }
    else
        return make_error_message("midi-meta_events", "data line missing");

    if (line_after(file, "[reveal-ports]"))
    {
        /*
         * If this flag is already raised, it was raised on the command line,
         * and we don't want to change it.  An ugly special case.
         */

        sscanf(scanline(), "%d", &flag);
        if (! rc_ref().reveal_ports())
            rc_ref().reveal_ports(bool(flag));
    }
    else
        (void) make_error_message("reveal-ports", "data line missing");

    if (line_after(file, "[last-used-dir]"))
    {
        if (! is_missing_string(line()))
        {
            std::string ludir = strip_quotes(line());
            rc_ref().last_used_dir(ludir);
        }
    }
    else
         (void) make_error_message("last-used-dir", "data line missing");

    if (line_after(file, "[recent-files]"))
    {
        int count, loadrecent;
        int number = sscanf(scanline(), "%d %d", &count, &loadrecent);
        if (number > 1)
            rc_ref().load_most_recent(loadrecent != 0);

        rc_ref().clear_recent_files();
        for (int i = 0; i < count; ++i)
        {
            if (next_data_line(file))
            {
                if (! is_missing_string(line()))
                {
                    std::string rfilename = strip_quotes(line());
                    if (! rc_ref().append_recent_file(rfilename))
                        file_message("Cannot read recent file", rfilename);
                }
            }
            else
                break;
        }
    }
    else
        (void) make_error_message("recent-files", "data line missing");

    rc_ref().playlist_active(false);
    if (line_after(file, "[playlist]"))
    {
        bool exists = false;
        int flag = 0;
        sscanf(scanline(), "%d", &flag);                /* playlist-active? */
        if (next_data_line(file))
        {
            std::string fname = strip_quotes(line());
            exists = ! is_missing_string(fname);
            if (exists)
            {
                /*
                 * Prepend the home configuration directory and, if needed,
                 * the playlist extension.  Also, even if we can't find the
                 * playlist file, leave the name set for when the 'rc' file is
                 * saved.
                 */

                bool active = flag != 0;
                fname = rc_ref().make_config_filespec(fname, ".playlist");
                exists = file_exists(fname);
                if (exists)
                {
                    rc_ref().playlist_filename(fname);
                    rc_ref().playlist_active(active);
                }
                else
                {
                    rc_ref().clear_playlist(true);
                    if (active)
                        file_error("No such playlist", fname);
                }
            }
        }
        if (next_data_line(file))
        {
            std::string midibase = trimline();
            if (! is_missing_string(midibase))
            {
                file_message("Playlist MIDI base directory", midibase);
                rc_ref().midi_base_directory(midibase);
            }
        }
    }
    else
    {
        /* A missing playlist section is not an error. */
    }

    rc_ref().notemap_active(false);
    if (line_after(file, "[note-mapper]"))
    {
        bool exists = false;
        int flag = 0;
        sscanf(scanline(), "%d", &flag);        /* note-mapper-active flag  */
        if (next_data_line(file))
        {
            std::string fname = strip_quotes(line());
            exists = ! is_missing_string(fname);
            if (exists)
            {
                /*
                 * Prepend the home configuration directory and, if needed,
                 * the drums extension.  The name, if set, is always set, even
                 * the the note-map is flagged as not active.
                 */

                bool active = flag != 0;
                fname = rc_ref().make_config_filespec(fname, ".drums");
                exists = file_exists(fname);
                rc_ref().notemap_filename(fname);
                if (exists)
                {
                    rc_ref().notemap_active(active);
                }
                else
                {
                    if (active)
                        file_error("No such note-mapper", fname);
                }
            }
        }
    }
    else
    {
        /* A missing note-mapper section is not an error. */
    }


    if (line_after(file, "[interaction-method]"))
    {
        int method = 0;
        sscanf(scanline(), "%d", &method);

#if defined SEQ66_USE_FRUITY_CODE         /* will not be supported in seq66   */
        /*
         * This now returns true if the value was correct, we should check it.
         */

        if (! rc_ref().interaction_method(method))
            (void) make_error_message("interaction-method", "illegal value");

#endif  // SEQ66_USE_FRUITY_CODE

        if (next_data_line(file))
        {
            sscanf(scanline(), "%d", &method);
            rc_ref().allow_mod4_mode(method != 0);
        }
        if (next_data_line(file))
        {
            sscanf(scanline(), "%d", &method);
            rc_ref().allow_snap_split(method != 0);
        }
        if (next_data_line(file))
        {
            sscanf(scanline(), "%d", &method);
            rc_ref().allow_click_edit(method != 0);
        }
    }
    else
    {
        /* A missing interaction-method section is not an error. */
    }

#if defined SEQ66_LASH_SUPPORT_MOVED

        /*
         * This option is moved to the "usr" file's [user-session] section.
         */

        if (line_after(file, "[lash-session]"))
        {
            sscanf(scanline(), "%d", &method);
            rc().lash_support(method != 0);
        }
#endif

    int method = 1;             /* preserve seq24 option if not present     */
    if (line_after(file, "[auto-option-save]"))
    {
        int count = sscanf(scanline(), "%d", &method);
        if (count == 1)
        {
            rc_ref().auto_option_save(bool(method));
        }
        else
        {
            bool flag = get_boolean(file, "[auto-option-save]", "auto-save-rc");
            rc_ref().auto_option_save(flag);
        }

        bool flag = get_boolean
        (
            file, "[auto-option-save]", "save-old-triggers"
        );
        rc_ref().save_old_triggers(flag);
        flag = get_boolean(file, "[auto-option-save]", "save-old-mutes");
        rc_ref().save_old_mutes(flag);
    }
    file.close();               /* done parsing the "rc" file               */
    return true;
}

/**
 *  Parses the [midi-control] section.  This function is used both in the
 *  original reading of the "rc" file, and for reloading the original
 *  midi-control data from the "rc".
 *
 *  We used to throw the midi-control count value away, since it was always
 *  1024, but it is useful if no mute groups have been created.  So, if it
 *  reads 0 (instead of 1024), we will assume there are no midi-control
 *  settings.  We also have to be sure to go to the next data line even if the
 *  strip-empty-mutes option is on.
 *
 * \return
 *      Returns true if the file was able to be opened for reading, and the
 *      desired data successfully extracted.
 */

bool
rcfile::parse_midi_control_section
(
    const std::string & fname,
    bool separatefile
)
{
    midicontrolfile mcf(separatefile ? fname : name(), rc_ref());
    return mcf.parse();
}

/**
 *  Parses the [mute-group] section.  This function is used both in the
 *  original reading of the "rc" file, and for reloading the original
 *  mute-group data from the "rc".
 *
 *  We used to throw the mute-group count value away, since it was always
 *  1024, but it is useful if no mute groups have been created.  So, if it
 *  reads 0 (instead of 1024), we will assume there are no mute-group
 *  settings.  We also have to be sure to go to the next data line even if the
 *  strip-empty-mutes option is on.
 *
 * \return
 *      Returns true if the file was able to be opened for reading, and the
 *      desired data successfully extracted.
 */

bool
rcfile::parse_mute_group_section
(
    const std::string & fname,
    bool separatefile
)
{
    mutegroupsfile mgf(separatefile ? fname : name(), rc_ref());
    return mgf.parse();
}

/**
 *  This options-writing function is just about as complex as the
 *  options-reading function.
 *
 * \return
 *      Returns true if the write operations all succeeded.
 */

bool
rcfile::write ()
{
    std::ofstream file(name(), std::ios::out | std::ios::trunc);
    bool ok = file.is_open();
    if (ok)
    {
        file_message("Writing 'rc'", name());
    }
    else
    {
        file_error("Write open fail", name());
        return false;
    }

    /*
     * Initial comments and MIDI control section.
     */

    write_date(file, "main ('rc')");
    file <<
            "# This file holds the main configuration options for Seq66.\n"
            "# It loosely follows the format of the seq24 'rc' configuration\n"
            "# file, but adds many new options, and is no longer compatible.\n"
            "\n"
            "[Seq66]\n"
            "\n"
            "# The sets-mode determines if sets are muted when going to the\n"
            "# next play-screen ('normal'), while 'autoarm' will automatically\n"
            "# unmute the next set.  The 'additive' options keeps the previous\n"
            "# set unmuted when moving to the next set.\n"
            "#\n"
            "# The port-naming values are 'short' or 'long'.  The short style\n"
            "# just shows the port number and short port name; the long style\n"
            "# shows all the numbers and the long port name.\n"
            "\n"
            "config-type = \"rc\"\n"
            "version = " << version() << "\n"
            "verbose = " << bool_to_string(rc().verbose()) << "\n"
            "sets-mode = " << rc().sets_mode_string() << "\n"
            "port-naming = " << rc().port_naming_string() << "\n"
        ;

    /*
     * [comments]
     */

    file << "\n"
    "# The [comments] section holds the user's documentation for this file.\n"
    "# Lines starting with '#' and '[' are ignored.  Blank lines are ignored;\n"
    "# add a blank line by adding a space character to the line.\n"
        << "\n[comments]\n\n" << rc_ref().comments_block().text() << "\n"
        ;

    /*
     * Note that, if there are no controls (e.g. there were none to read, as
     * occurs the first time Seq66 is run), then we create one and populate it
     * with blanks.
     *
     * rc_ref().use_midi_control_file() is now always true.  The check and the
     * else clause removed on 2021-05-12.
     */

    std::string mcfname = rc_ref().midi_control_filespec();
    bool exists = file_exists(mcfname);
    bool active = rc_ref().midi_control_active();
    const midicontrolin & ctrls = rc_ref().midi_control_in();
    bool result = ctrls.count() > 0;
    if (result)
    {
        if (active || ! exists)
        {
            /*
             * Eventually we should provide a write_midi_contro_file() function
             * in the midicontrolfile module to do this work.
             */

            midicontrolfile mcf(mcfname, rc_ref());
            result = mcf.container_to_stanzas(ctrls);
            if (result)
                ok = mcf.write();
        }
    }
    else
    {
        if (active || ! exists)
        {
            keycontainer keys;              /* call the default constructor */
            midicontrolin & ctrls = rc_ref().midi_control_in();
            midicontrolfile mcf(mcfname, rc_ref());
            ctrls.add_blank_controls(keys);
            result = mcf.container_to_stanzas(ctrls);
            if (result)
                ok = mcf.write();
        }
    }
    file <<
       "\n"
       "# This section provides a flag and a file-name for MIDI-control I/O\n"
       "# settings. Use '\"\"' to indicate no 'ctrl' file. If none, the default\n"
       "# internal keystrokes are used, with no MIDI control I/O.\n"
       ;
    write_file_status
    (
        file, "[midi-control-file]",
        rc_ref().midi_control_filename(), rc_ref().midi_control_active()
    );

    /*
     * rc_ref().use_mute_group_file() is now always true.  The check and the
     * else clause removed on 2021-05-12.
     */

    bool addfilename = ! rc().mute_group_filename().empty();
    if (addfilename)
    {
        std::string mgfname = rc_ref().mute_group_filespec();
        mutegroupsfile mgf(mgfname, rc_ref());
        mgfname = rc_ref().trim_home_directory(mgfname);
        mgfname = add_quotes(mgfname);
        file << "\n[mute-group-file]\n\n" << mgfname << "\n";

        const mutegroups & mgroups = rc().mute_groups();
        if (mgroups.group_save_to_mutes())
            ok = mgf.write();
    }

    std::string usrname = rc_ref().user_filespec();
    usrname = rc_ref().trim_home_directory(usrname);
    usrname = add_quotes(usrname);
    file << "\n[usr-file]\n\n" << usrname << "\n\n";

    file
        << "[playlist]\n\n"
        "# Provides a configured play-list file and a flag to activate it.\n"
        "# playlist_active: 1 = active, 0 = do not use it.\n\n"
        << (rc_ref().playlist_active() ? "1" : "0")
        << "\n"
        ;

    file << "\n"
        "# Provides the name of a play-list file. If there is none, use '\"\"',\n"
        "# or set the flag above to 0. Use the extension '.playlist'.\n"
        "\n"
        ;

    std::string plname = rc_ref().playlist_filename();
    plname = rc_ref().trim_home_directory(plname);
    plname = add_quotes(plname);
    file << plname << "\n";

    file << "\n"
		"# Optional MIDI-file base-directory for play-list files. If given, it\n"
		"# sets the base directory which holds all MIDI files in all playlists.\n"
		"# Helpful when moving a complete set of playlists from one directory\n"
		"# to another. It preserves the sub-directories.\n"
        "\n"
		;

	std::string mbasedir = rc_ref().midi_base_directory();
    mbasedir = add_quotes(mbasedir);
    file << mbasedir << "\n";

    file << "\n"
        "[note-mapper]\n\n"
        "# Provides a configured note-map and a flag to activate it.\n"
        "# notemap_active: 1 = active, 0 = do not use it.\n\n"
        << (rc_ref().notemap_active() ? "1" : "0") << "\n\n"
        << "# Provides the name of the note-map file. If none, use '\"\"'.\n"
           "# Use the extension '.drums'.  This file is used only when the user\n"
           "# invokes the note-conversion operation in the pattern editor.\n\n"
        ;

    std::string nmname = rc_ref().notemap_filespec();
    nmname = rc_ref().trim_home_directory(nmname);
    nmname = add_quotes(nmname);
    file << nmname << "\n";

    /*
     * New section for palette file.
     */

    file <<
       "\n"
       "# This section provides a flag and a file-name to allow modifying the\n"
       "# palette using the file given below.  Use '\"\"' to indicate no\n"
       "# 'palette' file. If none, the internal palette is used.\n"
       ;
    write_file_status
    (
        file, "[palette-file]",
        rc_ref().palette_filename(), rc_ref().palette_active()
    );

    /*
     * New section for MIDI meta events.
     */

    file
        << "\n[midi-meta-events]\n\n"
           "# This section defines features of MIDI meta-event handling.\n"
           "# Normally, tempo events occur in the first track (pattern 0), but\n"
           "# one can move tempo elsewhere to accomodate existing tunes. It\n"
           "# affects where tempo events are recorded.  The default is 0, the\n"
           "# maximum is 1023. A pattern must exist at this number.\n"
           "\n"
        << rc_ref().tempo_track_number() << "    # tempo_track_number\n"
        ;

    /*
     * Manual ports
     */

    file
        << "\n[manual-ports]\n\n"
           "# Set to 1 to have Seq66 create virtual ALSA/JACK I/O ports and not\n"
           "# auto-connect to other clients.  It allows up to 48 output ports\n"
           "# and 48 input ports (defaults to 8 and 4). Set the flag to 0 to\n"
           "# auto-connect Seq66 to the system's existing ALSA/JACK MIDI ports.\n"
           "\n"
        << (rc_ref().manual_ports() ? "1" : "0")
        << "   # flag for manual (virtual) ALSA or JACK ports\n"
        << rc().manual_port_count()
        << "   # number of manual/virtual output ports\n"
        << rc().manual_in_port_count()
        << "   # number of manual/virtual input ports\n"
        ;

    int inbuses = bussbyte(rc_ref().inputs().count());
    file <<
           "\n[midi-input]\n\n"
           "# These ports can be used for input into Seq66. From JACK's view,\n"
           "# these are 'playback' devices. The first number is a buss number,\n"
           "# and the second number is the input status, disabled (0) or\n"
           "# enabled (1). The item in quotes is the input-buss name.\n"
           "\n"
        << int(inbuses) << "   # number of input MIDI busses\n\n"
        ;

    for (bussbyte bus = 0; bus < inbuses; ++bus)
    {
        bool bus_on = rc_ref().inputs().get(bus);
        std::string activestring = bus_on ? "1" : "0";
        file
            << int(bus) << " " << activestring << "    \""
            << rc_ref().inputs().get_name(bus) << "\"\n"
            ;
    }

    const inputslist & inpsref = input_port_map();
    if (inpsref.not_empty())
    {
        bool active = inpsref.active();
        std::string activestring = active ? "1" : "0";
        std::string mapstatus = "map is ";
        if (! active)
            mapstatus += "not ";

        mapstatus += "active";
        file
        << "\n[midi-input-map]\n\n"
        << "# This table is similar to the [midi-clock-map] section.\n"
           "# Port-mapping is disabled in manual/virtual port mode.\n\n"
        << activestring << "   # " << mapstatus << "\n\n"
        << input_port_map_list()
        ;
    }

    /*
     * Bus mute/unmute data.  At this point, we can use the master_bus()
     * accessor, even if a pointer dereference, because it was created at
     * application start-up, and here we are at application close-down.
     *
     * However, since we get these from the 'rc' file via the
     * rc_refs().clocks() container accessor, we should depend on the
     * performer class getting them, and then passing them to rcsettings via
     * the performer::put_settings() function.
     */

    bussbyte outbuses = bussbyte(rc_ref().clocks().count());
    file << "\n"
       "[midi-clock]\n\n"
       "# These ports are used for output from Seq66, for playback/control.\n"
       "# From JACK's view, these are 'capture' devices. The first line shows\n"
       "# the count of MIDI 'capture' ports. Each line shows the buss number\n"
       "# and clock status of that buss:\n"
       "#\n"
       "#   0 = MIDI Clock is off.\n"
       "#   1 = MIDI Clock on; Song Position and MIDI Continue will be sent.\n"
       "#   2 = MIDI Clock Module.\n"
       "#  -1 = The output port is disabled.\n"
       "#\n"
       "# With Clock Modulo, MIDI clocking doesn't begin until the song\n"
       "# position reaches the start modulo value [midi-clock-mod-ticks].\n"
       "# One can disable a port manually for devices that are present, but\n"
       "# not available, perhaps because another application has exclusive\n"
       "# access to the device (e.g. on Windows).\n\n"
        << int(outbuses) << "    # number of MIDI clocks (output busses)\n\n"
        ;

    for (bussbyte bus = 0; bus < outbuses; ++bus)
    {
        int bus_on = clock_to_int(rc_ref().clocks().get(bus));
        file
            << int(bus) << " " << bus_on << "    \""
            << rc_ref().clocks().get_name(bus) << "\"\n"
            ;
    }

    const clockslist & outsref = output_port_map();
    if (outsref.not_empty())
    {
        bool active = outsref.active();
        std::string activestring = active ? "1" : "0";
        std::string mapstatus = "map is ";
        if (! active)
            mapstatus += "not ";

        mapstatus += "active";
        file
        << "\n[midi-clock-map]\n\n"
           "# This table, if present, allows the pattern to set buss numbers\n"
           "# as usual, but the use the table to look up the true buss number\n"
           "# by the short form of the port name. Thus, if the ports change\n"
           "# their order in the MIDI system, the pattern can still output to\n"
           "# the proper port. The short names are the same with ALSA or with\n"
           "# JACK with the a2jmidi bridge running. Note that port-mapping is\n"
           "# disabled in manual/virtual port mode.\n\n"
        << activestring << "   # " << mapstatus << "\n\n"
        << output_port_map_list()
        ;
    }

    /*
     * MIDI clock modulo value
     */

    file << "\n[midi-clock-mod-ticks]\n\n"
        "# The Song Position (in 16th notes) at which clocking will begin\n"
        "# if the buss is set to MIDI Clock mod setting.\n"
        "\n"
        << midibus::get_clock_mod() << "\n"
       ;

    /*
     * Filter by channel, new option as of 2016-08-20
     */

    file
        << "\n"
        << "# If set to 1, this option allows the master MIDI bus to record\n"
           "# (filter) incoming MIDI data by channel, allocating each incoming\n"
           "# MIDI event to the sequence that is set to that channel.\n"
           "# This is an option adopted from the Seq32 project at GitHub.\n"
           "\n"
        << (rc_ref().filter_by_channel() ? "1" : "0")
        << "   # flag to record incoming data by channel\n"
        ;

    /*
     * Reveal ports
     */

    file
        << "\n[reveal-ports]\n\n"
           "# Set to 1 to have Seq66 ignore any system port names\n"
           "# declared in the 'user' configuration file.  Use this option to\n"
           "# be able to see the port names as detected by ALSA/JACK.\n"
           "\n"
        << (rc_ref().reveal_ports() ? "1" : "0")
        << "   # flag for reveal ports\n"
        ;

    /*
     * Interaction-method
     */

    file
        << "\n[interaction-method]\n\n"
           "# Sets the mouse handling style for drawing and editing a pattern\n"
           "# This feature is currently NOT supported in Seq66. However, \n"
           "# there are some other interaction settings available.\n"
        ;

#if defined SEQ66_USE_FRUITY_CODE         /* will not be supported in seq66   */

    int x = 0;
    while (c_interaction_method_names[x] && c_interaction_method_descs[x])
    {
        file
            << "# " << x
            << " - '" << c_interaction_method_names[x]
            << "' (" << c_interaction_method_descs[x] << ")\n"
            ;
        ++x;
    }
    file
        << "\n" << rc_ref().interaction_method() << "   # interaction_method\n\n"
        ;

#endif  // SEQ66_USE_FRUITY_CODE

    file
        << "\n# Set to 1 to allow Seq66 to stay in note-adding mode when\n"
           "# the right-click is released while holding the Mod4 (Super or\n"
           "# Windows) key.\n"
           "\n"
        << (rc_ref().allow_mod4_mode() ? "1" : "0")
        << "   # allow_mod4_mode\n\n"
        ;

    file
        << "# Set to 1 to allow Seq66 to split performance editor\n"
           "# triggers at the closest snap position, instead of splitting the\n"
           "# trigger exactly in its middle.  Remember that the split is\n"
           "# activated by a middle click or Ctrl-left click.\n"
           "\n"
        << (rc_ref().allow_snap_split() ? "1" : "0")
        << "   # allow_snap_split\n\n"
        ;

    file
        << "# Set to 1 to allow a double-click on a slot to bring it up in\n"
           "# the pattern editor.  This is the default.  Set it to 0 if\n"
           "# it interferes with muting/unmuting a pattern.\n"
           "\n"
        << (rc_ref().allow_click_edit() ? "1" : "0")
        << "   # allow_click_edit\n"
        ;

    std::string jacktransporttype = "none";
    if (rc_ref().with_jack_master())
        jacktransporttype = "master";
    else if (rc_ref().with_jack_transport())
        jacktransporttype = "slave";
    else if (rc_ref().with_jack_master_cond())
        jacktransporttype = "conditional";

    file
        << "\n[jack-transport]\n\n"
        "# jack_transport - Enable slave synchronization with JACK Transport.\n"
        "# Also contains the new flag to use JACK MIDI. Now simplified to\n"
        "# use variables instead of 0/1 flags. JACK transport values can be\n"
        "# 'none', 'slave', 'master', and 'conditional'.\n"
        "#\n"
        "#   none: No JACK Transport in use.\n"
        "#   slave: Seq66 attempts to use JACK Transport as Slave.\n"
        "#   master: Seq66 attempts to serve as JACK Transport Master.\n"
        "#   conditional: Seq66 is JACK master if no JACK master exists.\n\n"
        << "transport-type = " << jacktransporttype << "\n\n"
        ;

    std::string songmodeflag = bool_to_string(rc_ref().song_start_mode());
    std::string jackmidiflag = bool_to_string(rc_ref().with_jack_midi());
    file
        << "# song-start-mode is one of the following values:\n"
        "#\n"
        "# false: Playback in Live mode. Allows muting and unmuting of loops.\n"
        "#        from the main (patterns) window. Same as 'live'.\n"
        "# true:  Playback uses the Song (performance) editor's data and mute\n"
        "#        controls. Same as 'song'.\n\n"
        << "song-start-mode = " << songmodeflag << "\n\n"
        "# jack-midi sets/unsets JACK MIDI, separate from JACK Transport.\n\n"
        << "jack-midi = " << jackmidiflag << "\n"
        ;

#if defined SEQ66_LASH_SUPPORT_MOVED
        /*
         * This option is moved to the "usr" file's [user-session] section.
         */

        file << "\n"
            "[lash-session]\n\n"
            "# Set this value to 0 to disable LASH session management.\n"
            "# Set it to 1 to enable LASH session management.\n"
            "# This value will have no effect if LASH support is not built in.\n"
            "\n"
            << (rc().lash_support() ? "1" : "0")
            << "     # LASH session management support flag\n"
            ;
#endif

    std::string autosave = bool_to_string(rc_ref().auto_option_save());
    file << "\n"
        "[auto-option-save]\n\n"
        "# Set this value to false to disable the automatic saving of the\n"
        "# current configuration to 'rc' and other files.  Set it to true to\n"
        "# follow Seq24 behavior of saving the configuration at exit.\n"
        "# Note that, if auto-save is set, many of the command-line settings,\n"
        "# such as the JACK/ALSA settings, are saved to the configuration,\n"
        "# which can confuse one at first.  Also note that one currently needs\n"
        "# this option set to true to save the configuration; there is no\n"
        "# user-interface control for it at present.\n\n"
        << "auto-save-rc = " << autosave << "\n"
        ;

    std::string oldtrigs = bool_to_string(rc_ref().save_old_triggers());
    std::string oldmutes = bool_to_string(rc_ref().save_old_mutes());
    file << "\n"
        "# Set the following values to true to save triggers in a format\n"
        "# compatible with Seq24.  Otherwise, triggers are saved with an\n"
        "# additional 'transpose' setting, and mutes are saved as a byte,\n"
        "# instead of a long value.\n\n"
        << "save-old-triggers = " << oldtrigs << "\n"
        << "save-old-mutes = " << oldmutes << "\n"
        ;

    std::string lud = rc_ref().last_used_dir();
    lud = add_quotes(lud);
    file << "\n"
        "[last-used-dir]\n\n"
        "# Last-used and currently-active directory:\n\n"
        << lud << "\n"
        ;

    /*
     *  Feature from Kepler34.
     */

    int count = rc_ref().recent_file_count();
    file << "\n"
        "[recent-files]\n\n"
        "# Holds a list of the last few recently-loaded MIDI files. The first\n"
        "# number is the number of items in the list.  The second value\n"
        "# indicates if to load the most recent file (the top of the list)\n"
        "# at startup (1 = load it, 0 = do not load it).\n\n"
        << count << " " << (rc_ref().load_most_recent() ? "1" : "0") << "\n\n"
        ;

    if (count > 0)
    {
        for (int i = 0; i < count; ++i)
        {
            std::string rfilespec = rc_ref().recent_file(i, false);
            rfilespec = add_quotes(rfilespec);
            file << rfilespec << "\n";
        }
        file << "\n";
    }

    file
        << "# End of " << name() << "\n#\n"
        << "# vim: sw=4 ts=4 wm=4 et ft=dosini\n"
        ;

    file.close();
    return true;
}

/**
 *  Writes the [midi-control] section to the given file stream.  It leverages
 *  the function midicontrolfile::write_midi_conrol().
 *
 * \param file
 *      Provides the output file stream to write to.
 *
 * \param separatefile
 *      If true, the [midi-control] section is being written to a separate
 *      file.  The default value is false.
 *
 * \return
 *      Returns true if the write operations all succeeded.
 */

bool
rcfile::write_midi_control (std::ofstream & file)
{
    midicontrolfile mcf(name(), rc_ref());
    bool result = mcf.container_to_stanzas(rc_ref().midi_control_in());
    if (result)
        result = mcf.write_midi_control(file);

    return result;
}

/**
 *  Writes the [mute-group] section to the given file stream.  It leverages
 *  the function mutegroupsfile::write_mute_groups().
 *
 * \param file
 *      Provides the output file stream to write to.
 *
 * \param separatefile
 *      If true, the [mute-group] section is being written to a separate
 *      file.  The default value is false.
 *
 * \return
 *      Returns true if the write operations all succeeded.
 */

bool
rcfile::write_mute_groups (std::ofstream & file)
{
    mutegroupsfile mgf(name(), rc_ref());
    return mgf.write_mute_groups(file);
}

}           // namespace seq66

/*
 * rcfile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


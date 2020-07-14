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
 *  the <code> ~/.config/seq66.rc </code> ("rc") configuration file.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-23
 * \updates       2020-07-07
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

/**
 *  Provides names for the mouse-handling used by the application.
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

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Principal constructor.
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
    // Empty body
}

/**
 *  A rote destructor.
 */

rcfile::~rcfile ()
{
    // Empty body
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
 *  [interaction-method]
 *
 *      This section specified the kind of mouse interaction.
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
    if (! file.is_open())
    {
        char temp[128];
        snprintf
        (
            temp, sizeof temp,
            "rcfile::parse(): error opening %s for reading\n", name().c_str()
        );
        return make_error_message("parse", temp);
    }
    file.seekg(0, std::ios::beg);                       /* seek to start    */

    std::string s = get_variable(file, "[Seq66]", "version");
    if (! s.empty())
    {
        int version = string_to_int(s);
        if (version != rc_ref().ordinal_version())
            (void) make_error_message("parse", "'rc' ordinal version changed!");
    }

    /*
     * [comments] Header comments (hash-tag lead) is skipped during parsing.
     * However, we now try to read an optional comment block.
     */

    std::string comments = parse_comments(file);
    if (! comments.empty())
        rc_ref().comments_block().set(comments);

    bool ok = true;                                     /* start hopefully! */
    if (line_after(file, "[midi-control-file]"))
    {
        ok = ! line().empty();                          /* still there?     */
        if (ok)
        {
            rc_ref().midi_control_filename(line());     /* set base name    */

            std::string fullpath = rc_ref().midi_control_filespec();
            pathprint("Reading MIDI control file", fullpath);
            ok = parse_midi_control_section(fullpath, true);
            if (! ok)
            {
                std::string info = "cannot parse file '";
                info += fullpath;
                info += "'";
                rc_ref().midi_control_filename("");
                return make_error_message("midi-control-file", info);
            }
        }
        rc_ref().use_midi_control_file(ok);             /* did it work?     */
    }
    else
    {
        /*
         * This call causes parsing to skip all of the header material.
         * Please note that the line_after() function starts from the
         * beginning of the file every time.  A lot a rescanning!  But it goes
         * fast these days.  There is a new position argument, but it is not
         * used here.
         */

        ok = parse_midi_control_section(name());
        rc_ref().use_midi_control_file(false);
    }

    if (line_after(file, "[mute-group-file]"))
    {
        ok = ! line().empty();                          /* still there?     */
        if (ok)
        {
            rc_ref().mute_group_filename(line());       /* base name        */

            std::string fullpath = rc_ref().mute_group_filespec();
            pathprint("Reading mute group file", fullpath);
            ok = parse_mute_group_section(fullpath, true);
            if (! ok)
            {
                std::string info = "cannot parse file '";
                info += fullpath;
                info += "'";
                rc_ref().midi_control_filename("");
                return make_error_message("mute-group-file", info);
            }
        }
        rc_ref().use_mute_group_file(ok);               /* did it work?     */
    }
    else
    {
        /*
         * [mute-group]
         *
         * After we parse the mute group, we need to see if there is another
         * value for the mute_group_handling enumeration.  One little issue...
         * the parse_mute_group_section() function actually re-opens the file
         * itself, and once it exits, it's as if the section never existed.
         * So we also have to pase the new mute-group handling feature there
         * as well.
         */

        ok = parse_mute_group_section(name());
        rc_ref().use_mute_group_file(false);
    }

    if (ok)
        ok = line_after(file, "[midi-clock]");

    long buses = 0;
    if (ok)
    {
        sscanf(scanline(), "%ld", &buses);
        ok = next_data_line(file) && buses > 0 && buses <= c_busscount_max;
    }
    if (ok)
    {
        /**
         * One thing about MIDI clock values.  If a device (e.g. my Korg
         * nanoKEY2) is present in a system when Seq66 is exited, it
         * will be saved in the [midi-clock] list.  When unplugged, it will be
         * read here at startup, but won't be shown.  The next exit will find
         * it removed from this list.
         *
         * Also, we want to pre-allocate the number of clock entries needed,
         * and then use the buss number to populate the list of clocks, in the
         * odd event that the user changed the bus-order of the entries.
         */

        rc_ref().clocks().resize(size_t(buses));
        for (int i = 0; i < buses; ++i)
        {
            int bus, bus_on;
            int count = sscanf(scanline(), "%d %d", &bus, &bus_on);
            ok = count == 2;
            if (ok)
            {
                rc_ref().clocks().set(bus, static_cast<e_clock>(bus_on));
                ok = next_data_line(file);
                if (! ok)
                {
                    if (i < (buses-1))
                    {
                        return make_error_message
                        (
                            "midi-clock", "data line missing"
                        );
                    }
                }
            }
            else
            {
                return make_error_message
                (
                    "midi-clock", "data line needs 2 values"
                );
            }
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

        rc_ref().clocks().add(e_clock::off, "Bad clock count");
    }

    /*
     * JACK transport settings are currently accessed only via the rcsetting's
     * rc() accessor function.
     */

    int flag = 0;
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

    /*
     *  We are taking a slightly different approach to this section.  When
     *  Seq66 exits, it saves all of the inputs it has.  If an input is
     *  removed from the system (e.g. unplugging a MIDI controller), then
     *  there will be too many entries in this section.  The user might remove
     *  one, and forget to update the buss count.  So we basically ignore the
     *  buss count.  But we also have to read the  channel-filter boolean.  If
     *  an error occurs, we abort... the user must fix the "rc" file.
     */

    if (line_after(file, "[midi-input]"))
    {
        int buses = 0;
        int count = sscanf(scanline(), "%d", &buses);
        if (count > 0 && buses > 0)
        {
            int b = 0;
            rc_ref().inputs().resize(size_t(buses));
            while (next_data_line(file))
            {
                long bus, bus_on;
                count = sscanf(scanline(), "%ld %ld", &bus, &bus_on);
                if (count == 2)
                {
                    rc_ref().inputs().set(bus, bool(bus_on));
                    ++b;
                }
                else if (count == 1)
                {
                    bool flag = bool(bus);
                    rc_ref().filter_by_channel(flag);
                    toggleprint("Filter-by-channel", flag);
                }
            }
            if (b < buses)
                return make_error_message("midi-input", "too few buses");
        }
    }
    else
        return make_error_message("midi-input", "data lines missing");

    if (line_after(file, "[midi-clock-mod-ticks]"))
    {
        long ticks = 64;
        sscanf(scanline(), "%ld", &ticks);
        rc_ref().set_clock_mod(ticks);
    }
    else
    {
        return make_error_message
        (
            "midi-clock-mod-ticks", "data lines missing"
        );
    }
    if (line_after(file, "[midi-meta-events]"))
    {
        int track = 0;
        sscanf(scanline(), "%d", &track);
        rc_ref().tempo_track_number(track); /* MIDI file can override this  */
    }
    else
    {
        return make_error_message
        (
            "midi-meta_events", "data lines missing"
        );
    }
    if (line_after(file, "[manual-ports]"))
    {
        sscanf(scanline(), "%d", &flag);
        rc_ref().manual_ports(bool(flag));
        if (next_data_line(file))
        {
            int count;
            sscanf(scanline(), "%d", &count);
            rc().manual_port_count(count);
        }
    }
    else
    {
        (void) make_error_message("manual-ports", "data line missing");
    }
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
    {
        (void) make_error_message("reveal-ports", "data line missing");
    }
    if (line_after(file, "[last-used-dir]"))
    {
        if (! line().empty())
            rc_ref().last_used_dir(line()); // FIXME: check for valid path
    }
    else
    {
         (void) make_error_message("last-used-dir", "data line missing");
    }
    if (line_after(file, "[recent-files]"))
    {
        int count;
        sscanf(scanline(), "%d", &count);
        for (int i = 0; i < count; ++i)
        {
            if (next_data_line(file))
            {
                if (! line().empty())
                {
                    if (! rc_ref().append_recent_file(std::string(line())))
                        break;
                }
            }
            else
                break;
        }
    }
    else
    {
        (void) make_error_message("recent-files", "data lines missing");
    }
    if (line_after(file, "[playlist]"))
    {
        bool exists = false;
        int flag = 0;
        sscanf(scanline(), "%d", &flag);
        if (flag != 0)
        {
            if (next_data_line(file))
            {
                std::string fname = trimline();
                exists = ! fname.empty() && fname != "\"\"";
                if (exists)
                {
                    /*
                     * Prepend the home configuration directory and, if needed,
                     * the playlist extension.
                     */

                    fname = rc_ref().make_config_filespec(fname, ".playlist");
                    exists = file_exists(fname);
                    if (exists)
                    {
                        rc_ref().playlist_active(true);
                        rc_ref().playlist_filename(fname);
                    }
                    else
                        file_error("no such playlist", fname);
                }
            }
        }
        if (! exists)
        {
            rc_ref().playlist_active(false);
            rc_ref().playlist_filename("");
        }
    }
    else
    {
        /* A missing playlist section is not an error. */
    }
    if (line_after(file, "[note-mapper]"))
    {
        bool exists = false;
        std::string fname = trimline();
        exists = ! fname.empty() && fname != "\"\"";
        if (exists)
        {
            /*
             * Prepend the home configuration directory and, if needed,
             * the playlist extension.
             */

            fname = rc_ref().make_config_filespec(fname, ".drums");
            exists = file_exists(fname);
            if (exists)
            {
                rc_ref().notemap_filename(fname);
            }
            else
                file_error("no such note mapping", fname);
        }
        if (! exists)
            rc_ref().notemap_filename("");
    }
    else
    {
        /* A missing note-mapper section is not an error. */
    }

    int method = 0;

#if defined SEQ66_USE_FRUITY_CODE         /* will not be supported in seq66   */

    if (line_after(file, "[interaction-method]"))
    {
        sscanf(scanline(), "%d", &method);

        /*
         * This now returns true if the value was correct, we should check it.
         */

        if (! rc_ref().interaction_method(method))
            (void) make_error_message("interaction-method", "illegal value");

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

#endif  // SEQ66_USE_FRUITY_CODE

#if defined SEQ66_LASH_SUPPORT
        if (line_after(file, "[lash-session]"))
        {
            sscanf(scanline(), "%d", &method);
            rc().lash_support(method != 0);
        }
#endif

    method = 1;                 /* preserve seq24 option if not present     */
    line_after(file, "[auto-option-save]");
    sscanf(scanline(), "%d", &method);
    rc_ref().auto_option_save(bool(method));
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
        pathprint("Writing rc configuration", name());
    }
    else
    {
        file_error("Error opening for writing", name());
        return false;
    }

    /*
     * Initial comments and MIDI control section.
     */

    file
        << "# Seq66 0.90.2 (and above) rc configuration file\n"
        << "#\n"
        << "# " << name() << "\n"
        << "# Written on " << current_date_time() << "\n"
        << "#\n"
        <<
    "# This file holds the main configuration options for Seq66.\n"
    "# It loosely follows the format of the seq24 'rc' configuration\n"
    "# file, but adds some new options, and is no longer compatible.\n"
    "\n"
    "[Seq66]\n\n"
    "config-type = \"rc\"\n"
    "version = 0\n"
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

    if (rc_ref().use_midi_control_file())
    {
        /*
         * Before using this [midi-control-file] file-name, change, if
         * necessary, the extension, from ".rc" to ".ctrl".  This will also be
         * changed in the [midi-control-file] section.
         */

        std::string mcfname = rc_ref().midi_control_filespec();
        midicontrolfile mcf(mcfname, rc_ref());
        const midicontrolin & ctrls = rc_ref().midi_controls();
        bool result = mcf.container_to_stanzas(ctrls);
        if (result)
        {
            mcfname = rc_ref().trim_home_directory(mcfname);
            file << "[midi-control-file]\n\n" << mcfname << "\n";
            ok = mcf.write();
        }
    }
    else
        ok = write_midi_control(file);

    if (rc_ref().use_mute_group_file())
    {
        /*
         * Before using this [mute-group-file] file-name, change, if
         * necessary, the extension, from ".rc" to ".mutes".  This will also
         * be changed in the [mute-group-file] section.
         */

        std::string mgfname = rc_ref().mute_group_filespec();
        mutegroupsfile mgf(mgfname, rc_ref());
        mgfname = rc_ref().trim_home_directory(mgfname);
        file << "\n[mute-group-file]\n\n" << mgfname << "\n";
        ok = mgf.write();
    }
    else
        ok = write_mute_groups(file);

    /*
     * Bus mute/unmute data.  At this point, we can use the master_bus()
     * accessor, even if a pointer dereference, because it was created at
     * application start-up, and here we are at application close-down.
     *
     * However, since we get these from the 'rc' file via the rc_refs().clocks()
     * container accessor, we should depend on the performer class getting them,
     * and then passing them to rcsettings via the performer::put_settings()
     * function.
     */

    bussbyte buses = bussbyte(rc_ref().clocks().count());
    file << "\n"
       "[midi-clock]\n\n"
       "# The first line indicates the number of MIDI busses defined.\n"
       "# Each buss line contains the buss (re 0) and the clock status of\n"
       "# that buss.  0 = MIDI Clock is off; 1 = MIDI Clock on, and Song\n"
       "# Position and MIDI Continue will be sent, if needed; 2 = MIDI\n"
       "# Clock Modulo, where MIDI clocking will not begin until the song\n"
       "# position reaches the start modulo value [midi-clock-mod-ticks].\n"
       "# A value of -1 indicates that the output port is totally\n"
       "# disabled.  One can set this value manually for devices that are\n"
       "# present, but not available, perhaps because another application\n"
       "# has exclusive access to the device (e.g. on Windows).\n"
       "\n"
        << int(buses) << "    # number of MIDI clocks (output busses)\n\n"
        ;
    for (bussbyte bus = 0; bus < buses; ++bus)
    {
        int bus_on = static_cast<int>(rc_ref().clocks().get(bus));
        file
            << "# Output buss name: " << rc_ref().clocks().get_name(bus) << "\n"
            << int(bus) << " " << bus_on << "   # buss number, clock status\n"
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
     * New section for MIDI meta events.
     */

    file
        << "\n[midi-meta-events]\n\n"
           "# This section defines some features of MIDI meta-event handling.\n"
           "# Normally, tempo events are supposed to occur in the first track\n"
           "# (pattern 0).  But one can move this track elsewhere to accomodate\n"
           "# one's existing body of tunes.  If affects where tempo events are\n"
           "# recorded.  The default value is 0, the maximum is 1023.\n"
           "# A pattern must exist at this number for it to work.\n"
           "\n"
        << rc_ref().tempo_track_number() << "    # tempo_track_number\n"
        ;

    buses = bussbyte(rc_ref().inputs().count());
    file
        << "\n[midi-input]\n\n"
        << int(buses) << "   # number of input MIDI busses\n\n"
           "# The first number is the port number, and the second number\n"
           "# indicates whether it is disabled (0), or enabled (1).\n"
           "\n"
        ;

    for (bussbyte bus = 0; bus < buses; ++bus)
    {
        int bus_on = static_cast<bool>(rc_ref().inputs().get(bus));
        file
            << "# Input buss name: " << rc_ref().inputs().get_name(bus) << "\n"
            << int(bus) << " " << bus_on << "  # buss number, input status\n"
            ;
    }

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
     * Manual ports
     */

    file
        << "\n[manual-ports]\n\n"
           "# Set to 1 to have seq66 create its own ALSA/JACK ports and not\n"
           "# connect to other clients.  Use 1 to expose all 16 MIDI ports to\n"
           "# JACK (e.g. via a2jmidid).  Use 0 to access the ALSA MIDI ports\n"
           "# already running on one's computer, or to use the autoconnect\n"
           "# feature (Seq66 connects to existing JACK ports on startup.\n"
           "# A new feature is to change the number of ports; defaults to 16.\n"
           "\n"
        << (rc_ref().manual_ports() ? "1" : "0")
        << "   # flag for manual (virtual) ALSA or JACK ports\n"
        << rc().manual_port_count() << "   # number of manual/virtual ports\n"
        ;

    /*
     * Reveal ports
     */

    file
        << "\n[reveal-ports]\n\n"
           "# Set to 1 to have seq66 ignore any system port names\n"
           "# declared in the 'user' configuration file.  Use this option to\n"
           "# be able to see the port names as detected by ALSA/JACK.\n"
           "\n"
        << (rc_ref().reveal_ports() ? "1" : "0")
        << "   # flag for reveal ports\n"
        ;

#if defined SEQ66_USE_FRUITY_CODE         /* will not be supported in seq66   */

    /*
     * Interaction-method
     */

    int x = 0;
    file
        << "\n[interaction-method]\n\n"
        << "# Sets the mouse handling style for drawing and editing a pattern\n"
        << "# This feature is current NOT supported in the Qt version of\n"
        << "# Seq66 (qpseq66).\n\n"
        ;

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
           "# activated by a middle click.\n"
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

    file
        << "\n[jack-transport]\n\n"
        "# jack_transport - Enable slave synchronization with JACK Transport.\n"
        "# Also contains the new flag to use JACK MIDI.\n\n"
        << rc_ref().with_jack_transport() << "   # with_jack_transport\n\n"
        "# jack_master - Seq66 attempts to serve as JACK Master.\n"
        "# Also must enable jack_transport (the user interface forces this,\n"
        "# and also disables jack_master_cond).\n\n"
        << rc_ref().with_jack_master() << "   # with_jack_master\n\n"
        "# jack_master_cond - Seq66 is JACK master if no other JACK\n"
        "# master exists. Also must enable jack_transport (the user interface\n"
        "# forces this, and disables jack_master).\n\n"
        << rc_ref().with_jack_master_cond()  << "   # with_jack_master_cond\n\n"
        "# song_start_mode applies to JACK/ALSA/PortMidi.\n\n"
        "# 0 = Playback in live mode. Allows muting and unmuting of loops.\n"
        "#     from the main (patterns) window.  Disables both manual and\n"
        "#     automatic muting and unmuting from the performance window.\n"
        "# 1 = Playback uses the song (performance) editor's data and mute\n"
        "#     controls, regardless of which window was used to start the\n"
        "#     playback.\n\n"
        << rc_ref().song_start_mode() << "   # song_start_mode\n\n"
        "# jack_midi - Enable JACK MIDI, which is a separate option from\n"
        "# JACK Transport.\n\n"
        << rc_ref().with_jack_midi() << "   # with_jack_midi\n"
        ;

#if defined SEQ66_LASH_SUPPORT
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

    file << "\n"
        "[auto-option-save]\n\n"
        "# Set the following value to 0 to disable the automatic saving of the\n"
        "# current configuration to the 'rc' and 'user' files.  Set it to 1 to\n"
        "# follow seq24 behavior of saving the configuration at exit.\n"
        "# Note that, if auto-save is set, many of the command-line settings,\n"
        "# such as the JACK/ALSA settings, are then saved to the configuration,\n"
        "# which can confuse one at first.  Also note that one currently needs\n"
        "# this option set to 1 to save the configuration, as there is not a\n"
        "# user-interface control for it at present.\n"
        "\n"
        << (rc_ref().auto_option_save() ? "1" : "0")
        << "     # auto-save-options-on-exit support flag\n"
        ;

    std::string lud = rc_ref().last_used_dir();
    if (lud.empty())
        lud = "\"\"";

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
        "# Holds a list of the last few recently-loaded MIDI files.\n\n"
        << count << "\n\n"
        ;

    if (count > 0)
    {
        for (int i = 0; i < count; ++i)
            file << rc_ref().recent_file(i, false) << "\n";

        file << "\n";
    }

    file
        << "[playlist]\n\n"
        "# Provides a configured play-list and a flag to activate it.\n"
        "# playlist_active: 1 = active, 0 = do not use it\n\n"
        << (rc_ref().playlist_active() ? "1" : "0")
        << "\n"
        ;

    file << "\n"
        "# Provides the name of a play-list. If there is none, use '\"\"',\n"
        "# or set the flag above to 0. Use the extension '.playlist'.\n"
        "\n"
        ;

    std::string plname = rc_ref().playlist_filespec();
    std::string fspec = "\"\"";
    if (! plname.empty())
    {
        fspec = file_extension_set(rc_ref().playlist_filespec(), ".playlist");
        fspec = rc_ref().trim_home_directory(fspec);
    }
    file << fspec << "\n\n";
    file
        << "[note-mapper]\n\n"
        "# Provides the name of a note-map file. If there is none, use '\"\"'.\n"
        "# Use the extension '.drums'.  This file is used only when the user\n"
        "# invokes the note-conversion operation in the pattern editor.\n"
        "\n"
        ;

    std::string nmname = rc_ref().notemap_filespec();
    fspec = "\"\"";
    if (! nmname.empty())
    {
        fspec = file_extension_set(rc_ref().notemap_filespec(), ".drums");
        fspec = rc_ref().trim_home_directory(fspec);
    }
    file << fspec << "\n\n";
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
    bool result = mcf.container_to_stanzas(rc_ref().midi_controls());
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


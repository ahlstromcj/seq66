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
 * \file          rcfile.cpp
 *
 *  This module declares/defines the base class for managing the
 *  the ~/.config/seq66.rc ("rc") configuration file.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-23
 * \updates       2021-11-18
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

#if defined SEQ66_USE_FRUITY_CODE       /* will not be supported in seq66   */

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

static const int s_rc_file_version = 2;

/**
 *  Principal constructor.
 *
 * Versions:
 *
 *      0:  The initial version, close to the Seq64 format.
 *      1:  2021-05-16. More modern JACK configuration settings.
 *      2:  2021-06-04. Transition to get-variable for booleans and integers,
 *                      finished on 2021-06-07.
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
    version(s_rc_file_version);
}

/**
 *  Parse the ~/.config/seq66/qseq66.rc file.  After this function is
 *  called, the performer::get_settings() function can be used to populate the
 *  performer with the settings it needs.
 *
 *  [midi-control-file]
 *
 *      The [midi-control] section is no longer supported.  The MIDI control
 *      information is in a separate file.  This allows the user to switch
 *      between different setups without having to mess with editing the "rc"
 *      file so much.
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
 *      This section covers various JACK settings, one setting per line.
 *      The following numbers are specfied:
 *
 *      -   jack_transport - Enable sync with JACK Transport.
 *      -   jack_master - Seq24 will attempt to serve as JACK Master.
 *      -   jack_master_cond - Seq24 will fail to be Master if there is
 *          already a Master set.
 *      -   jack_auto_connect - Automatically connect to discovered JACK
 *          ports.
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
    if (! set_up_ifstream(file))            /* verifies [Seq66]: version    */
        return false;

    bool verby = get_boolean(file, "[Seq66]", "verbose");
    std::string s = parse_version(file);
    if (s.empty() || file_version_old(file))
        rc_ref().auto_rc_save(true);

    rc().verbose(verby);
    s = get_variable(file, "[Seq66]", "sets-mode");
    rc().sets_mode(s);
    s = get_variable(file, "[Seq66]", "port-naming");
    rc().port_naming(s);

    /*
     * [comments] Header comments (hash-tag lead) are skipped during parsing.
     * However, we now try to read an optional comment block.
     */

    s = parse_comments(file);
    if (! s.empty())
        rc_ref().comments_block().set(s);

    bool ok = true;                                     /* start hopefully! */
    std::string tag = "[midi-control-file]";
    if (file_version_number() < s_rc_file_version)
    {
        (void) version_error_message("rc", file_version_number());
    }
    else
    {
        std::string pfname;
        bool active = get_file_status(file, tag, pfname);
        rc_ref().midi_control_active(active);
        rc_ref().midi_control_filename(pfname);             /* base name    */
    }

    std::string fullpath = rc_ref().midi_control_filespec();
    file_message("Reading ctrl", fullpath);
    ok = parse_midi_control_section(fullpath, true);
    if (! ok)
    {
        std::string info = "'";
        info += fullpath;
        info += "'";
        return make_error_message(tag, info);
    }

    tag = "[mute-group-file]";

    std::string pfname;
    bool active = get_file_status(file, tag, pfname);
    rc_ref().mute_group_active(active);
    rc_ref().mute_group_filename(pfname);   /* [[/]path/] basename.ext  */
    fullpath = rc_ref().mute_group_filespec();
    file_message("Reading mutes", fullpath);
    ok = parse_mute_group_section(fullpath, true);
    if (! ok)
    {
        std::string info = "cannot parse file '";
        info += fullpath;
        info += "'";
        return make_error_message(tag, info);
    }

    tag = "[usr-file]";
    active = get_file_status(file, tag, pfname);
    rc_ref().user_file_active(active);
    rc_ref().user_filename(pfname);                     /* base name    */

    tag = "[palette-file]";
    active = get_file_status(file, tag, pfname);
    rc_ref().palette_active(active);
    rc_ref().palette_filename(pfname);              /* base name    */

    /*
     * JACK transport settings are currently accessed only via the rcsetting's
     * rc() accessor function.  Also note that these setting must occur in the
     * given order in the 'rc' file: (1) Transport; (2) Master; (3)
     * Master-conditional.  Otherwise the coordination of these settings gets
     * messed up.
     */

    tag = "[jack-transport]";
    s = get_variable(file, tag, "transport-type");
    if (s.empty())
    {
        int flag = 0;
        if (line_after(file, tag))
        {
            std::sscanf(scanline(), "%d", &flag);                /* 1 */
            rc_ref().with_jack_transport(bool(flag));
            next_data_line(file);
            std::sscanf(scanline(), "%d", &flag);                /* 2 */
            rc_ref().with_jack_master(bool(flag));
            next_data_line(file);
            std::sscanf(scanline(), "%d", &flag);                /* 3 */
            rc_ref().with_jack_master_cond(bool(flag));
            next_data_line(file);
            std::sscanf(scanline(), "%d", &flag);                /* 4 */
            rc_ref().song_start_mode(bool(flag));
            if (next_data_line(file))
            {
                std::sscanf(scanline(), "%d", &flag);            /* 5 */
                rc_ref().with_jack_midi(bool(flag));
            }
        }
    }
    else
    {
        rc_ref().set_jack_transport(s);

        /*
         * If "live" or "song" are provided, then the "auto" boolean is false.
         */

        s = get_variable(file, tag, "song-start-mode");
        rc_ref().song_start_mode(s);

        bool flag = get_boolean(file, tag, "jack-midi");
        rc_ref().with_jack_midi(flag);
        s = get_variable(file, tag, "jack-auto-connect");
        if (s.empty())
        {
            rc_ref().jack_auto_connect(true);           /* legacy default   */
        }
        else
        {
            flag = get_boolean(file, tag, "jack-auto-connect");
            rc_ref().jack_auto_connect(flag);
        }
    }

    bool use_manual_ports = false;
    tag = "[manual-ports]";

    bool flag = get_boolean(file, tag, "virtual-ports");
    use_manual_ports = flag;
    rc_ref().manual_ports(flag);

    int count = get_integer(file, tag, "output-port-count");
    rc().manual_port_count(count);
    count = get_integer(file, tag, "input-port-count");
    rc().manual_in_port_count(count);

    /*
     *  When Seq66 exits, it saves all of the inputs it has.  If an input is
     *  removed from the system (e.g. unplugging a MIDI controller), then
     *  there will be too many entries in this section.  The user might remove
     *  one, and forget to update the buss count.  So we ignore the buss
     *  count. But we also have to read the channel-filter boolean.  If an
     *  error occurs, we abort...  the user must fix the 'rc' file.
     */

    tag = "[midi-input]";
    if (! use_manual_ports)
    {
        if (line_after(file, tag))
        {
            int inbuses = 0;
            int count = std::sscanf(scanline(), "%d", &inbuses);
            if (count > 0 && inbuses > 0)
            {
                int b = 0;
                rc_ref().inputs().clear();
                while (next_data_line(file))
                {
                    int bus, bus_on;
                    count = std::sscanf(scanline(), "%d %d", &bus, &bus_on);
                    if (count == 2)
                    {
                        rc_ref().inputs().add(bus, bool(bus_on), line());
                        ++b;
                    }
                    /*
                     * This is BOGUS
                     *
                    else if (count == 1)
                    {
                        bool flag = bool(bus);
                        rc_ref().filter_by_channel(flag);
                        toggleprint("Filter-by-channel", flag);
                    }
                     */
                }
                if (b < inbuses)
                    return make_error_message(tag, "too few buses");
            }
        }
        else
            return make_error_message(tag, "section missing");
    }

    /*
     *  Check for an optional input port map section.
     */

    tag = "[midi-input-map]";
    if (line_after(file, tag))
    {
        inputslist & inputref = input_port_map();
        int activeflag;
        int count = 0;
        inputref.clear();
        (void) std::sscanf(scanline(), "%d", &activeflag);
        inputref.active(activeflag != 0);
        for ( ; next_data_line(file); )
        {
            if (inputref.add_list_line(line()))
            {
                ++count;
            }
            else
            {
                inputref.clear();
                inputref.active(false);
                break;
            }
            infoprintf("%d midi-input-map entries added", count);
        }
    }
    tag = "[midi-clock]";
    if (ok)
        ok = ! use_manual_ports && line_after(file, tag);

    int outbuses = 0;
    if (ok)
    {
        std::sscanf(scanline(), "%d", &outbuses);
        ok = next_data_line(file) && is_good_busscount(outbuses);
    }
    if (ok)
    {
        /*
         * One thing about MIDI clock values.  If a device (e.g. Korg nanoKEY2)
         * is present in a system when Seq66 is exited, it will be saved in the
         * [midi-clock] list.  When unplugged, it will be read here at startup,
         * but won't be shown.  The next exit finds it removed from this list.
         * Also, we want to pre-allocate the number of clock entries needed, and
         * then use the buss number to populate the list of clocks, in the odd
         * event that the user changed the bus-order of the entries.
         */

        rc_ref().clocks().clear();
        for (int i = 0; i < outbuses; ++i)
        {
            int bus, bus_on;
            int count = std::sscanf(scanline(), "%d %d", &bus, &bus_on);
            ok = count == 2;
            if (ok)
            {
                e_clock e = int_to_clock(bus_on);
                rc_ref().clocks().add(bus, e, line());
                ok = next_data_line(file);
                if (! ok && i < (outbuses-1))
                    return make_error_message(tag, "missing data line");
            }
            else
                return make_error_message(tag, "data line error");
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

    tag = "[midi-clock-map]";
    if (line_after(file, tag))
    {
        clockslist & clocsref = output_port_map();
        int activeflag;
        int count = 0;
        clocsref.clear();
        (void) std::sscanf(scanline(), "%d", &activeflag);
        clocsref.active(activeflag != 0);
        for ( ; next_data_line(file); )
        {
            if (clocsref.add_list_line(line()))
            {
                ++count;
            }
            else
            {
                clocsref.clear();
                clocsref.active(false);
                break;
            }
        }
        infoprintf("%d midi-clock-map entries added", count);
    }

    int ticks = 64;
    bool recordbychannel = false;
    tag = "[midi-clock-mod-ticks]";
    ticks = get_integer(file, tag, "ticks");
    recordbychannel = get_boolean(file, tag, "record-by-channel");
    rc_ref().set_clock_mod(ticks);
    rc_ref().filter_by_channel(recordbychannel);

    int track = 0;
    tag = "[midi-meta-events]";
    track = get_integer(file, tag, "tempo-track");
    rc_ref().tempo_track_number(track);     /* MIDI file can override this  */
    tag = "[reveal-ports]";

    /*
     * If this flag is already raised, it was raised on the command line,
     * and we don't want to change it.  An ugly special case.
     */

    if (! rc_ref().reveal_ports())
    {
        bool flag = get_boolean(file, tag, "show-system-ports");
        rc_ref().reveal_ports(bool(flag));
    }

    tag = "[interaction-method]";

    flag = get_boolean(file, tag, "snap-split");
    rc_ref().allow_snap_split(flag);
    flag = get_boolean(file, tag, "double-click-edit");
    rc_ref().allow_click_edit(flag);
    tag = "[last-used-dir]";
    if (line_after(file, tag))
    {
        if (! is_missing_string(line()))
        {
            std::string ludir = strip_quotes(line());
            rc_ref().last_used_dir(ludir);
        }
    }
    else
         (void) make_error_message(tag, "section missing");

    tag = "[recent-files]";

    bool gotrf = line_after(file, tag);
    int recentcount;
    if (gotrf)
    {
        bool fullpaths = get_boolean(file, tag, "full-paths");
        bool loadem = get_boolean(file, tag, "load-most-recent");
        recentcount = get_integer(file, tag, "count");
        rc_ref().load_most_recent(loadem);
        rc_ref().full_recent_paths(fullpaths);
        (void) next_data_line(file);    /* skip "load-most-recent" line */
    }
    else
        (void) make_error_message(tag, "section missing");

    if (gotrf)
        gotrf = line_after(file, tag);                  /* start over       */

    if (gotrf)
    {
        while (line()[0] != '"')                        /* find quote       */
        {
            gotrf = next_data_line(file);
            if (! gotrf)
                break;
        }
        if (gotrf && line()[0] == '"')
        {
            rc_ref().clear_recent_files();
            for (int i = 0; i < recentcount; ++i)
            {
                std::string rfilename = strip_quotes(line());
                if (rfilename.empty())
                {
                    break;
                }
                else
                {
                    if (! rc_ref().append_recent_file(rfilename))
                        file_message("Cannot read recent file", rfilename);
                }
                if (! next_data_line(file))
                    break;
            }
        }
    }

    rc_ref().playlist_active(false);
    tag = "[playlist]";
    active = get_file_status(file, tag, pfname);
    rc_ref().playlist_active(active);
    rc_ref().playlist_filename(pfname);                 /* base name    */
    pfname = get_variable(file, tag, "base-directory");
    if (! is_missing_string(pfname))
    {
        file_message("Playlist MIDI base directory", pfname);
        rc_ref().midi_base_directory(pfname);
    }
    rc_ref().notemap_active(false);
    tag = "[note-mapper]";
    active = get_file_status(file, tag, pfname);
    rc_ref().notemap_active(active);
    rc_ref().notemap_filename(pfname);                  /* base name    */
    tag = "[auto-option-save]";
    flag = get_boolean(file, tag, "auto-save-rc");
    rc_ref().auto_rc_save(flag);

    bool f = get_boolean(file, tag, "save-old-triggers");
    rc_ref().save_old_triggers(f);
    f = get_boolean(file, tag, "save-old-mutes");
    rc_ref().save_old_mutes(f);
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
        file_message("Writing rc", name());
    }
    else
    {
        file_error("Write open fail", name());
        return false;
    }

    /*
     * Initial comments and MIDI control section.
     */

    std::string noname;
    write_date(file, "main ('rc')");
    file <<
        "# This file holds the main configuration for Seq66. It no longer\n"
        "# follows the format of the seq24rc configuration file much.\n"
        "#\n"
        "# 'version' is set by Seq66; it is used to detect older configuration\n"
        "# files, which are upgraded to the new version when saved.\n"
        "#\n"
        "# 'sets-mode' affects set muting when moving to the next set. 'normal'\n"
        "# leaves the next set muted. 'auto-arm' unmutes it. 'additive' keeps\n"
        "# the previous set armed when moving to the next set. 'all-sets' arms\n"
        "# all sets at once.\n"
        "#\n"
        "# 'port-naming' values are 'short' or 'long'.  'short' shows the port\n"
        "# number and short name; 'long' shows all number and the long name.\n"
        ;

    write_seq66_header(file, "rc", version());
    write_boolean(file, "verbose", rc().verbose());
    write_string(file, "sets-mode", rc().sets_mode_string());
    write_string(file, "port-naming", rc().port_naming_string());

    /*
     * [comments]
     */

    write_comment(file, rc_ref().comments_block().text());

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
    file << "\n"
        "# Provides a flag and a file-name for MIDI-control I/O settings. Use\n"
        "# '\"\"' to indicate no 'ctrl' file. If none, the default internal\n"
        "# keystrokes are used, with no MIDI control I/O.\n"
       ;
    write_file_status
    (
        file, "[midi-control-file]",
        rc_ref().midi_control_filename(), rc_ref().midi_control_active()
    );

    std::string mgfname = rc_ref().mute_group_filespec();
    mutegroupsfile mgf(mgfname, rc_ref());
    mgfname = rc_ref().trim_home_directory(mgfname);

    const mutegroups & mgroups = rc().mute_groups();
    if (mgroups.group_save_to_mutes())
        ok = mgf.write();

    file << "\n"
        "# Provides a flag and a file-name for mute-groups settings. Use\n"
        "# '\"\"' to indicate no 'mutes' file. If none, there are no mute\n"
        "# groups unless the MIDI file contains some.\n"
       ;
    write_file_status
    (
        file, "[mute-group-file]",
        rc_ref().mute_group_filename(), rc_ref().mute_group_active()
    );

    std::string usrname = rc_ref().user_filespec();
    usrname = rc_ref().trim_home_directory(usrname);
    file << "\n"
        "# Provides a flag and a file-name for 'user' settings. Use '\"\"' \n"
        "# to indicate no 'usr' file. If none, there are no special user\n"
        "# settings.  Using no 'usr' file should be considered experimental.\n"
       ;
    write_file_status
    (
        file, "[usr-file]",
        rc_ref().user_filename(), rc_ref().user_file_active()
    );
    file << "\n"
        "# Provides a play-list file and flag to activate it. If no list, use\n"
        "# '\"\"' and set active = false. Use the extension '.playlist'. Even\n"
        "# if not active, the play-list file is read. 'base-directory' sets the\n"
        "# sets the directory holding all MIDI files in all play-lists, useful\n"
        "# when copying play-lists/tunes from one place to another; it\n"
        "# preserves sub-directories (e.g. in creating an NSM session).\n"
        ;

    std::string plname = rc_ref().playlist_filename();
    std::string mbasedir = rc_ref().midi_base_directory();
    plname = rc_ref().trim_home_directory(plname);
    write_file_status(file, "[playlist]", plname, rc_ref().playlist_active());
    write_string(file, "base-directory", mbasedir, true);

    file << "\n"
       "# Provides a flag and file-name for note-mapping. '\"\"' indicates\n"
       "# 'drums' file. Use the extension '.drums'. This file is used when the\n"
       "# user invokes the note-conversion operation in the pattern editor of a\n"
       "# transposable pattern. Make the pattern temporarily transposable to\n"
       "# allow this operation.\n"
       ;

    std::string drumfile = rc_ref().notemap_filename();
    std::string drumname = rc_ref().trim_home_directory(drumfile);
    bool drumactive = rc_ref().notemap_active();
    write_file_status(file, "[note-mapper]", drumname, drumactive);

    /*
     * New section for palette file.
     */

    file << "\n"
       "# Provides a flag and a file-name to allow modifying the palette\n"
       "# using the file given below.  Use '\"\"' to indicate no 'palette'\n"
       "# file. If none or not active, the internal palette is used.\n"
       ;
    write_file_status
    (
        file, "[palette-file]",
        rc_ref().palette_filename(), rc_ref().palette_active()
    );

    /*
     * New section for MIDI meta events.
     */

    file << "\n"
       "# Defines features of MIDI meta-event handling. Tempo events occur in\n"
       "# the first track (pattern 0), but one can move tempo elsewhere. It\n"
       "# It changes where tempo events are recorded.  The default is 0, the\n"
       "# maximum is 1023. A pattern must exist at this number.\n"
       "\n[midi-meta-events]\n\n"
           ;
    write_integer(file, "tempo-track", rc_ref().tempo_track_number());

    /*
     * Manual ports
     */

    file << "\n"
       "# Set to true to have Seq66 create virtual ALSA/JACK I/O ports and not\n"
       "# auto-connect to other clients.  It allows up to 48 output ports and\n"
       "# 48 input ports (defaults to 8 and 4). Set to false to auto-connect\n"
       "# Seq66 to the system's existing ALSA/JACK MIDI ports.\n"
        "\n[manual-ports]\n\n"
        ;
    write_boolean(file, "virtual-ports", rc_ref().manual_ports());
    write_integer(file, "output-port-count", rc_ref().manual_port_count());
    write_integer(file, "input-port-count", rc_ref().manual_in_port_count());

    int inbuses = bussbyte(rc_ref().inputs().count());
    file << "\n"
       "# These system ports are available for input. From JACK's view, these\n"
       "# are 'playback' devices. The first number is a buss number, and the\n"
       "# second number is the input status, disabled (0) or\n"
       "# enabled (1). The item in quotes is the input-buss name.\n"
       "\n[midi-input]\n\n"
        << int(inbuses) << "      # number of input MIDI busses\n\n"
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
        file << "\n"
           "# This table is similar to the [midi-clock-map] section.\n"
           "# Port-mapping is disabled in manual/virtual port mode.\n"
           "\n[midi-input-map]\n\n"
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
        "# These system ports are available for output, for playback/control.\n"
        "# From JACK's view, these are 'capture' devices. The first line shows\n"
        "# the count of MIDI 'capture' ports. Each line shows the buss number\n"
        "# and clock status of that buss:\n"
        "#\n"
        "#  -1 = The output port is disabled.\n"
        "#   0 = MIDI Clock is off. The output port is enabled.\n"
        "#   1 = MIDI Clock on; Song Position and MIDI Continue are sent.\n"
        "#   2 = MIDI Clock Modulo.\n"
        "#\n"
        "# With Clock Modulo, MIDI clocking doesn't begin until song position\n"
        "# reaches the start modulo value [midi-clock-mod-ticks]. One can\n"
        "# disable a port manually for devices that are present, but not\n"
        "# available (because another application, e.g. Windows MIDI Mapper,\n"
        "# has exclusive access to the device.\n"
        "\n[midi-clock]\n\n"
        << int(outbuses) << "      # number of MIDI clocks (output busses)\n\n"
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
        file << "\n"
           "# This table allows the pattern to set buss numbers as usual,\n"
           "# but the use the table to look up the true buss number\n"
           "# by the short form of the port name. Thus, if the ports change\n"
           "# their order in the MIDI system, the pattern can still output to\n"
           "# the proper port. The short names are the same with ALSA or with\n"
           "# JACK with the a2jmidi bridge running. Note that port-mapping is\n"
           "# disabled in manual/virtual port mode.\n"
           "\n[midi-clock-map]\n\n"
        << activestring << "   # " << mapstatus << "\n\n"
        << output_port_map_list()
        ;
    }

    /*
     * MIDI clock modulo value, and filter by channel, new option as of
     * 2016-08-20.
     */

    file << "\n"
        "# 'ticks' provides the Song Position (in 16th notes) at which\n"
        "# clocking begins if the buss is set to MIDI Clock Mod setting.\n"
        "# 'record-by-channel' allows the master MIDI bus to record/filter\n"
        "# incoming MIDI data by channel, adding each new MIDI event to the\n"
        "# pattern that is set to that channel.  This option adopted from the\n"
        "# Seq32 project at GitHub.\n"
        "\n[midi-clock-mod-ticks]\n\n"
       ;
    write_integer(file, "ticks", midibus::get_clock_mod());
    write_boolean(file, "record-by-channel", rc_ref().filter_by_channel());

    /*
     * Reveal ports
     */

    file << "\n"
       "# Set to true to have Seq66 ignore port names defined in the 'usr'\n"
       "# file.  Use this option to to see the system ports as detected\n"
       "# by ALSA/JACK.\n"
       "\n[reveal-ports]\n\n"
       ;
    write_boolean(file, "show-system-ports", rc_ref().reveal_ports());

    /*
     * Interaction-method
     */

    file << "\n"
       "# Sets mouse usage for drawing/editing patterns. 'Fruity' mode is NOT\n"
       "# supported in Seq66. Also obsolete is the Mod4 feature. Other settings\n"
       "# are available: 'snap-split' enables splitting song-editor triggers\n"
       "# at a snap position instead of in its middle. Split is done by a\n"
       "# middle-click or Ctrl-left click. 'double-click-edit' allows double-\n"
       "# click on a slot to open it in a pattern editor. Set it to false if\n"
       "# you don't like how it works.\n"
       "\n[interaction-method]\n\n"
        ;
    write_boolean(file, "snap-split", rc_ref().allow_snap_split());
    write_boolean(file, "double-click-edit", rc_ref().allow_click_edit());

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

    /*
     * JACK Settings
     */

    std::string jacktransporttype = "none";
    if (rc_ref().with_jack_master())
        jacktransporttype = "master";
    else if (rc_ref().with_jack_transport())
        jacktransporttype = "slave";
    else if (rc_ref().with_jack_master_cond())
        jacktransporttype = "conditional";

    file << "\n"
        "# transport-type enables synchronizing with JACK Transport. Values:\n"
        "# none:        No JACK Transport in use.\n"
        "# slave:       Use JACK Transport as Slave.\n"
        "# master:      Attempt to serve as JACK Transport Master.\n"
        "# conditional: Serve as JACK master if no JACK master exists.\n"
        "#\n"
        "# song-start-mode playback is either Live, Song, or Auto:\n"
        "# live: Muting & unmuting of loops in the main window.\n"
        "# song: Playback uses Song (performance) editor data.\n"
        "# auto: If the loaded tune has song triggers, use Song mode.\n"
        "#\n"
        "# jack-midi sets/unsets JACK MIDI, separate from JACK transport.\n"
        "# jack-auto-connect sets connecting to JACK ports found. Default =\n"
        "# true; use false to have a session manager make the connections.\n"
        "\n[jack-transport]\n\n"
        << "transport-type = " << jacktransporttype << "\n"
        << "song-start-mode = " << rc_ref().song_mode_string() << "\n"
        ;
    write_boolean(file, "jack-midi", rc_ref().with_jack_midi());
    write_boolean(file, "jack-auto-connect", rc_ref().jack_auto_connect());
    file << "\n"
        "# auto-save-rc sets automatic saving of the running configuration\n"
        "# 'rc' and other files.  True is Seq24 behavior. If set, many\n"
        "# command-line settings are saved to configuration files. There is\n"
        "# no user-interface control for this setting.\n"
        "#\n"
        "# The old-triggers value indicates to save triggers in a format\n"
        "# compatible with Seq24.  Otherwise, triggers are saved with an\n"
        "# additional 'transpose' setting. Similarly, the old-mutes value,\n"
        "# if true, saves mute-groups as long values (!) instead of bytes.\n"
        "\n[auto-option-save]\n\n"
        ;
    write_boolean(file, "auto-save-rc", rc_ref().auto_rc_save());
    write_boolean(file, "save-old-triggers", rc_ref().save_old_triggers());
    write_boolean(file, "save-old-mutes", rc_ref().save_old_mutes());

    std::string lud = rc_ref().last_used_dir();
    file << "\n"
        "# Specifies the last-used and currently-active directory.\n"
        "\n[last-used-dir]\n\n"
        ;
    write_string(file, noname, rc_ref().last_used_dir(), true);

    /*
     *  Feature from Kepler34.
     */

    int count = rc_ref().recent_file_count();
    file << "\n"
        "# A list of the most recently-loaded MIDI files. 'full-paths' = true\n"
        "# indicates to show the full file-path in the menu.  The most recent\n"
        "# file (top of list) can be loaded via 'load-most-recent' at startup.\n"
        "\n[recent-files]\n\n"
        ;
    write_boolean(file, "full-paths", rc_ref().full_recent_paths());
    write_boolean(file, "load-most-recent", rc_ref().load_most_recent());
    write_integer(file, "count", count);
    file << "\n";
    if (count > 0)
    {
        for (int i = 0; i < count; ++i)
        {
            std::string rfilespec = rc_ref().recent_file(i, false);
            write_string(file, noname, rfilespec, true);
        }
    }

    /*
     * EOF
     */

    file << "\n# End of " << name()
        << "\n#\n# vim: sw=4 ts=4 wm=4 et ft=dosini\n"
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


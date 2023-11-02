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
 * \updates       2023-11-02
 * \license       GNU GPLv2 or above
 *
 *  The <code> ~/.config/seq66.rc </code> configuration file is fairly simple
 *  in layout.  See the user's manual included with Seq66.
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

#include <iomanip>                      /* std::setw() I/O manipulator      */

#include "cfg/midicontrolfile.hpp"      /* seq66::midicontrolfile class     */
#include "cfg/rcfile.hpp"               /* seq66::rcfile class              */
#include "cfg/settings.hpp"             /* seq66::rc() accessor             */
#include "midi/midibus.hpp"             /* seq66::midibus class             */
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
    "Original Seq66 method",
    "Similar to a certain Fruity sequencer",
    ""
};

#endif // defined SEQ66_USE_FRUITY_CODE

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

static const int s_rc_file_version = 3;

/**
 *  Principal constructor.
 *
 * Versions:
 *
 *      0:  The initial version, close to the Seq64 format.
 *      1:  2021-05-16. More modern JACK configuration settings.
 *      2:  2021-06-04. Transition to get-variable for booleans and integers,
 *                      finished on 2021-06-07.
 *      3:  2023-11-02. Moved style-sheets from 'usr' to 'rc'.
 *
 * \param rcs
 *      The source/destination for the configuration information.
 *
 * \param name
 *      Provides the name of the options file; this is usually a full path
 *      file-specification.
 */

rcfile::rcfile (const std::string & name, rcsettings & rcs) :
    configfile  (name, rcs, ".rc")
{
    version(s_rc_file_version);
}

/**
 *  Parse the ~/.config/seq66/qseq66.rc file.  After this function is called,
 *  the performer::get_settings() function can be used to populate the
 *  performer with the settings it needs.
 *
 *  [midi-control-file]
 *
 *      The [midi-control] section is no longer supported.  The MIDI control
 *      information is in a separate file.  This allows the user to switch
 *      between different setups without having to mess with editing the "rc"
 *      file so much.
 *
 *  [midi-clock]
 *
 *      The MIDI-clock section defines the clocking value for up to 16 output
 *      busses.  The first number indicates how many busses are specified.
 *      Generally, these busses are shown to the user with names such as "[1]
 *      seq66 1".
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

    /*
     * We no longer read the verbosity.  It should be only a command line
     * option, otherwise it gets annoying. However, we now have a
     * quiet option (it's not yet a command-line option).
     *
     * bool verby = get_boolean(file, "[Seq66]", "verbose");
     * rc_ref().verbose(verby);
     */

    bool verby = get_boolean(file, "[Seq66]", "quiet");
    rc_ref().quiet(verby);
    std::string s = parse_version(file);
    if (s.empty() || file_version_old(file))
        rc_ref().auto_rc_save(true);

    s = get_variable(file, "[Seq66]", "sets-mode");
    rc_ref().sets_mode(s);
    s = get_variable(file, "[Seq66]", "port-naming");
    rc_ref().port_naming(s);

    bool initem = get_boolean(file, "[Seq66]", "init-disabled-ports");
    rc_ref().init_disabled_ports(initem);

    /*
     * [comments] Header comments (hash-tag lead) are skipped during parsing.
     * However, we now try to read an optional comment block.
     */

    s = parse_comments(file);
    if (! s.empty())
        rc_ref().comments_block().set(s);

    bool ok = true;                                     /* start hopefully! */
    std::string tag = "[mute-group-file]";
    std::string fullpath;
    std::string pfname;
    bool active = get_file_status(file, tag, pfname);
    rc_ref().mute_group_file_active(active);
    rc_ref().mute_group_filename(pfname);               /* base name        */
    fullpath = rc_ref().mute_group_filespec();
    file_message("Reading mutes", fullpath);

    /*
     *  See get_usr_file() below.
     */

    tag = "[usr-file]";
    active = get_file_status(file, tag, pfname);
    rc_ref().user_file_active(active);
    rc_ref().user_filename(pfname);                     /* base name        */

    tag = "[palette-file]";
    active = get_file_status(file, tag, pfname);
    rc_ref().palette_active(active);
    rc_ref().palette_filename(pfname);                  /* base name        */

    tag = "[style-sheet-file]";
    active = get_file_status(file, tag, pfname);
    rc_ref().style_sheet_active(active);
    rc_ref().style_sheet_filename(pfname);              /* base name        */

    /*
     * JACK transport settings are currently accessed only via the rcsetting's
     * rc() accessor function.  Also note that these settings must occur in the
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
        rc_ref().song_start_mode_by_string(s);

        bool flag = get_boolean(file, tag, "jack-midi");
        rc_ref().with_jack_midi(flag);
        flag = get_boolean(file, tag, "jack-auto-connect", 0, true);
        rc_ref().jack_auto_connect(flag);
        flag = get_boolean(file, tag, "jack-use-offset", 0, true);
        rc_ref().jack_use_offset(flag);

        int buffersize = rc().jack_buffer_size();
        buffersize = get_integer(file, tag, "jack-buffer-size", 0);
        rc().jack_buffer_size(buffersize);
    }

    tag = "[manual-ports]";

    bool flag = get_boolean(file, tag, "virtual-ports");
    rc_ref().manual_ports(flag);
    flag = get_boolean(file, tag, "auto-enable");
    rc_ref().manual_auto_enable(flag);

    int count = get_integer(file, tag, "output-port-count");
    rc_ref().manual_port_count(count);
    count = get_integer(file, tag, "input-port-count");
    rc_ref().manual_in_port_count(count);

    /*
     *  When Seq66 exits, it saves all of the inputs it has.  If an input is
     *  removed from the system (e.g. unplugging a MIDI controller), then
     *  there will be too many entries in this section.  The user might remove
     *  one, and forget to update the buss count.  So we ignore the buss
     *  count. But we also have to read the channel-filter boolean.  If an
     *  error occurs, we abort...  the user must fix the 'rc' file.
     */

    tag = "[midi-input]";
    ok = line_after(file, tag);
    if (ok)
    {
        int inbuses = 0;
        int count = std::sscanf(scanline(), "%d", &inbuses);
        ok = count > 0 && is_good_busscount(inbuses);
    }
    if (ok)
    {
        rc_ref().inputs().clear();
        while (next_data_line(file))
        {
            ok = rc_ref().inputs().add_list_line(line());
            if (! ok)
                return make_error_message(tag, "in-bus data line error");
        }
    }
    else
    {
        return make_error_message(tag, "section missing");
    }

    /*
     *  Check for an optional input port map section.
     */

    bool portmaps_present = false;
    bool inportmap_active = false;
    bool outportmap_active = false;
    tag = "[midi-input-map]";
    if (line_after(file, tag))
    {
        inputslist & inputref = input_port_map();
        int activeflag;
        int count = 0;
        inputref.clear();
        (void) std::sscanf(scanline(), "%d", &activeflag);
        portmaps_present = true;
        inportmap_active = activeflag != 0;
        inputref.active(inportmap_active);
        while (next_data_line(file))
        {
            if (inputref.add_map_line(line()))
            {
                ++count;
            }
            else
            {
                inputref.clear();
                inputref.active(false);
                break;
            }
        }
        infoprintf("%d midi-input-map entries added", count);
    }

    /*
     * One thing about MIDI clock values.  If a device (e.g. Korg nanoKEY2)
     * is present in a system when Seq66 is exited, it will be saved in the
     * [midi-clock] list.  When unplugged, it will be read here at startup,
     * but won't be shown.  The next exit finds it removed from this list.
     * Also, we want to pre-allocate the number of clock entries needed, and
     * then use the buss number to populate the list of clocks, in the odd
     * event that the user changed the bus-order of the entries.
     */

    tag = "[midi-clock]";
    ok = line_after(file, tag);
    if (ok)
    {
        int outbuses = 0;
        int count = std::sscanf(scanline(), "%d", &outbuses);
        ok = count > 0 && is_good_busscount(outbuses);
    }
    if (ok)
    {

        rc_ref().clocks().clear();
        while (next_data_line(file))
        {
            ok = rc_ref().clocks().add_list_line(line());
            if (! ok)
                return make_error_message(tag, "out-bus data line error");
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

        rc_ref().clocks().add(0, false, e_clock::off, "Bad clock count");
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
        outportmap_active = activeflag != 0;
        clocsref.active(outportmap_active);
        while (next_data_line(file))
        {
            if (clocsref.add_map_line(line()))
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
    else
        portmaps_present = false;       /* both sections must be present    */

    rc().portmaps_present(portmaps_present);
    rc().portmaps_active(inportmap_active && outportmap_active);

    /*
     * Moved from original location above so that we have the port-mapping
     * in place for use here.
     */

    tag = "[midi-control-file]";
    if (file_version_number() < s_rc_file_version)
        (void) version_error_message("rc", file_version_number());

    active = get_file_status(file, tag, pfname);
    rc_ref().midi_control_active(active);
    rc_ref().midi_control_filename(pfname);             /* base name    */
    fullpath = rc_ref().midi_control_filespec();
    file_message("Reading ctrl", fullpath);

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

    /*
     * New for issue #97, add a configurable metronome function.
     */

    tag = "[metronome]";
    if (line_after(file, tag))
    {
        int temp = get_integer(file, tag, "output-buss");
        rc().metro_settings().buss(temp);
        temp = get_integer(file, tag, "output-channel");
        rc().metro_settings().channel(temp);
        temp = get_integer(file, tag, "beats-per-bar");
        rc().metro_settings().beats_per_bar(temp);
        temp = get_integer(file, tag, "beat-width");
        rc().metro_settings().beat_width(temp);

        temp = get_integer(file, tag, "main-patch");
        rc().metro_settings().main_patch(temp);
        temp = get_integer(file, tag, "main-note");
        rc().metro_settings().main_note(temp);
        temp = get_integer(file, tag, "main-note-velocity");
        rc().metro_settings().main_note_velocity(temp);

        double v = get_float(file, tag, "main-note-length");
        rc().metro_settings().main_note_fraction(v);

        temp = get_integer(file, tag, "sub-patch");
        rc().metro_settings().sub_patch(temp);
        temp = get_integer(file, tag, "sub-note");
        rc().metro_settings().sub_note(temp);
        temp = get_integer(file, tag, "sub-note-velocity");
        rc().metro_settings().sub_note_velocity(temp);
        v = get_float(file, tag, "sub-note-length");
        rc().metro_settings().sub_note_fraction(v);

        bool countin = get_boolean(file, tag, "count-in-active");
        rc().metro_settings().count_in_active(countin);
        temp = get_integer(file, tag, "count-in-measures");
        rc().metro_settings().count_in_measures(temp);
        countin = get_boolean(file, tag, "count-in-recording");
        rc().metro_settings().count_in_recording(countin);
        temp = get_integer(file, tag, "recording-buss");
        rc().metro_settings().recording_buss(temp);
        temp = get_integer(file, tag, "recording-measures");
        rc().metro_settings().recording_measures(temp);
        temp = get_integer(file, tag, "thru-buss");
        rc().metro_settings().thru_buss(temp);
        temp = get_integer(file, tag, "thru-channel");
        rc().metro_settings().thru_channel(temp);
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
            rc_ref().last_used_dir(ludir, false);       /* set w/o modify   */
        }
    }
    else
         (void) make_error_message(tag, "section missing");

    rc_ref().playlist_active(false);

    /*
     * [playlist]
     */

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

    /*
     * [note-mapper]
     */

    tag = "[note-mapper]";
    active = get_file_status(file, tag, pfname);
    rc_ref().notemap_active(active);
    rc_ref().notemap_filename(pfname);                  /* base name    */

    tag = "[auto-option-save]";
    flag = get_boolean(file, tag, "auto-save-rc");
    rc_ref().auto_rc_save(flag);
    flag = get_boolean(file, tag, "save-old-triggers");
    rc_ref().save_old_triggers(flag);
    flag = get_boolean(file, tag, "save-old-mutes");
    rc_ref().save_old_mutes(flag);

    /*
     * [recent-files]
     */

    tag = "[recent-files]";

    bool gotrf = line_after(file, tag);
    int recentcount;
    if (gotrf)
    {
        bool fullpaths = get_boolean(file, tag, "full-paths");
        bool loadem = ! rc_ref().playlist_active();
        if (loadem)
            loadem = get_boolean(file, tag, "load-most-recent");

        recentcount = get_integer(file, tag, "count");
        rc_ref().load_most_recent(loadem);
        rc_ref().full_recent_paths(fullpaths);
        (void) next_data_line(file);        /* skip "load-most-recent" line */
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
                        file_error("Cannot read recent file", rfilename);
                }
                if (! next_data_line(file))
                    break;
            }
        }
    }
    file.close();               /* done parsing the "rc" file               */
    return true;
}

/**
 *  Get only the 'usr' file and its active flags from the 'rc' file. This
 *  function supports testing to see if the application should be
 *  daemonized.  See cmdlineopts::parse_daemonization() and
 *  userfile::parse_daemonization().
 *
 * \return
 *      Returns true if the file is active.
 */

bool
rcfile::get_usr_file ()
{
    std::ifstream file(name(), std::ios::in | std::ios::ate);
    bool result = set_up_ifstream(file);
    if (result)
    {
        std::string tag = "[usr-file]";
        std::string usrfilename;
        result = get_file_status(file, tag, usrfilename);
        rc_ref().user_file_active(result);
        rc_ref().user_filename(usrfilename);
    }
    return result;
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
"# This file holds the main configuration for Seq66. It no longer follows the\n"
"# format of the seq24rc configuration file.\n"
"#\n"
"# 'version' is set by Seq66; it is used to detect older configuration files,\n"
"# which are upgraded to the new version when saved.\n"
"#\n"
"# 'quiet' suppresses start-up error messages. Useful when they are not\n"
"# relevant. There's no --quiet command-line option yet. It's NOT the opposite\n"
"# of 'verbose'.\n"
"#\n"
"# 'verbose' is temporary, same as --verbose; it's set to false at exit.\n"
"#\n"
"# 'sets-mode' affects set muting when moving to the next set. 'normal' leaves\n"
"# the next set muted. 'auto-arm' unmutes it. 'additive' keeps the previous set\n"
"# armed when moving to the next set. 'all-sets' arms all sets at once.\n"
"#\n"
"# 'port-naming': 'short', 'pair', or 'long'. If 'short', the device name is\n"
"# shown. If it  is generic, the client name is added for clarity. If 'pair',\n"
"# the client:port number is prepended. If 'long', the full set of name items\n"
"# is shown. If port-mapping is active (now the default), this does not apply.\n"
"#\n"
"# 'init-disabled-ports' is experimental. It tries live toggle of port state.\n"
        ;

    write_seq66_header(file, "rc", version());
    write_boolean(file, "quiet", rc_ref().quiet());
    write_boolean(file, "verbose", rc_ref().verbose());
    write_string(file, "sets-mode", rc_ref().sets_mode_string());
    write_string(file, "port-naming", rc_ref().port_naming_string());
    write_boolean(file, "init-disabled-ports", rc_ref().init_disabled_ports());

    /*
     * [comments]
     */

    write_comment(file, rc_ref().comments_block().text());

    /*
     * [midi-control-file]
     *
     * Work related to issue #89: We now write the 'ctrl' file in
     * smanager::save_session().
     *
     *  std::string mcfname = rc_ref().midi_control_filespec();
     *  ok = write_midi_control_file(mcfname, rc_ref());
     *
     * In all that follow, we no longer write the full-path. We write only
     * the base file-name (e.g. "qseq66.ctrl"), and enforce that all
     * configuration files being store in the "--home" directory.
     */

    file << "\n"
"# Provides a flag and file-name for MIDI-control I/O settings. '\"\"' means\n"
"# no 'ctrl' file. If none, default keystrokes are used, with no MIDI control.\n"
"# Note that all configuration files are stored in the \"home\" configuration\n"
"# directory; any paths in the file-names are stripped.\n"
       ;
    write_file_status
    (
        file, "[midi-control-file]",
        rc_ref().midi_control_filename(), rc_ref().midi_control_active()
    );
    file << "\n"
"# Provides a flag and file-name for mute-groups settings. '\"\"' means no\n"
"# 'mutes' file. If none, there are no mute groups, unless the MIDI file\n"
"# contains some.\n"
       ;
    write_file_status
    (
        file, "[mute-group-file]",
        rc_ref().mute_group_filename(), rc_ref().mute_group_file_active()
    );

    std::string usrname = rc_ref().user_filename();
    file << "\n"
"# Provides a flag and file-name for 'user' settings. '\"\"' means no 'usr'\n"
"# file. If none, there are no special user settings. Using no 'usr' file\n"
"# should be considered experimental.\n"
       ;
    write_file_status
    (
        file, "[usr-file]",
        usrname, rc_ref().user_file_active()
    );
    file << "\n"
"# Provides a flag and play-list file. If no list, use '\"\"' and set active\n"
"# = false. Use the extension '.playlist'. Even if not active, the play-list\n"
"# file is read. 'base-directory' sets the directory holding all MIDI files\n"
"# in all play-lists, useful when copying play-lists/tunes from one place to\n"
"# another; it preserves sub-directories (e.g. in creating an NSM session).\n"
        ;

    std::string plname = rc_ref().playlist_filename();
    std::string mbasedir = rc_ref().midi_base_directory();

    /*
     * The play-list file-name has the "home" directory prepended as a
     * side-effect of smanager::open_playlist() & performer::open_playlist().
     * We need to fix that, but here we could just strip the path.
     *
     *      plname = filename_base(plname);
     */

    write_file_status(file, "[playlist]", plname, rc_ref().playlist_active());
    write_string(file, "base-directory", mbasedir, true);

    file << "\n"
"# Provides a flag and file-name for note-mapping. '\"\"' means no 'drums' file.\n"
"# This file is used when the user invokes the note-conversion operation in\n"
"# the pattern editor of a transposable pattern. Make the pattern temporarily\n"
"# transposable to allow this operation.\n"
       ;

    std::string drumfile = rc_ref().notemap_filename();
    bool drumactive = rc_ref().notemap_active();
    write_file_status(file, "[note-mapper]", drumfile, drumactive);

    /*
     * New section for palette file.
     */

    file << "\n"
"# Provides a flag and a file-name to allow modifying the palette using the file\n"
"# specified. Use '\"\"' to indicate no 'palette' file. If none or not active,\n"
"# the internal palette is used.\n"
       ;
    write_file_status
    (
        file, "[palette-file]",
        rc_ref().palette_filename(), rc_ref().palette_active()
    );

    /*
     * New section for style-sheet file, moved from the 'usr' file.
     *
     * Might want to enforce having the style-sheet in the "home" directory.
     */

    file << "\n"
"# If specified, a style-sheet (e.g. 'qseq66.qss') is applied at startup.\n"
"# This file must be located in Seq66's \"home\" directory now.\n"
"# Note that style-sheet specification has been moved from the 'usr' file.\n"
        ;
    write_file_status
    (
        file, "[style-sheet-file]",
        rc_ref().style_sheet_filename(), rc_ref().style_sheet_active()
    );

    /*
     * New section for MIDI meta events.
     */

    file << "\n"
"# Defines features of MIDI meta-event handling. Tempo events occur in the first\n"
"# track (pattern 0), but one can move tempo elsewhere. It changes where tempo\n"
"# events are recorded. The default is 0, the maximum is 1023. A pattern must\n"
"# exist at this number.\n"
"\n[midi-meta-events]\n\n"
           ;
    write_integer(file, "tempo-track", rc_ref().tempo_track_number());

    /*
     * Manual ports
     */

    file << "\n"
"# Set to true to create virtual ALSA/JACK I/O ports and not auto-connect\n"
"# to other clients. It allows up to 48 output or input ports (defaults to 8\n"
"# and 4). Keep it false to auto-connect Seq66 to real ALSA/JACK MIDI ports.\n"
"# Set 'auto-enable' to enable all virtual ports automatically.\n"
"\n[manual-ports]\n\n"
        ;
    write_boolean(file, "virtual-ports", rc_ref().manual_ports());
    write_boolean(file, "auto-enable", rc_ref().manual_auto_enable());
    write_integer(file, "output-port-count", rc_ref().manual_port_count());
    write_integer(file, "input-port-count", rc_ref().manual_in_port_count());

    int inbuses = bussbyte(rc_ref().inputs().count());
    file << "\n"
"# These MIDI ports are for input and control. JACK's view: these are\n"
"# 'playback' devices. The first number is the bus, the second number is the\n"
"# input status, disabled (0) or enabled (1). The item in quotes is the full\n"
"# input bus name. The type of port depends on the 'virtual-ports' setting.\n"
"\n[midi-input]\n\n"
        << std::setw(2) << int(inbuses)
        << "      # number of MIDI input (or control) buses\n\n"
        ;

    std::string listlines = rc_ref().inputs().io_list_lines();
    file << listlines;

    const inputslist & inpsref = input_port_map();
    if (inpsref.not_empty())
    {
        bool active = inpsref.active();
        std::string maplines = input_port_map_list();
        std::string activestring = active ? " 1" : " 0";
        std::string mapstatus = "map is ";
        if (! active)
            mapstatus += "not ";

        mapstatus += "active";
        file << "\n"
"# This table is similar to the [midi-clock-map] section, but the values are\n"
"# different. -2 = unavailable; 0 = not inputing; 1 = enabled for inputing.\n"
"\n"
"[midi-input-map]\n"
"\n"
        << activestring << "      # " << mapstatus << "\n\n" << maplines
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
"# These MIDI ports are for output, playback, and display. JACK's view: these\n"
"# are 'capture' devices. The first line shows the count of output ports.\n"
"# Each line shows the bus number and clock status of that bus:\n"
"#\n"
"#  -2 = Output port is not present on the system (unavailable).\n"
"#  -1 = Output port is disabled.\n"
"#   0 = MIDI Clock is off. Output port is enabled.\n"
"#   1 = MIDI Clock on; Song Position and MIDI Continue are sent.\n"
"#   2 = MIDI Clock Modulo.\n"
"#\n"
"# With Clock Modulo, clocking doesn't begin until song position reaches the\n"
"# start-modulo value [midi-clock-mod-ticks]. Ports that are unavailable\n"
"# (because another port, e.g. Windows MIDI Mapper, has exclusive access to\n"
"# the device) are displayed ghosted. The type of port depends on the\n"
"# 'virtual-ports' setting.\n"
"\n[midi-clock]\n\n"
        << std::setw(2) << int(outbuses)
        << "      # number of MIDI clocks (output/display buses)\n\n"
        ;

    listlines = rc_ref().clocks().io_list_lines();
    file << listlines;

    const clockslist & outsref = output_port_map();
    if (outsref.not_empty())
    {
        bool active = outsref.active();
        std::string maplines = output_port_map_list();
        std::string activestring = active ? " 1" : " 0";
        std::string mapstatus = "map is ";
        if (! active)
            mapstatus += "not ";

        mapstatus += "active";
        file << "\n"
"# Patterns use bus numbers, not names. This table provides virtual bus numbers\n"
"# that match real devices and can be stored in each pattern. The bus number\n"
"# is looked up in this table, the port nick-name is retrieved, and the true\n"
"# bus number is obtained and used. Thus, if the ports change order in the MIDI\n"
"# system, the pattern will use the proper port. The short nick-names work in\n"
"# ALSA or JACK (a2jmidid bridge).\n"
"\n"
"[midi-clock-map]\n"
"\n"
        << activestring << "      # " << mapstatus << "\n\n" << maplines
        ;
    }

    /*
     * MIDI clock modulo value, and filter by channel, new option as of
     * 2016-08-20.
     */

    file << "\n"
"# 'ticks' provides the Song Position (16th notes) at which clocking begins if\n"
"# the bus is set to MIDI Clock Mod setting. 'record-by-channel' allows the\n"
"# master MIDI bus to record/filter incoming MIDI data by channel, adding each\n"
"# new MIDI event to the pattern that is set to that channel. Option adopted\n"
"# from the Seq32 project at GitHub.\n"
"\n[midi-clock-mod-ticks]\n\n"
       ;
    write_integer(file, "ticks", midibus::get_clock_mod());
    write_boolean(file, "record-by-channel", rc_ref().filter_by_channel());

    /*
     * Reveal ports
     */

    file << "\n"
"# Set to true to have Seq66 ignore port names defined in the 'usr' file. Use\n"
"# this option to to see the system ports as detected by ALSA/JACK.\n"
"\n[reveal-ports]\n\n"
       ;
    write_boolean(file, "show-system-ports", rc_ref().reveal_ports());

    /*
     * Metronome
     */

    file << "\n"
"# This section sets up a metronome that can be activated from the main live\n"
"# grid. It consists of a 'main' note on the first beat, then 'sub' notes on\n"
"# the rest of the beats.  The patch/program, note value, velocity, and\n"
"# fraction length relative to the beat width (can be specified. The length\n"
"# ranges from about 0.125 (one-eight) to 1.0 (the same length as the beat\n"
"# width) to 2.0).\n"
"\n[metronome]\n\n"
    ;
    write_integer(file, "output-buss", int(rc().metro_settings().buss()));
    write_integer(file, "output-channel", int(rc().metro_settings().channel()));
    write_integer
    (
        file, "beats-per-bar", int(rc().metro_settings().beats_per_bar())
    );
    write_integer
    (
        file, "beat-width", int(rc().metro_settings().beat_width())
    );
    write_integer(file, "main-patch", int(rc().metro_settings().main_patch()));
    write_integer(file, "main-note", int(rc().metro_settings().main_note()));
    write_integer
    (
        file, "main-note-velocity",
        int(rc().metro_settings().main_note_velocity())
    );
    write_float
    (
        file, "main-note-length", rc().metro_settings().main_note_length()
    );
    write_integer(file, "sub-patch", int(rc().metro_settings().sub_patch()));
    write_integer(file, "sub-note", int(rc().metro_settings().sub_note()));
    write_integer
    (
        file, "sub-note-velocity",
        int(rc().metro_settings().sub_note_velocity())
    );
    write_float
    (
        file, "sub-note-length", rc().metro_settings().sub_note_length()
    );
    write_boolean
    (
        file, "count-in-active", rc().metro_settings().count_in_active()
    );
    write_integer
    (
        file, "count-in-measures",
        int(rc().metro_settings().count_in_measures())
    );
    write_boolean
    (
        file, "count-in-recording",
        rc().metro_settings().count_in_recording()
    );
    write_integer
    (
        file, "recording-buss", int(rc().metro_settings().recording_buss())
    );
    write_integer
    (
        file, "recording-measures",
        int(rc().metro_settings().recording_measures())
    );
    write_integer(file, "thru-buss", int(rc().metro_settings().thru_buss()));
    write_integer
    (
        file, "thru-channel", int(rc().metro_settings().thru_channel())
    );

    /*
     * Interaction-method
     */

    file << "\n"
"# Sets mouse usage for drawing/editing patterns. 'Fruity' mode is NOT in\n"
"# Seq66. Other settings are available: 'snap-split' enables splitting\n"
"# song-editor triggers at a snap position instead of in its middle. Split is\n"
"# done by a middle-click or ctrl-left click. 'double-click-edit' allows double-\n"
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
"# jack-auto-connect sets connecting to JACK ports found. Default = true; use\n"
"# false to have a session manager make the connections.\n"
"# jack-use-offset attempts to calculate timestamp offsets to improve accuracy\n"
"# at high-buffer sizes. Still a work in progress.\n"
"# jack-buffer-size allows for changing the frame-count, a power of 2.\n"
"\n[jack-transport]\n\n"
        << "transport-type = " << jacktransporttype << "\n"
        << "song-start-mode = " << rc_ref().song_mode_string() << "\n"
        ;
    write_boolean(file, "jack-midi", rc_ref().with_jack_midi());
    write_boolean(file, "jack-auto-connect", rc_ref().jack_auto_connect());
    write_boolean(file, "jack-use-offset", rc_ref().jack_use_offset());
    write_integer(file, "jack-buffer-size", rc_ref().jack_buffer_size());
    file << "\n"
"# 'auto-save-rc' sets automatic saving of the  'rc' and other files. If set,\n"
"# many command-line settings are saved to configuration files.\n"
"#\n"
"# 'old-triggers' saves triggers in a format compatible with Seq24. Otherwise,\n"
"# triggers are saved with an additional 'transpose' setting. The old-mutes\n"
"# value, if true, saves mute-groups as long values (!) instead of bytes.\n"
"\n[auto-option-save]\n\n"
        ;
    write_boolean(file, "auto-save-rc", rc_ref().auto_rc_save());
    write_boolean(file, "save-old-triggers", rc_ref().save_old_triggers());
    write_boolean(file, "save-old-mutes", rc_ref().save_old_mutes());

    std::string lud = rc_ref().last_used_dir();
    file << "\n"
        "# Specifies the last-used/currently-active directory.\n"
        "\n[last-used-dir]\n\n"
        ;
    write_string(file, noname, rc_ref().last_used_dir(), true);

    /*
     *  Feature from Kepler34.
     */

    int count = rc_ref().recent_file_count();
    file << "\n"
"# The most recently-loaded MIDI files. 'full-paths' = true means to show the\n"
"# full file-path in the menu. The most recent file (top of list) can be loaded\n"
"# via 'load-most-recent' at startup.\n"
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
    write_seq66_footer(file);
    file.close();
    return true;
}

}           // namespace seq66

/*
 * rcfile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


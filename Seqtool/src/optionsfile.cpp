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
 * \file          optionsfile.cpp
 *
 *  This module declares/defines the base class for managing the <code>
 *  ~/.seq66rc </code> legacy configuration file or the new <code>
 *  ~/.config/seq66.rc </code> ("rc") configuration file.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2020-08-03
 * \license       GNU GPLv2 or above
 *
 *  The <code> ~/.seq66rc </code> or <code> ~/.config/seq66.rc
 *  </code> configuration file is fairly simple in layout.  The documentation
 *  for this module is supplemented by the following GitHub projects:
 *
 *      -   https://github.com/ahlstromcj/seq66-doc.git (legacy support)
 *      -   https://github.com/ahlstromcj/seq66-doc.git
 *
 *  Process for MIDI control conversion:
 *
\verbatim
 *                            rcsettings          midicontrolfile
 *       ------              -----------          -----------
 *      | Old  |----------->|   MIDI    |------->|  stanza   |
 *      | [MC] |            | container |        | container |
 *       ------              -----------          -----------
 *                         keys ^ Null_ff              |
 *                              |                      |
 * add_midi_control_stanza()    |                      |
 *                              |                      v
 *       --------               |                 ----------------------
 *      |  Old   |--------------                 | [loop-control]       |
 *      | [keys] |                               | [mute-group-control] |
 *       --------   midicontrolin::merge_key()   | [automation-control] |
 *                                                ----------------------
\endverbatim
 */

#include <iostream>
#include <string>

#include "cfg/midicontrolfile.hpp"      /* seq66::midicontrolfile           */
#include "cfg/settings.hpp"             /* seq66::m_rc config container     */
#include "ctrl/keymap.hpp"              /* seq66::qdk_key_name() function   */
#include "gdk_basic_keys.hpp"           /* SEQ66_equal, SEQ66_minus         */
#include "optionsfile.hpp"              /* this conversion module's header  */
#include "util/filefunctions.hpp"       /* seq66::filename_split() function */
#include "util/strfunctions.hpp"        /* seq66::strip_quotes() function   */

/**
 *  Provides names for the mouse-handling used by the application.
 */

static const std::string c_interaction_method_names[2] =
{
    "seq66",
    "fruity"
};

/**
 *  Provides descriptions for the mouse-handling used by the application.
 */

static const std::string c_interaction_method_descs[2] =
{
    "original seq66 method",
    "similar to a certain fruity sequencer we like"
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
 *      Provides the rcsettings configuration class to use.
 *
 * \param name
 *      Provides the name of the options file; this is usually a full path
 *      file-specification.
 *
 * \param outname
 *      Provides the base name of the output files.
 */

optionsfile::optionsfile
(
    rcsettings & rcs,
    const std::string & name
) :
    configfile  (name, rcs)
{
    // Empty body
}

/**
 *  A rote destructor.
 */

optionsfile::~optionsfile ()
{
    // Empty body
}

/**
 *  Helper function for error-handling.  It assembles a message and then
 *  passes it to error_message().
 *
 * \param sectionname
 *      Provides the name of the section for reporting the error.
 *
 * \param additional
 *      Additional context information to help in finding the error.
 *
 * \return
 *      Always returns false.
 */

bool
optionsfile::make_error_message
(
    const std::string & sectionname,
    const std::string & additional
)
{
    std::string msg = "BAD OR MISSING DATA in [";
    msg += sectionname;
    msg += "]: ";
    if (! additional.empty())
        msg += additional;

    errprint(msg.c_str());
    error_message(msg);
    return false;
}

/**
 *  Translate the key code to our keymap, get the name of the key, and
 *  try to update it in the container.
 *
 *  In parse_midi_control_section(), for the loop/pattern section, when adding
 *  a MIDI control stanza, we get the pattern number (0 to 31) and the 3
 *  blocks of values (for toggle, on, and off).
 *
 *  Then rcsettings::add_midicontrol_stanza() uses the loop category code,
 *  pattern number, and the 3 blocks of values to create 3 midicontrol
 *  objects, and a bogus key-name, "Null_ff", to add these 3 object to the
 *  midicontrolin.
 *
 *  Later, we get the legacy key control from its section, getting the key
 *  value and the pattern number.  We have to look up the pattern category
 *  code and the pattern number in the midicontrolin, and for all matches
 *  (there should be 3), modify the key-name of those objects.
 */

bool
optionsfile::merge_key
(
    automation::category opcat,
    unsigned key,
    unsigned slotnumber
)
{
    std::string kn = seq66::gdk_key_name(key);
    bool result = rc_ref().midi_control_in().merge_key
    (
        opcat, kn, int(slotnumber)
    );
    if (! result)
    {
#if defined USE_WHICHEVER_IS_BETTER
        std::cerr
            << "Failed to update mute-group control key code " << key
            << " named '" << kn << "'"
            << std::endl
            ;
#else
        msgprintf
        (
            msg_level::error,
            "Failed to update mute-group control key code %u names '%s'\n",
            key, kn.c_str()
        );
#endif

    }
    return result;
}

/**
 *  Parse the ~/.seq66rc or ~/.config/seq66.rc file.
 *
 * \return
 *      Returns true if the file was able to be opened for reading.
 *      Currently, there is no indication if the parsing actually succeeded.
 */

bool
optionsfile::parse ()
{
    bool result = true;
    std::ifstream file(name(), std::ios::in | std::ios::ate);
    if (! file.is_open())
    {
        errprintf
        (
            "optionsfile::parse(): error opening %s for reading\n",
            name().c_str()
        );
        result = false;
    }
    else
    {
        file.seekg(0, std::ios::beg);                   /* seek to start    */

        /*
         * [comments]
         *
         * Header commentary is skipped during parsing.  However, we now try to
         * read an optional comment block.
         */

        if (line_after(file, "[comments]"))             /* gets first line  */
        {
            rc_ref().comments_block().clear();
            do
            {
                rc_ref().comments_block().append(line());
                rc_ref().comments_block().append("\n");

            } while (next_data_line(file));
        }

        bool ok = true;                                 /* start hopefully! */
        std::string path;
        std::string basename;                           /* for ctrl, mutes  */
        (void) filename_split(name(), path, basename);  /* get base-name    */
        if (line_after(file, "[midi-control-file]"))
        {
            std::string fullpath;
            std::string filename = strip_comments(line());  /* base name    */
            filename = strip_quotes(filename);
            ok = ! filename.empty();
            if (ok)
            {
                /*
                 *  The current version of rcsettings specifies the directory
                 *  "seq66", not sequencer64".  The user can specify another
                 *  directory.  However, mostly, that won't happen.  We need to
                 *  revert to the legacy default directory in the file-path.
                 */

                rc_ref().midi_control_filename(filename);       /* base-name    */
                fullpath = rc_ref().midi_control_filespec();    /* full path    */
                fullpath = string_replace(fullpath, "66", "64");
                pathprint("Legacy MIDI control file", fullpath);
                ok = parse_midi_control_section(fullpath);
                if (! ok)
                {
                    std::string info = "cannot parse file '";
                    info += fullpath;
                    info += "'";
                    return make_error_message("midi-control-file", info);
                }
            }
            else
                result = false;

            rc_ref().use_midi_control_file(ok);                 /* did it work? */
            rc_ref().midi_control_filename(ok ? filename : ""); /* base-name    */
        }
        else
        {
            rc_ref().use_midi_control_file(false);
            rc_ref().midi_control_filename(basename);
            ok = parse_midi_control_section(name());
            if (ok)
            {
                /*
                 * Settings made in converter class.
                 */
            }
        }
        if (ok)
        {
            ok = parse_mute_group_section();
            if (ok)
            {
                /*
                 * Settings made in converter class.
                 */
            }
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
            rc_ref().clocks().resize(size_t(buses));
            for (int i = 0; i < buses; ++i)
            {
                int bus, bus_on;
                sscanf(scanline(), "%d %d", &bus, &bus_on);
                rc_ref().clocks().set(bus, static_cast<e_clock>(bus_on));
                ok = next_data_line(file);
                if (! ok)
                {
                    if (i < (buses - 1))
                        return make_error_message("midi-clock data line missing");
                }
            }
        }
        else
        {
            /*
             *  If this is zero, we need to fake it to have 1 buss with a 0 clock,
             *  rather than make the poor user figure out how to fix it.
             */

            rc_ref().clocks().add(e_clock::off, "Bad clocks count");
        }

        /*
         *  We used to crap out when this section had 0 entries.  But for working
         *  with the new Qt5 implmentation, it is worthwhile to continue.  Also,
         *  we note that Kepler34 has this section commented out.
         */

        line_after(file, "[keyboard-control]");

        long keys = 0;
        sscanf(scanline(), "%ld", &keys);
        ok = keys >= 0 && keys <= c_max_keys;
        if (ok && keys > 0)
            ok = next_data_line(file);

        if (ok)
        {
            if (keys == 0)
            {
                warnprint("[keyboard-control] keys = 0!");
            }
        }
        else
        {
            (void) make_error_message("keyboard-control");   // allowed to continue
        }

        /*
         * Bug involving the optionsfile and performer modules:  At the 4th or 5th
         * line of data in the "rc" file, setting this key event results in the
         * size remaining at 4, so the final size is 31.  This bug is present even
         * in seq66 r.0.9.2, and occurs only if the Keyboard options are actually
         * edited.  Also, the size of the reverse container is constant at 32.
         * Clearing the latter container as well appears to fix both bugs.
         */

        for (int i = 0; i < keys; ++i)
        {
            long key = 0, seq = 0;
            sscanf(scanline(), "%ld %ld", &key, &seq);
            ok = merge_key(automation::category::loop, key, seq);
            if (ok)
                ok = next_data_line(file);

            if (! ok && i < (keys - 1))
                return make_error_message("keyboard-control data line");
        }

        /*
         *  Keys for Group Learn.  We used to crap out when this section had 0
         *  entries.  But for working with the new Qt5 implmentation, it is
         *  worthwhile to continue.  Also, we note that Kepler34 has this section
         *  commented out.
         */

        line_after(file, "[keyboard-group]");
        long groups = 0;
        sscanf(scanline(), "%ld", &groups);
        ok = groups >= 0 && groups <= c_max_keys;
        if (ok && groups > 0)
            ok = next_data_line(file);

        if (ok)
        {
            if (groups == 0)
            {
                warnprint("[keyboard-group] groups = 0!");
            }
        }
        else
        {
            (void) make_error_message("keyboard-group");     // allowed to continue
        }

        for (int i = 0; i < groups; ++i)
        {
            long key = 0, group = 0;
            sscanf(scanline(), "%ld %ld", &key, &group);
            ok = merge_key(automation::category::mute_group, key, group);
            if (ok)
                ok = next_data_line(file);

            if (! ok && i < (groups - 1))
                return make_error_message("keyboard-group data line");
        }

        automation::category cat = automation::category::automation;
        unsigned k1, k2, k3, k4, k5;
        int sn;                                     /* slot number          */

        sscanf(scanline(), "%u %u", &k1, &k2);      /* bpm_up, bpm_dn       */
        sn = static_cast<int>(automation::slot::bpm_up);
        (void) merge_key(cat, k1, sn);
        sn = static_cast<int>(automation::slot::bpm_dn);
        (void) merge_key(cat, k2, sn);

        next_data_line(file);
        sscanf(scanline(), "%u %u %u", &k1, &k2, &k3); /* ss_up, ss_dn, play   */
        sn = static_cast<int>(automation::slot::ss_up);
        (void) merge_key(cat, k1, sn);
        sn = static_cast<int>(automation::slot::ss_dn);
        (void) merge_key(cat, k2, sn);
        sn = static_cast<int>(automation::slot::play_ss);
        (void) merge_key(cat, k3, sn);

        next_data_line(file);
        sscanf(scanline(), "%u %u %u", &k1, &k2, &k3); /* group on, off, learn */
        sn = static_cast<int>(automation::slot::mod_gmute);     // toggle group
        (void) merge_key(cat, k1, sn);

        /*
         * Ignore this key value.  The key above will be used as a toggle instead
         * for group_on and group_off, unless we get some complaints about it.
         *
         * sn = static_cast<int>(automation::slot::group_off);
         * (void) merge_key(cat, k2, sn);
         */

        sn = static_cast<int>(automation::slot::mod_glearn);
        (void) merge_key(cat, k3, sn);

        next_data_line(file);
        sscanf(scanline(), "%u %u %u %u %u", &k1, &k2, &k3, &k4, &k5);

        /*
         * In Seq24 (and hence Seq66), the "mod" MIDI controls
         * roughly overlapped with certain keystrokes, though not completely.
         *
         *    &ktx.kpt_replace &ktx.kpt_queue &ktx.kpt_snapshot_1
         *    &ktx.kpt_snapshot_2 &ktx.kpt_keep_queue
         */

        sn = static_cast<int>(automation::slot::mod_replace);
        (void) merge_key(cat, k1, sn);
        sn = static_cast<int>(automation::slot::mod_queue);
        (void) merge_key(cat, k2, sn);
        sn = static_cast<int>(automation::slot::mod_snapshot);
        (void) merge_key(cat, k3, sn);
        sn = static_cast<int>(automation::slot::mod_snapshot_2);
        (void) merge_key(cat, k4, sn);

        /*
         * The previous key is the 32nd key processed, and there are only ever 32
         * "[midi-control]" stanzas in the Seq64 "rc" file.
         */

        sn = static_cast<int>(automation::slot::keep_queue);
        (void) merge_key(cat, k5, sn);

        int show_key = 0;
        next_data_line(file);
        sscanf(scanline(), "%d", &show_key);
        rc_ref().show_ui_sequence_key(bool(show_key));

        next_data_line(file);
        sscanf(scanline(), "%u", &k1);      // &ktx.kpt_start);
        sn = static_cast<int>(automation::slot::start);
        (void) merge_key(cat, k1, sn);

        next_data_line(file);
        sscanf(scanline(), "%u", &k1);      // &ktx.kpt_stop);
        sn = static_cast<int>(automation::slot::stop);
        (void) merge_key(cat, k1, sn);

        next_data_line(file);
        sscanf(scanline(), "%u", &k1);      // &ktx.kpt_pause);
        if (k1 <= 1)                        /* no pause key value present   */
        {
            rc_ref().show_ui_sequence_number(bool(k1));
        }
        else
        {
            sn = static_cast<int>(automation::slot::playback);  // toggle/pause
            (void) merge_key(cat, k1, sn);

            next_data_line(file);
            sscanf(scanline(), "%d", &show_key);
            rc_ref().show_ui_sequence_number(bool(show_key));
        }

        next_data_line(file);
        sscanf(scanline(), "%u", &k1);              // &ktx.kpt_pattern_edit);
        sn = static_cast<int>(automation::slot::pattern_edit);
        (void) merge_key(cat, k1, sn);

        next_data_line(file);
        sscanf(scanline(), "%u", &k1);              // &ktx.kpt_event_edit);
        sn = static_cast<int>(automation::slot::event_edit);
        (void) merge_key(cat, k1, sn);

        if (next_data_line(file))
        {
            sscanf(scanline(), "%u", &k1);          // &ktx.kpt_pattern_shift);
            sn = static_cast<int>(automation::slot::slot_shift);
            (void) merge_key(cat, k1, sn);
        }

        if (line_after(file, "[extended-keys]"))
        {
            sscanf(scanline(), "%u", &k1);          // &ktx.kpt_song_mode);
            sn = static_cast<int>(automation::slot::song_mode);
            (void) merge_key(cat, k1, sn);

            next_data_line(file);
            sscanf(scanline(), "%u", &k1);          // &ktx.kpt_toggle_jack);
            sn = static_cast<int>(automation::slot::toggle_jack);
            (void) merge_key(cat, k1, sn);

            next_data_line(file);
            sscanf(scanline(), "%u", &k1);          // &ktx.kpt_menu_mode);
            sn = static_cast<int>(automation::slot::menu_mode);
            (void) merge_key(cat, k1, sn);

            next_data_line(file);
            sscanf(scanline(), "%u", &k1);          // &ktx.kpt_follow_transport);
            sn = static_cast<int>(automation::slot::follow_transport);
            (void) merge_key(cat, k1, sn);

            next_data_line(file);
            sscanf(scanline(), "%u", &k1);          // &ktx.kpt_fast_forward);
            sn = static_cast<int>(automation::slot::FF);
            (void) merge_key(cat, k1, sn);

            next_data_line(file);
            sscanf(scanline(), "%u", &k1);          // &ktx.kpt_rewind);
            sn = static_cast<int>(automation::slot::rewind);
            (void) merge_key(cat, k1, sn);

            next_data_line(file);
            sscanf(scanline(), "%u", &k1);          // &ktx.kpt_pointer_position);
            sn = static_cast<int>(automation::slot::song_pointer);
            (void) merge_key(cat, k1, sn);

            next_data_line(file);
            sscanf(scanline(), "%u", &k1);          // &ktx.kpt_tap_bpm);
            sn = static_cast<int>(automation::slot::tap_bpm);
            (void) merge_key(cat, k1, sn);

            next_data_line(file);
            sscanf(scanline(), "%u", &k1);          // &ktx.kpt_toggle_mutes);
            sn = static_cast<int>(automation::slot::toggle_mutes);
            (void) merge_key(cat, k1, sn);

            next_data_line(file);
            sscanf(scanline(), "%u", &k1);          // &ktx.kpt_song_record);
            sn = static_cast<int>(automation::slot::song_record);
            (void) merge_key(cat, k1, sn);

            next_data_line(file);
            sscanf(scanline(), "%u", &k1);          // &ktx.kpt_oneshot_queue);
            sn = static_cast<int>(automation::slot::mod_oneshot);
            (void) merge_key(cat, k1, sn);
        }
        else
        {
            warnprint("WARNING:  no [extended-keys] section");
        }

        long flag = 0;
        if (line_after(file, "[jack-transport]"))
        {
            sscanf(scanline(), "%ld", &flag);
            rc_ref().with_jack_transport(bool(flag));

            next_data_line(file);
            sscanf(scanline(), "%ld", &flag);
            rc_ref().with_jack_master(bool(flag));

            next_data_line(file);
            sscanf(scanline(), "%ld", &flag);
            rc_ref().with_jack_master_cond(bool(flag));

            next_data_line(file);
            sscanf(scanline(), "%ld", &flag);
            rc_ref().song_start_mode(bool(flag));

            if (next_data_line(file))
            {
                sscanf(scanline(), "%ld", &flag);
                rc_ref().with_jack_midi(bool(flag));
            }
        }

        /*
         *  We are taking a slightly different approach to this section.  When
         *  Seq66 exits, it saves all of the inputs it has.  If an input is
         *  removed from the system (e.g. unplugging a MIDI controller), then
         *  there will be too many entries in this section.  The user might remove
         *  one, and forget to update the buss count.  So we basically ignore the
         *  buss count.  But we also have to read the new channel-filter boolean
         *  if not in legacy format. If an error occurs, we abort... the user must
         *  fix the "rc" file.
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
            return make_error_message("midi-input");

        if (line_after(file, "[midi-clock-mod-ticks]"))
        {
            long ticks = 64;
            sscanf(scanline(), "%ld", &ticks);
            rc_ref().set_clock_mod(ticks);
        }
        if (line_after(file, "[midi-meta-events]"))
        {
            int track = 0;
            sscanf(scanline(), "%d", &track);
            rc_ref().tempo_track_number(track);
        }
        if (line_after(file, "[manual-alsa-ports]"))
        {
            sscanf(scanline(), "%ld", &flag);
            rc_ref().manual_ports(bool(flag));
        }
        if (line_after(file, "[reveal-alsa-ports]"))
        {
            /*
             * If this flag is already raised, it was raised on the command line,
             * and we don't want to change it.  An ugly special case.
             */

            sscanf(scanline(), "%ld", &flag);
            if (! rc_ref().reveal_ports())
                rc_ref().reveal_ports(bool(flag));
        }

        if (line_after(file, "[last-used-dir]"))
        {
            if (line().size() > 0)
                rc_ref().last_used_dir(line()); // FIXME: check for valid path
        }

        if (line_after(file, "[recent-files]"))
        {
            int count;
            sscanf(scanline(), "%d", &count);
            for (int i = 0; i < count; ++i)
            {
                if (next_data_line(file))
                {
                    if (line().size() > 0)
                    {
                        if (! rc_ref().append_recent_file(line()))
                            break;
                    }
                }
                else
                    break;
            }
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
                         * Prepend the home configuration directory and, if
                         * needed, the playlist extension.  Also, we want the
                         * playlist from the legacy directory.
                         */

                        fname = rc_ref().make_config_filespec(fname, ".playlist");
                        fname = string_replace(fname, "66", "64");
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

        long method = 0;
        if (line_after(file, "[interaction-method]"))
            sscanf(scanline(), "%ld", &method);

        /*
         * This now returns true if the value was correct, we should check it.
         */

        if (! rc_ref().interaction_method(method))
            return make_error_message("interaction-method", "illegal value");

        if (next_data_line(file))
        {
            sscanf(scanline(), "%ld", &method);
            rc_ref().allow_mod4_mode(method != 0);
        }
        if (next_data_line(file))
        {
            sscanf(scanline(), "%ld", &method);
            rc_ref().allow_snap_split(method != 0);
        }
        if (next_data_line(file))                   /* a new option */
        {
            sscanf(scanline(), "%ld", &method);
            rc_ref().allow_click_edit(method != 0);
        }

        /*
         * line_after(file, "[lash-session]");
         * sscanf(scanline(), "%ld", &method);
         * rc_ref().lash_support(method != 0);
         */

        method = 1;                     /* legacy seq66 option if not present */
        line_after(file, "[auto-option-save]");
        sscanf(scanline(), "%ld", &method);
        rc_ref().auto_option_save(method != 0);
        file.close();
    }
    return result;
}

/**
 *  An internal convention function to take advantage of simplifications to the
 *  mutegroups interface.  It converts an array of integers to a boolean vector.
 */

midibooleans
optionsfile::ints_to_booleans (const int * iarray, int len)
{
    midibooleans bits;
    for (int i = 0; i < len; ++i, ++iarray)
    {
        bool bit = midibool(*iarray);
        bits.push_back(bit);
    }
    return bits;
}

/**
 *  Parses the [mute-group] section.  This function is used both in the
 *  original reading of the "rc" file, and for reloading the original
 *  mute-group data from the "rc".
 *
 * \return
 *      Returns true if the file was able to be opened for reading, and the
 *      desired data successfully extracted.
 */

bool
optionsfile::parse_mute_group_section ()
{
    std::ifstream file(name(), std::ios::in | std::ios::ate);
    if (! file.is_open())
    {
        errprintf
        (
            "optionsfile::parse_mute_group_section(): "
            "error opening %s for reading\n",
            name().c_str()
        );
        return false;
    }
    file.seekg(0, std::ios::beg);                           /* seek to start */

    line_after(file, "[mute-group]");               /* Group MIDI control   */
    int gtrack = 0;
    sscanf(scanline(), "%d", &gtrack);
    bool result = next_data_line(file);
    if (result)
    {
        result = gtrack == 0 || gtrack == (c_max_sets * c_max_keys); /* 1024 */
    }
    if (! result)
        (void) make_error_message("mute-group");    /* abort the parsing!   */

    if (result && gtrack > 0)
    {
        /*
         * This loop is a bit odd.  We set groupmute, read it, increment it,
         * and then read it again.  We could just use the i variable, I think.
         * Note that this layout is STILL dependent on c_seqs_in_set = 32.
         * However, though we keep this layout, the boundaries for a
         * non-default value of seqs-in-set may be used internally.
         *
         * We could update this section to use the new parse_stanza_bits()
         * function.  At present we'll use it only in "new" code.
         */

        int gm[c_seqs_in_set];
        int groupmute = 0;
        for (int g = 0; g < c_max_groups; ++g)
        {
            sscanf
            (
                scanline(),
                "%d [%d %d %d %d %d %d %d %d]"
                  " [%d %d %d %d %d %d %d %d]"
                  " [%d %d %d %d %d %d %d %d]"
                  " [%d %d %d %d %d %d %d %d]",
                &groupmute,
                &gm[0],  &gm[1],  &gm[2],  &gm[3],
                &gm[4],  &gm[5],  &gm[6],  &gm[7],
                &gm[8],  &gm[9],  &gm[10], &gm[11],
                &gm[12], &gm[13], &gm[14], &gm[15],
                &gm[16], &gm[17], &gm[18], &gm[19],
                &gm[20], &gm[21], &gm[22], &gm[23],
                &gm[24], &gm[25], &gm[26], &gm[27],
                &gm[28], &gm[29], &gm[30], &gm[31]
            );
            if (groupmute < 0 || groupmute >= c_max_groups)
            {
                return make_error_message("group-mute number out of range");
            }
            else
            {
                midibooleans bits = ints_to_booleans(gm, c_seqs_in_set);
                rc_ref().mute_groups().load(groupmute, bits);
            }

            result = next_data_line(file);
            if (! result && g < (c_max_groups - 1))
                return make_error_message("mute-group data line");
            else
                result = true;
        }
        if (result)
        {
            bool present = ! at_section_start();    /* ok if not present    */
            if (present)
            {
                int v = 0;
                sscanf(scanline(), "%d", &v);
                result = rc_ref().mute_group_save
                (
                    v ? std::string("midi") : std::string("both")
                );
                if (! result)
                    return make_error_message("mute-group", "handling value bad");
            }
        }
    }
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
optionsfile::parse_midi_control_section (const std::string & fname)
{
    std::ifstream file(fname, std::ios::in | std::ios::ate);
    if (! file.is_open())
    {
        errprintf
        (
            "optionsfile::parse_midi_control_section(): "
            "error opening %s for reading\n",
            name().c_str()
        );
        return false;
    }
    file.seekg(0, std::ios::beg);                           /* seek to start */

    /*
     * This call causes parsing to skip all of the header material.  Please note
     * that the line_after() function always starts from the beginning of the
     * file every time.  A lot a rescanning!  But it goes fast these days.
     */

    unsigned sequences = 0;                                 /* seq & ctrl #s */
    line_after(file, "[midi-control]");
    sscanf(scanline(), "%u", &sequences);

    /*
     * The above value is called "sequences", but what was written was the
     * value of c_midi_controls.  If we make that value non-constant, then
     * we should modify that value, instead of the throw-away "sequences"
     * values.  Note that c_midi_controls is, in a roundabout way, defined
     * as 74.  See the old "dot-seq66rc" file in the contrib directory.
     */

    bool ok = false;
    int offset = 0;
    if (sequences > 0)
    {
        ok = next_data_line(file);
        if (! ok)
            return make_error_message("midi-control", "no data");

        automation::category cat;
        for (unsigned i = 0; i < sequences; ++i)    /* 0 to c_midi_controls-1 */
        {
            int sequence = 0;
            int a[6], b[6], c[6];
            sscanf
            (
                scanline(),
                "%d [ %d %d %d %d %d %d ]"
                  " [ %d %d %d %d %d %d ]"
                  " [ %d %d %d %d %d %d ]",
                &sequence,
                &a[0], &a[1], &a[2], &a[3], &a[4], &a[5],
                &b[0], &b[1], &b[2], &b[3], &b[4], &b[5],
                &c[0], &c[1], &c[2], &c[3], &c[4], &c[5]
            );
            if (sequence < 32)
            {
                cat = automation::category::loop;
            }
            else if (sequence < 64)
            {
                cat = automation::category::mute_group;
                offset = 32;
            }
            else
            {
                cat = automation::category::automation;
                offset = 64;
            }
            ok = rc_ref().add_midicontrol_stanza
            (
                "Null_ff", cat, sequence - offset, a, b, c
            );
            if (ok)
                ok = next_data_line(file);

            if (! ok && i < (sequences - 1))
                return make_error_message("midi-control", "not enough data");
            else
                ok = true;
        }
    }
    else
    {
        warnprint("[midi-controls] specifies a count of 0, so skipped");
    }

    /*
     * There are a number of additional controls in the new code.  We need to
     * create them (blanks) so that the existing keystrokes can be added.
     */

    automation::category cat = automation::category::automation;
    int lastslot = static_cast<int>(automation::slot::maximum);
    sequences -= 64;
    for (int s = sequences; s < lastslot; ++s)
    {
        ok = rc_ref().add_blank_stanza("Null_ff", cat, s);
    }
    return true;
}

}           // namespace seq66

/*
 * optionsfile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


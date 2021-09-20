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
 * \file          usrfile.cpp
 *
 *  This module declares/defines the base class for managing the user's
 *  <code> ~/.seq66usr </code> or <code> ~/.config/seq66.rc
 *  </code> configuration file.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-23
 * \updates       2021-09-20
 * \license       GNU GPLv2 or above
 *
 *  Note that the parse function has some code that is not yet enabled.
 *  Also note that, unlike the "rc" settings, these settings have no
 *  user-interface.  One must use a text editor to modify its settings.
 */

#include <iomanip>                      /* std::setw() manipulator          */

#include "cfg/settings.hpp"             /* seq66::usr() "global" settings   */
#include "cfg/usrfile.hpp"              /* seq66::usrfile                   */
#include "cfg/userinstrument.hpp"       /* seq66::userinstrument            */
#include "util/calculations.hpp"        /* seq66::current_date_time()       */
#include "util/strfunctions.hpp"        /* seq66::strip_quotes()            */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

static const int s_usr_file_version = 7;

/**
 *  Principal constructor.
 *
 * Versions:
 *
 *      0:  The initial version, close to the Seq64 format.
 *      4:  2021-05-15. Disabled using grid-style and grid-brackets settings.
 *      5:  2021-06-08. Transition to get-variable for booleans/integers.
 *      6:  2021-07-26. Added progress-note-min and progress-note-max.
 *
 * \param name
 *      Provides the full file path specification to the configuration file.
 *
 * \param rcs
 *      The source/destination for the configuration information.  In most
 *      cases, the caller will pass in seq66::rc(), the "global" rcsettings
 *      object.
 */

usrfile::usrfile (const std::string & name, rcsettings & rcs) :
    configfile (name, rcs)
{
    version(s_usr_file_version);
}

/**
 *  Provides a purely internal, ad hoc helper function to create numbered
 *  section names for the usrfile class.
 *
 * \param label
 *      The base-name of the section.
 *
 * \param value
 *      The numeric value to append to the section name.
 *
 * \return
 *      Returns a string of the form "[basename-1]".
 */

static std::string
make_section_name (const std::string & label, int value)
{
    std::string result = "[";
    result += label;
    result += "-";
    result += std::to_string(value);
    result += "]";
    return result;
}

/**
 *  Provides a debug dump of basic information to help debug a surprisingly
 *  intractable problem with all busses having the name and values of the last
 *  buss in the configuration.  Does work only if SEQ66_PLATFORM_DEBUG is
 *  defined; see the user_settings class.
 */

void
usrfile::dump_setting_summary ()
{
#if defined SEQ66_PLATFORM_DEBUG_TMI
    usr().dump_summary();
#endif
}

/**
 *  Parses a "usr" file, filling in the given performer object.  This function
 *  opens the file as a text file (line-oriented).
 *
 * \return
 *      Returns true if the parsing succeeded.
 */

bool
usrfile::parse ()
{
    std::ifstream file(name().c_str(), std::ios::in | std::ios::ate);
    if (! set_up_ifstream(file))            /* verifies [Seq66]: version    */
        return false;

    std::string temp = parse_version(file);
    if (temp.empty() || file_version_number() < s_usr_file_version)
        rc().auto_usr_save(true);

    temp = parse_comments(file);
    if (! temp.empty())
        usr().comments_block().set(temp);

    usr().clear_buses_and_instruments();
    if (! rc_ref().reveal_ports())
    {
        /*
         * [user-midi-bus-definitions]
         */

        int buses = 0;
        if (line_after(file, "[user-midi-bus-definitions]"))
            sscanf(scanline(), "%d", &buses);

        /*
         * [user-midi-bus-x]
         */

        for (int bus = 0; bus < buses; ++bus)
        {
            std::string label = make_section_name("user-midi-bus", bus);
            if (! line_after(file, label))
                break;

            std::string bussname = strip_quotes(line());
            if (usr().add_bus(bussname))
            {
                (void) next_data_line(file);
                int instruments = 0;
                sscanf(scanline(), "%d", &instruments);     /* no. of channels  */
                for (int j = 0; j < instruments; ++j)
                {
                    int channel, instrument;
                    (void) next_data_line(file);
                    sscanf(scanline(), "%d %d", &channel, &instrument);
                    if (! usr().set_bus_instrument(bus, channel, instrument))
                        break;
                }
            }
            else
            {
                fprintf
                (
                    stderr, "? error adding %s (line = '%s')\n",
                    label.c_str(), scanline()
                );
            }
        }
    }

    /*
     * [user-instrument-definitions]
     */

    int instruments = 0;
    if (line_after(file, "[user-instrument-definitions]"))
        sscanf(scanline(), "%d", &instruments);

    /*
     * [user-instrument-x]
     */

    for (int inst = 0; inst < instruments; ++inst)
    {
        std::string label = make_section_name("user-instrument", inst);
        if (! line_after(file, label))
            break;

        std::string instname = strip_quotes(line());
        if (usr().add_instrument(instname))
        {
            int cccount = 0;
            (void) next_data_line(file);
            sscanf(scanline(), "%d", &cccount);
            for (int cc = 0; cc < cccount; ++cc)
            {
                if (! next_data_line(file))
                    break;

                std::vector<std::string> instpair = tokenize(line(), " ");
                if (instpair.size() >= 1)
                {
                    int c = std::stoi(instpair[0]);
                    std::string name;
                    for (int i = 1; i < int(instpair.size()); ++i)
                    {
                        if (instpair[i][0] == '#')
                            break;

                        if (i > 1)
                            name += " ";

                        name += instpair[i];
                    }
                    name = strip_quotes(name);
                    if (name.empty())
                        name = "---";

                    if (! usr().set_instrument_controllers(inst, c, name, true))
                        break;
                }
            }
        }
        else
        {
            fprintf
            (
                stderr, "? error adding %s (line = '%s')\n",
                label.c_str(), scanline()
            );
        }
    }

    /*
     * [user-interface-settings]
     *
     * These are new items stored in the user file.  Only variables whose
     * effects we can be completely sure of are read from this section, and
     * used, at this time.  More to come.
     */

    int scratch = 0;
    std::string tag = "[user-interface-settings]";
    if (file_version_number() < s_usr_file_version)
    {
        if (line_after(file, tag))
        {
            (void) next_data_line(file); // usr().set_grid_style(scratch)
            (void) next_data_line(file); // usr().grid_brackets(scratch);
            sscanf(scanline(), "%d", &scratch);
            (void) usr().mainwnd_rows(scratch);
            (void) next_data_line(file);
            sscanf(scanline(), "%d", &scratch);
            (void) usr().mainwnd_cols(scratch);
            (void) next_data_line(file);
            (void) next_data_line(file); // usr().max_sets(scratch);
            (void) next_data_line(file); // usr().mainwnd_border(scratch);
            sscanf(scanline(), "%d", &scratch);
            usr().mainwnd_spacing(scratch);
            (void) next_data_line(file);
            (void) next_data_line(file); // usr().control_height(scratch);
            sscanf(scanline(), "%d", &scratch);
            usr().zoom(scratch);
            (void) next_data_line(file);
            sscanf(scanline(), "%d", &scratch);
            usr().global_seq_feature(scratch != 0);
            (void) next_data_line(file);
            (void) next_data_line(file); // usr().use_new_font(scratch != 0);
            (void) next_data_line(file); // usr().allow_two_perfedits(...);
            (void) next_data_line(file); // usr().perf_h_page_increment(...);
            if (next_data_line(file))    // usr().perf_v_page_increment(...);
            {
                (void) next_data_line(file); // usr().progress_bar_colored(...);
                sscanf(scanline(), "%d", &scratch);
                usr().progress_bar_thick(scratch != 0);
                (void) next_data_line(file);
                sscanf(scanline(), "%d", &scratch);
                usr().inverse_colors(scratch != 0);
                (void) next_data_line(file);
                sscanf(scanline(), "%d", &scratch);
                usr().window_redraw_rate(scratch);
                (void) next_data_line(file);
                (void) next_data_line(file); // usr().use_more_icons(scratch != 0);
                (void) next_data_line(file); // usr().block_rows(scratch);
                (void) next_data_line(file); // usr().block_columns(scratch);
                if (next_data_line(file))    // usr().block_independent(...);
                {
                    float scale = 1.0f;
                    float scaley = 1.0f;
                    int count = sscanf(scanline(), "%f %f", &scale, &scaley);
                    if (count == 1)
                        usr().window_scale(scale, 0.0, true);
                    else if (count == 2)
                        usr().window_scale(scale, scaley, true);
                }
            }
        }
    }
    else
    {
        int scratch = get_integer(file, tag, "mainwnd-rows");
        (void) usr().mainwnd_rows(scratch);
        scratch = get_integer(file, tag, "mainwnd-columns");
        (void) usr().mainwnd_cols(scratch);
        scratch = get_integer(file, tag, "mainwnd-spacing");
        usr().mainwnd_spacing(scratch);
        scratch = get_integer(file, tag, "default-zoom");
        usr().zoom(scratch);

        bool flag = get_boolean(file, tag, "global-seq-feature");
        usr().global_seq_feature(flag);
        flag = get_boolean(file, tag, "progress-bar-thick");
        usr().progress_bar_thick(flag);
        flag = get_boolean(file, tag, "inverse-colors");
        usr().inverse_colors(flag);
        flag = get_boolean(file, tag, "dark-theme");
        usr().dark_theme(flag);
        scratch = get_integer(file, tag, "window-redraw-rate");
        usr().window_redraw_rate(scratch);

        double scale = get_float(file, tag, "window-scale");
        double scaley = get_float(file, tag, "window-scale-y");
        usr().window_scale(scale, scaley, true);        /* x & y the same   */
    }
    usr().normalize();                                  /* recalculate      */

    /*
     * [user-midi-ppqn]
     */

    tag = "[user-midi-ppqn]";
    int ppqn = get_integer(file, tag, "default-ppqn");
    bool flag = get_boolean(file, tag, "use-file-ppqn");
    usr().default_ppqn(ppqn);
    usr().midi_ppqn(ppqn);              /* can change based on file PPQN    */
    usr().use_file_ppqn(flag);

    /*
     * [user-midi-settings]
     */

    tag = "[user-midi-settings]";
    if (file_version_number() < s_usr_file_version)
    {
        if (line_after(file, tag))
        {
            int scratch = 0;
            (void) next_data_line(file);
            sscanf(scanline(), "%d", &scratch);
            usr().midi_beats_per_bar(scratch);

            float beatspm;
            (void) next_data_line(file);
            sscanf(scanline(), "%f", &beatspm);
            usr().midi_beats_per_minute(midibpm(beatspm));

            (void) next_data_line(file);
            sscanf(scanline(), "%d", &scratch);
            usr().midi_beat_width(scratch);

            (void) next_data_line(file);
            sscanf(scanline(), "%d", &scratch);
            usr().midi_buss_override(char(scratch));

            if (next_data_line(file))
            {
                sscanf(scanline(), "%d", &scratch);
                usr().velocity_override(scratch);
            }
            if (next_data_line(file))
            {
                sscanf(scanline(), "%d", &scratch);
                usr().bpm_precision(scratch);
            }
            if (next_data_line(file))
            {
                float inc;
                sscanf(scanline(), "%f", &inc);
                usr().bpm_step_increment(midibpm(inc));
            }
            if (next_data_line(file))
            {
                float inc;
                sscanf(scanline(), "%f", &inc);
                usr().bpm_page_increment(midibpm(inc));
            }
            if (next_data_line(file))
            {
                sscanf(scanline(), "%f", &beatspm);
                usr().midi_bpm_minimum(midibpm(beatspm));
            }
            if (next_data_line(file))
            {
                sscanf(scanline(), "%f", &beatspm);
                usr().midi_bpm_maximum(midibpm(beatspm));
            }
        }
    }
    else
    {
        int scratch = get_integer(file, tag, "beats-per-bar");
        usr().midi_beats_per_bar(scratch);

        float f = get_float(file, tag, "beats-per-minute");
        usr().midi_beats_per_minute(midibpm(f));
        scratch = get_integer(file, tag, "beat-width");
        usr().midi_beat_width(scratch);
        scratch = get_integer(file, tag, "buss-override");
        usr().midi_buss_override(bussbyte(scratch));
        scratch = get_integer(file, tag, "velocity-override");
        usr().velocity_override(scratch);
        scratch = get_integer(file, tag, "bpm-precision");
        usr().bpm_precision(scratch);
        f = get_float(file, tag, "bpm-step-increment");
        usr().bpm_step_increment(midibpm(f));
        f = get_float(file, tag, "bpm-page-increment");
        usr().bpm_page_increment(midibpm(f));
        f = get_float(file, tag, "bpm-minimum");
        usr().midi_bpm_minimum(midibpm(f));
        f = get_float(file, tag, "bpm-maximum");
        usr().midi_bpm_maximum(midibpm(f));
    }

    /*
     * -o special options support.
     */

    tag = "[user-options]";
    if (file_version_number() < s_usr_file_version)
    {
        if (line_after(file, tag))
        {
            int scratch = 0;
            sscanf(scanline(), "%d", &scratch);
            usr().option_daemonize(scratch != 0);

            char temp[256];
            if (next_data_line(file))
            {
                sscanf(scanline(), "%s", temp);
                std::string logfile = std::string(temp);
                if (is_empty_string(logfile))
                    logfile.clear();
                else
                {
                    logfile = strip_quotes(logfile);
                    printf("[option_logfile: '%s']\n", logfile.c_str());
                }
                usr().option_logfile(logfile);
            }
        }
    }
    else
    {
        bool flag = get_boolean(file, tag, "daemonize");
        usr().option_daemonize(flag);

        std::string logfile = get_variable(file, tag, "log");
        logfile = strip_quotes(logfile);
        usr().option_logfile(logfile);
    }

    /*
     * [user-ui-tweaks].  The variables in this section are, in this order:
     * key-height and use-new-seqedit, which are currently not supporting the
     * new DOS-INI variable setting feature supported by get_variable().
     * The note-resume option is now implemented as per issue #5.
     */

    tag = "[user-ui-tweaks]";
    if (line_after(file, tag))
    {
        int scratch = 0;
        int count = sscanf(scanline(), "%d", &scratch);
        if (count == 1)
        {
            usr().key_height(scratch);
            (void) next_data_line(file);    // usr().use_new_seqedit()
        }
        else
        {
            int h = get_integer(file, tag, "key-height");
            usr().key_height(h);
        }

        std::string s = get_variable(file, tag, "key-view");
        usr().key_view(s);

        bool flag = get_boolean(file, tag, "note-resume");
        usr().resume_note_ons(flag);

        flag = get_boolean(file, tag, "style-sheet-active");
        usr().style_sheet_active(flag);

        s = get_variable(file, tag, "style-sheet");
        usr().style_sheet(strip_quotes(s));
        if (s.empty())
            usr().style_sheet_active(false);

        int v = get_integer(file, tag, "fingerprint-size");
        usr().fingerprint_size(v);

        double w = double(get_float(file, tag, "progress-box-width"));
        double h = double(get_float(file, tag, "progress-box-height"));
        usr().progress_box_size(w, h);
        v = get_integer(file, tag, "progress-note-min");
        usr().progress_note_min(v);
        v = get_integer(file, tag, "progress-note-max");
        usr().progress_note_max(v);
    }
    std::string s = get_variable(file, "[user-session]", "session");
    usr().session_manager(s);

    s = get_variable(file, "[user-session]", "url");
    usr().session_url(strip_quotes(s));
    flag = get_boolean(file, "[new-pattern-editor]", "armed");
    usr().new_pattern_armed(flag);
    flag = get_boolean(file, "[new-pattern-editor]", "thru");
    usr().new_pattern_thru(flag);
    flag = get_boolean(file, "[new-pattern-editor]", "record");
    usr().new_pattern_record(flag);
    flag = get_boolean(file, "[new-pattern-editor]", "qrecord");
    usr().new_pattern_qrecord(flag);
    s = get_variable(file, "[new-pattern-editor]", "record-style");
    usr().new_pattern_recordstyle(s);
    flag = get_boolean(file, "[new-pattern-editor]", "wrap-around");
    usr().new_pattern_wraparound(flag);

    /*
     * We have all of the data.  Close the file.
     */

    dump_setting_summary();
    file.close();                       /* End Of File, EOF, done! */
    return true;
}

/**
 *  This function just returns false, as there is no "performer" information
 *  in the user-file yet.
 *
 * \return
 *      Returns true if the writing succeeded.
 */

bool
usrfile::write ()
{
    std::ofstream file(name().c_str(), std::ios::out | std::ios::trunc);
    if (file.is_open())
    {
        file_message("Writing usr", name());
    }
    else
    {
        file_error("Write open fail", name());
        return false;
    }
    dump_setting_summary();

    /*
     * Header commentary.  Write out comments about the nature of this file.
     */

    write_date(file, "user ('usr')");
    file <<
        "# This is a Seq66 'usr' file. Edit it and place it in the\n"
        "# $HOME/.config/seq66 directory. It allows one to apply aliases\n"
        "# (alternate names) to each MIDI bus, channel, and control code, per\n"
        "# per channel. It has additional options not present in Seq24.\n"
        ;

    write_seq66_header(file, "usr", version());
    write_comment(file, usr().comments_block().text());
    file <<
        "# [user-midi-bus-definitions]\n"
        "#\n"
        "# 1. Define your instruments and their control-code names, if they\n"
        "#    have them.\n"
        "# 2. Define a MIDI bus, its name, and what instruments are on which\n"
        "#    channel.\n"
        "#\n"
        "# In the following MIDI buss definitions, channels are counted from\n"
        "# 0 to 15, not 1 to 16.  Instruments not set here are set to -1 and\n"
        "# are GM (General MIDI). These replacement MIDI buss labels are shown\n"
        "# in MIDI Clocks, MIDI Inputs, and in the Pattern Editor buss and\n"
        "# channel drop-downs. To disable the entries, set the counts to 0.\n"
        ;

    /*
     * [user-midi-bus-definitions]
     */

    file << "\n"
        "[user-midi-bus-definitions]\n"
        "\n"
        << usr().bus_count()
        << "     # number of user-defined MIDI busses\n"
        ;

    if (usr().bus_count() > 0)
        file << "\n";

    /*
     * [user-midi-bus-x]
     */

    for (int buss = 0; buss < usr().bus_count(); ++buss)
    {
        file << "\n" << make_section_name("user-midi-bus", buss) << "\n\n";

        const usermidibus & umb = usr().bus(buss);
        std::string bussname = add_quotes(umb.name());
        if (umb.is_valid())
        {
            file
                << "# Device/bus name\n\n"
                << bussname << "\n"
                << "\n"
                << umb.channel_count()
                << "      # number of instrument settings\n\n"
                << "# Channel, instrument number, and instrument names\n\n"
                ;

            for (int channel = 0; channel < umb.channel_count(); ++channel)
            {
                std::string instname = umb.instrument_name(channel);
                instname = add_quotes(instname);
                file
                    << std::setw(2) << channel << " " << umb.instrument(channel)
                    << " " << instname << "\n"
                    ;
            }
        }
        else
            file << "? This buss specification is invalid\n";
    }

    file << "\n"
        "# In the following MIDI instrument definitions, active controller\n"
        "# numbers (i.e. supported by the instrument) are paired with\n"
        "# the (optional) name of the controller supported.\n"
        ;

    /*
     * [user-instrument-definitions]
     */

    file << "\n"
        << "[user-instrument-definitions]\n"
        << "\n"
        << usr().instrument_count()
        << "     # instrument list count\n"
        ;

    if (usr().instrument_count() > 0)
        file << "\n";

    /*
     * [user-instrument-x]
     */

    for (int inst = 0; inst < usr().instrument_count(); ++inst)
    {
        file << "\n" << make_section_name("user-instrument", inst) << "\n\n";

        const userinstrument & uin = usr().instrument(inst);
        if (uin.is_valid())
        {
            std::string fixedname = add_quotes(uin.name());
            file
                << "# Name of instrument\n\n"
                << fixedname << "\n"
                << "\n"
                << uin.controller_count()
                << "    # number of MIDI controller number & name pairs\n"
                ;

            if (uin.controller_count() > 0)
            {
                for (int ctlr = 0; ctlr < uin.controller_max(); ++ctlr)
                {
                    std::string ctrlname = uin.controller_name(ctlr);
                    std::string fixedname = strip_quotes(ctrlname);
                    if (fixedname == "---" || is_empty_string(fixedname))
                        fixedname = empty_string();
                    else
                        fixedname = add_quotes(fixedname);

                    if (uin.controller_active(ctlr))
                    {
                        file << ctlr << " " << fixedname << "\n";
                    }
                }
            }
        }
        else
        {
            file << "? This instrument specification is invalid\n";
        }
    }

    /*
     * [user-interface settings]
     *
     * These are new items stored in the user file.  The settings are obtained
     * from member functions of the user_settings class.  Not all members are
     * saved to the "user" configuration file.
     */

    file << "\n"
        "# [user-interface-settings]\n"
        "#\n"
        "# Configures some user-interface elements.  Many became obsolete and\n"
        "# were removed in version 5 of this file. The grid contains Qt buttons\n"
        "# For a flat style, use Qt themes or style-sheets.\n"
        "#\n"
        "# 'mainwnd-rows' and 'mainwnd-columns' (option '-o sets=RxC') specify\n"
        "# rows/columns in the main grid. R ranges from 4 to 8, C from 4 to 12.\n"
        "# Values other than 4x8 have not been tested thoroughly.\n"
        "#\n"
        "# 'mainwnd-spacing' is for grid buttons; from 0 to 16, default = 2.\n"
        "#\n"
        "# 'default-zoom' is the initial zoom for the piano rolls. From 1 to\n"
        "# 512, defaults = 2. Larger PPQNs require larger zoom to look good.\n"
        "# Seq66 adapts the zoom to the PPQN if set to 0. The unit of zoom is\n"
        "# ticks/pixel.\n"
        "#\n"
        "# 'global-seq-feature' specifies if the key, scale, and background\n"
        "# pattern are applied to all patterns, or separately to each.  These\n"
        "# These values are stored in the MIDI file, either in the global\n"
        "# SeqSpec (if true) or in each track (if false).\n"
        "#\n"
        "# 'progress-bar-thick specifies a thicker progress bar.  Default is 1\n"
        "# pixel; thick is 2 pixels if set to true.\n"
        "#\n"
        "# 'inverse-colors' (option -K/--inverse) specifies use of an inverse\n"
        "# color palette. Palettes are for Seq66 drawing areas, not for the\n"
        "# Qt theme. Normal/inverse palettes are changed via a 'palette' file.\n"
        "#\n"
        "# 'dark-theme' specifies that are dark theme is active.\n"
        "#\n"
        "# 'window-redraw-rate' specifies the base window redraw rate for all\n"
        "# windows. From 10 to 100; default = 40 ms (25 ms for Windows).\n"
        "#\n"
        "# Window-scale (option '-o scale=m.n[xp.q]') specifies scaling the\n"
        "# main window at startup. Defaults to 1.0 x 1.0. If between 0.5 and\n"
        "# 3.0, it changes the size of the main window proportionately.\n"
        "\n[user-interface-settings]\n\n"
        ;

    write_integer(file, "mainwnd-rows", usr().mainwnd_rows());
    write_integer(file, "mainwnd-columns", usr().mainwnd_cols());
    write_integer(file, "mainwnd-spacing", usr().mainwnd_spacing());
    write_integer(file, "default-zoom", usr().zoom());
    write_boolean(file, "global-seq-feature", usr().global_seq_feature());
    write_boolean(file, "progress-bar-thick", usr().progress_bar_thick());
    write_boolean(file, "inverse-colors", usr().inverse_colors());
    write_boolean(file, "dark-theme", usr().dark_theme());
    write_integer(file, "window-redraw-rate", usr().window_redraw_rate());
    write_float(file, "window-scale", usr().window_scale());
    write_float(file, "window-scale-y", usr().window_scale_y());

    /*
     * [user-midi-ppqn]
     */

    file << "\n"
        "# [user-midi-ppqn]\n"
        "#\n"
        "# Seq66 separates file PPQN from the Seq66 PPQN the user wants.\n"
        "# 'default-ppqn' specifies the Seq66 PPQN, from 32 to 19200, default\n"
        "# = 192. 'use-file-ppqn' (recommended) indicates to use file PPQN.\n"
        "\n[user-midi-ppqn]\n\n"
        ;

    write_integer(file, "default-ppqn", usr().default_ppqn());
    write_boolean(file, "use-file-ppqn", usr().use_file_ppqn());

    /*
     * [user-midi-settings] and [user-options]
     */

    file << "\n"
        "# [user-midi-settings]\n"
        "#\n"
        "# Specifies MIDI-specific variables. -1 means the value isn't used.\n"
        "#\n"
        "#  Item                 Default   Range\n"
        "# 'beats-per-bar':      4         1 to 32.\n"
        "# 'beats-per-minute':   120.0     2.0 to 600.0.\n"
        "# 'beat-width':         4         1 to 32.\n"
        "# 'buss-override':     -1 (none) -1 to 48.\n"
        "# 'velocity-override': -1 (Free) -1 to 127.\n"
        "# 'bpm-precision':      0         0 to 2.\n"
        "# 'bpm-step-increment': 1.0       0.01 to 25.0.\n"
        "# 'bpm-page-increment': 1.0       0.01 to 25.0.\n"
        "# 'bpm-minimum':        0.0       127.0\n"
        "# 'bpm-maximum':        0.0       127.0\n"
        "#\n"
        "# 'buss-override' sets the output port for all patterns, for testing\n"
        "# or convenience.  Don't save the MIDI file unless you want to save\n"
        "# the buss value!\n"
        "#\n"
        "# 'velocity-override' when adding notes in the pattern editor is set\n"
        "# via the 'Vol' button.  -1 ('Free'), preserves incoming velocity.\n"
        "#\n"
        "# 'bpm-precision' (spinner and MIDI control) is 0, 1, or 2.\n"
        "# 'bpm-step-increment' affects the spinner and MIDI control. For 1\n"
        "# decimal point, 0.1 is good. For 2, 0.01 is good, 0.05 is faster.\n"
        "# Set 'bpm-page-increment' larger than the step-increment; used with\n"
        "# the Page-Up/Page-Down keys in the spinner. BPM minimum/maximum sets\n"
        "# the range in tempo graphing; defaults to 0.0 to 127.0. Decrease it\n"
        "# for a magnified view of tempo.\n"
        "\n[user-midi-settings]\n\n"
        ;

        write_integer(file, "beats-per-bar", usr().midi_beats_per_bar());
        write_integer(file, "beats-per-minute", usr().midi_beats_per_minute());
        write_integer(file, "beat-width", usr().midi_beat_width());

        int bo = int(usr().midi_buss_override());   /* writing char no good */
        float increment = float(usr().bpm_step_increment());
        if (is_null_buss(bussbyte(bo)))
            bo = (-1);

        write_integer(file, "buss-override", bo);
        write_integer(file, "velocity-override", usr().velocity_override());
        write_integer(file, "bpm-precision", usr().bpm_precision());
        write_float(file, "bpm-step-increment", increment);
        increment = float(usr().bpm_page_increment());
        write_float(file, "bpm-page-increment", increment);
        increment = float(usr().midi_bpm_minimum());
        write_float(file, "bpm-minimum", increment);
        increment = float(usr().midi_bpm_maximum());
        write_float(file, "bpm-maximum", increment);

    /*
     * [user-options]
     */

    file << "\n"
        "# [user-options]\n"
        "#\n"
        "# These settings specify -o or --option switch values.\n"
        "# 'daemonize' is used in seq66cli to indicate the application should \n"
        "# be run as a service. 'log' specifies a log-file that gets output to\n"
        "# standard output/error.  For no log-file, use \"\".  This option\n"
        "# also works from the command line: '-o log=filename.log'. The name\n"
        "# here is used for the no-name '-o log' option.\n"
        "\n[user-options]\n\n"
        ;

    write_boolean(file, "daemonize", usr().option_daemonize());
    std::string logfile = add_quotes(usr().option_logfile());
    file << "log = " << logfile << "\n";

    /*
     * [user-ui-tweaks]
     */

    file << "\n"
        "# [user-ui-tweaks]\n"
        "#\n"
        "# key-height specifies the initial height (before vertical zoom) of\n"
        "# the pattern editor keys.  Defaults to 10 pixels, ranges from 6 to\n"
        "# 32.\n"
        "#\n"
        "# key-view specifies how to show the labels for each key:\n"
        "# 'octave-letters' (default), 'even_letters', 'all-letters',\n"
        "# 'even-numbers', and 'all-numbers'.\n"
        "#\n"
        "# note-resume, if active, causes notes in progress to be resumed when\n"
        "# the pattern is toggled back on.\n"
        "#\n"
        "# If specified, a style-sheet (e.g. 'qseq66.qss') is applied at\n"
        "# startup.  Normally just a base-name, it can contain a file-path\n"
        "# to provide a style usable in many other applications.\n"
        "#\n"
        "# A fingerprint is a condensation of note events in a long track, to\n"
        "# reduce the time drawing the pattern in the buttons. Ranges from 32\n"
        "# (default) to 128. 0 = don't use a fingerprint.\n"
        "#\n"
        "# progress-box width and height settings change the scaled size of the\n"
        "# progress box in the live-loop grid buttons.  Width ranges from 0.50\n"
        "# to 1.0; the height from 0.10 to 0.50.  If either is 0, then the box\n"
        "# isn't drawn.  If either is 'default', defaults are used.\n"
        "#\n"
        "# progress-note-min and progress-note-max, if non-zero, change the\n"
        "# note range in the progress box so that notes are'nt centered in the\n"
        "# box, but shown at their position by pitch.\n"
        "\n[user-ui-tweaks]\n\n"
        ;

    write_integer(file, "key-height", usr().key_height());
    file << "key-view = " << usr().key_view_string() << "\n";
    write_boolean(file, "note-resume", usr().resume_note_ons());
    write_boolean(file, "style-sheet-active", usr().style_sheet_active());

    std::string v = add_quotes(usr().style_sheet());
    file << "style-sheet = " << v << "\n";
    write_integer(file, "fingerprint-size", usr().fingerprint_size());
    if (usr().progress_box_width() < 0.0)
        file << "progress-box-width = default\n";
    else
        write_float(file, "progress-box-width", usr().progress_box_width());

    if (usr().progress_box_height() < 0.0)
        file << "progress-box-height = default\n";
    else
        write_float(file, "progress-box-height", usr().progress_box_height());

    write_integer(file, "progress-note-min", usr().progress_note_min());
    write_integer(file, "progress-note-max", usr().progress_note_max());

    /*
     * [user-session]
     */

    file << "\n"
        "# [user-session]\n"
        "#\n"
        "# The session manager to use, if any. The 'session' value is 'none'\n"
        "# (default), 'nsm' (Non Session Manager), or 'jack' (JACK Session).\n"
        "# 'url' can be set to the value set by nsmd when run outside of the\n"
        "# the NSM user-interface. Set 'url' only if running nsmd stand-alone;\n"
        "# use a matching --osc-port number. Seq66 will detect if running in an\n"
        "# NSM environment, and override these settings.\n"
        "\n[user-session]\n\n"
        ;

    v = usr().session_url();
    v = add_quotes(v);
    file
        << "session = " << usr().session_manager_name() << "\n"
        << "url = " << v << "\n"
        ;

    /*
     * [new-pattern-editor]
     */

    file << "\n"
        "# [new-pattern-editor]\n"
        "#\n"
        "# Setup values for play/recording when a new pattern is opened.\n"
        "# A new pattern means that the loop has the default name 'Untitled'\n"
        "# and no events. These values save time during a live recording\n"
        "# session. The valid values for record-style are 'merge' (default),\n"
        "# 'overwrite', 'expand', and 'one-shot'. The 'wrap-around' value, if\n"
        "# true, allows recorded notes to wrap around to the pattern start.\n"
        "\n[new-pattern-editor]\n\n"
        ;

    write_boolean(file, "armed", usr().new_pattern_armed());
    write_boolean(file, "thru", usr().new_pattern_thru());
    write_boolean(file, "record", usr().new_pattern_record());
    write_boolean(file, "qrecord", usr().new_pattern_qrecord());
    file << "record-style = " << usr().new_pattern_record_string() << "\n";
    write_boolean(file, "wrap-around", usr().new_pattern_wraparound());

    /*
     * EOF
     */

    file << "\n"
        << "# End of " << name() << "\n#\n# vim: sw=4 ts=4 wm=4 et ft=dosini\n"
        ;

    file.close();
    return true;
}

}           // namespace seq66

/*
 * usrfile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


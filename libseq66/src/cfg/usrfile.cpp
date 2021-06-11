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
 * \updates       2021-06-08
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

static const int s_usr_file_version = 5;

/**
 *  Principal constructor.
 *
 * Versions:
 *
 *      0:  The initial version, close to the Seq64 format.
 *      4:  2021-05-15. Disabled using grid-style and grid-brackets settings.
 *      5:  2021-06-08. Transition to get-variable for booleans/integers.
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
        usr().save_user_config(true);

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
            usr().mainwnd_rows(scratch);
            (void) next_data_line(file);
            sscanf(scanline(), "%d", &scratch);
            usr().mainwnd_cols(scratch);
            (void) next_data_line(file);
            (void) next_data_line(file); // usr().max_sets(scratch);
            (void) next_data_line(file); // usr().mainwid_border(scratch);
            sscanf(scanline(), "%d", &scratch);
            usr().mainwid_spacing(scratch);
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
                        usr().window_scale(scale);          /* x & y the same   */
                    else if (count == 2)
                        usr().window_scale(scale, scaley);  /* x != y scale     */
                }
            }
        }
    }
    else
    {
        int scratch = get_integer(file, tag, "mainwnd-rows");
        usr().mainwnd_rows(scratch);
        scratch = get_integer(file, tag, "mainwnd-columns");
        usr().mainwnd_cols(scratch);
        scratch = get_integer(file, tag, "mainwnd-spacing");
        usr().mainwid_spacing(scratch);
        scratch = get_integer(file, tag, "default-zoom");
        usr().zoom(scratch);

        bool flag = get_boolean(file, tag, "global-seq-feature");
        usr().global_seq_feature(flag);
        flag = get_boolean(file, tag, "progress-bar-thick");
        usr().progress_bar_thick(flag);
        flag = get_boolean(file, tag, "inverse-colors");
        usr().inverse_colors(flag);
        scratch = get_integer(file, tag, "window-redraw-rate");
        usr().window_redraw_rate(scratch);

        double scale = get_float(file, tag, "window-scale");
        double scaley = get_float(file, tag, "window-scale-y");
        usr().window_scale(scale, scaley);              /* x & y the same   */
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

        bool flag = get_boolean(file, tag, "note-resume");
        usr().resume_note_ons(flag);

        std::string s = get_variable(file, tag, "style-sheet");
        usr().style_sheet(strip_quotes(s));

        int v = get_integer(file, tag, "fingerprint-size");
        usr().fingerprint_size(v);

        double w = double(get_float(file, tag, "progress-box-width"));
        double h = double(get_float(file, tag, "progress-box-height"));
        usr().progress_box_size(w, h);
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

    recordstyle rs = recordstyle::merge;
    s = get_variable(file, "[new-pattern-editor]", "record-style");
    if (s == "overwrite")
        rs = recordstyle::overwrite;
    else if (s == "expand")
        rs = recordstyle::expand;

    usr().new_pattern_recordstyle(rs);

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
        file_message("Writing 'usr'", name());
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
        "# This is a seq66.usr file. Edit it and place it in the\n"
        "# $HOME/.config/seq66 directory. It allows one to provide an\n"
        "# alias (alternate name) to each MIDI bus, MIDI channel, and MIDI\n"
        "# control codes per channel. It has additional options not present\n"
        "# in Seq24, and also supports DOS INI variable setting.\n"
        ;

    file <<
        "\n[Seq66]\n\n"
        "config-type = \"usr\"\n"
        "version = " << version() << "\n"
        ;

    file <<
    "\n"
    "# The [comments] section lets one document this file.  Lines starting\n"
    "# with '#' and '[' are ignored.  Blank lines are ignored.  To show a\n"
    "# blank line, add a space character to the line.\n"
        ;

    /*
     * [comments]
     */

    file << "\n[comments]\n\n" << usr().comments_block().text() << "\n";

    file <<
        "# [user-midi-bus-definitions]\n"
        "#\n"
        "# 1. Define your instruments and their control-code names, if they\n"
        "#    have them.\n"
        "# 2. Define a MIDI bus, its name, and what instruments are on which\n"
        "#    channel.\n"
        "#\n"
        "# In the following MIDI buss definitions, channels are counted from\n"
        "# 0 to 15, not 1 to 16.  Instruments not set here are set to -1\n"
        "# (SEQ66_GM_INSTRUMENT_FLAG) and are GM (General MIDI). These\n"
        "# replacement MIDI buss labels are shown in MIDI Clocks, MIDI Inputs\n"
        "# and in the Pattern Editor buss and channel drop-downs.\n"
        "# To temporarily disable the entries, set the count values to 0.\n"
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

#if defined SEQ66_PLATFORM_DEBUG && defined SHOW_IGNORED_ITEMS
                else
                {
                    fprintf
                    (
                        stderr, "bus %d, channel %d (%d) ignored\n",
                        buss, channel, umb.instrument(channel)
                    );
                }
#endif
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
#if defined SEQ66_PLATFORM_DEBUG && defined SHOW_IGNORED_ITEMS
                    else
                    {
                        fprintf
                        (
                            stderr, "instrument %d, controller %d (%s) ignored\n",
                            inst, ctlr, uin.controller_name(ctlr).c_str()
                        );
                    }
#endif
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
        "# Specifies the configuration of some user-interface elements.  Many\n"
        "# items were rendered obsolete and removed in version 5 of this file.\n"
        "# The main grid-style is now Qt buttons. To use a flat style, use\n"
        "# Qt themes/style-sheets.\n"
        "#\n"
        "# 'mainwnd-rows' and 'mainwnd-columns' (option '-o sets=RxC') specify\n"
        "# rows/columns in the main grid. R ranges from 4 to 8, C from 8 to 12.\n"
        "# Values other than 4x8 have not been tested, use at your own risk.\n"
        "#\n"
        "# 'mainwnd-spacing' specifies spacing in the main window. It ranges\n"
        "# from 2 (default) to 16.\n"
        "#\n"
        "# 'default-zoom' specifies initial zoom for the piano rolls. Ranges\n"
        "# from 1 to 512; defaults to 2. Larger PPQNs require larger zoom to\n"
        "# look good in the editors.  Seq66 adapts the zoom to the PPQN\n"
        "# if 'default-zoom' zoom is set to 0. The unit of zoon is ticks/pixel.\n"
        "#\n"
        "# 'global-seq-feature' specifies if the key, scale, and background\n"
        "# pattern are to be applied globally to all patterns, or separately\n"
        "# to each. These three values are stored in the MIDI file, either in\n"
        "# the global SeqSpec section, or in each track.\n"
        "#\n"
        "#   false: Each pattern has its own key/scale/background.\n"
        "#   true:  Apply these settings globally to all patterns.\n"
        "#\n"
        "# 'progress-bar-thick specifies a thicker progress bar.  Default is 1\n"
        "# pixel; thick is 2 pixels. Set it to true to enable the feature\n"
        "#\n"
        "# 'inverse-colors' (option -K/--inverse) specifies use of an inverse\n"
        "# color palette. Palettes are for Seq66 drawing areas, not for the\n"
        "# Qt theme. Normal/inverse palettes are changed via a 'palette' file.\n"
        "#\n"
        "# 'window-redraw-rate' specifies the base window redraw rate for all\n"
        "# windows. The default is 40 ms (25 ms for Windows).\n"
        "#\n"
        "# Window-scale (option '-o scale=m.n[xp.q]') specifies scaling the\n"
        "# main window at startup. Defaults to 1.0 x 1.0. If between 0.8 and\n"
        "# 3.0, it changes the size of the main window proportionately. If the\n"
        "# y-value is 0, the first value applies to both dimensions.\n"
        "\n[user-interface-settings]\n\n"
        ;

    write_integer(file, "mainwnd-rows", usr().mainwnd_rows());
    write_integer(file, "mainwnd-columns", usr().mainwnd_cols());
    write_integer(file, "mainwnd-spacing", usr().mainwid_spacing());
    write_integer(file, "default-zoom", usr().zoom());
    write_boolean(file, "global-seq-feature", usr().global_seq_feature());
    write_boolean(file, "progress-bar-thick", usr().progress_bar_thick());
    write_boolean(file, "inverse-colors", usr().inverse_colors());
    write_integer(file, "window-redraw-rate", usr().window_redraw_rate());
    write_float(file, "window-scale", usr().window_scale());
    write_float(file, "window-scale-y", usr().window_scale_y());

    /*
     * [user-midi-ppqn]
     */

    file << "\n"
        "# [user-midi-ppqn]\n"
        "#\n"
        "# Seq66 separates the file PPQN from the Seq66 PPQN the user wants\n"
        "# to use.\n"
        "#\n"
        "# 'default-ppqn' specifies the PPQN to use by default. The classic\n"
        "# default is 192, but can range from 32 to 19200.\n"
        "#\n"
        "# 'use-file-ppqn' indicates to use the file PPQN. This is the best\n"
        "# setting, to avoid changing the file's PPQN.\n"
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
        "# These settings specify MIDI-specific values better off as variables,\n"
        "# rather than constants. Values of -1 mean the value won't be used.\n"
        "#\n"
        "# 'beats-per-bar':      default = 4      range = 1 to 32.\n"
        "# 'beats-per-minute':   default = 120.0  range = 2.0 to 600.0.\n"
        "# 'beat-width':         default = 4      range = 1 to 32.\n"
        "# 'buss-override':      default = -1     range = 0 to 48.\n"
        "# 'velocity-override':  default = -1     range = 0 to 127.\n"
        "# 'bpm-precision':      default = 0      range = 0 to 2.\n"
        "# 'bpm-step-increment': default = 1.0    range = 0.01 to 25.0.\n"
        "# 'bpm-page-increment': default = 1.0    range = 0.01 to 25.0.\n"
        "# 'bpm-minimum':        default = 0.0    range = 127.0\n"
        "# 'bpm-maximum':        default = 0.0    range = 127.0\n"
        "#\n"
        "# A buss-override from 0 to 48 overrides the busses for all patterns,\n"
        "# for testing or convenience.  Do not save the MIDI file afterwards\n"
        "# unless you want to overwrite all the buss values!\n"
        "#\n"
        "# The velocity override when adding notes in the pattern editor is set\n"
        "# via the 'Vol' button.  -1 ('Free'), preserves incoming velocity.\n"
        "#\n"
        "# Precision of the BPM spinner and MIDI control of BPM is 0, 1, or 2.\n"
        "# The step increment affects the beats/minute spinner and MIDI control\n"
        "# of BPM.  For 1 decimal point, 0.1 is good.  For 2 decimal points,\n"
        "# 0.01 is good, but one might want something faster, like 0.05.\n"
        "# Set the page increment to a larger value than the step increment;\n"
        "# it is used when the Page-Up/Page-Down keys are pressed when the BPM\n"
        "# spinner has keyboard focus.\n"
        "# The BPM-minimum and maximum set the range BPM in tempo graphing.\n"
        "# By default, the tempo graph ranges from 0.0 to 127.0. This range\n"
        "# decreased to give a magnified view of tempo.\n"
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
        "# These settings specify values set via the -o or --option switch,\n"
        "# which helps expand the number of options supported.\n"
        "# The 'daemonize' option is used in seq66cli to indicate that the\n"
        "# application should be gracefully run as a service.\n"
        "# The 'log' value specifies a log-file that replaces output to\n"
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
        "# The key-height value specifies the initial height (before vertical\n"
        "# zoom) of the keys in the pattern editor.  Defaults to 10 pixels,\n"
        "# ranges from 6 to 32.\n"
        "#\n"
        "# The note-resume option, if active, causes any notes in progress\n"
        "# to be resumed when the pattern is toggled back on.\n"
        "#\n"
        "# If specified, a style-sheet (e.g. 'qseq66.qss') is applied at\n"
        "# startup.  Normally just a base-name, it can contain a file-path\n"
        "# to provide a style usable in many other applications.\n"
        "#\n"
        "# A fingerprint is a condensation of the note events in a long track,\n"
        "# to reduce the amount of drawing in the grid buttons. Ranges from 32\n"
        "# (the default) to 128. Set to 0 to not use a fingerprint.\n"
        "#\n"
        "# The progress-box width and height settings change the size of the\n"
        "# progress box in the live-loop grid buttons.  Width ranges from 0.50\n"
        "# to 1.0; the height from 0.10 to 0.50.  If either is 0, then the box\n"
        "# isn't drawn.  If either is 'default', defaults are used.\n"
        "\n[user-ui-tweaks]\n\n"
        ;

    write_integer(file, "key-height", usr().key_height());
    write_boolean(file, "note-resume", usr().resume_note_ons());

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

    /*
     * [user-session]
     */

    file << "\n"
        "# [user-session]\n"
        "#\n"
        "# This section specifies the session manager to use, if any. The\n"
        "# 'session' variable can be set to 'none' (the default), 'nsm'\n"
        "# (Non or New Session Manager), or 'lash' (LASH, not yet supported).\n"
        "# 'url' can be set to the value of the NSM_URL environment variable\n"
        "# set by nsmd when run outside of the Non Session Manager user-\n"
        "# interface. Set the URL only if running nsmd standalone with a\n"
        "# matching --osc-port number.\n"
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
        "# This section contains the setup values for recording when a new\n"
        "# pattern is opened. For flexibility, a new pattern means only that\n"
        "# the loop has the default name, 'Unititled'. These values save time\n"
        "# during a live recording session. Note that the valid values for\n"
        "# record-style are 'merge', 'overwrite', and 'expand'.\n"
        "\n[new-pattern-editor]\n\n"
        ;

    write_boolean(file, "armed", usr().new_pattern_armed());
    write_boolean(file, "thru", usr().new_pattern_thru());
    write_boolean(file, "record", usr().new_pattern_record());
    write_boolean(file, "qrecord", usr().new_pattern_qrecord());

    std::string rs = "merge";
    if (usr().new_pattern_recordstyle() == recordstyle::overwrite)
        rs = "overwrite";
    else if (usr().new_pattern_recordstyle() == recordstyle::expand)
        rs = "expand";

    file << "record-style = " << rs << "\n";

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


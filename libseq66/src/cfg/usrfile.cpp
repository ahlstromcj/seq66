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
 * \updates       2021-11-29
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

static const int s_usr_legacy = 5;
static const int s_usr_smf_1 = 8;
static const int s_usr_file_version = 9;

/**
 *  Principal constructor.
 *
 * Versions:
 *
 *      0:  The initial version, close to the Seq64 format.
 *      4:  2021-05-15. Disabled using grid-style and grid-brackets settings.
 *      5:  2021-06-08. Transition to get-variable for booleans/integers.
 *      6:  2021-07-26. Added progress-note-min and progress-note-max.
 *      7:  2021-09-20. Added "style-sheet-active" and "lock-main-window"
 *          flags.
 *      8:  2021-10-06: Added "convert-to-smf-l".
 *      9:  2021-10-26: Added "swap-coordinates".
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
                sscanf(scanline(), "%d", &instruments); /* no. of channels  */
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
                msgprintf
                (
                    msglevel::error, "Error adding %s (line = '%s')",
                    label, scanline()
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

                tokenization instpair = tokenize(line(), " ");
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
            msgprintf
            (
                msglevel::error, "Error adding %s (line = '%s')",
                label, scanline()
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

    std::string tag = "[user-interface-settings]";
    if (file_version_number() < s_usr_legacy)
    {
        (void) version_error_message("usr", file_version_number());
    }
    else
    {
        bool flag = get_boolean(file, tag, "swap-coordinates");
        usr().swap_coordinates(flag);

        int scratch = get_integer(file, tag, "mainwnd-rows");
        (void) usr().mainwnd_rows(scratch);
        scratch = get_integer(file, tag, "mainwnd-columns");
        (void) usr().mainwnd_cols(scratch);
        scratch = get_integer(file, tag, "mainwnd-spacing");
        usr().mainwnd_spacing(scratch);
        scratch = get_integer(file, tag, "default-zoom");
        usr().zoom(scratch);
        flag = get_boolean(file, tag, "global-seq-feature");
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
    if (file_version_number() < s_usr_smf_1)
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
        bool flag = get_boolean(file, tag, "convert-to-smf-1");
        std::string c = get_variable(file, tag, "convert-to-smf-1");
        bool convert = c.empty() ?  true : flag ;
        usr().convert_to_smf_1(convert);

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
    flag = get_boolean(file, tag, "daemonize");
    usr().option_daemonize(flag);

    std::string logfile = get_variable(file, tag, "log");
    logfile = strip_quotes(logfile);
    usr().option_logfile(logfile);

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

        int x = get_integer(file, tag, "progress-note-max");
        usr().progress_note_min_max(v, x);
        flag = get_boolean(file, tag, "lock-main-window");
        usr().lock_main_window(flag);
    }
    std::string s = get_variable(file, "[user-session]", "session");
    usr().session_manager(s);

    s = get_variable(file, "[user-session]", "url");
    usr().session_url(strip_quotes(s));
    s = get_variable(file, "[user-session]", "visibility");
    if (! s.empty())                    /* check succeeds, get it for real  */
    {
        flag = get_boolean(file, "[user-session]", "visibility");
        usr().session_visibility(flag);
    }
    flag = get_boolean(file, "[new-pattern-editor]", "armed");
    usr().new_pattern_armed(flag);
    flag = get_boolean(file, "[new-pattern-editor]", "thru");
    usr().new_pattern_thru(flag);
    flag = get_boolean(file, "[new-pattern-editor]", "record");
    usr().new_pattern_record(flag);
    flag = get_boolean(file, "[new-pattern-editor]", "qrecord");
    usr().new_pattern_qrecord(flag);
    s = get_variable(file, "[new-pattern-editor]", "record-style");
    usr().new_pattern_record_style(s);
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
        "# 'usr' file. Edit it and place it in $HOME/.config/seq66. It allows\n"
        "# aliases (alternate names) to each MIDI bus/port, channel, and\n"
        "# control code, per channel. It has options not present in Seq24.\n"
        ;

    write_seq66_header(file, "usr", version());
    write_comment(file, usr().comments_block().text());
    file <<
        "# [user-midi-bus-definitions]\n"
        "#\n"
        "# 1. Define instruments and their control-code names, as applicable.\n"
        "# 2. Define MIDI busses, names, and the instruments on each channel.\n"
        "#\n"
        "# In these definitions, channels are counted from 0-15, not 1-16.\n"
        "# Instruments not set here are set to -1 and are GM (General MIDI).\n"
        "# These labels are shown in MIDI Clocks, Inputs, the pattern editor\n"
        "# buss, channel, and event drop-downs.  To disable entries, set\n"
        "# counts to 0.\n"
        ;

    /*
     * [user-midi-bus-definitions]
     */

    file << "\n[user-midi-bus-definitions]\n\n"
        << usr().bus_count() << "     # number of user-defined MIDI busses\n"
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
        "# In these MIDI instrument definitions, active controller numbers\n"
        "# (i.e. supported by the instrument) are paired with the (optional)\n"
        "# name of the controller supported.\n"
        ;

    /*
     * [user-instrument-definitions]
     */

    file << "\n[user-instrument-definitions]\n\n"
        << usr().instrument_count() << "     # instrument list count\n"
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
                << fixedname << "\n\n"
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
                        file << ctlr << " " << fixedname << "\n";
                }
            }
        }
        else
            file << "? This instrument specification is invalid\n";
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
        "# were removed in version 5 of this file. Also see [user-ui-tweaks].\n"
        "# The grid holds Qt buttons. For styling, use Qt themes/style-sheets.\n"
        "#\n"
        "# 'swap-coordinates' swaps numbering so pattern numbers vary fastest\n"
        "# by column instead of (legacy) rows. This setting applies to the live\n"
        "# grid, mute-group buttons, and set-buttons.\n"
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
        "# pixel; thick is 2 pixels if set to true. Also makes the progress box\n"
        "# border 2 pixels, and the slot font bold.\n"
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
    write_boolean(file, "swap-coordinates", usr().swap_coordinates());
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
        "# Seq66 separates file PPQN from the Seq66 PPQN. 'default-ppqn'\n"
        "# specifies the Seq66 PPQN, from 32 to 19200, default = 192.\n"
        "# 'use-file-ppqn' (recommended) indicates to use file PPQN.\n"
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
        "# 'convert-to-smf-1':   true      true/false.\n"
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
        "# 'convert-to-smf-1' controls if SMF 0 files are split into SMF 1\n"
        "# track when read.\n"
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
        write_boolean(file, "convert-to-smf-1", usr().convert_to_smf_1());
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
    write_string(file, "log", usr().option_logfile(), true);

    /*
     * [user-ui-tweaks]
     */

    file << "\n"
        "# [user-ui-tweaks]\n"
        "#\n"
        "# key-height specifies the initial height (before vertical zoom) of\n"
        "# pattern editor keys.  Defaults to 10 pixels, ranges from 6 to 32.\n"
        "#\n"
        "# key-view specifies the default for showing labels for each key:\n"
        "# 'octave-letters' (default), 'even_letters', 'all-letters',\n"
        "# 'even-numbers', and 'all-numbers'.\n"
        "#\n"
        "# note-resume, if active, causes notes-in-progress to be resumed when\n"
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
        "# progress-note-min and progress-note-max set the progress-box note\n"
        "# range so that notes aren't centered in the\n"
        "# box, but shown at their position by pitch.\n"
        "#\n"
        "# lock-main-window prevents the accidental change of size of the main\n"
        "# window.\n"
        "\n[user-ui-tweaks]\n\n"
        ;

    write_integer(file, "key-height", usr().key_height());
    write_string(file, "key-view", usr().key_view_string());
    write_boolean(file, "note-resume", usr().resume_note_ons());
    write_boolean(file, "style-sheet-active", usr().style_sheet_active());
    write_string(file, "style-sheet", usr().style_sheet(), true);
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
    write_boolean(file, "lock-main-window", usr().lock_main_window());

    /*
     * [user-session]
     */

    file << "\n# [user-session]\n"
        "#\n"
        "# The session manager to use, if any. The 'session' value is 'none'\n"
        "# (default), 'nsm' (Non/New Session Manager), or 'jack'. 'url' can be\n"
        "# be set to the value set by nsmd when run by command-line. Set 'url'\n"
        "# only if running nsmd stand-alone; use the --osc-port number. Seq66\n"
        "# detects if started in an NSM environment, though. The visibility\n"
        "# flag is used only by NSM to restore visibility.\n"
        "\n[user-session]\n\n"
        ;
    write_string(file, "session", usr().session_manager_name());
    write_string(file, "url", usr().session_url(), true);
    write_boolean(file, "visibility", usr().session_visibility());

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
    write_string(file, "record-style", usr().new_pattern_record_string());
    write_boolean(file, "wrap-around", usr().new_pattern_wraparound());

    /*
     * EOF
     */

    file
        << "\n# End of " << name() <<
        "\n#\n# vim: sw=4 ts=4 wm=4 et ft=dosini\n"
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


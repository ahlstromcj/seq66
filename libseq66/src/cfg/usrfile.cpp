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
 * \updates       2023-08-22
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
#include "util/strfunctions.hpp"        /* seq66::strip_quotes()            */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

static const int s_usr_legacy = 5;
static const int s_usr_smf_1 = 8;
static const int s_usr_file_version = 10;       /* from 9 on 2022-07-21     */

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
 *      8:  2021-10-06: Added "convert-to-smf-1".
 *      9:  2021-10-26: Added "swap-coordinates".
 *     10:  2022-07-21: Added "pattern-box-shown" (issue #78).
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
    configfile (name, rcs, ".usr")
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
                    int c = string_to_int(instpair[0]);
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

        int rows = get_integer(file, tag, "mainwnd-rows");
        (void) usr().mainwnd_rows(rows);

        int cols = get_integer(file, tag, "mainwnd-columns");
        (void) usr().mainwnd_cols(cols);

        int scratch = get_integer(file, tag, "mainwnd-spacing");
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
        flag = get_boolean
        (
            file, tag, "enable-learn-confirmation", 0, true
        );
        usr().enable_learn_confirmation(flag);
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
     * [user-randomization]
     */

    tag = "[user-randomization]";

    int randvalue = get_integer(file, tag, "jitter-divisor");
    if (configfile::is_missing(randvalue))
    {
        rc().auto_usr_save(true);
    }
    else
    {
        usr().jitter_divisor(randvalue);
        randvalue = get_integer(file, tag, "amplitude");
        usr().randomization_amount(randvalue);
    }

    /*
     * [user-midi-settings]
     */

    tag = "[user-midi-settings]";
    if (file_version_number() < s_usr_smf_1)
    {
        (void) version_error_message("usr", file_version_number());
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

    std::string fname = get_variable(file, tag, "log");
    if (fname == "none")
        fname.clear();

    bool gotlog = ! fname.empty();
    if (gotlog)
        fname = strip_quotes(fname);

    usr().option_logfile(fname);
    usr().option_use_logfile(gotlog);
    fname = get_variable(file, tag, "pdf-viewer");
    if (fname.empty())
    {
        /*
         * See above.  rc().auto_usr_save(true);
         */
    }
    else
    {
        fname = strip_quotes(fname);
        usr().user_pdf_viewer(fname);
    }
    fname = get_variable(file, tag, "browser");
    if (fname.empty())
    {
        /*
         * See above.  rc().auto_usr_save(true);
         */
    }
    else
    {
        fname = strip_quotes(fname);
        usr().user_browser(fname);
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
            (void) next_data_line(file);
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
        flag = get_boolean(file, tag, "progress-box-shown");
        usr().progress_box_shown(flag);
        v = get_integer(file, tag, "progress-note-min");

        int x = get_integer(file, tag, "progress-note-max");
        usr().progress_note_min_max(v, x);
        flag = get_boolean(file, tag, "lock-main-window");
        usr().lock_main_window(flag);
    }
    tag = "[user-session]";

    std::string s = get_variable(file, tag, "session");
    usr().session_manager(s);

    s = get_variable(file, tag, "url");
    usr().session_url(strip_quotes(s));
    flag = get_boolean(file, tag, "visibility", 0, true);
    usr().session_visibility(flag);

    tag = "[new-pattern-editor]";
    flag = get_boolean(file, tag, "armed");
    usr().new_pattern_armed(flag);
    flag = get_boolean(file, tag, "thru");
    usr().new_pattern_thru(flag);
    flag = get_boolean(file, tag, "record");
    usr().new_pattern_record(flag);
    flag = get_boolean(file, tag, "qrecord");
    usr().new_pattern_qrecord(flag);
    s = get_variable(file, tag, "record-style");
    usr().new_pattern_record_style(s);
    flag = get_boolean(file, tag, "wrap-around");
    usr().new_pattern_wraparound(flag);

    /*
     * We have all of the data.  Close the file.
     */

    dump_setting_summary();
    file.close();                       /* End Of File, EOF, done! */
    return true;
}

/**
 *  Parses a "usr" file, but only for options important to start
 *  the daemonization process.
 *
 * \param [out] startdaemon
 *      Set to true if the function succeed and [user-options] daemonize is
 *      true.  Set to false otherwize, so it can still be checked.
 *
 * \param [out] logfile
 *      Set to the log-file specified in [user-options].  Set to empty
 *      if parsing failed.
 *
 * \return
 *      Returns true if the parsing succeeded.
 */

bool
usrfile::parse_daemonization (bool & startdaemon, std::string & logfile)
{
    std::ifstream file(name().c_str(), std::ios::in | std::ios::ate);
    bool result = set_up_ifstream(file);    /* verifies [Seq66]: version    */
    if (result)
    {
        std::string tag = "[user-options]";
        bool flag = get_boolean(file, tag, "daemonize");
        startdaemon = flag;                 /* set this side-effect         */
        usr().option_daemonize(flag);       /* set the 'usr' flag as well   */

        std::string fname = get_variable(file, tag, "log");
        bool gotlog = ! fname.empty();
        if (gotlog)
            fname = strip_quotes(fname);    /* set this side-effect         */

        usr().option_logfile(fname);        /* set the 'usr' flag as well   */
        usr().option_use_logfile(gotlog);   /* an easy flag to use, man!    */
    }
    else
    {
        result = false;
        startdaemon = false;
        logfile = "";
    }
    return result;
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
"# 'usr' file. Edit it and place it in ~/.config/seq66. It allows naming each\n"
"# MIDI bus/port, channel, and control code.\n"
        ;

    write_seq66_header(file, "usr", version());
    write_comment(file, usr().comments_block().text());
    file <<
"\n"
"# [user-midi-bus-definitions]\n"
"#\n"
"# 1. Define instruments and their control-code names, as applicable.\n"
"# 2. Define MIDI busses, names, and the instruments on each channel.\n"
"#\n"
"# Channels are counted from 0-15, not 1-16. Instruments not set here are set\n"
"# to -1 and are GM (General MIDI). These labels are shown in MIDI Clocks,\n"
"# Inputs, the pattern editor buss, channel, and event drop-downs. To disable\n"
"# entries, set counts to 0.\n"
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
"# In these MIDI instrument definitions, active (supported by the instrument)\n"
"# controller numbers are paired with the (optional) name of the controller.\n"
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
"# Configures some user-interface elements.  Obsolete ones were removed in\n"
"# version 5 of this file. Also see [user-ui-tweaks]. The grid holds Qt push-\n"
"# buttons. For styling, use Qt themes/style-sheets.\n"
"#\n"
"# 'swap-coordinates' swaps numbering so pattern numbers vary fastest by column\n"
"# instead of rows. This setting applies to the live grid, mute-group buttons,\n"
"# and set-buttons.\n"
"#\n"
"# 'mainwnd-rows' and 'mainwnd-columns' (option '-o sets=RxC') specify\n"
"# rows/columns in the main grid. R ranges from 4 to 8, C from 4 to 12.\n"
"# Values other than 4x8 have not been tested thoroughly.\n"
"#\n"
"# 'mainwnd-spacing' is for grid buttons; from 0 to 16, default = 2.\n"
"#\n"
"# 'default-zoom' is the initial zoom for piano rolls. From 1 to 512, default\n"
"# = 2. Larger PPQNs require larger zoom to look good. Seq66 adapts the zoom to\n"
"# the PPQN if set to 0. The unit of zoom is ticks/pixel.\n"
"#\n"
"# 'global-seq-feature' applies the key, scale, and background pattern to all\n"
"# patterns versus separately to each.  If all, these values are stored in the\n"
"# MIDI file in the global SeqSpec versus in each track.\n"
"#\n"
"# 'progress-bar-thick specifies a thicker progress bar.  Default is 1 pixel;\n"
"# thick is 2 pixels if set to true. Also makes the progress box border\n"
"# border 2 pixels, and the slot font bold.\n"
"#\n"
"# 'inverse-colors' (option -K/--inverse) specifies use of an inverse color\n"
"# palette. Palettes are for Seq66 drawing areas, not for Qt widgets.\n"
"# Normal/inverse palettes can be reconfigured via a 'palette' file.\n"
"#\n"
"# 'dark-theme' specifies that are dark theme is active.\n"
"#\n"
"# 'window-redraw-rate' specifies the base window redraw rate for all windows.\n"
"# From 10 to 100; default = 40 ms (25 ms for Windows).\n"
"#\n"
"# Window-scale (option '-o scale=m.n[xp.q]') specifies scaling the main\n"
"# window at startup. Defaults to 1.0 x 1.0. If between 0.5 and 3.0, it\n"
"# changes the size of the main window proportionately.\n"
"#\n"
"# 'enable-learn-confirmation' can be set to false to disable the prompt that\n"
"# the mute-group learn action succeeded. Can be annoying.\n"
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
    write_boolean
    (
        file, "enable-learn-confirmation", usr().enable_learn_confirmation()
    );

    /*
     * [user-midi-ppqn]
     */

    file << "\n"
"# Seq66 separates file PPQN from the Seq66 PPQN. 'default-ppqn' specifies the\n"
"# Seq66 PPQN, from 32 to 19200, default = 192. 'use-file-ppqn' (recommended)\n"
"# indicates to use file PPQN.\n"
"\n[user-midi-ppqn]\n\n"
    ;
    write_integer(file, "default-ppqn", usr().default_ppqn());
    write_boolean(file, "use-file-ppqn", usr().use_file_ppqn());

    /*
     * [user-randomization]
     */

    file << "\n"
"# This section specifies the default values to use to jitter the MIDI event\n"
"# time-stamps and randomize event amplitudes (e.g. velocity for notes). The\n"
"# range of jitter is 1/j times the current snap value.\n"
"\n[user-randomization]\n\n"
    ;
    write_integer(file, "jitter-divisor", usr().jitter_divisor());
    write_integer(file, "amplitude", usr().randomization_amount());

    /*
     * [user-midi-settings]
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
"# 'convert-to-smf-1' controls if SMF 0 files are split into SMF 1 when read.\n"
"# 'buss-override' sets the output port for all patterns, for testing, etc.\n"
"# This value will be saved if you save the MIDI file!!!\n"
"# 'velocity-override' controls adding notes in the pattern editor; see the\n"
"# 'Vol' button. -1 ('Free'), preserves incoming velocity.\n"
"# 'bpm-precision' (spinner and MIDI control) is 0, 1, or 2.\n"
"# 'bpm-step-increment' affects the spinner and MIDI control. For 1 decimal,\n"
"# 0.1 is good. For 2, 0.01 is good, 0.05 is faster. Set 'bpm-page-increment'\n"
"# larger than the step-increment; used with the Page-Up/Page-Down keys in the\n"
"# spinner. BPM minimum/maximum sets the range in tempo graphing; defaults to\n"
"# 0.0 to 127.0. Decrease it for a magnified view of tempo.\n"
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
"# These settings specify some -o or --option switch values.  'daemonize' in\n"
"# seq66cli indicates that it should run as a service. 'log' specifies a log-\n"
"# file redirecting output from standard output/error.  If no path in the name,\n"
"# the log is stored in the configuration directory. For no log-file, use\n"
"# \"none\" or \"\".  On the command line: '-o log=filename.log'.\n"
"\n[user-options]\n\n"
        ;

    std::string fname = usr().option_logfile();
    if (fname.empty())
        fname = "none";

    write_boolean(file, "daemonize", usr().option_daemonize());
    write_string(file, "log", usr().option_logfile(), true);
    write_string(file, "pdf-viewer", usr().user_pdf_viewer(), true);
    write_string(file, "browser", usr().user_browser(), true);

    /*
     * [user-ui-tweaks]
     */

    file << "\n"
"# [user-ui-tweaks]\n"
"#\n"
"# key-height specifies the initial height (before vertical zoom) of pattern\n"
"# editor keys.  Defaults to 10 pixels, ranges from 6 to 32.\n"
"#\n"
"# key-view specifies the default for showing labels for each key:\n"
"# 'octave-letters' (default), 'even_letters', 'all-letters',\n"
"# 'even-numbers', and 'all-numbers'.\n"
"#\n"
"# note-resume causes notes-in-progress to resume when the pattern toggles on.\n"
"#\n"
"# If specified, a style-sheet (e.g. 'qseq66.qss') is applied at startup.\n"
"# Normally just a base-name, it can contain a file-path to provide a style\n"
"# usable in many other applications.\n"
"#\n"
"# A fingerprint is a condensation of note events in a long track, to reduce\n"
"# the time drawing the pattern in the buttons. Ranges from 32 (default) to\n"
"# 128. 0 = don't use a fingerprint.\n"
"#\n"
"# progress-box-width and -height settings change the scaled size of the\n"
"# progress box in the live-grid buttons.  Width ranges from 0.50 to 1.0, and\n"
"# the height from 0.10 to 1.0.  If either is 'default', defaults (0.8 x 0.3)\n"
"# are used.  progress-box-shown controls if the boxes are shown at all.\n"
"#\n"
"# progress-note-min and progress-note-max set the progress-box note range so\n"
"# that notes aren't centered in the box, but shown at their position by pitch.\n"
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
    if (usr().progress_box_width() <= 0.0)
        file << "progress-box-width = default\n";
    else
        write_float(file, "progress-box-width", usr().progress_box_width());

    if (usr().progress_box_height() <= 0.0)
        file << "progress-box-height = default\n";
    else
        write_float(file, "progress-box-height", usr().progress_box_height());

    write_boolean(file, "progress-box-shown", usr().progress_box_shown());
    write_integer(file, "progress-note-min", usr().progress_note_min());
    write_integer(file, "progress-note-max", usr().progress_note_max());
    write_boolean(file, "lock-main-window", usr().lock_main_window());

    /*
     * [user-session]
     */

    file <<
"\n# [user-session]\n"
"#\n"
"# The session manager to use, if any. 'session' is 'none' (default), 'nsm'\n"
"# (Non/New Session Manager), or 'jack'. 'url' can be set to the value set by\n"
"# nsmd when run by command-line. Set 'url' if running nsmd stand-alone; use\n"
"# the --osc-port number. Seq66 detects if started in NSM. The visibility flag\n"
"# is used only by NSM to restore visibility.\n"
"\n[user-session]\n\n"
        ;
    write_string(file, "session", usr().session_manager_name());
    write_string(file, "url", usr().session_url(), true);
    write_boolean(file, "visibility", usr().session_visibility());

    /*
     * [new-pattern-editor]
     */

    file <<
"\n# [new-pattern-editor]\n"
"#\n"
"# Values for play/recording when a new pattern is created. A new pattern\n"
"# is indicated when the loop has the name 'Untitled' and no events. These\n"
"# values save time during a live recording session. The valid values for\n"
"# record-style are 'merge' (default), 'overwrite', 'expand', 'one-shot', \n"
"# and 'one-shot-reset'.\n"
"# 'wrap-around', if true, allows recorded notes to wrap around to the\n"
"# pattern start.\n"
"\n[new-pattern-editor]\n\n"
        ;
    write_boolean(file, "armed", usr().new_pattern_armed());
    write_boolean(file, "thru", usr().new_pattern_thru());
    write_boolean(file, "record", usr().new_pattern_record());
    write_boolean(file, "qrecord", usr().new_pattern_qrecord());
    write_string(file, "record-style", usr().new_pattern_record_string());
    write_boolean(file, "wrap-around", usr().new_pattern_wraparound());
    write_seq66_footer(file);
    file.close();
    return true;
}

}           // namespace seq66

/*
 * usrfile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


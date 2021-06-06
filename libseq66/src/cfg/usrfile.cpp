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
 * \file          usrfile.cpp
 *
 *  This module declares/defines the base class for managing the user's
 *  <code> ~/.seq66usr </code> or <code> ~/.config/seq66.rc
 *  </code> configuration file.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-23
 * \updates       2021-06-06
 * \license       GNU GPLv2 or above
 *
 *  Note that the parse function has some code that is not yet enabled.
 *  Also note that, unlike the "rc" settings, these settings have no
 *  user-interface.  One must use a text editor to modify its settings.
 */

#include <iomanip>                      /* std::setw manipulator            */

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

/**
 *  Principal constructor.
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
    version("4");                       /* new version on 2021-05-15    */
}

/**
 *  A rote destructor needed for a derived class.
 */

usrfile::~usrfile ()
{
    // Empty body
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
    if (! file.is_open())
    {
        errprintf("usrfile::parse(): error opening %s\n", name().c_str());
        return false;
    }
    file.seekg(0, std::ios::beg);                       /* seek to start    */
    (void) parse_version(file);

    /*
     * [comments] Header commentary is skipped during parsing.  However, we
     * now try to read an optional comment block.
     */

    std::string comments = parse_comments(file);
    if (! comments.empty())
        usr().comments_block().set(comments);

    comments = parse_version(file);
    if (comments.empty() || file_version_old(file))
        usr().save_user_config(true);

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
    if (line_after(file, "[user-interface-settings]"))
    {
        /*
         * The next two have been removed (2021-05-15) but we still must
         * scan for them, for now.
         */

        sscanf(scanline(), "%d", &scratch); // usr().set_grid_style(scratch)
        (void) next_data_line(file);
        sscanf(scanline(), "%d", &scratch); // usr().grid_brackets(scratch);

        (void) next_data_line(file);
        sscanf(scanline(), "%d", &scratch);
        usr().mainwnd_rows(scratch);

        (void) next_data_line(file);
        sscanf(scanline(), "%d", &scratch);
        usr().mainwnd_cols(scratch);

        (void) next_data_line(file);
        sscanf(scanline(), "%d", &scratch);
        usr().max_sets(scratch);                /* this setting deprecated  */

        (void) next_data_line(file);
        sscanf(scanline(), "%d", &scratch);
        usr().mainwid_border(scratch);

        (void) next_data_line(file);
        sscanf(scanline(), "%d", &scratch);
        usr().mainwid_spacing(scratch);

        (void) next_data_line(file);
        sscanf(scanline(), "%d", &scratch);
        usr().control_height(scratch);

        (void) next_data_line(file);
        sscanf(scanline(), "%d", &scratch);
        usr().zoom(scratch);

        /*
         * This boolean affects the behavior of the scale, key, and
         * background sequence features, but their actual values are
         * stored in the MIDI file, not in the "user" configuration file.
         */

        (void) next_data_line(file);
        sscanf(scanline(), "%d", &scratch);
        usr().global_seq_feature(scratch != 0);

        /*
         * The user-interface font is now selectable at run time.  Old
         * versus new font.
         */

        (void) next_data_line(file);
        sscanf(scanline(), "%d", &scratch);
        usr().use_new_font(scratch != 0);

        (void) next_data_line(file);
        sscanf(scanline(), "%d", &scratch);
        usr().allow_two_perfedits(scratch != 0);

        if (next_data_line(file))
        {
            sscanf(scanline(), "%d", &scratch);
            usr().perf_h_page_increment(scratch);
        }

        if (next_data_line(file))
        {
            sscanf(scanline(), "%d", &scratch);
            usr().perf_v_page_increment(scratch);
        }

        /*
         *  Here, we start checking the lines, on the theory that these
         *  very new (2016-02-14) items might mess up people who already
         *  have older Seq66 "user" configuration files.
         */

        if (next_data_line(file))
        {
            sscanf(scanline(), "%d", &scratch);             /* now an int   */
            usr().progress_bar_colored(scratch);        /* pick a color */
            if (next_data_line(file))
            {
                sscanf(scanline(), "%d", &scratch);
                usr().progress_bar_thick(scratch != 0);
            }
            if (next_data_line(file))
            {
                sscanf(scanline(), "%d", &scratch);
                if (scratch <= 1)                       /* boolean?     */
                {
                    usr().inverse_colors(scratch != 0);
                    if (next_data_line(file))
                        sscanf(scanline(), "%d", &scratch); /* get redraw   */
                }
                if (scratch < SEQ66_MINIMUM_REDRAW)
                    scratch = SEQ66_MINIMUM_REDRAW;
                else if (scratch > SEQ66_MAXIMUM_REDRAW)
                    scratch = SEQ66_MAXIMUM_REDRAW;

                usr().window_redraw_rate(scratch);
            }
            if (next_data_line(file))
            {
                sscanf(scanline(), "%d", &scratch);
                if (scratch <= 1)                       /* boolean?     */
                    usr().use_more_icons(scratch != 0);
            }
            if (next_data_line(file))
            {
                sscanf(scanline(), "%d", &scratch);
                if (scratch > 0)
                    usr().block_rows(scratch);
            }
            if (next_data_line(file))
            {
                sscanf(scanline(), "%d", &scratch);
                if (scratch > 0)
                    usr().block_columns(scratch);
            }
            if (next_data_line(file))
            {
                sscanf(scanline(), "%d", &scratch);
                usr().block_independent(scratch != 0);
            }
            if (next_data_line(file))
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
    usr().normalize();    /* calculate derived values */

    /*
     * [user-midi-ppqn]
     */

    bool ppqn_settings_made = false;
    std::string s = get_variable(file, "[user-midi-ppqn]", "default-ppqn");
    if (! s.empty())
    {
        usr().default_ppqn(std::stoi(s));
#if 0
        s = get_variable(file, "[user-midi-ppqn]", "use-file-ppqn");
        if (! s.empty())
        {
            bool use_it = string_to_bool(s);
            usr().use_file_ppqn(use_it);
            ppqn_settings_made = true;
        }
#endif
        bool flag = get_boolean(file, "[user-midi-ppqn]", "use-file-ppqn");
        usr().use_file_ppqn(flag);
        ppqn_settings_made = true;
    }

    /*
     * [user-midi-settings]
     */

    if (line_after(file, "[user-midi-settings]"))
    {
        int scratch = 0;
        sscanf(scanline(), "%d", &scratch);
        if (ppqn_settings_made)
            scratch = usr().default_ppqn();

        usr().midi_ppqn(scratch);
        next_data_line(file);
        sscanf(scanline(), "%d", &scratch);
        usr().midi_beats_per_bar(scratch);

        float beatspm;
        next_data_line(file);
        sscanf(scanline(), "%f", &beatspm);
        usr().midi_beats_per_minute(midibpm(beatspm));

        next_data_line(file);
        sscanf(scanline(), "%d", &scratch);
        usr().midi_beat_width(scratch);

        next_data_line(file);
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

    /*
     * -o special options support.
     */

    if (line_after(file, "[user-options]"))
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

    /*
     * Work-arounds for sticky issues
     */

    if (line_after(file, "[user-work-arounds]"))
    {
        int scratch = 0;
        sscanf(scanline(), "%d", &scratch);
        usr().work_around_play_image(scratch != 0);
        if (next_data_line(file))
        {
            sscanf(scanline(), "%d", &scratch);
            usr().work_around_transpose_image(scratch != 0);
        }
    }

    /*
     * [user-ui-tweaks].  The variables in this section are, in this order:
     * key-height and use-new-seqedit, which are currently not supporting the
     * new DOS-INI variable setting feature supported by get_variable().
     * The note-resume option is now implemented as per issue #5.
     */

    if (line_after(file, "[user-ui-tweaks]"))
    {
        int scratch = 0;
        int count = sscanf(scanline(), "%d", &scratch);
        if (count == 1)
        {
            usr().save_user_config(true);
            usr().key_height(scratch);
            if (next_data_line(file))
            {
                sscanf(scanline(), "%d", &scratch);

                /*
                 * usr().use_new_seqedit(scratch != 0);
                 */

                warnprintf("use-new-seqedit = %d now always true\n", scratch);
            }
        }
        else
        {
            std::string s = get_variable(file, "[user-ui-tweaks]", "key-height");
            usr().key_height(string_to_int(s, 10));

            /*
             * Disabled 2021-05-07
             *
             *  s = get_variable(file, "[user-ui-tweaks]", "use-new-seqedit");
             *  usr().use_new_seqedit(string_to_bool(s));
             */
        }

        bool flag = get_boolean(file, "[user-ui-tweaks]", "note-resume");
        usr().resume_note_ons(flag);

        std::string s = get_variable(file, "[user-ui-tweaks]", "style-sheet");
        usr().style_sheet(strip_quotes(s));
        s = get_variable(file, "[user-ui-tweaks]", "fingerprint-size");
        usr().fingerprint_size(string_to_int(s, 32));
        s = get_variable(file, "[user-ui-tweaks]", "progress-box-width");

        double w = string_to_double(s, -1.0);
        s = get_variable(file, "[user-ui-tweaks]", "progress-box-height");

        double h = string_to_double(s, -1.0);
        usr().progress_box_size(w, h);
    }
    s = get_variable(file, "[user-session]", "session");
    usr().session_manager(s);

    s = get_variable(file, "[user-session]", "url");
    usr().session_url(s);

#if 0
    s = get_variable(file, "[new-pattern-editor]", "armed");
    usr().new_pattern_armed(string_to_bool(s));

    s = get_variable(file, "[new-pattern-editor]", "thru");
    usr().new_pattern_thru(string_to_bool(s));

    s = get_variable(file, "[new-pattern-editor]", "record");
    usr().new_pattern_record(string_to_bool(s));

    s = get_variable(file, "[new-pattern-editor]", "qrecord");
    usr().new_pattern_qrecord(string_to_bool(s));
#endif
    bool flag = get_boolean(file, "[new-pattern-editor]", "armed");
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
        "# 1. Define your instruments and their control-code names,\n"
        "#    if they have them.\n"
        "# 2. Define a MIDI bus, its name, and what instruments are\n"
        "#    on which channel.\n"
        "#\n"
        "# In the following MIDI buss definitions, channels are counted\n"
        "# from 0 to 15, not 1 to 16.  Instruments not set here are set to\n"
        "# -1 (SEQ66_GM_INSTRUMENT_FLAG) and are GM (General MIDI).\n"
        "# These replacement MIDI buss labels are shown in MIDI Clocks,\n"
        "# MIDI Inputs, and in the Pattern Editor buss drop-down.\n"
        "#\n"
        "# To temporarily disable the entries, set the count values to 0.\n"
        ;

    /*
     * [user-midi-bus-definitions]
     */

    file
        << "\n[user-midi-bus-definitions]\n\n"
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
        "# ======== Seq66-Specific Variables Section ========\n"
        "\n"
        "[user-interface-settings]\n"
        "\n"
        "# These settings specify the modifiable configuration\n"
        "# of some of the Seq66 user-interface elements.\n"
        ;

    file << "\n"
        "# The main-window pattern grid is now always made using push-buttons.\n"
        "# To use the old flat style, using Qt themes and style-sheets.\n"
        "\n"
        "0       # grid_style\n"
        ;

    file << "\n"
        "# Specified the box style of an empty slot in the main-window\n"
        "# grid, for the GTK user-interface. Removed.\n"
        "\n"
        "0       # grid_brackets\n"
        ;

    file << "\n"
        "# Specifies the number of rows in the main window.\n"
        "# Values of 4 (the default) through 8 (the best alternative value)\n"
        "# are allowed. Same as R in the '-o sets=RxC' option.\n"
        "\n"
        << usr().mainwnd_rows() << "       # mainwnd_rows\n"
        ;

    file << "\n"
        "# Specifies the number of columns in the main window.\n"
        "# At present, values from 8 (the default) to 12 are supported.\n"
        "# are allowed. Same as C in the '-o sets=RxC' option.\n"
        "\n"
        << usr().mainwnd_cols() << "       # mainwnd_cols\n"
        ;

    file << "\n"
        "# Specifies the maximum number of sets, which defaults to 32. It is\n"
        "# never necessary to change this value. It is based on set-size now.\n"
        "# Do not change it. It is just informative.\n"
        "\n"
        << usr().max_sets() << "      # max_sets\n"
        ;

    file << "\n"
        "# Specifies the border width in the main window.\n"
        "\n"
        << usr().mainwid_border() << "      # mainwid_border\n"
        ;

    file << "\n"
        "# Specifies the border spacing in the main window. Normally 2, it\n"
        "# can range from 2 to 16.\n"
        "\n"
        << usr().mainwid_spacing() << "      # mainwid_spacing\n"
        ;

    file << "\n"
        "# Specifies a quantity that affects the height of the main window.\n"
        "\n"
        << usr().control_height() << "      # control_height\n"
        ;

    file << "\n"
        "# Specifies the initial zoom for the piano rolls.  Ranges from 1 to\n"
        "# 512, and defaults to 2 unless changed here. Larger PPQNs require\n"
        "# larger zoom values in order to look good in the sequence editor.\n"
        "# Seq66 adapts the zoom to the PPQN, if zoom is set to 0.\n"
        "\n"
        << usr().zoom() << "      # default zoom (0 = auto-adjust to PPQN)\n"
        ;

    /*
     * This boolean affects the behavior of the scale, key, and background
     * sequence features.
     */

    file << "\n"
        "# Specifies if the key, scale, and background sequence are to be\n"
        "# applied to all sequences, or to individual sequences.  The\n"
        "# behavior of Seq24 was to apply them to all sequences.  But\n"
        "# Seq66 takes it further by applying it immediately, and\n"
        "# by saving to the end of the MIDI file.  Note that these three\n"
        "# values are stored in the MIDI file, not this configuration file.\n"
        "# Also note that reading MIDI files not created with this feature\n"
        "# will pick up this feature if active, and the file gets saved.\n"
        "# It is contagious.\n"
        ;

    file << "#\n"
        "# 0 = Allow each sequence to have its own key/scale/background.\n"
        "#     Settings are saved with each sequence.\n"
        "# 1 = Apply these settings globally (similar to seq66).\n"
        "#     Settings are saved in the global final section of the file.\n"
        "\n"
        << (usr().global_seq_feature() ? "1" : "0")
        << "      # global_seq_feature\n"
        ;

    /*
     * The usage of the old versus new font is now a run-time option.
     */

    file << "\n"
        "# Specifies if the old, console-style font, or the new anti-\n"
        "# aliased font, is to be used as the font throughout the GUI.\n"
        "# In legacy mode, the old font is the default.\n"
        "#\n"
        "# 0 = Use the old-style font.\n"
        "# 1 = Use the new-style font.\n"
        "\n"
        << (usr().use_new_font() ? "1" : "0")
        << "      # use_new_font\n"
        ;

    file << "\n"
        "# Specifies if the user-interface will support two song editor\n"
        "# windows being shown at the same time.  This makes it easier to\n"
        "# edit songs with a large number of sequences.\n"
        "#\n"
        "# 0 = Allow only one song editor (performer editor).\n"
        "# 1 = Allow two song editors.\n"
        "\n"
        << (usr().allow_two_perfedits() ? "1" : "0")
        << "      # allow_two_perfedits\n"
        ;

    file << "\n"
        "# Specifies the number of 4-measure blocks for horizontal page\n"
        "# scrolling in the song editor.  The old default, 1, is a bit\n"
        "# small.  The new default is 4.  The legal range is 1 to 6, where\n"
        "# 6 is the width of the whole performer piano roll view.\n"
        "\n"
        << usr().perf_h_page_increment()
        << "      # perf_h_page_increment\n"
        ;

    file << "\n"
        "# Specifies the number of 1-track blocks for vertical page\n"
        "# scrolling in the song editor.  The old default, 1, is a bit\n"
        "# small.  The new default is 8.  The legal range is 1 to 18, where\n"
        "# 18 is about the height of the whole performer piano roll view.\n"
        "\n"
        << usr().perf_v_page_increment()
        << "      # perf_v_page_increment\n"
        ;

    file << "\n"
        "# Specifies if the progress bar is colored black, or a different\n"
        "# color.  The following integer color values are supported:\n"
        "# \n"
        "# 0 = black\n"
        "# 1 = dark red\n"
        "# 2 = dark green\n"
        "# 3 = dark orange\n"
        "# 4 = dark blue\n"
        "# 5 = dark magenta\n"
        "# 6 = dark cyan\n"
        "\n"
        << usr().progress_bar_colored() // (usr().progress_bar_colored() ? "1" : "0")
        << "      # progress_bar_colored\n"
        ;

    file << "\n"
        "# Specifies if the progress bar is thicker.  The default is 1\n"
        "# pixel.  The 'thick' value is 2 pixels.  (More than that is not\n"
        "# useful.  Set this value to 1 to enable the feature, 0 to disable\n"
        "# it.\n"
        "\n"
        << (usr().progress_bar_thick() ? "1" : "0")
        << "      # progress_bar_thick\n"
        ;

    file << "\n"
        "# Specifies using an alternate (darker) color palette.  The\n"
        "# default is the normal palette.  Not all items in the user\n"
        "# interface are altered by this setting, and it's not perfect.\n"
        "# Set this value to 1 to enable the feature, 0 to disable it.\n"
        "# Same as the -K or --inverse command-line options.\n"
        "\n"
        << (usr().inverse_colors() ? "1" : "0")
        << "      # inverse_colors\n"
        ;

    file << "\n"
        "# Specifies the window redraw rate for all windows that support\n"
        "# that concept.  The default is 40 ms.  Some windows used 25 ms,\n"
        "# which is faster.\n"
        "\n"
        << usr().window_redraw_rate()
        << "      # window_redraw_rate\n"
        ;

    file << "\n"
        "# Specifies using icons for some of the user-interface buttons\n"
        "# instead of text buttons.  This is purely a preference setting.\n"
        "# If 0, text is used in some buttons (the main window buttons).\n"
        "# Otherwise, icons are used.  One will have to experiment :-).\n"
        "\n"
        << usr().use_more_icons()
        << "      # use_more_icons (currently affects only main window)\n"
        ;


    file << "\n"
        "# Specifies the number of set-window ('wid') rows to show.\n"
        "# The long-standing default is 1, but 2 or 3 may also be set.\n"
        "# Corresponds to R in the '-o wid=RxC,F' option.\n"
#if ! defined SEQ66_MULTI_MAINWID
        "# Support for this option is obsolete in Seq66.\n"
#endif
        "\n"
        << usr().block_rows()
        << "      # block_rows (number of rows of set blocks/wids)\n"
        ;

    file << "\n"
        "# Specifies the number of set window ('wid') columns to show.\n"
        "# The long-standing default is 1, but 2 may also be set.\n"
        "# Corresponds to C in the '-o wid=RxC,F' option.\n"
#if ! defined SEQ66_MULTI_MAINWID
        "# Support for this option is obsolete in Seq66.\n"
#endif
        "\n"
        << usr().block_columns()
        << "      # block_columns (number of columns of set blocks/wids)\n"
        ;

    file << "\n"
        "# Specifies if the multiple set windows are 'in sync' or can\n"
        "# be set to arbitrary set numbers independently.\n"
        "# The default is false (0), means that there is a single set\n"
        "# spinner, which controls the set number of the upper-left 'wid',\n"
        "# and the rest of the set numbers follow sequentially.  If true\n"
        "# (1), then each 'wid' can be set to any set-number.\n"
        "# Corresponds to the 'f' (true, false, or 'indep') in the\n"
        "# '-o wid=RxC,F' option.  Here, 1 is the same as 'indep' or false,\n"
        "# and 0 is the same as f = true.  Backwards, so be careful.\n"
#if ! defined SEQ66_MULTI_MAINWID
        "# Support for this option is obsolete in Seq66.\n"
#endif
        "\n"
        << (usr().block_independent() ? "1" : "0")
        << "      # block_independent (set spinners for each block/wid)\n"
        ;

    file << "\n"
        "# Specifies scaling the main window of Seq66 at startup. The norm is\n"
        "# 1.0x1.0.  If this value is between 0.8 and 3.0, it changes the size\n"
        "# of the main window proportionately. Option '-o scale=m.n[xp.q]'.\n"
        "\n"
        << usr().window_scale() << " " << usr().window_scale_y()
        << "      # window_scale (scales main window width x height)\n"
        ;

    /*
     * [user-midi-ppqn]
     */

    file << "\n"
        "[user-midi-ppqn]\n"
        "\n"
        "# These settings replace the midi_ppqn setting below.  Seq66 separates\n"
        "# the file PPQN from the default PPQN that the user wants to use.\n"
        "#\n"
        "# default-ppqn specifies the PPQN to use by default. The classic\n"
        "# default is 192, but can range from 32 to 19200.\n"
        "#\n"
        "# use-file-ppqn, if true, indicates to use the file PPQN. Usually this\n"
        "# is the best setting, to avoid changing the file's PPQN.\n"
        "\n"
        "default-ppqn = " << std::to_string(usr().default_ppqn()) << "\n"
        "use-file-ppqn = " << bool_to_string(usr().use_file_ppqn()) << "\n"
        ;

    /*
     * [user-midi-settings] and [user-options]
     */

    file << "\n"
        "[user-midi-settings]\n"
        "\n"
        "# These settings specify MIDI-specific value that might be\n"
        "# better off as variables, rather than constants.\n"
        "# This value is no longer used.  See above.\n"
        "\n"
        << usr().midi_ppqn() << "       # midi_ppqn, --ppqn p, default PPQN\n"
        ;

    file << "\n"
        "# Specifies the default beats per measure, or beats per bar.\n"
        "# The default value is 4, the range is 1 to 32.\n"
        "\n"
        << usr().midi_beats_per_bar()
        << "       # midi_beats_per_measure/bar\n"
        ;

    file << "\n"
        "# Specifies the default beats per minute.  The default value is 120;\n"
        "# the legal range is 2 to 600. Also see the value of\n"
        "# midi_bpm_minimum and midi_bpm_maximum below.\n"
        "\n"
        << usr().midi_beats_per_minute() << "       # midi_beats_per_minute\n"
        ;

    file << "\n"
        "# Specifies the default beat width. The default value is 4.\n"
        "\n"
        << usr().midi_beat_width() << "       # midi_beat_width\n"
        ;

    file << "\n"
        "# Specifies the buss-number override, the same as --buss. The default\n"
        "# is -1 (no buss override).  If a value from 0 to 31 is given, that\n"
        "# buss value overrides the busses for all patterns, for testing or\n"
        "# convenience.  Do not save the MIDI file afterwards, unless you want\n"
        "# to overwrite all the buss values!\n"
        "\n"
        ;

    int bo = int(usr().midi_buss_override());   /* writing char no good */
    if (is_null_buss(bussbyte(bo)))
        file << "-1" << "       # midi_buss_override (disabled)\n";
    else
        file << bo   << "       # midi_buss_override (enabled, careful!)\n";

    file << "\n"
        "# Specifies the default velocity override when adding notes in the\n"
        "# sequence/pattern editor.  This value is obtained via the 'Vol'\n"
        "# button, and ranges from 0 (not recommended :-) to 127.  If the\n"
        "# value is -1, then the incoming note velocity is preserved.\n"
        "\n"
        ;

    int vel = usr().velocity_override();
    file << vel      << "       # velocity_override (-1 = 'Free')\n";

    file << "\n"
        "# Specifies the precision of the beats-per-minutes spinner and MIDI\n"
        "# control of the BPM value.  The default is 0 (BPM is an integer).\n"
        "# Other values are 1 and 2 decimal digits of precision.\n"
        "\n"
        ;

    int precision = usr().bpm_precision();
    file << precision << "       # bpm_precision\n";

    file << "\n"
        "# Specifies the step increment of the beats/minute spinner and MIDI\n"
        "# control over the BPM value.  The default is 1. For 1 decimal point,\n"
        "# 0.1 is a good value.  For a precision of 2 decimal points, 0.01 is\n"
        "# a good value, but one might want something faster, like 0.05.\n"
        "\n"
        ;

    midibpm increment = usr().bpm_step_increment();
    file << increment << "       # bpm_step_increment\n";

    file << "\n"
        "# Specifies the page increment of the beats/minute field. It is\n"
        "# used when the Page-Up/Page-Down keys are pressed while the BPM\n"
        "# field has the keyboard focus.  The default value is 10.\n"
        "\n"
        ;

    increment = usr().bpm_page_increment();
    file << increment << "       # bpm_page_increment\n";

    file << "\n"
        "# Specifies the minimum value of beats/minute in tempo graphing.\n"
        "# By default, the tempo graph ranges from 0.0 to 127.0.\n"
        "# This value can be increased to give a magnified view of tempo.\n"
        "\n"
        ;

    increment = usr().midi_bpm_minimum();
    file << increment << "       # midi_bpm_minimum\n";

    file << "\n"
        "# Specifies the maximum value of beats/minute in tempo graphing.\n"
        "# By default, the tempo graph ranges from 0.0 to 127.0.\n"
        "# This value can be increased to give a magnified view of tempo.\n"
        "\n"
        ;

    increment = usr().midi_bpm_maximum();
    file << increment << "       # midi_bpm_maximum\n";

    /*
     * [user-options]
     */

    file << "\n"
        "[user-options]\n"
        "\n"
        "# These settings specify application-specific values that are\n"
        "# set via the -o or --option switch, which help expand the number\n"
        "# of options the Seq66 options can support.\n"
        "\n"
        "# The 'daemonize' option is used in seq66cli to indicate that the\n"
        "# application should be gracefully run as a service.\n"
        "\n"
        ;

    int uscratch = usr().option_daemonize() ? 1 : 0 ;
    file << uscratch << "       # option_daemonize\n";
    file << "\n"
        "# This value specifies an optional log-file that replaces output\n"
        "# to standard output and standard error.  To indicate no log-file,\n"
        "# the string \"\" is used.  Currently, this option works best from\n"
        "# the command line, as in '-o log=filename.log'.  The name here is\n"
        "# used only for the no-name '-o log' option.\n"
        "\n"
        ;
    std::string logfile = usr().option_logfile();
    if (logfile.empty())
        file << "\"\"\n";
    else
        file << logfile << "\n";

    /*
     * [user-work-arounds]
     */

    file << "\n"
        "[user-work-arounds]\n"
        "\n"
        "# These settings were application-specific values for issues that\n"
        "# no longer apply.\n"
        "\n"
        ;

    uscratch = usr().work_around_play_image() ? 1 : 0 ;
    file << uscratch << "       # work_around_play_image\n";

    uscratch = usr().work_around_transpose_image() ? 1 : 0 ;
    file << uscratch << "       # work_around_transpose_image\n";

    /*
     * [user-ui-tweaks]
     */

    file << "\n"
        "[user-ui-tweaks]\n"
        "\n"
        "# This first value specifies the height of the keys in the\n"
        "# sequence editor.  Defaults to 12 (pixels), but 8 is better.\n"
        "\n"
        ;

    std::string v = std::to_string(usr().key_height());
    file << "key-height = " << v << "\n";

    file << "\n"
        "# The Qt version of Seq66 now always uses the new pattern editor in\n"
        "# the 'Edit' tab.  So, the 'use-new-seqedit' option is removed.\n"
        "\n"
        ;

    /*
     * Disabled 2021-05-07
     *
     *      v = bool_to_string(usr().use_new_seqedit());
     *      file << "use-new-seqedit = " << v << "\n";
     */

    v = bool_to_string(usr().resume_note_ons());
    file << "\n"
        "# The note-resume option, if active, causes any notes in progress\n"
        "# to be resumed when the pattern is toggled back on.\n\n"
        << "note-resume = " << v << "\n"
        ;

    v = add_quotes(usr().style_sheet());
    file << "\n"
        "# If specified, this style-sheet (e.g. 'qseq66.qss') is applied\n"
        "# at startup.  Although normally just a base-name, it can contain\n"
        "# a file-path, to provide a style usable in many applications,\n"
        "# outside the Seq66 configuration directory.\n\n"
        << "style-sheet = " << v << "\n"
        ;

    file << "\n"
        "# If specified, the fingerprint size is adjusted to this value.  The\n"
        "# fingerprint is a condensation of the note events in a long track,\n"
        "# which reduces the amount of drawing in the grid buttons. Ranges\n"
        "# from 32 (the default) to 128.\n\n"
        << "fingerprint-size = " << usr().fingerprint_size() << "\n"
        ;

    file << "\n"
        "# If specified, changes the default size of the progress box in the\n"
        "# live-loop grid buttons.  The numbers are in fractions.  The width\n"
        "# ranges from 0.50 to 1.0; the height from 0.10 to 0.50.  If either\n"
        "# is 0, then the box isn't drawn.  If either is 'default', defaults\n"
        "# are used.\n\n"
        ;
    if (usr().progress_box_width() < 0.0)
        file << "progress-box-width = default\n";
    else
        file << "progress-box-width = " << usr().progress_box_width() << "\n";

    if (usr().progress_box_height() < 0.0)
        file << "progress-box-height = default\n";
    else
        file << "progress-box-height = " << usr().progress_box_height() << "\n";

    /*
     * [user-session]
     */

    v = usr().session_url();
    if (v.empty())
        v = double_quotes();

    file <<
        "\n[user-session]\n\n"
        "# This section specifies the session manager to use, if any.  It\n"
        "# contains only one variable, 'session', which can be set to 'none'\n"
        "# (the default), 'nsm' (Non or New Session Manager), or 'lash' (the\n"
        "# LASH session manager.  The 'url' variable can be set to the value\n"
        "# of the NSM_URL environment variable set by nsmd when run outside\n"
        "# of the Non Session Manager user-interface. Set the URL only if\n"
        "# running nsmd standalone with a matching --osc-port number.\n\n"
        << "session = " << usr().session_manager_name() << "\n"
        << "url = " << v << "\n"
        ;

    /*
     * [new-pattern-editor]
     */

    file <<
        "\n[new-pattern-editor]\n\n"
        "# This section contains the setup values for recording when a new\n"
        "# pattern is opened. For flexibility, a new pattern means only that\n"
        "# the loop has the default name, 'Unititled'. These values save time\n"
        "# during a live recording session. Note that the valid values for\n"
        "# record-style are 'merge', 'overwrite', and 'expand'.\n"
        "\n"
        ;

    v = bool_to_string(usr().new_pattern_armed());
    file << "armed = " << v << "\n";

    v = bool_to_string(usr().new_pattern_thru());
    file << "thru = " << v << "\n";

    v = bool_to_string(usr().new_pattern_record());
    file << "record = " << v << "\n";

    v = bool_to_string(usr().new_pattern_qrecord());
    file << "qrecord = " << v << "\n";

    std::string rs = "merge";
    if (usr().new_pattern_recordstyle() == recordstyle::overwrite)
        rs = "overwrite";
    else if (usr().new_pattern_recordstyle() == recordstyle::expand)
        rs = "expand";

    file << "record-style = " << rs << "\n";

    /*
     * End of file.
     */

    file
        << "\n"
        << "# End of " << name() << "\n#\n"
        << "# vim: sw=4 ts=4 wm=4 et ft=dosini\n"
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


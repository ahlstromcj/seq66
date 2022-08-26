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
 * \file          midicontrolfile.cpp
 *
 *  This module declares/defines the base class for managing the reading and
 *  writing of the midi-control sections of the 'rc' file.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-13
 * \updates       2022-08-26
 * \license       GNU GPLv2 or above
 *
 *  This class handles the 'ctrl' file.
 */

#include <iostream>                     /* std::cout                        */
#include <iomanip>                      /* std::setw()                      */

#include "cfg/midicontrolfile.hpp"      /* seq66::midicontrolfile class     */
#include "cfg/settings.hpp"             /* seq66::rc()                      */
#include "ctrl/keymap.hpp"              /* seq66::qt_keyname_ordinal()      */
#include "play/setmaster.hpp"           /* seq66::setmaster::Size() static  */
#include "util/filefunctions.hpp"       /* seq66::file_exists()             */
#include "util/strfunctions.hpp"        /* seq66::strip_quotes()            */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 * Provides the number of the latest official version of this file.
 *
 *  Version 4: Baseline for this configuration file.
 *  Version 5: Adds 8 more potential midi-control-out entries.
 *  Version 6: Adds a large number of new automation controls. Version
 *             5 files will be renamed (for backup) and replaced.
 */

static const int s_ctrl_file_version = 6;

/*
 * -------------------------------------------------------------------------
 *  key: midicontrolfile nested class
 * -------------------------------------------------------------------------
 */

/**
 *  Constructs the key value from the given midicontrol.
 */

midicontrolfile::key::key (const midicontrol & mc) :
    m_category      (mc.category_code()),
    m_slot_control  (mc.slot_control())
{
    // No code needed
}

/**
 *  The less-than operator for this key class.
 */

bool
midicontrolfile::key::operator < (const key & rhs) const
{
   if (m_category == rhs.m_category)
       return m_slot_control < rhs.m_slot_control;
   else
       return m_category < rhs.m_category;
}

/*
 * -------------------------------------------------------------------------
 *  stanza: midicontrolfile nested class
 * -------------------------------------------------------------------------
 */

/**
 *  Extracts a control-stanza from a MIDI control object.
 */

midicontrolfile::stanza::stanza (const midicontrol & mc) :
    m_category          (mc.category_code()),
    m_key_name          (mc.key_name()),
    m_op_name           (mc.label()),
    m_slot_number       (mc.slot_control()),        /* slot vs control no.  */
    m_settings          ()                          /* 2-dimensional array  */
{
    set(mc);
}

bool
midicontrolfile::stanza::set (const midicontrol & mc)
{
    automation::action a = mc.action_code();
    if (a > automation::action::none && a < automation::action::max)
    {
        int index = static_cast<int>(a) - 1;        /* skips "none"         */
        m_settings[index][0] = int(mc.inverse_active());
        m_settings[index][1] = int(mc.status());
        m_settings[index][2] = int(mc.d0());        /* note: d1 not needed  */
        m_settings[index][3] = int(mc.min_d1());
        m_settings[index][4] = int(mc.max_d1());
    }
    return true;
}

/*
 * -------------------------------------------------------------------------
 *  midicontrolfile
 * -------------------------------------------------------------------------
 */

/**
 *  Static function to create a detailed error message.
 */

bool
midicontrolfile::keycontrol_error_message
(
    const keycontrol & kc,
    ctrlkey ordinal,
    int lineno
)
{
    char temp[256];
    snprintf
    (
        temp, sizeof temp,
        "Error at line %d ordinal 0x%2x key '%s' control '%s' code %d\n",
        lineno, unsigned(ordinal), kc.key_name().c_str(), kc.name().c_str(),
        kc.control_code()
    );
    return make_error_message("Key controls", temp);
}

/**
 *  Principal constructor.
 *
 * \param mainfilename
 *      Provides the name of the options file; this is usually a full path
 *      file-specification to the "rc"/"ctrl" file using this object.
 *
 * \param rcs
 *      Source/destination for the configuration information.
 */

midicontrolfile::midicontrolfile
(
    const std::string & filename,
    rcsettings & rcs
) :
    configfile              (filename, rcs, ".ctrl"),
    m_temp_key_controls     (),                             /* reading only */
    m_temp_midi_ctrl_in     ("ctrl"),                       /* reading only */
    m_stanzas               ()
{
    version(s_ctrl_file_version);
}

/**
 *  A rote destructor.
 */

midicontrolfile::~midicontrolfile ()
{
    // ~configfile() called automatically
}

/**
 *  Parse the ~/.config/seq66/qseq66.ctrl file-stream.
 *
 *  [comments]
 *
 *      [comments] Header commentary is skipped during parsing.  However, we now
 *      try to read an optional comment block.  This block is part of the MIDI
 *      container object, not part of the rcsettings object.
 *
 *  [midi-control-settings]  (was "midi-control-flags")
 *
\verbatim
        control-buss
        midi-enabled
        button-offset
        button-rows
        button-columns
\endverbatim
 *
 *  [midi-control] and [midi-control-file]
 *
 *      Get the number of sequence definitions provided in the following
 *      section.  Ranges from 32 on up.  Then read in all of the sequence lines.
 *      The first 32 apply to the first screen set.  There can also be a comment
 *      line "# mute in group" followed by 32 more lines.  Then there are
 *      additional comments and single lines for BPM up, BPM down, Screen Set
 *      Up, Screen Set Down, Mod Replace, Mod Snapshot, Mod Queue, Mod Gmute,
 *      Mod Glearn, and Screen Set Play.  These are all forms of MIDI automation
 *      useful to control the playback while not sitting near the computer.
 *
 *  [loop-control]
 *  [mute-group-control]
 *  [automation-control]
 *
 *      Provides the stanzas that define the various controls, both keys and
 *      MIDI controls.
 *
 *      Note that there are no default MIDI controls, but there are default key
 *      controls.  See the keys defined in keycontainer::add_defaults().
 */

bool
midicontrolfile::parse_stream (std::ifstream & file)
{
    bool result = true;
    file.seekg(0, std::ios::beg);                   /* seek to the start    */
    (void) parse_version(file);

    std::string s = parse_comments(file);
    if (! s.empty())
        m_temp_midi_ctrl_in.comments_block().set(s);

    std::string mctag = "[midi-control-settings]";  /* the new name for it  */
    if (bad_position(find_tag(file, mctag)))
        mctag = "[midi-control-flags]";             /* earlier name for it  */

    bool flag = get_boolean(file, mctag, "drop-empty-controls");
    rc_ref().drop_empty_in_controls(flag);

    bussbyte buss = get_buss_number(file, false, mctag, "control-buss");
    bool enabled = get_boolean(file, mctag, "midi-enabled");
    int offset = 0, rows = 0, columns = 0;
    result = parse_control_sizes(file, mctag, offset, rows, columns);
    if (result)
    {
        if (enabled)
            enabled = rc_ref().midi_control_active();

        if (m_temp_midi_ctrl_in.initialize(buss, rows, columns))
        {
            m_temp_midi_ctrl_in.is_enabled(enabled);
            m_temp_midi_ctrl_in.configure_enabled(enabled);
            m_temp_midi_ctrl_in.offset(offset);
            m_temp_midi_ctrl_in.configured_buss(buss);
        }
    }
    else
        enabled = false;

    std::string layout = get_variable(file, mctag, "keyboard-layout");
    m_temp_key_controls.clear();
    m_temp_key_controls.set_kbd_layout(layout);

    bool good = line_after(file, "[loop-control]");
    int count = 0;

    /*
     * Not important enough to cause concern.
     *
     * bool keep_empties = ! rc_ref().drop_empty_in_controls();
     * if (keep_empties)
     *     status_message("Keeping empty MIDI-In controls");
     */

    while (good)                                /* not at end of section?   */
    {
        if (! line().empty())                   /* any value in section?    */
            good = parse_control_stanza(automation::category::loop);

        if (good)
        {
            good = next_data_line(file);
            ++count;
        }
    }
    if (count > 0)
    {
        infoprintf("%d loop-control lines", count);
    }
    good = line_after(file, "[mute-group-control]");
    count = 0;
    while (good)                                /* not at end of section?   */
    {
        if (! line().empty())                   /* any value in section?    */
            good = parse_control_stanza(automation::category::mute_group);

        if (good)
        {
            good = next_data_line(file);
            ++count;
        }
    }
    if (count > 0)
    {
        infoprintf("%d mute-group-control lines", count);
    }

    good = line_after(file, "[automation-control]");
    count = 0;
    while (good)                                /* not at end of section?   */
    {
        if (! line().empty())                   /* any value in section?    */
            good = parse_control_stanza(automation::category::automation);

        if (good)
        {
            good = next_data_line(file);
            ++count;
        }
    }
    if (count > 0)
    {
        infoprintf("%d automation-control lines", count);
        if (count < current_slot_count())       /* pre-defined  */
             add_default_automation_stanzas(count);
    }
    if (rc_ref().verbose())
    {
        static bool s_not_shown = true;
        if (s_not_shown)
        {
            s_not_shown = false;                /* show only once per run   */
            m_temp_key_controls.show();
        }
    }
    if (m_temp_midi_ctrl_in.count() > 0)
    {
        rc_ref().midi_control_in().clear();
        rc_ref().midi_control_in() = m_temp_midi_ctrl_in;
        rc_ref().midi_control_in().inactive_allowed(true);  /* always true  */
    }
    if (m_temp_key_controls.count() > 0)
    {
        rc_ref().key_controls().clear();
        rc_ref().key_controls() = m_temp_key_controls;
    }
    (void) parse_midi_control_out(file);
    return result;
}

/**
 *  A helper function for parsing the MIDI Control I/O sections.
 */

bool
midicontrolfile::parse_control_sizes
(
    std::ifstream & file,
    const std::string & mctag,
    int & newoffset,
    int & newrows,
    int & newcolumns
)
{
    int defaultrows = usr().mainwnd_rows();
    int defaultcolumns = usr().mainwnd_cols();
    int rows = defaultrows;
    int columns = defaultcolumns;
    std::string s = get_variable(file, mctag, "button-offset");
    newoffset = string_to_int(s, 0);                /* currently constant   */
    s = get_variable(file, mctag, "button-rows");
    rows = string_to_int(s, defaultrows);
    if (rows <= 0)
        rows = defaultrows;

    infoprintf("Setting control rows = %d", rows);
    newrows = rows;
    s = get_variable(file, mctag, "button-columns");
    columns = string_to_int(s, defaultcolumns);
    if (columns <= 0)
        columns = defaultcolumns;

    infoprintf("Setting control columns = %d", columns);
    newcolumns = columns;

    bool result =
    (
        (rows >= screenset::c_min_rows) &&
        (rows <= screenset::c_max_rows) &&
        (columns >= screenset::c_min_columns) &&
        (columns <= screenset::c_max_columns)
    );
    return result;
}

/**
 *  Gets the number of sequence definitions provided in the midi-control
 *  sections.
 *
 * \return
 *      Returns true if the file was able to be opened for reading.
 *      Currently, there is no indication if the parsing actually succeeded.
 */

bool
midicontrolfile::parse ()
{
    bool result = true;
    std::ifstream file(name(), std::ios::in | std::ios::ate);
    if (! file.is_open())
    {
        file_error("Open failed", name());
        result = false;
    }
    else
        result = parse_stream(file);

    return result;
}

/*
 *  Removes the "enabled" flag from each control-input stanza: loops,
 *  mute-groups, and automation.  The "inverse" flag remains.
 *  The "enabled" status will be determined by the status value not being
 *  0x00.  We changed '%10s' to '%s' because the '10' causes sscanf() to
 *  return a count of 2 instead of 17.  Weird with a beard!
 */

static const char * const sg_scanf_fmt_ctrl_in_2 =
"%d %s [ %d %i %i %i %i ] [ %d %i %i %i %i ] [ %d %i %i %i %i ]";

/*
 *  Removes the "enabled" flag, replaced by testing for status not equal to
 *  0x00.  Also removes the channel value, which becomes part of the status
 *  value.  Used for [midi-control-out].
 */

static const char * const sg_scanf_fmt_ctrl_out_2 =
    "%d [ %i %i %i ] [ %i %i %i ] [ %i %i %i ] [ %i %i %i ]";

/*
 *  Adds a third stanze, "del" (unconfigured) value.  Used for
 *  [automation-control-out].
 */

static const char * const sg_scanf_fmt_triples =
    "%d [ %i %i %i ] [ %i %i %i ] [ %i %i %i ]";

/*
 *  Removes the channel value.  It goes into the status value instead.
 *  Used for [mute-control-out].
 */

static const char * const sg_scanf_fmt_mutes_triple =
    "%d [ %i %i %i ] [ %i %i %i ] [ %i %i %i ]";

/**
 *  It is not an error for the "[midi-contro-out]" section to be missing.
 */

bool
midicontrolfile::parse_midi_control_out (std::ifstream & file)
{
    bool result;
    std::string mctag = "[midi-control-out-settings]";
    std::string s = get_variable(file, mctag, "set-size");
    int sequences = string_to_int(s, setmaster::Size());
    bussbyte buss = get_buss_number(file, true, mctag, "output-buss");
    bool enabled = false;
    s = get_variable(file, mctag, "midi-enabled");
    if (s.empty())
    {
        s = get_variable(file, mctag, "enabled");       /* the old tag name */
        enabled = string_to_bool(s);
    }
    else
        enabled = string_to_bool(s);

    /*
     * We need to read them anyway, for saving back at exit.  The enabled-flag
     * will determine if they are used.
     */

    int offset = 0, rows = 0, columns = 0;
    result = parse_control_sizes(file, mctag, offset, rows, columns);
    if (result)
    {
        if (enabled)
            enabled = rc_ref().midi_control_active();
    }
    else
        enabled = false;

    if (line_after(file, "[midi-control-out]"))
    {
        /*
         * Set up the default-constructed midicontrolout object with its buss,
         * setsize, and enabled values.  Then read in the control-out data.
         * The performer sets the masterbus later on.
         */

        midicontrolout & mco = rc_ref().midi_control_out();
        if (mco.initialize(buss, rows, columns))
        {
            mco.is_enabled(enabled);
            mco.configure_enabled(enabled);
            mco.offset(offset);
            mco.configured_buss(buss);
        }
        if (file_version_number() < 2)
        {
            result = version_error_message("ctrl", file_version_number());
        }
        else
        {
            for (int i = 0; i < sequences; ++i)
            {
                int a[4], b[4], c[4], d[4];
                int sequence = 0;
                (void) std::sscanf
                (
                    scanline(), sg_scanf_fmt_ctrl_out_2, &sequence,
                    &a[0], &a[1], &a[2], &b[0], &b[1], &b[2],
                    &c[0], &c[1], &c[2], &d[0], &d[1], &d[2]
                );

                /*
                 *  Offset to avoid the deprecated enabled and channel values.
                 */

                mco.set_seq_event(i, midicontrolout::seqaction::armed, a);
                mco.set_seq_event(i, midicontrolout::seqaction::muted, b);
                mco.set_seq_event(i, midicontrolout::seqaction::queued, c);
                mco.set_seq_event(i, midicontrolout::seqaction::removed, d);
                if (i < (sequences - 1) && ! next_data_line(file))
                {
                    make_error_message("midi-control-out", "insufficient data");
                    break;
                }
            }
        }

        /*
         *  This code (now permanent) adds two section markers and one section
         *  for mutes, similar to the ctrl-pair options that follow this
         *  clause.
         */

        bool ok = true;
        if (line_after(file, "[mute-control-out]"))
        {
            int M = mutegroups::Size();
            for (int m = 0; m < M; ++m)
            {
                ok = read_mutes_triple(file, mco, m) || (m == (M - 1));
                if (! ok)
                    break;                  /* currently not an error   */
            }
        }
        if (ok)
            ok = line_after(file, "[automation-control-out]");

        /* Non-sequence (automation) actions */

        bool newtriples = file_version_number() > 3;
        ok = true;
        if (newtriples)
        {
            ok = read_triples(file, mco, midicontrolout::uiaction::panic);
            if (ok)
            {
                read_triples(file, mco, midicontrolout::uiaction::stop);
                read_triples(file, mco, midicontrolout::uiaction::pause);
                read_triples(file, mco, midicontrolout::uiaction::play);
                read_triples(file, mco, midicontrolout::uiaction::toggle_mutes);
                read_triples(file, mco, midicontrolout::uiaction::song_record);
                read_triples(file, mco, midicontrolout::uiaction::slot_shift);
                read_triples(file, mco, midicontrolout::uiaction::free);
                read_triples(file, mco, midicontrolout::uiaction::queue);
                read_triples(file, mco, midicontrolout::uiaction::oneshot);
                read_triples(file, mco, midicontrolout::uiaction::replace);
                read_triples(file, mco, midicontrolout::uiaction::snap);
                read_triples(file, mco, midicontrolout::uiaction::song);
                read_triples(file, mco, midicontrolout::uiaction::learn);
                read_triples(file, mco, midicontrolout::uiaction::bpm_up);
                read_triples(file, mco, midicontrolout::uiaction::bpm_dn);
                read_triples(file, mco, midicontrolout::uiaction::list_up);
                read_triples(file, mco, midicontrolout::uiaction::list_dn);
                read_triples(file, mco, midicontrolout::uiaction::song_up);
                read_triples(file, mco, midicontrolout::uiaction::song_dn);
                read_triples(file, mco, midicontrolout::uiaction::set_up);
                read_triples(file, mco, midicontrolout::uiaction::set_dn);
                read_triples(file, mco, midicontrolout::uiaction::tap_bpm);
                read_triples(file, mco, midicontrolout::uiaction::quit);
                read_triples(file, mco, midicontrolout::uiaction::visibility);
                read_triples(file, mco, midicontrolout::uiaction::alt_2);
                read_triples(file, mco, midicontrolout::uiaction::alt_3);
                read_triples(file, mco, midicontrolout::uiaction::alt_4);
                read_triples(file, mco, midicontrolout::uiaction::alt_5);
                read_triples(file, mco, midicontrolout::uiaction::alt_6);
                read_triples(file, mco, midicontrolout::uiaction::alt_7);
                read_triples(file, mco, midicontrolout::uiaction::alt_8);
            }
            if (ok)
            {
                const std::string tag{"[macro-control-out]"};
                ok = line_after(file, tag);
                if (ok)
                {
                    int count = 0;
                    mco.clear_macros();         /* clear it for each pass   */
                    while (ok)
                    {
                        tokenization t = tokenize(line(), "=");
                        ok = mco.add_macro(t);
                        if (ok)
                        {
                            ++count;
                            ok = next_data_line(file);
                        }
                    }
                    ok = count > 0;
                    if (ok)
                    {
                        (void) mco.expand_macros();
                        (void) info_message(mco.macro_byte_strings());
                    }
                }
                else
                    ok = mco.make_macro_defaults();

                ok = get_boolean(file, tag, "active", 0, true);
                if (ok)
                    ok = rc_ref().midi_control_active();/* ca 2022-08-08    */

                mco.macros_active(ok);
            }
            else
                make_error_message("midi-control-out", "read-triple error");
        }
        else
            result = version_error_message("ctrl", file_version_number());

        if (result)
            result = ok && ! is_error();
    }
    else
        result = false;

    if (! result)
        rc_ref().midi_control_out().is_enabled(false);  /* blank section    */

    return result;
}

/**
 *  Reads the first digit, which is the "enabled" bit, plus a pair of stanzas
 *  with four values in this order: channel, status, d1, and d2.
 *
 *  This function assumes we have already got the line to read, and it gets
 *  the next data line at the end.
 */

bool
midicontrolfile::read_triples
(
    std::ifstream & file,
    midicontrolout & mco,
    midicontrolout::uiaction a
)
{
    int enabled, ev_on[4], ev_off[4], ev_del[4];
    if (file_version_number() < 2)
    {
        return version_error_message("ctrl", file_version_number());
    }
    else
    {
        int count = std::sscanf
        (
            scanline(), sg_scanf_fmt_triples, &enabled,
            &ev_on[0], &ev_on[1], &ev_on[2],
            &ev_off[0], &ev_off[1], &ev_off[2],
            &ev_del[0], &ev_del[1], &ev_del[2]
        );
        if (count < 10)
            ev_del[0] = ev_del[1] = ev_del[2] = ev_del[3] = 0;

        if (count < 7)
            ev_off[0] = ev_off[1] = ev_off[2] = ev_off[3] = 0;

        mco.set_event(a, enabled, ev_on, ev_off, ev_del);
    }
    return next_data_line(file);
}

bool
midicontrolfile::read_mutes_triple
(
    std::ifstream & file,
    midicontrolout & mco,
    int group
)
{
    if (file_version_number() < 2)
    {
        return version_error_message("ctrl", file_version_number());
    }
    else
    {
        int number, ev_on[4], ev_off[4], ev_del[4];
        (void) std::sscanf
        (
            scanline(), sg_scanf_fmt_mutes_triple, &number,
            &ev_on[0],  &ev_on[1],  &ev_on[2],
            &ev_off[0], &ev_off[1], &ev_off[2],
            &ev_del[0], &ev_del[1], &ev_del[2]
        );
        mco.set_mutes_event(group, ev_on, ev_off, ev_del);
    }
    return next_data_line(file);
}

bool
midicontrolfile::write_stream (std::ofstream & file)
{
    write_date(file, "MIDI control");
    file <<
"# Sets up MIDI I/O control. The format is like the 'rc' file. To use it, set it\n"
"# active in the 'rc' [midi-control-file] section. It adds loop, mute, &\n"
"# automation buttons, MIDI display, new settings, and macros.\n"
    ;
    write_seq66_header(file, "ctrl", version());

    /*
     * [comments]
     */

    std::string s = rc_ref().midi_control_in().comments_block().text();
    write_comment(file, s);

    bool result = write_midi_control(file);
    if (result)
        result = write_midi_control_out(file);

    if (result)
        write_seq66_footer(file);

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
midicontrolfile::write ()
{
    std::ofstream file(name(), std::ios::out | std::ios::trunc);
    bool result = file.is_open();
    if (result)
    {
        result = container_to_stanzas(rc_ref().midi_control_in());
        if (result)
        {
            file_message("Writing ctrl", name());
            result = write_stream(file);
            if (! result)
                file_error("Write fail", name());
        }
        file.close();
    }
    else
    {
        file_error("Write open fail", name());
    }
    return result;
}

/**
 *  Writes these sections to the given file stream:
 *
 *      -   [midi-control-settings]
 *      -   [loop-control]
 *      -   [mute-group-control]
 *      -   [automation-control]
 *
 * \param file
 *      Provides the output file stream to write to.
 *
 * \return
 *      Returns true if the write operations all succeeded.
 */

bool
midicontrolfile::write_midi_control (std::ofstream & file)
{
    bool result = file.is_open();
    if (result)
    {
        const midicontrolin & mci = rc_ref().midi_control_in();
        bussbyte bb = mci.configured_buss();
        file <<
"\n[midi-control-settings]\n\n"
"# Input settings to control Seq66. 'control-buss' ranges from 0 to the highest\n"
"# system input buss. If set, that buss can send MIDI control. 255 (0xFF) means\n"
"# any enabled input device can send control. ALSA provides an extra 'announce'\n"
"# buss, altering port numbering vice JACK. With port-mapping enabled, the port\n"
"# nick-name can be provided.\n"
"#\n"
"# 'midi-enabled' applies to the MIDI controls; keystroke controls are always\n"
"# enabled. Supported keyboard layouts are 'qwerty' (default), 'qwertz', and\n"
"# 'azerty'. AZERTY turns off auto-shift for group-learn.\n\n"
        ;
        write_boolean
        (
            file, "drop-empty-controls", rc_ref().drop_empty_in_controls()
        );
        write_buss_info(file, false, "control-buss", bb);

        int defaultrows = mci.rows();
        int defaultcolumns = mci.columns();
        if (defaultrows == 0)
            defaultrows = usr().mainwnd_rows();

        if (defaultcolumns == 0)
            defaultcolumns = usr().mainwnd_cols();

        write_boolean(file, "midi-enabled", mci.configure_enabled());
        write_integer(file, "button-offset", mci.offset());
        write_integer(file, "button-rows", defaultrows);
        write_integer(file, "button-columns", defaultcolumns);
        write_string
        (
            file, "keyboard-layout",
            rc_ref().key_controls().kbd_layout_to_string()
        );
        file <<
"\n"
"# A control stanza sets key and MIDI control. Keys support 'toggle', and\n"
"# key-release is 'invert'. The leftmost number on each line is the loop number\n"
"# (0 to 31), mutes number (same range), or an automation number. 3 groups of\n"
"# of bracketed numbers follow, each providing a type of control:\n"
"#\n"
"#    Normal:         [toggle]    [on]        [off]\n"
"#    Increment/Decr: [increment] [increment] [decrement]\n"
"#    Playback:       [pause]     [start]     [stop]\n"
"#    Playlist/Song:  [by-value]  [next]      [previous]\n"
"#\n"
"# In each group, there are 5 numbers:\n"
"#\n"
"#    [invert status d0 d1min d1max]\n"
        ;

        file <<
"#\n"
"# A valid status (> 0x00) enables the control; 'invert' (1/0) inverts the,\n"
"# the action, but not all support this.  'status' is the MIDI event to match\n"
"# (channel is NOT ignored); 'd0' is the status value (eg. if 0x90, Note On,\n"
"# d0 is the note number; d1min to d1max is the range of d1 values detectable.\n"
"# Hex values can be used; precede with '0x'.\n"
"#\n"
"#  ------------------------ Loop/group/automation-slot number\n"
"# |    -------------------- Name of key (see the key map)\n"
"# |   |      -------------- Inverse\n"
"# |   |     |    ---------- MIDI status/event byte (eg. Note On)\n"
"# |   |     |   |   ------- d0: Data 1 (eg. Note number)\n"
"# |   |     |   |  |  ----- d1max: Data 2 min (eg. Note velocity)\n"
"# |   |     |   |  | |   -- d1min: Data 2 max\n"
"# |   |     |   |  | |  |\n"
"# v   v     v   v  v v  v\n"
"# 0 \"F1\"   [0 0x90 0 1 127] [0 0x00 0 0 0] [0 0x00 0 0 0]\n"
"#             Toggle           On              Off\n"
"#\n"
"# MIDI controls often send a Note On upon a press and a Note Off on release.\n"
"# To use a control as a toggle, define only the Toggle stanza. For the control\n"
"# to act only while held, define the On and Off stanzas with appropriate\n"
"# statuses for press-and-release.\n"
        ;

        /*
         *  Write out all of the 3-part stanzas, each in their own category
         *  section.  This sequence depends on the stanzas being sorted by
         *  category.
         */

        automation::category opcat = automation::category::none;
        for (const auto & stz : m_stanzas)
        {
            const midicontrolfile::stanza & stan = stz.second;
            automation::category currcat = stan.category_code();
            if (currcat != opcat)
            {
                opcat = currcat;
                if (currcat == automation::category::loop)
                    file << "\n[loop-control]\n\n";
                else if (currcat == automation::category::mute_group)
                    file << "\n[mute-group-control]\n\n";
                else if (currcat == automation::category::automation)
                    file << "\n[automation-control]\n\n";
            }
            int spacing = 12 - int(stan.key_name().size());
            file
                << std::setw(2) << stan.slot_number()
                << " \"" << stan.key_name() << "\""
                << std::setw(spacing) << " "
                ;
            for (int action = 0; action < automation::ACTCOUNT; ++action)
            {
                file
                    << "["
                    << std::setw(2) << stan.setting(action, 0)  /* inverse  */
                    << " 0x" << std::setw(2) << std::setfill('0')
                    << std::hex << stan.setting(action, 1)      /* status   */
                    << std::setw(4) << std::setfill(' ')
                    << std::dec << stan.setting(action, 2)      /* d0       */
                    << std::setw(4)
                    << std::dec << stan.setting(action, 3)      /* min      */
                    << std::setw(4)
                    << std::dec << stan.setting(action, 4)      /* max      */
                    << " ] "
                    ;
            }
            file << "# " << stan.op_name() << std::endl;
        }
    }
    return result;
}

/**
 *  Writes a MIDI user-interface-related data stanza of the form
 *  "1 [ 0 0x00 0 ] [ 0 0x00 0 ] [ 0 0x00 0 ]".  Here, action_del is
 *  used for the "unconfigured" (del) status.
 */

bool
midicontrolfile::write_triples
(
    std::ofstream & file,
    const midicontrolout & mco,
    midicontrolout::uiaction a
)
{
    bool active = mco.event_is_active(a);
    std::string act1str = mco.get_ctrl_event_str
    (
        a, midicontrolout::action_on
    );
    std::string act2str = mco.get_ctrl_event_str
    (
        a, midicontrolout::action_off
    );
    std::string act3str = mco.get_ctrl_event_str
    (
        a, midicontrolout::action_del
    );
    file
        << (active ? 1 : 0) << " "
        << act1str << " " << act2str << " " << act3str
        << "  # " << action_to_string(a) << "\n"
        ;
    return file.good();
}

bool
midicontrolfile::write_mutes_triple
(
    std::ofstream & file,
    const midicontrolout & mco,
    int group
)
{
    /*
     * Always active; group number written instead.
     *
     * bool active = mco.mutes_event_is_active(group);
     */

    std::string act1str = mco.get_mutes_event_str
    (
        group, midicontrolout::action_on
    );
    std::string act2str = mco.get_mutes_event_str
    (
        group, midicontrolout::action_off
    );
    std::string act3str = mco.get_mutes_event_str
    (
        group, midicontrolout::action_del
    );
    file
        << std::setw(2) << std::dec << group << " "
        << act1str << " " << act2str << " " << act3str << "\n"
        ;
    return file.good();
}

/**
 *  Writes out the MIDI control data for the patterns and for the
 *  user-interface actions.
 */

bool
midicontrolfile::write_midi_control_out (std::ofstream & file)
{
    /* const */ midicontrolout & mco = rc_ref().midi_control_out();
    int setsize = mco.screenset_size();
    bussbyte bb = mco.configured_buss();
    bool result = is_valid_buss(bb);            /* very light sanity check  */
    if (result)
    {
        if (setsize == 0)
        {
            mco.initialize
            (
                bb, screenset::c_default_rows, screenset::c_default_columns
            );
            setsize = mco.screenset_size();
        }
        file << "\n[midi-control-out-settings]\n\n";
        write_integer(file, "set-size", setsize);
        write_buss_info(file, true, "output-buss", bb);
        write_boolean(file, "midi-enabled", mco.configure_enabled());
        write_integer(file, "button-offset", mco.offset());
        write_integer(file, "button-rows", mco.rows());
        write_integer(file, "button-columns", mco.columns());
        file <<
"\n"
"[midi-control-out]\n"
"\n"
"# This section determines how pattern statuses are to be displayed.\n"
"\n"
"#   ---------------- Pattern or device-button number)\n"
"#  |     ----------- MIDI status+channel (eg. Note On)\n"
"#  |    |    ------- data 1 (eg. note number)\n"
"#  |    |   |  ----- data 2 (eg. velocity)\n"
"#  |    |   | |\n"
"#  v    v   v v\n"
"# 31 [ 0x00 0 0 ] [ 0x00 0 0 ] [ 0x00 0 0 ] [ 0x00 0 0]\n"
"#      Armed        Muted        (Un)queued   Empty/Deleted\n"
"#\n"
"# A test of the status byte determines the enabled status, and channel is\n"
"# included in the status.\n"
"\n"
            ;   /* heh heh */

        if (mco.is_blank())
        {
            for (int seq = 0; seq < setsize; ++seq)
            {
                file << std::setw(2) << seq <<
                    " [ 0x00   0   0 ]" " [ 0x00   0   0 ]"
                    " [ 0x00   0   0 ]" " [ 0x00   0   0 ]\n"
                    ;
            }
        }
        else
        {
            for (int seq = 0; seq < setsize; ++seq)
            {
                int minimum, maximum;
                midicontrolout::seqaction_range(minimum, maximum);
                file << std::setw(2) << std::dec << seq;
                for (int a = minimum; a < maximum; ++a)
                {
                    event ev = mco.get_seq_event
                    (
                        seq, midicontrolout::seqaction(a)
                    );
                    midibyte d0, d1;
                    char temp[48];
                    ev.get_data(d0, d1);
                    (void) snprintf             /* much easier format!  */
                    (
                        temp, sizeof temp, " [ 0x%02x %3d %3d ]",
                        unsigned(ev.get_status()), int(d0), int(d1)
                    );
                    file << temp;
                }
                file << "\n";
            }
        }
        file <<
"\n[mute-control-out]\n\n"
"# The format of the mute and automation output events is similar:\n"
"#\n"
"#  ----------------- mute-group number\n"
"# |    ------------- MIDI status+channel (eg. Note On)\n"
"# |   |    --------- data 1 (eg. note number)\n"
"# |   |   |  ------- data 2 (eg. velocity)\n"
"# |   |   | |\n"
"# v   v   v v\n"
"# 1 [0x00 0 0 ] [0x00 0 0] [0x00 0 0]\n"
"#       On         Off      Empty (dark)\n"
"#\n"
"# The mute-controls have an additional stanza for non-populated (\"deleted\")\n"
"# mute-groups.\n"
"\n"
            ;

        for (int m = 0; m < mutegroups::Size(); ++m)
        {
            if (! write_mutes_triple(file, mco, m))
                break;
        }

        file << "\n[automation-control-out]\n\n"
"# This format is similar to [mute-control-out], but the first number is an\n"
"# active-flag, not an index number. The stanzas are are on / off / inactive,\n"
"# except for 'snap', which is store /  restore / inactive.\n\n"
            ;
        write_triples(file, mco, midicontrolout::uiaction::panic);
        write_triples(file, mco, midicontrolout::uiaction::stop);
        write_triples(file, mco, midicontrolout::uiaction::pause);
        write_triples(file, mco, midicontrolout::uiaction::play);
        write_triples(file, mco, midicontrolout::uiaction::toggle_mutes);
        write_triples(file, mco, midicontrolout::uiaction::song_record);
        write_triples(file, mco, midicontrolout::uiaction::slot_shift);
        write_triples(file, mco, midicontrolout::uiaction::free);
        write_triples(file, mco, midicontrolout::uiaction::queue);
        write_triples(file, mco, midicontrolout::uiaction::oneshot);
        write_triples(file, mco, midicontrolout::uiaction::replace);
        write_triples(file, mco, midicontrolout::uiaction::snap);
        write_triples(file, mco, midicontrolout::uiaction::song);
        write_triples(file, mco, midicontrolout::uiaction::learn);
        write_triples(file, mco, midicontrolout::uiaction::bpm_up);
        write_triples(file, mco, midicontrolout::uiaction::bpm_dn);
        write_triples(file, mco, midicontrolout::uiaction::list_up);
        write_triples(file, mco, midicontrolout::uiaction::list_dn);
        write_triples(file, mco, midicontrolout::uiaction::song_up);
        write_triples(file, mco, midicontrolout::uiaction::song_dn);
        write_triples(file, mco, midicontrolout::uiaction::set_up);
        write_triples(file, mco, midicontrolout::uiaction::set_dn);
        write_triples(file, mco, midicontrolout::uiaction::tap_bpm);
        write_triples(file, mco, midicontrolout::uiaction::quit);
        write_triples(file, mco, midicontrolout::uiaction::visibility);
        write_triples(file, mco, midicontrolout::uiaction::alt_2);
        write_triples(file, mco, midicontrolout::uiaction::alt_3);
        write_triples(file, mco, midicontrolout::uiaction::alt_4);
        write_triples(file, mco, midicontrolout::uiaction::alt_5);
        write_triples(file, mco, midicontrolout::uiaction::alt_6);
        write_triples(file, mco, midicontrolout::uiaction::alt_7);
        write_triples(file, mco, midicontrolout::uiaction::alt_8);

        /*
         * Write any macros that exist.
         */

        file << "\n[macro-control-out]\n\n"
"# This format is 'macroname = [ hex bytes | macro-references]'. Macro references\n"
"# are macro-names preceded by a '$'.  Some values should always be defined, even\n"
"# if empty: footer, header, reset, startup, and shutdown.\n\n"
            ;

        std::string lines = mco.macro_lines();
        if (lines.empty())
        {
            file <<
                "footer =\n"
                "header =\n"
                "reset =\n"
                "startup =\n"
                "shutdown =\n"
                ;
        }
        else
            file << lines << std::endl;
    }
    return result;
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
read_midi_control_file
(
    const std::string & fname,
    rcsettings & rcs
)
{
    midicontrolfile mcf(fname, rcs);
    return mcf.parse();
}

/**
 *  For automation, slot and code are the same numeric value. We've
 *  added code so that blank stanzas can be automatically added to
 *  older configuration files.
 */

bool
midicontrolfile::parse_control_stanza (automation::category opcat, int index)
{
    bool result = true;
    automation::slot opslot = automation::slot::none;
    int count = 0;
    bool keep_empties = ! rc_ref().drop_empty_in_controls();
    std::string keyname;

    /*
     * Was deprecated, now no longer supported.
     */

    if (file_version_number() < 2)
    {
        result = version_error_message("ctrl", file_version_number());
    }
    else
    {
        int a[8] { };
        int b[8] { };
        int c[8] { };
        bool ok;
        if (index == 0)
        {
            char charname[16];
            count = std::sscanf
            (
                scanline(), sg_scanf_fmt_ctrl_in_2, &index, &charname[0],
                &a[0], &a[1], &a[2], &a[3], &a[4],
                &b[0], &b[1], &b[2], &b[3], &b[4],
                &c[0], &c[1], &c[2], &c[3], &c[4]
            );
            keyname = strip_quotes(std::string(charname));
            ok = count == 17;
        }
        else
        {
            keyname = keycontainer::automation_key_name(index);
            ok = ! keyname.empty();
        }
        if (ok)
        {
            opslot = automation::slot::none;
            if (opcat == automation::category::loop)
                opslot = automation::slot::loop;
            else if (opcat == automation::category::mute_group)
                opslot = automation::slot::mute_group;
            else if (opcat == automation::category::automation)
                opslot = opcontrol::set_slot(index);

            /*
             *  Create control objects, whether active or not.  We want to
             *  save all objects in the file, to avoid altering the user's
             *  preferences.
             */

            midicontrol mca
            (
                keyname, opcat, automation::action::toggle, opslot, index
            );
            if (mca.set(a) || keep_empties)
                (void) m_temp_midi_ctrl_in.add(mca);

            midicontrol mcb
            (
                keyname, opcat, automation::action::on, opslot, index
            );
            if (mcb.set(b) || keep_empties)
                (void) m_temp_midi_ctrl_in.add(mcb);

            midicontrol mcc
            (
                keyname, opcat, automation::action::off, opslot, index
            );
            if (mcc.set(c) || keep_empties)
                (void) m_temp_midi_ctrl_in.add(mcc);
        }
        else
            result = false;
    }
    if (result)
    {
        /*
         *  Make reverse-lookup map<pattern, keystroke> for the show_ui
         *  functions.  It would be an addition to the keycontainer class.
         *  keyname = key_controls().key_name(slotnumber);
         */

        keycontrol kc
        (
            "", keyname, opcat, automation::action::toggle, opslot, index
        );
        ctrlkey ordinal = qt_keyname_ordinal(keyname);
        if (! is_invalid_ordinal(ordinal))          /* ca 2021-12-31 */
        {
            bool ok = m_temp_key_controls.add(ordinal, kc);
            if (ok)
            {
                if (opcat == automation::category::loop)
                    ok = m_temp_key_controls.add_slot(kc);
                else if (opcat == automation::category::mute_group)
                    ok = m_temp_key_controls.add_mute(kc);
            }
            if (! ok)
                (void) keycontrol_error_message(kc, ordinal, line_number());
        }
    }
    else
    {
        errprintf("unexpected control count %d in stanza", count);
        result = false;
    }
    return result;
}

/**
 *  Adds to key-controls and midi-control-in controls in cases where the 'ctrl'
 *  file pre-dates the significant increase in automation slots.
 */

bool
midicontrolfile::add_default_automation_stanzas (int starting_index)
{
    bool result = true;
    int ending_index = current_slot_count();
    for (int i = starting_index; i < ending_index; ++i)
    {
        bool ok = parse_control_stanza(automation::category::automation, i);
        if (! ok)
        {
            result = false;
            break;
        }
    }
    return result;
}

/**
 *  The counterpart is write_buss_info().  This function depends on the 'rc'
 *  file being read first, in case port-mapping is specified.
 */

bussbyte
midicontrolfile::get_buss_number
(
    std::ifstream & file,
    bool isoutputport,
    const std::string & tag,
    const std::string & varname
)
{
    const int defalt = (-1);
    int result = defalt;
    std::string s = get_variable(file, tag, varname);
    if (! s.empty())
    {
        result = string_to_int(s, defalt);
        if (result == defalt)               /* could not convert as integer */
        {
            if (isoutputport)
            {
                clockslist & opm = output_port_map();
                if (opm.active())
                {
                    bussbyte b = opm.bus_from_name(s);          /* 0 to FF  */
                    result = int(b);
                    msgprintf
                    (
                        msglevel::status, "Output buss '%s' port %d",
                        s.c_str(), result
                    );
                }
                else
                    result = int(default_control_out_buss());
            }
            else
            {
                inputslist & ipm = input_port_map();
                if (ipm.active())
                {
                    bussbyte b = ipm.bus_from_name(s);          /* 0 to FF  */
                    result = int(b);
                    msgprintf
                    (
                        msglevel::status, "Input buss '%s' port %d",
                        s.c_str(), result
                    );
                }
                else
                    result = int(default_control_in_buss());
            }
        }
    }
    return result;
}

void
midicontrolfile::write_buss_info
(
    std::ofstream & file,
    bool isoutputport,
    const std::string & varname,
    bussbyte nb                     /* nominalbuss */
)
{
    bool active = false;
    std::string buss_string;
    if (isoutputport)
    {
        clockslist & opm = output_port_map();
        active = opm.active();
        if (active)
            buss_string = opm.port_name_from_bus(nb);
    }
    else
    {
        inputslist & ipm = input_port_map();
        active = ipm.active();
        if (active)
            buss_string = ipm.port_name_from_bus(nb);
    }
    if (active)
    {
        buss_string = add_quotes(buss_string);
        write_string(file, varname, buss_string);
    }
    else
    {
        write_integer(file, varname, int(nb), is_null_buss(nb));
    }
}

/**
 *  Note that midicontrolin is a multimap, and it can hold multiple
 *  midicontrols for a given midicontrol::key, so that the same event can
 *  trigger multiple operations/actions.
 */

bool
midicontrolfile::container_to_stanzas (const midicontrolin & mc)
{
    int current_count = mc.count();
    bool result = current_count > 0;
    if (result)
    {
        m_stanzas.clear();                      /* new ca 2021-12-03        */
        for (const auto & m : mc.container())
        {
            const midicontrol & mco = m.second;
            key k(mco);
            auto stanziter = m_stanzas.find(k);
            bool ok;
            if (stanziter != m_stanzas.end())
            {
                /*
                 * Here, the stanza is already in place, but we need to
                 * update it with the right action settings.  This normally
                 * occurs when all three sub-stanzas have the same values
                 * (which rationally happens when the MIDI control event is
                 * not configured (all zeroes).
                 */

                stanziter->second.set(mco);
                ok = true;                      /* points to the found one  */
            }
            else
            {
                stanza s(mco);                  /* includes settings sect.  */
                auto sz = m_stanzas.size();
                auto p = std::make_pair(k, s);
                (void) m_stanzas.insert(p);
                ok = m_stanzas.size() == (sz + 1);
            }
            if (! ok)
            {
                errprint("couldn't update midicontrol:");
                mco.show(true);
                result = false;
                break;
            }
        }

#if defined SEQ66_PLATFORM_DEBUG_TMI
        int origslotcount = original_slot_count();
        int currslotcount = current_slot_count();
        printf
        (
            "mc.count() = %d, orig slots = %d, curr slots = %d\n",
            current_count, origslotcount, currslotcount
        );
#endif
    }
    return result;
}

void
midicontrolfile::show_stanza (const stanza & stan) const
{
    std::cout
        << "[" << stan.category_name() << "-control] "
        << "'" << std::setw(7) << stan.key_name() << "'"
        << " " << std::setw(2) << stan.slot_number() << " "
        ;

    for (int action = 0; action < automation::ACTCOUNT; ++action)
    {
        std::cout
            << "["
            << std::setw(2) << stan.setting(action, 0)  /* active           */
            << std::setw(2) << stan.setting(action, 1)  /* inverse active   */
            << " 0x" << std::setw(2) << std::setfill('0')
            << std::hex << stan.setting(action, 2)      /* status           */
            << std::setw(4) << std::setfill(' ')
            << std::dec << stan.setting(action, 3)      /* d0               */
            << std::setw(4)
            << std::dec << stan.setting(action, 4)      /* min              */
            << std::setw(4)
            << std::dec << stan.setting(action, 5)      /* max              */
            << " ] ";
    }
    std::cout << stan.op_name() << std::endl;
}

void
midicontrolfile::show_stanzas () const
{
    std::cout << "Number of stanzas = " << m_stanzas.size() << std::endl;
    if (m_stanzas.size() > 0)
    {
        for (const auto & stz : m_stanzas)
            show_stanza(stz.second);
    }
}

/**
 *  A free function to write the MIDI control file. It handles these cases:
 *
 *  -   Recreating the file if the controls are active or if the file does
 *      not exist.
 *      -   If controls exist:
 *          -   If less than the maximum number (automation::slot::max), then
 *              pad the container to the max (the keycontainer always
 *              defaults to full size, so it can be used as a reference).
 *          -   If full-sized already, just write the file.
 *      -   Otherwise, use the key-controls to populate blank MIDI controls
 *          to the full-size of the automation slots.
 */

bool
write_midi_control_file
(
    const std::string & mcfname,
    rcsettings & rcs
)
{
    bool exists = file_exists(mcfname);
    bool active = rcs.midi_control_active();
    bool recreate = active || ! exists;
    const midicontrolin & ctrls = rcs.midi_control_in();
    bool result = ctrls.count() > 0;
    if (result)
    {
        if (recreate)
        {
            midicontrolfile mcf(mcfname, rcs);
            if (result)
                result = mcf.write();
        }
    }
    else
    {
        if (recreate)
        {
            keycontainer & keys = rcs.key_controls();
            midicontrolin & ctrls = rcs.midi_control_in();
            midicontrolfile mcf(mcfname, rcs);
            ctrls.add_blank_controls(keys);
            result = mcf.write();
        }
    }
    if (! result)
        file_error("Write ctrl failed", mcfname);

    return result;
}

}           // namespace seq66

/*
 * midicontrolfile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


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
 * \file          midicontrolfile.cpp
 *
 *  This module declares/defines the base class for managing the reading and
 *  writing of the midi-control sections of the "rc" file.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-13
 * \updates       2021-04-23
 * \license       GNU GPLv2 or above
 *
 */

#include <iostream>                     /* std::cout                        */
#include <iomanip>                      /* std::setw()                      */

#include "cfg/midicontrolfile.hpp"      /* seq66::midicontrolfile class     */
#include "cfg/settings.hpp"             /* seq66::rc()                      */
#include "ctrl/keymap.hpp"              /* seq66::qt_keyname_ordinal()      */
#include "play/setmaster.hpp"           /* seq66::setmaster::Size() static  */
#include "util/calculations.hpp"        /* seq66::string_to_bool(), etc.    */
#include "util/strfunctions.hpp"        /* seq66::strip_quotes()            */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

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
    m_slot_number       (static_cast<int>(mc.slot_number())),
    m_settings          ()                          /* 2-dimensional array  */
{
    if (m_category != automation::category::automation)
        m_slot_number = static_cast<int>(mc.control_code());

    set(mc);
}

bool
midicontrolfile::stanza::set (const midicontrol & mc)
{
    automation::action a = mc.action_code();
    if (a > automation::action::none && a < automation::action::max)
    {
        unsigned index = static_cast<int>(a) - 1;   /* skips "none"         */
        m_settings[index][0] = int(mc.inverse_active());
        m_settings[index][1] = int(mc.status());
        m_settings[index][2] = int(mc.d0());        /* note: d1 not needed  */
        m_settings[index][3] = int(mc.min_value());
        m_settings[index][4] = int(mc.max_value());
    }
    return true;
}

/*
 * -------------------------------------------------------------------------
 *  midicontrolfile
 * -------------------------------------------------------------------------
 */

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
    configfile              (filename, rcs),
    m_temp_key_controls     (),                             /* reading only */
    m_temp_midi_ctrl_in     ("ctrl"),                       /* reading only */
    m_stanzas               ()
{
    version("3");                               /* adds more automation out */
}

/**
 *  A rote destructor.
 */

midicontrolfile::~midicontrolfile ()
{
    // ~configfile() called automatically
}

/**
 *  Parse the ~/.config/seq66/qseq66.rc file-stream or the
 *  ~/.config/seq66/qseq66.ctrl file-stream.
 *
 *  [comments]
 *
 *      [comments] Header commentary is skipped during parsing.  However, we now
 *      try to read an optional comment block.  This block is part of the MIDI
 *      container object, not part of the rcsettings object.
 *
 *  [midi-control-settings]  (was "midi-control-flags")
 *
 *      load-key-controls
 *      load-midi-controls
 *      control-buss
 *      midi-enabled
 *      button-offset
 *      button-rows
 *      button-columns
 *
 *  [midi-control] and [midi-control-file]
 *
 *      Get the number of sequence definitions provided in the following
 *      section.  Ranges from 32 on up.  Then read in all of the sequence lines.
 *      The first 32 apply to the first screen set.  There can also be a comment
 *      line "# mute in group" followed by 32 more lines.  Then there are
 *      additional comments and single lines for BPM up, BPM down, Screen Set Up,
 *      Screen Set Down, Mod Replace, Mod Snapshot, Mod Queue, Mod Gmute, Mod
 *      Glearn, and Screen Set Play.  These are all forms of MIDI automation
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
    version(parse_version(file));

    std::string s = parse_comments(file);
    if (! s.empty())
        m_temp_midi_ctrl_in.comments_block().set(s);

    std::string mctag = "[midi-control-settings]";  /* the new name for it  */
    if (bad_position(find_tag(file, mctag)))
        mctag = "[midi-control-flags]";             /* earlier name for it  */

    s = get_variable(file, mctag, "load-key-controls");
    rc_ref().load_key_controls(string_to_bool(s));
    s = get_variable(file, mctag, "load-midi-controls");
    rc_ref().load_midi_control_in(string_to_bool(s));

    bool loadmidi = rc_ref().load_midi_control_in();
    bool loadkeys = rc_ref().load_key_controls();
    s = get_variable(file, mctag, "control-buss");

    int buss = string_to_int(s, SEQ66_MIDI_CONTROL_IN_BUSS);
    s = get_variable(file, mctag, "midi-enabled");

    bool enabled = string_to_bool(s);
    int offset = 0, rows = 0, columns = 0;
    result = parse_control_sizes(file, mctag, offset, rows, columns);
    if (! result)
        enabled = false;

    if (m_temp_midi_ctrl_in.initialize(buss, rows, columns))
    {
        m_temp_midi_ctrl_in.is_enabled(enabled);
        m_temp_midi_ctrl_in.offset(offset);
    }
    if (loadkeys)
    {
        std::string layout = get_variable(file, mctag, "keyboard-layout");
        m_temp_key_controls.clear();
        m_temp_key_controls.set_kbd_layout(layout);
    }
    if (loadmidi || loadkeys)
    {
        bool good = line_after(file, "[loop-control]");
        int count = 0;
        while (good)                            /* not at end of section?   */
        {
            if (! line().empty())               /* any value in section?    */
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
        while (good)                            /* not at end of section?   */
        {
            if (! line().empty())               /* any value in section?    */
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
        while (good)                            /* not at end of section?   */
        {
            if (! line().empty())               /* any value in section?    */
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
        }
    }
    if (loadmidi && m_temp_midi_ctrl_in.count() > 0)
    {
        rc_ref().midi_control_in().clear();
        rc_ref().midi_control_in() = m_temp_midi_ctrl_in;
        rc_ref().midi_control_in().inactive_allowed(true);  /* always   */
    }
    if (rc_ref().load_key_controls() && m_temp_key_controls.count() > 0)
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
    bool result = true;
    int defaultrows = usr().mainwnd_rows();
    int defaultcolumns = usr().mainwnd_cols();
    int defvalue = 0;
    std::string s = get_variable(file, mctag, "button-offset");
    newoffset = string_to_int(s, 0);                /* currently constant   */
    s = get_variable(file, mctag, "button-rows");
    if (s.empty())                                  /* no button-rows entry */
        defvalue = defaultrows;

    int rows = string_to_int(s, defvalue);
    if (rows != defaultrows)
    {
        if (rows > 0)
            result = make_error_message(mctag, "invalid row count");
        else
            rows = defaultrows;
    }
    newrows = rows;
    defvalue = 0;
    s = get_variable(file, mctag, "button-columns");
    if (s.empty())                                  /* no button-rows entry */
        defvalue = defaultcolumns;

    int columns = string_to_int(s, defvalue);
    if (columns != defaultcolumns)
    {
        if (columns > 0)
            result = make_error_message(mctag, "invalid column count");
        columns = defaultcolumns;
    }
    newcolumns = columns;
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
        errprintf
        (
            "midicontrolfile::parse(): error opening %s for reading",
            name().c_str()
        );
        result = false;
    }
    else
        result = parse_stream(file);

    return result;
}

/**
 *  These format statements assume the active and inverse flags are a single
 *  digit (0 or 1), and that the rest of the values can optionally be preceded
 *  by "0x" (which is a better format for displaying event statuses).
 *
 *  The "%10s" specifier scans for up to 10 non-whitespace characters.  The
 *  double-quotes are stripped off after reading the key's name.
 *
 *  These format string are used in parse_control_stanza(),
 *  parse_midi_control_out(), and read_ctrl_triple().
 */

static const char * const sg_scanf_fmt_ctrl_in =
"%d %10s [ %d %d %i %i %i %i ] [ %d %d %i %i %i %i ] [ %d %d %i %i %i %i ]";

/*
 *  Removes the "enabled" flag from each control-input stanza: loops,
 *  mute-groups, and automation.  The "inverse" flag remains.
 *  The "enabled" status will be determined by the status value not being
 *  0x00.
 */

static const char * const sg_scanf_fmt_ctrl_in_2 =
"%d %10s [ %d %i %i %i %i ] [ %d %i %i %i %i ] [ %d %i %i %i %i ]";

/*
 * For version 1 only.
 */

static const char * const sg_scanf_fmt_ctrl_out =
"%d [ %d %i %i %i %i ] [ %d %i %i %i %i ] [ %d %i %i %i %i ] [ %d %i %i %i %i ]";

/*
 *  Removes the "enabled" flag, replaced by testing for status not equal to
 *  0x00.  Also removes the channel value, which becomes part of the status
 *  value.  Used for [midi-control-out].
 */

static const char * const sg_scanf_fmt_ctrl_out_2 =
    "%d [ %i %i %i ] [ %i %i %i ] [ %i %i %i ] [ %i %i %i ]";

/*
 * For version 1 only.
 */

static const char * const sg_scanf_fmt_ctrl_pair =
    "%d [ %i %i %i %i ] [ %i %i %i %i ]";

/*
 *  Removes the channel value.  Used for [automation-control-out].
 */

static const char * const sg_scanf_fmt_ctrl_pair_2 =
    "%d [ %i %i %i ] [ %i %i %i ]";

/*
 *  Adds a third stanze, "del" (unconfigured) value.  Used for
 *  [automation-control-out].
 */

static const char * const sg_scanf_fmt_ctrl_triple =
    "%d [ %i %i %i ] [ %i %i %i ] [ %i %i %i ]";

/*
 * For version 1 only.
 */

static const char * const sg_scanf_fmt_mutes_triple_1 =
    "%d [ %i %i %i %i ] [ %i %i %i %i ] [ %i %i %i %i ]";

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
    s = get_variable(file, mctag, "output-buss");
    if (s.empty())
        s = get_variable(file, mctag, "buss");          /* the old tag name */

    int buss = string_to_int(s, SEQ66_MIDI_CONTROL_OUT_BUSS);
    s = get_variable(file, mctag, "midi-enabled");
    if (s.empty())
        s = get_variable(file, mctag, "enabled");       /* the old tag name */

    /*
     * We need to read them anyway, for saving back at exit.  The enabled-flag
     * will determine if they are used.
     */

    bool enabled = string_to_bool(s);
    int offset = 0, rows = 0, columns = 0;
    result = parse_control_sizes(file, mctag, offset, rows, columns);
    if (! result)
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
            mco.offset(offset);
        }
        if (version_number() < 2)
        {
            infoprint("Reading version 1 'ctrl' file, will upgrade at exit");
            for (int i = 0; i < sequences; ++i)
            {
                int a[8], b[8], c[8], d[8];
                int sequence = 0;
                (void) std::sscanf
                (
                    scanline(), sg_scanf_fmt_ctrl_out, &sequence,
                    &a[0], &a[1], &a[2], &a[3], &a[4],
                    &b[0], &b[1], &b[2], &b[3], &b[4],
                    &c[0], &c[1], &c[2], &c[3], &c[4],
                    &d[0], &d[1], &d[2], &d[3], &d[4]
                );

                /*
                 *  Offset to avoid the deprecated enabled and channel values.
                 */

                mco.set_seq_event(i, midicontrolout::seqaction::arm, &a[2]);
                mco.set_seq_event(i, midicontrolout::seqaction::mute, &b[2]);
                mco.set_seq_event(i, midicontrolout::seqaction::queue, &c[2]);
                mco.set_seq_event(i, midicontrolout::seqaction::remove, &d[2]);
                if (i < (sequences - 1) && ! next_data_line(file))
                {
                    make_error_message
                    (
                        "midi-control-out version 1", "insufficient data"
                    );
                    break;
                }
            }
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

                mco.set_seq_event(i, midicontrolout::seqaction::arm, a);
                mco.set_seq_event(i, midicontrolout::seqaction::mute, b);
                mco.set_seq_event(i, midicontrolout::seqaction::queue, c);
                mco.set_seq_event(i, midicontrolout::seqaction::remove, d);
                if (i < (sequences - 1) && ! next_data_line(file))
                {
                    make_error_message("midi-control-out", "insufficient data");
                    break;
                }
            }
        }

        /*
         *  If enabled, this adds two section markers and one section for
         *  mutes, similar to the ctrl-pair options that follow this clause.
         */

        bool ok = true;
        bool mute_out_enabled = version_number() > 0;
        if (mute_out_enabled)
        {
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
        }

        /* Non-sequence (automation) actions */

        if (version_number() >= 3)
        {
            if (ok)
                ok = read_ctrl_triple(file, mco, midicontrolout::uiaction::panic);

            if (ok)
                ok = read_ctrl_triple(file, mco, midicontrolout::uiaction::stop);

            if (ok)
                ok = read_ctrl_triple(file, mco, midicontrolout::uiaction::pause);

            if (ok)
                ok = read_ctrl_triple(file, mco, midicontrolout::uiaction::play);
        }
        if (version_number() >= 3)
        {
            if (ok)
            {
                ok = read_ctrl_triple
                (
                    file, mco, midicontrolout::uiaction::toggle_mutes
                );
            }
            if (ok)
            {
                ok = read_ctrl_triple
                (
                    file, mco, midicontrolout::uiaction::song_record
                );
            }
            if (ok)
            {
                ok = read_ctrl_triple
                (
                    file, mco, midicontrolout::uiaction::slot_shift
                );
            }
            if (ok)
                ok = read_ctrl_triple(file, mco, midicontrolout::uiaction::free);
        }
        if (ok)
            ok = read_ctrl_triple(file, mco, midicontrolout::uiaction::queue);

        if (ok)
            ok = read_ctrl_triple(file, mco, midicontrolout::uiaction::oneshot);

        if (ok)
            ok = read_ctrl_triple(file, mco, midicontrolout::uiaction::replace);

        if (ok)
            ok = read_ctrl_triple(file, mco, midicontrolout::uiaction::snap);

        if (ok)
            ok = read_ctrl_triple(file, mco, midicontrolout::uiaction::song);

        if (ok)
            read_ctrl_triple(file, mco, midicontrolout::uiaction::learn);

        if (version_number() >= 3)
        {
            if (ok)
                read_ctrl_triple(file, mco, midicontrolout::uiaction::bpm_up);

            if (ok)
                read_ctrl_triple(file, mco, midicontrolout::uiaction::bpm_dn);

            if (ok)
                read_ctrl_triple(file, mco, midicontrolout::uiaction::list_up);

            if (ok)
                read_ctrl_triple(file, mco, midicontrolout::uiaction::list_dn);

            if (ok)
                read_ctrl_triple(file, mco, midicontrolout::uiaction::song_up);

            if (ok)
                read_ctrl_triple(file, mco, midicontrolout::uiaction::song_dn);

            if (ok)
                read_ctrl_triple(file, mco, midicontrolout::uiaction::set_up);

            if (ok)
                read_ctrl_triple(file, mco, midicontrolout::uiaction::set_dn);

            if (ok)
                read_ctrl_triple(file, mco, midicontrolout::uiaction::tap_bpm);

            if (ok)
                read_ctrl_triple(file, mco, midicontrolout::uiaction::free_2);
        }
        if (! ok)
        {
             (void) make_error_message
             (
                "midi-control-out", "not enough control-pairs provided"
            );
        }
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
midicontrolfile::read_ctrl_triple
(
    std::ifstream & file,
    midicontrolout & mco,
    midicontrolout::uiaction a
)
{
    int enabled, ev_on[4], ev_off[4], ev_del[4];
    if (version_number() < 2)
    {
        int count = std::sscanf
        (
            scanline(), sg_scanf_fmt_ctrl_pair,
            &enabled, &ev_on[0], &ev_on[1], &ev_on[2], &ev_on[3],
            &ev_off[0], &ev_off[1], &ev_off[2], &ev_off[3]
        );
        ev_del[0] = ev_del[1] = ev_del[2] = ev_del[3] = 0;
        if (count < 9)
            ev_off[0] = ev_off[1] = ev_off[2] = ev_off[3] = 0;

        mco.set_event(a, enabled, &ev_on[1], &ev_off[1], &ev_del[1]);
    }
    else
    {
        int count = std::sscanf
        (
            scanline(), sg_scanf_fmt_ctrl_triple, &enabled,
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
    if (version_number() < 2)
    {
        /*
         * Index 0 is the channel, which has been removed.  We pass in only
         * the last three values.
         */

        int number, ev_on[4], ev_off[4], ev_del[4];
        int count = std::sscanf
        (
            scanline(), sg_scanf_fmt_mutes_triple_1, &number,
            &ev_on[0],  &ev_on[1],  &ev_on[2],  &ev_on[3],
            &ev_off[0], &ev_off[1], &ev_off[2], &ev_off[3],
            &ev_del[0], &ev_del[1], &ev_del[2], &ev_del[3]
        );
        if (count < 9)
            ev_off[0] = ev_off[1] = ev_off[2] = ev_off[3] = 0;

        if (count < 13)
            ev_del[0] = ev_del[1] = ev_del[2] = ev_del[3] = 0;

        mco.set_mutes_event(group, &ev_on[1], &ev_off[1], &ev_del[1]);
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
    file << "# Seq66 0.92.0 (and above) MIDI control configuration file\n"
        << "#\n"
        << "# " << name() << "\n"
        << "# Written on " << current_date_time() << "\n"
        << "#\n"
        <<
    "# This file holds the MIDI control configuration for Seq66. It follows\n"
    "# the format of the 'rc' configuration file, but is stored separately for\n"
    "# flexibility.  It is always stored in the main configuration directory.\n"
    "# To use this file, replace the [midi-control] section in the 'rc' file,\n"
    "# and its contents with a [midi-control-file] tag, and simply add the\n"
    "# basename (e.g. nanomap.ctrl) on a separate line.\n"
    "\n"
    "# Version 1 adds the [mute-control-out] and [automation-control-out]\n"
    "# sections. Versions 2 and 3 simplify the data items.\n"
    "\n"
    "[Seq66]\n\n"
    "config-type = \"ctrl\"\n"
    "version = " << version() << "\n"
        ;

    /*
     * [comments]
     */

    file << "\n"
    "# [comments] holds the user's documentation for this control file.\n"
    "# Lines starting with '#' and '[' are ignored.  Blank lines are ignored;\n"
    "# add an empty line by adding a space character to the line.\n"
        ;

    std::string s = rc_ref().midi_control_in().comments_block().text();
    file << "\n[comments]\n\n" << s;

    bool result = write_midi_control(file);
    if (result)
        result = write_midi_control_out(file);

    if (result)
    {
        file
            << "\n# End of " << name() << "\n#\n"
            << "# vim: sw=4 ts=4 wm=4 et ft=dosini\n"
            ;
    }
    else
        file_error("Write fail", name());

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
            file_message("Writing 'ctrl'", name());
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
 *  Writes the [midi-control] section to the given file stream.  This can also
 *  be called by the rcfile object to just dump the data into that file.
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
        bool disabled = mci.is_disabled();
        int bb = int(mci.nominal_buss());
        std::string k(bool_to_string(rc_ref().load_key_controls()));
        std::string m(bool_to_string(rc_ref().load_midi_control_in()));
        file <<
        "\n[midi-control-settings]\n\n"
        "# Setting 'load-midi-control' to 'false' will cause an empty MIDI\n"
        "# control setup to be written!  Keep backups! The control-buss value\n"
        "# ranges from 0 to the maximum system buss provided by the hardware.\n"
        "# If set, then only that buss will be allowed to send MIDI control.\n"
        "# A value of 255 (0xFF) means any buss can send MIDI control.\n"
        "# The 'midi-enabled' flag applies to the MIDI controls; keystrokes\n"
        "# are always enabled. Supported keyboard layouts are 'qwerty' (the\n"
        "# default), 'qwertz', and 'azerty'. AZERTY turns off the auto-shift\n"
        "# feature for group-learn.\n\n"
            << "load-key-controls = " << k << "\n"
            << "load-midi-controls = " << m << "\n"
            ;

        if (bb >= c_busscount_max)
            file << "control-buss = 0xFF\n";
        else
            file << "control-buss = " << bb << "\n";

        file
            << "midi-enabled = " << (disabled ? "false" : "true") << "\n"
            << "button-offset = " << mci.offset() << "\n"
            << "button-rows = " << mci.rows() << "\n"
            << "button-columns = " << mci.columns() << "\n"
            << "keyboard-layout = " <<
                rc_ref().key_controls().kbd_layout_to_string() << "\n";
            ;
        file <<
        "\n"
        "# This style of control stanza incorporates key control as well,\n"
        "# but keys support only 'toggle', and key-release is an 'invert'.\n"
        "# The leftmost number on each line here is the pattern number (e.g.\n"
        "# 0 to 31); the group number, same range, for up to 32 groups; or it\n"
        "# it is an automation control number, again a similar range.\n"
        "# This internal MIDI control number is followed by three groups of\n"
        "# bracketed numbers, each providing three different type of control:\n"
        "#\n"
        "#    Normal:           [toggle]    [on]        [off]\n"
        "#    Increment/Decr:   [increment] [increment] [decrement]\n"
        "#    Playback:         [pause]     [start]     [stop]\n"
        "#    Playlist/Song:    [by-value]  [next]      [previous]\n"
        "#\n"
        "# In each group, there are six numbers:\n"
        "#\n"
        "#    [on/off invert status d0 d1min d1max]\n"
        ;

        file <<
        "#\n"
        "# 'on/off' enables/disables (1/0) the control for the pattern;\n"
        "# 'invert' (1/0) causes the opposite, but not all support this, and\n"
        "# all keystroke-releases set invert to true; 'status' is the MIDI\n"
        "# event to match (channel is NOT ignored), and if set to 0x00, the\n"
        "# control is disabled; 'd0' is the first data value, e.g. if status\n"
        "# is 0x90 (Note On), d0 represents the note number; d1min to d1max\n"
        "# is the range of data values detected, e.g. for a Note On, 1 to 127\n"
        "# indicate that any non-zero velocity will invoke the control.\n"
        "# Hex values can be used; precede with '0x'.\n"
        "#\n"
        "#  ------------------------- Loop, group, or automation-slot number\n"
        "# |    ---------------------- Name of the key (see the key map)\n"
        "# |   |\n"
        "# |   |      ---------------- Inverse\n"
        "# |   |     |  -------------- MIDI status/event byte (e.g. Note On)\n"
        "# |   |     | |  ------------ d0: Data 1 (e.g. Note number)\n"
        "# |   |     | | |  ---------- d1max: Data 2 min (e.g. Note velocity)\n"
        "# |   |     | | | |  -------- d1min: Data 2 max\n"
        "# |   |     | | | | |\n"
        "# v   v     v v v v v\n"
        "# 0 \"F1\" [0 0 0 0 0 0]   [0 0 0 0 0 0]   [0 0 0 0 0 0]\n"
        "#           Toggle          On              Off\n"
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
            int spacing = 8 - int(stan.key_name().size());
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
midicontrolfile::write_ctrl_triple
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
        << std::setw(2) << group << " "
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
    int buss = int(mco.nominal_buss());
    bool disabled = mco.is_disabled();
    bool result = buss >= 0;                /* do a very light sanity check */
    if (result)
    {
        if (setsize == 0)
        {
            mco.initialize
            (
                buss, SEQ66_DEFAULT_SET_ROWS, SEQ66_DEFAULT_SET_COLUMNS
            );
            setsize = mco.screenset_size();
        }
        file <<
            "\n"
            "[midi-control-out-settings]\n\n"
            << "set-size = " << setsize << "\n"
            << "output-buss = " << buss << "\n"
            << "midi-enabled = " << (disabled ? "false" : "true") << "\n"
            << "button-offset = " << mco.offset() << "\n"
            << "button-rows = " << mco.rows() << "\n"
            << "button-columns = " << mco.columns() << "\n"
            ;

        file <<
            "\n"
            "[midi-control-out]\n"
            "\n"
            "#   --------------------- Pattern number (as applicable)\n"
            "#  |     ---------------- MIDI status+channel (e.g. Note On)\n"
            "#  |    |    ------------ data 1 (e.g. note number)\n"
            "#  |    |   |  ---------- data 2 (e.g. velocity)\n"
            "#  |    |   | |\n"
            "#  v    v   v v\n"
            "# 31 [ 0x00 0 0 ] [ 0x00 0 0 ] [ 0x00 0 0 ] [ 0x00 0 0]\n"
            "#       Arm      Mute      Queue    Delete\n"
            ;

        file <<
            "#\n"
            "# This is a change (2021-02-10) from version 1 of this file.\n"
            "# A test of the status/event byte determines the enabled status,\n"
            "# and channel is incorporated into the status.  Much cleaner!\n"
            "# The order of the lines that follow must must be preserved.\n"
            "\n"
            ;

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
                file << std::setw(2) << seq << std::setw(0);
                for (int a = minimum; a < maximum; ++a)
                {
                    event ev = mco.get_seq_event(seq, midicontrolout::seqaction(a));
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
            "# The format of the mute and automation output events is simpler:\n"
            "#\n"
            "#  ---------------------- mute-group number\n"
            "# |    ------------------ MIDI status+channel (e.g. Note On)\n"
            "# |   |    -------------- data 1 (e.g. note number)\n"
            "# |   |   |  ------------ data 2 (e.g. velocity)\n"
            "# |   |   | |\n"
            "# v   v   v v\n"
            "# 1 [0x00 0 0 ] [0x00 0 0] [0x00 0 0]\n"
            "#       On         Off      Empty (dark)\n"
            "#\n"
            "# The mute-controls have an additional stanza for non-populated\n"
            "# (\"deleted\") mute-groups.\n"
            "\n"
            ;

        for (int m = 0; m < mutegroups::Size(); ++m)
        {
            if (! write_mutes_triple(file, mco, m))
                break;
        }

        file << "\n[automation-control-out]\n\n"
            "# This format is similar to the [mute-control-out] format, but\n"
            "# the first number is an active-flag, not an index number.\n"
            "# The stanzas are on/off/inactive, except for 'snap', which is\n"
            "# store/restore/inactive.\n\n"
            ;
        write_ctrl_triple(file, mco, midicontrolout::uiaction::panic);
        write_ctrl_triple(file, mco, midicontrolout::uiaction::stop);
        write_ctrl_triple(file, mco, midicontrolout::uiaction::pause);
        write_ctrl_triple(file, mco, midicontrolout::uiaction::play);
        write_ctrl_triple(file, mco, midicontrolout::uiaction::toggle_mutes);
        write_ctrl_triple(file, mco, midicontrolout::uiaction::song_record);
        write_ctrl_triple(file, mco, midicontrolout::uiaction::slot_shift);
        write_ctrl_triple(file, mco, midicontrolout::uiaction::free);
        write_ctrl_triple(file, mco, midicontrolout::uiaction::queue);
        write_ctrl_triple(file, mco, midicontrolout::uiaction::oneshot);
        write_ctrl_triple(file, mco, midicontrolout::uiaction::replace);
        write_ctrl_triple(file, mco, midicontrolout::uiaction::snap);
        write_ctrl_triple(file, mco, midicontrolout::uiaction::song);
        write_ctrl_triple(file, mco, midicontrolout::uiaction::learn);
        write_ctrl_triple(file, mco, midicontrolout::uiaction::bpm_up);
        write_ctrl_triple(file, mco, midicontrolout::uiaction::bpm_dn);
        write_ctrl_triple(file, mco, midicontrolout::uiaction::list_up);
        write_ctrl_triple(file, mco, midicontrolout::uiaction::list_dn);
        write_ctrl_triple(file, mco, midicontrolout::uiaction::song_up);
        write_ctrl_triple(file, mco, midicontrolout::uiaction::song_dn);
        write_ctrl_triple(file, mco, midicontrolout::uiaction::set_up);
        write_ctrl_triple(file, mco, midicontrolout::uiaction::set_dn);
        write_ctrl_triple(file, mco, midicontrolout::uiaction::tap_bpm);
        write_ctrl_triple(file, mco, midicontrolout::uiaction::free_2);
    }
    return result;
}

/**
 *  For automation, slot and code are the same numeric value.
 */

bool
midicontrolfile::parse_control_stanza (automation::category opcat)
{
    bool result = true;
    char charname[16];
    int opcode = 0;
    int a[8], b[8], c[8];
    automation::slot opslot = automation::slot::none;
    std::string keyname;
    if (version_number() < 2)
    {
        int count = std::sscanf
        (
            scanline(), sg_scanf_fmt_ctrl_in, &opcode, &charname[0],
            &a[0], &a[1], &a[2], &a[3], &a[4], &a[5],
            &b[0], &b[1], &b[2], &b[3], &b[4], &b[5],
            &c[0], &c[1], &c[2], &c[3], &c[4], &c[5]
        );
        if (count == 20)
        {
            opslot = automation::slot::none;
            if (opcat == automation::category::loop)
                opslot = automation::slot::loop;
            else if (opcat == automation::category::mute_group)
                opslot = automation::slot::mute_group;
            else if (opcat == automation::category::automation)
                opslot = opcontrol::set_slot(opcode);

            /*
             *  Create control objects, whether active or not.  We want to save
             *  all objects in the file, to avoid altering the user's preferences.
             */

            keyname = strip_quotes(std::string(charname));
            midicontrol mca
            (
                keyname, opcat, automation::action::toggle, opslot, opcode
            );
            mca.set(&a[1]);
            (void) m_temp_midi_ctrl_in.add(mca);

            midicontrol mcb
            (
                keyname, opcat, automation::action::on, opslot, opcode
            );
            mcb.set(&b[1]);
            (void) m_temp_midi_ctrl_in.add(mcb);

            midicontrol mcc
            (
                keyname, opcat, automation::action::off, opslot, opcode
            );
            mcc.set(&c[1]);
            (void) m_temp_midi_ctrl_in.add(mcc);
        }
        else
            result = false;
    }
    else
    {
        int count = std::sscanf
        (
            scanline(), sg_scanf_fmt_ctrl_in_2, &opcode, &charname[0],
            &a[0], &a[1], &a[2], &a[3], &a[4],
            &b[0], &b[1], &b[2], &b[3], &b[4],
            &c[0], &c[1], &c[2], &c[3], &c[4]
        );
        if (count == 17)
        {
            opslot = automation::slot::none;
            if (opcat == automation::category::loop)
                opslot = automation::slot::loop;
            else if (opcat == automation::category::mute_group)
                opslot = automation::slot::mute_group;
            else if (opcat == automation::category::automation)
                opslot = opcontrol::set_slot(opcode);

            /*
             *  Create control objects, whether active or not.  We want to save
             *  all objects in the file, to avoid altering the user's preferences.
             */

            keyname = strip_quotes(std::string(charname));
            midicontrol mca
            (
                keyname, opcat, automation::action::toggle, opslot, opcode
            );
            mca.set(a);
            (void) m_temp_midi_ctrl_in.add(mca);

            midicontrol mcb
            (
                keyname, opcat, automation::action::on, opslot, opcode
            );
            mcb.set(b);
            (void) m_temp_midi_ctrl_in.add(mcb);

            midicontrol mcc
            (
                keyname, opcat, automation::action::off, opslot, opcode
            );
            mcc.set(c);
            (void) m_temp_midi_ctrl_in.add(mcc);
        }
        else
            result = false;
    }
    if (result)
    {
        if (rc_ref().load_key_controls())
        {
            /*
             *  Make reverse-lookup map<pattern, keystroke> for
             *  use with show_ui functions.  It would be an addition to the
             *  keycontainer class.
             *
             *  keyname = key_controls().key_name(slotnumber);
             */

            keycontrol kc
            (
                "", keyname, opcat, automation::action::toggle, opslot, opcode
            );
            std::string keyname = strip_quotes(std::string(charname));
            ctrlkey ordinal = qt_keyname_ordinal(keyname);
            (void) m_temp_key_controls.add(ordinal, kc);
            if (opcat == automation::category::loop)
                (void) m_temp_key_controls.add_slot(kc);
            else if (opcat == automation::category::mute_group)
                (void) m_temp_key_controls.add_mute(kc);
        }
    }
    else
    {
        errprint("unexpected control count in stanza");
        result = false;
    }
    return result;
}

/**
 *  Note that midicontrolin is a multimap, and it can hold multiple
 *  midicontrols for a givem midicontrol::key, so that the same event can
 *  trigger multiple operations/actions.
 */

bool
midicontrolfile::container_to_stanzas (const midicontrolin & mc)
{
    bool result = mc.count() > 0;
    if (result)
    {
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
                stanza s(mco);                /* includes settings sect.  */
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

}           // namespace seq66

/*
 * midicontrolfile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


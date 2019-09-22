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
 * \updates       2019-06-19
 * \license       GNU GPLv2 or above
 *
 */

#include <iostream>                     /* std::cout                        */
#include <iomanip>                      /* std::setw()                      */

#include "cfg/midicontrolfile.hpp"      /* seq66::midicontrolfile class     */
#include "cfg/settings.hpp"             /* seq66::rc()                      */
#include "ctrl/keymap.hpp"              /* seq66::qt_keyname_ordinal()      */
#include "util/calculations.hpp"        /* seq66::string_to_bool(), etc.    */
#include "util/strfunctions.hpp"        /* seq66::strip_quotes()            */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 * -------------------------------------------------------------------------
 *  midicontrolfile nested classes
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

/**
 *
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

/**
 *
 */

bool
midicontrolfile::stanza::set (const midicontrol & mc)
{
    automation::action a = mc.action_code();
    if (a > automation::action::none && a < automation::action::maximum)
    {
        unsigned index = static_cast<int>(a) - 1;   /* skips "none"         */
        m_settings[index][0] = int(mc.active());
        m_settings[index][1] = int(mc.inverse_active());
        m_settings[index][2] = int(mc.status());
        m_settings[index][3] = int(mc.d0());        /* note: d1 not needed  */
        m_settings[index][4] = int(mc.min_value());
        m_settings[index][5] = int(mc.max_value());
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
 *
 * \param allowinactive
 *      If true, this object will allow inactive controls to be loaded into
 *      the map.
 */

midicontrolfile::midicontrolfile
(
    const std::string & filename,
    rcsettings & rcs,
    bool allowinactive
) :
    configfile              (filename, rcs),
    m_temp_key_controls     (),             /* used during reading only */
    m_temp_midi_controls    (),             /* used during reading only */
    m_stanzas               (),             /* fill from rcs in writing */
    m_allow_inactive        (allowinactive)
{
    // Empty body
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
 *  [midi-control-flags]
 *
 *      load-key-controls
 *      load-midi-controls
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
    file.seekg(0, std::ios::beg);                   /* seek to start    */

    std::string s = parse_comments(file);
    if (! s.empty())
        rc_ref().midi_controls().comments_block().set(s);

    s = get_variable(file, "[midi-control-flags]", "load-key-controls");
    rc_ref().load_key_controls(string_to_bool(s));

    s = get_variable(file, "[midi-control-flags]", "load-midi-controls");
    rc_ref().load_midi_controls(string_to_bool(s));

    bool loadmidi = rc_ref().load_midi_controls();
    bool loadkeys = rc_ref().load_key_controls();
    if (loadkeys)
        m_temp_key_controls.clear();

    if (loadmidi || loadkeys)
    {
        bool good = line_after(file, "[loop-control]");
        while (good)                        /* not at end of section?   */
        {
            if (! line().empty())           /* any value in section?    */
                good = parse_control_stanza(automation::category::loop);

            if (good)
                good = next_data_line(file);
        }

        good = line_after(file, "[mute-group-control]");
        while (good)                        /* not at end of section?   */
        {
            if (! line().empty())           /* any value in section?    */
                good = parse_control_stanza(automation::category::mute_group);

            if (good)
                good = next_data_line(file);
        }

        good = line_after(file, "[automation-control]");
        while (good)                        /* not at end of section?   */
        {
            if (! line().empty())           /* any value in section?    */
                good = parse_control_stanza(automation::category::automation);

            if (good)
                good = next_data_line(file);
        }
    }

    if (loadmidi && m_temp_midi_controls.count() > 0)
    {
        rc_ref().midi_controls().clear();
        rc_ref().midi_controls().inactive_allowed(m_allow_inactive);
        rc_ref().midi_controls() = m_temp_midi_controls;
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
 *  Gets the number of sequence definitions provided in the midi-control sections.
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
 *  It is not an error for the "[midi-contro-out]" section to be missing.
 */

bool
midicontrolfile::parse_midi_control_out (std::ifstream & file)
{
    bool result = true;

    /**
     * [midi-control-out]
     */

    if (line_after(file, "[midi-control-out]"))
    {
        std::string s = get_variable
        (
            file, "[midi-control-out-settings]", "set-size"
        );
        int sequences = string_to_int(s, SEQ66_DEFAULT_SET_SIZE);
        s = get_variable
        (
            file, "[midi-control-out-settings]", "buss"
        );

        int buss = string_to_int(s, SEQ66_MIDI_CONTROL_OUT_BUSS);
        s = get_variable
        (
            file, "[midi-control-out-settings]", "enabled"
        );

        bool enabled = string_to_bool(s);
        midicontrolout & mctrl = rc_ref().midi_control_out();
        mctrl.initialize(sequences, buss);
        mctrl.is_enabled(enabled);
        for (int i = 0; i < sequences; ++i)         /* Sequence actions     */
        {
            if (! next_data_line(file))
                return make_error_message("midi-control-out", "no data");

            int a[5], b[5], c[5], d[5];
            int sequence = 0;
            sscanf
            (
                scanline(),
                "%d [%d %d %d %d %d] [%d %d %d %d %d]"
                " [%d %d %d %d %d] [%d %d %d %d %d]",
                &sequence,
                &a[0], &a[1], &a[2], &a[3], &a[4],
                &b[0], &b[1], &b[2], &b[3], &b[4],
                &c[0], &c[1], &c[2], &c[3], &c[4],
                &d[0], &d[1], &d[2], &d[3], &d[4]
            );
            mctrl.set_seq_event(i, midicontrolout::seqaction::arm, a);
            mctrl.set_seq_event(i, midicontrolout::seqaction::mute, b);
            mctrl.set_seq_event(i, midicontrolout::seqaction::queue, c);
            mctrl.set_seq_event(i, midicontrolout::seqaction::remove, d);
        }

        /* Non-sequence actions */

        read_ctrl_event(file, mctrl, midicontrolout::action::play);
        read_ctrl_event(file, mctrl, midicontrolout::action::stop);
        read_ctrl_event(file, mctrl, midicontrolout::action::pause);
        read_ctrl_pair
        (
            file, mctrl,
            midicontrolout::action::queue_on,
            midicontrolout::action::queue_off
        );
        read_ctrl_pair
        (
            file, mctrl,
            midicontrolout::action::oneshot_on,
            midicontrolout::action::oneshot_off
        );
        read_ctrl_pair
        (
            file, mctrl,
            midicontrolout::action::replace_on,
            midicontrolout::action::replace_off
        );
        read_ctrl_pair
        (
            file, mctrl,
            midicontrolout::action::snap1_store,
            midicontrolout::action::snap1_restore
        );
        read_ctrl_pair
        (
            file, mctrl,
            midicontrolout::action::snap2_store,
            midicontrolout::action::snap2_restore
        );
        read_ctrl_pair
        (
            file, mctrl,
            midicontrolout::action::learn_on,
            midicontrolout::action::learn_off
        );
        result = ! is_error();
    }
    else
        rc_ref().midi_control_out().is_enabled(false);  /* blank section    */

    return result;
}

/**
 *
 */

void
midicontrolfile::read_ctrl_pair
(
    std::ifstream & file,
    midicontrolout & mctrl,
    midicontrolout::action a1,
    midicontrolout::action a2
)
{
    if (next_data_line(file))
    {
        int ev_on[5], ev_off[5];
        sscanf
        (
            scanline(), "%d [%d %d %d %d] [%d %d %d %d]",
            &ev_on[0], &ev_on[1], &ev_on[2], &ev_on[3], &ev_on[4],
            &ev_off[1], &ev_off[2], &ev_off[3], &ev_off[4]
        );
        ev_off[0] = ev_on[0];
        mctrl.set_event(a1, ev_on);
        mctrl.set_event(a2, ev_off);
    }
    else
        (void) make_error_message("midi-control-out", "missing data");
}

/**
 *
 */

void
midicontrolfile::read_ctrl_event
(
    std::ifstream & file,
    midicontrolout & mctrl,
    midicontrolout::action a
)
{
    if (next_data_line(file))
    {
        int v[5];
        sscanf(scanline(), "%d [%d %d %d %d]", &v[0], &v[1], &v[2], &v[3], &v[4]);
        mctrl.set_event(a, v);
    }
    else
        (void) make_error_message("midi-control-out", "missing data");
}

/**
 *
 */

void
midicontrolfile::write_ctrl_event
(
    std::ofstream & file,
    midicontrolout & mctrl,
    midicontrolout::action a
)
{
    bool active = mctrl.event_is_active(a);
    std::string activestr = mctrl.get_event_str(a);
    file
        << "# MIDI Control Out: " << action_to_string(a) << "\n"
        << (active ? "1" : "0") << " " << activestr << "\n\n"
        ;
}

/**
 *
 */

bool
midicontrolfile::write_stream (std::ofstream & file)
{
    file << "# Seq66 0.90.0 (and above) MIDI control configuration file\n"
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
    "[Sequencer66]\n\n"
    "config-type = \"ctrl\"\n"
    "version = 0\n"
        ;

    /*
     * [comments]
     */

    file << "\n"
    "# The [comments] section holds the user's documentation for this file.\n"
    "# Lines starting with '#' and '[' are ignored.  Blank lines are ignored;\n"
    "# add a blank line by adding a space character to the line.\n"
        ;

    file << "\n[comments]\n\n" << rc_ref().comments_block().text();

    bool result = write_midi_control(file);
    if (result)
    {
        file
            << "\n\n# End of " << name() << "\n#\n"
            << "# vim: sw=4 ts=4 wm=4 et ft=dosini\n"
            ;
    }
    else
        file_error("failed to write", name());

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
        result = container_to_stanzas(rc_ref().midi_controls());
        if (result)
        {
            pathprint("Writing MIDI control configuration", name());
            result = write_stream(file);
        }
        file.close();
    }
    else
    {
        file_error("Error opening for writing", name());
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
    using namespace std;                /* easier access to setw(), etc.    */
    bool result = file.is_open();
    if (result)
    {
        std::string k(bool_string(rc_ref().load_key_controls()));
        std::string m(bool_string(rc_ref().load_midi_controls()));
        file
            << "\n[midi-control-flags]\n\n"
            << "load-key-controls = " << k << "\n"
            << "load-midi-control = " << m << "\n"
            ;

        file <<
        "\n"
        "# This new style of control stanza incorporates key control as well.\n"
        "# The leftmost number on each line here is the pattern number (e.g.\n"
        "# 0 to 31); the group number, same range, for up to 32 groups; or it\n"
        "# it is an automation control number, again a similar range.\n"
        "# This internal MIDI control number is followed by three groups of\n"
        "# bracketed numbers, each providing three different type of control:\n"
        "#\n"
        "#    Normal:           [toggle]    [on]      [off]\n"
        "#    Playback:         [pause]     [start]   [stop]\n"
        "#    Playlist:         [by-value]  [next]    [previous] (if active)\n"
        "#\n"
        "# In each group, there are six numbers:\n"
        "#\n"
        "#    [on/off invert status d0 d1min d1max]\n"
        ;

        file <<
        "#\n"
        "# 'on/off' enables/disables (1/0) the MIDI control for the pattern.\n"
        "# 'invert' (1/0) causes the opposite if data is outside the range.\n"
        "# 'status' is by MIDI event to match (channel is NOT ignored).\n"
        "# 'd0' is the first data value.  Example: if status is 144 (Note On),\n"
        "# then d0 represents Note 0.\n"
        "#\n"
        "# 'd1min'/'d1max' are the range of second values that should match.\n"
        "# Example:  For a Note On for note 0, 0 and 127 indicate that any\n"
        "# Note On velocity will cause the MIDI control to take effect.\n"
        "#\n"
        "#  ------------------------- Loop, group, or automation-slot number\n"
        "# |   ---------------------- Name of the key (see the key map)\n"
        "# |  |    ------------------ On/off (indicate if section is enabled)\n"
        "# |  |   | ----------------- Inverse\n"
        "# |  |   | |  -------------- MIDI status (event) byte (e.g. Note On)\n"
        "# |  |   | | |  ------------ Data 1 (e.g. Note number)\n"
        "# |  |   | | | |  ---------- Data 2 min\n"
        "# |  |   | | | | |  -------- Data 2 max\n"
        "# |  |   | | | | | |\n"
        "# v  v   v v v v v v\n"
        "# 0 \"1\" [0 0 0 0 0 0]   [0 0 0 0 0 0]   [0 0 0 0 0 0]\n"
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
                << setw(2) << stan.slot_number()
                << " \"" << stan.key_name() << "\""
                << setw(spacing) << " "
                ;
            for (int action = 0; action < automation::ACTCOUNT; ++action)
            {
                file
                    << "["
                    << setw(2) << stan.setting(action, 0)       /* active   */
                    << setw(2) << stan.setting(action, 1)       /* inverse  */
                    << " 0x" << setw(2) << setfill('0')
                    << hex << stan.setting(action, 2)           /* status   */
                    << setw(4) << setfill(' ')
                    << dec << stan.setting(action, 3)           /* d0       */
                    << setw(4)
                    << dec << stan.setting(action, 4)           /* min      */
                    << setw(4)
                    << dec << stan.setting(action, 5)           /* max      */
                    << " ] ";
            }
            file << " # " << stan.op_name() << endl;
        }
    }
    return result;
}

/**
 *
 */

void
midicontrolfile::write_ctrl_pair
(
    std::ofstream & file,
    const midicontrolout & mctrl,
    midicontrolout::action a1,
    midicontrolout::action a2
)
{
    bool active = mctrl.event_is_active(a1);
    std::string act1str = mctrl.get_event_str(a1);
    std::string act2str = mctrl.get_event_str(a2);
    file
        << "# MIDI Control Out: " << action_to_string(a1) << "/opposite\n"
        << (active ? "1" : "0") << " "
        << act1str << " " << act2str << "\n\n"
        ;
}

/**
 *
 */

bool
midicontrolfile::write_midi_control_out (std::ofstream & file)
{
    bool result = false;
    midicontrolout & mco = rc_ref().midi_control_out();
    if (mco.is_blank())
        return true;

    int setsize = mco.screenset_size();
    int buss = int(mco.buss());
    bool disabled = mco.is_disabled();
    if (! disabled && mco.is_blank())
        disabled = true;

    file <<
        "\n"
        "[midi-control-out-settings]\n"
        "\n"
        << "set-size = " << mco.screenset_size()
        << "buss = " << mco.buss()
        << "enabled = " << (mco.is_enabled() ? "true" : "false")
        ;

    file <<
        "\n"
        "[midi-control-out]\n"
        "\n"
        "#    ------------------- on/off (indicate is the section is enabled)\n"
        "#    | ----------------- MIDI channel (0-15)\n"
        "#    | | --------------- MIDI status (event) byte (e.g. note on)\n"
        "#    | | | ------------- data 1 (e.g. note number)\n"
        "#    | | | | ----------- data 2 (e.g. velocity)\n"
        "#    | | | | |\n"
        "#    v v v v v\n"
        "#   [0 0 0 0 0] [0 0 0 0 0] [0 0 0 0 0] [0 0 0 0 0]\n"
        "#       Arm         Mute       Queue      Delete\n"
        "\n"
        << setsize << " " << buss << " " << (disabled ? "0" : "1")
        << "     # screenset size, output buss, enabled (1) /disabled (0)\n\n"
        ;

    for (int seq = 0; seq < setsize; ++seq)
    {
        file << seq;
        for
        (
            int a = static_cast<int>(midicontrolout::seqaction::arm);
            a < static_cast<int>(midicontrolout::seqaction::max);
            ++a
        )
        {
            event ev = mco.get_seq_event(seq, midicontrolout::seqaction(a));
            bool active = mco.seq_event_is_active
            (
                seq, midicontrolout::seqaction(a)
            );
            midibyte d0, d1;

            /*
             * bool eia = mco.seq_event_is_active
             * (
             *     seq, (midicontrolout::seqaction) a
             * );
             */

            ev.get_data(d0, d1);
            file
                << " ["
                << (active ? "1" : "0") << " "
                << unsigned(ev.channel()) << " "
                << unsigned(ev.get_status()) << " "
                << unsigned(d0) << " "
                << unsigned(d1)
                << "]"
                ;
        }
        file << "\n";
    }

    file << "\n";

    write_ctrl_event(file, mco, midicontrolout::action::play);
    write_ctrl_event(file, mco, midicontrolout::action::stop);
    write_ctrl_event(file, mco, midicontrolout::action::pause);
    write_ctrl_pair
    (
        file, mco, midicontrolout::action::queue_on,
        midicontrolout::action::queue_off
    );
    write_ctrl_pair
    (
        file, mco, midicontrolout::action::oneshot_on,
        midicontrolout::action::oneshot_off
    );
    write_ctrl_pair
    (
        file, mco, midicontrolout::action::replace_on,
        midicontrolout::action::replace_off
    );
    write_ctrl_pair
    (
        file, mco, midicontrolout::action::snap1_store,
        midicontrolout::action::snap1_restore
    );
    write_ctrl_pair
    (
        file, mco, midicontrolout::action::snap2_store,
        midicontrolout::action::snap2_restore
    );
    write_ctrl_pair
    (
        file, mco, midicontrolout::action::learn_on,
        midicontrolout::action::learn_off
    );
    return result;
}

/**
 *  This format statement assumes the active and inverse flags are a single
 *  digit (0 or 1), and that the rest of the values can optionally be preceded
 *  by "0x" (which is a better format for displaying event statuses).
 *
 *  The "%10s" specifier scans for up to 10 non-whitespace characters.  The
 *  double-quotes are stripped off after reading the key's name.
 */

static const char * const sg_scanf_fmt_1 =
    "%d %10s [ %d %d %i %i %i %i ] [ %d %d %i %i %i %i ] [ %d %d %i %i %i %i ]";

/**
 *  For automation, slot and code are the same numeric value.
 */

bool
midicontrolfile::parse_control_stanza (automation::category opcat)
{
    bool result = true;
    char charname[16];
    int opcode = 0;
    int a[6], b[6], c[6];
    int count = std::sscanf
    (
        scanline(),
        sg_scanf_fmt_1,
        &opcode, &charname[0],
        &a[0], &a[1], &a[2], &a[3], &a[4], &a[5],
        &b[0], &b[1], &b[2], &b[3], &b[4], &b[5],
        &c[0], &c[1], &c[2], &c[3], &c[4], &c[5]
    );
    if (count == 20)
    {
        automation::slot opslot = automation::slot::none;
        if (opcat == automation::category::loop)
            opslot = automation::slot::loop;
        else if (opcat == automation::category::mute_group)
            opslot = automation::slot::mute_group;
        else if (opcat == automation::category::automation)
            opslot = opcontrol::set_slot(opcode);

        /*
         *  Create control objects, if active, using an empty
         *  name to generate the "Pattern" name.
         */

        std::string kn = strip_quotes(std::string(charname));
        if (a[0] != 0 || m_allow_inactive)
        {
            midicontrol mc(kn, opcat, automation::action::toggle, opslot, opcode);
            mc.set(a);
            (void) m_temp_midi_controls.add(mc);
        }
        if (b[0] != 0 || m_allow_inactive)
        {
            midicontrol mc(kn, opcat, automation::action::on, opslot, opcode);
            mc.set(b);
            (void) m_temp_midi_controls.add(mc);
        }
        if (c[0] != 0 || m_allow_inactive)
        {
            midicontrol mc(kn, opcat, automation::action::off, opslot, opcode);
            mc.set(c);
            (void) m_temp_midi_controls.add(mc);
        }
        if (result && rc_ref().load_key_controls())
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
                "", kn, opcat, automation::action::toggle, opslot, opcode
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
 *  Note that midicontainer is a multimap, and it can hold multiple
 *  midicontrols for a givem midicontrol::key, so that the same event can
 *  trigger multiple operations/actions.
 */

bool
midicontrolfile::container_to_stanzas (const midicontainer & mc)
{
    bool result = mc.count() > 0;
    if (result)
    {
#if defined SEQ66_PLATFORM_DEBUG_TMI
        std::cout
            << "midicontrolfile::container_to_stanzas(): input size = "
            << mc.count() << std::endl
            ;
#endif
        for (const auto & m : mc.container())
        {
            const midicontrol & mctrl = m.second;
            key k(mctrl);
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

#if defined SEQ66_PLATFORM_DEBUG_TMI
                std::cout
                    << "stanza key " << k.category_name()
                    << " control code " << k.slot_control()
                    << " already in place"
                    << std::endl
                    ;
#endif
                stanziter->second.set(mctrl);
                ok = true;                      /* points to the found one  */
            }
            else
            {
                stanza s(mctrl);                /* includes settings sect.  */
                auto sz = m_stanzas.size();
                auto p = std::make_pair(k, s);
                (void) m_stanzas.insert(p);
                ok = m_stanzas.size() == (sz + 1);
#if defined SEQ66_PLATFORM_DEBUG_TMI
                if (ok)
                {
                    std::cout
                        << "stanza key " << k.category_name()
                        << " control code " << k.slot_control() << " added"
                        ;
                }
                else
                {
                    std::cout
                        << "stanza key " << k.category_name()
                        << " control code " << k.slot_control() << " NOT ADDED"
                        ;
                }
                std::cout << std::endl;
#endif
            }
            if (! ok)
            {
                errprint("couldn't update midicontrol:");
                mctrl.show(true);
                result = false;
                break;
            }
        }
#if defined SEQ66_PLATFORM_DEBUG_TMI
        std::cout
            << "midicontrolfile::container_to_stanzas(): output size = "
            << m_stanzas.size() << std::endl
            ;
#endif
    }
    return result;
}

/**
 *
 */

void
midicontrolfile::show_stanza (const stanza & stan) const
{
    using namespace std;
    cout
        << "[" << stan.category_name() << "-control] "
        << "'" << setw(7) << stan.key_name() << "'"
        << " " << setw(2) << stan.slot_number() << " "
        ;

    for (int action = 0; action < automation::ACTCOUNT; ++action)
    {
        cout
            << "["
            << setw(2) << stan.setting(action, 0)       /* active           */
            << setw(2) << stan.setting(action, 1)       /* inverse active   */
            << " 0x" << setw(2) << setfill('0')
            << hex << stan.setting(action, 2)           /* status           */
            << setw(4) << setfill(' ')
            << dec << stan.setting(action, 3)           /* d0               */
            << setw(4)
            << dec << stan.setting(action, 4)           /* min              */
            << setw(4)
            << dec << stan.setting(action, 5)           /* max              */
            << " ] ";
    }
    cout << stan.op_name() << endl;
}

/**
 *
 */

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


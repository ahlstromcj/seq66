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
 * \file          opcontrol.cpp
 *
 *  This module declares/defines the opcontrol base class for the keycontrol
 *  class and the extended MIDI control feature.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-12-04
 * \updates       2023-08-18
 * \license       GNU GPLv2 or above
 *
 */

#include "ctrl/opcontrol.hpp"           /* seq66::opcontrol base class      */
#include "util/basic_macros.hpp"        /* seq66::tokenization alias        */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

using namespace automation;

/**
 *  This default constructor creates a "zero" object.  Every member is either
 *  false or some other form of zero.
 */

opcontrol::opcontrol () :
    m_name           (),
    m_category       (category::none),
    m_action         (action::none),
    m_slot_number    (slot::none)
{
    // Empty body
}

/**
 *  This constructor assigns the basic values of control name, number, and
 *  action code.  The rest of the members can be set via the set() function.
 */

opcontrol::opcontrol
(
    const std::string & opname,
    category opcategory,
    action opaction,
    slot opslot,
    int index                               /* for pattern, mutes only      */
) :
    m_name          (opname),
    m_category      (opcategory),
    m_action        (opaction),
    m_slot_number   (opslot)
{
    if (m_name.empty())
        m_name = build_slot_name(index);
}

/**
 *  Constructs the slot name based on the category and the operation number of
 *  this opcontrol object.  Note that this function is not static.  Also note
 *  that it works only for categories other than "none" or "max", which are
 *  just range-checkers.
 */

std::string
opcontrol::build_slot_name (int index) const
{
    std::string result;
    if (m_category == category::loop)
    {
        result = automation_slot_name(slot::loop);
        result += " ";
        result += std::to_string(static_cast<int>(index));
    }
    else if (m_category == category::mute_group)
    {
        result = automation_slot_name(slot::mute_group);
        result += " ";
        result += std::to_string(static_cast<int>(index));
    }
    else if (m_category == category::automation)
    {
        result = automation_slot_name(m_slot_number);
    }
    return result;
}

/**
 *  Access to the category names in a static function.  A static function to
 *  return the name of an operation category.  We keep these very short for
 *  readability in debugging dumps.
 */

std::string
opcontrol::category_name (category c)
{
    std::string result;
    switch (c)
    {
    case category::none:            result = "None";            break;
    case category::loop:            result = "Loop";            break;
    case category::mute_group:      result = "Mute";            break;
    case category::automation:      result = "Auto";            break;
    case category::max:             result = "Max";             break;
    }
    return result;
}

/**
 *  Access to the action names in a static function.  A static function to
 *  return the name of an action code.
 */

std::string
opcontrol::action_name (action a)
{
    std::string result;
    switch (a)
    {
    case action::none:              result = "None";            break;
    case action::toggle:            result = "Toggle";          break;
    case action::on:                result = "On";              break;
    case action::off:               result = "Off";             break;
    case action::max:               result = "Max";             break;
    }
    return result;
}

/**
 *  Access to the slot names in a static function.  A static function to
 *  return the base name of an operation slot.
 *
 *  Compare this list to the similar list in libseq66/src/ctrl/automation.cpp.
 *  and its header file.  *  They differ in letter case and (slightly)
 *  in numbering.
 *
 *  This list is meant mainly for decorating the configuration file and for
 *  human-readable reporting.
 *
 *  Note the asterisks; they indicate the slots having MIDI output display
 *  in the midicontrolout::uiaction enumeration.
 */

std::string
opcontrol::automation_slot_name (slot s)
{
    static tokenization s_slot_names =
    {
        "BPM Up",               //  0 bpm_up *
        "BPM Dn",               //  1 bpm_dn *
        "Set Up",               //  2 ss_up *
        "Set Dn",               //  3 ss_dn *
        "Replace",              //  4 mod_replace *
        "Snapshot",             //  5 mod_snapshot *
        "Queue",                //  6 mod_queue *
        "Group Mute",           //  7 mod_gmute
        "Group Learn",          //  8 mod_glearn *
        "Playing Set",          //  9 play_ss
        "Playback",             // 10 playback (pause) * * *
        "Song Record",          // 11 song_record *
        "Solo",                 // 12 solo
        "Thru",                 // 13 thru
        "BPM Page Up",          // 14 bpm_page_up
        "BPM Page Dn",          // 15 bpm_page_dn
        "Set Set",              // 16 ss_set
        "Record Style",         // 17 record_style (increment/decrement)
        "Quan Record",          // 18 quan_record
        "Reset Sets",           // 19 reset_sets
        "One-shot",             // 20 mod_oneshot *
        "FF",                   // 21 FF
        "Rewind",               // 22 rewind
        "Top",                  // 23 top (song beginning or L marker)
        "Play List",            // 24 playlist * *
        "Play Song",            // 25 playlist_song * *
        "Tap BPM",              // 26 tap_bpm *
        "Start",                // 27 start
        "Stop",                 // 28 stop ?
        "Loop L/R",             // 29 loop_LR
        "Toggle Mute",          // 30 toggle_mutes *
        "Song Pos",             // 31 song_pointer

        /*
         * The following add to what Seq64 supports.
         */

        "Keep Queue",           // 32 keep_queue
        "Slot Shift",           // 33 slot_shift *
        "Mutes Clear",          // 34 reserved_34
        "Quit",                 // 35 quit *
        "Loop Edit",            // 36 pattern_edit
        "Event Edit",           // 37 event_edit
        "Song Mode",            // 38 song_mode *
        "Toggle JACK",          // 39 toggle_jack
        "Menu Mode",            // 40 menu_mode
        "Follow JACK",          // 41 follow_transport
        "Panic",                // 42 panic *
        "Visibility",           // 43 visibility *
        "Save Session",         // 44 save_session
        "Reserved 45",          // 45 reserved_45
        "Reserved 46",          // 46 reserved_46
        "Reserved 47",          // 47 reserved_47
        "Reserved 48",          // 48 reserved_48

        /*
         * Record mode selection. Changed labels on 2023-03-18.
         */

        "Record Overdub",       // 49: record_overdub
        "Record Overwrite",     // 50: record_overwrite
        "Record Expand",        // 51: record_expand
        "Record Oneshot",       // 52: record_oneshot

        /*
         * Grid mode selection. Changed labels on 2023-03-18.
         */

        "Grid Loop",            // 53: grid_loop
        "Grid Record",          // 54: grid_record
        "Grid Copy",            // 55: grid_copy
        "Grid Paste",           // 56: grid_paste
        "Grid Clear",           // 57: grid_clear
        "Grid Delete",          // 58: grid_delete
        "Grid Thru",            // 59: grid_thru
        "Grid Solo",            // 60: grid_solo
        "Grid Cut",             // 61: grid_velocity
        "Grid Double",          // 62: grid_double

        /*
         * Grid quantization type selection.
         */

        "Q None",               // 63: grid_quant_none
        "Q Full",               // 64: grid_quant_full
        "Q Tighten",            // 65: grid_quant_tighten
        "Randomize",            // 66: grid_quant_random
        "Jitter",               // 67: grid_quant_jitter
        "Note-map",             // 68: grid_quant_notemap

        /*
         * A few more likely candidates.
         */

        "BBT/HMS",              // 69: mod_bbt_hms
        "LR Loop",              // 70: mod_LR_loop
        "Undo Record",          // 71: mod_undo_recording
        "Redo Record",          // 72: mod_redo_recording
        "Transpose Song",       // 73: mod_transpose_song
        "Copy Set",             // 74: mod_copy_set
        "Paste Set",            // 75: mod_paste_set
        "Toggle Tracks",        // 76: mod_toggle_tracks

        /*
         * Set playing modes.
         */

        "Sets Normal",          // 77: set_mode_norman
        "Sets Auto",            // 78: set_mode_auto
        "Sets Additive",        // 79: set_mode_additieve
        "All Sets",             // 80: set_mode_all_sets

        /*
         * Tricky ending.
         */

        "Max",                  // 81: Maximum -- used only for limit-checking

        /*
         * The following selectthe correct op function.
         */

        "Loop",                 // Indicates the pattern key group
        "Mute",                 // Indicates the mute key group
        "Auto"                  // Indicates automation key group
    };
    std::string result;
    if (s >= slot::bpm_up && s <= slot::automation)
    {
        int index = static_cast<int>(s);
        result = s_slot_names[index];
    }
    return result;
}

/**
 *  A static function to set a slot value from an integer, safely.
 */

automation::slot
opcontrol::set_slot (int op)
{
    static int s_minimum = static_cast<int>(automation::slot::bpm_up);
    static int s_maximum = static_cast<int>(automation::slot::max);
    return (op >= s_minimum && op < s_maximum) ?
        static_cast<automation::slot>(op) : automation::slot::none ;
}

}           // namespace seq66

/*
 * opcontrol.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


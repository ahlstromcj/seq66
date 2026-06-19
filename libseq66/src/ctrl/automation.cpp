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
 * \file          automation.cpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  and functions for the extended MIDI control feature.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-18
 * \updates       2026-06-17
 * \license       GNU GPLv2 or above
 *
 *  Currently, there is no code in this file.
 *
 * Concept:
 *
 *  In the original (Seq24/Seq64) perform class, there were 32 pattern
 *  controls, 32 mute-group controls, and 32 automation controls, all
 *  in the same [midi-control] section.  Each MIDI contol line was placed into
 *  one of 3 arrays of MIDI controls, for toggle, on, and off settings.
 *  In perform :: handle_midicontrol_event(), each array was checked for a
 *  match to the incoming MIDI control, and a perform function was executed.
 *
 *  In the new version, we want to look up an incoming MIDI event and
 *  determine in which control-section it belongs, and which kind of event it
 *  is (toggle/on/off).  If it is a pattern control, it will call
 *  performer::sequence_playing_toggle() with a pattern number. If it is a
 *  mute-group control, it will call performer::select_and_mute_group().
 *  If it is an automation control, it will call some other performer member
 *  function.  All of these functions will accept an action parameter
 *  (toggle/on/off), a pattern or mute-group number, or some other value if
 *  applicable.
 *
 * Status bits (enum class ctrlstatus):
 *
 *  These were purely internal constants used with the functions that
 *  implement MIDI control (and also some keystroke control) for the
 *  application, and they were defined in the perform header file in Seq64.
 *  However, we now have to expose them for the Qt5 implementation, until we
 *  can entirely reconcile/refactor the Kepler34-based body of code.  Note how
 *  they specify different bit values, as it they could be masked together to
 *  signal multiple functions.  We're going to explain them here so that class
 *  declaration doesn't become difficult to read.
 *
 *  "replace":
 *
 *      If this bit is set, then perform::sequence_playing_toggle() unsets
 *      this status and calls perform::off_sequences(), which calls
 *      sequence::set_playing(false) for all active sequences.
 *
 *      It works like this:
 *
 *      -#  The user presses the Replace key, or the MIDI control message for
 *          c_midicontrol_mod_replace is received.
 *      -#  This bit is OR'd into perform::m_control_status.  This status bit
 *          is used in perform::sequence_playing_toggle().
 *          -   Called in perform::sequence_key() so that keystrokes in
 *              the main window toggle patterns in the main window.
 *          -   Called in peform::toggle_other_seqs() to implement
 *              Shift-click to toggle all other patterns but the one
 *              clicked.
 *          -   Called in seqmenu::toggle_current_sequence(), called in
 *              mainwnd to implement clicking on a pattern.
 *          -   Also used in MIDI control to toggle patterns 0 to 31,
 *              offset by the screen-set.
 *          -   perform::sequence_playing_off(), similarly used in MIDI control.
 *          -   perform::sequence_playing_on(), similarly used in MIDI control.
 *      -#  When the key is released, this bit is AND'd out of
 *          perform::m_control_status.
 *
 *      Both the MIDI control and the keystroke set the sequence to be
 *      "replaced".
 *
 *  "snapshot":
 *
 *      By default, perform::sequence_playing_toggle() calls sequence ::
 *      toggle_playing() on the given sequence number, plus what is noted for
 *      c_status_snapshot.  It works like this:
 *
 *      -#  The user presses the Snapshot key.
 *      -#  This bit is OR'd into perform::m_control_status.
 *      -#  The playing state of the patterns is saved by
 *          perform::save_playing_state().
 *      -#  When the key is released, this bit is AND'd out of
 *          perform::m_control_status.
 *      -#  The playing state of the patterns is restored by
 *          perform::restore_playing_state().
 *
 *  "queue":
 *
 *      If this bit is set, then perform::sequence_playing_toggle() calls
 *      sequence::toggle_queued() on the given sequence number.  The regular
 *      queue key (configurable in File / Options / Keyboard) sets this bit
 *      when pressed, and unsets it when released.  The keep-queue key sets
 *      it, but it is not unset until the regular queue key is pressed and
 *      released.
 *
 *  "one-shot":
 *
 *      This value signals the Kepler34 "one-shot" functionality.  If this bit
 *      is set, then perform::sequence_playing_toggle() calls
 *      sequence::toggle_oneshot() on the given sequence number.
 *
 */

#include "ctrl/automation.hpp"          /* seq66::automation base class     */

namespace seq66
{

namespace automation
{

std::string
category_to_string (category c)
{
    switch (c)
    {
    case category::none:
        return std::string("none");

    case category::loop:
        return std::string("loop");

    case category::mute_group:
        return std::string("mutegroup");

    case category::automation:
        return std::string("automation");

    default:
        return std::string("unknown");
    }
}

category
string_to_category (const std::string & s)
{
    if (s == "none")
        return category::none;
    else if (s == "loop")
        return category::loop;
    else if (s == "mutegroup")
        return category::mute_group;
    else if (s == "automation")
        return category::automation;
    else
        return category::none;
}

std::string
action_to_string (action c)
{
    switch (c)
    {
    case action::none:
        return std::string("none");

    case action::toggle:
        return std::string("toggle");

    case action::on:
        return std::string("on");

    case action::off:
        return std::string("off");

    default:
        return std::string("unknown");
    }
}

action
string_to_action (const std::string & s)
{
    if (s == "none")
        return action::none;
    else if (s == "toggle")
        return action::toggle;
    else if (s == "on")
        return action::on;
    else if (s == "off")
        return action::off;
    else
        return action::none;
}

/**
 *  Some actions should work whether the user defined it as "on" or
 *  a "toggle".  For example, seee performer::automation_grid_mode().
 *  This function is especially important because the keystrokes configured
 *  in the 'ctrl' file are always treated like toggles.
 */

bool
actionable (action a)
{
    return (a == action::on || a == action::toggle);
}

/**
 *  Lists the bits set in a control-status value.
 */

std::string
ctrlstatus_to_string (ctrlstatus cs)
{
    std::string result;
    if (bit_test_and(cs, ctrlstatus::replace))
        result += "replace ";

    if (bit_test_and(cs, ctrlstatus::snapshot))
        result += "snapshot ";

    if (bit_test_and(cs, ctrlstatus::queue))
        result += "queue ";

    if (bit_test_and(cs, ctrlstatus::keep_queue))
        result += "keep queue ";

    if (bit_test_and(cs, ctrlstatus::oneshot))
        result += "oneshot ";

    if (bit_test_and(cs, ctrlstatus::learn))
        result += "learn ";

    if (result.empty())
        result = "none";

    return result;
}

/*
 *  This code is currently unused.  We currently don't need to lookup by a
 *  slots string name, and use performer::print_parameters() on a
 *  hardwired name.
 *
 *  Oops, now used in debugging and in qlearnframe.
 */

using slot_pair = struct
{
    slot slotcode;
    std::string slotname;
};

/**
 *  Compare this list to the similar list in libseq66/src/ctrl/opcontrol.cpp.
 *  They differ in letter case and (slightly) in numbering.
 *
 *  This list is meant for (eventually) lookups of names rather than numbers
 *  in configuration files.
 */

static slot_pair
s_slotnamelist []
{
    /*
     * { slot::none,             "none"                 },
     */

    { slot::bpm_up,           "bpm_up"                  },  //  0
    { slot::bpm_dn,           "bpm_dn"                  },  //  1
    { slot::ss_up,            "ss_up"                   },  //  2
    { slot::ss_dn,            "ss_dn"                   },  //  3
    { slot::mod_replace,      "mod_replace"             },  //  4
    { slot::mod_snapshot,     "mod_snapshot"            },  //  5
    { slot::mod_queue,        "mod_queue"               },  //  6
    { slot::mod_gmute,        "mod_gmute"               },  //  7
    { slot::mod_glearn,       "mod_glearn"              },  //  8
    { slot::play_ss,          "play_ss"                 },  //  9
    { slot::playback,         "playback"                },  // 10
    { slot::song_record,      "song_record"             },  // 11
    { slot::solo,             "solo"                    },  // 12
    { slot::thru,             "thru"                    },  // 13
    { slot::bpm_page_up,      "bpm_page_up"             },  // 14
    { slot::bpm_page_dn,      "bpm_page_dn"             },  // 15
    { slot::ss_set,           "ss_set"                  },  // 16
    { slot::record_style,     "record_style"            },  // 17
    { slot::quan_record,      "quan_record"             },  // 18
    { slot::reset_sets,       "reset_sets"              },  // 19
    { slot::mod_oneshot,      "mod_oneshot"             },  // 20
    { slot::FF,               "FF"                      },  // 21
    { slot::rewind,           "rewind"                  },  // 22
    { slot::top,              "top"                     },  // 23
    { slot::playlist,         "playlist"                },  // 24
    { slot::playlist_song,    "playlist_song"           },  // 25
    { slot::tap_bpm,          "tap_bpm"                 },  // 26
    { slot::start,            "start"                   },  // 27
    { slot::stop,             "stop"                    },  // 28
    { slot::loop_LR,            "loop_LR"               },  // 29
    { slot::toggle_mutes,     "toggle_mutes"            },  // 30
    { slot::song_pointer,     "song_pointer"            },  // 31

    /*
     * The following add to what Seq64 supports.
     */

    { slot::keep_queue,       "keep_queue"              },  // 32
    { slot::slot_shift,       "slot_shift"              },  // 33
    { slot::mutes_clear,      "mutes_clear"             },  // 34
    { slot::quit,             "quit"                    },  // 35
    { slot::pattern_edit,     "pattern_edit"            },  // 36
    { slot::event_edit,       "event_edit"              },  // 37
    { slot::song_mode,        "song_mode"               },  // 38
    { slot::toggle_jack,      "toggle_jack"             },  // 39
    { slot::menu_mode,        "menu_mode"               },  // 40
    { slot::follow_transport, "follow_transport"        },  // 41
    { slot::panic,            "panic"                   },  // 42
    { slot::visibility,       "visibility"              },  // 43
    { slot::save_session,     "save_session"            },  // 44
    { slot::record_toggle,    "record_toggle"           },  // 45
    { slot::grid_mutes,       "grid_mutes"              },  // 46
    { slot::reserved_47,      "reserved_47"             },  // 47
    { slot::reserved_48,      "reserved_48"             },  // 48

    /*
     * Proposed massive expansion in automation. Grid mode selection.
     */

    { slot::record_overdub,     "record_overdub"        },  // 49
    { slot::record_overwrite,   "record_overwrite"      },  // 50
    { slot::record_expand,      "record_expand"         },  // 51
    { slot::record_oneshot,     "record_oneshot"        },  // 52
    { slot::grid_loop,          "grid_loop"             },  // 53
    { slot::grid_record,        "grid_record"           },  // 54
    { slot::grid_copy,          "grid_copy"             },  // 55
    { slot::grid_paste,         "grid_paste"            },  // 56
    { slot::grid_clear,         "grid_clear"            },  // 57
    { slot::grid_delete,        "grid_delete"           },  // 58
    { slot::grid_thru,          "grid_thru"             },  // 59
    { slot::grid_solo,          "grid_solo"             },  // 50
    { slot::grid_cut,           "grid_cut"              },  // 61
    { slot::grid_double,        "grid_double"           },  // 62

    /*
     * Grid quantization type selection.
     */

    { slot::grid_quant_none,    "grid_quant_none"       },  // 63
    { slot::grid_quant_full,    "grid_quant_full"       },  // 64
    { slot::grid_quant_tighten, "grid_quant_tighten"    },  // 65
    { slot::grid_quant_random,  "grid_quant_random"     },  // 66
    { slot::grid_quant_jitter,  "grid_quant_jitter"     },  // 67
    { slot::grid_quant_notemap, "grid_quant_notemap",   },  // 68

    /*
     * A few more likely candidates.
     */

    { slot::mod_bbt_hms,        "mod_bbt_hms"           },  // 69
    { slot::mod_LR_loop,        "mod_LR_loop"           },  // 70
    { slot::mod_undo,           "mod_undo"              },  // 71
    { slot::mod_redo,           "mod_redo"              },  // 72
    { slot::mod_transpose_song, "mod_transpose_song"    },  // 73
    { slot::mod_copy_set,       "mod_copy_set"          },  // 74
    { slot::mod_paste_set,      "mod_paste_set"         },  // 75
    { slot::mod_toggle_tracks,  "mod_toggle_tracks"     },  // 76

    /*
     * Set playing modes.
     */

    { slot::set_mode_normal,    "set_mode_normal"       },  // 77
    { slot::set_mode_auto,      "set_mode_auto"         },  // 78
    { slot::set_mode_additive,  "set_mode_additive"     },  // 79
    { slot::set_mode_all_sets,  "set_mode_all_sets"     },  // 80

    /*
     * Tricky ending.
     */

    { slot::max,              "maximum"                 },  // 81
    { slot::loop,             "loop"                    },  // 82
    { slot::mute_group,       "mute_group"              },  // 83
    { slot::automation,       "automation"              },  // 84
    { slot::illegal,          "illegal"                 }   // 85
};

/*
 * The slotname field is basically the same as the std::string returned by
 * opcontrol::slot_name(slot s).
 */

std::string
slot_to_string (slot s)
{
    std::string result;
    if (s >= slot::bpm_up && s < slot::illegal)
    {
        int index = static_cast<int>(s);
        result = s_slotnamelist[index].slotname;
    }
    return result;
}

slot
string_to_slot (const std::string & s)
{
    slot result = slot::illegal;
    for
    (
        slot_pair * sptr = &s_slotnamelist[0];
        sptr->slotcode != slot::illegal; ++sptr
    )
    {
        if (sptr->slotname == s)
        {
            result = sptr->slotcode;
            break;
        }
    }
    return result;
}

}           // namespace automation

}           // namespace seq66

/*
 * automation.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

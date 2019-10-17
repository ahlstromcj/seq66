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
 * \file          faker.cpp
 * \library       Seqtool (from the Seq66 project)
 * \author        Chris Ahlstrom
 * \date          2018-11-18
 * \updates       2019-10-17
 * \version       $Revision$
 * \license       $XPC_SUITE_GPL_LICENSE$
 *
 *    This module provides helper functions for the unit tests of
 *    the midicontrol module of the libseq66 library.
 */

#include <iostream>
#include <string>

#include "faker.hpp"

/**
 *  Rote default constructor
 */

faker::faker () :
    m_scratch_op_ptr    (),
    m_keycontrols       ("Faker key controls"),
    m_midicontrols      ("Faker MIDI controls"),
    m_operations        ("Faker operations")
{
    (void) populate_default_ops();
}

/**
 *  Destructor.
 */

faker::~faker ()
{
    // anything to do?
}

/**
 *  Test creating a midioperation from a static function.
 *
 *  Holy Moly! Looks like we need some convertion functions to convert these
 *  long category names to something more tractable for the caller!
 */

void
faker::create_static_op ()
{
    m_scratch_op_ptr.reset
    (
        new seq66::midioperation
        (
            seq66::opcontrol::category_name             // Holy Moly !!!
            (
                seq66::automation::category::loop       // Holy Moly !!!
            ),
            seq66::automation::category::loop,          // Holy Moly !!!
            seq66::automation::slot::loop,              // opnumber
            static_midi_op
        )
    );
    m_scratch_op_ptr->call(seq66::automation::action::toggle, 0x0, 0x0, true);
}

/**
 *  Test creating a midioperation from a member function.
 */

void
faker::create_member_op ()
{
    using namespace std::placeholders;
    auto memberfunc = std::bind
    (
        &faker::member_midi_op, this, _1, _2, _3, _4
    );
    m_scratch_op_ptr.reset
    (
        new seq66::midioperation
        (
            seq66::opcontrol::category_name            // Holy Moly !!!
            (
                seq66::automation::category::mute_group // Holy Moly !!!
            ),
            seq66::automation::category::mute_group,    // Holy Moly !!!
            seq66::automation::slot::mute_group,        // opnumber
            memberfunc
        )
    );
    m_scratch_op_ptr->call(seq66::automation::action::on, 0x0, 0x0, true);
}

/**
 *  Test creating a midioperation from a lambda function.
 */

void
faker::create_lambda_op ()
{
    m_scratch_op_ptr.reset
    (
        new seq66::midioperation
        (
            seq66::opcontrol::category_name            // Holy Moly !!!
            (
                seq66::automation::category::mute_group // Holy Moly !!!
            ),
            seq66::automation::category::mute_group,    // Holy Moly !!!
            seq66::automation::slot::mute_group,        // opnumber
            [this] (seq66::automation::action a, int d0, int d1, bool inverse)
            {
                print_parameters("Lambda function", a, d0, d1, inverse);
                return true;
            }
        )
    );
    m_scratch_op_ptr->call(seq66::automation::action::off, 0x0, 0x0, true);
}

/**
 *  This static function merely prints the parameters passed to it.
 */

void
faker::print_parameters
(
    const std::string & tag,
    seq66::automation::action a,
    int d0, int d1, bool inverse
)
{
    std::cout
        << tag << ": "
        << "act = '" << seq66::opcontrol::action_name(a) << "'; "
        << "d0 = " << d0 << "; "
        << "d1 = " << d1 << "; "
        << "inv = " << inverse
        << std::endl
        ;
}

/**
 *  A static function to use as a midioperation::functor.
 */

bool
faker::static_midi_op
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    print_parameters("Static function", a, d0, d1, inverse);
    return true;
}

/**
 *  A member function to use as a midioperation::functor.
 */

bool
faker::member_midi_op
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    print_parameters("Member function", a, d0, d1, inverse);
    return true;
}

/**
 *  Adds a member function to an automation slot.
 */

bool
faker::add_automation
(
    seq66::automation::slot s,
    automation_function f
)
{
    std::string name = seq66::opcontrol::category_name
    (
        seq66::automation::category::automation
    );
    seq66::midioperation func
    (
        name, seq66::automation::category::automation, s,
        [this, f] (seq66::automation::action a, int d0, int d1, bool inverse)
        {
            return (this->*f) (a, d0, d1, inverse);
        }
    );
    return m_operations.add(func);
}

/**
 *  Tries to populate the opcontainer with simulated versions of a pattern
 *  control function, a mute-group control function, and functions to handle
 *  each of the automation controls.
 */

bool
faker::populate_default_ops ()
{
    /*
     * The single loop-control function.
     */

    seq66::midioperation patmop
    (
        seq66::opcontrol::category_name             // name
        (
            seq66::automation::category::loop
        ),
        seq66::automation::category::loop,          // category
        seq66::automation::slot::loop,              // opnumber
        [this] (seq66::automation::action a, int d0, int d1, bool inverse)
        {
            return pattern_control(a, d0, d1, inverse);
        }
    );
    bool result = m_operations.add(patmop);

    /*
     * The single mute-group-control function.
     */

    if (result)
    {
        seq66::midioperation mutmop
        (
            seq66::opcontrol::category_name
            (
                seq66::automation::category::mute_group
            ),
            seq66::automation::category::mute_group,
            seq66::automation::slot::mute_group,    // opnumber
            [this] (seq66::automation::action a, int d0, int d1, bool inverse)
            {
                return mute_group_control(a, d0, d1, inverse);
            }
        );
        result = m_operations.add(mutmop);
    }

    /*
     * The many automation-control functions.  Try this in a function table?
     * Yes, works like a champ and a lot fewer lines of code.  See the bottom
     * of this file for the function table.
     */

    for (int index = 0; ; ++index)
    {
        if (sm_auto_func_list[index].ap_slot != seq66::automation::slot::maximum)
        {
            result = add_automation
            (
                sm_auto_func_list[index].ap_slot,
                sm_auto_func_list[index].ap_function
            );
            if (! result)
            {
                // Show an error?

                break;
            }
        }
        else
            break;
    }
    m_operations.show();
    return result;
}

/**
 *  Provides the pattern- control function... hot-keys that toggle the patterns
 *  in the current set.
 *
 * \param a
 *      Provides the action code: toggle, on, or off.  Keystrokes that use this
 *      function will always provide automation::action::toggle.
 *
 * \param d0
 *      Provides the first MIDI data byte's value.
 *
 * \param d1
 *      Provides the second MIDI data byte's value.  For keystrokes, this value
 *      provides the sequence number (an offset in the active set), and is set
 *      via the keycontrol constructor.  See seq66 :: keycontainer ::
 *      add_defaults() for an example of this setup.
 */

bool
faker::pattern_control
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Pattern ";
    name += std::to_string(d1);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *
 */

bool
faker::mute_group_control
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Mutes ";
    name += std::to_string(d0);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *  Simple error reporting for debugging.
 */

void
faker::show_ordinal_error (seq66::ctrlkey ordinal, const std::string & tag)
{
    using namespace std;
    cerr << "Ordinal 0x" << hex << ordinal << " " << tag << endl;
}

/**
 *  Handle a control key.  The caller (e.g. a Qt key-press event handler) grabs
 *  the event text and modifiers and converts it to an seq66::ctrlkey value
 *  (ranging from 0x00 to 0xFE).  We show the code here for reference:
 *
\verbatim
        seq66::ctrlkey kkey = event->key();
        unsigned kmods = static_cast<unsigned>(event->modifiers());
        seq66::ctrlkey ordinal = seq66::qt_modkey_ordinal(kkey, kmods);
\endverbatim
 *
 *  Next, we look up the keycontrol based on the ordinal value.  If this
 *  keycontrol is usable (it is not a default-constructed keycontrol),
 *  then we can use its slot value to look up the midioperation associated with
 *  this slot.
 *
 *  If the midioperation is usable, then we can call the midioperation
 *  function, passing it the parameters based on the keystroke.
 */

bool
faker::handle_keystroke (seq66::ctrlkey ordinal)
{
    bool result = false;
    const seq66::keycontrol & kc = m_keycontrols.control(ordinal);
    if (kc.is_usable())
    {
        seq66::automation::slot s = kc.slot_number();
        const seq66::midioperation & mop = m_operations.operation(s);
        if (mop.is_usable())
        {
            seq66::automation::action a = kc.action_code();
            int d0 = 0;
            int index = kc.control_code();
            result = mop.call(a, d0, index, false);
            if (! result)
                show_ordinal_error(ordinal, "call failed");
        }
        else
            show_ordinal_error(ordinal, "call unusable");
    }
#if defined SEQ66_PLATFORM_DEBUG_TMI
    else
        show_ordinal_error(ordinal, "lookup failed");
#endif

    return result;
}

/**
 *
 */

std::string
faker::action_name (seq66::automation::action a)
{
    std::string result = " ";
    if (a == seq66::automation::action::on)
        result += "On";
    else if (a == seq66::automation::action::off)
        result += "Off";
    else if (a == seq66::automation::action::toggle)
        result += "Toggle";
    else
        result += "WTF?";

    return result;
}

/**
 *  Implements a no-op function for reserved slots not yet implemented.
 */

bool
faker::automation_no_op
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "No-op ";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *  Implements BPM Up and BPM Down for MIDI control.  There is really not need
 *  for two BPM configuration lines for MIDI control, since the configured MIDI
 *  event can specify which is needed.
 *
 *  For the configured BPM Up keystroke, this function is called with an action
 *  of "on", to implement BPM Up.  But a second function, automation_bpm_dn(),
 *  is provided to implement BPM Down for keystrokes.  It can also be
 *  configured for MIDI usage, and it will work like Seq24/Sequencer64, which
 *  just check for the event irregardless of whether it is toggle, on, or off.
 */

bool
faker::automation_bpm_up_dn
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "BPM ";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *  No matter how BPM Down is configured for MIDI control, if present and the
 *  MIDI event matches, it will act like a BPM Down.  This matches the behavior
 *  of Seq24/Sequencer64.
 */

bool
faker::automation_bpm_dn
(
    seq66::automation::action /*a*/, int d0, int d1, bool inverse
)
{
    return automation_bpm_up_dn(seq66::automation::action::off, d0, d1, inverse);
}

/**
 *  Implements screenset Up and Down.
 */

bool
faker::automation_ss_up_dn
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Screenset ";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *  No matter how Screenset Down is configured for MIDI control, if present and
 *  the MIDI event matches, it will act like a Screenet Down.  This matches the
 *  behavior of Seq24/Sequencer64.
 */

bool
faker::automation_ss_dn
(
    seq66::automation::action /*a*/, int d0, int d1, bool inverse
)
{
    return automation_ss_up_dn(seq66::automation::action::off, d0, d1, inverse);
}

/**
 *  Implements mod_replace.
 */

bool
faker::automation_replace
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Mod Replace ";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *  Implements mod_snapshot.
 */

bool
faker::automation_snapshot
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Mod Snapshot ";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *  Implements mod_queue.
 */

bool
faker::automation_queue
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Mod Queue ";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *  Implements mod_gmute.
 */

bool
faker::automation_gmute
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Mod Group Mute ";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *  Implements mod_glearn.
 */

bool
faker::automation_glearn
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Mod Group Learn ";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *  Implements play_ss.
 */

bool
faker::automation_play_ss
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Play Screen-set";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *  Implements playback.
 */

bool
faker::automation_playback
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Playback";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *  Implements song_record.
 */

bool
faker::automation_song_record
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Song Record";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *  Implements solo.
 */

bool
faker::automation_solo
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Solo";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *  Implements thru.
 */

bool
faker::automation_thru
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Thru";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *
 */

bool
faker::automation_bpm_page_up_dn
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "BPM Page";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *  No matter how BPM Down is configured for MIDI control, if present and the
 *  MIDI event matches, it will act like a BPM Down.  This matches the behavior
 *  of Seq24/Sequencer64.
 */

bool
faker::automation_bpm_page_dn
(
    seq66::automation::action /*a*/, int d0, int d1, bool inverse
)
{
    return automation_bpm_up_dn(seq66::automation::action::off, d0, d1, inverse);
}

/**
 *
 */

bool
faker::automation_ss_set
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Screenset Set";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *
 */

bool
faker::automation_record
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Record";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *
 */

bool
faker::automation_quan_record
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Quantized Record";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *
 */

bool
faker::automation_reset_seq
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Reset Sequence";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *
 */

bool
faker::automation_oneshot
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "One-shot Queue";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *
 */

bool
faker::automation_FF
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Fast-forware";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *
 */

bool
faker::automation_rewind
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Rewind";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *
 */

bool
faker::automation_top
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Top";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *
 */

bool
faker::automation_playlist
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Playlist";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *
 */

bool
faker::automation_playlist_song
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Playlist Song";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *
 */

bool
faker::automation_start
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Start";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *
 */

bool
faker::automation_stop
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Stop";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *
 */

bool
faker::automation_snapshot_2
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Snapshot 2";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *
 */

bool
faker::automation_toggle_mutes
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Toggle Mutes";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *
 */

bool
faker::automation_song_pointer
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Song Pointer";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *
 */

bool
faker::automation_keep_queue
(
    seq66::automation::action a, int d0, int d1, bool inverse
)
{
    std::string name = "Keep queue";
    name += action_name(a);
    print_parameters(name, a, d0, d1, inverse);
    return true;
}

/**
 *
 */

faker::automation_pair
faker::sm_auto_func_list [] =
{
    { seq66::automation::slot::bpm_up, &faker::automation_bpm_up_dn         },
    { seq66::automation::slot::bpm_dn, &faker::automation_bpm_dn            },
    { seq66::automation::slot::ss_up, &faker::automation_ss_up_dn           },
    { seq66::automation::slot::ss_dn, &faker::automation_ss_dn              },
    { seq66::automation::slot::mod_replace, &faker::automation_replace      },
    { seq66::automation::slot::mod_snapshot, &faker::automation_snapshot    },
    { seq66::automation::slot::mod_queue, &faker::automation_queue          },
    { seq66::automation::slot::mod_gmute, &faker::automation_gmute          },
    { seq66::automation::slot::mod_glearn, &faker::automation_glearn        },
    { seq66::automation::slot::play_ss, &faker::automation_play_ss          },
    { seq66::automation::slot::playback, &faker::automation_playback        },
    { seq66::automation::slot::song_record, &faker::automation_song_record  },
    { seq66::automation::slot::solo, &faker::automation_solo                },
    { seq66::automation::slot::thru, &faker::automation_thru                },
    {
        seq66::automation::slot::bpm_page_up,
        &faker::automation_bpm_page_up_dn
    },
    { seq66::automation::slot::bpm_page_dn, &faker::automation_bpm_page_dn  },
    { seq66::automation::slot::ss_set, &faker::automation_ss_set            },
    { seq66::automation::slot::record, &faker::automation_record            },
    { seq66::automation::slot::quan_record, &faker::automation_quan_record  },
    { seq66::automation::slot::reset_seq, &faker::automation_reset_seq      },
    { seq66::automation::slot::mod_oneshot, &faker::automation_oneshot      },
    { seq66::automation::slot::FF, &faker::automation_FF                    },
    { seq66::automation::slot::rewind, &faker::automation_rewind            },
    { seq66::automation::slot::top, &faker::automation_top                  },
    { seq66::automation::slot::playlist, &faker::automation_playlist        },
    {
        seq66::automation::slot::playlist_song,
        &faker::automation_playlist_song
    },
    { seq66::automation::slot::start, &faker::automation_start              },
    { seq66::automation::slot::stop, &faker::automation_stop                },
    {
        seq66::automation::slot::mod_snapshot_2,
        &faker::automation_snapshot_2
    },
    {
        seq66::automation::slot::toggle_mutes,
        &faker::automation_toggle_mutes
    },
    {
        seq66::automation::slot::song_pointer,
        &faker::automation_song_pointer
    },
    { seq66::automation::slot::keep_queue,  &faker::automation_keep_queue   },
    { seq66::automation::slot::slot_shift,  &faker::automation_no_op        },
    { seq66::automation::slot::mutes_clear, &faker::automation_no_op        },
    { seq66::automation::slot::reserved_35, &faker::automation_no_op        },

    /*
     * There are more, but we will ignore them here for testing, for now.
     */

    /*
     * Terminator
     */

    { seq66::automation::slot::maximum, &faker::automation_no_op            }
};

/*
 * faker.cpp
 *
 * vim: ts=4 sw=4 et ft=cpp
 */


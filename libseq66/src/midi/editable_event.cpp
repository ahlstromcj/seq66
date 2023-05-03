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
 * \file          editable_event.cpp
 *
 *  This module declares/defines the base class for MIDI editable_events.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2023-05-03
 * \license       GNU GPLv2 or above
 *
 *  A MIDI editable event is encapsulated by the seq66::editable_event
 *  object.
 */

#include <cstdlib>                      /* atoi(3) and atof(3) for 32-bit   */

#include "midi/editable_event.hpp"      /* seq66::editable_event            */
#include "midi/editable_events.hpp"     /* seq66::editable_events multimap  */
#include "util/strfunctions.hpp"        /* seq66::strings_match(), etc.     */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Provides an integer value that is larger than any MIDI value, to be
 *  used to terminate a array of items keyed by a midibyte value.
 */

midishort s_end_of_table = 0x100;       /* one more than 0xFF               */

/**
 *  We moved all of these static arrays out of editable_events because
 *  they are shown by the GDB print command, and they make the GDB wrapper we
 *  use, cgdb, crash trying to output them all.
 *
 *  Initializes the array of event/name pairs for the MIDI events categories.
 *  Terminated by an empty string, the latter being the preferred test, for
 *  consistency with the other arrays and because 0 is often a legitimate code
 *  value.
 */

static const editable_event::name_value_t
s_category_names [] =
{
    {
        0, midishort(editable_event::subgroup::channel_message),
        "Channel Message"
    },
    {
        1, midishort(editable_event::subgroup::system_message),
        "System Message"
    },
    {
        2, midishort(editable_event::subgroup::meta_event),
        "Meta Event"
    },
    {
        3, midishort(editable_event::subgroup::seqspec_event),
        "SeqSpec Event"
    },
    {
        -1, s_end_of_table, ""
    }
};

/**
 *  Initializes the array of event/name pairs for the channel MIDI events.
 *  Terminated by an empty string.
 */

static const editable_event::name_value_t
s_channel_event_names [] =
{
    {  0, midishort(EVENT_NOTE_OFF),         "Note Off"          },  // 0x80
    {  1, midishort(EVENT_NOTE_ON),          "Note On"           },  // 0x90
    {  2, midishort(EVENT_AFTERTOUCH),       "Aftertouch"        },  // 0xA0
    {  3, midishort(EVENT_CONTROL_CHANGE),   "Control"           },  // 0xB0
    {  4, midishort(EVENT_PROGRAM_CHANGE),   "Program"           },  // 0xC0
    {  5, midishort(EVENT_CHANNEL_PRESSURE), "Ch Pressure"       },  // 0xD0
    {  6, midishort(EVENT_PITCH_WHEEL),      "Pitch Wheel"       },  // 0xE0
    { -1, s_end_of_table,                    ""                  }   // end
};

/**
 *  Initializes the array of event/name pairs for the system MIDI events.
 *  Terminated by an empty string.
 */

static const editable_event::name_value_t
s_system_event_names [] =
{
    {  0, midishort(EVENT_MIDI_SYSEX),         "SysEx Start"     },  // 0xF0
    {  1, midishort(EVENT_MIDI_QUARTER_FRAME), "Quarter Frame"   },  //   .
    {  2, midishort(EVENT_MIDI_SONG_POS),      "Song Position"   },  //   .
    {  3, midishort(EVENT_MIDI_SONG_SELECT),   "Song Select"     },  //   .
    { -1, midishort(EVENT_MIDI_SONG_F4),       "F4"              },
    { -1, midishort(EVENT_MIDI_SONG_F5),       "F5"              },
    {  4, midishort(EVENT_MIDI_TUNE_SELECT),   "Tune Request"    },
    {  5, midishort(EVENT_MIDI_SYSEX_END),     "SysEx End"       },
    {  6, midishort(EVENT_MIDI_CLOCK),         "Timing Clock"    },
    { -1, midishort(EVENT_MIDI_SONG_F9),       "F9"              },
    {  7, midishort(EVENT_MIDI_START),         "Start"           },
    {  8, midishort(EVENT_MIDI_CONTINUE),      "Continue"        },
    {  9, midishort(EVENT_MIDI_STOP),          "Stop"            },  //   .
    { -1, midishort(EVENT_MIDI_SONG_FD),       "FD"              },  //   .
    { 10, midishort(EVENT_MIDI_ACTIVE_SENSE),  "Active sensing"  },  //   .
    { 11, midishort(EVENT_MIDI_RESET),         "Reset"           },  // 0xFF
    { -1, s_end_of_table,                      ""                }   // end
};

/**
 *  Initializes the array of event/name pairs for all of the Meta events.
 *  Terminated only by the empty string. Events with an index of -1 are not
 *  supported.  Only s_end_of_table is used to detect the end of the table.
 *  Previous to version 0.99.5, this array wasn't used, so we are free
 *  to mess with it and hide non-editable events. Events that are non-editable
 *  include events handled by non-MIDI manipulations:
 *
 *      -   Seq Number
 *      -   MIDI Channel
 *      -   MIDI Port
 *      -   Track End
 *      -   SeqSpec (maybe)
 *
 *  However, we need to be able to look up their names for display in case
 *  someone's tune contains them.
 */

static const editable_event::name_value_t
s_meta_event_names [] =
{
    { -1, 0x00, "Seq Number"            },  // FF 00 02 ss ss (16-bit)
    {  0, 0x01, "Text Event"            },  // FF 01 len text
    {  1, 0x02, "Copyright"             },  // FF 02 len text
    {  2, 0x03, "Track Name"            },  // FF 03 len text
    {  3, 0x04, "Instrument Name"       },  // FF 04 len text
    {  4, 0x05, "Lyric"                 },  // FF 05 len text
    {  5, 0x06, "Marker"                },  // FF 06 len text
    {  6, 0x07, "Cue Point"             },  // FF 07 len text
    {  7, 0x08, "Program Name"          },  // FF 08 len text
    {  8, 0x09, "Device Name"           },  // FF 09 len text
    { -1, 0x0A, "Event 0A"              },
    { -1, 0x0B, "Event 0B"              },
    { -1, 0x0C, "Event 0C"              },
    { -1, 0x0D, "Event 0D"              },
    { -1, 0x0E, "Event 0E"              },
    { -1, 0x0F, "Event 0F"              },
    { -1, 0x20, "MIDI Channel"          },  // FF 20 01 cc (obsolete)
    { -1, 0x21, "MIDI Port"             },  // FF 21 01 pp (obsolete)
    { -1, 0x2F, "Track End"             },  // FF 2F 00 (mandatory event)
    {  9, 0x51, "Tempo"                 },  // FF 51 03 tt tt tt (set tempo)
    { 10, 0x54, "SMPTE Offset"          },  // FF 54 05 hh mm ss fr ff
    { 11, 0x58, "Time Sig"              },  // FF 58 04 nn dd cc bb
    { 12, 0x59, "Key Sig"               },  // FF 59 02 sf mi
    { 13, 0x7F, "Seq Spec"              },  // FF 7F len id data (seq66 prop)
    { -1, 0xFF, "Illegal meta event"    },  // indicator of problem
    { -1, s_end_of_table, ""            }   // terminator
};

/**
 *  Initializes the array of event/length pairs for all of the Meta events.
 *  Terminated only by the empty string.
 */

static const editable_event::meta_length_t
sm_meta_lengths [] =
{
    { 0x00, 2   },                              // "Seq Number"

    /*
     * Since meta_event_length() returns 0 by default, we can save some lookup
     * time.
     *
     * { 0x01, 0   },                           // "Text Event"
     * { 0x02, 0   },                           // "Copyright"
     * { 0x03, 0   },                           // "Track Name"
     * { 0x04, 0   },                           // "Instrument Name"
     * { 0x05, 0   },                           // "Lyric"
     * { 0x06, 0   },                           // "Marker"
     * { 0x07, 0   },                           // "Cue Point"
     * { 0x08, 0   },                           // "Program Name"
     * { 0x09, 0   },                           // "Device Name"
     */

    /*
     * The following events are normally not documented, so let's save some
     * more lookup time.
     *
     * { 0x0A, 0    },                          // "Text Event 0A"
     * { 0x0B, 0    },                          // "Text Event 0B"
     * { 0x0C, 0    },                          // "Text Event 0C"
     * { 0x0D, 0    },                          // "Text Event 0D"
     * { 0x0E, 0    },                          // "Text Event 0E"
     * { 0x0F, 0    },                          // "Text Event 0F"
     */

    { 0x20, 1   },                              // "MIDI Channel"
    { 0x21, 1   },                              // "MIDI Port"
    { 0x2F, 0   },                              // "Track End"
    { 0x51, 3   },                              // "Tempo"
    { 0x54, 5   },                              // "SMPTE Offset"
    { 0x58, 4   },                              // "Time Sig"
    { 0x59, 2   },                              // "Key Sig"
    { 0x7F, 0   },                              // "Seq Spec"
    { 0xFF, 0   },                              // "Illegal meta event"
    { s_end_of_table, 0 }                       // terminator
};

/**
 *  Initializes the array of event/name pairs for all of the
 *  seq66-specific events.  Terminated only by the empty string.
 *  Note that the numbers reflect the masking off of the high-order bits
 *  of 0x242400nn to retrieve 0xnn.
 *
 *  Also see the list of midilong value in midi_vector_base.hpp.
 */

static const editable_event::name_value_t
s_seqspec_event_names [] =
{
    {  0, 0x01, "Buss number"               },
    {  1, 0x02, "Channel number"            },
    {  2, 0x03, "Clocking"                  },
    {  3, 0x04, "Old trigger"               },  // original Seq24-style trigger
    {  4, 0x05, "Song notes"                },
    {  5, 0x06, "Time signature"            },
    {  6, 0x07, "Beats per minute"          },
    {  7, 0x08, "Trigger ex"                },  // newer Seq24 trigger
    {  8, 0x09, "Mute groups"               },
    { -1, 0x0A, "Gap A"                     },
    { -1, 0x0B, "Gap B"                     },
    { -1, 0x0C, "Gap C"                     },
    { -1, 0x0D, "Gap D"                     },
    { -1, 0x0E, "Gap E"                     },
    { -1, 0x0F, "Gap F"                     },
    {  9, 0x10, "Song MIDI control"         },
    { 10, 0x11, "Music key"                 },
    { 11, 0x12, "Music scale"               },
    { 12, 0x13, "Background pattern"        },
    { 13, 0x14, "Track transpose"           },  // Seq32
    { 14, 0x15, "Perfedit beats/measure"    },  // Seq32
    { 15, 0x16, "Perfedit beat width"       },  // Seq32
    { 16, 0x17, "Tempo map"                 },  // Seq32
    { -1, 0x18, "Reserved 1"                },
    { -1, 0x19, "Reserved 2"                },
    { 17, 0x1A, "Tempo track"               },
    { 18, 0x1B, "Pattern color"             },
    { 19, 0x1C, "Patter edit mode"          },
    { 20, 0x1D, "Pattern loop count"        },
    { -1, 0x1E, "Reserved 3"                },
    { -1, 0x1F, "Reserved 4"                },
    { 21, 0x20, "Transposable trigger"      },  // Seq66/Sequencer64 trigger
    { -1, s_end_of_table, ""                }   // terminator
};

/**
 *  Contains pointers (references cannot be stored in an array)  to the
 *  desired array for a given category.  This code could be considered a bit
 *  rococo.
 */

static const editable_event::name_value_t * const
s_category_arrays [] =
{
    s_category_names,
    s_channel_event_names,
    s_system_event_names,
    s_meta_event_names,
    s_seqspec_event_names
};

/**
 *  A static member function used to fill the event category combo-box.
 */

std::string
editable_event::category_name (int index)
{
    std::string result;
    int counter = 0;
    while (s_category_names[counter].event_value != s_end_of_table)
    {
        if (counter == index)
        {
            result = s_category_names[counter].event_name;
            break;
        }
        ++counter;
    }
    return result;
}

/**
 *  A static member function used to fill a channel-event status combo-box.
 */

std::string
editable_event::channel_event_name (int index)
{
    std::string result;
    int counter = 0;
    while (s_channel_event_names[counter].event_value != s_end_of_table)
    {
        if (counter == index)
        {
            result = s_channel_event_names[counter].event_name;
            break;
        }
        ++counter;
    }
    return result;
}

/**
 *  A static member function used to fill a system-event combo-box.
 */

std::string
editable_event::system_event_name (int index)
{
    std::string result;
    int counter = 0;
    while (s_system_event_names[counter].event_value != s_end_of_table)
    {
        int tindex = s_system_event_names[counter].event_index;
        if (tindex >= 0)
        {
            if (tindex == index)
            {
                result = s_system_event_names[counter].event_name;
                break;
            }
        }
        ++counter;
    }
    return result;
}

/**
 *  A static member function used to fill a meta-event combo-box.
 */

std::string
editable_event::meta_event_name (int index)
{
    std::string result;
    int counter = 0;
    while (s_meta_event_names[counter].event_value != s_end_of_table)
    {
        int tindex = s_meta_event_names[counter].event_index;
        if (tindex >= 0)
        {
            if (tindex == index)
            {
                result = s_meta_event_names[counter].event_name;
                break;
            }
        }
        ++counter;
    }
    return result;
}

/**
 *  A static member function used to fill a seqspec-event combo-box.
 */

std::string
editable_event::seqspec_event_name (int index)
{
    std::string result;
    int counter = 0;
    while (s_seqspec_event_names[counter].event_value != s_end_of_table)
    {
        int tindex = s_seqspec_event_names[counter].event_index;
        if (tindex >= 0)
        {
            if (tindex == index)
            {
                result = s_seqspec_event_names[counter].event_name;
                break;
            }
        }
        ++counter;
    }
    return result;
}

/**
 *  Provides a static lookup function that returns the name, if any,
 *  associated with a midibyte value.
 *
 * \param value
 *      The MIDI byte value to look up.
 *
 * \param cat
 *      The category of the MIDI byte.  Each category calls a different name
 *      array into play.
 *
 *  \return
 *      Returns the name associated with the value.  If there is no such name,
 *      then an empty string is returned.
 */

std::string
editable_event::value_to_name
(
    midibyte value,
    editable_event::subgroup cat
)
{
    std::string result;
    const name_value_t * const table = s_category_arrays[int(cat)];
    if (cat == subgroup::channel_message)
        value = event::mask_status(value);

    midibyte counter = 0;
    while (table[counter].event_value != s_end_of_table)
    {
        if (value == table[counter].event_value)
        {
            result = table[counter].event_name;
            break;
        }
        ++counter;
    }
    return result;
}

/**
 *  Provides a static lookup function that returns the value, if any,
 *  associated with a name string.  The string_match() function, which can
 *  match abbreviations, case-insensitively, is used to make the string
 *  comparisons.
 *
 * \param name
 *      The string value to look up.
 *
 * \param cat
 *      The category of the MIDI byte.  Each category calls a different name
 *      array into play.
 *
 *  \return
 *      Returns the value associated with the name.  If there is no such value,
 *      then s_end_of_table is returned.
 */

midishort
editable_event::name_to_value
(
    const std::string & name,
    editable_event::subgroup cat
)
{
    midishort result = s_end_of_table;
    if (! name.empty())
    {
        const name_value_t * const table = s_category_arrays[int(cat)];
        midibyte counter = 0;
        while (table[counter].event_value != s_end_of_table)
        {
            if (strings_match(table[counter].event_name, name))
            {
                result = table[counter].event_value;
                break;
            }
            ++counter;
        }
    }
    return result;
}

/**
 *  Provides a static lookup function that takes a meta-event number and
 *  returns the expected length of the data for that event.
 *
 * \param value
 *      The MIDI byte value to look up.
 *
 *  \return
 *      Returns the length associated with the meta event.  If the expected
 *      length is actually 0, or is variable, then 0 is returned.
 */

midishort
editable_event::meta_event_length (midibyte value)
{
    midishort result = 0;
    midibyte counter = 0;
    while (sm_meta_lengths[counter].event_value != s_end_of_table)
    {
        if (value == sm_meta_lengths[counter].event_value)
        {
            result = sm_meta_lengths[counter].event_length;
            break;
        }
        ++counter;
    }
    return result;
}

/**
 *  Principal constructor.
 *
 *  The default constructor is hidden and unimplemented.  We will get the
 *  default controller name from the controllers module.  We should also be
 *  able to look up the selected buss's entries for a sequence, and load up
 *  the CC/name pairs on the fly.
 *
 * \param parent
 *      Provides the overall editable-events object that manages the whole set
 *      of editable-event.
 */

editable_event::editable_event (const editable_events & parent) :
    event               (),
    m_parent            (&parent),
    m_link_time         (c_null_midipulse),
    m_category          (subgroup::name),
    m_name_category     (),
    m_format_timestamp  (timestamp_measures),
    m_name_timestamp    (),
    m_name_status       (),
    m_name_meta         (),
    m_name_seqspec      (),
    m_name_channel      (),
    m_name_data         ()
{
    // No code needed
}

/**
 *  Event constructor.  This function basically adds all of the extra
 *  editable_event stuff to a standard event, so that the resulting
 *  editable_event is container-ready.
 */

editable_event::editable_event
(
    const editable_events & parent,
    const event & ev
) :
    event               (ev),
    m_parent            (&parent),
    m_link_time         (c_null_midipulse),
    m_category          (subgroup::name),
    m_name_category     (),
    m_format_timestamp  (timestamp_measures),
    m_name_timestamp    (),
    m_name_status       (),
    m_name_meta         (),
    m_name_seqspec      (),
    m_name_channel      (),
    m_name_data         ()
{
    if (is_linked())
        m_link_time = ev.link()->timestamp();
}

/**
 * \setter m_category by value
 *      Also keeps the m_name_category member in synchrony.  Note that a bad
 *      value is translated to the enum value subgroup::name.
 *
 * \param c
 *      Provides the category value to set.
 */

void
editable_event::category (editable_event::subgroup c)
{
    if (c >= subgroup::channel_message && c <= subgroup::seqspec_event)
        m_category = c;
    else
        m_category = subgroup::name;

    std::string name = value_to_name(static_cast<midibyte>(c), subgroup::name);
    if (! name.empty())
        m_name_category = name;
}

/**
 * \setter m_category by name
 *      Also keeps the m_name_category member in synchrony, but looks up the
 *      name, rather than using the name parameter, to avoid storing
 *      abbreviations.  Note that a bad value is translated to the value of
 *      subgroup::name.
 *
 * \param name
 *      Provides the category name for the category value to set.
 */

void
editable_event::category (const std::string & name)
{
    midishort catcode = name_to_value(name, subgroup::name);
    if (catcode < s_end_of_table)
        m_category = static_cast<subgroup>(catcode);
    else
        m_category = subgroup::name;

    m_name_category = value_to_name
    (
        static_cast<midibyte>(m_category), subgroup::name
    );
}

/**
 * \setter event::set_timestamp()
 *      Implemented to allow a uniform naming convention that is not
 *      slavish to the get/set crowd [this ain't Java].  Plus, we also
 *      have to set the string version at the same time.
 *
 *  The format of the string representation is of the format selected by the
 *  m_format_timestamp member and is set by the format_timestamp() function.
 *
 * \param ts
 *      Provides the timestamp in units of MIDI pulses.
 */

void
editable_event::timestamp (midipulse ts)
{
    event::set_timestamp(ts);
    (void) format_timestamp();
}

/**
 * \setter event::set_timestamp() [string version]
 *
 *  The format of the string representation is of the format selected by the
 *  m_format_timestamp member and is set by the format_timestamp() function.
 *
 * \param ts_string
 *      Provides the timestamp in units of MIDI pulses.
 */

void
editable_event::timestamp (const std::string & ts_string)
{
    if (not_nullptr(parent()))
    {
        midipulse ts = parent()->string_to_pulses(ts_string);
        event::set_timestamp(ts);
        (void) format_timestamp();
    }
}

/**
 *  Formats the current timestamp member as a string.  The format of the
 *  string representation is of the format selected by the m_format_timestamp
 *  member.
 */

std::string
editable_event::format_timestamp ()
{
    if (m_format_timestamp == timestamp_measures)
        m_name_timestamp = time_as_measures();
    else if (m_format_timestamp == timestamp_time)
        m_name_timestamp = time_as_minutes();
    else if (m_format_timestamp == timestamp_pulses)
        m_name_timestamp = time_as_pulses();
    else
        m_name_timestamp = "unsupported category in editable event";

    return m_name_timestamp;
}

/**
 *  Converts the current time-stamp to a string representation in units of
 *  measures, beats, and divisions.  Cannot be inlined because of a circular
 *  dependency between the editable_event and editable_events classes.
 */

std::string
editable_event::time_as_measures ()
{
    if (not_nullptr(parent()))
    {
        return pulses_to_measurestring(timestamp(), parent()->timing());
    }
    else
    {
        static std::string s_dummy;
        return s_dummy;
    }
}

/**
 *  Converts the current time-stamp to a string representation in units of
 *  hours, minutes, seconds, and fraction.  Cannot be inlined because of a
 *  circular dependency between the editable_event and editable_events
 *  classes.
 */

std::string
editable_event::time_as_minutes ()
{
    if (not_nullptr(parent()))
    {
        return pulses_to_time_string(timestamp(), parent()->timing());
    }
    else
    {
        static std::string s_dummy;
        return s_dummy;
    }
}

/**
 *  Converts a string into an event status, along with timestamp and data
 *  bytes.  Currently, this function handles only the following two messages:
 *
 *      -   subgroup::channel_message.
 *          Handle Meta or SysEx events, setting that status to 0xFF and the
 *          meta-type (in the m_channel member) to the meta event type-value,
 *          then filling in m_sysex based on the field values in the sd0
 *          parameter.
 *      -   subgroup::system_message.
 *      -   subgroup::meta_event.
 *
 *  The Tempo data 0 field consists of one double BPM value.  We convert it to
 *  a tempo-in-microseconds value, then populate a 3-byte array with it.  Then
 *  we need to create an event from it.
 *
 *  The Time Signature data 0 field consists of a string like "4/4".  The data
 *  1 field has two values for metronome support.  First, parse the "nn/dd"
 *  string; the slash (solidus) is required.  Then get the cc and bb metronome
 *  values, if present.  Otherwise, hardwired them to values of 0x18 and 0x08.
 *
 *  After all of the numbering member items have been set, they are converted
 *  and assigned to the string versions via a call to the analyze() function.
 *
 * \param ts
 *      Provides the time-stamp string of the event.
 *
 * \param s
 *      Provides the name of the event, such as "Program Change".
 *
 * \param sd0
 *      Provides the string defining the first data byte of the event.  For
 *      Meta events, this might have multiple values, though we support only
 *      Set Tempo and Time Signature at present.
 *
 * \param sd1
 *      Provides the string defining the second data byte of the event, if
 *      applicable to the event.  Some meta event may provide multiple values
 *      in this string.
 *
 * \param chan
 *      Provides the string name for the channel.  If empty (the default
 *      value), the channel is not changed.  The name of the channel is re
 *      "1", not 0.
 */

void
editable_event::set_status_from_string
(
    const std::string & ts,
    const std::string & s,
    const std::string & sd0,
    const std::string & sd1,
    const std::string & chan,
    const std::string & text
)
{
    midishort value = name_to_value(s, subgroup::channel_message);
    timestamp(ts);
    if (value != s_end_of_table)                        /* channel message  */
    {
        midibyte status = midibyte(value);
        midibyte c = string_to_midibyte(chan, 1) - 1;   /* default == 0     */
        midibyte d0 = string_to_midibyte(sd0);
        midibyte d1 = string_to_midibyte(sd1);
        set_channel_status(status, c);
        if (is_one_byte_msg(status))
            set_data(d0);
        else if (is_two_byte_msg(status))
            set_data(d0, d1);
    }
    else
    {
        value = name_to_value(s, subgroup::meta_event);
        if (value != s_end_of_table)                    /* meta message     */
        {
            set_meta_status(value);
            if (value == EVENT_META_SET_TEMPO)                      /* 0x51 */
            {
                double bpm = string_to_double(sd0);
                if (bpm > 0.0f)
                    (void) set_tempo(bpm);
            }
            else if (value == EVENT_META_TIME_SIGNATURE)            /* 0x58 */
            {
                auto pos = sd0.find_first_of("/");
                if (pos != std::string::npos)
                {
                    int nn = string_to_int(sd0);
                    int dd = nn;
                    int cc = 0x18;
                    int bb = 0x08;
                    ++pos;

                    std::string sd0_partial = sd0.substr(pos);  // drop "nn/"
                    if (! sd0_partial.empty())
                        dd = string_to_int(sd0_partial);

                    if (dd > 0)
                    {
                        pos = sd0.find_first_of(" ", pos);      // bypass dd
                        if (pos != std::string::npos)
                        {
                            pos = sd0.find_first_of("0123456789x", pos);
                            if (pos != std::string::npos)
                            {
                                cc = int(strtol(&sd0[pos], NULL, 0));
                                pos = sd0.find_first_of(" ", pos);
                                if (pos != std::string::npos)
                                {
                                    pos = sd0.find_first_of("0123456789x", pos);
                                    if (pos != std::string::npos)
                                        bb = int(strtol(&sd0[pos], NULL, 0));
                                }
                            }
                        }
                        midibyte t[4];
                        t[0] = midibyte(nn);
                        t[1] = midibyte(dd);
                        t[2] = midibyte(cc);
                        t[3] = midibyte(bb);
                        (void) set_sysex(t, 4);     /* add ex-data bytes    */
                    }
                }
            }
            else if (value == EVENT_META_KEY_SIGNATURE)             /* 0x59 */
            {
                // TO DO
            }
            else if
            (
                value >= EVENT_META_TEXT_EVENT && value <= EVENT_META_CUE_POINT
            )
            {
            }
            else
            {
                /*
                 * Parse the string of (potentially) hex digits.  However, we
                 * still need to determine the length value and allocate the
                 * midibyte array ahead of time, or add a function to set
                 * SysEx. TODO.
                 *
                auto pos = sd0.find_first_of("0123456789x");
                while (pos != std::string::npos)
                {
                    // TODO
                }
                 */
            }
        }
    }
    analyze();                          /* create the strings   */
}

/**
 *  This function can modify the data bytes and the channel of a channel
 *  event.  For example, it can change the note number, note velocity, and
 *  note channel.  Not modified are the name and type of the event, and its
 *  timestamp.  This function is useful in modifying the linked note event of
 *  a Note On/Off event.
 */

void
editable_event::modify_channel_status_from_string
(
    const std::string & sd0,
    const std::string & sd1,
    const std::string & chan
)
{
    midibyte status = mask_status(get_status());
    midibyte c = midibyte(string_to_int(chan) - 1);
    set_channel_status(status, c);          /* pass in status and channel   */
    if (is_one_byte_msg(status) || is_pitchbend_msg(status))
    {
        /*
         * Do not change the Program Change or Channel Pressure data.
         * Do not change the Coarse or Fine Pitchbend.
         */
    }
    else if (is_two_byte_msg(status))
    {
        midibyte d0 = string_to_midibyte(sd0);
        midibyte d1 = string_to_midibyte(sd1);
        if (is_note_msg(status))            /* Note On/Off and Aftertouch   */
        {
            d1 = note_velocity();           /* keep velocity or pressure    */
        }
        else if (is_controller_msg(status)) /* keep CC# and CC value        */
        {
            get_data(d0, d1);
        }
        set_data(d0, d1);
    }
    analyze();                              /* (re)create the strings       */
}

/**
 *  Converts the event into a string desribing the full event.  We get the
 *  time-stamp as a string, make sure the event is fully analyzed so that all
 *  items and strings are set correctly.
 *
 * \return
 *      Returns a human-readable string describing this event.  This string is
 *      displayed in an event list, such as in the eventedit module.
 */

std::string
editable_event::stock_event_string ()
{
    char temp[64];
    std::string ts = format_timestamp();
    analyze();
    if (is_ex_data())
    {
        if (is_tempo() || is_time_signature())
        {
            snprintf
            (
                temp, sizeof temp, "%9s %-11s %-10s",
                ts.c_str(), m_name_status.c_str(), m_name_data.c_str()
            );
        }
        else
        {
            snprintf
            (
                temp, sizeof temp, "%9s %-11s %-12s",
                ts.c_str(), m_name_status.c_str(), m_name_data.c_str()
            );
        }
    }
    else
    {
        snprintf
        (
            temp, sizeof temp, "%9s %-11s %-10s %-20s",
            ts.c_str(), m_name_status.c_str(),
            m_name_channel.c_str(), m_name_data.c_str()
        );
    }
    return std::string(temp);
}

/**
 *  Analyzes an editable-event to make all the settings it needs.  Used in the
 *  constructors.  Some of the setters indirectly set the appropriate string
 *  representation, as well.
 *
 * Category:
 *
 *      This function can figure out if the status byte implies a channel
 *      message or a system message, and set the category string as well.
 *      However, at this time, detection of Meta events (0xFF) or
 *      Proprietary/SeqSpec events (0xFF with 0x2424) doesn't work due to lack
 *      of context here (and due to the fact that currently such events are
 *      not yet stored in a Seq66 sequence/track, and the
 *      least-significant-byte gets masked off anyway.)
 *
 * Status:
 *
 *      We distinguish between channel and system messages, and then one- and
 *      two-byte messages, but don't yet distinguish the data values fully.
 *
 * Sysex and Meta events:
 *
 *      We are starting to support events with statuses ranging from 0xF0 to
 *      0xFF, with a concentration on Set Tempo and Time Signature events.
 *      We want them to be full-fledged Seq66 events.
 *
 *      The 0xFF byte represents a Meta event, not a Reset event, when we're
 *      dealing with data from a MIDI file, as we are here. And we need to get
 *      the next byte after the status byte.
 *
 *      We want Set Tempo events to appear as "Tempo 120.0" and Time Signature
 *      events to appear as "Time Sig 4/4".
 */

void
editable_event::analyze ()
{
    midibyte status = get_status();
    (void) format_timestamp();
    if (is_channel_msg(status))
    {
        char tmp[32];
        int ch = int(channel()) + 1;
        int di0, di1;
        midibyte d0, d1;
        get_data(d0, d1);
        di0 = int(d0);
        di1 = int(d1);
        category(subgroup::channel_message);
        status = event::mask_status(status);

        /*
         * Get channel message name (e.g. "Program change");
         */

        m_name_status = value_to_name(status, subgroup::channel_message);
        snprintf(tmp, sizeof tmp, "%d", ch);        /* no "Ch", too much    */
        m_name_channel = std::string(tmp);
        if (is_one_byte_msg(status))
        {
            snprintf(tmp, sizeof tmp, "Data %d", di0);
        }
        else
        {
            if (is_note_msg(status))
                snprintf(tmp, sizeof tmp, "Key %d Vel %d", di0, di1);
            else
                snprintf(tmp, sizeof tmp, "Data %d, %d", di0, di1);
        }
        m_name_data = std::string(tmp);
    }
    else if (is_system_msg(status))
    {
        if (is_meta_msg(status))
        {
            midibyte metatype = get_meta_status();  /* stored in channel!   */
            category(subgroup::meta_event);
            m_name_status = value_to_name(metatype, subgroup::meta_event);
            m_name_channel.clear();                 /* will not be output   */
            m_name_data = ex_data_string();
        }
        else
        {
            /*
             * Get system message name (e.g. "SysEx start");
             */

            category(subgroup::system_message);
            m_name_status = value_to_name(status, subgroup::system_message);
            m_name_channel.clear();
            m_name_data.clear();
        }
    }
    else
    {
        /*
         * Would try to detect SysEx versus Meta message versus SeqSpec here.
         * Then set either m_name_meta and/or m_name_seqspec.
         * ALso see eventslots::set_current_event().
         */
    }
}

/**
 *  Assuming the event is a Meta event or a SysEx, this function returns a
 *  short string representation of the event data, usable in the eventeditor
 *  class or elsewhere.  Most SysEx events will only show the first few bytes;
 *  we could make a SysEx viewer/editor for handling long events.
 *
 * \return
 *      Returns the data string.  If empty, the data is bad in some way, or
 *      the event is not a Meta event.
 */

std::string
editable_event::ex_data_string () const
{
    std::string result;
    char tmp[32];
    if (is_tempo())
    {
        snprintf(tmp, sizeof tmp, "%6.2f", tempo());
        result = tmp;
    }
    else if (is_time_signature())
    {
        if (sysex_size() > 0)
        {
            int nn = get_sysex()[0];
            int dd = get_sysex()[1];            /* hopefully a power of 2   */
            int cc = get_sysex()[2];
            int bb = get_sysex()[3];
            snprintf(tmp, sizeof tmp, "%d/%d 0x%X 0x%X", nn, dd, cc, bb);
            result += tmp;
        }
    }
    else
    {
        std::string data;
        int limit = sysex_size();
        if (limit > 4)
            limit = 4;                          /* we have space limits     */

        for (int i = 0; i < limit; ++i)
        {
            snprintf(tmp, sizeof tmp, "%2X ", get_sysex()[i]);
            result += tmp;
        }
        if (sysex_size() > limit)
            result += "...";
    }
    return result;
}

/**
 *  Assuming the event is a Meta text event, this function returns a
 *  short string representation of the event data in ASCII text.
 *  Only a few characters are shown, at present.
 *
 * \return
 *      Returns the data string.  If empty, the data is bad in some way, or
 *      the event is not a Meta event.
 */

std::string
editable_event::ex_text_string () const
{
    std::string result;
    int limit = sysex_size();
    if (limit > 24)
        limit = 24;                          /* we have space limits     */

    for (int i = 0; i < limit; ++i)
    {
        char ch = char(get_sysex()[i]);
        result += ch;
    }
    if (sysex_size() > limit)
        result += "...";

    return result;
}

}           // namespace seq66

/*
 * editable_event.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


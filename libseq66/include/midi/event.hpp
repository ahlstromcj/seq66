#if ! defined SEQ66_EVENT_HPP
#define SEQ66_EVENT_HPP

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
 * \file          event.hpp
 *
 *  This module declares/defines the event class for operating with
 *  MIDI events.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2023-09-05
 * \license       GNU GPLv2 or above
 *
 *  This module also declares/defines the various constants, status-byte
 *  values, or data values for MIDI events.  This class is also a base class,
 *  so that we can manage "editable events".
 *
 *  One thing we need to add to this event class is a way to encapsulate
 *  Meta events.  First, we use the existing event::sysex to hold
 *  this data.
 *
 *  The MIDI protocol consists of MIDI events that carry four types of messages:
 *
 *      -   Voice messages.  0x80 to 0xEF; includes channel information.
 *      -   System common messages.  0xF0 (SysEx) to 0xF7 (End of SysEx)
 *      -   System realtime messages. 0xF8 to 0xFF.
 *      -   Meta messages. 0xFF is the flag, followed by type, length, and data.
 */

#include <string>                       /* used in to_string()              */
#include <vector>                       /* SYSEX data stored in vector      */

#include "midi/midibytes.hpp"           /* seq66::midibyte alias, etc.      */

#undef  SEQ66_STAZED_SELECT_EVENT_HANDLE    /* nowhere near ready!  */

/**
 *  Defines the number of data bytes in MIDI status data.
 *
 *  But consider this, events other than System Exclusive, which do not have
 *  an arbitrary number of bytes, but a definite number. These events are:
 *
\verbatim
        -   Sequence No.:   FF 00 02 s1 s1
        -   MIDI Channel:   FF 20 01 cc
        -   MIDI Port:      FF 21 01 pp
        -   Set Tempo:      FF 51 03 tt tt tt
        -   SMPTE Offset:   FF 54 05 hh mm ss fr ff
        -   Time Signature: FF 58 04 nn dd cc bb
        -   Key Signature:  FF 59 02 sf mi
\endverbatim
 *
 *  The arbitrarily-sized Meta events are:
 *
\verbatim
        -   Text:           FF 01 len text
        -   Copyright:      FF 02 len text
        -   Track Name:     FF 03 len name
        -   Instrument:     FF 04 len name
        -   Marker:         FF 05 len text
        -   Cue Point:      FF 06 len text
        -   Seq. Specific:  FF 7F len data
\endverbatim
 *
 *  The maximum amount of constant-size data is 5 bytes.  We should aim to
 *  increase this and use it, using event::m_status as the "meta" byte and
 *  perhaps m_channel as the "meta-event" byte.  But curently, the tempo and
 *  time signature events are stored as data in the sequence object, so
 *  that's probably the best tact for the future.
 */

#define SEQ66_MIDI_DATA_BYTE_COUNT      2

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  This highest bit of the STATUS byte is always 1.  If this bit is not set,
 *  then the MIDI byte is a DATA byte.
 */

const midibyte EVENT_STATUS_BIT         = 0x80;

/**
 *  Channel Voice Messages.
 *
 *  The following MIDI events are channel messages.  The comments represent
 *  the one or two data-bytes of the message.
 *
 *  Note that Channel Mode Messages use the same code as the Control Change,
 *  but uses reserved controller numbers ranging from 122 to 127.
 *
 *  The EVENT_ANY (0x00) value may prove to be useful in allowing any event to
 *  be dealt with.  Not sure yet, but the cost is minimal.
 */

const midibyte EVENT_ANY                = 0x00u;      // our own value
const midibyte EVENT_NOTE_OFF           = 0x80u;      // 0kkkkkkk 0vvvvvvv
const midibyte EVENT_NOTE_ON            = 0x90u;      // 0kkkkkkk 0vvvvvvv
const midibyte EVENT_AFTERTOUCH         = 0xA0u;      // 0kkkkkkk 0vvvvvvv
const midibyte EVENT_CONTROL_CHANGE     = 0xB0u;      // 0ccccccc 0vvvvvvv
const midibyte EVENT_PROGRAM_CHANGE     = 0xC0u;      // 0ppppppp
const midibyte EVENT_CHANNEL_PRESSURE   = 0xD0u;      // 0vvvvvvv
const midibyte EVENT_PITCH_WHEEL        = 0xE0u;      // 0lllllll 0mmmmmmm

/**
 *  Control Change Messages.  This is a small subset of the roughly 40 control
 *  changes.
 */

const midibyte EVENT_CTRL_VOLUME        = 0x07u;
const midibyte EVENT_CTRL_BALANCE       = 0x08u;
const midibyte EVENT_CTRL_PAN           = 0x0Au;
const midibyte EVENT_CTRL_EXPRESSION    = 0x0Bu;

/**
 *  System Messages.
 *
 *  The following MIDI events have no channel.  We have included redundant
 *  constant variables for the SysEx Start and End bytes just to make it
 *  clear that they are part of this sequence of values, though usually
 *  treated separately.
 *
 *  Only the following constants are followed by some data bytes:
 *
 *      -   EVENT_MIDI_SYSEX            = 0xF0  // ends with 0xF7
 *      -   EVENT_MIDI_QUARTER_FRAME    = 0xF1  // and 0x0n to 0x7n
 *      -   EVENT_MIDI_SONG_POS         = 0xF2  // and 0x0 to 0x3FFF 16th note
 *      -   EVENT_MIDI_SONG_SELECT      = 0xF3  // and 0x0 to 0x7F song number
 *      -   EVENT_MIDI_TUNE_SELECT      = 0xF6  // no data, tune yourself
 *
 *  A MIDI System Exclusive (SYSEX) message starts with F0, followed
 *  by the manufacturer ID (how many? bytes), a number of data bytes, and
 *  ended by an F7.
 *
 * MIDI System Real-Time Messages:
 *
 *      -   https://en.wikipedia.org/wiki/MIDI_beat_clock
 *      -   http://www.midi.org/techspecs/midimessages.php
 */

const midibyte EVENT_MIDI_REALTIME       = 0xF0u;   // 0xFn when masked
const midibyte EVENT_MIDI_SYSEX          = 0xF0u;   // redundant, see below
const midibyte EVENT_MIDI_QUARTER_FRAME  = 0xF1u;   // system common > 0 bytes
const midibyte EVENT_MIDI_SONG_POS       = 0xF2u;   // 2 data bytes
const midibyte EVENT_MIDI_SONG_SELECT    = 0xF3u;   // 1 data byte, not used
const midibyte EVENT_MIDI_SONG_F4        = 0xF4u;   // undefined
const midibyte EVENT_MIDI_SONG_F5        = 0xF5u;   // undefined
const midibyte EVENT_MIDI_TUNE_SELECT    = 0xF6u;   // 0 data bytes, not used
const midibyte EVENT_MIDI_SYSEX_END      = 0xF7u;   // redundant, see below
const midibyte EVENT_MIDI_SYSEX_CONTINUE = 0xF7u;   // redundant, see below
const midibyte EVENT_MIDI_CLOCK          = 0xF8u;   // no data bytes
const midibyte EVENT_MIDI_SONG_F9        = 0xF9u;   // undefined
const midibyte EVENT_MIDI_START          = 0xFAu;   // no data bytes
const midibyte EVENT_MIDI_CONTINUE       = 0xFBu;   // no data bytes
const midibyte EVENT_MIDI_STOP           = 0xFCu;   // no data bytes
const midibyte EVENT_MIDI_SONG_FD        = 0xFDu;   // undefined
const midibyte EVENT_MIDI_ACTIVE_SENSE   = 0xFEu;   // 0 data bytes, not used
const midibyte EVENT_MIDI_RESET          = 0xFFu;   // 0 data bytes, not used

/**
 *  0xFF is a MIDI "escape code" used in MIDI files to introduce a MIDI meta
 *  event.  Note that it has the same code (0xFF) as the Reset message, but
 *  the Meta message is read from a MIDI file, while the Reset message is sent
 *  to the sequencer by other MIDI participants.
 */

const midibyte EVENT_MIDI_META           = 0xFFu;   // an escape code

/**
 *  Provides values for the currently-supported Meta events, and many others:
 *
 *      -   Set Tempo (0x51)
 *      -   Time Signature (0x58)
 */

const midibyte EVENT_META_SEQ_NUMBER     = 0x00u;
const midibyte EVENT_META_TEXT_EVENT     = 0x01u;   // meta text
const midibyte EVENT_META_COPYRIGHT      = 0x02u;   // meta text
const midibyte EVENT_META_TRACK_NAME     = 0x03u;   // meta text
const midibyte EVENT_META_INSTRUMENT     = 0x04u;   // meta text
const midibyte EVENT_META_LYRIC          = 0x05u;   // meta text
const midibyte EVENT_META_MARKER         = 0x06u;   // meta text
const midibyte EVENT_META_CUE_POINT      = 0x07u;   // meta text
const midibyte EVENT_META_MIDI_CHANNEL   = 0x20u;   // skipped, obsolete
const midibyte EVENT_META_MIDI_PORT      = 0x21u;   // skipped, obsolete
const midibyte EVENT_META_END_OF_TRACK   = 0x2Fu;
const midibyte EVENT_META_SET_TEMPO      = 0x51u;
const midibyte EVENT_META_SMPTE_OFFSET   = 0x54u;   // skipped
const midibyte EVENT_META_TIME_SIGNATURE = 0x58u;
const midibyte EVENT_META_KEY_SIGNATURE  = 0x59u;
const midibyte EVENT_META_SEQSPEC        = 0x7Fu;

/**
 *  Provides a sanity-check limit for the number of bytes in a MIDI Meta Text
 *  message and similar messages. Might be better larger, but....
 */

const size_t c_meta_text_limit           = 1024;    // for sanity only

/**
 *  As a "type" (overloaded on channel) value for a Meta event, 0xFF indicates
 *  an illegal meta type.
 */

const midibyte EVENT_META_ILLEGAL = c_midibyte_max;  /* problem code */

/**
 *  These file masks are used to obtain (or mask off) the channel data and
 *  status portion from an (incoming) status byte.
 */

const midibyte EVENT_GET_CHAN_MASK      = 0x0Fu;
const midibyte EVENT_GET_STATUS_MASK    = 0xF0u;
const midibyte EVENT_DATA_MASK          = 0x7Fu;

/**
 *  Variable from the "stazed" extras.  We reversed the parts of each token
 *  for consistency with the macros defined above.
 */

const int EVENTS_ALL                     = -1;
const int EVENTS_UNSELECTED              =  0;

/**
 *  Provides events for management of MIDI events.
 *
 *  A MIDI event consists of 3 bytes:
 *
 *      -#  Status byte, 1sssnnnn, where the 1sss bits specify the type of
 *          message, and the nnnn bits denote the channel number, 0 to 15.
 *          The status byte always starts with 1.
 *      -#  The first data byte, 0xxxxxxx, where the data byte always
 *          start with 0, and the xxxxxxx values range from 0 to 127.
 *      -#  The second data byte, 0xxxxxxx.
 *
 *  This class may have too many member functions.
 */

class event
{

    friend class eventlist;
    friend class sequence;

public:

    /**
     *  Provides a type definition for a vector of midibytes.  This type will
     *  also hold the raw data of Meta events.
     */

    using sysex = midibytes;

    /**
     *  The data buffer for MIDI events.  This item replaces the
     *  eventlist::Events type definition so that we can replace event
     *  pointers with iterators, safely.
     */

    using buffer = std::vector<event>;
    using iterator = buffer::iterator;
    using const_iterator = buffer::const_iterator;
    using reverse_iterator = buffer::reverse_iterator;
    using const_reverse_iterator = buffer::const_reverse_iterator;

public:

    /**
     *  Provides a key value for an event.  Its types match the
     *  m_timestamp and get_rank() function of the event class.
     *  It is not needed in the eventlist class, which uses a vector as a
     *  container, but it is needed in the editable_events class, which
     *  uses a multimap.
     */

    class key
    {

    private:

        midipulse m_timestamp;      /**< The primary key-value for the key. */
        int m_rank;                 /**< The sub-key-value for the key.     */

    public:

        key () = default;
        key (midipulse tstamp, int rank);
        key (const event & e);
        bool operator < (const key & rhs) const;
        bool operator == (const key & rhs) const;
        key (const key & ek) = default;
        key & operator = (const key & ek) = default;
    };

private:

    /**
     *  Indicates the input buss on which this event came in.  The default
     *  value is unusable: null_buss() from the midibytes.hpp module.
     */

    bussbyte m_input_buss;

    /**
     *  Provides the MIDI timestamp in ticks, otherwise known as the "pulses"
     *  in "pulses per quarter note" (PPQN).
     */

    midipulse m_timestamp;

    /**
     *  This is the status byte without the channel. The channel is included
     *  when recording MIDI, but, once a sequence with a matching channel is
     *  found, the channel nybble is cleared for storage.  The channel will be
     *  added back on the MIDI bus upon playback.  The high nybble = type of
     *  event; The low nybble = channel.  Bit 7 is present in all status
     *  bytes.
     *
     *  Note that, for status values of 0xF0 (Sysex) or 0xFF (Meta), special
     *  handling of the event can occur.  We would like to eventually use
     *  inheritance to keep the event class simple.  For now, search for
     *  "tempo" and "sysex" to tease out their implementations. Sigh.
     */

    midibyte m_status;

    /**
     *  In order to be able to handle MIDI channel-splitting of an SMF 0 file,
     *  we need to store the channel, even if we override it when playing the
     *  MIDI data.
     *
     *  Overload:  For Meta events, where is_meta() is true, this value holds
     *  the type of Meta event. See the editable_event::sm_meta_event_names[]
     *  array.  Note that EVENT_META_ILLEGAL (0xFF) indicates an illegal Meta
     *  event.
     */

    midibyte m_channel;

    /**
     *  The two bytes of data for the MIDI event.  Remember that the
     *  most-significant bit of a data byte is always 0.  A one-byte message
     *  uses only the 0th index.
     */

    midibyte m_data[SEQ66_MIDI_DATA_BYTE_COUNT];

    /**
     *  The data buffer for SYSEX messages.  Adapted from Stazed's Seq32
     *  project on GitHub.
     *
     * Note:
     *
     *  This object will also hold the generally small amounts of data needed
     *  for Meta events.  Compare is_sysex() to is_meta() and is_ex_data()
     *  [which tests for both]. In addition, detect and handle the other
     *  Meta message that hold variable amounts of bytes.
     */

    sysex m_sysex;

    /**
     *  This event is used to link NoteOns and NoteOffs together.  The NoteOn
     *  points to the NoteOff, and the NoteOff points to the NoteOn.  See, for
     *  example, eventlist::link_notes().
     *
     *  We currently do not link tempo events; this would be necessary to
     *  display a line from one tempo event to the next.  Currently we display
     *  a small circle for each tempo event.
     */

    iterator m_linked;

    /**
     *  Indicates that a link has been made.  This item is used [via
     *  the get_link() and link() accessors] in the sequence class.
     */

    bool m_has_link;

    /**
     *  Answers the question "is this event selected in editing."
     */

    bool m_selected;

    /**
     *  Answers the question "is this event marked in processing."  This
     *  marking is more of an internal function for purposes of reorganizing
     *  events.
     */

    bool m_marked;

    /**
     *  Answers the question "is this event being painted."  This setting is
     *  made by sequence::add_event() or add_note() if the paint parameter is
     *  true (it defaults to false).
     */

    bool m_painted;

public:

    event ();
    event
    (
        midipulse tstamp,
        midibyte status,
        midibyte d0         = 0,
        midibyte d1         = 0
    );
    event (midipulse tstamp, midibpm tempo);
    event
    (
        midipulse tstamp, midibyte notekind, midibyte channel,
        int note, int velocity
    );
    event (const event & rhs);
    event & operator = (const event & rhs);
    virtual ~event ();

    /*
     * Operator overload, the only one needed for sorting events in a list
     * or a map.
     */

    bool operator < (const event & rhsevent) const;
    bool match (const event & target) const;
    void prep_for_send (midipulse tick, const event & source);

    void set_input_bus (bussbyte b)
    {
        if (is_good_buss(b))
            m_input_buss = b;
    }

    bussbyte input_bus () const
    {
        return m_input_buss;
    }

    void set_timestamp (midipulse time)
    {
        m_timestamp = time;
    }

    midipulse timestamp () const
    {
        return m_timestamp;
    }

    midibyte channel () const
    {
        return m_channel;
    }

    /**
     *  Checks the channel number to see if the event's channel matches it, or
     *  if the event has no channel.  Used in the SMF 0 track-splitting code.
     *  The value of 0xFF is Seq66's channel value that indicates that the
     *  event's m_channel value is bogus.  However, it also means that the
     *  channel, if applicable to the event, is encoded in the m_status byte
     *  itself.  This is our work around to be able to hold a multi-channel
     *  SMF 0 track in a sequence.  In a Seq66 SMF 0 track, every event has a
     *  channel.  In a Seq66 SMF 1 track, the events do not have a channel.
     *  Instead, the channel is a global value of the sequence, and is stuffed
     *  into each event when the event is played, but not when written to a
     *  MIDI file. (New behavior 20201-08-10).
     *
     * \param channel
     *      The channel to check for a match.
     *
     * \return Returns true if the given channel matches the event's channel.
     */

    bool match_channel (int channel) const
    {
        return is_null_channel(m_channel) || midibyte(channel) == m_channel;
    }

    static midibyte mask_channel (midibyte m)
    {
        return m & EVENT_GET_CHAN_MASK;
    }

    static midibyte mask_status (midibyte m)
    {
        return m & EVENT_GET_STATUS_MASK;
    }

    /**
     *  Static test for the status bit.  The "opposite" test is is_data().
     *  Currently used only in midifile.
     *
     * \return
     *      Returns true if the status bit is set.  Covers 0x80 to 0xFF.
     */

    static bool is_status (midibyte m)
    {
        return (m & EVENT_STATUS_BIT) != 0;
    }

    /**
     *  Makes sure the status byte matches the "EVENT" message bytes exactly
     *  by stripping the channel nybble if necessary.
     */

    static midibyte normalized_status (midibyte status)
    {
        return is_channel_msg(status) ? mask_status(status) : status ;
    }

#if defined SEQ66_THIS_FUNCTION_IS_USED

    /*
     *  Static test for the status bit.  The opposite test is is_status().
     *  Currently not used anywhere.
     *
     * \return
     *      Returns true if the status bit is not set.
     */

    static bool is_data (midibyte m)
    {
        return (m & EVENT_STATUS_BIT) == 0x00;
    }

#endif

    /*
     *  Static functions used in event and editable event.  These can be useful
     *  to any caller.
     */

public:

    static bool is_system_msg (midibyte m)
    {
        return m >= EVENT_MIDI_SYSEX;
    }

    static bool is_meta_msg (midibyte m)
    {
        return m == EVENT_MIDI_META;
    }

    static bool is_ex_data_msg (midibyte m)
    {
        return m == EVENT_MIDI_META || m == EVENT_MIDI_SYSEX;
    }

    static bool is_pitchbend_msg (midibyte m)
    {
        return mask_status(m) == EVENT_PITCH_WHEEL;
    }

    static bool is_controller_msg (midibyte m)
    {
        return mask_status(m) == EVENT_CONTROL_CHANGE;
    }

    static bool is_note_on_msg (midibyte m)
    {
        return m >= EVENT_NOTE_ON || m < EVENT_AFTERTOUCH;
    }

    /**
     *  Static test for messages that involve notes only: Note On and
     *  Note Off, useful in note-event linking.
     *
     * \param m
     *      The channel status or message byte to be tested.
     *
     * \return
     *      Returns true if the byte represents a MIDI note on/off message.
     */

    static bool is_strict_note_msg (midibyte m)
    {
        return m >= EVENT_NOTE_OFF || m < EVENT_AFTERTOUCH;
    }

    /**
     *  We don't want a progress bar for patterns that just contain textual
     *  information. Tempo event are important, though, and visible in some
     *  pattern views.
     */

    static bool is_playable_msg (midibyte m)
    {
        return m != EVENT_MIDI_META && m != EVENT_MIDI_SYSEX;
    }

public:

    /*
     *  Static functions used in analysizing MIDI events by external callers.
     */

    /**
     *  Static test for the channel message/statuse values: Note On, Note Off,
     *  Aftertouch, Control Change, Program Change, Channel Pressure, and
     *  Pitch Wheel.  This function is also a test for a Voice Category
     *  status.  The allowed range is 0x80 to 0xEF.  Currently not used
     *  anywhere.
     *
     * \param m
     *      The channel status or message byte to be tested, with the channel
     *      bits masked off.
     *
     * \return
     *      Returns true if the byte represents a MIDI channel message.
     */

    static bool is_channel_msg (midibyte m)
    {
        return m >= EVENT_NOTE_OFF && m < EVENT_MIDI_REALTIME;
    }

    /**
     *  Static test for channel messages that have only one data byte: Program
     *  Change and Channel Pressure.  The rest of the channel messages have
     *  two data bytes.
     *
     * \param m
     *      The channel status or message byte to be tested. The channel
     *      bits are masked off before the test.
     *
     * \return
     *      Returns true if the byte represents a MIDI channel message that
     *      has only one data byte.
     */

    static bool is_one_byte_msg (midibyte m)
    {
        m = mask_status(m);
        return m == EVENT_PROGRAM_CHANGE || m == EVENT_CHANNEL_PRESSURE;
    }

    /**
     *  Static test for channel messages that have two data bytes: Note On,
     *  Note Off, Control Change, Aftertouch, and Pitch Wheel.
     *
     * \param m
     *      The channel status or message byte to be tested. The channel
     *      bits are masked off before the test.
     *
     * \return
     *      Returns true if the byte represents a MIDI channel message that
     *      has two data bytes.
     */

    static bool is_two_byte_msg (midibyte m)
    {
        return
        (
            (m >= EVENT_NOTE_OFF && m < EVENT_PROGRAM_CHANGE) ||
            mask_status(m) == EVENT_PITCH_WHEEL
        );
    }

    /**
     *  Static test for messages that involve notes and velocity: Note On,
     *  Note Off, and Aftertouch.
     *
     * \param m
     *      The channel status or message byte to be tested, and the channel
     *      bits are masked off before testing.  Actually, no longer
     *      necessary, we have a faster test, since these three events have
     *      values in an easy range to check.
     *
     * \return
     *      Returns true if the byte represents a MIDI note message.
     */

    static bool is_note_msg (midibyte m)
    {
        return m >= EVENT_NOTE_OFF && m < EVENT_CONTROL_CHANGE;
    }

    /**
     *  This static member function is used in the midifile module and in the
     *  is_note_off_recorded() member function.
     *
     * \param status
     *      The type of event, which might be EVENT_NOTE_ON.
     *
     * \param vel
     *      The velocity byte to check.  It should be zero for a note-on is
     *      note-off event.
     */

    static bool is_note_off_velocity (midibyte status, midibyte vel)
    {
        return mask_status(status) == EVENT_NOTE_ON && vel == 0;
    }

    static bool is_program_change_msg (midibyte m)
    {
        return mask_status(m) == EVENT_PROGRAM_CHANGE;
    }

    /*
     * This function seems iffy.  Replaced by is_tempo_status() in the GUI
     * classes.
     */

    static bool is_meta_status (midibyte m)
    {
        return m <= EVENT_META_SEQSPEC;
    }

    /*
     *  This currently include Meta Track Name, which is handled differently.
     *  handled differently.
     */

    static bool is_meta_text_msg (midibyte m)
    {
        return m >= EVENT_META_TEXT_EVENT && m <= EVENT_META_CUE_POINT;
    }

    static bool is_tempo_status (midibyte m)
    {
        return m == EVENT_META_SET_TEMPO;
    }

    static bool is_time_signature_status (midibyte m)
    {
        return m == EVENT_META_TIME_SIGNATURE;
    }

    static bool is_sysex_msg (midibyte m)
    {
        return m == EVENT_MIDI_SYSEX;
    }

    /**
     *  Static test for channel messages that are either not control-change
     *  messages, or are and match the given controller value.
     *  Replaced with a better function set.
     *
     * \param m
     *      The channel status or message byte to be tested.
     *
     * \param cc
     *      The desired cc value, which the datum must match, if the message
     *      is a control-change message.
     *
     * \param datum
     *      The current datum, to be compared to cc, if the message is a
     *      control-change message.
     *
     * \return
     *      Returns true if the message is not a control-change, or if it is
     *      and the cc and datum parameters match.
     */

    static inline bool is_desired_cc_or_not_cc
    (
        midibyte m, midibyte cc, midibyte datum
    )
    {
        m = mask_status(m);
        return (m != EVENT_CONTROL_CHANGE) || (datum == cc);
    }

    /**
     *  Checks for a System Common status, which is supposed to clear any
     *  running status.  Use in midifile.
     */

    static bool is_system_common_msg (midibyte m)
    {
        return m >= EVENT_MIDI_SYSEX && m < EVENT_MIDI_CLOCK;
    }

    /**
     *  Checks for a Realtime Category status, which ignores running status.
     *  Ranges from 0xF8 to 0xFF,  and m <= EVENT_MIDI_RESET is always true.
     *  Use in midifile.
     */

    static bool is_realtime_msg (midibyte m)
    {
        return m >= EVENT_MIDI_CLOCK;
    }

    /**
     *  Used in midi_jack.
     */

    static bool is_sense_or_reset (midibyte m)
    {
        return m == EVENT_MIDI_ACTIVE_SENSE || m == EVENT_MIDI_RESET;
    }

public:

    /**
     *  Calculates the value of the current timestamp modulo the given
     *  parameter.
     *
     * \param modtick
     *      The tick value to mod the timestamp against.  Usually the length
     *      of the pattern receiving this event.
     */

    void mod_timestamp (midipulse modtick)
    {
        if (modtick > 1)
            m_timestamp %= modtick;
    }

    void set_status (midibyte status);
    void set_channel (midibyte channel);
    void set_channel_status (midibyte eventcode, midibyte channel);
    void set_meta_status (midibyte metatype);
    void set_status_keep_channel (midibyte eventcode);
#if defined SEQ66_THIS_FUNCTION_IS_USED
    void set_note_off (int note, midibyte channel);
#endif
    bool set_midi_event
    (
        midipulse timestamp,
        const midibyte * buffer,
        int count
    );

    /**
     *  Note that we have ensured that status ranges from 0x80 to 0xFF.
     *  And recently, the status now holds the channel, redundantly.
     *  Unless the event is a meta event, in which case the channel is the
     *  number of the event.  We can return the bare status, or status with
     *  the channel stripped, for channel messages.
     */

    midibyte get_status () const
    {
        return m_status;
    }

    midibyte normalized_status () const
    {
        return normalized_status(m_status);     /* may strip channel nybble */
    }

    midibyte get_status (midibyte channel) const
    {
        return mask_status(m_status) | channel;
    }

    midibyte get_meta_status () const
    {
        return is_meta_msg(m_status) ? m_channel : 0 ;
    }

    bool valid_status () const
    {
        return is_status(m_status);
    }

    /**
     *  Checks that statuses match, clearing the channel nybble if needed.
     *
     * \param status
     *      Provides the desired status, without any channel nybble (that is,
     *      the channel is 0).
     *
     * \return
     *      Returns true if the event's status (after removing the channel)
     *      matches the status parameter.
     */

    bool match_status (midibyte status) const
    {
        return (has_channel() ? mask_status(m_status) : m_status) == status;
    }

    /**
     *  Clears the most-significant-bit of both parameters, and sets them into
     *  the first and second bytes of m_data.
     *
     * \param d1
     *      The first byte value to set.
     *
     * \param d2
     *      The second byte value to set.
     */

    void set_data (midibyte d0, midibyte d1 = 0)
    {
        m_data[0] = d0 & EVENT_DATA_MASK;
        m_data[1] = d1 & EVENT_DATA_MASK;
    }

    /**
     *  Yet another overload.
     */

    void set_data (midipulse tstamp, midibyte status, midibyte d0, midibyte d1);

    /**
     *  Clears the data, useful in reusing an event to hold incoming MIDI.
     */

    void clear_data ()
    {
        m_data[0] = m_data[1] = 0;
    }

    void clear_links ()
    {
        unmark();
        unlink();
    }

    /**
     *  Retrieves only the first data byte from m_data[] and copies it into
     *  the parameter.
     *
     * \param d0 [out]
     *      The return reference for the first byte.
     */

    void get_data (midibyte & d0) const
    {
        d0 = m_data[0];
    }

    /**
     *  Retrieves the two data bytes from m_data[] and copies each into its
     *  respective parameter.
     *
     * \param d0 [out]
     *      The return reference for the first byte.
     *
     * \param d1 [out]
     *      The return reference for the first byte.
     */

    void get_data (midibyte & d0, midibyte & d1) const
    {
        d0 = m_data[0];
        d1 = m_data[1];
    }

    /**
     *  Two alternative getters for the data bytes.  Useful for one-offs.
     */

    midibyte d0 () const
    {
        return m_data[0];
    }

    void d0 (midibyte b)
    {
        m_data[0] = b;
    }

    midibyte d1 () const
    {
        return m_data[1];
    }

    void d1 (midibyte b)
    {
        m_data[1] = b;
    }

    /**
     *  Increments the first data byte (m_data[0]) and clears the most
     *  significant bit.
     */

    void increment_d0 ()
    {
        m_data[0] = (m_data[0] + 1) & EVENT_DATA_MASK;
    }

    /**
     *  Decrements the first data byte (m_data[0]) and clears the most
     *  significant bit.
     */

    void decrement_d0 ()
    {
        m_data[0] = (m_data[0] - 1) & EVENT_DATA_MASK;
    }

    /**
     *  Increments the second data byte (m_data[1]) and clears the most
     *  significant bit.
     */

    void increment_d1 ()
    {
        m_data[1] = (m_data[1] + 1) & EVENT_DATA_MASK;
    }

    /**
     *  Decrements the second data byte (m_data[1]) and clears the most
     *  significant bit.
     */

    void decrement_d1 ()
    {
        m_data[1] = (m_data[1] - 1) & EVENT_DATA_MASK;
    }

    virtual bool set_text (const std::string & s);
    virtual std::string get_text () const;

    bool append_meta_data (midibyte metatype, const midibyte * data, int len);
    bool append_meta_data (midibyte metatype, const midibytes & data);
    bool append_sysex_byte (midibyte data);
    bool append_sysex (const midibyte * data, int len);
    bool append_sysex (const midibytes & data);
    bool set_sysex (const midibyte * data, int len);
    bool set_sysex (const midibytes & data);
    void set_sysex_size (int len);

    void reset_sysex ()
    {
        m_sysex.clear();
    }

    sysex & get_sysex ()
    {
        return m_sysex;
    }

    const sysex & get_sysex () const
    {
        return m_sysex;
    }

    midibyte get_sysex (size_t i) const
    {
        return m_sysex[i];
    }

    int sysex_size () const
    {
        return int(m_sysex.size());
    }

    /**
     *  Determines if this event is a note-on event and is not already linked.
     */

    bool on_linkable () const
    {
        return is_note_on() && ! is_linked();
    }

    bool off_linkable () const
    {
        return is_note_off() && ! is_linked();
    }

    /**
     *  Determines if a Note Off event is linkable to this event (which is
     *  assumed to be a Note On event).  A test used in verify_and_link().
     *
     * \param e
     *      Normally this is a Note Off event.  This status is required.
     *
     * \return
     *      Returns true if the event is a Note Off, it's the same note as
     *      this note, and the Note Off is not yet linked.
     */

    bool off_linkable (buffer::iterator & eoff) const
    {
        return eoff->off_linkable() ? eoff->get_note() == get_note() : false ;
    }

    /**
     *  Sets m_has_link and sets m_link to the provided event pointer.
     *
     * \param ev
     *      Provides a pointer to the event value to set.  Since we're using
     *      an iterator, we can't use a null-pointer test that.  We assume the
     *      caller has checked that the value is not end() for the container.
     */

    void link (iterator ev)
    {
        m_linked = ev;
        m_has_link = true;
    }

    iterator link () const
    {
        return m_linked;        /* iterator could be invalid, though    */
    }

    bool is_linked () const
    {
        return m_has_link;
    }

    bool is_note_on_linked () const
    {
        return is_note_on() && is_linked();
    }

    bool is_note_unlinked () const
    {
        return is_strict_note() && ! is_linked();
    }

    void unlink ()
    {
        m_has_link = false;
    }

    void paint ()
    {
        m_painted = true;
    }

    void unpaint ()
    {
        m_painted = false;
    }

    bool is_painted () const
    {
        return m_painted;
    }

    void mark ()
    {
        m_marked = true;
    }

    void unmark ()
    {
        m_marked = false;
    }

    bool is_marked () const
    {
        return m_marked;
    }

    void select ()
    {
        m_selected = true;
    }

    void unselect ()
    {
        m_selected = false;
    }

    bool is_selected () const
    {
        return m_selected;
    }

    /**
     *  Sets m_status to EVENT_MIDI_CLOCK;
     */

    void make_clock ()
    {
        m_status = EVENT_MIDI_CLOCK;
    }

    midibyte data (int index) const    /* index not checked, for speed */
    {
        return m_data[index];
    }

    /**
     *  Assuming m_data[] holds a note, get the note number, which is in the
     *  first data byte, m_data[0].
     */

    midibyte get_note () const
    {
        return m_data[0];
    }

    /**
     *  Sets the note number, clearing off the most-significant-bit and
     *  assigning it to the first data byte, m_data[0].
     *
     * \param note
     *      Provides the note value to set.
     */

    void set_note (midibyte note)
    {
        m_data[0] = note & EVENT_DATA_MASK;
    }

    void transpose_note (int tn);

    /**
     *  Sets the note velocity, which is held in the second data byte, and
     *  clearing off the most-significant-bit, storing it in m_data[1].
     *
     * \param vel
     *      Provides the velocity value to set.
     */

    void note_velocity (int vel)
    {
        m_data[1] = midibyte(vel) & EVENT_DATA_MASK;
    }

    midibyte note_velocity () const
    {
        return is_note() ? m_data[1] : 0 ;
    }

    bool is_note_on () const
    {
        return mask_status(m_status) == EVENT_NOTE_ON;
    }

    /**
     *  Check for the Note Off value in m_status.  Currently assumes that the
     *  channel nybble has already been stripped.
     *
     * \return
     *      Returns true if m_status is EVENT_NOTE_OFF.
     */

    bool is_note_off () const
    {
        return mask_status(m_status) == EVENT_NOTE_OFF;
    }

    /**
     *  Returns true if m_status is a Note On, Note Off, or Aftertouch message.
     *  All of these are notes, associated with a MIDI key value.  Uses the
     *  static function is_note_msg().
     *
     * \return
     *      The return value of is_note_msg() is returned.
     */

    bool is_note () const
    {
        return is_note_msg(m_status);
    }

    bool is_strict_note () const
    {
        return is_strict_note_msg(m_status);
    }

    bool is_selected_note () const
    {
        return is_selected() && is_note();
    }

    bool is_selected_note_on () const
    {
        return is_selected() && is_note_on();
    }

    bool is_controller () const
    {
        return is_controller_msg(m_status);
    }

    bool is_pitchbend () const
    {
        return is_pitchbend_msg(m_status);
    }

    bool is_playable () const
    {
        return is_playable_msg(m_status) || is_tempo();
    }

    bool is_selected_status (midibyte status) const
    {
        return is_selected() && mask_status(m_status) == mask_status(status);
    }

    bool is_desired (midibyte status, midibyte cc) const;
#if defined SEQ66_STAZED_SELECT_EVENT_HANDLE
    bool is_data_in_handle_range (midibyte target) const;
    bool is_desired (midibyte status, midibyte cc, midibyte data) const;
#endif
    bool is_desired_ex (midibyte status, midibyte cc) const;

    /**
     *  Some keyboards send Note On with velocity 0 for Note Off, so we
     *  provide this function to test that during recording.
     *
     * \return
     *      Returns true if the event is a Note On event with velocity of 0.
     */

    bool is_note_off_recorded () const
    {
        return is_note_off_velocity(m_status, m_data[1]);
    }

    bool is_midi_start () const
    {
        return m_status == EVENT_MIDI_START;
    }

    bool is_midi_continue () const
    {
        return m_status == EVENT_MIDI_CONTINUE;
    }

    bool is_midi_stop () const
    {
        return m_status == EVENT_MIDI_STOP;
    }

    bool is_midi_clock () const
    {
        return m_status == EVENT_MIDI_CLOCK;
    }

    bool is_midi_song_pos () const
    {
        return m_status == EVENT_MIDI_SONG_POS;
    }

    bool has_channel () const
    {
        return is_channel_msg(m_status);
    }

    /**
     *  Indicates if the m_status value is a one-byte message (Program Change
     *  or Channel Pressure.  Channel is stripped, because sometimes we keep
     *  the channel.
     */

    bool is_one_byte () const
    {
        return is_one_byte_msg(m_status);
    }

    /**
     *  Indicates if the m_status value is a two-byte message (everything
     *  except Program Change and Channel Pressure.  Channel is stripped,
     *  because sometimes we keep the channel.
     */

    bool is_two_bytes () const
    {
        return is_two_byte_msg(m_status);
    }

    bool is_program_change () const
    {
        return is_program_change_msg(m_status);
    }

    /**
     *  Indicates an event that has a line-drawable data item, such as
     *  velocity.  It is false for discrete data such as program/path number
     *  or Meta events.
     */

    bool is_continuous_event () const
    {
        return ! is_program_change() && ! is_meta();
    }

    /**
     *  Indicates if the event is a System Exclusive event or not.
     *  We're overloading the SysEx support to handle Meta events as well.
     *  Perhaps we need to split this support out at some point.
     */

    bool is_sysex () const
    {
        return m_status == EVENT_MIDI_SYSEX;
    }

    bool below_sysex () const
    {
        return m_status < EVENT_MIDI_SYSEX;
    }

    /**
     *  Indicates if the event is a Sense event or a Reset event.
     *  Currently ignored by Sequencer64.
     */

    bool is_sense_reset ()
    {
        return m_status == EVENT_MIDI_ACTIVE_SENSE ||
            m_status == EVENT_MIDI_RESET;
    }

    /**
     *  Indicates if the event is a Meta event or not.
     *  We're overloading the SysEx support to handle Meta events as well.
     */

    bool is_meta () const
    {
        return is_meta_msg(m_status);
    }

    bool is_meta_text () const
    {
        return is_meta() && is_meta_text_msg(m_channel);
    }

    /**
     *  Indicates if we need to use extended data (SysEx or Meta).  If true,
     *  then m_channel encodes the type of meta event.
     */

    bool is_ex_data () const
    {
        return is_ex_data_msg(m_status);
    }

    bool is_system () const
    {
        return is_system_msg(m_status);
    }

    /**
     *  Indicates if the event is a tempo event.  See sm_meta_event_names[].
     */

    bool is_tempo () const
    {
        return is_meta() && m_channel == EVENT_META_SET_TEMPO;      /* 0x51 */
    }

    midibpm tempo () const;
    bool set_tempo (midibpm tempo);
    bool set_tempo (midibyte t[3]);

    /**
     *  Indicates if the event is a Time Signature event.  See
     *  sm_meta_event_names[].
     */

    bool is_time_signature () const
    {
        return is_meta() && m_channel == EVENT_META_TIME_SIGNATURE; /* 0x58 */
    }

    /**
     *  Indicates if the event is a Key Signature event.  See
     *  sm_meta_event_names[].
     */

    bool is_key_signature () const
    {
        return is_meta() && m_channel == EVENT_META_KEY_SIGNATURE;  /* 0x59 */
    }

    void print (const std::string & tag = "") const;
    void print_note (bool showlink = true) const;
    std::string to_string () const;
    int get_rank () const;
    void rescale (int newppqn, int oldppqn);

private:    // used by friend eventlist

    /*
     * Changes timestamp.
     */

    bool jitter (int snap, int range, midipulse seqlength);
    bool tighten (int snap, midipulse seqlength);
    bool quantize (int snap, midipulse seqlength);

    /*
     * Changes the amplitude of d0 or d1, depending on the event.
     */

    bool randomize (int range);

};          // class event

/*
 * Global functions in the seq66 namespace.
 */

extern event create_tempo_event (midipulse tick, midibpm tempo);

}           // namespace seq66

#endif      // SEQ66_EVENT_HPP

/*
 * event.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


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
 * \file          event.cpp
 *
 *  This module declares/defines the base class for MIDI events.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2023-06-07
 * \license       GNU GPLv2 or above
 *
 *  A MIDI event (i.e. "track event") is encapsulated by the seq66::event
 *  object.
 *
 *      -   Varinum delta time stamp.
 *      -   Event byte:
 *          -   MIDI event.
 *              -   Channel event (0x80 to 0xE0, channel in low nybble).
 *                  -   Data byte 1.
 *                  -   Data byte 2 (for all but patch and channel pressure).
 *              -   Non-channel event (0xF0 to 0xFF).
 *                  -   SysEx (0xF0), discussed below, includes data bytes.
 *                  -   Song Position (0xF2) includes data bytes.
 *                  -   Song Select (0xF3) includes data bytes.
 *                  -   The rest of the non-channel events don't include data
 *                      byte.
 *          -   Meta event (0xFF).
 *              -   Meta type byte.
 *              -   Varinum length.
 *              -   Data bytes.
 *          -   SysEx event.
 *              -   Start byte (0xF0) or continuation/escape byte (0xF7).
 *              -   Varinum length???
 *              -   Data bytes (not yet fully supported in event class).
 *              -   End byte (0xF7).
 *
 *  Running status is used, where status bytes of MIDI channel messages can be
 *  omitted if the preceding event is a MIDI channel message with the same
 *  status.  Running status continues across delta-times.
 *
 *  In Seq24/Seq66, none of the non-channel events are stored in an
 *  event object.  There is some provisional support for storing SysEx, but
 *  none of the support functions are yet called.  In mastermidibus, there is
 *  support for recording SysEx data, but it is macro'ed out.  In rcsettings,
 *  there is an option for SysEx.  The midibus and performer objects also deal
 *  with Sysex.  But the midifile module does not read it -- it skips SysEx.
 *  Apparently, it does serve as a Thru for incoming SysEx, though.
 *  See this message thread:
 *
 *     http://sourceforge.net/p/seq66/mailman/message/1049609/
 *
 *  In Seq24/Seq66, the Meta events are handled directly, and they
 *  set up sequence parameters.
 *
 *  This module also defines the event::key object.
 */

#include <cstring>                      /* std::memcpy()                    */

#include "midi/event.hpp"               /* seq66::event class               */
#include "midi/calculations.hpp"        /* seq66::rescale_tick()            */
#include "util/basic_macros.hpp"        /* helpful debugging/build macros   */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 * --------------------------------------------------------------
 *  event
 * --------------------------------------------------------------
 */

/**
 *  This constructor simply initializes all of the class members to default
 *  values.
 */

event::event () :
    m_input_buss    (null_buss()),          /* 0xFF                 */
    m_timestamp     (0),
    m_status        (EVENT_NOTE_OFF),       /* note-off, channel 0  */
    m_channel       (null_channel()),       /* 0x80                 */
    m_data          (),                     /* a two-element array  */
    m_sysex         (),                     /* an std::vector       */
    m_linked        (nullptr),
    m_has_link      (false),
    m_selected      (false),
    m_marked        (false),
    m_painted       (false)
{
    m_data[0] = m_data[1] = 0;
}

/**
 *  This constructor initializes some of the class members to default
 *  values, and provides the most oft-changed values a parameters.
 *
 * \param tstamp
 *      Provides the timestamp of this event.
 *
 * \param status
 *      Provides the status value.  The channel nybble is cleared, since the
 *      channel is generally provided by the settings of the sequence.
 *      However, this value should include the channel if applicable!
 *
 * \param d0
 *      Provides the first data byte.  There is no default value.
 *
 * \param d1
 *      Provides the second data byte.  There is no default value.
 */

event::event (midipulse tstamp, midibyte status, midibyte d0, midibyte d1) :
    m_input_buss    (null_buss()),          /* 0xFF                 */
    m_timestamp     (tstamp),
    m_status        (status),               /* keep the channel 2021-08-09  */
    m_channel       (mask_channel(status)),
    m_data          (),                     /* two-element array, midibytes */
    m_sysex         (),                     /* an std::vector of midibytes  */
    m_linked        (nullptr),
    m_has_link      (false),
    m_selected      (false),
    m_marked        (false),
    m_painted       (false)
{
    set_data(d0, d1);                       /* fills the m_data array       */
}

/**
 *  Creates a tempo event.
 */

event::event (midipulse tstamp, midibpm tempo) :
    m_input_buss    (null_buss()),
    m_timestamp     (tstamp),
    m_status        (EVENT_MIDI_META),
    m_channel       (EVENT_META_SET_TEMPO),
    m_data          (),                     /* two-element array, midibytes */
    m_sysex         (),                     /* an std::vector of midibytes  */
    m_linked        (nullptr),
    m_has_link      (false),
    m_selected      (false),
    m_marked        (false),
    m_painted       (false)
{
    set_tempo(tempo);                       /* fills the m_sysex vector     */
}

/**
 *  Creates a note event.
 */

event::event
(
    midipulse tstamp,
    midibyte notekind,              /* must not include channel (ch == 0)   */
    midibyte channel,
    int note,
    int velocity
) :
    m_input_buss    (null_buss()),
    m_timestamp     (tstamp),
    m_status        (notekind),
    m_channel       (channel),
    m_data          (),                     /* two-element array, midibytes */
    m_sysex         (),                     /* an std::vector of midibytes  */
    m_linked        (nullptr),
    m_has_link      (false),
    m_selected      (false),
    m_marked        (false),
    m_painted       (false)
{
    m_data[0] = midibyte(note);
    m_data[1] = midibyte(velocity);
    if (is_null_channel(channel))
    {
        m_channel = 0;
    }
    else
    {
        midibyte chan = mask_channel(channel);
        m_status = mask_status(notekind) | chan;
        m_channel = chan;
    }
}

/**
 *  This copy constructor initializes most of the class members.  This
 *  function is currently geared only toward support of the SMF 0
 *  channel-splitting feature.  Many of the members are not set to useful
 *  values when the MIDI file is read, so we don't handle them for now.
 *
 * \warning
 *      Note that now events are also copied when creating the editable_events
 *      container, so this function is even more important.  The event links,
 *      for linking Note Off events to their respective Note On events, are
 *      dropped.  Generally, they will need to be reconstituted by calling the
 *      eventlist::verify_and_link() function.
 *
 * \warning
 *      This function does not yet copy the SysEx data.  The inclusion
 *      of SysEx events was not complete in Seq24, and it is still not
 *      complete in Seq66.  Nor does it currently bother with the
 *      links, as noted above.
 *
 * \param rhs
 *      Provides the event object to be copied.
 */

event::event (const event & rhs) :
    m_input_buss    (rhs.m_input_buss),
    m_timestamp     (rhs.m_timestamp),
    m_status        (rhs.m_status),
    m_channel       (rhs.m_channel),
    m_data          (),                     /* a two-element array      */
    m_sysex         (rhs.m_sysex),          /* copies a vector of data  */
    m_linked        (rhs.m_linked),         /* for vector implemenation */
    m_has_link      (rhs.m_has_link),       /* m_linked has 2 linkers!  */
    m_selected      (rhs.m_selected),
    m_marked        (rhs.m_marked),
    m_painted       (rhs.m_painted)
{
    m_data[0] = rhs.m_data[0];
    m_data[1] = rhs.m_data[1];
}

/**
 *  This principal assignment operator sets most of the class members.  This
 *  function is currently geared only toward support of the SMF 0
 *  channel-splitting feature.  Many of the member are not set to useful value
 *  when the MIDI file is read, so we don't handle them for now.
 *
 * \warning
 *      This function now copies the SysEx data, but the inclusion of SysEx
 *      events was not complete in Seq24, and it is still not complete in
 *      Seq66.  Nor does it currently bother with the link the event
 *      might have, except in the std::vector implementation.
 *
 * \param rhs
 *      Provides the event object to be assigned.
 *
 * \return
 *      Returns a reference to "this" object, to support the serial assignment
 *      of events.
 */

event &
event::operator = (const event & rhs)
{
    if (this != &rhs)
    {
        m_input_buss    = rhs.m_input_buss;
        m_timestamp     = rhs.m_timestamp;
        m_status        = rhs.m_status;
        m_channel       = rhs.m_channel;
        m_data[0]       = rhs.m_data[0];
        m_data[1]       = rhs.m_data[1];
        m_sysex         = rhs.m_sysex;
        m_linked        = rhs.m_linked;             /* vector implemenation */
        m_has_link      = rhs.m_has_link;           /* two linkers!         */
        m_selected      = rhs.m_selected;           /* false instead?       */
        m_marked        = rhs.m_marked;             /* false instead?       */
        m_painted       = rhs.m_painted;            /* false instead?       */
    }
    return *this;
}

/**
 *  This destructor explicitly deletes m_sysex and sets it to null.
 *  The reset_sysex() function does what we need.  But now that m_sysex is a
 *  vector, no action is needed.
 */

event::~event ()
{
    // Automatic destruction of members is enough
}

/**
 *  Copies a subset of data to the calling event.  Used in the
 *  sequence::put_event_on_bus() function to add a timestamp to outgoing
 *  events, a lapse in earlier versions of the SeqXX series. We don't
 *  need the following settings in an event merely to be played.
 *
 *      m_channel       = source.m_channel;
 *      m_linked        = source.m_linked;
 *      m_has_link      = source.m_has_link;
 *      m_selected      = source.m_selected;
 *      m_marked        = source.m_marked;
 *      m_painted       = source.m_painted;
 *
 *  It is assumed that "this" event has been default constructed.
 *
 * \param tick
 *      Provides the current tick (pulse) time of playback.  This always
 *      increases, and never loops back.
 *
 * \param source
 *      The event to be sent.  We need just some items from this
 *      event.
 */

void
event::prep_for_send (midipulse tick, const event & source)
{
    m_input_buss    = source.m_input_buss;
    m_timestamp     = tick;
    m_status        = source.m_status;
    m_data[0]       = source.m_data[0];
    m_data[1]       = source.m_data[1];
    m_sysex         = source.m_sysex;
}

/**
 *  If the current timestamp equal the event's timestamp, then this
 *  function returns true if the current rank is less than the event's
 *  rank.
 *
 *  Otherwise, it returns true if the current timestamp is less than
 *  the event's timestamp.
 *
 * \warning
 *      The less-than operator is supposed to support a "strict weak
 *      ordering", and is supposed to leave equivalent values in the same
 *      order they were before the sort.  However, every time we load and
 *      save our sample MIDI file, events get reversed.  Here are
 *      program-changes that get reversed:
 *
\verbatim
        Save N:     0070: 6E 00 C4 48 00 C4 0C 00  C4 57 00 C4 19 00 C4 26
        Save N+1:   0070: 6E 00 C4 26 00 C4 19 00  C4 57 00 C4 0C 00 C4 48
\endverbatim
 *
 *      The 0070 is the offset within the versions of the b4uacuse-seq64.midi
 *      file.
 *
 *      Because of this issue, and the very slow speed of loading a MIDI file
 *      when built for debugging, we explored using an std::multimap instead
 *      of an std::list.  This worked better than a list, for loading MIDI
 *      events, but may cause the upper limit of the number of playing
 *      sequences to drop a little, due to the overhead of incrementing
 *      multimap iterators versus list iterators).  Now we use only the
 *      std::vector implementation, slightly faster than using std::list.
 *
 * \param rhs
 *      The object to be compared against.
 *
 * \return
 *      Returns true if the time-stamp and "rank" are less than those of the
 *      comparison object.
 */

bool
event::operator < (const event & rhs) const
{
    if (m_timestamp == rhs.m_timestamp)
        return get_rank() < rhs.get_rank();
    else
        return m_timestamp < rhs.m_timestamp;
}

/**
 *  Indicates that the events are "identical".  This function is not a
 *  replacement for operator < ().  It is meant to be used in a brute force
 *  search for one particular event in the sorted event list.
 *
 *  SysEx (or text) data are not checked, just the status and channel values..
 */

bool
event::match (const event & target) const
{
    bool result = false;
    bool ignore_ts = is_null_midipulse(target.timestamp());
    if (ignore_ts || timestamp() == target.timestamp())
    {
        result =
        (
            get_status() == target.get_status() &&
            channel() == target.channel()
        );
        if (result && ! is_meta())                  /* additional matching  */
        {
            result =
            (
                m_data[0] == target.m_data[0] &&
                m_data[1] == target.m_data[1]
            );
        }
    }
    return result;
}

/**
 *  Returns true if the event's status is *not* a control-change, but does
 *  match the given status OR if the event's status is a control-change that
 *  matches the given status, and has a control value matching the given
 *  control-change value.
 *
 * \param status
 *      The status to be checked.
 *
 * \param cc
 *      The controller value to be matched, for control-change events.
 */

bool
event::is_desired (midibyte status, midibyte cc) const
{
    bool result;
    if (is_tempo_status(status))
    {
        result = is_tempo();
    }
    else
    {
        midibyte s = mask_status(status);
        midibyte ms = mask_status(m_status);            /* ca 2023-05-22    */
        result = s == ms;
        if (result && (s == EVENT_CONTROL_CHANGE))
            result = m_data[0] == cc;
    }
    return result;
}

/**
 *  We should also match tempo events here.  But we have to treat them
 *  differently from the matched status events.
 */

bool
event::is_desired_ex (midibyte status, midibyte cc) const
{
    bool result;                            /* is_desired_cc_or_not_cc      */
    bool match = match_status(status);
    if (status == EVENT_CONTROL_CHANGE)
    {
        result = match && m_data[0] == cc;  /* correct status & correct CC  */
    }
    else
    {
        if (is_tempo())
            result = true;                  /* Set tempo always editable    */
        else
            result = match;                 /* correct status and not CC    */
    }
    return result;
}

/**
 *  Sometimes we need to alter the event completely.
 */

void
event::set_data
(
    midipulse tstamp,
    midibyte status,
    midibyte d0,
    midibyte d1
)
{
    set_timestamp(tstamp);
    set_status(status);
    set_data(d0, d1);
}

/**
 *  Transpose the note, if possible.
 *
 * \param tn
 *      The amount (positive or negative) to transpose a note.  If the result
 *      is out of range, the transposition is not performered.
 */

void
event::transpose_note (int tn)
{
    int note = int(m_data[0]) + tn;
    if (note >= 0 && note < c_midibyte_data_max)
        m_data[0] = midibyte(note);
}

void
event::set_channel (midibyte channel)
{
    if (is_null_channel(channel))
    {
        m_channel = channel;
    }
    else
    {
        int chan = mask_channel(channel);           /* clears status nybble */
        m_channel = chan;
        if (has_channel())                          /* a channel message    */
            m_status = mask_status(m_status) | chan;
    }
}

/**
 *  Sets the m_status member to the value of status.  If \a status is a
 *  channel event, then the channel portion of the status is cleared using
 *  a bitwise AND against EVENT_GET_STATUS_MASK.  This version is basically
 *  the Seq24 version with the additional setting of the Seq66-specific
 *  m_channel member.
 *
 *  Found in yet another fork of seq24: // ORL fait de la merde
 *  He also provided a very similar routine: set_status_midibus().
 *
 *  Stazed:
 *
 *      The record parameter, if true, does not clear channel portion
 *      on record for channel specific recording. The channel portion is
 *      cleared in sequence::stream_event() by calling set_status() (record
 *      = false) after the matching channel is determined.  Otherwise, we use
 *      a bitwise AND to clear the channel portion of the status.  All events
 *      will be stored without the channel nybble.  This is necessary since
 *      the channel is appended by midibus::play() based on the track.
 *
 *  Instead of adding a "record" parameter to set_status(), we provide a more
 *  specific function, set_status_keep_channel(), for use in the mastermidibus
 *  class.  This usage also has the side-effect of allowing the usage of
 *  channel in the MIDI-control feature.
 *
 * \param status
 *      The status byte, perhaps read from a MIDI file or edited in the
 *      sequencer's event editor.  Sometimes, this byte will have the channel
 *      nybble masked off.  If that is the case, the eventcode/channel
 *      overload of this function is more appropriate.  Only values with the
 *      highest bit set are allowed, as per the MIDI specification.
 */

void
event::set_status (midibyte status)
{
    if (status >= EVENT_MIDI_SYSEX)             /* 0xF0 and above           */
    {
        m_status = status;
        m_channel = null_channel();             /* channel "not applicable" */
    }
    else if (status >= EVENT_NOTE_OFF)          /* 0x80 to 0xEF             */
    {
        m_status = mask_status(status);
        m_channel = mask_channel(status);
    }
}

/**
 *  This overload is useful when synthesizing events, such as converting a
 *  Note On event with a velocity of zero to a Note Off event.  See its usage
 *  in midifile and qseventslots, for example.
 *
 * \param status
 *      The status byte, perhaps read from a MIDI file. If the event is a
 *      channel event, it will have its channel updated via the \a channel
 *      parameter as well.
 *
 * \param channel
 *      The channel byte.  Combined with the event-code, this makes a valid
 *      MIDI "status" byte.  This byte is masked to guarantee the high nybble
 *      is clear.
 */

void
event::set_channel_status (midibyte status, midibyte channel)
{
    m_status = status;
    set_channel(channel);
}

/**
 *  This function is used in recording to preserve the input channel
 *  information for deciding what to do with an incoming MIDI event.
 *  It replaces stazed's set_status() that had an optional "record"
 *  parameter.  This call allows channel to be detected and used in MIDI
 *  control events.  It "keeps" the channel in the status byte.
 *
 * \param eventcode
 *      The status byte, generally read from the MIDI buss.  If it is not a
 *      channel message, the channel is not modified.
 */

void
event::set_status_keep_channel (midibyte eventcode)
{
    m_status = eventcode;
    if (is_channel_msg(eventcode))
        m_channel = mask_channel(eventcode);
}

#if defined SEQ66_THIS_FUNCTION_IS_USED

void
event::set_note_off (int note, midibyte channel)
{
    midibyte chan = mask_channel(channel);
    m_status = EVENT_NOTE_OFF | chan;
    set_data(midibyte(note), 0);
}

#endif

/**
 *  In relation to issue #206.
 *
 *  Combines a bunch of common operations in getting events.  Code used in:
 *
 *      -   midi_in_jack::api_get_midi_event(event *)
 *      -   midi_alsa_info::api_get_midi_event(event *)
 *
 *  Can we use it in these contexts?
 *
 *      -   wrkfile::NoteArray(int, int)
 *      -   midifile::parse_smf_1(...)      [very unlikely]
 *
 *  Some keyboards send Note On with velocity 0 for Note Off, so we take care
 *  of that situation here by creating a Note Off event, with the channel
 *  nybble preserved. Note that we call event::set_status_keep_channel()
 *  instead of using stazed's set_status function with the "record" parameter.
 *  Also, we have to mask in the actual channel number.
 *
 *  Encapsulates some common code.  This function assumes we have already set
 *  the status and data bytes.
 */

bool
event::set_midi_event
(
    midipulse timestamp,
    const midibyte * buffer,
    int count
)
{
    bool result = true;
    set_timestamp(timestamp);
    set_sysex_size(count);
    if (count == 0)             /* portmidi: analyze the event to get count */
    {
        if (is_two_byte_msg(buffer[0]))
            count = 3;
        else if (is_one_byte_msg(buffer[0]))
            count = 2;
        else
            count = 1;
    }
    if (count == 3)
    {
        set_status_keep_channel(buffer[0]);
        set_data(buffer[1], buffer[2]);
        if (is_note_off_recorded())
        {
            midibyte channel = mask_channel(buffer[0]);
            midibyte status = EVENT_NOTE_OFF | channel;
            set_status_keep_channel(status);
        }
    }
    else if (count == 2)
    {
        set_status_keep_channel(buffer[0]);
        set_data(buffer[1]);
    }
    else if (count == 1)
    {
        set_status(buffer[0]);
        clear_data();
    }
    else
    {
        if (buffer[0] == EVENT_MIDI_SYSEX)
        {
            reset_sysex();            /* set up for sysex if needed   */
            if (! append_sysex(buffer, count))
            {
                errprint("event::append_sysex() failed");
            }
        }
        else
            result = false;
    }
    return result;
}

/**
 *  Sets a Meta event.  Meta events have a status byte of EVENT_MIDI_META ==
 *  0xff and a channel value that reflects the type of Meta event (e.g. 0x51
 *  for a "Set Tempo" event.
 *
 *  Note that the data bytes (if any) for this event will still need to be
 *  added to the event via (for example) the append_sysex() or set_sysex()
 *  function.
 *
 * \param metatype
 *      Indicates the type of meta event.
 */

void
event::set_meta_status (midibyte metatype)
{
    m_status = EVENT_MIDI_META;
    m_channel = metatype;
}

/**
 *  This base class version simply returns the bytes as characters in a
 *  string.
 *
 * \return
 *      Returns the text if valid, otherwise returns an empty string.
 *
 *      Note
 *      that the text is in "midi-bytes" format, where characters greater
 *      than 127 are encodes as a hex value, "\xx".
 */

std::string
event::get_text () const
{
    std::string result;
    size_t dsize = m_sysex.size();
    for (size_t i = 0; i < dsize; ++i)
    {
        char c = char(m_sysex[i]);
        result.push_back(c);
    }
    return result;
}

/**
 *  This base-class version unconditionally loads bytes into the
 *  m_sysex vector.
 */

bool
event::set_text (const std::string & s)
{
    bool result = ! s.empty();
    if (result)
    {
        m_sysex.clear();
        for (const auto c : s)
            m_sysex.push_back(c);
    }
    return result;
}

/**
 *  Appends Meta-event data to a new buffer.  Similar to append_sysex(), but
 *  useful for holding the data for a Meta event.  Please note that Meta
 *  events and SysEx events shared the same "extended" data buffer that
 *  originated to support SysEx.
 *
 *  Also see set_meta_status(), which, like this function, sets the
 *  event::m_channel_member to indicate the type of Meta event, but, unlike
 *  this function, leaves the data alone.  Also note that the set_status()
 *  call in midifile flags the event as a Meta event.  The handling of Meta
 *  events is not yet uniform between all the modules.
 *
 * \warning
 *      Currently does not clear the "sysex" buffer first.
 *
 * \param metatype
 *      Provides the type of the Meta event, which is stored in the m_channel
 *      member.
 *
 * \param data
 *      Provides the Meta event's data.  If not provided, nothing is done,
 *      and false is returned.
 *
 * \param dsize
 *      Provides the size of the data.  If not provided, nothing is done.
 *
 * \return
 *      Returns false if an error occurred, and the caller needs to stop
 *      trying to process the data.
 */

bool
event::append_meta_data (midibyte metatype, const midibyte * data, int dsize)
{
    bool result = not_nullptr(data) && (dsize > 0);
    if (result)
    {
        set_meta_status(metatype);
        for (int i = 0; i < dsize; ++i)
            m_sysex.push_back(data[i]);
    }
    else
    {
        errprint("event::append_meta_data(null data)");
    }
    return result;
}

/**
 *  This overload appends Meta-event data from a vector to a new buffer.
 *
 * \param metatype
 *      Provides the type of the Meta event, which is stored in the m_channel
 *      member.
 *
 * \param data
 *      Provides the Meta event's data as a vector.
 *
 * \return
 *      Returns false if an error occurred, and the caller needs to stop
 *      trying to process the data.
 */

bool
event::append_meta_data (midibyte metatype, const midibytes & data)
{
    int dsize = int(data.size());
    bool result = dsize > 0;
    if (result)
    {
        set_meta_status(metatype);
        for (int i = 0; i < dsize; ++i)
             m_sysex.push_back(data[i]);
    }
    else
    {
        errprint("event::append_meta_data(no data)");
    }
    return result;
}

/**
 *  An overload for logging SYSEX data byte-by-byte.
 *
 * \param data
 *      A single MIDI byte of data, assumed to be part of a SYSEX message
 *      event.
 *
 * \return
 *      Returns true if the event is not a SysEx-end event.
 */

bool
event::append_sysex_byte (midibyte data)
{
    m_sysex.push_back(data);
    return data != EVENT_MIDI_SYSEX_END;
}

/**
 *  Appends SYSEX data to a new buffer.  We now use a vector instead of an
 *  array, so there is no need for reallocation and copying of the current
 *  SYSEX data.  The data represented by data and dsize is appended to that
 *  data buffer.
 *
 * \param data
 *      Provides the additional SysEx/Meta data.  If not provided, nothing is
 *      done, and false is returned.
 *
 * \param dsize
 *      Provides the size of the additional SYSEX data.  If not provided,
 *      nothing is done.
 *
 * \return
 *      Returns true if there was data to add.  The End-of-SysEx byte is
 *      included.
 */

bool
event::append_sysex (const midibyte * data, int dsize)
{
    bool result = not_nullptr(data) && (dsize > 0);
    if (result)
    {
        for (int i = 0; i < dsize; ++i)
            m_sysex.push_back(data[i]);
    }
    else
    {
        errprint("event::append_sysex(): null parameters");
    }
    return result;
}

bool
event::append_sysex (const midibytes & data)
{
    bool result = ! data.empty();
    if (result)
    {
        for (auto b : data)
            m_sysex.push_back(b);
    }
    else
    {
        errprint("event::append_sysex(): no data");
    }
    return result;
}

/**
 *  Resets and adds ex data.
 *
 * \param data
 *      Provides the SysEx/Meta data.  If not provided, nothing is done,
 *      and false is returned.
 *
 * \param len
 *      The number of bytes to set.
 *
 * \return
 *      Returns true if the function succeeded.
 */

bool
event::set_sysex (const midibyte * data, int len)
{
    reset_sysex();
    return append_sysex(data, len);
}

bool
event::set_sysex (const midibytes & data)
{
    reset_sysex();
    return append_sysex(data);
}

void
event::set_sysex_size (int len)
{
    if (len == 0)
        m_sysex.clear();
    else if (len > 0)
        m_sysex.resize(len);
}

/**
 *  Prints out the timestamp, data size, the current status byte, channel
 *  (which is the type value for Meta events), any SysEx or
 *  Meta data if present, or the two data bytes for the status byte.
 *
 *  There's really no percentage in converting this code to use std::cout, we
 *  feel.  We might want to make it contingent on the --verbose option at some
 *  point.
 */

void
event::print (const std::string & tag) const
{
    std::string buffer = to_string();
    if (tag.empty())
        printf("%s", buffer.c_str());
    else
        printf("%s: %s", tag.c_str(), buffer.c_str());
}

void
event::print_note (bool showlink) const
{
    if (is_note())
    {
        bool shownote = is_note_on() || (is_note_off() && ! showlink);
        if (shownote)
        {
            std::string type = is_note_on() ? "On " : "Off" ;
            char channel[8];
            if (m_channel == null_channel())
            {
                channel[0] = '-';
                channel[1] = 0;
            }
            else
            {
                snprintf(channel, sizeof channel, "%1x", int(m_channel));
            }
            printf
            (
                "%06ld Note %s:%s %3d Vel %02X",
                long(m_timestamp), type.c_str(), channel,
                int(m_data[0]), int(m_data[1])
            );
            if (is_linked() && showlink)
            {
                const_iterator mylink = link();
                printf(" --> ");
                mylink->print_note(false);
            }
            else
                printf("\n");
        }
    }
}

/**
 *  Prints out the timestamp, data size, the current status byte, channel
 *  (which is the type value for Meta events), any SysEx or
 *  Meta data if present, or the two data bytes for the status byte.
 *
 *  There's really no percentage in converting this code to use std::cout, we
 *  feel.  We might want to make it contingent on the --verbose option at some
 *  point.
 */

std::string
event::to_string () const
{
    char tmp[64];
    (void) snprintf(tmp, sizeof tmp, "[%06ld] (", long(m_timestamp));

    const char * label = is_meta() ? "type" : "channel" ;
    std::string result = tmp;
    result += is_linked() ? "L" : " ";
    result += is_marked() ? "M" : " ";
    result += is_selected() ? "S" : " ";
    result += is_painted() ? "P" : " ";
    result += ") ";
    (void) snprintf
    (
        tmp, sizeof tmp, "event 0x%02X %s 0x%02X d0=%d d1=%d\n",
        unsigned(m_status), label, unsigned(m_channel),
        int(m_data[0]), int(m_data[1])
    );
    result += tmp;
    if (is_sysex() || is_meta())
    {
        bool use_linefeeds = sysex_size() > 8;
        (void) snprintf(tmp, sizeof tmp, "SysEx/Meta[%d]:   ", sysex_size());
        result += tmp;
        for (int i = 0; i < sysex_size(); ++i)
        {
            if (use_linefeeds && (i % 16) == 0)
                result += "\n         ";

            (void) snprintf(tmp, sizeof tmp, "%02X ", m_sysex[i]);
            result += tmp;
        }
        result += "\n";
    }
    return result;
}

void
event::rescale (int newppqn, int oldppqn)
{
    set_timestamp(rescale_tick(timestamp(), newppqn, oldppqn));
}

/**
 *  This function is used in sorting MIDI status events (e.g. note on/off,
 *  aftertouch, control change, etc.)  The sort order is not determined by the
 *  actual status values.
 *
 *  The ranking, from high to low, is note off, note on, aftertouch, channel
 *  pressure, and pitch wheel, control change, and program changes.  The lower
 *  the ranking, the more upfront an item comes in the sort order, given the
 *  same time-stamp.
 *
 * Note:
 *      We could add the channel number as part of the ranking. Sound?
 *
 * \return
 *      Returns the rank of the current m_status byte.
 */

int
event::get_rank () const
{
    int result;
    int eventcode = mask_status(m_status);  /* strip off channel nybble     */
    switch (eventcode)
    {
    case EVENT_NOTE_OFF:
        result = 0x2000 + get_note();
        break;

    case EVENT_NOTE_ON:
        result = 0x1000 + get_note();
        break;

    case EVENT_AFTERTOUCH:
    case EVENT_CHANNEL_PRESSURE:
    case EVENT_PITCH_WHEEL:
        result = 0x0050;
        break;

    case EVENT_CONTROL_CHANGE:
        result = 0x0020;
        break;

    case EVENT_PROGRAM_CHANGE:
        result = 0x0010;
        break;

    default:
        result = 0;
        break;
    }
    if (result != 0)
        result += mask_channel(m_status) << 8;

    return result;
}

/**
 *  Calculates the tempo from the stored event bytes, if the event is a Tempo
 *  meta-event and has valid data.  Remember that we are overloading the SysEx
 *  support to hold Meta-event data.  Also note that we're treating the vector
 *  like an array, which is supposed to work.
 *
 * \return
 *      Returns the result of calculating the tempo from the three data bytes.
 *      If an error occurs, 0.0 is returned.
 */

midibpm
event::tempo () const
{
    midibpm result = 0.0;
    if (is_tempo() && sysex_size() == 3)
    {
        midibyte b[3];
        b[0] = m_sysex[0];                  /* convert vector to array type */
        b[1] = m_sysex[1];
        b[2] = m_sysex[2];
        result = bpm_from_bytes(b);
    }
    return result;
}

/**
 *  The inverse of tempo().  First, we convert beats/minute to a tempo
 *  microseconds value.  Then we convert the microseconds to three tempo
 *  bytes.
 */

bool
event::set_tempo (midibpm tempo)
{
    double us = tempo_us_from_bpm(tempo);
    midibyte t[3];
    tempo_us_to_bytes(t, midibpm(us));
    return set_sysex(t, 3);
}

bool
event::set_tempo (midibyte t[3])
{
    double tt = tempo_us_from_bytes(t);
    bool result = tt > 0.0;
    if (result)
        set_sysex(t, 3);

    return result;
}

/*
 *  Helper functions for alteration of events.
 */

/**
 *  Modifies the timestamp of the event by plus or minus the range value.
 *
 * \param range
 *      The range of the changes up and down. Not used if 0 or less.
 *
 * \return
 *      Returns true if the timestamp was actually jittered.
 */

bool
event::jitter (int range, midipulse seqlength)
{
    bool result = range > 0;
    if (result)
    {
        midipulse delta = midipulse(randomize(range));
        midipulse tstamp = timestamp() + delta;
        result = delta != 0;
        if (tstamp < 0)
            tstamp = 0;
        else if (tstamp >= seqlength)
            tstamp = seqlength - 1;

        set_timestamp(tstamp);
    }
    return result;
}

/**
 *  Modifies the velocity. However, the caller will like not want to
 *  change the velocity of a Note On with velocity 0.
 *
 * \param range
 *      The range of the changes up and down. Not used if 0 or less.
 *
 * \return
 *      Returns true if the timestamp was actually jittered.
 */

bool
event::randomize (int range)
{
    bool result = range > 0;
    if (result)
    {
        bool twobytes = is_two_bytes();
        int datum = int(twobytes ? m_data[1] : m_data[0]);
        datum += seq66::randomize(range);

        midibyte d = clamp_midibyte_value(datum);
        if (twobytes)
            m_data[1] = d;
        else
            m_data[0] = d;
    }
    return result;
}

/**
 *  Division by 2 "tightens" toward the nearest snap time.
 */

bool
event::tighten (int snap, midipulse seqlength)
{
    bool result = snap > 0;
    if (result)
    {
        midipulse t = timestamp();
        midipulse tremainder = t % snap;
        midipulse tdelta;
        if (tremainder < snap / 2)
            tdelta = -(tremainder / 2);
        else
            tdelta = (snap - tremainder) / 2;

        if ((tdelta + t) >= seqlength)  /* wrap-around Note On      */
            tdelta = -t;

        set_timestamp(t + tdelta);
    }
    return result;
}

/**
 *  Quantizes the time-stamp toward the nearest snap time.
 */

bool
event::quantize (int snap, midipulse seqlength)
{
    bool result = snap > 0;
    if (result)
    {
        midipulse t = timestamp();
        midipulse tremainder = t % snap;
        midipulse tdelta;
        if (tremainder < snap / 2)
            tdelta = -tremainder;
        else
            tdelta = (snap - tremainder);

        if ((tdelta + t) >= seqlength)  /* wrap-around Note On      */
            tdelta = -t;

        set_timestamp(t + tdelta);
    }
    return result;
}

/*
 * --------------------------------------------------------------
 *  event::key
 * --------------------------------------------------------------
 */

/**
 *  Principal event::key constructor.
 *
 * \param tstamp
 *      The time-stamp is the primary part of the key.  It is the most
 *      important key item.
 *
 * \param rank
 *      Rank is an arbitrary number used to order events that have the
 *      same time-stamp.  See the event::get_rank() function for more
 *      information.
 */

event::key::key (midipulse tstamp, int rank) :
    m_timestamp (tstamp),
    m_rank      (rank)
{
    // Empty body
}

/**
 *  Event-based constructor.  This constructor makes it even easier to
 *  create an key.  Note that the call to event::get_rank() makes a
 *  simple calculation based on the status of the event.
 *
 * \param rhs
 *      Provides the event key to be copied.
 */

event::key::key (const event & rhs) :
    m_timestamp (rhs.timestamp()),
    m_rank      (rhs.get_rank())
{
    // Empty body
}

/**
 *  Provides the minimal operator needed to sort events using an key.
 *
 * \param rhs
 *      Provides the event key to be compared against.
 *
 * \return
 *      Returns true if the rank and timestamp of the current object are less
 *      than those of rhs.
 */

bool
event::key::operator < (const key & rhs) const
{
    if (m_timestamp == rhs.m_timestamp)
        return (m_rank < rhs.m_rank);
    else
        return (m_timestamp < rhs.m_timestamp);
}

/**
 *  Necessary for comparing keys directly (rather than in sorting).
 */

bool
event::key::operator == (const key & rhs) const
{
    return (m_timestamp == rhs.m_timestamp) && m_rank == rhs.m_rank;
}

/*
 * --------------------------------------------------------------
 *  Free functions
 * --------------------------------------------------------------
 */

/**
 *  Creates and returns a tempo event.
 */

event
create_tempo_event (midipulse tick, midibpm tempo)
{
    event e;
    e.set_meta_status(EVENT_META_SET_TEMPO);
    e.set_timestamp(tick);
    e.set_tempo(tempo);
    return e;
}

}           // namespace seq66

/*
 * event.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


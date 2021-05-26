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
 * \file          event.cpp
 *
 *  This module declares/defines the base class for MIDI events.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2021-05-19
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

#include "app_limits.h"                 /* various macro constants          */
#include "midi/event.hpp"               /* seq66::event class               */
#include "util/basic_macros.hpp"        /* helpful debugging/build macros   */
#include "util/calculations.hpp"        /* functions for math and stuff     */

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
    m_status        (EVENT_NOTE_OFF),
    m_channel       (max_midibyte()),       /* 0xFF                 */
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
    m_status        (mask_status(status)),  /* remove the channel   */
    m_channel       (max_midibyte()),       /* 0xFF                 */
    m_data          (),                     /* a two-element array  */
    m_sysex         (),                     /* an std::vector       */
    m_linked        (nullptr),
    m_has_link      (false),
    m_selected      (false),
    m_marked        (false),
    m_painted       (false)
{
    set_data(d0, d1);
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
 *  The restart_sysex() function does what we need.  But now that m_sysex is a
 *  vector, no action is needed.
 */

event::~event ()
{
    // restart_sysex() no longer necessary
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
 *      The 0070 is the offset within the versions of the
 *      b4uacuse-seq64.midi file.
 *
 *      Because of this issue, and the very slow speed of loading a MIDI file
 *      when built for debugging, we explored using an std::multimap instead of
 *      an std::list.  [The SEQ66_USE_EVENT_MAP macro is now OBSOLETE].
 *      This worked better than a list, for loading MIDI events, but may cause
 *      the upper limit of the number of playing sequences to drop a little, due
 *      to the overhead of incrementing multimap iterators versus list
 *      iterators).  Now we use only the std::vector implementation, slightly
 *      faster than using std::list.
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

/**
 *  Sets the m_status member to the value of status.  If \a status is a
 *  channel event, then the channel portion of the status is cleared using
 *  a bitwise AND against EVENT_CLEAR_CHAN_MASK.  This version is basically
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
 *      overload of this function is more appropriate.
 */

void
event::set_status (midibyte status)
{
    if (status >= EVENT_MIDI_SYSEX)             /* 0xF0 and above           */
    {
        m_status = status;
        m_channel = c_midibyte_max;             /* channel "not applicable" */
    }
    else
    {
        m_status = mask_status(status);
        m_channel = get_channel(status);
    }
}

/**
 *  This overload is useful when synthesizing events, such as converting a
 *  Note On event with a velocity of zero to a Note Off event.  See its usage
 *  around line 681 of midifile.cpp.
 *
 * \param eventcode
 *      The status byte, perhaps read from a MIDI file.  This byte is
 *      assumed to have already had its low nybble cleared by masking against
 *      EVENT_CLEAR_CHAN_MASK.
 *
 * \param channel
 *      The channel byte.  Combined with the event-code, this makes a valid
 *      MIDI "status" byte.  This byte is assumed to have already had its high
 *      nybble cleared by masking against EVENT_GET_CHAN_MASK.
 */

void
event::set_channel_status (midibyte eventcode, midibyte channel)
{
    m_status = eventcode;               /* already masked against 0xF0      */
    if (! is_null_channel(channel))
        m_channel = channel;            /* already masked against 0x0F :-D  */
}

/**
 *  This function is used in recording to preserve the input channel
 *  information for deciding what to do with an incoming MIDI event.
 *  It replaces stazed's set_status() that had an optional "record"
 *  parameter.  This call allows channel to be detected and used in MIDI
 *  control events.  It "keeps" the channel in the status byte.
 *
 * \todo
 *      THIS function WILL set a BOGUS CHANNEL on events >= SYSEX !!!!!
 *
 * \param eventcode
 *      The status byte, generally read from the MIDI buss.
 */

void
event::set_status_keep_channel (midibyte eventcode)
{
    m_status = eventcode;
    m_channel = get_channel(eventcode);
}

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
#if defined SEQ66_PLATFORM_DEBUG_TMI
    printf
    (
        "set_midi_event([%ld], status %02x, d0 %02X, d1 %02X, %d bytes)\n",
        timestamp, buffer[0], buffer[1], buffer[2], count
    );
#endif
    if (count == 3)
    {
        set_status_keep_channel(buffer[0]);
        set_data(buffer[1], buffer[2]);
        if (is_note_off_recorded())
        {
            midibyte channel = get_channel(buffer[0]);
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
            restart_sysex();            /* set up for sysex if needed   */
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
 *  Deletes and clears out the SYSEX buffer.  (The m_sysex member used to be a
 *  pointer.)  This function also causes get_sysex_size() to return 0.
 */

void
event::restart_sysex ()
{
    m_sysex.clear();
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
 *      Returns false if there was an EVENT_MIDI_SYSEX_END byte in the
 *      appended data, or if an error occurred, and the caller needs to stop
 *      trying to process the data.  We're not quite sure what to do with any
 *      extra data that remains.
 */

bool
event::append_sysex (const midibyte * data, int dsize)
{
    bool result = not_nullptr(data) && (dsize > 0);
    if (result)
    {
        for (int i = 0; i < dsize; ++i)
        {
            m_sysex.push_back(data[i]);
            if (data[i] == EVENT_MIDI_SYSEX_END)
            {
                result = false;
                break;                  /* is this the right thing to do? */
            }
        }
    }
    else
    {
        errprint("event::append_sysex(): null parameters");
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
        errprint("event::append_meta_data(): null data");
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
event::append_meta_data (midibyte metatype, const std::vector<midibyte> & data)
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
        errprint("event::append_meta_data(): no data");
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
event::append_sysex (midibyte data)
{
    m_sysex.push_back(data);
    return data != EVENT_MIDI_SYSEX_END;
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
event::print () const
{
    std::string buffer = to_string();
    printf("%s", buffer.c_str());
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
            if (m_channel == c_midibyte_max)
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
                m_timestamp, type.c_str(), channel,
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

    std::string result = tmp;
    result += is_linked() ? "L" : " ";
    result += is_marked() ? "M" : " ";
    result += is_selected() ? "S" : " ";
    result += is_painted() ? "P" : " ";
    result += ") ";
    (void) snprintf
    (
        tmp, sizeof tmp, "status %02X chan/type %02X d0=%d d1=%d\n",
        unsigned(m_status), unsigned(m_channel),
        int(m_data[0]), int(m_data[1])
    );
    result += tmp;
    if (is_sysex() || is_meta())
    {
        bool use_linefeeds = get_sysex_size() > 8;
        (void) snprintf(tmp, sizeof tmp, "SysEx/Meta[%d]:   ", get_sysex_size());
        result += tmp;
        for (int i = 0; i < get_sysex_size(); ++i)
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
 *  the ranking the more upfront an item comes in the sort order.
 *
 * \return
 *      Returns the rank of the current m_status byte.
 */

int
event::get_rank () const
{
    switch (m_status)
    {
    case EVENT_NOTE_OFF:
        return 0x200 + get_note();

    case EVENT_NOTE_ON:
        return 0x100 + get_note();

    case EVENT_AFTERTOUCH:
    case EVENT_CHANNEL_PRESSURE:
    case EVENT_PITCH_WHEEL:
        return 0x050;

    case EVENT_CONTROL_CHANGE:
        return 0x010;

    case EVENT_PROGRAM_CHANGE:
        return 0x000;

    default:
        return 0;
    }
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
    if (is_tempo() && get_sysex_size() == 3)
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
 *  microseconds value.  Then we convert the microseconds to threee tempo
 *  bytes.
 */

bool
event::set_tempo (midibpm tempo)
{
    double us = tempo_us_from_bpm(tempo);
    midibyte t[3];
    tempo_us_to_bytes(t, us);
    return set_sysex(t, 3);
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

event::key::key (midipulse tstamp, int rank)
 :
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

event::key::key (const event & rhs)
 :
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


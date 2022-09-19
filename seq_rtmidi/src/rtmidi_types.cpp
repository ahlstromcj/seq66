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
 * \file          rtmidi_types.cpp
 *
 *    Classes that used to be structs.
 *
 * \library       seq66 application
 * \author        Gary P. Scavone; severe refactoring by Chris Ahlstrom
 * \date          2016-12-01
 * \updates       2022-09-17
 * \license       See above.
 *
 *  Provides some basic types for the (heavily-factored) rtmidi library, very
 *  loosely based on Gary Scavone's RtMidi library.
 */

#include <cstring>                      /* std::memcpy()                    */

#include "rtmidi_types.hpp"             /* seq66::rtmidi, etc.              */
#include "util/basic_macros.hpp"        /* errprintfunc() macro, etc.       */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 * class midi_message
 */

/**
 *  Constructs an empty MIDI message.  Well, empty except for the timestamp
 *  bytes.  The caller will then push the data bytes for the MIDI event.
 *
 * \param ts
 *      The timestamp of the event in ticks (pulses).
 */

midi_message::midi_message (midipulse ts) :
    m_bytes     (),
    m_timestamp (ts)
{
#if defined SEQ66_ENCODE_TIMESTAMP_FOR_JACK
    (void) push_timestamp(ts);
#endif
}

/**
 *  Constructs a midi_message from an array of bytes.
 *  Also sets the timestamp member based on the first few (4 or 8) bytes.
 *
 * \param mbs
 *      Provides the data, which should start with the timestamp bytes, and
 *      optionally followed by event bytes.
 *
 * \param sz
 *      The putative size of the byte array.
 */

midi_message::midi_message (const midibyte * mbs, size_t sz) :
    m_bytes     (),
    m_timestamp (0)
{
#if defined SEQ66_ENCODE_TIMESTAMP_FOR_JACK
    m_timestamp = extract_timestamp(mbs, sz);
#endif
    for (size_t i = 0; i < sz; ++i)
        m_bytes.push_back(*mbs++);
}

#if defined USE_EVENT_COPY_FUNCTION         // not yet needed

/**
 *  Copies only the event bytes to the destination buffer.
 *
 * \param destination
 *      The place to put the bytes.
 *
 * \param sz
 *      The size the caller allocated to the buffer.
 *
 * \return
 *      Returns true if the bytes could be copied.
 */

bool
midi_message::event_copy (midibyte * destination, size_t sz) const
{
    bool result = false;
    if (not_nullptr(destination) && (sz > 0))
    {
        if (size_t(event_count()) <= sz)
        {
            const midibyte * source = event_bytes();
            if (not_nullptr(source))
            {
                (void) std::memcpy(destination, source, sz);
                result = true;
            }
        }
    }
    return result;
}

#endif

/**
 *  These function handle adding a time-stamp to the message as bytes.
 *  The format of the midi_message is now:
 *
 *      -   4 bytes of a pulse (tick).
 *      -   The rest of the message bytes.
 *
 *  These implementations are based on midifile::write_long() and
 *  midifile::read_long().
 *
 *  However, using sizeof(midipulse) [i.e. sizeof(long)] is tricky, because,
 *  according to section 6.2.8 of Stroustrop's C++ 11 book, the size of a long
 *  is 4 at a minimum, but it is NOT guaranteed that it must be less than
 *  the size of a long long (8 bytes).  And in gcc/g++ the size of a long is
 *  8 bytes.  Do we want to encode the 4 MSB's that will always be 0?
 *
 *  The maximum 4-byte signed value is M = 2,147,483,648.  At our highest PPQN,
 *  P = 19200, this M / (P * 4) = 27962 measures.  At 320 beats/minutes, this
 *  would be almost 350 minutes of music.
 *
 *  For now, we will make it configurable to support 8 byte timestamps in
 *  a midi_message.
 *
 *  Remember that the bytes are pushed to the back of the vector.
 *
 * \return
 *      Returns true if the message was empty.  Otherwise, we cannot push a
 *      timestamp.
 */

bool
midi_message::push_timestamp (midipulse b)
{
    bool result = buffer_empty();
    if (result)
    {
#if defined SEQ66_8_BYTE_TIMESTAMPS
        push((b & 0xFF00000000000000) >> 56);
        push((b & 0xFF00000000000000) >> 48);
        push((b & 0xFF00000000000000) >> 40);
        push((b & 0xFF00000000000000) >> 32);
#endif
        push((b & 0x00000000FF000000) >> 24);
        push((b & 0x0000000000FF0000) >> 16);
        push((b & 0x000000000000FF00) >> 8);
        push((b & 0x00000000000000FF));
    }
    return result;
}

midipulse
midi_message::extract_timestamp () const
{
    midipulse result = 0;
    if (buffer_count() > int(size_of_timestamp()))
    {
#if defined SEQ66_8_BYTE_TIMESTAMPS
        result  = (midipulse(m_bytes[0]) << 56);
        result += (midipulse(m_bytes[1]) << 48);
        result += (midipulse(m_bytes[2]) << 40);
        result += (midipulse(m_bytes[3]) << 32);
        result += (midipulse(m_bytes[4]) << 24);
        result += (midipulse(m_bytes[5]) << 16);
        result += (midipulse(m_bytes[6]) << 8);
        result +=  midipulse(m_bytes[7]);
#else
        result  = (midipulse(m_bytes[0]) << 24);
        result += (midipulse(m_bytes[1]) << 16);
        result += (midipulse(m_bytes[2]) << 8);
        result +=  midipulse(m_bytes[3]);
#endif
    }
    return result;
}

/**
 *  Static function.
 */

midipulse
midi_message::extract_timestamp (const midibyte * mbs, size_t sz)
{
    midipulse result = 0;
    if (sz >= size_of_timestamp())
    {
#if defined SEQ66_8_BYTE_TIMESTAMPS
        result  = (midipulse(mbs[0]) << 56);
        result += (midipulse(mbs[1]) << 48);
        result += (midipulse(mbs[2]) << 40);
        result += (midipulse(mbs[3]) << 32);
        result += (midipulse(mbs[4]) << 24);
        result += (midipulse(mbs[5]) << 16);
        result += (midipulse(mbs[6]) << 8);
        result +=  midipulse(mbs[7]);
#else
        result  = (midipulse(mbs[0]) << 24);
        result += (midipulse(mbs[1]) << 16);
        result += (midipulse(mbs[2]) << 8);
        result +=  midipulse(mbs[3]);
#endif
    }
    return result;
}

void
midi_message::timestamp (midipulse t)
{
#if defined SEQ66_ENCODE_TIMESTAMP_FOR_JACK
    container temp;
    int c = buffer_count() - int(size_of_timestamp());
    bool havedata = c >= int(size_of_timestamp());
    if (havedata)
    {
        for (size_t i = size_of_timestamp(); i < size_t(c); ++i)
            temp.push_back(m_bytes[i]);
    }
    m_bytes.clear();
    if (push_timestamp(t) && havedata)
    {
        for (size_t i = 0; i < temp.size(); ++i)
            push(temp[i]);
    }
    m_bytes = temp;
#endif
    m_timestamp = t;
}

/**
 *  Shows the bytes in a message, for trouble-shooting.
 */

void
midi_message::show () const
{
    if (m_bytes.empty())
    {
        std::fprintf(stderr, "midi_message: empty\n");
        fflush(stderr);
    }
    else
    {
        midipulse ts = timestamp();
        std::fprintf(stderr, "midi_message (ts %ld):", long(ts));
        for (auto c : m_bytes)
        {
            std::fprintf(stderr, " 0x%2x", int(c));
        }
        std::fprintf(stderr, "\n");
        fflush(stderr);
    }
}

/*
 * class midi_queue
 */

/**
 *  Default constructor.
 */

midi_queue::midi_queue () :
    m_front     (0),
    m_back      (0),
    m_size      (0),
    m_ring_size (0),
    m_ring      (nullptr)
{
    allocate();
}

/**
 *  Destructor.
 */

midi_queue::~midi_queue ()
{
    deallocate();
}

/**
 *
 *  This would be better off as a constructor operation.  But one step at a
 *  time.
 */

void
midi_queue::allocate (unsigned queuesize)
{
    deallocate();
    if (queuesize > 0 && is_nullptr(m_ring))
    {
        m_ring = new(std::nothrow) midi_message[queuesize];
        if (not_nullptr(m_ring))
            m_ring_size = queuesize;
    }
}

/**
 *
 *  This would be better off as a destructor operation.  But one step at a
 *  time.
 */

void
midi_queue::deallocate ()
{
    if (not_nullptr(m_ring))
    {
        delete [] m_ring;
        m_ring = nullptr;
    }
}

/**
 *
 *  As long as we haven't reached our queue size limit, push the message.
 */

bool
midi_queue::add (const midi_message & mmsg)
{
    bool result = ! full();
    if (result)
    {
        m_ring[m_back++] = mmsg;
        if (m_back == m_ring_size)
            m_back = 0;

        ++m_size;
    }
    else
    {
        /*
         * errprintfunc("message queue limit reached");
         */
    }
    return result;
}

/**
 *  Pops, so to speak, the front message out of the queue, effectively
 *  throwing it away.  One useful call sequence is:
 *
\verbatim
    midi_message latest = queue.front();
    queue.pop();
\endverbatim
 *
 *  An alternative is to use the pop_front() function instead.
 */

void
midi_queue::pop ()
{
    --m_size;
    ++m_front;
    if (m_front == m_ring_size)
        m_front = 0;
}

/**
 *  Pops a copy of the front message.   Could be a little inefficient, since a
 *  couple of copies are made, and we cannot use return-code optimization.
 *
 *  Perhaps at some point we could use move semantics?
 *
 * \return
 *      Returns a copy of the message that was in front before the popping.
 *      If the queue is empty, an empty (all zeros) message is returned.
 *      Can be checked with the midi_message::empty() function.
 */

midi_message
midi_queue::pop_front ()
{
    midi_message result;
    if (m_size != 0)
    {
        result = m_ring[m_front];
        pop();
    }
    return result;
}

/*
 *  Class rtmidi_in_data is used to hold the data needed by some of the JACK
 *  callback functions.
 */

rtmidi_in_data::rtmidi_in_data ()
 :
    m_queue             (),
    m_first_message     (true),
    m_continue_sysex    (false)
{
    // no body
}

}           // namespace seq66

/*
 * rtmidi_types.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


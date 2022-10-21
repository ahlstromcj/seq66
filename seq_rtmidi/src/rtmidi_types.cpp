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
 * \updates       2022-10-18
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

#if defined SEQ66_PLATFORM_DEBUG
unsigned midi_message::sm_msg_number = 0;
#endif

/**
 *  Constructs an empty MIDI message.  Well, empty except for the timestamp
 *  bytes.  The caller will then push the data bytes for the MIDI event.
 *
 * \param ts
 *      The timestamp of the event in ticks (pulses).
 */

midi_message::midi_message (midipulse ts) :
#if defined SEQ66_PLATFORM_DEBUG
    m_msg_number    (sm_msg_number++),
#endif
    m_bytes         (),
    m_timestamp     (ts)
{
    // No code
}

/**
 *  Constructs a midi_message from an array of bytes.
 *
 * \param mbs
 *      Provides the data, which should start with the timestamp bytes, and
 *      optionally followed by event bytes.
 *
 * \param sz
 *      The putative size of the byte array.
 */

midi_message::midi_message (const midibyte * mbs, std::size_t sz) :
#if defined SEQ66_PLATFORM_DEBUG
    m_msg_number    (sm_msg_number++),
#endif
    m_bytes         (),
    m_timestamp     (0)
{
    for (std::size_t i = 0; i < sz; ++i)
        m_bytes.push_back(*mbs++);
}

/**
 *  Shows the bytes in a string, for trouble-shooting.  It includes only the
 *  timestamp and the first few bytes.
 */

std::string
midi_message::to_string () const
{
    unsigned long ts = (unsigned long)(timestamp());    /* pulse or frame   */
#if defined SEQ66_PLATFORM_DEBUG
    std::string result = "Event #";
    result += std::to_string(msg_number());
    result += " @ ";
#else
    std::string result = "Event @ ";
#endif
    result += std::to_string(ts);
    result += ":";

    int counter = 8;
    if (event_count() < counter)
        counter = event_count();

    for (int i = 0; i < counter; ++i)
    {
        result += " ";
        if (i == 0)
        {
            char temp[8];
            snprintf(temp, sizeof temp, "0x%2x", m_bytes[i]);
            result += temp;
        }
        else
            result += std::to_string(m_bytes[i]);
    }
    return result;
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


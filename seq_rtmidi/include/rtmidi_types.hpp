#if ! defined SEQ66_RTMIDI_TYPES_HPP
#define SEQ66_RTMIDI_TYPES_HPP

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
 * \file          rtmidi_types.hpp
 *
 *  Type definitions pulled out for the needs of the refactoring.
 *
 * \library       seq66 application
 * \author        Gary P. Scavone; severe refactoring by Chris Ahlstrom
 * \date          2016-11-20
 * \updates       2022-09-21
 * \license       See above.
 *
 *  The lack of hiding of these types within a class is a little to be
 *  regretted.  On the other hand, it does make the code much easier to
 *  refactor and partition, and slightly easier to read.
 */

#include <string>                           /* std::string                  */
#include <vector>                           /* std::vector container        */

#include "midi/event.hpp"                   /* seq66::event namespace       */
#include "midi/midibytes.hpp"               /* seq66::midibyte alias        */

/**
 * This was the version of the RtMidi library from which this reimplementation
 * was forked.  However, the divergence from RtMidi by this library is now
 * very great... only the idea of selecting the MIDI API at runtime, and the
 * queuing and call-back mechanism, have been preserved.
 */

#define SEQ66_RTMIDI_VERSION "2.1.1"        /* the revision at fork time    */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Default size of the MIDI queue.
 */

const int c_default_queue_size  = 100;

/**
 *    MIDI API specifier arguments.  These items used to be nested in
 *    the rtmidi class, but that only worked when RtMidi.cpp/h were
 *    large monolithic modules.
 */

enum class rtmidi_api
{
    unspecified,        /**< Search for a working compiled API.     */
    alsa,               /**< Advanced Linux Sound Architecture API. */
    jack,               /**< JACK Low-Latency MIDI Server API.      */

#if defined SEQ66_USE_RTMIDI_API_ALL

    /*
     * Not supported until we get a simplified seq66-friendly API worked out.
     * Windows (and presumably OSX) is currently supported by the Seq66
     * portmidi library.
     */

    macosx_core,        /**< Macintosh OS-X Core Midi API.          */
    windows_mm,         /**< Microsoft Multimedia MIDI API.         */
    dummy,              /**< A compilable but non-functional API.   */

#endif

    max                 /**< A count of APIs; an erroneous value.   */
};

/**
 *  Provides a container of API values.
 */

using rtmidi_api_list = std::vector<rtmidi_api>;

/*
 * Inline functions.
 */

inline rtmidi_api
int_to_api (int index)
{
    return index < static_cast<int>(rtmidi_api::max) ?
        static_cast<rtmidi_api>(index) : rtmidi_api::max ;
}

inline int
api_to_int (rtmidi_api api)
{
    return static_cast<int>(api);
}

/**
 *  The size of a Seq66 MIDI timestamp in bytes, or 0 if we're
 *  not encoding timestamps in the JACK ringbuffer.
 */

#if defined SEQ66_ENCODE_TIMESTAMP_FOR_JACK
#if defined SEQ66_8_BYTE_TIMESTAMPS
const size_t c_timestamp_size = 8;
#else
const size_t c_timestamp_size = 4;
#endif
#else
const size_t c_timestamp_size = 0;
#endif

/**
 *  Provides a handy capsule for a MIDI message, based on the
 *  std::vector<unsigned char> data type from the RtMidi project.
 *
 *  For issue #100, we add the timestamp (in units of MIDI ticks, also
 *  known as pulses) to the data.  We then provide functions to
 *  handle the array of data in two different ways:
 *
 *      -#  Buffer: Access the data buffer for all bytes, in order to put
 *          them on the JACK ringbuffer for the process callback to use:
 *          -   MIDI timestamp bytes (4)
 *          -   Status byte
 *          -   Data bytes
 *      -#  Event: Access the status and data bytes as a unit to pass them
 *          to the JACK engine for transmitting.
 *
 *  Please note that the ALSA module in seq66's rtmidi infrastructure
 *  uses the seq66::event rather than the seq66::midi_message object.
 *  For the moment, we will translate between them until we have the
 *  interactions between the old and new modules under control.
 */

class midi_message
{

public:

    /**
     *  Holds the data of the MIDI message.  Callers should use
     *  midi_message::container rather than using the vector directly.
     *  Bytes are added by the push() function, and are safely accessed
     *  (with bounds-checking) by operator [].
     */

    using container = std::vector<midibyte>;

private:

    /**
     *  Holds the event status and data bytes.
     */

    container m_bytes;

    /**
     *  Holds the (optional) timestamp of the MIDI message.  Non-zero only
     *  in the JACK implementation.
     */

    midipulse m_timestamp;

public:

    midi_message (midipulse ts = 0);
    midi_message (const midibyte * mbs, size_t sz);

#if defined SEQ66_ENCODE_TIMESTAMP_FOR_JACK
    static midipulse extract_timestamp (const midibyte * mbs, size_t sz);
    static size_t size_of_timestamp ()
    {
        return c_timestamp_size;
    }
#endif

    midibyte & operator [] (size_t i)
    {
        static midibyte s_zero = 0;
        return (i < m_bytes.size()) ? m_bytes[i] : s_zero ;
    }

    const midibyte & operator [] (size_t i) const
    {
        static midibyte s_zero = 0;
        return (i < m_bytes.size()) ? m_bytes[i] : s_zero ;
    }

    const char * buffer () const                // was "array"
    {
        return reinterpret_cast<const char *>(&m_bytes[0]);
    }

    int buffer_count () const
    {
        return int(m_bytes.size());
    }

    bool buffer_empty () const
    {
        return m_bytes.empty();
    }

    const midibyte * event_bytes () const       // bypasses timestamp
    {
#if defined SEQ66_ENCODE_TIMESTAMP_FOR_JACK
        return m_bytes.data() + size_of_timestamp();
#else
        return m_bytes.data();
#endif
    }

    bool empty () const
    {
        return event_count() == 0;
    }

    int event_count () const                    // was "count"
    {
#if defined SEQ66_ENCODE_TIMESTAMP_FOR_JACK
        return m_bytes.size() <= size_of_timestamp() ?
            0 : int(m_bytes.size() - size_of_timestamp()) ;
#else
        return int(m_bytes.size());
#endif
    }

    void push (midibyte b)
    {
        m_bytes.push_back(b);
    }

#if defined SEQ66_ENCODE_TIMESTAMP_FOR_JACK
    bool push_timestamp (midipulse b);
    midipulse extract_timestamp () const;
#endif

    midipulse timestamp () const
    {
        return m_timestamp;
    }

    void timestamp (midipulse t);

    bool is_sysex () const
    {
#if defined SEQ66_ENCODE_TIMESTAMP_FOR_JACK
        int index = int(size_of_timestamp());
#else
        int index = 0;
#endif
        return m_bytes.size() > 0 ?
            event::is_sysex_msg(m_bytes[index]) : false ;
    }

    void show () const;

private:

};          // class midi_message

/**
 *  MIDI caller callback function type definition.  Used to be nested in the
 *  rtmidi_in class.  The timestamp parameter has been folded into the
 *  midi_message class (a wrapper for std::vector<unsigned char>), and the
 *  pointer has been replaced by a reference.
 */

using rtmidi_callback_t = void (*)
(
    midi_message & message,             /* includes the timestamp already */
    void * userdata
);

/**
 *  Provides a queue of midi_message structures.  This entity used to be a
 *  plain structure nested in the midi_in_api class.  We made it a class to
 *  encapsulate some common operations to save a burden on the callers.
 */

class midi_queue
{

private:

    unsigned m_front;
    unsigned m_back;
    unsigned m_size;
    unsigned m_ring_size;
    midi_message * m_ring;

public:

    midi_queue ();
    ~midi_queue ();

    bool empty () const
    {
        return m_size == 0;
    }

    int count () const
    {
        return int(m_size);
    }

    bool full () const
    {
        return m_size == m_ring_size;
    }

    const midi_message & front () const
    {
        return m_ring[m_front];
    }

    bool add (const midi_message & mmsg);
    void pop ();
    midi_message pop_front ();
    void allocate (unsigned queuesize = c_default_queue_size);
    void deallocate ();

};          // class midi_queue

/**
 *  The rtmidi_in_data structure is used to pass private class data to the
 *  MIDI input handling function or thread.  Used to be nested in the
 *  rtmidi_in class.
 */

class rtmidi_in_data
{

private:

    /**
     *  Provides a queue of MIDI messages. Used when not using a JACK callback
     *  for MIDI input.
     */

    midi_queue m_queue;

    /**
     *  A one-time flag that starts out true and is falsified when the first
     *  MIDI messages comes in to this port.  It simply resets the delta JACK
     *  time.
     */

    bool m_first_message;

    /**
     *  Indicates that SysEx is still coming in.
     */

    bool m_continue_sysex;

public:

    rtmidi_in_data ();

    const midi_queue & queue () const
    {
        return m_queue;
    }

    midi_queue & queue ()
    {
        return m_queue;
    }

    bool first_message () const
    {
        return m_first_message;
    }

    void first_message (bool flag)
    {
        m_first_message = flag;
    }

    bool continue_sysex () const
    {
        return m_continue_sysex;
    }

    void continue_sysex (bool flag)
    {
        m_continue_sysex = flag;
    }

};          // class rtmidi_in_data

}           // namespace seq66

#endif      // SEQ66_RTMIDI_TYPES_HPP

/*
 * rtmidi_types.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


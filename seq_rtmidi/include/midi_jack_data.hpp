#if ! defined SEQ66_MIDI_JACK_DATA_HPP
#define SEQ66_MIDI_JACK_DATA_HPP

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
 * \file          midi_jack_data.hpp
 *
 *    Object for holding the current status of JACK and JACK MIDI data.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2017-01-02
 * \updates       2019-02-09
 * \license       See above.
 *
 *  GitHub issue #165: enabled a build and run with no JACK support.
 */

#include "midi/midibytes.hpp"           /* seq66::midibyte, other aliases   */

#if defined SEQ66_JACK_SUPPORT

#include <jack/jack.h>
#include <jack/ringbuffer.h>

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Contains the JACK MIDI API data as a kind of scratchpad for this object.
 *  This guy needs a constructor taking parameters for an rtmidi_in_data
 *  pointer.
 */

struct midi_jack_data
{
    /**
     *  Holds the JACK sequencer client pointer so that it can be used by the
     *  midibus objects.  This is actually an opaque pointer; there is no way
     *  to get the actual fields in this structure; they can only be accessed
     *  through functions in the JACK API.  Note that it is also stored as a
     *  void pointer in midi_info::m_midi_handle.  This item is the single
     *  JACK client created by the midi_jack_info object.
     */

    jack_client_t * m_jack_client;

    /**
     *  Holds the JACK port information of the JACK client.
     */

    jack_port_t * m_jack_port;

    /**
     *  Holds the size of data for communicating between the client
     *  ring-buffer and the JACK port's internal buffer.
     */

    jack_ringbuffer_t * m_jack_buffsize;

    /**
     *  Holds the data for communicating between the client ring-buffer and
     *  the JACK port's internal buffer.
     */

    jack_ringbuffer_t * m_jack_buffmessage;

    /**
     *  The last time-stamp obtained.  Use for calculating the delta time, I
     *  would imagine.
     */

    jack_time_t m_jack_lasttime;

    /**
     *  Holds special data peculiar to the client and its MIDI input
     *  processing.
     */

    rtmidi_in_data * m_jack_rtmidiin;

    /**
     * \ctor midi_jack_data
     */

    midi_jack_data () :
        m_jack_client       (nullptr),
        m_jack_port         (nullptr),
        m_jack_buffsize     (nullptr),
        m_jack_buffmessage  (nullptr),
        m_jack_lasttime     (0),
        m_jack_rtmidiin     (nullptr)
    {
        // Empty body
    }

    /**
     *  This destructor currently does nothing.  We rely on the enclosing class
     *  to close out the things that it created.
     */

    ~midi_jack_data ()
    {
        // Empty body
    }

    /**
     *  Tests that the buffer is good.
     */

    bool valid_buffer () const
    {
        return not_nullptr(m_jack_buffsize) && not_nullptr(m_jack_buffmessage);
    }

};          // class midi_jack_data

}           // namespace seq66

#endif      // SEQ66_JACK_SUPPORT

#endif      // SEQ66_MIDI_JACK_DATA_HPP

/*
 * midi_jack_data.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


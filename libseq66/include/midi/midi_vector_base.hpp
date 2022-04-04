#if ! defined SEQ66_MIDI_VECTOR_BASE_HPP
#define SEQ66_MIDI_VECTOR_BASE_HPP

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
 * \file          midi_vector_base.hpp
 *
 *  This module declares the abstract base class for the management of some
 *  MIDI events, using the sequence class.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-10-10
 * \updates       2021-10-04
 * \license       GNU GPLv2 or above
 *
 *  This class is meant to hold the bytes that represent MIDI events and other
 *  MIDI data, which can then be dumped to a MIDI file.
 */

#include <string>                       /* std::string class                */

#include "midi/midibytes.hpp"           /* seq66::midibyte                  */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class event;
    class performer;
    class sequence;
    class trigger;

/**
 *  Provides tags used by the midifile class to control the reading and
 *  writing of the extra "proprietary" information stored in a Seq24 MIDI
 *  file.  See the cpp file for more information.
 */

const midilong c_midibus        = 0x24240001; /**< Track buss number.         */
const midilong c_midichannel    = 0x24240002; /**< Track channel number.      */
const midilong c_midiclocks     = 0x24240003; /**< Track clocking.            */
const midilong c_triggers       = 0x24240004; /**< See c_triggers_ex.         */
const midilong c_notes          = 0x24240005; /**< Song data.                 */
const midilong c_timesig        = 0x24240006; /**< Track time signature.      */
const midilong c_bpmtag         = 0x24240007; /**< Song beats/minute.         */
const midilong c_triggers_ex    = 0x24240008; /**< Trigger data w/offset.     */
const midilong c_mutegroups     = 0x24240009; /**< Song mute group data.      */
const midilong c_gap_A          = 0x2424000A; /**< Gap. A.                    */
const midilong c_gap_B          = 0x2424000B; /**< Gap. B.                    */
const midilong c_gap_C          = 0x2424000C; /**< Gap. C.                    */
const midilong c_gap_D          = 0x2424000D; /**< Gap. D.                    */
const midilong c_gap_E          = 0x2424000E; /**< Gap. E.                    */
const midilong c_gap_F          = 0x2424000F; /**< Gap. F.                    */
const midilong c_midictrl       = 0x24240010; /**< Song MIDI control.         */
const midilong c_musickey       = 0x24240011; /**< The track's key. *         */
const midilong c_musicscale     = 0x24240012; /**< The track's scale. *       */
const midilong c_backsequence   = 0x24240013; /**< Track background sequence. */
const midilong c_transpose      = 0x24240014; /**< Track transpose value.     */
const midilong c_perf_bp_mes    = 0x24240015; /**< Perfedit beats/measure.    */
const midilong c_perf_bw        = 0x24240016; /**< Perfedit beat-width.       */
const midilong c_tempo_map      = 0x24240017; /**< Reserve seq32 tempo map.   */
const midilong c_reserved_1     = 0x24240018; /**< Reserved for expansion.    */
const midilong c_reserved_2     = 0x24240019; /**< Reserved for expansion.    */
const midilong c_tempo_track    = 0x2424001A; /**< Alternate tempo track no.  */
const midilong c_seq_color      = 0x2424001B; /**< Feature from Kepler34.     */
const midilong c_seq_edit_mode  = 0x2424001C; /**< Feature from Kepler34.     */
const midilong c_seq_loopcount  = 0x2424001D; /**< N-play loop, 0 = infinite. */
const midilong c_reserved_3     = 0x2424001E; /**< Reserved for expansion.    */
const midilong c_reserved_4     = 0x2424001F; /**< Reserved for expansion.    */
const midilong c_trig_transpose = 0x24240020; /**< Triggers with transpose.   */

/**
 *    This class is the abstract base class for a container of MIDI track
 *    information.  It is the base class for midi_list and midi_vector.
 */

class midi_vector_base
{
    //  friend class midifile;

private:

    /**
     *  Provide a hook into a sequence so that we can exchange data with a
     *  sequence object.
     */

    sequence & m_sequence;

    /**
     *  Provides the position in the container when making a series of get()
     *  calls on the container.
     */

    mutable unsigned m_position_for_get;

public:

    midi_vector_base (sequence & seq);

    /**
     *  A rote constructor needed for a base class.
     */

    virtual ~midi_vector_base ()
    {
        // empty body
    }

    void fill (int tracknumber, const performer & p, bool doseqspec = true);

    /**
     *  Returns the size of the container, in midibytes.  Must be overridden
     *  in the derived class.
     */

    virtual unsigned size () const = 0;

    /**
     *  Instead of checking for the size of the container when "emptying" it
     *  [see the midifile::write() function], use this function, which is
     *  overridden to match the type of container being used.
     */

    virtual bool done () const
    {
        return true;
    }

    /**
     *  Provides a way to add a MIDI byte into the container.  The original
     *  seq66 container used an std::list and a push_front operation.
     */

    virtual void put (midibyte b) = 0;

    /**
     *  Combines a number of put() calls.  It puts the preamble for a MIDI Meta
     *  event.  After this function is called, the call then puts() the actual
     *  data.  Note that the data-length is assumed to fit into a midibyte (255
     *  maximum).
     */

    void put_meta (midibyte metavalue, int datalen, midipulse deltatime = 0);
    void put_seqspec (midilong spec, int datalen);

    /**
     *  Provide a way to get the next byte from the container.  It also
     *  increments m_position_for_get.
     */

    virtual midibyte get () const = 0;

    /**
     *  Provides a way to clear the container.
     */

    virtual void clear () = 0;

protected:

    sequence & seq ()
    {
        return m_sequence;
    }

    /**
     *  Sets the position to 0 and then returns that value. So far, it is not
     *  used, because we create a new midi_vector for each write_track()
     *  call.
     */

    unsigned position_reset () const
    {
        m_position_for_get = 0;
        return m_position_for_get;
    }

    /**
     * \getter m_position_for_get
     *      Returns the current position.
     */

    unsigned position () const
    {
        return m_position_for_get;
    }

    /**
     * \getter m_position_for_get
     *      Increments the current position.
     */

    void position_increment () const
    {
        ++m_position_for_get;
    }

private:

    void add_byte (midibyte b)
    {
        put(b);
    }

    void add_varinum (midilong v);
    void add_long (midilong x);
    void add_short (midishort x);
    void add_event (const event & e, midipulse deltatime);
    void add_ex_event (const event & e, midipulse deltatime);
    void fill_meta_track_end (midipulse deltatime);
    void fill_proprietary ();

protected:

    void fill_seq_number (int seq);
    void fill_seq_name (const std::string & name);

#if defined SEQ_USE_FILL_TIME_SIG_AND_TEMPO
    void fill_time_sig_and_tempo
    (
        const performer & p,
        bool has_time_sig = false,
        bool has_tempo    = false
    );
    void fill_time_sig (const performer & p);
    void fill_tempo (const performer & p);
#endif

    midipulse song_fill_seq_event
    (
        const trigger & trig, midipulse prev_timestamp
    );
    void song_fill_seq_trigger
    (
        const trigger & trig, midipulse len, midipulse prev_timestamp
    );

};          // class midi_vector_base

}           // namespace seq66

#endif      // SEQ66_MIDI_VECTOR_BASE_HPP

/*
 * midi_vector_base.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


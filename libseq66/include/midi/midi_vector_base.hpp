#if ! defined SEQ66_MIDI_VECTOR_BASE_HPP
#define SEQ66_MIDI_VECTOR_BASE_HPP

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
 * \file          midi_vector_base.hpp
 *
 *  This module declares the abstract base class for the management of some
 *  MIDI events, using the sequence class.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-10-10
 * \updates       2021-03-30
 * \license       GNU GPLv2 or above
 *
 *  This class is meant to hold the bytes that represent MIDI events and other
 *  MIDI data, which can then be dumped to a MIDI file.
 */

#include <cstddef>                      /* std::size_t type             */
#include <string>                       /* std::string class            */

#include "app_limits.h"                 /* SEQ66_NULL_SEQUENCE          */
#include "midi/midibytes.hpp"           /* seq66::midibyte              */

/**
 *  The number of MIDI notes in what?  This value is used in the sequence
 *  module.  It looks like it is the maximum number of notes that
 *  seq24/seq66 can have playing at one time.  In other words,
 *  "only" 256 simultaneously-playing notes can be managed.
 *  Internal global.  Will hide later.
 */

const int c_midi_notes = SEQ66_PLAYING_NOTES_MAX;

/**
 *  This macro is used for detecting SeqSpec data that Seq66 does not
 *  handle.  If this word is found, then we simply extract the expected number
 *  of characters specified by that construct, and skip them when parsing a
 *  MIDI file.
 */

#define SEQ66_PROPTAG_HIGHWORD          0x24240000

/**
 *  An easier, shorter test for the SEQ66_PROPTAG_HIGHWORD part of a long
 *  value, that clients can use.
 */

#define SEQ66_IS_PROPTAG(p) \
    (((p) & SEQ66_PROPTAG_HIGHWORD) == SEQ66_PROPTAG_HIGHWORD)

/**
 *  Defines the size of the time-signature and tempo information.  The sizes of
 *  these meta events consists of the delta time of 0 (1 byte), the event and
 *  size bytes (3 bytes), and the data (4 bytes for time-signature and 3 bytes
 *  for the tempo.  So, 8 bytes for the time-signature and 7 bytes for the
 *  tempo.
 */

#define SEQ66_TIME_TEMPO_SIZE           15

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
 *  file.  Some of the information is stored with each track (and in the
 *  midi_vector_base-derived classes), and some is stored in the proprietary
 *  header.
 *
 *  Track (sequencer-specific) data:
 *
\verbatim
            c_midibus
            c_midich
            c_timesig
            c_triggers (deprecated)
            c_triggers_new
            c_trig_transpose (c_triggers_new plus!)
            c_musickey (can be in footer, as well)
            c_musicscale (ditto)
            c_backsequence (ditto)
            c_transpose
            c_seq_color (performance colors for a sequence)
            c_seq_edit_mode
            c_seq_loopcount
            c_trig_transpose
\endverbatim
 *
 *  Note that c_seq_color in Seq66 is stored per sequence, and only if
 *  not PaletteColor::NONE.  In Kepler34, all 1024 sequence colors are stored
 *  in a "proprietrary", whole-song section, whether used or not.  There are
 *  also numeric differences in many of these "c_feature" constants.
 *
 * Footer ("proprietary", whole-song) data:
 *
\verbatim
            c_midictrl
            c_midiclocks
            c_notes
            c_bpmtag (beats per minute)
            c_mutegroups
            c_perf_bp_mes (perfedit's beats-per-measure setting)
            c_perf_bw     (perfedit's beat-width setting)
            c_tempo_map   (seq32's tempo map)
            c_reserved_1 and c_reserved_2
            c_tempo_track (holds the song's particular tempo track)
            c_seq_edit_mode (a potential future feature from Kepler34).
\endverbatim
 *
 *  Also see the PDF file in the following project for more information about
 *  the "proprietary" data:
 *
 *      https://github.com/ahlstromcj/seq66-doc.git
 *
 *  Note that the track data is read from the MIDI file, but not written
 *  directly to the MIDI file.  Instead, it is stored in the MIDI container as
 *  sequences are edited to used these "sequencer-specific" features.
 *  Also note that c_triggers has been replaced by c_triggers_new as the code
 *  that marks the triggers stored with a sequence. And c_trig_transpose
 *  extends it even further with a byte-value for tranposing a trigger.
 *
 *  As an extension, we can also grab the key, scale, and background sequence
 *  value selected in a sequence and write these values as track data, where
 *  they can be read in and applied to a specific sequence, when the seqedit
 *  object is created.  These values would not be stored in the legacy format.
 *
 *  Something like this could be done in the "user" configuration file, but
 *  then the key and scale would apply to all songs.  We don't want that.
 *
 *  We could also add snap and note-length to the per-song defaults, but
 *  the "user" configuration file seems like a better place to store these
 *  preferences.
 *
 * \note
 *      -   The value c_transpose value is from Stazed's seq32 project.
 *          The code to support this option is turned on permanently.
 *          There are additional values from Stazed/seq32, not yet used.
 *      -   The values below are compatible with Seq32, but they are not
 *          compatible with Kepler34.  It uses 0x24240011 and 0x24240012 for
 *          difference purposes.  See the asterisks below.
 *      -   Note the new "gap" values.  We just noticed this gap, which has
 *          existed between 0x24240009 and 0x24240010 since Seq24!
 */

const midilong c_midibus        = 0x24240001; /**< Track buss number.         */
const midilong c_midich         = 0x24240002; /**< Track channel number.      */
const midilong c_midiclocks     = 0x24240003; /**< Track clocking.            */
const midilong c_triggers       = 0x24240004; /**< See c_triggers_new.        */
const midilong c_notes          = 0x24240005; /**< Song data.                 */
const midilong c_timesig        = 0x24240006; /**< Track time signature.      */
const midilong c_bpmtag         = 0x24240007; /**< Song beats/minute.         */
const midilong c_triggers_new   = 0x24240008; /**< Trigger data w/offset.     */
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
const midilong c_seq_loopcount  = 0x2424001D; /**< Future: N-play pattern.    */
const midilong c_reserved_3     = 0x2424001E; /**< Reserved for expansion.    */
const midilong c_reserved_4     = 0x2424001F; /**< Reserved for expansion.    */
const midilong c_trig_transpose = 0x24240020; /**< Triggers with transpose.   */

/**
 *    This class is the abstract base class for a container of MIDI track
 *    information.  It is the base class for midi_list and midi_vector.
 */

class midi_vector_base
{
    friend class midifile;

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

    void add_variable (midipulse v);
    void add_long (midipulse x);
    void add_short (midishort x);
    void add_event (const event & e, midipulse deltatime);
    void add_ex_event (const event & e, midipulse deltatime);
    void fill_seq_number (int seq);
    void fill_seq_name (const std::string & name);
    void fill_meta_track_end (midipulse deltatime);
    void fill_proprietary ();

#if defined USE_FILL_TIME_SIG_AND_TEMPO
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


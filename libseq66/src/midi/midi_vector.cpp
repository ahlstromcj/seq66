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
 * \file          midi_vector.cpp
 *
 *  This module declares/defines the concrete class for a container of MIDI
 *  data.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-10-11
 * \updates       2021-10-04
 * \license       GNU GPLv2 or above
 *
 */

#include "midi/midi_vector.hpp"         /* seq66::midi_vector_base class    */
#include "play/sequence.hpp"            /* seq66::sequence class            */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  This constructor fills in the members of this class.
 *
 * \param seq
 *      Provides a reference to the sequence/track for which this container
 *      holds MIDI data.
 */

midi_vector::midi_vector (sequence & seq) :
    midi_vector_base    (seq),
    m_char_vector       ()
{
    // Empty body
}

/**
 *  Fills this list with an exportable track.  Following stazed, we're
 *  consolidate the tracks at the beginning of the song, replacing the actual
 *  track number with a counter that is incremented only if the track was
 *  exportable.  Note that this loop is kind of an elaboration of what goes on
 *  in the midi_vector_base :: fill() function for normal Seq66 file writing.
 *
 *  Exportability ensures that the sequence pointer is valid.  This function
 *  adds all triggered events.
 *
 *  For each trigger in the sequence, add events to the list below; fill
 *  one-by-one in order, creating a single long sequence.  Then set a single
 *  trigger for the big sequence: start at zero, end at last trigger end with
 *  snap.  We're going to reference (not copy) the triggers now, since the
 *  write_song() function is now locked.
 *
 *  The we adjust the sequence length to snap to the nearest measure past the
 *  end.  We fill the MIDI container with trigger "events", and then the
 *  container's bytes are written.
 *
 *  tick_end() isn't quite a trigger length, off by 1.  Subtracting
 *  tick_start() can really screw it up.
 */

bool
midi_vector::song_fill_track (int track, bool standalone)
{
    bool result = seq().is_exportable();
    if (result)
    {
        clear();
        if (standalone)
        {
            fill_seq_number(track);
            fill_seq_name(seq().name());

#if defined SEQ66_USE_FILL_TIME_SIG_AND_TEMPO       // undefined
            if (track == rc().tempo_track_number)
            {
                seq().events().scan_meta_events();
                fill_time_sig_and_tempo
                (
                    p, seq().events().has_time_signature(),
                    seq().events().has_tempo()
                );
            }
#endif
        }

        midipulse last_ts = 0;
        const auto & trigs = seq().get_triggers();
        for (auto & t : trigs)
            last_ts = song_fill_seq_event(t, last_ts);

        const trigger & ender = trigs.back();
        midipulse seqend = ender.tick_end();
        midipulse measticks = seq().measures_to_ticks();
        if (measticks > 0)
        {
            midipulse remainder = seqend % measticks;
            if (remainder != (measticks - 1))
                seqend += measticks - remainder - 1;
        }
        song_fill_seq_trigger(ender, seqend, last_ts);
    }
    return result;
}

}           // namespace seq66

/*
 * midi_vector.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


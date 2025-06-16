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
 * \file          midi_splitter.cpp
 *
 *  This module declares/defines the class for splitting a MIDI track based on
 *  channel number.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-11-24
 * \updates       2022-10-26
 * \license       GNU GPLv2 or above
 *
 *  We have recently updated this module to put Set Tempo events into the
 *  first track (channel 0).
 */

#include "cfg/settings.hpp"             /* seq66::usr().seqs_in_set()       */
#include "midi/eventlist.hpp"           /* seq66::eventlist                 */
#include "midi/midi_splitter.hpp"       /* seq66::midi_splitter             */
#include "play/performer.hpp"           /* seq66::performer                 */
#include "play/sequence.hpp"            /* seq66::sequence                  */
#include "util/palette.hpp"             /* seq66::palette_to_int(), colors  */

namespace seq66
{

/**
 *  Principal constructor.
 *
 */

midi_splitter::midi_splitter () :
    m_smf0_channels_count   (0),
    m_smf0_channels         (),         /* array, initialized in parse()    */
    m_smf0_main_sequence    (nullptr),
    m_smf0_seq_number       (-1)
{
    initialize();
}

/**
 *  Resets the SMF 0 support variables in preparation for parsing a new MIDI
 *  file.
 */

void
midi_splitter::initialize ()
{
    m_smf0_channels_count = 0;
    for (int i = 0; i < c_midichannel_max; ++i)
        m_smf0_channels[i] = false;
}

/**
 *  Processes a channel number by raising its flag in the m_smf0_channels[]
 *  array.  If it is the first entry for that channel, m_smf0_channels_count
 *  is incremented.  We won't check the channel number, to save time,
 *  until someday we segfault :-D
 *
 * \param channel
 *      The MIDI channel number.  The caller is responsible to make sure it
 *      ranges from 0 to 15.
 */

void
midi_splitter::increment (int channel)
{
    if (! m_smf0_channels[channel])  /* channel not yet logged?      */
    {
        m_smf0_channels[channel] = true;
        ++m_smf0_channels_count;
    }
}

/**
 *  Logs the main sequence (an SMF 0 track) for later usage in splitting the
 *  track.
 *
 * /param seq
 *      The main sequence to be logged.
 *
 * /param seqnum
 *      The sequence number of the main sequence.
 *
 * /return
 *      Returns true if the main sequence's address was logged, and false if
 *      it was already logged.
 */

bool
midi_splitter::log_main_sequence (sequence & seq, int seqnum)
{
    bool result;
    if (is_nullptr(m_smf0_main_sequence))
    {
        seq.sort_events();                          /* really necessary?    */
        seq.set_color(palette_to_int(PaletteColor::cyan));
        m_smf0_main_sequence = &seq;
        m_smf0_seq_number = seqnum;
        infoprint("SMF 0 main sequence logged");
        result = true;
    }
    else
    {
        errprint("SMF 0 main sequence already logged");
        result = false;
    }
    return result;
}

/**
 *  This function splits an SMF 0 file, splitting all of the channels in the
 *  sequence out into separate sequences, and adding each to the performer
 *  object.  Lastly, it adds the SMF 0 track as the last track; the user can
 *  then examine it before removing it.  Is this worth the effort?
 *
 *  There is a little oddity, in that, if the SMF 0 track has events for only
 *  one channel, this code will still create a new sequence, as well as the
 *  main sequence.  Not sure if this is worth extra code to just change the
 *  channels on the main sequence and put it into the correct track for the
 *  one channel it contains.  In fact, we just want to keep it in pattern slot
 *  number 16, to keep it out of the way.
 *
 * \param p
 *      Provides a reference to the performer object into which sequences/tracks
 *      are to be added.
 *
 * \param screenset
 *      The screen-set offset to be used when loading a sequence (track) from
 *      the file.
 *
 * \param ppqn
 *      Use the provided PPQN.
 *
 * \return
 *      Returns true if the parsing succeeded.  Returns false if no SMF 0 main
 *      sequence was logged.
 */

bool
midi_splitter::split (performer & p, int screenset, int ppqn)
{
    bool result = not_nullptr(m_smf0_main_sequence);
    if (result)
    {
        if (m_smf0_channels_count > 0)
        {
            int seqnum = screenset * usr().seqs_in_set();
            for (int chan = 0; chan < c_midichannel_max; ++chan, ++seqnum)
            {
                if (m_smf0_channels[chan])
                {
                    /*
                     * The master MIDI buss must be set before the split,
                     * otherwise the null pointer causes a segfault.
                     */

                    sequence * s = new sequence(ppqn);
                    if (split_channel(p, *m_smf0_main_sequence, s, chan))
                        p.install_sequence(s, seqnum);
                    else
                        delete s;   /* empty sequence, not even meta events */
                }
            }
            m_smf0_main_sequence->set_midi_channel(null_channel());
            p.install_sequence(m_smf0_main_sequence, seqnum);
        }
    }
    return result;
}

/**
 *  This function splits the given sequence into a new sequence, for the given
 *  channel found in the SMF 0 track.  It is called for each possible channel,
 *  resulting in multiple passes over the SMF 0 track.
 *
 *  Note that the events that are read from the MIDI file have delta times.
 *  Seq66 converts these delta times to cumulative times.    We
 *  need to preserve that here.  Conversion back to delta times is needed only
 *  when saving the sequences to a file.  This is done in
 *  midi_vector_base::fill().
 *
 *  We have to accumulate the delta times in order to be able to set the
 *  length of the sequence in pulses.
 *
 *  Luckily, we don't have to worry about copying triggers, since the imported
 *  SMF 0 track won't have any Seq24/Sequencer24 triggers.
 *
 *  It doesn't set the sequence number of the sequence; that is set when the
 *  sequence is added to the performer object.
 *
 * \param main_seq
 *      This parameter is the whole SMF 0 track that was read from the MIDI
 *      file.  It contains all of the channel data that needs to be split into
 *      separate sequences.
 *
 * \param s
 *      Provides the new sequence that needs to have its settings made, and
 *      all of the selected channel events added to it.
 *
 * \param channel
 *      Provides the MIDI channel number (re 0) that marks the channel data
 *      the needs to be extracted and added to the new sequence.  If this
 *      channel is 0, then we need to add certain Meta events to this
 *      sequence, as well.  So far we support only Tempo Meta events.
 *
 * \return
 *      Returns true if at least one event got added.   If none were added,
 *      the caller should delete the sequence object represented by parameter
 *      \a s.
 */

bool
midi_splitter::split_channel
(
    const performer & p,
    const sequence & main_seq,
    sequence * s,
    int channel
)
{
    bool result = false;
    char tmp[32];
    if (main_seq.name().empty())
    {
        snprintf(tmp, sizeof tmp, "Track %d", channel+1);
    }
    else
    {
        snprintf
        (
            tmp, sizeof tmp, "%d: %.13s", channel+1, main_seq.name().c_str()
        );
    }

    /*
     * It is necessary to (redundantly, it turns out) set the master MIDI buss
     * first, before setting the other sequence parameters.
     */

    s->set_master_midi_bus(p.master_bus());
    s->set_name(std::string(tmp));
    s->set_midi_channel(channel);
    s->set_midi_bus(main_seq.seq_midi_bus());
    s->zero_markers();

    midipulse length_in_ticks = 0;      /* an accumulator of delta times    */
    const eventlist & evl = main_seq.events();
    for (auto i = evl.cbegin(); i != evl.cend(); ++i)
    {
        const event & er = eventlist::cdref(i);
        if (er.is_ex_data())
        {
            if (channel == 0 || er.is_sysex())
            {
                length_in_ticks = er.timestamp();
                if (s->append_event(er))    /* adds event, no sorting       */
                    result = true;          /* the event got added          */
            }
        }
        else if (er.match_channel(channel))
        {
            length_in_ticks = er.timestamp();
            if (s->append_event(er))        /* adds event, no sorting       */
                result = true;              /* the event got added          */
        }
    }

    /*
     * No triggers to add.  Whew!  And setting the length is now a no-brainer,
     * since the tick value is that of the last logged event in the sequence.
     * Also, sort all the events after they have been appended.
     */

    s->set_length(length_in_ticks);
    s->sort_events();
    return result;
}

}           // namespace seq66

/*
 * midi_splitter.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


#if ! defined SEQ66_MIDI_SPLITTER_HPP
#define SEQ66_MIDI_SPLITTER_HPP

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
 * \file          midi_splitter.hpp
 *
 *  This module declares/defines the base class for MIDI files.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-11-24
 * \updates       2021-06-10
 * \license       GNU GPLv2 or above
 *
 *  Seq66 can also split an SMF 0 file into multiple tracks, effectively
 *  converting it to SMF 1.  This class holds all the information needed to do
 *  that.
 */

#include <string>

namespace seq66
{
    class performer;
    class sequence;

/**
 *  This class handles the parsing and writing of SMF 0 files.
 */

class midi_splitter
{

private:

    /**
     *  Provides support for SMF 0, indicates how many channels were found in
     *  the file in a single sequence.  SMF 1 file parsing will only warn
     *  about more than one channel found in a given sequence.
     */

    int m_smf0_channels_count;

    /**
     *  Provides support for SMF 0, holds a bool value that indicates the
     *  occurrence of a given channel.  We don't have to worry about multiple
     *  MIDI busses here, we hope.
     */

    bool m_smf0_channels[16];

    /**
     *  Provides support for SMF 0, points to the initial SMF 0 sequence, from
     *  which the single-channel sequences will be created.
     */

    sequence * m_smf0_main_sequence;

    /**
     *  Provides support for SMF 0, holds the prospective sequence number of
     *  the main (SMF 0) sequence.  We want to be able to add that sequence
     *  last, for easier and cleaner removal of that sequence by the user.
     */

    int m_smf0_seq_number;

public:

    midi_splitter ();
    midi_splitter (const midi_splitter &) = delete;
    midi_splitter & operator = (const midi_splitter &) = delete;
    ~midi_splitter () = default;

    bool log_main_sequence (sequence & seq, int seqnum);
    void initialize ();
    void increment (int channel);
    bool split (performer & p, int screenset, int ppqn);

    /**
     * \getter m_smf0_channels_count
     */

    int count () const
    {
        return m_smf0_channels_count;
    }

private:

    bool split_channel
    (
        const performer & p,
        const sequence & main_seq,
        sequence * seq,
        int channel
    );

};          // class midi_splitter

}           // namespace seq66

#endif      // SEQ66_MIDI_SPLITTER_HPP

/*
 * midi_splitter.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


#if ! defined SEQ66_SONGSUMMARY_HPP
#define SEQ66_SONGSUMMARY_HPP

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
 * \file          songsummary.hpp
 *
 *  This module declares/defines the base class for MIDI files.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2021-01-22
 * \updates       2021-01-24
 * \license       GNU GPLv2 or above
 *
 */

#include <string>

#include "play/seq.hpp"                 /* seq66::seq pattern class         */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class performer;

/**
 *  This class handles dumping a summary of a MIDI file.
 */

class songsummary
{

private:

    /**
     *  The unchanging name of the MIDI file.
     */

    const std::string m_name;

public:

    songsummary (const std::string & name);
    ~songsummary ();
    bool write (performer & p, bool doseqspec = true);

    bool write_song (performer & p);

    const std::string & name () const
    {
        return m_name;
    }

protected:

    bool write_sequence (std::ofstream & file, seq::pointer s);
    bool write_header (std::ofstream & file, const performer & p);
    void write_mute_groups (std::ofstream & file, const performer & p);
    void write_prop_header
    (
        std::ofstream & file,
        midilong control_tag,
        int value
    );
    void write_set_names
    (
        std::ofstream & file,
        const performer & p
    );
    bool write_proprietary_track (std::ofstream & file, performer & p);
    void write_bpm
    (
        std::ofstream & file,
        const performer & p
    );
    void write_mutes
    (
        std::ofstream & file,
        const performer & p
    );

    void write_global_bg (std::ofstream & file);
    void write_beat_info
    (
        std::ofstream & file,
        const performer & p
    );

};          // class songsummary

/*
 *  Free functions related to songsummary.
 */

extern bool write_song_summary (performer & p, const std::string & fn);

}           // namespace seq66

#endif      // SEQ66_SONGSUMMARY_HPP

/*
 * songsummary.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


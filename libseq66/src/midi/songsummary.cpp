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
 * \file          songsummary.cpp
 *
 *  This module declares/defines a class for writing summary information about a
 *  MIDI file.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2021-01-22
 * \updates       2021-01-22
 * \license       GNU GPLv2 or above
 *
 */

#include <fstream>                      /* std::ifstream and std::ofstream  */
#include <iomanip>                      /* std::hex etc.                    */

#include "cfg/settings.hpp"             /* seq66::rc() and choose_ppqn()    */
#include "midi/midi_vector_base.hpp"    /* seq66::c_notes, other tags       */
#include "midi/songsummary.hpp"         /* seq66::songsummary               */
#include "play/performer.hpp"           /* must precede songsummary.hpp !   */
#include "play/sequence.hpp"            /* seq66::sequence                  */
#include "util/strfunctions.hpp"        /* seq66::bool_to_string()          */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Principal constructor.
 *
 * \param name
 *      Provides the name of the MIDI file to be read or written.
 *
 * \param ppqn
 *      Provides the initial value of the PPQN setting.  It is handled
 *      differently for parsing (reading) versus writing the MIDI file.
 *      WARNING: It is the responsibility of the caller to make sure the PPQN
 *      value is valid, usually by passing in the result of the choose_ppqn()
 *      function.
 *      -   Reading.  The caller of read_midi_file(), as well as the function
 *          itself, determine the value of ppqn used here.  It is either 0 or
 *          the result of seq66::choose_ppqn().
 *          -   If set to SEQ66_USE_FILE_PPQN (0), then m_ppqn is set to the
 *              value read from the MIDI file.  No PPQN scaling is done.
 *          -   Otherwise, the ppqn value is used as is.  If the file uses a
 *              different PPQN than 192, PPQN rescaling is done to make it
 *              192.  The PPQN value read from the MIDI file is used to scale
 *              the running-time of the sequence relative to
 *              SEQ66_DEFAULT_PPQN.
 *      -   Writing.  This value is written to the MIDI file in the header
 *          chunk of the song.  Note that the caller must query for the
 *          PPQN set during parsing, and pass it to the constructor when
 *          preparing to write the file.  See how it is done in the qsmainwnd
 *          class.
 *
 * \param globalbgs
 *      If true, write any non-default values of the key, scale, and
 *      background sequence to the global "proprietary" section of the MIDI
 *      file, instead of to each sequence.  Note that this option is only used
 *      in writing; reading can handle either format transparently.
 *
 * \param verifymode
 *      If set to true, we are opening files just to verify them before
 *      accepting the usage of a playlist.  In this case, we clear out each
 *      song after it is read in for verification.  It defaults to false.
 *      Actually, the playlist::verify() function clears the song, but this
 *      variable is still useful to cut down on output during verfication.
 *      See grab_input_stream().
 */

songsummary::songsummary (const std::string & name) :
    m_name (name)
{
    // no other code needed
}

/**
 *  A rote destructor.
 */

songsummary::~songsummary ()
{
    // empty body
}

/**
 *  Write the whole MIDI data and Seq24 information out to a text file.
 *
 * \param p
 *      Provides the object that will contain and manage the entire
 *      performance.
 *
 * \param doseqspec
 *      If true (the default, then the Seq66-specific SeqSpec sections
 *      are written to the file.  If false, we want to export the tracks as a
 *      basic MIDI sequence (which is not the same as exporting a Song, with
 *      triggers, as a MIDI sequence).
 *
 * \return
 *      Returns true if the write operations succeeded.
 */

bool
songsummary::write (performer & p, bool doseqspec)
{
    std::ofstream file(name(), std::ios::out | std::ios::trunc);
    bool result = file.is_open();
    if (result)
    {
        int numtracks = 0;
        for (int i = 0; i < p.sequence_high(); ++i)
        {
            if (p.is_seq_active(i))
                ++numtracks;             /* count number of active tracks   */
        }
        result = numtracks > 0;
        if (result)
            result = write_header(file, numtracks);
    }
    if (result)
    {
        for (int track = 0; track < p.sequence_high(); ++track)
        {
            if (p.is_seq_active(track))
            {
                seq::pointer s = p.get_sequence(track);
                if (s)
                {
                    result =write_sequence(file, s);
                    if (! result)
                        break;
                }
            }
        }
    }
    if (result && doseqspec)
    {
        result = write_proprietary_track(file, p);
        if (! result)
        {
            file_error("SeqSpec write failed", name());
        }
    }
    return result;
}

/**
 *
 */

bool
songsummary::write_sequence (std::ofstream & file, seq::pointer s)
{
    file
        << "Sequence #" << s->seq_number() << " '" << s->name() << "'\n"
        << "      Beats/Bar: " << s->get_beats_per_bar() << "\n"
        << "     Beat Width: " << s->get_beat_width() << "\n"
        << "   Nominal buss: " << int(s->get_midi_bus()) << "\n"
        << "      True buss: " << int(s->true_bus()) << "\n"
        << " Length (ticks): " << long(s->get_length()) << "\n"
        << "  No. of events: " << s->event_count() << "\n"
        << "No. of triggers: " << s->trigger_count() << "\n"
        << "        Channel: " << s->get_midi_channel() << "\n"
        << "   Transposable: " << bool_to_string(s->transposable()) << "\n"
        << "            Key: " << int(s->musical_key()) << "\n"
        << "          Scale: " << int(s->musical_scale()) << "\n"
        << "          Color: " << s->color() << "\n"
        ;
    return true;
}

/**
 *  For each groups in the mute-groups,
 *
 *  mutegroups has rows and columns for each group, but doesn't have a way to
 *  iterate through all the groups.
 */

void
songsummary::write_mute_groups
(
    std::ofstream & /*file*/, const performer & /*p*/
)
{
    /*
    const mutegroups & mutes = p.mutes();
    for (const auto & stz : mutes.list())
    {
        int groupnumber = stz.first;
        const mutegroup & m = stz.second;
        midibooleans mutebits = m.get();
        result = mutebits.size() > 0;
        write_long(groupnumber);
        for (auto mutestatus : mutebits)
            write_long(bool(mutestatus) ? 1 : 0);
    }
    */
}

bool
songsummary::write_header (std::ofstream & file, int numtracks)
{
    file
        << "File name:      " << name() << "\n"
        << "No. of tracks:  " << numtracks << "\n"
        << "MIDI format:    " << 1 << "\n"
//      << "PPQN:           " << m_ppqn << "\n"
        ;

    return numtracks > 0;
}

#if defined USE_WRITE_START_TEMPO

/**
 *  Writes the initial or only tempo, occurring at the beginning of a MIDI
 *  song.  Compare this function to midi_vector_base::fill_time_sig_and_tempo().
 *
 * \param start_tempo
 *      The beginning tempo value.
 */

void
songsummary::write_start_tempo (std::ofstream & file, midibpm start_tempo)
{
    file << "Initial tempo:  " << start_tempo << "\n" ;
}

#endif  // USE_WRITE_START_TEMPO

#if defined USE_WRITE_TIME_SIG

/**
 *  Note that the cc value (MIDI ticks per metronome click) is hardwired to
 *  0x18 (24) and the bb value (32nd notes per quarter note) is hardwired
 *  to 0x08 (8).
 *
 * \param beatsperbar
 *      The numerator of the time signature.
 *
 * \param beatwidth
 *      The denominator of the time signature.
 */

void
songsummary::write_time_sig (std::ofstream & file, int beatsperbar, int beatwidth)
{
    file
        << "Time signature: " << beatsperbar << "/" << beatwidth << "\n"
        << "Clocks/metro:   " << 24 << "\n"
        << "32nds/beats:    " << 8 << "\n"
        ;
}

#endif  // USE_WRITE_TIME_SIG

/**
 *  Writes a "proprietary" (SeqSpec) Seq24 header item.
 *
 *  The new format writes 0x00 0xFF 0x7F len 0x242400xx; the first 0x00 is the
 *  delta time.
 *
 *  In the new format, the 0x24 is a kind of "manufacturer ID".  At
 *  http://www.midi.org/techspecs/manid.php we see that most manufacturer IDs
 *  start with 0x00, and are thus three bytes long, or start with codes at
 *  0x40 and above.  Similary, this site shows that no manufacturer uses 0x24:
 *
 *      http://sequence15.blogspot.com/2008/12/midi-manufacturer-ids.html
 *
 * \warning
 *      Currently, the manufacturer ID is not handled; it is part of the
 *      data, which can be misleading in programs that analyze MIDI files.
 *
 * \param control_tag
 *      Determines the type of sequencer-specific section to be written.
 *      It should be one of the value in the globals module, such as
 *      c_midibus or c_mutegroups.
 */

void
songsummary::write_prop_header
(
    std::ofstream & file,
    midilong control_tag,
    int value
)
{
    file << "0xFF 0x7F " << std::hex << control_tag << " = " << value << "\n";
}

void
songsummary::write_notepads
(
    std::ofstream & file,
    const performer & p
)
{
    file << "Screen-set Notes:" << "\n";
    write_prop_header(file, c_notes, c_max_sets);
    for (int s = 0; s < c_max_sets; ++s)            /* see "cnotesz" calc       */
    {
        const std::string & note = p.get_screenset_notepad(s);
        file << "   Set #" << s << ": '" << note << "'\n";
    }
}

/*
 *  We now encode the Seq66-specific BPM value by multiplying it
 *  by 1000.0 first, to get more implicit precision in the number.
 *  We should probably sanity-check the BPM at some point.
 */

void
songsummary::write_bpm
(
    std::ofstream & file,
    const performer & p
)
{
    long scaled_bpm = long(p.get_beats_per_minute() * SEQ66_BPM_SCALE_FACTOR);
    write_prop_header(file, c_bpmtag, scaled_bpm);
    file
        << "Encoded BPM: " << scaled_bpm
        << " (" << p.get_beats_per_minute() << ")\n"
        ;
}

void
songsummary::write_mutes
(
    std::ofstream & file,
    const performer & p
)
{
    const mutegroups & mutes = p.mutes();
    unsigned groupcount = c_max_groups;         /* 32, the maximum          */
    unsigned setsize = p.seqs_in_set();
    if (mutes.any())
    {
        groupcount = unsigned(mutes.count());   /* no. of existing groups  */
        setsize = unsigned(mutes.group_size());
    }

    file << "Mute Groups: " << groupcount << " of size " << setsize << "\n";
    write_prop_header(file, c_mutegroups, c_max_groups);
    write_mute_groups(file, p);
}

void
songsummary::write_global_bg (std::ofstream & file)
{
    file << "Global key, scale, and background sequence:" << "\n";
    write_prop_header(file, c_musickey, usr().seqedit_key());
    write_prop_header(file, c_musicscale, usr().seqedit_scale());
    write_prop_header(file, c_backsequence, usr().seqedit_bgsequence());
}

void
songsummary::write_beat_info
(
    std::ofstream & file,
    const performer & p
)
{
    file << "Global beats, beat width, and tempo track:" << "\n";
    write_prop_header(file, c_perf_bp_mes, p.get_beats_per_bar());
    write_prop_header(file, c_perf_bw, p.get_beat_width());
    write_prop_header(file, c_tempo_track, p.tempo_track_number());
}

/**
 *  Writes out the final proprietary/SeqSpec section, using the new format.
 *
 * \param p
 *      Provides the object that will contain and manage the entire
 *      performance.
 *
 * \return
 *      Always returns true.  No efficient way to check all of the writes that
 *      can happen.  Might revisit this issue if some bug crops up.
 */

bool
songsummary::write_proprietary_track (std::ofstream & file, performer & p)
{
    file << "Start of SeqSpecs:" << "\n";
    write_prop_header(file, c_midictrl, 0);     /* midi control tag + 4     */
    write_prop_header(file, c_midiclocks, 0);   /* bus mute/unmute data + 4 */
    write_notepads(file, p);
    write_bpm(file, p);
    write_mutes(file, p);
    write_global_bg(file);
    write_beat_info(file, p);
    return true;
}

bool
write_song_summary (performer & p, const std::string & fname)
{
    songsummary f(fname);
    bool result = f.write(p);
    if (result)
    {
        file_message("Wrote MIDI file", fname);
    }
    else
    {
        file_error("Write failed", fname);
    }
    return result;
}

}           // namespace seq66

/*
 * songsummary.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


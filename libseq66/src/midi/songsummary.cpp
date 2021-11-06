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
 * \file          songsummary.cpp
 *
 *  This module declares/defines a class for writing summary information about
 *  a MIDI file.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2021-01-22
 * \updates       2021-11-06
 * \license       GNU GPLv2 or above
 *
 */

#include <fstream>                      /* std::ifstream and std::ofstream  */
#include <iomanip>                      /* std::hex etc.                    */
#include <map>                          /* std::map<> template              */

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

std::map<midilong, std::string>
s_tag_names_container
{
    { c_midibus      ,  "Track buss number" },
    { c_midichannel  ,  "Track channel number" },
    { c_midiclocks   ,  "Track clocking" },
    { c_triggers     ,  "Old triggers" },
    { c_notes        ,  "Set notes" },
    { c_timesig      ,  "Track time signature" },
    { c_bpmtag       ,  "Main beats/minute" },
    { c_triggers_ex  ,  "Track trigger data" },
    { c_mutegroups   ,  "Song mute group data" },
    { c_gap_A        ,  "Gap A" },
    { c_gap_B        ,  "Gap B" },
    { c_gap_C        ,  "Gap C" },
    { c_gap_D        ,  "Gap D" },
    { c_gap_E        ,  "Gap E" },
    { c_gap_F        ,  "Gap F" },
    { c_midictrl     ,  "MIDI control" },
    { c_musickey     ,  "Track key" },
    { c_musicscale   ,  "Track scale" },
    { c_backsequence ,  "Track background sequence" },
    { c_transpose    ,  "Track transposability" },
    { c_perf_bp_mes  ,  "Perfedit beats/measure" },
    { c_perf_bw      ,  "Perfedit beat-width" },
    { c_tempo_map    ,  "Reserve seq32 tempo map" },
    { c_reserved_1   ,  "Reserved 1" },
    { c_reserved_2   ,  "Reserved 2" },
    { c_tempo_track  ,  "Alternate tempo track number" },
    { c_seq_color    ,  "Color" },
    { c_seq_edit_mode,  "Normal/drum edit mode" },
    { c_seq_loopcount,  "Future: N-play pattern" },
    { c_reserved_3,     "Reserved 3" },
    { c_reserved_4,     "Reserved 4" },
    { c_trig_transpose, "Transposable trigger" }
};

/**
 *  Principal constructor.
 *
 * \param name
 *      Provides the name of the MIDI file to be read or written.
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
        result = write_header(file, p);
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
                    result = write_sequence(file, s);
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

bool
songsummary::write_sequence (std::ofstream & file, seq::pointer s)
{
    int triggercount = s->trigger_count();
    file
        << "Sequence #" << s->seq_number() << " '" << s->name() << "'\n"
        << "        Channel: " << int(s->seq_midi_channel()) << "\n"
        << "          Beats: " << s->get_beats_per_bar() << "/"
                               << s->get_beat_width() << "\n"
        << "         Busses: " << int(s->seq_midi_bus()) << "-->"
                               << int(s->true_bus()) << "\n"
        << " Length (ticks): " << long(s->get_length()) << "\n"
        << "Events;triggers: " << s->event_count() << "; "
                               << triggercount << "\n"
        << "   Transposable: " << bool_to_string(s->transposable()) << "\n"
        << "  Key and scale: " << int(s->musical_key()) << "; "
                               << int(s->musical_scale()) << "\n"
        ;
    if (s->color() >= 0)
    {
#if defined SEQ66_COLORS_NOT_REQUIRING_A_GUI
        PaletteColor pc = PaletteColor(s->color());
        std::string colorname = get_color_name(pc);
        file << "          Color: " << s->color() << " " << colorname << "\n";
#else
        file << "          Color: " << s->color() << "\n";
#endif
    }

    /*
     * The format of c_triggers_ex:  0x24240008, followed by a length value
     * of 4 + triggercount * 12.  Each trigger has three 4-byte values:
     * trigger-on, trigger-off, and trigger-offset.  The c_trig_transpose
     * (0x24240020) tag adds a byte value for trigger transposition.
     */

    if (triggercount > 0)
    {
        file << s->trigger_listing() << std::endl;
    }
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
    std::ofstream & file, const performer & p
)
{
    const mutegroups & mutes = p.mutes();
    for (const auto & stz : mutes.list())
    {
        int groupnumber = stz.first;
        const mutegroup & m = stz.second;
        midibooleans mutebits = m.get();
        bool ok = mutebits.size() > 0;
        if (ok)
        {
            int count = 0;
            file << "Mute group #" << std::setw(2) << groupnumber << ": ";
            for (auto mutestatus : mutebits)
            {
                file << (bool(mutestatus) ? "1" : "0");
                if (++count % 8 == 0)
                    file << " ";
            }
            file << " \"" << m.name() << "\"" << std::endl;
        }
        else
        {
            file << "Mute group #" << groupnumber << " empty" << std::endl;
        }
    }
}

bool
songsummary::write_header (std::ofstream & file, const performer & p)
{
    int numtracks = 0;
    for (int i = 0; i < p.sequence_high(); ++i)
    {
        if (p.is_seq_active(i))
            ++numtracks;             /* count number of active tracks   */
    }

    bool result = numtracks > 0;
    if (result)
    {
        file
            << "File name:      " << name() << "\n"
            << "No. of sets:    " << p.screenset_count() << "\n"
            << "No. of tracks:  " << numtracks << "\n"
            << "MIDI format:    " << 1 << "\n"
            << "PPQN:           " << p.ppqn() << "\n"
            ;
    }
    else
    {
        file
            << "File name:      " << name() << "\n"
            << "No. of tracks:  " << 0 << "! Aborting!\n"
            ;
    }
    return result;
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
 *
 * \param value
 *      This value appears in the output, and its meaning depends on the
 *      control tag.
 */

void
songsummary::write_prop_header
(
    std::ofstream & file,
    midilong control_tag,
    int value
)
{
    std::string ctagname = "Unknown";
    std::map<midilong, std::string>::const_iterator ci =
        s_tag_names_container.find(control_tag);

    if (ci != s_tag_names_container.end())
        ctagname = ci->second;

    file
        << "0xFF 0x7F " << std::hex << control_tag << std::dec
        << " (" << ctagname << ") = "
        << value << "\n"
        ;
}

void
songsummary::write_set_names
(
    std::ofstream & file,
    const performer & p
)
{
    int setcount = p.screenset_count();
    file << "Screen-set Notes:" << "\n";
    write_prop_header(file, c_notes, setcount);
    for (int s = 0; s < setcount; ++s)
    {
        const std::string & note = p.screenset_name(s);
        file << "   Set #" <<std::dec << s << ": '" << note << "'\n";
    }
}

void
songsummary::write_bpm
(
    std::ofstream & file,
    const performer & p
)
{
    midibpm bpm = p.get_beats_per_minute();
    write_prop_header(file, c_bpmtag, bpm);
    file << "        BPM: " << bpm << "\n";
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
    unsigned setsize = p.screenset_size();
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
    write_prop_header(file, c_tempo_track, rc().tempo_track_number());
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
    write_set_names(file, p);
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
        file_message("Wrote", fname);
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


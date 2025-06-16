#if ! defined SEQ66_WRKFILE_HPP
#define SEQ66_WRKFILE_HPP

/*
 *  WRK File component
 *  Copyright (C) 2010-2018, Pedro Lopez-Cabanillas <plcl@users.sf.net>
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  This library is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file          wrkfile.hpp
 *
 *  This module declares/defines the class for reading WRK files.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-06-04
 * \updates       2022-05-14
 * \license       GNU GPLv2 or above
 *
 *  For a quick guide to the WRK format, see, for example:
 *
 *      WRK Cakewalk WRK File Parser (Input).
 */

#include <list>                         /* std::list                        */

#include "midi/midifile.hpp"            /* seq66::midifile base class       */

namespace seq66
{
    class performer;
    class sequence;

/**
 * Cakewalk WRK file format (input only)
 *
 * This class is used to parse Cakewalk WRK Files since 0.3.0.
 */

class wrkfile : public midifile
{

private:

    struct RecTempo
    {
        long time;
        double tempo;
        double seconds;
    };

private:

    /**
     *  A nested class for holding all the data elements.
     */

    class wrkfile_private
    {
        friend class wrkfile;

    public:

        wrkfile_private ();

    private:

        midilong m_Now;          ///< Now marker time.
        midilong m_From;         ///< From marker time.
        midilong m_Thru;         ///< Thru marker time.
        midibyte m_KeySig;       ///< Key signature (0=C, 1=C#, ... 11=B).
        midibyte m_Clock;        ///< Clock Src (0=Int, 1=MIDI, 2=FSK, 3=SMPTE).
        midibyte m_AutoSave;     ///< Auto save (0=disabled, 1..256=minutes).
        midibyte m_PlayDelay;    ///< Play Delay.
        bool m_ZeroCtrls;        ///< Zero continuous controllers?
        bool m_SendSPP;          ///< Send Song Position Pointer?
        bool m_SendCont;         ///< Send MIDI Continue?
        bool m_PatchSearch;      ///< Patch/controller search-back?
        bool m_AutoStop;         ///< Auto-stop?
        midilong m_StopTime;     ///< Auto-stop time.
        bool m_AutoRewind;       ///< Auto-rewind?
        midilong m_RewindTime;   ///< Auto-rewind time.
        bool m_MetroPlay;        ///< Metronome on during playback?
        bool m_MetroRecord;      ///< Metronome on during recording?
        bool m_MetroAccent;      ///< Metronome accents primary beats?
        midibyte m_CountIn;      ///< Measures of count-in (0=no count-in).
        bool m_ThruOn;           ///< MIDI Thru enabled? Only if no THRU rec.
        bool m_AutoRestart;      ///< Auto-restart?
        midibyte m_CurTempoOfs;  ///< Which of 3 tempo offsets is used: 0..2.
        midibyte m_TempoOfs1;    ///< Fixed-point ratio value of offset 1.
        midibyte m_TempoOfs2;    ///< Fixed-point ratio value of offset 2.
        midibyte m_TempoOfs3;    ///< Fixed-point ratio value of offset 3.
        bool m_PunchEnabled;     ///< Auto-Punch enabled?
        midilong m_PunchInTime;  ///< Punch-in time.
        midilong m_PunchOutTime; ///< Punch-out time.
        midilong m_EndAllTime;   ///< Time of latest event (incl. all tracks).
        int m_division;          ///< TODO.
        midistring m_lastChunkData; ///< Holds the latest raw data chunk.
        std::list<RecTempo> m_tempos;   ///< Tempo data.
    };

private:

    wrkfile_private m_wrk_data;

    /**
     *  Holds a pointer to the (single) performer object in the Seq66
     *  session.  We save it in order to avoid having to pass it around to the
     *  numerous functions defined in the wrkfile class.  See the perfp()
     *  function.
     */

    performer * m_performer;

    /** Holds the screen-set number in force for reading this WRK file.  While
     * it is normally 0, it can be non-zero for WRK-file import.
     */

    int m_screen_set;

    /**
     *  If true, we are importing a file, most likely at a screen-set greater
     *  than 0 (the first and main screen-set.
     */

    bool m_importing;

    /**
     *  The number of the current sequencer, re 0.  It is -1 if a sequence is
     *  not yet in progress.
     */

    int m_seq_number;

    /**
     *  The current track number as obtained from the WRK file.  It is -1 if a
     *  track is not yet in progress.
     */

    int m_track_number;

    /**
     *  Saves the track-name for the NoteArray() function.
     */

    std::string m_track_name;

    /**
     *  Saves the track channel for the EndChunk() function.
     */

    int m_track_channel;

    /**
     *  The number of tracks/sequences created so far.
     */

    int m_track_count;

    /**
     *  Holds the maximum time encountered for the current track.
     */

    midipulse m_track_time;

    /**
     *  Holds the sequence currently being filled.  As in midifile, the
     *  sequence remains in memory for the duration of the performance.
     */

    sequence * m_current_seq;

public:

    wrkfile
    (
        const std::string & name,
        int ppqn,
        bool playlistmode = false
    );
    virtual ~wrkfile () override;

    virtual bool parse
    (
        performer & p, int screenset = 0, bool importing = false
    ) override;
    double get_real_time (midipulse ticks) const;

private:

    void Set_timestamp (event & e, midipulse rawtime);

    /**
     *  Returns an integer version of a midibyte, returning -1 if it was 255.
     */

    int ibyte (midibyte b) const
    {
        return b == 255 ? (-1) : int(b) ;
    }

    performer * perfp ()
    {
        return m_performer;
    }

    virtual sequence * create_sequence (performer & p) override;
    virtual bool finalize_sequence
    (
        performer & p, sequence & seq, int seqnum, int screenset
    ) override;
    void next_track
    (
        int trackno,
        int channel,
        const std::string & trackname,
        bool end_chunk = false
    );
    void finalize_track ();
    void not_supported (const std::string & tag);
    midishort to_16_bit (midibyte c1, midibyte c2);
    midilong to_32_bit (midibyte c1, midibyte c2, midibyte c3, midibyte c4);
    midilong read_16_bit ();
    midilong read_24_bit ();
    midilong read_32_bit ();
    std::string read_string (int len);
    std::string read_var_string ();
    void read_raw_data (int size);
    int read_chunk ();
    void NoteArray (int track, int events);
    void TrackChunk ();
    void VarsChunk ();
    void TimebaseChunk ();
    void StreamChunk ();
    void MeterChunk ();
    void TempoChunk (int factor = 1);
    void SysexChunk ();
    void Sysex2Chunk ();
    void NewSysexChunk ();
    void ThruChunk ();
    void TrackOffset ();
    void TrackReps ();
    void TrackPatch ();
    void TrackBank ();
    void TimeFormat ();
    void Comments ();
    void VariableRecord (int max);
    void NewTrack ();
    void SoftVer ();
    void TrackName ();
    void StringTable ();
    void LyricsStream ();
    void TrackVol ();
    void NewTrackOffset ();
    void MeterKeyChunk ();
    void SegmentChunk ();
    void NewStream ();
    void UnknownChunk (int id);
    void TrackNumPlusChunk ();
    void EndChunk ();

};          // class wrkfile

}           // namespace seq66

#endif      // SEQ66_WRKFILE_HPP

/*
 * wrkfile.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


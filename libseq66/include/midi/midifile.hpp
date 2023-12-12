#if ! defined SEQ66_MIDIFILE_HPP
#define SEQ66_MIDIFILE_HPP

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
 * \file          midifile.hpp
 *
 *  This module declares/defines the base class for MIDI files.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2023-12-11
 * \license       GNU GPLv2 or above
 *
 *  The Seq24 MIDI file is a standard, Format 1 MIDI file, with some extra
 *  "proprietary" tracks that hold information needed to set up the song in
 *  Seq24.
 *
 *  Seq66 can write out the Seq24 file with the "proprietary" tracks written
 *  in a format more palatable for strict MIDI programs, such as midicvt (a
 *  MIDI-to-ASCII conversion program available at the
 *  https://github.com/ahlstromcj/midicvt.git repository.
 *
 *  Seq66 can also split an SMF 0 file into multiple tracks, effectively
 *  converting it to SMF 1.
 */

#include <string>
#include <list>
#include <vector>

#include "cfg/rcsettings.hpp"           /* enum class rsaction              */
#include "midi/midibytes.hpp"           /* midishort, midibyte, etc.        */
#include "midi/midi_splitter.hpp"       /* seq66::midi_splitter             */
#include "util/automutex.hpp"           /* seq66::recmutex, automutex       */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class event;
    class midi_splitter;
    class midi_vector;
    class performer;

/**
 *  This class handles the parsing and writing of MIDI files.  In addition to
 *  the standard MIDI tracks, it also handles some "private" or "proprietary"
 *  tracks specific to Seq24.  It does not, however, handle SYSEX events.
 */

class midifile
{

public:

    /**
     *  Instead of having two save options, we now have three. These values
     *  were used in seq_gtkmm2/src/mainwnd.cpp.  Should use them in
     *  qsmainwnd.  Currently unused; distinguished by function call.
     */

    enum class save_option
    {
        normal,
        export_song,
        export_midi
    };

    static const std::string sm_meta_text_labels[8];

private:

    /**
     *  Provides locking for the sequence.  Made mutable for use in
     *  certain locked getter functions.
     */

    mutable seq66::recmutex m_mutex;

    /**
     *  Indicates if we are reading this file simply to verify it.  If so,
     *  then the song data will be removed after checking, via a call to
     *  performer::clear_all().
     */

    bool m_verify_mode;

    /**
     *  Holds the size of the MIDI file.  This variable was added when loading
     *  a file that caused an attempt to load data well beyond the file-size
     *  of the midicvt test file Dixie04.mid.
     */

    size_t m_file_size;

    /**
     *  Holds the last error message, useful for trouble-shooting without
     *  having Seq66 running in a console window.  If empty, there's no
     *  pending error.  Currently most useful in the parse() function.
     */

    std::string m_error_message;

    /**
     *  Indicates if the error should be considered fatal to the loading of
     *  the midifile.  The caller can query for this value after getting the
     *  return value from parse().
     */

    bool m_error_is_fatal;

    /**
     *  Indicates that file reading has already been disabled (due to serious
     *  errors), so don't complain about it anymore.  Once is enough.
     */

    bool m_disable_reported;

    /**
     *  Holds the value for how to handle mistakes in running status.
     */

    rsaction m_running_status_action;

    /**
     *  Holds the position in the MIDI file.  This is at least a 31-bit
     *  value in the recent architectures running Linux and Windows, so it
     *  will handle up to 2 Gb of data.  This member is used as the offset
     *  into the m_data vector.
     */

    size_t m_pos;

    /**
     *  The unchanging name of the MIDI file.
     */

    const std::string m_name;

    /**
     *  This vector of characters holds our MIDI data.  We could also use
     *  a string of characters, unsigned.  This member is resized to the
     *  putative size of the MIDI file, in the parse() function.  Then the
     *  whole file is read into it, as if it were an array.  This member is an
     *  input buffer.
     */

    std::vector<midibyte> m_data;

    /**
     *  Provides a list of characters.  The class pushes each MIDI byte into
     *  this list using the write_byte() function.  Also note that the write()
     *  function calls sequence::fill_list() to fill a temporary
     *  std::list<char> (!) buffer, then writes that data <i> backwards </i> to
     *  this member.  This member is an output buffer.
     */

    std::list<midibyte> m_char_list;

    /**
     *  Indicates to store the new key, scale, and background
     *  sequence in the global, "proprietary" section of the MIDI song.
     */

    bool m_global_bgsequence;

    /**
     *  Indicates that we are rescaling the PPQN of a file as it is read in.
     */

    bool m_use_scaled_ppqn;

    /**
     *  Provides the current value of the PPQN, which used to be constant.
     */

    int m_ppqn;

    /**
     *  The value of the PPQN from the file itself.
     */

    int m_file_ppqn;

    /**
     *  Provides the ratio of the main PPQN to the file PPQN, for use with scaling.
     */

    double m_ppqn_ratio;

    /**
     *  Provides support for SMF 0. This object holds all of the information
     *  needed to split a multi-channel sequence.
     */

    midi_splitter m_smf0_splitter;

public:

    midifile
    (
        const std::string & name,
        int ppqn,
        bool globalbgs      = true,
        bool playlistmode   = false
    );
    virtual ~midifile ();
    virtual bool parse
    (
        performer & p,
        int screenset = 0,
        bool importing = false
    );
    virtual bool write (performer & p, bool doseqspec = true);

    bool write_song (performer & p);

    const std::string & error_message () const
    {
        return m_error_message;
    }

    bool error_is_fatal () const
    {
        return m_error_is_fatal;
    }

    /**
     * \getter m_ppqn
     *      Provides a way to get the actual value of PPQN used in processing
     *      the sequences when parse() was called.  The PPQN will be either
     *      the global ppqn (legacy behavior) or the value read from the
     *      file, depending on the ppqn parameter passed to the midifile
     *      constructor.
     */

    int ppqn () const
    {
        return m_ppqn;
    }

    int file_ppqn () const
    {
        return m_file_ppqn;
    }

    double ppqn_ratio () const
    {
        return m_ppqn_ratio;
    }

    bool scaled () const
    {
        return m_use_scaled_ppqn;
    }

    /**
     *  Current position in the data stream.
     */

    size_t pos ()
    {
        return m_pos;
    }

protected:

    virtual sequence * create_sequence (performer & p);
    virtual bool finalize_sequence
    (
        performer & p, sequence & seq, int seqnum, int screenset
    );

    bool verify_mode () const
    {
        return m_verify_mode;
    }

    void clear_errors ()
    {
        m_error_message.clear();
        m_disable_reported = false;
    }

    void ppqn (int p)
    {
        m_ppqn = p;
    }

    void file_ppqn (int p)
    {
        m_file_ppqn = p;
    }

    void ppqn_ratio (double r)
    {
        m_ppqn_ratio = r;
    }

    void scaled (bool flag)
    {
        m_use_scaled_ppqn = flag;
    }

    /**
     *  Checks if the data stream pointer has reached the end position
     *
     * \return
     *      Returns true if the read pointer is at the end.
     */

    bool at_end () const
    {
        return m_disable_reported || m_pos >= m_file_size;
    }

    bool grab_input_stream (const std::string & tag);
    bool parse_smf_0 (performer & p, int screenset);
    bool parse_smf_1 (performer & p, int screenset, bool is_smf0 = false);

    midilong parse_seqspec_header (int file_size);
    bool parse_seqspec_track (performer & p, int file_size);
    bool prop_header_loop (performer & p, int file_size);
    bool parse_c_midictrl (performer & p);
    bool parse_c_midiclocks (performer & p);
    bool parse_c_notes (performer & p);
    bool parse_c_bpmtag (performer & p);
    bool parse_c_mutegroups (performer & p);
    bool parse_c_mutegroups_legacy
    (
        performer & p,
        unsigned groupcount,
        unsigned groupsize
    );
    bool parse_c_musickey ();
    bool parse_c_musicscale ();
    bool parse_c_backsequence ();
    bool parse_c_perf_bp_mes (performer & p);
    bool parse_c_perf_bw (performer & p);
    bool parse_c_tempo_track ();
    bool write_c_mutegroups (const performer & p);
    bool checklen (midilong len, midibyte type);
    void add_trigger (sequence & seq, midishort ppqn, bool tposable);
    void add_old_trigger (sequence & seq);
    bool read_seek (size_t pos);
    midilong read_long ();
    midilong read_split_long (unsigned & highbytes, unsigned & lowbytes);
    midishort read_short ();
    midibyte read_byte ();
    midilong read_varinum ();
    bool read_byte_array (midibyte * b, size_t len);
    bool read_byte_array (midistring & b, size_t len);
    bool read_string (std::string & b, size_t len);
    bool read_meta_data (sequence & s, event & e, midibyte metatype, size_t len);
    int read_sysex_data
    (
        sequence & s, event & e,
        size_t len, bool continuation = false
    );
    void read_gap (size_t sz);

    midibyte peek () const
    {
        return m_data[m_pos];
    }

    void skip (size_t sz)                       /* compare to read_gap()    */
    {
        m_pos += sz;                            /* sz can be 0 or positive  */
    }

    void back_up (size_t sz)
    {
        m_pos -= sz;                            /* sz can be 0 or positive  */
    }

    void write_long (midilong value);
    void write_split_long
    (
        unsigned highbytes, unsigned lowbytes, bool oldstyle = false
    );
    void write_triple (midilong value);
    void write_short (midishort value);

    /**
     *  Writes 1 byte.  The byte is written to the m_char_list member, using a
     *  call to push_back().
     *
     * \param c
     *      The MIDI byte to be "written".
     */

    void write_byte (midibyte c)
    {
        m_char_list.push_back(c);
    }

    void write_varinum (midilong);
    void write_track (const midi_vector & lst);
    void write_track_name (const std::string & trackname);
    void write_track_end ();
    std::string read_track_name ();
    long track_name_size (const std::string & trackname) const;
    void write_seq_number (midishort seqnum);
    int read_seq_number ();
    bool write_header (int numtracks, int smfformat = 1);
#if defined SEQ66_USE_WRITE_START_TEMPO
    void write_start_tempo (midibpm start_tempo);
#endif
#if defined SEQ66_USE_WRITE_TIME_SIG
    void write_time_sig (int beatsperbar, int beatwidth);
#endif
    void write_seqspec_header (midilong tag, long len);
    bool write_seqspec_track (performer & p);
    long varinum_size (long len) const;
    long prop_item_size (long datalen) const;
    bool set_error (const std::string & msg);
    bool set_error_dump (const std::string & msg);
    bool set_error_dump (const std::string & msg, unsigned long p);

    /**
     *  Returns the size of a sequence-number event, which is always 5
     *  bytes, plus one byte for the delta time that precedes it.
     */

    long seq_number_size () const
    {
        return 6;
    }

    /**
     *  Returns the size of a track-end event, which is always 3 bytes.
     */

    long track_end_size () const
    {
        return 3;
    }

    /**
     *  Check for special SysEx ID byte.
     *
     * \param ch
     *      Provides the byte to be checked against 0x7D through 0x7F.
     *
     * \return
     *      Returns true if the byte is SysEx special ID.
     *
     * THIS FUNCTION IS WRONG, BOGUS!!!
     *
     *  bool is_sysex_special_id (midibyte ch)
     *  {
     *      return ch >= 0x7D && ch <= 0x7F;
     *  }
     *
     */

};          // class midifile

/*
 *  Free functions related to midifile.
 */

extern bool read_midi_file
(
    performer & p,
    const std::string & fn,
    int ppqn,
    std::string & errmsg,
    bool addtorecent = true
);
extern bool write_midi_file
(
    performer & p,
    const std::string & fn,
    std::string & errmsg
);

}           // namespace seq66

#endif      // SEQ66_MIDIFILE_HPP

/*
 * midifile.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


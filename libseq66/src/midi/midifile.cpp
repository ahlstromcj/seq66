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
 * \file          midifile.cpp
 *
 *  This module declares/defines the base class for MIDI files.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2023-04-08
 * \license       GNU GPLv2 or above
 *
 *  For a quick guide to the MIDI format, see, for example:
 *
 *  http://www.mobilefish.com/tutorials/midi/midi_quickguide_specification.html
 *
 *  It is important to note that most sequencers have taken a shortcut or two
 *  in reading the MIDI format.  For example, most will silently ignored an
 *  unadorned control tag (0x242400nn) which has not been packaged up as a
 *  proper sequencer-specific meta event.  The midicvt program
 *  (https://github.com/ahlstromcj/midicvt, derived from midicomp, midi2text,
 *  and mf2t/t2mf) does not ignore this lack of a SeqSpec wrapper, and hence
 *  we decided to provide a new, more strict input and output format for the
 *  the "proprietary"/SeqSpec track in Seq66.
 *
 *  Elements written:
 *
 *      -   MIDI header.
 *      -   Tracks.
 *          These items are then written, preceded by the "MTrk" tag and
 *          the track size.
 *          -   Sequence number.
 *          -   Sequence name.
 *          -   Time-signature and tempo (track 0 only)
 *          -   Sequence events.
 *
 *  Items handled in midi_vector_base:
 *
 *      -   Key signature.  Although Seq66 does not create or use this
 *          event, if present, it is preserved so that it can be written out
 *          to the file when saved.
 *      -   Time signature.
 *      -   Tempo.
 *      -   Sequence number.
 *      -   Track end.
 *      -   Seq66-specific SeqSpec data.
 *
 *  Uses the new format for the Seq66 footer section of the Seq24 MIDI
 *  file.
 *
 *  In the new format, each sequencer-specfic value (0x242400xx) is preceded
 *  by the sequencer-specific prefix, 0xFF 0x7F len). By default, the new
 *  format is used.  In Seq66, the old format is no longer supported.
 *
 * Running status:
 *
 *      A recommended approach for a receiving device is to maintain its
 *      "running status buffer" as so:
 *
 *      -#  Buffer is cleared (ie, set to 0) at power up.
 *      -#  Buffer stores the status when a Voice Category Status (ie, 0x80
 *          to 0xEF) is received.
 *      -#  Buffer is cleared when a System Common Category Status (ie, 0xF0
 *          to 0xF7) is received.
 *      -#  Nothing is done to the buffer when a RealTime Category message is
 *          received.
 *      -#  Any data bytes are ignored when the buffer is 0.
 */

#include <fstream>                      /* std::ifstream and std::ofstream  */
#include <memory>                       /* std::unique_ptr<>                */

#include "cfg/settings.hpp"             /* seq66::rc() and choose_ppqn()    */
#include "midi/midifile.hpp"            /* seq66::midifile                  */
#include "midi/midi_vector.hpp"         /* seq66::midi_vector container     */
#include "midi/wrkfile.hpp"             /* seq66::wrkfile class             */
#include "play/performer.hpp"           /* seq66::performer                 */
#include "play/sequence.hpp"            /* seq66::sequence                  */
#include "util/filefunctions.hpp"       /* seq66::get_full_path()           */
#include "util/palette.hpp"             /* seq66::palette_to_int(), colors  */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Minimal MIDI file size.  Just used for a sanity check.
 *
 * <Header Chunk> = <chunk type><length><format><ntrks><division>
 *
 * Bytes:                 4   +   4   +   2  +   2   +   2   = 14
 */

static const size_t c_minimum_midi_file_size = 14;

/**
 *  Magic number for handling mute-group formats.
 */

static const unsigned c_legacy_mute_group = 1024;           /* 0x0400       */

/**
 *  A manifest constant for controlling the length of the stream buffering
 *  array in a MIDI file.
 */

static const int c_midi_line_max = 1024;

/**
 *  The maximum length of a Seq24/Seq66 track nam3.
 */

static const int c_trackname_max =  256;

/**
 *  The maximum allowed variable length value for a MIDI file, which allows
 *  the length to fit in a 32-bit integer.
 */

static const int c_varlength_max = 0x0FFFFFFF;

/**
 *  Highlights the MIDI file header value, "MThd".
 */

static const miditag c_mthd_tag  = 0x4D546864;      /* magic number 'MThd'  */

/**
 *  Highlights the MIDI file track-marker (chunk) value, "MTrk".
 */

static const miditag c_mtrk_tag  = 0x4D54726B;      /* magic number 'MTrk'  */

/**
 *  The chunk header value for the Seq66 proprietary/SeqSpec section.  We
 *  might try other chunks, as well, since, as per the MIDI specification,
 *  unknown chunks should not cause an error in a sequencer (or our midicvt
 *  program).  For now, we stick with "MTrk".
 */

static const miditag c_prop_chunk_tag = c_mtrk_tag;

/**
 *  Provides the track number for the proprietary/SeqSpec data when using
 *  the new format.
 *  Can't use numbers, such as 0xFFFF, that have MIDI meta tags in them,
 *  confuses the "SeqSpec" track parser.
 */

static const midishort c_prop_seq_number     = 0x3FFF;
static const midishort c_prop_seq_number_old = 0x7777;

/**
 *  Provides the track name for the "proprietary" data when using the new
 *  format.  (There is no track-name for the "proprietary" footer track when
 *  the legacy format is in force.)  This is more useful for examining a hex
 *  dump of a Seq66 song than for checking its validity.  It's overkill
 *  that causes needless error messages.
 */

static const std::string c_prop_track_name = "Seq66-S";

/**
 *  This const is used for detecting SeqSpec data that Seq66 does not handle.
 *  If this word is found, then we simply extract the expected number of
 *  characters specified by that construct, and skip them when parsing a MIDI
 *  file.
 */

static const miditag c_prop_tag_word = 0x24240000;

/**
 *  Defines the size of the time-signature and tempo information.  The sizes of
 *  these meta events consists of the delta time of 0 (1 byte), the event and
 *  size bytes (3 bytes), and the data (4 bytes for time-signature and 3 bytes
 *  for the tempo.  So, 8 bytes for the time-signature and 7 bytes for the
 *  tempo.  Unused, so commented out.
 *
 *      static const int c_time_tempo_size  = 15;
 */

/*
 *  Internal functions.
 */

/**
 *  An easier, shorter test for the c_prop_tag_word part of a long
 *  value, that clients can use.
 */

static bool
is_proptag (midilong p)
{
    return (miditag(p) & c_prop_tag_word) == c_prop_tag_word;
}

/**
 *  Name of the initial text meta events (00 through 07).
 */

const std::string midifile::sm_meta_text_labels[8] =
{
    "Seq number",
    "Text",
    "Copyright",
    "Track Name",
    "Instrument Name",
    "Lyric",
    "Marker",
    "Cue Point"
};

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
 *          -   If usr().use_file_ppqn() is true, then m_ppqn is set to the
 *              value read from the MIDI file.  No PPQN scaling is done.
 *          -   Otherwise, the ppqn value is used as is.  If the file uses a
 *              different PPQN than the default, PPQN rescaling is done to
 *              make it so.  The PPQN value read from the MIDI file is used to
 *              scale the running-time of the track relative to
 *              usr().default_ppqn().
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
 *      in writing; reading can handle either format transparently. Depends on
 *      usr().global_seq_feature() returning true.
 *
 * \param verifymode
 *      If set to true, we are opening files just to verify them before
 *      accepting the usage of a playlist.  In this case, we clear out each
 *      song after it is read in for verification.  It defaults to false.
 *      Actually, the playlist::verify() function clears the song, but this
 *      variable is still useful to cut down on output during verfication.
 *      See grab_input_stream().
 */

midifile::midifile
(
    const std::string & name,
    int ppqn,
    bool globalbgs,
    bool verifymode
) :
    m_mutex                     (),
    m_verify_mode               (verifymode),
    m_file_size                 (0),
    m_error_message             (),
    m_error_is_fatal            (false),
    m_disable_reported          (false),
    m_pos                       (0),
    m_name                      (name),
    m_data                      (),
    m_char_list                 (),
    m_global_bgsequence         (globalbgs),
    m_use_scaled_ppqn           (false),                /* scaled()         */
    m_ppqn                      (ppqn),                 /* can start as 0   */
    m_file_ppqn                 (0),                    /* can change       */
    m_ppqn_ratio                (1.0),                  /* for scaled()     */
    m_smf0_splitter             ()
{
    // no other code needed
}

/**
 *  A rote destructor.
 */

midifile::~midifile ()
{
    // empty body
}

/**
 *  Seeks to a new, absolute, position in the data stream.  All this function
 *  does is change the value of m_pos.  All of the file is already in memory.
 *
 * \param pos
 *      Provides the new position to seek.
 *
 * \return
 *      Returns true if the seek could be accomplished.  No error message is
 *      logged, but the caller should take evasive action if false is
 *      returned.  And, in that case, m_pos is unchanged.
 */

bool
midifile::read_seek (size_t pos)
{
    bool result = false;
    if (pos < m_file_size)
    {
        m_pos = pos;            // m_d->m_IOStream->device()->seek(pos);
        result = true;
    }
    return result;
}

/**
 *  Reads 1 byte of data directly from the m_data vector, incrementing
 *  m_pos after doing so.
 *
 * \return
 *      Returns the byte that was read.  Returns 0 if there was an error,
 *      though there's no way for the caller to determine if this is an error
 *      or a good value.
 */

midibyte
midifile::read_byte ()
{
    if (m_pos < m_file_size)
    {
        return m_data[m_pos++];
    }
    else if (! m_disable_reported)
        (void) set_error_dump("'End-of-file', further MIDI reading disabled");

    return 0;
}

/**
 *  Reads 2 bytes of data using read_byte().
 *
 * \return
 *      Returns the two bytes, shifted appropriately and added together,
 *      most-significant byte first, to sum to a short value.
 */

midishort
midifile::read_short ()
{
    midishort result = read_byte() << 8;
    result += read_byte();
    return result;
}

/**
 *  Reads 4 bytes of data using read_byte().
 *
 * \return
 *      Returns the four bytes, shifted appropriately and added together,
 *      most-significant byte first, to sum to a long value.
 */

midilong
midifile::read_long ()
{
    midilong result = read_byte();
    result <<= 24;
    result += read_byte() << 16;
    result += read_byte() << 8;
    result += read_byte();
    return result;
}

/**
 *  We want to be able to support a special case, the c_mutegroups count.  In
 *  Seq24, the count was always 1024 (0x400), which would be represented in
 *  the MIDI file as 0x00 0x04 0x00 0x00.  Other numbers use the same
 *  encoding, which does not use the MIDI VLV encoding at all.  The bytes are
 *  stored conventionally.
 *
 *  We also want to be able to support counts other than 1024 (32 groups x 32
 *  sequences), such as 1024 = 16 groups x 64 sequences and 1536 = 16 groups x
 *  96 sequences.  We can adopt a special encoding if the value is not 0 or
 *  1024:
 *
 *      -   2-bytes group countvalue
 *      -   2-bytes sequence count value
 *
 *  So, 16 groups x 64 sequences would be represented by 0x1040 = 4160 dec.
 *  16 groups x 96 sequences would be represented by 0x1060 = 4192 dec.  And
 *  1024 dec = 0x0400 would represent 4 groups and 0 sequences, obviously
 *  bogus.
 *
 *  Recall that midilong (see midi/midibytes.hpp) is still signed.
 *
 * \param [out] highbytes
 *      Provides the value of the most significant 2 bytes of the four-byte
 *      ("long") value.  If the value read is 0, 0 is written to this
 *      parameter.  If the value is 1024, then 32 is written.
 *
 * \param [out] lowbytes
 *      Provides the value of the least significant 2 bytes of the four-byte
 *      ("long") value.  If the value read is 0, 0 is written to this
 *      parameter.  If the value is 1024, then 32 is written.
 *
 * \return
 *      Returns the value returned by read_long().
 */

midilong
midifile::read_split_long (unsigned & highbytes, unsigned & lowbytes)
{
    unsigned short high = static_cast<unsigned short>(read_byte());
    high <<= 8;
    high += static_cast<unsigned short>(read_byte());

    unsigned short low = static_cast<unsigned short>(read_byte());
    low <<= 8;
    low += static_cast<unsigned short>(read_byte());

    midilong result = (midilong(high) << 16) + midilong(low);
    if (result == c_legacy_mute_group)                  /* 1024 (0x0400)    */
    {
        high = 32U;
        low  = 32U;
    }
    else if (result == 0)
    {
        high = 0;
        low  = 0;
    }
    highbytes = static_cast<unsigned>(high);
    lowbytes = static_cast<unsigned>(low);
    return result;
}

/**
 *  A helper function to simplify reading midi_control data from the MIDI
 *  file.
 *
 * \param b
 *      The byte array to receive the data.
 *
 * \param len
 *      The number of bytes in the array, and to be read.
 *
 * \return
 *      Returns true if any bytes were read.  Do not use \a b if false is
 *      returned.
 */

bool
midifile::read_byte_array (midibyte * b, size_t len)
{
    bool result = not_nullptr(b) && len > 0;
    if (result)
    {
        for (size_t i = 0; i < len; ++i)
            *b++ = read_byte();
    }
    return result;
}

/**
 *  A helper function for arbitrary, otherwise unhandled meta data.
 */

bool
midifile::read_meta_data (sequence & s, event & e, midibyte metatype, size_t len)
{
    bool result = checklen(len, metatype);
    if (result)
    {
        std::vector<midibyte> bt;
        for (int i = 0; i < int(len); ++i)
            bt.push_back(read_byte());

        bool ok = e.append_meta_data(metatype, bt);
        if (ok)
            s.append_event(e);
    }
    return result;
}

/**
 *  A overload function to simplify reading midi_control data from the MIDI
 *  file.  It uses a standard string object instead of a buffer.
 *
 * \param b
 *      The std::string to receive the data.
 *
 * \param len
 *      The number of bytes to be read.
 *
 * \return
 *      Returns true if any bytes were read.  The string \a b will be empty if
 *      false is returned.
 */

bool
midifile::read_string (std::string & b, size_t len)
{
    bool result = len > 0;
    b.clear();
    if (result)
    {
        if (len > b.capacity())
            b.reserve(len);

        for (size_t i = 0; i < len; ++i)
            b.push_back(read_byte());
    }
    return result;
}

/**
 *  A overload function to simplify reading midi_control data from the MIDI
 *  file.  It uses a midistring object instead of a buffer.
 *
 * \param b
 *      The midistring to receive the data.
 *
 * \param len
 *      The number of bytes to be read.
 *
 * \return
 *      Returns true if any bytes were read.  The string \a b will be empty if
 *      false is returned.
 */

bool
midifile::read_byte_array (midistring & b, size_t len)
{
    bool result = len > 0;
    b.clear();
    if (result)
    {
        if (len > b.capacity())
            b.reserve(len);

        for (size_t i = 0; i < len; ++i)
            b.push_back(read_byte());
    }
    return result;
}

/**
 *  Read a MIDI Variable-Length Value (VLV), which has a variable number
 *  of bytes.  This function reads the bytes while bit 7 is set in each
 *  byte.  Bit 7 is a continuation bit.  See write_varinum() for more
 *  information.
 *
 * \return
 *      Returns the accumulated values as a single number.
 */

midilong
midifile::read_varinum ()
{
    midilong result = 0;
    midibyte c;
    while (((c = read_byte()) & 0x80) != 0x00)      /* while bit 7 is set  */
    {
        result <<= 7;                               /* shift result 7 bits */
        result += c & 0x7F;                         /* add bits 0-6        */
    }
    result <<= 7;                                   /* bit was clear       */
    result += c & 0x7F;
    return result;
}

/**
 *  Jumps/skips the given number of bytes in the data stream.  If too large,
 *  the position is left at the end.  Primarily used in the derived class
 *  wrkfile.
 *
 * \param sz
 *      Provides the gap size, in bytes.
 */

void
midifile::read_gap (size_t sz)
{
    if (sz > 0)
    {
        size_t p = m_pos + sz;
        if (p >= m_file_size)
        {
            p = m_file_size - 1;
            if (! m_disable_reported)
                (void) set_error_dump("'End-of-file', MIDI reading disabled");
        }
        m_pos = p;
    }
}

/**
 *  Creates the stream input, reads it into the "buffer", and then closes
 *  the file.  No file buffering needed on these beefy machines!  :-)
 *  As a side-effect, also sets m_file_size.
 *
 *  We were using the assignment operator, but this caused an error using old
 *  32-bit debian stable, g++ 4.9 on one of our old laptops.  The assignment
 *  operator was deleted by the compiler.  So now we use constructor notation.
 *  A little bit odd, since we thought the compiler would convert assignment
 *  operator notation to constructor notation, but hey, compilers are not
 *  perfect.  Also, no need to use the krufty string pointer for the
 *  file-name.
 *
 * \param tag
 *      Basically an informative string to denote what kind of file is being
 *      opened, "MIDI" or "WRK".
 *
 * \return
 *      Returns true if the input stream was successfully opend on a good
 *      file.  Use it only if the return value is true.
 */

bool
midifile::grab_input_stream (const std::string & tag)
{
    if (m_name.empty())
        return false;

    std::ifstream file(m_name, std::ios::in | std::ios::binary | std::ios::ate);
    bool result = file.is_open();
    m_error_is_fatal = false;
    if (result)
    {
        try
        {
            (void) file.seekg(0, file.end);     /* seek to the file's end   */
            m_file_size = file.tellg();         /* get the end offset       */
        }
        catch (...)
        {
            m_file_size = 0;
        }
        if (m_file_size < c_minimum_midi_file_size)
        {
            result = set_error("File too small");
        }
        else
        {
            file.seekg(0, std::ios::beg);       /* seek to the file's start */
            try
            {
                m_data.resize(m_file_size);     /* allocate the data        */
                file.read((char *)(&m_data[0]), m_file_size);
            }
            catch (const std::bad_alloc & ex)
            {
                result = set_error("MIDI file stream memory allocation failed");
            }
            file.close();
        }
    }
    else
    {
        std::string errmsg = "Open failed: ";
        errmsg += tag;
        errmsg += " file '";
        errmsg += m_name;
        errmsg += "'";
        result = set_error(errmsg);
    }
    return result;
}

/**
 *  This function opens a binary MIDI file and parses it into sequences
 *  and other application objects.
 *
 *  In addition to the standard MIDI track data in a normal track,
 *  Seq24/Seq66 adds four sequencer-specific events just before the end
 *  of the track:
 *
\verbatim
    c_midibus:          SeqSpec FF 7F 05 24 24 00 01 00
    c_midichannel:      SeqSpec FF 7F 05 24 24 00 02 06
    c_timesig:          SeqSpec FF 7F 06 24 24 00 06 04 04
    c_triggers_ex:      SeqSpec FF 7F 1C 24 24 00 08 00 00 ...
    c_trig_transpose:   SeqSpec FF 7F 1C 24 24 00 20 00 00 ...
\endverbatim
 *
 *  Note that only Seq66 adds "FF 7F len" to the SeqSpec data.
 *
 *  Standard MIDI provides for port and channel specification meta events, but
 *  they are apparently considered obsolete:
 *
\verbatim
    Obsolete meta-event:                Replacement:
    MIDI port (buss):   FF 21 01 po     Device (port) name: FF 09 len text
    MIDI channel:       FF 20 01 ch
\endverbatim
 *
 *  What do other applications use for specifying port/channel?
 *
 *  Note the is-modified flag:  We now assume that the performer object is
 *  starting from scratch when parsing.  But we let mainwnd tell the performer
 *  object when to clear everything with performer::clear_all().  The mainwnd
 *  does this for a new file, opening a file, but not for a file import, which
 *  might be done simply to add more MIDI tracks to the current composition.
 *  So, if parsing succeeds, all we want to do is make sure the flag is set.
 *  Parsing a file successfully is not always a modification of the setup.
 *  For instance, the first read of a MIDI file should start clean, not dirty.
 *
 * SysEx notes:
 *
 *      Some files (e.g. Dixie04.mid) do not always encode System Exclusive
 *      messages properly for a MIDI file.  Instead of a varinum length value,
 *      they are followed by extended IDs (0x7D, 0x7E, or 0x7F).
 *
 *      We've covered some of those cases by disabling access to m_data if the
 *      position passes the size of the file, but we want try to bypass these
 *      odd cases properly.  So we look ahead for one of these special values.
 *
 *      Currently, Seq66, like Se24, handles SysEx message only to the
 *      extend of passing them via MIDI Thru.  We hope to improve on that
 *      capability eventually.
 *
 * \param p
 *      Provides a reference to the performer object into which sequences/tracks
 *      are to be added.
 *
 * \param screenset
 *      The screen-set offset to be used when loading a sequence (track) from
 *      the file.  This value ranges from -31 to 0 to +31 (32 is the maximum
 *      screen-set available in Seq24).  This offset is added to the sequence
 *      number read in for the sequence, to place it elsewhere in the imported
 *      tune, and locate it in a specific screen-set.  If this parameter is
 *      non-zero, then we will assume that the performer data is dirty. The
 *      default is 0.
 *
 * \param importing
 *      Indicates that we are importing a file, and do not want to parse/erase
 *      any "proprietrary" information from the performance.  Defaults to false.
 *      Also will flag a modification.
 *
 * \return
 *      Returns true if the parsing succeeded.  Note that the error status is
 *      saved in m_error_is_fatal, and a message (to display later) is saved
 *      in m_error_message.
 */

bool
midifile::parse (performer & p, int screenset, bool importing)
{
    bool result = grab_input_stream(std::string("MIDI"));
    if (result)
    {
        midilong ID = read_long();                      /* hdr chunk info   */
        midilong hdrlength = read_long();               /* MThd length      */
        clear_errors();
        if (ID != c_mthd_tag && hdrlength != 6)     /* magic 'MThd'     */
            return set_error_dump("Invalid MIDI header chunk detected", ID);

        midishort Format = read_short();                /* 0, 1, or 2       */
        m_smf0_splitter.initialize();                   /* SMF 0 support    */
        if (Format == 0)
        {
            result = parse_smf_0(p, screenset);
            p.smf_format(0);
        }
        else if (Format == 1)
        {
            result = parse_smf_1(p, screenset);
            p.smf_format(1);
        }
        else
        {
            m_error_is_fatal = true;
            result = set_error_dump
            (
                "Unsupported MIDI format number", midilong(Format)
            );
        }
        if (result)
        {
            if (m_pos < m_file_size)                    /* any data left?   */
            {
                if (! importing)
                    result = parse_seqspec_track(p, m_file_size);
            }
            if (result && importing)
                 p.modify();                            /* modify flag      */
        }
    }
    else
    {
        m_error_is_fatal = true;
        result = set_error_dump("Cannot open MIDI", 0);
    }
    return result;
}

/**
 *  This function parses an SMF 0 binary MIDI file as if it were an SMF 1
 *  file, then, if more than one MIDI channel was encountered in the sequence,
 *  splits all of the channels in the sequence out into separate sequences.
 *  The original sequence remains in place, in sequence slot 16 (the 17th
 *  slot).  The user is responsible for deleting it if it is not needed.
 *
 * \param p
 *      Provides a reference to the performer object into which sequences/tracks
 *      are to be added.
 *
 * \param screenset
 *      The screen-set offset to be used when loading a sequence (track) from
 *      the file.
 *
 * \return
 *      Returns true if the parsing succeeded.
 */

bool
midifile::parse_smf_0 (performer & p, int screenset)
{
    bool c = usr().convert_to_smf_1();              /* true by default      */
    bool result = parse_smf_1(p, screenset, c);     /* format 0 conversion? */
    if (c)
    {
        if (result)
        {
            result = m_smf0_splitter.split(p, screenset, m_ppqn);
            if (result)
            {
                p.modify();                         /* to prompt for save   */
                p.smf_format(1);                    /* converted to SMF 1   */
            }
            else
                result = set_error("No SMF 0 track found, bad MIDI file");
        }
    }
    else if (result)
    {
        seq::pointer s = p.get_sequence(0);
        if (s)
        {
            s->set_midi_channel(null_channel());
            s->set_color(palette_to_int(cyan));
            p.smf_format(0);
        }
    }
    return result;
}

/**
 *  Internal function to check for and report a bad length value.
 *  A length of zero is now considered legal, but a "warning" message is shown.
 *  The largest value allowed within a MIDI file is 0x0FFFFFFF. This limit is
 *  set to allow variable-length quantities to be manipulated as 32-bit
 *  integers.
 *
 * \param len
 *      The length value to be checked, and it should be greater than 0.
 *      However, we have seen files with zero-length events, such as Lyric
 *      events (0x05).
 *
 * \param type
 *      The type of meta event.  Used for displaying an error.
 *
 * \return
 *      Returns true if the length parameter is valid.  This now means it is
 *      simply less than 0x0FFFFFFF.
 */

bool
midifile::checklen (midilong len, midibyte type)
{
    bool result = len <= c_varlength_max;                   /* 0x0FFFFFFF */
    if (result)
    {
        result = len > 0;
        if (! result)
        {
            char m[40];
            snprintf(m, sizeof m, "0 data length for meta type 0x%02X", type);
            (void) set_error_dump(m);
        }
    }
    else
    {
        char m[40];
        snprintf(m, sizeof m, "bad data length for meta type 0x%02X", type);
        (void) set_error_dump(m);
    }
    return result;
}

/**
 *  Internal function to make the parser easier to read.  Handles the
 *  c_triggers_ex and c_trig_transpose values, as well as the old and
 *  deprecated c_triggers value.
 *
 *  If m_ppqn isn't set to the default value, then we must scale these triggers
 *  accordingly, just as is done for the MIDI events.
 *
 * \param seq
 *      Provides the sequence to which the trigger is to be added.
 *
 * \param oldppqn
 *      Provides the ppqn value to use to scale the tick values if
 *      scaled() is true.  If 0, the ppqn value is not used.
 *
 * \param transposable
 *      If true, use the new style trigger, which adds a byte-long transpose
 *      value.
 */

void
midifile::add_trigger (sequence & seq, midishort oldppqn, bool transposable)
{
    midilong on = read_long();
    midilong off = read_long();
    midilong offset = read_long();
    midibyte tpose = 0;
    if (transposable)
        tpose = read_byte();

    if (oldppqn > 0)
    {
        on = rescale_tick(on, m_ppqn, oldppqn);         /* new and old PPQN */
        off = rescale_tick(off, m_ppqn, oldppqn);
        offset = rescale_tick(offset, m_ppqn, oldppqn);
    }
    seq.add_trigger(on, off - on + 1, offset, tpose, false);
}

void
midifile::add_old_trigger (sequence & seq)
{
    midilong on = read_long();
    midilong off = read_long();
    seq.add_trigger(on, off - on + 1, 0, false, false);
}

/**
 *  This function parses an SMF 1 binary MIDI file; it is basically the
 *  original seq66 midifile::parse() function.  It assumes the file-data has
 *  already been read into memory.  It also assumes that the ID, track-length,
 *  and format have already been read.
 *
 *  If the MIDI file contains both proprietary (c_timesig) and MIDI type 0x58
 *  then it came from seq42 or seq32 (Stazed versions).  In this case the MIDI
 *  type is parsed first (because it is listed first) then it gets overwritten
 *  by the proprietary, above.
 *
 *  Note that track_count doesn't count the Seq24 "proprietary" footer
 *  section, even if it uses the new format, so that section will still be
 *  read properly after all normal tracks have been processed.
 *
 * PPQN:
 *
 *      Current time (runningtime) is re the ppqn according to the file, we
 *      have to adjust it to our own ppqn.  PPQN / ppqn gives us the ratio.
 *      (This change is not enough; a song with a ppqn of 120 plays too fast
 *      in Seq24, which has a constant ppqn of 192.  Triggers must also be
 *      modified.)
 *
 * Tempo events:
 *
 *      If valid, set the global tempo to the first encountered tempo; this is
 *      legacy behavior.  Bad tempos occur and stick around, munging exported
 *      songs.  We log only the first tempo officially; the rest are stored as
 *      events if in the first track.  We also adjust the upper draw-tempo
 *      range value to about twice this value, to give some headroom... it
 *      will not be saved unless the --user-save option is in force.
 *
 * Channel:
 *
 *      We are transitioning away from preserving the channel in the status
 *      byte, which will require using the event::m_channel value.
 *
 * Time Signature:
 *
 *      Like Tempo, Time signature is now handled more robustly.
 *
 * Key Signature and other Meta events:
 *
 *      Although we don't support these events, we do want to keep them, so we
 *      can output them upon saving.  Instead of bypassing unhandled Meta
 *      events, we now store them, so that they are not lost when
 *      exporting/saving the MIDI data.
 *
 * Track name:
 *
 *      This event is optional. It's interpretation depends on its context. If
 *      it occurs in the first track of a format 0 or 1 MIDI file, then it
 *      gives the Sequence Name. Otherwise it gives the Track Name.
 *
 * End of Track:
 *
 *      "If delta is 0, then another event happened at the same time as
 *      track-end.  Class sequence discards the last note.  This fixes that.
 *      A native Seq24 file will always have a delta >= 1." Not true!  We've
 *      fixed the real issue by commenting the code that increments current
 *      time.  Question:  What if BPM is set *after* this event?
 *
 * Sequences:
 *
 *      If the sequence is shorter than a quarter note, assume it needs to be
 *      padded to a measure.  This happens anyway if the short pattern is
 *      opened in the sequence editor (seqedit).
 *
 *      Add sorting after reading all the events for the sequence.  Then add
 *      the sequence with it's preferred location as a hint.
 *
 * Unknown chunks:
 *
 *      Let's say we don't know what kind of chunk it is.  It's not a MTrk, we
 *      don't know how to deal with it, so we just eat it.  If this happened
 *      on the first track, it is a fatal error.
 *
 * \param p
 *      Provides a reference to the performer object into which sequences/tracks
 *      are to be added.
 *
 * \param screenset
 *      The screen-set offset to be used when loading a sequence (track) from
 *      the file.
 *
 * \param is_smf0
 *      True if we detected that the MIDI file is in SMF 0 format.
 *
 * \return
 *      Returns true if the parsing succeeded.
 */

bool
midifile::parse_smf_1 (performer & p, int screenset, bool is_smf0)
{
    bool result = true;
    midibyte buss_override = usr().midi_buss_override();
    midishort track_count = read_short();
    midishort fileppqn = read_short();
    bool gotfirst_bpm = false;
    file_ppqn(int(fileppqn));                       /* original file PPQN   */
    if (usr().use_file_ppqn())
    {
        p.file_ppqn(file_ppqn());                   /* let performer know   */
        ppqn(file_ppqn());                          /* PPQN == file PPQN    */
        scaled(false);                              /* do not scale time    */
    }
    else
    {
        scaled(file_ppqn() != usr().default_ppqn());
        if (scaled())
            ppqn_ratio(double(ppqn()) / double(file_ppqn()));
    }
    if (rc().verbose())
    {
        if (! is_null_buss(buss_override))
        {
            infoprintf("Buss override %d", int(buss_override));
        }
        infoprintf("Track count %d", int(track_count));
    }
    for (midishort track = 0; track < track_count; ++track)
    {
        midibyte tentative_channel = null_channel();
        midilong ID = read_long();                  /* get track marker     */
        midilong TrackLength = read_long();         /* get track length     */
        if (ID == c_mtrk_tag)                       /* magic number 'MTrk'  */
        {
            bool timesig_set = false;               /* seq66 style wins     */
            midipulse runningtime = 0;              /* reset time           */
            midipulse currenttime = 0;              /* adjusted by PPQN     */
            midishort seqnum = c_midishort_max;     /* either read or set   */
            midibyte status = 0;
            midibyte runningstatus = 0;
            midilong seqspec = 0;                   /* sequencer-specific   */
            bool done = false;                      /* done for each track  */
            sequence * sp = create_sequence(p);     /* create new sequence  */
            if (is_nullptr(sp))
            {
                set_error_dump("MIDI file parse: sequence allocation failed");
                return false;
            }
            sequence & s = *sp;                     /* references better    */
            while (! done)                          /* get events in track  */
            {
                event e;                            /* note-off, no channel */
                midilong len;                       /* important counter!   */
                midibyte d0, d1;                    /* the two data bytes   */
                midipulse delta = read_varinum();   /* time delta from prev */
                status = m_data[m_pos];             /* current event byte   */
                if (event::is_status(status))       /* is there a 0x80 bit? */
                {
                    skip(1);                                /* get to d0    */
                    if (event::is_system_common_msg(status))
                        runningstatus = 0;                  /* clear it     */
                    else if (! event::is_realtime_msg(status))
                        runningstatus = status;             /* log status   */
                }
                else
                {
                    /*
                     * Handle data values. If in running status, set that as
                     * status; the next value to be read is the d0 value.  If
                     * not running status, is this an error?
                     */

                    if (runningstatus > 0)      /* running status in force? */
                        status = runningstatus; /* yes, use running status  */
                }
                e.set_status_keep_channel(status);  /* set status, channel  */

                /*
                 *  See "PPQN" section in banner.
                 */

                runningtime += delta;           /* add in the time          */
                currenttime = runningtime;
                if (scaled())                   /* adjust time via ppqn     */
                    currenttime = midipulse(currenttime * ppqn_ratio());

                e.set_timestamp(currenttime);

                midibyte eventcode = event::mask_status(status);    /* F0 */
                midibyte channel = event::mask_channel(status);     /* 0F */
                switch (eventcode)
                {
                case EVENT_NOTE_OFF:                    /* 3-byte events    */
                case EVENT_NOTE_ON:
                case EVENT_AFTERTOUCH:
                case EVENT_CONTROL_CHANGE:
                case EVENT_PITCH_WHEEL:

                    d0 = read_byte();
                    d1 = read_byte();
                    if (event::is_note_off_velocity(eventcode, d1))
                        e.set_channel_status(EVENT_NOTE_OFF, channel);

                    e.set_data(d0, d1);               /* set data and add   */

                    /*
                     * s.append_event() doesn't sort events; sort after we
                     * get them all.  Also, it is kind of weird we change the
                     * channel for the whole sequence here.
                     */

                    s.append_event(e);                  /* does not sort    */
                    tentative_channel = channel;        /* log MIDI channel */
                    if (is_smf0)
                        m_smf0_splitter.increment(channel); /* count chan.  */
                    break;

                case EVENT_PROGRAM_CHANGE:              /* 1-data-byte event*/
                case EVENT_CHANNEL_PRESSURE:

                    d0 = read_byte();                   /* was data[0]      */
                    e.set_data(d0);                     /* set data and add */

                    /*
                     * s.append_event() doesn't sort events; they're sorted
                     * after we read them all.
                     */

                    s.append_event(e);                  /* does not sort    */
                    tentative_channel = channel;
                    if (is_smf0)
                        m_smf0_splitter.increment(channel); /* count chan.  */
                    break;

                case EVENT_MIDI_REALTIME:               /* 0xFn MIDI events */

                    if (status == EVENT_MIDI_META)      /* 0xFF             */
                    {
                        midibyte mtype = read_byte();   /* get meta type    */
                        len = read_varinum();           /* if 0 catch later */
                        switch (mtype)
                        {
                        case EVENT_META_SEQ_NUMBER:     /* FF 00 02 ss      */

                            if (! checklen(len, mtype))
                                return false;

                            seqnum = read_short();
                            break;

                        case EVENT_META_TRACK_NAME:     /* FF 03 len text   */

                            if (checklen(len, mtype))
                            {
                                int count = 0;
                                char trackname[c_trackname_max];
                                for (int i = 0; i < int(len); ++i)
                                {
                                    char ch = char(read_byte());
                                    if (count < c_trackname_max)
                                    {
                                        trackname[count] = ch;
                                        ++count;
                                    }
                                }
                                trackname[count] = '\0';
                                s.set_name(trackname);
                            }
                            else
                                return false;

                            break;

                        case EVENT_META_END_OF_TRACK:   /* FF 2F 00         */

                            s.set_length(currenttime, false);
                            s.zero_markers();
                            done = true;
                            break;

                        case EVENT_META_SET_TEMPO:      /* FF 51 03 tttttt  */

                            if (! checklen(len, mtype))
                                return false;

                            if (len == 3)
                            {
                                midibyte bt[4];         /* "Tempo events"   */
                                bt[0] = read_byte();                /* tt   */
                                bt[1] = read_byte();                /* tt   */
                                bt[2] = read_byte();                /* tt   */

                                double tt = tempo_us_from_bytes(bt);
                                if (tt > 0)
                                {
                                    if (track == 0)
                                    {
                                        midibpm bpm = bpm_from_tempo_us(tt);
                                        if (! gotfirst_bpm)
                                        {
                                            gotfirst_bpm = true;
                                            p.set_beats_per_minute(bpm);
                                            p.us_per_quarter_note(int(tt));
                                            s.us_per_quarter_note(int(tt));
                                        }
                                    }

                                    bool ok = e.append_meta_data(mtype, bt, 3);
                                    if (ok)
                                        s.append_event(e);
                                }
                            }
                            else
                                skip(len);              /* eat it           */
                            break;

                        case EVENT_META_TIME_SIGNATURE: /* FF 58 04 n d c b */

                            if (! checklen(len, mtype))
                                return false;

                            if ((len == 4) && ! timesig_set)
                            {
                                int bpm = int(read_byte());         // nn
                                int logbase2 = int(read_byte());    // dd
                                int cc = read_byte();               // cc
                                int bb = read_byte();               // bb
                                int bw = beat_power_of_2(logbase2);
                                s.set_beats_per_bar(bpm);
                                s.set_beat_width(bw);
                                s.clocks_per_metronome(cc);
                                s.set_32nds_per_quarter(bb);

#if defined SEQ66_USE_TRACK_0_AS_GLOBAL_TIME_SIG

                                /*
                                 * Should use c_perf_bp_mes and c_perf_bw
                                 * instead.
                                 */

                                if (track == 0)
                                {
                                    p.set_beats_per_bar(bpm);
                                    p.set_beat_width(bw);
                                    p.clocks_per_metronome(cc);
                                    p.set_32nds_per_quarter(bb);
                                }
#endif

                                midibyte bt[4];
                                bt[0] = midibyte(bpm);
                                bt[1] = midibyte(logbase2);
                                bt[2] = midibyte(cc);
                                bt[3] = midibyte(bb);

                                bool ok = e.append_meta_data(mtype, bt, 4);
                                if (ok)
                                    s.append_event(e);
                            }
                            else
                                skip(len);              /* eat it           */
                            break;

                        case EVENT_META_KEY_SIGNATURE:  /* FF 59 02 ss kk   */

                            if (len == 2)
                            {
                                midibyte bt[2];
                                bt[0] = read_byte();            /* #/b no.  */
                                bt[1] = read_byte();            /* min/maj  */

                                bool ok = e.append_meta_data(mtype, bt, 2);
                                if (ok)
                                    s.append_event(e);
                            }
                            else
                                skip(len);              /* eat it           */
                            break;

                        case EVENT_META_SEQSPEC:      /* FF F7 = SeqSpec    */

                            if (len > 4)              /* FF 7F len data     */
                            {
                                seqspec = read_long();
                                len -= 4;
                            }
                            else if (! checklen(len, mtype))
                                return false;

                            if (seqspec == c_midibus)
                            {
                                (void) s.set_midi_bus(read_byte());
                                --len;
                            }
                            else if (seqspec == c_midichannel)
                            {
                                midibyte channel = read_byte();
                                tentative_channel = channel;
                                --len;
                                if (is_smf0)
                                    m_smf0_splitter.increment(channel);
                            }
                            else if (seqspec == c_timesig)
                            {
                                timesig_set = true;
                                int bpm = int(read_byte());
                                int bw = int(read_byte());
                                s.set_beats_per_bar(bpm);
                                s.set_beat_width(bw);

                                /*
                                 * The usr() values replaced these.
                                 *
                                 *      p.set_beats_per_bar(bpm);
                                 *      p.set_beat_width(bw);
                                 */

                                len -= 2;
                            }
                            else if (seqspec == c_triggers)
                            {
                                int sz = trigger::datasize(c_triggers);
                                int num_triggers = len / sz;
                                for (int i = 0; i < num_triggers; ++i)
                                {
                                    add_old_trigger(s);
                                    len -= sz;
                                }
                            }
                            else if (seqspec == c_triggers_ex)
                            {
                                int sz = trigger::datasize(c_triggers_ex);
                                int num_triggers = len / sz;
                                midishort p = scaled() ? file_ppqn() : 0 ;
                                for (int i = 0; i < num_triggers; ++i)
                                {
                                    add_trigger(s, p, false);
                                    len -= sz;
                                }
                            }
                            else if (seqspec == c_trig_transpose)
                            {
                                int sz = trigger::datasize(c_trig_transpose);
                                int num_triggers = len / sz;
                                midishort p = scaled() ? file_ppqn() : 0 ;
                                for (int i = 0; i < num_triggers; ++i)
                                {
                                    add_trigger(s, p, true);
                                    len -= sz;
                                }
                            }
                            else if (seqspec == c_musickey)
                            {
                                s.musical_key(read_byte());
                                --len;
                            }
                            else if (seqspec == c_musicscale)
                            {
                                s.musical_scale(read_byte());
                                --len;
                            }
                            else if (seqspec == c_backsequence)
                            {
                                s.background_sequence(int(read_long()));
                                len -= 4;
                            }
                            else if (seqspec == c_transpose)
                            {
                                s.set_transposable(read_byte() != 0);
                                --len;
                            }
                            else if (seqspec == c_seq_color)
                            {
                                s.set_color(read_byte());
                                --len;
                            }
                            else if (seqspec == c_seq_loopcount)
                            {
                                s.loop_count_max(int(read_short()));
                                len -= 2;
                            }
                            else if (seqspec == c_mutegroups)
                            {
                                /* handled in parse_seqspec_track() */
                            }
                            else if (is_proptag(seqspec))
                            {
                                (void) set_error_dump
                                (
                                    "Unknown Seq66 SeqSpec, skipping",
                                    seqspec
                                );
                            }
                            else
                            {
                                /* will skip all other SeqSpecs */
                            }
                            skip(len);                  /* eat it           */
                            break;

                        /*
                         * Handled above: EVENT_META_TRACK_NAME
                         */

                        case EVENT_META_TEXT_EVENT:      /* FF 01 len text  */
                        case EVENT_META_COPYRIGHT:       /* FF 02 ...       */
                        case EVENT_META_INSTRUMENT:      /* FF 04 ...       */
                        case EVENT_META_LYRIC:           /* FF 05 ...       */
                        case EVENT_META_MARKER:          /* FF 06 ...       */
                        case EVENT_META_CUE_POINT:       /* FF 07 ...       */

                            if (checklen(len, mtype))
                            {
                                int count = 0;
                                midibyte mt[c_trackname_max];
                                for (int i = 0; i < int(len); ++i)
                                {
                                    char ch = char(read_byte());
                                    if (count < c_trackname_max)
                                    {
                                        mt[count] = ch;
                                        ++count;
                                    }
                                }
                                mt[count] = '\0';

                                bool ok = e.append_meta_data(mtype, mt, count);
                                if (ok)
                                    s.append_event(e);
                            }
                            else
                                return false;

                            break;

                        case EVENT_META_MIDI_CHANNEL:   /* FF 20 01 cc      */
                        case EVENT_META_MIDI_PORT:      /* FF 21 01 pp      */
                        case EVENT_META_SMPTE_OFFSET:   /* FF 54 03 t t t   */

                            (void) read_meta_data(s, e, mtype, len);
                            break;

                        default:

                            if (rc().verbose())
                            {
                                std::string m = "Illegal meta value skipped";
                                (void) set_error_dump(m);
                            }
                            break;
                        }
                    }
                    else if (status == EVENT_MIDI_SYSEX)    /* 0xF0 */
                    {
                        /*
                         * Some files do not properly encode SysEx messages;
                         * see the function banner for notes.
                         */

                        midibyte check = read_byte();
                        if (is_sysex_special_id(check))
                        {
                            /*
                             * TMI: "SysEx ID byte = 7D to 7F");
                             */
                        }
                        else                            /* handle normally  */
                        {
                            --m_pos;                    /* put byte back    */
                            len = read_varinum();       /* sysex            */
#if defined SEQ66_USE_SYSEX_PROCESSING
                            int bcount = 0;
                            while (len--)
                            {
                                midibyte b = read_byte();
                                ++bcount;
                                if (! e.append_sysex(b)) /* SysEx end byte? */
                                    break;
                            }
                            skip(len);                  /* eat it           */
#else
                            skip(len);                  /* eat it           */
                            if (m_data[m_pos-1] != 0xF7)
                            {
                                std::string m = "SysEx terminator F7 not found";
                                (void) set_error_dump(m);
                            }
#endif
                        }
                    }
                    else
                    {
                        (void) set_error_dump
                        (
                            "Unexpected meta code", midilong(status)
                        );
                    }
                    break;

                default:

                    /*
                     * Some files (e.g. 2rock.mid, which has "00 24 40"
                     * hanging out there all alone at offset 0xba) have junk
                     * in them.
                     */

                    (void) set_error_dump
                    (
                        "Unsupported MIDI event", midilong(status)
                    );
                    return true;        /* allow further processing */
                    break;
                }
            }                          /* while not done loading Trk chunk */

            /*
             * Sequence has been filled, add it to the performance or SMF 0
             * splitter.  If there was no sequence number embedded in the
             * track, use the for-loop track number.  It's not fool-proof.
             * "If the ID numbers are omitted, the sequences' locations in
             * order in the file are used as defaults."
             */

            if (seqnum == c_midishort_max)
                seqnum = track;

            if (seqnum < c_prop_seq_number)
            {
                s.set_midi_channel(tentative_channel);
                if (! is_null_buss(buss_override))
                    (void) s.set_midi_bus(buss_override);

                if (is_smf0)
                    (void) m_smf0_splitter.log_main_sequence(s, seqnum);
                else
                    finalize_sequence(p, s, seqnum, screenset);
            }
        }
        else
        {
            if (track > 0)                              /* non-fatal later  */
            {
                (void) set_error_dump("Unknown MIDI track ID, skipping...", ID);
            }
            else                                        /* fatal in 1st one */
            {
                result = set_error_dump("First track unsupported track ID", ID);
                break;
            }
            skip(TrackLength);
        }
    }                                                   /* for each track   */
    return result;
}

sequence *
midifile::create_sequence (performer & p)
{
    sequence * result = new sequence(ppqn());
    if (not_nullptr(result))
    {
        mastermidibus * masterbus = p.master_bus();
        if (not_nullptr(masterbus))
            result->set_master_midi_bus(masterbus);     /* set master buss  */
    }
    return result;
}

/**
 *  Sets some MIDI parameters for a new sequence, then calls
 *  performer::install_sequence() to add the sequence pointer to the set-manager
 *  and hook it into the master bus.
 */

bool
midifile::finalize_sequence
(
    performer & p,
    sequence & s,
    int seqnum,
    int screenset
)
{
    int preferred_seqnum = seqnum + screenset * p.screenset_size();
    return p.install_sequence(&s, preferred_seqnum, true);
}

/**
 *  Parse the proprietary header for sequencer-specific data.  The new format
 *  creates a final track chunk, starting with "MTrk".  Then comes the
 *  delta-time (here, 0), and the event.  An event is a MIDI event, a SysEx
 *  event, or a Meta event.
 *
 *  A MIDI Sequencer Specific meta message includes either a delta time or
 *  absolute time, and the MIDI Sequencer Specific event encoded as
 *  follows:
 *
\verbatim
        0x00 0xFF 0x7F length data
\endverbatim
 *
 *  For convenience, this function first checks the amount of file data left.
 *  If enough, then it reads a long value.  If the value starts with 0x00 0xFF
 *  0x7F, then that is a SeqSpec event, which signals usage of the new
 *  Seq66 "proprietary" format.  Otherwise, it is probably the old
 *  format, and the long value is a control tag (0x242400nn), which can be
 *  returned immedidately.
 *
 *  If it is the new format, we back up to the FF, then get the next byte,
 *  which should be a 7F.  If so, then we read the length (a variable
 *  length value) of the data, and then read the long value, which should
 *  be the control tag, which, again, is returned by this function.
 *
 * \note
 *      Most sequencers seem to be tolerant of both the lack of an "MTrk"
 *      marker and of the presence of an unwrapped control tag, and so can
 *      handle both the old and new formats of the final proprietary track.
 *
 * \param file_size
 *      The size of the data file.  This value is compared against the
 *      member m_pos (the position inside m_data[]), to make sure there is
 *      enough data left to process.
 *
 * \return
 *      Returns the control-tag value found.  These are the values, such as
 *      c_midichannel, found in the midi_vector_base module, that indicate the
 *      type of sequencer-specific data that comes next.  If there is not
 *      enough data to process, then 0 is returned.
 */

midilong
midifile::parse_seqspec_header (int file_size)
{
    midilong result = 0;
    if ((file_size - m_pos) > int(sizeof(midilong)))
    {
        result = read_long();                   /* status (new), or C_tag   */
        midibyte status = (result & 0x00FF0000) >> 16;      /* 2-byte shift */
        if (status == EVENT_MIDI_META)          /* 0xFF meta marker         */
        {
            skip(-2);                           /* back up to meta type     */
            midibyte type = read_byte();        /* get meta type            */
            if (type == EVENT_META_SEQSPEC)     /* 0x7F event marker     */
            {
                (void) read_varinum();          /* prop section length      */
                result = read_long();           /* control tag              */
            }
            else
            {
                msgprintf
                (
                    msglevel::error,
                    "Unexpected meta type 0x%x offset ~0x%lx",
                    int(type), long(m_pos)
                );
            }
        }
    }
    return result;
}

/**
 *  After all of the conventional MIDI tracks are read, we're now at the
 *  "proprietary" Seq24 data section, which describes the various features
 *  that Seq24 supports.  It consists of series of tags, layed out in the
 *  midi_vector_base.hpp header file (search for c_mutegroups, for example).
 *
 *  The format is (1) tag ID; (2) length of data; (3) the data.
 *
 *  First, we separated out this function for a little more clarity.  Then we
 *  added code to handle reading both the legacy Seq24 format and the new,
 *  MIDI-compliant format.  Note that even the new format is not quite
 *  correct, since it doesn't handle a MIDI manufacturer's ID, making it a
 *  single byte that is part of the data.  But it does have the "MTrk" marker
 *  and track name, so that must be processed for the new format.
 *
 *  Now, in our "midicvt" project, we have a test MIDI file,
 *  b4uacuse-non-mtrk.midi that is good, except for having a tag "MUnk"
 *  instead of "MTrk".  We should consider being more permissive, if possible.
 *  Otherwise, though, the only penality is that the "proprietary" chunk is
 *  completely skipped.
 *
 * Extra precision BPM:
 *
 *  Based on a request for two decimals of precision in beats-per-minute, we
 *  now save a scaled version of BPM.  Our supported range of BPM is 2.0 to
 *  600.0.  If this range is encountered, the value is read as is.  If greater
 *  than this range (actually, we use 999 as the limit), then we divide the
 *  number by 1000 to get the actual BPM, which can thus have more precision
 *  than the old integer value allowed.  Obviously, when saving, we will
 *  multiply by 1000 to encode the BPM.
 *
 * \param p
 *      The performance object that is being set via the incoming MIDI file.
 *
 * \param file_size
 *      The file size as determined in the parse() function.
 *
 *  There are also implicit parameters, with the m_pos member
 *  variable.
 */

bool
midifile::parse_seqspec_track (performer & p, int file_size)
{
    bool result = true;
    midilong ID = read_long();                      /* Get ID + Length      */
    if (ID == c_prop_chunk_tag)                     /* magic number 'MTrk'  */
    {
        midilong tracklength = read_long();
        if (tracklength > 0)
        {
            /*
             * The old number, 0x7777, is now 0x3FFF.  We don't want to
             * startle people, so we will silently ignore (and replace upon
             * saving) this number.
             */

            int sn = read_seq_number();
            bool ok = (sn == c_prop_seq_number) || (sn == c_prop_seq_number_old);
            if (ok)                                 /* sanity check         */
            {
                std::string trackname = read_track_name();
                result = ! trackname.empty();

                /*
                 * This "sanity check" is probably a bit much.  It causes
                 * errors in Sequencer24 tracks, which are otherwise fine
                 * to scan in the new format.  Let the "MTrk" and 0x3FFF
                 * markers be enough.
                 *
                 * if (trackname != c_prop_track_name)
                 *     result = false;
                 */
            }
            else if (sn == (-1))
            {
                m_error_is_fatal = false;
                result = set_error_dump
                (
                    "No sequence number in SeqSpec track, extra data"
                );
            }
            else
                result = set_error("Unexpected sequence number, SeqSpec track");
        }
    }
    else
        skip(-4);                                   /* unread the "ID code" */

    if (result)
        result = prop_header_loop(p, file_size);

    return result;
}

/**
 *  This section used to depend on the ordering and presence of all supported
 *  SeqSpecs, and hence was kind of brittle.  Now we loop here and use a
 *  switch-statement to figure out which code to execute.
 *
 *  Seq24 would store the MIDI control setting in the MIDI file.  While this
 *  could be a useful feature, it seems a bit confusing, since the
 *  user/musician will more likely define those controls for his set of
 *  equipment to apply to all songs.
 *
 *  Furthermore, we would need to load these control settings into a
 *  midicontrolin (see ctrl/midicontrolin modules).  And, lastly, Seq24 never
 *  wrote these controls to the file.  It merely wrote the c_midictrl code,
 *  followed by a long 0.  For now, we are going to evade this functionality.
 *  We will continue to write this section, and try to read it, but expect it
 *  to be empty.
 *
 * Track-specific SeqSpecs handled in parse_smf_1():
 *
 *      c_midibus          c_timesig         c_midichannel    c_musickey *
 *      c_musicscale *     c_backsequence *  c_transpose *    c_seq_color
 *      c_seq_loopcount   c_triggers       c_triggers_ex      c_trig_transpose
 *
 * Global SeqSpecs handled here:
 *
 *      c_midictrl         c_midiclocks      c_notes          c_bpmtag
 *      c_mutegroups       c_musickey *      c_musicscale *
 *      c_backsequence *   c_perf_bp_mes     c_perf_bw        c_tempo_map !
 *      c_reserved_1 !     c_reserved_2 !    c_tempo_track
 *
 * Not handled:
 *
 *      c_gap_A to _F      c_reserved_3      c_reserved_4     c_seq_edit_mode
 */

bool
midifile::prop_header_loop (performer & p, int file_size)
{
    bool ok = true;
    while (ok)
    {
        midilong seqspec = parse_seqspec_header(file_size);
        ok = seqspec > 0;
        if (ok)
        {
            switch (seqspec)
            {
            case c_midictrl:        ok = parse_c_midictrl(p);       break;
            case c_midiclocks:      ok = parse_c_midiclocks(p);     break;
            case c_notes:           ok = parse_c_notes(p);          break;
            case c_bpmtag:          ok = parse_c_bpmtag(p);         break;
            case c_mutegroups:      (void) parse_c_mutegroups(p);   break;
            case c_musickey:        ok = parse_c_musickey();        break;
            case c_musicscale:      ok = parse_c_musicscale();      break;
            case c_backsequence:    ok = parse_c_backsequence();    break;
            case c_perf_bp_mes:     ok = parse_c_perf_bp_mes(p);    break;
            case c_perf_bw:         ok = parse_c_perf_bw(p);        break;
            case c_tempo_track:     ok = parse_c_tempo_track();     break;
#if defined USE_SEQ32_SEQSPECS      /* stazed features not implemented */
            case c_tempo_map:
            case c_reserved_1:
            case c_reserved_2:
                break;
#endif
            default:
                break;
            }
        }
    }
    return true;
}

/*
 * Some old code wrote some bad files, we need to work around that and fix it.
 * The value of max_sequence() is generally 1024. Note that we no longer
 * support setting up MIDI control in a song.  Too mysterious, and the 'ctrl'
 * file is far more powerful (and handles keystrokes at the same time).
 */

bool
midifile::parse_c_midictrl (performer & /* p*/)
{
    int ctrls = int(read_long());
    if (ctrls > usr().max_sequence())
    {
        skip(-4);
        (void) set_error_dump
        (
            "Bad MIDI-control sequence count, fixing.\n"
            "Please save the file now!",
            midilong(ctrls)
        );
        ctrls = midilong(read_byte());
    }
    midibyte a[6];
    for (int i = 0; i < ctrls; ++i)
    {
        read_byte_array(a, 6);
        read_byte_array(a, 6);
        read_byte_array(a, 6);
    }
    return true;
}

/*
 * Some old code wrote some bad files, we work around and fix it.
 */

bool
midifile::parse_c_midiclocks (performer & p)
{
    int busscount = int(read_long());
    mastermidibus * masterbus = p.master_bus();
    if (not_nullptr(masterbus))
    {
        for (int buss = 0; buss < busscount; ++buss)
        {
            bussbyte clocktype = read_byte();
            masterbus->set_clock
            (
                bussbyte(buss), static_cast<e_clock>(clocktype)
            );
        }
    }
    return true;
}

/*
 * The default number of sets is c_max_sets = 32.  This is also the initial
 * minimum number of screen-set names, though some can be empty.  More than 32
 * sets can be supported, though that claim is currently untested.  The
 * highest set number is determined by the highest pattern number; the
 * setmaster::add_set() function creates a new set when a pattern number falls
 * outside the boundaries of the existing sets.
 */

bool
midifile::parse_c_notes (performer & p)
{
    midishort screen_sets = read_short();
    for (midishort x = 0; x < screen_sets; ++x)
    {
        midishort len = read_short();               /* string size  */
        std::string notess;
        for (midishort i = 0; i < len; ++i)
            notess += read_byte();                  /* unsigned!    */

        p.screenset_name(x, notess, true);          /* load time    */
    }
    return true;
}

/*
 * Should check here for a conflict between the Tempo meta event and this
 * tag's value.  NOT YET.  Also, as of 2017-03-22, we want to be able to
 * handle two decimal points of precision in BPM.  See the prop_header_loop()
 * function banner for a discussion.
 */

bool
midifile::parse_c_bpmtag (performer & p)
{
    midilong longbpm = read_long();
    midibpm bpm = usr().unscaled_bpm(longbpm);
    p.set_beats_per_minute(bpm);                    /* 2nd setter   */
    return true;
}

/**
 *  Read in the mute group information.  If the length of the mute group
 *  section is 0, then this file is a Seq42 file, and we ignore the section.
 *  (Thanks to Stazed for that catch!)
 *
 *  Updated mute-groups parsing that supports the old style "32 x 32"
 *  mutegroups, and more variable setup.  Lots to do yet!
 *
 *  Also need to check for any mutes being present.
 *
 *  What about rows & columns?  Ultimately, the set-size must match that
 *  specified by the application's user-interface as must the rows and
 *  columns.
 *
 * New:  We write the group name.  So the new format of mute-groups is:
 *
 *      -#  Split byte count for the number of groups and the size of each
 *          group. Long, 4 bytes.
 *      -#  Group number.  Byte value.
 *      -#  Mute-group bit values, 1 byte each, and group-size of them.
 *      -#  Optional:  The mute-group name in double quotes.
 */

bool
midifile::parse_c_mutegroups (performer & p)
{
    mutegroups & mutes = p.mutes();
    bool result = mutes.group_load_from_midi();
    if (result)
        result = ! mutes.loaded_from_mutes();       /* loaded from config?  */

    if (result)
    {
        unsigned groupcount, groupsize;
        long len = long(read_split_long(groupcount, groupsize));
        if (len > 0)
        {
            bool legacyformat = len == c_legacy_mute_group;
            p.mutes().clear();                      /* makes it empty       */
            if (legacyformat)
            {
                mutes.legacy_mutes(true);
                for (unsigned g = 0; g < groupcount; ++g)
                {
                    midibooleans mutebits;
                    midilong group = read_long();
                    for (unsigned s = 0; s < groupsize; ++s)
                    {
                        midilong gmutestate = read_long();  /* long bit !?  */
                        bool status = gmutestate != 0;
                        mutebits.push_back(midibool(status));
                    }
                    if (mutes.load(group, mutebits))
                    {
                        /*
                         * Related to issue #87.
                         */

                        if (mutebits.size() != size_t(p.group_count()))
                        {
                            mutebits = fix_midibooleans
                            (
                                mutebits, p.group_count()
                            );
                            rc().auto_mutes_save(true);
                        }
                    }
                    else
                        break;                              /* a duplicate? */
                }
            }
            else
            {
                std::string gname;
                mutes.legacy_mutes(false);
                for (unsigned g = 0; g < groupcount; ++g)
                {
                    midibooleans mutebits;
                    midilong group = read_byte();
                    gname.clear();
                    for (unsigned s = 0; s < groupsize; ++s)
                    {
                        midibyte gmutestate = read_byte();  /* byte for bit */
                        bool status = gmutestate != 0;
                        mutebits.push_back(midibool(status));
                    }
                    char letter = (char) read_byte();
                    if (letter == '"')                      /* next a quote? */
                    {
                        for (;;)
                        {
                            char letter = (char) read_byte();
                            if (letter == '"')
                                break;
                            else
                                gname += letter;
                        }
                    }
                    else
                        --m_pos;                            /* put it back  */

                    if (mutes.load(group, mutebits))
                    {
                        /*
                         * Related to issue #87.
                         */

                        mutes.group_name(group, gname);
                        if (mutebits.size() != size_t(p.group_count()))
                        {
                            mutebits = fix_midibooleans
                            (
                                mutebits, p.group_count()
                            );
                            rc().auto_mutes_save(true);
                        }
                    }
                    else
                        break;                              /* a duplicate  */
                }
            }
        }
    }
    return result;
}

/*
 * We let Seq66 read this new stuff even the global-background sequence is in
 * force.  That flag affects only the writing of the MIDI file, not the
 * reading.
 */

bool
midifile::parse_c_musickey ()
{
    int key = int(read_byte());
    usr().seqedit_key(key);
    return true;
}

bool
midifile::parse_c_musicscale ()
{
    int scale = int(read_byte());
    usr().seqedit_scale(scale);
    return true;
}

bool
midifile::parse_c_backsequence ()
{
    int seqnum = int(read_long());
    usr().seqedit_bgsequence(seqnum);
    return true;
}

/*
 *  Store the beats/measure and beat-width values from the perfedit window.
 *
 *  We should also calculate:
 *
 *      performer::clocks_per_metronome(cc);
 *      performer::set_32nds_per_quarter(bb);
 */

bool
midifile::parse_c_perf_bp_mes (performer & p)
{
    int bpmes = int(read_long());
    p.set_beats_per_bar(bpmes);
    return true;
}

bool
midifile::parse_c_perf_bw (performer & p)
{
    int bw = int(read_long());
    p.set_beat_width(bw);
    return true;
}

/*
 * If this value is present and greater than 0, set it into the performance.
 * It might override the value specified in the "rc" configuration file.
 */

bool
midifile::parse_c_tempo_track ()
{
    int tempotrack = int(read_long());
    if (tempotrack >= 0)
        rc().tempo_track_number(tempotrack);

    return true;
}

/**
 *  For each groups in the mute-groups, write the status bits to the
 *  c_mutegroups SeqSpec.
 *
 *  The mutegroups class has rows and columns for each group, but doesn't have
 *  a way to iterate through all the groups.
 */

bool
midifile::write_c_mutegroups (const performer & p)
{
    const mutegroups & mutes = p.mutes();
    bool result = mutes.saveable_to_midi();
    if (result)
    {
        if (rc().save_old_mutes())
        {
            for (const auto & stz : mutes.list())
            {
                int groupnumber = stz.first;
                const mutegroup & m = stz.second;
                midibooleans mutebits = m.get();
                result = mutebits.size() > 0;
                if (result)
                {
                    write_long(groupnumber);
                    for (auto mutestatus : mutebits)
                        write_long(bool(mutestatus) ? 1 : 0);   /* waste!   */
                }
                else
                    break;
            }
        }
        else
        {
            for (const auto & stz : mutes.list())
            {
                int groupnumber = stz.first;
                const mutegroup & m = stz.second;
                midibooleans mutebits = m.get();
                result = mutebits.size() > 0;
                if (result)
                {
                    write_byte(groupnumber);
                    for (auto mutestatus : mutebits)
                        write_byte(bool(mutestatus) ? 1 : 0);   /* better!  */

                    std::string gname = m.name();
                    if (! gname.empty())
                    {
                        write_byte(midibyte('"'));
                        for (auto ch : gname)
                            write_byte(midibyte(ch));

                        write_byte(midibyte('"'));
                    }
                }
                else
                    break;
            }
        }
    }
    return result;
}

/**
 *  Writes 4 bytes, each extracted from the long value and shifted rightward
 *  down to byte size, using the write_byte() function.
 *
 * \param x
 *      The long value to be written to the MIDI file.
 */

void
midifile::write_long (midilong x)
{
    write_byte((x & 0xFF000000) >> 24);
    write_byte((x & 0x00FF0000) >> 16);
    write_byte((x & 0x0000FF00) >> 8);
    write_byte((x & 0x000000FF));
}

/**
 *  Writes 4 bytes in the style readable by read_split_long().  If the values
 *  are both 32 (note that 32 x 32 = 1024), then 1024, the legacy value, is
 *  written out in the normal way, via write_long().  Otherwise, the highbytes
 *  and lowbytes values are each written out in two bytes each, with the most
 *  significant byte written first.
 *
 * \param [out] highbytes
 *      Provides the value of the most significant 2 bytes of the four-byte
 *      ("long") value.  The most significant bytes are masked out; the values
 *      are limited to range from 0 to 65535 = 0xFFFF, a short value. For
 *      mute-groups, this is the number of mute-groups.
 *
 * \param [out] lowbytes
 *      Provides the value of the least significant 2 bytes of the four-byte
 *      ("long") value.  The most significant bytes are masked out; the values
 *      are limited to the same range as the \a highbytes parameter. For
 *      mute-groups, this is the number of patterns in the mute-groups.
 *
 * \param oldstyle
 *      If true (the default is false), then just write a long value of 1024.
 *      Otherwise, the bytes are masked, shifted, and written.
 */

void
midifile::write_split_long (unsigned highbytes, unsigned lowbytes, bool oldstyle)
{
    if (oldstyle)
    {
        write_long(c_legacy_mute_group); /* 1024, long-standing Seq24 value */
    }
    else
    {
        write_byte((highbytes & 0x0000FF00) >> 8);
        write_byte(highbytes & 0x000000FF);
        write_byte((lowbytes & 0x0000FF00) >> 8);
        write_byte(lowbytes & 0x000000FF);
    }
}

/**
 *  Writes 3 bytes, each extracted from the long value and shifted rightward
 *  down to byte size, using the write_byte() function.
 *
 *  This function is kind of the reverse of tempo_us_to_bytes() defined in the
 *  calculations.cpp module.
 *
 * \warning
 *      This code looks endian-dependent.
 *
 * \param x
 *      The long value to be written to the MIDI file.
 */

void
midifile::write_triple (midilong x)
{
    write_byte((x & 0x00FF0000) >> 16);
    write_byte((x & 0x0000FF00) >> 8);
    write_byte((x & 0x000000FF));
}

/**
 *  Writes 2 bytes, each extracted from the long value and shifted rightward
 *  down to byte size, using the write_byte() function.
 *
 * \warning
 *      This code looks endian-dependent.
 *
 * \param x
 *      The short value to be written to the MIDI file.
 */

void
midifile::write_short (midishort x)
{
    write_byte((x & 0xFF00) >> 8);
    write_byte((x & 0x00FF));
}

/**
 *  Writes a MIDI Variable-Length Value (VLV), which has a variable number
 *  of bytes.
 *
 *  A MIDI file Variable Length Value is stored in bytes. Each byte has
 *  two parts: 7 bits of data and 1 continuation bit. The highest-order
 *  bit is set to 1 if there is another byte of the number to follow. The
 *  highest-order bit is set to 0 if this byte is the last byte in the
 *  VLV.
 *
 *  To recreate a number represented by a VLV, first you remove the
 *  continuation bit and then concatenate the leftover bits into a single
 *  number.
 *
 *  To generate a VLV from a given number, break the number up into 7 bit
 *  units and then apply the correct continuation bit to each byte.
 *  Basically this is a base-128 system where the digits range from 0 to 7F.
 *
 *  In theory, you could have a very long VLV number which was quite
 *  large; however, in the standard MIDI file specification, the maximum
 *  length of a VLV value is 5 bytes, and the number it represents can not
 *  be larger than 4 bytes.
 *
 *  Here are some common cases:
 *
 *      -   Numbers between 0 and 127 are represented by a single byte:
 *          0x00 to 7F.
 *      -   0x80 is represented as 0x81 00.  The first number is
 *          10000001, with the left bit being the continuation bit.
 *          The rest of the number is multiplied by 128, and the second byte
 *          (0) is added to that. So 0x82 would be 0x81 0x01
 *      -   The largest 2-byte MIDI value (e.g. a sequence number) is
 *          0xFF 7F, which is 127 * 128 + 127 = 16383 = 0x3FFF.
 *      -   The largest 3-byte MIDI value is 0xFF FF 7F = 2097151 = 0x1FFFFF.
 *      -   The largest number, 4 bytes, is 0xFF FF FF 7F = 536870911 =
 *          0xFFFFFFF.
 *
 *  Also see the varinum_size() and midi_vector_base::add_variable() functions.
 *
 * \param value
 *      The long value to be encoded as a MIDI varinum, and written to the
 *      MIDI file.
 */

void
midifile::write_varinum (midilong v)
{
    midilong buffer = v & 0x7f;
    while ((v >>= 7) > 0)
    {
        buffer <<= 8;
        buffer |= 0x80;
        buffer += (v & 0x7f);
    }
    for (;;)
    {
        write_byte(midibyte(buffer & 0xff));
        if (buffer & 0x80)                      /* continuation bit?        */
            buffer >>= 8;                       /* yes, get next MSB        */
        else
            break;                              /* no, we are done          */
    }
}

/**
 *  Calculates the length of a variable length value (VLV).  This function is
 *  needed when calculating the length of a track.  Note that it handles
 *  only the following situations:
 *
 *      https://en.wikipedia.org/wiki/Variable-length_quantity
 *
 *  This restriction allows the calculation to be simple and fast.
 *
\verbatim
       1 byte:  0x00 to 0x7F
       2 bytes: 0x80 to 0x3FFF
       3 bytes: 0x4000 to 0x001FFFFF
       4 bytes: 0x200000 to 0x0FFFFFFF
\endverbatim
 *
 * \param len
 *      The long value whose length, when encoded as a MIDI varinum, is to be
 *      found.
 *
 * \return
 *      Returns values as noted above.  Anything beyond that range returns
 *      0.
 */

long
midifile::varinum_size (long len) const
{
    int result = 0;
    if (len >= 0x00 && len < 0x80)
        result = 1;
    else if (len >= 0x80 && len < 0x4000)
        result = 2;
    else if (len >= 0x4000 && len < 0x200000)
        result = 3;
    else if (len >= 0x200000 && len < 0x10000000)
        result = 4;

    return result;
}

/**
 * We want to write:
 *
 *  -   0x4D54726B.
 *      The track tag "MTrk".  The MIDI spec requires that software can skip
 *      over non-standard chunks. "Prop"?  Would require a fix to midicvt.
 *  -   0xaabbccdd.
 *      The length of the track.  This needs to be calculated somehow.
 *  -   0x00.  A zero delta time.
 *  -   0x7f7f.  Sequence number, a special value, well out of normal range.
 *  -   The name of the track:
 *      -   "Seq24-Spec"
 *      -   "Seq66-S"
 *
 *   Then follows the proprietary/SeqSpec data, written in the normal manner.
 *   Finally, tack on the track-end meta-event.
 *
 *      Components of final track size:
 *
 *          -# Delta time.  1 byte, always 0x00.
 *          -# Sequence number.  5 bytes.  OPTIONAL.  We won't write it.
 *          -# Track name. 3 + 10 or 3 + 15
 *          -# Series of proprietary/SeqSpec specs:
 *             -# Prop header:
 *                -# If legacy [obsolete] format, 4 bytes.
 *                -# Otherwise, 2 bytes + varinum_size(length) + 4 bytes.
 *                -# Length of the prop data.
 *          -# Track End. 3 bytes.
 *
 * \param numtracks
 *      The number of tracks to be written.  For SMF 0 this should be 1.
 *
 * \param smfformat
 *      The SMF value to write.  Defaults to 1.
 */

bool
midifile::write_header (int numtracks, int smfformat)
{
    write_long(0x4D546864);                 /* MIDI Format 1 header MThd    */
    write_long(6);                          /* Length of the header         */
    write_short(smfformat);                 /* MIDI Format 1 (or 0)         */
    write_short(numtracks);                 /* number of tracks             */
    write_short(m_ppqn);                    /* parts per quarter note       */
    return numtracks > 0;
}

#if defined SEQ66_USE_WRITE_START_TEMPO

/**
 *  Writes the initial or only tempo, occurring at the beginning of a MIDI
 *  song.  Compare this function to midi_vector_base::fill_time_sig_and_tempo().
 *
 * \param start_tempo
 *      The beginning tempo value.
 */

void
midifile::write_start_tempo (midibpm start_tempo)
{
    write_byte(0x00);                       /* delta time at beginning      */
    write_short(0xFF51);
    write_byte(0x03);                       /* message length, must be 3    */
    write_triple(midilong(60000000.0 / start_tempo));
}

#endif  // SEQ66_USE_WRITE_START_TEMPO

#if defined SEQ66_USE_WRITE_TIME_SIG

/**
 *  Writes the main time signature, in a more simplistic manner than
 *  midi_vector_base::fill_time_sig_and_tempo().
 *
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
midifile::write_time_sig (int beatsperbar, int beatwidth)
{
    write_byte(0);                          /* delta time at beginning      */
    write_byte(EVENT_MIDI_META);            /* 0xFF meta marker             */
    write_byte(EVENT_META_TIME_SIGNATURE);  /* 0x58                         */
    write_byte(4);                          /* the message length           */
    write_byte(beatsperbar);                /* nn                           */
    write_byte(beat_log2(beatwidth));       /* dd                           */
    write_short(0x1808);                    /* cc bb                        */
}

#endif  // SEQ66_USE_WRITE_TIME_SIG

/**
 *  Writes a "proprietary" (SeqSpec) Seq24 footer header in the new
 *  MIDI-compliant format.  This function does not write the data.  It
 *  replaces calls such as "write_long(c_midichannel)" in the proprietary
 *  secton of write().
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
 * \param data_length
 *      The amount of data that will be written.  This parameter does not
 *      count the length of the header itself.
 */

void
midifile::write_seqspec_header (midilong control_tag, long len)
{
    write_byte(0);                      /* delta time                   */
    write_byte(EVENT_MIDI_META);        /* 0xFF meta marker             */
    write_byte(EVENT_META_SEQSPEC);     /* 0x7F sequencer-specific mark */
    write_varinum(len + 4);             /* data + sizeof(control_tag);  */
    write_long(control_tag);            /* use legacy output call       */
}

/**
 *  Write a MIDI track to the file.
 *
 * \param lst
 *      The MIDI vector containing the events.
 */

void
midifile::write_track (const midi_vector & lst)
{
    midilong tracksize = midilong(lst.size());
    write_long(c_mtrk_tag);                 /* magic number 'MTrk'          */
    write_long(tracksize);
    while (! lst.done())                    /* write the track data         */
        write_byte(lst.get());
}

/**
 *  Calculates the size of a proprietary item, as written by the
 *  write_seqspec_header() function, plus whatever is called to write the
 *  data.  If using the new format, the length includes the sum of
 *  sequencer-specific tag (0xFF 0x7F) and the size of the variable-length
 *  value.  Then, for the new format, 4 bytes are added for the Seq24 MIDI
 *  control value, and then the data length is added.
 *
 * \param data_length
 *      Provides the data length value to be encoded.
 *
 * \return
 *      Returns the length of the item size, including the delta time, meta
 *      bytes, length byes, the control tag, and the data-length itself.
 */

long
midifile::prop_item_size (long data_length) const
{
    long result = 0;
    int len = data_length + 4;              /* data + sizeof(control_tag);  */
    result += 3;                            /* count delta time, meta bytes */
    result += varinum_size(len);            /* count the length bytes       */
    result += 4;                            /* write_long(control_tag);     */
    result += data_length;                  /* add the data size itself     */
    return result;
}

/**
 *  Write the whole MIDI data and Seq24 information out to the file.
 *  Also see the write_song() function, for exporting to standard MIDI.
 *
 *  Seq66 sometimes reverses the order of some events, due to popping
 *  from its container.  Not an issue, but can make a file slightly different
 *  for no reason.
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
 * \param smfformat
 *      Defaults to 1.  Can be set to 0 for writing an SMF 0 file.
 *
 * \return
 *      Returns true if the write operations succeeded.  If false is returned,
 *      then m_error_message will contain a description of the error.
 */

bool
midifile::write (performer & p, bool doseqspec)
{
    automutex locker(m_mutex);
    bool result = usr().is_ppqn_valid(m_ppqn);
    m_error_message.clear();
    if (! result)
        m_error_message = "Invalid PPQN for MIDI file to write.";

    if (result)
    {
        int numtracks = 0;
        int sequencehigh = p.sequence_high();
        if (rc().verbose())
        {
            infoprintf("Highest track is %d", sequencehigh - 1);
        }
        for (int i = 0; i < sequencehigh; ++i)
        {
            if (p.is_seq_active(i))
                ++numtracks;             /* count number of active tracks   */
        }
        result = numtracks > 0;
        if (result)
        {
            int smfformat = p.smf_format();
            bool result = write_header(numtracks, smfformat);
            if (result)
            {
                std::string temp = "Writing ";
                temp += doseqspec ? "Seq66" : "Normal" ;
                temp += " SMF ";
                temp += std::to_string(smfformat);
                temp += " MIDI file ";
                temp += std::to_string(m_ppqn);
                temp += " PPQN";
                file_message(temp, m_name);
            }
            else
                m_error_message = "Failed to write header to MIDI file.";
        }
        else
            m_error_message = "No patterns/tracks to write.";
    }

    /*
     * Write out the active tracks.
     * Note that we don't need to check the sequence pointer.
     */

    if (result)
    {
        for (int track = 0; track < p.sequence_high(); ++track)
        {
            if (p.is_seq_active(track))
            {
                seq::pointer s = p.get_sequence(track);
                if (s)
                {
                    sequence & seq = *s;
                    midi_vector lst(seq);

                    /*
                     * midi_vector_base::fill() also handles the time-signature
                     * and tempo meta events, if they are not part of the
                     * file's MIDI data.  All the events are put into the
                     * container, and then the container's bytes are written
                     * out below.
                     */

                    lst.fill(track, p, doseqspec);
                    write_track(lst);
                }
            }
        }
    }
    if (result && doseqspec)
    {
        result = write_seqspec_track(p);
        if (! result)
            m_error_message = "Could not write SeqSpec track.";
    }
    if (result)
    {
        std::ofstream file
        (
            m_name.c_str(), std::ios::out | std::ios::binary | std::ios::trunc
        );
        if (file.is_open())
        {
            char file_buffer[c_midi_line_max];      /* enable bufferization */
            file.rdbuf()->pubsetbuf(file_buffer, sizeof file_buffer);
            for (auto c : m_char_list)              /* list of midibytes    */
            {
                char kc = char(c);
                file.write(&kc, 1);
                if (file.fail())
                {
                    m_error_message = "Error writing byte.";
                    result = false;
                }
            }
            m_char_list.clear();
        }
        else
        {
            m_error_message = "Failed to open MIDI file for writing.";
            result = false;
        }
    }
    if (result)
        p.unmodify();               /* it worked, tell performer about it   */

    return result;
}

/**
 *  Write the whole MIDI data and Seq24 information out to a MIDI file, writing
 *  out patterns based on their song/performance information (triggers) and
 *  ignoring any patterns that are muted.
 *
 *  We get the number of active tracks, and we don't count tracks with no
 *  triggers, or tracks that are muted.
 *
 *  This function, write_song(), was not included in Seq24 because it
 *  Seq24 writes standard MIDI files (with SeqSpec information that a
 *  decent sequencer should ignore).  But we believe this is a good feature
 *  for export, and created the Export Song command to do this.  The
 *  write_song() function doesn't count tracks that are muted or that have no
 *  triggers.  For sequences that have triggers, it adds the events in order,
 *  to create a long sequence that plays as if the triggers are present.
 *
 * Stazed/Seq32:
 *
 *      The sequence trigger is not part of the standard MIDI format and is
 *      proprietary to seq32/seq66.  It is added here because the trigger
 *      combining has an alternative benefit for editing.  The user can split,
 *      slice and rearrange triggers to form a new sequence. Then mute all
 *      other tracks and export to a temporary MIDI file. Now they can import
 *      the combined triggers/sequence as a new item. This makes editing of
 *      long improvised sequences into smaller or modified sequences as well as
 *      combining several sequence parts painless.  Also, if the user has a
 *      variety of common items such as drum beats, control codes, etc that can
 *      be used in other projects, this method is very convenient. The common
 *      items can be kept in one file and exported all, individually, or in
 *      part by creating triggers and muting.
 *
 *  Write out the exportable tracks.  Note that we don't need to check the
 *  sequence pointer.  We need to use the same criterion we used to count the
 *  tracks in the first place, not just if the track is active and unmuted.
 *  Also, since we already know that an exportable track is valid, no need to
 *  check for a null pointer.
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
 * \param p
 *      Provides the object that will contain and manage the entire
 *      performance.
 *
 * \return
 *      Returns true if the write operations succeeded.  If false is returned,
 *      then m_error_message will contain a description of the error.
 */

bool
midifile::write_song (performer & p)
{
    automutex locker(m_mutex);
    int numtracks = p.count_exportable();
    bool result = numtracks > 0;
    m_error_message.clear();
    if (result)
    {
        int midiformat = p.smf_format();
        if (midiformat == 0)
        {
            if (numtracks == 1)
            {
                msgprintf
                (
                    msglevel::status, "Exporting song to SMF 0, %d ppqn",
                    m_ppqn
                );
                result = write_header(numtracks, midiformat);
            }
            else
            {
                result = false;
                m_error_message =
                    "The song has more than one track; "
                    "it is unsuitable for saving as SMF 0."
                    ;
            }
        }
        else
        {
            msgprintf(msglevel::status, "Exporting song, %d ppqn", m_ppqn);
            result = write_header(numtracks, midiformat);
        }
    }
    else
    {
        result = false;
        m_error_message =
            "The song has no exportable tracks; "
            "each track to export must have triggers in the song editor "
            "and be unmuted."
            ;
    }
    if (result)
    {
        /*
         * Write out the exportable tracks as described in the banner.
         * Following stazed, we're consolidate the tracks at the beginning of
         * the song, replacing the actual track number with a counter that is
         * incremented only if the track was exportable.  Note that this loop
         * is kind of an elaboration of what goes on in the midi_vector_base ::
         * fill() function for normal Seq66 file writing.
         */

        for (int track = 0; track < p.sequence_high(); ++track)
        {
            if (p.is_exportable(track))
            {
                seq::pointer s = p.get_sequence(track); /* guaranteed good  */
                sequence & seq = *s;
                midi_vector lst(seq);
                result = lst.song_fill_track(track);    /* standalone fill  */
                if (result)
                    write_track(lst);
            }
        }
    }
    if (result)
    {
        std::ofstream file
        (
            m_name.c_str(), std::ios::out | std::ios::binary | std::ios::trunc
        );
        if (file.is_open())
        {
            char file_buffer[c_midi_line_max];      /* enable bufferization */
            file.rdbuf()->pubsetbuf(file_buffer, sizeof file_buffer);
            for (auto c : m_char_list)
            {
                char kc = char(c);
                file.write(&kc, 1);
            }
            m_char_list.clear();
        }
        else
        {
            m_error_message = "Failed to open MIDI file for export.";
            result = false;
        }
    }
    return result;
}

/**
 *  Writes out the final proprietary/SeqSpec section, using the new format.
 *
 *  The first thing to do, for the new format only, is calculate the length
 *  of this big section of data.  This was quite tricky; we tweaked and
 *  adjusted until the midicvt program handled the whole new-format file
 *  without emitting any errors.
 *
 *  Here's the basics of what Seq24 did for writing the data in this part of
 *  the file:
 *
 *      -#  Write the c_midictrl value, then write a 0.  To us, this looks like
 *          no one wrote any code to write this data.  And yet, the parsing
 *          code can handles a non-zero value, which is the number of sequences
 *          as a long value, not a byte.  So shouldn't we write 4 bytes, not
 *          one?  Yes, indeed, we made a mistake.  However, we should be
 *          writing out the full data set as well.  But not even Seq24 does
 *          that!  Perhaps they decided it was best kept in the "rc"
 *          configuration file.
 *      -#  MORE TO COME.
 *
 *  We need a way to make the group mute data optional.  Why write 4096 bytes
 *  of zeroes?
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
midifile::write_seqspec_track (performer & p)
{
    const mutegroups & mutes = p.mutes();
    long tracklength = 0;
    int cnotesz = 2;                            /* first value is short     */
    int highset = p.highest_set();              /* high set number re 0     */
    int maxsets = c_max_sets;                   /* i.e. 32                  */
    if (highset >= maxsets)
        maxsets = highset + 1;

    for (int s = 0; s < maxsets; ++s)
    {
        if (s <= highset)                       /* unused tracks = no name  */
        {
            const std::string & note = p.set_name(s);
            cnotesz += 2 + note.length();       /* short + note length      */
        }
    }

    unsigned groupcount = c_max_groups;         /* 32, the maximum          */
    unsigned groupsize = p.screenset_size();
    int gmutesz = 0;
    if (mutes.saveable_to_midi())
    {
        groupcount = unsigned(mutes.count());   /* includes unused groups   */
        groupsize = unsigned(mutes.group_count());
        if (rc().save_old_mutes())
            gmutesz = 4 + groupcount * (4 + groupsize * 4); /* 4-->longs    */
        else
            gmutesz = 4 + groupcount * (1 + groupsize);     /* 1-->bytes    */

        gmutesz += mutes.group_names_letter_count();        /* NEW NEW NEW  */
    }
    tracklength += seq_number_size();           /* bogus sequence number    */
    tracklength += track_name_size(c_prop_track_name);
    tracklength += prop_item_size(4);           /* c_midictrl               */
    tracklength += prop_item_size(4);           /* c_midiclocks             */
    tracklength += prop_item_size(cnotesz);     /* c_notes                  */
    tracklength += prop_item_size(4);           /* c_bpmtag, beats/minute   */
    if (gmutesz > 0)
        tracklength += prop_item_size(gmutesz); /* c_mutegroups             */

    if (m_global_bgsequence)
    {
        tracklength += prop_item_size(1);       /* c_musickey               */
        tracklength += prop_item_size(1);       /* c_musicscale             */
        tracklength += prop_item_size(4);       /* c_backsequence           */
        tracklength += prop_item_size(4);       /* c_perf_bp_mes            */
        tracklength += prop_item_size(4);       /* c_perf_bw                */
        tracklength += prop_item_size(4);       /* c_tempo_track            */
    }
    tracklength += track_end_size();            /* Meta TrkEnd              */
    write_long(c_prop_chunk_tag);               /* "MTrk" or something else */
    write_long(tracklength);
    write_seq_number(c_prop_seq_number);        /* bogus sequence number    */
    write_track_name(c_prop_track_name);        /* bogus track name         */
    write_seqspec_header(c_midictrl, 4);        /* midi control tag + 4     */
    write_long(0);                              /* Seq24 writes only a zero */
    write_seqspec_header(c_midiclocks, 4);      /* bus mute/unmute data + 4 */
    write_long(0);                              /* Seq24 writes only a zero */
    write_seqspec_header(c_notes, cnotesz);     /* namepad data tag + data  */
    write_short(maxsets);                       /* data, not a tag          */
    for (int s = 0; s < maxsets; ++s)           /* see "cnotesz" calc       */
    {
        if (s <= highset)                       /* unused tracks = no name  */
        {
            const std::string & note = p.set_name(s);
            write_short(midishort(note.length()));
            for (unsigned n = 0; n < unsigned(note.length()); ++n)
                write_byte(midibyte(note[n]));
        }
        else
            write_short(0);                         /* name is empty        */
    }
    write_seqspec_header(c_bpmtag, 4);              /* bpm tag + long data  */

    /*
     *  We now encode the Seq66-specific BPM value by multiplying it
     *  by 1000.0 first, to get more implicit precision in the number.
     *  We should probably sanity-check the BPM at some point.
     */

    midilong scaled_bpm = usr().scaled_bpm(p.get_beats_per_minute());
    write_long(scaled_bpm);                         /* 4 bytes              */
    if (gmutesz > 0)
    {
        write_seqspec_header(c_mutegroups, gmutesz);   /* mute groups tag etc. */
        write_split_long(groupcount, groupsize, rc().save_old_mutes());
        (void) write_c_mutegroups(p);
    }
    if (m_global_bgsequence)
    {
        write_seqspec_header(c_musickey, 1);             /* control tag+1   */
        write_byte(midibyte(usr().seqedit_key()));       /* key change      */
        write_seqspec_header(c_musicscale, 1);           /* control tag+1   */
        write_byte(midibyte(usr().seqedit_scale()));     /* scale change    */
        write_seqspec_header(c_backsequence, 4);         /* control tag+4   */
        write_long(long(usr().seqedit_bgsequence()));    /* background      */
    }
    write_seqspec_header(c_perf_bp_mes, 4);              /* control tag+4   */
    write_long(long(p.get_beats_per_bar()));             /* perfedit BPM    */
    write_seqspec_header(c_perf_bw, 4);                  /* control tag+4   */
    write_long(long(p.get_beat_width()));                /* perfedit BW     */
    write_seqspec_header(c_tempo_track, 4);              /* control tag+4   */
    write_long(long(rc().tempo_track_number()));         /* perfedit BW     */
    write_track_end();
    return true;
}

/**
 *  Writes out a track name.  Note that we have to precede this "event"
 *  with a delta time value, set to 0.  The format of the output is
 *  "0x00 0xFF 0x03 len track-name-bytes".
 *
 * \param trackname
 *      Provides the name of the track to be written to the MIDI file.
 */

void
midifile::write_track_name (const std::string & trackname)
{
    bool ok = ! trackname.empty();
    if (ok)
    {
        write_byte(0);                                  /* delta time       */
        write_byte(EVENT_MIDI_META);                    /* 0xFF meta tag    */
        write_byte(EVENT_META_TRACK_NAME);              /* 0x03 second byte */
        write_varinum(midilong(trackname.size()));
        for (int i = 0; i < int(trackname.size()); ++i)
            write_byte(trackname[i]);
    }
}

/**
 *  Reads the track name.  Meant only for usage in the proprietary/SeqSpec
 *  footer track, in the new file format.
 *
 * \return
 *      Returns the track name, or an empty string if there was a problem.
 */

std::string
midifile::read_track_name ()
{
    std::string result;
    (void) read_byte();                         /* throw-away delta time    */
    midibyte status = read_byte();              /* get the seq-spec marker  */
    if (status == EVENT_MIDI_META)              /* 0x7F                     */
    {
        if (read_byte() == EVENT_META_TRACK_NAME)       /* 0x03             */
        {
            midilong tl = int(read_varinum());  /* track length             */
            for (midilong i = 0; i < tl; ++i)
            {
                midibyte c = read_byte();
                result += c;
            }
        }
    }
    return result;
}

/**
 *  Calculates the size of a trackname and the meta event that specifies
 *  it.
 *
 * \param trackname
 *      Provides the name of the track to be written to the MIDI file.
 *
 * \return
 *      Returns the length of the event, which is of the format "0x00 0xFF
 *      0x03 len track-name-bytes".
 */

long
midifile::track_name_size (const std::string & trackname) const
{
    long result = 0;
    if (! trackname.empty())
    {
        result += 3;                                    /* 0x00 0xFF 0x03   */
        result += varinum_size(long(trackname.size())); /* variable length  */
        result += long(trackname.size());               /* data size        */
    }
    return result;
}

/**
 *  Writes out a sequence number.  The format is "00 FF 00 02 ss ss", where
 *  "02" is actually the constant length of the data.  We have to precede
 *  these values with a 0 delta time, of course.
 *
 *  Now, for sequence 0, an alternate format is "FF 00 00".  But that
 *  format can only occur in the first track, and the rest of the tracks then
 *  don't need a sequence number, since it is assumed to increment.  Our
 *  application doesn't bother with that shortcut.
 *
 * \param seqnum
 *      The sequence number to write.
 */

void
midifile::write_seq_number (midishort seqnum)
{
    write_byte(0);                              /* delta time               */
    write_byte(EVENT_MIDI_META);                /* 0xFF meta tag            */
    write_byte(EVENT_META_SEQ_NUMBER);          /* 0x00 second byte         */
    write_byte(2);                              /* 2-bytes of data          */
    write_short(seqnum);                        /* write sequence number    */
}

/**
 *  Reads the sequence number.  Meant only for usage in the
 *  proprietary/SeqSpec footer track, in the new file format.
 *
 * \return
 *      Returns the sequence number found, or -1 if it was not found.
 */

int
midifile::read_seq_number ()
{
    int result = -1;
    (void) read_byte();                         /* throw-away delta time    */
    midibyte status = read_byte();              /* get the seq-spec marker  */
    if (status == EVENT_MIDI_META)
    {
        if (read_byte() == EVENT_META_SEQ_NUMBER && read_byte() == 2)
            result = int(read_short());
    }
    return result;
}

/**
 *  Writes out the end-of-track marker.
 */

void
midifile::write_track_end ()
{
    write_byte(EVENT_MIDI_META);                    /* 0xFF meta tag        */
    write_byte(EVENT_META_END_OF_TRACK);            /* 0x2F                 */
    write_byte(0);                                  /* no data              */
}

/**
 *  A new function that just sets the fatal-error status and the error message.
 *
 * \param msg
 *      Provides the error message.
 *
 * \return
 *      Returns false, so that the caller can just assign this as the erroneous
 *      boolean function result.
 */

bool
midifile::set_error (const std::string & msg)
{
    m_error_message = msg;
    errprint(msg.c_str());
    m_error_is_fatal = true;
    return false;
}

/**
 *  Helper function to emit more useful error messages.  It adds the file
 *  offset to the message.
 *
 * \param msg
 *      The main error message string, without an ending newline character.
 *
 * \return
 *      Always returns false, to make it easier on the caller.  The constructed
 *      string is returned as a side-effect (m_error_message), plus some other
 *      side-effects (m_error_is_fatal, m_disabled_reported) in case we want to
 *      pass it along to the externally-accessible error-message buffer.
 */

bool
midifile::set_error_dump (const std::string & msg)
{
    char temp[80];
    snprintf
    (
        temp, sizeof temp, "Offset ~0x%zx of 0x%zx bytes (%zu/%zu): ",
        m_pos, m_file_size, m_pos, m_file_size
    );
    std::string result = temp;
    result += "\n";
    result += "   ";
    result += msg;
    msgprintf(msglevel::error, "%s", result.c_str());
    m_error_message = result;
    m_error_is_fatal = true;
    m_disable_reported = true;
    return false;
}

/**
 *  Helper function to emit more useful error messages for erroneous long
 *  values.  It adds the file offset to the message.
 *
 * \param msg
 *      The main error message string, without an ending newline character.
 *
 * \param value
 *      The long value to show as part of the message.
 *
 * \return
 *      Always returns false, to make it easier on the caller.
 */

bool
midifile::set_error_dump (const std::string & msg, unsigned long value)
{
    char temp[64];
    snprintf(temp, sizeof temp, ". Bad value 0x%lx.", value);
    std::string result = msg;
    result += temp;
    return set_error_dump(result);
}

/**
 *  A global function to unify the opening of a MIDI or WRK file.  It also
 *  handles PPQN discovery.
 *
 *  We do not need to clear any existing playlist.  The new function,
 *  performer::clear_all(), also does a clear-all, including the playlist, if
 *  its boolean parameter is set to true.  We leave it false here.
 *
 * \todo
 *      Tighten up wrkfile/midifile handling re PPQN!!!
 *
 * \param [in,out] p
 *      Provides the performance object to update with information read from
 *      the file.
 *
 * \param fn
 *      The full path specification for the file to be opened.
 *
 * \param out ppqn
 *      Provides the PPQN to start with.  It can be c_use_file_ppqn,
 *      or a legitimate PPQN value.  The performer's PPQN value is updated,
 *      and will affect the rest of the application.
 *
 * \param [out] errmsg
 *      If the function fails, this string is filled with the error message.
 *
 * \return
 *      Returns true if reading the MIDI/WRK file succeeded. As a side-effect,
 *      the usrsettings::file_ppqn() is set to return the final PPQN to be
 *      used.
 */

bool
read_midi_file
(
    performer & p,
    const std::string & fn,
    int ppqn,                                   /* might get altered        */
    std::string & errmsg,
    bool addtorecent
)
{
    bool result = file_readable(fn);            /* how to disable Save?     */
    if (result)
    {
        bool is_wrk = file_extension_match(fn, "wrk");
        if (usr().use_file_ppqn())
            ppqn = c_use_file_ppqn;

        ppqn = choose_ppqn(ppqn);               /* no usr().file_ppqn() yet */

        midifile * fp = is_wrk ?
            new (std::nothrow) wrkfile(fn, ppqn) :
            new (std::nothrow) midifile(fn, ppqn) ;

        std::unique_ptr<midifile> f(fp);
        p.clear_all();                          /* see banner notes         */
        result = bool(f);
        if (result)
            result = f->parse(p, 0);

        if (result)
        {
            if (usr().use_file_ppqn())
            {
                ppqn = f->ppqn();               /* get & return file PPQN   */
                usr().file_ppqn(ppqn);          /* save the value from file */
            }
            usr().midi_ppqn(ppqn);              /* save the current value   */
            p.set_ppqn(ppqn);                   /* set up PPQN for MIDI     */
            rc().midi_filename(fn);             /* save current file-name   */
            if (addtorecent)
            {
                rc().last_used_dir(fn.substr(0, fn.rfind("/") + 1));
                rc().add_recent_file(fn);       /* Oli Kester's Kepler34!   */
            }
            p.announce_playscreen();            /* tell MIDI control out    */
            file_message("Read MIDI file", fn);
        }
        else
        {
            errmsg = f->error_message();
            if (f->error_is_fatal())
            {
                if (addtorecent)
                    rc().remove_recent_file(fn);
            }
        }
    }
    else
    {
        std::string msg = "File not accessible";
        file_error(msg, fn);
        errmsg = msg + ": " + fn;
        rc().remove_recent_file(fn);
    }
    return result;
}

bool
write_midi_file
(
    performer & p,
    const std::string & fn,
    std::string & errmsg
)
{
    bool result = false;
    std::string fname = fn.empty() ? rc().midi_filename() : fn ;
    if (fname.empty())
    {
        errmsg = "No file-name to write";
    }
    else
    {
        bool glob = usr().global_seq_feature();
        midifile f(fname, p.ppqn(), glob);
        result = f.write(p);
        if (result)
        {
            rc().midi_filename(fname);
            rc().add_recent_file(fname);            /* rc().midi_filename() */
            file_message("Wrote MIDI file", fname);
            p.unmodify();
        }
        else
        {
            errmsg = f.error_message();
            file_error("Write failed", fname);
        }
    }
    return result;
}

}           // namespace seq66

/*
 * midifile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


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
 * \file          midifile.cpp
 *
 *  This module declares/defines the base class for MIDI files.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2021-05-13
 * \license       GNU GPLv2 or above
 *
 *  For a quick guide to the MIDI format, see, for example:
 *
 *  http://www.mobilefish.com/tutorials/midi/midi_quickguide_specification.html
 *
 *  It is important to note that most sequencers have taken a shortcut or
 *  two in reading the MIDI format.  For example, most will silently
 *  ignored an unadorned control tag (0x242400nn) which has not been
 *  packaged up as a proper sequencer-specific meta event.  The midicvt
 *  program (https://github.com/ahlstromcj/midicvt, derived from midicomp,
 *  midi2text, and mf2t/t2mf) does not ignore this lack of a SeqSpec wrapper,
 *  and hence we decided to provide a new, more strict input and output format
 *  for the the proprietary/SeqSpec track in Seq66.
 *
 *  Elements written:
 *
 *      -   MIDI header.
 *      -   Tracks.
 *          These items are then written, preceded by the "MTrk" tag and
 *          the track size.
 *          -   Sequence number.
 *          -   Sequence name.
 *          -   Time-signature and tempo (sequence 0 only)
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
 *      -   Proprietary SeqSpec data.
 *
 *  Uses the new format for the proprietary footer section of the Seq24
 *  MIDI file.
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
#include "util/calculations.hpp"        /* seq66::bpm_from_tempo_us() etc.  */
#include "util/filefunctions.hpp"       /* seq66::get_full_path()           */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Magic numbers for handling mute-group formats.
 */

const unsigned c_compact_mutes_shift    = 8;                /* 1 << 8 = 256 */
const unsigned c_legacy_mute_group      = 1024;             /* 0x0400       */
const unsigned c_compact_mute_group     = 256 << 16;

static unsigned
to_compact_byte (unsigned value)
{
    if (value > 0)
        value <<= c_compact_mutes_shift;

    return value;
}

static unsigned
from_compact_byte (unsigned value)
{
    if (value > 0)
        value >>= c_compact_mutes_shift;

    return value;
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
 *              scale the running-time of the sequence relative to
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
    m_use_scaled_ppqn           (ppqn != SEQ66_USE_FILE_PPQN),
    m_ppqn                      (ppqn),                 /* can start as 0   */
    m_file_ppqn                 (0),                    /* can change       */
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
    midilong result = read_long();              /* amount of data           */
    if (result == c_legacy_mute_group)
    {
        highbytes = 32U;
        lowbytes = 32U;
    }
    else if (result == 0)
    {
        highbytes = 0;
        lowbytes = 0;
    }
    else
    {
        if (result >= c_compact_mute_group)
        {
            highbytes = from_compact_byte(result & 0xFFFF0000 >> 16);
            lowbytes = from_compact_byte(result & 0x0000FFFF);
        }
        else
        {
            highbytes = result & 0xFFFF0000 >> 16;      /* was 8, bug! */
            lowbytes = result & 0x0000FFFF;
        }
    }
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
        std::string path = get_full_path(m_name);
        m_file_size = file.tellg();                 /* get the end offset   */

        /*
         * Kind of annoying with playlists.  Also, be verbose only if asked to
         * be, via the -v/--verbose option.  Actually, this is annoying for
         * long path-names.
         *
         * if (rc().verbose() && ! verify_mode())
         *     printf("[Opened %s file, '%s']\n", tag.c_str(), path.c_str());
         */

        if (m_file_size <= sizeof(long))
        {
            result = set_error("Invalid file size... reading a directory?");
        }
        else
        {
            file.seekg(0, std::ios::beg);           /* seek to start        */
            try
            {
                m_data.resize(m_file_size);         /* allocate more data   */
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
    c_midich:           SeqSpec FF 7F 05 24 24 00 02 06
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
 *      non-zero, then we will assume that the performer data is dirty.
 *
 * \param importing
 *      Indicates that we are importing a file, and do not want to parse/erase
 *      any "proprietrary" information from the performance.
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
        clear_errors();
        m_smf0_splitter.initialize();                   /* SMF 0 support    */

        midilong ID = read_long();                      /* hdr chunk info   */
        midilong hdrlength = read_long();               /* MThd length      */
        if (ID != SEQ66_MTHD_TAG && hdrlength != 6)     /* magic 'MThd'     */
            return set_error_dump("Invalid MIDI header chunk detected", ID);

        midishort Format = read_short();                /* 0, 1, or 2       */
        if (Format == 0)
        {
            result = parse_smf_0(p, screenset);
        }
        else if (Format == 1)
        {
            result = parse_smf_1(p, screenset);
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
                    result = parse_proprietary_track(p, m_file_size);
            }
            if (result && screenset != 0)
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
    bool result = parse_smf_1(p, screenset, true);  /* format 0 is flagged  */
    if (result)
    {
        result = m_smf0_splitter.split(p, screenset, m_ppqn);
        if (result)
            p.modify();                             /* to prompt for save   */
        else
            result = set_error("No SMF 0 main sequence found, bad MIDI file");
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
    bool result = len <= SEQ66_VARLENGTH_MAX;               /* 0x0FFFFFFF */
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
 * \param ppqn
 *      Provides the ppqn value to use to scale the tick values if
 *      scaled() is true.  If 0, the ppqn value is not used.
 *
 * \param transposable
 *      If true, use the new style trigger, which adds a byte-long transpose
 *      value.
 */

void
midifile::add_trigger (sequence & seq, midishort ppqn, bool transposable)
{
    midilong on = read_long();
    midilong off = read_long();
    midilong offset = read_long();
    midibyte tpose = 0;
    if (transposable)
        tpose = read_byte();

    if (ppqn > 0)
    {
        on = rescale_tick(on, ppqn, m_ppqn);        /* old PPQN, new PPQN   */
        off = rescale_tick(off, ppqn, m_ppqn);
        offset = rescale_tick(offset, ppqn, m_ppqn);
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
 *  Note that NumTracks doesn't count the Seq24 "proprietary" footer section,
 *  even if it uses the new format, so that section will still be read
 *  properly after all normal tracks have been processed.
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
    midishort NumTracks = read_short();
    midishort fileppqn = read_short();
    file_ppqn(int(fileppqn));                       /* original file PPQN   */
    if (usr().use_file_ppqn())
    {
        p.file_ppqn(file_ppqn());                   /* let performer know   */
        ppqn(file_ppqn());                          /* PPQN == file PPQN    */
        m_use_scaled_ppqn = false;                  /* do not scale time    */
    }
    else
        m_use_scaled_ppqn = file_ppqn() != usr().default_ppqn();

    for (midishort track = 0; track < NumTracks; ++track)
    {
        midilong ID = read_long();                  /* get track marker     */
        midilong TrackLength = read_long();         /* get track length     */
        if (ID == SEQ66_MTRK_TAG)                   /* magic number 'MTrk'  */
        {
            char TrackName[SEQ66_TRACKNAME_MAX];    /* track name from file */
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
#if defined SEQ66_PLATFORM_DEBUG_TMI
            int eventcounter = 0;
#endif
            while (! done)                          /* get events in track  */
            {
                event e;
                midilong len;                       /* important counter!   */
                midibyte d0, d1;                    /* was data[2];         */
                midipulse delta = read_varinum();           /* time delta   */
                status = m_data[m_pos];                     /* current byte */
                if (event::is_status(status))               /* 0x80 bit?    */
                {
                    skip(1);                                /* get to d0    */
                    if (event::is_system_common(status))    /* 0xF0 to 0xF7 */
                        runningstatus = 0;                  /* clear it     */
                    else if (! event::is_realtime(status))  /* 0xF8 to 0xFF */
                        runningstatus = status;             /* log status   */
                }
                else
                {
                    /*
                     * Handle data values. If in running status, set that as
                     * status; the next value to be read is the d0 value.
                     * If not running status, is this an ERROR?
                     */

                    if (runningstatus > 0)      /* running status in force? */
                        status = runningstatus; /* yes, use running status  */
                }
                e.set_status(status);           /* set the members in event */

                /*
                 *  See "PPQN" section in banner.
                 */

                runningtime += delta;           /* add in the time          */
                if (scaled())                   /* adjust time via ppqn     */
                {
                    currenttime = runningtime * m_ppqn / m_file_ppqn;
                    e.set_timestamp(currenttime);
                }
                else
                {
                    currenttime = runningtime;
                    e.set_timestamp(currenttime);
                }

                midibyte eventcode = event::mask_status(status);    /* F0 */
                midibyte channel = event::get_channel(status);      /* 0F */
                switch (eventcode)
                {
                case EVENT_NOTE_OFF:          /* cases for 2-data-byte events */
                case EVENT_NOTE_ON:
                case EVENT_AFTERTOUCH:
                case EVENT_CONTROL_CHANGE:
                case EVENT_PITCH_WHEEL:

                    d0 = read_byte();                     /* was data[0]      */
                    d1 = read_byte();                     /* was data[1]      */
                    if (is_note_off_velocity(eventcode, d1))
                        e.set_channel_status(EVENT_NOTE_OFF, channel);

                    e.set_data(d0, d1);                   /* set data and add */

                    /*
                     * s.append_event() doesn't sort events; sort after we
                     * get them all.  Also, it is kind of weird we change the
                     * channel for the whole sequence here.
                     */

                    s.append_event(e);                    /* does not sort    */
                    s.set_midi_channel(channel);          /* set MIDI channel */
                    if (is_smf0)
                        m_smf0_splitter.increment(channel);
                    break;

                case EVENT_PROGRAM_CHANGE:    /* cases for 1-data-byte events */
                case EVENT_CHANNEL_PRESSURE:

                    d0 = read_byte();                   /* was data[0]      */
                    e.set_data(d0);                     /* set data and add */

                    /*
                     * s.append_event() doesn't sort events; they're sorted
                     * after we read them all.
                     */

                    s.append_event(e);                  /* does not sort    */
                    s.set_midi_channel(channel);        /* set midi channel */
                    if (is_smf0)
                        m_smf0_splitter.increment(channel);
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
                                for (int i = 0; i < int(len); ++i)
                                {
                                    char ch = char(read_byte());
                                    if (count < SEQ66_TRACKNAME_MAX)
                                    {
                                        TrackName[count] = ch;
                                        ++count;
                                    }
                                }
                                TrackName[count] = '\0';
                                s.set_name(TrackName);
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
                                bt[3] = 0;

                                double tt = tempo_us_from_bytes(bt);
                                if (tt > 0)
                                {
                                    static bool gotfirst = false;
                                    if (track == 0)
                                    {
                                        midibpm bpm = bpm_from_tempo_us(tt);
                                        if (! gotfirst)
                                        {
                                            gotfirst = true;
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
                                int bw = beat_pow2(logbase2);
                                s.set_beats_per_bar(bpm);
                                s.set_beat_width(bw);
                                s.clocks_per_metronome(cc);
                                s.set_32nds_per_quarter(bb);
                                if (track == 0)
                                {
                                    p.set_beats_per_bar(bpm);
                                    p.set_beat_width(bw);
                                    p.clocks_per_metronome(cc);
                                    p.set_32nds_per_quarter(bb);
                                }

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
                                s.set_midi_bus(read_byte());
                                --len;
                            }
                            else if (seqspec == c_midich)
                            {
                                midibyte channel = read_byte();
                                s.set_midi_channel(channel);
                                if (is_smf0)
                                    m_smf0_splitter.increment(channel);

                                --len;
                            }
                            else if (seqspec == c_timesig)
                            {
                                timesig_set = true;
                                int bpm = int(read_byte());
                                int bw = int(read_byte());
                                s.set_beats_per_bar(bpm);
                                s.set_beat_width(bw);
                                p.set_beats_per_bar(bpm);
                                p.set_beat_width(bw);
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
                                midishort p = scaled() ?  m_file_ppqn : 0 ;
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
                                midishort p = scaled() ?  m_file_ppqn : 0 ;
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
                                s.color(read_byte());
                                --len;
                            }
#if defined SEQ66_SEQUENCE_EDIT_MODE        /* same as "not transposable"?  */
                            else if (seqspec == c_seq_edit_mode)
                            {
                                sequence::edit_mode m = (read_byte());
                                s.edit_mode(read_byte());
                                --len;
                            }
#endif
                            else if (seqspec == c_seq_loopcount)
                            {
                                s.loop_count_max(int(read_short()));
                                len -= 2;
                            }
                            else if (SEQ66_IS_PROPTAG(seqspec))
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

                            if (rc().verbose())
                            {
                                int index = int(mtype);
                                if (index >= 0 && index < 8)
                                {
                                    std::string text;
                                    if (read_string(text, len))
                                    {
                                        std::string m = "Skipping meta: ";
                                        m += sm_meta_text_labels[index];
                                        m += " '";
                                        m += text;
                                        m += "'";
                                        (void) set_error_dump(m);
                                    }
                                }
                            }
                            else
                                skip(len);              /* eat it           */
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
#if defined SEQ66_PLATFORM_DEBUG_TMI
                ++eventcounter;
#endif
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

            if (seqnum < PROP_SEQ_NUMBER)
            {
                if (buss_override != c_bussbyte_max)
                    s.set_midi_bus(buss_override);

                if (is_smf0)
                    (void) m_smf0_splitter.log_main_sequence(s, seqnum);
                else
                    finalize_sequence(p, s, seqnum, screenset);
            }

#if defined SEQ66_PLATFORM_DEBUG_TMI
            s.print();
#endif
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
    int preferred_seqnum = seqnum + screenset * usr().seqs_in_set();
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
 *      c_midich, found in the midi_vector_base module, that indicate the type
 *      of sequencer-specific data that comes next.  If there is not enough
 *      data to process, then 0 is returned.
 */

midilong
midifile::parse_prop_header (int file_size)
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
                fprintf
                (
                    stderr, "Unexpected meta type 0x%x near offset 0x%lx\n",
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
 *  now save a scaled version of BPM.  Our supported range of BPM is
 *  SEQ66_MINIMUM_BPM = 1 to SEQ66_MAXIMUM_BPM = 600.  If this range is
 *  encountered, the value is read as is.  If greater than this range
 *  (actually, we use 999 as the limit), then we divide the number by 1000 to
 *  get the actual BPM, which can thus have more precision than the old
 *  integer value allowed.  Obviously, when saving, we will multiply by 1000
 *  to encode the BPM.
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
midifile::parse_proprietary_track (performer & p, int file_size)
{
    bool result = true;
    midilong ID = read_long();                      /* Get ID + Length      */
    if (ID == PROP_CHUNK_TAG)                       /* magic number 'MTrk'  */
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
            bool ok = (sn == PROP_SEQ_NUMBER) || (sn == PROP_SEQ_NUMBER_OLD);
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
                 * if (trackname != PROP_TRACK_NAME)
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
    {
        /*
         * This section depends on the ordering and presence of all supported
         * SeqSpecs, and hence is kind of brittle.  It would be better to loop
         * here and use a switch-statement to figure out which code to
         * execute.
         */

        midilong seqspec = parse_prop_header(file_size);

        /*
         * Seq24 would store the MIDI control setting in the MIDI file.  While
         * this could be a useful feature, it seems a bit confusing, since the
         * user/musician will more likely define those controls for his set of
         * equipment to apply to all songs.
         *
         * Furthermore, we would need to load these control settings into a
         * midicontrolin (see ctrl/midicontrolin modules).
         * And, lastly, Seq24 never wrote these controls to the file.  It
         * merely wrote the c_midictrl code, followed by a long 0.
         * For now, we are going to evade this functionality.  We will
         * continue to write this section, and try to read it, but expect it
         * to be empty.
         *
         * Track-specific SeqSpecs handled in parse_smf_1():
         *
         *  c_midibus          c_timesig         c_midich         c_musickey *
         *  c_musicscale *     c_backsequence *  c_transpose *    c_seq_color
         *  c_seq_edit_mode !  c_seq_loopcount   c_triggers       c_triggers_ex
         *  c_trig_transpose
         *
         * Global SeqSpecs handled here:
         *
         *  c_midictrl         c_midiclocks      c_notes          c_bpmtag
         *  c_mutegroups       c_musickey *      c_musicscale *
         *  c_backsequence *   c_perf_bp_mes     c_perf_bw        c_tempo_map !
         *  c_reserved_1 !     c_reserved_2 !    c_tempo_track
         *  c_seq_edit_mode !
         *
         * Not handled:
         *
         *  c_gap_A to _F      c_reserved_3 and _4
         */

        if (seqspec == c_midictrl)
        {
            int ctrls = int(read_long());

            /*
             * Some old code wrote some bad files, we need to work around that
             * and fix it.  The value of max_sequence() is generally 1024.
             */

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

#if defined USE_MIDI_CONTROL_IN_SONGS                   /* deprecated */
                p.midi_control_toggle(i).set(a);
#endif
                read_byte_array(a, 6);

#if defined USE_MIDI_CONTROL_IN_SONGS
                p.midi_control_on(i).set(a);
#endif
                read_byte_array(a, 6);

#if defined USE_MIDI_CONTROL_IN_SONGS
                p.midi_control_off(i).set(a);
#endif
            }
        }
        seqspec = parse_prop_header(file_size);
        if (seqspec == c_midiclocks)
        {
            /*
             * Some old code wrote some bad files, we work around and fix it.
             */

            int busscount = int(read_long());

#if defined USE_MIDI_CLOCK_IN_SONGS
            if (busscount > c_busscount_max)
            {
                (void) set_error_dump
                (
                    "Bad buss count, fixing; please save the file now."
                );
                skip(-4);
                busscount = int(read_byte());
            }
#endif  // defined USE_MIDI_CLOCK_IN_SONGS

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
        }
        seqspec = parse_prop_header(file_size);
        if (seqspec == c_notes)
        {
            /*
             * The default number of sets is c_max_sets = 32.  This is also
             * the initial minimum number of screen-set names, though some
             * can be empty.  More than 32 sets can be supported, though that
             * claim is currently untested.  The highest set number is
             * determined by the highest pattern number; the
             * setmaster::add_set() function creates a new set when a pattern
             * number falls outside the boundaries of the existing sets.
             */

            midishort screen_sets = read_short();
            for (midishort x = 0; x < screen_sets; ++x)
            {
                midishort len = read_short();               /* string size  */
                std::string notess;
                for (midishort i = 0; i < len; ++i)
                    notess += read_byte();                  /* unsigned!    */

                p.set_screenset_notepad(x, notess, true);   /* load time    */
            }
        }
        seqspec = parse_prop_header(file_size);
        if (seqspec == c_bpmtag)                            /* beats/minute */
        {
            /*
             * Should check here for a conflict between the Tempo meta event
             * and this tag's value.  NOT YET.  Also, as of 2017-03-22, we
             * want to be able to handle two decimal points of precision in
             * BPM.  See the function banner for a discussion.
             */

            midibpm bpm = midibpm(read_long());
            if (bpm > (SEQ66_BPM_SCALE_FACTOR - 1.0))
                bpm /= SEQ66_BPM_SCALE_FACTOR;

            p.set_beats_per_minute(bpm);                    /* 2nd setter   */
        }

        /*
         * Read in the mute group information.  If the length of the mute
         * group section is 0, then this file is a Seq42 file, and we
         * ignore the section.  (Thanks to Stazed for that catch!)
         */

        seqspec = parse_prop_header(file_size);
        if (seqspec == c_mutegroups)
            (void) parse_mute_groups(p);

        /*
         * We let Seq66 read this new stuff even the global-background
         * sequence is in force.  That flag affects only the writing of the
         * MIDI file, not the reading.
         */

        seqspec = parse_prop_header(file_size);
        if (seqspec == c_musickey)
        {
            int key = int(read_byte());
            usr().seqedit_key(key);
        }
        seqspec = parse_prop_header(file_size);
        if (seqspec == c_musicscale)
        {
            int scale = int(read_byte());
            usr().seqedit_scale(scale);
        }
        seqspec = parse_prop_header(file_size);
        if (seqspec == c_backsequence)
        {
            int seqnum = int(read_long());
            usr().seqedit_bgsequence(seqnum);
        }

        /*
         * Store the beats/measure and beat-width values from the perfedit
         * window.
         */

        seqspec = parse_prop_header(file_size);
        if (seqspec == c_perf_bp_mes)
        {
            int bpmes = int(read_long());
            p.set_beats_per_bar(bpmes);
        }
        seqspec = parse_prop_header(file_size);
        if (seqspec == c_perf_bw)
        {
            int bw = int(read_long());
            p.set_beat_width(bw);
        }

#if defined USE_THESE_SEQSPECS

        seqspec = parse_prop_header(file_size);
        if (seqspec == c_tempo_map)
        {
            /*
             * A stazed feature not implemented.
             */
        }

        seqspec = parse_prop_header(file_size);
        if (seqspec == c_reserved_1)
        {
            /*
             * Reserved for other projects to use.
             */
        }

        seqspec = parse_prop_header(file_size);
        if (seqspec == c_reserved_2)
        {
            /*
             * Reserved for other projects to use.
             */
        }

#endif  // USE_THESE_SEQSPECS

        /*
         * If this value is present and greater than 0, set it into the
         * performance.  It might override the value specified in the "rc"
         * configuration file.
         */

        seqspec = parse_prop_header(file_size);
        if (seqspec == c_tempo_track)
        {
            int tempotrack = int(read_long());
            if (tempotrack > 0)
                p.tempo_track_number(tempotrack);
        }

#if defined SEQ66_SEQUENCE_EDIT_MODE_GLOBAL

        /*
         * Sequence editing mode are a feature of Kepler34.  We don't know
         * what these modes do, yet, but let's leave room for them.
         *
         * We will eventually store this in the MIDI file. Also the current
         * code below will SKIP DATA!!!  So we are disabling it!!!
         */

        seqspec = parse_prop_header(file_size);
        if (seqspec == c_seq_edit_mode)
        {
            for (int track = 0; track < p.sequence_high(); ++track)
            {
                if (p.is_seq_active(track))
                    p.edit_mode(track, sequence::editmode(read_long()));
            }
        }

#endif  // SEQ66_SEQUENCE_EDIT_MODE_GLOBAL

        /*
         * ADD NEW CONTROL TAGS AT THE END OF THE LIST HERE.  Don't forget to
         * fill in any blanks with new code above, if need be.
         */
    }
    return result;
}

/**
 *  Updated mute-groups parsing that supports the old style "32 x 32"
 *  mutegroups, and more variable setup.  Lots to do yet!
 *
 *  Also need to check for any mutes being present.
 *
 *  What about rows & columns?  Ultimately, the set-size must
 *  match that specified by the application's user-interface as
 *  must the rows and columns.
 */

bool
midifile::parse_mute_groups (performer & p)
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
            /*
             * Alternative: legacyformat = len < c_compact_mute_group;
             */

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
                    if (! mutes.load(group, mutebits))
                        break;                              /* a duplicate? */
                }
            }
            else
            {
                mutes.legacy_mutes(false);
                for (unsigned g = 0; g < groupcount; ++g)
                {
                    midibooleans mutebits;
                    midilong group = read_byte();
                    for (unsigned s = 0; s < groupsize; ++s)
                    {
                        midibyte gmutestate = read_byte();  /* byte for a bit */
                        bool status = gmutestate != 0;
                        mutebits.push_back(midibool(status));
                    }
                    if (! mutes.load(group, mutebits))
                        break;                              /* often duplicate  */
                }
            }
        }
    }
    return result;
}

/**
 *  For each groups in the mute-groups, write the status bits to the
 *  c_mutegroups SeqSpec.
 *
 *  The mutegroups class has rows and columns for each group, but doesn't have
 *  a way to iterate through all the groups.
 */

bool
midifile::write_mute_groups (const performer & p)
{
    const mutegroups & mutes = p.mutes();
    bool result = mutes.group_save_to_midi();
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
 *      are limited to range from 0 to 65535, a short value. For mute-groups,
 *      this is the number of mute-groups.
 *
 * \param [out] lowbytes
 *      Provides the value of the least significant 2 bytes of the four-byte
 *      ("long") value.  The most significant bytes are masked out; the values
 *      are limited to range from 0 to 65535, a short value. For mute-groups,
 *      this is the number of patterns in the mute-groups.
 */

void
midifile::write_split_long (unsigned highbytes, unsigned lowbytes)
{
    if (highbytes == 32U && lowbytes == 32U)
    {
        write_long(1024);               /* a long-standing legacy value */
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
 */

bool
midifile::write_header (int numtracks)
{
    write_long(0x4D546864);                 /* MIDI Format 1 header MThd    */
    write_long(6);                          /* Length of the header         */
    write_short(1);                         /* MIDI Format 1                */
    write_short(numtracks);                 /* number of tracks             */
    write_short(m_ppqn);                    /* parts per quarter note       */
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
midifile::write_start_tempo (midibpm start_tempo)
{
    write_byte(0x00);                       /* delta time at beginning      */
    write_short(0xFF51);
    write_byte(0x03);                       /* message length, must be 3    */
    write_triple(midilong(60000000.0 / start_tempo));
}

#endif  // USE_WRITE_START_TEMPO

#if defined USE_WRITE_TIME_SIG

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

#endif  // USE_WRITE_TIME_SIG

/**
 *  Writes a "proprietary" (SeqSpec) Seq24 footer header in the new
 *  MIDI-compliant format.  This function does not write the data.  It
 *  replaces calls such as "write_long(c_midich)" in the proprietary secton of
 *  write().
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
midifile::write_prop_header (midilong control_tag, long len)
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
    write_long(SEQ66_MTRK_TAG);             /* magic number 'MTrk'          */
    write_long(tracksize);
    while (! lst.done())                    /* write the track data         */
        write_byte(lst.get());
}

/**
 *  Calculates the size of a proprietary item, as written by the
 *  write_prop_header() function, plus whatever is called to write the data.
 *  If using the new format, the length includes the sum of sequencer-specific
 *  tag (0xFF 0x7F) and the size of the variable-length value.  Then, for the
 *  new format, 4 bytes are added for the Seq24 MIDI control
 *  value, and then the data length is added.
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
 * \return
 *      Returns true if the write operations succeeded.  If false is returned,
 *      then m_error_message will contain a description of the error.
 */

bool
midifile::write (performer & p, bool doseqspec)
{
    automutex locker(m_mutex);
    bool result = m_ppqn >= SEQ66_MINIMUM_PPQN && m_ppqn <= SEQ66_MAXIMUM_PPQN;
    m_error_message.clear();
    if (! result)
        m_error_message = "Error, invalid PPQN for MIDI file to write";

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
        {
            bool result = write_header(numtracks);
            if (result)
            {
                std::string temp = "Writing ";
                temp += doseqspec ? "Seq66" : "normal" ;
                temp += " MIDI file ";
                temp += std::to_string(m_ppqn);
                temp += " PPQN";
                file_message(temp, m_name);
            }
            else
                m_error_message = "Failed to write header to MIDI file";
        }
        else
            m_error_message = "No patterns/tracks available to write";
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
        result = write_proprietary_track(p);
        if (! result)
            m_error_message = "Error, could not write SeqSpec track";
    }
    if (result)
    {
        std::ofstream file
        (
            m_name.c_str(), std::ios::out | std::ios::binary | std::ios::trunc
        );
        if (file.is_open())
        {
            char file_buffer[SEQ66_MIDI_LINE_MAX];  /* enable bufferization */
            file.rdbuf()->pubsetbuf(file_buffer, sizeof file_buffer);
            for (auto c : m_char_list)              /* list of midibytes    */
            {
                char kc = char(c);
                file.write(&kc, 1);
                if (file.fail())
                {
                    m_error_message = "Error writing MIDI byte";
                    result = false;
                }
            }
            m_char_list.clear();
        }
        else
        {
            m_error_message = "Error opening MIDI file for writing";
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
    int numtracks = 0;
    m_error_message.clear();
    for (int i = 0; i < p.sequence_high(); ++i) /* count exportable tracks  */
    {
        if (p.is_exportable(i))                 /* do muted tracks count?   */
            ++numtracks;
    }

    bool result = numtracks > 0;
    if (result)
    {
        printf("[Exporting song, %d ppqn]\n", m_ppqn);
        result = write_header(numtracks);
    }
    else
    {
        m_error_message =
            "The current song has no exportable tracks; "
            "each track to export must have triggers in the Song Editor "
            "and be unmuted."
            ;
        result = false;
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
                seq::pointer s = p.get_sequence(track);
                if (s)
                {
                    sequence & seq = *s;
                    midi_vector lst(seq);
                    lst.fill_seq_number(track);
                    lst.fill_seq_name(seq.name());

#if defined USE_FILL_TIME_SIG_AND_TEMPO
                    if (track == 0)
                    {
                        /*
                         * As per issue #141, do not force the
                         * creation/writing of time-sig and tempo events.
                         */

                        seq.events().scan_meta_events();
                        lst.fill_time_sig_and_tempo
                        (
                            p, seq.events().has_time_signature(),
                            seq.events().has_tempo()
                        );
                    }
#endif

                    /*
                     * Add all triggered events (see the function banner).
                     * If any, then make one long trigger.
                     */

                    midipulse last_ts = 0;
                    const auto & trigs = seq.get_triggers();
                    for (auto & t : trigs)
                        last_ts = lst.song_fill_seq_event(t, last_ts);

                    if (! trigs.empty())        /* adjust sequence length   */
                    {
                        /*
                         * tick_end() isn't quite a trigger length, off by 1.
                         * Subtracting tick_start() can really screw it up.
                         */

                        const trigger & ender = trigs.back();
                        midipulse seqend = ender.tick_end();
                        midipulse measticks = seq.measures_to_ticks();
                        if (measticks > 0)
                        {
                            midipulse remainder = seqend % measticks;
                            if (remainder != (measticks - 1))
                                seqend += measticks - remainder - 1;
                        }
                        lst.song_fill_seq_trigger(ender, seqend, last_ts);
                    }
                    write_track(lst);
                }
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
            char file_buffer[SEQ66_MIDI_LINE_MAX];  /* enable bufferization */
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
            m_error_message = "Error opening MIDI file for exporting";
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
midifile::write_proprietary_track (performer & p)
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
            const std::string & note = p.get_screenset_notepad(s);
            cnotesz += 2 + note.length();       /* short + note length      */
        }
    }

    unsigned groupcount = c_max_groups;         /* 32, the maximum          */
    unsigned groupsize = p.seqs_in_set();
    int gmutesz = 0;
    if (mutes.group_save_to_midi() && mutes.any())
    {
        groupcount = unsigned(mutes.count());   /* no. of existing groups  */
        groupsize = unsigned(mutes.group_size());
        if (rc().save_old_mutes())
            gmutesz = 4 + groupcount * (4 + groupsize * 4); /* 4-->longs    */
        else
            gmutesz = 4 + groupcount * (1 + groupsize);     /* 1-->bytes    */
    }
    tracklength += seq_number_size();           /* bogus sequence number    */
    tracklength += track_name_size(PROP_TRACK_NAME);
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

    write_long(PROP_CHUNK_TAG);                 /* "MTrk" or something else */
    write_long(tracklength);
    write_seq_number(PROP_SEQ_NUMBER);          /* bogus sequence number    */
    write_track_name(PROP_TRACK_NAME);          /* bogus track name         */

    write_prop_header(c_midictrl, 4);           /* midi control tag + 4     */
    write_long(0);                              /* Seq24 writes only a zero */
    write_prop_header(c_midiclocks, 4);         /* bus mute/unmute data + 4 */
    write_long(0);                              /* Seq24 writes only a zero */
    write_prop_header(c_notes, cnotesz);        /* notepad data tag + data  */
    write_short(maxsets);                       /* data, not a tag          */
    for (int s = 0; s < maxsets; ++s)           /* see "cnotesz" calc       */
    {
        if (s <= highset)                       /* unused tracks = no name  */
        {
            const std::string & note = p.get_screenset_notepad(s);
            write_short(midishort(note.length()));
            for (unsigned n = 0; n < unsigned(note.length()); ++n)
                write_byte(midibyte(note[n]));
        }
        else
            write_short(0);                     /* name is empty            */
    }
    write_prop_header(c_bpmtag, 4);             /* bpm tag + long data      */

    /*
     *  We now encode the Seq66-specific BPM value by multiplying it
     *  by 1000.0 first, to get more implicit precision in the number.
     *  We should probably sanity-check the BPM at some point.
     */

    long scaled_bpm = long(p.get_beats_per_minute() * SEQ66_BPM_SCALE_FACTOR);
    write_long(scaled_bpm);                     /* 4 bytes                  */
    if (gmutesz > 0)
    {
        write_prop_header(c_mutegroups, gmutesz);   /* mute groups tag etc. */
        if (rc().save_old_mutes())
        {
            write_split_long(groupcount, groupsize);
        }
        else
        {
            unsigned gcount = to_compact_byte(groupcount);
            unsigned gsize = to_compact_byte(groupsize);
            write_split_long(gcount, gsize);
        }
        (void) write_mute_groups(p);
    }
    if (m_global_bgsequence)
    {
        write_prop_header(c_musickey, 1);               /* control tag+1 */
        write_byte(midibyte(usr().seqedit_key()));      /* key change    */
        write_prop_header(c_musicscale, 1);             /* control tag+1 */
        write_byte(midibyte(usr().seqedit_scale()));    /* scale change  */
        write_prop_header(c_backsequence, 4);           /* control tag+4 */
        write_long(long(usr().seqedit_bgsequence()));   /* background    */
    }
    write_prop_header(c_perf_bp_mes, 4);                /* control tag+4 */
    write_long(long(p.get_beats_per_bar()));            /* perfedit BPM  */
    write_prop_header(c_perf_bw, 4);                    /* control tag+4 */
    write_long(long(p.get_beat_width()));               /* perfedit BW   */
    write_prop_header(c_tempo_track, 4);                /* control tag+4 */
    write_long(long(p.tempo_track_number()));           /* perfedit BW   */
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
        temp, sizeof temp, "Near offset 0x%zx of 0x%zx bytes (%zu/%zu): ",
        m_pos, m_file_size, m_pos, m_file_size
    );
    std::string result = temp;
    result += "\n";
    result += "   ";
    result += msg;
    fprintf(stderr, "%s\n", result.c_str());
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
 *      Provides the PPQN to start with.  It can be SEQ66_USE_FILE_PPQN,
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
    std::string & errmsg
)
{
    bool result = file_accessible(fn);
    if (result)
    {
        bool is_wrk = file_extension_match(fn, "wrk");
        if (usr().use_file_ppqn())
            ppqn = SEQ66_USE_FILE_PPQN;         /* no usr().file_ppqn() yet */
        else
            ppqn = usr().default_ppqn();

        midifile * fp = is_wrk ? new wrkfile(fn, ppqn) : new midifile(fn, ppqn) ;
        std::unique_ptr<midifile> f(fp);
        p.clear_all();                          /* see banner notes         */
        result = f->parse(p, 0);
        if (result)
        {
            if (usr().use_file_ppqn())
            {
                ppqn = f->ppqn();               /* get & return file PPQN   */
                usr().file_ppqn(ppqn);          /* save the value from file */
            }
            p.set_ppqn(choose_ppqn(ppqn));      /* set chosen PPQN for MIDI */
            rc().last_used_dir(fn.substr(0, fn.rfind("/") + 1));
            rc().midi_filename(fn);             /* save current file-name   */
            rc().add_recent_file(fn);           /* Oli Kester's Kepler34!   */
            p.announce_playscreen();            /* tell MIDI control out    */
            file_message("Read MIDI file", fn);
        }
        else
        {
            errmsg = f->error_message();
            if (f->error_is_fatal())
                rc().remove_recent_file(fn);
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
        errmsg = "No file-name for write_midi_file()";
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


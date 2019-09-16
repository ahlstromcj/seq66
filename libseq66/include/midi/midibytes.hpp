#if ! defined SEQ66_MIDIBYTES_HPP
#define SEQ66_MIDIBYTES_HPP

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
 * \file          midibytes.hpp
 *
 *  This module declares a number of useful aliases (in place of the old C++
 *  stand-by for type definition).
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-09
 * \updates       2019-09-16
 * \license       GNU GPLv2 or above
 *
 *  These alias specifications are intended to remove the ambiguity we have
 *  seen between signed and unsigned values.  MIDI bytes and pulses, ticks, or
 *  clocks are, by their nature, unsigned, and we should enforce that.
 *
 *  One minor issue is why we didn't tack on "_t" to most of these types, to
 *  adhere to C conventions.  Well, no real reason except to save a couple
 *  characters and save some beauty.  Besides, it is easy to set up vim to
 *  highlight these new types in a special color, making them stand out easily
 *  while reading the code.
 *
 *  Also included are some small classes for encapsulating MIDI timing
 *  information.
 */

#include <vector>                       /* std::vector<midibool>            */
#include <string>
#include <climits>                      /* ULONG_MAX and other limits       */

#include "util/basic_macros.hpp"        /* insure build macros defined      */

/*
 *  Since we're using unsigned variables for counting pulses, we can't do the
 *  occasional test for negativity, we have to use wraparound.  One way is to
 *  use this macro.  However, we will probably just ignore the issue of
 *  wraparound.  With 32-bit longs, we have a maximum of 4,294,967,295.
 *  Even at an insane PPQN of 9600, that's almost 450,000 quarter notes.
 *  And for 64-bit code?  Forgeddaboudid!
 *
 *  #define IS_SEQ66_MIDIPULSE_WRAPAROUND(x)  ((x) > (ULONG_MAX / 2))
 */

/*
 *  This namespace is not documented because it screws up the document
 *  processing done by Doxygen.
 */

namespace seq66
{

/**
 *  Provides a fairly common type definition for a byte value.  This can be
 *  used for a MIDI buss/port number or for a MIDI channel number.
 */

using midibyte = unsigned char;

/**
 *  There are issues with using std::vector<bool>, so we need a type that can
 *  be returned by reference.  See the mutes class.
 */

using midibool = unsigned char;

/**
 *  Distinguishes a buss/bus number from other MIDI bytes.
 */

using bussbyte = unsigned char;

/**
 *  Distinguishes a short value from the unsigned short values implicit in
 *  short-valued MIDI numbers.
 */

using midishort = unsigned short;

/**
 *  Distinguishes a long value from the unsigned long values implicit in
 *  long-valued MIDI numbers.
 */

using midilong = unsigned long;

/**
 *  This value is used for representing pattern, mute-group, and automation
 *  operations.  It overlaps with automation::slot values.
 */

using ctrlop = unsigned short;

/**
 *  This value is used for representing pattern, mute-group, and automation
 *  keystrokes.
 */

using ctrlkey = unsigned;

/**
 *  Provides a way to save a sequence palette color in a single byte.
 *  This value is signed since we need a value of -1 to indicate no color, and
 *  0 to 127 to indicate the index that "points" to a palette color.
 */

using colorbyte = char;

/**
 *  We need a unique midipulse value that can be used to be indicate a bad,
 *  unusable pulse value (type definition midipulse).  This value should be
 *  modified if the alias of midipulse is changed.  For a signed long value,
 *  -1 can be used.  For an unsigned long value, ULONG_MAX is probably best.
 *  To avoid issues, when testing for this value, use the inline function
 *  is_null_midipulse().
 */

const long null_midipulse = (-1);       /* ULONG_MAX */

/**
 *  Distinguishes a long value from the unsigned long values implicit in MIDI
 *  time measurements.
 *
 *  HOWEVER, CURRENTLY, if you make this value unsigned, then perfroll won't
 *  show any notes in the sequence bars!!!  Also, a number of manipulations of
 *  this type currently depend upon it being a signed value.
 */

using midipulse = long;

/**
 *  Provides the data type for BPM (beats per minute) values.  This value used
 *  to be an integer, but we need to provide more precision in order to
 *  support better tempo matching.
 */

using midibpm = double;

/*
 *  Container types.  The next few type are common enough to warrant aliasing in
 *  this file.
 */

/**
 *  Provides a string specialization to explicitly use unsigned characters.
 */

using midistring = std::basic_string<midibyte>;

/**
 *  Provides a convenient way to package a number of booleans, such as
 *  mute-group values or a screenset's sequence statuses.
 */

using midibooleans = std::vector<midibool>;

/**
 *  Maximum and unusable values.  Use these value to avoid sign issues.
 *  Only c_midishort is used right now.  Also see null_midipulse.
 */

const midibyte c_midibyte_max = midibyte(0xFF);
const bussbyte c_bussbyte_max = bussbyte(0xFF);
const midishort c_midishort_max = midishort(0xFFFF);
const midilong c_midilong_max = midilong(0xFFFFFFFF);

/**
 *  Provides a data structure to hold the numeric equivalent of the measures
 *  string "measures:beats:divisions" ("m:b:d").  More commonly known as
 *  "bars:beats:ticks", or "BBT".
 */

class midi_measures
{

private:

    /**
     *  The integral number of measures in the measures-based time.
     */

    int m_measures;

    /**
     *  The integral number of beats in the measures-based time.
     */

    int m_beats;

    /**
     *  The integral number of divisions/pulses in the measures-based time.
     *  There are two possible translations of the two bytes of a division. If
     *  the top bit of the 16 bits is 0, then the time division is in "ticks
     *  per beat" (or “pulses per quarter note”). If the top bit is 1, then
     *  the time division is in "frames per second".  This member deals only
     *  with the ticks/beat definition.
     */

    int m_divisions;

public:

    midi_measures ();
    midi_measures (int measures, int beats, int divisions);

    void clear ()
    {
        m_measures = m_beats = m_divisions = 0;
    }

    /**
     * \getter m_measures
     */

    int measures () const
    {
        return m_measures;
    }

    /**
     * \setter m_measures
     *
     * \param m
     *      The value to which to set the number of measures.
     *      We can add validation later.
     */

    void measures (int m)
    {
        m_measures = m;
    }

    int beats () const
    {
        return m_beats;
    }

    /**
     * \setter m_beats
     *
     * \param b
     *      The value to which to set the number of beats.
     *      We can add validation later.
     */

    void beats (int b)
    {
        m_beats = b;
    }

    int divisions () const
    {
        return m_divisions;
    }

    /**
     * \setter m_divisions
     *
     * \param d
     *      The value to which to set the number of divisions.
     *      We can add validation later.
     */

    void divisions (int d)
    {
        m_divisions = d;
    }

};          // class midi_measures

/**
 *  We anticipate the need to have a small structure holding the parameters
 *  needed to calculate MIDI times within an arbitrary song.  Although
 *  Seq24/Seq66 currently are heavily dependent on hard-wired values,
 *  that will be rectified eventually, so let us get ready for it.
 */

class midi_timing
{
    /**
     *  This value should match the BPM value selected when editing the song.
     *  This value is most commonly set to 120, but is also read from the MIDI
     *  file.  This value is needed if one want to calculate durations in true
     *  time units such as seconds, but is not needed to calculate the number
     *  of pulses/ticks/divisions.
     */

    midibpm m_beats_per_minute;         /* T (tempo, BPM in upper-case)   */

    /**
     *  This value should match the numerator value selected when editing the
     *  sequence.  This value is most commonly set to 4.
     */

    int m_beats_per_measure;            /* B (bpm in lower-case)          */

    /**
     *  This value should match the denominator value selected when editing
     *  the sequence.  This value is most commonly set to 4, meaning that the
     *  fundamental beat unit is the quarter note.
     *
     */

    int m_beat_width;                   /* W (bw in lower-case)           */

    /**
     *  This value provides the precision of the MIDI song.  This value is
     *  most commonly set to 192, but is also read from the MIDI file.  We are
     *  still working getting "non-standard" values to work.
     */

    int m_ppqn;                         /* P (PPQN or ppqn)               */

public:

    midi_timing ();
    midi_timing (midibpm bpminute, int bpmeasure, int beatwidth, int ppqn);

    midibpm beats_per_minute () const
    {
        return m_beats_per_minute;
    }

    /**
     * \setter m_beats_per_minute
     *
     * \param b
     *      The value to which to set the number of beats/minute.
     *      We can add validation later.
     */

    void beats_per_minute (midibpm b)
    {
        m_beats_per_minute = b;
    }

    int beats_per_measure () const
    {
        return m_beats_per_measure;
    }

    /**
     * \setter m_beats_per_measure
     *
     * \param b
     *      The value to which to set the number of beats/measure.
     *      We can add validation later.
     */

    void beats_per_measure (int b)
    {
        m_beats_per_measure = b;
    }

    int beat_width () const
    {
        return m_beat_width;
    }

    /**
     * \setter m_beats_per_beat_width
     *
     * \param bw
     *      The value to which to set the number of beats in the denominator
     *      of the time signature.  We can add validation later.
     */

    void beat_width (int bw)
    {
        m_beat_width = bw;
    }

    int ppqn () const
    {
        return m_ppqn;
    }

    /**
     * \setter m_ppqn
     *
     * \param p
     *      The value to which to set the PPQN member.
     *      We can add validation later.
     */

    void ppqn (int p)
    {
        m_ppqn = p;
    }

};              // class midi_timing

/**
 *  Compares a midipulse value to null_midipulse.  By "null" in this
 *  case, we mean "unusable", not 0.  Sigh, it's always something.
 */

inline bool
is_null_midipulse (midipulse p)
{
    return p == null_midipulse;
}

}               // namespace seq66

#endif          // SEQ66_MIDIBYTES_HPP

/*
 * midibytes.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


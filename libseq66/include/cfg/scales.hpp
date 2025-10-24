#if ! defined SEQ66_SCALES_HPP
#define SEQ66_SCALES_HPP

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
 * \file          scales.hpp
 *
 *  This module declares/defines the scales-related global variables, types,
 *  and functions.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-23
 * \updates       2025-10-22
 * \license       GNU GPLv2 or above
 *
 *  These values were moved from the Seq64 globals module.  Includes the
 *  chord-generation data.
 *
 *  Phrygian scales added by user WinkoErades.  Thank you!
 */

#include <string>
#include <vector>                       /* holds the scale analyses results */

#include "midi/midibytes.hpp"           /* seq66::midibytes                 */
#include "util/basic_macros.hpp"        /* seq66::tokenization container    */

namespace seq66
{

class eventlist;                       /* forward reference                */

/**
 *  Provides the list of musical key signatures, using sharps.
 */

enum class keys
{
    C,                              /*  0 */
    Csharp,                         /*  1 */
    D,                              /*  2 */
    Dsharp,                         /*  3 */
    E,                              /*  4 */
    F,                              /*  5 */
    Fsharp,                         /*  6 */
    G,                              /*  7 */
    Gsharp,                         /*  8 */
    A,                              /*  9 */
    Asharp,                         /* 10 */
    B,                              /* 11 */
    max                             /* size of set */
};

/**
 *  A manifest constant for the normal number of semitones in an
 *  equally-tempered octave.
 */

const int c_octave_size = 12;

/**
 *  A constant for clarification of the value of zero, which, in the context
 *  of a musical key, is the default key of C.
 */

const int c_key_of_C = static_cast<int>(keys::C);

/**
 *  A constant for clarification of the value of zero, which, in the context
 *  of a musical key, is the default key of C.
 */

const int c_key_of_max = static_cast<int>(keys::max);

/**
 *  An inline function to test that an integer is a legal key-name index
 *  value.
 */

inline bool
legal_key (int k)
{
    return k >= c_key_of_C && k < c_octave_size;
}

inline keys
int_to_key (int k)
{
    return legal_key(k) ? static_cast<keys>(k) : keys::C ;
}

inline int
key_to_int (keys k)
{
    return static_cast<int>(k);
}

inline bool
legal_note (int note)
{
    return note >= 0 && note < 128;
}

/**
 *  Corresponds to the small number of musical scales that the application
 *  can handle.  Scales can be shown in the piano roll as gray bars for
 *  reference purposes.
 *
 *  We've added three more scales; there are still a number of them that could
 *  be fruitfully added to the list of scales.
 *
 *  It would be good to offload this stuff into a new "scale" class.
 *
 *  For now we peel off the "c_scale_" and let enum-class take care of the
 *  scope.
 */

enum class scales
{
    off,
    chromatic = off,
    major,
    minor,                          /* Natural Minor scale                  */
    harmonic_minor,
    melodic_minor,                  /* Just the ascending version           */
    c_whole_tone,
    minor_blues,
    major_pentatonic,
    minor_pentatonic,
    phrygian,
    enigmatic,
    diminished,
    dorian,
    mixolydian,                     /* Same as descending melodic minor     */
    max                             /* a "maximum" or "size of set" value   */
};

/**
 *  Avoids a cast in order to use scales::max as an initializer.
 */

const int c_scales_off = static_cast<int>(scales::off);

/**
 *  Avoids a cast in order to use scales::max as an array size.
 */

const int c_scales_max = static_cast<int>(scales::max);

/**
 *  An inline function to test that an integer in a legal scale value.
 */

inline bool
legal_scale (int s)
{
    return s >= c_scales_off && s < c_scales_max;

    /*
     * return s >= scale_to_int(scales::off) && s < scale_to_int(scales::max);
     */
}

inline scales
int_to_scale (int s)
{
    return legal_scale(s) ? static_cast<scales>(s) : scales::off ;
}

inline int
scale_to_int (scales s)
{
    return static_cast<int>(s);
}

/**
 *  Supported chords.
 */

enum class chords
{
    none, major, majb5, minor, minb5, sus2, sus4, aug, augsus4, tri, sixth,
    sixthsus4, sixthadd9, m6, m6add9, seventh, seventhsus4, seventh_5,
    seventhb5, seventh_9, seventhb9, seventh_5_9, seventh_5b9, seventhb5b9,
    seventhadd11, seventhadd13, seventh_11, maj7, maj7b5, maj7_5, maj7_11,
    maj7add13, m7, m7b5, m7b9, m7add11, m7add13, mmaj7, mmaj7add11,
    mmaj7add13,
    max
};

inline chords
int_to_chord (int c)
{
    return c >= 0 && c < static_cast<int>(chords::max) ?
        static_cast<chords>(c) : chords::none ;
}

inline int
chord_to_int (chords c)
{
    return static_cast<int>(c);
}

inline bool
legal_chord (int s)
{
    return s >= chord_to_int(chords::major) && s < chord_to_int(chords::max);
}

/**
 *  Provides the number of chord values in each chord's specification
 *  array.
 */

const int c_chord_number    = 40;
const int c_chord_size      =  6;
const int c_interval_size   = 15;
const int c_harmonic_size   =  8;

/**
 *  Provides a short vector containing the chord values in each chord's
 *  specification array.  The valid values stop at -1.
 */

using chord_notes = std::vector<int>;               /* all init'd to size 6 */

/*
 *  Free functions for scales.
 */

extern std::string musical_note_name (int n);
extern std::string musical_key_name (keys k);
extern std::string musical_scale_name (scales s);
extern const char * interval_name_ptr (int interval);
extern bool harmonic_number_valid (int number);
extern const char * harmonic_interval_name_ptr (int interval);
extern bool chord_number_valid (int number);
extern const char * chord_name_ptr (int number);
extern const chord_notes & chord_entry (int number);
extern std::string chord_intervals (chords c);
extern bool note_in_chord (chords chord, keys key, int note);
extern bool scales_policy (scales s, int k);
extern bool scales_policy (scales s, keys keyofpattern, int k);
extern const int * scales_up (int scale, int key = 0);
extern const int * scales_down (int scale, int key = 0);
extern double midi_note_frequency (midibyte note);
extern int analyze_notes
(
    const eventlist & evlist,
    std::vector<keys> & outkey,
    std::vector<scales> & outscale
);
extern std::string key_signature_string (int sfcount, bool isminor);
extern bool key_signature_bytes
(
    const std::string & keysigname,
    midibytes & keysigbytes
);
extern bool note_name_translation
(
    const std::string & notename,
    int & notenumber,
    int & octavenumber,
    int & basenumber
);
extern bool get_pitch_range
(
    const tokenization & values,
    int & lowest, int & highest
);

}           // namespace seq66

#endif      // SEQ66_SCALES_HPP

/*
 * scales.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

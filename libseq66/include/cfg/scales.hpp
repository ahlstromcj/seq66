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
 *  This module declares/defines the scales-related global variables and
 *  functions.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-23
 * \updates       2021-07-07
 * \license       GNU GPLv2 or above
 *
 *  These values were moved from the globals module.  Includes the
 *  chord-generation data.
 *
 *  Phrygian scales added by user WinkoErades.  Thank you!
 */

#include <string>
#include <vector>                       /* holds the scale analyses results */

#include "midi/midibytes.hpp"           /* seq66::midibytes                 */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

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
    major,
    minor,                          /* Natural Minor scale                  */
    harmonic_minor,
    melodic_minor,                  /* Just the ascending version           */
    c_whole_tone,
    blues,
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
}

/**
 *  Provides the number of chord values in each chord's specification
 *  array.
 */

const int c_chord_number    = 40;
const int c_chord_size      = 6;
const int c_interval_size   = 15;
const int c_harmonic_size   = 8;

/**
 *  Provides a short vector containing the chord values in each chord's
 *  specification array.  The valid values stop at -1.
 */

using chord_notes = std::vector<int>;               /* all init'd to size 6 */

/*
 *  Free functions for scales.
 */

extern std::string musical_key_name (int k);
extern std::string musical_note_name (int n);
extern std::string musical_scale_name (int s);
extern const char * interval_name_ptr (int interval);
extern bool harmonic_number_valid (int number);
extern const char * harmonic_interval_name_ptr (int interval);
extern bool chord_number_valid (int number);
extern const char * chord_name_ptr (int number);
extern const chord_notes & chord_entry (int number);
extern bool scales_policy (scales s, int k);
extern const int * scales_up (int scale, int key = 0);
extern const int * scales_down (int scale, int key = 0);
extern double midi_note_frequency (midibyte note);
extern int analyze_notes
(
    const eventlist & evlist,
    std::vector<keys> & outkey,
    std::vector<scales> & outscale
);

}           // namespace seq66

#endif      // SEQ66_SCALES_HPP

/*
 * scales.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


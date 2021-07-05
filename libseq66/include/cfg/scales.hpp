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
 * \updates       2021-07-05
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
 *  Provides the entries for the Interval dropdown menu in the Pattern Editor
 *  window.
 */

const std::string
c_interval_text [16] =
{
    "P1", "m2", "M2", "m3", "M3", "P4", "TT", "P5",
    "m6", "M6", "m7", "M7", "P8", "m9", "M9", ""
};

/**
 *  Provides the entries for the Chord dropdown menu in the Pattern Editor
 *  window.  However, we have not seen this menu in the GUI!  Ah, it only
 *  appears if the user has selected a musical scale like Major or Minor.
 */

const std::string
c_chord_text [8] =
{
    "I", "II", "III", "IV", "V", "VI", "VII", "VIII"
};

/**
 *  Additional support data for the chord-generation feature from Stazed's
 *  seq32 project.  The chord-number is a count of the number of entries in
 *  c_chord_table_text.  Will never change, luckily.
 */

const int c_chord_number = 40;

/**
 *  Additional support data for the chord-generation feature from Stazed's
 *  seq32 project.  These chords appear in the sequence-editor chord-button
 *  dropdown menu.  The longest string is 11 characters, and we add one
 *  for the null terminator.  A good case for using std::string here. :-)
 */

const std::string
c_chord_table_text [c_chord_number] =
{
    "Chords off",   "Major",       "Majb5",      "minor",   "minb5",
    "sus2",         "sus4",        "aug",        "augsus4", "tri",
    "6",            "6sus4",       "6add9",      "m6",      "m6add9",
    "7",            "7sus4",       "7#5",        "7b5",     "7#9",       "7b9",
    "7#5#9",        "7#5b9",       "7b5b9",      "7add11",  "7add13",    "7#11",
    "Maj7",         "Maj7b5",      "Maj7#5",     "Maj7#11", "Maj7add13",
    "m7",           "m7b5",        "m7b9",       "m7add11", "m7add13",
    "m-Maj7",       "m-Maj7add11", "m-Maj7add13"
};

/**
 *  Provides the number of chord values in each chord's specification
 *  array.
 */

const int c_chord_size = 6;

/**
 *  Additional support data for the chord-generation feature from Stazed's
 *  seq32 project.  These values indicate the note offsets needed for a
 *  particular kind of chord.  0 means no offset, and a -1 ends the list of
 *  note offsets for the chord.
 */

const int
c_chord_table [c_chord_number] [c_chord_size] =
{
    { 0, -1, 0, 0, 0, 0 },      /* Off          */
    { 0, 4, 7, -1, 0, 0 },      /* Major        */
    { 0, 4, 6, -1, 0, 0 },      /* Majb5        */
    { 0, 3, 7, -1, 0, 0 },      /* minor        */
    { 0, 3, 6, -1, 0, 0 },      /* minb5        */
    { 0, 2, 7, -1, 0, 0 },      /* sus2         */
    { 0, 5, 7, -1, 0, 0 },      /* sus4         */
    { 0, 4, 8, -1, 0, 0 },      /* aug          */
    { 0, 5, 8, -1, 0, 0 },      /* augsus4      */
    { 0, 3, 6, 9, -1, 0 },      /* tri          */
    { 0, 4, 7, 9, -1, 0 },      /* 6            */
    { 0, 5, 7, 9, -1, 0 },      /* 6sus4        */
    { 0, 4, 7, 9, 14, -1 },     /* 6add9        */
    { 0, 3, 7, 9, -1, 0 },      /* m6           */
    { 0, 3, 7, 9, 14, -1 },     /* m6add9       */
    { 0, 4, 7, 10, -1, 0 },     /* 7            */
    { 0, 5, 7, 10, -1, 0 },     /* 7sus4        */
    { 0, 4, 8, 10, -1, 0 },     /* 7#5          */
    { 0, 4, 6, 10, -1, 0 },     /* 7b5          */
    { 0, 4, 7, 10, 15, -1 },    /* 7#9          */
    { 0, 4, 7, 10, 13, -1 },    /* 7b9          */
    { 0, 4, 8, 10, 15, -1 },    /* 7#5#9        */
    { 0, 4, 8, 10, 13, -1 },    /* 7#5b9        */
    { 0, 4, 6, 10, 13, -1 },    /* 7b5b9        */
    { 0, 4, 7, 10, 17, -1 },    /* 7add11       */
    { 0, 4, 7, 10, 21, -1 },    /* 7add13       */
    { 0, 4, 7, 10, 18, -1 },    /* 7#11         */
    { 0, 4, 7, 11, -1, 0 },     /* Maj7         */
    { 0, 4, 6, 11, -1, 0 },     /* Maj7b5       */
    { 0, 4, 8, 11, -1, 0 },     /* Maj7#5       */
    { 0, 4, 7, 11, 18, -1 },    /* Maj7#11      */
    { 0, 4, 7, 11, 21, -1 },    /* Maj7add13    */
    { 0, 3, 7, 10, -1, 0 },     /* m7           */
    { 0, 3, 6, 10, -1, 0 },     /* m7b5         */
    { 0, 3, 7, 10, 13, -1 },    /* m7b9         */
    { 0, 3, 7, 10, 17, -1 },    /* m7add11      */
    { 0, 3, 7, 10, 21, -1 },    /* m7add13      */
    { 0, 3, 7, 11, -1, 0 },     /* m-Maj7       */
    { 0, 3, 7, 11, 17, -1 },    /* m-Maj7add11  */
    { 0, 3, 7, 11, 21, -1 }     /* m-Maj7add13  */
};

/*
 *  Free functions for scales.
 */

extern std::string musical_key_name (int k);
extern std::string musical_note_name (int n);
extern std::string musical_scale_name (int s);
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


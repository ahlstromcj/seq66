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
 * \file          scales.cpp
 *
 *  This module declares/defines functions related to scales.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-10-04
 * \updates       2021-07-05
 * \license       GNU GPLv2 or above
 *
 *  Here is a list of many scale interval patterns if working with
 *  more than just diatonic scales:
 *
\verbatim
    Major Scale (Ionian):           R   W   W   h   W   W   W   h
    Natural Minor Scale (Aeolian):  R   W   h   W   W   h   W   W
    Harmonic Minor Scale:           R   W   h   W   W   h   W+h h
    Melodic Minor Scale going up:   R   W   h   W   W   W   W   h
    Melodic Minor Scale going down: R   W   W   h   W   W   h   W
    Whole Tone:                     R   W   W   W   W   W   W
    Blues Scale:                    R   W+h W   h   h   W+h W
    Major Pentatonic:               R   W   W   W+h W   W+h
    Minor Pentatonic Blues (no #5): R   W+h W   W   W+h W
    Phrygian:                       R   h   W   W   W   h   W   W
    Enigmatic:                      R   h   W+h W   W   W   h   h
    Diminished                      R   W   h   W   h   W   h   W  h
\endverbatim
 *
 *  Unimplemented:
\verbatim
    Dorian Mode:                    R   W   h   W   W   W   h   W
    Mixolydian Mode:                R   W   W   h   W   W   h   W
    Phygian Dominant (Ahava Raba):  R   h   W+h h   W   h   W   W
    Octatonic 2:                    R   h   W   h   W   h   W   h
\endverbatim
 *
\verbatim
    R:      root note
    h:      half step (semitone)
    W:      whole step (2 semitones)
    W+h:    1 1/2 - a step and a half (3 semitones)
\endverbatim
 */

#include <cmath>                        /* for the pow() function           */

#include "cfg/scales.hpp"               /* seq66::scales declarations       */
#include "cfg/settings.hpp"             /* seq66::rc().verbose()            */
#include "midi/eventlist.hpp"           /* seq66::eventlist                 */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

static const std::string
c_key_text[c_octave_size] =
{
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

/**
 *  Each value in the kind of scale is denoted by a true value in these
 *  arrays.  See the following sites for more information:
 *
 *      -   http://method-behind-the-music.com/theory/scalesandkeys/
 *      -   https://en.wikipedia.org/wiki/Heptatonic_scale
 *      -   https://en.wikibooks.org/wiki/Music_Theory/Scales_and_Intervals
 *
 *  Note that melodic minor descends in the same way as the natural minor
 *  scale, so it descends differently than it ascends.  We don't deal with
 *  that trick, at all.  In the following table, the scales all start with C,
 *  but seq66 allow other starting notes (e.g. "keys").
 *
\verbatim
    Chromatic           C  C# D  D# E  F  F# G  G# A  A# B   Notes, chord
    Major               C  .  D  .  E  F  .  G  .  A  .  B
    Minor               C  .  D  Eb .  F  .  G  Ab .  Bb .
    Harmonic Minor      C  .  D  Eb .  F  .  G  Ab .  .  B
    Melodic Minor       C  .  D  Eb .  F  .  G  .  A  .  B   Descending diff.
    C Whole Tone        C  .  D  .  E  .  F# .  G# .  A# .   C+7 chord
    Blues               C  .  .  Eb .  F  Gb G  .  .  Bb .
    Major Pentatonic    C  .  D  .  E  .  .  G  .  A  .  .
    Minor Pentatonic    C  .  .  Eb .  F  .  G  .  .  Bb .
    Phrygian            C  Db .  Eb .  F  .  G  G# .  Bb .
    Enigmatic           C  Db .  .  E  .  F# .  G# .  A# B
    Diminished          C  .  D  Eb .  F  Gb .  Ab A  .  B
    Octatonic           C  Db .  Eb E  F  F# G  .  A  Bb .   Unimplemented
\endverbatim
 */

static const bool
c_scales_policy [c_scales_max] [c_octave_size] =
{
    {                                                   /* off = chromatic  */
        true, true, true, true, true, true,
        true, true, true, true, true, true
    },
    {                                                   /* major            */
        true, false, true, false, true, true,
        false, true, false, true, false, true
    },
    {                                                   /* minor            */
        true, false, true, true, false, true,
        false, true, true, false, true, false
    },
    {                                                   /* harmonic minor   */
        true, false, true, true, false, true,
        false, true, true, false, false, true
    },
    {                                                   /* melodic minor    */
        true, false, true, true, false, true,
        false, true, false, true, false, true
    },
    {                                                   /* whole tone       */
        true, false, true, false, true, false,
        true, false, true, false, true, false
    },
    {                                                   /* blues            */
        true, false, false, true, false, true,
        true, true, false, false, true, false
    },
    {                                                   /* maj pentatonic   */
        true, false, true, false, true, false,
        false, true, false, true, false, false
    },
    {                                                   /* min pentatonic   */
        true, false, false, true, false, true,
        false, true, false, false, true, false
    },
    {                                                   /* phrygian         */
        true, true, false, true, false, true,
        false, true, true, false, true, false
    },
    {                                                   /* enigmatic        */
        true, true, false, false, true, false,
        true, false, true, false, true, true
    },
    {                                                   /* diminished       */
        true, false, true, true, false, true,
        true, false, true, true, false, true
    }
};

bool
scales_policy (scales s, int k)
{
    return c_scales_policy[int(s)][(k - 1) % c_octave_size];
}

/**
 *  This function rotates a scale policy array to the "right" by one semitone.
 *  For example, see this shift from C major to C# major, where each dot
 *  represents a "false" boolean value:
 *
\verbatim
    C Major               C  .  D  .  E  F  .  G  .  A  .  B
    booleans              T  F  T  F  T  T  F  T  F  T  F  T
    histogram sample      1  0  2  0  2  0  1  1  0  1  0  2    = 9 - 1
                          +  .  +  .  +  +  -  +  .  +  .  +
                          C  C# D  D# E  F  F# G  G# A  A# B

    C# Major              C  C# .  D# .  F  F# .  G# .  A# .
    booleans              T  T  F  T  F  T  T  F  T  F  T  F
    histogram sample      1  0  2  0  2  0  1  1  0  1  0  2    = 2 - 8
\endverbatim
 */

static void
rotate_scale_right (bool p [c_octave_size])
{
    int lastindex = c_octave_size - 1;
    bool last = p[lastindex];                   /* p[11]            */
    for (int n = lastindex; n > 0; --n)         /* p[11] to p[1]    */
        p[n] = p[n - 1];

    p[0] = last;
}

static void
rotate_transpose_right (int p [c_octave_size])
{
    int lastindex = c_octave_size - 1;
    int last = p[lastindex];                    /* p[11]            */
    for (int n = lastindex; n > 0; --n)         /* p[11] to p[1]    */
        p[n] = p[n - 1];

    p[0] = last;
}

/**
 *  Increment values needed to transpose each scale up so that it remains in
 *  the exact same key, for harmonic transposition.  For example, if we simply
 *  add 1 semitone to each note, it remains a minor key, but it is in a
 *  different minor key.  Using the transpositions in these arrays, the minor
 *  key remains the same minor key.
 *
\verbatim
    Major               C  .  D  .  E  F  .  G  .  A  .  B
    Transpose up        2  .  2  .  1  2  .  2  .  2  .  1
    Result up           D  .  E  .  F  G  .  A  .  B  .  C
\endverbatim
 *
\verbatim
    Minor               C  .  D  D# .  F  .  G  G# .  A# .
    Transpose up        2  .  1  2  .  2  .  1  2  .  2  .
    Result up           D  .  D# F  .  G  .  G# A# .  C  .
\endverbatim
 *
\verbatim
    Harmonic minor      C  .  D  Eb .  F  .  G  Ab .  .  B
    Transpose up        2  .  1  2  .  2  .  1  3  .  .  1
    Result up           D  .  Eb F  .  G  .  Ab B  .  .  C
\endverbatim
 *
\verbatim
    Melodic minor       C  .  D  Eb .  F  .  G  .  A  .  B
    Transpose up        2  .  1  2  .  2  .  2  .  2  .  1
    Result up           D  .  Eb F  .  G  .  A  .  B  .  C
\endverbatim
 *
\verbatim
    C Whole Tone        C  .  D  .  E  .  F# .  G# .  A# .
    Transpose up        2  .  2  .  2  .  2  .  2  .  2  .
    Result up           D  .  E  .  F# .  G# .  A# .  C  .
\endverbatim
 *
\verbatim
    Blues               C  .  .  Eb .  F  Gb G  .  .  Bb .
    Transpose up        3  .  .  2  .  1  1  3  .  .  2  .
    Result up           Eb .  .  F  .  Gb G  Bb .  .  C  .
\endverbatim
 *
\verbatim
    Major Pentatonic    C  .  D  .  E  .  .  G  .  A  .  .
    Transpose up        2  .  2  .  3  .  .  2  .  3  .  .
    Result up           D  .  E  .  G  .  .  A  .  C  .  .
\endverbatim
 *
\verbatim
    Minor Pentatonic    C  .  .  Eb .  F  .  G  .  .  Bb .
    Transpose up        3  .  .  2  .  2  .  3  .  .  2  .
    Result up           Eb .  .  F  .  G  .  Bb .  .  C  .
\endverbatim
 *
 \verbatim
    Phrygian            C  Db .  Eb .  F  .  G  Ab .  Bb .
    Transpose up        1  2  .  2  .  2  .  1  2  .  2  .
    Result up           D  Eb .  F  .  G  .  A  Bb .  C  .
\endverbatim
 *
 \verbatim
    Enigmatic           C  Db .  .  E  .  F# .  G# .  A# B
    Transpose up        1  3  .  .  2  .  2  .  2  .  1  1
    Result up           Db E  .  .  F# .  G# .  A# .  B  C
\endverbatim
 *
 \verbatim
    Diminished          C  .  D  Eb .  F  Gb .  Ab A  .  B
    Transpose up        2  .  1  2  .  1  2  .  1  2  .  1
    Result up           D  .  Eb F  .  Gb Ab .  A  B  .  C
\endverbatim
 */

const int *
scales_up (int scale, int key)
{
    static const int c_scales_transpose_up [c_scales_max] [c_octave_size] =
    {
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},          /* off = chromatic  */
        { 2, 0, 2, 0, 1, 2, 0, 2, 0, 2, 0, 1},          /* major            */
        { 2, 0, 1, 2, 0, 2, 0, 1, 2, 0, 2, 0},          /* minor            */
        { 2, 0, 1, 2, 0, 2, 0, 1, 3, 0, 0, 1},          /* harmonic minor   */
        { 2, 0, 1, 2, 0, 2, 0, 2, 0, 2, 0, 1},          /* melodic minor    */
        { 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0},          /* C whole tone     */
        { 3, 0, 0, 2, 0, 1, 1, 3, 0, 0, 2, 0},          /* blues            */
        { 2, 0, 2, 0, 3, 0, 0, 2, 0, 3, 0, 0},          /* maj pentatonic   */
        { 3, 0, 0, 2, 0, 2, 0, 3, 0, 0, 2, 0},          /* min pentatonic   */
        { 1, 2, 0, 2, 0, 2, 0, 1, 2, 0, 2, 0},          /* phrygian         */
        { 1, 3, 0, 0, 2, 0, 2, 0, 2, 0, 1, 1},          /* enigmatic        */
        { 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1}           /* diminished       */
    };
    if (key > 0)
    {
        static int rotated_scale[c_octave_size];
        for (int k = 0; k < c_octave_size; ++k)
            rotated_scale[k] = c_scales_transpose_up[scale][k];

        for (int count = key; count > 0; --count)
            rotate_transpose_right(rotated_scale);

        return &rotated_scale[0];
    }
    else
        return &c_scales_transpose_up[scale][0];
}

/**
 *  Making these positive makes it easier to read, but the actual array
 *  contains negative values.
 *
\verbatim
    Major               C  .  D  .  E  F  .  G  .  A  .  B
    Transpose down      1  .  2  .  2  1  .  2  .  2  .  2
    Result down         B  .  C  .  D  E  .  F  .  G  .  A
\endverbatim
 *
\verbatim
    Minor               C  .  D  D# .  F  .  G  G# .  A# .
    Transpose down      2  .  2  1  .  2  .  2  1  .  2  .
    Result down         A# .  C  D  .  D# .  F  G  .  G# .
\endverbatim
 *
\verbatim
    Harmonic minor      C  .  D  Eb .  F  .  G  Ab .  .  B
    Transpose down      1  .  2  1  .  2  .  2  1  .  .  3
    Result down         B  .  C  D  .  Eb .  F  G  .  .  Ab
\endverbatim
 *
\verbatim
    Melodic minor       C  .  D  Eb .  F  .  G  .  A  .  B
    Transpose down      1  .  2  1  .  2  .  2  .  2  .  2
    Result down         B  .  C  D  .  Eb .  F  .  G  .  A
\endverbatim
 *
\verbatim
    C whole tone        C  .  D  .  E  .  F# .  G# .  A# .
    Transpose down      2  .  2  .  2  .  2  .  2  .  2  .
    Result down         A# .  C  .  D  .  E  .  F# .  G# .
\endverbatim
 *
\verbatim
    Blues               C  .  .  Eb .  F  Gb G  .  .  Bb .
    Transpose down      2  .  .  3  .  2  1  1  .  .  3  .
    Result down         Bb .  .  C  .  Eb F  Gb .  .  G  .
\endverbatim
 *
\verbatim
    Major Pentatonic    C  .  D  .  E  .  .  G  .  A  .  .
    Transpose down      3  .  2  .  2  .  .  3  .  2  .  .
    Result down         A  .  C  .  D  .  .  E  .  G  .  .
\endverbatim
 *
\verbatim
    Minor Pentatonic    C  .  .  Eb .  F  .  G  .  .  Bb .
    Transpose down      2  .  .  3  .  2  .  2  .  .  3  .
    Result down         Bb .  .  C  .  Eb .  F  .  .  G  .
\endverbatim
 *
  \verbatim
    Phrygian            C  Db .  Eb .  F  .  G  Ab .  Bb .
    Transpose down      1  1  .  1  .  1  .  1  1  .  1  .
    Result down         B  C  .  D  .  E  .  Fb G  .  A  .
\endverbatim
 *
  \verbatim
    Enigmatic           C  Db .  .  E  .  F# .  G# .  A# B
    Transpose down      1  1  .  .  3  .  2  .  2  .  2  1
    Result down         B  C  .  .  Db .  E  .  F# .  G# A#
\endverbatim
 *
  \verbatim
    Diminished          C  .  D  Eb .  F  Gb .  Ab A  .  B
    Transpose down      1  .  2  1  .  2  1  .  2  1  .  2
    Result down         B  .  C  D  .  Eb F  .  Gb Ab .  A
\endverbatim
 */

const int *
scales_down (int scale, int key)
{
    static const int c_scales_transpose_dn [c_scales_max] [c_octave_size] =
    {
        { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* off = chromatic */
        { -1,  0, -2,  0, -2, -1,  0, -2,  0, -2,  0, -2}, /* major (ionian)  */
        { -2,  0, -2, -1,  0, -2,  0, -2, -1,  0, -2,  0}, /* minor (aeolian) */
        { -1,  0, -2, -1,  0, -2,  0, -2, -1,  0,  0, -3}, /* harmonic minor  */
        { -1,  0, -2, -1,  0, -2,  0, -2,  0, -2,  0, -2}, /* melodic minor   */
        { -2,  0, -2,  0, -2,  0, -2,  0, -2,  0, -2,  0}, /* C whole tone    */
        { -2,  0,  0, -3,  0, -2, -1, -1,  0,  0, -3,  0}, /* blues           */
        { -3,  0, -2, -0,  2,  0,  0, -3,  0, -2,  0,  0}, /* maj pentatonic  */
        { -2,  0,  0, -3,  0, -2,  0, -2,  0,  0, -3,  0}, /* min pentatonic  */
        { -1, -1,  0, -1,  0, -1,  0, -1, -1,  0, -1,  0}, /* phrygian        */
        { -1, -1,  0,  0, -3,  0, -2,  0, -2,  0, -2, -1}, /* enigmatic       */
        { -1,  0, -2, -1,  0, -2, -1,  0, -2, -1,  0, -2}  /* diminished      */
    };
    if (key > 0)
    {
        static int rotated_scale[c_octave_size];
        for (int k = 0; k < c_octave_size; ++k)
            rotated_scale[k] = c_scales_transpose_dn[scale][k];

        for (int count = key; count > 0; --count)
            rotate_transpose_right(rotated_scale);

        return &rotated_scale[0];
    }
    else
        return &c_scales_transpose_dn[scale][0];
}

std::string
musical_note_name (int n)
{
    std::string result = "Xb";
    if (legal_note(n))
    {
        char note[8];
        int key = n % c_octave_size;
        int octave = (n / c_octave_size) - 1;
        if (octave >= 0)
        {
            snprintf
            (
                note, sizeof note, "%2s%1d", c_key_text[key].c_str(), octave
            );
        }
        else
            snprintf(note, sizeof note, "%2s-", c_key_text[key].c_str());

        result = note;
    }
    return result;
}

std::string
musical_key_name (int k)
{
    std::string result = "Xb";
    if (legal_key(k))
        result = c_key_text[k];

    return result;
}

std::string
musical_scale_name (int s)
{
    static const std::string
    c_scales_text[c_scales_max] =
    {
        "Off (Chromatic)",
        "Major (Ionian)",
        "Minor (Aeolan)",
        "Harmonic Minor",
        "Melodic Minor",
        "Whole Tone",
        "Blues",
        "Pentatonic Major",
        "Pentatonic Minor",
        "Phrygian",
        "Enigmatic",
        "Diminished"
    };
    std::string result = "Unsupported";
    if (legal_scale(s))
        result = c_scales_text[s];

    return result;
}

/**
 *  Convert a MIDI note number to frequency.  The formula, based on A4 (note
 *  69) being the concert pitch, 440 Hz, is:
 *
\verbatim
                   (n-69)/12
        f = 440 x 2
\endverbatim
 *
 * \return
 *      0.0 is returned upon an error.
 */

double
midi_note_frequency (midibyte note)
{
    double result = 0.0;
    if (note < c_midibyte_data_max)
    {
        double exponent = double(note) - 69.0 / 12.0;
        result = 440.0 * pow(2.0, exponent);
    }
    return result;
}

/**
 *  Analyzes one note to see what key (0 to 11, keys::C to keys::B) it is, and
 *  the octave (0 to 9) it resides in.
 *
\verbatim
      0: C-1
     12: C0
     24: C1
     36: C2
     48: C3
     60: C4 (middle C)
     72: C5
     84: C6
     96: C7
    108: C8
    120: C9
    127: G9
\endverbatim
 *
 * \return
 *      Returns true if the analysis was workable.
 */

static bool
analyze_note (midibyte note, keys & outkey, int & outoctave)
{
    bool result = note < c_midibyte_data_max;
    if (result)
    {
        unsigned base_semitone = unsigned(note) % c_octave_size;
        outkey = static_cast<keys>(base_semitone);
        outoctave = unsigned(note) / c_octave_size - 1;
    }
    return result;
}

static void
show_all_counts
(
    int histo [c_octave_size],
    int cmatrix [c_scales_max - 1] [c_key_of_max] [2]
)
{
    printf("       Histogram:   C   C#  D   D#  E   F   F#  G   G#  A   A#  B\n");
    printf("                  ");
    for (int k = 0; k < c_octave_size; ++k)
        printf(" %2d ", histo[k]);

    printf("\nOn-Scale:\n");
    for (int s = 0; s < c_scales_max - 1; ++s)
    {
        bool policy[c_octave_size];
        int scale = s + 1;
        printf("%16s: ", musical_scale_name(scale).c_str());
        for (int k = 0; k < c_octave_size; ++k)
            policy[k] = c_scales_policy[scale][k];      /* base (C) scale   */

        for (int K = 0; K < c_key_of_max; ++K)
        {
            printf(" %c%2d", '+', cmatrix[s][K][0]);
            rotate_scale_right(policy);
        }
        printf("\n");
    }
    printf("Corrected:\n");
    for (int s = 0; s < c_scales_max - 1; ++s)
    {
        bool policy[c_octave_size];
        int scale = s + 1;
        printf("%16s: ", musical_scale_name(scale).c_str());
        for (int k = 0; k < c_octave_size; ++k)
            policy[k] = c_scales_policy[scale][k];      /* base (C) scale   */

        for (int K = 0; K < c_key_of_max; ++K)
        {
            printf(" %c%2d", '%', cmatrix[s][K][0] - cmatrix[s][K][1]);
            rotate_scale_right(policy);
        }
        printf("\n");
    }
}

/**
 *  The algorithm is simple.  Get a histogram of the 12 semitones found in the
 *  event list.  Then see which key/scale combination contains the most of
 *  those semitones.  The key/scale combinations are in an array of dimension
 *  c_key_of_max x (c_scales_max -1).
 *
 *  We start with the major scale and C, and total up all the notes found for
 *  the valid notes in that scale, and record the count in
 *  count_matrix[major-1][C].  Then we move the scale up to C# and record the
 *  count in count_matrix[major-1][C#].
 *
 *  Another algorithm to consider is the Krumhansl-Schmuckler key-finding
 *  algorithm.
 *
 * \return
 *      Returns the number of keys/scales found.  If 0, the analysis was
 *      unworkable and the output parameters should not be used.
 */

int
analyze_notes
(
    const eventlist & evlist,
    std::vector<keys> & outkeys,
    std::vector<scales> & outscales
)
{
    int result = 0;
    bool ok = evlist.count() > 0;
    if (ok)
    {
        int histogram[c_octave_size];
        int highcount = 0;                  /* max count found on scale     */
        int notecount = 0;
        for (int k = 0; k < c_octave_size; ++k)
            histogram[k] = 0;

        for (auto e = evlist.cbegin(); e != evlist.cend(); ++e)
        {
            const event & er = eventlist::cdref(e);
            keys thekey;
            int theoctave;
            if (er.is_note_on())
            {
                midibyte note = er.get_note();
                ++notecount;
                ok = analyze_note(note, thekey, theoctave);
                if (ok)
                {
                    int k = static_cast<int>(thekey);
                    ++histogram[k];
                }
            }
        }
        if (notecount < 8)
        {
            infoprint("Not enough notes to analyze.");
            ok = false;
        }
        if (ok)
        {
            /*
             *  The first value is for "hits" and the second is for "misses".
             */

            int count_matrix [c_scales_max - 1] [c_key_of_max] [2];
            const std::initializer_list<keys> keyslist =
            {
                keys::C, keys::Csharp, keys::D, keys::Dsharp,
                keys::E, keys::F, keys::Fsharp, keys::G, keys::Gsharp,
                keys::A, keys::Asharp, keys::B
            };
            for (int s = 0; s < c_scales_max - 1; ++s)
                for (int k = 0; k < c_key_of_max; ++k)
                    count_matrix[s][k][0] = count_matrix[s][k][1] = 0;

            for (int s = 0; s < c_scales_max - 1; ++s)
            {
                bool policy[c_octave_size];
                for (int k = 0; k < c_octave_size; ++k)
                    policy[k] = c_scales_policy[s+1][k];  /* base (C) scale */

                for (auto ken : keyslist)
                {
                    int count_in = 0;
                    int count_out = 0;
                    int k = static_cast<int>(ken);

                    /*
                     * Here, we have the scale and it's (next) key.  For each
                     * policy value, add the count for each semitone in the
                     * histogram.
                     */

                    for (int bin = 0; bin < c_octave_size; ++bin)
                    {
                        if (policy[bin])
                            count_in += histogram[bin];
                        else
                            count_out += histogram[bin];
                    }
                    count_matrix[s][k][0] = count_in;
                    count_matrix[s][k][1] = count_out;
                    if (count_in > highcount)
                        highcount = count_in;

                    rotate_scale_right(policy);
                }
            }

            if (rc().verbose())
                show_all_counts(histogram, count_matrix);

            for (int s = 0; s < c_scales_max - 1; ++s)
            {
                for (auto ken : keyslist)
                {
                    int k = static_cast<int>(ken);
                    if (count_matrix[s][k][0] == highcount)
                    {
                        outscales.push_back(static_cast<scales>(s + 1));
                        outkeys.push_back(static_cast<keys>(k));
                        ++result;                   /* count it */
                    }
                }
            }
        }
    }
    return result;
}

}           // namespace seq66

/*
 * scales.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


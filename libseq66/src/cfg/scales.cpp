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
 * \updates       2023-05-04
 * \license       GNU GPLv2 or above
 *
 *  Here is a list of many scale interval patterns if working with
 *  more than just diatonic scales:
 *
\verbatim
    Major Scale (Ionian):           R-W-W-h-W-W-W-h
    Natural Minor Scale (Aeolian):  R-W-h-W-W-h-W-W
    Harmonic Minor Scale:           R-W-h-W-W-h-3-h
    Melodic Minor Scale going up:   R-W-h-W-W-W-W-h
 *  Melodic Minor Scale going down: R-W-W-h-W-W-h-W     (Mixolydian mode?)
    Whole Tone:                     R-W-W-W-W-W-W       (Augmented, Bebop)
    Minor Blues Scale:              R-3-W-h-h-3-W
 *  Major Blues Scale:              R-W-W-h-h-3-W-W     (Pentatonic Blues)
    Major Pentatonic:               R-W-W-3-W-3
    Minor Pentatonic Blues (no #5): R-3-W-W-3-W
    Phrygian:                       R-h-W-W-W-h-W-W
    Phygian Dominant (Ahava Raba):  R-h-3-h-W-h-W-W
    Enigmatic:                      R-h-3-W-W-W-h-h
    Diminished                      R-W-h-W-h-W-h-W-h
    Dorian Mode:                    R-W-h-W-W-W-h-W
    Mixolydian Mode:                R-W-W-h-W-W-h-W     (Dominant 7th)
    Dominant Diminished Scale:      R-h-W-h-W-h-W-h-W   (Diminished Blues)
\endverbatim
 *
\verbatim
    *: unimplemented in Seq66
    R: root note
    h: half step (semitone)
    W: whole step (2 semitones)
    3: 1 1/2 - a step and a half (3 semitones)
\endverbatim
 */

#include <cmath>                        /* for the pow() function           */

#include "cfg/scales.hpp"               /* seq66::scales declarations       */
#include "midi/eventlist.hpp"           /* seq66::eventlist                 */
#include "midi/midibytes.hpp"           /* seq66::midibytes                 */
#include "util/strfunctions.hpp"        /* seq66::contains()                */

#if defined USE_SHOE_ALL_COUNTS
#include "cfg/settings.hpp"             /* seq66::rc().verbose()            */
#endif

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  The minimum number of notes needed for a scale analysis.
 */

static const int c_analysis_minimum = 8;

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
    Minor               C  .  D  Eb .  F  .  G  Ab .  Bb .   Natural Minor
    Harmonic Minor      C  .  D  Eb .  F  .  G  Ab .  .  B
    Melodic Minor       C  .  D  Eb .  F  .  G  .  A  .  B   Descending diff.
    C Whole Tone        C  .  D  .  E  .  F# .  G# .  A# .   C+7 chord
    Minor Blues         C  .  .  Eb .  F  Gb G  .  .  Bb .   Gb = "blue note"
    Major Pentatonic    C  .  D  .  E  .  .  G  .  A  .  .
    Minor Pentatonic    C  .  .  Eb .  F  .  G  .  .  Bb .  See Minor Blues
    Phrygian            C  Db .  Eb .  F  .  G  G# .  Bb .
    Enigmatic           C  Db .  .  E  .  F# .  G# .  A# B
    Diminished          C  .  D  Eb .  F  Gb .  Ab A  .  B
    Dorian Mode         C  .  D  Eb .  F  .  G  .  A  Bb .
    Mixolydian Mode     C  .  D  .  E  F  .  G  .  A  Bb .
    Dominant Dimin'ed   C  Db .  Eb E  .  F# G  .  A  Bb    Diminished Blues
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
    {                                                   /* minor blues      */
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
    },
    {                                                   /* dorian           */
        true, false, true, true, false, true,
        false, true, false, true, true, false
    },
    {                                                   /* mixolydian       */
        true, false, true, false, true, true,
        false, true, false, true, true, false
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
    Minor Blues         C  .  .  Eb .  F  Gb G  .  .  Bb .
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
 *
 \verbatim
    Dorian Mode         C  .  D  Eb .  F  .  G  .  A  Bb .
    Transpose up        2  .  1  2  .  2  .  2  .  1  2  .
    Result up           D  .  Eb F  .  G  .  A  .  Bb C  .
\endverbatim
 *
 \verbatim
    Mixolydian Mode     C  .  D  .  E  F  .  G  .  A  Bb .
    Transpose up        2  .  2  .  1  2  .  2  .  1  2  .
    Result up           D  .  E  .  F  G  .  A  .  Bb .  C
\endverbatim
 *
 *  Note that the D Dorian scale is all white keys: D E F G A B C D.
 *  The "result up" shown above is for a transposition that preserves the
 *  C Dorian scale.
 *
 *  The Mixolydian scale starts on the 5th note of the Major scale and ends on
 *  the fifth note. For instance, the C Major scale is C, D, E, F, G, A, B,
 *  and C. The fifth note of C Major is G. Therefore the 5th mode of C Major
 *  is G Mixolydian: G, A, B, C, D, E, F, and G.  It is sometimes referred to
 *  as the Dominant 7th scale.
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
        { 3, 0, 0, 2, 0, 1, 1, 3, 0, 0, 2, 0},          /* minor blues      */
        { 2, 0, 2, 0, 3, 0, 0, 2, 0, 3, 0, 0},          /* maj pentatonic   */
        { 3, 0, 0, 2, 0, 2, 0, 3, 0, 0, 2, 0},          /* min pentatonic   */
        { 1, 2, 0, 2, 0, 2, 0, 1, 2, 0, 2, 0},          /* phrygian         */
        { 1, 3, 0, 0, 2, 0, 2, 0, 2, 0, 1, 1},          /* enigmatic        */
        { 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1},          /* diminished       */
        { 2, 0, 1, 2, 0, 2, 0, 2, 0, 1, 2, 0},          /* dorian           */
        { 2, 0, 2, 0, 1, 2, 0, 2, 0, 1, 2, 0}           /* mixolydian       */
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
    Semitone Number     1  2  3  4  5  6  7  8  9 10 11 12
    Chromatic (Off)     C  .  D  .  E  F  .  G  .  A  .  B
\endverbatim
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
    Minor Blues         C  .  .  Eb .  F  Gb G  .  .  Bb .
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
 *
 \verbatim
    Dorian Mode         C  .  D  Eb .  F  .  G  .  A  Bb .
    Transpose down      2  .  2  1  .  2  .  2  .  2  1  .
    Result down         Bb .  C  D  .  Eb .  F  .  G  A  .
\endverbatim
 *
 \verbatim
    Mixolydian Mode     C  .  D  .  E  F  .  G  .  A  Bb .
    Transpose down      2  .  2  .  2  1  .  2  .  2  1  .
    Result down         Bb .  C  .  D  E  .  F  .  G  A  .
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
        { -2,  0,  0, -3,  0, -2, -1, -1,  0,  0, -3,  0}, /* minor blues     */
        { -3,  0, -2, -0,  2,  0,  0, -3,  0, -2,  0,  0}, /* maj pentatonic  */
        { -2,  0,  0, -3,  0, -2,  0, -2,  0,  0, -3,  0}, /* min pentatonic  */
        { -1, -1,  0, -1,  0, -1,  0, -1, -1,  0, -1,  0}, /* phrygian        */
        { -1, -1,  0,  0, -3,  0, -2,  0, -2,  0, -2, -1}, /* enigmatic       */
        { -1,  0, -2, -1,  0, -2, -1,  0, -2, -1,  0, -2}, /* diminished      */
        { -2,  0, -2, -1,  0, -2,  0, -2,  0, -2, -1,  0}, /* dorian          */
        { -2,  0, -2,  0, -2,  1,  0, -2,  0, -2, -1,  0}  /* mixolydian      */
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

static const std::string
c_key_text[c_octave_size] =
{
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

std::string
musical_note_name (int n)
{
    std::string result = "Xb";
    if (legal_note(n))
    {
        char note[16];
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

/**
 *  Each diatonic scale in Western music consists of 7 notes.
 */

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
        "Melodic Minor",            /* ascending only; see Mixolydian mode  */
        "Whole Tone",
        "Minor Blues",
        "Pentatonic Major",
        "Pentatonic Minor",
        "Phrygian",
        "Enigmatic",
        "Diminished",
        "Dorian",
        "Mixolydian"
    };
    std::string result = "Unsupported";
    if (legal_scale(s))
        result = c_scales_text[s];

    return result;
}

/**
 *  Provides the entries for the normal pitch interval dropdown menu in the
 *  Pattern Editor window.  We use pointers for convenience.  See
 *  qseqeditframe64.cpp for usage.
 *
 *  "P" means "perfect"; "M" means "major"; "m" means "minor".
 *  This represents "chromatic transposition".
 */

const char *
interval_name_ptr (int interval)
{
    static const std::string c_interval_text [c_interval_size + 1] =
    {
        "P1", "m2", "M2", "m3", "M3", "P4", "TT", "P5",
        "m6", "M6", "m7", "M7", "P8", "m9", "M9", "0"   /* "0" if error */
    };
    int index = abs(interval);
    if (index > c_interval_size)
        index = c_interval_size;

    return c_interval_text[index].c_str();
}

/**
 *  Provides the entries for the harmonic pitch interval dropdown menu in the
 *  Pattern Editor window.  We use pointers for convenience.  See
 *  qseqeditframe64.cpp for usage.
 *
 *  Provides the entries for the Chord dropdown menu in the Pattern Editor
 *  window.  However, we have not seen this menu in the GUI!  Ah, it only
 *  appears if the user has selected a musical scale like Major or Minor.
 *  It replaces the -12 to +12 transposition menu.
 *
 *  This represents "diatonic transposition".  We believe the numbering is
 *  called the "Nashville numbering system."
 */

bool
harmonic_number_valid (int number)
{
    return abs(number) < c_harmonic_size;
}

/**
 *  Was: "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "0"
 */

const char *
harmonic_interval_name_ptr (int interval)
{
    static const std::string c_interval_text [c_harmonic_size + 1] =
    {
        "I", "ii", "iii", "IV", "V", "vi", "vii", "I", "0"
    };
    int index = abs(interval);
    if (index > c_harmonic_size)
        index = c_harmonic_size;

    return c_interval_text[index].c_str();
}

/**
 *  Additional support data for the chord-generation feature from Stazed's
 *  seq32 project.  The chord-number is a count of the number of entries in
 *  c_chord_table_text.  Will never change, luckily.
 *
 *  Additional support data for the chord-generation feature from Stazed's
 *  seq32 project.  These chords appear in the sequence-editor chord-button
 *  dropdown menu.  The longest string is 11 characters, and we add one
 *  for the null terminator.  A good case for using std::string here. :-)
 */

bool
chord_number_valid (int number)
{
    return (number >= 0) && (number < c_chord_number);
}

const char *
chord_name_ptr (int number)
{
    static const std::string c_chord_table_text [c_chord_number + 1] =
    {
        "Chords off",   "Major",       "Majb5",      "minor",   "minb5",
        "sus2",         "sus4",        "aug",        "augsus4", "tri",
        "6",            "6sus4",       "6add9",      "m6",      "m6add9",
        "7",            "7sus4",       "7#5",        "7b5",     "7#9",
        "7b9",          "7#5#9",       "7#5b9",      "7b5b9",   "7add11",
        "7add13",       "7#11",        "Maj7",       "Maj7b5",  "Maj7#5",
        "Maj7#11",      "Maj7add13",   "m7",         "m7b5",    "m7b9",
        "m7add11",      "m7add13",     "m-Maj7", "m-Maj7add11", "m-Maj7add13",
        ""              /* terminator */
    };
    if (! chord_number_valid(number))
        number = c_chord_number;

    return c_chord_table_text[number].c_str();
}

/**
 *  Additional support data for the chord-generation feature from Stazed's
 *  seq32 project.  These values indicate the note offsets needed for a
 *  particular kind of chord.  0 means no offset, and a -1 ends the list of
 *  note offsets for the chord.
 */

const chord_notes &
chord_entry (int number)
{
    static const std::vector<chord_notes> s_chord_table =
    {
        { 0, -1, 0,  0,   0,  0 },      /* Off          */
        { 0,  4, 7, -1,   0,  0 },      /* Major        */
        { 0,  4, 6, -1,   0,  0 },      /* Majb5        */
        { 0,  3, 7, -1,   0,  0 },      /* minor        */
        { 0,  3, 6, -1,   0,  0 },      /* minb5        */
        { 0,  2, 7, -1,   0,  0 },      /* sus2         */
        { 0,  5, 7, -1,   0,  0 },      /* sus4         */
        { 0,  4, 8, -1,   0,  0 },      /* aug          */
        { 0,  5, 8, -1,   0,  0 },      /* augsus4      */
        { 0,  3, 6,  9,  -1,  0 },      /* tri          */
        { 0,  4, 7,  9,  -1,  0 },      /* 6            */
        { 0,  5, 7,  9,  -1,  0 },      /* 6sus4        */
        { 0,  4, 7,  9,  14, -1 },      /* 6add9        */
        { 0,  3, 7,  9,  -1,  0 },      /* m6           */
        { 0,  3, 7,  9,  14, -1 },      /* m6add9       */
        { 0,  4, 7,  10, -1,  0 },      /* 7            */
        { 0,  5, 7,  10, -1,  0 },      /* 7sus4        */
        { 0,  4, 8,  10, -1,  0 },      /* 7#5          */
        { 0,  4, 6,  10, -1,  0 },      /* 7b5          */
        { 0,  4, 7,  10, 15, -1 },      /* 7#9          */
        { 0,  4, 7,  10, 13, -1 },      /* 7b9          */
        { 0,  4, 8,  10, 15, -1 },      /* 7#5#9        */
        { 0,  4, 8,  10, 13, -1 },      /* 7#5b9        */
        { 0,  4, 6,  10, 13, -1 },      /* 7b5b9        */
        { 0,  4, 7,  10, 17, -1 },      /* 7add11       */
        { 0,  4, 7,  10, 21, -1 },      /* 7add13       */
        { 0,  4, 7,  10, 18, -1 },      /* 7#11         */
        { 0,  4, 7,  11, -1,  0 },      /* Maj7         */
        { 0,  4, 6,  11, -1,  0 },      /* Maj7b5       */
        { 0,  4, 8,  11, -1,  0 },      /* Maj7#5       */
        { 0,  4, 7,  11, 18, -1 },      /* Maj7#11      */
        { 0,  4, 7,  11, 21, -1 },      /* Maj7add13    */
        { 0,  3, 7,  10, -1,  0 },      /* m7           */
        { 0,  3, 6,  10, -1,  0 },      /* m7b5         */
        { 0,  3, 7,  10, 13, -1 },      /* m7b9         */
        { 0,  3, 7,  10, 17, -1 },      /* m7add11      */
        { 0,  3, 7,  10, 21, -1 },      /* m7add13      */
        { 0,  3, 7,  11, -1,  0 },      /* m-Maj7       */
        { 0,  3, 7,  11, 17, -1 },      /* m-Maj7add11  */
        { 0,  3, 7,  11, 21, -1 }       /* m-Maj7add13  */
    };
    if (! chord_number_valid(number))
        number = 0;

    return s_chord_table[number];
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

#if defined USE_SHOE_ALL_COUNTS

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

#endif  // defined USE_SHOE_ALL_COUNTS

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
        if (notecount < c_analysis_minimum)
        {
            ok = false;     /* infoprint("Not enough notes to analyze.")    */
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

#if defined USE_SHOE_ALL_COUNTS
            if (rc().verbose())
                show_all_counts(histogram, count_matrix);
#endif

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

/**
 *  Returns the root note of a key signature.
 */

using key_sig_data = struct
{
    int sharp_flat_count;
    std::string major_root_note;
    std::string minor_root_note;
};

static key_sig_data s_key_sig_root_table [15] =
{
    {   -7,  "Cbmaj", "Abmin"     },
    {   -6,  "Gbmaj", "Ebmin"     },
    {   -5,  "Dbmaj", "Bbmin"     },
    {   -4,  "Abmaj", "Fmin"      },
    {   -3,  "Ebmaj", "Cmin"      },
    {   -2,  "Bbmaj", "Gmin"      },
    {   -1,  "Fmaj",  "Dmin"      },
    {    0,  "Cmaj",  "Amin"      },
    {    1,  "Gmaj",  "Emin"      },
    {    2,  "Dmaj",  "Bmin"      },
    {    3,  "Amaj",  "F#min"     },
    {    4,  "Emaj",  "C#min"     },
    {    5,  "Bmaj",  "G#min"     },
    {    6,  "F#maj", "D#min"     },
    {    7,  "C#maj", "A#min"     }
};

/**
 *  Returns the key-signature string.
 *
 * \param sfcount
 *      Provides the sharp/flat count, rangine from -7 to 7.
 *
 * \param isminor
 *      If true, the scale is minor, otherwise it is major.
 *
 * \return
 *      Returns the string via a fast lookup. It is empty if
 *      the sfcount is out of range.
 */

std::string
key_signature_string (int sfcount, bool isminor)
{
    std::string result;
    if (sfcount >= -7 && sfcount <= 7)
    {
        int index = sfcount + 7;
        const key_sig_data & ksd = s_key_sig_root_table[index];
        result = isminor ? ksd.minor_root_note : ksd.major_root_note ;
    }
    return result;
}

/**
 * \param keysigname
 *      The human-readable name of the key signature, from the above table.
 *
 * \param keysigbytes
 *      Returns the key-sig value, ranging from -7 to 7. This is a
 *      brute-force lookup. Defaults to 0 if the keysigname is not found.
 *
 * \return
 *      Returns true if the bytes can be used, because no error occurred.
 *      If false, then the keysigbytes parameter is empty [size() == 0].
 */

bool
key_signature_bytes
(
    const std::string & keysigname,
    midibytes & keysigbytes
)
{
    bool hasminor = contains(keysigname, "min");
    bool hasmajor = contains(keysigname, "maj");
    bool result = hasminor || hasmajor;
    keysigbytes.clear();
    if (result)
    {
        int sfcount = (-99);
        if (hasminor)
        {
            for (int i = 0; i < 15; ++i)
            {
                if (keysigname == s_key_sig_root_table[i].minor_root_note)
                {
                    sfcount = i - 7;
                    break;
                }
            }
        }
        else
        {
            for (int i = 0; i < 15; ++i)
            {
                if (keysigname == s_key_sig_root_table[i].major_root_note)
                {
                    sfcount = i - 7;
                    break;
                }
            }
        }
        if (sfcount != (-99))
        {
            keysigbytes.push_back(midibyte(sfcount));
            keysigbytes.push_back(hasminor ? midibyte(1) : 0);
        }
        else
            result = false;
    }
    return result;
}

}           // namespace seq66

/*
 * scales.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


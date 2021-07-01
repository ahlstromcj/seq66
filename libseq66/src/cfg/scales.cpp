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
 * \updates       2021-07-01
 * \license       GNU GPLv2 or above
 *
 *  Here is a list of many scale interval patterns if working with
 *  more than just diatonic scales:
 *
\verbatim
    Major Scale (Ionian):           R    W    W    h    W    W    W    h
    Natural Minor Scale (Aeolian):  R    W    h    W    W    h    W    W
    Harmonic Minor Scale:           R    W    h    W    W    h    W+h  h
    Melodic Minor Scale going up:   R    W    h    W    W    W    W    h
    Melodic Minor Scale going down: R    W    W    h    W    W    h    W
    Whole Tone:                     R    W    W    W    W    W    W
    Blues Scale:                    R    W+h  W    h    h    W+h  W
    Major Pentatonic:               R    W    W    W+h  W    W+h
    Minor Pentatonic Blues (no #5): R    W+h  W    W    W+h  W
    Phrygian:                       R    h    W    W    W    h    W    W
    Enigmatic:                      R    h    W+h  W    W    W    h    h
\endverbatim
 *
 *  Unimplemented:
\verbatim
    Dorian Mode:                    R    W    h    W    W    W    h    W
    Mixolydian Mode:                R    W    W    h    W    W    h    W
    Phygian Dominant (Ahava Raba):  R    h    W+h  h    W    h    W    W
    Octatonic 1:                    R    W    h    W    h    W    h    W   h
    Octatonic 2:                    R    h    W    h    W    h    W    h
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
#include "midi/eventlist.hpp"          /* seq66::eventlist                */

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
        "Enigmatic"
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

bool
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

/**
 *  This function rotates a scale policy array to the "right" by one semitone.
 *  For example, see this shift from C major to C# major, where each dot
 *  represents a "false" boolean value:
 *
\verbatim
    C Major               C  .  D  .  E  F  .  G  .  A  .  B
    booleans              t  f  t  f  t  t  f  t  f  t  f  t
    histogram sample      1  0  2  0  2  0  1  1  0  1  0  2    = 9 - 1
                          +  .  +  .  +  +  -  +  .  +  .  +
                          C  C# D  D# E  F  F# G  G# A  A# B

    C# Major              C  C# .  D# .  F  F# .  G# .  A# .
    booleans              t  t  f  t  f  t  t  f  t  f  t  f
    histogram sample      1  0  2  0  2  0  1  1  0  1  0  2    = 2 - 8
\endverbatim
 */

static void
increment_scale (bool p [c_octave_size])
{
    bool last = p[c_octave_size - 1];               // p[11]
    for (int n = c_octave_size - 2; n = 0; --n)     // p[10] down to p[0]
        p[n] = p[n - 1];

    p[0] = last;
}

static void
show_all_counts
(
    int histo [c_octave_size],
    int cmatrix [c_scales_max - 1] [c_key_of_max] [2]
)
{
    printf("         Overall:   C   C#  D   D#  E   F   F#  G   G#  A   A#  B\n");
    printf("                  ");
    for (int k = 0; k < c_octave_size; ++k)
        printf(" %2d ", histo[k]);

    printf("\n");
    for (int s = 0; s < c_scales_max - 1; ++s)
    {
        bool policy[c_octave_size];
        int scale = s + 1;
        printf("%16s: ", musical_scale_name(scale).c_str());
        for (int k = 0; k < c_octave_size; ++k)
            policy[k] = c_scales_policy[scale][k];      /* base (C) scale   */

        for (int K = 0; K < c_key_of_max; ++K)
        {
            char c = policy[K] ? '+' : '-' ;
            printf(" %c%2d", c, cmatrix[s][K][0]);
            increment_scale(policy);
        }

        printf("\n");
    }
#if 0
    for (int s = 0; s < c_scales_max - 1; ++s)
    {
        int scale = s + 1;
        printf("%16s: ", musical_scale_name(scale).c_str());
        for (int k = 0; k < c_key_of_max; ++k)
        {
            printf(" %2d:%2d", cmatrix[s][k][0], cmatrix[s][k][1]);
        }

        printf("\n");
    }
#endif
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
 *      Returns true if the analysis was workable.
 */

bool
analyze_notes (const eventlist & evlist, keys & outkey, scales & outscale)
{
    bool result = evlist.count() > 0;
    if (result)
    {
        int highscale = static_cast<int>(scales::max);
        int highkey = static_cast<int>(keys::max);
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
                result = analyze_note(note, thekey, theoctave);
                if (result)
                {
                    int k = static_cast<int>(thekey);
                    ++histogram[k];
                }
            }
        }
        if (notecount < 8)
        {
            infoprint("Not enough notes to analyze.");
            result = false;
        }
        if (result)
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
                {
                    policy[k] = c_scales_policy[s+1][k];  /* base (C) scale */
                }

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
                    {
                        highscale = s;
                        highkey = k;
                        highcount = count_in;
                    }
                    increment_scale(policy);
                }
            }
            outkey = static_cast<keys>(highkey);
            outscale = static_cast<scales>(highscale + 1);
            show_all_counts(histogram, count_matrix);
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


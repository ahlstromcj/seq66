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
 * \file          scales.cpp
 *
 *  This module declares/defines functions related to scales.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-10-04
 * \updates       2019-10-09
 * \license       GNU GPLv2 or above
 *
 *  Here is a list of all the scale interval patterns if you are working with
 *  more than just diatonic scales:
 *
\verbatim
    Major Scale:                    R, W, W, H, W, W, W, H
    Natural Minor Scale:            R, W, H, W, W, H, W, W
    Harmonic Minor Scale:           R, W, H, W, W, H, X, H
    Melodic Minor Scale going up:   R, W, H, W, W, W, W, H
    Melodic Minor Scale going down: R, W, W, H, W, W, H, W
    Dorian Mode is:                 R, W, H, W, W, W, H, W
    Mixolydian Mode is:             R, W, W, H, W, W, H, W
    Ahava Raba Mode is:             R, H, X, H, W, H, W, W
    Minor Pentatonic Blues (no #5): R, X, W, W, X, W
\endverbatim
 *
\verbatim
    R: root
    W: whole step
    H: half step
    X: 1 1/2 - a step and a half
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

/**
 *
 */

std::string
musical_key_name (int k)
{
    std::string result = "Unsupported";
    if (legal_key(k))
        result = c_key_text[k];

    return result;
}

/**
 *
 */

std::string
musical_scale_name (int s)
{
    std::string result = "Unsupported";
    if (legal_scale(s))
        result = c_scales_text[s];

    return result;
}

/**
 *  Convert a MIDI note number to frequency.  The formula, based on A4 (note 69)
 *  being the concert pitch, 440 Hz, is:
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
 *  the octave (0 to xxxx) it resides in.
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
 *
 * \return
 *      Returns true if the analysis was workable.
 */

bool
analyze_note
(
    midibyte note,
    keys & outkey,
    int & outoctave
)
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
 *  Analyzes a set of notes to see what key and scale best fits the notes.
 *
 *  To get the kind of scale, we first need to go through each of the
 *  c_scales_max values in the the c_scales_policy[] array and see if the
 *  booleans in the scratchpad match.  If so, we have a match for a C scale.
 *  Otherwise, go to the next scales value, shift/rotate the scratchpad
 *  leftward, and look for a match.
 *
 * \return
 *      Returns true if the analysis was workable.
 */

bool
analyze_notes
(
    const eventlist & evlist,
    keys & outkey,
    scales & outscale
)
{
    bool result = evlist.count() > 0;
    if (result)
    {
        midi_booleans scratchpad(c_octave_size);
        int notecount = 0;
        for (auto e = evlist.cbegin(); e != evlist.cend(); ++e)
        {
            const event & er = eventlist::cdref(e);
            keys thekey;
            int theoctave;
            if (er.is_note_on())
            {
                ++notecount;
                midibyte note = er.get_note();
                result = analyze_note(note, thekey, theoctave);
                if (result)
                {
                    int n = static_cast<int>(thekey);
                    scratchpad[n] = midibool(true);
                }
                else
                    break;
            }
        }
        if (notecount == 0)
            result = false;

        if (result)
        {
            const /*constexpr*/ std::initializer_list<keys> keyslist =
            {
                keys::C, keys::Csharp, keys::D, keys::Dsharp,
                keys::E, keys::F, keys::Fsharp, keys::G, keys::Gsharp,
                keys::A, keys::Asharp, keys::B
            };
            result = false;
            for (auto ken : keyslist)
            {
                for (int s = c_scales_off; s < c_scales_max; ++s)
                {
                    midi_booleans policy(&c_scales_policy[s][0], c_octave_size);
                    if (scratchpad.match(policy, scratchpad.true_count()))
                    {
                        outkey = ken;
                        outscale = static_cast<scales>(s);
                        result = true;
                        break;
                    }

                    std::string scratchprint = scratchpad.fingerprint();
                    std::string policyprint = policy.fingerprint();
                    int k = static_cast<int>(ken);
                    printf
                    (
                        "key %s (%d), scale %s (%d): "
                        "fingerprint: %s; policy: %s\n",
                        musical_key_name(k).c_str(), k,
                        musical_scale_name(s).c_str(), s,
                        scratchprint.c_str(), policyprint.c_str()
                    );
                }
                if (result)
                    break;
                else
                    scratchpad.rotate(1);
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


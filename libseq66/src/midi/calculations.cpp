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
 * \file          calculations.cpp
 *
 *  This module declares/defines some utility functions and calculations
 *  needed by this application.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-11-07
 * \updates       2023-08-16
 * \license       GNU GPLv2 or above
 *
 *  This code was moved from the globals module so that other modules
 *  can include it only if they need it.
 *
 *  To convert the ticks for each MIDI note into a millisecond value to
 *  display the notes visually along a timeline, one needs to use the division
 *  and the tempo to determine the value of an individual tick.  That
 *  conversion looks like:
 *
\verbatim
        1 min    60 sec   1 beat     Z clocks
       ------- * ------ * -------- * -------- = seconds
       X beats   1 min    Y clocks       1
\endverbatim
 *
 *  X is the tempo (beats per minute, or BPM), Y is the division (pulses per
 *  quarter note, PPQN), and Z is the number of clocks from the incoming
 *  event. All of the units cancel out, yielding a value in seconds. The
 *  condensed version of that conversion is therefore:
 *
\verbatim
        (60 * Z) / (X * Y) = seconds
        seconds = 60 * clocks / (bpm * ppqn)
\endverbatim
 *
 *  The value given here in seconds is the number of seconds since the
 *  previous MIDI event, not from the sequence start.  One needs to keep a
 *  running total of this value to construct a coherent sequence.  Especially
 *  important if the MIDI file contains tempo changes.  Leaving the clocks (Z)
 *  out of the equation yields the periodicity of the clock.
 *
 *  The inverse calculation is:
 *
\verbatim
        clocks = seconds * bpm * ppqn / 60
\endverbatim
 *
 * \todo
 *      There are additional user-interface and MIDI scaling variables in the
 *      perfroll module that we need to move here.
 */

#include <cctype>                       /* std::isspace(), std::isdigit()   */
#include <cmath>                        /* std::floor(), std::log()         */
#include <cstdlib>                      /* std::atoi(), std::strtol()       */
#include <cstring>                      /* std::memset()                    */
#include <ctime>                        /* std::strftime()                  */
#include <random>                       /* std::uniform_int_distribution    */

#include "cfg/settings.hpp"
#include "midi/calculations.hpp"        /* MIDI-related calculations        */
#include "util/strfunctions.hpp"        /* seq66::contains(), etc.          */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  This value represent the smallest horizontal unit in a Sequencer66 grid.
 *  It is the number of pixels in the smallest increment between vertical
 *  lines in the grid.  For a zoom of 2, this number gets doubled.
 */

static const int c_pixels_per_substep = 6;

/**
 *  This value represents the fundamental and default beats-per-bar.
 */

static const int c_qn_beats = 4;

/**
 *  Convenience function. We don't want to use seq66::string_to_int()
 *  because that uses a leading "0" or "0x" to determine the base of the
 *  conversions.
 */

static int
strtoi (const std::string & v)
{
    return v.empty() ? 0 : std::atoi(v.c_str());
}

/**
 *  Extracts up to 4 numbers from a colon-delimited string, or 1 from a
 *  non-delimited string.  Actually colon or period are used.
 *
 *      -   measures : beats : divisions
 *          -   "8" represents solely the number of pulses.  That is, if the
 *              user enters a single number, it is treated as the number of
 *              pulses.
 *          -   "8:1" represents a measure and a beat.
 *          -   "213:4:920"  represents a measure, a beat, and pulses.
 *      -   hours : minutes : seconds . fraction.  We really don't support
 *          this concept at present.  Beware!
 *          -   "2:04:12.14"
 *          -   "0:1:2"
 *
 * \warning
 *      This is not the most efficient implementation you'll ever see.
 *      At some point we will tighten it up.
 *
 * \param s
 *      Provides the input time string, in measures or time format,
 *      to be processed.
 *
 * \param [out] part_1
 *      The destination reference for the first part of the time.
 *      In some contexts, this number alone is a pulse (ticks) value;
 *      in other contexts, it is a measures value.
 *
 * \param [out] part_2
 *      The destination reference for the second part of the time.
 *
 * \param [out] part_3
 *      The destination reference for the third part of the time.
 *
 * \param [out] fraction
 *      The destination reference for the fractional part of the time.
 *
 * \return
 *      Returns the number of parts provided, ranging from 0 to 4.
 *      If 0, there is an error. If 1, it is assumed to be an single number,
 *      such as 768.
 */

int
extract_timing_numbers
(
    const std::string & s,
    std::string & part_1,
    std::string & part_2,
    std::string & part_3,
    std::string & fraction
)
{
    tokenization tokens;
    int count = tokenize_string(s, tokens); /* a function in this module    */
    part_1.clear();
    part_2.clear();
    part_3.clear();
    fraction.clear();
    if (count > 0)
        part_1 = tokens[0];

    if (count > 1)
        part_2 = tokens[1];

    if (count > 2)
        part_3 = tokens[2];

    if (count > 3)
        fraction = tokens[3];

    return count;
}

/**
 *  Tokenizes a string using the colon, space, or period as delimiters.  They
 *  are treated equally, and the caller must determine what to do with the
 *  parts.  Here are the steps:
 *
 *      -#  Skip any delimiters found at the beginning.  The position will
 *          either exist, or there will be nothing to parse.
 *      -#  Get to the next delimiter.  This will exist, or not.  Get all
 *          non-delimiters until the next delimiter or the end of the string.
 *      -#  Repeat until no more delimiters exist.
 *
 * \param source
 *      The string to be parsed and tokenized.
 *
 * \param tokens
 *      Provides a vector into which to push the tokens.
 *
 * \return
 *      Returns the number of tokens pushed (i.e. the final size of the tokens
 *      vector).
 */

int
tokenize_string
(
    const std::string & source,
    tokenization & tokens
)
{
    static std::string s_delims = ":. ";
    int result = 0;
    tokens.clear();
    auto pos = source.find_first_not_of(s_delims);
    if (pos != std::string::npos)
    {
        for (;;)
        {
            auto depos = source.find_first_of(s_delims, pos);
            if (depos != std::string::npos)
            {
                tokens.push_back(source.substr(pos, depos - pos));
                pos = source.find_first_not_of(s_delims, depos +1);
                if (pos == std::string::npos)
                    break;
            }
            else
            {
                tokens.push_back(source.substr(pos));
                break;
            }
        }
        result = int(tokens.size());
    }
    return result;
}

/**
 *  Converts MIDI pulses (also known as ticks, clocks, or divisions) into a
 *  string.
 *
 * \param p
 *      The MIDI pulse/tick value to be converted.
 *
 * \return
 *      Returns the string as an unsigned ASCII integer number.
 */

std::string
pulses_to_string (midipulse p)
{
    char tmp[32];
    snprintf(tmp, sizeof tmp, "%lu", (unsigned long)(p));
    return std::string(tmp);
}

/**
 *  Converts a MIDI pulse/ticks/clock value into a string that represents
 *  "measures:beats:ticks" ("measures:beats:division").
 *
 * \param p
 *      The number of MIDI pulses (clocks, divisions, ticks, you name it) to
 *      be converted.  If the value is c_null_midipulse, it is converted
 *      to 0, because callers don't generally worry about such niceties, and
 *      the least we can do is convert illegal measure-strings (like
 *      "000:0:000") to a legal value.
 *
 * \param seqparms
 *      This small structure provides the beats/measure, beat-width, and PPQN
 *      that hold for the sequence involved in this calculation.  These values
 *      are needed in the calculations.
 *
 * \return
 *      Returns the string, in measures notation, for the absolute pulses that
 *      mark this duration.
 */

std::string
pulses_to_measurestring (midipulse p, const midi_timing & seqparms)
{
    midi_measures measures;                 /* measures, beats, divisions   */
    char tmp[32];
    if (is_null_midipulse(p))
        p = 0;                              /* punt the runt!               */

    pulses_to_midi_measures(p, seqparms, measures); /* fill measures struct */
    snprintf
    (
        tmp, sizeof tmp, "%03d:%d:%03d",
        measures.measures(), measures.beats(), measures.divisions()
    );
    return std::string(tmp);
}

/**
 *  Converts a MIDI pulse/ticks/clock value into a string that represents
 *  "measures:beats:ticks" ("measures:beats:division").
 *
\verbatim
        m = p * W / (4 * P * B)
\endverbatim
 *
 * \param p
 *      Provides the MIDI pulses (as in "pulses per quarter note") that are to
 *      be converted to MIDI measures format.
 *
 * \param seqparms
 *      This small structure provides the beats/measure (B), beat-width (W),
 *      and PPQN (P) that hold for the sequence involved in this calculation.
 *      The beats/minute (T for tempo) value is not needed.
 *
 * \param [out] measures
 *      Provides the current MIDI song time structure to hold the results,
 *      which are the measures, beats, and divisions values for the time of
 *      interest.  Note that the measures and beats are corrected to be re 1,
 *      not 0.
 *
 * \return
 *      Returns true if the calculations were able to be made.  The P, B, and
 *      W values all need to be greater than 0.
 */

bool
pulses_to_midi_measures
(
    midipulse p,
    const midi_timing & seqparms,
    midi_measures & measures
)
{
    int W = seqparms.beat_width();
    int P = seqparms.ppqn();
    int B = seqparms.beats_per_measure();
    bool result = (W > 0) && (P > 0) && (B > 0);
    if (result)
    {
        double qnotes = double(c_qn_beats) * B / W; /* Q notes per measure  */
        double measlength = P * qnotes;             /* pulses/std measure   */
        int beatticks = measlength / B;             /* pulses/beat          */
        int m = int(p / measlength) + 1;            /* measure no. of pulse */
        int metro = 1 + ((p * W / P / c_qn_beats ) % B);
        measures.measures(m);                       /* number of measures   */
        measures.beats(metro);                      /* beats within measure */
        measures.divisions(int(p % beatticks));     /* leftover pulses      */
    }
    return result;
}

/**
 *  Function used in sequence::analyze_time_signatures() to precalculate
 *  the size of each time-signature (sequence::timesig) segment.
 *
 * \param p
 *      Provides either the time in ticks (pulses), or the duration of a
 *      timesig segment.
 *
 * \param P
 *      The PPQN in force for this calculation. Must be greater than 0.
 *
 * \param B
 *      The beats/measure in force for this calculation. Must be greater
 *      than 0.
 *
 * \param W
 *      The beat width in force for this calculation. Must be greater
 *      than 0.
 *
 * \return
 *      If the parameters are valid, returns the measure count or size
 *      as a floating-point value.  The caller is responsible for any
 *      rounding.  If parameters are invalid, 0.0 is returned.
 */

double
pulses_to_measures
(
    midipulse p,                                    /* time or duration     */
    int P,                                          /* PPQN                 */
    int B,                                          /* beats per measure    */
    int W                                           /* beat width           */
)
{
    double result = 0.0;                            /* indicates an error   */
    bool ok = (W > 0) && (P > 0) && (B > 0);
    if (ok)
    {
        double qnotes = double(c_qn_beats) * B / W; /* Q notes per measure  */
        double measlength = P * qnotes;             /* pulses/std measure   */
        result = p / measlength;
    }
    return result;
}

/**
 *  Converts a MIDI pulse/ticks/clock value into a string that represents
 *  "hours:minutes:seconds.fraction".  See the other pulses_to_time_string()
 *  overload.
 *
 * \param p
 *      Provides the number of ticks, pulses, or divisions in the MIDI
 *      event time.
 *
 * \param timinginfo
 *      Provides the tempo of the song, in beats/minute, and the
 *      pulse-per-quarter-note of the song.
 *
 * \return
 *      Returns the return-value of the other pulses_to_time_string() function.
 */

std::string
pulses_to_time_string (midipulse p, const midi_timing & timinginfo)
{
    return pulses_to_time_string
    (
        p, timinginfo.beats_per_minute(), timinginfo.ppqn()
    );
}

/**
 *  Converts a MIDI pulse/ticks/clock value into a string that represents
 *  "hours:minutes:seconds.fraction".  If the fraction part is 0, then it is
 *  not shown.  Examples:
 *
 *      -   "0:0:0"
 *      -   "0:0:0.102333"
 *      -   "12:3:1"
 *      -   "12:3:1.000001"
 *
 * \param p
 *      Provides the number of ticks, pulses, or divisions in the MIDI
 *      event time.
 *
 * \param bpm
 *      Provides the tempo of the song, in beats/minute.
 *
 * \param ppqn
 *      Provides the pulses-per-quarter-note of the song.
 *
 * \param showus
 *      If true (the default), shows the microseconds as well.  Hours are now
 *      shown only if greater than zero.
 *
 * \return
 *      Returns the time-string representation of the pulse (ticks) value.
 */

std::string
pulses_to_time_string (midipulse p, midibpm bpm, int ppqn, bool showus)
{
    unsigned long microseconds = ticks_to_delta_time_us(p, bpm, ppqn);
    int seconds = int(microseconds / 1000000UL);
    int minutes = seconds / 60;
    int hours = seconds / (60 * 60);
    int hoursecs = hours * 60 * 60;
    int minutesecs = minutes * 60;
    minutes -= hours * 60;
    seconds -= hoursecs + minutesecs;

    char tmp[48];
    if (showus)
    {
        microseconds -= (hoursecs + minutesecs + seconds) * 1000000UL;
        microseconds /= 10000L;
        if (hours > 0)
        {
            snprintf
            (
                tmp, sizeof tmp, "%d:%02d:%02d.%02lu",
                hours, minutes, seconds, microseconds
            );
        }
        else
        {
            snprintf
            (
                tmp, sizeof tmp, "%02d:%02d.%02lu",
                minutes, seconds, microseconds
            );
        }
    }
    else
    {
        /*
         * Why the spaces?  It is inconsistent.  But see the
         * timestring_to_pulses() function first.
         */

        if (hours > 0)
        {
            snprintf
            (
                tmp, sizeof tmp, "%d:%02d:%02d   ", hours, minutes, seconds
            );
        }
        else
            snprintf(tmp, sizeof tmp, "%02d:%02d   ", minutes, seconds);
    }
    return std::string(tmp);
}

/**
 *  A handy function for checking for long songs (an hour or more).
 */

int
pulses_to_hours (midipulse p, midibpm bpm, int ppqn)
{
    unsigned long microseconds = ticks_to_delta_time_us(p, bpm, ppqn);
    int seconds = int(microseconds / 1000000UL);
    return seconds / (60 * 60);
}

/**
 *  Converts a string that represents "measures:beats:division" (also known
 *  as "B:B:T") to a MIDI pulse/ticks/clock value. Note that, here, "division"
 *  is simply a number of pulses less than a beat.
 *
 *  If the third value (the MIDI pulses or ticks value) is set to the dollar
 *  sign ("$"), then the pulses are set to PPQN-1, as a handy shortcut to
 *  indicate the end of the beat.
 *
 * \warning
 *      If only one number is provided, it is treated in this function like
 *      a measures value, not a pulses value.
 *
 * \param measures
 *      Provides the current MIDI song time in "measures:beats:divisions"
 *      format, where divisions are the MIDI pulses in
 *      "pulses-per-quarter-note".
 *
 * \param seqparms
 *      This small structure provides the beats/measure, beat-width, and PPQN
 *      that hold for the sequence involved in this calculation.
 *
 * \return
 *      Returns the absolute pulses that mark this duration.  If the input
 *      string is empty, then 0 is returned.
 */

midipulse
measurestring_to_pulses
(
    const std::string & measures,
    const midi_timing & seqparms
)
{
    midipulse result = 0;
    if (! measures.empty())
    {
        std::string m, b, d, dummy;
        int valuecount = extract_timing_numbers(measures, m, b, d, dummy);
        if (valuecount >= 1)
        {
            midi_measures meas_values;                      /* 0 in ctor    */
            meas_values.measures(strtoi(m));
            if (valuecount > 1)
            {
                meas_values.beats(strtoi(b));
                if (valuecount > 2)
                {
                    if (d == "$")
                        meas_values.divisions(seqparms.ppqn() - 1);
                    else
                        meas_values.divisions(strtoi(d));
                }
            }
            result = midi_measures_to_pulses(meas_values, seqparms);
        }
    }
    return result;
}

/**
 *  Converts a string that represents "measures:beats:division" to a MIDI
 *  pulse/ticks/clock value.
 *
 *  p = 4 * P * m * B / W
 *      p == pulse count (ticks or pulses)
 *      m == number of measures
 *      B == beats per measure (constant)
 *      P == pulses per quarter-note (constant)
 *      W == beat width in beats per measure (constant) [NOT CORRECT]
 *
 *  Note that the 0-pulse MIDI measure is "1:1:0", which means "at the
 *  beginning of the first beat of the first measure, no pulses'.  It is not
 *  "0:0:0" as one might expect.
 *
 *  If we get a 0 for measures or for beats, we
 *  treat them as if they were 1.  It is too easy for the user to mess up.
 *
 *  We should consider clamping the beats to the beat-width value as well.
 *
 *  Example: Current time-signature = 3/16. Then qn_per_beat = 4/16 = 0.25.
 *  For 1 measure and 3 beats, the pulses are p = 1 * 3 * 0.25 * PPQN. If PPQN
 *  is 192, the pulses per beat are 0.25 * PPQN = 48.
 *
 * \param measures
 *      Provides the current MIDI song time structure holding the
 *      measures, beats, and divisions values for the time of interest.
 *      Note that it does not employ beat-width. It is a time position.
 *
 * \param seqparms
 *      This small structure provides the beats/minute, beats/measure,
 *      beat-width, and PPQN that hold for the sequence in this calculation.
 *
 * \return
 *      Returns the absolute pulses that mark this duration.  If the
 *      pulse-value cannot be calculated, then c_null_midipulse is
 *      returned.
 */

midipulse
midi_measures_to_pulses
(
    const midi_measures & measures,                 /* B:B:T time value     */
    const midi_timing & seqparms                    /* ppqn and beat-width  */
)
{
    midipulse result = c_null_midipulse;
    int m = measures.measures() - 1;                /* true measure count   */
    int b = measures.beats() - 1;                   /* true beats count     */
    if (m >= 0 && b >= 0)
    {
        double ppq = double(seqparms.ppqn());
        double beats_per_bar = double(seqparms.beats_per_measure());
        double beat_width = double(seqparms.beat_width());
        double qn_per_beat = double(c_qn_beats) / beat_width;        /* 4/W */
        double pulses_per_beat = qn_per_beat * ppq;
        double pulses_per_meas = m * pulses_per_beat * beats_per_bar;
        double pulses = m * pulses_per_meas;
        pulses += b * pulses_per_beat;
        result = midipulse(pulses);
        result += measures.divisions();
    }
    else
        result = 0;

    return result;
}

/**
 *  A new function to create a midi_measures structure from a string assumed
 *  to have a formate of "B:B:T".  Any fractional part is ignored as being
 *  less than a pulse.
 */

midi_measures
string_to_measures (const std::string & bbt)
{
    std::string m;
    std::string b;
    std::string t;
    std::string fraction;
    int count = extract_timing_numbers(bbt, m, b, t, fraction);
    if (count > 0)
    {
        int measures = strtoi(m);
        int beats = strtoi(b);
        int ticks = strtoi(t);
        if (measures == 0)
            measures = 1;

        if (beats == 0)
            beats = 1;

        return midi_measures(measures, beats, ticks);
    }
    else
    {
        static midi_measures s_dummy;
        return s_dummy;
    }
}

/**
 *  Converts a string that represents "hours:minutes:seconds.fraction" into a
 *  MIDI pulse/ticks/clock value.
 *
 * \param timestring
 *      The time value to be converted, which must be of the form
 *      "hh:mm:ss" or "hh:mm:ss.fraction".  That is, all four parts must
 *      be found.
 *
 * \param bpm
 *      The beats-per-minute tempo (e.g. 120) of the current MIDI song.
 *
 * \param ppqn
 *      The parts-per-quarter note precision (e.g. 192) of the current MIDI
 *      song.
 *
 * \return
 *      Returns 0 if an error occurred or if the number actually translated to
 *      0.
 */

midipulse
timestring_to_pulses (const std::string & timestring, midibpm bpm, int ppqn)
{
    midipulse result = 0;
    if (! timestring.empty())
    {
        std::string sh, sm, ss, us;
        if (extract_timing_numbers(timestring, sh, sm, ss, us) >= 4)
        {
            /**
             * This conversion assumes that the fractional parts of the
             * seconds is padded with zeroes on the left or right to 6 digits.
             */

            int hours = strtoi(sh);
            int minutes = strtoi(sm);
            int seconds = strtoi(ss);
            double secfraction = string_to_double(us, 0, 3); /* atof(us)    */
            long sec = ((hours * 60) + minutes) * 60 + seconds;
            long microseconds = 1000000 * sec + long(1000000.0 * secfraction);
            double pulses = delta_time_us_to_ticks(microseconds, bpm, ppqn);
            result = midipulse(pulses);
        }
    }
    return result;
}

/**
 *  Converts a time string to pulses.  First, the type of string is deduced by
 *  the characters in the string.  If the string contains two colons and a
 *  decimal point, it is assumed to be a time-string ("hh:mm:ss.frac"); in
 *  addition ss will have to be less than 60. ???  Actually, now we only care
 *  if four numbers are provided.
 *
 *  If the string just contains two colons, then it is assumed to be a
 *  measure-string ("measures:beats:divisions").
 *
 *  If it has none of the above, it is assumed to be pulses.  Testing is not
 *  rigorous.
 *
 * measurestring_to_pulses(): Converts "B:B:T" values to pulses.
 * timestring_to_pulses(): Converts "H:M:S.f" values to pulses.
 *
 * \param s
 *      Provides the string to convert to pulses.
 *
 * \param mt
 *      Provides the structure needed to provide BPM and other values needed
 *      for some of the conversions done by this function.
 *
 * \param timestring
 *      If true, interpret the string as an "H:M:S" string.
 *
 * \return
 *      Returns the string as converted to MIDI pulses (or divisions, clocks,
 *      ticks, whatever you call it).
 */

midipulse
string_to_pulses
(
    const std::string & s,
    const midi_timing & mt,
    bool timestring
)
{
    midipulse result = 0;
    tokenization tokens;
    int count = tokenize_string(s, tokens);     /* function in this module  */
    if (count == 1)                             /* no colons in it          */
    {
        result = midipulse(string_to_long(s));
    }
    else if (count > 1)
    {
        if (timestring)
            result = timestring_to_pulses(s, mt.beats_per_minute(), mt.ppqn());
        else
            result = measurestring_to_pulses(s, mt);
    }
    return result;
}

/**
 *  Creates a number with a negative-to-0-to-positive range.
 *
 *  The first call is seeded with the current time, then a pseudo-random
 *  number is returned.  This is a simplistic linear congruence generator. It
 *  returns a random number between -range and + range. [Could consider using
 *  random(3) which has a much longer periodicity.]
 *
 *  Another options is rand() % (2 * range + 1) + (-range + 1), but it uses
 *  only the low-order bits of the number.
 *
 *  C++11 has a default random engine template, but it is a pain!
 *
 *  return rand() / (RAND_MAX / (2 * range + 1) + 1) - range;
 *
 * \param range
 *      The amount of "randomness" desired.
 *
 * \return
 *      Returns a number from -range to +range, uniformly distributed.
 */

int
randomize (int range)
{
    static bool s_uninitialized = true;
    if (s_uninitialized)
    {
        s_uninitialized = false;
        srand(unsigned(time(NULL)));
    }
    if (range < 0)
        range = -range;

    long result = 2 * range * long(rand()) / RAND_MAX;
    return int(result) - range;
}

/**
 *  Returns true if a number is a power of 2.  MIDI's beatwidth values
 *  provide the power of 2 needed for a valid beat width value.
 *  First b in the below expression is for the case when b is 0.
 *
 *  Taken from:
 *
 * https://www.geeksforgeeks.org/c-program-to-find-whether-a-no-is-power-of-two/
 */

bool
is_power_of_2 (int b)
{
    return b && (! (b & (b - 1)));
}

/**
 *  Calculates the log-base-2 value of a number that is already a power of 2.
 *  Useful in converting a time signature's denominator to a Time Signature
 *  meta event's "dd" value.
 *
 * \param tsd
 *      The time signature denominator, which must be a power of 2:  2, 4, 8,
 *      16, or 32.
 *
 * \return
 *      Returns the power of 2 that achieves the \a tsd parameter value.
 */

int
log2_time_sig_value (int tsd)
{
    int result = 0;
    while (tsd > 1)
    {
        ++result;
        tsd >>= 1;
    }
    return result;
}

/**
 *  This function provides the size of the smallest horizontal grid unit in
 *  units of pulses (ticks).  We need this to be able to increment grid
 *  drawing by more than one (time-wasting!) without skipping any lines.
 *
 *  The smallest grid unit in the seqroll is a "sub-step".  The next largest
 *  unit is a "note-step", which is inside a note.  Each note contains PPQN
 *  ticks.
 *
 *  Current status at PPQN = 192, Base pixels = 6:
 *
\verbatim
    Zoom    Note-steps  Substeps   Substeps/Note (#SS)
      1         4           8           32
      2         4           4           16
      4         4           2            8
      8         4           0            4
     16         2           0            2
     32         0           0            1
     64        1/2          0           1/2
    128         0           0           1/4
\endverbatim
 *
 *  Pulses-per-substep is given by PPSS = PPQN / #SS, where #SS = 32 / Zoom.
 *  But 32 is the base PPQN (192) divided by the base pixels (6). In the end,
 *
 *      PPSS = (PPQN * Zoom * Base Pixels) / Base PPQN
 *
 *  Currently the Base values are hardwired (see usrsettings).  The base
 *  pixels value is c_pixels_per_substep = 6, and the base PPQN is
 *  usr().base_ppqn() = 192.  The numerator of this equation is well within
 *  the limit of a 32-bit integer.
 *
 * \param ppqn
 *      Provides the actual PPQN used by the currently-loaded tune.
 *
 * \param zoom
 *      Provides the current zoom value.  Defaults to 1, but is normally
 *      another value.
 *
 * \return
 *      The result of the above equation is returned.
 */

int
pulses_per_substep (midipulse ppqn, int zoom)
{
    return int((ppqn * zoom * c_pixels_per_substep) / usr().base_ppqn());
}

/**
 *  Similar to pulses_per_substep(), but for a single pixel.  Actually, what
 *  this function does is scale the PPQN against usr().base_ppqn() (192).
 *
 * \param ppqn
 *      Provides the actual PPQN used by the currently-loaded tune.
 *
 * \param zoom
 *      Provides the current zoom value.  Defaults to 1, which can be used
 *      to simply get the ratio between the actual PPQN, but only when PPQN >=
 *      usr().base_ppqn().
 *
 * \return
 *      The result of the above equation is returned.
 */

int
pulses_per_pixel (midipulse ppqn, int zoom)
{
    midipulse result = (ppqn * zoom) / usr().base_ppqn();
    if (result == 0)
        result = 1;

    return result;
}

/**
 *  Internal function for simple calculation of a power of 2 without a lot of
 *  math.  Use for calculating the denominator of a time signature.
 *
 * \param logbase2
 *      Provides the power to which 2 is to be raised.  This integer is
 *      probably only rarely greater than 5 (which represents a denominator of
 *      32).
 *
 * \return
 *      Returns 2 raised to the logbase2 power.
 */

int
beat_power_of_2 (int logbase2)
{
    int result;
    if (logbase2 == 0)
    {
        result = 1;
    }
    else
    {
        result = 2;
        for (int c = 1; c < logbase2; ++c)
            result *= 2;
    }
    return result;
}

/**
 *  Calculates positive integer powers.
 *
 * \param base
 *      The number to be raised to the power.
 *
 * \param exponent
 *      The power to be applied.  Only 0 or above are accepted.  However,
 *      there is currently no check for integer overflow of the result. This
 *      function is meant for reasonably small powers.
 *
 * \return
 *      Returns the power.  If the exponent is illegal, the 0 is returned.
 */

int
power (int base, int exponent)
{
    int result = 0;
    if (exponent > 1)
    {
        result = base;
        for (int p = exponent; p > 1; --p)
            result *= base;
    }
    else if (exponent == 1)
        result = base;
    else if (exponent == 0)
        result = 1;

    return result;
}

/**
 *  Calculates the base-2 log of a number. This number is truncated to an
 *  integer byte value, as it is used in calculating values to be written to a
 *  MIDI file.
 *
 * \param value
 *      The integer value for which the log2(value) is needed.
 *
 * \return
 *      Returns log2(value) for values of 1 or greater. Otherwise returns 0.
 */

midibyte
beat_log2 (int value)
{
    return value > 0 ? midibyte(std::log(double(value)) / std::log(2.0)) : 0 ;
}

/**
 *  Calculates the tempo in microseconds from the bytes read from a Tempo
 *  event in the MIDI file.
 *
 *  Is it correct to simply cast the bytes to a double value?
 *
 * \param tt
 *      Provides the 3-byte array of values making up the raw tempo data.
 *
 * \return
 *      Returns the result of converting the bytes to a double value.
 */

midibpm
tempo_us_from_bytes (const midibyte tt[3])
{
    midibpm result = midibpm(tt[0]);
    result = (result * 256) + midibpm(tt[1]);
    result = (result * 256) + midibpm(tt[2]);
    return result;
}

/**
 *  Provide a way to convert a tempo value (microseconds per quarter note)
 *  into the three bytes needed as value in a Tempo meta event.  Recall the
 *  format of a Tempo event:
 *
 *  0 FF 51 03 t2 t1 t0 (tempo as number of microseconds per quarter note)
 *
 *  This code is the inverse of the lines of code around line 768 in
 *  midifile.cpp, which is basically
 *  <code> ((t2 * 256) + t1) * 256 + t0 </code>.
 *
 *  As a test case, note that the default tempo is 120 beats/minute, which is
 *  equivalent to tttttt=500000 (0x07A120).  The output of this function will
 *  be t[] = { 0x07, 0xa1, 0x20 } [the indices go 0, 1, 2].
 *
 * \param t
 *      Provides a small array of 3 elements to hold each tempo byte.
 *
 * \param tempo_us
 *      Provides the temp value in microseconds per quarter note.  This is
 *      always an integer, not a double, so do not get confused here.
 */

bool
tempo_us_to_bytes (midibyte t[3], midibpm tempo_us)
{
    bool result = tempo_us > 0.0;
    if (result)
    {
        int temp = int(tempo_us + 0.5);
        t[2] = midibyte(temp & 0x0000FF);
        t[1] = midibyte((temp & 0x00FF00) >> 8);
        t[0] = midibyte((temp & 0xFF0000) >> 16);
    }
    else
    {
        t[2] = t[1] = t[0] = 0;
    }
    return result;
}

/**
 *  Converts a tempo value to a MIDI note value for the purpose of displaying
 *  a tempo value in the mainwnd, seqdata section (hopefully!), and the
 *  perfroll.  It implements the following linear equation, with clamping just
 *  in case.
 *
\verbatim
                           N1 - N0
        N = N0 + (B - B0) ---------     where (N1 - N0) is always 127
                           B1 - B0
\endverbatim
 *
\verbatim
                        127
        N = (B - B0) ---------
                      B1 - B0
\endverbatim
 *
 *  where N0 = 0 (MIDI note 0 is the minimum), N1 = 127 (the maximum MIDI
 *  note), B0 is the value of usr().midi_bpm_minimum(),
 *  B1 is the value of usr().midi_bpm_maximum(), B is the input beats/minute,
 *  and N is the resulting note value.  As a precaution due to rounding error,
 *  we clamp the values between 0 and 127.
 *
 * \param tempovalue
 *      The tempo in beats/minute.
 *
 * \return
 *      Returns the tempo value scaled to the range 0 to 127, based on the
 *      configured BPM minimum and maximum values.
 */

midibyte
tempo_to_note_value (midibpm tempovalue)
{
    double slope = double(max_midi_value());
    slope /= usr().midi_bpm_maximum() - usr().midi_bpm_minimum();

    int note = int(slope * (tempovalue - usr().midi_bpm_minimum()) + 0.5);
    return clamp_midibyte_value(note);
}

/**
 *  Fixes the tempo value, truncating it to the number of digits of tempo
 *  precision (0, 1, or 2) specified by "bpm_precision" in the "usr" file.
 *
 * \param bpm
 *      The uncorrected BPM value.
 *
 * \return
 *      Returns the BPM truncated to the desired precision.
 */

midibpm
fix_tempo (midibpm bpm)
{
    int precision = usr().bpm_precision();  /* 0/1/2 digits past decimal */
    if (precision > 0)
    {
        bpm *= 10.0;
        if (precision == 2)
            bpm *= 10.0;
    }
    bpm = trunc(bpm);
    if (precision > 0)
    {
        bpm /= 10.0;
        if (precision == 2)
            bpm /= 10.0;
    }
    return bpm;
}

/**
 *  Combines bytes into an unsigned-short value.
 *
 *  http://www.blitter.com/~russtopia/MIDI/~jglatt/tech/midispec/wheel.htm
 *
 *  Two data bytes follow the status. The two bytes should be combined
 *  together to form a 14-bit value. The first data byte's bits 0 to 6 are
 *  bits 0 to 6 of the 14-bit value. The second data byte's bits 0 to 6 are
 *  really bits 7 to 13 of the 14-bit value. In other words, assuming that a
 *  C program has the first byte in the variable First and the second data
 *  byte in the variable Second, here's how to combine them into a 14-bit
 *  value (actually 16-bit since most computer CPUs deal with 16-bit, not
 *  14-bit, integers).
 *
 *  I think Kepler34 got the bytes backward.
 *
 * \param b0
 *      The first byte to be combined.
 *
 * \param b1
 *      The second byte to be combined.
 *
 * \return
 *      Returns the bytes basically OR'd together.
 */

unsigned short
combine_bytes (midibyte b0, midibyte b1)
{
   unsigned short short_14bit = (unsigned short)(b1);
   short_14bit <<= 7;
   short_14bit |= (unsigned short)(b0);
   return short_14bit * 48;
}

/**
 *  The inverse of tempo_to_note_value().
 *
\verbatim
                  (N - N0) (B1 - B0)
        B = B0 + --------------------
                       N1 - N0
\endverbatim
 *
\verbatim
                    (B1 - B0)
        B = B0 + N -----------
                       127
\endverbatim
 *
 * \param note
 *      The note value used for displaying the tempo in the seqdata pane, the
 *      perfroll, and in a mainwnd slot.
 *
 * \return
 *      Returns the tempo in beats/minute.
 */

midibpm
note_value_to_tempo (midibyte note)
{
    double slope = usr().midi_bpm_maximum() - usr().midi_bpm_minimum();
    slope *= double(note);
    slope /= double(max_midi_value());
    slope += usr().midi_bpm_minimum();
    return slope;
}

/**
 *  Calculates a wave function for use as an LFO (low-frequency oscillator)
 *  for modifying data values in a sequence.  We extracted this function from
 *  mattias's lfownd module, as it is more generally useful.  The angle
 *  parameter is provided by the lfownd object.  It is calculated by
 *
\verbatim
                 speed * tick
        angle = -------------- + phase
                    length
\endverbatim
 *
 *  The speed (number of periods in the transformation) ranges from 0 to 16 in
 *  the user interface; the ratio of tick/seqlength ranges from 0 to 1; the
 *  phase ranges from 0 to 1, equivalent to 0 to 360 degrees.
 *
 * \param angle
 *      Provides the radial "angle" to be applied. Assuming that the "speed"
 *      (number of periods) is 1, then, for a one-measure pattern or
 *      a longer pattern with "Use Measures" unchecked in qlfoframe, this value
 *      ranges from 0.0 to 1.0.  Increasing the "speed" or the number of
 *      measures increases the angle range proportionately.
 *
 * \param wavetype
 *      Provides the wave value to select the type of wave data-point
 *      to be generated.
 *
 * \return
 *      Returns the result of the calculation, which will range from -1.0 to
 *      0.0 to 1.0, depending on the wave employed.
 */

double
wave_func (double angle, waveform wavetype)
{
    double result = 0.0;
    double tmp;
    double anglefixed;
    switch (wavetype)
    {
    case waveform::sine:
        tmp = 2.0 * M_PI * angle;                       /* angle in radians */
        result = sin(tmp);
        break;

    case waveform::sawtooth:
        anglefixed = angle - int(angle);
        tmp = 2.0 * anglefixed;
        result = tmp - 1.0;
        break;

    case waveform::reverse_sawtooth:
        anglefixed = angle - int(angle);
        tmp = -2.0 * anglefixed;
        result = tmp + 1.0;
        break;

    case waveform::triangle:
        tmp = 2.0 * angle;
        result = (tmp - int(tmp));
        if ((int(tmp)) % 2 == 1)
            result = 1.0 - result;

        result = 2.0 * result - 1.0;
        break;

    case waveform::exponential:
        result = exp_normalize(angle);
        break;

    case waveform::reverse_exponential:
        result = exp_normalize(angle, true);
        break;

    default:
        break;
    }
    return result;
}

/**
 *  Converts a double value to range from 0.0 to 1.0. That is, it returns the
 *  fractional portion.  For example, 4.145 would become 0.145.
 */

double
unit_truncation (double angle)
{
    double result = angle;
    if (result > 1.0)
    {
        double truncated = trunc(result);
        result -= truncated;
    }
    return result;
}

/**
 *  This function maps midibyte values from 0 to 127 to the range of
 *  approximately e^(-2.436) to e^(+2.436), which is 0.884 to 11.314.
 *  These values convert nicely to a range of 11.314 / 0.0884 = 128.
 *
 *  So we take the angle A:
 *
 *  -#  Convert A via truncation so it ranges from 0.0 to 1.0.  Call it T.
 *  -#  Re-map A to range from Emin = -2.436 to Emax = 2.436. Call it A'.
 *  -#  Take e to the power A'.
 *  -#  Rescale the result to the range of 0.0 to 1.0, for use in the LFO
 *      generator.
 *
 *  The re-mapping equations is y = mx + b, where the y-intercept is easily
 *  seen to be b = Emin, and the slope is m = Emax - Emin = range.
 *
 *  For a reverse exponential, we simply negate the exponent
 */

double
exp_normalize (double angle, bool negate)
{
    static const double s_range = log(double(max_midi_value())); /* 4.852 */
    static const double s_exp_max = s_range / 2.0;               /* +2.42 */
    static const double s_exp_min = -s_exp_max;                  /* -2.42 */
    static const double s_scaler = exp(s_exp_min);
    double T = unit_truncation(angle);
    double Aprime = s_range * T + s_exp_min;
    if (negate)
        Aprime = -Aprime;

    double result = exp(Aprime);
    result *= s_scaler;
    return result;
}

/**
 *  Converts a wave type value to a string.  These names are short because I
 *  cannot figure out how to get the window pad out to show the longer names.
 *
 * \param wavetype
 *      The wave-type value to be displayed.
 *
 * \return
 *      Returns a short description of the wave type.
 */

std::string
wave_type_name (waveform wavetype)
{
    std::string result = "None";
    switch (wavetype)
    {
    case waveform::sine:

        result = "Sine";
        break;

    case waveform::sawtooth:

        result = "Ramp Up Saw";
        break;

    case waveform::reverse_sawtooth:

        result = "Decay Saw";
        break;

    case waveform::triangle:

        result = "Triangle";
        break;

    case waveform::exponential:

        result = "Exponential Rise";
        break;

    case waveform::reverse_exponential:

        result = "Exponential Fall";
        break;

    default:

        break;
    }
    return result;
}

/**
 *  Extracts the two names from the ALSA/JACK client/port name format:
 *
 *      [0] 128:0 client name:port name
 *
 *  When a2jmidid is running:
 *
 *      a2j:Midi Through [14] (playback): Midi Through Port-0
 *
 *  with "a2j" as the client name and the rest, including the second colon, as
 *  the port name.  For that kind of name, use extract_a2j_port_name().
 *
 * \param fullname
 *      The full port specification to be split.
 *
 * \param [out] clientname
 *      The destination for the client name portion, "clientname".
 *
 * \param [out] portname
 *      The destination for the port name portion, "portname".
 *
 * \return
 *      Returns true if all items are non-empty after the process.
 */

bool
extract_port_names
(
    const std::string & fullname,
    std::string & clientname,
    std::string & portname
)
{
    bool result = ! fullname.empty();
    clientname.clear();
    portname.clear();
    if (result)
    {
        std::string cname;
        std::string pname;
        std::size_t colonpos = fullname.find_first_of(":"); /* not last! */
        if (colonpos != std::string::npos)
        {
            /*
             * The client name consists of all characters up the the first
             * colon.  Period.  The port name consists of all characters
             * after that colon.  Period.
             */

            cname = fullname.substr(0, colonpos);
            pname = fullname.substr(colonpos+1);
            result = ! cname.empty() && ! pname.empty();
        }
        else
            pname = fullname;

        clientname = cname;
        portname = pname;
    }
    return result;
}

/**
 *  Extracts the buss name from "bus:port".  Sometimes we don't need both
 *  parts at once.
 *
 *  However, when a2jmidid is active. the port name will have a colon in it.
 *
 * \param fullname
 *      The "bus:port" name.
 *
 * \return
 *      Returns the "bus" portion of the string.  If there is no colon, then
 *      it is assumed there is no buss name, so an empty string is returned.
 */

std::string
extract_bus_name (const std::string & fullname)
{
    std::size_t colonpos = fullname.find_first_of(":");  /* not last! */
    return (colonpos != std::string::npos) ?
        fullname.substr(0, colonpos) : std::string("");
}

/**
 *  Extracts the port name from "bus:port".  Sometimes we don't need both
 *  parts at once.
 *
 *  However, when a2jmidid is active. the port name will have a colon in it.
 *
 * \param fullname
 *      The "bus:port" name.
 *
 * \return
 *      Returns the "port" portion of the string.  If there is no colon, then
 *      it is assumed that the name is a port name, and so \a fullname is
 *      returned.
 */

std::string
extract_port_name (const std::string & fullname)
{
    std::size_t colonpos = fullname.find_first_of(":");  /* not last! */
    return (colonpos != std::string::npos) ?
        fullname.substr(colonpos + 1) : fullname ;
}

/**
 *  Sets the name to be displayed for showing to the user, and hopefully,
 *  later, for look-up.
 *
 *  NOT YET USED.
 *
 *  For JACK ports created by a2jmidid (a2j_control), we want to shorten the
 *  name radically, and also set the bus ID, which is contained in square
 *  brackets.
 *
 *  - ALSA: "[0] 14:0 Midi Through Port-0"
 *  - JACK: "[0] 0:0 seq66:system midi_playback_1"
 *  - A2J:  "[0] 0:0 seq66:a2j Midi Through [14] (playback): Midi Through Port-0"
 *
 *  Skip past the two colons to get to the main part of the name.  Extract it,
 *  and prepend "a2j".
 *
 * \param alias
 *      The system-supplied or a2jmidid name for the port.  One example:
 *      "a2j:Midi Through [14] (playback): Midi Through Port-0".  Obviously,
 *      this function depends on the formatting of a2jmidid name assignments
 *      not changing.
 *
 * \sideeffect
 *      The bus ID is also modified, if present in the string (see "[14]"
 *      above).
 *
 * \return
 *      Returns the bare port-name is "a2j" appears in the alias. Otherwise, and
 *      empty string is returned.
 */

std::string
extract_a2j_port_name (const std::string & alias)
{
    std::string result;
    if (contains(alias, "a2j"))
    {
        auto lpos = alias.find_first_of(":");
        if (lpos != std::string::npos)
        {
            lpos = alias.find_first_of(":", lpos + 1);
            if (lpos != std::string::npos)
            {
                result = alias.substr(lpos + 2);
                result = "A2J " + result;
            }
        }
    }
    return result;
}

/**
 *  Calculates the closest snap value.
 *
 * \param S
 *      Provides the snap value to be applied.  Ignored it it is 0.
 *
 * \param p
 *      Provide the value to be snapped.
 *
 * \return
 *      Returns the snapped value.
 */

midipulse
closest_snap (int S, midipulse p)
{
    midipulse result = p;
    if (p < 0)
        return 0;

    if (S > 0)
    {
        midipulse Sn0 = p - (p % S);
        midipulse Sn1 = Sn0 + S;
        int deltalo = p - Sn0;                  /* do we need to use abs()? */
        int deltahi = Sn1 - p;
        result = deltalo <= deltahi ? Sn0 : Sn1 ;
    }
    return result;
}

midipulse
down_snap (int S, midipulse p)
{
    midipulse result = p;
    if (p < 0)
        return 0;

    if (S > 0)
        result = midipulse(p - (p % S));

    return result;
}

midipulse
up_snap (int S, midipulse p)
{
    midipulse result = p;
    if (p < 0)
        return 0;

    if (S > 0)
    {
        midipulse Sn0 = p - (p % S);
        result = Sn0 + midipulse(S);
    }
    return result;
}

}       // namespace seq66

/*
 * calculations.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


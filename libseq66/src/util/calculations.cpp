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
 * \file          calculations.cpp
 *
 *  This module declares/defines some utility functions and calculations
 *  needed by this application.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-11-07
 * \updates       2020-12-09
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

#include "app_limits.h"
#include "cfg/settings.hpp"
#include "midi/event.hpp"
#include "util/calculations.hpp"
#include "util/strfunctions.hpp"        /* seq66::contains(), etc.          */

#if ! defined PI
#define PI     3.14159265359
#endif

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *
 */

int
satoi (const std::string & v)
{
    return std::atoi(v.c_str());
}

/**
 *  Extracts up to 4 numbers from a colon-delimited string.
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
 *      At some point we will tighten it up.  This function is tested in the
 *      seq66-tests project, in the "calculations_unit_test" module.
 *      At present this test is surely BROKEN!
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
    std::vector<std::string> tokens;
    int count = tokenize_string(s, tokens);
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
    std::vector<std::string> & tokens
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
 * \todo
 *      Still needs to be unit tested.
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
#if defined USE_OLD_PULSES_TO_MIDI_MEASURES
        static const double s_epsilon = 0.0001; /* Hmmmmmmmmmmmmmmmmmmmmmmm */
        double m = p * W / (4.0 * P * B);       /* measures, whole.frac     */
        double m_whole = std::floor(m);         /* holds integral measures  */
        m -= m_whole;                           /* get fractional measure   */
        double b = m * B;                       /* beats, whole.frac        */
        double b_whole = std::floor(b);         /* get integral beats       */
        b -= b_whole;                           /* get fractional beat      */
        double Lp = 4 * P / W;                  /* pulses/qn * qn/beat      */
        measures.measures(int(m_whole + s_epsilon) + 1);
        measures.beats(int(b_whole + s_epsilon) + 1);
        measures.divisions(int(b * Lp + s_epsilon));
#else
        double tbc = p * W / (4.0 * P);         /* total beat-count for p   */
        midipulse Lp = 4 * P / W;               /* beat length in pulses    */
        int beatticks = int(tbc) * Lp;          /* pulses in total beats    */
        int b = int(tbc) % B;                   /* beat within measure re 0 */
        measures.measures(int(tbc / B) + 1);    /* number of measures       */
        measures.beats(b + 1);                  /* beats within the measure */
        measures.divisions(int(p - beatticks)); /* leftover pulses / ticks  */
#endif
    }
    return result;
}

/**
 *  Converts a MIDI pulse/ticks/clock value into a string that represents
 *  "hours:minutes:seconds.fraction".  See the other pulses_to_timestring()
 *  overload.
 *
 * \todo
 *      Still needs to be unit tested.
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
 *      Returns the return-value of the other pulses_to_timestring() function.
 */

std::string
pulses_to_timestring (midipulse p, const midi_timing & timinginfo)
{
    return pulses_to_timestring
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
 *      If true (the default), shows the microseconds as well.
 *
 * \return
 *      Returns the time-string representation of the pulse (ticks) value.
 */

std::string
pulses_to_timestring (midipulse p, midibpm bpm, int ppqn, bool showus)
{
    unsigned long microseconds = ticks_to_delta_time_us(p, bpm, ppqn);
    int seconds = int(microseconds / 1000000UL);
    int minutes = seconds / 60;
    int hours = seconds / (60 * 60);
    minutes -= hours * 60;
    seconds -= (hours * 60 * 60) + (minutes * 60);
    microseconds -= (hours * 60 * 60 + minutes * 60 + seconds) * 1000000UL;

    char tmp[64];
    if (! showus || (microseconds == 0))
    {
        /*
         * Why the spaces?  It is inconsistent.  But see the
         * timestring_to_pulses() function first.
         */

        snprintf(tmp, sizeof tmp, "%03d:%d:%02d   ", hours, minutes, seconds);
    }
    else
    {
        snprintf
        (
            tmp, sizeof tmp, "%03d:%d:%02d.%02lu",
            hours, minutes, seconds, microseconds
        );
    }
    return std::string(tmp);
}

/**
 *  Converts a string that represents "measures:beats:division" to a MIDI
 *  pulse/ticks/clock value.
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
            midi_measures meas_values;          /* initializes to 0 in ctor */
            meas_values.measures(satoi(m));
            if (valuecount > 1)
            {
                meas_values.beats(satoi(b));
                if (valuecount > 2)
                {
                    if (d == "$")
                        meas_values.divisions(seqparms.ppqn() - 1);
                    else
                        meas_values.divisions(satoi(d));
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
 *  "0:0:0" as one might expect.  If we get a 0 for measures or for beats, we
 *  treat them as if they were 1.  It is too easy for the user to mess up.
 *
 *  We should consider clamping the beats to the beat-width value as well.
 *
 * \param measures
 *      Provides the current MIDI song time structure holding the
 *      measures, beats, and divisions values for the time of interest.
 *
 * \param seqparms
 *      This small structure provides the beats/measure, beat-width, and PPQN
 *      that hold for the sequence involved in this calculation.
 *
 * \return
 *      Returns the absolute pulses that mark this duration.  If the
 *      pulse-value cannot be calculated, then c_null_midipulse is
 *      returned.
 */

midipulse
midi_measures_to_pulses
(
    const midi_measures & measures,
    const midi_timing & seqparms
)
{
    midipulse result = c_null_midipulse;
    int m = measures.measures() - 1;                /* true measure count   */
    int b = measures.beats() - 1;
    if (m < 0)
        m = 0;

    if (b < 0)
        b = 0;

    double qn_per_beat = 4.0 / seqparms.beat_width();
    result = 0;
    if (m > 0)
        result += int(m * seqparms.beats_per_measure() * qn_per_beat);

    if (b > 0)
        result += int(b * qn_per_beat);

    result *= seqparms.ppqn();
    result += measures.divisions();
    return result;
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

            int hours = satoi(sh);
            int minutes = satoi(sm);
            int seconds = satoi(ss);
            double secfraction = atof(us.c_str());
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
 *  addition ss will have to be less than 60. ???  Actually, now we only care if
 *  four numbers are provided.
 *
 *  If the string just contains two colons, then it is assumed to be a
 *  measure-string ("measures:beats:divisions").
 *
 *  If it has none of the above, it is assumed to be pulses.  Testing is not
 *  rigorous.
 *
 * \param s
 *      Provides the string to convert to pulses.
 *
 * \param mt
 *      Provides the structure needed to provide BPM and other values needed
 *      for some of the conversions done by this function.
 *
 * \return
 *      Returns the string as converted to MIDI pulses (or divisions, clocks,
 *      ticks, whatever you call it).
 */

midipulse
string_to_pulses
(
    const std::string & s,
    const midi_timing & mt
)
{
    midipulse result = 0;
    std::string s1;
    std::string s2;
    std::string s3;
    std::string fraction;
    int count = extract_timing_numbers(s, s1, s2, s3, fraction);
    if (count > 1)
    {
        if (fraction.empty() || satoi(s3) >= 60)     // why???
            result = measurestring_to_pulses(s, mt);
        else
            result = timestring_to_pulses(s, mt.beats_per_minute(), mt.ppqn());
    }
    else
        result = atol(s.c_str());

    return result;
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
 *  Calculates a suitable starting zoom value for the given PPQN value.  The
 *  default starting zoom is 2, but this value is suitable only for PPQN of
 *  192 and below.  Also, zoom currently works consistently only if it is a
 *  power of 2.  For starters, we scale the zoom to the selected ppqn, and
 *  then shift it each way to get a suitable power of two.
 *
 * \param ppqn
 *      The ppqn of interest.
 *
 * \return
 *      Returns the power of 2 appropriate for the given PPQN value.
 */

int
zoom_power_of_2 (int ppqn)
{
    int result = SEQ66_DEFAULT_ZOOM;
    if (ppqn > SEQ66_DEFAULT_PPQN)
    {
        int zoom = result * ppqn / SEQ66_DEFAULT_PPQN;
        zoom >>= 2;                     /* "divide" by 2    */
        zoom <<= 2;                     /* "multiply" by 2  */
        result = zoom;
        if (result > SEQ66_MAXIMUM_ZOOM)
            result = SEQ66_MAXIMUM_ZOOM;
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
 *  Currently the Base values are hardwired (see app_limits.h).  The base
 *  pixels value is SEQ66_PIXELS_PER_SUBSTEP = 6, and the base PPQN is
 *  SEQ66_DEFAULT_PPQN = 192.  The numerator of this equation is well within
 *  the limit of a 32-bit integer.
 *
 * \param ppqn
 *      Provides the actual PPQN used by the currently-loaded tune.
 *
 * \param zoom
 *      Provides the current zoom value.
 *
 * \return
 *      The result of the above equation is returned.
 */

midipulse
pulses_per_substep (midipulse ppqn, int zoom)
{
    return (ppqn * zoom * SEQ66_PIXELS_PER_SUBSTEP) / SEQ66_DEFAULT_PPQN;
}

/**
 *  Similar to pulses_per_substep(), but for a single pixel.
 */

midipulse
pulses_per_pixel (midipulse ppqn, int zoom)
{
    return (ppqn * zoom) / SEQ66_DEFAULT_PPQN;
}

/**
 *  Internal function for simple calculation of a power of 2 without a lot of
 *  math.  Use for calculating the denominator of a time signature.
 *
 * \param logbase2
 *      Provides the power to which 2 is to be raised.  This integer is
 *      probably only rarely greater than 4 (which represents a denominator of
 *      16).
 *
 * \return
 *      Returns 2 raised to the logbase2 power.
 */

int
beat_pow2 (int logbase2)
{
    int result;
    if (logbase2 == 0)
        result = 1;
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
 *      The integer value for which log2(value) is needed.
 *
 * \return
 *      Returns log2(value).
 */

midibyte
beat_log2 (int value)
{
    return midibyte(std::log(double(value)) / std::log(2.0));
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

void
tempo_us_to_bytes (midibyte t[3], int tempo_us)
{
    t[2] = midibyte(tempo_us & 0x0000FF);
    t[1] = midibyte((tempo_us & 0x00FF00) >> 8);
    t[0] = midibyte((tempo_us & 0xFF0000) >> 16);
}

/**
 *  Converts a tempo value to a MIDI note value for the purpose of displaying
 *  a tempo value in the mainwid, seqdata section (hopefully!), and the
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
    double slope = double(c_max_midi_data_value);
    slope /= usr().midi_bpm_maximum() - usr().midi_bpm_minimum();
    double note = (tempovalue - usr().midi_bpm_minimum()) * slope;
    if (note < 0.0)
        note = 0.0;
    else if (note > double(c_max_midi_data_value))
        note = double(c_max_midi_data_value);

    return midibyte(note);
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
 *      perfroll, and in a mainwid slot.
 *
 * \return
 *      Returns the tempo in beats/minute.
 */

midibpm
note_value_to_tempo (midibyte note)
{
    double slope = usr().midi_bpm_maximum() - usr().midi_bpm_minimum();
    slope *= double(note);
    slope /= double(c_max_midi_data_value);
    slope += usr().midi_bpm_minimum();
    return slope;
}

/**
 *  Calculates the quotient and remainder of a midipulse division, which is a
 *  common operation in Seq66.  This function also avoids division by
 *  zero (and currently ignores negative denominators, which are still
 *  possible with the current definition of the midipulse type definition
 *  (alias).
 *
 * \param numerator
 *      Provides the numerator in the division operation.
 *
 * \param denominator
 *      Provides the denominator in the division operation.
 *
 * \param [out] remainder
 *      The remainder is written here.  If the division cannot be done, it is
 *      set to 0.
 *
 * \return
 *      Returns the result of the division.
 */

midipulse
pulse_divide (midipulse numerator, midipulse denominator, midipulse & remainder)
{
    midipulse result = 0;
    if (denominator > 0)
    {
        ldiv_t temp = ldiv(numerator, denominator);
        result = temp.quot;
        remainder = temp.rem;
    }
    else
        remainder = 0;

    return result;
}

/**
 *  Calculates a wave function for use as an LFO (low-frequency oscillator)
 *  for modifying data values in a sequence.  We extracted this function from
 *  mattias's lfownd module, as it is more generally useful.  The angle
 *  parameter is provided by the lfownd object.  It is calculated by
 *
\verbatim
                 speed * tick * BW
        angle = ------------------- + phase
                      seqlength
\endverbatim
 *
 *  The speed ranges from 0 to 16; the ratio of tick/seqlength ranges from 0
 *  to 1; BW (beat width) is generally 4; the phase ranges from 0 to 1.
 *
 * \param angle
 *      Provides the radial angle to be applied.  Units of radians,
 *      apparently.
 *
 * \param wavetype
 *      Provides the wave value to select the type of wave data-point
 *      to be generated.
 */

double
wave_func (double angle, wave wavetype)
{
    double result = 0.0;
    switch (wavetype)
    {
    case wave::sine:

        result = sin(angle * PI * 2.0);
        break;

    case wave::sawtooth:

        result = (angle - int(angle)) * 2.0 - 1.0;
        break;

    case wave::reverse_sawtooth:

        result = (angle - int(angle)) * -2.0 + 1.0;
        break;

    case wave::triangle:
    {
        double tmp = angle * 2.0;
        result = (tmp - int(tmp));
        if ((int(tmp)) % 2 == 1)
            result = 1.0 - result;

        result = result * 2.0 - 1.0;
        break;
    }

    default:

        break;
    }
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
wave_type_name (wave wavetype)
{
    std::string result = "None";
    switch (wavetype)
    {
    case wave::sine:

        result = "Sine";
        break;

    case wave::sawtooth:

        result = "Ramp Up Saw";
        break;

    case wave::reverse_sawtooth:

        result = "Decay Saw";
        break;

    case wave::triangle:

        result = "Triangle";
        break;

    default:

        break;
    }
    return result;
}

/**
 *  Extracts the two names from the ALSA/JACK client/port name format:
 *
 *      [0] 128:0 clientname:portname
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

int
extract_a2j_bus_id (const std::string & alias)
{
    int result = (-1);
    if (contains(alias, "a2j"))
    {
        auto lpos = alias.find_first_of("[");
        auto rpos = alias.find_first_of("]");
        bool ok = lpos != std::string::npos && rpos != std::string::npos;
        if (ok)
            ok = rpos > lpos;

        if (ok)
        {
            std::string temp = alias.substr(lpos, rpos - lpos - 1);
            result = string_to_int(temp);
        }
    }
    return result;
}

/**
 *  Gets the current date/time.
 *
 * \return
 *      Returns the current date and time as a string.
 */

std::string
current_date_time ()
{
    static char s_temp[64];
    static const char * const s_format = "%Y-%m-%d %H:%M:%S";
    time_t t;
    std::memset(s_temp, 0, sizeof s_temp);
    time(&t);

    struct tm * tm = localtime(&t);
    std::strftime(s_temp, sizeof s_temp - 1, s_format, tm);
    return std::string(s_temp);
}

}       // namespace seq66

/*
 * calculations.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


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
 * \file          midibytes.cpp
 *
 *  This module declares a couple of useful data classes.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-09
 * \updates       2022-06-27
 * \license       GNU GPLv2 or above
 *
 *  These classes were originally structures, but now they are "constant"
 *  classes, filled in at construction time and accessed only through getter
 *  functions.
 */

#include <stdexcept>                    /* std::invalid_argument            */

#include "midi/midibytes.hpp"           /* seq66::midi_timing, _measures    */
#include "util/basic_macros.h"          /* not_nullptr() macro              */

namespace seq66
{

/*
 * -------------------------------------------------------------------------
 *  midi_measures
 * -------------------------------------------------------------------------
 */

/**
 *  Default constructor for midi_measures.
 */

midi_measures::midi_measures () :
    m_measures      (0),
    m_beats         (0),
    m_divisions     (0)
{
    // Empty body
}

/**
 *  Principal constructor for midi_measures.
 *
 * \param measures
 *      Copied into the m_measures member.
 *
 * \param beats
 *      Copied into the m_beats member.
 *
 * \param divisions
 *      Copied into the m_divisions member.
 */

midi_measures::midi_measures (int measures, int beats, int divisions) :
    m_measures      (measures),
    m_beats         (beats),
    m_divisions     (divisions)
{
    // Empty body
}

/*
 * -------------------------------------------------------------------------
 *  midi_timing
 * -------------------------------------------------------------------------
 */

/**
 *  Defaults constructor for midi_timing.
 */

midi_timing::midi_timing () :
    m_beats_per_minute      (0),
    m_beats_per_measure     (0),
    m_beat_width            (0),
    m_ppqn                  (0)
{
    // Empty body
}

/**
 *  Principal constructor for midi_timing.
 *
 * \param bpminute
 *      Copied into the m_beats_per_minute member.
 *
 * \param bpmeasure
 *      Copied into the m_beats_per_measure member.
 *
 * \param beatwidth
 *      Copied into the m_beat_width member.
 *
 * \param ppqn
 *      Copied into the m_ppqn member.
 */

midi_timing::midi_timing
(
    midibpm bpminute,
    int bpmeasure,
    int beatwidth,
    int ppqn
) :
    m_beats_per_minute      (bpminute),
    m_beats_per_measure     (bpmeasure),
    m_beat_width            (beatwidth),
    m_ppqn                  (ppqn)
{
    // Empty body
}

/*
 *  Free functions.
 */

std::string
midi_bytes_string (const midistring & b, int limit)
{
    std::string result;
    int count = int(b.length());
    bool no_0x = limit > 0;
    int len = count;
    if (no_0x && (limit < count))
        len = limit;

    if (len > 0)
    {
        char tmp[8];
        const char * fmt = no_0x ? "%02X" : "0x%02x" ;
        for (int i = 0; i < len; ++i)
        {
            (void) snprintf(tmp, sizeof tmp, fmt, unsigned(b[i]));
            result += tmp;
            if (i < (len + 1))
                result += " ";
        }
        if (len < count)
            result += " ...";
    }
    return result;
}

/**
 *  Converts a string to a MIDI byte.  Similar to string_to_long() in the
 *  strfunctions module.
 *
 * \param s
 *      Provides the string to convert to a MIDI byte.
 *
 * \return
 *      Returns the MIDI byte value represented by the string.
 */

midibyte
string_to_midibyte (const std::string & s, midibyte defalt)
{
    midibyte result = defalt;
    try
    {
        int temp = std::stoi(s, nullptr, 0);
        if (temp >= 0 && temp <= UCHAR_MAX)
            result = midibyte(temp);
    }
    catch (std::invalid_argument const &)
    {
        // no code
    }
    return result;
}

/**
 *  Fix up the size of a midibooleans vector.  It copies as many midibool
 *  values as is correct from the old vector.  Then, if it needs to be longer,
 *  additional 0 values are pushed into the result.
 */

midibooleans
fix_midibooleans (const midibooleans & mbs, int newsz)
{
    midibooleans result;
    int sz = int(mbs.size());
    if (newsz >= sz)
    {
        result = mbs;
        if (newsz > sz)
        {
            int diff = newsz - sz;
            for (int i = 0; i < diff; ++i)
                result.push_back(0);
        }
    }
    else
    {
        for (int i = 0; i < newsz; ++i)
            result.push_back(mbs[i]);
    }
    return result;
}

}           // namespace seq66

/*
 * midibytes.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


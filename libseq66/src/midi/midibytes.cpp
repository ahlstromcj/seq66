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
 * \updates       2019-10-07
 * \license       GNU GPLv2 or above
 *
 *  These classes were originally structures, but now they are "constant"
 *  classes, filled in at construction time and accessed only through getter
 *  functions.
 */

#include <algorithm>                    /* std::rotate() function           */

#include "midi/midibytes.hpp"           /* seq66::midi_timing, _measures    */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

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
 * -------------------------------------------------------------------------
 *  midi_booleans
 * -------------------------------------------------------------------------
 */

/**
 *  Constructs either an empty set or a set of false values.
 */

midi_booleans::midi_booleans (int count) :
    m_booleans  ()
{
    if (count > 0)
    {
        for (int i = 0; i < count; ++i)
            m_booleans.push_back(midibool(false));
    }
}

/**
 *  Construct from an array of booleans.
 */

midi_booleans::midi_booleans (const bool * barray, int count) :
    m_booleans  ()
{
    if (not_nullptr(barray) && count > 0)
    {
        for (int i = 0; i < count; ++i)
            m_booleans.push_back(midibool(barray[i]));
    }
}


/**
 *  Constructs a vector and fills it.
 */

midi_booleans::midi_booleans (const midibooleans & mbs) :
    m_booleans  ()
{
    for (size_t i = 0; i < mbs.size(); ++i)
        m_booleans.push_back(mbs[i]);
}

/**
 *  Copies a vector.
 */

midi_booleans::midi_booleans (const midi_booleans & mbs) :
    m_booleans  (mbs.m_booleans)
{
    // no code
}

/**
 *  Assigns a vector.
 */

midi_booleans &
midi_booleans::operator = (const midi_booleans & rhs)
{
    if (this != &rhs)
    {
        m_booleans = rhs.m_booleans;
    }
    return *this;
}

/**
 *  Rotates the boolean vector by the given count.  Turns out the good old STL
 *  has an easy solution to this task.
 *
 * \param count
 *      If positive, the vector is rotated leftward (subtracts from the index
 *      of higher elements).  If negative, the vector is rotated rightward
 *      (low index items move to a higher index).
 */

void
midi_booleans::rotate (int count)
{
    if (count != 0)
    {
        if (count > 0)
        {
            std::rotate                 /* rotate left  */
            (
                m_booleans.begin(),
                m_booleans.begin() + std::size_t(count),
                m_booleans.end()
            );
        }
        else
        {
            std::rotate                 /* rotate left  */
            (
                m_booleans.begin(),
                m_booleans.begin() + m_booleans.size() - std::size_t(count),
                m_booleans.end()
            );
        }
    }
}

midibool &
midi_booleans::operator [] (std::size_t index)
{
    static midibool s_default_value = midibool(false);
    return index < m_booleans.size() ? m_booleans[index] : s_default_value ;
}

midibool
midi_booleans::operator [] (std::size_t index) const
{
    static midibool s_default_value = midibool(false);
    return index < m_booleans.size() ? m_booleans[index] : s_default_value ;
}

bool
midi_booleans::match (const midi_booleans & rhs, int count) const
{
    if (count > 0)
    {
        int actualcount = 0;
        for (std::size_t i = 0; i < m_booleans.size(); ++i)
        {
            bool target = bool(m_booleans[i]);
            bool policy = bool(rhs.m_booleans[i]);
            if (target)                             /* a note exists here   */
            {
                if (policy)
                    ++actualcount;                  /* note matches policy  */
            }
        }
        return actualcount >= count;
    }
    else
        return m_booleans == rhs.m_booleans;
}

std::string
midi_booleans::fingerprint () const
{
    std::string result;
    for (auto mb : m_booleans)
    {
        bool bit = bool(mb);
        result += bit ? "1" : "0" ;
    }
    result += "\n";
    return result;
}

int
midi_booleans::true_count () const
{
    int result = 0;
    for (auto mb : m_booleans)
    {
        bool bit = bool(mb);
        if (bit)
            ++result;
    }
    return result;
}

}           // namespace seq66

/*
 * midibytes.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


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

midi_measures::midi_measures
(
    int measures,
    int beats,
    int divisions
) :
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

}           // namespace seq66

/*
 * midibytes.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


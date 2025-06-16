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
 * \file          zoomer.cpp
 *
 *  This module declares/defines zoom management.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2023-09-08
 * \updates       2024-12-17
 * \license       GNU GPLv2 or above
 *
 *  Refactoring:
 *
 *      Originally, Seq66 started with PPQN and then derived the proper
 *      ticks-to-pixels conversion. This has issues with PPQNs of 120
 *      and 240 (versus 192). Let's start with the zoom (ticks per pixel)
 *      and work toward PPQN. The following suggested zoom member functions
 *      are similar to the like-named functions in the calculations module,
 *      but we start from the pixel and move up, ignoring PPQN.
 *
 *      -   pulses_per_pixel(). This is basically the zoom value, which
 *          starts at 2. So we don't need this function.
 *      -   pulses_per_substep(). The sub-step vertical lines are 6 pixels.
 *          We need to stick with that, no matter what the zoom. This function
 *          can call pulses_per_pixel() and multiply it by 6.
 *      -   pulses_per_quarter_beat(). Assuming it is good to use a 4th of
 *          a beat (but what about beat-widths of 8, 16, ...? 4/x?)
 *          pulses_per_partial_beat()? Default = factor of 4.
 *      -   pulses_per_beat(). Default = factor of 4.
 *      -   pulses_per_measure(). Based on beats.
 *
 *
 *  Diagram:
 *
 *      measure
 *        sub-step
 *           quarter-beat
 *                       beat     (beats)       beat            measure
 *      ||...:...:...:...|...:...:. . . . ..:...|...:...:...:...||
 */

#include "cfg/settings.hpp"             /* seq66::zoom_items()              */
#include "cfg/zoomer.hpp"               /* seq66::zoomer class              */

namespace seq66
{

/**
 *  Default constructor.
 */

zoomer::zoomer () :
    m_ppqn                  (192),
    m_initial_zoom          (2),
    m_zoom                  (2),
    m_scale                 (1),
    m_scale_zoom            (2),
    m_zoom_index            (0),
    m_zoom_expansion        (1)
{
    initialize();
}

/**
 *  Principal constructor.
 */

zoomer::zoomer (int ppq, int initialzoom, int scalex) :
    m_ppqn                  (ppq),
    m_initial_zoom          (initialzoom),
    m_zoom                  (initialzoom),
    m_scale                 (scalex > 4 ? scalex / 4 : 1),
    m_scale_zoom            (m_scale * zoom()),     /* see change_ppqn()    */
    m_zoom_index            (0),
    m_zoom_expansion        (1)
{
    initialize();
}

bool
zoomer::initialize ()
{
    int index = log2_of_power_of_2(m_initial_zoom);
    bool result = index >= 0;
    if (result)
    {
        m_zoom_index = index;
        m_zoom_expansion = 0;
        m_zoom = m_initial_zoom;
    }
    else
    {
        m_zoom_index = 1;
        m_zoom_expansion = 0;
        m_zoom = zoom_item(1);
    }
    m_scale_zoom = zoom() * m_scale;
    return result;
}

/**
 *  Make the view cover less horizontal length.  The lowest zoom possible
 *  is 1.  But, if the user still wants to zoom in some more, we fake it
 *  by using "zoom expansion". This factor increases the pixel spread by
 *  a factor of 1, 2, 4, or 8.
 *
 *  If the new index is valid, then the zoom index, expansion factor, and
 *  zoom itself are modified.
 */

bool
zoomer::zoom_in ()
{
    int index = m_zoom_index - 1;
    return set_zoom_by_index(index);
}

bool
zoomer::zoom_out ()
{
    int index = m_zoom_index + 1;
    return set_zoom_by_index(index);
}

/**
 *  This handles only the normal zooms, no zoom expansion support.
 *  It rejects zooms that are not powers of 2.
 */

bool
zoomer::set_zoom (int z)
{
    int index = log2_of_power_of_2(z);
    bool result = index >= 0;
    if (result)
        set_zoom_by_index(index);

    return result;
}

bool
zoomer::set_zoom_by_index (int i)
{
    bool result = false;
    if (i >= 0)
    {
        int z = zoom_item(i);
        if (z > 0)
        {
            m_zoom_index = i;
            m_zoom_expansion = 0;
            m_zoom = z;
            m_scale_zoom = zoom() * m_scale;
            result = true;
        }
    }
    else
    {
        m_zoom_expansion = expanded_zoom_item(i);
        if (expanded_zoom())
        {
            m_zoom_index = i;
            m_zoom = 1;
            result = true;
        }
    }
    return result;
}

bool
zoomer::reset_zoom (int ppq)
{
    if (ppq != 0)
        m_ppqn = ppq;

    return initialize();
}

/*
 * Takes screen coordinates, give us notes/keys (to be generalized to
 * other vertical user-interface quantities) and ticks (always the
 * horizontal user-interface quantity).  Compare this function to
 * qbase::pix_to_tix().
 */

midipulse
zoomer::pix_to_tix (int x) const
{
    midipulse result = x * pulses_per_pixel();
    if (expanded_zoom())
        result /= m_zoom_expansion;

    return result;
}

int
zoomer::tix_to_pix (midipulse ticks) const
{
    int result = ticks / pulses_per_pixel();
    if (expanded_zoom())
        result *= m_zoom_expansion;

    return result;
}

/**
 *  Handles changes to the PPQN value in one place.  Useful mainly at startup.
 */

bool
zoomer::change_ppqn (int p)
{
    m_scale_zoom = zoom() * m_scale;
    m_ppqn = p;
    return true;
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
zoomer::zoom_power_of_2 (int ppqn)
{
    int result = c_default_seq_zoom;
    if (ppqn > usr().base_ppqn())
    {
        int zoom = result * ppqn / usr().base_ppqn();
        result = next_power_of_2(zoom);
        if (result > c_maximum_zoom)
            result = c_maximum_zoom;
        else if (result == 0)
            result = c_minimum_zoom;
    }
    return result;
}

}           // namespace seq66

/*
 * settings.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


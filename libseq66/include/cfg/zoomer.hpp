#if ! defined SEQ66_ZOOMER_HPP
#define SEQ66_ZOOMER_HPP

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
 *  Place, Suite 330, Boston, MA  02111-1307  USA.
 */

/**
 * \file          zoomer.hpp
 *
 *  This module declares/defines the base class for operations common to all
 *  seq66 windows.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2023-09-08
 * \updates       2024-12-17
 * \license       GNU GPLv2 or above
 *
 */

#include "midi/midibytes.hpp"           /* seq66::midipulse, etc.           */

/*
 * Do not document namespaces, it breaks Doxygen.
 */

namespace seq66
{

/**
 *  FROM qseqeditbase!!! Maybe MOVE to settings!!!
 *
 *  The default value of the zoom indicates that one pixel represents two
 *  ticks.  However, it turns out we're going to have to support adapting the
 *  default zoom to the PPQN, in addition to allowing some extra zoom values.
 *  A redundant definition is used on the calculations module at present.
 *
 *  The maximum value of the zoom indicates that one pixel represents 512
 *  ticks.  The old maximum was 32, but now that we support PPQN up to 19200,
 *  we need extra entries.
 *
 *  Redundantly defined in usrsettings.
 */

const int c_minimum_zoom        =   1;  /* limit the amount of zoom         */
const int c_default_seq_zoom    =   2;  /* default snap from app limits     */
const int c_default_perf_zoom   =  16;  /* default snap from app limits     */
const int c_maximum_zoom        = 512;  /* limit the amount of zoom         */

/**
 *  This frame is the basis for editing an individual MIDI sequence.
 */

class zoomer
{
    /**
     *  Holds the current PPQN for convenience.
     */

    int m_ppqn;

    /**
     *  Provides the initial zoom, used for restoring the original zoom using
     *  the 0 key.
     */

    const int m_initial_zoom;

    /**
     *  Horizontal zoom setting.  This is the ratio between pixels and MIDI
     *  ticks, written "pixels:ticks".   As ticks increases, the effect is to
     *  zoom out, making the beats look shorter.  The default zoom is 2 for
     *  the normal PPQN of 192.
     *
     *  Provides the zoom values, as given by the zoom_items() and
     *  expanded_zoom_items() functions in the settings module.
     *
     *  The value of zoom is the same as the number of ticks per pixels on
     *  the piano roll.
     *
     *  Moved here to avoid warning about m_zoom unitialized as detected by
     *  g++ 14.2.1 on Arch Linux.
     */

    int m_zoom;

    /**
     *  X scaling.  Allows the caller to adjust the overall zoom. A
     *  constant.
     */

    const int m_scale;

    /**
     *  Zoom times the scale, to save a very common calculation,
     *  m_zoom * m_scale.
     */

    int m_scale_zoom;

    /**
     *  Provides the current zoom index. If 0 to 9, this is the index
     *  into the zoom_items() tokenization. If -1 to -4, this number is
     *  negated, and 1 is subtracted, to get an index into the
     *  expanded_zoom_items() tokenization.
     */

    int m_zoom_index;

    /**
     *  An additional kind of zoom, useful for depicting dense events such as
     *  pitch-bend.  All it does is multiply the pixel numbers by this factor.
     *  The supported values are 1 (the same as no expansion), 2, 4, and 8.
     *  It is accessible only via the zoom buttons and zoom keys, and applies
     *  only to the x (horizontal) direction. If set to 1, this value is not
     *  used.
     */

    int m_zoom_expansion;

public:

    zoomer ();
    zoomer (int ppq, int initialzoom, int scalex = 1);
    ~zoomer () = default;

public:

    bool zoom_in ();
    bool zoom_out ();
    bool set_zoom (int z);
    bool set_zoom_by_index (int i);
    bool reset_zoom (int ppq = 0);

    bool change_zoom (bool in)
    {
        return in ? zoom_in() : zoom_out() ;        /* calls the override   */
    }

    int zoom () const
    {
        return m_zoom;
    }

    int scale () const
    {
        return m_scale;
    }

    int ppqn () const
    {
        return m_ppqn;
    }

    int scale_zoom () const
    {
        return m_scale_zoom;
    }

    bool expanded_zoom () const
    {
        return m_zoom_expansion > 0;
    }

    int zoom_expansion () const
    {
        return m_zoom_expansion;
    }

    midipulse pix_to_tix (int x) const;
    int tix_to_pix (midipulse ticks) const;

    int xoffset (midipulse tick) const
    {
        return tix_to_pix(tick);            //  + m_padding_x;
    }

    bool change_ppqn (int ppq);
    int zoom_power_of_2 (int ppq);

public:     // new functions ca 2024-12-16

    int pulses_per_pixel () const
    {
        return zoom();
    }

    int pulses_per_substep () const
    {
        return 6 * pulses_per_pixel();
    }

    /*
     * return (bw > 0) ? 4 * ppqn() * bpb / bw / divisor : ppqn() ;
     */

    int pulses_per_partial_beat (int bpb = 4, int bw = 4) const
    {
        const int divisor = 4;
        int div = bw * divisor;
        return (bw > 0) ? ppqn() * bpb / div : ppqn() ;
    }

    int pulses_per_beat (int bw = 4) const
    {
        return (bw > 0) ? 4 * ppqn() / bw : ppqn() ;
    }

    int pulses_per_bar (int bpb = 4, int bw = 4) const
    {
        return (bw > 0) ? 4 * ppqn() * bpb / bw : ppqn() * bpb ;
    }

private:

    bool initialize ();

};          // class zoomer

}           // namespace seq66

#endif      // SEQ66_ZOOMER_HPP

/*
 * zoomer.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


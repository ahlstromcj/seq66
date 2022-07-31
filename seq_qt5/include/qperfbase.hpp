#if ! defined SEQ66_QPERFBASE_HPP
#define SEQ66_QPERFBASE_HPP

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
 * \file          qperfbase.hpp
 *
 *  This module declares/defines the base class for the various song panes
 *  of Seq66's Qt 5 version.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-07-14
 * \updates       2021-09-12
 * \license       GNU GPLv2 or above
 *
 *  This class WILL BE the base class for qseqroll, qseqdata, qtriggereditor,
 *  and qseqtime, the four panes of the qseqeditframe64 class or the legacy
 *  Kepler34 qseqeditframe class.
 *
 *  And maybe we can use it in the qperf* classes as well.
 */

#include "qeditbase.hpp"                /* seq66:qeditbase base class     */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class performer;
    class sequence;

/**
 *  Provides constants for the perfroll object (performance editor).
 *  Note the current dependence on the width of a font pixmap's character!
 *  So we will use the font's numeric accessors soon.
 */

const int c_names_x         = 6 * 24;   /* used in qperfroll, qperfnames    */
const int c_names_y         = 22;       /* used in qperfroll, qperfnames    */
const int c_perf_scale_x    = 32;       /* units are ticks per pixel        */

/**
 * The MIDI note grid in the sequence editor
 */

class qperfbase : public qeditbase
{

private:

    /**
     *  Allows for expansion of the song-editor horizontally.  Starts out
     *  at 1.25. See qperfbase::horizSizeHint().
     */

    float m_width_factor = 1.25f;

    /**
     *  Provides the height of the track and names displays.  Starts at
     *  c_name_y, and can be halved or doubled from that.  We could allow it
     *  to be more than doubled, but that doesn't seem necessary.  A height
     *  less than half is unworkable.
     */

    int m_track_height;

    /**
     *  Indicates if the track height is halved.
     */

    bool m_track_thin;

    /**
     *  Indicates if the track height is doubled.
     */

    bool m_track_thick;

public:

    qperfbase
    (
        performer & perf,
        int zoom        = c_default_zoom,
        int snap        = c_default_snap,
        int unitheight  = 1,
        int totalheight = 1
    );

    bool track_thin () const
    {
        return m_track_thin;
    }

    bool track_thick () const
    {
        return m_track_thick;
    }

    int track_height () const
    {
        return m_track_height;
    }

    void increment_width ()
    {
        m_width_factor += 0.50f;
    }

    float width_factor () const
    {
        return m_width_factor;
    }

protected:

    virtual int horizSizeHint () const override;
    void force_resize (QWidget *);

    void set_thin ()
    {
        m_track_height = c_names_y / 2;
        m_track_thick = false;
        m_track_thin = true;
    }

    void set_thick ()
    {
        m_track_height = c_names_y * 2;
        m_track_thick = true;
        m_track_thin = false;
    }

    void set_normal ()
    {
        m_track_height = c_names_y;
        m_track_thick = false;
        m_track_thin = false;
    }

protected:

    void convert_x (int x, midipulse & tick);
    void convert_xy (int x, int y, midipulse & ticks, int & seq);
    void convert_ts (midipulse ticks, int seq, int & x, int & y);
    void convert_ts_box_to_rect
    (
        midipulse tick_s, midipulse tick_f, int seq_h, int seq_l,
        seq66::rect & r
    );

private:

    virtual void update_midi_buttons () override
    {
        // TODO, in the derived qperf classes.
    }

};          // class qperfbase

}           // namespace seq66

#endif      // SEQ66_QPERFBASE_HPP

/*
 * qperfbase.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


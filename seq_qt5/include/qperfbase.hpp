#if ! defined SEQ66_QPERFBASE_HPP
#define SEQ66_QPERFBASE_HPP

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
 * \file          qperfbase.hpp
 *
 *  This module declares/defines the base class for the various song panes
 *  of Seq66's Qt 5 version.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-07-14
 * \updates       2019-08-06
 * \license       GNU GPLv2 or above
 *
 *  This class WILL BE the base class for qseqroll, qseqdata, qtriggereditor,
 *  and qseqtime, the four panes of the qseqeditframe64 class or the legacy
 *  Kepler34 qseqeditframe class.
 *
 *  And maybe we can use it in the qperf* classes as well.
 */

#include "qeditbase.hpp"                /* seq66:qeditbase base class     */
#include "util/rect.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class performer;
    class sequence;

/**
 * The MIDI note grid in the sequence editor
 */

class qperfbase : public qeditbase
{

private:

    // No additional members

public:

    qperfbase
    (
        performer & perf,
        int zoom,
        int snap            = SEQ66_DEFAULT_SNAP,
        int unit_height     = 1,
        int total_height    = 1
    );

protected:

    int horizSizeHint () const;

protected:

    void convert_x (int x, midipulse & tick);
    void convert_xy (int x, int y, midipulse & ticks, int & seq);
    void convert_ts (midipulse ticks, int seq, int & x, int & y);
    void convert_ts_box_to_rect
    (
        midipulse tick_s, midipulse tick_f, int seq_h, int seq_l,
        seq66::rect & r
    );

};          // class qperfbase

}           // namespace seq66

#endif      // SEQ66_QPERFBASE_HPP

/*
 * qperfbase.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


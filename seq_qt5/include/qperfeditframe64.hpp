#if ! defined SEQ66_QPERFEDITFRAME64_HPP
#define SEQ66_QPERFEDITFRAME64_HPP

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
 * \file          qperfeditframe64.hpp
 *
 *  This module declares/defines the base class for the Performance Editor,
 *  also known as the Song Editor.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-07-18
 * \updates       2020-07-24
 * \license       GNU GPLv2 or above
 *
 *  Note that the z and Z keys, when focus is on the perfroll (piano roll), will
 *  zoom the view horizontally. Other keys are also available.
 */

#include <QFrame>
#include <qmath.h>

#include "app_limits.h"                 /* SEQ66_USE_DEFAULT_PPQN           */
#include "midi/midibytes.hpp"           /* seq66::midipulse alias           */

class QPaintEvent;

/*
 * Do not document namespaces, it breaks Doxygen.
 */

namespace Ui
{
    class qperfeditframe64;
}

/*
 * Do not document namespaces, it breaks Doxygen.
 */

namespace seq66
{
    class performer;
    class qperfroll;
    class qperfnames;
    class qperftime;

/**
 *  This class is an improved version of qperfeditframe.
 */

class qperfeditframe64 final : public QFrame
{
    friend class qsmainwnd;
    friend class qperfroll;

private:

    Q_OBJECT

public:

    qperfeditframe64
    (
        performer & p,
        QWidget * parent = nullptr
    );

    virtual ~qperfeditframe64 ();

    int get_beat_width () const
    {
        return m_beat_width;
    }

    void set_beat_width (int bw)
    {
        m_beat_width = bw;
        set_guides();
    }

    int get_beats_per_measure () const
    {
        return m_beats_per_measure;
    }

    void set_beats_per_measure (int bpm)
    {
        m_beats_per_measure = bpm;
        set_guides();
    }

    void follow_progress ();
    void update_sizes ();
    void set_needs_update ();

private:

    void set_snap (midipulse s);
    void set_guides ();
    void grow ();

    performer & perf ()
    {
        return m_mainperf;
    }

    void reset_zoom ();
    void set_transpose (int transpose);

private slots:

    void updateGridSnap (int snapIndex);
    void zoom_in ();
    void zoom_out ();
    void reset_transpose ();
    void update_transpose (int index);
    void markerCollapse ();
    void markerExpand ();
    void markerExpandCopy ();
    void markerLoop (bool loop);
    void follow (bool ischecked);

private:

    Ui::qperfeditframe64 * ui;
    performer & m_mainperf;
    QPalette * m_palette;
    int m_snap;                 /* set snap-to in pulses/ticks  */
    int m_beats_per_measure;
    int m_beat_width;
    qperfroll * m_perfroll;
    qperfnames * m_perfnames;
    qperftime * m_perftime;

};              // class qperfeditframe64

}               // namespace seq66

#endif          // SEQ66_QPERFEDITFRAME64_HPP

/*
 * qperfeditframe64.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


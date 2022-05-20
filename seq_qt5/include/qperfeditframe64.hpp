#if ! defined SEQ66_QPERFEDITFRAME64_HPP
#define SEQ66_QPERFEDITFRAME64_HPP

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
 * \file          qperfeditframe64.hpp
 *
 *  This module declares/defines the base class for the Performance Editor,
 *  also known as the Song Editor.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-07-18
 * \updates       2022-05-20
 * \license       GNU GPLv2 or above
 *
 *  Note that the z and Z keys, when focus is on the perfroll (piano roll),
 *  will zoom the view horizontally. Other keys are also available.
 */

#include <QFrame>

#include "cfg/settings.hpp"             /* seq66::combolist class, helpers  */
#include "midi/midibytes.hpp"           /* seq66::midipulse alias           */
#include "qscrollmaster.h"              /* qscrollmaster::dir enum class    */

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

    qperfeditframe64 (performer & p, QWidget * parent, bool isexternal = false);

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
    void scroll_to_tick (midipulse tick);
    void update_sizes ();
    void set_dirty ();
    void set_loop_button (bool looping);

private:

    void set_snap (midipulse s);
    void set_guides ();

    const combolist & snap_list () const
    {
        return m_snap_list;
    }

    performer & perf ()
    {
        return m_mainperf;
    }

    void reset_zoom ();
    void set_transpose (int transpose);
    void update_entry_mode (bool on);
    void scroll_by_step (qscrollmaster::dir d);

protected:                          // overrides of event handlers

    virtual void keyPressEvent (QKeyEvent *) override;
    virtual void keyReleaseEvent (QKeyEvent *) override;

private slots:

    void update_grid_snap (int snapindex);
    void zoom_in ();
    void zoom_out ();
    void reset_transpose ();
    void update_transpose (int index);
    void marker_collapse ();
    void marker_expand ();
    void marker_expand_copy ();
    void marker_loop (bool loop);
    void grow ();
    void follow (bool ischecked);
    void entry_mode (bool ischecked);
    void slot_duration (bool ischecked);
    void reset_trigger_transpose (bool ischecked);
    void set_trigger_transpose (int tpose);
    void v_zoom_in ();
    void v_zoom_out ();
    void reset_v_zoom ();

private:

    Ui::qperfeditframe64 * ui;
    performer & m_mainperf;
    QPalette * m_palette;
    bool m_is_external;
    bool m_duration_mode;               /* true == H:M:S.fraction       */
    combolist m_snap_list;
    int m_snap;                         /* set snap-to in pulses/ticks  */
    int m_beats_per_measure;
    int m_beat_width;
    int m_trigger_transpose;
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


#if ! defined SEQ66_QPERFROLL_HPP
#define SEQ66_QPERFROLL_HPP

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
 * \file          qperfroll.hpp
 *
 *  This module declares/defines the base class for the Qt 5 version of the
 *  Performance window piano roll.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2025-06-15
 * \license       GNU GPLv2 or above
 *
 *  This class represents the central piano-roll user-interface area of the
 *  performance/song editor.
 */

#include <QWidget>

#include "qperfbase.hpp"                /* seq66::qperfbase base class      */

/*
 *  Forward references.
 */

class QKeyEvent;
class QMouseEvent;
class QTimer;

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class performer;
    class qperfeditframe64;
    class qperfnames;

/**
 * The grid in the song editor for setting out sequences
 */

class qperfroll final : public QWidget, public qperfbase
{
    friend class qperfeditframe64;  /* for scrolling a horizontal page  */

    Q_OBJECT

public:

    qperfroll
    (
        performer & p,
        int zoom,
        int snap                 = c_default_snap,
        qperfnames * seqnames    = nullptr,
        qperfeditframe64 * frame = nullptr,
        QWidget * parent         = nullptr
    );

    virtual ~qperfroll ();

    void set_guides (midipulse snap, midipulse measure, midipulse beat);

    void set_trigger_transpose (int tpose)
    {
        m_trigger_transpose = tpose;
    }

    bool v_zoom_in ();
    bool v_zoom_out ();
    bool reset_v_zoom ();
    virtual bool zoom_in () override;
    virtual bool zoom_out () override;
    virtual bool reset_zoom (int ppq = 0) override;

private:

    bool in_selection_area (midipulse tick);
    bool move_by_key (bool forward, bool single = true);
    int seq_id_from_xy (int /*click_x*/, int click_y);
    void draw_grid (QPainter & painter, const QRect & r);
    void draw_triggers (QPainter & painter, const QRect & r);

    void resize ()
    {
        force_resize(this);
    }

private:

    virtual void paintEvent (QPaintEvent *) override;
    virtual void mousePressEvent (QMouseEvent *) override;
    virtual void mouseReleaseEvent (QMouseEvent *) override;
    virtual void mouseMoveEvent (QMouseEvent *) override;
    virtual void mouseDoubleClickEvent (QMouseEvent *) override;
    virtual void keyPressEvent (QKeyEvent *) override;
    virtual void keyReleaseEvent (QKeyEvent *) override;
    virtual QSize sizeHint () const override;

signals:

    /*
     * Open or create an editor window for the selected pattern.
     */

    void signal_call_editor_ex (int seqid, bool active);

public slots:

    void undo ();
    void redo ();
    void conditional_update ();

private:

    virtual void set_adding (bool adding) override;

    qperfeditframe64 * frame64 ()
    {
        return m_parent_frame;
    }

    const qperfeditframe64 * frame64 () const
    {
        return m_parent_frame;
    }

    qperfnames * perf_names ()
    {
        return m_perf_names;
    }

    const qperfnames * perf_names () const
    {
        return m_perf_names;
    }

    // We could add these functions to performer and here:
    //
    //  cut_selected_trigger()
    //  copy_selected_trigger()
    //  paste_trigger()
    //  void half_split_trigger (int seq, midipulse tick);

    void add_trigger (int seq, midipulse tick);
    void delete_trigger (int seq, midipulse tick);
    void follow_progress ();

private:

    qperfeditframe64 * m_parent_frame;

    /**
     *  Holds a pointer to the qperfnames pane that is associated with the
     *  qperfroll piano roll.
     */

    qperfnames * m_perf_names;

    /**
     *  Pre-allocation of gradient brushes for drawing triggers.
     */

    QLinearGradient m_back_grad;
    QLinearGradient m_sel_grad;

    QTimer * m_timer;
    QFont m_font;
    int m_trigger_transpose;
    midipulse m_tick_s;                     // start of tick window
    midipulse m_tick_f;                     // end of tick window
    int m_seq_h;                            // highest seq in window
    int m_seq_l;                            // lowest seq in window
    int m_drop_track;                       // sequence selection
    midipulse m_drop_tick;
    midipulse m_drop_tick_offset;           // ticks clicked from trigger
    midipulse m_last_tick;                  // tick using at last mouse event
    bool m_box_select;
    bool m_grow_direction;
    bool m_adding_pressed;

};          // class qperfroll

}           // namespace seq66

#endif      // SEQ66_QPERFROLL_HPP

/*
 * qperfroll.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


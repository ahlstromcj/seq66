#if ! defined SEQ66_QSEQROLLPIX_HPP
#define SEQ66_QSEQROLLPIX_HPP

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
 * \file          qseqrollpix.hpp
 *
 *  This module declares/defines the base class for drawing on the piano
 *  roll of the patterns editor.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-06-04
 * \updates       2019-07-25
 * \license       GNU GPLv2 or above
 *
 *  We are currently moving toward making this class a base class.
 *
 *  User jean-emmanual added support for disabling the following of the
 *  progress bar during playback.  See the qseqbase::m_progress_follow member.
 */

#include <QPixmap>
#include <QTimer>
#include <QWidget>

#include <memory>                       /* std::unique_ptr<>                */
#include <vector>                       /* std::vector<>                    */

#include "qseqbase.hpp"                 /* seq66::qseqbase mixin class      */
#include "play/sequence.hpp"            /* seq66::sequence::editmode mode   */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class performer;
    class qseqeditframe64;
    class qseqframe;
    class qseqkeys;

/**
 *  A class that manages the information for a single pixmap frame.
 */

class rollframe
{
private:

    QPixmap m_grid_pixmap;    // std::unique_ptr<QPixmap> m_grid_pixmap;
    int m_frame_number;
    midipulse m_t0;
    midipulse m_t1;

public:

    rollframe () = delete;
    rollframe
    (
        const QPixmap & pixptr,
        int fnumber,
        midipulse t0,
        midipulse t1
    );
    rollframe (const rollframe & rhs);

    const QPixmap & get () const
    {
        return m_grid_pixmap;           // return m_grid_pixmap->get();
    }

    int frame () const
    {
        return m_frame_number;
    }

    midipulse t0 () const
    {
        return m_t0;
    }

    midipulse t1 () const
    {
        return m_t1;
    }

    void dump () const;

};


/**
 * The MIDI note grid in the sequence editor
 */

class qseqrollpix final : public QWidget, public qseqbase
{
    friend class qseqframe;
    friend class qseqeditframe;
    friend class qseqeditframe64;

    /**
     *  A container for rollframes.
     */

    using pixmaps = std::vector<rollframe>;

    Q_OBJECT

public:

    qseqrollpix
    (
        performer & perf,
        seq::pointer seqp,
        qseqkeys * seqkeys_wid  = nullptr,
        int zoom                = SEQ66_DEFAULT_ZOOM,
        int snap                = SEQ66_DEFAULT_SNAP,
        sequence::editmode mode = sequence::editmode::note,
        qseqframe * parent      = nullptr
    );

    virtual ~qseqrollpix ()
    {
        // no code needed
    }

    void follow_progress ();

    virtual bool zoom_in () override;
    virtual bool zoom_out () override;
    virtual bool reset_zoom () override;

    /**
     *  Zoom without forwarding to the parent frame.  To be called by the
     *  parent frame.  Slightly tricky, sigh.
     *
     * \param in
     *      If true, zoom in, otherwise zoom out.

    bool change_zoom (bool in)
    {
        if (in)
            qseqbase::zoom_in();
        else
            qseqbase::zoom_out();
    }
     */

private:

    /**
     * \getter m_note_length
     */

    int get_note_length () const
    {
        return m_note_length;
    }

    /**
     * \setter m_note_length
     */

    void set_note_length (int len)
    {
        m_note_length = len;
    }

    void set_chord (int chord);
    void set_key (int key);
    void set_scale (int scale);
    void set_background_sequence (bool state, int seq);
    void move_selected_notes (int dx, int dy);
    void grow_selected_notes (int dx);

private:

    virtual void paintEvent (QPaintEvent *) override;
    virtual void mousePressEvent (QMouseEvent *) override;
    virtual void mouseReleaseEvent (QMouseEvent *) override;
    virtual void mouseMoveEvent (QMouseEvent *) override;
    virtual void keyPressEvent (QKeyEvent *) override;
    virtual void keyReleaseEvent (QKeyEvent *) override;
    virtual QSize sizeHint () const override;

private:

    void set_note_coordinates (sequence::note_info & ni)
    {
        m_note_x = xoffset(ni.start());
        m_note_y = m_keyarea_y - (ni.note() * m_key_y) - m_key_y - 1 + 2;
    }

    void draw_notes
    (
        QPainter & painter,
        const rollframe & roll,
        bool background
    );
    void draw_drum_notes
    (
        QPainter & painter,
        const rollframe & roll,
        bool background
    );
    void draw_drum_note (QPainter & painter);
    void snap_y (int & y);
    void set_adding (bool a_adding);
    void start_paste();
    QPixmap * create_pixmap (int w, int h);
    bool create_pixmaps ();
    int analyze_width () const;

private:

    /**
     *  Holds a pointer to the scroll-master object in the edit-frame window.
     */

    qseqframe * m_parent_frame;     /* qseqeditframe64 or qseqeditframe */

    /**
     *  Avoids continual dynamic_cast tests.
     */

    bool m_is_new_edit_frame;

    /**
     *  Holds a pointer to the qseqkeys pane that is associated with the
     *  qseqrollpix piano roll.
     */

    qseqkeys * m_seqkeys_wid;

    /**
     *  The background pixmap, showing the piano roll grid, and nothing else.
     */

    std::unique_ptr<QPixmap> m_grid_pixmap;

    /**
     *  A complete set of prebuilt pixmaps representing the grid in a seqeuence
     *  piano roll.
     */

    pixmaps m_pixmap_list;

    /**
     *  Number of frames needed in current configuration.  Will change if zoom
     *  or pattern length changes.
     */

    mutable int m_pixmap_count;

    /**
     *
     */

    mutable int m_pixmap_width;

    /**
     *
     */

    mutable int m_pixmap_height;

    /**
     *  Indicates if the background pixmap has already been created.  Might be
     *  modified everytime sizeHint() is called.
     */

    mutable bool m_pixmaps_ready;

    /**
     *  A copy of the background pixmap, to be drawn with notes and the progress
     *  bar.
     */

    std::unique_ptr<QPixmap> m_grid_canvas;

    /**
     *  Screen update timer.
     */

    QTimer * m_timer;

    /**
     *  Indicates the musical scale in force for this sequence.
     */

    int m_scale;

    /**
     *  A position value.  Need to clarify what exactly this member is used
     *  for.
     */

    int m_pos;

    /**
     *  Indicates either that chord support is disabled (0), or a particular
     *  chord is to be created when inserting notes.
     */

    int m_chord;

    /**
     *  The current musical key selected.
     */

    int m_key;

    /**
     *  Holds the note length in force for this sequence.  Used in the
     *  seq66seqroll module only.
     */

    int m_note_length;

    /**
     *  Holds the value of the musical background sequence that is shown in
     *  cyan (formerly grey) on the background of the piano roll.
     */

    int m_background_sequence;

    /**
     *  Set to true if the drawing of the background sequence is to be done.
     */

    bool m_drawing_background_seq;

    /**
     *  The current status/event selected in the seqedit.  Not used in seqroll
     *  at present.
     */

    midibyte m_status;

    /**
     *  The current MIDI control value selected in the seqedit.  Not used in
     *  seqroll at present.
     */

    midibyte m_cc;

    /**
     *  Indicates the edit mode, note versus drum.
     */

    sequence::editmode m_edit_mode;

    int m_note_x;               // note drawing variables
    int m_note_width;
    int m_note_y;
    int m_note_height;
    int m_key_y;                // dimensions of height
    int m_keyarea_y;
    int m_keypadding_x;

    /**
     *  The current full width of the pattern, in pixels, based on the length
     *  of the pattern and the zoom value.  It is independent of the seqroll
     *  window size.  See also m_pixmap_width;
     */

    mutable int m_current_width;

    /**
     *  Current full height of the pattern, in pixels, based solely on the
     *  constant height of the seqkeys panel.  However, one day we might
     *  implement vertical zoom as well.  See also m_pixmap_height;
     */

    mutable int m_current_height;

    /**
     *  The starting time, in ticks, of the current frame.
     */

    mutable midipulse m_t0;

    /**
     *  The ending time, in ticks, of the current frame.
     */

    mutable midipulse m_t1;

    /**
     *  The width of a frame in ticks.
     */

    mutable midipulse m_frame_ticks;

    /**
     * Indicates the current frame in view.
     */

    int m_current_frame;

    /**
     *  The base number of ticks in the smallest division in the piano roll.
     *  This value is commonplace in Seq24 and beyond.
     */

    const int cm_base_step_ticks;

    /**
     *  The smallest horizontal step size in ticks.  This value provides the
     *  spacing for the narrowest vertical grid lines.  When zoom = 1 (the most
     *  zoomed in), this value is 6.  It is given by 6 * zoom. In the Gtkmm
     *  user-interface this value was called "m_ticks_per_step".  This will
     *  change if the zoom changes.
     */

    mutable midipulse m_ticks_per_step;

    /**
     *  The number of ticks per beat.  It is not the same as PPQN.  But it can
     *  change is PPQN is changed.
     */

    mutable midipulse m_ticks_per_beat;

    /**
     *  The number of ticks in a bar (or measure).  But it can change is PPQN is
     *  changed, or if the number of beats/bar changes.
     */

    mutable midipulse m_ticks_per_bar;

signals:

public slots:

    void conditional_update ();
    void update_edit_mode (sequence::editmode mode);

};          // class qseqrollpix

}           // namespace seq66

#endif      // SEQ66_QSEQROLLPIX_HPP

/*
 * qseqrollpix.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


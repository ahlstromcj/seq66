#if ! defined SEQ66_QSEQROLL_HPP
#define SEQ66_QSEQROLL_HPP

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
 * \file          qseqroll.hpp
 *
 *  This module declares/defines the base class for drawing on the piano
 *  roll of the patterns editor.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2019-10-03
 * \license       GNU GPLv2 or above
 *
 *  We are currently moving toward making this class a base class.
 *
 *  User jean-emmanual added support for disabling the following of the
 *  progress bar during playback.  See the qseqbase::m_progress_follow member.
 */

#include <QWidget>

#include "cfg/scales.hpp"               /* seq66::scales enum class         */
#include "play/sequence.hpp"            /* sequence::editmode mode          */
#include "qseqbase.hpp"                 /* seq66::qseqbase mixin class      */

/**
 *  Trying to erase the progress-bar (playhead) without redrawing the whole
 *  viewport.  Cannot yet figure it out fully!  Also even with drawing only the
 *  scrollbars, the CPU usage doesn't decrease.  So tabling this one for awhile.
 */

#undef  SEQ66_SEQROLL_PLAYHEAD_RENDER

#include "qrollframe.hpp"               /* seq66::qrollframe helper class   */

/*
 * Forward references
 */

class QPixmap;
class QTimer;

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
 * The MIDI note grid in the sequence editor
 */

class qseqroll final : public QWidget, public qseqbase
{
    friend class qseqframe;
    friend class qseqeditframe;
    friend class qseqeditframe64;

    Q_OBJECT

public:

    qseqroll
    (
        performer & perf,
        seq::pointer seqp,
        qseqkeys * seqkeys_wid  = nullptr,
        int zoom                = SEQ66_DEFAULT_ZOOM,
        int snap                = SEQ66_DEFAULT_SNAP,
        sequence::editmode mode = sequence::editmode::note,
        qseqframe * parent      = nullptr
    );

    virtual ~qseqroll ();

    void follow_progress ();

    virtual bool zoom_in () override;
    virtual bool zoom_out () override;
    virtual bool reset_zoom () override;

protected:

    virtual void scroll_offset (int v) override;
    virtual int scroll_offset () const override
    {
        return qseqbase::scroll_offset();
    }

    void set_redraw ();

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

protected:      // overrides for painting, mouse/keyboard events, & size hints

    virtual void paintEvent (QPaintEvent *) override;
    virtual void resizeEvent (QResizeEvent *) override;
    virtual void mousePressEvent (QMouseEvent *) override;
    virtual void mouseReleaseEvent (QMouseEvent *) override;
    virtual void mouseMoveEvent (QMouseEvent *) override;
    virtual void keyPressEvent (QKeyEvent *) override;
    virtual void keyReleaseEvent (QKeyEvent *) override;
    virtual QSize sizeHint () const override;

private:

    void move_selected_notes (int dx, int dy);
    void grow_selected_notes (int dx);
    void snap_y (int & y);
    void set_adding (bool a_adding);
    void start_paste();
    void draw_grid (QPainter & painter, const QRect & r);
    void draw_notes (QPainter & painter, const QRect & r, bool background);
    void draw_drum_notes (QPainter & painter, const QRect & r, bool background);
    void draw_drum_note (QPainter & painter);
    void call_draw_notes (QPainter & painter, const QRect & view);

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
     *  qseqroll piano roll.
     */

    qseqkeys * m_seqkeys_wid;

    /**
     *  Screen update timer.
     */

    QTimer * m_timer;

    /**
     *  The width, in pixels, of the progress-bar/playhead.  Usually 1 or 2
     *  pixels.
     */

    int m_progbar_width;

    /**
     *  Manages the "erasing" of the previous progress-bar (playhead) by
     *  restoring the background grid and notes.  Even if the full functionality
     *  is disabled, we still use it for frame management.
     */

    qrollframe m_roll_frame;

    /**
     *  Indicates the musical scale in force for this sequence.
     */

    scales m_scale;

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

    /**
     *  Indicates to draw the whole grid.
     */

    bool m_draw_whole_grid;

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

    int m_note_x;               // note drawing variables
    int m_note_width;
    int m_note_y;
    int m_note_height;
    int m_keypadding_x;

    /**
     *  Hold the note value first grabbed when starting a move.
     */

    int m_last_base_note;

signals:

public slots:

    void conditional_update ();
    void update_edit_mode (sequence::editmode mode);

};          // class qseqroll

}           // namespace seq66

#endif      // SEQ66_QSEQROLL_HPP

/*
 * qseqroll.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


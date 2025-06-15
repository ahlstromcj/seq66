#if ! defined SEQ66_QSEQROLL_HPP
#define SEQ66_QSEQROLL_HPP

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
 * \file          qseqroll.hpp
 *
 *  This module declares/defines the base class for drawing on the piano
 *  roll of the patterns editor.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2025-06-15
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
#include "util/rect.hpp"                /* seq66::rect class                */
#include "qseqbase.hpp"                 /* seq66::qseqbase mixin class      */

/*
 * Forward references
 */

class QLabel;
class QMessageBox;
class qscrollmaster;
class QTimer;

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class performer;
    class qseqeditframe64;
    class qseqkeys;

/**
 * The MIDI note grid in the pattern (sequence) editor.
 */

class qseqroll final : public QWidget, public qseqbase
{
    friend class qseqframe;             /* for qseqroll::set_dirty() access */
    friend class qseqeditframe64;

    Q_OBJECT

public:

    qseqroll
    (
        performer & perf,
        sequence & s,
        qseqeditframe64 * parent,
        qseqkeys * seqkeys_wid,
        int zoom                    = c_default_seq_zoom,
        int snap                    = c_default_snap,
        sequence::editmode mode     = sequence::editmode::note,
        int unit_height             =  1,
        int total_height            =  1
    );

    virtual ~qseqroll ();

    bool follow_progress (qscrollmaster * qsm, bool expand = false);
    int note_height () const;
    bool v_zoom_in ();
    bool v_zoom_out ();
    bool reset_v_zoom ();
    virtual bool zoom_in () override;
    virtual bool zoom_out () override;
    virtual bool reset_zoom (int ppq = 0) override;

    const Color & backseq_color () const
    {
        return m_backseq_color;
    }

protected:

    virtual void set_dirty () override;

private:

    virtual void scroll_offset (int v) override;

    virtual int scroll_offset () const override
    {
        return qseqbase::scroll_offset();
    }

    void flag_dirty ();                 /* tricky code */
    void set_redraw ();

    bool is_drum_mode () const
    {
        return m_edit_mode == sequence::editmode::drum;
    }

    int get_note_length () const
    {
        return m_note_length;
    }

    void set_note_length (int len)
    {
        m_note_length = len;
    }

    int note_to_pix (int n) const;
    void set_chord (int chord);
    void set_key (int key);
    void set_scale (int scale);
    void set_background_sequence (bool state, int seq);
    void analyze_seq_notes ();
    void show_note_tooltip (int mx, int my);
    int note_off_length () const;
    bool add_painted_note (midipulse tick, int note);
    bool zoom_key_press (bool shifted, int key);
    bool movement_key_press (int key);
    bool get_selected_box ();

private:        // overrides for painting, mouse/keyboard events, & size hints

    virtual void paintEvent (QPaintEvent *) override;
    virtual void resizeEvent (QResizeEvent *) override;
    virtual void mousePressEvent (QMouseEvent *) override;
    virtual void mouseReleaseEvent (QMouseEvent *) override;
    virtual void mouseMoveEvent (QMouseEvent *) override;
    virtual void keyPressEvent (QKeyEvent *) override;
    virtual QSize sizeHint () const override;

private:

    virtual void set_adding (bool a_adding) override;

#if defined USE_GROW_SELECTED_NOTES_FUNCTION
    void grow_selected_notes (int dx);
#endif

    void set_tooltip_mode (bool ischecked)
    {
        m_show_note_info = ischecked;
    }

    int snapped_x (int x);

    void move_selected_notes (int dx, int dy);
    void snap_y (int & y);
    void start_paste();
    void draw_grid (QPainter & painter, const QRect & r);
    void draw_notes (QPainter & painter, const QRect & r, bool background);
    void draw_drum_notes (QPainter & painter, const QRect & r, bool background);
    void draw_drum_note (QPainter & painter, int x, int y);
    void call_draw_notes (QPainter & painter, const QRect & view);

#if defined SEQ66_SHOW_TEMPO_IN_PIANO_ROLL
    void draw_tempo (QPainter & painter, int x, int y, int velocity);
#endif

    void draw_ghost_notes
    (
        QPainter & painter,
        const seq66::rect & selection   /* why is seq66 scoped needed???    */
    );

private:

    /**
     *  Pre-allocation of gradient brushes for drawing notes.
     */

    QLinearGradient m_note_grad;
    QLinearGradient m_wrap_grad;
    QLinearGradient m_sel_grad;

    /**
     *  Used for showing the estimated scale/key upon a Ctrl-K in the qseqroll.
     */

    QMessageBox * m_analysis_msg;
    QFont m_font;

    /**
     *  The color (from the palette) for the background sequence.
     */

    const Color m_backseq_color;

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

    keys m_key;

    /**
     *  If true (the default is false), this will allow hovering to show
     *  the values for a note in a tooltip.
     */

    bool m_show_note_info;

    /**
     *  The label that serves as a tooltip.
     */

    QLabel * m_note_tooltip;

    /**
     *  Holds the note length in force for this sequence.  Used in the
     *  seq66seqroll module only.
     */

    int m_note_length;

    /**
     *  Provides the number of ticks to shave off of the end of painted notes.
     *  Also used when the user attempts to shrink a note to zero (or less
     *  than zero) length.
     */

    const midipulse m_note_off_margin;

    /**
     *  Holds the value of the musical background sequence that is shown in
     *  cyan (formerly grey) on the background of the piano roll.
     */

    int m_background_sequence;

    /**
     *  Set to true if the drawing of the background sequence is to be done.
     */

    bool m_draw_background_seq;

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

    int m_note_x;               // note drawing variables; can we dump 'em?
    int m_note_width;
    int m_note_y;

    /**
     *  Offset for keys.
     */

    int m_keypadding_x;
    bool m_v_zooming;

    /**
     *  The ranges of the selection, needed to draw ghost notes.
     *  The coordinates are of the form (x, y) == (ticks, pixels).
     *  The top-left corner is (start tick, high note) and the
     *  bottom right corner is (end tick, low note). It is anticipated
     *  that the highest tick possible will fit within an int.
     */

    seq66::rect m_selection;

    /**
     *  Hold the note value first grabbed when starting a move.
     */

    int m_last_base_note;

    /**
     *  Stores the setting of usr().pattern_wraparound().  It is used in
     *  drawing wrapped notes.
     */

    bool m_link_wraparound;

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


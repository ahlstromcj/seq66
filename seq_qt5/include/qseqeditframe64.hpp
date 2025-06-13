#if ! defined SEQ66_QSEQEDITFRAME64_HPP
#define SEQ66_QSEQEDITFRAME64_HPP

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
 * \file          qseqeditframe64.hpp
 *
 *  This module declares/defines the edit frame for sequences.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-06-15
 * \updates       2025-06-12
 * \license       GNU GPLv2 or above
 *
 */

#include "qscrollmaster.h"              /* qscrollmaster::dir enum class    */
#include "qseqframe.hpp"                /* QFrame and seq66::qseqframe      */
#include "cfg/settings.hpp"             /* seq66::combolist class, helpers  */
#include "play/performer.hpp"           /* seq66::performer::callbacks      */
#include "play/setmapper.hpp"           /* seq66::setmapper and others      */

/**
 *  Need to check before applying the change? Currently the measure length
 *  is adjusted. Kept around "just in case".
 */

#undef  USE_WOULD_TRUNCATE_BPB_BW

/**
 *  Experimenting with weird issue where changing the number of measures
 *  via the length drop-down works in the external pattern editor, but
 *  not in the tab. The old code (circa 0.99.12) does not work, either.
 */

#undef USE_LEGACY_MEASURES_ADJUSTMENT

/**
 *  Specifies the reported final size of the main window when the larger edit
 *  frame "kicks in".  See the comments for qsmainwnd::refresh().  The final
 *  vertical size of the main window ends up at around 700, puzzling!  The
 *  vertical size of the "external" edit-frame is only about 600.  Here are
 *  the current measured (via kruler) heights:
 *
 *      -   Top panel: 90
 *      -   Time pane: 20
 *      -   Roll pane: 250
 *      -   Event pane: 27
 *      -   Data pane: 128
 *      -   Bottom panel: 57
 *
 *  That total is 572.
 *
 *      -   qseqframe_height = 558, qseqeditframe64.ui
 *      -   qsmainwnd_height = 580, qsmainwnd.ui
 */

/*
 *  A few forward declarations.  The Qt header files are in the cpp file.
 */

class QIcon;
class QMenu;
class QWidget;

/*
 * Do not document namespaces, it breaks Doxygen.
 */

namespace Ui
{
    class qseqeditframe64;
}

/*
 * Do not document namespaces, it breaks Doxygen.  Note that the forward
 * references somewhat duplicate those in qseqframe.  FIX IT!
 */

namespace seq66
{
    class qlfoframe;
    class qpatternfix;
    class qseqeditex;
    class screenset;

/**
 *  This frame holds tools for editing an individual MIDI sequence.  This
 *  frame is a more advanced version of qseqeditframe (now moved to
 *  contrib/code), which was based on Kepler34's EditFrame class.
 */

class qseqeditframe64 final :
    public qseqframe,
    protected performer::callbacks
{
    friend class qlfoframe;
    friend class qpatternfix;
    friend class qperfroll;
    friend class qseqbase;
    friend class qseqdata;
    friend class qseqeditex;
    friend class qseqkeys;
    friend class qseqroll;
    friend class qseqtime;
    friend class qstriggereditor;

    Q_OBJECT

private:

    /**
     *  Enumerates the events we support for editing.  Note that tempo and
     *  time-signature are meta events and must be handled differently.
     *  And text covers a number of difference meta text events.
     */

    enum class event_index
    {
        note_on,
        note_off,
        aftertouch,
        control_change,
        program_change,
        channel_pressure,
        pitch_wheel,
        tempo,
        time_signature,
        text
    };

public:

    qseqeditframe64
    (
        performer & p,
        sequence & s,
        QWidget * parent,
        bool shorter = false
    );
    virtual ~qseqeditframe64 ();

    void get_position (int & x, int & y);

    void initialize_panels ();
    void set_editor_mode (sequence::editmode mode);
    bool follow_progress (bool expand = false);
    void scroll_to_tick (midipulse tick);
    void scroll_to_note (int note);

    int edit_channel () const
    {
        return m_edit_channel;
    }

protected:

    void set_track_change (bool modified = true);
    void set_external_frame_title (bool modified = true);
    void adjust_for_zoom (int zprevious);
    bool zoom_key_press (bool shifted, int key);

private:        /* performer::callback overrides    */

    virtual bool on_automation_change (automation::slot s) override;
    virtual bool on_sequence_change
    (
        seq::number seqno, performer::change ctype
    ) override;
    virtual bool on_trigger_change
    (
        seq::number seqno, performer::change mod
    ) override;
    virtual bool on_resolution_change
    (
        int ppqn, midibpm bp, performer::change ch
    ) override;

private:        /* qbase and qseqframe overrides    */

    virtual bool change_ppqn (int ppqn) override;
    virtual void update_midi_buttons () override;
    virtual void update_note_entry (bool on) override;
    virtual void set_dirty () override;
    virtual bool zoom_in () override;
    virtual bool zoom_out () override;
    virtual bool set_zoom (int z) override;
    virtual bool reset_zoom (int ppq = 0) override;
    virtual void update_draw_geometry () override;

protected:      /* QWidget overrides                */

    virtual bool eventFilter (QObject * target, QEvent * event) override;
    virtual void paintEvent (QPaintEvent * ) override;
    virtual void resizeEvent (QResizeEvent *) override;
    virtual void wheelEvent (QWheelEvent *) override;
    virtual void keyPressEvent (QKeyEvent *) override;
    virtual void keyReleaseEvent (QKeyEvent *) override;
    virtual void closeEvent (QCloseEvent *) override;

private:

    bool short_version () const
    {
        return m_short_version;
    }

#if defined USE_WOULD_TRUNCATE_BPB_BW
    bool would_truncate (int bpb, int bw);
#endif
    bool would_truncate (int measures);
    void scroll_by_step (qscrollmaster::dir d);
    void remove_lfo_frame ();
    void remove_patternfix_frame ();
    QIcon * create_menu_image (bool state);
    void set_log_timesig_text (int bpb, int bw);
    void set_log_timesig_status (bool flag);
    bool log_timesig (bool islogbutton);
    bool detect_time_signature ();
    void setup_record_styles ();
    void setup_alterations ();
    void q_record_change (alteration mode, toggler t);
    void set_toggle_qrecord_button ();

private:        /* combo-box list accessors */

    const combolist & measures_list () const
    {
        return m_measures_list;
    }

    const combolist & beats_per_bar_list () const
    {
        return m_beats_per_bar_list;
    }

    const combolist & beatwidth_list () const
    {
        return m_beatwidth_list;
    }

    const combolist & snap_list () const
    {
        return m_snap_list;
    }

    const combolist & zoom_list () const
    {
        return m_zoom_list;
    }

    const combolist & rec_vol_list () const
    {
        return m_rec_vol_list;
    }

signals:

private slots:

    void conditional_update ();
    void slot_reset_zoom ();
    void slot_update_zoom (int index);
    void update_seq_name ();
    void slot_log_timesig ();
    void slot_spacer_button_2 ();
    void update_beats_per_bar (int index);
    void text_beats_per_bar ();
    void update_beat_width (int index);
    void text_beat_width ();
    void reset_beats_per_bar ();
    void reset_beat_width ();
#if defined USE_LEGACY_MEASURES_ADJUSTMENT
    void text_measures (const QString & text);
#else
    void update_measures (int index);
    void text_measures_edit ();
#endif
    void reset_measures ();
    void transpose (bool ischecked);
    void update_chord (int index);
    void reset_chord ();
    void update_midi_bus (int index);
    void reset_midi_bus ();
    void update_midi_channel (int index);
    void reset_midi_channel ();
    void undo ();
    void redo ();

    /*
     * Tools button and handlers.
     */

    void tools ();
    void select_all_notes ();
    void inverse_note_selection ();
    void quantize_notes ();
    void tighten_notes ();
    void jitter_notes ();
    void randomize_note_velocities ();
    void transpose_notes ();
    void transpose_harmonic ();
    void remap_notes ();
    void tooltip_mode (bool ischecked);
    void note_entry (bool ischecked);

    /*
     * More slots.
     */

    void sequences ();
    void update_grid_snap (int index);
    void reset_grid_snap ();
    void update_note_length (int index);
    void reset_note_length ();
    void update_key (int index);
    void reset_key ();
    void update_scale (int index);
    void reset_scale ();
    void editor_mode (bool ischecked);
    void loop_mode (bool ischecked);
    void events ();
    void data ();
    void show_lfo_frame ();
    void show_pattern_fix ();
    void slot_play_change (bool ischecked);
    void slot_thru_change (bool ischecked);
    void slot_record_change (bool ischecked);
    void slot_q_record_change (bool ischecked);
    void slot_record_style (int index);
    void slot_recording_volume (int index);
    void slot_loop_count (int value);
    void reset_recording_volume ();
    void slot_follow (bool ischecked);
    void v_zoom_in ();
    void v_zoom_out ();
    void reset_v_zoom ();

private:        /* slot helper functions        */

    void do_action (eventlist::edit action, int var);
    void popup_tool_menu ();
    void popup_sequence_menu ();
    void repopulate_usr_combos (int buss, int channel);
    void repopulate_event_menu (int buss, int channel);
    void repopulate_mini_event_menu (int buss, int channel);
    void repopulate_midich_combo (int buss);
    bool add_back_set (QMenu ** qm, screenset & s, screenset::number index);
    bool add_back_sequence (QMenu ** qm, seq::pointer p, seq::number sn);

private:        /* setters and getters          */

    void set_beats_per_bar (int bpm, qbase::status qs = qbase::status::edit);
    void set_beat_width (int bw, qbase::status qs = qbase::status::edit);
    void set_bpb_and_bw
    (
        int bpb, int bw, qbase::status qs = qbase::status::edit
    );
    void set_measures (int len, qbase::status qs = qbase::status::edit);
    void set_midi_channel
    (
        int midichannel,
        qbase::status qs = qbase::status::edit
    );
    void set_midi_bus (int midibus, qbase::status qs = qbase::status::edit);
    void set_note_length (int nlen);
    void set_snap (midipulse s);
    void set_chord (int chord);
    void set_key (int key, qbase::status qs = qbase::status::edit);
    void set_scale (int key, qbase::status qs = qbase::status::edit);
    void set_background_sequence
    (
        int seqnum,
        qbase::status qs = qbase::status::edit
    );
    void set_transpose_image (bool istransposable);
    void set_event_entry
    (
        QMenu * menu,
        const std::string & text,
        bool present,
        midibyte status,
        midibyte control = 0
    );
    void set_event_entry (QMenu * menu, bool present, event_index ei);
    void set_data_type (midibyte status, midibyte control = 0);
    void set_recording_volume (int recvol);
    void enable_note_menus ();
    QWidget * rollview ();
    QWidget * rollwidget () const;

private:

    /**
     *  Needed for Qt.
     */

    Ui::qseqeditframe64 * ui;

    /**
     *  We want to support holding this frame in a qseqeditex window, to
     *  be able modify the parent's title bar and get position information.
     *  is a qseqeditex object. If null, then this frame is embedded in the
     *  main window.
     */

    qseqeditex * m_qseqeditex_frame;

    /**
     *  This item is not null if this frame is embedded in the main window.
     *  It is actually the Edit tab widget.
     */

    QWidget * m_edit_tab_widget;

    /**
     *  Indicates to compress this window vertically, for use in the Edit tab.
     */

    bool m_short_version;

    /**
     *  Hold the current L/R looping status.
     */

    bool m_is_looping;

    /**
     *  The LFO window object that might used by the pattern editor.
     */

    qlfoframe * m_lfo_wnd;

    /**
     *  The pattern-fix window object that might used by the pattern editor.
     */

    qpatternfix * m_patternfix_wnd;

    /**
     *  Menus for Tools and its Harmonic Transpose sub-menu.
     *  Menu for the Background Sequences button.
     *  Menu for the Event Data button.
     *  Menu for the "mini" Event Data button.
     */

    QMenu * m_tools_popup;
    QMenu * m_tools_harmonic;
    QMenu * m_tools_pitch;
    QMenu * m_tools_timing;
    QMenu * m_sequences_popup;
    QMenu * m_events_popup;
    QMenu * m_minidata_popup;

    /**
     *  Holds the measure selection for the beats-per-measure combo-box.
     */

    combolist m_measures_list;

    /**
     *  Provides the length of the sequence in measures.
     */

    int m_measures;

    /**
     *  Holds the beats-per-bar selection for the beats-per-bar combo-box.
     */

    combolist m_beats_per_bar_list;

    /**
     *  Holds the current beats-per-measure selection and the value to log
     *  when the time-sig button is clicked.
     */

    int m_beats_per_bar;
    int m_beats_per_bar_to_log;

    /**
     *  Holds the beat-width selection for the beats-width combo-box.
     */

    combolist m_beatwidth_list;

    /**
     *  Holds the current beat-width selection and the value to log
     *  when the time-sig button is clicked.
     */

    int m_beat_width;
    int m_beat_width_to_log;

    /**
     *  Holds the list for the snap settings.  These also apply to the note
     *  settings.
     */

    combolist m_snap_list;

    /**
     *  Used in setting the snap-to value in pulses, off = 1.
     */

    int m_snap;

    /**
     *  Holds the list for the zoom settings.
     */

    combolist m_zoom_list;

    /**
     *  Holds the list for the zoom settings.
     */

    combolist m_rec_vol_list;

    /**
     *  The default length of a note to be inserted by a right-left-click
     *  operation.
     */

    int m_note_length;

    /**
     *  Setting for the music scale, can now be saved with the sequence.
     */

    int m_scale;

    /**
     *  Setting for the current chord generation; not now saved with the
     *  sequence.
     */

    int m_chord;

    /**
     *  Setting for the music key, can now be saved with the sequence.
     */

    int m_key;

    /**
     *  Setting for the background sequence, can now be saved with the
     *  sequence.
     */

    int m_bgsequence;

    /**
     *  Indicates what MIDI output bus is active for this pattern.
     */

    bussbyte m_edit_bus;

    /**
     *  Indicates what MIDI input bus is active for this pattern.
     *  Currently not part of the user-interface.
     */

    bussbyte m_edit_in_bus;

    /**
     *  Indicates what MIDI channel the data window is currently editing.
     */

    int m_edit_channel;

    /**
     *  Indicates the first event found in the sequence while setting up the
     *  data menu via set_event_entry().  If no events exist, the value is
     *  max_midibyte() [0xFF].
     */

    midibyte m_first_event;

    /**
     *  Provides the string describing the first event, or "(no events)".
     */

    std::string m_first_event_name;

    /**
     *  Indicates that the focus has already been changed to this sequence.
     */

    bool m_have_focus;

    /**
     *  Indicates if this sequence is in note-edit more versus drum-edit mode.
     */

    sequence::editmode m_edit_mode;

    /**
     *  Indicates the last-selected recording mode, for use with safely using
     *  the one-shot reset option.
     */

    recordstyle m_last_record_style;

    /**
     * TO DO
     */

    bool m_armed_status;

    /**
     *  Update timer for pass-along to the roll, event, and data classes.
     */

    QTimer * m_timer;

private:

    /*
     * Documented in the cpp file.
     */

    static int sm_initial_snap;
    static int sm_initial_note_length;
    static int sm_initial_chord;

};          // class qseqeditframe64

}           // namespace seq66

#endif      // SEQ66_QSEQEDITFRAME64_HPP

/*
 * qseqeditframe64.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


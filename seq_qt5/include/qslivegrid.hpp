#if ! defined SEQ66_QSLIVEGRID_HPP
#define SEQ66_QSLIVEGRID_HPP

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
 * \file          qslivegrid.hpp
 *
 *  This module declares/defines the base class for the Qt 5 version of
 *  the pattern window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-06-21
 * \updates       2025-05-10
 * \license       GNU GPLv2 or above
 *
 *
 *  The qslivegrid is Sequencer66's alternative to the qsliveframe class (now
 *  moved to contrib/code for posterity).  But instead of a large pixmap, it
 *  consists of a grid of pushbuttons.
 */

#include <functional>                   /* std::function, function objects  */
#include <vector>                       /* std::vector<>                    */

#include "qslivebase.hpp"               /* seq66::qslivebase ABC            */
#include "play/screenset.hpp"           /* seq66::screenset class           */

/*
 * Qt forward references.
 */

class QMenu;
class QTimer;
class QMessageBox;

/*
 * Do not document namespaces.
 */

namespace Ui
{
    class qslivegrid;
}

namespace seq66
{
    class keystroke;
    class performer;
    class qsmainwnd;
    class qslotbutton;

/**
 *  Provides a grid of Qt buttons to implement the Live frame.
 *
 *  The protected inheritance could be moved to qslivebase, maybe.
 */

class qslivegrid final :
    public qslivebase,
    protected performer::callbacks
{
    friend class qsmainwnd;
    friend class qliveframeex;

private:

    /**
     *  This vector represents all buttons. Rows and columns calculated only
     *  when necessary.
     */

    using buttons = std::vector<qslotbutton *>;

private:

    Q_OBJECT

public:

    qslivegrid
    (
        performer & perf,                   /* performance master   */
        qsmainwnd * window,                 /* functional parent    */
        screenset::number bank = screenset::unassigned(),
        QWidget * parent = nullptr          /* the Qt parent        */
    );
    virtual ~qslivegrid ();

private:                            // overrides of qslivebase functions

    virtual void refresh () override
    {
        qslivebase::refresh();
        (void) refresh_all_slots();
    }

    virtual void refresh (seq::number seqno) override
    {
        if (seqno == seq::all())
            refresh();
    }

    virtual void color_by_number (int i) override;
    virtual void set_playlist_name
    (
        const std::string & plname = "",
        bool modified = false
    ) override;
    virtual void set_bank_values
    (
        const std::string & name = "",
        int id = 0
    ) override;
    virtual void update_bank (int bank) override;
    virtual void update_bank () override;
    virtual void update_bank_name (const std::string & name) override;
    virtual void update_sequence (seq::number seqno, bool redo) override;
    virtual void reupdate () override;
    virtual void update_geometry () override;
    virtual void change_event (QEvent *) override;

private:        /* performer::callback overrides    */

    virtual bool on_trigger_change
    (
        seq::number seqno, performer::change mod
    ) override;
    virtual bool on_automation_change (automation::slot s) override;

private:                                // overrides of event handlers

    virtual void paintEvent (QPaintEvent *) override;
    virtual void resizeEvent (QResizeEvent *) override;
    virtual void mousePressEvent (QMouseEvent *) override;
    virtual void mouseReleaseEvent (QMouseEvent *) override;
    virtual void mouseMoveEvent (QMouseEvent *) override;
    virtual void mouseDoubleClickEvent (QMouseEvent *) override;
    virtual void keyPressEvent (QKeyEvent *) override;
    virtual void keyReleaseEvent (QKeyEvent *) override;
    virtual void changeEvent (QEvent *) override;

    /*
     * Trial for drag-and-drop onto the Live grid.
     */

    virtual void dragEnterEvent (QDragEnterEvent * event) override;
    virtual void dragMoveEvent (QDragMoveEvent * event) override;
    virtual void dragLeaveEvent (QDragLeaveEvent * event) override;
    virtual void dropEvent (QDropEvent * event) override;

private:

    virtual bool recreate_all_slots () override;

    bool can_clear () const
    {
        return ! perf().is_seq_empty(current_seq());
    }

    seq::number seq_id_from_xy (int click_x, int click_y);
    qslotbutton * create_one_button (seq::number seqno);
    qslotbutton * button (int row, int column);
    qslotbutton * loop_button (seq::number seqno);
    bool get_slot_coordinate (int x, int y, int & row, int & column);
    bool handle_key_press (const keystroke & k);
    bool handle_key_release (const keystroke & k);
    bool delete_slot (int row, int column);
    bool delete_slot (seq::number seqno);
    bool delete_all_slots ();
    bool refresh_all_slots ();
    bool modify_slot (qslotbutton * newslot, int row, int column);
    void button_toggle_enabled (seq::number seqno);
    void button_toggle_checked (seq::number seqno);
    void alter_sequence (seq::number seqno);
    void create_loop_buttons ();
    void clear_loop_buttons ();
    void measure_loop_buttons ();
    void setup_button (qslotbutton * pb);
    void popup_menu ();
    void sequence_key_check ();
    void show_record_style ();
    void show_record_alteration ();
    void show_grid_mode ();
    void populate_grid_mode ();
    void set_grid_mode ();
    void enable_solo (bool enable);
    void update_state ();                               /* ca 2023-04-26    */

signals:

    void signal_call_editor (int seqid);        /* editor tab for pattern      */
    void signal_call_editor_ex (int seqid);     /* editor window for pattern   */
    void signal_call_edit_events (int seqid);   /* event tab for pattern       */
    void signal_live_frame (int ssnum);         /* call live frame for screen# */

private slots:

    void conditional_update ();
    void new_sequence ();
    void edit_sequence ();
    void edit_sequence_ex ();
    void edit_events ();
    void record_sequence ();
    void flatten_sequence ();
    void copy_sequence ();
    void cut_sequence ();
    void paste_sequence ();
    void merge_sequence ();
    void delete_sequence ();
    void clear_sequence ();
    void new_live_frame ();
    void slot_set_bank_name ();
    void slot_activate_bank (bool clicked);
    void slot_record_style (bool clicked);
    void slot_record_alteration (bool clicked);
    void slot_toggle_metronome (bool clicked);
    void slot_toggle_background_record (bool clicked);
    void slot_grid_mode (int index);

private:

    Ui::qslivegrid * ui;
    QMenu * m_popup;
    QTimer * m_timer;
    QMessageBox * m_msg_box;

    /**
     *  Indicates if the buttons should (re)drawn.
     */

    bool m_redraw_buttons;

    /**
     *  A two-dimensional vector of buttons containing a vector of rows, each
     *  row being a vector of columns.
     *
     *  The fastest varying index is the row: m_loop_buttons[column][row].
     */

    buttons m_loop_buttons;

    /**
     *  Layout of buttons for determining sequence numbers.
     */

    int m_x_min;
    int m_x_max;
    int m_y_min;
    int m_y_max;

};              // class qslivegrid

}               // namespace seq66

#endif          // SEQ66_QSLIVEGRID_HPP

/*
 * qslivegrid.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


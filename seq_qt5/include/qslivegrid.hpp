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
 * \updates       2021-07-26
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
 */

class qslivegrid final : public qslivebase
{
    friend class qsmainwnd;
    friend class qliveframeex;

private:

    /**
     *  This vector represents a row of buttons.  Its size will be the number
     *  of columns in the grid.
     */

    using gridrow = std::vector<qslotbutton *>;

    /**
     *  This vector holds all of the rows of buttons.  Its size with be the
     *  number of rows in the grid.
     */

    using buttons = std::vector<gridrow>;

private:

    Q_OBJECT

public:

    qslivegrid
    (
        performer & perf,               /* performance master   */
        qsmainwnd * window,             /* functional parent    */
        QWidget * parent = nullptr      /* Qt-parent            */
    );
    virtual ~qslivegrid ();

private:                            // overrides of qslivebase functions

    virtual void refresh ()
    {
        qslivebase::refresh();
        (void) refresh_all_slots();
    }

    virtual void refresh (seq::number seqno);

    virtual void color_by_number (int i) override;
    virtual void set_mode_text (const std::string & mode = "") override;
    virtual void set_playlist_name (const std::string & plname = "") override;
    virtual void set_bank_values
    (
        const std::string & bankname = "", int bankid = 0
    ) override;

    virtual void update_bank (int bank) override;
    virtual void update_bank () override;
    virtual void update_bank_name (const std::string & name) override;
    virtual void update_sequence (seq::number seqno, bool redo) override;
    virtual void reupdate () override;
    virtual void update_geometry () override;
    virtual void change_event (QEvent *) override;

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

private:

    seq::number seq_id_from_xy (int click_x, int click_y);
    qslotbutton * create_one_button (int seqno);
    qslotbutton * button (int row, int column);
    qslotbutton * find_button (int seqno);
    bool get_slot_coordinate (int x, int y, int & row, int & column);
    bool handle_key_press (const keystroke & k);
    bool handle_key_release (const keystroke & k);
    bool delete_slot (int row, int column);
    bool delete_all_slots ();
    bool recreate_all_slots ();
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
    void copy_sequence ();
    void cut_sequence ();
    void paste_sequence ();
    void merge_sequence ();
    void delete_sequence ();
    void new_live_frame ();

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


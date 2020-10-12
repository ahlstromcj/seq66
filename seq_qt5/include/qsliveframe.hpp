#if ! defined SEQ66_QSLIVEFRAME_HPP
#define SEQ66_QSLIVEFRAME_HPP

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
 * \file          qsliveframe.hpp
 *
 *  This module declares/defines the base class for the Qt 5 version of
 *  the pattern window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2020-07-29
 * \license       GNU GPLv2 or above
 *
 *  The qsliveframe is Sequencer66's analogue to the Gtkmm mainwid class.  These
 *  classes display a grid of patterns (loops) that can be controlled via the
 *  grid.
 *
 *  Also see the new qslivegrid class.
 */

#include <functional>                   /* std::function, function objects  */

#include "qslivebase.hpp"               /* seq66::qslivebase ABC            */
#include "midi/midibytes.hpp"           /* seq66::ctrlkey alias             */
#include "play/screenset.hpp"           /* seq66::screenset class           */

class QMenu;
class QTimer;
class QMessageBox;
class QFont;

/*
 * Do not document namespaces.
 */

namespace Ui
{
    class qsliveframe;
}

namespace seq66
{
    class keystroke;
    class performer;
    class qsmainwnd;

/**
 *  Provides the old version of the live-frame, where every "button" is drawn
 *  from scratch.  Some might like this better than the qslivegrid.
 */

class qsliveframe final : public qslivebase
{
    friend class qsmainwnd;
    friend class qliveframeex;

    Q_OBJECT

public:

    qsliveframe
    (
        performer & perf,               /* performance master   */
        qsmainwnd * window,             /* functional parent    */
        QWidget * parent = nullptr      /* Qt-parent            */
    );
    virtual ~qsliveframe ();

private:                            // overrides of qslivebase functions

    virtual void calculate_base_sizes
    (
        seq::number seq, int & basex, int & basey
    ) override;
    virtual void color_by_number (int i) override;
    virtual void set_playlist_name (const std::string & plname = "") override;
    virtual void set_bank_values
    (
        const std::string & bankname = "",
        int bankid = 0
    ) override;

    virtual void update_bank (int newbank) override;
    virtual void update_bank_name (const std::string & name) override;
    virtual void reupdate () override;
    virtual void update_geometry () override;
    virtual void change_event (QEvent *) override;
    virtual void draw_sequences () override;
    virtual bool draw_sequence (seq::pointer s, seq::number) override;
    virtual bool draw_slot (seq::number seqnum) override;
    virtual void draw_box (QPainter & p, int x, int y, int w, int h) override;

protected:

    virtual void paintEvent (QPaintEvent *) override;
    virtual void mousePressEvent (QMouseEvent *) override;
    virtual void mouseReleaseEvent (QMouseEvent *) override;
    virtual void mouseMoveEvent (QMouseEvent *) override;
    virtual void mouseDoubleClickEvent (QMouseEvent *) override;
    virtual void keyPressEvent (QKeyEvent *) override;
    virtual void keyReleaseEvent (QKeyEvent *) override;
    virtual void changeEvent (QEvent *) override;

private:

    seq::number seq_id_from_xy (int click_x, int click_y);
    bool handle_key_press (const keystroke & k);
    bool handle_key_release (const keystroke & k);
    void sequence_key (int seq);
    void sequence_key_check ();

signals:

    void signal_call_editor (int seqid);        /* editor tab for pattern      */
    void signal_call_editor_ex (int seqid);     /* editor window for pattern   */
    void signal_call_edit_events (int seqid);   /* event tab for pattern       */
    void signal_live_frame (int ssnum);         /* live frame for seq/screen # */

private slots:

    void conditional_update ();
    void new_seq ();
    void edit_seq ();
    void edit_seq_ex ();
    void edit_events ();
    void copy_sequence ();
    void cut_sequence ();
    void paste_sequence ();
    void delete_sequence ();
    void new_live_frame ();

private:

    Ui::qsliveframe * ui;
    QMenu * m_popup;
    QTimer * m_timer;
    QMessageBox * m_msg_box;

    /**
     *  A function object to draw the sequences in the playing set.
     */

    screenset::slothandler m_slot_function;

    /**
     *  Indicates how to draw the slots.
     */

    bool m_gtkstyle_border;

};              // class qsliveframe

}               // namespace seq66

#endif          // SEQ66_QSLIVEFRAME_HPP

/*
 * qsliveframe.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


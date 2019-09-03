#if ! defined SEQ66_QSLIVEBASE_HPP
#define SEQ66_QSLIVEBASE_HPP

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
 * \file          qslivebase.hpp
 *
 *  This module declares/defines the base class for the Qt 5 version of
 *  the pattern window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-06-22
 * \updates       2019-08-30
 * \license       GNU GPLv2 or above
 *
 *  The qslivebase and its child classes, qsliveframe and qslivegride, are
 *  Sequencer66's analogue to the Gtkmm mainwid class.  These classes display a
 *  grid of patterns (loops) that can be controlled via the grid.
 */

#include <QFrame>

#include <functional>                   /* std::function, function objects  */

#include "gui_palette_qt5.hpp"
#include "midi/midibytes.hpp"           /* seq66::ctrlkey alias             */
#include "play/screenset.hpp"           /* seq66::screenset class           */

class QEvent;

namespace seq66
{
    class keystroke;
    class performer;
    class qsmainwnd;

/**
 *  This base class provides access to the performer, the main window, some
 *  basic items needed for drawing text, handling banks (sets), and manipulating
 *  sequences/loops/patterns.
 */

class qslivebase : public QFrame, protected gui_palette_qt5
{
    friend class qsmainwnd;
    friend class qliveframeex;

public:

    qslivebase
    (
        performer & perf,               /* performance master   */
        qsmainwnd * window,             /* functional parent    */
        QWidget * parent = nullptr      /* Qt-parent            */
    );
    virtual ~qslivebase ();

    bool set_bank (int newBank, bool hasfocus = false);
    void set_bank ();               // bank number retrieved from performer

    int bank () const
    {
        return m_bank_id;           // same as the screen-set number
    }

    const performer & perf () const
    {
        return m_performer;
    }

    int rows () const
    {
        return m_mainwnd_rows;
    }

    int columns () const
    {
        return m_mainwnd_cols;
    }

    int spacing () const
    {
        return m_mainwid_spacing;
    }

    int current_seq () const
    {
        return m_current_seq;
    }

protected:

    performer & perf ()
    {
        return m_performer;
    }

protected:

    bool copy_seq ();
    bool cut_seq ();
    bool delete_seq ();
    bool paste_seq ();
    virtual void update_bank (int bank);
    virtual void update_bank_name ()
    {
        // no code
    }

    virtual void color_by_number (int i) = 0;   /* implemented!!!   */

    virtual bool recreate_all_slots ()
    {
        return false;
    }

private:            // pure virtual functions

    virtual void calculate_base_sizes (seq::number, int &, int &)
    {
        // no code
    }

    virtual void set_playlist_name (const std::string & plname = "") = 0;
    virtual void set_bank_values
    (
        const std::string & bankname = "",
        int bankid = 0
    ) = 0;

    virtual void reupdate () = 0;               /* update()         */
    virtual void update_geometry () = 0;        /* updateGeometry() */
    virtual void change_event (QEvent *) = 0;   /* changeEvent()    */

    virtual void draw_sequences ()
    {
        // no code
    }

    virtual bool draw_sequence (seq::pointer, seq::number)
    {
        return true;
    }

    virtual bool draw_slot (seq::number)
    {
        return true;
    }

    /*
     *  The integer parameters are x, y, w, and h.
     */

    virtual void draw_box (QPainter &, int, int, int, int)
    {
        // no code
    }

protected:

    /**
     *  Access to the most important class in Sequencer66.
     */

    performer & m_performer;

    /**
     *  Access to the main window.
     */

    qsmainwnd * m_parent;

    /**
     *  Provide the font used for drawing text in qsliveframe.  Note that text
     *  in the qslivegrid's qslotbuttons will use setText() for drawing the slot
     *  numbers, and qloopbutton has its own font for the buttons.
     */

    QFont m_font;

    /**
     *  Kepler34 calls "screensets" by the name "banks".  Same as the screen-set
     *  number.
     */

    int m_bank_id;

    /**
     *  These values are assigned to the values given by the constants of
     *  similar names in globals.h (obsolete), and we will make them parameters
     *  or user-interface configuration items later.  Some of them already have
     *  counterparts in the usrsettings class.
     */

    int m_mainwnd_rows;         /* from usr().mainwnd_rows()                */
    int m_mainwnd_cols;         /* from usr().mainwnd_cols()                */
    int m_mainwid_spacing;      /* from usr().mainwid_spacing(): 2 to 16    */
    int m_space_rows;           /* total space between all rows (e.g. 2*4)  */
    int m_space_cols;           /* ditto for columns (e.g. 2 * 8)           */

    /**
     *  Provides a convenience variable for avoiding multiplications.
     *  It is equal to m_mainwnd_rows * m_mainwnd_cols.
     */

    const int m_screenset_slots;

    /**
     *  Width of a pattern slot in pixels.  Corresponds to the mainwid's
     *  m_seqarea_x value.
     */

    int m_slot_w;

    /**
     *  Height of a pattern slot in pixels.  Corresponds to the mainwid's
     *  m_seqarea_y value.
     */

    int m_slot_h;

    /**
     *  Used in beat pulsing in the qsmaintime bar, which is a bit different
     *  than the legacy progress pill in maintime.
     */

    int m_last_metro;

    /**
     *  Holds the current transparency value, used in beat-pulsing for fading.
     */

    int m_alpha;

    /**
     *  For mouse interaction, holds the current sequence/loop/pattern number
     *  indicated by clicking in the live frame.
     */

    int m_current_seq;

    /**
     *  Indicates previously-selected sequence number when copying it.
     */

    bool m_button_down;

    /**
     *
     */

    bool m_moving;                      // are we moving between slots

    /**
     *
     */

    bool m_adding_new;                  // new seq here, wait for double click

    /**
     *  Indicates that there is something to paste.
     */

    bool m_can_paste;

    /**
     *
     */

    bool m_has_focus;

    /**
     *  Indicates this live frame is in an external window.  It does not have
     *  a tab widget as a parent, and certain menu entries cannot be used.
     */

    bool m_is_external;

};              // class qslivebase

}               // namespace seq66

#endif          // SEQ66_QSLIVEBASE_HPP

/*
 * qslivebase.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


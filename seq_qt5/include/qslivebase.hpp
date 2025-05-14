#if ! defined SEQ66_QSLIVEBASE_HPP
#define SEQ66_QSLIVEBASE_HPP

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
 * \file          qslivebase.hpp
 *
 *  This module declares/defines the base class for the Qt 5 version of
 *  the pattern window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-06-22
 * \updates       2025-05-14
 * \license       GNU GPLv2 or above
 *
 *  The qslivebase and its child class, qslivegride, are Sequencer66's
 *  analogue to the Gtkmm mainwid class.  These classes display a grid of
 *  patterns (loops) that can be controlled via the grid.
 */

#include <QFrame>

#include "play/performer.hpp"           /* seq66::performer class           */
#include "play/screenset.hpp"           /* seq66::screenset class           */

class QEvent;

/*
 * Do not document a namespace, it can break Doxygen.
 */

namespace seq66
{
    class keystroke;
    class performer;
    class qsmainwnd;

/**
 *  This base class provides access to the performer, the main window, some
 *  basic items needed for drawing text, handling banks (sets), and
 *  manipulating sequences/loops/patterns.
 */

class qslivebase : public QFrame
{
    friend class qsmainwnd;
    friend class qliveframeex;

public:

    qslivebase
    (
        performer & perf,                   /* performance master   */
        qsmainwnd * window,                 /* functional parent    */
        screenset::number bank = screenset::unassigned(),
        QWidget * parent = nullptr          /* the Qt parent        */
    );
    virtual ~qslivebase ();

    bool set_bank (int newBank, bool hasfocus = false);
    void set_bank ();               // bank number retrieved from performer

    screenset::number bank_id () const
    {
        return m_bank_id;           // same as the screen-set number
    }

    const std::string & bank_name () const
    {
        return m_bank_name;
    }

    const performer & perf () const
    {
        return m_performer;
    }

    int rows () const
    {
        return perf().rows();
    }

    int columns () const
    {
        return perf().columns();
    }

    seq::number seq_offset () const;

    bool seq_to_grid (seq::number seqno, int & row, int & column) const
    {
        return perf().seq_to_grid(seqno, row, column, is_external());
    }

    int spacing () const
    {
        return m_mainwnd_spacing;
    }

protected:

    performer & perf ()
    {
        return m_performer;
    }

    void set_needs_update (bool flag = true)
    {
        m_needs_update = flag;
    }

    bool check_needs_update () const
    {
        bool result = m_needs_update;
        m_needs_update = false;
        return result;
    }

    bool is_external () const
    {
        return m_is_external;
    }

protected:

    qsmainwnd * parent ()
    {
        return m_parent;
    }

    const qsmainwnd * parent () const
    {
        return m_parent;
    }

    bool can_paste () const
    {
        return m_can_paste;
    }

    void can_paste (bool flag)
    {
        m_can_paste = flag;
    }

    seq::number current_seq () const
    {
        return m_current_seq;
    }

    void current_seq (seq::number n)
    {
        m_current_seq = n;
    }

    virtual void update_bank (int bank);

    virtual void update_bank ()
    {
        // no code, override to recreate current bank
    }

    virtual void update_bank_name (const std::string & /*name*/)
    {
        // no code, see qslivegrid
    }

    virtual void update_sequence (seq::number /*seqno*/, bool /*redo*/)
    {
        // no code (yet), see qslivegrid
    }

    virtual void color_by_number (int i) = 0;   /* implemented!!!   */
    virtual void set_midi_bus (int b);
    virtual void set_midi_channel (int b);
    virtual void set_midi_in_bus (int b);

    virtual bool recreate_all_slots ()
    {
        return false;
    }

    virtual void refresh ()
    {
        set_needs_update();
    }

    /*
     * No support for refreshing only a specific slot in this base-class
     * version.
     */

    virtual void refresh (seq::number /*seqno*/)
    {
        set_needs_update();
    }

    virtual void set_bank_values
    (
        const std::string & name = "",
        int id = 0
    )
    {
        m_bank_name = name;
        m_bank_id = id;
    }

private:            // pure virtual functions

    virtual void calculate_base_sizes (seq::number, int &, int &)
    {
        // no code
    }

    virtual void set_playlist_name
    (
        const std::string & plname = "",
        bool modified = false
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
     *  Provide the font used for drawing text in qslivegrid.  Note that text
     *  in the qslivegrid's qslotbuttons will use setText() for drawing the
     *  slot numbers, and qloopbutton has its own font for the buttons.
     */

    QFont m_font;

    /**
     *  Kepler34 calls "screensets" by the name "banks".  Same as the
     *  screen-set number. This is either the constructor-specified bank, or
     *  is the same as the current bank/set logged in the performer.
     */

    int m_bank_id;

    /**
     *  A copy of the bank name, which is not necessarily the name of the
     *  playing screen-set.
     */

    std::string m_bank_name;

    /**
     *  These values are assigned to the values given by the constants of
     *  similar names in globals.h (obsolete), and we will make them
     *  parameters or user-interface configuration items later.  Some of them
     *  already have counterparts in the usrsettings class.
     */

    int m_mainwnd_spacing;      /* from usr().mainwnd_spacing(): 2 to 16    */
    int m_space_rows;           /* total space between all rows (e.g. 2*4)  */
    int m_space_cols;           /* ditto for columns (e.g. 2 * 8)           */

    /**
     *  Provides a convenience variable for avoiding multiplications.  It is
     *  equal to rows * columns.
     */

    const int m_screenset_slots;

    /**
     *  Width of a pattern slot in pixels.  Corresponds to the live frame's
     *  m_seqarea_x value.
     */

    int m_slot_w;

    /**
     *  Height of a pattern slot in pixels.  Corresponds to the live frame's
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

    seq::number m_current_seq;

    /**
     *  Holds the initial sequence number when attempting to move the sequence.
     */

    seq::number m_source_seq;

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

    /**
     *  Indicates a need for a button update, as opposed to a complete redraw
     *  of all the buttons.
     */

    mutable bool m_needs_update;

};              // class qslivebase

}               // namespace seq66

#endif          // SEQ66_QSLIVEBASE_HPP

/*
 * qslivebase.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


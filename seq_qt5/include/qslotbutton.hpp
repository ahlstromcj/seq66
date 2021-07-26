#if ! defined SEQ66_QSLOTBUTTON_HPP
#define SEQ66_QSLOTBUTTON_HPP

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
 * \file          qslotbutton.hpp
 *
 *  This module declares/defines the base class for drawing on a pattern-slot
 *  button.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-06-26
 * \updates       2021-07-26
 * \license       GNU GPLv2 or above
 *
 *  All this button can do is enable a new pattern to be created.  It is
 *  impossible to recreate live-frame features like drag-and-drop patterns,
 *  using Qt slots for toggle or press actions.  The qslivegrid class disables
 *  the use of slots.  Instead, it calculates the button number based on the
 *  mouse pointer and handles the button operation on behalf of the button.
 */

#include <QPushButton>
#include <string>

#include "gui_palette_qt5.hpp"          /* seq66::Color                     */
#include "play/seq.hpp"                 /* seq66::seq sequence-plus class   */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class performer;
    class qslivegrid;

/**
 * The timebar for the sequence editor
 */

class qslotbutton : public QPushButton
{

    friend class qslivegrid;

protected:

    /**
     *  Holds a pointer to the parent, needed to evaluated changes in
     *  user-interface size.
     */

    const qslivegrid * const m_slot_parent;

    /**
     *  Indicates the sequence number of the slot.  Needed when the slot is
     *  empty (has a null seq::pointer), which will always be true for a
     *  slot-button.
     */

    seq::number m_slot_number;

    /**
     *  Provides the initial labelling for this button.
     */

    std::string m_label;

    /**
     *  Provides the hot-key (slot-key) for this button, provided by the
     *  performer.
     */

    std::string m_hotkey;

    /**
     *  More colors, to save precious time, at the cost of luxurious space.
     */

    const Color m_drum_color;

#if defined DRAW_TEMPO_LINE
    const Color m_tempo_color;
#endif

    const Color m_progress_color;

    /*
     * Can be modified to match a Qt theme.
     * We have the button-text color, the color of lines in the progress box,
     * and the background color specified by the user.
     */

    Color m_label_color;
    Color m_text_color;
    Color m_pen_color;
    Color m_back_color;

    /**
     *  Indicates we are running with more than the usual number of rows, 4.
     */

    bool m_vert_compressed;

    /**
     *  Indicates if the button is checkable, or just clickable.  Empty slots
     *  need to be enabled, but not checkable, so that we can do different
     *  things with them.  We could probably just hook up a different callback
     *  to qslotbuttons versus qloopbuttons.
     */

    bool m_is_checkable;

    /**
     *  Used in repainting the button.  Usually more important for the derived
     *  class (qloopbutton).
     */

    mutable bool m_is_dirty;

public:

    qslotbutton
    (
        const qslivegrid * const slotparent,
        seq::number slotnumber,
        const std::string & label,
        const std::string & hotkey,
        QWidget * parent    = nullptr
    );

    virtual ~qslotbutton ()
    {
        // no code needed
    }

    virtual void setup ();

    virtual seq::pointer loop ()
    {
        static seq::pointer s_dummy;
        return s_dummy;
    }

    virtual void set_checked (bool /*flag*/)
    {
        // no code for this base class
    }

    virtual bool toggle_enabled ()
    {
        return false;                   /* no functionality in base class   */
    }

    virtual bool toggle_checked ()
    {
        return false;                   /* no functionality in base class   */
    }

    seq::number slot_number () const
    {
        return m_slot_number;
    }

    const std::string & label () const
    {
        return m_label;
    }

    std::string hotkey () const
    {
        return m_hotkey;
    }

    bool is_checkable () const
    {
        return m_is_checkable;
    }

protected:

    const qslivegrid * slot_parent () const
    {
        return m_slot_parent;
    }

    void make_checkable ()
    {
        m_is_checkable = true;;
    }

    bool is_dirty () const
    {
        return m_is_dirty;
    }

    void set_dirty (bool f)
    {
        m_is_dirty = f;
    }

    const Color & text_color () const
    {
        return m_text_color;
    }

    const Color & label_color () const
    {
        return m_label_color;
    }

    const Color & drum_color () const
    {
        return m_drum_color;
    }

#if defined DRAW_TEMPO_LINE
    const Color & tempo_color () const
    {
        return m_tempo_color;
    }
#endif

    const Color & progress_color () const
    {
        return m_progress_color;
    }

    const Color & pen_color () const
    {
        return m_pen_color;
    }

    const Color & back_color () const
    {
        return m_back_color;
    }

    virtual void reupdate (bool all = true);

    virtual void draw_progress (QPainter & /*p*/, midipulse /*tick*/)
    {
        // no code yet
    }

protected:

    void label_color (Color c)
    {
        m_label_color = c;
    }

    void text_color (Color c)
    {
        m_text_color = c;
    }

    void pen_color (Color c)
    {
        m_pen_color = c;
    }

    void back_color (Color c)
    {
        m_back_color = c;
    }

};          // class qslotbutton

}           // namespace seq66

#endif      // SEQ66_QSLOTBUTTON_HPP

/*
 * qslotbutton.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


#if ! defined SEQ66_MIDILEARN_HPP
#define SEQ66_MIDILEARN_HPP

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
 * \file          midilearn.hpp
 *
 *  This module declares/defines containers and management for a
 *  MIDI Learn function.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2026-06-09
 * \updates       2026-06-25
 * \license       GNU GPLv2 or above
 *
 */

#include <atomic>                       /* std::atomic<bool>                */

#include "ctrl/midicontrolin.hpp"       /* seq66::midicontrolin             */

namespace seq66
{

class performer;

/**
 *  Provides an object specifying what a keystroke, GUI action, or a MIDI
 *  control should do.
 */

class midilearn final
{

    friend class midicontrolfile;
    friend class performer;
    friend class qlearnframe;

private:

    /**
     * Access to the performer.
     */

    performer & m_perf;

    /**
     *  Holds the original or saved state of the controls. This is grabbed
     *  from the performer's copy of rc().midi_control_in().
     */

    midicontrolin m_original_controls;

    /**
     *  Holds the current state of the controls. It might at first be
     *  empty or it might contain the original controls.
     */

    midicontrolin m_current_controls;

    /**
     *  Mirrors the Learn selection in qlearnframe: loop, mute_group, or
     *  automation.
     */

    automation::category m_automation_category { automation::category::none };

    /**
     *  Mirrors the Action selection in qlearnframe: toggle, on, or off.
     */

    automation::action m_automation_action { automation::action::toggle };

    /**
     *  This is set when processing automation controls. For the loop
     *  and mute_group category, see m_current_index.
     */

    automation::slot m_automation_slot { automation::slot::none };

    /**
     *  Provides the maximum number of loop controls. Usually 32,
     *  this can be configured to be less or more. Might change in
     *  the constructor.
     */

    int m_loop_count_max { 32 };

    /**
     *  Provides the maximum number of mute-group controls, always
     *  32. See mutegroups::c_mute_groups_max's definition.
     */

    int m_mute_count_max { 32 };

    /**
     *  A count on non-zero controls for each set of control values.
     */

    mutable int m_loops_ctrl_count { 0 };
    mutable int m_mutes_ctrl_count { 0 };
    mutable int m_automation_ctrl_count { 0 };

    /**
     *  Indicates an event has been received (i.e. a button pressed),
     *  so that the next event, the release event, should be ignored.
     */

    std::atomic<bool> m_pressed { false };

    /**
     *  Indicates a change has been made.
     */

    bool m_is_dirty { false };

    /**
     *  Indicates the index of the current button in the loops or mutes grid.
     */

    int m_current_index { 0 };

    /**
     *  Values from the qlearnframe user-interface.
     */

    bool m_inverse;
    int m_d1min;
    int m_d1max;

public:

    midilearn () = delete;
    midilearn
    (
        performer & p,
        bool clearcontrols = false
    );
    midilearn (const midilearn &) = delete;
    midilearn & operator = (const midilearn &) = delete;
    midilearn (midilearn &&) = delete;
    midilearn & operator = (midilearn &&) = delete;
    ~midilearn () = default;

    performer & perf ()
    {
        return m_perf;
    }

    const performer & perf () const
    {
        return m_perf;
    }

    automation::category automation_category () const
    {
        return m_automation_category;
    }

    void automation_category (automation::category c)
    {
        m_automation_category = c;
        clear_current_index();
    }

    bool is_loop () const
    {
        return m_automation_category == automation::category::loop;
    }

    bool is_mute_group () const
    {
        return m_automation_category == automation::category::mute_group;
    }

    bool is_automation () const
    {
        return m_automation_category == automation::category::automation;
    }

    bool is_hold_active () const
    {
        return m_automation_action == automation::action::hold;
    }

    automation::action automation_action () const
    {
        return m_automation_action;
    }

    void automation_action (automation::action a)
    {
        m_automation_action = a;
    }

    automation::slot automation_slot () const
    {
        return m_automation_slot;
    }

    void automation_slot (automation::slot s)
    {
        m_automation_slot = s;
        m_current_index = slot_to_int_cast(s);
    }

    int loops_ctrl_count () const
    {
        return m_loops_ctrl_count;
    }

    int mutes_ctrl_count () const
    {
        return m_mutes_ctrl_count;
    }

    int automation_ctrl_count () const
    {
        return m_automation_ctrl_count;
    }

    bool automation_slot_active () const
    {
        return m_automation_slot != automation::slot::none;
    }

    void clear_automation_slot ()
    {
        m_automation_slot = automation::slot::none;
    }

    bool is_dirty () const
    {
        return m_is_dirty;
    }

    int current_index () const
    {
        return m_current_index;
    }

    void initialize_current_index ();

    void clear_current_index ()
    {
        m_current_index = 0;
    }

    bool clear_all ();

    bool clear (automation::category c)
    {
        return m_current_controls.clear(c);
    }

    bool reset ();
    bool start ();
    bool save ();
    bool learn_control
    (
        const event & ev,
        const std::string & keyname,
        bool isinverse,
        int d1min,
        int d1max,
        automation::action altaction = automation::action::none
    );

    bool active_counts () const
    {
        return  m_current_controls.active_counts
        (
            m_loops_ctrl_count, m_mutes_ctrl_count, m_automation_ctrl_count
        );
    }

    bool active_counts
    (
        int & loopcount,
        int & mutescount,
        int & autocount
    ) const;

    bool pressed () const
    {
        return m_pressed;
    }

    void pressed (bool on)
    {
        m_pressed = on;
    }

    bool inverse () const
    {
        return m_inverse;
    }

    int d1min () const
    {
        return m_d1min;
    }

    int d1max () const
    {
        return m_d1max;
    }

private:

    void set_dirty (bool f)
    {
        m_is_dirty = f;
    }

    void inverse (bool inv)
    {
        m_inverse = inv;
    }

    void d1min (int d1)
    {
        m_d1min = d1;
    }

    void d1max (int d1)
    {
        m_d1max = d1;
    }

};              // class midilearn

}               // namespace seq66

#endif          // SEQ66_MIDILEARN_HPP

/*
 * midilearn.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

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
 * \updates       2026-06-10
 * \license       GNU GPLv2 or above
 *
 */

#include <map>                          /* std::map<> and multimap<>        */
#include <string>                       /* std::string                      */

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
     *  Mirrors the Learn selection in qlearnframe.
     */

    automation::category m_current_opcat;

    /**
     *  A count on non-zero controls for each set of control values.
     */

    int m_loops_ctrl_count { 0 };
    int m_mutes_ctrl_count { 0 };
    int m_automation_ctrl_count { 0 };
    bool m_is_dirty { false };

    /**
     *  Indicates the index of the current button in the loops or mutes grid.
     */

    int m_index { 0 };

    /**
     *  Holds the current control statuses ...
     */

    automation::ctrlstatus m_control_status;

public:

    midilearn () = delete;
    midilearn
    (
        performer & p,
        bool clearcontrols = false
    );
    midilearn (const midilearn &) = default;
    midilearn & operator = (const midilearn &) = default;
    midilearn (midilearn &&) = default;
    midilearn & operator = (midilearn &&) = default;
    ~midilearn () = default;

    performer & perf ()
    {
        return m_perf;
    }

    const performer & perf () const
    {
        return m_perf;
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

    bool is_dirty () const
    {
        return m_is_dirty;
    }

    void clear ();
    void reset ();
    void start ();
    bool save ();

    bool learn_control (const event & ev);

private:

    void set_dirty (bool f)
    {
        m_is_dirty = f;
    }

};              // class midilearn

}               // namespace seq66

#endif          // SEQ66_MIDILEARN_HPP

/*
 * midilearn.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

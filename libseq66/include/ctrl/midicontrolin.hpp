#if ! defined SEQ66_MIDICONTROLIN_HPP
#define SEQ66_MIDICONTROLIN_HPP

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
 * \file          midicontrolin.hpp
 *
 *  This module declares/defines the class for holding MIDI operation data for
 *  the application.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-23
 * \updates       2024-01-01
 * \license       GNU GPLv2 or above
 *
 *  This container holds a map of midicontrol objects keyed by a key ordinal
 *  number that can range from 0 to 255.
 *
 *  It requires C++11 and above.
 */

#include <map>                          /* std::map<> and multimap<>        */
#include <string>                       /* std::string                      */

#include "cfg/comments.hpp"             /* seq66::comments class            */
#include "ctrl/midicontrol.hpp"         /* seq66::midicontrol event item    */
#include "ctrl/midicontrolbase.hpp"     /* seq66::midicontrolbase class     */

namespace seq66
{

class keycontainer;

/**
 *  Provides an object specifying what a keystroke, GUI action, or a MIDI
 *  control should do.
 */

class midicontrolin final : public midicontrolbase
{

    friend class midicontrolfile;
    friend class performer;

public:

    /**
     *  Provides the type definition for this container.  The key of the
     *  container is based on the control value itself.  It is used to find
     *  one or more instances of the MIDI control.  Once found, the operation
     *  associated with that control can be exercised.
     */

    using mccontainer = std::multimap<midicontrol::key, midicontrol>;

private:

    /**
     *  The container itself.
     */

    mccontainer m_container;

    /**
     *  Provides the text of a "[comments]" section of the MIDI control "ctrl"
     *  file.  It can, for example, note the device for which the controls
     *  apply.
     */

    comments m_comments_block;

    /**
     *  Indicates if inactive controls are allowed to be added to the
     *  container.  When generating a "ctrl" file, all controls need to be
     *  processed and appear in that file.
     */

    bool m_inactive_allowed;

    /**
     *  Holds the current control statuses for use by the performer.  It
     *  replaces Sequencer64's c_status_replace, c_status_snapshot,
     *  c_status_queue, and c_status_oneshot.  Functions are provided to query
     *  and modify these values.
     */

    automation::ctrlstatus m_control_status;

    /**
     *  If true, there is at least one non-zero (i.e. functional) MIDI control
     *  in the container.  If this value if false, even if the container is
     *  full of zeroed stanzas, the container is considered empty.
     */

    bool m_have_controls;

public:

    midicontrolin (const std::string & name);
    midicontrolin (const midicontrolin &) = default;
    midicontrolin & operator = (const midicontrolin &) = default;
    midicontrolin (midicontrolin &&) = default;
    midicontrolin & operator = (midicontrolin &&) = default;
    virtual ~midicontrolin () = default;
    virtual bool initialize (int buss, int rows, int columns) override;

    comments & comments_block ()
    {
        return m_comments_block;
    }

    const comments & comments_block () const
    {
        return m_comments_block;
    }

    void clear ()
    {
        m_container.clear();
    }

    int count () const
    {
        return int(m_container.size());
    }

    bool have_controls () const
    {
        return m_have_controls;
    }

    const mccontainer & container () const
    {
        return m_container;
    }

    bool add (const midicontrol & mc);
    void add_blank_controls (const keycontainer & kc);
    const midicontrol & control (const midicontrol::key & k) const;
    std::string status_string () const;

    bool inactive_allowed () const
    {
        return m_inactive_allowed;
    }

    void inactive_allowed (bool flag)
    {
        m_inactive_allowed = flag;
    }

    automation::ctrlstatus status () const
    {
        return m_control_status;
    }

    bool is_status () const
    {
        return m_control_status != automation::ctrlstatus::none;
    }

    bool is_set (automation::ctrlstatus status) const
    {
        return bit_test_and(status, m_control_status);
    }

    /*
     * Use test_and() or test_or()?  We are testing for a single bit, so use
     * the "and" test.
     */

    bool is_replace () const
    {
        return is_replace(m_control_status);
    }

    bool is_replace (automation::ctrlstatus status) const
    {
        return bit_test_and(status, automation::ctrlstatus::replace);
    }

    bool is_snapshot () const
    {
        return is_snapshot(m_control_status);
    }

    bool is_snapshot (automation::ctrlstatus status) const
    {
        return bit_test_and(status, automation::ctrlstatus::snapshot);
    }

    bool is_queue () const
    {
        return is_queue(m_control_status);
    }

    bool is_queue (automation::ctrlstatus status) const
    {
        return bit_test_and(status, automation::ctrlstatus::queue);
    }

    bool is_keep_queue () const
    {
        return is_keep_queue(m_control_status);
    }

    bool is_keep_queue (automation::ctrlstatus status) const
    {
        return bit_test_and(status, automation::ctrlstatus::keep_queue);
    }

    bool is_oneshot () const
    {
        return is_oneshot(m_control_status);
    }

    bool is_oneshot (automation::ctrlstatus status) const
    {
        return bit_test_and(status, automation::ctrlstatus::oneshot);
    }

    bool is_learn () const
    {
        return bit_test_and(m_control_status, automation::ctrlstatus::learn);
    }

    bool is_learn (automation::ctrlstatus status) const
    {
        return bit_test_and(status, automation::ctrlstatus::learn);
    }

    bool is_solo () const
    {
        return is_replace() && is_queue();
    }

    bool is_solo (automation::ctrlstatus status) const
    {
        return is_replace(status) && is_queue(status);
    }

    void add_status (automation::ctrlstatus status)
    {
        m_control_status |= status;
    }

    void remove_status (automation::ctrlstatus status)
    {
        m_control_status &= ~status;
    }

    void clear_status ()
    {
        m_control_status = automation::ctrlstatus::none;
    }

    void remove_queued_replace ()
    {
        m_control_status &=
            ~(automation::ctrlstatus::queue | automation::ctrlstatus::replace);
    }

public:

    void show () const;

};              // class midicontrolin

}               // namespace seq66

#endif          // SEQ66_MIDICONTROLIN_HPP

/*
 * midicontrolin.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


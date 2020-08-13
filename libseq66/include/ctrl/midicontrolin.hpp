#if ! defined SEQ66_MIDICONTROLIN_HPP
#define SEQ66_MIDICONTROLIN_HPP

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
 * \file          midicontrolin.hpp
 *
 *  This module declares/defines the class for holding MIDI operation data for
 *  the application.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-23
 * \updates       2020-08-13
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

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

    class keycontainer;

/**
 *  Provides an object specifying what a keystroke, GUI action, or a MIDI
 *  control should do.
 */

class midicontrolin final : public midicontrolbase
{

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
     *  A name to use for showing the contents of the container.
     */

    std::string m_container_name;

    /**
     *  Provides the text of a "[comments]" section of the MIDI control "rc"
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
     *  Indicates if the control values were loaded from an "rc" configuration
     *  file, as opposed to being empty.  (There are no default values at this
     *  time.)
     */

    bool m_loaded_from_rc;

    /**
     *  Holds the current control status for use by the performer.  It replaces
     *  Sequencer64's 0, c_status_replace, c_status_snapshot, c_status_queue, and
     *  c_status_oneshot.  Functions are provided to query and modify these
     *  values.
     *
     *      WHY HERE in the container?
     */

    automation::ctrlstatus m_control_status;

    /**
     *  If true, there is at least one non-zero (i.e. functional) MIDI control
     *  in the container.  If this value if false, even if the container is full
     *  of zeroed stanzas, the container is considered empty.
     */

    bool m_have_controls;

public:

    midicontrolin
    (
        int buss    = 0,                        /* NOT YET USED */
        int rows    = SEQ66_DEFAULT_SET_ROWS,
        int columns = SEQ66_DEFAULT_SET_COLUMNS
    );
    midicontrolin (const std::string & name);
    midicontrolin (const midicontrolin &) = default;
    midicontrolin & operator = (const midicontrolin &) = default;
    midicontrolin (midicontrolin &&) = default;
    midicontrolin & operator = (midicontrolin &&) = default;
    virtual ~midicontrolin () = default;

    const std::string & name () const
    {
        return m_container_name;
    }

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
    bool merge_key
    (
        automation::category opcat,
        const std::string & keyname,
        int opslot
    );
    const midicontrol & control (const midicontrol::key & k) const;

    bool inactive_allowed () const
    {
        return m_inactive_allowed;
    }

    void inactive_allowed (bool flag)
    {
        m_inactive_allowed = flag;
    }

    bool loaded_from_rc () const
    {
        return m_loaded_from_rc;
    }

    void loaded_from_rc (bool flag)
    {
        m_loaded_from_rc = flag;
    }

    bool is_status () const
    {
        return m_control_status != automation::ctrlstatus::none;
    }

    /*
     * Use test_and() or test_or()?  We are testing for a single bit, so use
     * the "and" test.
     */

    bool is_replace () const
    {
        return bit_test_and(m_control_status, automation::ctrlstatus::replace);
    }

    bool is_replace (automation::ctrlstatus status) const
    {
        return bit_test_and(status, automation::ctrlstatus::replace);
    }

    bool is_snapshot () const
    {
        return bit_test_and(m_control_status, automation::ctrlstatus::snapshot);
    }

    bool is_snapshot (automation::ctrlstatus status) const
    {
        return bit_test_and(status, automation::ctrlstatus::snapshot);
    }

    bool is_queue () const
    {
        return bit_test_and(m_control_status, automation::ctrlstatus::queue);
    }

    bool is_queue (automation::ctrlstatus status) const
    {
        return bit_test_and(status, automation::ctrlstatus::queue);
    }

    bool is_keep_queue () const
    {
        return is_queue();
    }

    bool is_oneshot () const
    {
        return bit_test_and(m_control_status, automation::ctrlstatus::oneshot);
    }

    bool is_oneshot (automation::ctrlstatus status) const
    {
        return bit_test_and(status, automation::ctrlstatus::oneshot);
    }

    void add_status (automation::ctrlstatus status)
    {
        m_control_status |= status;
    }

    void remove_status (automation::ctrlstatus status)
    {
        m_control_status &= ~status;
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


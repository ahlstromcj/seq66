#ifndef SEQ66_MIDICONTROLOUT_HPP
#define SEQ66_MIDICONTROLOUT_HPP

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
 * \file          midicontrolout.hpp
 *
 *  This module declares/defines the class for handling MIDI control
 *  <i>output</i> of the application.
 *
 * \library       seq66 application
 * \author        Igor Angst (major modifications by C. Ahlstrom)
 * \date          2018-03-28
 * \updates       2021-01-25
 * \license       GNU GPLv2 or above
 *
 * The class contained in this file encapsulates most of the
 * functionality to send feedback to an external control surface in
 * order to reflect the state of seq66. This includes updates on
 * the playing and queueing status of the sequences.
 *
 */

#include <vector>                       /* std::vector<>                    */

#include "ctrl/midicontrolbase.hpp"     /* seq66::midicontrolbase class     */
#include "midi/event.hpp"               /* seq66::event class               */
#include "midi/mastermidibus.hpp"       /* seq66::mastermidibus class       */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

class performer;

/**
 *  Provides some management support for MIDI control... on output.  Many
 *  thanks to igorangst!
 */

class midicontrolout final : public midicontrolbase
{

public:

    /**
     *  Provides the kind of MIDI control event that is sent out.
     *
     * \todo
     *      Additional sequence actions to consider: record on, record off.
     *
     * \var play
     *      Sequence is playing.
     *
     * \var mute
     *      Sequence is muted.
     *
     * \var queue
     *      Sequence is queued.
     *
     * \var delete
     *      Sequence is deleted from its slot.
     *
     * \var max
     *      Marker for the maximum value of actions.
     */

    enum class seqaction
    {
        arm,
        mute,
        queue,
        remove,
        max
    };

    /**
     *  Provides codes for various other actions.  This enumeration will
     *  replace the action enumeration.  All items with have an On control and
     *  an Off control.
     */

    enum class uiaction
    {
        play,           /* button   */
        stop,           /* button   */
        pause,          /* button   */
        queue,          /* button?  */
        oneshot,
        replace,
        snap1,
        snap2,
        learn,          /* button?  */
        max
    };

private:

    /**
     *  Manifest constants for midicontrolfile to use as array indices.
     *  These correspond to the MIDI Controls for UI (user-interface) actions;
     *  see the uiactions enumeration.
     */

    enum outindex
    {
        enabled,
        channel,
        status,
        data_1,
        data_2,
        max
    };

    /**
     *  Provides a type to hold a MIDI-control-out sequence event and its
     *  status.  There are four of these for each sequence slot, one for each
     *  of the seqactions of arm, mute, queue, and remove.
     */

    using actionpair = struct
    {
        bool apt_action_status;
        event apt_action_event;
    };

    /**
     *  Holds an array of actionpairs, one for each item in the actions
     *  enumeration.  These apply to pattern/sequence actions.
     */

    using actions = std::vector<actionpair>;

    /**
     *  Provides a type for a vector of action pairs, which can be essentially
     *  unlimited in size.  However, currently, the number needed is
     *  action::max, or 15.
     */

    using actionlist = std::vector<actions>;

    /**
     *  Provides a place to hold MIDI control events in response to a
     *  user-interface change, such as starting or stopping playback.
     *  Is also adapted to handling the toggling (on/off) of mute groups.
     */

    using actiontriplet = struct
    {
        bool att_action_status;
        event att_action_event_on;
        event att_action_event_off;
    };

    /**
     *  Holds an array of actiontriplets, one for each item in the uiaction
     *  enumeration.
     */

    using uiactions = std::vector<actiontriplet>;

    /**
     *  Provides a type for a vector of uiaction pairs, which can be
     *  essentially unlimited in size.  However, currently, the number needed
     *  is uiaction::max, or 9.
     */

    using uiactionlist = std::vector<uiactions>;

private:

    /**
     *  Provides the MIDI master bus, provided by the performer class.
     *  The midicontrolout class does not own this pointer, and assumes that
     *  it is correct.
     */

    mastermidibus * m_master_bus;

    /**
     *  Provides the events to be sent out for sequence status changes.  This
     *  is a vector of vectors, by default of size 32 patterns by 4
     *  seqactions.
     */

    actionlist m_seq_events;

    /**
     *  Provides the events to be sent out for non-sequence actions.  This
     *  item is a vector of uiaction::max = 9 actiontriplets.
     */

    uiactions m_ui_events;

    /**
     *  EXPERIMENTAL.
     *  Provides action events for toggling a mute-group.  Handles the default
     *  and unchanging value of 32 mutegroups.
     */

    uiactions m_mutes_events;

    /**
     *  Holds the screenset size, to use rather than calling the container.
     */

    int m_screenset_size;

public:

    midicontrolout
    (
        int buss    = 0,                        /* SET INDIRECTLY   */
        int rows    = SEQ66_DEFAULT_SET_ROWS,
        int columns = SEQ66_DEFAULT_SET_COLUMNS
    );
    midicontrolout (const midicontrolout &) = default;
    midicontrolout & operator = (const midicontrolout &) = default;
    virtual ~midicontrolout () = default;

    void initialize (int count, int buss = SEQ66_MIDI_CONTROL_OUT_BUSS);

    void set_master_bus (mastermidibus * mmbus)
    {
        m_master_bus = mmbus;
    }

    int screenset_size () const
    {
        return m_screenset_size;
    }

    void send_seq_event (int seq, seqaction what, bool flush = true);
    void clear_sequences (bool flush = true);
    event get_seq_event (int seq, seqaction what) const;
    void set_seq_event (int seq, seqaction what, int * ev);
    bool seq_event_is_active (int seq, seqaction what) const;
    bool event_is_active (uiaction what) const;
    std::string get_event_str (uiaction what, bool on) const;
    std::string get_event_str (int w, bool on) const;

#if defined SEQ66_USE_REFERENCE_PARAMETERS
    void set_seq_event (int seq, seqaction what, event & ev);
    void set_event (uiaction what, bool enabled, event & on, event & off);
#endif

    void set_event (uiaction what, bool enabled, int * onp, int * offp);
    void set_mutes_event
    (
        int group, bool enabled, int * onp, int * offp
    );
    bool mutes_event_is_active (int group) const;
    void send_mutes_event (int group, bool on);
    void send_event (uiaction what, bool on);
    void send_learning (bool learning)
    {
        send_event(uiaction::learn, learning);
    }

};          // class midicontrolout

/*
 *  Free functions related to midicontrolout.
 */

extern std::string seqaction_to_string (midicontrolout::seqaction a);
extern std::string action_to_string (midicontrolout::uiaction a);
extern std::string action_to_type_string (midicontrolout::uiaction a);

}           // namespace seq66

#endif      // SEQ66_MIDICONTROLOUT_HPP

/*
 * midicontrolout.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


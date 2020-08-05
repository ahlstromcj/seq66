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
 * \author        Igor Angst (with modifications by C. Ahlstrom)
 * \date          2018-03-28
 * \updates       2019-11-25
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
 *  Provides some management support for MIDI control... on output.  Many thanks
 *  to igorangst!
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
     * \var action_play
     *      Sequence is playing.
     *
     * \var action_mute
     *      Sequence is muted.
     *
     * \var action_queue
     *      Sequence is queued.
     *
     * \var action_delete
     *      Sequence is deleted from its slot.
     *
     * \var action_max
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
     *  Provides codes for various other actions.
     */

    enum class action
    {
        play,
        stop,
        pause,
        queue_on,
        queue_off,
        oneshot_on,
        oneshot_off,
        replace_on,
        replace_off,
        snap1_store,
        snap1_restore,
        snap2_store,
        snap2_restore,
        learn_on,
        learn_off,
        max
    };

    /**
     *  Manifest constants for rcfile to use as array indices.
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
     *  Provides a type to hold a MIDI-control-out event and its status.
     *
     *      action_pair_t;
     */

    using actionpair = struct
    {
        event apt_action_event;
        bool apt_action_status;

    };

    /**
     *
     */

    using actions = std::vector<actionpair>;

    /**
     *  Provides a type for a vector of action pairs, which can be essentially
     *  unlimited in size.
     */

    using actionlist = std::vector<actions>;

private:

    /**
     *  Provides the MIDI output master bus.
     */

    mastermidibus * m_master_bus;

    /**
     *  Provides the events to be sent out for sequence status changes.
     */

    actionlist m_seq_events;

    /**
     *  Provides the events to be sent out for non-sequence actions.
     */

    actions m_events;

    /**
     *  Holds the screenset size, to use rather than calling the container.
     */

    int m_screenset_size;

    /**
     *  Current screen set offset. Since the sequences dispatch the output
     *  messages, and sequences don't know about screen-sets, we need to do the
     *  math in this class in order to send screen-set relative events out to
     *  external controllers. For now, the size of the screen-set is hard-wired
     *  to 32.
     *
     *  TODO: Make this behavior configurable via rcfile
     */

    int m_screenset_offset;

public:

    midicontrolout ();
    midicontrolout (const midicontrolout &) = default;
    midicontrolout & operator = (const midicontrolout &) = default;
    ~midicontrolout () = default;

    void initialize (int count, int buss = SEQ66_MIDI_CONTROL_OUT_BUSS);

    void set_master_bus (mastermidibus * mmbus)
    {
        m_master_bus = mmbus;
    }

    int screenset_size () const
    {
        return m_screenset_size;
    }

    /**
     *  Set the current screen-set offset
     *
     * \param offset
     *      New screen-set offset
     */

    void set_screenset_offset (int offset)
    {
        m_screenset_offset = offset;
    }

    void send_seq_event (int seq, seqaction what, bool flush = true);
    void clear_sequences (bool flush = true);
    event get_seq_event (int seq, seqaction what) const;
    void set_seq_event (int seq, seqaction what, event & ev);
    void set_seq_event (int seq, seqaction what, int * ev);
    bool seq_event_is_active (int seq, seqaction what) const;
    void send_event (action what);

    void send_learning (bool learning)
    {
        send_event(learning ? action::learn_on : action::learn_off);
    }

    event get_event (action what) const;
    std::string get_event_str (action what) const;
    void set_event (action what, event & ev);
    void set_event (action what, int * ev);
    bool event_is_active (action what) const;

};          // class midicontrolout

/*
 *  Free functions related to midicontrolout.
 */

extern std::string seqaction_to_string (midicontrolout::seqaction a);
extern std::string action_to_string (midicontrolout::action a);

}           // namespace seq66

#endif      // SEQ66_MIDICONTROLOUT_HPP

/*
 * midicontrolout.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


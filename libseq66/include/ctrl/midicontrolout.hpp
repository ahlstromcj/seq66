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

class midicontrolout : public midicontrolbase       // final ?
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
        remove,     // delete,
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

    /**
     * Send out notification about playing status of a sequence.
     *
     * \param seq
     *      The index of the sequence.
     *
     * \param what
     *      The status action of the sequence.
     *
     * \param flush
     *      Flush MIDI buffer after sending (default true).
     */

    void send_seq_event (int seq, seqaction what, bool flush = true);

    /**
     *  Clears all visible sequences by sending "delete" messages for all
     *  sequences ranging from 0 to screenset_size().
     */

    void clear_sequences ();

    /**
     * Getter for sequence action events.
     *
     * \param seq
     *      The index of the sequence.
     *
     * \param what
     *      The action to be notified.
     *
     * \return
     *      The MIDI event to be sent.
     */

    event get_seq_event (int seq, seqaction what) const;

    /**
     * Register a MIDI event for a given sequence action.
     *
     * \param seq
     *      The index of the sequence.
     *
     * \param what
     *      The action to be notified.
     *
     * \param ev
     *      The MIDI event to be sent.
     */

    void set_seq_event (int seq, seqaction what, event & ev);

    /**
     * Register a MIDI event for a given sequence action.
     *
     * \param seq
     *      The index of the sequence.
     *
     * \param what
     *      The action to be notified.
     *
     * \param ev
     *      Raw int array representing The MIDI event to be sent.
     */

    void set_seq_event (int seq, seqaction what, int * ev);

    /**
     * Checks if a sequence status event is active.
     *
     * \param seq
     *      The index of the sequence.
     *
     * \param what
     *      The action to be notified.
     *
     * \return
     *      Returns true if the respective event is active.
     */

    bool seq_event_is_active (int seq, seqaction what) const;

    /**
     * Send out notification about non-sequence actions.
     *
     * \param what
     *      The action to be notified.
     */

    void send_event (action what);

    /**
     * Getter for non-sequence action events.
     *
     * \param what
     *      The action to be notified.
     *
     * \return
     *      The MIDI event to be sent.
     */

    event get_event (action what) const;

    /**
     * Getter for non-sequence action events.
     *
     * \param what
     *      The action to be notified.
     *
     * \return
     *      The MIDI event in a config-compatible string
     */

    std::string get_event_str (action what) const;

    /**
     * Register a MIDI event for a given non-sequence action.
     *
     * \param what
     *      The action to be notified.
     *
     * \param event
     *      The MIDI event to be sent.
     */

    void set_event (action what, event & ev);

    /**
     * Register a MIDI event for a given non-sequence action.
     *
     * \param what
     *      The action to be notified.
     *
     * \param ev
     *      Raw int data representing the MIDI event to be sent.
     */

    void set_event (action what, int * ev);

    /**
     * Checks if an event is active.
     *
     * \param what
     *      The action to be notified.
     *
     * \return
     *      Returns true if the respective event is active.
     */

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


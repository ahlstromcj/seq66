#ifndef SEQ66_MIDICONTROLOUT_HPP
#define SEQ66_MIDICONTROLOUT_HPP

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
 * \file          midicontrolout.hpp
 *
 *  This module declares/defines the class for handling MIDI control
 *  <i>output</i> of the application.
 *
 * \library       seq66 application
 * \author        Igor Angst (major modifications by C. Ahlstrom)
 * \date          2018-03-28
 * \updates       2021-06-14
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
    friend class midicontrolfile;
    friend class performer;

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
     *  an Off control, as well an an Inactive (dark) mode.  This list
     *  currently numbers 24 entries.  We could double that by mapping all of
     *  the 48 values of the automation::slot enumeration.  However, some
     *  (such as the Thru and Record actions) apply to specific sequences as
     *  opposed to a single function, and would require two consecutive
     *  controls to be clicked.  Maybe later.  Some are either reserved at
     *  this time, are actions we don't yet care much about, or are less
     *  common actions that can be done by leaning over to the computer
     *  keyboard.
     */

    enum class uiaction
    {
        panic,                              /* button 0 */
        stop,                               /* button 1 */
        pause,                              /* button 2 */
        play,                               /* button 3 */
        toggle_mutes,                       /* button 4 */
        song_record,                        /* button 5 */
        slot_shift,                         /* button 6 */
        free,                               /* button 7 */
        queue,                              /* button A */
        oneshot,                            /* button B */
        replace,                            /* button C */
        snap,                               /* button D */
        song,                               /* button E */
        learn,                              /* button F */
        bpm_up,                             /* button G */
        bpm_dn,                             /* button H */
        list_up,                            /* extra 0  */
        list_dn,                            /* extra 1  */
        song_up,                            /* extra 2  */
        song_dn,                            /* extra 3  */
        set_up,                             /* extra 4  */
        set_dn,                             /* extra 5  */
        tap_bpm,                            /* extra 6  */
        quit,                               /* extra 7  */
        alt_1, max_old = alt_1,
        alt_2,
        alt_3,
        alt_4,
        alt_5,
        alt_6,
        alt_7,
        alt_8,
        max
    };

private:

    /**
     *  Manifest constants for midicontrolfile to use as array indices.  These
     *  correspond to the MIDI Controls for UI (user-interface) actions; see
     *  the uiactions enumeration. This enumeration cannot be a class
     *  enumeration, because enum classes cannot be used as array indices.
     *
     *  ca 2021-02-10.
     *  We dropped the enabled and channel values.  We can test for an output
     *  control to be enabled by checking for status > 0x00.  And we can make
     *  the channel part of the status.  We will read the old style in the
     *  midicontrolfile class and convert it to the new style.  We change the
     *  name of the enumeration for brevity and to uncover all usages via
     *  compiler errors. :-D
     *
     *  Obsolete: enabled, channel
     */

    enum index
    {
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
        event att_action_event_del;             /* inactive, show as dark   */
    };

    /**
     *  Matches which event in the actiontriplet to use.  (Otherwise, a
     *  boolean can be used to access only the "on" and "off" fields.
     */

    enum actionindex
    {
        action_on,      /**< The mute-group is active and selected.         */
        action_off,     /**< The mute-group is active, but not selected.    */
        action_del      /**< Mute-group or automation inactive.             */
    };

    /**
     *  Holds an array of actiontriplets, one for each item in the uiaction
     *  enumeration.
     */

    using uiactions = std::vector<actiontriplet>;

    /**
     *  Provides a type for a vector of uiaction pairs, which can be
     *  essentially unlimited in size.  However, the number needed is
     *  constrained to uiaction::max.
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
     *  item is a vector of uiaction::max actiontriplets.
     */

    uiactions m_ui_events;

    /**
     *  Provides action events for toggling a mute-group.  Handles the default
     *  and unchanging value of 32 mutegroups.
     */

    uiactions m_mutes_events;

    /**
     *  Holds the screenset size, to use rather than calling the container.
     */

    int m_screenset_size;

public:

    midicontrolout (const std::string & name);
    midicontrolout (const midicontrolout &) = default;
    midicontrolout & operator = (const midicontrolout &) = default;
    virtual ~midicontrolout () = default;
    virtual bool initialize (int buss, int rows, int columns) override;

    static void seqaction_range (int & minimum, int & maximum)
    {
        minimum = static_cast<int>(midicontrolout::seqaction::arm);
        maximum = static_cast<int>(midicontrolout::seqaction::max);
    }

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
    void clear_mutes (bool flush = true);
    event get_seq_event (int seq, seqaction what) const;
    void set_seq_event (int seq, seqaction what, int * ev);
    bool seq_event_is_active (int seq, seqaction what) const;
    bool event_is_active (uiaction what) const;
    std::string get_event_str (const event & ev) const;
    std::string get_ctrl_event_str (uiaction what, actionindex which) const;
    std::string get_mutes_event_str (int group, actionindex which) const;
    void set_event
    (
        uiaction what, bool enabled,
        int * onp, int * offp, int * readyp
    );
    void set_mutes_event
    (
        int group, int * onp, int * offp, int * delp = nullptr
    );
    bool mutes_event_is_active (int group) const;
    void send_mutes_event (int group, actionindex which);
    void send_event (uiaction what, actionindex which);
    void send_learning (bool learning);
    void send_automation (bool activate);

};          // class midicontrolout

/*
 *  Free functions related to midicontrolout.
 */

extern midicontrolout::uiaction & operator ++ (midicontrolout::uiaction & e);
extern std::string seqaction_to_string (midicontrolout::seqaction a);
extern std::string action_to_string (midicontrolout::uiaction a);

#if defined USE_ACTION_TO_TYPE_STRING
extern std::string action_to_type_string (midicontrolout::uiaction a);
#endif

}           // namespace seq66

#endif      // SEQ66_MIDICONTROLOUT_HPP

/*
 * midicontrolout.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


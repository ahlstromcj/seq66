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
 * \updates       2019-06-19
 * \license       GNU GPLv2 or above
 *
 * The class contained in this file encapsulates most of the functionality to
 * send feedback to an external control surface in order to reflect the state of
 * seq66. This includes updates on the playing and queueing status of the
 * sequences.
 */

#include <sstream>                      /* std::ostringstream class         */

#include "ctrl/midicontrolout.hpp"      /* seq66::midicontrolout class    */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

midicontrolout::midicontrolout ()
 :
    midicontrolbase     (SEQ66_MIDI_CONTROL_OUT_BUSS),
    m_master_bus        (nullptr),
    m_seq_events        (),
    m_events            (),
    m_screenset_size    (0),
    m_screenset_offset  (0)
{
    initialize(SEQ66_DEFAULT_SET_SIZE);
}

/**
 *  Reinitializes an empty set of MIDI-control-out values.  It first clears any
 *  existing values from the vectors.
 *
 *  Next, it loads an action-pair with "empty" values.  It the creates an array
 *  of these pairs.
 *
 *  Finally, it pushes the desired number of action-pair arrays into an
 *  actionlist, which is, for example, a vector of 32 elements, each containing
 *  4 pairs of event + status.  A vector of vector of pairs.
 *
 * \param count
 *      The number of controls to allocate.  Normally, this is 32, but larger
 *      values can now be handled.
 *
 * \param bus
 *      The buss number, which can range from 0 to 31, and defaults to
 *      SEQ66_MIDI_CONTROL_OUT_BUSS (15).
 */

void
midicontrolout::initialize (int count, int bus)
{
    event dummy_event;
    actions actionstemp;
    actionpair apt;
    dummy_event.set_channel_status(0, 0);       /* set status and channel   */
    apt.apt_action_event = dummy_event;
    apt.apt_action_status = false;
    m_seq_events.clear();
    m_events.clear();
    is_blank(true);
    is_enabled(false);
    if (count > 0)
    {
        is_enabled(true);
        if (bus >= 0 && bus < c_busscount_max)  /* also note c_bussbyte_max */
            buss(bussbyte(bus));

        m_screenset_size = count;
        for (int a = 0; a < static_cast<int>(seqaction::max); ++a)
            actionstemp.push_back(apt);     /* a blank action-pair vector   */

        for (int c = 0; c < count; ++c)
            m_seq_events.push_back(actionstemp);

        for (int a = 0; a < static_cast<int>(action::max); ++a)
            m_events.push_back(apt);
    }
    else
        m_screenset_size = 0;
}

/**
 *  A "to_string" function for the seqaction enumeration.
 */

std::string
seqaction_to_string (midicontrolout::seqaction a)
{
    switch (a)
    {
    case midicontrolout::seqaction::arm:
        return "arm";

    case midicontrolout::seqaction::mute:
        return "mute";

    case midicontrolout::seqaction::queue:
        return "queue";

    case midicontrolout::seqaction::remove:
        return "delete";

    default:
        return "unknown";
    }
}

/**
 *  A "to_string" function for the action enumeration.
 */

std::string
action_to_string (midicontrolout::action a)
{
    switch (a)
    {
    case midicontrolout::action::play:
        return "play";

    case midicontrolout::action::stop:
        return "stop";

    case midicontrolout::action::pause:
        return "pause";

    case midicontrolout::action::queue_on:
        return "queue on";

    case midicontrolout::action::queue_off:
        return "queue off";

    case midicontrolout::action::oneshot_on:
        return "oneshot on";

    case midicontrolout::action::oneshot_off:
        return "oneshot off";

    case midicontrolout::action::replace_on:
        return "replace on";

    case midicontrolout::action::replace_off:
        return "replace off";

    case midicontrolout::action::snap1_store:
        return "snap1 store";

    case midicontrolout::action::snap1_restore:
        return "snap1 restore";

    case midicontrolout::action::snap2_store:
        return "snap2 store";

    case midicontrolout::action::snap2_restore:
        return "snap2 restore";

    case midicontrolout::action::learn_on:
        return "learn on";

    case midicontrolout::action::learn_off:
        return "learn off";

    default:
        return "unknown";
    }
}

/**
 *  Send out notification about playing status of a sequence.
 *
 * \todo
 *      Need to handle screen sets. Since sequences themselves are ignorant about
 *      the current screen set, maybe we can centralise this knowledge inside
 *      this class, so before sending a sequence event, we check here if the
 *      sequence is in the active screen set, otherwise we drop the event. This
 *      requires that in the performer class, we do a "repaint" each time the
 *      screen set is changed.  For now, the size of the screenset is fixed to 32
 *      in this function.
 *
 * Also, maybe consider adding an option to the config file, making this behavior
 * optional: So either absolute sequence actions (let the receiver do the
 * math...), or sending events relative (modulo) the current screen set.
 *
 * \param seq
 *      The index of the sequence.
 *
 * \param what
 *      The status action of the sequence.  This indicates if the sequence is
 *      playing, muted, queued, or deleted (removed, empty).
 *
 * \param flush
 *      Flush MIDI buffer after sending (default true).
 */

void
midicontrolout::send_seq_event (int seq, seqaction what, bool flush)
{
    if (is_enabled())
    {
        /*
         *  This is not needed when called from screenset::slots_function(),
         *  which starts the sequence number at the set-offset of the set.
         */

        seq -= m_screenset_offset;      // adjust relative to current screen-set
        int w = static_cast<int>(what);
        if (m_seq_events[seq][w].apt_action_status)
        {
            event ev = m_seq_events[seq][w].apt_action_event;
            if (not_nullptr(m_master_bus))
            {
#ifdef SEQ66_PLATFORM_DEBUG_TMI
                std::string act = seqaction_to_string(w);
                std::string evstring = to_string(ev);
                printf
                (
                    "send_seq_event(%s): %s\n", act.c_str(), evstring.c_str()
                );
#endif
                m_master_bus->play(buss(), &ev, ev.channel());
                if (flush)
                    m_master_bus->flush();
            }
        }
    }
}

/**
 *  Clears all visible sequences by sending "delete" messages for all
 *  sequences ranging from 0 to 31.
 */

void
midicontrolout::clear_sequences (bool flush)
{
    if (is_enabled())
    {
        for (int seq = 0; seq < screenset_size(); ++seq)
            send_seq_event(seq, midicontrolout::seqaction::remove, false);

        if (flush && not_nullptr(m_master_bus))
            m_master_bus->flush();
    }
}

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

event
midicontrolout::get_seq_event (int seq, seqaction what) const
{
    static event s_dummy_event;
    int w = static_cast<int>(what);
    return seq >= 0 && seq < screenset_size() ?
        m_seq_events[seq][w].apt_action_event : s_dummy_event;
}

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

void
midicontrolout::set_seq_event (int seq, seqaction what, event & ev)
{
    if (is_enabled())
    {
        int w = static_cast<int>(what);
        m_seq_events[seq][w].apt_action_event = ev;
        m_seq_events[seq][w].apt_action_status = true;
        is_blank(false);     // ???
    }
}

/**
 *  Register a MIDI event for a given sequence action.
 *
 * \tricky
 *      We have to call the overloaded two-parameter version of set_status()
 *      in lieu of calling set_status() and set_channel(), because the
 *      single-parameter set_status() assumes the channel nybble is present.
 *      This is too tricky.
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

void
midicontrolout::set_seq_event (int seq, seqaction what, int * eva)
{
    if (is_enabled() && what < seqaction::max)
    {
        int w = static_cast<int>(what);
        event ev;
        ev.set_channel_status(eva[outindex::status], eva[outindex::channel]);
        ev.set_data(eva[outindex::data_1], eva[outindex::data_2]);
        m_seq_events[seq][w].apt_action_event = ev;
        m_seq_events[seq][w].apt_action_status = bool(eva[outindex::enabled]);
        is_blank(false);
    }
}

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

bool
midicontrolout::seq_event_is_active (int seq, seqaction what) const
{
    int w = static_cast<int>(what);
    return (seq >= 0 && seq < screenset_size()) ?
        m_seq_events[seq][w].apt_action_status : false ;
}

/**
 * Send out notification about non-sequence actions.
 *
 * \param what
 *      The action to be notified.
 */

void
midicontrolout::send_event (action what)
{
    if (is_enabled() && event_is_active(what))
    {
        int w = static_cast<int>(what);
        event ev = m_events[w].apt_action_event;
        if (not_nullptr(m_master_bus))
        {
#ifdef SEQ66_PLATFORM_DEBUG_TMI
                std::string act = action_to_string(w);
                std::string evstring = to_string(ev);
                printf
                (
                    "send_event(%s): %s\n", act.c_str(), evstring.c_str()
                );
#endif
            m_master_bus->play(buss(), &ev, ev.channel());
            m_master_bus->flush();
        }
    }
}

event
midicontrolout::get_event (action what) const
{
    static event s_dummy_event;
    int w = static_cast<int>(what);
    return event_is_active(what) ?
        m_events[w].apt_action_event : s_dummy_event ;
}

/**
 * Getter for non-sequence action events.
 *
 * \param what
 *      The action to be notified.
 *
 * \return
 *      The MIDI event to be sent.
 */

std::string
midicontrolout::get_event_str (action what) const
{
    if (what < action::max)              /* not event_is_active(what)!!  */
    {
        int w = static_cast<int>(what);
        event ev(m_events[w].apt_action_event);
        midibyte d0, d1;
        ev.get_data(d0, d1);
        std::ostringstream str;
        str
            << "[" << int(ev.channel()) << " "
            << int(ev.get_status()) << " " << int(d0) << " " << int(d1) << "]"
            ;
        return str.str();
    }
    else
        return std::string("[0 0 0 0]");
}

/**
 * Register a MIDI event for a given non-sequence action.
 *
 * \param what
 *      The action to be notified.
 *
 * \param event
 *      The MIDI event to be sent.
 */

void
midicontrolout::set_event (action what, event & ev)
{
    if (is_enabled() && what < action::max)
    {
        int w = static_cast<int>(what);
        m_events[w].apt_action_event = ev;
        m_events[w].apt_action_status = true;
        is_blank(false);     // ???
    }
}

/**
 *  Register a MIDI event for a given non-sequence action.  An overload taking
 *  an array of 5 integers.
 *
 * \param what
 *      Provides the action code to be set.
 *
 * \param eva
 *      A pointer to an array of 5 integers.  These are used to see if the event
 *      is enabled, and to provide parameters for reconstructing the event.
 */

void
midicontrolout::set_event (action what, int * eva)
{
    if (is_enabled() && what < action::max)
    {
        int w = static_cast<int>(what);
        event ev;
        ev.set_channel_status(eva[outindex::status], eva[outindex::channel]);
        ev.set_data(eva[outindex::data_1], eva[outindex::data_2]);
        m_events[w].apt_action_event = ev;
        m_events[w].apt_action_status = bool(eva[outindex::enabled]);
    }
}

/**
 * Checks if an event is active.
 *
 * \param what
 *      The action to be notified.
 *
 * \return
 *      Returns true if the respective event is active.
 */

bool
midicontrolout::event_is_active (action what) const
{
    int w = static_cast<int>(what);
    return what < action::max ?  m_events[w].apt_action_status : false;
}

}           // namespace seq66

/*
 * midicontrolout.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


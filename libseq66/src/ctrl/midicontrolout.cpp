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
 * \file          midicontrolout.cpp
 *
 *  This module declares/defines the class for handling MIDI control
 *  <i>output</i> of the application.
 *
 * \library       seq66 application
 * \author        Igor Angst (with refactoring by C. Ahlstrom)
 * \date          2018-03-28
 * \updates       2022-03-03
 * \license       GNU GPLv2 or above
 *
 * The class contained in this file encapsulates most of the functionality to
 * send feedback to an external control surface in order to reflect the state
 * of seq66. This includes updates on the playing and queueing status of the
 * sequences.
 */

#include <iomanip>                      /* std::setw() manipulator          */
#include <sstream>                      /* std::ostringstream class         */

#include "ctrl/midicontrolout.hpp"      /* seq66::midicontrolout class      */
#include "play/mutegroups.hpp"          /* seq66::mutegroups::Size()        */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  This constructor assigns the basic values of control name, number, and
 *  action code.  The rest of the members can be set via the set() function.
 */

midicontrolout::midicontrolout (const std::string & name) :
    midicontrolbase     (name),
    m_master_bus        (nullptr),
    m_seq_events        (),
    m_ui_events         (),
    m_mutes_events      (),
    m_macro_events      (),
    m_screenset_size    (0)
{
   // no code
}

/**
 *  Reinitializes an empty set of MIDI-control-out values.  It first clears
 *  any existing values from the vectors.
 *
 *  Next, it loads an action-pair with "empty" values.  It the creates an
 *  array of these pairs.
 *
 *  Finally, it pushes the desired number of action-pair arrays into an
 *  actionlist, which is, for example, a vector of 32 elements, each
 *  containing 4 pairs of event + status.  A vector of vector of pairs.
 *
 * \param buss
 *      The buss number, which can range from 0 to 31, and defaults to
 *      default_control_out_buss (15).
 *
 * \param rows
 *      The number of rows in the set, normally 4.
 *
 * \param columns
 *      The number of columns in the set, normally 8.
 */

bool
midicontrolout::initialize (int buss, int rows, int columns)
{
    bool result = midicontrolbase::initialize(buss, rows, columns);
    m_seq_events.clear();
    m_ui_events.clear();
    m_mutes_events.clear();
    if (result)
    {
        int count = rows * columns;
        event dummy_event;
        actions actionstemp;
        dummy_event.set_channel_status(0, 0);       /* set status & channel */
        actionpair apt;
        apt.apt_action_status = false;
        apt.apt_action_event = dummy_event;
        for (int a = 0; a < static_cast<int>(seqaction::max); ++a)
            actionstemp.push_back(apt);         /* blank action-pair vector */

        for (int c = 0; c < count; ++c)
            m_seq_events.push_back(actionstemp);

        actiontriplet att;
        att.att_action_status = false;
        att.att_action_event_on = dummy_event;
        att.att_action_event_off = dummy_event;
        att.att_action_event_del = dummy_event;
        for (int a = 0; a < static_cast<int>(uiaction::max); ++a)
            m_ui_events.push_back(att);

        for (int m = 0; m < mutegroups::Size(); ++m)
            m_mutes_events.push_back(att);

        m_screenset_size = count;
        is_enabled(true);                           /* master bus???        */
    }
    else
    {
        m_screenset_size = 0;
        is_enabled(false);
    }
    return result;
}

/**
 *  This implementation of the prefix "++" operator is needed to use
 *  midicontrolout::uiaction in a for-loop.
 */

midicontrolout::uiaction &
operator ++ (midicontrolout::uiaction & e)
{
    if (e == midicontrolout::uiaction::max)
    {
        throw std::out_of_range("operator++(midicontrolout::uiaction)");
    }
    e = midicontrolout::uiaction
    (
        static_cast<std::underlying_type<midicontrolout::uiaction>::type>(e) + 1
    );
    return e;
}


/**
 *  A "to_string" function for the seqaction enumeration.
 */

std::string
seqaction_to_string (midicontrolout::seqaction a)
{
    switch (a)
    {
    case midicontrolout::seqaction::arm:    return "arm";
    case midicontrolout::seqaction::mute:   return "mute";
    case midicontrolout::seqaction::queue:  return "queue";
    case midicontrolout::seqaction::remove: return "delete";
    default:                                return "unknown";
    }
}

/**
 *  A "to_string" function for the action enumeration.
 */

std::string
action_to_string (midicontrolout::uiaction a)
{
    switch (a)
    {
    case midicontrolout::uiaction::panic:           return "Panic";
    case midicontrolout::uiaction::stop:            return "Stop";
    case midicontrolout::uiaction::pause:           return "Pause";
    case midicontrolout::uiaction::play:            return "Play";
    case midicontrolout::uiaction::toggle_mutes:    return "Toggle_mutes";
    case midicontrolout::uiaction::song_record:     return "Song_record";
    case midicontrolout::uiaction::slot_shift:      return "Slot_shift";
    case midicontrolout::uiaction::free:            return "Free";
    case midicontrolout::uiaction::queue:           return "Queue";
    case midicontrolout::uiaction::oneshot:         return "One-shot";
    case midicontrolout::uiaction::replace:         return "Replace";
    case midicontrolout::uiaction::snap:            return "Snap";
    case midicontrolout::uiaction::song:            return "Song";
    case midicontrolout::uiaction::learn:           return "Learn";
    case midicontrolout::uiaction::bpm_up:          return "BPM_Up";
    case midicontrolout::uiaction::bpm_dn:          return "BPM_Dn";
    case midicontrolout::uiaction::list_up:         return "List_Up";
    case midicontrolout::uiaction::list_dn:         return "List_Dn";
    case midicontrolout::uiaction::song_up:         return "Song_Up";
    case midicontrolout::uiaction::song_dn:         return "Song_Dn";
    case midicontrolout::uiaction::set_up:          return "Set_Up";
    case midicontrolout::uiaction::set_dn:          return "Set_Dn";
    case midicontrolout::uiaction::tap_bpm:         return "Tap_BPM";
    case midicontrolout::uiaction::quit:            return "Quit";
    case midicontrolout::uiaction::visibility:      return "Visibility";
    case midicontrolout::uiaction::alt_2:           return "Alt_2";
    case midicontrolout::uiaction::alt_3:           return "Alt_3";
    case midicontrolout::uiaction::alt_4:           return "Alt_4";
    case midicontrolout::uiaction::alt_5:           return "Alt_5";
    case midicontrolout::uiaction::alt_6:           return "Alt_6";
    case midicontrolout::uiaction::alt_7:           return "Alt_7";
    case midicontrolout::uiaction::alt_8:           return "Alt_8";
    default:                                        return "Unknown";
    }
}

/**
 *  Send out notification about playing status of a sequence.
 *
 * \todo
 *      Need to handle screen sets. Since sequences themselves are ignorant
 *      about the current screen set, maybe we can centralise this knowledge
 *      inside this class, so before sending a sequence event, we check here
 *      if the sequence is in the active screen set, otherwise we drop the
 *      event. This requires that in the performer class, we do a "repaint"
 *      each time the screen set is changed.  For now, the size of the
 *      screenset is fixed to 32 in this function.
 *
 * Also, maybe consider adding an option to the config file, making this
 * behavior optional: So either absolute sequence actions (let the receiver do
 * the math...), or sending events relative (modulo) the current screen set.
 *
 * \param index
 *      The index into the m_seq_events[][] array.
 *
 * \param seq
 *
 * \param what
 *      The status action of the sequence.  This indicates if the sequence is
 *      playing, muted, queued, or deleted (removed, empty).
 *
 * \param flush
 *      Flush MIDI buffer after sending (default true).
 */

void
midicontrolout::send_seq_event (int index, seqaction what, bool flush)
{
    bool ok = is_enabled() && index < int(m_seq_events.size());
    if (ok)
        ok = what < seqaction::max;

    if (ok)
    {
        int w = static_cast<int>(what);
        if (m_seq_events[index][w].apt_action_status)
        {
            event ev = m_seq_events[index][w].apt_action_event;
            if (not_nullptr(m_master_bus) && ev.valid_status())
            {
#if defined SEQ66_PLATFORM_DEBUG_TMI
                std::string act = seqaction_to_string(what);
                std::string evstring = ev.to_string();
                printf
                (
                    "send_seq_event(%s): %s\n", act.c_str(), evstring.c_str()
                );
#endif
                bussbyte tb = true_buss();
                if (flush)
                    m_master_bus->play_and_flush(tb, &ev, ev.channel());
                else
                    m_master_bus->play(tb, &ev, ev.channel());
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

void
midicontrolout::clear_mutes (bool flush)
{
    if (is_enabled())
    {
        for (int g = 0; g < mutegroups::Size(); ++g)
            send_mutes_event(g, action_del);

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
 *  Register a MIDI event for a given sequence action.
 *
 * \param seq
 *      The index of the sequence.
 *
 * \param what
 *      The action to be notified.
 *
 * \param ev
 *      Raw int array representing The MIDI event to be sent. It has been
 *      reduced to the three values need to specify a status event with channel
 *      and two data values.
 */

void
midicontrolout::set_seq_event (int seq, seqaction what, int * eva)
{
    bool ok = what < seqaction::max && seq < int(m_seq_events.size());
    if (ok)
    {
        int w = static_cast<int>(what);
        bool enabled = eva[index::status] > 0x00;
        event ev;
        ev.set_status_keep_channel(eva[index::status]);
        ev.set_data(eva[index::data_1], eva[index::data_2]);
        m_seq_events[seq][w].apt_action_event = ev;
        m_seq_events[seq][w].apt_action_status = enabled;
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
 *  Note the att_action_event_del is now used with uiaction events.
 *
 *  Issue #55:  While researching this issue, we found the portmidi
 *  implementation of Seq66 was trying to send events of status 0x00.
 */

void
midicontrolout::send_event (uiaction what, actionindex which)
{
    if (is_enabled() && not_nullptr(m_master_bus))
    {
        event ev;
        int w = static_cast<int>(what);
        if (event_is_active(what))
        {
            if (which == action_on)
                ev = m_ui_events[w].att_action_event_on;
            else if (which == action_off)
                ev = m_ui_events[w].att_action_event_off;
            else if (which == action_del)
                ev = m_ui_events[w].att_action_event_del;
        }
        else
            ev = m_ui_events[w].att_action_event_del;

        if (ev.valid_status())
            m_master_bus->play_and_flush(true_buss(), &ev, ev.channel());
    }
}

void
midicontrolout::send_learning (bool learning)
{
    actionindex which = learning ? action_on : action_off ;
    send_event(uiaction::learn, which);
}

void
midicontrolout::send_automation (bool activate)
{
    actionindex ai = activate ? action_off : action_del ;
    for (uiaction uia = uiaction::panic; uia < uiaction::max; ++uia)
        send_event(uia, ai);
}

void
midicontrolout::send_macro (const std::string & name, bool flush)
{
    if (is_enabled() && not_nullptr(m_master_bus))
    {
        midistring byts = m_macro_events.bytes(name);
        if (! byts.empty())
        {
            int len = int(byts.length());
            bussbyte tb = true_buss();

            /*
             * This test is inadequate.
             *
             * if (len > 3)                                // assume sysex  //
             */

            if (event::is_ex_data_msg(byts[0]))
            {
                event ev;
                const midibyte * b = midi_bytes(byts);
                (void) ev.set_sysex(b, len);
                m_master_bus->sysex(tb, &ev);               /* flushes      */
            }
            else
            {
                midibyte d1 = len == 3 ? byts[2] : 0 ;
                event ev(0, byts[0], byts[1], d1);
                if (flush)
                    m_master_bus->play_and_flush(tb, &ev, ev.channel());
                else
                    m_master_bus->play(tb, &ev, ev.channel());
            }
        }
    }
}

std::string
midicontrolout::get_event_str (const event & ev) const
{
    int s = int(ev.get_status());
    midibyte d0, d1;
    ev.get_data(d0, d1);
    std::ostringstream str;
    str
        << "[ 0x" << std::hex << std::setw(2) << std::setfill('0') << s << " "
        << std::dec << std::setw(3) << std::setfill(' ') << int(d0) << " "
        << std::dec << std::setw(3) << std::setfill(' ') << int(d1) << " ]"
        ;
    return str.str();
}

std::string
midicontrolout::get_ctrl_event_str (uiaction what, actionindex which) const
{
    std::string result;
    if (! m_ui_events.empty())
    {
        int w = static_cast<int>(what);
        event ev;
        if (which == action_on)
            ev = m_ui_events[w].att_action_event_on;
        else if (which == action_off)
            ev = m_ui_events[w].att_action_event_off;
        else if (which == action_del)
            ev = m_ui_events[w].att_action_event_del;

        result = get_event_str(ev);
    }
    return result;
}

std::string
midicontrolout::get_mutes_event_str (int group, actionindex which) const
{
    std::string result;
    event ev;
    if (m_mutes_events.size() > 0)
    {
        if (which == action_on)
            ev = m_mutes_events[group].att_action_event_on;
        else if (which == action_off)
            ev = m_mutes_events[group].att_action_event_off;
        else if (which == action_del)
            ev = m_mutes_events[group].att_action_event_del;
    }
    result = get_event_str(ev);
    return result;
}

/**
 *  3 elements in each integer array: status, d1, d2.  If either status (on vs
 *  off) is 0x00, we disable it, to avoid sending junk.
 */

void
midicontrolout::set_event
(
    uiaction what, bool enabled,
    int * onp, int * offp, int * delp
)
{
    if (what < uiaction::max && ! m_ui_events.empty())
    {
        int w = static_cast<int>(what);
        event on;
        on.set_status_keep_channel(onp[index::status]);
        on.set_data(onp[index::data_1], onp[index::data_2]);
        m_ui_events[w].att_action_event_on = on;

        event off;
        off.set_status_keep_channel(offp[index::status]);
        off.set_data(onp[index::data_1], offp[index::data_2]);
        m_ui_events[w].att_action_event_off = off;

        event del;
        del.set_status_keep_channel(delp[index::status]);
        del.set_data(onp[index::data_1], delp[index::data_2]);
        m_ui_events[w].att_action_event_del = del;    /* tricky */

        if (enabled)
        {
            /*
             * Currently the new and third section, "inactive", is optional.
             */

            if (onp[index::status] == 0x00 || offp[index::status] == 0x00)
                enabled = false;
        }
        m_ui_events[w].att_action_status = enabled;
        if (enabled)
            is_blank(false);
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
midicontrolout::event_is_active (uiaction what) const
{
    int w = static_cast<int>(what);
    return (what < uiaction::max && ! m_ui_events.empty()) ?
        m_ui_events[w].att_action_status : false ;
}

void
midicontrolout::set_mutes_event
(
    int group, int * onp, int * offp, int * delp
)
{
    if (group >= 0 && group < mutegroups::Size())
    {
        event on;
        bool enabled = onp[index::status] > 0x00;
        on.set_status_keep_channel(onp[index::status]);
        on.set_data(onp[index::data_1], onp[index::data_2]);
        m_mutes_events[group].att_action_event_on = on;

        event off;
        off.set_status_keep_channel(offp[index::status]);
        off.set_data(offp[index::data_1], offp[index::data_2]);
        m_mutes_events[group].att_action_event_off = off;

        event del;
        del.set_status_keep_channel(delp[index::status]);
        del.set_data(delp[index::data_1], delp[index::data_2]);
        m_mutes_events[group].att_action_event_del = del;
        m_mutes_events[group].att_action_status = enabled;
        if (enabled)
            is_blank(false);
    }
}

bool
midicontrolout::mutes_event_is_active (int group) const
{
    return group >= 0 && group < mutegroups::Size() ?
        m_mutes_events[group].att_action_status : false ;
}

void
midicontrolout::send_mutes_event (int group, actionindex which)
{
    bool ok = is_enabled() && mutes_event_is_active(group);
    if (ok)
    {
        event ev;
        if (which == action_on)
            ev = m_mutes_events[group].att_action_event_on;
        else if (which == action_off)
            ev = m_mutes_events[group].att_action_event_off;
        else if (which == action_del)
            ev = m_mutes_events[group].att_action_event_del;

        if (ev.valid_status() && not_nullptr(m_master_bus))
            m_master_bus->play_and_flush(true_buss(), &ev, ev.channel());
    }
}

}           // namespace seq66

/*
 * midicontrolout.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


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
 * \file          editable_events.cpp
 *
 *  This module declares/defines the base class for an ordered container of
 *  MIDI editable_events.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-12-04
 * \updates       2023-07-04
 * \license       GNU GPLv2 or above
 *
 *  A MIDI editable event is encapsulated by the seq66::editable_events
 *  object.
 */

#include "midi/editable_events.hpp"     /* seq66::editable_events           */
#include "play/sequence.hpp"            /* seq66::sequence                  */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 * We will get the default controller name from the controllers module.
 * We should also be able to look up the selected buss's entries for a
 * sequence, and load up the CC/name pairs on the fly.
 */

/**
 *  This constructor hooks into the sequence object.
 *
 * \param seq
 *      Provides a reference to the sequence object, which provides the events
 *      and some of the MIDI timing parameters.
 *
 * \param bpm
 *      Provides the beats/minute value, which the caller figures out how to
 *      get and provides in this parameter.
 */

editable_events::editable_events (sequence & s, midibpm bp) :
    m_events            (),
    m_current_event     (m_events.end()),
    m_seq               (s),
    m_midi_parameters
    (
        bp, s.get_beats_per_bar(), s.get_beat_width(), s.get_ppqn()
    )
{
    // Empty body
}

/**
 *  This copy constructor initializes most of the class members.
 *  Note that we need to reconstitute the event links here, as well.
 *
 * \note
 *      Like eventlist::verify_and_link(), this class counts on the caller
 *      (in this case, the user-interface instead of the sequence), to call it.
 *
 * \param rhs
 *      Provides the editable_events object to be copied.
 */

editable_events::editable_events (const editable_events & rhs) :
    m_events            (rhs.m_events),
    m_current_event     (rhs.m_current_event),
    m_seq               (rhs.m_seq),
    m_midi_parameters   (rhs.m_midi_parameters)
{
    // no code
}

/**
 *  This principal assignment operator sets most of the class members.
 *  Note that we need to reconstitute the event links here, as well.
 *
 * \param rhs
 *      Provides the editable_events object to be assigned.
 *
 * \return
 *      Returns a reference to "this" object, to support the serial assignment
 *      of editable_eventss.
 */

editable_events &
editable_events::operator = (const editable_events & rhs)
{
    if (this != &rhs)
    {
        m_events            = rhs.m_events;
        m_current_event     = rhs.m_current_event;
        m_midi_parameters   = rhs.m_midi_parameters;
        m_seq.partial_assign(rhs.m_seq);
    }
    return *this;
}

/**
 *  Provides the length of the events in MIDI pulses.  This function gets the
 *  iterator for the last element and returns its length value.
 *
 * \return
 *      Returns the timestamp of the latest event in the container.
 */

midipulse
editable_events::get_length () const
{
    midipulse result = 0;
    if (count() > 0)
    {
        auto lci = m_events.rbegin();               /* get last element */
        result = lci->second.timestamp();           /* get length value */
    }
    return result;
}

/**
 *  Adds an event, converted to an editable_event, to the internal event list.
 *
 * \param e
 *      Provides the regular event to be added to the list of editable events.
 *
 * \return
 *      Returns true if the insertion succeeded, as evidenced by an increment
 *      in container size.
 */

bool
editable_events::add (const event & e)
{
    editable_event ed(*this, e);            /* make the event "editable"    */
    return add(ed);
}

/**
 *  Adds an editable event to the internal event list.  For the std::multimap
 *  implementation, this is an option if we want to make sure the insertion
 *  succeeded:
 *
\verbatim
 *      std::pair<Events::iterator, bool> result = m_events.insert(p);
 *      return result.second;
\endverbatim
 *
 * \param e
 *      Provides the regular event to be added to the list of editable events.
 *
 * \return
 *      Returns true if the insertion succeeded, as evidenced by an increment
 *      in container size.
 *
 * \sideeffect
 *      Sets m_current_event, which can be used right-away in a
 *      single-threaded context to get an iterator to the event via the
 *      current_event() accessor.
 */

bool
editable_events::add (const editable_event & e)
{
    size_t count = m_events.size();         /* save initial size            */
    event::key key(e);                      /* create the key value         */
    auto p = std::make_pair(key, e);        /* EventsPair                   */
    auto ei = m_events.insert(p);           /* std::multimap operation      */
    bool result = m_events.size() == (count + 1);
    if (result)
        current_event(ei);

    return result;
}

/**
 *  Accesses the sequence's event-list, iterating through it from beginning to
 *  end, wrapping each event in the list in an editable event and inserting it
 *  into the editable-event container.
 *
 *  Note that the new events will not have valid links (actually, no links).
 *  These links are used for associating Note Off events with their respective
 *  Note On events.  To be consistent, we must take the time to reconstitute
 *  these links, using editable_events::verify_and_link().
 *
 * \return
 *      Returns true if the size of the final editable_event container matches
 *      the size of the original events container.
 */

bool
editable_events::load_events ()
{
    bool result;
    int original_count = track().events().count();
    for (const auto & ei : track().events())
    {
        if (! add(ei))
            break;
    }
    result = count() == original_count;
    return result;
}

/**
 *  Erases the sequence's event container and recreates it using the edited
 *  container of editable events.
 *
 *  Note that the old events are replaced only if the container of editable
 *  events is not empty.  There are safer ways for the user to erase all the
 *  events.
 *
 * \todo
 *      Consider what to do about the sequence::m_is_modified flag.
 *
 * \return
 *      Returns true if the size of the final event container matches
 *      the size of the original editable_events container.
 */

bool
editable_events::save_events ()
{
    bool result = count() > 0;
    if (result)
    {
        track().events().clear();
        for (const auto & ei : events())
        {
            if (! track().add_event(ei.second))         /* sorts the events */
                break;
        }
        result = track().events().count () == count();
        if (result)
        {
            /*
             * ca 2021-0-02 Reload in case of note changes.
             */

            (void) track().events().verify_and_link();  /* hmmm, 0, false   */
            clear();
            result = load_events();
        }
    }
    return result;
}

/**
 *  Calculates the MIDI pulses (divisions) from a string using one of the
 *  free functions of the calculations module.
 */

midipulse
editable_events::string_to_pulses (const std::string & ts_string) const
{
#if defined USE_OLD_CODE
    return seq66::string_to_pulses(ts_string, timing());
#else
    return track().time_signature_pulses(ts_string);
#endif
}

/**
 *  Gets the index (integer position in the map) of the linked event, if any.
 */

int
editable_events::count_to_link (const editable_event & ee) const
{
    if (ee.is_linked())
    {
        event::key k(ee);
        int index = 0;
        for (const auto & i : events())
        {
            const editable_event & e = i.second;
            if (e.is_linked())
            {
                event::key k2(*e.link());
                if (k2 == k)
                    return index;
            }
            ++index;
        }
    }
    return (-1);
}

/**
 *  One can use the event::valid_status() to make sure the item was found.
 *  Using the event::key is not fool-proof.
 */

editable_event &
editable_events::lookup_link (const editable_event & ee)
{
    static editable_event s_dummy_event;
    if (ee.is_linked())
    {
        event::key k(ee);
        for (auto & i : events())
        {
            editable_event & e = i.second;
            if (e.is_linked())
            {
                event::key k2(*e.link());
                if (k2 == k)
                    return e;
            }
        }
    }
    return s_dummy_event;
}

/**
 *  Prints a list of the currently-held events.  Useful for debugging.
 */

void
editable_events::print () const
{
    printf("editable_events[%d]:\n", count());
    for (const auto & i : events())
        i.second.print();
}

}           // namespace seq66

/*
 * editable_events.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


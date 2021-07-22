#if ! defined SEQ66_EDITABLE_EVENTS_HPP
#define SEQ66_EDITABLE_EVENTS_HPP

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
 * \file          editable_events.hpp
 *
 *  This module declares/defines a sorted container for editable_events class
 *  for operating with an ordered collection MIDI editable_events in a
 *  user-interface.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-12-04
 * \updates       2021-02-02
 * \license       GNU GPLv2 or above
 *
 *  This module extends the event class to support conversions between events
 *  and human-readable (and editable) strings.
 */

#include <map>                          /* std::multimap                */

#include "midi/editable_event.hpp"      /* seq66::editable_event        */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

class sequence;

/*
 * ui->combo_ev_name entries from editable_event::channel_event_name():
 *
 *      -   Note Off
 *      -   Note On
 *      -   Aftertouch
 *      -   Control
 *      -   Program
 *      -   Ch Pressure
 *      -   Pitch Wheel
 */


/**
 *  Provides for the management of an ordered collection MIDI editable events.
 */

class editable_events
{
    friend class qseventslots;          /* the Qt 5 version                 */

private:

    /**
     *  Types to use to with the multimap implementation.  We should consider
     *  reusing, somehow, the event::buffer (vector) implementation.  Also too
     *  much re-do here from eventlist!
     */

    using Key = event::key;
    using Events = std::multimap<Key, editable_event>;
    using iterator = Events::iterator;
    using const_iterator = Events::const_iterator;

    /**
     *  Holds the editable_events.  The multimap works well here.
     */

    Events m_events;

    /**
     *  Points to the current event, which is the event that has just been
     *  inserted.  (From this event we can get the current time and other
     *  parameters.)  If the container were a plain map, we could instead use
     *  a key to access it.  But we can at least use an iterator, rather than
     *  a bare pointer.
     */

    iterator m_current_event;

    /**
     *  Provides a reference to the sequence containing the events to be
     *  edited.  Besides the events, this object also holds the beats/measure,
     *  beat-width, and the PPQN value.  The beats/minute have to be obtained
     *  from the application's performer object, and passed to the
     *  editable_events constructor by the caller.
     */

    sequence & m_sequence;

    /**
     *  Holds the current settings for the sequence (and usually for the whole
     *  MIDI tune as well).  It holds the beats/minute, beats/measure,
     *  beat-width, and PPQN values needed to properly convert MIDI pulse
     *  timestamps to time and measure values.
     */

    midi_timing m_midi_parameters;

private:

    editable_events ();                 /* unimplemented    */

public:

    editable_events (sequence & seq, midibpm bpm);
    editable_events (const editable_events & rhs);
    editable_events & operator = (const editable_events & rhs);

    /**
     *  This destructor current is a rote virtual function override.
     */

    virtual ~editable_events ()
    {
        // Empty body
    }

public:

    /**
     * \getter m_midi_parameters
     */

    const midi_timing & timing () const
    {
        return m_midi_parameters;
    }

    /**
     *  Calculates the MIDI pulses (divisions) from a string using one of the
     *  free functions of the calculations module.
     */

    midipulse string_to_pulses (const std::string & ts_string) const
    {
        return seq66::string_to_pulses(ts_string, timing());
    }

    bool load_events ();
    bool save_events ();

    Events & events ()
    {
        return m_events;
    }

    const Events & events () const
    {
        return m_events;
    }

    iterator begin ()
    {
        return m_events.begin();
    }

    const_iterator begin () const
    {
        return m_events.begin();
    }

    iterator end ()
    {
        return m_events.end();
    }

    const_iterator end () const
    {
        return m_events.end();
    }

    /**
     *  Dereference access for list or map.
     *
     * \param ie
     *      Provides the iterator to the event to which to get a reference.
     */

    static editable_event & dref (iterator ie)
    {
        return ie->second;
    }

    /**
     *  Dereference const access for list or map.
     *
     * \param ie
     *      Provides the iterator to the event to which to get a reference.
     */

    static const editable_event & cdref (const_iterator ie)
    {
        return ie->second;
    }

    /**
     *  Returns the number of events stored in m_events.  We like returning
     *  an integer instead of size_t, and rename the function so nobody is
     *  fooled.
     */

    int count () const
    {
        return int(m_events.size());
    }

    midipulse get_length () const;

    bool add (const event & e);
    bool add (const editable_event & e);

    /**
     *  Provides a wrapper for the iterator form of erase(), which is the
     *  only one that the editable_events container uses.
     */

    bool replace (iterator ie, const editable_event & e)
    {
        if (ie != m_events.end())           /* \change ca 2017-04-30    */
            m_events.erase(ie);

        return add(e);
    }

    /**
     *  Provides a wrapper for the iterator form of erase(), which is the
     *  only one that sequence uses.
     */

    void remove (iterator ie)
    {
        if (ie != m_events.end())           /* \change ca 2017-04-30    */
            m_events.erase(ie);
    }

    void clear ()
    {
        m_events.clear();
    }

    /**
     *  Sorts the event list; active only for the std::list implementation.
     */

    void sort ()
    {
        /*
         * we need nothin' for sorting a multimap.
         */
    }

    /**
     * \getter m_current_event
     *      The caller must make sure the iterator is not Events::end().
     */

    iterator current_event () const
    {
        return m_current_event;
    }

    /**
     *  Validates the given iterator.
     */

    bool is_valid_iterator (iterator & cit) const
    {
        return cit != m_events.end();
    }

    int count_to_link (const editable_event & source);
    void print () const;

private:

    /**
     * \setter m_current_event
     *
     * \param cei
     *      Provide an iterator to the event to set as the current event.
     */

    void current_event (iterator cei)
    {
        m_current_event = cei;
    }

};          // class editable_events

}           // namespace seq66

#endif      // SEQ66_EDITABLE_EVENTS_HPP

/*
 * editable_events.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


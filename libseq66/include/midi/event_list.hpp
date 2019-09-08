#if ! defined SEQ66_EVENT_LIST_HPP
#define SEQ66_EVENT_LIST_HPP

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
 * \file          event_list.hpp
 *
 *  This module provides a stand-alone module for the event-list container
 *  used by the application.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-09-19
 * \updates       2019-09-05
 * \license       GNU GPLv2 or above
 *
 *  This module extracts the event-list functionality from the sequencer
 *  module, so that it is easier to try to replace it with some better
 *  container later.
 *
 *  We should leverage "for-each" functionality.
 *
 *  List versus Map:  #if defined or derivation from an interface?  For our
 *  purposes, #if defined might be simplest, and we only want to pick the
 *  fastest one, ultimately.
 *
 *  It turns out the the std::multimap implementation is a little bit faster in
 *  release mode, and a lot faster in debug mode.  Why?  Probably because
 *  the std::list implementation calls std::list::sort() a lot, and the
 *  std::multimap implementation is a lot faster at sorting.  But since the
 *  map iterator is slower, we stick with std::list.
 *
 *  But, based on this article:
 *
 * https://baptiste-wicht.com/posts/2012/12/cpp-benchmark-vector-list-deque.html
 *
 *  We will now try std::vector for the event list.
 */

#include <string>
#include <stack>

#include "midi/event.hpp"

#if defined SEQ66_USE_EVENT_LIST
#include <list>                         /* std::list                    */
#else
#include <algorithm>                    /* std::sort(), std::merge()    */
#include <vector>                       /* std::vector                  */
#endif

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  The event_list class is a receptable for MIDI events.
 */

class event_list
{
    friend class editable_events;       // access to event_key class
    friend class midifile;              // access to print()
    friend class sequence;              // any_selected_notes()

private:

    /**
     *  Provides a key value for an event map.  Its types match the
     *  m_timestamp and get_rank() function of this event class.
     */

    class event_key
    {

    private:

        midipulse m_timestamp;  /**< The primary key-value for the key. */
        int m_rank;             /**< The sub-key-value for the key.     */

    public:

        event_key (midipulse tstamp, int rank);
        event_key (const event & e);
        bool operator < (const event_key & rhs) const;
        event_key (const event_key & ek) = default;
        event_key & operator = (const event_key & ek) = default;
    };

public:

#if defined SEQ66_USE_EVENT_LIST
    using Events = std::list<event>;
#else
    using Events = std::vector<event>;
#endif
    using iterator = Events::iterator;
    using const_iterator = Events::const_iterator;
    using reverse_iterator = Events::reverse_iterator;
    using const_reverse_iterator = Events::const_reverse_iterator;

private:

    /**
     *  This list holds the current pattern/sequence events.
     */

    Events m_events;

    /**
     *  A flag to indicate if an event was added or removed.  We may need to
     *  give client code a way to reload the sequence.  This is currently an
     *  issue when a seqroll and an eventedit/eventslots are active for the
     *  same sequence.
     */

    bool m_is_modified;

    /**
     *  A new flag to indicate that a tempo event has been added.  Legacy
     *  behavior forces the tempo to be written to the track-0 sequence,
     *  but we don't want to do that if the MIDI file (or the current event
     *  list) contains a tempo event.
     */

    bool m_has_tempo;

    /**
     *  A new flag to indicate that a time-signature event has been added.
     *  Legacy behavior forces the time-signature to be written to the track-0
     *  sequence, but we don't want to do that if the MIDI file (or the
     *  current event list) contains a time-signature event.
     */

    bool m_has_time_signature;

public:

    event_list ();
    event_list (const event_list & a_rhs);
    event_list & operator = (const event_list & a_rhs);
    ~event_list ();

    iterator begin ()
    {
        return m_events.begin();
    }

    const_iterator cbegin () const
    {
        return m_events.cbegin();
    }

    iterator end ()
    {
        return m_events.end();
    }

    const_iterator cend () const
    {
        return m_events.cend();
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

    /**
     *  Returns true if there are no events.
     *
     *  return m_events.size() == 0;
     */

    bool empty () const
    {
        return m_events.empty();
    }

    /**
     *  Adds an event to the internal event list in an optionally sorted
     *  manner.
     *
     *  Note that, for speed, it is better to call append() for each event, and
     *  then later sort them.
     *
     * \param e
     *      Provides the event to be added to the list.
     *
     * \return
     *      Returns true.  We assume the insertion succeeded, and no longer
     *      care about an increment in container size.  It's a multimap, so it
     *      always inserts, and if we don't have memory left, all bets are off
     *      anyway.
     */

    bool add (const event & e)
    {
        bool result = append(e);
        sort();                         /* by time-stamp and "rank" */
        return result;
    }

    bool append (const event & e);

    /**
     *  Needed as a special case when std::list is used.
     *
     * \param e
     *      Provides the event value to push at the back of the event list.
     */

    void push_back (const event & e)
    {
        m_events.push_back(e);
    }

    bool is_modified () const
    {
        return m_is_modified;
    }

    bool has_tempo () const
    {
        return m_has_tempo;
    }

    bool has_time_signature () const
    {
        return m_has_time_signature;
    }

    /**
     * \setter m_is_modified
     *      This function may be needed by some of the sequence editors.
     *      But use it with great caution.
     */

    void unmodify ()
    {
        m_is_modified = false;
    }

    /**
     *  Provides a wrapper for the iterator form of erase(), which is the
     *  only one that sequence uses.  Currently, no check on removal is
     *  performered.  Sets the modified-flag.
     *
     * \param ie
     *      Provides the iterator to the event to be removed.
     *
     * \return
     *      Returns an iterator to the next element, or end() if the container
     *      is now empty.
     */

    iterator remove (iterator ie)
    {
        iterator result = m_events.erase(ie);
        m_is_modified = true;
        return result;
    }

    /**
     *  Provides a wrapper for clear().  Sets the modified-flag.
     */

    void clear ()
    {
        m_events.clear();
        m_is_modified = true;
    }

    void merge (event_list & el, bool presort = true);

    /**
     *  Sorts the event list; active only for the std::list or std::vector
     *  implementation. For the vector, equivalent elements are not guaranteed
     *  to keep their original relative order [see std::stable_sort(), which
     *  we could try at some point].
     */

    void sort ()
    {
#if defined SEQ66_USE_EVENT_LIST
        m_events.sort();
#else
        std::sort(m_events.begin(), m_events.end());
#endif
    }

    /**
     *  Dereference access for list or map.
     *
     * \param ie
     *      Provides the iterator to the event to which to get a reference.
     */

    static event & dref (iterator ie)
    {
        return *ie;
    }

    /**
     *  Dereference const access for list or map.
     *
     * \param ie
     *      Provides the iterator to the event to which to get a reference.
     */

    static const event & cdref (const_iterator ie)
    {
        return *ie;
    }

private:                                // functions for friend sequence

    /*
     * The following functions provide internal for-loops that do not
     * involved data from the caller.
     */

    void link_new ();
    void clear_links ();
#if defined USE_FILL_TIME_SIG_AND_TEMPO
    void scan_meta_events ();
#endif
    void verify_and_link (midipulse slength);
    bool link_new_note (event & eon, event & eoff);
    bool link_note (event & eon, event & eoff);
    void link_tempos ();
    void clear_tempo_links ();
    bool mark_selected ();
    void mark_out_of_range (midipulse slength);
    void mark_all ();
    void unmark_all ();
    bool remove_marked ();
    void unpaint_all ();
    int count_selected_notes () const;
    bool any_selected_notes () const;
    int count_selected_events (midibyte status, midibyte cc) const;
    bool any_selected_events (midibyte status, midibyte cc) const;
    void select_all ();
    void unselect_all ();
    void print () const;

    const Events & events () const
    {
        return m_events;
    }

};          // class event_list

}           // namespace seq66

#endif      // SEQ66_EVENT_LIST_HPP

/*
 * event_list.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


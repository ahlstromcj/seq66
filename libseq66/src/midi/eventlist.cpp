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
 * \file          eventlist.cpp
 *
 *  This module declares/defines a class for handling MIDI events in a list
 *  container.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-09-19
 * \updates       2019-10-29
 * \license       GNU GPLv2 or above
 *
 *  This container now can indicate if certain Meta events (time-signaure or
 *  tempo) have been added to the container.
 *
 *  This module also defines the  eventlist::event_key object.  Although the
 *  main MIDI container are now back to using std::list (with sorting after
 *  loading).
 */

#include <stdio.h>                      /* C::printf()                  */

#include "util/basic_macros.hpp"
#include "midi/eventlist.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Principal event_key constructor.
 *
 * \param tstamp
 *      The time-stamp is the primary part of the key.  It is the most
 *      important key item.
 *
 * \param rank
 *      Rank is an arbitrary number used to prioritize events that have the
 *      same time-stamp.  See the event::get_rank() function for more
 *      information.
 */

eventlist::event_key::event_key (midipulse tstamp, int rank)
 :
    m_timestamp (tstamp),
    m_rank      (rank)
{
    // Empty body
}

/**
 *  Event-based constructor.  This constructor makes it even easier to
 *  create an event_key.  Note that the call to event::get_rank() makes a
 *  simple calculation based on the status of the event.
 *
 * \param rhs
 *      Provides the event key to be copied.
 */

eventlist::event_key::event_key (const event & rhs)
 :
    m_timestamp (rhs.timestamp()),
    m_rank      (rhs.get_rank())
{
    // Empty body
}

/**
 *  Provides the minimal operator needed to sort events using an event_key.
 *
 * \param rhs
 *      Provides the event key to be compared against.
 *
 * \return
 *      Returns true if the rank and timestamp of the current object are less
 *      than those of rhs.
 */

bool
eventlist::event_key::operator < (const event_key & rhs) const
{
    if (m_timestamp == rhs.m_timestamp)
        return (m_rank < rhs.m_rank);
    else
        return (m_timestamp < rhs.m_timestamp);
}

/**
 *  Principal constructor.
 */

eventlist::eventlist ()
 :
    m_events                (),
    m_length                (0),
    mc_note_off_margin      (2),
    m_is_modified           (false),
    m_has_tempo             (false),
    m_has_time_signature    (false)
{
    // No code needed
}

/**
 *  Copy constructor.
 *
 * \param rhs
 *      Provides the event list to be copied.
 */

eventlist::eventlist (const eventlist & rhs)
 :
    m_events                (rhs.m_events),
    m_length                (rhs.m_length),
    mc_note_off_margin      (rhs.mc_note_off_margin),
    m_is_modified           (rhs.m_is_modified),
    m_has_tempo             (rhs.m_has_tempo),
    m_has_time_signature    (rhs.m_has_time_signature)
{
    // No code needed
}

/**
 *  Principal assignment operator.  Follows the stock rules for such an
 *  operator, just assigning member values.
 *
 * \param rhs
 *      Provides the event list to be assigned.
 */

eventlist &
eventlist::operator = (const eventlist & rhs)
{
    if (this != &rhs)
    {
        m_events                = rhs.m_events;
        m_is_modified           = rhs.m_is_modified;
        m_has_tempo             = rhs.m_has_tempo;
        m_has_time_signature    = rhs.m_has_time_signature;
    }
    return *this;
}

/**
 *  Provides the length of the events in MIDI pulses.  This function gets the
 *  iterator for the last element and returns its length value.
 *
 * \return
 *      Returns the timestamp of the last event in the container.
 */

midipulse
eventlist::get_max_timestamp () const
{
    midipulse result = 0;
    if (count() > 0)
    {
        auto lci = m_events.rbegin();                   /* get last element */
        result = lci->timestamp();                      /* get length value */
    }
    return result;
}

/**
 *  Adds an event to the internal event list without sorting.  It is a
 *  wrapper, wrapper for insert() or push_front(), with an option to call
 *  sort().
 *
 *  The add() function without sorting, useful to speed up the initial
 *  container loading into the event-list.
 *
 *  For the std::multimap implementation, This is an option if we want to make
 *  sure the insertion succeed.
 *
 *  If the std::list implementation has been built in, then the event list is
 *  not sorted after the addition.  This is a time-consuming operation.
 *
 *  We also have to raise some new flags if the event is a Set Tempo or
 *  Time Signature event, so that we do not force the current tempo and
 *  time-signature when writing the MIDI file.
 *
 * \warning
 *      This pushing (and, in writing the MIDI file, the popping),
 *      causes events with identical timestamps to be written in
 *      reverse order.  Doesn't affect functionality, but it's puzzling
 *      until one understands what is happening.  That's why we're
 *      now preferring to use an std::multimap as the container.  But see the
 *      new news on using std::vector.
 *
 * \param e
 *      Provides the event to be added to the list.
 *
 * \return
 *      Returns true.  We assume the insertion succeeded, and no longer care
 *      about an increment in container size.  It's a multimap, so it always
 *      inserts, and if we don't have memory left, all bets are off anyway.
 */

bool
eventlist::append (const event & e)
{
    m_events.push_back(e);              /* std::vector operation        */
    m_is_modified = true;
    if (e.is_tempo())
        m_has_tempo = true;

    if (e.is_time_signature())
        m_has_time_signature = true;

    return true;
}

/**
 *  An internal function to add events to a temporary list.  Used in
 *  quantization and tightening operations.
 */

bool
eventlist::add (Events & evlist, const event & e)
{
    evlist.push_back(e);                /* std::vector operation        */
    std::sort(evlist.begin(), evlist.end());
    return true;
}

/**
 *  An internal function to merge events from a temporary list.  Used in
 *  quantization and tightening operations.
 */

void
eventlist::merge (const Events & evlist)
{
    m_events.reserve(m_events.size() + evlist.size());
    m_events.insert(m_events.end(), evlist.begin(), evlist.end());
    std::sort(m_events.begin(), m_events.end());
}

/**
 *  Provides a merge operation for the event multimap analogous to the merge
 *  operation for the event list.  We have certain constraints to preserve, as
 *  the following discussion shows.
 *
 *  For std::list, sequence merges list T into list A by first calling
 *  T.sort(), and then A.merge(T).  The merge() operation merges T into A by
 *  transferring all of its elements, at their respective ordered positions,
 *  into A.  Both containers must already be ordered.
 *
 *  The merge effectively removes all the elements in T (which becomes empty),
 *  and inserts them into their ordered position within container (which
 *  expands in size by the number of elements transferred). The operation is
 *  performered without constructing nor destroying any element, whether T is an
 *  lvalue or an rvalue, or whether the value-type supports move-construction
 *  or not.
 *
 *  std::vector::insert(iterator pos, InputIterator 1st, InputIterator 2nd)
 *
 *  Each element of T is inserted at the position that corresponds to its
 *  value according to the strict weak ordering defined by operator <. The
 *  resulting order of equivalent elements is stable (i.e. equivalent elements
 *  preserve the relative order they had before the call, and existing
 *  elements precede those equivalent inserted from x).  The function does
 *  nothing if (&x == this).
 *
 *  For std::multimap, sorting is automatic.  However, unless
 *  move-construction is supported, merging will be less efficient than for
 *  the list (now a vector) version.
 *
 * \param el
 *      Provides the event list to be merged into the current event list.
 *
 * \param presort
 *      If true, the events are presorted.  This is a requirement for merging
 *      an std::list or std::vector, but is a no-op for the std::multimap
 *      implementation [which no longer exists].
 */

void
eventlist::merge (eventlist & el, bool presort)
{
    if (presort)                            /* not really necessary here    */
        el.sort();                          /* el.m_events.sort();          */

    m_events.reserve(m_events.size() + el.m_events.size());
    m_events.insert(m_events.end(), el.m_events.begin(), el.m_events.end());
    std::sort(m_events.begin(), m_events.end());
}

/**
 *  Links a new event.  This function checks for a note on, then look for
 *  its note off.  This function is provided in the eventlist because it
 *  does not depend on any external data.  Also note that any desired
 *  thread-safety must be provided by the caller.
 */

void
eventlist::link_new ()
{
    bool endfound = false;
    for (auto on = m_events.begin(); on != m_events.end(); ++on)
    {
        event & eon = dref(on);
        if (eon.linkable())
        {
            auto off = on;                          /* point to note on     */
            ++off;                                  /* get next element     */
            endfound = false;
            while (off != m_events.end())
            {
                event & eoff = dref(off);
                endfound = link_new_note(eon, eoff);
                ++off;
                if (endfound)
                    break;
            }
            if (! endfound)
            {
                off = m_events.begin();
                while (off != on)
                {
                    event & eoff = dref(off);
                    endfound = link_note(eon, eoff);
                    ++off;
                    if (endfound)
                        break;
                }
            }
        }
    }
}


/**
 *  THINK ABOUT IT:  If we're in legacy merge mode for a loop, the Note Off is
 *  actually earlier than the Note On.  And in replace mode, the Note On is
 *  cleared, leaving us with a dangling Note Off event.
 *
 *  We should consider, in both modes, automatically adding the Note Off at the
 *  end of the loop and ignoring the next note off on the same note from the
 *  keyboard.
 *
 *  Careful!
 */

bool
eventlist::link_new_note (event & eon, event & eoff)
{
    bool result = eon.linkable(eoff);
    if (result)
    {
        eon.link(&eoff);                /* link + mark                  */
        eoff.link(&eon);

        /*
         * Not sure why we bother to mark here, when we unmark them
         * all afterward in preparation for potential pruning.
         *
         *  eon.mark();
         *  eoff.mark();
         */
    }
    return result;
}

/**
 *  The same as link_new_note(), except that it checks is_marked() instead of
 *  is_linked().
 */

bool
eventlist::link_note (event & eon, event & eoff)
{
    bool result = eon.linkable(eoff);
    if (result)
    {
        eon.link(&eoff);                /* link + mark                  */
        eoff.link(&eon);

        /*
         * Not sure why we bother to mark here, when we unmark them
         * all afterward in preparation for potential pruning.
         *
         *  eon.mark();
         *  eoff.mark();
         */
    }
    return result;
}

/**
 *  This function verifies state: all note-ons have an off, and it links
 *  note-offs with their note-ons.
 *
 * Stazed (seq32):
 *
 *      This function now deletes any notes that are >= m_length, so any
 *      resize or move of notes must modify for wrapping if Note Off is >=
 *      m_length.
 *
 * \threadunsafe
 *      As in most case, the caller will use an automutex to call this
 *      function safely.
 *
 * \param slength
 *      Provides the length beyond which events will be pruned. Normally the
 *      called supplies sequence::get_length().
 */

void
eventlist::verify_and_link (midipulse slength)
{
    clear_links();
    sort();                                 /* IMPORTANT!                   */
    for (auto on = m_events.begin(); on != m_events.end(); ++on)
    {
        event & eon = dref(on);
        if (eon.is_note_on())               /* Note On, find its Note Off   */
        {
            bool endfound = false;
            auto off = on;                  /* next possible Note Off...    */
            ++off;                          /* ...starting here             */
            while (off != m_events.end())
            {
                event & eoff = dref(off);
                endfound = link_note(eon, eoff);
                if (endfound)
                    break;

                ++off;
            }
            if (! endfound)
            {
                off = m_events.begin();
                while (off != on)
                {
                    event & eoff = dref(off);
                    endfound = link_note(eon, eoff);
                    if (endfound)
                        break;

                    ++off;
                }
            }
        }
    }
    unmark_all();
    if (slength > 0)
    {
        mark_out_of_range(slength);
        (void) remove_marked();             /* prune out-of-range events    */
    }

    /*
     *  Link the tempos in a separate pass (it makes the logic easier and the
     *  amount of time should be unnoticeable to the user.
     */

    link_tempos();
}

/**
 *  Clears all event links and unmarks them all.
 */

void
eventlist::clear_links ()
{
    for (auto & e : m_events)
    {
        e.unmark();
        e.unlink();                     /* used to be e.clear_link()        */
    }
}

/**
 *  Tries to fix the selected notes that started near the end of the pattern and
 *  wrapped around to the beginning, by moving the note.
 *
 * \param snap
 *      Provides the sequence's current snap value.  Notes that start at less
 *      than half that from the end of the pattern, and end earlier in the
 *      pattern, will be adjusted.
 *
 * \param seqlength
 *      Provides the length of the pattern.
 *
 * \return
 *      Returns true if a least one note was adjusted. There must be a note-on
 *      that is linked and fits the criterion noted above.
 */

bool
eventlist::edge_fix (midipulse snap, midipulse seqlength)
{
    bool result = false;
    for (auto & e : m_events)
    {
        if (e.is_selected() && e.is_note_on() && e.is_linked())
        {
            midipulse onstamp = e.timestamp();
            midipulse maximum = seqlength - snap / 2;
            if (onstamp > maximum)
            {
                midipulse delta = seqlength - onstamp;
                midipulse offstamp = e.link()->timestamp();
                if (offstamp < onstamp)
                {
                    e.set_timestamp(0);         /* move to beginning    */
                    e.link()->set_timestamp(offstamp + delta);
                    result = true;
                }
            }
        }
    }
    if (result)
        sort();                                 /* timestamps altered   */

    return result;
}

/**
 *  Quantizes the currently-selected set of events that match the type of
 *  event specified.  This function first marks the selected events.  Then it
 *  grabs the matching events, puts them into a list of events to be quantized
 *  and quantizes them against the snap ticks.  Linked events (which are
 *  always Note On or Note Off) are adjusted as well, with Note Offs that wrap
 *  around being adjust to be just at the end of the pattern.  This function
 *  them removes the marked event from the sequence, and merges the quantized
 *  events into the pattern's event container.  Finally, the modified event
 *  list is verified and linked.
 *
 *  Seq32:
 *
 *      If ft is negative, then we have a Note Off previously wrapped before
 *      adjustment. Since the delta is based on the Note On (not wrapped), we
 *      must add back the m_length for the wrapping.  If the ft is then >=
 *      m_length, it will be deleted by verify_and_link(), which discards any
 *      notes (ON or OFF) that are >= m_length. So we must wrap if > m_length
 *      and trim if == m_length.  Compare to trim_timestamp().
 *
 * \param status
 *      Indicates the type of event to be quantized.
 *
 * \param cc
 *      The desired control-change to count, if the event is a control-change.
 *
 * \param snap_tick
 *      Provides the maximum amount to move the events.  Actually, events are
 *      moved to the previous or next snap_tick value depend on whether they
 *      are halfway to the next one or not.
 *
 * \param divide
 *      A rough indicator of the amount of quantization.  The only values used
 *      in the application are either 1 ("quantize") or 2 ("tighten").  The
 *      latter value reduces the amount of change slightly.  This value is not
 *      tested for 0.
 *
 * \param fixlink
 *      False by default, this parameter indicates if linked events are to be
 *      adjusted against the length of the pattern
 */

bool
eventlist::quantize_events
(
    midibyte status, midibyte cc, int snap,
    int divide, bool fixlink
)
{
    bool result = false;
    if (mark_selected())
    {
        midipulse length = get_length();
        Events quantized_events;
        for (auto & er : m_events)
        {
            if (! er.is_marked())
                continue;

            midibyte d0, d1;
            er.get_data(d0, d1);
            bool match = er.get_status() == status;
            bool canselect;
            if (status == EVENT_CONTROL_CHANGE)
                canselect = match && d0 == cc;  /* correct status, correct cc */
            else
                canselect = match;              /* correct status, any cc     */

            if (canselect)
            {
                event e = er;                   /* copy the event             */
                midipulse t = e.timestamp();
                midipulse t_remainder = t % snap;
                midipulse t_delta;
                if (t_remainder < snap / 2)
                    t_delta = -(t_remainder / divide);
                else
                    t_delta = (snap - t_remainder) / divide;

                if ((t_delta + t) >= length)  /* wrap-around Note On    */
                    t_delta = -e.timestamp();

                er.select();                    /* selected original event    */
                e.unmark();                     /* unmark copy of the event   */
                e.set_timestamp(e.timestamp() + t_delta);
                add(quantized_events, e);
                result = true;

                if (er.is_linked() && fixlink)
                {
                    /*
                     * Only notes are linked; the status of all notes here are
                     * On, so the link must be an Off.  Also see "Seq32" in
                     * banner.
                     */

                    event f = *er.link();
                    midipulse ft = f.timestamp() + t_delta; /* seq32 */
                    f.unmark();
                    er.link()->select();
                    if (ft < 0)                     /* unwrap Note Off      */
                        ft += length;

                    if (ft > length)                /* wrap it around       */
                        ft -= length;

                    if (ft == length)               /* trim it a little     */
                        ft -= note_off_margin();

                    f.set_timestamp(ft);
                    add(quantized_events, f);
                    result = true;
                }
            }
        }
        (void) remove_marked();
        merge(quantized_events);                    /* also sorts events    */
        verify_and_link();                          /* sorts them again!!!  */
    }
    return result;
}

/**
 *  Consolidates the adjustment of timestamps in a pattern.
 *
 *  -   If the timestamp plus the delta is greater that m_length, we do
 *      round robin magic.
 *  -   If the timestamp is greater than m_length, then it is wrapped
 *      around to the beginning.
 *  -   If the timestamp equals m_length, then it is set to 0, and later,
 *      trimmed.
 *  -   If the timestamp is less than 0, then it is set to the end.
 *
 *  Taken from similar code in move_selected_notes() and grow_selected().  Be
 *  careful using this function.
 *
 * \param t
 *      Provides the timestamp to be adjusted based on m_length.
 *
 * \param isnoteoff
 *      Used for "expanding" the timestamp from 0 to just less than m_length,
 *      if necessary.  Should be set to true only for Note Off events; it
 *      defaults to false, which means to wrap the events around the end of
 *      the sequence if necessary, and is used only in movement, not in growth.
 *
 * \return
 *      Returns the adjusted timestamp.
 */

midipulse
eventlist::adjust_timestamp (midipulse t, bool isnoteoff)
{
    midipulse seqlength = get_length();
    if (t > seqlength)
        t -= seqlength;

    if (t < 0)                          /* only if midipulse is signed  */
        t += seqlength;

    if (isnoteoff)
    {
        if (t == 0)
            t = seqlength - note_off_margin();
    }
    else                                /* if (wrap)                    */
    {
        if (t == seqlength)
            t = 0;
    }
    return t;
}

/**
 *  Removes and adds selected notes in position.  Also currently moves any
 *  other events in the range of the selection.
 *
 *  Another thing this function does is wrap-around when movement occurs.
 *  Any events (except Note Off) that will start just after the END of the
 *  pattern will be wrapped around to the beginning of the pattern.
 *
 * \param delta_tick
 *      Provides the amount of time to move the selected notes.  Note that it
 *      also applies to events.  Note-Off events are expanded to m_length if
 *      their timestamp would be 0.  All other events will wrap around to 0.
 *
 * \param delta_note
 *      Provides the amount of pitch to move the selected notes.  This value
 *      is applied only to Note (On and Off) events.  Also, if this value
 *      would bring a note outside the range of 0 to 127, that note is not
 *      changed and the event is not moved.
 */

bool
eventlist::move_selected_notes (midipulse delta_tick, int delta_note)
{
    bool result = false;
    for (auto & er : m_events)
    {
        if (er.is_selected() && er.is_note())
        {
            int newnote = er.get_note() + delta_note;
            if (newnote >= 0 && newnote < c_num_keys)
            {
                midipulse newts = er.timestamp() + delta_tick;
                newts = adjust_timestamp(newts, er.is_note_off());
                if (er.is_note())                /* Note On or Note Off  */
                    er.set_note(midibyte(newnote));

                er.set_timestamp(newts);
                result = true;
            }
        }
    }
    if (result)
        sort();                                 /* timestamps altered   */

    return result;
}

/**
 *  This function randomizes a portion of each selected event. If the event is
 *  a two-byte message (note on/off, aftertouch, pitch wheel, or control
 *  change), the second byte (e.g. velocity for notes) is altered. If the
 *  event is one byte (program change or channel pressure), the first byte is
 *  altered.
 *
 *  See http://c-faq.com/lib/randrange.html for details.
 */

bool
eventlist::randomize_selected (midibyte status, midibyte control, int range)
{
    bool result = false;
    for (auto & e : m_events)
    {
        if (e.is_selected() && e.get_status() == status)
        {
            midibyte data[2];
            e.get_data(data[0], data[1]);

            int datidx = event::is_two_byte_msg(status) ? 1 : 0 ;
            midibyte datitem = data[datidx];
            int random = rand() / (RAND_MAX / (2 * range + 1) + 1) - range;
            datitem += random;
            if (datitem > c_max_midi_data_value)            /* 127 */
                datitem = c_max_midi_data_value;

            data[datidx] = datitem;
            e.set_data(data[0], data[1]);
            result = true;
        }
    }
    return result;
}

/**
 *  This function randomizes a Note On or Note Off message, and more
 *  thoroughly than randomize_selected().  We want to be able to jitter the
 *  note event in time, and jitter the velocity (data byte d[1]) of the note.
 *
 * \param length
 *      The length of the sequence containing the notes.
 *
 * \param jitter
 *      Provides the amount of time jitter in ticks.  Defaults to 8.
 *
 * \param range
 *      Provides the amount of velocity jitter.  Defaults to 8.
 */

bool
eventlist::randomize_selected_notes (int jitter, int range)
{
    bool result = false;
    midipulse length = get_length();

#if defined SEQ66_PLATFORM_DEBUG_TMI
    printf("Before randomization\n");
    print();
#endif

    for (auto & e : m_events)
    {
        if (e.is_selected() && e.is_note())
        {
            if (range > 0)
            {
                int random = rand() / (RAND_MAX / (2*range + 1) + 1) - range;
                if (random != 0)
                {
                    midibyte data[2];
                    e.get_data(data[0], data[1]);

                    int velocity = int(data[1]);
                    velocity += random;
                    if (velocity < 0)
                        velocity = 0;
                    else if (velocity > c_max_midi_data_value)
                        velocity = c_max_midi_data_value;

                    e.note_velocity(velocity);
                    result = true;
                }
            }
            if (jitter > 0)
            {
                int random = rand() / (RAND_MAX / (2*jitter + 1) + 1) - jitter;
                if (random != 0)
                {
                    int tstamp = int(e.timestamp());
                    tstamp += random;
                    if (tstamp < 0)
                        tstamp = 0;
                    else if (tstamp > int(length))
                        tstamp = int(length);

                    e.set_timestamp(midipulse(tstamp));
                    result = true;
                }
            }
        }
    }
    if (result)
        sort();                                 /* timestamps altered   */

#if defined SEQ66_PLATFORM_DEBUG_TMI
    printf("After randomization\n");
    print();
#endif

    return result;
}

#if defined USE_FILL_TIME_SIG_AND_TEMPO

/**
 *  Scans the event-list for any tempo or time_signature events.
 *  The use may have deleted them and is depending on a setting made in the
 *  user-interface.  So we must set/unset the flags before saving.  This check
 *  was added to fix issue #141.
 */

void
eventlist::scan_meta_events ()
{
    m_has_tempo = false;
    m_has_time_signature = false;
    for (auto & e : m_events)
    {
        if (e.is_tempo())
            m_has_tempo = true;

        if (e.is_time_signature())
            m_has_time_signature = true;
    }
}

#endif  // USE_FILL_TIME_SIG_AND_TEMPO

/**
 *  This function tries to link tempo events.  Native support for temp tracks
 *  is a new feature of seq66.  These links are only in one direction: forward
 *  in time, to the next tempo event, if any.
 *
 *  Also, at present, tempo events are not markable.
 *
 * \threadunsafe
 *      As in most case, the caller will use an automutex to call this
 *      function safely.
 */

void
eventlist::link_tempos ()
{
    clear_tempo_links();
    for (auto t = m_events.begin(); t != m_events.end(); ++t)
    {
        event & e = dref(t);
        if (e.is_tempo())
        {
            auto t2 = t;                    /* next possible Set Tempo...   */
            ++t2;                           /* ...starting here             */
            while (t2 != m_events.end())
            {
                event & et2 = dref(t2);
                if (et2.is_tempo())
                {
                    e.link(&et2);                   /* link + mark          */
                    break;                          /* tempos link one way  */
                }
                ++t2;
            }
        }
    }
}

/**
 *  Clears all tempo event links.
 */

void
eventlist::clear_tempo_links ()
{
    for (auto & e : m_events)
    {
        if (e.is_tempo())
            e.unlink();                 /* was e.clear_link()   */
    }
}

/**
 *  Marks all selected events.
 *
 * \return
 *      Returns true if there was even one event selected and marked.
 */

bool
eventlist::mark_selected ()
{
    bool result = false;
    for (auto & e : m_events)
    {
        if (e.is_selected())
        {
            e.mark();
            result = true;
        }
    }
    return result;
}

/**
 *  Marks all events.  Not yet used, but might come in handy with the event
 *  editor dialog.
 */

void
eventlist::mark_all ()
{
    for (auto & e : m_events)
        e.mark();
}

/**
 *  Unmarks all events.
 */

void
eventlist::unmark_all ()
{
    for (auto & e : m_events)
        e.unmark();
}

/**
 *  Marks all events that have a time-stamp that is out of range.
 *  Used for killing (pruning) those events not in range.  If the current
 *  time-stamp is greater than the length, then the event is marked for
 *  pruning.
 *
 * \note
 *      This code was comparing the timestamp as greater than or equal to the
 *      sequence length.  However, being equal is fine.  This may explain why
 *      the midifile code would add one tick to the length of the last note
 *      when processing the end-of-track.
 *
 * \param slength
 *      Provides the length beyond which events will be pruned.
 */

void
eventlist::mark_out_of_range (midipulse slength)
{
    for (auto & e : m_events)
    {
        bool prune = e.timestamp() > slength;   /* WAS ">=", SEE BANNER */
        if (! prune)
            prune = e.timestamp() < 0;          /* added back, seq66    */

        if (prune)
        {
            e.mark();
            if (e.is_linked())
                e.link()->mark();
        }
    }
}

/**
 *  A helper function for sequence.
 *  Finds the given event, and removes the first iterator
 *  matching that.  If there are events that would match after that, they
 *  remain in the container.  This matches seq24 behavior.
 *
 * \todo
 *      Use the find() function to find the matching event more
 *      conventionally.
 *
 * \param e
 *      Provides a reference to the event to be removed.
 *
 * \return
 *      Returns true if the event was found and removed.
 */

bool
eventlist::remove_event (event & e)
{
    bool result = false;
    for (auto i = m_events.begin(); i != m_events.end(); ++i)
    {
        event & er = dref(i);
        if (&e == &er)                  /* comparing pointers, not values   */
        {
            (void) remove(i);           /* an iterator is required here     */
            result = true;
            break;
        }
    }
    return result;
}

/**
 *  Removes marked events.  Note how this function handles removing a
 *  value to avoid incrementing a now-invalid iterator.
 *
 * \threadsafe
 * \deprecated
 *
 * \return
 *      Returns true if at least one event was removed.
 */

bool
eventlist::remove_marked ()
{
    bool result = false;
    auto i = m_events.begin();
    while (i != m_events.end())
    {
        if (dref(i).is_marked())
        {
            auto t = remove(i);
            i = t;
            result = true;
        }
        else
            ++i;
    }
    return result;
}

/**
 *  Removes selected events.  Note how this function handles removing a
 *  value to avoid incrementing a now-invalid iterator.
 *
 *  We want to get rid of the concept of marking events.  Selected events can
 *  be handled directly in the event container.
 *
 * \return
 *      Returns true if at least one event was removed.
 */

bool
eventlist::remove_selected ()
{
    bool result = false;
    auto i = m_events.begin();
    while (i != m_events.end())
    {
        if (dref(i).is_selected())
        {
            auto t = remove(i);
            i = t;
            result = true;
        }
        else
            ++i;
    }
    return result;
}

/**
 *  Unpaints all list-events.
 */

void
eventlist::unpaint_all ()
{
    for (auto & e : m_events)
        e.unpaint();
}

/**
 *  Counts the selected note-on events in the event list.
 */

int
eventlist::count_selected_notes () const
{
    int result = 0;
    for (auto & e : m_events)
    {
        if (e.is_note_on() && e.is_selected())
            ++result;
    }
    return result;
}

/**
 *  Indicates that at least one note is selected.  Acts like
 *  eventlist::count_selected_notes(), but stops after finding a selected
 *  note.
 *
 * \return
 *      Returns true if at least one note is selected.
 */

bool
eventlist::any_selected_notes () const
{
    bool result = false;
    for (auto & e : m_events)
    {
        if (e.is_note_on() && e.is_selected())
        {
            result = true;
            break;
        }
    }
    return result;
}

/**
 *  Counts the selected events, with the given status, in the event list.
 *  If the event is a control change (CC), then it must also match the
 *  given CC value.  The one exception is tempo events, which are always
 *  selectable.
 *
 * \param status
 *      The desired status value to count.
 *
 * \param cc
 *      The desired control-change to count.  Used only if the status
 *      parameter indicates a control-change event.
 *
 * \return
 *      Returns the number of selected events.
 */

int
eventlist::count_selected_events (midibyte status, midibyte cc) const
{
    int result = 0;
    for (auto & e : m_events)
    {
        if (e.is_tempo())
        {
            if (e.is_selected())
                ++result;
        }
        else if (e.get_status() == status)
        {
            midibyte d0, d1;
            e.get_data(d0, d1);                 /* get the two data bytes */
            if (event::is_desired_cc_or_not_cc(status, cc, d0))
            {
                if (e.is_selected())
                    ++result;
            }
        }
    }
    return result;
}

/**
 *  Indicates that at least one matching event is selected.  Acts like
 *  eventlist::count_selected_events(), but stops after finding a selected
 *  note.
 *
 * \return
 *      Returns true if at least one matching event is selected.
 */

bool
eventlist::any_selected_events (midibyte status, midibyte cc) const
{
    bool result = false;
    for (auto & e : m_events)
    {
        if (e.is_tempo())
        {
            if (e.is_selected())
            {
                result = true;
                break;
            }
        }
        else if (e.get_status() == status)
        {
            midibyte d0, d1;
            e.get_data(d0, d1);                 /* get the two data bytes */
            if (event::is_desired_cc_or_not_cc(status, cc, d0))
            {
                if (e.is_selected())
                {
                    result = true;
                    break;
                }
            }
        }
    }
    return result;
}

/**
 *  Selects all events, unconditionally.
 */

void
eventlist::select_all ()
{
    for (auto & e : m_events)
        e.select();
}

/**
 *  Deselects all events, unconditionally.
 */

void
eventlist::unselect_all ()
{
    for (auto & e : m_events)
        e.unselect();
}

/**
 *  Select all events in the given range, and returns the number
 *  selected.  Note that there is also an overloaded version of this
 *  function.
 *
 * \threadsafe
 *
 * \param tick_s
 *      The start time of the selection.
 *
 * \param tick_f
 *      The finish time of the selection.
 *
 * \param status
 *      The desired event in the selection.  Now, as a new feature, tempo
 *      events are also selectable, in addition to events selected by this
 *      parameter.
 *
 * \param cc
 *      The desired control-change in the selection, if the event is a
 *      control-change.
 *
 * \param action
 *      The desired selection action.
 *
 * \return
 *      Returns the number of events selected.
 */

int
eventlist::select_events
(
    midipulse tick_s, midipulse tick_f,
    midibyte status, midibyte cc, select action
)
{
    int result = 0;
    for (auto & er : m_events)
    {
        if (event_in_range(er, status, tick_s, tick_f))
        {
            midibyte d0, d1;
            er.get_data(d0, d1);
            if (er.is_tempo() || event::is_desired_cc_or_not_cc(status, cc, d0))
            {
                if (action == select::selecting)
                {
                    er.select();
                    ++result;
                }
                if (action == select::select_one)
                {
                    er.select();
                    ++result;
                    break;
                }
                if (action == select::selected)
                {
                    if (er.is_selected())
                    {
                        result = 1;
                        break;
                    }
                }
                if (action == select::would_select)
                {
                    result = 1;
                    break;
                }
                if (action == select::toggle)
                {
                    if (er.is_selected())
                        er.unselect();
                    else
                        er.select();
                }
                if (action == select::remove)
                {
                    remove_event(er);           // reset_draw_marker();
                    ++result;
                    break;
                }
                if (action == select::deselect)
                    er.unselect();
            }
        }
    }
    return result;
}

/**
 *  This function selects events in range of tick start, note high, tick end,
 *  and note low.
 *
 *  Compare this function to the convenience function select_all_notes(), which
 *  doesn't use range information.
 *
 *  Note that we have not offloaded this function to eventlist because it
 *  depends on the sequence::select enumeration, and we're too lazy at the
 *  moment to move that enumeration to eventlist.
 *
 * \threadsafe
 *
 * \param tick_s
 *      The start time of the selection.
 *
 * \param note_h
 *      The high note of the selection, inclusive.
 *
 * \param tick_f
 *      The finish time of the selection.
 *
 * \param note_l
 *      The low note of the selection, inclusive.
 *
 * \param action
 *      The action to perform, one of the values of the sequence::select
 *      enumeration.
 *
 * \return
 *      Returns the number of events acted on, or 0 if no desired event was
 *      found.
 */

int
eventlist::select_note_events
(
    midipulse tick_s, int note_h,
    midipulse tick_f, int note_l, select action
)
{
    int result = 0;
    for (auto & er : m_events)
    {
        if (er.get_note() <= note_h && er.get_note() >= note_l)
        {
            midipulse stick = 0, ftick = 0;
            if (er.is_linked())
            {
                event * ev = er.link();
                if (er.is_note_off())
                {
                    stick = ev->timestamp();    /* time of the Note On  */
                    ftick = er.timestamp();     /* time of the Note Off */
                }
                else if (er.is_note_on())
                {
                    ftick = ev->timestamp();    /* time of the Note Off */
                    stick = er.timestamp();     /* time of the Note On  */
                }

#define USE_THIS_CONVOLUTED_CHECK
#if defined USE_THIS_CONVOLUTED_CHECK

                /*
                 * "tand" indicates that the event start is less than the finish
                 * parameter, and the event finish is greater than the start
                 * parameter.
                 *
                 * "tor" is the OR of these two tests, and is needed when the
                 * event start is greater than the finish, which occurs in a
                 * note-off.
                 *
                 * Not sure why so complex; all we need to know is that both the
                 * start and end times are within the desired range. However,
                 * then we cannot click on a note to select it.  Odd!
                 */

                bool tand = (stick <= tick_f) && (ftick >= tick_s);
                bool tor = (stick <= tick_f) || (ftick >= tick_s);
                bool ok = tand || ((stick > ftick) && tor);
#else
                bool ok_start = (stick >= tick_s) && (stick <= tick_f);
                bool ok_finish = (ftick >= tick_s) && (ftick <= tick_f);
                bool ok = ok_start && ok_finish;
#endif
                if (ok)
                {
                    if (action == select::selecting)
                    {
                        er.select();
                        ev->select();
                        ++result;
                    }
                    if (action == select::select_one)
                    {
                        er.select();
                        ev->select();
                        ++result;
                        break;
                    }
                    if (action == select::selected)
                    {
                        if (er.is_selected())
                        {
                            result = 1;
                            break;
                        }
                    }
                    if (action == select::would_select)
                    {
                        result = 1;
                        break;
                    }
                    if (action == select::deselect)
                    {
                        er.unselect();
                        ev->unselect();
                        result = 0;                 /* no break;            */
                    }
                    if (action == select::toggle && er.is_note_on())
                    {
                        if (er.is_selected())       /* don't toggle twice   */
                        {
                            er.unselect();
                            ev->unselect();
                        }
                        else
                        {
                            er.select();
                            ev->select();
                        }
                        ++result;
                    }
                    if (action == select::remove)
                    {
                        remove_event(er);
                        remove_event(*ev);      // reset_draw_marker();
                        ++result;
                        break;
                    }
                }
            }
            else
            {
                /*
                 * Here, the note event is not linked, and so the event is
                 * considered "junk".  We still handle the event itself.
                 * There's no way to fix it except by an expensive
                 * verify_and_link() call!
                 */

#if defined SEQ66_PLATFORM_DEBUG
                errprint("sequence::select_note_events(): unlinked note");
#endif
                stick = ftick = er.timestamp();
                if (stick >= (tick_s - 16) && ftick <= tick_f)  /* why -16? */
                {
                    if (action == select::selecting)
                    {
                        er.select();
                        ++result;
                    }
                    if (action == select::select_one)
                    {
                        er.select();
                        ++result;
                        break;
                    }
                    if (action == select::selected)
                    {
                        if (er.is_selected())
                        {
                            result = 1;
                            break;
                        }
                    }
                    if (action == select::would_select)
                    {
                        result = 1;
                        break;
                    }
                    if (action == select::deselect)
                    {
                        result = 0;
                        er.unselect();
                    }
                    if (action == select::toggle)
                    {
                        ++result;
                        if (er.is_selected())
                            er.unselect();
                        else
                            er.select();
                    }
                    if (action == select::remove)
                    {
                        remove_event(er);           // reset_draw_marker();
                        ++result;
                        break;
                    }
                }
            }
        }
    }
    return result;
}

/**
 *  A convenience function used a couple of times.  Makes if-clauses
 *  easier to read.
 *
 * \param e
 *      Provides the event to be checked.
 *
 * \param status
 *      Provides the event type that must be matched.  However, Set Tempo
 *      events will always be matched.
 *
 * \param tick_s
 *      The lower end of the range of timestamps that the event must fall
 *      within.
 *
 * \param tick_f
 *      The upper end of the range of timestamps that the event must fall
 *      within.
 *
 * \return
 *      Returns true if the event matchs all of the restrictions noted.
 */

bool
eventlist::event_in_range
(
    const event & e, midibyte status,
    midipulse tick_s, midipulse tick_f
) const
{
    bool result = e.is_tempo() || e.get_status() == status;
    if (result)
        result = e.timestamp() >= tick_s && e.timestamp() <= tick_f;

    return result;
}

/**
 *
 */

bool
eventlist::get_selected_events_interval
(
    midipulse & first, midipulse & last
) const
{
    bool result = false;
    midipulse first_ev = midipulse(0x7fffffff);     /* timestamp lower limit */
    midipulse last_ev = midipulse(0x00000000);      /* timestamp upper limit */
    for (auto & er : m_events)
    {
        if (er.is_selected())
        {
            if (er.timestamp() < first_ev)
            {
                first_ev = er.timestamp();
                result = true;
            }
            if (er.timestamp() >= last_ev)
            {
                last_ev = er.timestamp();
                result = true;
            }
        }
    }
    if (result)
    {
        first = first_ev;
        last = last_ev;
    }
    return result;
}

/**
 *  Performs a stretch operation on the selected events.  This should move
 *  a note off event, according to old comments, but it doesn't seem to do
 *  that.  See the grow_selected() function.  Rather, it moves any event in
 *  the selection.
 *
 *  Also, we've moved external calls to push_undo() into this function.
 *  The caller shouldn't have to do that.
 *
 *  Finally, we don't need to mark the selected, only to remove the unmodified
 *  versions later.  Just adjust their timestamps directly.
 *
 * \param delta_tick
 *      Provides the amount of time to stretch the selected notes.
 */

bool
eventlist::stretch_selected (midipulse delta)
{
    midipulse first_ev, last_ev;
    bool result = get_selected_events_interval(first_ev, last_ev);
    if (result)
    {
        midipulse old_len = last_ev - first_ev;
        midipulse new_len = old_len + delta;
        if (new_len > 1 && old_len > 0)
        {
            float ratio = float(new_len) / float(old_len);
            result = false;
            for (auto & er : m_events)
            {
                if (er.is_selected())
                {
                    midipulse t = er.timestamp();
                    midipulse nt = midipulse(ratio * (t - first_ev)) + first_ev;
                    er.set_timestamp(nt);
                    result = true;
                }
            }
            if (result)
                sort();                             /* timestamps altered   */
        }
    }
    return result;
}

/**
 *  The original description was "Moves note off event."  But this also gets
 *  called when simply selecting a second note via a ctrl-left-click, even in
 *  seq66.  And, though it doesn't move Note Off events, it does reconstruct
 *  them.
 *
 *  This function grows/shrinks only Note On events that are marked and linked.
 *  If an event is not linked, this function now ignores the event's timestamp,
 *  rather than risk a segfault on a null pointer.  Compare this function to
 *  the stretch_selected() and move_selected_notes() functions.
 *
 *  This function would strip out non-Notes, but now it at least preserves
 *  them and moves them, to try to preserve their relative position re the
 *  notes.
 *
 *  In any case, we want to mark the original off-event for deletion, otherwise
 *  we get duplicate off events, for example in the "Begin/End" pattern in the
 *  test.midi file.
 *
 *  This function now tries to prevent pathological growth, such as trying to
 *  shrink the notes to zero length or less, or stretch them beyond the length
 *  of the sequence.  Otherwise we get weird and unexpected results.  Also,
 *  we've moved external calls to push_undo() into this function.  The caller
 *  shouldn't have to do that.
 *
 *  A comment on terminology:  The user "selects" notes, while the sequencer
 *  "marks" notes. The first thing this function does is mark all the selected
 *  notes.
 *
 * \threadsafe
 *
 * \param delta
 *      An offset for each linked event's timestamp.
 */

bool
eventlist::grow_selected (midipulse delta, int snap)
{
    bool result = false;
    for (auto & er : m_events)
    {
        if (er.is_selected())
        {
            if (er.is_note())
            {
                if (er.is_note_on() && er.is_linked())
                {
                    event * off = er.link();
                    event e = *off;                 /* original off-event   */
                    midipulse offtime = off->timestamp();
                    midipulse newtime = trim_timestamp(offtime + delta);
                    e.set_timestamp(newtime);       /* new off-time         */
                    result = true;
                }
            }
            else                                    /* non-Note event       */
            {
                midipulse ontime = er.timestamp();
                midipulse newtime = clip_timestamp
                (
                    ontime, ontime + delta, snap
                );
                er.set_timestamp(newtime);          /* adjust time-stamp    */
                result = true;
            }
        }
    }
    if (result)
        sort();                                     /* timestamps altered   */

    return result;
}

/**
 *
 */

bool
eventlist::copy_selected (eventlist & clipbd)
{
    bool result = false;
    for (auto & e : m_events)
    {
        if (e.is_selected())
            clipbd.add(e);                              /* sorts every time */
    }
    if (! clipbd.empty())
    {
        midipulse first_tick = dref(clipbd.begin()).timestamp();
        if (first_tick >= 0)
        {
            for (auto & e : clipbd)                     /* 2019-09-12       */
            {
                midipulse t = e.timestamp();
                if (t >= first_tick)
                {
                    e.set_timestamp(t - first_tick);    /* slide left!      */
                    result = true;
                }
            }
            if (result)
                std::sort(clipbd.m_events.begin(), clipbd.m_events.end());
        }
    }
    return result;
}

/**
 *
 */

bool
eventlist::paste_selected (eventlist & clipbd, midipulse tick, int note)
{
    bool result = false;
    if (! clipbd.empty())
    {
        int highest_note = 0;
        for (auto & e : clipbd)
        {
            midipulse t = e.timestamp();
            e.set_timestamp(t + tick);
            result = true;
            if (e.is_note())                    /* includes Aftertouch      */
            {
                midibyte n = e.get_note();
                if (n > highest_note)
                    highest_note = n;
            }
        }

        int note_delta = note - highest_note;
        for (auto & e : clipbd)
        {
            if (e.is_note())                    /* includes Aftertouch      */
            {
                midibyte n = e.get_note();
                e.set_note(n + note_delta);
                result = true;
            }
        }
        merge(clipbd);                          /* will presort clipboard   */
        verify_and_link();
    }
    return result;
}


/**
 *  A new function to consolidate the adjustment of timestamps in a pattern.
 *  Similar to adjust_timestamp, but it doesn't have an \a isnoteoff
 *  parameter.
 *
 * \param t
 *      Provides the timestamp to be adjusted based on m_length.
 *
 * \return
 *      Returns the adjusted timestamp.
 */

midipulse
eventlist::trim_timestamp (midipulse t) const
{
    if (t >= get_length())
        t -= get_length();

    if (t < 0)                          /* only if midipulse is signed  */
        t += get_length();

    if (t == 0)
        t = get_length() - note_off_margin();

    return t;
}

/**
 *  A new function to consolidate the growth/shrinkage of timestamps in a
 *  pattern.  If the new (off) timestamp is less than the on-time, it is
 *  clipped to the snap value.  If it is greater than the length of the
 *  sequence, then it is clipped to the sequence length.  No wrap-around.
 *
 * \param ontime
 *      Provides the original time, which limits the amount of negative
 *      adjustment that can be done.
 *
 * \param offtime
 *      Provides the timestamp to be adjusted and clipped.
 *
 * \return
 *      Returns the adjusted timestamp.
 */

midipulse
eventlist::clip_timestamp (midipulse ontime, midipulse offtime, int snap) const
{
    if (offtime <= ontime)
        offtime = ontime + snap - note_off_margin();
    else if (offtime >= get_length())
        offtime = get_length() - note_off_margin();

    return offtime;
}

#if defined USE_STAZED_SELECTION_EXTENSIONS

/**
 *  Used with seqevent when selecting Note On or Note Off, this function will
 *  select the opposite linked event.  This is a Stazed selection fix we have
 *  activated unilaterally.
 *
 * \param tick_s
 *      Provides the starting tick.
 *
 * \param tick_f
 *      Provides the ending (finishing) tick.
 *
 * \param status
 *      Provides the desired MIDI event to be selected.
 *
 * \return
 *      Returns the number of linked note notes selected.
 */

int
eventlist::select_linked (midipulse tick_s, midipulse tick_f, midibyte status)
{
    int result = 0;
    for (auto & e : m_events)
    {
        bool ok = e.get_status() == status &&
            e.timestamp() >= tick_s && e.timestamp() <= tick_f;

        if (ok)
        {
            if (e.is_linked())
            {
                ++result;
                if (e.is_selected())
                    e.link()->select();
                else
                    e.link()->unselect();
            }
        }
    }
    return result;
}

#endif  // defined USE_STAZED_SELECTION_EXTENSIONS

/**
 *  Prints a list of the currently-held events.  Useful for debugging.
 */

void
eventlist::print () const
{
    printf("events[%d]:\n", count());
    for (auto & e : m_events)
        e.print();
}

}           // namespace seq66

/*
 * eventlist.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


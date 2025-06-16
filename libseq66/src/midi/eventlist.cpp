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
 * \file          eventlist.cpp
 *
 *  This module declares/defines a class for handling MIDI events in a list
 *  container.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-09-19
 * \updates       2025-06-14
 * \license       GNU GPLv2 or above
 *
 *  This container now can indicate if certain Meta events (time-signaure or
 *  tempo) have been added to the container.
 */

#include <algorithm>                    /* std::sort(), std::merge()        */

#include "cfg/settings.hpp"             /* seq66::usr()                     */
#include "midi/eventlist.hpp"           /* seq66::eventlist                 */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Principal constructor.
 */

eventlist::eventlist () :
    m_events                (),
    m_match_iterating       (false),
    m_match_iterator        (m_events.end()),
#if defined SEQ66_USE_ACTION_IN_PROGRESS_FLAG
    m_action_in_progress    (false),                    /* atomic boolean   */
#endif
    m_length                (0),
    m_note_off_margin       (3),
    m_zero_len_correction   (16),
    m_is_modified           (false),
    m_has_tempo             (false),
    m_has_time_signature    (false),
    m_has_key_signature     (false),
    m_link_wraparound       (usr().pattern_wraparound())
{
    // No code needed
}

/**
 *  We have to now define this copy constructor because the atomic copy
 *  constructor is deleted, making the compiler-generated copy constructor
 *  ill-formed.
 */

eventlist::eventlist (const eventlist & rhs) :
    m_events                (rhs.m_events),
    m_match_iterating       (false),
    m_match_iterator        (m_events.end()),
#if defined SEQ66_USE_ACTION_IN_PROGRESS_FLAG
    m_action_in_progress    (false),                    /* atomic boolean   */
#endif
    m_length                (rhs.m_length),
    m_note_off_margin       (rhs.m_note_off_margin),
    m_zero_len_correction   (rhs.m_zero_len_correction),
    m_is_modified           (rhs.m_is_modified),
    m_has_tempo             (rhs.m_has_tempo),
    m_has_time_signature    (rhs.m_has_time_signature),
    m_has_key_signature     (rhs.m_has_key_signature),
    m_link_wraparound       (rhs.m_link_wraparound)
{
    // no code
}

eventlist &
eventlist::operator = (const eventlist & rhs)
{
    if (this != &rhs)
    {
        m_events                = rhs.m_events;
        m_match_iterating       = rhs.m_match_iterating;    /* ok? */
        m_match_iterator        = rhs.m_match_iterator;     /* ok? */
#if defined SEQ66_USE_ACTION_IN_PROGRESS_FLAG
        m_action_in_progress    = false;                /* atomic boolean   */
#endif
        m_length                = rhs.m_length;
        m_note_off_margin       = rhs.m_note_off_margin;
        m_zero_len_correction   = rhs.m_zero_len_correction;
        m_is_modified           = rhs.m_is_modified;
        m_has_tempo             = rhs.m_has_tempo;
        m_has_time_signature    = rhs.m_has_time_signature;
        m_has_key_signature     = rhs.m_has_key_signature;
        m_link_wraparound       = rhs.m_link_wraparound;
    }
    return *this;
}

/**
 *  Provides the minimum and maximux timestamps of the events, in MIDI pulses.
 *  These functions get the iterator for the first or last element and returns
 *  its value. Obviously the events must already be sorted.
 *
 * \return
 *      Returns the timestamp of the first or last event in the container.
 */

midipulse
eventlist::get_min_timestamp () const
{
    midipulse result = 0;
    if (count() > 0)
    {
        auto lci = m_events.begin();                    /* get 1st element  */
        result = lci->timestamp();                      /* get length value */
    }
    return result;
}

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
    m_events.push_back(e);                      /* std::vector operation    */
    m_is_modified = true;
    if (e.is_tempo())
        m_has_tempo = true;

    if (e.is_time_signature())
        m_has_time_signature = true;

    if (e.is_key_signature())
        m_has_key_signature = true;

    return true;
}

/**
 *  An internal function to add events to a temporary list.  Used in
 *  quantization and tightening operations.
 */

bool
eventlist::add (event::buffer & evlist, const event & e)
{
    evlist.push_back(e);                        /* std::vector operation    */
    std::sort(evlist.begin(), evlist.end());
    return true;
}

/**
 *  Adds an event to the internal event list in a sorted manner.  Note
 *  that, for speed, it is better to call append() for each event, and
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

bool
eventlist::add (const event & e)
{
    bool result = append(e);
    if (result)
        sort();                         /* by time-stamp and "rank" */

    return result;
}

/**
 *  Sorts the event list.  For the vector, equivalent elements are not
 *  guaranteed to keep their original relative order [see
 *  std::stable_sort(), which we could try at some point].
 *
 *  This method is probably flawed.
 */

void
eventlist::sort ()
{
#if defined SEQ66_USE_ACTION_IN_PROGRESS_FLAG
    m_action_in_progress = true;
    std::sort(m_events.begin(), m_events.end());
    m_action_in_progress = false;
#else
    std::sort(m_events.begin(), m_events.end());
#endif
}

/**
 *  An internal function to merge events from a temporary list.  Used in
 *  quantization and tightening operations.
 */

void
eventlist::merge (const event::buffer & evlist)
{
    std::size_t totalsize = m_events.size() + evlist.size();
    m_events.reserve(totalsize);
    m_events.insert(m_events.end(), evlist.begin(), evlist.end());
    sort();
}

/**
 *  Provides a merge operation for the event container managed by this
 *  eventlist.  The event::buffer container is a vector.  We don't have to
 *  presort the container in this case.
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
 *      If true (the default), then the events are presorted.  This is a
 *      requirement for merging an std::list or std::vector, but is a no-op
 *      for the std::multimap implementation (which no longer exists).
 */

bool
eventlist::merge (const eventlist & el, bool presort)
{
    if (presort)                            /* not really necessary here    */
    {
        eventlist & el_nc = const_cast<eventlist &>(el);
        el_nc.sort();
    }
    std::size_t totalsize = m_events.size() + el.m_events.size();
    m_events.reserve(totalsize);
    m_events.insert(m_events.end(), el.m_events.begin(), el.m_events.end());

    /*
     * Done via verify_and_link(): sort();
     */

    bool result = m_events.size() == totalsize;
    if (result)
        verify_and_link();

    return result;
}

/**
 *  Links a new event.  This function checks for a note on, then looks for
 *  its note off.  This function is provided in the eventlist because it
 *  does not depend on any external data.  Also note that any desired
 *  thread-safety must be provided by the caller.
 *
 * Link wraparound:
 *
 *      This is a Stazed addition; not in seq24.  Not sure that we need it, it
 *      is now optional.  It can handle cases where the Note Off comes before
 *      the Note On (i.e.  the note wraps around to the beginning of the
 *      pattern).
 *
 *      Without it, we can get unlinked notes when the key press lasts too
 *      long (which can be removed by the 'u' keystroke in the piano roll).
 *      With it, the note extends to the end of the pattern and then wraps
 *      around to the beginning.
 *
 *      For recording, to avoid issues, make the pattern length one measure
 *      longer than desired while recording.
 *
 *      We could add a feature to truncate the note.  Think!
 *
 * \param wrap
 *      Optionally (the default is false) wrap when relinking.  Can be used to
 *      override usr().pattern_wraparound().  Defaults to false.
 *
 * \return
 *      Returns true if any events were linked.
 */

bool
eventlist::link_new (bool wrap)
{
    bool result = false;
    for (auto eon = m_events.begin(); eon != m_events.end(); ++eon)
    {
        if (eon->on_linkable())                     /* note-on, not linked  */
        {
            bool endfound = false;                  /* end-of-note flag     */
            auto eoff = eon;                        /* point to note on     */
            ++eoff;                                 /* get next element     */
            while (eoff != m_events.end())
            {
                endfound = link_notes(eon, eoff);   /* calls off_linkable() */
                if (endfound)
                {
                    result = true;
                    break;
                }
                ++eoff;
            }
            if (! endfound)
            {
                eoff = m_events.begin();
                while (eoff != eon)
                {
                    bool wrapped = eoff->timestamp() < eon->timestamp();
                    if (link_notes(eon, eoff))
                    {
                        result = true;
                        if (wrapped && ! wrap)
                            eoff->set_timestamp(get_length() - 1);
                    }
                    ++eoff;
                }
            }
        }
    }
    return result;
}

#if defined SEQ66_LINK_NEWEST_NOTE_ON_RECORD        /* undefined            */

/**
 *  This function links only the latest note, and should be called
 *  only for Note Off events. It requires the following:
 *
 *  -   The note is the last one appended.
 *  -   It is a Note Off.
 *  -   All previous notes are already linked.
 *
 *  The caller assumes the above responsibilities. Additional checks are made:
 *
 *  -   The Note On is not already linked.
 *  -   The Note Off is on the same channel as the Note On.
 *  -   The note value (pitch) is the same for both notes.
 *  -   The time-stamp of the Note Off is at are after the Note On.
 *      If right at it (due to quantization), then a zero-length
 *      correction is made for the Note Off.
 *
 *  Note that we cannot (easily) use a reverse_iterator because
 *  the event object stores links as normal iterators. It's a real
 *  issue to convert between reverse and forward iterators, or pointers
 *  or references. Luckily, the container (std::vector) has bidirectional
 *  iterators.
 *
 *  We could change to using the std::bidirectional_iterator once we
 *  graduate to C++20, which won't be for some time.
 */

void
eventlist::link_new_note ()
{
    if (count() > 1)
    {
        bool done = false;
        for (auto off = m_events.end(); off != m_events.begin(); /* none */ )
        {
            --off;                                  /* can't use end() val  */
            if (off->off_linkable())                /* note-off, not linked */
            {
                auto on = off;
                while (! done)
                {
                    --on;
                    if (on == m_events.begin())
                    {
                        done = true;
                        break;
                    }

                    bool ok = on->on_linkable();
                    if (ok)
                        ok = on->channel() == off->channel();

                    if (ok)
                        ok = on->get_note() == off->get_note();

                    if (ok)
                    {
                        ok = on->timestamp() <= off->timestamp();
                        if (on->timestamp() == off->timestamp())
                        {
                            long ts = on->timestamp();
                            ts += m_zero_len_correction;
                            off->set_timestamp(ts);
#if defined SEQ66_PLATFORM_DEBUG_TMI
                            printf ("Zero-length note @%ld fixed\n", ts);
#endif
                        }
                    }
                    if (ok)
                    {
                        (void) link_notes(on, off);
                        done = true;
                    }
                }
            }
            if (done)
                break;
        }
    }
}

#endif  // defined SEQ66_LINK_NEWEST_NOTE_ON_RECORD

/**
 *  If we're in legacy merge mode for a loop, the Note Off is actually earlier
 *  than the Note On.  And in replace mode, the Note On is cleared, leaving us
 *  with a dangling Note Off event.
 *
 *  We should consider, in both modes, automatically adding the Note Off at
 *  the end of the loop and ignoring the next note off on the same note from
 *  the keyboard.
 *
 *  Careful!
 *
 * \param eon
 *      Provides an event already known to satisfy the event::on_linkable()
 *      function.
 *
 * \param eoff
 *      Provides an event that will be checked according to
 *      event::off_linkable().
 *
 * \return
 *      Returns true if the notes were linked.
 */

bool
eventlist::link_notes (event::iterator eon, event::iterator eoff)
{
    bool result = eon->off_linkable(eoff);
    if (result)
    {
        eon->link(eoff);
        eoff->link(eon);
        if (eon->timestamp() == eoff->timestamp())
        {
            long ts = eon->timestamp();
            ts += m_zero_len_correction;
            eoff->set_timestamp(ts);
#if defined SEQ66_PLATFORM_DEBUG_TMI
            printf ("Zero-length note @%ld fixed\n", ts);
#endif
        }
    }
    return result;
}

/**
 *  This function verifies state: all note-ons have an off, and it links
 *  note-offs with their note-ons.
 *
 * No longer correct:
 *
 *      It also links the tempos in a separate pass (it makes the logic easier
 *      and the amount of time should be unnoticeable to the user).
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
 *      caller supplies sequence::get_length(). Can be set to 0 ignore
 *      the length, as in expanded step-edit note entry. See the function
 *      sequence::verify_and_link().
 *
 * \param wrap
 *      Optionally (the default is false) wrap when relinking.  Can be used to
 *      override usr().pattern_wraparound().
 *
 * \return
 *      Returns true if any events were linked by linked_new().
 */

bool
eventlist::verify_and_link (midipulse slength, bool wrap)
{
    clear_links();                          /* unlink, no unmark all events */
    sort();                                 /* important, but be careful... */

    bool wrap_em = m_link_wraparound || wrap;       /* a Stazed extension   */
    bool result = link_new(wrap_em);
    if (slength > 0)
    {
        if (mark_out_of_range(slength))
            (void) remove_marked();         /* prune out-of-range events    */
    }

    /*
     * Not sure we want to draw lines for tempos yet.  Also, linking tempos
     * makes them double-selectable in the event editor.
     *
     * link_tempos();
     */

    return result;
}

/**
 *  Provides a wrapper for clear().  Sets the modified-flag.
 */

void
eventlist::clear ()
{
    if (! m_events.empty())
    {
#if defined SEQ66_USE_ACTION_IN_PROGRESS_FLAG
        m_action_in_progress = true;          /* might not help */
        m_events.clear();
        m_action_in_progress = false;
#else
        m_events.clear();
#endif
        m_is_modified = true;
    }
}

/**
 *  Clears all event links and unmarks them all. We get a segfault here
 *  pretty regulary when recording is enabled and the pattern's event list
 *  is showing.
 */

bool
eventlist::clear_links ()
{
    bool result = false;
    for (auto & e : m_events)
    {
        if (e.is_linked())
        {
            result = true;
            e.clear_link();                 /* does unmark() and unlink()   */
        }
    }
    return result;
}

int
eventlist::playable_count () const
{
    int result = 0;
    for (const auto & e : m_events)
    {
        if (e.is_playable())
            ++result;
    }
    return result;
}

bool
eventlist::is_playable () const
{
    bool result = false;
    for (const auto & e : m_events)
    {
        if (e.is_playable())
        {
            result = true;
            break;
        }
    }
    return result;
}

int
eventlist::note_count () const
{
    int result = 0;
    for (const auto & e : m_events)
    {
        if (e.is_note_on())
            ++result;
    }
    return result;
}

/**
 *  Look at note events within the snap interval. Return the first time-stamp
 *  and the first (or average within the snap interval) note value. This
 *  function is used for centering the seqroll on visible notes.
 */

bool
eventlist::first_notes (midipulse & ts, int & n, midipulse snap) const
{
    bool result = false;
    bool doaverage = snap > 0;
    if (doaverage)
    {
        midipulse ts_first = (-1);
        int note_avg = 0;
        int note_count = 0;
        for (const auto & e : m_events)
        {
            if (e.is_note_on())
            {
                result = true;

                midipulse ts_temp = e.timestamp();
                if (ts_first == (-1))
                    ts_first = ts_temp;

                int note = int(e.get_note());
                if (ts_temp < (ts_first + snap))
                {
                    note_avg += note;
                    ++note_count;
                }
                else
                    break;
            }
        }
        if (result)
        {
            ts = ts_first;
            n = note_avg / note_count;
        }
    }
    else
    {
        for (const auto & e : m_events)
        {
            if (e.is_note_on())
            {
                ts = e.timestamp();
                n = int(e.get_note());
                result = true;
                break;
            }
        }
    }
    return result;
}

/**
 *  Tries to fix the selected notes that started near the end of the pattern
 *  and wrapped around to the beginning, by moving the note.
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
        if (e.is_selected_note_on() && e.is_linked())
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
        (void) verify_and_link();                   /* sorts as well        */

    return result;
}

/**
 *  Removes unlinked notes.  We must verify_and_link() to get the pattern roll
 *  to show the new note-list.
 */

bool
eventlist::remove_unlinked_notes ()
{
    bool result = false;
    for (auto i = m_events.begin(); i != m_events.end(); /*++i*/)
    {
        if (i->is_note_unlinked())
        {
            auto t = remove(i);
            i = t;
            result = true;
        }
        else
            ++i;
    }
    if (result)
        (void) verify_and_link();                   /* sorts as well        */

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
 * \param astatus
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
 *      tested for 0.  The caller should do it.
 */

bool
eventlist::quantize_events
(
    midibyte astatus, midibyte cc,
    int snap, int divide
)
{
    bool result = false;
    midipulse len = get_length();
    bool tight = divide == 2;
    for (auto & er : m_events)
    {
        if (er.is_selected())
        {
            midibyte d0, d1;
            er.get_data(d0, d1);
            bool match = er.match_status(astatus);
            bool canselect = false;
            if (er.is_marked())                 /* ignore marked events     */
            {
                er.unmark();
                continue;                       /* it was a linked note     */
            }
            if (astatus == EVENT_CONTROL_CHANGE)
                canselect = match && d0 == cc;  /* correct status and cc    */
            else
                canselect = match;              /* correct status, any cc   */

            if (canselect)
            {
                result = tight ?
                    er.tighten(snap, len) : er.quantize(snap, len) ;

                if (er.is_note_on_linked())
                {
                    /*
                     * In some cases, the linked Note Off, when quantized,
                     * will end up next to the Note On.  How to fix? If they
                     * are closer than half the snap, add the snap.
                     */

                    event::iterator f = er.link();
                    if (tight)
                        f->tighten(snap, len);
                    else
                        f->quantize(snap, len);

                    midipulse ts1 = er.timestamp();
                    midipulse ts2 = f->timestamp();
                    if (ts2 >= ts1)
                    {
                        if (ts2 - ts1 < (snap / 2))
                        {
                            ts1 += snap / 2;
                            f->set_timestamp(ts1);
                        }
                    }
                    f->mark();                  /* mark linked note, ignore */
                }
            }
        }
    }
    if (result)
        (void) verify_and_link();               /* sorts the events again!! */

    return result;
}

/**
 *  Quantizes events, unconditionally.  No adjustment for wrapped notes
 *  is made.
 *
 * \param snap
 *      Provides the maximum amount to move the events.  Actually, events are
 *      moved to the previous or next snap_tick value depend on whether they
 *      are halfway to the next one or not.
 *
 * \param divide
 *      An indicator of the amount of quantization.  The values are either
 *      1 ("quantize") or 2 ("tighten"). The default value is 1, which makes
 *      the snap parameter the quantization value.
 *
 * \param all
 *      If true (the default is false), then all events, not just selected
 *      ones, are quantized.
 *
 * \return
 *      Returns true if events were quantized.
 */

bool
eventlist::quantize_events (int snap, int divide, bool all)
{
    bool result = false;
    bool tight = divide == 2;
    bool found_note = false;
    midipulse len = get_length();
    for (auto & er : m_events)
    {
        if (all || er.is_selected())
        {
            bool ok = tight ? er.tighten(snap, len) : er.quantize(snap, len) ;
            if (ok)
                result = true;

            if (er.is_note())
                found_note = true;
        }
    }
    if (result && found_note)
        (void) verify_and_link();                   /* sorts them again!!!  */

    return result;
}

/**
 *  Quantize/tighten all Note events, including Aftertouch.
 *
 * \param snap
 *      The value to which to snap the events.
 *
 * \param divide
 *      Divides the snap, e.g. in order to "tighten" rather than fully
 *      quantize. Use set to 1 (the default) or 2.
 *
 * \param all
 *      If false (the default), then only selected notes are acted on.
 *      Otherwise, they all are.
 *
 * \return
 *      Returns true if notes were quantized.
 */

bool
eventlist::quantize_notes (int snap, int divide, bool all)
{
    bool result = false;
    midipulse len = get_length();
    bool tight = divide == 2;
    for (auto & er : m_events)
    {
        if (all || er.is_selected_note())
        {
            if (er.is_marked())                 /* ignore marked events     */
            {
                er.unmark();
                continue;
            }
            result = tight ? er.tighten(snap, len) : er.quantize(snap, len) ;
            if (er.is_note_on_linked())
            {
                event::iterator f = er.link();
                if (tight)
                    f->tighten(snap, len);
                else
                    f->quantize(snap, len);

                midipulse ts1 = er.timestamp();
                midipulse ts2 = f->timestamp();
                if (ts2 >= ts1)
                {
                    if (ts2 - ts1 < (snap / 2))
                    {
                        ts1 += snap / 2;
                        f->set_timestamp(ts1);
                    }
                }
                f->mark();                      /* mark linked for later    */
            }
        }
    }
    if (result)
        (void) verify_and_link();               /* sorts them again!!!      */

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
eventlist::adjust_timestamp (event & er, midipulse delta_tick)
{
    static const bool s_allow_wrap = true;  /* wrap: note on after note-off */
    midipulse result = er.timestamp() + delta_tick;
    midipulse len = get_length();
    if (result > len)
        result -= len;

    if (result < 0)                         /* only if midipulse is signed  */
    {
        if (s_allow_wrap)
            result += len;
        else
            result = 0;
    }
    if (er.is_note_off())
    {
        if (result == 0)
        {
            if (s_allow_wrap)
                result = len - note_off_margin();
            else
                result = note_off_margin();
        }
    }
    else                                    /* if (wrap)                    */
    {
        if (result == len)
        {
            if (s_allow_wrap)
                result = 0;
        }
    }
    return result;
}

/**
 *  Removes and adds selected notes in position.  Also currently moves any
 *  other events in the range of the selection.
 *
 *  Another thing this function does is wrap-around when movement occurs.
 *  Any events (except Note Off) that will start just after the END of the
 *  pattern will be wrapped around to the beginning of the pattern.
 *
 *  After this function, we also have to call verify_and_link(), which sorts
 *  and relinks the notes from scratch.
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
        if (er.is_selected_note())                  /* moveable event?      */
        {
            int newnote = er.get_note() + delta_note;
            if (newnote >= 0 && newnote < c_notes_count)
            {
                midipulse newts = adjust_timestamp(er, delta_tick);
                if (er.is_note())                   /* Note On or Note Off  */
                    er.set_note(midibyte(newnote));

                er.set_timestamp(newts);
                result = true;
            }
        }
    }
    if (result)
        (void) verify_and_link();                   /* sort and relink      */

    return result;
}

/**
 *  Used only in qstriggereditor.
 */

bool
eventlist::move_selected_events (midipulse delta_tick)
{
    bool result = false;
    for (auto & er : m_events)
    {
        if (er.is_selected() && ! er.is_note())
        {
            midipulse newts = adjust_timestamp(er, delta_tick);
            er.set_timestamp(newts);
            result = true;
        }
    }
    return result;
}

/**
 *  Makes the first event start at time 0. Might also change the length of
 *  the pattern. Hmmm???
 *
 * \param relink
 *      If true (the default is false), the events are sorted and relinked.
 *
 * \return
 *      Returns true all timestamps were adjusted. Otherwise, false is
 *      returned, which means the original events should be restored.
 */

bool
eventlist::align_left (bool relink)
{
    bool result = ! empty();
    if (result)
    {
        const auto startev = m_events.begin();
        midipulse shift = startev->timestamp();
        result = shift > 0;
        if (result)
        {
            for (auto & ev : m_events)
            {
                midipulse newstamp = ev.timestamp() - shift;
                if (newstamp >= 0)
                {
                    ev.set_timestamp(newstamp);
                }
                else
                {
                    result = false;
                    break;
                }
            }
            if (result && relink)
            {
                sort();
                result = verify_and_link();
            }
        }
    }
    return result;
}

/**
 *  Makes the last event end at the end of the pattern.
 *
 * \param relink
 *      If true (the default is false), the events are sorted and relinked.
 *
 * \return
 *      Returns true all timestamps were adjusted. Otherwise, false is
 *      returned, which means the original events should be restored.
 */

bool
eventlist::align_right (bool relink)
{
    bool result = ! empty();
    if (result)
    {
        const auto endev = m_events.rbegin();
        midipulse endts = get_length();
        midipulse shift = endts - endev->timestamp() - 1;
        result = shift > 0;
        if (result)
        {
            for (auto & ev : m_events)
            {
                midipulse newstamp = ev.timestamp() + shift;
                if (newstamp < endts)
                {
                    ev.set_timestamp(newstamp);
                }
                else
                {
                    result = false;
                    break;
                }
            }
            if (result && relink)
            {
                sort();
                result = verify_and_link();
            }
        }
    }
    return result;
}

/**
 *  Helper function for scaling note-off events properly..
 */

void
eventlist::scale_note_off (event & noteoff, double factor)
{
    midipulse stamp = noteoff.timestamp();
    stamp += note_off_margin();                     /* remove the margin    */
    stamp *= factor;                                /* scale the note off   */
    stamp -= note_off_margin();                     /* put back the margin  */
    noteoff.set_timestamp(stamp);
}

/**
 *  Scales the time of all event by the given factor.
 *
 *  -   If the factor <= 1.0:
 *      -#  Scale all events.
 *      -#  Leave the length of the pattern (in measures) the same; the
 *          user can manually reduce the length in the pattern editor, if
 *          desired.
 *  -   if the factor is > 1.0:
 *      -#  Scale all events.
 *      -#  Find the new maximum timestamp.
 *      -#  Increase it to the next full measure, then set the length.
 *      -#  Return a non-zero so that the sequence (the caller) can update
 *          the measures count.
 *
 * \param factor
 *      The multiplier for each timestamp.
 *
 * \param savenotelength
 *      If true (the default is false), then we have to keep track of note
 *      events (on, off, but not aftertouch), to preserve the length of each
 *      note.
 *
 * \param relink
 *      If true (the default is false), then sort, verify, and link.
 *      Not sure if this is useful at this point.
 */

midipulse
eventlist::apply_time_factor (double factor, bool savenotelength, bool relink)
{
    midipulse result = 0;
    bool ok = ! empty() && factor > 0.01;
    if (ok)
    {
        for (auto & ev : m_events)
        {
            midipulse stamp = ev.timestamp();
            bool linked = ev.is_linked();           /* do note on and off   */
            if (ev.is_note_on())
            {
                midipulse newstamp = midipulse(stamp * factor);
                if (linked)
                {
                    midipulse offstamp = ev.link()->timestamp();
                    if (savenotelength)
                    {
                        midipulse len = offstamp - stamp;
                        ev.link()->set_timestamp(newstamp + len);
                    }
                    else
                    {
                        offstamp = midipulse(offstamp * factor);
                        scale_note_off(*ev.link(), factor);
                    }
                }
                ev.set_timestamp(newstamp);
            }
            else if (ev.is_note_off())
            {
                if (! ev.is_linked())               /* correction needed    */
                    scale_note_off(ev, factor);
            }
            else
            {
                midipulse newstamp = midipulse(stamp * factor);
                ev.set_timestamp(newstamp);
            }
        }
        if (relink)
        {
            sort();
            verify_and_link();
        }
        result = get_max_timestamp();
    }
    return result;
}

/**
 *  This function reverses the events in a sequence.  Note events are treated
 *  specially:
 *
 *      -#  The Note Off timestamp (reversed) has to be used as the new Note On
 *          timestamp.
 *      -#  Only the Note On gets that new-timestamp at first.
 *      -#  The Note Off is placed at the original duration past the new
 *          Note On time.
 */

bool
eventlist::reverse_events (bool inplace, bool relink)
{
    bool result = ! empty();
    if (result)
    {
        midipulse offset = inplace ? get_min_timestamp() : 0;
        midipulse ending = inplace ? get_max_timestamp() : get_length() - 1 ;
        for (auto & ev : m_events)
        {
            midipulse stamp = ev.timestamp();
            midipulse newstamp = ending - stamp + offset;
            if (ev.is_note_on())
            {
                bool linked = ev.is_linked();   /* do note on and off   */
                if (linked)
                {
                    midipulse offstamp = ev.link()->timestamp();
                    midipulse duration = offstamp - stamp + 1;
                    newstamp = ending - offstamp + offset;
                    ev.set_timestamp(newstamp);
                    ev.link()->set_timestamp(newstamp + duration);
                }
                else
                    ev.set_timestamp(newstamp);
            }
            else if (ev.is_note_off())
            {
                if (! ev.is_linked())           /* correction needed    */
                    ev.set_timestamp(newstamp);
            }
            else
                ev.set_timestamp(newstamp);
        }
        if (relink)
        {
            sort();
            verify_and_link();
        }
    }
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
 *
 *  Note that we do not need to call verify_and_link() here, since we are not
 *  altering the timestamps or the note values.
 *
 * \param astatus
 *      The kind of event to be randomized.
 *
 * \param range
 *      The amount of randomization.  A positive non-zero value is enforced.
 *
 * \param all
 *      If true, randomize all events regardless of status value. The default
 *      is false.
 *
 * \return
 *      Returns true if some randomization occurred.
 */

bool
eventlist::randomize (midibyte astatus, int range, bool all)
{
    bool result = false;
    if (range > 0)
    {
        for (auto & e : m_events)
        {
            if (all || e.is_selected_status(astatus))
            {
                if (e.randomize(range))
                    result = true;
            }
        }
    }
    return result;
}

/**
 *  This function randomizes a Note On or Note Off message, and more
 *  thoroughly than randomize().  We want to be able to "jitter" the
 *  velocity (data byte d[1]) of the note.  The note pitch (d[0]) is not
 *  altered.
 *
 * \param range
 *      Provides the amount of velocity randomization.
 *
 * \param all
 *      If true (the default is false), handle all notes, not just the
 *      selected notes.
 *
 * \return
 *      Returns true if any event got altered.
 */

bool
eventlist::randomize_note_velocities (int range, bool all)
{
    bool result = range > 0;
    if (result)
    {
        for (auto & e : m_events)
        {
            if (all || e.is_selected_note())        /* randomizable event?  */
            {
                if (! e.is_note_off_recorded())     /* don't ruin fake Off  */
                {
                    if (e.randomize(range))
                        result = true;
                }
            }
        }

        /*
         * ca 2025-06-13. We're not changing the order or timing
         * of notes, why relink?
         */
#if 0
        if (result)
            (void) verify_and_link();               /* sort & relink notes  */
#endif
    }
    return result;
}

/**
 *  This function randomizes the pitch of a Note On/Note Off message pair.
 *  We want it to fall within the given scale (which might be
 *  scale::chromatic, which includes all twelve notes.)
 *
 *  This is a little tricky. If the randomized pitch isn't part of the
 *  specified scale we need to go up or down a pitch value until it is.
 *
 * \param range
 *      Provides the amount of velocity randomization.
 *
 * \param s
 *      Provides the scale to adhere to. If equal to scale::chromatic
 *      (also known as scale::off), no adjustment is needed.
 *
 * \param all
 *      If true (the default is false), handle all notes, not just the
 *      selected notes.
 *
 * \return
 *      Returns true if any event got altered.
 */

bool
eventlist::randomize_note_pitches
(
    int range, scales s, keys keyofpattern, bool all
)
{
    bool result = range > 0;
    if (result)
    {
#if defined SEQ66_PLATFORM_DEBUG_TMI
        printf
        (
            "Key of %s, %s scale\n",
            musical_key_name(keyofpattern).c_str(),
            musical_scale_name(s).c_str()
        );
#endif
        result = false;
        (void) verify_and_link();                       /* play safe & sort */
        for (auto & e : m_events)
        {
            bool ok = all ? e.is_note() : e.is_selected_note() ;
            if (ok)                                     /* randomizable?    */
            {
#if defined SEQ66_USE_UNIFORM_INT_DISTRIBUTION
                int delta = seq66::randomize_uniformly(range);
#else
                int delta = seq66::randomize(range);
#endif
                int p = int(e.get_note());
                if (s == scales::off)
                {
                    p += delta;
                }
                else
                {
                    p += delta;
                    for (int offset = 0; ; ++offset)    /* find legal note  */
                    {
                        int testp = p + offset;
                        if (scales_policy(s, keyofpattern, testp))
                        {
                            p = testp;
                            break;
                        }
                        else
                        {
                            testp = p - offset;
                            if (scales_policy(s, keyofpattern, testp))
                            {
                                p = testp;
                                break;
                            }
                        }
                    }
                }
                if (e.is_note_on())
                {
                    result = true;
                    e.set_note(midibyte(p));
                    if (e.is_linked())
                    {
                        e.link()->set_note(midibyte(p));
                    }
                    else
                    {
#if defined SEQ66_PLATFORM_DEBUG
                        printf("Orphaned Note On in randomizing pitch\n");
#endif
                    }
                }
                else if (e.is_note_off())
                {
                    if (! e.is_linked())
                    {
                        result = true;
                        e.set_note(midibyte(p));
#if defined SEQ66_PLATFORM_DEBUG
                        printf("Orphaned Note Off in randomizing pitch\n");
#endif
                    }
                }
            }
            else
                continue;
        }
        (void) verify_and_link();                       /* play safe & sort */
    }
    return result;
}

#if defined SEQ66_USE_JITTER_EVENTS

/**
 *  This function jitters the timestamps of all events. If note events were
 *  jittered, then we verify-and-link.
 *
 *  Since we jitter the timestamps, we have to call verify_and_link()
 *  afterward.
 *
 * \param snap
 *      Provides the current snap value, which is used to avoid gross
 *      jittering.
 *
 * \param jitr
 *      Provides the amount of time jitter in ticks.
 *
 * \return
 *      Returns true if some jittering was actually done.
 */

bool
eventlist::jitter_events (int snap, int jitr)
{
    bool result = false;
    if (jitr > 0)
    {
        bool note_changed = false;
        for (auto & e : m_events)
        {
            if (e.is_marked())                  /* ignore marked events     */
            {
                e.unmark();
                continue;                       /* it was a linked note     */
            }
            if (e.jitter(snap, jitr, get_length()))
            {
                result = true;
                if (e.is_note())
                    note_changed = true;

                if (e.is_note_on_linked())
                {
                    /*
                     * In some cases, the linked Note Off, when quantized,
                     * will end up next to the Note On.  How to fix? If they
                     * are closer than half the snap, add the snap.
                     *
                     * Hmmmm, how about the zero-length correction???
                     */

                    event::iterator f = e.link();
                    f->jitter(snap, jitr, get_length());

                    midipulse ts1 = e.timestamp();
                    midipulse ts2 = f->timestamp();
                    if (ts2 >= ts1)
                    {
                        if (ts2 - ts1 < (snap / 2))
                        {
                            ts1 += snap / 2;
                            f->set_timestamp(ts1);
                        }
                    }
                    f->mark();                  /* mark link note for later */
                }
            }
        }
        if (note_changed)
            verify_and_link();                  /* sort and relink notes    */
    }
    return result;
}

#endif  // defined SEQ66_USE_JITTER_EVENTS

/**
 *  This function jitters the timestamps of all note events.  If any
 *  were jittered, then we verify-and-link.
 *
 *  Since we jitter the timestamps, we have to call verify_and_link()
 *  afterward.
 *
 * \param snap
 *      Provides the current snap value, which is used to avoid gross
 *      jittering.
 *
 * \param jitr
 *      Provides the amount of time jitter in ticks.
 *
 * \param all
 *      If true (the default is false), all events are jittered,
 *      not just the selected events.
 *
 * \return
 *      Returns true if some jittering was actually done.
 */

bool
eventlist::jitter_notes (int snap, int jitr, bool all)
{
    bool result = false;
    if (jitr > 0)
    {
        for (auto & e : m_events)
        {
            if (all || e.is_selected_note())
            {
                if (e.jitter(snap, jitr, get_length()))
                    result = true;
            }
        }
        if (result)
            verify_and_link();                      /* sort and relink      */
    }
    return result;
}

#if defined SEQ66_USE_FILL_TIME_SIG_AND_TEMPO

/**
 *  Scans the event-list for any tempo or time_signature events.
 *  The user may have deleted them and is depending on a setting made in the
 *  user-interface.  So we must set/unset the flags before saving.  This check
 *  was added to fix issue #141.
 */

void
eventlist::scan_meta_events ()
{
    m_has_tempo = false;
    m_has_time_signature = false;
    m_has_key_signature = false;
    for (auto & e : m_events)
    {
        if (e.is_tempo())
            m_has_tempo = true;
        else if (e.is_time_signature())
            m_has_time_signature = true;
        else if (e.is_key_signature())
            m_has_key_signature = true;
    }
}

#endif  // SEQ66_USE_FILL_TIME_SIG_AND_TEMPO

#if defined SEQ66_LINK_TEMPOS

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

bool
eventlist::link_tempos ()
{
    bool result = false;
    clear_tempo_links();
    for (auto t = m_events.begin(); t != m_events.end(); ++t)
    {
        if (t->is_tempo())
        {
            auto t2 = t;                    /* next possible Set Tempo...   */
            ++t2;                           /* ...starting here             */
            while (t2 != m_events.end())
            {
                if (t2->is_tempo())
                {
                    result = true;
                    t->link(t2);
                    break;                  /* tempos link only one way     */
                }
                ++t2;
            }
        }
    }
    return result;
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
            e.unlink();
    }
}

#endif  // defined SEQ66_LINK_TEMPOS

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

#if defined SEQ66_MARK_ALL

/**
 *  Marks all events.  Not yet used, but might come in handy with the event
 *  editor dialog.
 */

bool
eventlist::mark_all ()
{
    bool result = false;
    for (auto & e : m_events)
    {
        result = true;
        e.mark();
    }
    return result;
}

/**
 *  Unmarks all events.
 */

bool
eventlist::unmark_all ()
{
    bool result = false;
    for (auto & e : m_events)
    {
        if (e.is_marked())
        {
            result = true;
            e.unmark();
        }
    }
    return result;
}

#endif  // defined SEQ66_MARK_ALL

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
 *
 * \return
 *      Returns true if any event(s) got marked.
 */

bool
eventlist::mark_out_of_range (midipulse slength)
{
    bool result = false;
    for (auto & e : m_events)
    {
        bool prune = e.timestamp() > slength;   /* WAS ">=", SEE BANNER */
        if (! prune)
            prune = e.timestamp() < 0;          /* added back, seq66    */

        if (prune)
        {
            result = true;
            e.mark();
            if (e.is_linked())
                e.link()->mark();
        }
    }
    return result;
}

/**
 *  A helper function for sequence.  Finds the given event, and removes the
 *  first iterator matching that.  If there are events that would match after
 *  that, they remain in the container.  This matches seq24 behavior.
 *
 * \todo
 *      Use the find() function to find the matching event more
 *      conventionally.
 *
 * \param e
 *      Provides a reference to the event to be removed.
 *
 * \return
 *      Returns true if the event was found and removed.  This function
 *      returns immediately after the event is removed.
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
 *  We want to get event iterators for given events.
 *
 *  What we want to support:
 *
 *      -   Meta text.
 *          -   Song-info: Find (or remove) the first meta text event at
 *              timestamp 0.
 *          -   Track-info: Find (and take note of) the first meta text event
 *              at any timestamp (e.timestamp == c_null_midipulse).
 *              -   Get the text and timestamp. As en event?
 *              -   Set the event to a new value.
 *
 * \param e
 *      The event to match.
 *
 * \param starttick
 *      The starting point in time of the search.  Defaults to 0.
 *
 * \return
 *      Returns the iterator to the next match.  If end(), the event was
 *      not found.
 */

event::iterator
eventlist::find_first_match (const event & e, midipulse starttick)
{
    event::iterator result = m_events.end();
    for (auto i = m_events.begin(); i != m_events.end(); ++i)
    {
        event & er = dref(i);
        midipulse t = er.timestamp();
        if (t >= starttick)
        {
            if (er.match(e))                /* compares values, not ptrs    */
            {
                result = i;
                m_match_iterator = result;  /* keeps track of position      */
                break;
            }
        }
    }
    m_match_iterating = result != m_events.end();
    return result;
}

event::iterator
eventlist::find_next_match (const event & e)
{
    event::iterator result = m_events.end();
    if (m_match_iterating)
    {
        for (auto i = m_match_iterator; i != m_events.end(); ++i)
        {
            event & er = dref(i);
            if (er.match(e))            /* comparing values, not pointers   */
            {
                result = i;
                break;
            }
        }
        m_match_iterating = result != m_events.end();
        m_match_iterator = result;
    }
    else
        result = find_first_match(e);

    return result;

}

/**
 *  Removes the first event where there is a match based on event data,
 *  not event address.
 *
 * \param e
 *      Provides a reference to the event for which the first match is to be
 *      removed.
 *
 * \return
 *      Returns true if the event was found and removed.  This function
 *      returns immediately after the event is removed.
 */

bool
eventlist::remove_first_match (const event & e, midipulse starttick)
{
    bool result = false;
    for (auto i = m_events.begin(); i != m_events.end(); ++i)
    {
        event & er = dref(i);
        midipulse t = er.timestamp();
        if (t >= starttick)
        {
            if (er.match(e))            /* comparing values, not pointers   */
            {
                (void) remove(i);       /* an iterator is required here     */
                result = true;
                break;
            }
        }
    }
    return result;
}

/**
 *  Removes marked events.  Note how this function handles removing a
 *  value to avoid incrementing a now-invalid iterator.
 *
 * \threadsafe
 *
 * \return
 *      Returns true if at least one event was removed.
 */

bool
eventlist::remove_marked ()
{
    bool result = false;
    for (auto i = m_events.begin(); i != m_events.end(); /*++i*/)
    {
        if (i->is_marked())
        {
            auto t = remove(i);
            i = t;
            result = true;
        }
        else
            ++i;
    }
    if (result)
        verify_and_link();

    return result;
}

/**
 *  Removes events on or after the given timestamp, which is normally
 *  the beginning of a measure (though it does not have to be that).
 *
 *  We have to handle notes across the boundary carefully. We can either
 *  removed both parts (On and Off) of the note, or shorten the note.
 *  Probably the latter is better.
 *
 *  TO BE DETERMINED.
 *
 * \param limit
 *      The time stamp at or after which events are to be remove.
 */

bool
eventlist::remove_trailing_events (midipulse limit)
{
    bool result = false;
    for (auto i = m_events.begin(); i != m_events.end(); /*++i*/)
    {
        if (i->timestamp() >= limit)
        {
            /*
             * TODO: handled linked notes
             */

            auto t = remove(i);
            i = t;
            result = true;
        }
        else
        {
            if (i->is_note_on_linked())
            {
                auto ioff = i->link();
                if (ioff->timestamp() >= limit)
                    ioff->set_timestamp(limit - 1);
            }
            ++i;
        }
    }
    if (result)
        verify_and_link();

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
    for (auto i = m_events.begin(); i != m_events.end(); /*++i*/)
    {
        if (i->is_selected())
        {
            auto t = remove(i);
            i = t;
            result = true;
        }
        else
            ++i;
    }
    if (result)
        verify_and_link();

    return result;
}

/**
 *  Unpaints all list-events.
 */

void
eventlist::unpaint_all ()
{
    for (auto & er : m_events)
        er.unpaint();
}

/**
 *  Counts the selected Note On events in the event list.
 */

int
eventlist::count_selected_notes () const
{
    int result = 0;
    for (auto & er : m_events)
    {
        if (er.is_selected_note_on())
            ++result;
    }
    return result;
}

/**
 *  Indicates that at least one Note On is selected.  Acts like
 *  eventlist::count_selected_notes(), but stops after finding a selected
 *  note.
 *
 * \return
 *      Returns true if at least one Note On is selected.
 */

bool
eventlist::any_selected_notes () const
{
    bool result = false;
    for (auto & er : m_events)
    {
        if (er.is_selected_note_on())
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
 *  given CC value.  One exception is tempo events, which are selected
 *  based on the event::is_tempo() test.
 *
 * \param astatus
 *      The desired status value to count.  Note that tempo is 0x51.
 *
 * \param cc
 *      The desired control-change to count.  Used only if the status
 *      parameter indicates a control-change event.
 *
 * \return
 *      Returns the number of selected events.
 */

int
eventlist::count_selected_events (midibyte astatus, midibyte cc) const
{
    int result = 0;
    for (auto & er : m_events)
    {
        if (er.is_selected() && er.is_desired(astatus, cc))
            ++result;
    }
    return result;
}

/**
 *  Indicates that at least one event of any kind is selected.
 *
 * \return
 *      Returns true if at least one event is selected.
 */

bool
eventlist::any_selected_events () const
{
    bool result = false;
    for (auto & er : m_events)
    {
        if (er.is_selected())
        {
            result = true;
            break;
        }
    }
    return result;
}

/**
 *  This is an overload of any_selected_events(). It indicates that at least
 *  one matching event is selected.  Acts like eventlist ::
 *  count_selected_events(), but stops after finding a selected note.
 *
 * \return
 *      Returns true if at least one matching event is selected.
 */

bool
eventlist::any_selected_events (midibyte astatus, midibyte cc) const
{
    bool result = false;
    for (auto & er : m_events)
    {
        if (er.is_selected() && er.is_desired(astatus, cc))
        {
            result = true;
            break;
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
    for (auto & er : m_events)
        er.select();
}

void
eventlist::select_by_channel (int channel)
{
    midibyte target = midibyte(channel);
    for (auto & er : m_events)
    {
        if (er.channel() == target)
            er.select();
    }
}

/**
 *  Selects all note events with the given channel. Although we can extract
 *  the channel nybble from the status, we access the event channel member.
 */

void
eventlist::select_notes_by_channel (int channel)
{
    midibyte target = midibyte(channel);
    for (auto & er : m_events)
    {
        if (er.is_note() && er.channel() == target)
            er.select();
    }
}

/**
 *  Allows the events to be permanently set to a given channel. Obviously,
 *  it applies only to channel events such as Note On/Off.
 *
 * \param channel
 *      The caller is responsible for ensuring this parameter ranges from
 *      0 to 15.
 *
 * \return
 *      Returns true if even one event was modified.
 */

bool
eventlist::set_channels (int channel)
{
    bool result = false;
    midibyte target = midibyte(channel);
    for (auto & er : m_events)
    {
        if (er.has_channel())
        {
            er.set_channel(target);
            result = true;
        }
    }
    return result;
}

/**
 *  Deselects all events, unconditionally.
 */

void
eventlist::unselect_all ()
{
    for (auto & er : m_events)
        er.unselect();
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
 * \param astatus
 *      The desired event in the selection.  Now, as a new feature, tempo
 *      events are also selectable, in addition to events selected by this
 *      parameter. Oh, and now time-signature events.
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
    midibyte astatus, midibyte cc, select action
)
{
    int result = 0;
    for (auto & er : m_events)
    {
        if (event_in_range(er, astatus, tick_s, tick_f))
        {
            if (er.is_desired(astatus, cc))
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
                    remove_event(er);
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
 *  Selects the seqdata event handle if in range.  One issue in adjusting data
 *  is Pitch events, which have two components [d0() and d1()] which must be
 *  combined. Another issue is tempo events, where the value is converted to a
 *  note-velocity in order to display it in the data pane.
 *
 * \param tick_s
 *      Provides the starting tick, which is some small amount below the tick
 *      represented by the mouse position.
 *
 * \param tick_f
 *      Provides the finishing  tick, which is some small amount above the
 *      tick represented by the mouse position.
 *
 * \param astatus
 *      Provides the type of event, such as Note-On/Off, Pitchbend, or Tempo.
 *
 * \param cc
 *      For control events, represents the control code. Does not apply to
 *      Notes, Tempo, Pitchbend.  For Meta events, cc is the type of Meta
 *      event.
 *
 * \param data
 *      Currently represents the note value or d0().
 */

int
eventlist::select_event_handle
(
    midipulse tick_s, midipulse tick_f,
    midibyte astatus, midibyte cc,
    midibyte data
)
{
    int result = 0;
    bool have_selected_note_ons = false;
    if (event::is_note_on_msg(astatus))
    {
        if (count_selected_events(astatus, cc) > 0)
            have_selected_note_ons = true;
    }
    else if (event::is_tempo_status(cc))
    {
        // TODO?
    }
    for (auto & er : m_events)
    {
        if (event_in_range(er, astatus, tick_s, tick_f)) /* in time-range   */
        {
            bool isctrl = event::is_controller_msg(astatus);
            if (isctrl && er.is_desired(astatus, cc, data)) /* in range     */
            {
                unselect_all();                         /* or unmark()      */
                er.select();                            /* or mark()???     */
                ++result;
                break;
            }
            if (! isctrl)                               /* chan. pressure?  */
            {
                bool twobytes = event::is_two_byte_msg(astatus);
                if (twobytes)
                {
                    if (er.is_data_in_handle_range(data))   /* checks d1()  */
                    {
                        if (have_selected_note_ons)     /* Note Ons only    */
                        {
                            if (er.is_selected())
                            {
                                unselect_all();
                                er.select();
                                if (result > 0)         /* have a marked    */
                                {
                                    for (auto & ev : m_events)
                                    {
                                        if (ev.is_marked())
                                        {
                                            ev.unmark();    /* clear it     */
                                            break;
                                        }
                                    }
                                    --result;
                                    have_selected_note_ons = false;
                                }
                                ++result;               /* for selected one */
                                break;
                            }
                            else
                            {
                                if (result == 0)        /* mark only first  */
                                {
                                    er.mark();          /* mark for hold    */
                                    ++result;           /* we got one       */
                                }
                                continue;               /* till sel'ed/done */
                            }
                        }
                        else                            /* not Note On      */
                        {
                            unselect_all();
                            er.select();
                            ++result;
                            break;
                        }
                    }
                }
                else
                {
                    if (er.is_data_in_handle_range(data))   /* checks d0()  */
                    {
                        unselect_all();
                        er.select();
                        ++result;
                        break;
                    }
                }
            }
        }
    }

    /*
     * Is it a Note On that is unselected, but in range? Then use it....
     * The have_selected_note_ons flag will be false if we found a
     * selected one in range (?)
     */

    if (result > 0 && have_selected_note_ons)
    {
        for (auto & er : m_events)
        {
            if (er.is_marked())
            {
                unselect_all();
                er.unmark();
                er.select();
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
        int n = int(er.get_note());                 /* gets byte m_data[0]  */
        if (er.is_note() && n <= note_h && n >= note_l)
        {
            midipulse stick = 0, ftick = 0;
            if (er.is_linked())
            {
                event::iterator ev = er.link();
                if (er.is_note_off())
                {
                    stick = ev->timestamp();        /* time of the Note On  */
                    ftick = er.timestamp();         /* time of the Note Off */
                }
                else if (er.is_note_on())
                {
                    ftick = ev->timestamp();        /* time of the Note Off */
                    stick = er.timestamp();         /* time of the Note On  */
                }

                /*
                 * "tand" indicates that the event start is less than the
                 * finish parameter, and the event finish is greater than the
                 * start parameter.
                 *
                 * "tor" is the OR of these two tests, and is needed when the
                 * event start is greater than the finish, which occurs in a
                 * note-off.
                 *
                 * Not sure why so complex; all we need to know is that both
                 * the start and end times are within the desired range.
                 * However, then we cannot click on a note to select it.  Odd!
                 */

                bool tand = (stick <= tick_f) && (ftick >= tick_s);
                bool tor = (stick <= tick_f) || (ftick >= tick_s);
                bool ok = tand || ((stick > ftick) && tor);
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
                        remove_event(*ev);
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
                        remove_event(er);
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
 * \param astatus
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
    const event & e, midibyte astatus,
    midipulse tick_s, midipulse tick_f
) const
{
    bool result = e.match_status(astatus) || e.is_tempo() ||
        e.is_time_signature();

    if (result)
        result = e.timestamp() >= tick_s && e.timestamp() <= tick_f;

    return result;
}

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

bool
eventlist::rescale (int newppqn, int oldppqn)
{
    bool result = oldppqn > 0;
    if (result)
    {
        for (auto & er : m_events)
            er.rescale(newppqn, oldppqn);

        set_length(rescale_tick(get_length(), newppqn, oldppqn));
    }
    return result;
}

/**
 *  Performs a stretch operation on the selected events.  This should move
 *  a note off event, according to old comments, but it doesn't seem to do
 *  that.  See the grow_selected() function.  Rather, it moves any event in
 *  the selection.
 *
 *  Also, we've moved external push-undo into sequence functions.  The caller
 *  shouldn't have to do that.
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
                    midipulse nt = midipulse(ratio * (t - first_ev)) +
                        first_ev;

                    er.set_timestamp(nt);
                    result = true;
                }
            }
            if (result)
                verify_and_link();                  /* sorts as well        */
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
 *  This function grows/shrinks only Note On events that are linked.  If an
 *  event is not linked, this function ignores the event's timestamp, rather
 *  than risk a segfault on a null pointer.  Compare this function to the
 *  stretch_selected() and move_selected_notes() functions.
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
 *  we've moved push-undo into sequence functions.  The caller shouldn't have
 *  to do that.
 *
 *  A comment on terminology:  The user "selects" notes, while the sequencer
 *  "marks" notes. This function no longer bothers to mark all the selected
 *  notes.
 *
 * \threadsafe
 *
 * \param delta
 *      An offset for each linked event's timestamp.
 *
 * \param snap
 *      The snap amount for the growth.  Currently used only for non-notes.
 *
 * \return
 *      Returns true if at least one time-stamp was altered.
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
                    event::iterator off = er.link();
                    midipulse offtime = off->timestamp();
                    midipulse newtime = trim_timestamp(offtime + delta);
                    off->set_timestamp(newtime);    /* new off-time         */
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
        verify_and_link();                          /* sorts as well        */

    return result;
}

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
        verify_and_link();                      /* vice remove_selected()   */
    }
    return result;
}

/**
 *  A new function to consolidate the adjustment of timestamps in a pattern.
 *  Similar to adjust_timestamp, but it doesn't have an \a isnoteoff
 *  parameter.  Used only in this class.
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

/**
 *  Prints a list of the currently-held events.  Useful for debugging.
 */

void
eventlist::print () const
{
    std::printf("%d MIDI events:\n", count());
    for (auto & e : m_events)
        e.print();
}

/**
 *  Prints a list of the currently-held notes.  Useful for debugging.
 */

void
eventlist::print_notes (const std::string & tag) const
{
    std::printf("Notes %s:\n", tag.c_str());
    if (count() > 0)
    {
        for (auto & e : m_events)
            e.print_note();
    }
}

/**
 *  Constructs a list of the currently-held events.  Useful for debugging.
 */

std::string
eventlist::to_string () const
{
    std::string result = "Events (";
    result += std::to_string(count());
    result += "):\n";
    for (auto & e : m_events)
        result += e.to_string();

    return result;
}

}           // namespace seq66

/*
 * eventlist.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


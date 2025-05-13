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
 * \file          qseventslots.cpp
 *
 *  This module declares/defines the base class for displaying events in their
 *  editing slots.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-08-13
 * \updates       2025-05-09
 * \license       GNU GPLv2 or above
 *
 *  Also note that, currently, the editable_events container does not support
 *  a verify_and_link() function like that of the eventlist container.
 *  Eventually, we want to support that feature, so that we can delete a Note
 *  On event and have the corresponding Note Off event automatically deleted
 *  as well.
 */

#include "play/performer.hpp"           /* seq66::performer class           */
#include "util/strfunctions.hpp"        /* seq66::strings_match()           */
#include "qseqeventframe.hpp"
#include "qseventslots.hpp"

namespace seq66
{

/**
 *  Principal constructor for this user-interface object.
 *
 * \param p
 *      The parent performer object.
 *
 * \param parent
 *      The parent event-editor object.
 *
 * \param seq
 *      Provides the sequence represented by this event-editing object.
 */

qseventslots::qseventslots
(
    performer & p,
    qseqeventframe & parent,
    sequence & s
) :
    m_parent                (parent),
    m_seq                   (s),
    m_event_container       (s, p.get_beats_per_minute()),
    m_current_event         (m_event_container),
    m_event_count           (0),
    m_last_max_timestamp    (0),
    m_measures              (0),
    m_line_count            (0),
    m_line_maximum          (999999),   /* don't need with Qt table widget  */
    m_line_overlap          (5),
    m_top_index             (0),
    m_current_index         (SEQ66_NULL_EVENT_INDEX),   /* -1 */
    m_current_row           (0),
    m_top_iterator          (),
    m_bottom_iterator       (),
    m_current_iterator      (),
    m_pager_index           (0),
    m_show_data_as_hex      (false),                    /* hexadecimal()    */
    m_show_time_as_pulses   (false)                     /* pulses()         */
{
    load_events();
}

/**
 *  Grabs the event list from the sequence and uses it to fill the
 *  editable-event list.  Determines how many events can be shown in the
 *  GUI [later] and adjusts the top and bottom editable-event iterators to
 *  show the first page of events.
 *
 *  Note that, for this QWdigetTable support class, the line_maximum() is more
 *  of a sanity check, since the table can grow indefinitely and has no
 *  viewport in the sense the Gtkmm-2.4 version had.
 *
 * \return
 *      Returns true if the event iterators were able to be set up as valid.
 */

bool
qseventslots::load_events ()
{
    bool result = m_event_container.load_events();
    if (result)
    {
        m_event_count = m_event_container.count();
        if (m_event_count > 0)
        {
            if (m_event_count < m_line_count)
                m_line_count = m_event_count;
            else
                m_line_count = line_maximum();

            m_current_iterator = m_bottom_iterator =
                m_top_iterator = m_event_container.begin();

            for (int i = 0; i < m_line_count - 1; ++i)
            {
                if (increment_bottom() == SEQ66_NULL_EVENT_INDEX)
                    break;
            }
            for (auto & ei : m_event_container)
                ei.second.analyze();        /* creates the event strings    */
        }
        else
            result = false;
    }
    if (! result)
    {
        m_line_count = 0;
        m_current_iterator = m_bottom_iterator =
            m_top_iterator = m_event_container.end();
    }
    return result;
}

bool
qseventslots::load_table ()
{
    bool result = m_event_container.count() > 0;
    if (m_event_count > 0)
    {
        int row = 0;
        for (auto & ei : m_event_container)
        {
            set_table_event(ei.second, row);
            ++row;
        }
    }
    return result;
}

/**
 *  Any way to easily add the link indexes or add arrows to the linked
 *  note?
 *
 *      "Link-time L> <L  Rank\n"
 */

std::string
qseventslots::events_to_string () const
{
    std::string result;
    if (m_event_count > 0)
    {
        int row = 0;
        result +=
" No.  Ticks Timestamp Event   Status Ch. --   D0    D1 Link-time Length  Rank\n"
            ;
        for (const auto & ei : m_event_container)
        {
            result += event_to_string(ei.second, row);
            ++row;
        }
    }
    return result;
}

/**
 *  Provides the "printf()" format statement for a data value for both the
 *  data columns in the event table and the data field in the right-hand
 *  editable area of the event editor.
 */

#define SEQ66_EVENT_DATA_FMT_DEC    "%d"
#define SEQ66_EVENT_DATA_FMT_HEX    "0x%02x"

/**
 *  Set the current event, which is the event that is highlighted.  Note in
 *  the snprintf() calls that the first digit is part of the data byte, so
 *  that translation is easier.
 *
 * \param ei
 *      The iterator that points to the event.
 *
 * \param index
 *      The index (re 0) of the event, starting at the top line of the frame.
 *      It is a frame index, not a container index. (NOT TRUE using Qt!)
 *
 * \param full_redraw
 *      If true (the default) does a full redraw of the frame.  Otherwise,
 *      only the current event is drawn.  Generally, the only time a single
 *      event (actually, two adjacent events) is convenient to draw is when
 *      using the arrow keys, where the speed of keystroke auto-repeats makes
 *      the full-frame update scrolling very flickery and disconcerting.
 */

void
qseventslots::set_current_event
(
    const editable_events::iterator ei,
    int index,
    bool /* full_redraw */
)
{
    int channel = null_channel();
    std::string data_0;
    std::string data_1;
    const editable_event /* & */ ev = editable_events::cdref(ei);
    if (ev.is_meta())
    {
        /*
         * This may not be suitable for all meta events. But we will
         * add support for using the "sysex" (text) data for storing
         * data for these events.
         */

        std::string text = ev.get_text();
        m_parent.set_event_plaintext(text);
    }
    else if (ev.is_sysex())
    {
        std::string text = ev.get_text();
        m_parent.set_event_plaintext(text);
    }
    else if (ev.is_ex_data())
    {
        data_0 = ev.ex_data_string();
        m_parent.set_event_plaintext("Ex data");
    }
    else if (ev.is_system())
    {
        m_parent.set_event_system("System data");
        m_parent.set_event_plaintext("System data: To do!");
    }
    else
    {
        /*
         * qseqeventframe::data_0_helper() is used to display program change and
         * controller names. We could use it here. Also note that setting
         * the plaintext data to empty will show its place-holder text,
         * if set.
         */

        midibyte d0, d1;
        ev.get_data(d0, d1);
        data_0 = data_string(d0);
        data_1 = data_string(d1);
        channel = int(ev.channel());
        if (ev.is_pitchbend())                  /* TODO: use RPN setting    */
        {
            double semis = pitch_value_semitones(d0, d1);
            std::string s = double_to_string(semis, 3); /* number of digits */
            std::string msg = "Pitch bend ";
            msg += s;
            msg += " semitones";
            m_parent.set_event_plaintext(msg);
        }
        else
            m_parent.set_event_plaintext("");   /* no plaintext data here   */
    }
    set_event_text
    (
        ev.category_string(), ev.timestamp_string(), ev.status_string(),
        data_0, data_1, channel
    );
    m_current_row = m_current_index = index;
    m_current_iterator = ei;
    m_current_event = ev;
}

std::string
qseventslots::data_string (midibyte d)
{
    char tmp[32];
    const char * format = SEQ66_EVENT_DATA_FMT_DEC;
    if (m_show_data_as_hex)
        format = SEQ66_EVENT_DATA_FMT_HEX;

    (void) snprintf(tmp, sizeof tmp, format, int(d));
    return std::string(tmp);
}

/**
 *  Similar to set_current_event(), but fills in the table row with data,
 *  rather than filling the side fields for the current event.
 */

void
qseventslots::set_table_event (editable_event & ev, int row)
{
    std::string data_0;
    std::string data_1;
    std::string linktime;
    std::string tstring = m_show_time_as_pulses ?
        std::to_string(long(ev.timestamp())) : ev.timestamp_string() ;

    int buss = int(ev.input_bus());
    std::string busno;
    if (is_null_buss(buss))
    {
        if (m_seq.has_in_bus())
            busno = "<" + std::to_string(int(m_seq.seq_midi_in_bus())) + ">";
        else
            busno = "-";
    }
    else
        busno = std::to_string(buss);

    if (ev.is_meta_text())
    {
        data_0 = ev.ex_text_string();
    }
    else if (ev.is_ex_data())
    {
        data_0 = ev.ex_data_string();
    }
    else
    {
        midibyte d0, d1;
        ev.get_data(d0, d1);
        data_0 = data_string(d0);
        data_1 = data_string(d1);
        if (ev.is_linked())
        {
            midipulse lt = ev.link_time();
            if (m_show_time_as_pulses)
            {
                linktime = std::to_string(long(lt));
            }
            else
            {
                linktime = pulses_to_measurestring
                (
                    lt, m_event_container.timing()
                );
            }
        }
        else
            linktime = "None";
    }
    m_parent.set_event_line
    (
        row, tstring, ev.status_string(), busno,
        ev.channel_string(), data_0, data_1, linktime
    );
}

std::string
qseventslots::time_string (midipulse lt)
{
    std::string result = "None";
    if (! is_null_midipulse(lt))
        result = pulses_to_measurestring(lt, m_event_container.timing());

    return result;
}

std::string
qseventslots::event_to_string
(
    const editable_event & ev,
    int index, bool usehex
) const
{
    char line[132];
    if (ev.is_ex_data())
    {
        std::string data_0 = ev.ex_data_string();
        snprintf
        (
            line, sizeof line,
            "%4d %6ld %-9s %-9s %-30s  0x%04x\n",
            index, long(ev.timestamp()), ev.timestamp_string().c_str(),
            ev.status_string().c_str(), data_0.c_str(), ev.get_rank()
        );
    }
    else
    {
        std::string data_0;
        std::string data_1;
        std::string linktime;
        std::string lenstring = "--";
        const char * fmt = usehex ? "0x%02x" : "%5d";
        char tmp[32];
        midibyte d0, d1;
        midibyte rawstatus = ev.get_status();
        ev.get_data(d0, d1);
        snprintf(tmp, sizeof tmp, fmt, int(d0));
        data_0 = tmp;
        snprintf(tmp, sizeof tmp, fmt, int(d1));
        data_1 = tmp;
        if (ev.is_linked())
        {
            midipulse lt = ev.link_time();
            linktime = pulses_to_measurestring(lt, m_event_container.timing());
            if (ev.is_note_on())
                lenstring = std::to_string(long(lt) - long(ev.timestamp()));
        }
        else
            linktime = "None";

        snprintf
        (
            line, sizeof line,
            "%4d %6ld %-9s %-9s 0x%02x Ch %2s %3s %3s %-9s %6s  0x%04x\n",
            index, long(ev.timestamp()), ev.timestamp_string().c_str(),
            ev.status_string().c_str(), rawstatus, ev.channel_string().c_str(),
            data_0.c_str(), data_1.c_str(), linktime.c_str(),
            lenstring.c_str(), ev.get_rank()
        );
    }
    return std::string(line);
}

/**
 *  Sets the text in the parent dialog, qseqeventframe.
 *
 * \param evcategory
 *      The category of event to be set in the parent.
 *
 * \param evts
 *      The event time-stamp to be set in the parent.
 *
 * \param evname
 *      The event name to be set in the parent.
 *
 * \param evdata0
 *      The first event data byte to be set in the parent.
 *
 * \param evdata1
 *      The second event data byte to be set in the parent.
 */

void
qseventslots::set_event_text
(
    const std::string & evcategory,
    const std::string & evts,
    const std::string & evname,
    const std::string & evdata0,
    const std::string & evdata1,
    int channel
)
{
    m_parent.set_event_timestamp(evts);
    m_parent.set_event_category(evcategory);
    m_parent.set_event_name(evname);
    m_parent.set_event_channel(channel);
    m_parent.set_event_data_0(evdata0);
    m_parent.set_event_data_1(evdata1);
}

midibyte
qseventslots::string_to_channel (const std::string & channel)
{
    midibyte result = track().seq_midi_channel();
    if (! channel.empty())
        result = midibyte(string_to_int(channel) - 1);

    return result;
}

/**
 *  Inserts an event.  What actually happens here depends if the new event is
 *  before the frame, within the frame, or after the frame, based on the
 *  timestamp.  Also, we want to allow the lengthening of a sequence by
 *  inserting an event past its current length.  Especially useful for the
 *  tempo track.
 *
 *  If before the frame: To keep the previous events visible, we do not need
 *  to increment the iterators (insertion does not affect multimap iterators),
 *  but we do need to increment their indices.  The contents shown in the frame
 *  should not change.
 *
 *  If at the frame top:  The new timestamp equals the top timestamp. We don't
 *  know exactly where the new event goes in the multimap, but we do have an
 *  new event.
 *
 *  If after the frame: No action needed if the bottom event is actually at
 *  the bottom of the frame.  But if the frame is not yet filled, we need to
 *  increment the bottom iterator, and its index.
 *
 * \note
 *      Actually, it is far easier to just adjust all the counts and iterators
 *      and redraw the screen, as done by the page_topper() function.
 *
 * \param edev
 *      The event to insert, prebuilt.
 *
 * \return
 *      Returns true if the event was inserted.
 */

bool
qseventslots::insert_event (editable_event ev)          // Q: where called?
{
    bool result = m_event_container.add(ev);
    if (result)
    {
        m_event_count = m_event_container.count();
        if (m_event_count == 1)
        {
            m_line_count = 1;           /* used in drawing in a bit */
            m_top_index = 0;
            m_current_index = 0;
            m_top_iterator = m_current_iterator =
                m_bottom_iterator = m_event_container.begin();

            select_event(m_current_index);
        }
        else
        {
            /*
             * This iterator is a short-lived [changed after the next add()
             * call] pointer to the added event.  This check somehow breaks
             * the redisplay of the modified event list in the Gtkmm-2.4
             * user-interface.
             *
             * if (m_event_container.is_valid_iterator(nev))
             */

            auto nev = m_event_container.current_event();
            page_topper(nev);
            m_parent.set_dirty();
        }

        /*
         * Now see if the timestamp of the new event is past the end of the
         * sequence.  In this case, we will allow the sequence to increase in
         * length.  We also need to account for any change in length.
         */

        if (get_length() > m_last_max_timestamp)
        {
            m_last_max_timestamp = get_length();
        }
    }
    return result;
}

/**
 *  Inserts an event based on the setting provided, which the qseqeventframe
 *  object gets from its Entry fields.  It calls the other insert_event()
 *  overload.
 *
 *  Note that we need to qualify the temporary event class object we create
 *  below, with the seq66 namespace, otherwise the compiler thinks we're
 *  trying to access some Gtkmm thing.
 *
 * \param evts
 *      The time-stamp of the new event, as obtained from the event-edit
 *      timestamp field.
 *
 * \param evname
 *      The type name (status name)  of the new event, as obtained from the
 *      event-edit event-name field.
 *
 * \param evdata0
 *      The first data byte of the new event, as obtained from the event-edit
 *      data 1 field.
 *
 * \param evdata1
 *      The second data byte of the new event, as obtained from the event-edit
 *      data 2 field.  Used only for two-parameter events.
 *
 * \param channel
 *      Provides the channel string.  Defaults to "", which means the
 *      sequence's internal MIDI channel (most often it is 0) is used.
 *
 * \return
 *      Returns true if the event was inserted.
 */

bool
qseventslots::insert_event
(
    const std::string & evts,
    const std::string & evname,
    const std::string & evdata0,
    const std::string & evdata1,
    const std::string & channel,
    const std::string & text
)
{
    seq66::event e;                                 /* new default event    */
    editable_event edev(m_event_container, e);
    edev.set_status_from_string
    (
        evts, evname, evdata0, evdata1, channel, text
    );
    m_current_event = edev;
    return insert_event(edev);
}

/**
 *  Deletes the current event, and makes adjustments due to that deletion.
 *  Also deletes the linked note for Note Offs and Ons.
 *
 *  To delete the current event, this function moves the current iterator to
 *  the next event, deletes the previously-current iterator, adjusts the event
 *  count and the bottom iterator, and redraws the pixmap.  The exact changes
 *  depend upon whether the deleted event was at the top of the visible frame,
 *  within the visible frame, or at the bottom the visible frame.  Note that
 *  only visible events can be the current event, and thus get deleted.
 *
\verbatim
         Event Index
          0
          1
          2         Top
          3  <--------- Top case: The new top iterator, index becomes 2
          4
          .
          .         Inside of Visible Frame
          .
         43
         44         Bottom
         45  <--------- Top case: The new bottom iterator, index becomes 44
         46             Bottom case: Same result
\endverbatim
 *
 *  Basically, when an event is deleted, the frame (delimited by the
 *  event-index members) stays in place, while the frame iterators move to the
 *  previous event.  If the top of the frame would move to before the first
 *  event, then the frame must shrink.
 *
 *  Top case: If the current iterator is the top (of the frame) iterator, then
 *  the top iterator needs to be incremented.  The new top event has the same
 *  index as the now-gone top event. The index of the bottom event is
 *  decremented, since an event before it is now gone.  The bottom iterator
 *  moves to the next event, which is now at the bottom of the frame.  The
 *  current event is treated like the top event.
 *
 *  Inside case: If the current iterator is in the middle of the frame, the top
 *  iterator and index remain unchanged.  The current iterator is incremented,
 *  but its index is now the same as the old bottom index.  Same for the bottom
 *  iterator.
 *
 *  Bottom case: If the current iterator (and bottom iterator) point to the
 *  last event in the frame, then both of them need to be
 *  decremented.  The frame needs to be moved up by one event, so that the
 *  current event remains at the bottom (it's just simpler to manage that way).
 *
 *  If there is no event after the bottom of the frame, the iterators that now
 *  point to end() must backtrack one event.  If the container becomes empty,
 *  then everything is invalidated.
 *
 * \return
 *      Returns true if the delete was possible.  If the container was empty
 *      then false is returned.  It is the caller's responsibility to check if
 *      the container is empty after this operation, via a call to
 *      qseventslots::empty().
 */

bool
qseventslots::delete_current_event ()
{
    bool result = m_event_count > 0;
    if (result)
        result = m_current_iterator != m_event_container.end();

    if (result)
    {
        auto oldcurrent = m_current_iterator;
        int oldcount = m_event_container.count();
        if (oldcount > 1)
        {
            if (m_current_index == 0)
            {
                (void) increment_top();         /* bypass to-delete event   */
                (void) increment_current();     /* ditto                    */
                (void) increment_bottom();      /* next event up to bottom  */
            }
            else if (m_current_index == (m_line_count - 1))
            {
                /*
                 * If we are before the last event in the event container, we
                 * can increment the iterators.  Otherwise, we have to back
                 * off by one so we are above the last event, which will be
                 * deleted.
                 */

                if (m_current_index < (m_event_count - 1))
                {
                    (void) increment_current();
                    (void) increment_bottom();
                }
                else
                {
                    /*
                     * The frame must shrink.
                     */

                    m_current_index = decrement_current();
                    (void) decrement_bottom();
                    if (m_line_count > 0)
                        --m_line_count;
                }
            }
            else
            {
                /*
                 * \change ca 2016-05-18
                 *      Try to avoid a crash deleting the last item again
                 *      (since it is still visible).
                 *
                 * (void) increment_current();
                 * (void) increment_bottom();
                 */

                if (increment_current() != SEQ66_NULL_EVENT_INDEX)
                {
                    (void) increment_bottom();
                    m_bottom_iterator = m_event_container.end();
                }
                else            // if (m_current_index >= 0)  /* issues/26 */
                    --m_current_index;
            }
        }

        /*
         * Has to be done after the adjustment, otherwise iterators are
         * invalid and cannot be adjusted.  The remove() function validates
         * the iterator before trying to delete it.
         */

        m_event_container.remove(oldcurrent);       /* wrapper for erase()  */

        int newcount = m_event_container.count();
        if (newcount == 0)
        {
            m_top_index = m_current_index = 0;
            m_top_iterator = m_current_iterator =
                m_bottom_iterator = m_event_container.end();
        }

        bool ok = newcount == (oldcount - 1);
        if (ok)
        {
            m_parent.set_dirty();
            m_event_count = newcount;
            result = true;                          /* an event was deleted */
            if (newcount > 0)
                select_event(m_current_index);
            else
                select_event(SEQ66_NULL_EVENT_INDEX);
        }
    }
    return result;
}

/**
 *  Modifies the data in the currently-selected event.  If the timestamp has
 *  changed, however, we can't just modify the event in place.  Instead, we
 *  finish modifying the event, but tell the caller to delete and reinsert the
 *  new event (in its proper new location based on timestamp).
 *
 *  This function always copies the original event, modifiies the copy,
 *  deletes the original event, and inserts the "new" event into the
 *  editable-event container.  The insertion takes care of updating any length
 *  increase of the sequence.
 *
 * \param evts
 *      Provides the new event time-stamp as edited by the user.
 *
 * \param evname
 *      Provides the event name as edited by the user.
 *
 * \param evdata0
 *      Provides the first data byte as edited by the user.
 *
 * \param evdata1
 *      Provides the second data byte as edited by the user.
 *
 * \param channel
 *      Provides the channel string.  Defaults to "", which means the
 *
 * \param text
 *      For ex-data events (meta, sysex, and system), this provides
 *      data to be converted into storable bytes.
 *
 * \return
 *      Returns true simply if the event-count is greater than 0.
 */

bool
qseventslots::modify_current_event
(
    int row,
    const std::string & evts,
    const std::string & evname,
    const std::string & evdata0,
    const std::string & evdata1,
    const std::string & channel,
    const std::string & text
)
{
    bool result = m_event_count > 0;
    if (result)
       result =  m_current_iterator != m_event_container.end();

    if (result)
    {
        editable_event & evref = editable_events::dref(m_current_iterator);
        bool isnoteevent = evref.is_note();
        midibyte channelbyte = string_to_channel(channel);
        if (isnoteevent)
        {
            evref.set_status_from_string
            (
                evts, evname, evdata0, evdata1, channel
            );
            if (row >= 0)
                set_table_event(evref, row);
        }
        else
        {
            editable_event ev = editable_events::dref(m_current_iterator);
            ev.set_status_from_string
            (
                evts, evname, evdata0, evdata1, channel, text
            );
            if (! ev.is_ex_data())
                ev.set_channel(channelbyte);

            result = delete_current_event();            /* full karaoke del */
            if (result)
                result = insert_event(ev);              /* full karaoke add */
        }
    }
    return result;
}

/**
 *  This function assumes it is called only for channel events.
 */

bool
qseventslots::modify_current_channel_event
(
    int row,
    const std::string & evdata0,
    const std::string & evdata1,
    const std::string & channel
)
{
    bool result = m_event_count > 0;
    if (result)
       result =  m_current_iterator != m_event_container.end();

    if (result)
    {
        editable_event & ev = editable_events::dref(m_current_iterator);
        ev.modify_channel_status_from_string(evdata0, evdata1, channel);
        if (row >= 0)
            set_table_event(ev, row);
    }
    return result;
}

/**
 *  Writes the events back to the sequence.  Added a copy_events() function in
 *  the sequence class for the purpose of reconstructing the events container
 *  for the sequence.  It is locked by a mutex, and so will not draw until all
 *  is done, preventing a segfault.
 *
 *  We create a new plain event container here, and then passing it to the new
 *  locked/threadsafe sequence::copy_events() function that clears the
 *  sequence container and copies the events from the parameter container.
 *  Note that this code will operate event if all events were deleted.
 *
 * \return
 *      Returns true if the operations succeeded.
 */

bool
qseventslots::save_events ()
{
    bool result = m_event_count >= 0 &&
        m_event_count == m_event_container.count();

    if (result)
    {
        eventlist newevents;
        for (auto & ei : m_event_container)
        {
            result = newevents.add(ei.second);
            if (! result)
                break;
        }
        if (result)
            result = newevents.count() == m_event_count;

        if (result)
        {
            track().copy_events(newevents);
            result = track().event_count() == m_event_count;
            if (result && m_last_max_timestamp > track().get_length())
                track().set_length(m_last_max_timestamp);
        }
    }
    return result;
}

#if defined QSEVENTSLOTS_FUNCTION_USED

/**
 *  Adjusts the vertical position of the frame according to the given new
 *  scrollbar/vadjust value.  The adjustment is done via movement from the
 *  current position.
 *
 *  Do we even need a way to detect excess movement?  The scrollbar, if
 *  properly set up, should never move the frame too high or too low.
 *  Verified by testing.
 *
 * \param new_value
 *      Provides the new value of the scrollbar position.
 */

void
qseventslots::page_movement (int new_value)
{
    if ((new_value >= 0) && (new_value < m_event_count))
    {
        int movement = new_value - m_pager_index;       /* can be negative */
        int absmovement = movement >= 0 ? movement : -movement;
        m_pager_index = new_value;
        if (movement != 0)
        {
            /*
             * @change ca 2016-10-08
             *      Issue #38.  We see a double-increment when moving the
             *      scroll down one slot.  But if we don't do this, the event
             *      indices at the left always start at 0 at the top of the
             *      view, no matter what the page.  Weird!
             */

            m_top_index += movement;

            if (movement > 0)
            {
                for (int i = 0; i < movement; ++i)
                {
                    (void) increment_top();
                    (void) increment_bottom();
                }
            }
            else if (movement < 0)
            {
                for (int i = 0; i < absmovement; ++i)
                {
                    (void) decrement_top();
                    (void) decrement_bottom();
                }
            }

            /*
             * Don't move the current event (highlighted) unless
             * we move more than one event.  Annoying to the user.
             */

            if (absmovement > 1)
            {
                set_current_event(m_top_iterator, 0);
            }
            else
            {
                int newindex = m_current_index + movement;
                set_current_event(m_current_iterator, newindex);
            }
        }
    }
}

#endif  // defined QSEVENTSLOTS_FUNCTION_USED

/**
 *  Adjusts the vertical position of the frame according to the given new
 *  bottom iterator.  The adjustment is done "from scratch".  We've found page
 *  movement to be an insoluable problem in some editing circumstances.  So
 *  now we move to the inserted event, and make it the top event.
 *
 *  However, always moving an inserted event to the top is a bit annoying.  So
 *  now we backtrack so that the inserted event is at the bottom.
 *
 * \param newcurrent
 *      Provides the iterator to the event to be shown at the bottom of the
 *      frame.
 */

void
qseventslots::page_topper (editable_events::iterator newcurrent)
{
    bool ok = newcurrent != m_event_container.end();
    if (ok)
        ok = m_event_count > 0;

    if (ok)
    {
        auto ei = m_event_container.begin();
        int botindex = 0;
        while (ei != newcurrent)
        {
            ++botindex;
            ++ei;
            if (botindex == m_event_count)
            {
                ok = false;                     /* never found the event!   */
                break;
            }
        }
        if (m_event_count <= line_maximum())    /* fewer events than lines  */
        {
            if (ok)
            {
                m_pager_index = m_top_index = 0;
                m_top_iterator = m_event_container.begin();
                m_line_count = m_event_count;
                m_current_iterator = newcurrent;
                m_current_index = botindex;
            }
        }
        else
        {
            m_line_count = line_maximum();
            if (ok)
            {
                /*
                 * Backtrack by up to line_maximum() events so that the new
                 * event is shown at the bottom or at its natural location; it
                 * also needs to be the current event, for highlighting.
                 * Count carefully!
                 */

                auto ei = m_event_container.begin();
                int pageup = botindex - line_maximum();
                if (pageup < 0)
                {
                    m_top_index = m_pager_index = pageup = 0;
                }
                else
                {
                    int countdown = pageup;
                    while (countdown-- > 0)
                        ++ei;

                    m_top_index = m_pager_index = pageup + 1;   /* re map   */
                }

                m_top_iterator = ei;
                m_current_iterator = newcurrent;
                m_current_index = botindex - m_top_index;       /* re frame */
            }
        }
        if (ok)
            select_event(m_current_index);
    }
}

/**
 *  Selects and highlights the event that is located in the frame at the given
 *  event index.  The event index is provided by the QtTable Widget.
 *  Note that, if the event index is negative, then we just queue up a draw
 *  operation, which should paint an empty frame -- the event container is
 *  empty.
 *
 * \param event_index
 *      Provides the numeric index of the event in the event frame, or
 *      SEQ66_NULL_EVENT if there is no event to draw.
 *
 * \param full_redraw
 *      Defaulting to true, this parameter can be set to false in some case to
 *      reduce the flickering of the frame under fast movement.  Doesn't
 *      really make sense yet in the Qt 5 version.
 */

void
qseventslots::select_event (int event_index, bool full_redraw)
{
    bool ok = event_index != SEQ66_NULL_EVENT_INDEX;
    if (ok)
        ok = (event_index < m_line_count);

    if (ok)
    {
        auto ei = m_event_container.begin();    /* not m_top_iterator   */
        ok = ei != m_event_container.end();
        if (ok && (event_index > 0))
        {
            int i = 0;
            while (i++ < event_index)
            {
                ++ei;
                ok = ei != m_event_container.end();
                if (! ok)
                    break;
            }
        }
        if (ok)
            set_current_event(ei, event_index, full_redraw);
    }
}

/**
 *  Decrements the top iterator, if possible.
 *
 * \return
 *      Returns 0, or SEQ66_NULL_EVENT_INDEX if the iterator could not be
 *      decremented.
 */

int
qseventslots::decrement_top ()
{
    if (m_top_iterator != m_event_container.begin())
    {
        --m_top_iterator;
        return m_top_index - 1;
    }
    else
        return SEQ66_NULL_EVENT_INDEX;
}

/**
 *  Increments the top iterator, if possible.  Also handles the top-event
 *  index, so that the GUI can display the proper event numbers.
 *
 * \return
 *      Returns the top index, or SEQ66_NULL_EVENT_INDEX if the iterator could
 *      not be incremented, or would increment to the end of the container.
 */

int
qseventslots::increment_top ()
{
    auto ei = m_top_iterator;
    if (ei != m_event_container.end())
    {
        ++ei;
        if (ei != m_event_container.end())
        {
            m_top_iterator = ei;
            return m_top_index + 1;
        }
        else
            return SEQ66_NULL_EVENT_INDEX;
    }
    else
        return SEQ66_NULL_EVENT_INDEX;
}

/**
 *  Decrements the current iterator, if possible.
 *
 * \return
 *      Returns the decremented index, or SEQ66_NULL_EVENT_INDEX if the
 *      iterator could not be decremented.  Remember that the index ranges
 *      only from 0 to m_line_count-1, and that is enforced here.
 */

int
qseventslots::decrement_current ()
{
    if (m_current_iterator != m_event_container.begin())
    {
        --m_current_iterator;
        int result = m_current_index - 1;
        if (result < 0)
            result = 0;

        return result;
    }
    else
        return SEQ66_NULL_EVENT_INDEX;
}

/**
 *  Increments the current iterator, if possible.
 *
 * \return
 *      Returns the incremented index, or SEQ66_NULL_EVENT_INDEX if the
 *      iterator could not be incremented.  Remember that the index ranges
 *      only from 0 to m_line_count-1, and that is enforced here.
 */

int
qseventslots::increment_current ()
{
    auto ei = m_current_iterator;
    if (ei != m_event_container.end())
    {
        int result = SEQ66_NULL_EVENT_INDEX;
        ++ei;
        if (ei != m_event_container.end())
        {
            m_current_iterator = ei;
            result = m_current_index + 1;
            if (result >= m_line_count)
                result = m_line_count - 1;
        }
        return result;
    }
    else
        return SEQ66_NULL_EVENT_INDEX;
}

/**
 *  Decrements the bottom iterator, if possible.
 *
 * \return
 *      Returns 0, or SEQ66_NULL_EVENT_INDEX if the iterator could not be
 *      decremented.
 */

int
qseventslots::decrement_bottom ()
{
    if (m_bottom_iterator != m_event_container.begin())
    {
        --m_bottom_iterator;
        return 0;
    }
    else
        return SEQ66_NULL_EVENT_INDEX;
}

/**
 *  Increments the bottom iterator, if possible.  There is an issue in paging
 *  down using the scrollbar where, at the bottom of the scrolling, the bottom
 *  iterator ends up bad.  Not yet sure how this happens, so for now we
 *  backtrack one event if this happens.
 *
 * \return
 *      Returns the incremented index, or SEQ66_NULL_EVENT_INDEX if the
 *      iterator could not be incremented.
 */

int
qseventslots::increment_bottom ()
{
    int result = SEQ66_NULL_EVENT_INDEX;
    if (m_bottom_iterator != m_event_container.end())
    {
        auto old = m_bottom_iterator++;
        if (m_bottom_iterator != m_event_container.end())
            result = 0;
        else
            m_bottom_iterator = old;        /* backtrack to initial value   */
    }
    return result;
}

/**
 *  This function basically duplicates sequence::calculate_measures().
 *
 * \return
 *      Returns the number of measures needed to cover the full length of the
 *      current events in the container.  The lowest valid measure is 1.
 */

int
qseventslots::calculate_measures () const
{
    return track().calculate_measures();
}

}           // namespace seq66

/*
 * qseventslots.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


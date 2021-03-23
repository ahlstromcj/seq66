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
 * \file          triggers.cpp
 *
 *  This module declares/defines the base class for handling
 *  triggers for patterns/sequences.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-10-30
 * \updates       2021-01-12
 * \license       GNU GPLv2 or above
 *
 *  Man, we need to learn a lot more about triggers.  One important thing to
 *  note is that the triggers are written to a MIDI file using the
 *  sequencer-specific code c_triggers_new.
 *
 * Stazed:
 *
 *      I believe there were only two things I did with triggers. First, you
 *      could split triggers with a middle mouse button click and seq66 would
 *      cut the trigger down the middle. I fixed the code to allow the split
 *      on the mouse pointer location, grid snapped. This was actually a
 *      simplification of code - just removed the calculation of trigger mid
 *      point and added the grid snap calc.
 *
 *      The second issue was Ctrl-C copy of selected triggers and Ctrl-V
 *      paste of triggers.  Original seq66 just pasted after the selected
 *      Ctrl-C trigger, and repeated Ctrl-V would continue paste after the
 *      previous paste. I added the ability to paste to any location on the
 *      sequence track line if open.  This is done with a middle mouse click
 *      on the open part of the track.  The mouse click sets the location,
 *      and Ctrl-V will paste it via set_trigger_paste_tick(). Then the paste
 *      tick is set to -1 (none) and the original method applies: subsequent
 *      Ctrl-Vs will paste after the previous paste.
 *
 *      Middle click on a new location and it will use the new location for
 *      the first paste. If you paste very close to and in front of an
 *      existing trigger that overlaps, it will overwrite and crop. The
 *      Fruity method uses middle mouse or Ctrl-Left mouse click for paste
 *      location.
 */

#include "cfg/settings.hpp"             /* seq66::rc() settings access      */
#include "play/sequence.hpp"            /* the "parent" of the triggers     */
#include "play/triggers.hpp"            /* seq66::triggers helper class     */
#include "util/strfunctions.hpp"        /* seq66::bool_to_string()          */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

trigger::trigger () :
    m_tick_start    (0),
    m_tick_end      (0),
    m_offset        (0),
    m_selected      (false)
{
    // Empty body
}

trigger::trigger (midipulse tick, midipulse len, midipulse offset) :
    m_tick_start    (tick),
    m_tick_end      (tick + len - 1),
    m_offset        (offset),
    m_selected      (false)
{
    // Empty body
}

void
trigger::rescale (int oldppqn, int newppqn)
{
    m_tick_start = rescale_tick(m_tick_start, oldppqn, newppqn);
    m_tick_end = rescale_tick(m_tick_end, oldppqn, newppqn);
    m_offset = rescale_tick(m_offset, oldppqn, newppqn);
}

std::string
trigger::to_string () const
{
    std::string result = "trigger: ";
    result += std::to_string(tick_start());
    result += " to ";
    result += std::to_string(tick_end());
    result += " at ";
    result += std::to_string(offset());
    return result;
}

/**
 *  Principal constructor.
 *
 * \param parent
 *      The triggers object often needs to tell its parent sequence object
 *      what to do (such as stop playing).
 */

triggers::triggers (sequence & parent)
 :
    m_parent                    (parent),
    m_triggers                  (),
    m_number_selected           (0),
    m_clipboard                 (),
    m_undo_stack                (),
    m_redo_stack                (),
    m_draw_iterator             (),
    m_trigger_copied            (false),
    m_paste_tick                (c_no_paste_trigger),   // stazed
    m_ppqn                      (0),
    m_length                    (0)
{
    // Empty body
}

/**
 *  Principal assignment operator.  Follows the stock rules for such an
 *  operator, but does a little more then just assign member values.
 *
 * \param rhs
 *      Provides the "right-hand side" of the assignment operation.
 *
 * \return
 *      Returns a reference to self, for use in concatenated assignment
 *      operations.
 */

triggers &
triggers::operator = (const triggers & rhs)
{
    if (this != &rhs)
    {
        /*
         * Reference member: m_parent = rhs.m_parent;
         */

        m_triggers = rhs.m_triggers;
        m_clipboard = rhs.m_clipboard;
        m_undo_stack = rhs.m_undo_stack;
        m_redo_stack = rhs.m_redo_stack;
        m_draw_iterator = rhs.m_draw_iterator;
        m_trigger_copied = rhs.m_trigger_copied;
        m_ppqn = rhs.m_ppqn;
        m_length = rhs.m_length;
    }
    return *this;
}

bool
triggers::rescale (int oldppqn, int newppqn)
{
    bool result = oldppqn > 0;
    if (result)
    {
        for (auto & t : m_triggers)
            t.rescale(oldppqn, newppqn);

        set_length(rescale_tick(m_length, oldppqn, newppqn));
    }
    return result;
}

bool
triggers::change_ppqn (int p)
{
    bool result = p > 0;
    if (result)
    {
        bool result = rescale(m_ppqn, p);
        if (result)
            set_ppqn(p);
    }
    return result;
}

/**
 *  Pushes the list-trigger into the trigger undo-list, then flags each
 *  item in the undo-list as unselected.
 */

void
triggers::push_undo ()
{
    m_undo_stack.push(m_triggers);
    for (auto & t : m_undo_stack.top())
        unselect(t, false);             /* do not count this unselection    */
}

/**
 *  If the trigger undo-list has any items, the list-trigger is pushed
 *  into the redo list, the top of the undo-list is coped into the
 *  list-trigger, and then pops from the undo-list.
 */

void
triggers::pop_undo ()
{
    if (m_undo_stack.size() > 0)
    {
        m_redo_stack.push(m_triggers);
        m_triggers = m_undo_stack.top();
        m_undo_stack.pop();
    }
}

/**
 *  If the trigger redo-list has any items, the list-trigger is pushed
 *  into the undo list, the top of the redo-list is coped into the
 *  list-trigger, and then pops from the redo-list.
 */

void
triggers::pop_redo ()
{
    if (m_redo_stack.size() > 0)
    {
        m_undo_stack.push(m_triggers);
        m_triggers = m_redo_stack.top();
        m_redo_stack.pop();
    }
}

/**
 *  If playback-mode (song mode) is in force, that is, if using in-triggers
 *  and on/off triggers, this function handles that kind of playback.
 *  This is a new function for sequence::play() to call.
 *
 *  The for-loop goes through all the triggers, determining if there are
 *  trigger start/end values before the \a end_tick.  If so, then the trigger
 *  state is set to true (start only within the tick range) or false (end is
 *  within the tick range), and the trigger tick is set to start or end.  The
 *  first start or end trigger that is past the end tick cause the search to
 *  end.
 *
 *                  -------------------------------------
 *      tick_start |                                     | tick_end
 *                  -------------------------------------
 *                 start_tick     ||                     start_tick ||
 *                 end_tick       ||                     end_tick
 *
 * Song recording:
 *
 *      If we've reached a new chunk of drawn sequences in the song data, and
 *      we're not recording, unset the block on this sequence's events.
 *
 *  If the trigger state has changed, then the start/end ticks are passed back
 *  to the sequence, and the trigger offset is adjusted.
 *
 *  If we have reached a new chunk of drawn patterns in the song data, and we
 *  are not recording, then trigger unsets the playback block on this
 *  pattern's events.  The song_playback_block() function deals with live-play
 *  recording of triggers.
 *
 * \param start_tick
 *      Provides the starting tick value, and returns the modified value as a
 *      side-effect.
 *
 * \param end_tick
 *      Provides the ending tick value, and returns the modified value as a
 *      side-effect.
 *
 * \param resumenoteons
 *      Indicates what to do with notes when song-recording.
 *
 * \return
 *      Returns true if we're through playing the frame (trigger turning off),
 *      and the caller should stop the playback.
 *
 * \sideeffect
 *      -   start_tick and end_tick as noted above
 *      -   sequence::song_playback_block()
 *      -   sequence::set_playing()
 *      -   sequence::resume_note_ons()
 *      -   sequence::set_trigger_offset();
 */

bool
triggers::play
(
    midipulse & start_tick,
    midipulse & end_tick,
    bool resumenoteons
)
{
    bool result = false;                    /* turns off after frame play   */
    bool trigger_state = false;
    midipulse tick = start_tick;            /* saved for later              */
    midipulse trigger_offset = 0;
    midipulse trigger_tick = 0;
    for (auto & t : m_triggers)
    {
        /*
         *  See the song_playback_block() function note in the banner.
         */

        if (t.at_trigger_transition(start_tick, end_tick))
            m_parent.song_playback_block(false);

        midipulse trigstart = t.tick_start();
        midipulse trigend = t.tick_end();
        midipulse trigoffset = t.offset();
        if (trigstart <= end_tick)
        {
            trigger_state = true;
            trigger_tick = trigstart;
            trigger_offset = trigoffset;
        }
        if (trigend <= end_tick)
        {
            trigger_state = false;
            trigger_tick = trigend;
            trigger_offset = trigoffset;
        }
        if (trigstart > end_tick || trigend > end_tick)
            break;
    }

    /*
     * Had triggers in the slice, not equal to current state.  Therefore, it
     * is time to change the sequence trigger state.  We only change state if
     * we are not improvising (i.e. playing "Live").
     */

    bool ok = trigger_state != m_parent.playing();
    if (ok)
        ok = ! m_parent.song_playback_block();

    if (ok)
    {
        if (trigger_state)                              /* turning on       */
        {
            if (trigger_tick < m_parent.m_last_tick)
                start_tick = m_parent.m_last_tick;      /* side-effect      */
            else
                start_tick = trigger_tick;              /* side-effect      */

            m_parent.set_playing(true);

            /*
             * If triggered between a Note On and a Note Off, then play it.
             * Fixed for issue #5.
             */

            if (resumenoteons)
                m_parent.resume_note_ons(tick);
        }
        else
        {
            end_tick = trigger_tick;                    /* on, turning off  */
            result = true;                              /* done, ditto      */
        }
    }

    bool offplay = m_triggers.empty() && m_parent.playing();
    if (offplay)
        offplay = ! m_parent.song_playback_block();

    if (offplay)                                    /* stop playing     */
    {
        /*
         * Not yet sure if we should do a trigger notification here.
         */

        m_parent.set_playing(false);
    }
    m_parent.set_trigger_offset(trigger_offset);
    return result;
}

/**
 *  Adjusts the given offset by mod'ing it with m_length and adding
 *  m_length if needed, and returning the result.
 *
 * \param offset
 *      Provides the offset, mod'ed against m_length, used to adjust the
 *      offset.
 *
 * \return
 *      Returns the new offset.  However, if m_length is 0, no change is made,
 *      and the original offset is returned.
 */

midipulse
triggers::adjust_offset (midipulse offset)
{
    if (m_length > 0)
    {
        offset %= m_length;
        if (offset < 0)
            offset += m_length;
    }
    return offset;
}

/**
 *  Adds a trigger.
 *
 * \param tick
 *      Provides the tick (pulse) time at which the trigger goes on.
 *
 * \param len
 *      Provides the length of the trigger.  This value is actually calculated
 *      from the "on" value minus the "off" value read from the MIDI file.
 *
 * \param offset
 *      This value specifies the offset of the trigger.  It is a feature of
 *      the c_triggers_new that c_triggers doesn't have.  It is the third
 *      value in the trigger specification of the Seq66 MIDI file. The default
 *      value is 0.
 *
 * \param fixoffset
 *      If true, the offset parameter is modified by adjust_offset() first.
 *      We think that basically makes sure it is positive. The default is true.
 */

void
triggers::add (midipulse tick, midipulse len, midipulse offset, bool fixoffset)
{
    trigger t(tick, len, fixoffset ? adjust_offset(offset) : offset);
    for (auto ti = m_triggers.begin(); ti != m_triggers.end(); /* ++ti */)
    {
        midipulse tickstart = ti->tick_start();
        midipulse tickend = ti->tick_end();
        if (tickstart >= t.tick_start() && tickend <= t.tick_end())
        {
            unselect(*ti);                      /* adjust selection count   */
            ti = m_triggers.erase(ti);          /* inside new one? erase.   */
            continue;                           /* skip the ++ti            */
        }
        else if (tickend >= t.tick_end() && tickstart <= t.tick_end())
        {
            ti->tick_start(t.tick_end() + 1);   /* is event's end inside?   */
        }
        else if (tickend >= t.tick_start() && tickstart <= t.tick_start())
        {
            ti->tick_end(t.tick_start() - 1);   /* last start in new end    */
        }
        ++ti;                                   /* tricky code              */
    }
    m_triggers.push_front(t);
    m_triggers.sort();                          /* hmmm, another sort       */
}

/**
 *  This function examines each trigger in the trigger list.  If the given
 *  position is between the current trigger's tick-start and tick-end
 *  values, the these values are copied to the start and end parameters,
 *  respectively, and then we exit.
 *
 * \param position
 *      The position to examine.
 *
 * \param start
 *      The destination for the starting tick (m_tick_start) of the
 *      matching trigger.
 *
 * \param ender
 *      The destination for the ending tick (m_tick_end) of the
 *      matching trigger.
 *
 * \return
 *      Returns true if a trigger was found whose start/end ticks
 *      contained the position.  Otherwise, false is returned, and the
 *      start and end return parameters should not be used.
 */

bool
triggers::intersect (midipulse position, midipulse & start, midipulse & ender)
{
    for (auto & ti : m_triggers)
    {
        if (ti.tick_start() <= position && position <= ti.tick_end())
        {
            start = ti.tick_start();        /* return by reference */
            ender = ti.tick_end();          /* ditto               */
            return true;
        }
    }
    return false;
}

bool
triggers::intersect (midipulse position)
{
    for (auto & ti : m_triggers)
    {
        if (ti.tick_start() <= position && position <= ti.tick_end())
            return true;
    }
    return false;
}

/**
 *  Grows a trigger.  This function looks for the first trigger where
 *  the tickfrom parameter is between the trigger's tick-start and tick-end
 *  values.  If found then the trigger's start is moved back to tickto, if
 *  necessary, or the trigger's end is moved to tickto plus the length
 *  parameter, if necessary.
 *
 *  Then this new trigger is added, and the function breaks from the search
 *  loop.
 *
 * \param tickfrom
 *      The desired from-value back which to expand the trigger, if necessary.
 *
 * \param tickto
 *      The desired to-value towards which to expand the trigger, if necessary.
 *
 * \param len
 *      The additional length to append to tickto for the check.
 */

void
triggers::grow_trigger (midipulse tickfrom, midipulse tickto, midipulse len)
{
    for (auto & t : m_triggers)
    {
        midipulse start = t.tick_start();
        midipulse ender = t.tick_end();
        if (start <= tickfrom && tickfrom <= ender)
        {
            midipulse calcend = tickto + len - 1;
            if (tickto < start)
                start = tickto;

            if (calcend > ender)
                ender = calcend;

            add(start, ender - start + 1, t.offset());
            break;
        }
    }
}

/**
 *  Deletes the first trigger that brackets the given tick from the
 *  trigger-list.
 *
 * \param tick
 *      Provides the tick to be examined.
 */

void
triggers::remove (midipulse tick)
{
    for (auto i = m_triggers.begin(); i != m_triggers.end(); ++i)
    {
        if (i->tick_start() <= tick && tick <= i->tick_end())
        {
            unselect(*i);                       /* adjust selection count   */
            m_triggers.erase(i);
            break;
        }
    }
}

#if defined USE_TRIGGERS_FIND

triggers::List::iterator
triggers::find (midipulse tick)
{
    triggers::List::iterator result = m_triggers.end();
    for (auto i = m_triggers.begin(); i != m_triggers.end(); ++i)
    {
        if (i->tick_start() <= tick && tick <= i->tick_end())
        {
            result = i;
            break;
        }
    }
    return result;
}

#endif  // USE_TRIGGERS_FIND

/**
 *  Splits a single trigger into two triggers.  The
 *  original trigger ends 1 tick before the splittick parameter,
 *  and the new trigger starts at splittick and ends where the original
 *  trigger ended.
 *
 * \param trig
 *      Provides the original trigger, and also holds the changes made to
 *      that trigger as it is shortened, as a side-effect.
 *
 * \param splittick
 *      The position just after where the original trigger will be
 *      truncated, and the new trigger begins.
 */

bool
triggers::split (trigger & trig, midipulse splittick)
{
    midipulse new_tick_end = trig.tick_end();
    midipulse new_tick_start = splittick;
    midipulse len = new_tick_end - new_tick_start;
    bool result = len > 1;
    trig.tick_end(splittick - 1);
    if (result)
        add(new_tick_start, len + 1, trig.offset());

    return result;
}

/**
 *  Splits the first trigger that brackets the splittick parameter.  This is
 *  the first trigger where splittick is greater than L and less than R.
 *
 * \param splittick
 *      Provides the tick that must be bracketed for the split to be made.
 *
 * \return
 *      Returns true if a split was able to be made.
 */

bool
triggers::split (midipulse splittick, trigger::splitpoint splittype)
{
    bool result = false;
    for (auto & t : m_triggers)
    {
        if (t.tick_start() <= splittick && splittick <= t.tick_end())
        {
            midipulse tick = splittick;             /* snap or exact    */
            midipulse offset = 0;
            if (splittype == trigger::splitpoint::middle)
            {
                tick = (t.tick_end() - t.tick_start() + 1) / 2;
                offset = t.tick_start();
            }
            result = split(t, tick + offset);
            break;
        }
    }
    return result;
}

/**
 *  Adjusts trigger offsets to the length specified for all triggers, and undo
 *  triggers.
 *
 * \param newlength
 *      Provides the length to which to adjust the offsets.
 */

void
triggers::adjust_offsets_to_length (midipulse newlength)
{
    for (auto & t : m_triggers)
    {
        t.offset(adjust_offset(t.offset()));
        t.offset(m_length - t.offset());               /* flip */

        midipulse inverse_offset = m_length - (t.tick_start() % m_length);
        midipulse local_offset = (inverse_offset - t.offset());
        local_offset %= m_length;

        midipulse inverse_offset_new = newlength - (t.tick_start() % newlength);
        midipulse new_offset = inverse_offset_new - local_offset;

        /** COMMON CODE? **/

        t.offset(new_offset % newlength);
        t.offset(newlength - t.offset());
    }
}

/**
 *  Copies triggers to a point distant from a given tick.
 *
 * \param starttick
 *      The current location of the triggers.
 *
 * \param distance
 *      The distance away from the current location to which to copy the
 *      triggers.
 */

void
triggers::copy (midipulse starttick, midipulse distance)
{
    midipulse from_start_tick = starttick + distance;
    midipulse from_end_tick = from_start_tick + distance - 1;
    move(starttick, distance, true);
    for (auto & t : m_triggers)
    {
        midipulse tickstart = t.tick_start();
        if (tickstart >= from_start_tick && tickstart <= from_end_tick)
        {
            midipulse tickend = t.tick_end();
            trigger xt;
            xt.offset(t.offset());
            xt.tick_start(tickstart - distance);
            if (tickend <= from_end_tick)
                xt.tick_end(tickend - distance);
            else if (tickend > from_end_tick)
                xt.tick_end(from_start_tick - 1);

            xt.increment_offset(m_length - (distance % m_length));
            xt.offset(t.offset() % m_length);
            if (xt.offset() < 0)
                xt.increment_offset(m_length);

            m_triggers.push_front(xt);
        }
    }
    m_triggers.sort();
}

/**
 *  Moves triggers in the trigger-list.  There's no way to optimize this by
 *  saving tick values, as they are potentially modified at each step.
 *
 * \param starttick
 *      The current location of the triggers.
 *
 * \param distance
 *      The distance away from the current location to which to move the
 *      triggers.
 *
 * \param direction
 *      If true, the triggers are moved forward. If false, the triggers are
 *      moved backward.
 */

void
triggers::move (midipulse starttick, midipulse distance, bool direction)
{
    midipulse endtick = starttick + distance;
    for (auto i = m_triggers.begin(); i != m_triggers.end(); ++i)
    {
        if (i->tick_start() < starttick && starttick < i->tick_end())
        {
            if (direction)                              /* forward */
                split(*i, starttick);
            else                                        /* back    */
                split(*i, endtick);
        }
        if (i->tick_start() < starttick && starttick < i->tick_end())
        {
            if (direction)                              /* forward */
                split(*i, starttick);
            else                                        /* back    */
                i->tick_end(starttick - 1);
        }
        if
        (
            i->tick_start() >= starttick &&
            i->tick_end() <= endtick && ! direction
        )
        {
            unselect(*i);                       /* adjust selection count    */
            m_triggers.erase(i);
            i = m_triggers.begin();                     /* A BETTER WAY? */
        }
        if (i->tick_start() < endtick && endtick < i->tick_end())
        {
            if (! direction)                            /* forward */
                i->tick_start(endtick);
        }
    }
    for (auto & t : m_triggers)
    {
        if (direction)                                  /* forward */
        {
            if (t.tick_start() >= starttick)
            {
                midipulse added = t.tick_start() + distance;
                t.tick_start(added);
                added = t.tick_end() + distance;
                t.tick_end(added);
                added = (t.offset() + distance) % m_length;
                t.offset(added);
            }
        }
        else                                            /* back    */
        {
            if (t.tick_start() >= endtick)
            {
                midipulse deducted = t.tick_start() - distance;
                t.tick_start(deducted);
                deducted = t.tick_end() - distance;
                t.tick_end(deducted);
                deducted = (m_length - (distance % m_length)) % m_length;
                t.offset(deducted);
            }
        }
        t.offset(adjust_offset(t.offset()));
    }
}

/**
 *  Gets the selected trigger's start tick.  We guess this ends up selecting
 *  only one trigger, otherwise only the last selected one would effectively
 *  set the result.
 *
 * \return
 *      Returns the tick_start() value of the last-selected trigger.  If no
 *      triggers are selected, then midipulse(-1) is returned.
 */

midipulse
triggers::get_selected_start ()
{
    midipulse result = midipulse(-1);
    for (auto & t : m_triggers)
    {
        if (t.selected())
            result = t.tick_start();
    }
    return result;
}

/**
 *  Gets the selected trigger's end tick.
 *
 * \return
 *      Returns the tick_end() value of the last-selected trigger.  If no
 *      triggers are selected, then midipulse(-1) is returned.
 */

midipulse
triggers::get_selected_end ()
{
    midipulse result = midipulse(-1);
    for (auto & t : m_triggers)
    {
        if (t.selected())
            result = t.tick_end();
    }
    return result;
}

/**
 *  Moves selected triggers as per the given parameters.
 *
\verbatim
          mintick][0                1][maxtick
                            2
\endverbatim
 *
 *  The \a which parameter has three possible values:
 *
 *  -#  If we are moving 0 (triggers::grow::start), use first as offset.
 *  -#  If we are moving the 1 (triggers::grow::end), use the last as the offset.
 *  -#  If we are moving both, 2 (triggers::grow::move), use first as offset.
 *
 * \param tick
 *      The tick at which the trigger starts.
 *
 * \param fixoffset
 *      Set to true if the offset is to be adjusted.
 *
 * \param which
 *      Selects which movement will be done, as discussed above.
 *      See the values of the triggers::grow type.
 *
 * \return
 *      Returns true if there was room to move.  Otherwise, false is returned.
 *      We need this feature to support keystoke movement of a selected
 *      trigger in the perfroll window, and keep it from continually
 *      incrementing when there can be no more movement. This causes moving
 *      the other direction to be delayed while the accumulating movement
 *      counter is used up.  However, right now we can't rely on this result,
 *      and ignore it.  There may be no way around this minor issue.
 */

bool
triggers::move_selected (midipulse tick, bool fixoffset, grow which)
{
    bool result = true;
    midipulse mintick = 0;
    midipulse maxtick = 0x7ffffff;                          /* 0x7fffffff ? */
    auto s = m_triggers.begin();
    for (auto i = m_triggers.begin(); i != m_triggers.end(); ++i)
    {
        if (i->selected())
        {
            /*
             * Too tricky.  Beware the side-effect of incrementing i.
             */

            s = i;
            if (++i != m_triggers.end())                    /* side-effect  */
                maxtick = i->tick_start() - 1;

            midipulse deltatick = 0;
            if (which == triggers::grow::end)
            {
                midipulse ppqn_start = s->tick_start() + (m_ppqn / 8);
                deltatick = tick - s->tick_end();
                if (deltatick > 0 && tick > maxtick)
                    deltatick = maxtick - s->tick_end();

                if (deltatick < 0 && (deltatick + s->tick_end() <= ppqn_start))
                    deltatick = ppqn_start - s->tick_end();
            }
            else if (which == triggers::grow::start)
            {
                midipulse ppqn_end = s->tick_end() - (m_ppqn / 8);
                deltatick = tick - s->tick_start();
                if (deltatick < 0 && tick < mintick)
                    deltatick = mintick - s->tick_start();

                if (deltatick > 0 && (deltatick + s->tick_start() >= ppqn_end))
                    deltatick = ppqn_end - s->tick_start();
            }
            else if (which == triggers::grow::move)
            {
                deltatick = tick - s->tick_start();
                if (deltatick < 0 && tick < mintick)
                    deltatick = mintick - s->tick_start();

                if (deltatick > 0 && (deltatick + s->tick_end()) > maxtick)
                    deltatick = maxtick - s->tick_end();
            }

            /*
             * This code must be executed, even if deltatick == 0!
             * And setting result = deltatick == 0 causes some weirdness
             * in selection movement with the arrow keys in the perfroll.
             */

            if (which == triggers::grow::start || which == triggers::grow::move)
                s->increment_tick_start(deltatick);

            if (which == triggers::grow::end || which == triggers::grow::move)
                s->increment_tick_end(deltatick);

            if (fixoffset)
            {
                s->increment_offset(deltatick);
                s->offset(adjust_offset(s->offset()));
            }
            break;
        }
        else
            mintick = i->tick_end() + 1;
    }
    return result;
}

void
triggers::offset_selected (midipulse tick, grow editmode)
{
    for (auto & t : m_triggers)
    {
        if (t.selected())
        {
            if
            (
                editmode == triggers::grow::start ||
                editmode == triggers::grow::move
            )
            {
                t.increment_tick_start(tick);
            }

            if
            (
                editmode == triggers::grow::end ||
                editmode == triggers::grow::move
            )
                t.increment_tick_end(tick);

            if (editmode == triggers::grow::move)
                t.increment_offset(tick);
        }
    }
}

/**
 *  Get the ending value of the last trigger in the trigger-list.
 *
 * \return
 *      Returns the tick-end for the last trigger, if available.  Otherwise, 0
 *      is returned.
 */

midipulse
triggers::get_maximum () const
{
    midipulse result = 0;
    if (! m_triggers.empty())
        result = m_triggers.back().tick_end();

    return result;
}

/**
 *  Checks the list of triggers against the given tick.  If any
 *  trigger is found to bracket that tick, then true is returned.
 *
 * \param tick
 *      Provides the tick of interest.
 *
 * \return
 *      Returns true if a trigger is found that brackets the given tick.
 */

bool
triggers::get_state (midipulse tick) const
{
    bool result = false;
    for (const auto & t : m_triggers)
    {
        if (t.tick_start() <= tick && tick <= t.tick_end())
        {
            result = true;
            break;
        }
    }
    return result;
}

/**
 *  Selects the desired trigger.  Checks the list of triggers against the given
 *  tick.  If any trigger is found to bracket that tick, then true is returned,
 *  and the trigger is marked as selected.
 *
 * \param tick
 *      Provides the tick of interest.
 *
 * \return
 *      Returns true if a trigger is found that brackets the given tick.
 */

bool
triggers::select (midipulse tick)
{
    bool result = false;
    for (auto & t : m_triggers)
    {
        if (t.tick_start() <= tick && tick <= t.tick_end())
        {
            select(t);
            result = true;
        }
    }
    return result;
}

/**
 *  Unselects the desired trigger.  Checks the list of triggers against the
 *  given tick.  If any trigger is found to bracket that tick, then true is
 *  returned, and the trigger is marked as unselected.
 *
 * \param tick
 *      Provides the tick of interest.
 *
 * \return
 *      Returns true if a trigger is found that brackets the given tick.
 */

bool
triggers::unselect (midipulse tick)
{
    bool result = false;
    for (auto & t : m_triggers)
    {
        if (t.tick_start() <= tick && tick <= t.tick_end())
        {
            unselect(t);
            result = true;
        }
    }
    return result;
}

/**
 *  Unselects all triggers for the sequence.
 *
 * \return
 *      Always returns false.
 */

bool
triggers::unselect ()
{
    for (auto & t : m_triggers)
        unselect(t);

    return false;
}

/**
 *  Deletes the first selected trigger that is found.
 *
 * \return
 *      Returns true if a trigger was removed.
 */

bool
triggers::remove_selected ()
{
    bool result = false;
    for (auto i = m_triggers.begin(); i != m_triggers.end(); ++i)
    {
        if (i->selected())
        {
            unselect(*i);               /* this adjusts the selection count */
            m_triggers.erase(i);
            result = true;
            break;
        }
    }
    return result;
}

/**
 *  Copies the first selected trigger that is found.
 */

void
triggers::copy_selected ()
{
    for (auto & t : m_triggers)
    {
        if (t.selected())
        {
            m_clipboard = t;
            m_trigger_copied = true;
            break;
        }
    }
}

/**
 *  If there is a copied trigger, then this function grabs it from the trigger
 *  clipboard and adds it.  It pastes at the copy end. or at the paste-tick,
 *  if supplied.
 *
 * \param paste_tick
 *      Provides the optional tick at which to paste the trigger.  If not
 *      set to c_no_paste_trigger, this value is used to adjust the paste
 *      offset.
 */

void
triggers::paste (midipulse paste_tick)
{
    if (m_trigger_copied)
    {
        midipulse len = m_clipboard.tick_end() - m_clipboard.tick_start() + 1;
        if (paste_tick == c_no_paste_trigger)
        {
            add(m_clipboard.tick_end() + 1, len, m_clipboard.offset() + len);
            m_clipboard.tick_start(m_clipboard.tick_end() + 1);
            m_clipboard.tick_end(m_clipboard.tick_start() + len - 1);

            midipulse offset = m_clipboard.offset() + len;
            m_clipboard.offset(adjust_offset(offset));
        }
        else
        {
            /*
             * Set the +/- distance to paste the tick, from the start.
             */

            long offset = paste_tick - m_clipboard.tick_start();
            add(paste_tick, len, m_clipboard.offset() + offset);
            m_clipboard.tick_start(paste_tick);
            m_clipboard.tick_end(m_clipboard.tick_start() + len - 1);
            m_clipboard.increment_offset(offset);
            m_clipboard.offset(adjust_offset(m_clipboard.offset()));
            set_trigger_paste_tick(c_no_paste_trigger);         /* reset */
        }
        m_trigger_copied = false;   // NEW TRIAL EXPERIMENTAL 2019-08-10
    }
}

/**
 *  Get the next trigger in the trigger list.
 *
 * \return
 *      Returns the next trigger.  If there is none, a default trigger object
 *      is returned.
 */

trigger
triggers::next ()
{
    trigger result;
    if (m_draw_iterator != m_triggers.end())
    {
        result = *m_draw_iterator;
        ++m_draw_iterator;
    }
    return result;
}

/**
 *  Selects the given trigger and increments the count of selected triggers if
 *  appropriate.  Don't confuse this function with select(midipulse).
 *
 * \param t
 *      Provides a reference to the desired trigger.
 *
 * \param count
 *      If true, count the selection.  This can only be done in normal
 *      triggers, not triggers in the undo container.
 */

void
triggers::select (trigger & t, bool count)
{
    if (! t.selected())
    {
        t.selected(true);
        if (count)
            ++m_number_selected;
    }
}

/**
 *  Unselects the given trigger and decrements the count of selected triggers if
 *  appropriate.  Don't confuse this function with unselect(midipulse).
 *
 * \param t
 *      Provides a reference to the desired trigger.
 *
 * \param count
 *      If true, uncount the selection.  This can only be done in normal
 *      triggers, not triggers in the undo container.
 */

void
triggers::unselect (trigger & t, bool count)
{
    if (t.selected())
    {
        t.selected(false);
        if (count)
        {
            if (m_number_selected > 0)
            {
                --m_number_selected;
            }
            else
            {
                 warnprint("trigger unselect yields count error");
            }
        }
    }
}

/**
 *  Prints a list of the currently-held triggers.
 *
 * \param seqname
 *      A tag name to accompany the print-out, for the human to read.
 */

void
triggers::print (const std::string & seqname) const
{
    printf
    (
        "sequence '%s' triggers (%d selected):\n",
        seqname.c_str(), number_selected()
    );
    for (const auto & t : m_triggers)
    {
        printf
        (
            "  tick_start = %ld; tick_end = %ld; offset = %ld; selected = %s\n",
            t.tick_start(), t.tick_end(), t.offset(),
            bool_to_string(t.selected()).c_str()
        );
    }
}

std::string
triggers::to_string () const
{
    std::string result = std::to_string(count());
    result += " triggers:\n";
    for (const auto & t : m_triggers)
    {
        result += "   ";
        result += t.to_string();
        result += "\n";
    }
    return result;
}

}           // namespace seq66

/*
 * triggers.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


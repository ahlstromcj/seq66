#if ! defined SEQ66_TRIGGERS_HPP
#define SEQ66_TRIGGERS_HPP

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
 * \file          triggers.hpp
 *
 *  This module declares/defines the base class for handling
 *  triggers used with patterns/sequences.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-10-30
 * \updates       2019-08-10
 * \license       GNU GPLv2 or above
 *
 *  By segregating trigger support into its own module, the sequence class is
 *  a bit easier to understand.
 */

#include <string>
#include <list>
#include <stack>

#include "midi/midibytes.hpp"           /* seq66::midipulse alias, etc.     */

/**
 *  Indicates that there is no paste-trigger.  This is a new feature from the
 *  stazed/seq32 code.
 */

#define SEQ66_NO_PASTE_TRIGGER          (-1)

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class sequence;

/**
 *  This class hold a single trigger for a sequence object.  This class is used
 *  in playback, and instantiations of this class are contained in the triggers
 *  class defined later in this module.
 */

class trigger
{

private:

    /**
     *  Provides the starting tick for this trigger.  Also known as "tick on".
     */

    midipulse m_tick_start;

    /**
     *  Provides the ending tick for this trigger.  Also known as "tick off".
     */

    midipulse m_tick_end;

    /**
     *  Provides the offset for this trigger.  The offset indicates where the
     *  trigger is placed on the "perf roll".
     */

    midipulse m_offset;

    /**
     *  Indicates that the trigger is part of a selection.
     */

    bool m_selected;

public:

    trigger () :
        m_tick_start    (0),
        m_tick_end      (0),
        m_offset        (0),
        m_selected      (false)
    {
        // Empty body
    }

    /**
     *  This operator compares only the m_tick_start members.
     *
     * \param rhs
     *      The "right-hand side" of the less-than operation.
     *
     * \return
     *      Returns true if m_tick_start is less than rhs's.
     */

    bool operator < (const trigger & rhs)
    {
        return m_tick_start < rhs.m_tick_start;
    }

    bool is_valid () const
    {
        return m_tick_end > m_tick_start;
    }

    /**
     * \getter m_tick_end and m_tick_start.
     *      We've seen that some of the calculations of trigger length are
     *      wrong, being 1 tick less than the true length of the trigger in
     *      pulses.  This function calculates trigger length the correct way.
     */

    midipulse length () const
    {
        return m_tick_end - m_tick_start + 1;
    }

    midipulse tick_start () const
    {
        return m_tick_start;
    }

    void tick_start (midipulse s)
    {
        m_tick_start = s;
    }

    void increment_tick_start (midipulse s)
    {
        m_tick_start += s;
    }

    void decrement_tick_start (midipulse s)
    {
        m_tick_start -= s;
    }

    /**
     *  Test if the input parameters indicate we are touching a trigger
     *  transition.  If we have reached a new chunk of drawn pattern in the
     *  Song data, and we are not recording, we unset the playback-block on
     *  this sequence's events.
     *
     * \param s
     *      The starting tick.
     *
     * \param e
     *      The ending tick.
     */

    bool at_trigger_transition (midipulse s, midipulse e)
    {
        return
        (
            s == m_tick_start || e == m_tick_start ||
            s == m_tick_end   || e == m_tick_end
        );
    }

    midipulse tick_end () const
    {
        return m_tick_end;
    }

    void tick_end (midipulse e)
    {
        m_tick_end = e;
    }

    void increment_tick_end (midipulse s)
    {
        m_tick_end += s;
    }

    void decrement_tick_end (midipulse s)
    {
        m_tick_end -= s;
    }

    midipulse offset () const
    {
        return m_offset;
    }

    void offset (midipulse o)
    {
        m_offset = o;
    }

    void increment_offset (midipulse s)
    {
        m_offset += s;
    }

    void decrement_offset (midipulse s)
    {
        m_offset -= s;
    }

    bool selected () const
    {
        return m_selected;
    }

    void selected (bool s)
    {
        m_selected = s;
    }

};          // class trigger

/**
 *  The triggers class is a receptable the triggers that can be used with a
 *  sequence object.
 */

class triggers
{
    friend class midi_vector_base;
    friend class midifile;
    friend class sequence;

public:

    /**
     *  Provides an alias introduced by Stazed to make the trigger grow/move
     *  code easier to understand.
     */

    enum class grow
    {
        start  = 0,    /**< Grow the start of the trigger.         */
        end    = 1,    /**< Grow the end of the trigger.           */
        move   = 2     /**< Move the entire trigger block.         */
    };

private:

    /**
     *  Exposes the triggers type, currently needed for midi_vector_base only.
     *  We might convert to using a vector instead of a list.
     */

    using List = std::list<trigger>;

    /**
     *  Provides a stack for use with the undo/redo features of the
     *  trigger support.
     */

    using Stack = std::stack<List>;

private:

    /**
     *  Holds a reference to the parent sequence object that owns this trigger
     *  object.
     */

    sequence & m_parent;

    /**
     *  This list holds the current pattern/triggers events.
     */

    List m_triggers;

    /**
     *  Holds a count of the selected triggers, for better control over
     *  selections.
     */

    int m_number_selected;

    /**
     *  This item holds a single copied trigger, to be pasted later.
     */

    trigger m_clipboard;

    /**
     *  Handles the undo list for a series of operations on triggers.
     */

    Stack m_undo_stack;

    /**
     *  Handles the redo list for a series of operations on triggers.
     */

    Stack m_redo_stack;

    /**
     *  An iterator for cycling through the triggers during drawing.
     */

    List::iterator m_draw_iterator;

    /**
     *  Set to true if there is an active trigger in the trigger clipboard.
     */

    bool m_trigger_copied;

    /**
     *  The tick point for pasting.  Set to -1 if not in force.  This is a new
     *  feature from stazed's Seq32 project.
     */

    midipulse m_paste_tick;

    /**
     *  Holds the value of the PPQN from the parent sequence, for easy access.
     *  This should not change, but we have to set it after construction, and
     *  so we provide a setter for it, set_ppqn(), called by the sequence
     *  constructor.
     */

    int m_ppqn;

    /**
     *  Holds the value of the length from the parent sequence, for easy access.
     *  This might change, we're not yet sure.
     */

    int m_length;

public:

    triggers (sequence & parent);
    ~triggers ();

    triggers & operator = (const triggers & rhs);

    /**
     * \setter m_ppqn
     *      We have to set this value after construction for best safety.
     */

    void set_ppqn (int ppqn)
    {
        if (ppqn > 0)
            m_ppqn = ppqn;
    }

    /**
     * \setter m_length
     *      We have to set this value after construction for best safety.
     *      Also, there a chance that the length of the parent might change
     *      from time to time.  Currently, only the sequence constructor and
     *      midifile call this function.
     */

    void set_length (int len)
    {
        if (len > 0)
            m_length = len;
    }

    const List & triggerlist () const
    {
        return m_triggers;
    }

    List & triggerlist ()
    {
        return m_triggers;
    }

    int count () const
    {
        return int(m_triggers.size());
    }

    int number_selected () const
    {
        return m_number_selected;
    }

    void push_undo ();
    void pop_undo ();
    void pop_redo ();
    void print (const std::string & seqname) const;
    bool play (midipulse & starttick, midipulse & endtick, bool resume = false);
    void add
    (
        midipulse tick, midipulse len,
        midipulse offset = 0, bool adjustoffset = true
    );
    void adjust_offsets_to_length (midipulse newlen);
    void split (midipulse tick);
    void half_split (midipulse tick);
    void exact_split (midipulse tick);

    void grow_trigger (midipulse tickfrom, midipulse tickto, midipulse length);
    void remove (midipulse tick);
    bool get_state (midipulse tick) const;
    bool select (midipulse tick);
    bool unselect (midipulse tick);
    bool unselect ();
    bool intersect (midipulse position, midipulse & start, midipulse & end);
    bool intersect (midipulse position);

    bool remove_selected ();
    void copy_selected ();
    void paste (midipulse paste_tick = SEQ66_NO_PASTE_TRIGGER);
    bool move_selected
    (
        midipulse tick, bool adjustoffset,
        triggers::grow which = triggers::grow::move
    );

    midipulse get_selected_start ();
    midipulse get_selected_end ();
    midipulse get_maximum () const;
    void move (midipulse starttick, midipulse distance, bool direction);
    void copy (midipulse starttick, midipulse distance);

    /**
     *  Clears the whole list of triggers, and zeroes the number selected.
     */

    void clear ()
    {
        m_triggers.clear();
        m_number_selected = 0;
    }

    trigger next ();

    /**
     *  Sets the draw-trigger iterator to the beginning of the trigger list.
     */

    void reset_draw_trigger_marker ()
    {
        m_draw_iterator = m_triggers.begin();
    }

    void set_trigger_paste_tick (midipulse tick)
    {
        m_paste_tick = tick;
    }

    midipulse get_trigger_paste_tick () const
    {
        return m_paste_tick;
    }

private:

    midipulse adjust_offset (midipulse offset);
    void offset_selected (midipulse tick, grow editmode);
    void split (trigger & t, midipulse splittick);
    void select (trigger & t, bool count = true);
    void unselect (trigger & t, bool count = true);

};          // class triggers

}           // namespace seq66

#endif      // SEQ66_TRIGGERS_HPP

/*
 * triggers.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


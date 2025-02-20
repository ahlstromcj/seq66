#if ! defined SEQ66_EDITABLE_EVENT_HPP
#define SEQ66_EDITABLE_EVENT_HPP

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
 * \file          editable_event.hpp
 *
 *  This module declares/defines the editable_event class for operating with
 *  MIDI editable_events.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-11-28
 * \updates       2025-02-20
 * \license       GNU GPLv2 or above
 *
 *  This module extends the event class to support conversions between events
 *  and human-readable (and editable) strings.
 */

#include "midi/calculations.hpp"        /* seq66::pulses_to_string() etc.   */
#include "midi/event.hpp"               /* seq66::event                     */
#include "midi/midibytes.hpp"           /* seq66::midishort data type       */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

class editable_events;                  /* forward reference to container   */

/**
 *  Provides for the management of MIDI editable events.  It makes the
 *  following members of an event modifiable using human-readable strings:
 *
 *      -   m_timestamp
 *      -   m_status
 *      -   m_channel
 *      -   m_data[]
 *
 *  Eventually, it would be nice to be able to edit, or at least view, the
 *  SysEx events and the Meta events.  Those two will require extensions to
 *  make events out of them (SysEx is partly supported).
 *
 *  To the concepts of event, the editable_event class adds a category field
 *  and strings to represent all of these members.
 */

class editable_event final : public event
{

public:

    /**
     *  These values determine the major kind of event, which determines what
     *  types of events are possible for this editable event object.
     *  These tags are accompanied by category names in sm_category_names[].
     *  The enum values are cast to midibyte values for the purposes of using
     *  the lookup infrastructure.
     */

    enum class subgroup : midibyte
    {
        /**
         *  Indicates that the lookup needs to be done on the category names,
         *  as listed in sm_category_names[].
         */

        name,                           /* sm_category_names[]      */

        /**
         *  Indicates a channel event, with a value ranging from 0x80 through
         *  0xEF.  Some examples are note on/off, control change, and program
         *  change.  Values are looked up in sm_channel_event_names[].
         */

        channel_message,                /* sm_channel_event_names[] */

        /**
         *  Indicates a system event, with a value ranging from 0xF0 through
         *  0xFF.  Some examples are SysEx start/end, song position, and
         *  stop/start/continue/reset.  Values are looked up in
         *  sm_system_event_names[].  These values are "real" only in MIDI
         *  data coming in "over the wire".  In MIDI files, they represent
         *  Meta events.
         */

        system_message,                 /* sm_system_event_names[]  */

        /**
         *  Indicates a meta event, and there is a second value that is used
         *  to look up the name of the meta event, in sm_meta_event_names[].
         *  Meta messages are message that are stored in a MIDI file.
         *  Although they start with 0xFF, they are not to be confused with
         *  the 0xFF message that can be sent "over the wire", which denotes a
         *  Reset event.
         */

        meta_event,                     /* sm_meta_event_names[]    */

        /**
         *  Indicates a "proprietary", Seq66 event.  Indicates to look
         *  up the name of the event in sm_seqspec_event_names[].  Not sure if
         *  these kinds of events will be stored separately.
         */

        seqspec_event                   /* sm_seqspec_event_names[] */
    };

    /**
     *  Provides a code to indicate the desired timestamp format.  Three are
     *  supported.  All editable events will share the same timestamp format,
     *  but it seems good to make this a event class member, rather than
     *  something imposed from an outside static value.  We shall see.
     */

    using timestamp_format_t = enum
    {
        /**
         *  This format displays the time in "measures:beats:divisions"
         *  format, where measures and beats start at 1.  Thus, "1:1:0" is
         *  equivalent to 0 pulses or to "0:0:0.0" in normal time values.
         */

        timestamp_measures,

        /**
         *  This format displays the time in "hh:mm:second.fraction" format.
         *  The value displayed should not depend upon the internal timing
         *  parameters of the event.
         */

        timestamp_time,

        /**
         *  This format specifies a bare pulse format for the timestamp -- a
         *  long integer ranging from 0 on up.  Obviously, this representation
         *  depends on the PPQN value for the sequence holding this event.
         */

        timestamp_pulses
    };

    /**
     *  Provides a type that contains the pair of values needed for the
     *  various lookup maps that are needed to manage editable events.
     */

    using name_value_t = struct
    {
        /**
         *  ca 2023-05-02
         *  Supplements the event value with an index into a combo-box or
         *  similar list.  We cannot support every possible event_value for
         *  lookup.
         */

        int event_index;

        /**
         *  Holds a midibyte value (0x00 to 0xFF or 0x100 for end-of-list).
         *  This field can be considered a "key" value, as it is often looked
         *  up to find the event name.
         */

        midishort event_value;

        /**
         *  Holds the human-readable name for an event code or other numeric
         *  value in an array of name_value_t items.
         */

        std::string event_name;
    };

    /**
     *  Provides a type that contains the pair of values needed to get the
     *  Meta event's data length.
     */

    using meta_length_t = struct
    {
        /**
         *  Holds a midibyte value (0x00 to 0xFF).  This field has the same
         *  meaning as the event_value of the name_value_t type.
         */

        midishort event_value;

        /**
         *  Holds the length expected for the Meta event, or 0 if it does not
         *  apply to the Meta event.
         */

        midishort event_length;
    };

private:

    /**
     *  Provides a reference (pointer) to the container that holds this event.
     *  The container's "children" need to go to their "parent" to get certain
     *  (very limited) items of information.  The event doesn't own this
     *  pointer.
     */

    const editable_events * m_parent;

    /**
     *  Holds the linked event's timestamp (if applicable), for display in the
     *  event table.
     */

    midipulse m_link_time;

    /**
     *  Indicates the overall category of this event, which will be
     *  subgroup::channel_message, subgroup::system_message,
     *  subgroup::meta_event, and subgroup::seqspec_event.  The subgroup::name
     *  value is not set here, since that category is used only for looking up
     *  the human-readable form of the category.
     */

    subgroup m_category;

    /**
     *  Holds the name of the event category for this event.
     */

    std::string m_name_category;

    /**
     *  Indicates the format to display the time-stamp.  The default is to
     *  display in timestamp_measures format.
     */

    timestamp_format_t m_format_timestamp;

    /**
     *  Holds the string version of the MIDI pulse's time-stamp.
     */

    std::string m_name_timestamp;

    /**
     *  Holds the name of the status value for this event.  It will include
     *  the names of the channel messages and the system messages.  The latter
     *  includes SysEx and Meta messages.
     */

    std::string m_name_status;

    /**
     *  Holds the name of the meta message, if applicable.  If not applicable,
     *  this name will be empty.
     */

    std::string m_name_meta;

    /**
     *  If we eventually implement the editing of the Seq24/Seq66
     *  "proprietary" meta sequencer-specific events, the name of the SeqSpec
     *  will be stored here.
     */

    std::string m_name_seqspec;

    /**
     *  Holds the channel description, if applicable.
     */

    std::string m_name_channel;

    /**
     *  Holds the data description, if applicable.
     */

    std::string m_name_data;

public:

    editable_event () = default;
    editable_event (const editable_events & parent);
    editable_event
    (
        const editable_events & parent,
        const event & ev
    );
    editable_event (const editable_event & rhs) = default;
    editable_event & operator = (const editable_event & rhs) = default;

    virtual ~editable_event () override
    {
        // Empty body
    }

    virtual bool set_text (const std::string & s) override;
    virtual std::string get_text () const override;

    midipulse link_time () const
    {
        return m_link_time;
    }

    void link_time (midipulse lt)
    {
        m_link_time = lt;
    }

public:

    subgroup category () const
    {
        return m_category;
    }

    void category (subgroup c);

    const std::string & category_string () const
    {
        return m_name_category;
    }

    void category (const std::string & cs);

    const std::string & timestamp_string () const
    {
        return m_name_timestamp;
    }

    /**
     * \getter event::timestamp()
     *      Implemented to allow a uniform naming convention that is not
     *      slavish to the get/set crowd [this ain't Java or, chuckle, C#].
     */

    midipulse timestamp () const
    {
        return event::timestamp();
    }

    void timestamp (midipulse ts);
    void timestamp (const std::string & ts_string);

    /**
     *  Converts the current time-stamp to a string representation in units of
     *  pulses.
     */

    std::string time_as_pulses ()
    {
        return pulses_to_string(timestamp());
    }

    std::string time_as_measures ();
    std::string time_as_minutes ();
    void set_status_from_string
    (
        const std::string & ts,
        const std::string & s,
        const std::string & sd0,
        const std::string & sd1,
        const std::string & ch      = "",
        const std::string & text    = ""
    );
    void modify_channel_status_from_string
    (
        const std::string & sd0,
        const std::string & sd1,
        const std::string & chan
    );
    std::string format_timestamp ();
    std::string stock_event_string ();
    std::string ex_data_string () const;
    std::string ex_text_string () const;

    std::string status_string () const
    {
        return m_name_status;
    }

    std::string meta_string () const
    {
        return m_name_meta;
    }

    std::string seqspec_string () const
    {
        return m_name_seqspec;
    }

    std::string channel_string () const
    {
        return m_name_channel;
    }

    std::string data_string () const
    {
        return m_name_data;
    }

    void analyze ();

    static std::string category_name (int index);
    static std::string channel_event_name (int index);
    static std::string system_event_name (int index);
    static std::string meta_event_name (int index);
    static std::string seqspec_event_name (int index);

    static int channel_event_index (const std::string & name);

private:

    const editable_events * parent () const
    {
        return m_parent;
    }

    static std::string value_to_name (midibyte value, subgroup cat);
    static midishort name_to_value (const std::string & name, subgroup cat);
    static midishort meta_event_length (midibyte value);

};          // class editable_event

/*
 *  Free functions in the seq66 namespace.
 */

extern std::string time_signature_string (int n, int d, int c = 24, int b = 8);
extern bool time_signature_bytes
(
    const std::string & text,
    midibytes & timesigbytes
);
extern std::string sysex_string (const event::sysex & s);
extern bool sysex_bytes
(
    const std::string & text,
    event::sysex & sxbytes
);

}           // namespace seq66

#endif      // SEQ66_EDITABLE_EVENT_HPP

/*
 * editable_event.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


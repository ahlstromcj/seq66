#if ! defined SEQ66_MIDICONTROL_HPP
#define SEQ66_MIDICONTROL_HPP

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
 * \file          midicontrol.hpp
 *
 *  This module declares/defines the class for handling MIDI control data for
 *  the application.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-09
 * \updates       2021-02-11
 * \license       GNU GPLv2 or above
 *
 *  This module defines a number of constants relating to control of pattern
 *  unmuting, group control, and a number of additional controls to make
 *  Seq66 controllable without a graphical user interface.
 *
 *  It requires C++11 and above.
 *
 * Concept:
 * Status bits:
 *
 *  See the top banner in the automation.cpp module.
 */

#include "ctrl/keycontrol.hpp"          /* seq66::keycontrol class          */
#include "midi/event.hpp"               /* seq66::event class               */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  This class contains the MIDI control information for sequences that make
 *  up a live set.  It defines a single MIDI control.  It is derived from
 *  keycontrol so that we can store a whole control section stanza, including
 *  the key name, in one configuration stanza.
 *
 *  Note that, although we've converted this to a full-fledged class, the
 *  ordering of variables and the data arrays used to fill them is very
 *  significant.  See the midifile and optionsfile modules.
 */

class midicontrol : public keycontrol
{

public:

    /**
     *  Provides a key for looking up a MIDI control in the midicontainer.
     *  When doing a lookup, the status and first data byte must match. Once
     *  found, if the minimum and maximum byte values are not 0, then the
     *  range is also checked.  This object is easily filled via
     *  event::get_status() and event::get_data().  We might provide an
     *  event-parameter constructor.
     */

    class key
    {
        friend class midicontrol;

    private:

        midibyte m_status;  /**< Provides the (incoming) event type.    */
        midibyte m_d0;      /**< Provides the first byte, for searches. */
        midibyte m_d1;      /**< Provides the second byte, for ranging. */

    public:

        key (midibyte status, midibyte d0, midibyte d1 = 0) :
            m_status (status),
            m_d0     (d0),
            m_d1     (d1)
        {
            // no code
        }

        key (const event & ev) :
            m_status (ev.get_status()),
            m_d0     (0),
            m_d1     (0)
        {
            ev.get_data(m_d0, m_d1);
        }

        bool operator < (const key & rhs) const
        {
            return (m_status == rhs.m_status) ?
                (m_d0 < rhs.m_d0) : (m_status < rhs.m_status) ;
        }

        midibyte status () const
        {
            return m_status;
        }

        midibyte d0 () const
        {
            return m_d0;
        }

        midibyte d1 () const
        {
            return m_d1;
        }

    };      // nested class key

private:

    /**
     *  Provides the value for active.  If false, this control will be
     *  ignored.
     */

    bool m_active;

    /**
     *  Provides the value for inverse-active.
     */

    bool m_inverse_active;

    /**
     *  Provides the value for the status.  Big question is, is the channel
     *  included here?  Yes.  So the next question is, is it ignored?  We
     *  don't think so.
     */

    midibyte m_status;

    /**
     *  Provides the value for the first data byte of the event, d0.
     *  Useful for searches and for incoming data.
     */

    midibyte m_d0;

    /**
     *  Provides the second data byte, d1.  Useful only with exact matches.
     *  TBD.
     */

    midibyte m_d1;

    /**
     *  Provides the minimum value for the second data byte of the event, d1,
     *  if applicable.
     */

    midibyte m_min_value;

    /**
     *  Provides the maximum value for the second data byte of the event, d1,
     *  if applicable.
     */

    midibyte m_max_value;

public:

    /*
     *  A default constructor is needed to provide a dummy object to return
     *  when the desired one cannot be found.  The opcontrol::is_usable()
     *  function will return false.
     */

    midicontrol ();

    /*
     * The move and copy constructors, the move and copy assignment operators,
     * and the destructors are all compiler generated.
     */

    midicontrol
    (
        const std::string & keyname,
        automation::category opcategory,
        automation::action actioncode,
        automation::slot opnumber,
        int opcode
    );

    midicontrol (const midicontrol &) = default;
    midicontrol & operator = (const midicontrol &) = default;
    midicontrol (midicontrol &&) = default;
    midicontrol & operator = (midicontrol &&) = default;
    virtual ~midicontrol () = default;

    bool active () const
    {
        return m_active;
    }

    bool inverse_active () const
    {
        return m_inverse_active;
    }

    int status () const
    {
        return m_status;
    }

    int d0 () const
    {
        return m_d0;
    }

    int d1 () const
    {
        return m_d1;
    }

    int min_value () const
    {
        return m_min_value;
    }

    int max_value () const
    {
        return m_max_value;
    }

    /*
     *  This test does not include "inverse".
     */

    bool blank () const
    {
        return
        (
            ! m_active && m_status == 0 && m_d0 == 0 &&
            m_d1 == 0 && m_min_value == 0 && m_max_value == 0
        );
    }

    void set (int values [automation::SUBCOUNT]);

    /**
     *  Handles a common check in the perform module.
     *
     * \param status
     *      Provides the status byte, which is checked against m_status.
     *
     * \param d0
     *      Provides the data byte, which is checked against m_d0.
     */

    bool match (midibyte status, midibyte d0) const
    {
        return
        (
            m_active && (status == m_status) && (d0 == m_d0)
        );
    }

    /**
     *  Handles a common check in the perform module.
     */

    bool in_range (midibyte d1) const
    {
        return d1 >= midibyte(m_min_value) && d1 <= midibyte(m_max_value);
    }

    /**
     *  Another version for quick key comparison.
     */

    bool in_range (const key & k) const
    {
        return in_range(k.m_d1);
    }

public:

    key make_key () const
    {
        return key(m_status, m_d0);     /* no usage of m_d1 needed here */
    }

    bool merge_key_match (automation::category c, int opslot) const;
    void show (bool add_newline = true) const;

};              // class midicontrol

}               // namespace seq66

#endif          // SEQ66_MIDICONTROL_HPP

/*
 * midicontrol.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


#if ! defined SEQ66_BUSINFO_HPP
#define SEQ66_BUSINFO_HPP

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
 * \file          businfo.hpp
 *
 *  This module declares/defines the Master MIDI Bus base class.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2016-12-31
 * \updates       2026-05-09
 * \license       GNU GPLv2 or above
 *
 *  The businfo module defines the businfo and busarray classes so that we can
 *  start avoiding arrays and explicit access to them.
 *
 *  The businfo class holds a pointer to its midibus object.
 *
 *  The busarray class holds a number of businfo classes, and two busarrays
 *  are maintained, one for input and one for output.
 */

// #include <memory>                    /* std::shared_ptr<>                */
#include <vector>                       /* for containing the bus objects   */

#include "midi/midibus_common.hpp"      /* enum class e_clock               */

namespace seq66
{
    class event;
    class midibus;

/**
 *  A new class to consolidate a number of bus-related arrays into one array.
 *  There will be in input instances and an output instance of this object
 *  contained by mastermidibus.  Inputs will be in one container, and output in
 *  another container.
 */

class businfo
{
    friend class busarray;

private:

    /**
     *  Points to an existing midibus object.
     *
     *      std::shared_ptr<midibus> m_bus;
     */

    midibus * m_bus;

    /**
     *  Indicates if the existing bus is active.
     */

    bool m_active;

    /**
     *  Indicates if the existing bus is initialized.
     */

    bool m_initialized;

    /**
     *  Clock initialization, if this businfo is stored in an output container.
     */

    e_clock m_init_clock;

    /**
     *  Input initialization, if the businfo is stored in an output container.
     */

    bool m_init_input;

public:

    businfo () = delete;
    businfo (midibus * bus);
    businfo (const businfo & rhs);
    ~businfo () = default;              // the bus pointer is self-deleting

    /**
     *  Deletes and nullifies the m_bus pointer.
     */

    void remove ()
    {
        /*
         * if (bool(m_bus))
         *    m_bus.reset();
         */
    }

    const midibus * bus () const
    {
        return m_bus;                   // .get();
    }

    midibus * bus ()
    {
        return m_bus;                   // .get();
    }

    bool active () const
    {
        return m_active;
    }

    bool initialize ();

    bool initialized () const
    {
        return m_initialized;
    }

    e_clock init_clock () const
    {
        return m_init_clock;
    }

    bool init_input () const
    {
        return m_init_input;
    }

public:

    void activate ()
    {
        m_active = m_initialized = true;
    }

    void deactivate ()
    {
        m_active = m_initialized = false;
    }

    void init_clock (e_clock clocktype);
    void init_input (bool flag);

private:

    void start ();
    void stop ();
    void continue_from (midipulse tick);
    void init_clock (midipulse tick);
    void clock (midipulse tick);
    void sysex (const event * ev);

private:

    void print () const;

};          // class businfo

}           // namespace seq66

#endif      // SEQ66_BUSINFO_HPP

/*
 * businfo.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

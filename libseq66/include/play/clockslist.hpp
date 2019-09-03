#if ! defined SEQ66_CLOCKSLIST_HPP
#define SEQ66_CLOCKSLIST_HPP

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
 * \file          clockslist.hpp
 *
 *  This module declares/defines the elements that are common to the Linux
 *  and Windows implmentations of midibus.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-12
 * \updates       2019-04-14
 * \license       GNU GPLv2 or above
 *
 *  Defines some midibus constants and the seq66::clock enumeration.  In
 *  Sequencer64, this module was called "midibus_common". Also, we use an
 *  enum class to replace the clock_e enumeration, dropping the "e_clock"
 *  from the enumeration names, and replacing them with "e_clock::".
 */

#include <vector>                       /* std::vector for e_clock values   */
#include "midi/midibus_common.hpp"      /* e_clock, etc.                    */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  A wrapper for a vector of clocks, as used in mastermidibus and the
 *  performer object.
 */

class clockslist
{
    /**
     *  Provides a port name and the associated e_clock value.  The name is
     *  populated by the sound system at startup time, and starts out empty.
     *  It is filled from the output bus_array of the mastermidibus object.
     */

    using clock = struct
    {
        e_clock out_clock;          /**<< The clock setting for this buss.  */
        std::string out_name;       /**<< The name of the output buss.      */
    };

    /**
     *  Saves the clock settings obtained from the "rc" (options) file so that
     *  they can be loaded into the mastermidibus once it is created.
     */

    std::vector<clock> m_master_clocks;

public:

    clockslist () : m_master_clocks ()
    {
        // no code
    }

    /**
     *  Clears the list of clocks.
     */

    void clear ()
    {
        m_master_clocks.clear();
    }

    /**
     *  Resizes the list.
     */

    void resize (size_t sz)
    {
        if (sz > 0)
            m_master_clocks.resize(sz);
    }

    /**
     * \getter m_master_clocks.size()
     */

    int count () const
    {
        return int(m_master_clocks.size());
    }

    /**
     *  Saves the clock settings read from the "rc" file so that they can be
	 *  passed to the mastermidibus after it is created.

     * \param clocktype
     *      The clock value read from the "rc" file.
     */

    void add (e_clock clocktype, const std::string & name)
    {
        clock cpair;
        cpair.out_clock = clocktype;
        if (! name.empty())
            cpair.out_name = name;

        m_master_clocks.push_back(cpair);
    }

    /**
     *  Sets a single clock item, if in the currently existing range.
     *  Mostly meant for use by the Options / MIDI Input tab.
     */

    bool set (bussbyte bus, e_clock clocktype)
    {
        bool result = bus < count();
        if (result)
            m_master_clocks[bus].out_clock = clocktype;

        return result;
    }

    /**
     *  Sets the name of the given buss.
     */

    void set_name (bussbyte bus, const std::string & name)
    {
        if (bus < count())
            m_master_clocks[bus].out_name = name;
    }

	/**
    * \getter m_master_bus->get_clock(bus);  DO WE NEED THIS ONE????
     */

    e_clock get (bussbyte bus) const
    {
        return bus < count() ?
            m_master_clocks[bus].out_clock : e_clock::off ; // e_clock::disabled
    }

    /**
     *  Gets the name of the given buss.
     */

    const std::string & get_name (bussbyte bus) const
    {
        static std::string s_dummy = "dummy clock!";
        return bus < count() ?  m_master_clocks[bus].out_name : s_dummy ;
    }

};

}               // namespace seq66

#endif          // SEQ66_CLOCKSLIST_HPP

/*
 * clockslist.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


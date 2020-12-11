#if ! defined SEQ66_INPUTSLIST_HPP
#define SEQ66_INPUTSLIST_HPP

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
 * \file          inputslist.hpp
 *
 *  This module declares/defines the elements that are common to the Linux
 *  and Windows implmentations of midibus.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-12
 * \updates       2020-12-10
 * \license       GNU GPLv2 or above
 *
 *  Defines the list of MIDI inputs, pulled out of the old perform module.
 */

#include <string>                       /* std::string                      */
#include <vector>                       /* std::vector for input values     */

#include "midi/midibytes.hpp"           /* bussbyte and other types         */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  A wrapper for a vector of clocks, as used in mastermidibus and the
 *  performer object.
 */

class inputslist
{
    /**
     *  Provides a port name and the associated boolean input value.  The name
     *  is populated by the sound system at startup time, and starts out empty.
     *  It is filled from the input bus_array of the mastermidibus object.
     */

    using input = struct
    {
        bool in_enabled;            /**<< The input setting for this buss.  */
        std::string in_name;        /**<< The name of the input buss.       */
    };

    /**
     *  Saves the clock settings obtained from the "rc" (options) file so that
     *  they can be loaded into the mastermidibus once it is created.
     */

    std::vector<input> m_master_inputs;

public:

    inputslist ();

    /**
     *  Clears the list of clocks.
     */

    void clear ()
    {
        m_master_inputs.clear();
    }

    /**
     *  Return the size of the list.
     */

    int count () const
    {
        return int(m_master_inputs.size());
    }

    /**
     *  Resizes the list.
     */

    void resize (size_t sz)
    {
        if (sz > 0)
            m_master_inputs.resize(sz);
    }

    void add (bool flag, const std::string & name);
    bool set (bussbyte bus, bool inputing);
    void set_name (bussbyte bus, const std::string & name);
    bool get (bussbyte bus) const;
    std::string get_name (bussbyte bus) const;

};              // class inputslist

}               // namespace seq66

#endif          // SEQ66_INPUTSLIST_HPP

/*
 * inputslist.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


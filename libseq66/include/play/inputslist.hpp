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
 * \updates       2020-12-27
 * \license       GNU GPLv2 or above
 *
 *  Defines the list of MIDI inputs, pulled out of the old perform module.
 */

#include <string>                       /* std::string                      */
#include <vector>                       /* std::vector for input values     */

#include "play/listsbase.hpp"           /* seq66::listsbase base class      */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  A wrapper for a vector of clocks, as used in mastermidibus and the
 *  performer object.
 */

class inputslist final : public listsbase
{

public:

    inputslist (bool pmflag = false) : listsbase (pmflag)
    {
        // Nothing to do
    }

    virtual ~inputslist ()
    {
        // Nothing to do
    }

    bool add
    (
        bool flag,
        const std::string & name,
        const std::string & nickname = ""
    );
    bool set (bussbyte bus, bool inputing);
    bool get (bussbyte bus) const;

};              // class inputslist

/*
 * Free functions
 */

extern inputslist & input_port_map ();
extern bool build_input_port_map (const inputslist & lb);
extern bussbyte true_input_bus (const inputslist & cl, bussbyte nominalbuss);
extern std::string input_port_name (bussbyte b, bool addnumber = false);
extern bussbyte input_port_number (bussbyte b);
extern std::string input_port_map_list ();

}               // namespace seq66

#endif          // SEQ66_INPUTSLIST_HPP

/*
 * inputslist.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


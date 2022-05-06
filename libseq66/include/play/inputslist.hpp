#if ! defined SEQ66_INPUTSLIST_HPP
#define SEQ66_INPUTSLIST_HPP

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
 * \file          inputslist.hpp
 *
 *  This module declares/defines the elements that are common to the Linux
 *  and Windows implmentations of midibus.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-12
 * \updates       2022-05-06
 * \license       GNU GPLv2 or above
 *
 *  Defines the list of MIDI inputs, pulled out of the old perform module.
 */

#include <string>                       /* std::string                      */
#include <vector>                       /* std::vector for input values     */

#include "play/portslist.hpp"           /* seq66::portslist base class      */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  A wrapper for a vector of clocks, as used in mastermidibus and the
 *  performer object.
 */

class inputslist final : public portslist
{
    friend bool build_input_port_map (const inputslist & lb);

public:

    inputslist (bool isportmap = false) : portslist (isportmap)
    {
        // Nothing to do
    }

    virtual ~inputslist () = default;

    virtual std::string io_list_lines () const override;
    virtual bool add_list_line (const std::string & line) override;
    virtual bool add_map_line (const std::string & line) override;

    bool add
    (
        int buss,
        bool flag,
        const std::string & name,
        const std::string & nickname = "",
        const std::string & alias = ""
    );
    bool set (bussbyte bus, bool inputing);
    bool get (bussbyte bus) const;

};              // class inputslist

/*
 * Free functions
 */

extern inputslist & input_port_map ();
extern bool build_input_port_map (const inputslist & lb);
extern void clear_input_port_map ();
extern void activate_input_port_map (bool flag);
extern bussbyte true_input_bus (const inputslist & cl, bussbyte nominalbuss);
#if defined USE_IOPUT_PORT_NAME_FUNCTION
extern std::string input_port_name (bussbyte b, bool addnumber = false);
#endif
extern bussbyte input_port_number (bussbyte b);
extern std::string input_port_map_list ();

}               // namespace seq66

#endif          // SEQ66_INPUTSLIST_HPP

/*
 * inputslist.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


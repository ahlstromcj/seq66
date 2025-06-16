#if ! defined SEQ66_CLOCKSLIST_HPP
#define SEQ66_CLOCKSLIST_HPP

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
 * \file          clockslist.hpp
 *
 *  This module declares/defines the elements that are common to the Linux
 *  and Windows implmentations of midibus.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-12
 * \updates       2023-06-24
 * \license       GNU GPLv2 or above
 *
 *  Defines some midibus constants and the seq66::clock enumeration.  In
 *  Sequencer64, this module was called "midibus_common". Also, we use an
 *  enum class to replace the clock_e enumeration, dropping the "e_clock"
 *  from the enumeration names, and replacing them with "e_clock::".
 */

#include "play/portslist.hpp"           /* seq66::listbase base class       */

namespace seq66
{

/**
 *  A wrapper for a vector of clocks, as used in mastermidibus and the
 *  performer object.
 */

class clockslist final : public portslist
{
    friend bool build_output_port_map (const clockslist & lb);

public:

    clockslist (bool isportmap = false) : portslist (isportmap)
    {
        // Nothing to do
    }

    virtual ~clockslist () = default;

    virtual std::string io_list_lines () const override;
    virtual bool add_list_line (const std::string & line) override;
    virtual bool add_map_line (const std::string & line) override;

    bool add
    (
        int buss,
        bool available,
        e_clock clocktype,
        const std::string & name,
        const std::string & nickname = "",
        const std::string & alias = ""
    );
    bool set (bussbyte bus, e_clock clocktype);
    e_clock get (bussbyte bus) const;

};              // class clockslist

/*
 * Free functions
 */

extern clockslist & output_port_map ();
extern bool build_output_port_map (const clockslist & lb);
extern void clear_output_port_map ();
extern void activate_output_port_map (bool flag);
extern bussbyte true_output_bus (const clockslist & cl, bussbyte nominalbuss);
#if defined USE_IOPUT_PORT_NAME_FUNCTION
extern std::string output_port_name (bussbyte b, bool addnumber = false);
#endif
extern bussbyte output_port_number (bussbyte b);
extern std::string output_port_map_list ();

}               // namespace seq66

#endif          // SEQ66_CLOCKSLIST_HPP

/*
 * clockslist.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


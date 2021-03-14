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
 * \file          clockslist.cpp
 *
 *  This module defines some of the more complex functions of the clockslist.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2020-12-10
 * \updates       2020-12-27
 * \license       GNU GPLv2 or above
 *
 */

#include "play/clockslist.hpp"          /* seq66::clockslist class          */
#include "util/strfunctions.hpp"        /* seq66::string_format() template  */

/*
 *  This namespace is not documented because it screws up the document
 *  processing done by Doxygen.
 */

namespace seq66
{

/**
 *  Default and principal constructor defined in the header.
 */

/**
 *  Saves the clock settings read from the "rc" file so that they can be
 *  passed to the mastermidibus after it is created.  Also used in the
 *  creation of a port-map, in which the port nick-name is ultimately used to
 *  look up an index into the ports actually discovered in the system.
 *
 * \param clocktype
 *      The clock value read from the "rc" file.
 *
 * \param name
 *      The full name of the port, except when a port-map is being formed.
 *      Then, this is just a string version of the buss number.  If this
 *      parameter is empty, nothing is added to the list.
 *
 * \param nickname
 *      The short name for the port, normally.  This is generally the text
 *      after the last colon in the bus/port name discovered by the system.
 *      By default, it is empty.
 *
 * \return
 *      Returns true if the item was added to the list.
 */

bool
clockslist::add
(
    e_clock clocktype,
    const std::string & name,
    const std::string & nickname
)
{
    bool result = false;
    io ioitem;
    ioitem.io_enabled = clocktype != e_clock::disabled;
    ioitem.out_clock = clocktype;
    if (! name.empty())
    {
        ioitem.io_name = name;
        if (nickname.empty())
        {
            std::string nick = extract_nickname(name);
            ioitem.io_nick_name = nick;
        }
        else
            ioitem.io_nick_name = nickname;

        m_master_io.push_back(ioitem);
        result = true;
    }
    return result;
}

/**
 *  Sets a single clock item, if in the currently existing range.
 *  Mostly meant for use by the Options / MIDI Input tab and configuration
 *  files.
 */

bool
clockslist::set (bussbyte bus, e_clock clocktype)
{
    bool result = bus < count();
    if (result)
    {
        bool enabled = clocktype != e_clock::disabled;
        m_master_io[bus].io_enabled = enabled;
        m_master_io[bus].out_clock = clocktype;
    }
    return result;
}

e_clock
clockslist::get (bussbyte bus) const
{
    return bus < count() ? m_master_io[bus].out_clock : e_clock::off ;
}

/*
 * Free functions
 */

clockslist &
output_port_map ()
{
    static clockslist s_clocks_list(true);      /* flag this as a port-map  */
    return s_clocks_list;
}

/**
 *  Gets the nominal port name for the given bus, from the internal port-map
 *  object for clocks.
 */

std::string
output_port_name (bussbyte b, bool addnumber)
{
    const clockslist & cloutref = output_port_map();
    return cloutref.get_name(b, addnumber);
}

/**
 *  Gets the port-string (e.g. "1") from the internal port-map object for
 *  clocks.
 */

bussbyte
output_port_number (bussbyte b)
{
    bussbyte result = b;
    const clockslist & cloutref = output_port_map();
    std::string nickname = cloutref.get_nick_name(b);
    if (! nickname.empty())
        result = std::stoi(nickname);

    return result;
}

/**
 *  Builds the internal clockslist which holds a simplified list of nominal
 *  outputs where the io_name field of each element is the nick-ndame of the
 *  source clockslist's element, and the io_nick_name field is the index
 *  number (starting from 0) converted to a string.
 */

bool
build_output_port_map (const clockslist & cl)
{
    bool result = cl.not_empty();
    if (result)
    {
        clockslist & cloutref = output_port_map();
        cloutref.clear();
        cloutref.active(true);
        for (int b = 0; b < cl.count(); ++b)
        {
            std::string name = std::to_string(b);
            bussbyte bb = bussbyte(b);
            result = cloutref.add(e_clock::off, cl.get_nick_name(bb), name);
            if (! result)
            {
                cloutref.clear();
                break;
            }
        }
    }
    return result;
}

/**
 *  If an output map exists and is not empty [see the output_port_map()
 *  function], this function looks up the nominal buss number in order to find
 *  the registered (in the '[midi-clocks-map]' section of the 'rc' file) name
 *  of this port. That name is then used to look up the actual buss number of
 *  that port as set up by the system according to existing MIDI equipment.
 *
 * \param cl
 *      Provides the clockslist that holds the actual existing MIDI output
 *      ports.
 *
 * \param nominalbuss
 *      Provides the buss number to be mapped to the true buss number. The
 *      nominal buss number is the number stored with each pattern in the
 *      tune, and should never change just because the set of MIDI equipment
 *      changes.
 *
 * \return
 *      If the port map exists, the looked-up port/buss number is returned. If
 *      that port cannot be found by name, then c_bussbyte_max (0xFF) is
 *      returned.  Otherwise, the nominal buss parameter is returned, which
 *      preserves the legacy behavior of the pattern buss number. Also,
 *      c_bussbyte_max will be returned if the nomimal buss is that value.
 */

bussbyte
true_output_bus (const clockslist & cl, bussbyte nominalbuss)
{
    bussbyte result = nominalbuss;
    if (! is_null_buss(result))
    {
        const clockslist & cloutref = output_port_map();
        if (cloutref.active())
        {
            std::string shortname = cloutref.port_name_from_bus(nominalbuss);
            if (shortname.empty())
                result = c_bussbyte_max;                    /* no such buss */
            else
                result = cl.bus_from_nick_name(shortname);

            if (is_null_buss(result))
            {
                std::string msg = string_format
                (
                    "true_output_bus(%d) failed for port '%s'",
                    nominalbuss, shortname.c_str()
                );
                errprint(msg);
            }
        }
    }
    return result;
}

/**
 *  Returns a string representing the two columns of the internal clocks list.
 *  It is suitable for writing to a configuration file.  Quotes are included
 *  for readability and parse-ability.
 *
\verbatim
        0   "MIDI Port 1 Through"
        1   "Jazzy MIDI Out 1"
        2   "Jazzy MIDI Out 2"
\endverbatim
 *
 * \return
 *      Returns a string like the above.  If it is empty, the output port map
 *      is empty.
 */

std::string
output_port_map_list ()
{
    const clockslist & cloutref = output_port_map();
    return cloutref.port_map_list();
}

}               // namespace seq66

/*
 * clockslist.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


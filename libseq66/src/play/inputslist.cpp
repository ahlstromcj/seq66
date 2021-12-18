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
 * \file          inputslist.cpp
 *
 *  This module defines some of the more complex functions of the inputslist.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2020-12-10
 * \updates       2021-12-18
 * \license       GNU GPLv2 or above
 *
 */

#include "play/inputslist.hpp"          /* seq66::inputslist class          */
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
 *  Saves the input settings read from the "rc" file so that they can be
 *  passed to the mastermidibus after it is created.
 *
 * \param flag
 *      Indicates in the input is enabled.
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
 *
 */

bool
inputslist::add
(
    int buss,
    bool flag,
    const std::string & name,
    const std::string & nickname,
    const std::string & alias
)
{
    bool result = buss >= 0 && ! name.empty();
    if (result)
    {
        std::string portname = next_quoted_string(name);
        if (portname.empty())                   /* was already parsed       */
            portname = name;

        io ioitem;
        ioitem.io_enabled = flag;
        ioitem.out_clock = e_clock::off;        /* not e_clock::disabled    */
        ioitem.io_name = portname;
        ioitem.io_alias = alias;
        result = listsbase::add(buss, ioitem, nickname);
    }
    return result;
}

bool
inputslist::add_list_line (const std::string & line)
{
    int pnumber;
    int pstatus;
    std::string pname;
    bool result = parse_port_line(line, pnumber, pstatus, pname);
    if (result)
    {
        bool enabled = pstatus > 0;
        result = add(pnumber, enabled, pname);
    }
    return result;
}

/**
 *  Sets a single clock item, if in the currently existing range.
 *  Mostly meant for use by the Options / MIDI Input tab and configuration
 *  files.
 */

bool
inputslist::set (bussbyte bus, bool inputing)
{
    auto it = m_master_io.find(bus);
    bool result = it != m_master_io.end();
    if (result)
    {
        it->second.io_enabled = inputing;
        it->second.out_clock = e_clock::off;
    }
    return result;
}

bool
inputslist::get (bussbyte bus) const
{
    auto it = m_master_io.find(bus);
    return it != m_master_io.end() ?  it->second.io_enabled : false ;
}

std::string
inputslist::io_list_lines () const
{
    std::string result;
    int bus = 0;
    for (const auto & iopair : m_master_io)
    {
        const io & item = iopair.second;
        int status = item.io_enabled ? 1 : 0 ;
        result += io_line(bus, status, item.io_name, item.io_alias);
        ++bus;
    }
    return result;
}

/*
 * Free functions
 */

inputslist &
input_port_map ()
{
    static inputslist s_inputs_list(true);      /* flag this as a port-map  */
    return s_inputs_list;
}

/**
 *  Gets the nominal port name for the given bus, from the internal port-map
 *  object for inputs.
 */

std::string
input_port_name (bussbyte b, bool addnumber)
{
    const inputslist & ipm = input_port_map();
    return ipm.get_name(b, addnumber);
}

/**
 *  Gets the port-string (e.g. "1") from the internal port-map object for
 *  inputs.
 */

bussbyte
input_port_number (bussbyte b)
{
    bussbyte result = b;
    const inputslist & ipm = input_port_map();
    std::string nickname = ipm.get_nick_name(b);
    if (! nickname.empty())
        result = string_to_int(nickname);

    return result;
}

/**
 *  Builds the internal inputslist which holds a simplified list of nominal
 *  inputs where the io_name field of each element is the nick-ndame of the
 *  source inputslist's element, and the io_nick_name field is the index
 *  number (starting from 0) converted to a string.
 */

bool
build_input_port_map (const inputslist & il)
{
    bool result = il.not_empty();
    if (result)
    {
        inputslist & ipm = input_port_map();
        ipm.clear();
        int bus = 0;
        for (const auto & iopair : il.master_io())
        {
            const listsbase::io & item = iopair.second;
            std::string number = std::to_string(bus);
            if (item.io_alias.empty())
                result = ipm.add(bus, true, item.io_nick_name, number);
            else
                result = ipm.add(bus, true, item.io_alias, number);

            if (! result)
            {
                ipm.clear();
                break;
            }
            ++bus;
        }
        ipm.active(result);
    }
    return result;
}

void
clear_input_port_map ()
{
    inputslist & ipm = input_port_map();
    ipm.deactivate();
}

/**
 *  If an input map exists and is not empty [see the input_port_map()
 *  function], this function looks up the nominal buss number in order to find
 *  the registered (in the '[midi-clocks-map]' section of the 'rc' file) name
 *  of this port. That name is then used to look up the actual buss number of
 *  that port as set up by the system according to existing MIDI equipment.
 *
 * \param cl
 *      Provides the clockslist that holds the actual existing MIDI input
 *      ports.
 *
 * \param seqbuss
 *      Provides the buss number to be mapped to the true buss number. The
 *      nominal buss number is the number stored with each pattern in the
 *      tune, and should never change just because the set of MIDI equipment
 *      changes.
 *
 * \return
 *      If the port map exists, the looked-up port/buss number is returned. If
 *      that port cannot be found by name, then null_buss() (0xFF) is
 *      returned.  Otherwise, the nominal buss parameter is returned, which
 *      preserves the legacy behavior of the pattern buss number.  Also,
 *      null_buss() will be returned if the nomimal buss is that value.
 */

bussbyte
true_input_bus (const inputslist & cl, bussbyte seqbuss)
{
    bussbyte result = seqbuss;
    if (! is_null_buss(result))
    {
        const inputslist & ipm = input_port_map();
        if (ipm.active())
        {
            std::string shortname = ipm.port_name_from_bus(seqbuss);
            if (shortname.empty())
            {
                std::string msg = string_format("No input buss %d", seqbuss);
                errprint(msg);
                result = null_buss();
            }
            else
            {
#if defined USE_ALIAS_IF_PRESENT
                result = cl.bus_from_alias(shortname);
                if (is_null_buss(result))
                    result = cl.bus_from_nick_name(shortname);
#else
                result = cl.bus_from_nick_name(shortname);
#endif
                if (is_null_buss(result))
                {
                    const char * sn = shortname.c_str();
                    std::string msg = string_format
                    (
                        "No input buss %d (%s)", seqbuss, sn
                    );
                    errprint(msg);
                }
            }
        }
    }
    return result;
}

/**
 *  Returns a string representing the two columns of the internal inputs list.
 *  It is suitable for writing to a configuration file.  Quotes are included
 *  for readability and parse-ability.
 *
\verbatim
        0   "MIDI Port 1 Through"
        1   "Jazzy MIDI In 1"
        2   "Jazzy MIDI In 2"
\endverbatim
 *
 * \return
 *      Returns a string like the above.  If it is empty, the input port map
 *      is empty.
 */

std::string
input_port_map_list ()
{
    const inputslist & ipm = input_port_map();
    return ipm.port_map_list(false);                /* not clock */
}

}               // namespace seq66

/*
 * inputslist.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


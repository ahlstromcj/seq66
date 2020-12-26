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
 * \file          inputslist.cpp
 *
 *  This module defines some of the more complex functions of the inputslist.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2020-12-10
 * \updates       2020-12-19
 * \license       GNU GPLv2 or above
 *
 */

#include "play/inputslist.hpp"          /* seq66::inputslist class          */

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
    bool flag,
    const std::string & name,
    const std::string & nickname
)
{
    bool result = false;
    io ioitem;
    ioitem.io_enabled = flag;
    ioitem.out_clock = e_clock::disabled;
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
inputslist::set (bussbyte bus, bool inputing)
{
    bool result = bus < count();
    if (result)
    {
        m_master_io[bus].io_enabled = inputing;
        m_master_io[bus].out_clock = e_clock::disabled;
    }
    return result;
}

bool
inputslist::get (bussbyte bus) const
{
    return bus < count() ? m_master_io[bus].io_enabled : false ;
}

/*
 * Free functions
 */

inputslist &
input_port_map ()
{
    static inputslist s_inputs_list;
    return s_inputs_list;
}

/**
 *  Gets the nominal port name for the given bus, from the internal port-map
 *  object for inputs.
 */

std::string
input_port_name (bussbyte b, bool addnumber)
{
    const inputslist & inpsref = input_port_map();
    return inpsref.get_name(b, addnumber);
}

/**
 *  Gets the port-string (e.g. "1") from the internal port-map object for
 *  inputs.
 */

bussbyte
input_port_number (bussbyte b)
{
    bussbyte result = b;
    const inputslist & inpsref = input_port_map();
    std::string nickname = inpsref.get_nick_name(b);
    if (! nickname.empty())
        result = std::stoi(nickname);

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
        inputslist & inpsref = input_port_map();
        inpsref.clear();
        for (int b = 0; b < il.count(); ++b)
        {
            std::string name = std::to_string(b);
            bussbyte bb = bussbyte(b);
            result = inpsref.add(true, il.get_nick_name(bb), name);
            if (! result)
            {
                inpsref.clear();
                break;
            }
        }
    }
    return result;
}

/**
 *  If an input map exists, looks up the actual buss name.  Otherwise, just return
 *  input buss paremater.
 */

bussbyte
true_input_bus (const inputslist & cl, bussbyte nominalbuss)
{
    bussbyte result = nominalbuss;
    const inputslist & inpsref = input_port_map();
    if (inpsref.not_empty())
    {
        for (int b = 0; b < cl.count(); ++b)
        {
            std::string shortname = inpsref.port_name_from_bus(nominalbuss);
            if (! shortname.empty())
                result = cl.bus_from_nick_name(shortname);

            if (is_null_bussbyte(result))
                continue;
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
    const inputslist & inpsref = input_port_map();
    return inpsref.port_map_list();
}

}               // namespace seq66

/*
 * inputslist.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


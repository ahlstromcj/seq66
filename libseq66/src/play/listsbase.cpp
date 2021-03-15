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
 * \file          listsbase.cpp
 *
 *  This module defines some of the more complex functions of the listsbase.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2020-12-10
 * \updates       2021-03-15
 * \license       GNU GPLv2 or above
 *
 *  The listbase provides common code for the clockslist and inputslist
 *  classes.
 */

#include <iostream>                     /* std::cout, etc.                  */
#include "play/listsbase.hpp"           /* seq66::listsbase class           */
#include "util/calculations.hpp"        /* seq66::extract_port_names()      */
#include "util/strfunctions.hpp"        /* seq66::strncompare()             */

/*
 *  This namespace is not documented because it screws up the document
 *  processing done by Doxygen.
 */

namespace seq66
{

/**
 *  Short names.
 */

static const std::string s_short_names [] =
{
    "input",
    "output",
    "in",
    "out",
    "midi input",
    "midi output",
    "midi in",
    "midi out",
    ""                                  /* empty string is a terminator     */
};

/**
 *  Looks for the name in the short-name list.
 *
 * \param portname
 *      The name to be checked.  This is the name after the colon in a
 *      "client:port" pair.
 *
 * \return
 *      Returns true if the port-name is found in the short-name list, or is
 *      empty. This is a signal to get the nick-name from the client name and the
 *      portname.
 */

static bool
detect_short_name (const std::string & portname)
{
    bool result = portname.empty();
    if (! result)
    {
        for (int i = 0; /* forever */; ++i)
        {
            std::string compared = s_short_names[i];
            if (compared.empty())
            {
                break;
            }
            else
            {
                result = strncompare(compared, portname, compared.length());
                if (result)
                    break;
            }
        }
    }
    return result;
}

/*
 *  The simple destructor defined in the header file.  A few functions
 *  included here for better debugging.
 */

listsbase::listsbase (bool pmflag) :
    m_master_io     (),
    m_is_port_map   (pmflag)
{
    // Nothing to do
}

bool
listsbase::add
(
    int buss,
    const std::string & name,
    const std::string & nickname
)
{
    bool result = buss >= 0 && ! name.empty();
    if (result)
    {
        io ioitem;
        ioitem.io_enabled = true;                   // ??????
        ioitem.out_clock = e_clock::off;
        ioitem.io_name = name;
        if (nickname.empty())
        {
            std::string nick = extract_nickname(name);
            ioitem.io_nick_name = nick;
        }
        else
            ioitem.io_nick_name = nickname;

        auto p = std::make_pair(bussbyte(buss), ioitem);
        m_master_io.insert(p);          // later, check the insertion
    }
    return result;
}

/**
 *  Parses a string of the form:
 *
 *      0   "Nickname of the Port"
 *
 *  These lines are created by output_port_map_list().  Their format is
 *  strict.  These lines are those created in the port_map_list() function.
 *
 * \return
 *      Returns true if the line started with a number, followed by text
 *      contained inside double-quotes.
 */

bool
listsbase::add_list_line (const std::string & line)
{
    bool result = false;
    std::string temp = line;
    auto lpos = temp.find_first_of("0123456789");
    if (lpos != std::string::npos)
    {
        int portnum = std::stoi(temp);
        std::string portname = next_quoted_string(temp, lpos);
        if (! portname.empty())
        {
            std::string pnum = std::to_string(portnum);
            result = add(portnum, portname, pnum);
        }
    }
    return result;
}

/**
 *  New rule:  whether input or output, a clock value of "disabled" marks the
 *  port as missing or otherwise unusable.
 */

bool
listsbase::is_disabled (bussbyte bus) const
{
    bool result = true;
    auto it = m_master_io.find(bus);
    if (it != m_master_io.end())
        result = it->second.out_clock == e_clock::disabled;

    return result;
}

void
listsbase::set_name (bussbyte bus, const std::string & name)
{
    auto it = m_master_io.find(bus);
    if (it != m_master_io.end())
    {
        std::string nick = extract_nickname(name);
        it->second.io_name = name;
        it->second.io_nick_name = nick;
    }
}

void
listsbase::set_nick_name (bussbyte bus, const std::string & name)
{
    auto it = m_master_io.find(bus);
    if (it != m_master_io.end())
        it->second.io_nick_name = name;
}

std::string
listsbase::get_name (bussbyte bus, bool addnumber) const
{
    static std::string s_dummy;
    auto it = m_master_io.find(bus);
    std::string result = it != m_master_io.end() ?
        it->second.io_name : s_dummy ;

    if (addnumber && ! result.empty())
        result = "[" + std::to_string(int(bus)) + "] " + result;

    return result;
}

std::string
listsbase::get_nick_name (bussbyte bus, bool addnumber) const
{
    static std::string s_dummy;
    auto it = m_master_io.find(bus);
    std::string result = it != m_master_io.end() ?
        it->second.io_nick_name : s_dummy ;

    if (addnumber && ! result.empty())
        result = "[" + std::to_string(int(bus)) + "] " + result;

    return result;
}

static int
count_colons (const std::string & name)
{
    int result = 0;
    for (std::string::size_type cpos = 0; ; ++cpos)
    {
        cpos = name.find_first_of(":", cpos + 1);
        if (cpos != std::string::npos)
            ++result;
        else
            break;
    }
    return result;
}

/**
 *  The nick-name of a port is roughly all the text following the last colon
 *  in the display-name [see midibase::display_name()].  It seems to be the
 *  same text whether the port name comes from ALSA or from a2jmidid when
 *  running JACK.  We don't have any MIDI hardware that JACK detects without
 *  a2jmidid.
 *
 *  QSynth has a name like the following, which breaks the algorithm and makes
 *  the space position far outside the bounds of the string.  In that case, we
 *  punt and get the whole string.  Also see extract_port_names() in the
 *  calculations module.
 *
\verbatim
        [6] 130:0 FLUID Synth (125507):Synth input port (125507:0)
\endverbatim
 *
 *  Other cases to handle:
 *
\verbatim
        "[3] 36:0 Launchpad Mini MIDI 1"
        a2j:Midi Through [14] (playback): Midi Through Port-0
\endverbatim
 *
 */

std::string
listsbase::extract_nickname (const std::string & name) const
{
    std::string result;
    if (count_colons(name) > 2)
    {
        auto cpos = name.find_first_of(":");
        auto spos = name.find_first_of(" ", cpos);
        if (spos != std::string::npos)
        {
            ++spos;
            cpos = name.find_first_of(":", cpos + 1);
            result = name.substr(spos, cpos - spos);
        }
    }
    else
    {
        auto cpos = name.find_last_of(":");
        if (cpos != std::string::npos)
        {
            ++cpos;
            if (std::isdigit(name[cpos]))
            {
                cpos = name.find_first_of(" ", cpos);
                if (cpos != std::string::npos)
                    ++cpos;
            }
            else if (std::isspace(name[cpos]))
                ++cpos;

            if (cpos == std::string::npos)
                cpos = 0;

            result = name.substr(cpos);
        }
    }
    if (detect_short_name(result))
    {
        std::string clientname, portname;
        bool extracted = extract_port_names(name, clientname, portname);
        if (extracted)
            result = clientname + ":" + portname;

        if (result == name)
            result = simplify(result);
    }
    if (result.empty())
        result = name;

    return result;
}

/**
 *  This function is used to get the buss number from the main clockslist or
 *  main inputslist, using its nick-name.
 *
 * \param nick
 *      Provides the nick-name to be looked up.  This name is obtained from
 *      the internal clockslist or (pending) inputslist by lookup given a
 *      nominal buss number.
 *
 * \return
 *      Returns the actual buss number that will be used for I/O.
 */

bussbyte
listsbase::bus_from_nick_name (const std::string & nick) const
{
    bussbyte result = c_bussbyte_max;           /* a "null", unusable value */
    for (const auto & iopair : m_master_io)
    {
        if (nick == iopair.second.io_nick_name)
        {
            result = iopair.first;              // bussbyte(b);
            break;
        }
    }
    return result;
}

/**
 *  Looks up the nick-name, which should be a string version of the nominal
 *  buss number.  Returns the port name (short name) if found in the list.
 *  This function should be used only on the internal clockslist [returned by
 *  output_port_map() in the clockslist module] or (pending) the internal
 *  inputslist.  Only these lists stored the buss number as a string.  It is a
 *  linear lookup, but the lists are short, usually a half-dozen elements.
 *
 * \param nominalbuss
 *      Provides the external, nominal buss number which is often stored in a
 *      pattern to indicate what output port is to be used.
 */

std::string
listsbase::port_name_from_bus (bussbyte nominalbuss) const
{
    std::string result;
    std::string nick = std::to_string(int(nominalbuss));
    for (const auto & iopair : m_master_io)
    {
        if (nick == iopair.second.io_nick_name)
        {
            result = iopair.second.io_name;
            break;
        }
    }
    return result;
}

/**
 *  Sets the enabled/disabled status based on the source list.  Used to
 *  prepare the lists for showing the port-map along with the status of the
 *  disabled ports.  Each port in the port-map is looked up in the given
 *  source list.  If not found, it is disabled.
 *
 *  Currently, it is assumed that the "this" here is the listsbase object
 *  returned by the input_port_map() or output_port_map() functions. Recall that
 *  its full-name is the nick-name of an actual port, and its nick-name is a
 *  string version of the port-number.  Too tricky... unless it works. :-)
 *
 * \param source
 *      The source for the statuses to be applied.  Ultimately, the sources are
 *      the clocks and inputs from the performer, provided by mastermidibase ::
 *      get_port_statuses().
 */

void
listsbase::match_up (const listsbase & source)
{
    for (auto & iopair : m_master_io)
    {
        const std::string & portname = iopair.second.io_name;   /* io_nick_name */
        const io & sourceio = source.get_io_block(portname);
        iopair.second.io_enabled = sourceio.io_enabled;
        iopair.second.out_clock = sourceio.out_clock;
    }
}

const listsbase::io &
listsbase::get_io_block (const std::string & nickname) const
{
    static bool s_needs_initing = true;
    static io s_dummy_io;
    if (s_needs_initing)
    {
        s_needs_initing = false;
        s_dummy_io.io_enabled = false;
        s_dummy_io.out_clock = e_clock::disabled;
    }

    for (const auto & iopair : m_master_io)
    {
        if (iopair.second.io_nick_name == nickname)
            return iopair.second;
    }
    return s_dummy_io;
}

std::string
listsbase::e_clock_to_string (e_clock e) const
{
    std::string result;
    switch (e)
    {
        case e_clock::disabled:     result = "Disabled";    break;
        case e_clock::off:          result = "Off";         break;
        case e_clock::pos:          result = "Pos";         break;
        case e_clock::mod:          result = "Mod";         break;
        default:                    result = "Unknown";     break;
    }
    return result;
}

std::string
listsbase::port_map_list () const
{
    std::string result;
    if (not_empty())
    {
        for (const auto & iopair : m_master_io)
        {
            std::string port = iopair.second.io_nick_name;
            std::string name = iopair.second.io_name;
            std::string temp = port + "   \"" + name + "\"\n";
            result += temp;
        }
    }
    return result;
}

std::string
listsbase::to_string (const std::string & tag) const
{
    std::string result = "I/O List: '" + tag + "'\n";
    int count = 0;
    for (const auto & iopair : m_master_io)
    {
        std::string temp = std::to_string(count) + ". ";
        temp += iopair.second.io_enabled ? "Enabled;  " : "Disabled; " ;
        temp += "Clock = " + e_clock_to_string(iopair.second.out_clock);
        temp += "\n   ";
        temp += "Name:     " + iopair.second.io_name + "\n   ";
        temp += "Nickname: " + iopair.second.io_nick_name + "\n";
        result += temp;
        ++count;
    }
    return result;
}

void
listsbase::show (const std::string & tag) const
{
    std::string listdump = to_string(tag);
    std::cout << listdump << std::endl;
}

}               // namespace seq66

/*
 * listsbase.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


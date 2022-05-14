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
 * \file          portslist.cpp
 *
 *  This module defines some of the more complex functions of the portslist.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2020-12-10
 * \updates       2022-05-14
 * \license       GNU GPLv2 or above
 *
 *  The listbase provides common code for the clockslist and inputslist
 *  classes.
 */

#include <iostream>                     /* std::cout, etc.                  */
#include <stdexcept>                    /* std::invalid_argument            */

#include "cfg/settings.hpp"             /* seq66::rc() accessor             */
#include "midi/calculations.hpp"        /* clock_ticks_from_ppqn()          */
#include "play/portslist.hpp"           /* seq66::portslist class           */
#include "util/strfunctions.hpp"        /* seq66::strncompare()             */

/*
 *  This namespace is not documented because it screws up the document
 *  processing done by Doxygen.
 */

namespace seq66
{

/**
 *  Looks for the port name in the short-name list. We are interested in
 *  seeing if it is a generic name such as "midi in".
 *
 * \param portname
 *      The name to be checked.  This is the name after the colon in a
 *      "client:port" pair.
 *
 * \return
 *      Returns true if the port-name is found in the short-name list, or is
 *      empty. This is a signal to get the nick-name from the client name and
 *      the portname.
 */

static bool
detect_short_name (const std::string & portname)
{
    static const std::string s_short_names [] =
    {
        "midi_",
        "midi ",
        "in",
        "out",
        "input",
        "output",
        ""                              /* empty string is a terminator     */
    };
    bool result = portname.empty();
    if (! result)
    {
        for (int i = 0; /* forever */; ++i)
        {
            std::string compared = s_short_names[i];
            if (compared.empty())
            {
                break;                              /* there is no match    */
            }
            else
            {
                result = strncompare(compared, portname);
                if (result)
                    break;                          /* a match was found    */
            }
        }
    }
    return result;
}

/*
 *  The simple destructor defined in the header file.  A few functions
 *  included here for better debugging.
 */

portslist::portslist (bool pmflag) :
    m_master_io     (),
    m_is_active     (false),
    m_is_port_map   (pmflag)
{
    // Nothing to do
}

void
portslist::activate (status s)
{
    m_is_active = s == status::on;
    if (s == status::cleared)
        clear();
}

bool
portslist::add
(
    int buss,
    int status,                                 /* covers bool or e_clock   */
    const std::string & name,
    const std::string & nickname,
    const std::string & alias
)
{
    bool result = buss >= 0 && ! name.empty();
    if (result)
    {
        io ioitem;
        int client, port;
        if (extract_port_pair(name, client, port))
        {
            ioitem.io_client_number = client;
            ioitem.io_port_number = port;
        }
        else
        {
            ioitem.io_client_number = (-1);
            ioitem.io_port_number = (-1);
        }
        ioitem.io_enabled = status > 0;
        ioitem.out_clock = int_to_clock(status);
        ioitem.io_name = name;
        ioitem.io_alias = alias;
        result = add(buss, ioitem, nickname);
    }
    return result;
}

bool
portslist::add
(
    int buss,
    io & ioitem,
    const std::string & nickname
)
{
    bool result = buss >= 0;
    if (result)
    {
        if (nickname.empty())
        {
            std::string nick = extract_nickname(ioitem.io_name);
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
 *  Added for clarity, convenience, and, last, but not least, cohesion.
 *  The issue is that this can mess up the clock type, so we leave that alone
 *  and rely on the boolean, which we should have been doing all along.
 *
 * \param portio
 *      The iterator to the io structure for the port.
 *
 * \param input
 *      The desired enabled status of the port.
 *
 * \return
 *      Returns true if the buss number lookup succeeded.
 */

bool
portslist::set_enabled (bussbyte bus, bool enabled)
{
    auto it = m_master_io.find(bus);
    bool result = it != m_master_io.end();
    if (result)
        it->second.io_enabled = enabled;

    return result;
}

/**
 *  New rule:  whether input or output, an input value of "disabled" marks the
 *  port as missing or otherwise unusable. This is enforced in the child
 *  classes' set() functions.  The old check has some issues, in retrospect:
 *
 *      result = it->second.out_clock == e_clock::disabled;
 */

bool
portslist::is_enabled (bussbyte bus) const
{
    bool result = false;
    auto it = m_master_io.find(bus);
    if (it != m_master_io.end())
        result = it->second.io_enabled;

    return result;
}

void
portslist::set_name (bussbyte bus, const std::string & name)
{
    auto it = m_master_io.find(bus);
    if (it != m_master_io.end())
    {
        std::string nick = extract_nickname(name);
        it->second.io_name = name;
        it->second.io_nick_name = nick;
    }
}

#if defined USE_SET_NICK_NAME

void
portslist::set_nick_name (bussbyte bus, const std::string & name)
{
    auto it = m_master_io.find(bus);
    if (it != m_master_io.end())
        it->second.io_nick_name = name;
}

#endif

void
portslist::set_alias (bussbyte bus, const std::string & alias)
{
    auto it = m_master_io.find(bus);
    if (it != m_master_io.end())
        it->second.io_alias = alias;
}

static std::string
buss_string (const std::string & name, bussbyte bus)
{
    std::string result;
    if (! name.empty())
    {
        result = "[" + std::to_string(int(bus)) + "] " + name;
    }
    return result;
}

std::string
portslist::get_name (bussbyte bus) const
{
    static std::string s_dummy;
    auto it = m_master_io.find(bus);
    std::string result = it != m_master_io.end() ?
        it->second.io_name : s_dummy ;

    return result;
}

std::string
portslist::get_nick_name (bussbyte bus, portname style) const
{
    static std::string s_dummy;
    bool addnumber = style != portname::brief;
    auto it = m_master_io.find(bus);
    std::string result = it != m_master_io.end() ?
        it->second.io_nick_name : s_dummy ;

    if (addnumber)
        result = buss_string(result, bus);

    return result;
}

std::string
portslist::get_alias (bussbyte bus, portname style) const
{
    static std::string s_dummy;
    bool addnumber = style != portname::brief;
    auto it = m_master_io.find(bus);
    std::string result = it != m_master_io.end() ?
        it->second.io_alias : s_dummy ;

    if (addnumber)
        result = buss_string(result, bus);

    return result;
}

std::string
portslist::get_pair_name (bussbyte bus) const
{
    std::string result;
    std::string name = get_name(bus);
    std::string nick = get_nick_name(bus);
    int client, port;
    bool ok = extract_port_pair(name, client, port);        /* side-effects */
    if (ok)
    {
        std::string pairdigits = std::to_string(client);
        pairdigits += ":";
        pairdigits += std::to_string(port);
        result = pairdigits + " " + nick;
    }
    else
        result = name;

    return result;
}

std::string
portslist::get_display_name (bussbyte bus, portname style) const
{
    std::string result;
    switch (style)
    {
    case portname::brief:

        result = get_nick_name(bus, style);
        break;

    case portname::pair:

        result = get_pair_name(bus);
        break;

    case portname::full:

        result = get_name(bus);
        break;

    default:

        break;
    }
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
 *  calculations module.  Another issue is that each incarnation of QSynth
 *  produces a name with a different port number.
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
portslist::extract_nickname (const std::string & name) const
{
    std::string result;
    int colons = count_colons(name);
    if (colons > 2)
    {
        if (rc().with_jack_midi())
        {
            auto cpos = name.find_last_of(":");
            ++cpos;
            if (name[cpos] == ' ')
                cpos = name.find_first_not_of(" ", cpos);

            result = name.substr(cpos);
        }
        else
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
            result = simplify(result);          /* can we call only this?? */
    }
    else
    {
        auto ppos = result.find_first_of("(");  /* happens with fluidsynth  */
        if (ppos != std::string::npos && ppos > 1)
        {
            --ppos;
            if (result[ppos] == ' ')
                --ppos;

            result = result.substr(0, ppos + 1);
        }
    }
    if (result.empty())
        result = name;

    return result;
}

bool
portslist::extract_port_pair
(
    const std::string & name,
    int & client,
    int & port
) const
{
    int colons = count_colons(name);
    bool result = colons >= 2;
    if (result)
    {
        tokenization tokens = tokenize(name);
        result = tokens.size() >= 2;
        if (result)
            result = string_to_int_pair(tokens[1], client, port, ":");
    }
    return result;
}

bussbyte
portslist::bus_from_name (const std::string & nick) const
{
    bussbyte result = null_buss();
    for (const auto & iopair : m_master_io)
    {
        if (nick == iopair.second.io_name)
        {
            result = iopair.first;
            break;
        }
    }
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
portslist::bus_from_nick_name (const std::string & nick) const
{
    bussbyte result = null_buss();
    for (const auto & iopair : m_master_io)
    {
        if (nick == iopair.second.io_nick_name)
        {
            result = iopair.first;
            break;
        }
    }
    return result;
}

bussbyte
portslist::bus_from_alias (const std::string & alias) const
{
    bussbyte result = null_buss();
    for (const auto & iopair : m_master_io)
    {
        if (alias == iopair.second.io_alias)
        {
            result = iopair.first;
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
portslist::port_name_from_bus (bussbyte nominalbuss) const
{
    std::string result;
    if (is_null_buss(nominalbuss))
    {
        result = "0xFF";
    }
    else
    {
        std::string nick = std::to_string(int(nominalbuss));
        for (const auto & iopair : m_master_io)
        {
            if (nick == iopair.second.io_nick_name)
            {
                result = iopair.second.io_name;
                break;
            }
        }
    }
    return result;
}

/**
 *  Sets the enabled/disabled status in the source port-list based on the
 *  statuses set in the port-map.  The port-map is what the user will see
 *  in the MIDI Clocks and Inputs tabs, and that is where the user will
 *  enable/disable the ports.
 *
 *  Used to prepare the lists for showing the port-map along with the status
 *  of the disabled ports.  Each port in the port-map is looked up in the
 *  given source list.  If not found, it is disabled.
 *
 *  Currently, it is assumed that the "this" here is the portslist object
 *  returned by the input_port_map() or output_port_map() functions. Recall
 *  that its full-name is the nick-name of an actual port, and its nick-name
 *  is a string version of the port-number.  Too tricky... unless it works.
 *  :-)
 *
 * \param source
 *      The source for the statuses to be applied.  Ultimately, the sources
 *      are the clocks and inputs from the performer, provided by
 *      mastermidibase :: get_port_statuses(), which gets the system ports.
 *
 * \return
 *      Returns true if all the port-map entries mapped to a source port item.
 *      If false is returned, the port-map may need to be reconstructed.
 */

void
portslist::match_system_to_map (/*const*/ portslist & source)
{
    if (is_port_map())
    {
        for (auto & iopair : m_master_io)
        {
            io & item = iopair.second;
            const std::string & portname = item.io_name;  /* nick-name */
            io & sourceio = source.io_block(portname);
            if (valid(sourceio))
            {
                sourceio.io_enabled = item.io_enabled;
                sourceio.out_clock = item.out_clock;
            }
            else
            {
                item.io_enabled = false;
                item.out_clock = e_clock::disabled;
            }
        }
    }
}

/**
 *  Given a port-name (which might be a nick-name), this function checks if
 *  the master (internal) I/O item's nick-name or alias matches the given
 *  nick-name.  If it matches, then that internal item is returned.
 *
 * Issue:
 *
 *  The [midi-clock] name and the [midi-clock-map] nick-name might be like:
 *
 *      -   "FLUID Synth (3088150):Synth input port (3088150:0)"
 *      -   "FLUID Synth (1070760)"
 *
 */

const portslist::io &
portslist::const_io_block (const std::string & nickname) const
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
        const io & item = iopair.second;
        const std::string & comparison = item.io_alias.empty() ?
            item.io_nick_name : item.io_alias ;     /* TODO */

        bool matches = contains(comparison, nickname);
        if (matches)
            return item;            /* iopair.second */
    }
    return s_dummy_io;
}

std::string
portslist::e_clock_to_string (e_clock e) const
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

/**
 *  This function is used by input_port_map_list() and output_port_map_list()
 *  in rcfile to dump the maps into the 'rc' file.
 *
 *  Compare this function to io_list_lines(). This one does not emit an alias,
 *  as port-maps don't use them.
 *
 * \param isclock
 *      Unfortunately, we need to determine the derived object (clockslist vs
 *      inputslist) with this freakin' flag.
 */

std::string
portslist::port_map_list (bool isclock) const
{
    std::string result;
    if (not_empty())
    {
        for (const auto & iopair : m_master_io)
        {
            const io & item = iopair.second;
            std::string pname = item.io_name;
            int pnumber = string_to_int(item.io_nick_name);
            int pstatus;
            if (isclock)
                pstatus = clock_to_int(item.out_clock);
            else
                pstatus = item.io_enabled ? 1 : 0 ;

            std::string tmp = io_line(pnumber, pstatus, pname);
            result += tmp;
        }
    }
    return result;
}

/**
 *  Static function to parse port lines in a unified fashion.
 */

bool
portslist::parse_port_line
(
    const std::string & line,
    int & portnumber,
    int & portstatus,
    std::string & portname
)
{
    tokenization tokens = tokenize_quoted(line);
    bool result = tokens.size() >= 3;       /* buss, status, & quoted name  */
    if (result)
    {
        int pnumber = string_to_int(tokens[0]);
        int pstatus = string_to_int(tokens[1], (-1));
        std::string pname = tokens[2];      /* next_quoted_string(line)     */
        portnumber = pnumber;
        portstatus = pstatus;
        portname   = pname;
    }
    return result;
}

/**
 *  Static function to test an io object for validity.  To be valid it must
 *  have a non-empty io_name field.
 */

bool
portslist::valid (const io & item)
{
    return ! item.io_name.empty();
}

/**
 *  This virtual base-class function writes a port line (for the 'rc' file)
 *  from a clockslist or inputslist.  The line consists of two integers,
 *  followed by the quoted port name, and optionally followed by the alias,
 *  shown as a comment.
 */

std::string
portslist::io_line
(
    int portnumber,
    int status,
    const std::string & portname,
    const std::string & portalias
) const
{
    std::string name = add_quotes(portname);
    char tmp[128];
    if (portalias.empty())
    {
        snprintf
        (
            tmp, sizeof tmp, "%2d %2d   %s\n",
            portnumber, status, name.c_str()
        );
    }
    else
    {
        snprintf
        (
            tmp, sizeof tmp, "%2d %2d   %-40s  # '%s'\n",
            portnumber, status, name.c_str(), portalias.c_str()
        );
    }
    return std::string(tmp);
}

std::string
portslist::to_string (const std::string & tag) const
{
    std::string result = "I/O List: '" + tag + "'\n";
    int count = 0;
    for (const auto & iopair : m_master_io)
    {
        const io & item = iopair.second;
        std::string temp = std::to_string(count) + ". ";
        temp += item.io_enabled ? "Enabled;  " : "Disabled; " ;
        temp += "Clock = " + e_clock_to_string(item.out_clock);
        temp += "\n   ";
        temp += "Name:     " + item.io_name + "\n  ";
        temp += "Nickname: " + item.io_nick_name + "\n  ";
        temp += "Alias:    " + item.io_alias + "\n";
        result += temp;
        ++count;
    }
    return result;
}

void
portslist::show (const std::string & tag) const
{
    std::string listdump = to_string(tag);
    std::cout << listdump << std::endl;
}

}               // namespace seq66

/*
 * portslist.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


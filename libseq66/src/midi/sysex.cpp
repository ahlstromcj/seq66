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
 *
 *  This file is derived from the sysex (.h, .c) module of gmidimonitor
 *
 *   Copyright (C) 2005,2006,2007,2008,2011
 *   Nedko Arnaudov <nedko@arnaudov.name>
 */

/**
 * \file          sysex.cpp
 *
 *  This module declares the right version of the sysex header for the
 *  current API.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2026-09-20
 * \updates       2026-09-21
 * \license       GNU GPLv2 or above
 *
 */

#include <map>                          /* std::map template                */

#include "midi/sysex.hpp"

namespace seq66
{

/**
 *  An alias.
 */

using command_name_pair = std::map<int, std::string>;

/**
 *  A helper function to replace a big switch clause.
 */

std::string
mmc_commands (int index)
{
    static const command_name_pair s_cmp
    {
        {  1, "Stop" },
        {  2, "Play" },
        {  3, "Deferred Play" },
        {  4, "Fast Forward" },
        {  5, "Rewind" },
        {  6, "Record Strobe (Punch In)" },
        {  7, "Record Exit (Punch Out)" },
        {  8, "Record Pause" },
        {  9, "Pause" },
        { 10, "Eject" },
        { 11, "Chase" },
        { 12, "Command Error Reset" },
        { 13, "Reset" },
    };
    std::string result;
    const auto p { s_cmp.find(index) };
    if (p != s_cmp.end())
        result = p->second;

    return result;
}

std::string
generic_sysex (const midibytes & mbuf)
{
    std::string result { "SYSEX of size " };
    result += std::to_string(mbuf.size());
    for (auto b : mbuf)
    {
        char tmp[8];
        snprintf(tmp, sizeof tmp, " %02X", unsigned(b));
        result += tmp;
    }
    return result;
}

/*
 * General MMC decoding, as seen at http://www.borg.com/~jglatt/tech/mmc.htm
 * and extended from "Advanced User Guide for MK-449C MIDI keyboard"
 * information.
 *
 * It has been refactored to use existing Seq66 library functions and
 * data types.
 */

std::string
decode_sysex (const midibytes & mbuf)             // size_t buffer_size,
{
    std::string result;
    if
    (
        mbuf.size() == 6 && mbuf[0] == 0xF0 && mbuf[1] == 0x7F &&
        mbuf[3] == 0x06 && mbuf[5] == 0xF7
    )
    {
        result = mmc_commands(mbuf[4]);
        if (! result.empty())
        {
            result = generic_sysex(mbuf);
        }
        else
        {
            std::string mmc { "MMC " };
            mmc += result;
            mmc += ", for ";
            if (mbuf[2] == 0x7F)
            {
                result += " all devices";
            }
            else
            {
                result += "device ";
                result += std::to_string(mbuf[2]);
            }
        }
    }
    else if
    (
        mbuf.size() == 13 && mbuf[0] == 0xF0 && mbuf[1] == 0x7F &&
        mbuf[3] == 0x06 && mbuf[4] == 0x44 && mbuf[5] == 0x06 &&
        mbuf[6] == 0x01 && mbuf[12] == 0xF7
    )
    {
        char tmp[128];
        snprintf
        (
            tmp, sizeof tmp, "MMC goto %u:%u:%u/%u:%u",
            unsigned(mbuf[7] & 0x1F), /* fps encoding */
            unsigned(mbuf[8]),
            unsigned(mbuf[9]),

            /*
             * no fps > 32, but bit 5 looks used for something.
             */

            unsigned(mbuf[10] & 0x1F),
            unsigned(mbuf[11])
        );
        result += tmp;
        switch (mbuf[7] & 0x60)
        {
        case 0:     result += ", 24 fps";           break;
        case 1:     result += ", 25 fps";           break;
        case 2:     result += ", 29.97 fps";        break;
        case 3:     result += ", 30 fps";           break;
        }
        if (mbuf[2] == 127)
        {
            result += ", for all devices";
        }
        else
        {
            result += ", for device ";
            result += std::to_string(mbuf[2]);
        }
    }
    if (result.empty())
    {
        result = generic_sysex(mbuf);
    }
    return result;
}

}           // namespace seq66

/*
 * sysex.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

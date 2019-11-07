/*
 * midicvtpp - A MIDI-text-MIDI translater
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/**
 * \file          notemapper.cpp
 *
 *    This module provides functions for advanced MIDI/text conversions.
 *
 * \library       libmidipp
 * \author        Chris Ahlstrom
 * \date          2014-04-24
 * \updates       2019-11-05
 * \version       $Revision$
 * \license       GNU GPL
 *
 */

#include <iomanip>                      /* std::setw manipulator            */
#include <iostream>                     /* std::cerr to note errors         */

#include "play/notemapper.hpp"          /* this module's functions and stuff   */

namespace seq66
{

/**
 *    Principal constructor for the notemap class.
 *
 * \param value
 *    The integer value to which the incoming (key) value is to be
 *    mapped.
 *
 * \param keyname
 *    The name of the drum note or patch represented by the key value.
 *
 * \param valuename
 *    The name of the drum note or patch represented by the integer
 *    value.
 */

notemap::notemap
(
    int devvalue,
    int gmvalue,
    const std::string & gmname
) :
    basesettings   (),
    m_dev_value    (devvalue),
    m_gm_value     (gmvalue),
    m_gm_name      (gmname),
    m_remap_count  (0)
{
    // no other code
}

/**
 *
 */

bool
notemapper::add (int devnote, int gmnote, const std::string & gmname)
{
    auto count = m_note_map.size();
    if (m_reverse)
    {
        notepair np(devnote, gmnote, gmname);
        auto p = std::make_pair(, m);
        (void) m_note_map.insert(p);
    }
    else
    {
        notepair np(gmnote, devnote, gmname);
        auto p = std::make_pair(, m);
        (void) m_note_map.insert(p);
    }

    bool result = m_note_map.size() == (sz + 1);
    if (! result)
        std::cerr << "Duplicate note pair " << gmute << std::endl;

    return result;
}

/**
 *  Looks up the device-note in the map, decides if the map is a reversed
 *  map or note, and then reconstructs the "drum" section in a string.
 */

std::string
notemapper::to_string (int devnote)
{
    std::string result;
    int gmnote;
    const std::string & gmname;
    auto noteiterator = m_note_map.find(devnote);
    if (noteiterator != m_note_map.end())
    {
        notepair & np = noteiterator.second;
        int gmnote;
        int devnote;
        if (map_reversed)
        {
            gmnote = np.dev_value();
            devnote = np.gm_value();
        }
        else
        {
            devnote = np.dev_value();
            gmnote = np.gm_value();
        }
        result = "\n[ Drum ";
        result += std::to_string(gmnote);
        result += "]\n\n";
        result += "gm-name = \"";
        result += np.gm_name();
        result += "\"\n"
        result += "gm-note = ";
        result += np.gm_value();
        result += "\n";
        result += "dev-note = ";
        result += np.dev_value();
        result += "\n";
    }
    return result;
}

/*
 * notemapper.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */


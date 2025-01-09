/*
 * This file is part of seq66.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307 USA
 */

/**
 * \file          notemapper.cpp
 *
 *    This module provides functions for advanced MIDI/text conversions.
 *
 * \library       libseq66
 * \author        Chris Ahlstrom
 * \date          2014-04-24
 * \updates       2025-01-09
 * \version       $Revision$
 * \license       GNU GPL
 *
 */

#include <iomanip>                      /* std::setw manipulator            */
#include <iostream>                     /* std::cerr to note errors         */

#include "play/notemapper.hpp"          /* this module's functions & stuff  */
#include "util/strfunctions.hpp"        /* seq66::bool_to_string()          */

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

notemapper::pair::pair
(
    int devvalue,
    int gmvalue,
    const std::string & devname,
    const std::string & gmname,
    bool reverse
) :
    m_is_reverse    (reverse),
    m_dev_value     (devvalue),
    m_gm_value      (gmvalue),
    m_dev_name      (devname),
    m_gm_name       (gmname),
    m_remap_count   (0)
{
    // no other code
}

/**
 *  Looks up the device-note in the map, decides if the map is a reversed
 *  map or note, and then reconstructs the "drum" section in a string.
 */

std::string
notemapper::pair::to_string () const
{
    std::string result;
    int gmnote;
    int devnote;
    std::string gmname;
    std::string devname;
    if (m_is_reverse)
    {
        devnote = gm_value();
        devname = gm_name();
        gmnote = dev_value();
        gmname = dev_name();
    }
    else
    {
        gmnote = gm_value();
        gmname = gm_name();
        devnote = dev_value();
        devname = dev_name();
    }
    result += "dev-name = \"";
    result += dev_name();
    result += "\"\n";
    result += "gm-name = \"";
    result += gm_name();
    result += "\"\n";
    result += "dev-note = ";
    result += std::to_string(devnote);
    result += "\n";
    result += "gm-note = ";
    result += std::to_string(gmnote);
    result += "\n";
    return result;
}

void
notemapper::pair::show () const
{
    std::cout
        << "'" << dev_name() << "' "
        << dev_value() << " --> "
        << gm_value() << " '"
        << gm_name() << "'"
        << std::endl
        ;
}

/**
 *  Default constructor for the note-mapper.
 *
 *  The default setting of dir is direction::file.
 */

notemapper::notemapper (direction dir) :
    basesettings        ("Note Mapper"),
    m_direction         (dir),
    m_mode              (false),
    m_map_type          (),
    m_note_minimum      (999),
    m_note_maximum      (0),
    m_gm_channel        (0),
    m_device_channel    (0),
    m_map_reversed      (false),
    m_note_map          (),
    m_note_array        (),
    m_is_valid          (false)
{
    if (dir != direction::file)
        map_reversed(dir == direction::reverse);

    for (int n = 0; n < c_notes_count; ++n)
        m_note_array[n] = midibyte(n);
}

bool
notemapper::add
(
    int devnote, int gmnote,
    const std::string & devname, const std::string & gmname
)
{
    bool result =
    (
        devnote >= 0 && devnote < c_notes_count &&
        gmnote >= 0 && gmnote < c_notes_count
    );
    if (result)
    {
        auto count = m_note_map.size();
        if (map_reversed())
        {
            pair np(gmnote, devnote, devname, gmname, true);    /* reversed */
            auto p = std::make_pair(gmnote, np);
            (void) m_note_map.insert(p);
            m_note_array[gmnote] = devnote;
            if (devnote < m_note_minimum)
                m_note_minimum = devnote;

            if (devnote > m_note_maximum)
                m_note_maximum = devnote;
        }
        else
        {
            pair np(devnote, gmnote, devname, gmname, false);   /* !reverse */
            auto p = std::make_pair(devnote, np);
            (void) m_note_map.insert(p);
            m_note_array[devnote] = gmnote;
            if (gmnote < m_note_minimum)
                m_note_minimum = gmnote;

            if (gmnote > m_note_maximum)
                m_note_maximum = gmnote;
        }
        result = m_note_map.size() == (count + 1);              /* clunky!  */
        if (! result)
        {
            std::cerr
                << "Duplicate note pair " << devnote << " & " << gmnote
                << std::endl
                ;
        }
    }
    else
        std::cerr << "Note-mapper note out of range" << std::endl;

    return result;
}

/**
 *  Looks up an incoming note, and, if found, returns the mapped note value.
 *
 * \param incoming
 *      The note to be remapped.
 *
 * \return
 *      Returns the mapped note, if found.  Otherwise, the original note is
 *      returned.
 */

int
notemapper::convert (int incoming) const
{
    int result = incoming;
    auto noteiterator = m_note_map.find(incoming);
    if (noteiterator != m_note_map.end())
        result = noteiterator->second.gm_value();

    return result;
}

/**
 *  Looks up the device-note in the map, decides if the map is a reversed
 *  map or note, and then reconstructs the "drum" section in a string.
 */

std::string
notemapper::to_string (int devnote) const
{
    std::string result;
    auto noteiterator = m_note_map.find(devnote);
    if (noteiterator != m_note_map.end())
    {
        const pair & np = noteiterator->second;
        int gmnote = map_reversed() ? np.dev_value() : np.gm_value();
        result = "[Drum ";
        result += std::to_string(gmnote);
        result += "]\n\n";
        result += np.to_string();
    }
    return result;
}

void
notemapper::show () const
{
    std::cout
        << "Note-map size: " << list().size() << "\n"
        << "         Type: " << map_type() << "\n"
        << "     Reversed: " << bool_to_string(map_reversed()) << "\n"
        << " Note minimum: " << note_minimum() << "\n"
        << " Note maximum: " << note_maximum() << "\n"
        << "  Dev channel: " << std::dec << device_channel() << "\n"
        << "   GM channel: " << std::dec << gm_channel() << "\n"
        << std::endl
        ;
    for (auto & np : list())
    {
        std::cout << "Key " << np.first << ": ";
        np.second.show();
    }
}

}           // namespace seq66

/*
 * notemapper.cpp
 *
 * vim: sw=4 ts=4 wm=8 et ft=cpp
 */


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
 * \file          mutegroups.cpp
 *
 *  This module declares a container for a number of optional mutegroup objects.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-12-01
 * \updates       2019-09-24
 * \license       GNU GPLv2 or above
 *
 *  The mutegroups object contains the mute-group data read from a mute-group
 *  file.  It provides the mute-groups that are available for access by the
 *  performer.
 *
 *  Do not confuse it with the mutegroupmanager, which supports the performer's
 *  processing of mute-group selections.
 */

#include <iomanip>                      /* std::setw manipulator            */
#include <iostream>                     /* std::cerr to note errors         */

#include "play/mutegroups.hpp"          /* seq66::mutegroups class          */

/*
 *  This namespace is not documented because it screws up the document
 *  processing done by Doxygen.
 */

namespace seq66
{

/**
 *  Creates an empty, default mutegroups object.
 *
 * \param rows
 *      Provides the number of virtual rows in each mutegroup.  This will match
 *
 * \param columns
 *      Provides the number of virtual columns in each mutegroup.  This will
 *      match the number of virtual columns in a screenset.
 */

mutegroups::mutegroups (int rows, int columns) :
    basesettings                (),
    m_container                 (),
    m_container_name            ("Default"),
    m_rows                      (rows),
    m_columns                   (columns),
    m_group_format_hex          (false),
    m_loaded_from_rc            (false),
    m_group_event               (false),
    m_group_error               (false),
    m_group_mode                (true),         /* see its description  */
    m_group_learn               (false),
    m_group_selected            (SEQ66_NO_MUTE_GROUP_SELECTED),
    m_group_present             (false)
{
    // no code needed
}

/**
 *  Creates an empty, named mutegroups object.  The number of mutegroup object
 *  in a mutegroups object is \a rows times \a columns.
 *
 * \param name
 *      Provides the name of the mutegroups object, sometimes useful in
 *      troubleshooting.
 *
 * \param rows
 *      Provides the number of virtual rows in each mutegroup.  This will match
 *
 * \param columns
 *      Provides the number of virtual columns in each mutegroup.  This will
 *      match the number of virtual columns in a screenset.
 *
 */

mutegroups::mutegroups (const std::string & name, int rows, int columns) :
    basesettings                (),
    m_container                 (),
    m_container_name            (name),
    m_rows                      (rows),
    m_columns                   (columns),
    m_group_format_hex          (false),
    m_loaded_from_rc            (false),
    m_group_event               (false),
    m_group_error               (false),
    m_group_mode                (true),         /* see its description  */
    m_group_learn               (false),
    m_group_present             (false)
{
    // no code needed
}

/**
 *  Creates a new mutegroup from the incoming bit vector, and adds it at the
 *  given mute-group number.
 */

bool
mutegroups::load (mutegroup::number gmute, const midibooleans & bits)
{
    bool result = gmute >= 0;
    if (result)
    {
        mutegroup m(gmute, m_rows, m_columns);
        m.set(bits);                        // there is no load(bits)
        result = add(gmute, m);
    }
    return result;
}

/**
 *  Similar to load(), but assumes an existing group that is to be modified,
 *  rather than added to the container.
 *
 * \param gmute
 *      Provides the mute-group number to be set.
 *
 * \return
 *      Returns true if the mute-group was found.
 */

bool
mutegroups::set (mutegroup::number gmute, const midibooleans & bits)
{
    auto mgiterator = m_container.find(gmute);
    bool result = mgiterator != m_container.end();
    if (result)
        mgiterator->second.set(bits);

    return result;
}

/**
 *  Copies the mute bits into a boolean container.
 *
 * \param gmute
 *      Provides the mute-group number to be obtained.
 *
 * \return
 *      Returns the contents of the mute group.  If empty, it is likely that the
 *      mute-group number is bad.
 */

midibooleans
mutegroups::get (mutegroup::number gmute)
{
    auto mgiterator = m_container.find(gmute);
    if (mgiterator != m_container.end())
    {
        return mgiterator->second.get();
    }
    else
    {
        midibooleans no_bits;
        return no_bits;
    }
}

/**
 *
 */

bool
mutegroups::add (mutegroup::number gmute, const mutegroup & m)
{
    container::size_type sz = m_container.size();
    auto p = std::make_pair(gmute, m);
    (void) m_container.insert(p);

    bool result = m_container.size() == (sz + 1);
    if (! result)
        std::cerr << "Duplicate group-mute value " << gmute << std::endl;

    return result;
}

/**
 *  \return
 *      Returns true if any of the mutegroup objects has an armed (true) status.
 */

bool
mutegroups::any () const
{
    bool result = false;
    for (const auto & m : m_container)
    {
        if (m.second.any())
        {
            result = true;
            break;
        }
    }
    return result;
}

/**
 *  An accessor to a specific mute-group.
 */

const mutegroup &
mutegroups::mute_group (mutegroup::number gmute) const
{
    static mutegroup sm_mute_group_dummy;
    const auto & cmi = m_container.find(gmute);
    return (cmi != m_container.end()) ? cmi->second : sm_mute_group_dummy;
}

/**
 *  Loads all empty mute-groups.  Useful in writing an "empty" mutegroups file.
 */

bool
mutegroups::reset_defaults ()
{
    bool result = false;
    clear();
    int count = m_rows * m_columns;
    for (int gmute = 0; gmute < count; ++gmute)
    {
        mutegroup m;
        result = add(gmute, m);
        if (! result)
            break;
    }
    return result;
}

/**
 *  Returns the mute number for the given row and column.  Remember that the
 *  layout of mutes doesn't match that of sets and sequences.
 *
 * \param row
 *      Provides the desired row, clamped to a legal value.
 *
 * \param row
 *      Provides the desired row, clamped to a legal value.
 *
 * \return
 *      Returns the calculated set value, which will range from 0 to
 *      (m_rows * m_columns) - 1 = m_set_count.
 */

mutegroup::number
mutegroups::calculate_mute (int row, int column) const
{
    if (row < 0)
        row = 0;
    else if (row >= m_rows)
        row = m_rows - 1;

    if (column < 0)
        column = 0;
    else if (column >= m_columns)
        row = m_columns - 1;

    return row + m_rows * column;
}

/**
 *  Just shows each mute-group object.  For troubleshooting, mainly.
 */

void
mutegroups::show (mutegroup::number gmute) const
{
    std::cout << "Mute-group size: " << count() << std::endl;
    if (gmute == (-1))
    {
        int index = 0;
        for (const auto & mgpair : m_container)
        {
            int g = mgpair.first;
            const mutegroup & m = mgpair.second;
            std::cout << "[" << std::setw(2) << index++ << "] " << g << ": ";
            m.show();
        }
    }
    else
    {
        auto mgiterator = m_container.find(gmute);
        bool result = mgiterator != m_container.end();
        std::cout << "Mute-group ";
        std::cout << "[" << std::setw(2) << int(gmute) << "]: ";
        if (result)
            mgiterator->second.show();
        else
            std::cout << "MISSING" << std::endl;
    }
}

}               // namespace seq66

/*
 * mutegroups.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


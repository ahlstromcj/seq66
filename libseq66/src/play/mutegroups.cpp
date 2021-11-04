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
 * \file          mutegroups.cpp
 *
 *  This module declares a container for a number of optional mutegroup objects.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-12-01
 * \updates       2021-11-04
 * \license       GNU GPLv2 or above
 *
 *  The mutegroups object contains the mute-group data read from a mute-group
 *  file.  It provides the mute-groups that are available for access by the
 *  performer.
 *
 *  Do not confuse it with the mutegroupmanager, which supports the performer's
 *  processing of mute-group selections.
 */

#include <iomanip>                      /* std::setw() manipulator          */
#include <iostream>                     /* std::cerr to note errors         */

#include "cfg/settings.hpp"             /* seq66::usr()                     */
#include "play/mutegroups.hpp"          /* seq66::mutegroups class          */

/*
 *  This namespace is not documented because it screws up the document
 *  processing done by Doxygen.
 */

namespace seq66
{

bool mutegroups::s_swap_coordinates = false;

/**
 *  Creates an empty, default mutegroups object.  The default size is 4 x 8,
 *  but this is currently the only size we will support.  32 mute-groups.
 *
 * \param rows
 *      Provides the number of virtual rows in the set of mute-groups.
 *      This value is treated like a constant.
 *
 * \param columns
 *      Provides the number of virtual columns the set of mute-groups.
 *      This value is treated like a constant.
 */

mutegroups::mutegroups (int rows, int columns) :
    basesettings                (),
    m_container                 (),
    m_container_name            ("Default"),
    m_rows                      (rows),
    m_columns                   (columns),
    m_group_format_hex          (false),
    m_loaded_from_mutes         (false),
    m_group_event               (false),
    m_group_error               (false),
    m_group_mode                (true),             /* see its description  */
    m_group_learn               (false),
    m_group_selected            (c_null_mute_group),
    m_group_present             (false),
    m_group_save                (saving::both),     /* midi or mutes files  */
    m_group_load                (loading::both),    /* midi or mutes files  */
    m_toggle_active_only        (false),
    m_strip_empty               (true),
    m_legacy_mutes              (false)
{
    s_swap_coordinates = usr().swap_coordinates();
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
 *      Provides the number of virtual rows in the set of mute-groups.
 *
 * \param columns
 *      Provides the number of virtual columns in the set of mute-groups.
 *
 */

mutegroups::mutegroups (const std::string & name, int rows, int columns) :
    basesettings                (),
    m_container                 (),
    m_container_name            (name),
    m_rows                      (rows),
    m_columns                   (columns),
    m_group_format_hex          (false),
    m_loaded_from_mutes         (false),
    m_group_event               (false),
    m_group_error               (false),
    m_group_mode                (true),         /* see its description  */
    m_group_learn               (false),
    m_group_selected            (c_null_mute_group),
    m_group_present             (false),
    m_group_save                (saving::both),
    m_group_load                (loading::both),    /* midi and mutes files */
    m_toggle_active_only        (false),
    m_legacy_mutes              (false)
{
    s_swap_coordinates = usr().swap_coordinates();
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
 *      Returns the contents of the mute group.  If empty, it is likely that
 *      the mute-group number is bad.
 */

midibooleans
mutegroups::get (mutegroup::number gmute) const
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
 *  Creates a vector of the size of the set of mutegroups, and sets the "bit"
 *  for each one that corresponds to an existing mutegroup.
 */

midibooleans
mutegroups::get_active_groups () const
{
    midibooleans result;
    result.resize(Size());
    for (const auto & mgpair : m_container)
    {
        int g = mgpair.first;   /* const mutegroup & m = mgpair.second */
        const mutegroup & m = mgpair.second;
        if (g >= 0 && g < Size())
            result[g] = midibool(m.any());
    }
    return result;
}

bool
mutegroups::add (mutegroup::number gmute, const mutegroup & m)
{
    container::size_type sz = m_container.size();
    auto p = std::make_pair(gmute, m);
    (void) m_container.insert(p);

    bool result = m_container.size() == (sz + 1);
    if (! result)
        std::cerr << "[Duplicate group " << gmute  << "]" << std::endl;

    return result;
}

bool
mutegroups::update (mutegroup::number gmute, const midibooleans & bits)
{
    mutegroup & mdestination = mute_group(gmute);
    bool result = mdestination.valid();
    if (result)
    {
        result = mdestination.set(bits);
        if (! result)
            std::cerr << "[Group " << gmute << " bits not set]" << std::endl;
    }
    else
        std::cerr << "[Group " << gmute << " not found]" << std::endl;

    return result;
}

/**
 *  \return
 *      Returns true if any of the mutegroup objects has an armed (true)
 *      status.
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

bool
mutegroups::any (mutegroup::number gmute) const
{
    return mute_group(gmute).any();
}

/**
 *  An accessor to a specific mute-group.
 */

const mutegroup &
mutegroups::mute_group (mutegroup::number gmute) const
{
    static bool s_dummy_uninitialized = true;
    static mutegroup s_mute_group_dummy;
    if (s_dummy_uninitialized)
    {
        s_dummy_uninitialized = false;
        s_mute_group_dummy.invalidate();
    }

    const auto cmi = m_container.find(gmute);
    return cmi != m_container.end() ? cmi->second : s_mute_group_dummy;
}

mutegroup &
mutegroups::mute_group (mutegroup::number gmute)
{
    static bool s_dummy_uninitialized = true;
    static mutegroup s_mute_group_dummy;
    if (s_dummy_uninitialized)
    {
        s_dummy_uninitialized = false;
        s_mute_group_dummy.invalidate();
    }

    auto mi = m_container.find(gmute);
    return mi != m_container.end() ? mi->second : s_mute_group_dummy;
}

/**
 *  Applies a mute group.  The caller, who knows the screenset, must do the
 *  unapply operation.
 */

bool
mutegroups::apply (mutegroup::number group, midibooleans & bits)
{
    auto mgiterator = list().find(clamp_group(group));
    bool result = mgiterator != list().end();
    if (result)
    {
        mutegroup & mg = mgiterator->second;
        result = mg.any();              /* ignore an inactive mute-group    */
        if (result)
        {
            bits = mg.get();
            mg.group_state(true);
            m_group_selected = group;
        }
    }
    return result;
}

/**
 *  Unapplies a mute group. If less than zero (see c_null_mute_group), then
 *  the unapply is applied (heh heh) to the current group.
 */

bool
mutegroups::unapply (mutegroup::number group, midibooleans & bits)
{
    bool result = false;
    if (group >= 0)
    {
        auto mgiterator = list().find(clamp_group(group));
        result = mgiterator != list().end();
        if (result)
        {
            mutegroup & mg = mgiterator->second;
            result = mg.any();          /* ignore an inactive mute-group    */
            if (result)
            {
                bits = mg.zeroes();
                mg.group_state(false);
                m_group_selected = c_null_mute_group;
            }
        }
    }
    else if (m_group_selected >= 0)
    {
        result = unapply(m_group_selected, bits);
    }
    return result;
}

/**
 *  If another group was selected when we enter this function, we first need
 *  to turn off its group state before setting up the currently desired group.
 *
 * An issue to solve:
 *
 *      Let's say we start up with group 1 active (a new feature for issue
 *      #27).  The MIDI file is loaded and group 1 is applied.  Now we select
 *      group 0.  At this point, we need to do what?
 */

bool
mutegroups::toggle (mutegroup::number group, midibooleans & bits)
{
    auto mgiterator = list().find(clamp_group(group));
    bool result = mgiterator != list().end();
    if (result)
    {
        if (group != m_group_selected && m_group_selected >= 0)
        {
            auto mgiterator = list().find(clamp_group(m_group_selected));
            bool result = mgiterator != list().end();
            if (result)
            {
                mutegroup & mg = mgiterator->second;
                mg.group_state(false);
            }
        }

        mutegroup & mg = mgiterator->second;
        result = mg.any();              /* ignore an inactive mute-group    */
        if (result)
        {
            bool mgnewstate = ! mg.group_state();
            bits = mgnewstate ? mg.get() : mg.zeroes() ;
            mg.group_state(mgnewstate);
            m_group_selected = mgnewstate ? group : c_null_mute_group ;
        }
    }
    return result;
}

/**
 *  Toggles a mute group to the current play-screen in an alternative way.
 *  This alternative is to disarm only the patterns that are marked as active
 *  in the mute group, leaving the other ones set to their current status.
 */

bool
mutegroups::toggle_active (mutegroup::number group, midibooleans & armedbits)
{
    auto mgiterator = list().find(clamp_group(group));
    bool result = mgiterator != list().end();
    if (result)
    {
        if (group != m_group_selected && m_group_selected >= 0)
        {
            (void) toggle_active(m_group_selected, armedbits);
        }

        mutegroup & mg = mgiterator->second;
        midibooleans mutebits = mg.get();                /* get mutes set    */
        bool active = mg.group_state();
        result = mutebits.size() == armedbits.size();
        if (result)
        {
            auto ait = armedbits.begin();
            auto mit = mutebits.begin();
            for ( ; mit != mutebits.end(); ++mit, ++ait)
            {
                if (active)
                {
                    if (*mit)                           /* on in group?     */
                        *ait = midibool(false);         /* force it off     */
                }
                else
                {
                    *ait = *mit || *ait;
                }
            }
            active = ! active;
            mg.group_state(active);
            m_group_selected = active ? group : c_null_mute_group ;
        }
    }
    return result;
}

void
mutegroups::group_learn (bool flag)
{
    if (flag)
    {
        m_group_mode = m_group_learn = true;
    }
    else
    {
        m_group_learn = false;
        m_group_selected = c_null_mute_group;
    }
}

bool
mutegroups::group_save (saving mgh)
{
    if (mgh >= saving::mutes && mgh < saving::maximum)
    {
        m_group_save = mgh;
        return true;
    }
    else
        return false;
}

bool
mutegroups::group_save (const std::string & v)
{
    if (v == "both" || v == "stomp")
        return group_save(saving::both);
    else if (v == "mutes")
        return group_save(saving::mutes);
    else if (v == "midi" || v == "preserve")
        return group_save(saving::midi);
    else
        return false;
}

bool
mutegroups::group_save (bool midi, bool mutes)
{
    if (midi && mutes)
        return group_save(saving::both);
    else if (mutes)
        return group_save(saving::mutes);
    else if (midi)
        return group_save(saving::midi);
    else
        return false;
}

/**
 * \getter m_mute_group_save, string version
 */

std::string
mutegroups::group_save_label () const
{
    std::string result = "bad";
    if (m_group_save == saving::mutes)
        result = "mutes";
    else if (m_group_save == saving::midi)
        result = "midi";
    else if (m_group_save == saving::both)
        result = "both";

    return result;
}

bool
mutegroups::group_load (loading mgh)
{
    if (mgh >= loading::none && mgh < loading::maximum)
    {
        m_group_load = mgh;
        return true;
    }
    else
        return false;
}

bool
mutegroups::group_load (const std::string & v)
{
    if (v == "none")
        return group_load(loading::none);
    else if (v == "both" || v == "stomp")
        return group_load(loading::both);
    else if (v == "mutes")
        return group_load(loading::mutes);
    else if (v == "midi" || v == "preserve")
        return group_load(loading::midi);
    else
        return false;
}

bool
mutegroups::group_load (bool midi, bool mutes)
{
    if (midi && mutes)
        return group_load(loading::both);
    else if (mutes)
        return group_load(loading::mutes);
    else if (midi)
        return group_load(loading::midi);
    else
        return group_load(loading::none);
}

/**
 * \getter m_mute_group_load, string version
 */

std::string
mutegroups::group_load_label () const
{
    std::string result = "bad";
    if (m_group_load == loading::none)
        result = "none";
    else if (m_group_load == loading::mutes)
        result = "mutes";
    else if (m_group_load == loading::midi)
        result = "midi";
    else if (m_group_load == loading::both)
        result = "both";

    return result;
}

void
mutegroups::load_mute_groups (bool midi, bool mutes)
{
    if (mutes && midi)
        m_group_load = loading::both;
    else if (mutes)
        m_group_load = loading::mutes;
    else if (midi)
        m_group_load = loading::midi;
    else
        m_group_load = loading::none;
}

bool
mutegroups::clear ()
{
    bool result = const_cast<mutegroups *>(this)->any();
    m_container.clear();
    return result;
}

/**
 *  Loads all empty mute-groups.  Useful in writing an "empty" mutegroups
 *  file.
 *
 *  Hmmm, rows x columns isn't the way to count the mute groups -- that is the
 *  size of one mute group.  Generally, we support only 32 mute-groups.
 */

bool
mutegroups::reset_defaults ()
{
    bool result = false;
    int count = c_mute_groups_max;              /* not m_rows * m_columns   */
    clear();
    for (int gmute = 0; gmute < count; ++gmute)
    {
        mutegroup m(gmute);     /* m(mutegroup::number(gmute)) bad, why?    */
        result = add(gmute, m);
        if (! result)
            break;
    }
    return result;
}

/**
 *  Returns the mute group number for the given row and column.  Remember that
 *  the layout of mutes doesn't match that of sets and sequences.  The row and
 *  column here currently match the 4 x 8 mute-group grid in the qmutemaster
 *  module.  This function is static.
 *
 * \param row
 *      Provides the desired row of the virtual mute-group grid, clamped to a
 *      legal value.
 *
 * \param column
 *      Provides the desired column of the virtual mute-group grid, clamped to
 *      a legal value.
 *
 * \return
 *      Returns the calculated mute-group value, which will range from 0 to
 *      (m_rows * m_columns) - 1 = m_set_count.
 */

mutegroup::number
mutegroups::grid_to_group (int row, int column)
{
    if (row < 0)
        row = 0;
    else if (row >= Rows())
        row = Rows() - 1;

    if (column < 0)
        column = 0;
    else if (column >= Columns())
        column = Columns() - 1;

    return Swap() ? (column + row * Columns()) : (row + column * Rows());
}

/**
 *  The "inverse" of grid_to_group().  This function is static.
 *
 *  Compare this to the non-static function mutegroup::mute_to_grid().
 */

bool
mutegroups::group_to_grid (mutegroup::number group, int & row, int & column)
{
    bool result = group >= 0 && group < Size();
    if (result)
    {
        if (Swap())
        {
            row = group / Columns();
            column = group % Columns();
        }
        else
        {
            row = group % Rows();
            column = group / Rows();
        }
    }
    return result;
}

/**
 *  Counts the letters of each mute-group name, adding 2 to account for the
 *  double-quote characters used in specifying a mute-group name.  The names
 *  are written to the 'mutes' file and to the MIDI file, but, for the latter,
 *  we have to check for the names byte-by-byte.  Also, if empty, the quotes
 *  aren't written (and counted here).
 */

int
mutegroups::group_names_letter_count () const
{
    int result = 0;
    int group = 0;
    for (const auto & mgpair : m_container)
    {
        const mutegroup & m = mgpair.second;
        const std::string & gname = m.name();
        if (! gname.empty())
            result += gname.length() + 2;

        ++group;
    }
    return result;
}

/**
 *  Just shows each mute-group object.  For troubleshooting, mainly.
 */

void
mutegroups::show (const std::string & tag, mutegroup::number gmute) const
{
    std::cout << "Mute-group " << tag << " size: " << count() << std::endl;
    if (gmute == c_null_mute_group)
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


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
 * \file          basesettings.cpp
 *
 *  This module declares/defines some basic settings items.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-01-17
 * \updates       2019-09-21
 * \license       GNU GPLv2 or above
 *
 */

#include "util/basic_macros.hpp"        /* the C++ errprint() macro         */
#include "cfg/basesettings.hpp"         /* seq66::basesettings class        */

/**
 *  Indicates the "version" of the format of the "rc" ("rc", "ctrl",
 *  "mutes", and "playlist" files.  We hope to increment this number only
 *  rarely.
 */

#define SEQ66_ORDINAL_VERSION           0

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Default constructor.
 */

basesettings::basesettings (const std::string & filename) :
    m_ordinal_version   (SEQ66_ORDINAL_VERSION),
    m_comments_block    (),                         /* [comments]   */
    m_file_name         (filename),
    m_error_message     (),
    m_is_error          ()
{
    // Empty body; it's no use to call normalize() here, see set_defaults().
}

/**
 *  Sets the default values.  For the m_midi_buses and m_instruments members,
 *  this function can only iterate over the current size of the vectors.  But
 *  the default size is zero!
 */

void
basesettings::set_defaults ()
{
    m_ordinal_version = SEQ66_ORDINAL_VERSION;
    m_file_name.clear();
    m_error_message.clear();
    m_is_error = false;
    normalize();                            // recalculate derived values
}

/**
 *  Calculate the derived values from the already-set values.  Should we
 *  normalize the BPM increment values here, in case they are irregular?
 *
 *  gmute_tracks() is viable with variable set sizes only if we stick with the
 *  32 sets by 32 patterns, at this time. It's semantic meaning is......
 *
 *  m_max_sequence is now actually a constant (1024), so we enforce that here
 *  now.
 */

void
basesettings::normalize ()
{
    // \todo
}

/**
 * \return
 *      Returns false if there is an error message in force.
 */

bool
basesettings::set_error_message (const std::string & em) const
{
    bool result = em.empty();
    if (result)
    {
        m_error_message.clear();
        m_is_error = false;
    }
    else
    {
        if (! m_error_message.empty())
            m_error_message += "; ";

        m_error_message += em;
        errprint(em);
    }
    return result;
}

}           // namespace seq66

/*
 * basesettings.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


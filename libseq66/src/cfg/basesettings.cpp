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
 * \updates       2019-01-18
 * \license       GNU GPLv2 or above
 *
 */

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

basesettings::basesettings () :
    m_ordinal_version           (SEQ66_ORDINAL_VERSION),
    m_comments_block            ()      /* [comments]                       */
{
    // Empty body; it's no use to call normalize() here, see set_defaults().
}

/**
 *  Copy constructor.
 */

basesettings::basesettings (const basesettings & rhs) :
    m_ordinal_version           (rhs.m_ordinal_version),
    m_comments_block            (rhs.m_comments_block)
{
    // Empty body; no need to call normalize() here.
}

/**
 *  Principal assignment operator.
 */

basesettings &
basesettings::operator = (const basesettings & rhs)
{
    if (this != &rhs)
    {
        m_ordinal_version       = rhs.m_ordinal_version;
        m_comments_block        = rhs.m_comments_block;
    }
    return *this;
}

/**
 *  Sets the default values.  For the m_midi_buses and m_instruments members,
 *  this function can only iterate over the current size of the vectors.  But
 *  the default size is zero!
 */

void
basesettings::set_defaults ()
{
    m_ordinal_version           = SEQ66_ORDINAL_VERSION;

    /*
     * m_comments_block.clear();
     */

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

}           // namespace seq66

/*
 * basesettings.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


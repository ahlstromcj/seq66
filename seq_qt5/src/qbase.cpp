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
 * \file          qbase.cpp
 *
 *  This module declares/defines the base class for managing editing.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-07-22
 * \updates       2023-09-09
 * \license       GNU GPLv2 or above
 *
 *  We eventually want to migrate all user-interface widgets so that they
 *  ultimately use this base class.  Then we aim to greatly reduce the use of
 *  timers to update the widgets.
 */

#include "cfg/settings.hpp"             /* seq66::usr()                     */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "qbase.hpp"

/*
 *  Do not document the name space.
 */

namespace seq66
{

/**
 *  Sets initial values for common Qt window/frames.
 *
 * \param p
 *      Provides the performer object to use for interacting with this
 *      sequence.  Among other things, this object provides the active PPQN.
 */

qbase::qbase (performer & p) :
    m_performer             (p),
    m_is_dirty              (false),
    m_is_initialized        (false)
{
    // No code needed
}

/**
 *  This destructor no longer does anything.
 */

qbase::~qbase ()
{
    // No code needed
}

}           // namespace seq66

/*
 * qbase.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


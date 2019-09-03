#if ! defined SEQ66_BASESETTINGS_HPP
#define SEQ66_BASESETTINGS_HPP

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
 * \file          basesettings.hpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  in this application.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-01-17
 * \updates       2019-02-09
 * \license       GNU GPLv2 or above
 *
 *  This module defines some items common to all configuration files that get
 *  written.
 */

#include <string>
#include <vector>

#include "cfg/comments.hpp"             /* seq66::comments class            */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Holds the current values of sequence settings and settings that can
 *  modify the number of sequences and the configuration of the
 *  user-interface.  These settings will eventually be made part of the
 *  "user" settings file.
 */

class basesettings
{

private:

    /**
     *  A [Sequencer66] marker section indicates the ordinal version of the
     *  file.  Starts at 0, and is incremented when a new feature is added or
     *  a change is made.
     */

    int m_ordinal_version;

    /**
     *  [comments]
     *
     *  Provides a way to embed comments in the "usr" file and not lose
     *  them when the "usr" file is auto-saved.
     */

    comments m_comments_block;

public:

    basesettings ();
    basesettings (const basesettings & rhs);
    basesettings & operator = (const basesettings & rhs);
    virtual ~basesettings ()
    {
        // default, member automatically deleted
    }

    virtual void set_defaults ();
    virtual void normalize ();

public:

    /**
     * \getter m_ordinal_version
     */

    int ordinal_version () const
    {
        return m_ordinal_version;
    }

    /**
     * \getter m_comments_block
     */

    const comments & comments_block () const
    {
        return m_comments_block;
    }

    /**
     * \getter m_comments_block
     */

    comments & comments_block ()
    {
        return m_comments_block;
    }

protected:

    /**
     * \setter m_ordinal_version
     */

    void ordinal_version (int value)
    {
        m_ordinal_version = value;
    }

    /**
     * \setter m_ordinal_version
     */

    void increment_ordinal_version ()
    {
        ++m_ordinal_version;
    }

};          // class basesettings

}           // namespace seq66

#endif      // SEQ66_BASESETTINGS_HPP

/*
 * basesettings.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


#if ! defined SEQ66_OPCONTAINER_HPP
#define SEQ66_OPCONTAINER_HPP

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
 * \file          opcontainer.hpp
 *
 *  This module declares/defines the class for holding MIDI operation data for
 *  the application.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-13
 * \updates       2019-02-11
 * \license       GNU GPLv2 or above
 *
 *  This container holds a map of midioperation objects keyed by an operation
 *  number that can range from 0 to 255.  This container is to be used in two
 *  contexts:
 *
 *      -   Keystroke.  When a keystroke is received by the user interface:
 *          -   The key-event callback will use the key-event keycode and
 *              modifier to look up the ordinal for the key.  This
 *              standardizes the set of keys supported.
 *          -   This ordinal is then used to look up what was was configured
 *              in one of three MIDI control sections in
 *              the "rc" file:
 *              -   The operation category (pattern, mute-group, or
 *                  automation).
 *              -   The operation number (which pattern, mute-group, or
 *                  automation control.
 *          -   The value for this operation number is a midioperation object
 *              that can be queried, if need be, to get the name of the
 *              operation, the category, and even the operation number.  But
 *              in most cases, all that the caller will want to do is execute
 *              the midioperation::call() function.
 *      -   MIDI.  If an incoming MIDI event is found in the list of supported
 *          MIDI events, which will also obtain the proper ordinal/operation
 *          number to look up the operation, as in the three sub-steps above.
 *
 *  After the midioperation is obtained, the caller will execute
 *  midioperation::call(), passing to it the desired automation::action and
 *  the two data values.
 *
 *  It requires C++11 and above.
 */

#include <map>                          /* std::map<>                       */
#include <string>                       /* std::string                      */

#include "ctrl/midioperation.hpp"       /* seq66::midioperation             */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Provides an object specifying what a keystroke, GUI action, or a MIDI
 *  control should do.
 */

class opcontainer
{

public:

    /**
     *  Provides the type definition for this container.  The key is the
     *  operation number (generally ranging from 0 to 31 for each
     *  automation::category), and the value is a midioperation object.
     */

    using opmap = std::map<automation::slot, midioperation>;

private:

    /**
     *  The container itself.
     */

    opmap m_container;

    /**
     *  A name to use for showing the contents of the container.
     */

    std::string m_container_name;

private:

    opcontainer ();

public:

    opcontainer (const std::string & name);
    opcontainer (const opcontainer &) = default;
    opcontainer & operator = (const opcontainer &) = default;
    opcontainer (opcontainer &&) = default;
    opcontainer & operator = (opcontainer &&) = default;
    ~opcontainer () = default;

    const std::string & name () const
    {
        return m_container_name;
    }

    void clear ()
    {
        m_container.clear();
    }

    bool add (const midioperation & op);
    const midioperation & operation (automation::slot) const;

public:

    void show () const;

};              // class opcontainer

}               // namespace seq66

#endif          // SEQ66_OPCONTAINER_HPP

/*
 * opcontainer.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


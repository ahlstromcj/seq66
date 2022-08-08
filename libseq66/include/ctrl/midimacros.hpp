#ifndef SEQ66_MIDIMACROS_HPP
#define SEQ66_MIDIMACROS_HPP

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
 * \file          midimacros.hpp
 *
 *  This module declares/defines the base class for handling MIDI control
 *  I/O of the application.
 *
 * \library       seq66 application
 * \author        C. Ahlstrom
 * \date          2021-11-22
 * \updates       2022-08-08
 * \license       GNU GPLv2 or above
 *
 *  Provides the base class for midicontrolout.
 *
 * Warning:
 *
 *      It is NOT a base class for midicontrol or midicontrolin!
 */

#include <map>                          /* std::map container class         */

#include "ctrl/midimacro.hpp"           /* seq66::midimacro class           */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Represents a string of midibytes and provides the infrastructure for
 *  reading them.
 */

class midimacros
{

public:

    static const std::string footer;
    static const std::string header;
    static const std::string reset;
    static const std::string startup;
    static const std::string shutdown;

    using container = std::map<std::string, midimacro>;

private:

    /**
     *  This is a list of tokens making up the macro. Although it can take up
     *  extra space, it is useful to write the macro back to the configuration
     *  file. Also see the tokenize() function in the strfunctions module.
     */

    container m_macros;

    /**
     *  We need a way to not emit startup and exit macros if the user doesn't
     *  want that.
     */

    bool m_active;

public:

    midimacros ();
    midimacros (const midimacros &) = default;
    midimacros & operator = (const midimacros &) = default;
    midimacros (midimacros &&) = default;
    midimacros & operator = (midimacros &&) = default;
    ~midimacros () = default;

    void clear ()
    {
        m_macros.clear();
    }

    bool add (const tokenization & tokens);     /* data from 'ctrl' file    */

    int count () const
    {
        return int(m_macros.size());
    }

    bool active () const
    {
        return m_active;
    }

    void active (bool flag)
    {
        m_active = flag;
    }

    bool expand ();
    midistring bytes (const std::string & name) const;
    std::string lines () const;
    tokenization names () const;
    std::string byte_strings () const;
    bool make_defaults ();

private:

    void tokenize ();
    midistring expand (midimacro & m);

};          // class midimacros

}           // namespace seq66

#endif      // SEQ66_MIDIMACROS_HPP

/*
 * midimacros.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


#if ! defined SEQ66_MIDIOPERATION_HPP
#define SEQ66_MIDIOPERATION_HPP

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
 * \file          midioperation.hpp
 *
 *  This module declares/defines the class for handling MIDI control data for
 *  the application.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-13
 * \updates       2019-02-23
 * \license       GNU GPLv2 or above
 *
 *  This module defines a number of concepts relating to control of pattern
 *  unmuting, group control, and a number of additional controls to make
 *  Seq66 controllable without a graphical user interface.  It is also used in
 *  handling automation keystrokes.
 *
 *  It requires C++11 and above.
 */

#include <functional>                   /* std::function<>                  */
#include <vector>                       /* std::vector<>                    */

#include "ctrl/opcontrol.hpp"           /* seq66::opcontrol & automation    */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Provides an object specifying what a keystroke, GUI action, or a MIDI
 *  control should do.
 */

class midioperation
{

public:

    /**
     *  Provides the function object with a signature needed to handle any
     *  MIDI control operation.  The integers are generally the values of MIDI
     *  d0 and d1, but the second value can also hold a pattern number or a
     *  group number, and then it is called an "index".  For keystrokes, the
     *  d0 value is always 0 [perhaps it should be -1?]. The boolean holds the
     *  state of the inverse setting for MIDI control, and is always false for
     *  key-control.
     */

    using functor = std::function<bool (automation::action, int, int, bool)>;

private:

    /**
     *  Names the operation for use in various human-readble purposes.
     *  An example would be "BPM Page Up".
     */

    std::string m_op_name;

    /**
     *  Which section of the "rc" control file is this operation in?
     *  Pattern, Mute_group, or Automation?
     */

    automation::category m_op_category;

    /**
     *  Provides the operation number, ranging from 0 to the maximum number of
     *  keys supported, say 96 to 127.  For a pattern control, this is the
     *  pattern number.  For a mute-group control, this is the group number.
     *  For an automation control, this is the number of the performer
     *  operation to call.
     */

    automation::slot m_op_number;

    /**
     *  Holds the function that the caller wants to call for this
     *  midioperation object.
     */

    functor m_parent_function;

public:

    midioperation ();
    midioperation
    (
        const std::string & opname,
        automation::category opcategory,
        automation::slot opnumber,
        functor pfunction
    );
    midioperation (const midioperation &) = default;
    midioperation & operator = (const midioperation &) = default;
    midioperation (midioperation &&) = default;
    midioperation & operator = (midioperation &&) = default;
    ~midioperation () = default;

    bool is_usable () const
    {
        return m_op_category != automation::category::none;
    }

    /**
     *  Calls the function that was registered with this operation.  This call
     *  will not alter this object.
     */

    bool call (automation::action a, int d0, int d1, bool inverse) const
    {
        return m_parent_function(a, d0, d1, inverse);
    }

    const std::string & name () const
    {
        return m_op_name;
    }

    automation::category cat_code () const
    {
        return m_op_category;
    }

    std::string cat_name () const
    {
        return opcontrol::category_name(m_op_category);
    }

    automation::slot number () const
    {
        return m_op_number;
    }

    std::string slot_name () const
    {
        return opcontrol::slot_name(m_op_number);
    }

public:

    void show () const;

};              // class midioperation

}               // namespace seq66

#endif          // SEQ66_MIDIOPERATION_HPP

/*
 * midioperation.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


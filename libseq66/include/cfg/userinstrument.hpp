#if ! defined SEQ66_USERINSTRUMENT_HPP
#define SEQ66_USERINSTRUMENT_HPP

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
 * \file          userinstrument.hpp
 *
 *  This module declares/defines the user instrument section of the "user"
 *  configuration file.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-24
 * \updates       2021-01-21
 * \license       GNU GPLv2 or above
 *
 */

#include <string>

#include "util/basic_macros.hpp"        /* seq66_platform_macros.h too      */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Provides the maximum number of instruments that can be defined in the
 *  <code> ~/.seq66usr </code> or
 *  <code> ~/.config/seq66.rc </code>
 *  file.  With a value of 64, this is more of a sanity-check than a
 *  realistic number of instruments defined by a user.
 */

const int c_max_instruments = 64;

/**
 *  Manifest constant for the maximum value limit of a MIDI byte when used
 *  to limit the size of an array.  Here, it is the upper limit on the
 *  number of MIDI controllers that can be supported.
 */

const int c_midi_controller_max = 128;

/**
 *  This structure corresponds to <code> [user-instrument-N] </code>
 *  definitions in the <code> ~/.seq66usr </code> or
 *  <code> ~/.config/seq66.usr </code> file.
 */

struct userinstrument_t
{
    /**
     *  Provides the name of the "instrument" being supported.  Do not confuse
     *  "instrument" with "program" here.   An "instrument" is most likely
     *  a hardware MIDI sound-box (though it could be a software synthesizer
     *  as well.
     */

    std::string instrument;

    /**
     *  Provides a list of up to 128 controllers (e.g. "Modulation").
     *  If a controller isn't present, or if General MIDI is in force,
     *  this name might be empty.
     */

    std::string controllers[c_midi_controller_max];

    /**
     *  Provides a flag that indicates if each of up to 128 controller is
     *  active and supported.  If false, it might be an unsupported controller
     *  or a General MIDI device.
     */

    bool controllers_active[c_midi_controller_max];
};

/**
 *  Provides data about the MIDI instruments, readable from the "user"
 *  configuration file.  Will later make the size adjustable, if it
 *  makes sense to do so.
 */

class userinstrument
{

    /**
     *  Provides a validity flag, useful in returning a reference to a
     *  bogus object for internal error-check.  Callers should check
     *  this flag via the is_valid() accessor before using this object.
     *  This flag is set to true when any valid member assignment occurs
     *  via a public setter call.  However, setting an empty name for the
     *  instrument member will render the object invalid.
     */

    bool m_is_valid;

    /**
     *  Provides the actual number of non-default controllers actually
     *  set.  Often, the "user" configuration file has only a few out of
     *  the 128 assigned explicitly.
     */

    int m_controller_count;

    /**
     *  The instance of the structure that this class wraps.
     */

    userinstrument_t m_instrument_def;

public:

    userinstrument (const std::string & name = "");
    userinstrument (const userinstrument & rhs);
    userinstrument & operator = (const userinstrument & rhs);

    bool is_valid () const
    {
        return m_is_valid;
    }

    void clear ();

    const std::string & name () const
    {
        return m_instrument_def.instrument;
    }

    /**
     *  This function returns the number of active controllers.
     */

    int controller_count () const
    {
        return m_controller_count;
    }

    /**
     *  This function returns the maximum number of controllers, active or
     *  inactive.  Remember that the controller numbers for each MIDI
     *  instrument range from 0 to 127 (MIDI_CONTROLLER_MAX-1).
     */

    int controller_max () const
    {
        return c_midi_controller_max;
    }

    const std::string & controller_name (int c) const;  // getter
    bool controller_active (int c) const;               // getter
    bool set_controller                                 // setter
    (
        int c, const std::string & cname, bool isactive
    );

private:

    void set_name (const std::string & instname);
    void copy_definitions (const userinstrument & rhs);

};

}           // namespace seq66

#endif      // SEQ66_USERINSTRUMENT_HPP

/*
 * userinstrument.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


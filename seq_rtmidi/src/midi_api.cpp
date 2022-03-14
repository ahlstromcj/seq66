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
 * \file          midi_api.cpp
 *
 *    A class for a generic MIDI API.
 *
 * \library       seq66 application
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2022-03-13
 * \license       See above.
 *
 *  In this refactoring, we had to adapt the existing Seq66
 *  infrastructure to how the "RtMidi" library works.  We also had to
 *  refactor the RtMidi library significantly to fit it within the working
 *  mode of the Seq66 application and libraries.
 */

#include "cfg/settings.hpp"             /* seq66::rcsettings ...            */
#include "midi/event.hpp"
#include "midi_api.hpp"
#include "midi_info.hpp"
#include "midibus_rm.hpp"
#include "util/basic_macros.hpp"        /* errprint() etc.                  */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 *  midi_api section
 */

/**
 *  Principle constructor.
 */

midi_api::midi_api (midibus & parentbus, midi_info & masterinfo) :
    m_master_info               (masterinfo),
    m_parent_bus                (parentbus),
    m_input_data                (),
    m_connected                 (false),
    m_error_string              (),
    m_error_callback            (0),
    m_first_error_occurred      (false),
    m_error_callback_user_data  (0)
{
    // no code
}

/**
 *  Destructor, needed because it is virtual.
 */

midi_api::~midi_api ()
{
    // no code
}

/**
 *  Provides an error handler that can support an error callback.
 *
 * \throw
 *      If the error is not just a warning, then an rterror object is thrown.
 *
 * \param type
 *      The type of the error.
 *
 * \param errorstring
 *      The error message, which gets copied if this is the first error.
 */

void
midi_api::error (rterror::kind errtype, const std::string & errorstring)
{
    if (m_error_callback)
    {
        if (m_first_error_occurred)
            return;

        m_first_error_occurred = true;

        const std::string errorMessage = errorstring;
        m_error_callback(errtype, errorMessage, m_error_callback_user_data);
        m_first_error_occurred = false;
        return;
    }
    else
    {
        /*
         * Not a big fan of throwing errors, especially since we currently log
         * errors in rtmidi to the console.  Might make this a build option.
         *
         * throw rterror(errorstring, type);
         */

        errprint(errorstring);
    }
}

/**
 *  This function makes it a bit simpler on the caller.
 */

void
midi_api::master_midi_mode (midibase::io iotype)
{
    master_info().midi_mode(iotype);
}

}           // namespace seq66

/*
 * midi_api.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


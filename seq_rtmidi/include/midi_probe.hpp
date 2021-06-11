#if ! defined SEQ66_MIDI_PROBE_HPP
#define SEQ66_MIDI_PROBE_HPP

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
 * \file          midi_probe.hpp
 *
 *    Functions for testing and probing the MIDI support.
 *
 * \library       seq66 application
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-19
 * \updates       2017-01-11
 * \license       See above.
 *
 */

#include <string>

#include "rtmidi_info.hpp"

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq66
{

extern std::string midi_api_name (int i);
extern int midi_probe ();
extern bool midi_input_test (rtmidi_info & info, int portindex);

}           // namespace seq66

#endif      // SEQ66_MIDI_PROBE_HPP

/*
 * midi_probe.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


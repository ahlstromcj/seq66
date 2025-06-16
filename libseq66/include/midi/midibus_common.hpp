#if ! defined SEQ66_MIDIBUS_COMMON_HPP
#define SEQ66_MIDIBUS_COMMON_HPP

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
 * \file          midibus_common.hpp
 *
 *  This module declares/defines the elements that are common to the Linux
 *  and Windows implmentations of midibus.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2023-06-25
 * \license       GNU GPLv2 or above
 *
 *  Defines some midibus constants and the clock_e enumeration.
 */

namespace seq66
{

/**
 *  Manifest global constants.  These constants were also defined in
 *  midibus_portmidi.h, but we made them common to both implementations here.
 *
 *  The c_midibus_output_size value is passed, in mastermidibus, to
 *  snd_seq_set_output_buffer_size().  Not sure if the value needs to be so
 *  large.
 */

const int c_midibus_output_size = 0x100000;     // 1048576

/**
 *  The c_midibus_input_size value is passed, in mastermidibus,  to
 *  snd_seq_set_input_buffer_size().  Not sure if the value needs to be so
 *  large.
 */

const int c_midibus_input_size  = 0x100000;     // 1048576

/**
 *  Controls the amount a SysEx data sent at one time, in the midibus module.
 */

const int c_midibus_sysex_chunk = 0x100;        // 256

/**
 *  A clock enumeration, as used in the File / Options / MIDI Clock dialog.
 *  It is also (perhaps ill-advisedly) used for other statuses, including
 *  for some input port statuses.
 *
 * \var unavailable
 *      This value indicates that a port defined in a port-map is not
 *      present on the system.
 *
 * \var disabled
 *      A value to indicate to ignore/disable an output port. If a port always
 *      fails to open, we want just to ignore it. But see the unavailable
 *      status above.
 *
 * \var off
 *      Corresponds to the "Off" selection in the MIDI Clock tab.  With
 *      this setting, the MIDI Clock is disabled for the buss using this
 *      setting.  Notes will still be sent that buss, of course.  Some
 *      software synthesizers might require this setting in order to make
 *      a sound.  This value also doubles as "enabled" for inputs, which
 *      don't support the concept of clocks.
 *
 * \var pos
 *      Corresponds to the "Pos" selection in the MIDI Clock tab.  With
 *      this setting, MIDI Clock will be sent to this buss, and, if
 *      playback is starting beyond tick 0, then MIDI Song Position and
 *      MIDI Continue will also be sent on this buss.
 *
 * \var mod
 *      Corresponds to the "Mod" selection in the MIDI Clock tab.  With
 *      this setting, MIDI Clock and MIDI Start will be sent.  But
 *      clocking won't begin until the Song Position has reached the start
 *      modulo (in 1/16th notes) that is specified.
 *
 * \var max
 *      Illegal value for terminator.  Follows our convention for enum
 *      class maximums-but-out-of-bounds.
 */

enum class e_clock
{
    unavailable = -2,
    disabled    = -1,
    off         = 0,
    pos,
    mod,
    max

};          // enum class e_clock

/*
 *  Inline free functions.
 */

inline e_clock
int_to_clock (int e)
{
    return e < static_cast<int>(e_clock::max) ?
        static_cast<e_clock>(e) : e_clock::disabled ;
}

inline int
clock_to_int (e_clock e)
{
    return static_cast<int>(e == e_clock::max ? e_clock::disabled : e);
}

inline bool
clocking_enabled (e_clock ce)
{
    return ce == e_clock::pos || ce == e_clock::mod;
}

inline bool
port_unavailable (e_clock ce)
{
    return ce == e_clock::unavailable;
}

inline bool
port_disabled (e_clock ce)
{
    return ce == e_clock::disabled;
}

}           // namespace seq66

#endif      // SEQ66_MIDIBUS_COMMON_HPP

/*
 * midibus_common.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


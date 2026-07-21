#if ! defined SEQ66_SCRATCHPAD_HPP
#define SEQ66_SCRATCHPAD_HPP

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
 * \file          scratchpad.hpp
 *
 *      Provides a configurable pattern to record events for background
 *      recording.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2026-07-18
 * \updates       2026-07-18
 * \license       GNU GPLv2 or above
 *
 *  The scratchpad is a sequence with a special configuration. It can be added
 *  to the performer's playset to be recorded "unofficially".
 *
 *  ===It is not visible and it is not editable once created.
 *  ===There is also a lot of stuff in seq66::sequence not needed here.
 *
 *  The scratchpad class extends the sequence class for recording in the
 *  background automatically.
 */

#include "play/sequence.hpp"            /* seq66::sequence                  */

namespace seq66
{

/**
 *  An extension of metro for recording in the backbround.
 */

class scratchpad final : public sequence
{
    friend class performer;

private:

    scratchpad & operator = (const scratchpad & rhs);

public:

    scratchpad ();
    virtual ~scratchpad ();

    bool initialize
    (
        performer * p,
        bussbyte recbuss,
        int recmeasures         = 0,
        bussbyte thrubuss       = null_buss(),          /* c_bussbyte_max   */
        midibyte thruchannel    = 0
    );
    bool uninitialize ();

};          // class scratchpad

}           // namespace seq66

#endif      // SEQ66_SCRATCHPAD_HPP

/*
 * scratchpad.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

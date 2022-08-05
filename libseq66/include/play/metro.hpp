#if ! defined SEQ66_METRO_HPP
#define SEQ66_METRO_HPP

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
 * \file          metro.hpp
 *
 *  Provides a configurable pattern that can be used as a metro.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2022-08-05
 * \updates       2022-08-05
 * \license       GNU GPLv2 or above
 *
 *  The metro is a sequence with a special configuration.  It can be added
 *  to the performer's playset to be played along with the rest of the
 *  patterns.  It is not visible and it is not editable once created.
 *  There is also a lot of stuff in seq66::sequence not needed here.
 */

#include "play/sequence.hpp"            /* seq66::sequence                  */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  The metro class is just a sequence used for implementing a metronome
 *  functionality.
 */

class metro : public sequence
{
    friend class performer;

private:

    /**
     *  Provides the patch/program number to use.  This selects the
     *  sound the metronome should have.  It is played at the start of each
     *  loop; added first in the event list.
     */

    midibyte m_main_patch;

    /**
     *  Optionally, the other beats can be played with a different patch.
     */

    midibyte m_sub_patch;

    /**
     *  The highlight (measure) note to play, its velocity, and its length.
     */

    midibyte m_main_note;
    midibyte m_main_note_velocity;
    midipulse m_main_note_length;

    /**
     *  The sub-measure (beat) notes to play, their velocity, and their
     *  lengths.
     */

    midibyte m_sub_note;
    midibyte m_sub_note_velocity;
    midipulse m_sub_note_length;

private:

    metro & operator = (const metro & rhs);

public:

    metro ();

    metro
    (
        int mainpatch, int subpatch,
        int mainnote, int mainnote_velocity,
        int subnote, int subnote_velocity,
        midipulse mainnote_len  = 0,
        midipulse subnote_len   = 0
    );

    virtual ~metro ();

    bool initialize ();

};          // class metro

}           // namespace seq66

#endif      // SEQ66_METRO_HPP

/*
 * metro.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


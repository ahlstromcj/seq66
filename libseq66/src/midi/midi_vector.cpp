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
 * \file          midi_vector.cpp
 *
 *  This module declares/defines the concrete class for a container of MIDI
 *  data.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-10-11
 * \updates       2019-01-27
 * \license       GNU GPLv2 or above
 *
 */

#include "midi/midi_vector.hpp"         /* seq66::midi_vector_base base     */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  This constructor fills in the members of this class.
 *
 * \param seq
 *      Provides a reference to the sequence/track for which this container
 *      holds MIDI data.
 */

midi_vector::midi_vector (sequence & seq)
 :
    midi_vector_base    (seq),
    m_char_vector       ()
{
    // Empty body
}

}           // namespace seq66

/*
 * midi_vector.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


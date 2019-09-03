#if ! defined SEQ66_QSEQSTYLE_HPP
#define SEQ66_QSEQSTYLE_HPP

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
 * \file          qseqstyle.hpp
 *
 *  This module declares/defines the base class for... TBD
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-02-19
 * \license       GNU GPLv2 or above
 *
 */

#include <QCommonStyle>

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

class qseqstyle : public QCommonStyle
{

public:

    qseqstyle ();

    virtual ~qseqstyle ()
    {
        // no code needed
    }

};          // class qseqstyle

#endif      // SEQ66_QSEQSTYLE_HPP

/*
 * qseqstyle.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


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
 * \file          nsmserver.cpp
 *
 *  This module could serve as an alternative to nsmd (Non Session Manager
 *  daemon) someday.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2020-03-11
 * \updates       2020-03-11
 * \license       GNU GPLv2 or above
 *
 */

#include <memory>                       /* std::unique_ptr<>                */

#include "nsm/nsmserver.hpp"            /* seq66::nsmserver class           */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *
 */

nsmserver::nsmserver (const std::string & nsmurl) :
    nsmbase (nsmurl)
{
    //
}

/**
 *
 */

std::unique_ptr<nsmserver>
create_nsmserver ()
{
    std::unique_ptr<nsmserver> result;
    std::string url = get_nsm_url();
    if (! url.empty())
        result.reset(new nsmserver(url));

    return result;
}

}           // namespace seq66

/*
 * nsmserver.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


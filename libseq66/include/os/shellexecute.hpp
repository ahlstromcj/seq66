#if ! defined SEQ66_SHELLEXECUTE_HPP
#define SEQ66_SHELLEXECUTE_HPP

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
 * \file          shellexecute.hpp
 * \author        Chris Ahlstrom
 * \date          2022-05-19
 * \updates       2022-05-29
 * \license       GNU GPLv2 or above
 *
 *    This module provides functions for executing commands from within
 *    the application.
 */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 *  Free functions for Linux and Windows support.
 */

extern bool command_line (const std::string & cmdline);
extern bool open_document (const std::string & documentpath);
extern bool open_pdf (const std::string & pdfspec);
extern bool open_url (const std::string & pdfspec);
extern bool open_local_url (const std::string & pdfspec);

}        // namespace seq66

#endif   // SEQ66_SHELLEXECUTE_HPP

/*
 * vim: ts=4 sw=4 et ft=cpp
 */


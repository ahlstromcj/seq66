#if ! defined SEQ66_TIMING_HPP
#define SEQ66_TIMING_HPP

/**
 * \file          daemonize.hpp
 * \author        Chris Ahlstrom
 * \date          2005-07-03 to 2007-08-21 (from xpc-suite project)
 * \updates       2020-07-06
 * \license       GNU GPLv2 or above
 *
 *    Daemonization of POSIX C Wrapper (PSXC) library
 *    Copyright (C) 2005-2020 by Chris Ahlstrom
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *    02110-1301, USA.
 *
 *    This module provides a function to make it easy to run an application
 *    as a daemon.
 */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 *  Free functions for Linux and Windows support.
 */

extern bool microsleep (int us);
extern bool millisleep (int ms);
extern long microtime ();
extern long millitime ();

}        // namespace seq66

#endif   // SEQ66_TIMING_HPP

/*
 * vim: ts=4 sw=4 et ft=cpp
 */


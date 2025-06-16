#if ! defined SEQ66_TIMING_HPP
#define SEQ66_TIMING_HPP

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
 * \file          timing.hpp
 * \author        Chris Ahlstrom
 * \date          2005-07-03 to 2007-08-21 (from xpc-suite project)
 * \updates       2021-11-19
 * \license       GNU GPLv2 or above
 *
 *    Daemonization of POSIX C Wrapper (PSXC) library
 *    Copyright (C) 2005-2025 by Chris Ahlstrom
 *
 *    This module provides functions for timing and increasing thread
 *    priority.
 */

#include <thread>                       /* std::thread                      */

#include "seq66_platform_macros.h"      /* for detecting 32-bit builds      */

namespace seq66
{

/*
 *  Free functions for Linux and Windows support.
 */

extern int std_sleep_us ();
extern bool microsleep (int us);
extern bool millisleep (int ms);
extern void thread_yield ();
extern long microtime ();
extern long millitime ();
extern bool set_thread_priority (std::thread & t, int p = 1);
extern bool set_timer_services (bool on);

}        // namespace seq66

#endif   // SEQ66_TIMING_HPP

/*
 * vim: ts=4 sw=4 et ft=cpp
 */


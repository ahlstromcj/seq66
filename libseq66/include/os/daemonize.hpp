#if ! defined SEQ66_DAEMONIZE_HPP
#define SEQ66_DAEMONIZE_HPP

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
 * \file          daemonize.hpp
 * \author        Chris Ahlstrom
 * \date          2005-07-03 to 2007-08-21 (from xpc-suite project)
 * \updates       2021-07-14
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

#include <string>

#include "seq66_platform_macros.h"      /* for detecting 32-bit builds      */

/*
 * uint32_t alias for 32-bit code
 */

#if defined SEQ66_PLATFORM_32_BIT
using uint32_t = unsigned int;
#endif

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 *  Free functions.
 *    These functions do a lot of the work of dealing with UNIX daemons.
 */

extern bool check_daemonize (int argc, char * argv []);
extern uint32_t daemonize
(
    const std::string & appname,
    const std::string & cwd         = ".",
    int mask                        = 0
);
extern void undaemonize (uint32_t previous_umask);

/*
 * Linux and Windows support.
 */

extern bool reroute_stdio
(
    const std::string & logfile = "",
    bool closem = false
);

#if defined SEQ66_USE_PID_EXISTS
extern bool pid_exists (const std::string & exename);
#endif

extern std::string get_pid ();

/*
 * Basic session handling from use falkTX, circa 2020-02-02.  The following
 * function is internal.
 *
 *      extern void session_handler (int sig);
 *
 * The following functions return status booleans that the caller can use to
 * determine what to do.
 */

extern void session_setup ();
extern bool session_close ();
extern bool session_save ();

/*
 *  Useful for the performer to flag an application exit.  Be freakin' careful
 *  with this one!  :-D
 */

extern void signal_for_exit ();

}        // namespace seq66

#endif   // SEQ66_DAEMONIZE_HPP

/*
 * vim: ts=4 sw=4 et ft=cpp
 */


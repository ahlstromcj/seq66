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
 * \updates       2023-03-22
 * \license       GNU GPLv2 or above
 *
 *    Daemonization of POSIX C Wrapper (PSXC) library
 *    Copyright (C) 2005-2023 by Chris Ahlstrom
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
using mode_t = unsigned int;
#endif

/*
 *  The "flags" parameters described in Michael Kerrisk's book,
 *  "The Linux Programming Interface", 2010. We also add a flag to avoid
 *  a second fork, plus some other flags.
 */

enum d_flags_t
{
    d_flag_none             = 0x00, /**< No flags provided.                 */
    d_flag_no_chdir         = 0x01, /**< Don't chdir() to file root '/'.    */
    d_flag_no_close_files   = 0x02, /**< Don't close all open files.        */
    d_flag_no_reopen_stdio  = 0x04, /**< No stdin etc. sent to /dev/null.   */
    d_flag_no_umask         = 0x08, /**< Don't call umask(0).               */
    d_flag_no_fork_twice    = 0x10, /**< Don't call fork() a second time.   */
    d_flag_no_set_directory = 0x20, /**< Don't change current directory.    */
    d_flag_no_syslog        = 0x40, /**< Do not open a system log file.     */
    d_flag_no_to_all        = 0xFF  /**< All of the above!                  */
};

using daemonize_flags = enum d_flags_t;

/**
 *  Status of the daemonize() call.  The following actions should be taken
 *  based on the result.
 *
 *      -   failure. The parent process should exit with a return value of
 *          EXIT_FAILURE.
 *      -   child. We're in the child process, so that normal operation of
 *          the child application, including reading all the configuration
 *          files, should proceed.
 *      -   parent. We're in the parent process, and the fork succeeded,
 *          so that the parent should exit with a return value of
 *          EXIT_SUCCESS.
 */

enum class daemonization
{
    failure = (-1),                 /**< The call to fork() failed.         */
    child   = 0,                    /**< Result of fork() in child process. */
    parent  = 1                     /**< Result of fork() is child's PID.   */
};

const int c_daemonize_max_fd = 8192; /**< Max. file-descriptors to close.   */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 *  Free functions.  These functions do a lot of the work of dealing with UNIX
 *  daemons.
 */

// extern bool check_daemonize (int argc, char * argv []);

extern daemonization daemonize
(
    mode_t & previousmask,
    const std::string & appname,
    int flags,
    const std::string & cwd         = ".",
    int mask                        = 0
);
extern void undaemonize (mode_t previous_umask);

/*
 * Linux and Windows support.
 */

extern bool close_stdio ();
extern bool reroute_stdio (const std::string & logfile = "");
extern bool reroute_stdio_to_dev_null ();

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
extern bool session_restart ();

/*
 *  Useful for the performer to flag an application exit.  Be freakin' careful
 *  with this one!  :-D
 */

extern void signal_for_save ();
extern void signal_for_exit ();
extern void signal_for_restart ();
extern void signal_end_restart ();

}        // namespace seq66

#endif   // SEQ66_DAEMONIZE_HPP

/*
 * vim: ts=4 sw=4 et ft=cpp
 */


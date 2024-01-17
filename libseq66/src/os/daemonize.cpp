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
 * \file          daemonize.cpp
 * \library       seq66 application (from PSXC library)
 * \author        Chris Ahlstrom
 * \date          2005-07-03 to 2007-08-21 (pre-Sequencer24/64)
 * \updates       2023-12-30
 * \license       GNU GPLv2 or above
 *
 *  Daemonization module of the POSIX C Wrapper (PSXC) library
 *  Copyright (C) 2005-2024 by Chris Ahlstrom
 *
 *  Provides a function to make it easy to run an application as a (Linux)
 *  daemon.  There are large differences between POSIX daemons and Win32
 *  services.  Thus, this module is mostly Linux/POSIX-specific.
 *
 *  Updating daemonize().  Instead of calling exit on error, it now
 *  returns the error code; the return value is now int instead of
 *  uint32_t, and uint32_t is now mode_t to reflect the return type of
 *  umask(2). Note that digging reveals that mode_t is an unsigned 32-bit
 *  type anyway.
 *
 *  We also calls fork() again to insure that the application is not
 *  the session leader, and implement the "flags" parameters; both steps
 *  are described in Michael Kerrisk's book, "The Linux Programming
 *  Interface", 2010.
 *
 *  Questions:
 *
 *      -   Do we also want to add some code to insure that the application
 *          can run only one instance?
 *      -   Do we want to handle SIGHUP?
 *
 *  The quick-and-dirty story on creating a daemon.
 *
 *      -#  Fork off the parent process.
 *      -#  Change file mode mask (umask)
 *      -#  Open any logs for writing.
 *      -#  Create a unique Session ID (SID)
 *      -#  Change the current working directory to a safe place.
 *      -#  Close standard file descriptors.
 *      -#  Enter actual daemon code.
 *
 *  This module handles all of the above.  Things not yet handled:
 *
 *      -#  Generation of various helpful files:
 *          -   PID file
 *          -   Lock file
 *          -   Error and information output file (though we do log some
 *              information via the syslog).
 *      -#  Thorough setting of the environment.
 *      -#  Thorough handling of user IDs and groups.
 *      -#  Redirection of the standard outputs to files.
 *
 *  See this project for an application that does all or most of the above:
 *
 *      https://github.com/bmc/daemonize
 *
 * \todo
 *      There is a service wrapper available under Win32.  It is called
 *      "srvhost.exe".  At this time, we *still* don't know how to use it, but
 *      it is available, and Windows XP seems to use it quite a bit.
 */

#include <atomic>                       /* std::atomic<bool>                */
#include <stdlib.h>                     /* EXIT_FAILURE for 32-bit builds   */
#include <string.h>                     /* memset()                         */

#include "seq66_features.hpp"           /* seq66::seq_app_name()            */
#include "os/daemonize.hpp"             /* daemonization functions & macros */
#include "util/basic_macros.hpp"        /* errprint()                       */
#include "util/filefunctions.hpp"       /* seq66::get_full_path() etc.      */

#if defined SEQ66_PLATFORM_UNIX         /* it's not just LINUX, dude!       */

#include <fcntl.h>                      /* O_RDWR flag                      */
#include <signal.h>                     /* struct sigaction                 */
#include <sys/stat.h>                   /* umask(), etc.                    */
#include <syslog.h>                     /* syslog() and related constants   */
#include <unistd.h>                     /* exit(), setsid()                 */

#define STD_CLOSE               close
#define STD_OPEN                open
#define STD_O_RDWR              O_RDWR
#define DEV_NULL                "/dev/null"

/*
 *  In Linux, dup2(int oldfd, int newfd) uses newfd as the new file descriptor,
 *  closing it if open before reusing it.  dup2() returns this new file
 *  descriptor on success, and a (-1) on error (setting errno, too).
 */

#define STD_DUP2                dup2
#define STD_DUP2_SUCCESS(rc)    (rc >= 0)

#elif defined SEQ66_PLATFORM_WINDOWS

/*
 * For Windows, only the reroute_stdio() function is defined, currently.
 */

#include <windows.h>                    /* WaitForSingleObject(), INFINITE  */
#include <fcntl.h>                      /* _O_RDWR                          */
#include <io.h>                         /* _open(), _close()                */
#include <process.h>                    /* Windows _getpid() function       */
#include <synchapi.h>                   /* recent Windows "wait" functions  */
#include <sys/stat.h>                   /* Windows S_IWUSR, S_IWGRP, etc.   */

#define STD_CLOSE               _close
#define STD_OPEN                _open
#define STD_O_RDWR              _O_RDWR
#define DEV_NULL                "NUL"

/*
 *  In Windows, _dup2() has the same parameters with the same meaning, but
 *  it returns 0 for success.
 */

#define STD_DUP2                _dup2
#define STD_DUP2_SUCCESS(rc)    (rc == 0)

#endif

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

#if defined SEQ66_PLATFORM_POSIX_API

/**
 *
 *  Performs a number of actions needed by a UNIX daemon.
 *
 *  These actions are layed out in the following URLs.
 *
 *      http://www.linuxprofilm.com/articles/linux-daemon-howto.html#s1
 *      http://www.deez.info/sengelha/projects/sigrandd/doc/#5
 *
 *      -# Fork off the parent process.
 *      -# Change file mode mask (umask).
 *      -# Open the system log for writing (optional).
 *      -# Create a unique Session ID (SID).
 *      -# Open any logs for writing.
 *      -# Change the current working directory to a safe place.
 *      -# Close standard file descriptors.
 *      -# Enter actual daemon code.
 *      -# Close the log upon exiting the daemon.
 *
 * \algorithm
 *  -# Fork an identical child process.  After the fork() call, we now
 *     have a copy of this application running as a child process.  We can
 *     then kill off the parent process using exit().  The child inherits
 *     the process group ID of the parent but gets a new process ID, so
 *     we're guaranteed that the child is not a process group leader. This
 *     is a prerequisite for the call to setsid() that is done later.
 *  -# Once this succeeds, we want to change the file-mode mask so that
 *     the daemon has access to system files that it creates.  The file
 *     mode creation mask that's inherited could have been set to deny
 *     certain permissions.
 *  -# Optional:  Open the system log for writing.  We make
 *     the system log our error-log, using the set_syslogging()
 *     function of the errorlogging.c module.
 *  -# The child process must get a unique SID (session ID) from the
 *     kernel in order to operate.  Otherwise, the child process becomes
 *     an orphan in the system. The pid_t type is used for the new SID.
 *     The process becomes a session leader of a new session, becomes the
 *     process group leader of a new process group, and has no controlling
 *     terminal.
 *  -# The current working directory should be changed to some place that
 *     is guaranteed to always be there. Since many Linux distributions do
 *     not completely follow the Linux Filesystem Hierarchy standard, the
 *     only directory that is guaranteed to be there is the root (/).
 *     However, we make it a command-line option that defaults to ".".
 *     See the functions in the audio_arguments.c/h module.
 *     Since daemons normally exist until the system is rebooted, if the
 *     daemon stays on a mounted filesystem, that filesystem cannot be
 *     unmounted.
 *  -# An important step in setting up a daemon is closing out the
 *     standard file descriptors (STDIN, STDOUT, STDERR). Since a daemon
 *     cannot use the terminal, these file descriptors are useless, and a
 *     potential security hazard.
 *  -# As the last step, we check out the command-line arguments and the
 *     audio parameters, and use them to start the desired task.
 *  -# If the server is stopped normally, we go ahead and call closelog(),
 *     even though it is optional.
 *
 * \param [inout] previousmask
 *      Returns the previous mask for storage.  It should be saved for later
 *      restoration.
 *
 * \param flags
 *      Provides potential bitmask values (daemonize_flags) for the call
 *      to this function, as per Kerrisk's book.
 *
 * \param appname
 *      Name of the application to daemonize.
 *
 * \param cwd
 *      Current working directory to set.  Defaults to ".", for now.
 *
 * \param mask
 *      The umask value to set.  Defaults to 0.
 *
 * \return
 *      Returns either EXIT_FAILURE or EXIT_SUCCESS. If EXIT_SUCCESS is
 *      returned, the application (the parent) should call exit().
 */

daemonization
daemonize
(
    mode_t & previousmask,
    const std::string & appname,
    int flags,
    const std::string & cwd,
    int mask
)
{
    static std::string s_app_name;          /* to survive forking?           */
    previousmask = 0;
    s_app_name.clear();                     /* blank out the base app name   */
    if (! appname.empty())
        s_app_name = appname;               /* copy the base app name        */

    /*
     *  fork():
     *
     *      -   On success, the PID of the child is returned in the parent,
     *          and 0 is returned in the child.
     *      -   On failure, -1 is returned in the parent, and there is no
     *          child.
     */

    pid_t pid;
    if ((flags & d_flag_fake_fork) != 0)    /* just pretend a fork occurred */
    {
        pid = 0;                            /* pretend we're in the child   */
        flags = d_flag_fake_fork_flags;
    }
    else
        pid = fork();                       /* 1. Fork the parent process   */

    if (is_posix_error(pid))  /* -1 */      /*    process creation failed   */
    {
        errprint("parent fork() failed");
        return daemonization::failure;      /*    exit() parent as failure  */
    }
    else if (pid != 0)                      /*    child creation succeeded  */
    {
        return daemonization::parent;       /*    exit() parent w/success   */
    }
    else                                    /*    we're in child process    */
    {
        /*
         *  Now we are the child process.
         *
         *  Create a new session and set the process group ID. This succeeds
         *  if the calling process is not a process group leader. If it fails,
         *  then we become the leader of the new session and exit with
         *  EXIT_FAILURE.
         */

        pid_t sid = setsid();               /* 2. Get a new session ID...    */
        if (sid < 0)                        /*    ... couldn't get one       */
            return daemonization::failure;  /*    exit() child as a failure     */

        if ((flags & d_flag_no_fork_twice) == 0)
        {
            /*
             *  Now ensure that we are not the session leader. This eliminates
             *  the possibility of the application/device does not become a
             *  controlling terminal.
             */

            pid = fork();                       /* fork you again!          */
            if (is_posix_error(pid))
            {
                errprint("child fork() failed");
                return daemonization::failure;
            }
            else if (pid != 0)
            {
                return daemonization::parent;
            }
        }
        if ((flags & d_flag_no_umask) == 0)
        {
            if (mask > 0)
                previousmask = ::umask(mask);   /* 3. Save & set user mask  */
            else
                (void) ::umask(0);              /* 3. clear file mask       */
        }
        if ((flags & d_flag_no_chdir) == 0)     /* this is only for ROOT    */
        {
            int rc = chdir("/");
            if (rc != 0)
            {
                errprint("chdir('/') failed");
                return daemonization::failure;
            }
        }
        if ((flags & d_flag_no_close_files) == 0)
        {
            int maxfd = ::sysconf(_SC_OPEN_MAX);
            if (maxfd == (-1))
                maxfd = c_daemonize_max_fd;     /* this is just a guess     */

            for (int fd = 0; fd < maxfd; ++fd)
                (void) close(fd);
        }
        if ((flags & d_flag_no_reopen_stdio) == 0)
            (void) reroute_stdio_to_dev_null();

        if (s_app_name.empty())
            s_app_name = "anonymous daemon";

        if ((flags & d_flag_no_syslog) == 0)    /* system log               */
            openlog(s_app_name.c_str(), LOG_CONS|LOG_PID, LOG_USER);

        if ((flags & d_flag_no_set_currdir) == 0)
        {
            bool cwdgood = cwd != "." && ! cwd.empty();
            if (cwdgood)
            {
                if (! set_current_directory(cwd))
                    return daemonization::failure;
            }
        }

        /*
         * No longer needed.  Caller can use it. Also see
         * d_flag_no_reopen_stdio.
         *
         * (void) reroute_stdio("", true);  // 6. close standard files      //
         */

        if ((flags & d_flag_no_syslog) == 0)    /* system log               */
            syslog(LOG_NOTICE, "daemon started");
    }
    return daemonization::child;
}

/*
 *    This function undoes the daemon setup.  It undoes the actions of the
 *    daemonize() function by first restoring the previous umask.  Then, it
 *    restores the error-level of the application to the default error-level
 *    ("--error").  Not sure how useful this function is, since the daemon is
 *    probably exiting anyway.
 *
 * \param previous_umask
 *    Previous umask value, for later restoring.
 */

void
undaemonize (mode_t previous_umask)
{
   syslog(LOG_NOTICE, "seq66 daemon exited");
   closelog();
   if (previous_umask != 0)
      (void) umask(previous_umask);          /* restore user mask             */
}

#endif      // SEQ66_PLATFORM_POSIX_API

/**
 * \todo
 *    Implement "daemonizing" for Windows, including redirection to the
 *    Windows Event Log.  Still need to figure out a way to do this very
 *    simply, a la' Microsoft's 'svchost' executable.
 */

/**
 *  Route the standard terminal file descriptors to "/dev/null".
 */

bool
reroute_stdio_to_dev_null ()
{
    int rc = STD_CLOSE(STDIN_FILENO);
    bool result = rc == 0;
    if (result)
    {
        int fd = STD_OPEN(DEV_NULL, STD_O_RDWR);
        result = fd == STDIN_FILENO;
        if (result)
        {
            int newfd = STD_DUP2(STDIN_FILENO, STDOUT_FILENO);
            result = STD_DUP2_SUCCESS(newfd);
            if (result)
            {
                newfd = STD_DUP2(STDIN_FILENO, STDOUT_FILENO);
                result = STD_DUP2_SUCCESS(newfd);
            }
        }
        if (result)
            warnprint("Standard I/O rerouted to " DEV_NULL);
        else
            file_error("Failed to reroute standard I/O", DEV_NULL);
    }
    return result;
}

/**
 *  Alters the standard terminal file descriptors so that they either route to
 *  to a log file, under Linux or Windows.
 *
 * \param logfile
 *      The optional name of the file to which to log messages.  Defaults to
 *      an empty string.
 *
 * \param closem
 *      Just closes the standard file descriptors, rather than rerouting them
 *      to /dev/null.  Defaults to false.  This is the value needed if the
 *      \a logfile parameter is not empty.
 *
 * \return
 *      Returns true if the log-file functionality has been enabled.
 *      I think :-D.
 */

bool
close_stdio ()
{
    bool result = true;
    int rc = STD_CLOSE(STDIN_FILENO);
    if (rc == (-1))
        result = false;

    rc = STD_CLOSE(STDOUT_FILENO);
    if (rc == (-1))
        result = false;

    rc = STD_CLOSE(STDERR_FILENO);
    if (rc == (-1))
        result = false;

    return result;
}

bool
reroute_stdio (const std::string & logfile)
{
    bool result = false;
    if (logfile.empty())                    /* route output to /dev/null    */
    {
        result = reroute_stdio_to_dev_null();
    }
    else
    {
        int rc = STD_CLOSE(STDOUT_FILENO);
        result = rc == 0;
        if (result)
        {
            int flags = O_WRONLY | O_CREAT | O_APPEND ;
            mode_t mode = S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP ;
            int fd = open(logfile.c_str(), flags, mode);
            result = fd != (-1);
            if (result)
            {
                int newfd = STD_DUP2(fd, STDOUT_FILENO);
                result = STD_DUP2_SUCCESS(newfd);
                if (result)
                {
                    newfd = STD_DUP2(fd, STDERR_FILENO);
                    result = STD_DUP2_SUCCESS(newfd);
                    if (result)
                    {
                        std::string logpath = get_full_path(logfile);
                        std::string normedpath = normalize_path(logpath);
                        printf
                        (
                            "\n%s\n%s\n%s\n",
                            seq_app_name().c_str(), normedpath.c_str(),
                            current_date_time().c_str()
                        );
                    }
                    else
                        file_error("Dup2 failed", "stderr");
                }
                else
                    file_error("Dup2 failed", "stdout");
            }
        }
        if (! result)
            file_error("Failed to reroute standard I/O", logfile);
    }
    return result;
}

/*
 * --------------------------------------------------------------------------
 *  Functions common to Linux and Windows
 * --------------------------------------------------------------------------
 */

/*
 *  Session-handling atomic booleans.
 */

static std::atomic<bool> sg_needs_close {};
static std::atomic<bool> sg_needs_save {};
static std::atomic<bool> sg_restart {};

bool
session_restart ()
{
    bool result = sg_restart;
    if (sg_needs_close)
        result = false;

    return result;
}

/**
 *  Returns the boolean to indicate a request to close the application.
 */

bool
session_close ()
{
    bool result = sg_needs_close;

#if defined SEQ66_PLATFORM_DEBUG_TMI
    if (result)
        warn_message("App marked for close...");
#endif

    sg_needs_close = false;
    return result;
}

/**
 *  Returns the boolean to indicate a request to save the current sequence
 *  file.
 */

bool
session_save ()
{
    bool result = sg_needs_save;
#if defined SEQ66_PLATFORM_DEBUG
    if (result)
        warn_message("Marked for file_save...");
#endif
    sg_needs_save = false;
    return result;
}

void signal_for_save ()
{
    sg_needs_save = true;
}

void signal_for_exit ()
{
    sg_needs_close = true;
}

/**
 *  Sets the flag for restarting the main() functions, instead of exiting.
 */

void
signal_for_restart ()
{
    sg_restart = true;
}

void
signal_end_restart ()
{
    sg_restart = false;
}

/*
 * --------------------------------------------------------------------------
 *  Functions for Linux
 * --------------------------------------------------------------------------
 */

#if defined SEQ66_PLATFORM_UNIX         // LINUX

/**
 *  Provides a basic session handler, called upon receipt of a POSIX signal.
 *  Note that SIGSTOP and SIGKILL cannot be blocked, ignored, or caught by a
 *  handler.  Also note that SIGKILL bypasses the SIGTERM handler; it is a last
 *  resort for runaway processes that don't respond to SIGTERM.
 */

static void
session_handler (int sig)
{
    psignal(sig, "Signal caught");
    switch (sig)
    {
    case SIGINT:                        /* 2: Ctrl-C "terminal interrupt"   */

        sg_needs_close = true;
        break;

    case SIGTERM:                       /* 15: "terminate process"          */

        sg_needs_close = true;
        break;

    case SIGUSR1:                       /* 10: "user-defined signal 1       */

        sg_needs_save = true;
        break;
    }
}

/**
 *  Sets up the application to intercept SIGINT, SIGTERM, and SIGUSR1.
 *
 * \param earlyexit
 *      It turns out that the test for the need to remap ports occurs
 *      (in Seq66} before this function is called. We don't want to continue
 *      to run in this case.
 *
 * \return
 *      Returns true if the application can continue.
 */

bool
session_setup (bool earlyexit)
{
    bool result = ! earlyexit;
    if (result)
    {
        struct sigaction action;
        memset(&action, 0, sizeof action);
        action.sa_handler = session_handler;
        sg_needs_close = sg_needs_save = sg_restart = false;
        sigaction(SIGINT, &action, NULL);               /* SIGINT is 2      */
        sigaction(SIGTERM, &action, NULL);              /* SIGTERM is 15    */
        sigaction(SIGUSR1, &action, NULL);              /* SIGUSR1 is 10    */
    }
    return result;
}

/**
 *  Looks up an executable in the process list using the pidof program.  This
 *  function copies the pidof command line, then opens a pipe to that process
 *  to read from it.  If anything is read, then the process ID is calculated
 *  and returned.  Otherwise, 0 is returned.
 *
 *  Example: "pidof nsmd", which will emit a PID if nsmd is running and return
 *  1 if the nsmd is not running.
 *
 *  This is actually way too strict, disabling the detection of NSM workalikes
 *  such as RaySession.
 */

pid_t
get_pid_by_name (const std::string & exename)
{
#if defined SEQ66_DEFINE_GET_PID_BY_NAME
    static const int s_pid_size = 200;      /* really only need about 10!   */
    pid_t result = 0;
    char cmd[s_pid_size + 1];
    snprintf(cmd, s_pid_size, "pidof %s", exename.c_str());

    FILE * fp = popen(cmd, "r");
    if (not_nullptr(fp))
    {
        size_t count = fread(cmd, sizeof(char), s_pid_size, fp);
        fclose(fp);
        if (count > 0)
        {
            result = atoi(cmd);
            file_message(exename, std::to_string(result));
        }
    }
    return result;
#else
    (void) exename;
    return 0;
#endif
}

bool
pid_exists (const std::string & exename)
{
    return get_pid_by_name(exename) > 0;
}

std::string
get_pid ()
{
    long p = long(getpid());
    return std::to_string(p);
}

std::string
get_process_name ()
{
    pid_t pid = getpid();
    return get_process_name(pid);
}

/**
 *  This Linux-only function reads /proc/PID/comm to get the (short) name of
 *  the process with the give PID.
 *
 *  It has been stated that the size of the process name in /proc/PID/comm is
 *  less than TASK_COMM_LEN = 16.  Thus, the return value might be a truncated
 *  process name.
 */

std::string
get_process_name (pid_t pid)
{
    std::string result;
    char temp[32];
    snprintf(temp, sizeof temp, "/proc/%d/comm", int(pid));

    FILE * f = fopen(temp, "r");
    if (not_nullptr(f))
    {
        size_t sz = fread(temp, sizeof(char), sizeof temp , f);
        if (sz > 0)
        {
            if (temp[sz - 1] == '\n')
                temp[sz - 1] = 0;

            result = std::string(temp);
        }
        fclose(f);
    }
    return result;
}

std::string
get_parent_process_name ()
{
    pid_t parentpid = getppid();
    return get_process_name(parentpid);
}

#else

/*
 * --------------------------------------------------------------------------
 *  Functions for Windows
 * --------------------------------------------------------------------------
 */

bool
session_setup (bool earlyexit)
{
    bool result = ! earlyexit;
    if (result)
    {
        sg_needs_close = sg_needs_save = sg_restart = false;
    }
    return result;
}

bool
pid_exists (const std::string & /*exename*/)
{
    return false;       /* to do, if possible */
}

pid_t
get_pid_by_name (const std::string & /*exename*/)
{
    return 0;
}

std::string
get_pid ()
{
    long p = long(_getpid());
    return std::to_string(p);
}

std::string
get_process_name ()
{
    pid_t pid = _getpid();
    return get_process_name(pid);
}

std::string
get_process_name (pid_t /*pid*/)
{
    std::string result;
    return result;              /* TO DO */
}

/*
 * See https://stackoverflow.com/questions/29939893/get-parent-process-name-windows
 */

std::string
get_parent_process_name ()
{
    /*
     * Not available on Windoze. And used only for the Non/New Session Manager,
     * which is not supported on Windoze.
     *
     * long parentpid = long(_getppid());
     */

    return std::string("None");
}

#endif  // defined SEQ66_PLATFORM_UNIX

}           // namespace seq66

/*
 * vim: ts=4 sw=4 et ft=cpp
 */


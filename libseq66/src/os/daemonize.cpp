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
 * \updates       2021-10-05
 * \license       GNU GPLv2 or above
 *
 *  Daemonization module of the POSIX C Wrapper (PSXC) library
 *  Copyright (C) 2005-2021 by Chris Ahlstrom
 *
 *  Provides a function to make it easy to run an application as a (Linux)
 *  daemon.  There are large differences between POSIX daemons and Win32
 *  services.  Thus, this module is currently Linux-specific.
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
 *  See this project for an application that does all of the above:
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
#include <string.h>                     /* strlen() etc.                    */

#include "seq66_features.hpp"           /* seq66::seq_app_name()            */
#include "os/daemonize.hpp"             /* daemonization functions & macros */
#include "util/calculations.hpp"        /* seq66::current_date_time()       */
#include "util/filefunctions.hpp"       /* seq66::get_full_path() etc.      */

#if defined SEQ66_PLATFORM_LINUX

#include <fcntl.h>                      /* O_RDWR flag                      */
#include <signal.h>                     /* struct sigaction                 */
#include <sys/stat.h>                   /* umask(), etc.                    */
#include <syslog.h>                     /* syslog() and related constants   */
#include <unistd.h>                     /* exit(), setsid()                 */

#define STD_CLOSE       close
#define STD_OPEN        open
#define STD_O_RDWR      O_RDWR
#define DEV_NULL        "/dev/null"

#elif defined SEQ66_PLATFORM_WINDOWS

/*
 * For Windows, only the reroute_stdio() function is defined, currently.
 */

#include <windows.h>                    /* WaitForSingleObject(), INFINITE  */
#include <fcntl.h>                      /* _O_RDWR                          */
#include <io.h>                         /* _open(), _close()                */
#include <synchapi.h>                   /* recent Windows "wait" functions  */
#include <process.h>                    /* Windows _getpid() function       */

#define STD_CLOSE       _close
#define STD_OPEN        _open
#define STD_O_RDWR      _O_RDWR
#define DEV_NULL        "NUL"

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
 *      http://www.linuxprofilm.com/articles/linux-daemon-howto.html#s1   <br>
 *      http://www.deez.info/sengelha/projects/sigrandd/doc/#5.           <br>
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
 *      The previous umask is returned, and should be saved for later
 *      restoration.
 */

uint32_t
daemonize
(
    const std::string & appname,
    const std::string & cwd,
    int mask
)
{
    static std::string s_app_name;          /* to survive forking?           */
    uint32_t result = 0;
    s_app_name.clear();                     /* blank out the base app name   */
    if (! appname.empty())
        s_app_name = appname;               /* copy the base app name        */

    pid_t pid = fork();                     /* 1. fork the parent process    */
    if (is_posix_error(pid))                /*    -1 process creation failed */
    {
        errprint("fork() failed");
        exit(EXIT_FAILURE);                 /*    exit parent as a failure   */
    }
    else if (pid > 0)                       /*    process creation succeeded */
    {
        exit(EXIT_SUCCESS);                 /*    exit parent successfully   */
    }
    else                                    /*    now we're in child process */
    {
        bool cwdgood = ! cwd.empty();
        result = uint32_t(umask(mask));     /* 2. save and set the user mask */
        pid_t sid = setsid();               /* 3. get a new session ID       */
        if (sid < 0)                        /*    couldn't get one           */
            exit(EXIT_FAILURE);             /*    exit the child process     */

        if (s_app_name.empty())
            s_app_name = "bad daemon";

        openlog(s_app_name.c_str(), LOG_CONS|LOG_PID, LOG_USER); /* 4. log it   */
        if (cwdgood)
            cwdgood = cwd != ".";           /*    don't bother with "."      */

        if (cwdgood)
        {
            if (! set_current_directory(cwd))
                exit(EXIT_FAILURE);         /*    bug out royally!           */
        }
        (void) reroute_stdio("", true);     /* 6. close standard files       */
        syslog(LOG_NOTICE, "seq66 daemon started");
    }
    return result;
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
undaemonize (uint32_t previous_umask)
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
reroute_stdio (const std::string & logfile, bool closem)
{
    bool result = false;
    if (closem)
    {
        int rc = STD_CLOSE(STDIN_FILENO);
        if (rc == (-1))
            result = false;

        rc = STD_CLOSE(STDOUT_FILENO);
        if (rc == (-1))
            result = false;

        rc = STD_CLOSE(STDERR_FILENO);
        if (rc == (-1))
            result = false;
    }
    else
    {
        result = true;
        (void) STD_CLOSE(STDIN_FILENO);

        int fd = STD_OPEN(DEV_NULL, STD_O_RDWR);
        if (fd != STDIN_FILENO)
            result = false;

        if (result)
        {
            if (logfile.empty())            /* route output to /dev/null    */
            {
                if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
                    result = false;

                if (result)
                {
                    if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
                        result = false;
                }
            }
            else                            /* route output to log-file     */
            {
                /*
                 * ca 2018-04-28
                 *  Change from "w" (O_WRONLY|O_CREAT|O_TRUNC) to
                 *  "a" (O_WRONLY|O_CREAT|O_APPEND).  Oops!
                 */

                FILE * fp = freopen(logfile.c_str(), "a", stdout);
                if (not_nullptr(fp))
                {
#if defined SEQ66_PLATFORM_WINDOWS
                    (void) dup2(STDOUT_FILENO, STDERR_FILENO);
#else
                    if (dup2(STDOUT_FILENO, STDERR_FILENO) != STDERR_FILENO)
                        result = false;
#endif
                }
                else
                    result = false;
            }
        }
        if (result)
        {
            std::string logpath = get_full_path(logfile);
            std::string normedpath = normalize_path(logpath);
            printf
            (
                "\n'%s' \n'%s' \n'%s' \n",
                seq_app_name().c_str(), normedpath.c_str(),
                current_date_time().c_str()
            );
        }
    }
    return result;
}

#if defined SEQ66_PLATFORM_LINUX

/*
 *  Session-handling is Linux-only.
 *
 *  Make these values atomic?
 */

static std::atomic<bool> sg_needs_close {};
static std::atomic<bool> sg_needs_save {};

/**
 *  Provides a basic session handler, called upon receipt of a POSIX signal.
 *
 *  Note that SIGSTOP and SIGKILL cannot be blocked, ignored, or caught by a
 *  handler.  Also note that SIGKILL bypass the SIGTERM handler; it is a last
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
 */

void
session_setup ()
{
    struct sigaction action;
    memset(&action, 0, sizeof action);
    action.sa_handler = session_handler;
    sigaction(SIGINT, &action, NULL);                   /* SIGINT is 2      */
    sigaction(SIGTERM, &action, NULL);                  /* SIGTERM is 15    */
    sigaction(SIGUSR1, &action, NULL);                  /* SIGUSR1 is 10    */
}

/**
 *  Returns the boolean to indicate a request to close the application.
 */

bool
session_close ()
{
    bool result = sg_needs_close;
#if defined SEQ66_PLATFORM_DEBUG
    if (result)
        printf("App marked for close....\n");
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
        printf("App marked for file_save....\n");
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

#if defined SEQ66_USE_PID_EXISTS

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

static pid_t
get_pid_by_name (const std::string & exename)
{
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
}

bool
pid_exists (const std::string & exename)
{
    return get_pid_by_name(exename) > 0;
}

#endif  // SEQ66_USE_PID_EXISTS

std::string
get_pid ()
{
    long p = long(getpid());
    return std::to_string(p);
}

#else

void
session_setup ()
{
    // no code at this time
}

bool
session_close ()
{
    return false;
}

bool
session_save ()
{
    return false;
}

void signal_for_save ()
{
    // no code at this time
}

void signal_for_exit ()
{
    // no code at this time
}

#if defined SEQ66_USE_PID_EXISTS

bool
pid_exists (const std::string & exename)
{
    return false;       /* to do, if possible */
}

#endif  // SEQ66_USE_PID_EXISTS

std::string
get_pid ()
{
    long p = long(_getpid());
    return std::to_string(p);
}

#endif  // defined SEQ66_PLATFORM_LINUX

}           // namespace seq66

/*
 * vim: ts=4 sw=4 et ft=cpp
 */


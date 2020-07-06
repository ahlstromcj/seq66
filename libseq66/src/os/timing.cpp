/**
 * \file          timing.cpp
 * \library       seq66 application (from PSXC library)
 * \author        Chris Ahlstrom
 * \date          2005-07-03 to 2007-08-21 (pre-Sequencer24/64)
 * \updates       2020-07-06
 * \license       GNU GPLv2 or above
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  this program; if not, write to the Free Software Foundation, Inc., 51
 *  Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "seq66_platform_macros.h"      /* for detecting 32-bit builds      */
#include "os/timing.hpp"                /* seq66::microsleep(), etc.        */

#if defined SEQ66_PLATFORM_LINUX

#include <errno.h>                      /* error numbers                    */
#include <sched.h>                      /* sched_yield()                    */
#include <time.h>                       /* C::nanosleep(2)                  */
#include <unistd.h>                     /* exit(), setsid()                 */

#elif defined SEQ66_PLATFORM_WINDOWS

/*
 * For Windows, only the reroute_stdio() and microsleep() functions are
 * defined, currently.
 */

#include <windows.h>                    /* WaitForSingleObject(), INFINITE  */
#include <mmsystem.h>                   /* Win32 timeGetTime() [!timeapi.h] */
#include <synchapi.h>                   /* recent Windows "wait" functions  */

#endif

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 *  This free-function in the seq66 namespace provides a way to suspend a
 *  thread for a small amount of time, or to yield the processor.
 *
 * \linux
 *      We can use the usleep(3) function.
 *
 * \unix
 *    In POSIX, select() can return early if any signal occurs.  We don't
 *    correct for that here at this time.  Actually, it is a convenient
 *    feature, and we wish that Sleep() would provide it.
 *
 * \win32
 *    In Windows, the Sleep(0) function does not sleep, but it does cede
 *    control of the thread to the operating system, which then schedules
 *    another thread to run.
 *
 * \warning
 *    Please note that this function isn't all that accurate for small
 *    sleep values, due to the time taken to set up the operation, and
 *    resolution issues in many operating systems.
 *
 * \param ms
 *    The number of milliseconds to "sleep".  For Linux and Windows, the
 *    microsleep() function handles this, including the case of ms == 0.
 *
 * \return
 *    Returns true if the ms parameter was 0 or greater.
 */

bool
millisleep (int ms)
{
    bool result = ms >= 0;
    if (result)
    {
#if defined SEQ66_PLATFORM_LINUX
        result = microsleep(ms * 1000);
#elif defined SEQ66_PLATFORM_UNIX
        struct timeval tv;
        struct timeval * tvptr = &tv;
        tv.tv_usec = long(ms % 1000) * 1000;
        tv.tv_sec = long(ms / 1000);
        result = select(0, 0, 0, 0, tvptr) != (-1);
#else
        result = microsleep(ms * 1000);
#endif
    }
    return result;
}

#if defined SEQ66_PLATFORM_LINUX

/**
 *  Sleeps for the given number of microseconds. nanosleep() is a Linux
 *  function which has some advantage over sleep(3) and usleep(3), such as not
 *  interacting with signals.  It seems that it supports a non-busy wait.
 *
 * \param us
 *      Provides the desired number of microseconds to wait.  If set to 0, then
 *      a sched_yield() is called, similar to Sleep(0) in Windows.
 *
 * \return
 *      Returns true if the full sleep occurred, or if interruped by a signal.
 */

bool
microsleep (int us)
{
    bool result = us >= 0;
    if (result)
    {
        if (us == 0)
        {
            sched_yield();
        }
        else
        {
            struct timespec ts;
            ts.tv_sec = us / 1000000;
            ts.tv_nsec = (us % 1000000) * 1000;

            int rc = nanosleep(&ts, NULL);
            result = rc == 0 || rc == EINTR;
        }
    }
    return result;
}

/**
 *  Gets the current system time in microseconds.
 *
 *  Should we try rounding of the nanoseconds here and in millitime()?
 */

long
microtime ()
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (t.tv_sec * 1000000) + (t.tv_nsec / 1000);
}

/**
 *  Gets the current system time in milliseconds.
 *
 * Better?
 *
 *  return (t.tv_sec * 1000) + ((t.tv_nsec + 500000) / 1000000);
 */

long
millitime ()
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (t.tv_sec * 1000) + (t.tv_nsec / 1000000);
}

#elif defined SEQ66_PLATFORM_WINDOWS

/**
 *  This implementation comes from https://gist.github.com/ngryman/6482577 and
 *  performs busy-waiting, meaning it will NOT relinquish the processor.
 *
 * \param us
 *      Provides the desired number of microseconds to wait.
 *
 * \return
 *      Returns true only if all calls succeeded.  It doesn't matter if the
 *      wait completed, at this point.
 */

bool
microsleep (int us)
{
    bool result = us >= 0;
    if (result)
    {
        if (us == 0)
        {
            Sleep(0);
        }
        else
        {
            HANDLE timer = CreateWaitableTimer(NULL, TRUE, NULL);
            bool result = timer != NULL;
            if (result)
            {
                LARGE_INTEGER ft;
                ft.QuadPart = -(10 * (__int64) us);
                result = SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0) != 0;
                if (result)
                    result = WaitForSingleObject(timer, INFINITE) != WAIT_FAILED;

                CloseHandle(timer);
            }
        }
    }
    return result;
}

/**
 *  Gets the current system time in microseconds.  Currently, we use
 *  millitime() and multiply by 1000, in effect what Seq32 does.
 *
 * GetSystemTimeAsFileTime:
 *
 *      It gives you accuracy in 0.1 microseconds or 100 nanoseconds.
 *
 *      Note that it's Epoch different from POSIX Epoch.
 *      So to get POSIX time in microseconds you need:
 *
\verbatim
            FILETIME ft;
            GetSystemTimeAsFileTime(&ft);
            unsigned long long tt = ft.dwHighDateTime;
            tt <<=32;
            tt |= ft.dwLowDateTime;
            tt /=10;
            tt -= 11644473600000000ULL;
\endverbatim
 *
 */

long
microtime ()
{
#if defined THIS_CODE_IS_READY  // NOT!
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    unsigned long long tt = ft.dwHighDateTime;
    tt <<=32;
    tt |= ft.dwLowDateTime;
    tt /=10;
    tt -= 11644473600000000ULL;
    return tt;
#else
    return millitime() * 1000;
#endif
}

/**
 *  Gets the current system time in milliseconds. Uses the Win32 function
 *  timeGetTime().
 *
 *  The default precision of timeGetTime() can be five milliseconds or more.
 *  One can use the timeBeginPeriod() and timeEndPeriod() functions to increase
 *  the precision of timeGetTime(). If done, the minimum difference between
 *  successive values returned by timeGetTime() can be as large as the minimum
 *  period value set using timeBeginPeriod() and timeEndPeriod(). Use
 *  QueryPerformanceCounter() and QueryPerformanceFrequency() to measure
 *  short time intervals at a high resolution.
 */

long
millitime ()
{
    return long(timeGetTime());
}

#endif      // SEQ66_PLATFORM_LINUX, SEQ66_PLATFORM_WINDOWS

}           // namespace seq66

/*
 * vim: ts=4 sw=4 et ft=cpp
 */


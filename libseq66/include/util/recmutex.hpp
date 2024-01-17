#if ! defined SEQ66_RECMUTEX_HPP
#define SEQ66_RECMUTEX_HPP

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
 * \file          recmutex.hpp
 *
 *  This module declares/defines the base class for recursive mutexes.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2024-01-16
 * \license       GNU GPLv2 or above
 *
 *  This recursive mutex is implemented in pthreads due to difficulties we had
 *  with C++11's std::mutex.
 *
 *  We also had to implement a flag to indicate if locking was in place or
 *  not, but only for Windows at this time.  Seems to work fine under Linux.
 *  However, we're commenting it out for now and simply disabling the locking
 *  of the condition variable's signal() call.  Could be really problematic.
 *
 * https://en.cppreference.com/w/cpp/named_req/BasicLockable
 *
 *  BasicLockable requirements describe the minimal characteristics of types
 *  that provide exclusive blocking semantics for execution agents (i.e.
 *  threads).
 *
 *      -#  lock().  Precondition = none.  Blocks until a lock can be acquired
 *          for the current execution agent (thread, process, task). If an
 *          exception is thrown, no lock is acquired.
 *      -# unlock. Precondition = The current execution agent holds a
 *          non-shared lock. Releases the non-shared lock held by the
 *          execution agent.  Throws no exceptions.
 *
 * https://en.cppreference.com/w/cpp/named_req/Lockable
 *
 *  Lockable extends the BasicLockable requirements to include attempted
 *  locking via try_lock(). It Attempts to acquire the lock for the current
 *  execution agent (thread, process, task) without blocking. If an exception
 *  is thrown, no lock is obtained.
 */

/*
 *  We need to expose the native mutex type so that automutex can access it.
 *  We could go back to putting the mutex, automutex, and condition definitions
 *  in a single module, perhaps.  We also use pthread_mutex_t rather than the
 *  C++ mutex, as noted above.
 */

#include "seq66_platform_macros.h"      /* pick the compiler and platform   */

#include <pthread.h>

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  The mutex class provides a simple wrapper for the pthread_mutex_t type
 *  used as a recursive mutex.
 *
 *  std::experimental::propagate_const<std::unique_ptr<impl>> p_imple;
 */

class recmutex
{

public:

    /**
     *  Sets the native type of recursive mutex in use.  We declare it here,
     *  rather than in the impl classes, because it needs to be exposed for
     *  the use of automutex. The pthread_mutex_t type is a union of a structure
     *  and a byte array. The structure contains a lock integer, a count, an
     *  owner integer, and some other values.
     */

    using native = pthread_mutex_t;

private:

    /**
     *  Provides a recursive mutex that can be used by the whole
     *  application, and is, apparently.
     */

    static native sm_global_mutex;

#if defined SEQ66_PLATFORM_FREEBSD

    /**
     *  Needed for setting up a recursive mutex.
     */

    pthread_mutexattr_t m_mutex_attributes;

#endif

    /**
     *  Provides a mutex lock usable by a single module or class.
     *  However, this mutex ends up being a copy of the static
     *  sm_global_mutex (and, of course, a different "object").
     */

    mutable native m_mutex_lock;

public:

    recmutex ();
    recmutex (recmutex &&) = delete;                    /* default; */
    recmutex (const recmutex &);                        /* delete;  */
    recmutex & operator = (recmutex &&) = delete;       /* default; */
    recmutex & operator = (const recmutex &);           /* delete;  */
    ~recmutex ();                                       /* default; */

    void lock () const;
    void unlock () const;

    /*
     * bool try_lock () noexcept;
     */

    native & native_locker () const
    {
        return m_mutex_lock;
    }

private:

#if defined USE_GLOBAL_MUTEX
    static void init_global_mutex ();
#endif
    void init ();
    void destroy ();

};          // class recmutex

}           // namespace seq66

#endif      // SEQ66_RECMUTEX_HPP

/*
 * recmutex.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


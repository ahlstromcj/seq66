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
 * \file          recmutex.cpp
 *
 *  This module declares/defines the base class for mutexes.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2024-01-16
 * \license       GNU GPLv2 or above
 *
 *  Seq66 needs a mutex for sequencer operations. We have finally, after a
 *  week of monkeying around with std::condition_variable, which requires
 *  std::mutex, decided to stick with the old pthreads implementation for now.
 */

#include "util/recmutex.hpp"            /* seq66::recmutex                  */

/**
 *  FreeBSD pthreads is different from the others.
 *
 *  Using pthread_mutex_init() and pthread_mutex_destroy() seems to hang
 *  up automutex usage.  The mutex gets destroyed in the first thread in
 *  which it goes out of scope.`
 */

#if defined SEQ66_PLATFORM_FREEBSD

    /* TODO */

#else

#define SEQ66_USE_MUTEX_INITIALIZER

#if defined SEQ66_PLATFORM_MING_OR_WINDOWS

/**
 *  Using the init()/destroy() paradigm with an automutex seems to hang
 *  things up, so we define this macro to do it the old way.
 *
 *  The MingW compiler (at least on Windows) and the Microsoft Visual Studio
 *  compiler do not support the pthread PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
 *  macro.
 *
 *  Currently, we do not use the CRITICAL_SECTION or alternate threads API.
 *  Kept here for reference
 */

#define MUTEX_INITIALIZER   PTHREAD_RECURSIVE_MUTEX_INITIALIZER

#else

#define MUTEX_INITIALIZER   PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
#endif

#endif      // FreeBSD

namespace seq66
{

/**
 *  Define the static recursive mutex and its condition variable.
 */

recmutex::native recmutex::sm_global_mutex;

#if defined USE_GLOBAL_MUTEX

/**
 *  We need a static flag to guarantee that the static native mutex object
 *  is initialized.
 */

void
recmutex::init_global_mutex ()
{
    static bool s_global_mutex_initialized = false;
    if (! s_global_mutex_initialized)
    {
        s_global_mutex_initialized = true;
        recmutex::sm_global_mutex = MUTEX_INITIALIZER;
    }
}

#endif

/**
 *  Constructor for recmutex.
 */

recmutex::recmutex () :
#if defined SEQ66_PLATFORM_FREEBSD
    m_mutex_attributes  (),             /* uninit'd pthread_mutexattr_t     */
#endif
    m_mutex_lock        ()              /* uninitialized pthread_mutex_t    */
{
#if defined USE_GLOBAL_MUTEX
    init_global_mutex();                /* might not need global mutex, tho */
#endif
    init();                             /* m_mutex_lock = MUTEX_INITIALIZER */
}

/**
 *  Copy constructor. It doesn't make any sense to copy a mutex (and, in fact,
 *  std::mutex is Noncopyable and Nonmoveable.
 *
 *  However, there are objects that use a mutex and should still be copyable.
 *  So rather than defining a default copy constructor or deleting it, we
 *  make one that creates a new mutex. We do not need to call the static
 *  function init_global_mutex().  We could call it, but nothing would happen.
 */

recmutex::recmutex (const recmutex & /* rhs */ ) : m_mutex_lock ()
{
    init();
}

/**
 *  Similarly, we want to support the principal assignment operator.
 */

recmutex &
recmutex::operator = (const recmutex & rhs)
{
    if (this != & rhs)
    {
#if defined USE_GLOBAL_MUTEX
        init_global_mutex();
#endif
        init();
    }
    return *this;
}

recmutex::~recmutex ()
{
    destroy();
}

/**
 *  Locks the recmutex.
 */

void
recmutex::lock () const
{
    (void) pthread_mutex_lock(&m_mutex_lock);
}

/**
 *  Unlocks the recmutex.
 */

void
recmutex::unlock () const
{
    (void) pthread_mutex_unlock(&m_mutex_lock);
}

/**
 *  FreeBSD prthreads is different from the others.
 */

#if defined SEQ66_PLATFORM_FREEBSD

void
recmutex::init ()
{
    int rc = pthread_mutexattr_init(&m_mutex_attributes);
    if (rc == 0)
    {
        rc = pthread_mutexattr_settype
        (
            &m_mutex_attributes, PTHREAD_MUTEX_RECURSIVE
        );
        if (rc == 0)
        {
            rc = pthread_mutex_init(&m_mutex_lock, &m_mutex_attributes);
        }
    }
    if (rc != 0)
    {
        /* TODO */
    }
}

void
recmutex::destroy ()
{
    int rc = pthread_mutex_unlock(&m_mutex_lock);
    if (rc == 0)
    {
        rc = pthread_mutex_destroy(&m_mutex_lock);
        if (rc == 0)
            rc = pthread_mutexattr_destroy(&m_mutex_attributes);
    }
    if (rc != 0)
    {
        /* TODO */
    }
}

#else

/**
 *  Initializes the recmutex.
 *
 *  From https://pages.cs.wisc.edu/~remzi/OSTEP/
 *      "Operating Systems: Three Easy Pieces":
 *
 *  With POSIX threads, there are two ways to initialize locks. One way to do
 *  this is to use PTHREAD_MUTEX_INITIALIZER, as follows: pthread_mutex_t lock
 *  = PTHREAD_MUTEX_INITIALIZER;
 *
 *  Doing so sets the lock to the default values and thus makes the lock
 *  usable.  The dynamic way to do it (i.e., at run time) is to make a call to
 *  pthread_mutex_init() as noted below.
 *
 *  The first argument to this routine is the address of the lock itself,
 *  whereas the second is an optional set of attributes. Read more about the
 *  attributes yourself; passing NULL in simply uses the defaults. Either way
 *  works, but we usually use the dynamic (latter) method.
 */

void
recmutex::init ()
{
#if defined SEQ66_USE_MUTEX_INITIALIZER
    m_mutex_lock = MUTEX_INITIALIZER;
#else
    int rc = pthread_mutex_init(&m_mutex_lock, NULL);
    if (rc != 0)
    {
        // what to do?
    }
#endif
}

void
recmutex::destroy ()
{
#if ! defined SEQ66_USE_MUTEX_INITIALIZER
    pthread_mutex_unlock(&m_mutex_lock);
#endif
}

#endif      // FreeBSD

}           // namespace seq66

/*
 * recmutex.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


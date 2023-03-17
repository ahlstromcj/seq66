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
 * \updates       2023-03-09
 * \license       GNU GPLv2 or above
 *
 *  Seq66 needs a mutex for sequencer operations. We have finally, after a
 *  week of monkeying around with std::condition_variable, which requires
 *  std::mutex, decided to stick with the old pthreads implementation for now.
 */

#include "seq66_platform_macros.h"      /* pick the compiler and platform   */
#include "util/recmutex.hpp"            /* seq66::recmutex                  */

/**
 *  Using the init()/destroy() paradigm with an automutex seems to hang
 *  things up, so we define this macro to do it the old way.
 */

#define SEQ66_USE_MUTEX_INITIALIZER

/**
 *  The MingW compiler (at least on Windows) and the Microsoft Visual Studio
 *  compiler do not support the pthread PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
 *  macro.
 */

#if defined SEQ66_PLATFORM_MING_OR_WINDOWS
#define MUTEX_INITIALIZER   PTHREAD_RECURSIVE_MUTEX_INITIALIZER
#else
#define MUTEX_INITIALIZER   PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
#endif

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Define the static recursive mutex and its condition variable.
 */

recmutex::native recmutex::sm_global_mutex;

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

/**
 *  Constructor for recmutex.
 */

recmutex::recmutex () : m_mutex_lock () /* uninitialized pthread_mutex_t    */
{
    init_global_mutex();                /* might not need global mutex, tho */
    init();                             /* m_mutex_lock = MUTEX_INITIALIZER */
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
    pthread_mutex_lock(&m_mutex_lock);
}

/**
 *  Unlocks the recmutex.
 */

void
recmutex::unlock () const
{
    pthread_mutex_unlock(&m_mutex_lock);
}

/**
 *  Unlocks the recmutex.
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

}           // namespace seq66

/*
 * recmutex.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


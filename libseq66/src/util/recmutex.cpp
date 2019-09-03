/*
 *  This file is part of seq66.
 *
 *  seq66 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  seq66 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq66; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          recmutex.cpp
 *
 *  This module declares/defines the base class for mutexes.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2019-06-23
 * \license       GNU GPLv2 or above
 *
 *  2019-04-21 Reverted to commit 5b125f71 to stop GUI deadlock :-(
 *
 *  Seq66 needs a mutex for sequencer operations. We have finally, after a week
 *  of monkeying around with std::condition_variable, which requires std::mutex,
 *  decided to stick with the old pthreads implementation for now.
 */

#include "util/recmutex.hpp"

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

#if defined PLATFORM_MINGW
        recmutex::sm_global_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
#else
        recmutex::sm_global_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif
    }
}

/**
 *  The constructor assigns the global recursive mutex to the local locking
 *  mutex.
 */

#ifdef USE_IFFY_GLOBAL_MUTEX

recmutex::recmutex () :
    m_mutex_lock (sm_global_mutex)
{
    init_global_mutex();                /* or move to lock()/unlock()?      */
}

#else   // USE_IFFY_GLOBAL_MUTEX

recmutex::recmutex () :
    m_mutex_lock ()                     /* uninitialized pthread_mutex_t    */
{
    init_global_mutex();                /* might not need global mutex, tho */

#if defined SEQ66_PLATFORM_MINGW
    m_mutex_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
#else
    m_mutex_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif
}

#endif  // USE_IFFY_GLOBAL_MUTEX

void
recmutex::lock () const
{
    pthread_mutex_lock(&m_mutex_lock);
}

void
recmutex::unlock () const
{
    pthread_mutex_unlock(&m_mutex_lock);
}

}           // namespace seq66

/*
 * recmutex.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


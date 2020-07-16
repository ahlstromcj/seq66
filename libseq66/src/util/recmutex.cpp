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
 * \updates       2020-07-16
 * \license       GNU GPLv2 or above
 *
 *  Seq66 needs a mutex for sequencer operations. We have finally, after a
 *  week of monkeying around with std::condition_variable, which requires
 *  std::mutex, decided to stick with the old pthreads implementation for now.
 */

#include "seq66_platform_macros.h"      /* pick the compiler and platform   */
#include "util/recmutex.hpp"            /* seq66::recmutex                  */

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

recmutex::recmutex () :
#if defined SEQ66_USE_MUTEX_UNLOCKED_FLAG
    m_is_locked  (false)
#endif
    m_mutex_lock ()                     /* uninitialized pthread_mutex_t    */
{
    init_global_mutex();                /* might not need global mutex, tho */
    m_mutex_lock = MUTEX_INITIALIZER;
}

/**
 *  Locks the recmutex.
 */

void
recmutex::lock () const
{
    pthread_mutex_lock(&m_mutex_lock);
#if defined SEQ66_USE_MUTEX_UNLOCKED_FLAG
    m_is_locked = true;
#endif
}

/**
 *  Unlocks the recmutex.
 */

void
recmutex::unlock () const
{
#if defined SEQ66_USE_MUTEX_UNLOCKED_FLAG
    if (m_is_locked)
    {
        m_is_locked = false;
        pthread_mutex_unlock(&m_mutex_lock);
    }
#else
    pthread_mutex_unlock(&m_mutex_lock);
#endif
}

}           // namespace seq66

/*
 * recmutex.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


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
 * \file          condition.cpp
 *
 *  This module declares/defines the base class for mutexes.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2020-07-15
 * \license       GNU GPLv2 or above
 *
 *  2019-04-21 Reverted to commit 5b125f71 to stop GUI deadlock :-(
 *
 *  Seq66 needs a mutex and a condition-variable for sequencer operations.
 *  We tried to convert from pthreads to std::condition_variable, but it caused
 *  issues such as freezing the user interfaces.  For now, even though we don't
 *  have to hide it, we using the "pimpl" paradigm to use a pthreads
 *  implementation.  See the Doxygen documentation for more information.
 */

#include <memory>                       /* std::make_unique()               */
#include <pthread.h>

#include "util/condition.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 * --------------------------------------------------------------------------
 *  condition::impl nested class for pthreads
 * --------------------------------------------------------------------------
 */

class condition::impl
{

private:

    using variable = pthread_cond_t;

    /**
     *  We need a global condition-variable so that it can coordinate threads in
     *  different objects.
     */

    static variable sm_cond;

    /**
     *  Provides a class-specific way to wait for a condition to change.  It is
     *  normally set to the static variable sm_cond.
     */

    variable & m_cond;

    /**
     *  Access to the outer class's recmutex.
     */

    recmutex & m_rec_mutex;

public:

    impl (recmutex & rm) : m_cond (sm_cond), m_rec_mutex (rm)
    {
        // no code
    }

    /**
     *  Signals the condition variable.
     */

    void signal ()
    {
        pthread_cond_signal(&m_cond);
    }

    /**
     *  Waits for the condition variable.  If we use std::condition_variable, we
     *  would need to provide a non-recursive mutex for locking.  This
     *  somehow freezes some things.  A battle we will fight another day.
     */

    void wait ()
    {
        pthread_cond_wait(&m_cond, &(m_rec_mutex.native_locker()));
    }

    /**
     *  Nanoseconds:  convert milliseconds to microsecond
     */

    void wait (int ms)
    {
        struct timespec w;
        w.tv_sec = long(ms / 1000);
        w.tv_nsec = long((ms * 1000) % 1000000) * 1000;
        pthread_cond_timedwait(&m_cond, &(m_rec_mutex.native_locker()), &w);
    }

};          // class mutex::impl for pthreads

/**
 *  Define the static condition variable used by all mutex locks.
 */

condition::impl::variable condition::impl::sm_cond;

}           // namespace seq66

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 * --------------------------------------------------------------------------
 *  condition class p_imple wrapper
 * --------------------------------------------------------------------------
 */

/**
 *  Initialize the condition variable with the global variable.
 */

condition::condition () :
    m_mutex_lock    (),
    p_imple         (std::make_unique<impl>(m_mutex_lock))
{
    // Empty body
}

condition::~condition () = default;
condition & condition::operator = (condition &&) = default;

void
condition::signal ()
{
    p_imple->signal();
}

void
condition::wait ()
{
    p_imple->wait();
}

void
condition::wait (int ms)
{
    p_imple->wait(ms);
}

}           // namespace seq66

/*
 * condition.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


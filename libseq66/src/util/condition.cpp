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
 * \file          condition.cpp
 *
 *  This module declares/defines the base class for mutexes.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2024-01-16
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
     *  We need a global condition-variable so that it can coordinate threads
     *  in different objects.
     */

    static variable sm_cond;

    /**
     *  Provides a class-specific way to wait for a condition to change.  It
     *  is normally set to the static variable sm_cond.
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
     *  Waits for the condition variable.  If we use std::condition_variable,
     *  we would need to provide a non-recursive mutex for locking.  This
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
    p_imple         (std::make_unique<condition::impl>(m_mutex_lock))
{
    // Empty body
}

condition::condition (const condition & /* rhs */) :
    m_mutex_lock    (),
    p_imple         (std::make_unique<condition::impl>(m_mutex_lock))
{
    // Empty body
}

condition &
condition::operator = (const condition & rhs)
{
    if (this != & rhs)
    {
        m_mutex_lock = rhs.m_mutex_lock;
        p_imple.reset(std::make_unique<condition::impl>(m_mutex_lock).get());
    }
    return *this;
}

condition::~condition ()
{
    // No code needed at this time, we think
}


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

/*
 * --------------------------------------------------------------------------
 *  A C++-only implmenation
 * --------------------------------------------------------------------------
 *
 *  How does the synchronisation work? The program has two threads. They use a
 *  wait function and a signal function, respectively.  The signal() function
 *  in one thread notifies, using the condition variable, that it is done with
 *  the preparation for work. While holding the lock, the other thread waits
 *  for its notification: using the condition variable, the lock mutex, and
 *  the predicate function.
 *
 *  The sender and receiver both need a lock. In the case of the sender, a
 *  std::lock_guard is sufficient, because it calls to lock and unlock only
 *  once.  In the case of the receiver, a std::unique_lock is necessary
 *  because it usually frequently locks and unlocks its mutex.
 *
 *  See: https://www.modernescpp.com/index.php/
 *          c-core-guidelines-be-aware-of-the-traps-of-condition-variables
 */

synchronizer::synchronizer () :
    m_helper_mutex      (),
    m_condition_var     ()
{
    // no other code
}

bool
synchronizer::wait ()
{
    std::unique_lock<std::mutex> locker(m_helper_mutex);
    m_condition_var.wait(locker, [this]{ return predicate(); });
    return predicate();
}

void
synchronizer::signal ()
{
    std::lock_guard<std::mutex> locker(m_helper_mutex);
    m_condition_var.notify_one();
}

}           // namespace seq66

/*
 * condition.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


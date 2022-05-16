#if ! defined SEQ66_CONDITION_HPP
#define SEQ66_CONDITION_HPP

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
 * \file          condition.hpp
 *
 *  This module declares/defines the base class for coordinating a condition
 *  variable and a mutex.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2022-05-16
 * \license       GNU GPLv2 or above
 *
 *  This module defines the class seq66::condition_var, which provides a common
 *  usage paradigm, for the performer object.  Note that the mutex header
 *  defines recursive_mutex, mutex, unique_lock, and lock_guard.
 *
 *  2019-04-21 Reverted to commit 5b125f71 to stop GUI deadlock :-(
 */

#include <condition_variable>           /* for seq66::synchronizer class    */

#include "util/recmutex.hpp"            /* seq66::recmutex wrapper class    */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  A mutex works best in conjunction with a condition variable.  The "has-a"
 *  relationship is more logical than an "is-a" relationship.  Note the
 *  additional member function condition::wait_on_predicate(), which allow the
 *  usage of a test function returning a boolean.
 */

class condition
{

private:

    /**
     *  A nested implementation class that is sequestered inside the
     *  condition.cpp module.
     */

    class impl;

    /**
     *  Provides a recursive mutex that can be used by the whole application,
     *  and is.
     */

    static recmutex sm_recursive_mutex;

    /**
     *  Our recursive mutex (not the normal C++11 non-recursive mutex) used
     *  for locking the conditional wait operation.  We use our own automutex,
     *  not the recursive mkutex that std::unique_lock<> require.  Provides a
     *  mutex lock usable by a single module or class.  However, this mutex
     *  ends up being a copy of the static sm_recursive_mutex (and, of course,
     *  a different "object").
     */

    mutable recmutex m_mutex_lock;

    /**
     *  A pointer to the class implementation.  Needs to use the mutex created
     *  above.
     */

    std::unique_ptr<impl> p_imple;

public:

    condition ();
    ~condition ();
    condition (condition &&) = default;
    condition (const condition &) = delete;
    condition & operator = (condition &&);
    condition & operator = (const condition &) = delete;

    void lock () const
    {
        m_mutex_lock.lock();
    }

    void unlock () const
    {
        m_mutex_lock.unlock();
    }

    recmutex & locker ()
    {
        return m_mutex_lock;
    }

    void signal ();
    void wait ();
    void wait (int ms);

};          // class condition

/*
 * --------------------------------------------------------------------------
 *  A C++-only implmenation
 * --------------------------------------------------------------------------
 */

class synchronizer
{

private:

    /**
     *  Used for locking the condition variable.
     */

    std::mutex m_helper_mutex;

    /**
     *  The new-style (for Seq66) condition variable.  It replaces the pthread
     *  implementation, at least when used in the performer class.
     */

    std::condition_variable m_condition_var;

public:

    synchronizer ();
    synchronizer (const synchronizer &) = delete;
    synchronizer & operator = (const synchronizer &) = delete;
    virtual ~synchronizer () = default;

    bool wait ();
    void signal ();

    /**
     * The user of this class must derive a class to properly define this
     * function.  It should return true when some internal thread is ready to
     * run or when the thread has raised a flag for an exit.  See
     * performer::predicate() for a useful override.
     */

    virtual bool predicate () const = 0;

};          // class synchronzier

}           // namespace seq66

#endif      // SEQ66_CONDITION_HPP

/*
 * condition.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


#if ! defined SEQ66_AUTOMUTEX_HPP
#define SEQ66_AUTOMUTEX_HPP

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
 * \file          automutex.hpp
 *
 *  This module declares/defines the base class for mutexes.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2019-04-16
 * \license       GNU GPLv2 or above
 *
 *  This module defines the following classes:
 *
 *      -   seq66::mutex.  An alias for std::recursive_mutex.
 *      -   seq66::automutex. A way to lock a function exception-safely and
 *          easily.
 *
 *          2019-04-21 Reverted to commit 5b125f71 to stop GUI deadlock :-(
 */

#include "util/recmutex.hpp"            /* seq66::recmutex wrapper class    */

namespace seq66
{

/**
 *  Provides a mutex that locks automatically when created, and unlocks
 *  when destroyed.  This has a couple of benefits.  First, it is threadsafe
 *  in the face of exception handling.  Secondly, it can be done with just one
 *  line of code.
 *
 *  It could potentially be replaced by std::lock_guard<std::recursive_mutex> or
 *  std::lock_guard<seq66::mutex>.  However, it provides lock() and unlock()
 *  functions for extra flexibility and danger.  :-)
 *
 *  How to use it? An example:
 *
 *                  int test_func (recmutex & mut, const sequence & s)
 *                  {
 *  Created ----------> automutex locker(mut);
 *  Value to return --> return s.events().count();
 *  Destroyed ----> }
 *
 *  Quoting the standard (§3.7.3/3): "If a variable with automatic storage
 *  duration has initialization or a destructor with side effects, it shall
 *  not be destroyed before the end of its block, nor shall it be eliminated
 *  as an optimization even if it appears to be unused." The end of the block,
 *  for a function, is after the return statement.
 */

class automutex
{

private:

    /**
     *  Provides the mutex reference to be used for locking.
     */

    recmutex & m_safety_mutex;

private:                        /* do not allow these functions to be used  */

    automutex () = delete;
    automutex (const automutex &) = delete;
    automutex & operator = (const automutex &) = delete;

public:

    /**
     *  Principal constructor gets a reference to a mutex parameter, and
     *  then locks the mutex.
     *
     * \param my_mutex
     *      The caller's mutex to be used for locking.
     */

    automutex (recmutex & my_mutex) : m_safety_mutex (my_mutex)
    {
        lock();
    }

    /**
     *  The destructor unlocks the mutex.
     */

    ~automutex ()
    {
        unlock();
    }

    /**
     *  The lock() and unlock() functions are provided for additional
     *  flexibility in usage.
     */

    void lock ()
    {
        m_safety_mutex.lock();
    }

    void unlock ()
    {
        m_safety_mutex.unlock();
    }

};          // class automutex

}           // namespace seq66

#endif      // SEQ66_AUTOMUTEX_HPP

/*
 * automutex.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


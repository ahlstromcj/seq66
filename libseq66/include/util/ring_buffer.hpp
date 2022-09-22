#if ! defined SEQ66_RING_BUFFER_HPP
#define SEQ66_RING_BUFFER_HPP

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
 * \file          ring_buffer.cpp
 *
 *  This module defines our own ringbuffer that support objects, not just
 *  characters.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2022-09-19
 * \updates       2022-09-21
 * \license       GNU GPLv2 or above
 */

#include <cstddef>
#include <sys/types.h>
#include <vector>

#include "seq66_features.h"             /* SEQ66_PLATFORM_DEBUG macro       */

#undef  SEQ66_USE_MEMORY_LOCK           /* TODO: needs a lot of work !      */

#if defined SEQ66_USE_MEMORY_LOCK
#include <sys/mman.h>
#endif

namespace seq66
{

template <typename TYPE>
class ring_buffer
{

public:

    using value_type = TYPE;
    using reference = TYPE & ;
    using const_reference = const TYPE &;
    using size_type = std::size_t;
    using container = std::vector<value_type>;

private:

    container m_buffer;         /**< Container for all push/popped items.   */
    size_type m_buffer_size;    /**< Constant power-of-two container size.  */
    size_type m_contents_size;  /**< Number of active entries in container. */
    volatile size_type m_tail;  /**< Index where next item is written.      */
    volatile size_type m_head;  /**< Index where next item is read.         */
    size_type m_size_mask;      /**< Restricts index to < buffer size.      */
    bool m_locked;              /**< Is memory locked? NOT YET SUPPORTED.   */
    int m_dropped;              /**< Number of items overwritten in run.    */

public:

    explicit ring_buffer (size_type sz);
    ~ring_buffer ();

    bool mlock ();

    /**
     *  Reset the read and write pointers to zero. This is not thread safe.
     *  Neither is the clear() function.
     */

    void reset ()
    {
        m_head = m_tail = 0;
    }

    void clear ()
    {
        m_dropped = m_contents_size = 0;
        reset();
        initialize();
    }

    int count () const
    {
        return int(m_contents_size);
    }

    bool empty () const
    {
        return count() == 0;
    }

    /*
     * Useful?
     */

    bool default_slot (const_reference item)
    {
        static TYPE empty_value;
        return item == empty_value;
    }

    int dropped () const
    {
        return m_dropped;
    }

    void write_advance ();
    void read_advance ();
    size_type write_space () const;
    size_type read_space () const;
    size_type read (reference dest);
    size_type write (const_reference src);

    void push_back (const value_type & value);
    void pop_front ()
    {
        if (m_contents_size > 0)
            increment_head();
    }

    /*
     * Returns reference to the first element in the queue. This element will
     * be the first element to be removed on a call to pop().  The return
     * value might not be valid, either a default-constructed object or an
     * old and "overwritten" value. An alternative is to call the read()
     * function and check the return value.
     */

    reference front ()
    {
        return m_buffer[m_head];
    }

    const_reference front () const
    {
        return m_buffer[m_head];
    }

    /**
     *  Currently there's no way to be sure that the back item is a
     *  valid item.
     */

    reference back ()
    {
        return m_buffer[previous_tail()];           // return m_buffer[m_tail];
    }

    const_reference back () const
    {
        return m_buffer[previous_tail()];           // return m_buffer[m_tail];
    }

private:    // helper functions

    void initialize ();

    void increment_head ()
    {
        if (m_contents_size > 0)
        {
            ++m_head;
            --m_contents_size;
            if (m_head == m_buffer_size)
                m_head = 0;                                 /* wrap around  */
        }
    }

    void increment_tail ()
    {
        ++m_tail;
        ++m_contents_size;
        if (m_tail == m_buffer_size)
            m_tail = 0;                                     /* wrap around  */
    }

    size_t previous_tail ()
    {
        return m_tail > 0 ? m_tail - 1 : m_buffer_size -1  ;
    }

};          // class ring_buffer<TYPE>

/**
 *  Create a new ringbuffer to hold at least `sz' elements (TYPE) of data.
 *  The actual buffer size is rounded up to the next power of two.
 */

template<typename TYPE>
ring_buffer<TYPE>::ring_buffer (size_type sz) :
    m_buffer        (),
    m_buffer_size   (0),
    m_contents_size (0),
    m_tail          (0),
    m_head          (0),                    /* supports empty buffer case   */
    m_size_mask     (0),
    m_locked        (false),
    m_dropped       (0)
{
    int power_of_two;
    for (power_of_two = 1; 1 << power_of_two < int(sz); ++power_of_two)
        ;

    size_type psize = size_t(1 << power_of_two);
    m_buffer.reserve(psize);
    m_buffer_size = psize;
    m_size_mask = psize - 1;                /* 0xFF... for index safety     */
    initialize();
}

/**
 *  Free all data associated with the ringbuffer `m_rb'.
 *
 *  Note that we will have some work to do (like writing an allocator that uses
 *  an unswappable block of memory for the vector data) if we define this macro.
 */

template<typename TYPE>
ring_buffer<TYPE>::~ring_buffer ()
{
#if defined SEQ66_USE_MEMORY_LOCK
    if (m_locked)
        ::munlock(m_buffer, m_buffer_size);
#endif
}

template<typename TYPE>
void
ring_buffer<TYPE>::initialize ()
{
    TYPE empty_value;
    m_buffer.clear();
    m_buffer.reserve(m_buffer_size);
    for (size_t i = 0; i < m_buffer_size; ++i)
        m_buffer.push_back(empty_value);        /* prepare buffer for usage */
}

/**
 *  Lock the data block of `rb' using the system call 'mlock'.
 *  Not nearly ready for prime time!
 */

template<typename TYPE>
bool
ring_buffer<TYPE>::mlock ()
{
#if defined SEQ66_USE_MEMORY_LOCK
    if (::mlock(m_buffer, m_buffer_size) != 0)
        return false;

    m_locked = true;
    return true;
#else
    return false;
#endif
}

/**
 *  Return the number of elements available for writing.  This is the number
 *  of elements in front of the write/tail pointer and behind the read/head
 *  pointer.
 */

template<typename TYPE>
std::size_t
ring_buffer<TYPE>::write_space () const
{
    size_type t = m_tail;                           /* index for writing    */
    size_type h = m_head;                           /* index for reading    */
    if (t > h)
        return ((h - t + m_buffer_size) & m_size_mask) - 1;
    else if (t < h)
        return (h - t) - 1;
    else
        return m_buffer_size - m_contents_size;
}

template<typename TYPE>
void
ring_buffer<TYPE>::write_advance ()
{
#if defined USE_OLD_CODE
    ++m_tail;
    m_tail &= m_size_mask;
#else
    increment_tail();
#endif
}

/**
 *  Since we only push one element at a time, the return code is used to determine
 *  the number of elements currently active in the ring_buffer, unless 0 is
 *  returned, which indicates an error (no space left).
 */

template<typename TYPE>
std::size_t
ring_buffer<TYPE>::write (const_reference src)
{
    size_type result = 0;
    size_type write_cnt = write_space();
    if (write_cnt > 0)
    {
        push_back(src);
        result = m_contents_size;
    }
    return result;
}

/**
 *  Return the number of elements (TYPE) available for reading.  This is the
 *  number of elements in front of the read pointer and behind the write
 *  pointer.
 */

template<typename TYPE>
std::size_t
ring_buffer<TYPE>::read_space () const
{
    if (count() > 0)
    {
        size_type t = m_tail;                       /* index for writing    */
        size_type h = m_head;                       /* index for reading    */
        if (t > h)
            return t - h;
        else if (t == h)
            return m_contents_size;
        else
            return (t - h + m_buffer_size) & m_size_mask;
    }
    else
        return 0;
}

template<typename TYPE>
void
ring_buffer<TYPE>::read_advance ()
{
    ++m_head;
    m_head &= m_size_mask;
}

/**
 *  The copying data reader.  Unlike the original "C" version, this function
 *  does not copy `cnt' bytes from `rb'.  Instead it copies one element to
 *  the destination and returns 1 if that works.
 *
 *  Unlike front(), this function and pop_front() "remove" the element.
 *  The result return is the number of elements still stored.
 */

template<typename TYPE>
std::size_t
ring_buffer<TYPE>::read (reference dest)
{
    size_t result = 0;
    size_type read_cnt = read_space();
    if (read_cnt > 0)
    {
        dest = m_buffer[m_head];
        pop_front();
        result = m_contents_size;
    }
    return result;
}

/**
 *  Increment the tail, then increment the head. This function supports the
 *  special case of an empty buffer.
 */

template<typename TYPE>
void
ring_buffer<TYPE>::push_back (const value_type & item)
{
    if (m_contents_size == 0)
    {
        m_tail = m_head;
        m_buffer[m_tail] = item;
        increment_tail();
    }
    else if (m_contents_size < m_buffer_size)
    {
        m_buffer[m_tail] = item;
        increment_tail();
    }
    else                                    /* accept item and drop front() */
    {
        increment_head();
        m_buffer[m_tail] = item;
        increment_tail();
        ++m_dropped;
    }
}

/*
 *  Free functions (for testing).
 */

#if defined SEQ66_PLATFORM_DEBUG

extern bool run_ring_test ();

#endif

}           // namespace seq66

#endif      // SEQ66_RING_BUFFER_HPP

/*
 * ring_buffer.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


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
 * \updates       2022-09-19
 * \license       GNU GPLv2 or above
 */

#include <cstddef>
#include <sys/types.h>
#include <vector>

#include "seq66_features.h"             /* SEQ66_PLATFORM_DEBUG macro       */

#if defined SEQ66_USE_MEMORY_LOCK
#include <sys/mman.h>
#endif

namespace seq66
{

template <class TYPE>
class ring_buffer
{

public:

    using value_type = TYPE;
    using reference = TYPE & ;
    using const_reference = const TYPE &;
    using size_type = std::size_t;
    using container = std::vector<value_type>;

private:

    container m_buffer;
    size_type m_buffer_size;
    size_type m_contents_size;
    volatile size_type m_tail;
    volatile size_type m_head;
    size_type m_size_mask;
    bool m_locked;

public:

    ring_buffer (size_type sz);
    ~ring_buffer ();

    bool mlock ();
    void reset ();
    void write_advance ();
    void read_advance ();
    size_type write_space () const;
    size_type read_space () const;

    size_type read (TYPE & dest);
    size_type write (const TYPE & src);

    void clear ()
    {
        m_head = m_tail = m_contents_size = 0;
    }

    void push_back (const value_type & value);
    void pop_front ();

    /*
     * Returns reference to the first element in the queue. This element will
     * be the first element to be removed on a call to pop().
     */

    reference front ()
    {
        return m_buffer[m_head];
    }

    const_reference front () const
    {
        return m_buffer[m_head];
    }

    reference back ()
    {
        return m_buffer[m_tail];
    }

    const_reference back () const
    {
        return m_buffer[m_tail];
    }

};          // class ring_buffer<TYPE>

/**
 *  Create a new ringbuffer to hold at least `sz' elements (TYPE) of data.
 *  The actual buffer size is rounded up to the next power of two.
 */

template<class TYPE>
ring_buffer<TYPE>::ring_buffer (size_type sz) :
    m_buffer        (),
    m_buffer_size   (0),
    m_contents_size (0),
    m_size_mask     (0),
    m_tail          (0),
    m_head          (0),
    m_locked        (false)
{
    size_type power_of_two;
    for (power_of_two = 1; 1 << power_of_two < sz; ++power_of_two)
        ;

    size_type psize = 1 << power_of_two;
    m_buffer.reserve(psize);
    m_buffer_size = psize;
    m_size_mask = psize - 1;
}

/**
 *  Free all data associated with the ringbuffer `m_rb'.
 *
 *  Note that we will have some work to do (like writing an allocator that uses
 *  an unswappable block of memory for the vector data) if we define this macro.
 */

template<class TYPE>
ring_buffer<TYPE>::~ring_buffer ()
{
#if defined SEQ66_USE_MEMORY_LOCK
    if (m_locked)
        ::munlock(m_buffer, m_buffer_size);
#endif
}

/**
 *  Lock the data block of `rb' using the system call 'mlock'.
 */

template<class TYPE>
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
 *  Reset the read and write pointers to zero. This is not thread safe.
 */

template<class TYPE>
void
ring_buffer<TYPE>::reset ()
{
    m_head = m_tail = 0;
}

/**
 *  Return the number of elements available for writing.  This is the number of
 *  elements in front of the write/tail pointer and behind the read/head pointer.
 */

template<class TYPE>
std::size_t
ring_buffer<TYPE>::write_space () const
{
    size_type w = m_tail;
    size_type r = m_head;
    if (w > r)
        return ((r - w + m_buffer_size) & m_size_mask) - 1;
    else if (w < r)
        return (r - w) - 1;

    return m_buffer_size - 1;
}

template<class TYPE>
void
ring_buffer<TYPE>::write_advance ()
{
    ++m_tail;
    m_tail &= m_size_mask;
}

/**
 *  Since we only push one element at a time, the return code is used to determine
 *  the number of elements currently active in the ring_buffer, unless 0 is
 *  returned, which indicates an error (no space left).
 */

template<class TYPE>
std::size_t
ring_buffer<TYPE>::write (const TYPE & src)
{
    size_type result = 0;
    size_type free_cnt = write_space();
    if (free_cnt > 0)
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

template<class TYPE>
std::size_t
ring_buffer<TYPE>::read_space () const
{
    size_type w = m_tail;
    size_type r = m_head;
    if (w > r)
        return w - r;

    return (w - r + m_buffer_size) & m_size_mask;
}

template<class TYPE>
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

template<class TYPE>
std::size_t
ring_buffer<TYPE>::read (TYPE & dest)
{
    size_t result = 0;
    size_type free_cnt = read_space();
    if (free_cnt > 0)
    {
        dest = m_buffer[m_head];
        result = ++m_contents_size;
    }
    return result;
}

/**
 *  Increment the tail, then increment the head.
 */

template<class TYPE>
void
ring_buffer<TYPE>::push_back (const value_type & item)
{
    ++m_tail;
    ++m_contents_size;
    if (m_tail == m_buffer_size)
        m_tail = 0;                                 /* wrap around  */

    if (m_contents_size > m_buffer_size)
    {
        ++m_head;
        --m_contents_size;
        if (m_head == m_buffer_size)
            m_head = 0;
    }
    m_buffer[m_tail] = item;
}

/**
 * Increment the head.
 */

template<class TYPE>
void
ring_buffer<TYPE>::pop_front ()
{
    if (m_contents_size == 0)
        return;

    ++m_head;
    --m_contents_size;
    if (m_head == m_buffer_size)
        m_head = 0;
}

}           // namespace seq66

/*
 *  Free functions (for testing).
 */

#if defined SEQ66_PLATFORM_DEBUG

extern bool run_ring_test ();

#endif

#endif      // SEQ66_RING_BUFFER_HPP

/*
 * ring_buffer.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


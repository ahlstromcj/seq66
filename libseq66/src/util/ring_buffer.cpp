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
 *
 *  A lock-free ring buffer.
 *
 *   (C) Copyright 2000 Paul Davis
 *   (C) Copyright 2003 Rohan Drape
 *   (C) Copyright 2002 Fred Gleason <fredg@paravelsystems.com>
 *
 *    $Id: ringbuffer.h,v 1.1 2007/12/19 20:22:23 fredg Exp $
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   Adapted from code by Paul Davis and Rohan Drape in
 *   'example-clients/ringbuffer.ch' in the Jack Audio Connection Kit.
 *
 *  https://accu.org/journals/overload/10/50/goodliffe_389/
 *
 *  https://codereview.stackexchange.com/questions/208293/
 *      ring-buffer-implementation-in-c14
 *
\verbatim
        start 0 ------------------ array ---------------------> end

           B U F F E R                             R I N G _
           ---------------------------------------------------  wrap-around
      --->| data      |   e m p t y   s p a c e   | data...   |-----
     |     ---------------------------------------------------|
     |                ^                           ^           |
     |                |                           |           |
     |              back()                      front()       |
     |              "tail"                      "head"        |
      --------------------------------------------------------
\endverbatim
 *
 *      -   At start, tail = head = 0.
 *      -   New data is added at the back of the circular buffer via
 *          push_back(). This increments the tail, adding to the number of
 *          elements in the ring_buffer.
 *      -   The buffer starts at the front, and one reads from there.
 *          This decrements the head.
 *      -   At the end of the array, we wrap around to the start.
 *
 *  This implementation:
 *
 *      -   Encodes whole objects, not characters.
 *      -   Provides insertion to the back of the container, but no
 *          direct access to the back.
 *      -   Provides access to the front of the container to get that
 *          object.
 *      -   Provides a pop_front() to remove the front object.
 */

#include <string>                       /* std::string                      */

#include "util/ring_buffer.hpp"

namespace seq66
{

#if defined SEQ66_PLATFORM_DEBUG

class ring_test
{

private:

    int m_test_counter;
    std::string m_test_text;

public:

    ring_test (int counter = 0);
    ring_test (const ring_test & rhs) = default;
    ringtest & operator = (const ring_test & rhs) = default;
    ~ring_test ();

    void increment ()
    {
        ++m_test_counter;
    }

    int test_counter () const
    {
        return m_test_counter;
    }

    void set_test_text (const std::string & t)
    {
        m_test_text = t;
    }

    const std::string & test_text () const
    {
        return m_test_text;
    }

    std::string show (size_t index);

};

ring_test::ring_test (int counter) :
    m_test_counter  (counter),
    m_test_text     ()
{
    // No code
}

std::string
ring_test::show (size_t index)
{
    std::string result = "[" + std::to_string(index) + "]: ";
    result += "counter " + std::to_string(test_counter) + "; ";
    result += "text '" + text_text() + "'.";
    return result;
}

bool
run_ring_test ()
{
    bool result = false;
    ring_buffer<ring_test> rb(7);           /* should become 8 (power of 2) */
    ring_test rt_a(1);
    ring_test rt_b(2);
    ring_test rt_c(3);
    ring_test rt_c(4);
    ring_test rt_c(5);
    ring_test rt_c(6);
    ring_test rt_c(7);
    ring_test rt_c(8);
    rt_a.set_test_text("rt_a");
    rt_b.set_test_text("rt_b");
    rt_c.set_test_text("rt_c");
    rt_d.set_test_text("rt_d");

    rt_e.set_test_text("rt_e");
    rt_f.set_test_text("rt_f");
    rt_g.set_test_text("gt_g");
    rt_h.set_test_text("rt_h");
}

#endif

}           // namespace seq66

/*
 * ring_buffer.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

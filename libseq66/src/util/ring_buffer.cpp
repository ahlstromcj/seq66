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
 *  The ring_buffer is a FIFO (first-in/first-out), where one adds data to the
 *  back of the buffer and consumes data from the front.
 *
\verbatim
     Start 0-1-2-3-4-5-6.--------buffer --------------------32> End
           B U F F E R                              R I N G _
           ----------------------------------------------------  wrap-around
      --> | data       |   w r i t e   s p a c e   | data...   | ----
     |     ----------------------------------------------------      |
     |       ^         ^ next item                 ^                 |
     |       |         | goes here                 |                 |
     |       |         |                           |                 |
     ^    front()    back()                      front()             v
     |     "head"    "tail"                  "head" later            |
     |                                                               |
      -------------------------------<-------------------------------
\endverbatim
 *
 *      -   At start, tail = 0, head = 0. Unlike the original implementation,
 *          we don't set head to 1, in order to support an empty buffer.
 *      -   New data is added at the back of the circular buffer via
 *          push_back(). This increments the tail, adding to the number of
 *          elements in the ring_buffer. The first item pushed goes to
 *          slot 0, not slot 1, in this implementation.
 *      -   The buffer starts at the front, and one reads from there.
 *          This decrements the head.
 *      -   At the end of the array, we wrap around to the start.
 *
 *  This implementation:
 *
 *      -   Encodes whole objects, not characters.
 *      -   Provides insertion to the back of the container and direct
 *          access to the back.
 *      -   Provides access to the front of the container to get that
 *          object.
 *      -   Provides a front() function to inspect the object.  If worried
 *          about the usability of the result, then use the read() function
 *          and test the result for a value greater than 0.
 *      -   Provides a pop_front() to remove the front object.
 *      -   Currently does not handle TYPE = char as strings, just single
 *          characters.
 *
 *  Recommendations for TYPE:
 *
 *      -   Provide a boolean or integer that indicates the retrieved item
 *          is not usable. For example, a count() of -1 or a boolean called
 *          usable(). The ring_buffer template does not enforce this.
 */

#include <string>                       /* std::string                      */

#include "util/ring_buffer.hpp"

#if defined SEQ66_PLATFORM_DEBUG
#include <iostream>
#endif

namespace seq66
{

#if defined SEQ66_PLATFORM_DEBUG

class ring_test
{

public:

    using cref = const ring_buffer<ring_test>::reference;

private:

    int m_test_counter;
    std::string m_test_text;

public:

    ring_test (int counter = (-1), const std::string & text = "");
    ring_test (const ring_test & rhs) = default;
    ring_test & operator = (const ring_test & rhs) = default;
    ~ring_test () = default;

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

    std::string to_string ();

};              // class ring_test

static void
show_message (const std::string & msg)
{
    std::cout << msg << std::endl;
}

static void
show_error (const std::string & msg)
{
    std::cerr << msg << std::endl;
}

static bool
item_test (ring_test::cref item, const std::string & tag, int counter)
{
    std::string values = item.to_string();
    int c = item.test_counter();
    std::cout << "Item test " << tag << " " << values << std::endl;
    bool result = c == counter;
    if (! result)
        std::cerr << "'" << tag << "' test failed" << std::endl;

    return result;
}

ring_test::ring_test (int counter, const std::string & text) :
    m_test_counter  (counter),
    m_test_text     (text)
{
    // No code
}

std::string
ring_test::to_string ()
{
    std::string result = "counter " + std::to_string(test_counter()) + "; ";
    result += "text '" + test_text() + "'.";
    return result;
}

bool
run_ring_test ()
{
    bool result = true;
    ring_test rt_a(1, "rt_a");
    ring_test rt_b(2, "rt_b");
    ring_test rt_c(3, "rt_c");
    ring_test rt_d(4, "rt_d");
    ring_test rt_e(5, "rt_e");
    ring_test rt_f(6, "rt_f");
    ring_test rt_g(7, "rt_g");
    ring_test rt_h(8, "rt_h");
    ring_test rt_i(9, "rt_i");
    ring_test rt_j(10, "rt_j");

    /*
     * Smoke test
     */

    ring_buffer<ring_test> rb(7);           /* should become 8 (power of 2) */
    std::size_t sz = rb.write(rt_a);
    if (sz != 1)
    {
        show_error("ring_buffer::write() failed");
        result = false;
    }
    else
    {
        ring_test rt;
        sz = rb.read(rt);
        if (sz > 0)
        {
            show_error("ring_buffer::read() failed");
            result = false;
        }
        else
        {
            std::string ss = rt.to_string();
            std::cout << "Read test object '" << ss << "'" << std::endl;
            if (rb.count() > 0)
            {
                show_error("read() failed to pop the object");
                result = false;
            }
        }
    }

    /*
     * Full buffer test. Ultimately 10 items entered. Only 8 should remain.
     * Then we pop all items in the ring_buffer and show them.
     * (Should compare counters at some point.)
     */

    if (result)
    {
        rb.clear();
        if (! rb.empty())
        {
            show_error("ring_buffer not empty");
            result = false;
        }
    }

    if (result)
    {
        std::size_t space = rb.read_space();
        if (space == 0)
        {
            rb.push_back(rt_a);
            rb.push_back(rt_b);
            rb.push_back(rt_c);
            rb.push_back(rt_d);
            rb.push_back(rt_e);
            rb.push_back(rt_f);
            rb.push_back(rt_g);
            rb.push_back(rt_h);
            space = rb.read_space();
            if (rb.count() != 8 || space != 8)
            {
                show_error("ring_buffer count mismatch");
                result = false;
            }
            if (result)
            {
                space = rb.write_space();
                if (space > 0)
                {
                    show_error("write space > 0");
                    result = false;
                }
            }
        }
        else
        {
            show_error("empty read-space error");
            result = false;
        }
    }
    if (result)
    {
        /*
         * Here, rt_a and rt_b should be dropped.
         */

        rb.push_back(rt_i);
        rb.push_back(rt_j);

        std::size_t rspace = rb.read_space();
        std::size_t wspace = rb.write_space();
        if (rb.count() != 8 || rspace != 8 || wspace != 0)
        {
            show_error("objects not overwritten");
            result = false;
        }
        if (rb.dropped() != 2)
        {
            show_error("unexpected number of dropped items");
            result = false;
        }
        if (result)
        {
            int imax = rb.count();              /* can't use count() in loop */
            for (int i = 0; i < imax; ++i)
            {
                ring_test::cref item = rb.front();
                std::string values = item.to_string();
                printf("[%d] %s\n", i, values.c_str());
                if (item.test_counter() != (i + 3))
                    result = false;

                rb.pop_front();
            }
            if (! result)
                show_message("Item-counter mismatch detected");

            if (rb.empty())
            {
                show_message("Should see rt_c through rt_j values");
            }
            else
            {
                show_error("ringbuffer still has items!");
                result = false;
            }
        }
    }

    /*
     *  Alternative push/pops with access via front(). Note that back()
     *  goes back one step from the tail in order to (hopefully) get a valid
     *  object.
     */

    if (result)
    {
        rb.clear();

        ring_test::cref fitem = rb.front();
        ring_test::cref bitem = rb.back();
        result = item_test(fitem, "front", (-1));   /* usability test   */
        result = item_test(bitem, "back", (-1));    /* usability test   */

        rb.push_back(rt_a);             /* front (1)    */
        rb.push_back(rt_b);
        rb.push_back(rt_c);
        rb.push_back(rt_d);             /* back (4)     */
        result = rb.count() == 4;
        if (result)
        {
            ring_test::cref frontitem = rb.front();
            result = item_test(frontitem, "[front]", 1);
            if (result)
            {
                ring_test::cref backitem = rb.back();
                result = item_test(backitem, "[back] ", 4);
            }
            if (result)
            {
                rb.pop_front();
            }
            else
                show_error("First front call failed");
        }
        else
            show_error("Bad ring_buffer count");
    }

    /*
     * End of tests.
     */

    return result;
}

#endif

}           // namespace seq66

/*
 * ring_buffer.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

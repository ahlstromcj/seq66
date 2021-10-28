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
 * \file          seq.cpp
 *
 *  This module declares a two dimensional vector class solely to hold the
 *  mute status of a number of sequences in a set.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-02-12
 * \updates       2019-05-24
 * \license       GNU GPLv2 or above
 *
 *  We added three classes:  seq, screenset, and setmapper, which replace a
 *  number of data items and functions in the seq66::performer class.  The
 *  seq66::seq class holds state data that is an adjunct to the data already
 *  stored in a sequence.
 *
 *  The state-saving buffers are incorporated into a seq object, which, in a
 *  way, simply adds some more state variables to the seq66::sequence class.
 *  These state-saving buffers were arrays of hardwired size.
 */

#include <iomanip>                      /* std::setw() manipulator          */
#include <iostream>                     /* std::cout                        */
#include <sstream>                      /* std::ostringstream               */

#include "play/seq.hpp"                 /* seq66::seq class                 */

/*
 *  This namespace is not documented because it screws up the document
 *  processing done by Doxygen.
 */

namespace seq66
{

/**
 *  Default and principal constructor.
 *
 *  The seq class is a small class to hold additional information about a
 *  sequence.  It replaces a number of arrays.
 *
 *  Initialize the members.  The computer-generated default constructor does
 *  not clear the boolean members.
 *
 *  Please note that the activate() function must be called before this object
 *  can be used.
 */

seq::seq () :
    m_seq                       (),         /* seq::pointer, shared pointer */
    m_seq_active                (false),    /* indicates a valid pointer    */
    m_was_active_main           (false),
    m_was_active_edit           (false),
    m_was_active_perf           (false),
    m_was_active_names          (false),
    m_snapshot_status           (false),
    m_armed_status              (false),
    m_queued                    (false)
{
    m_seq.reset();                          /* must call seq::activate(s)   */
}

/**
 *  Debuggable destructor.
 */

seq::~seq ()
{
#if defined SEQ66_PLATFORM_DEBUG_TMI
    special_message("~seq()");
#endif
}

bool
seq::activate (sequence * s, number seqno)
{
    bool result = not_nullptr(s);
    if (result)
    {
        m_seq.reset(s);
        result = activate(seqno, true);
    }
    return result;
}

/**
 *  Sets or unsets the active state of the given pattern/sequence number.
 *  If setting it active, the sequence::seq_number() setter is called. It won't
 *  modify the sequence's internal copy of the sequence number if it has already
 *  been set.
 *
 * \param seqno
 *      Provides the prospective sequence number.
 *
 * \param active
 *      True if the sequence is to be set to the active state, which is the
 *      default value.
 *
 * \return
 *      Returns true if the sequence existed and could have its sequence number
 *      set.
 */

bool
seq::activate (number seqno, bool active)
{
    bool result = true;
    if (m_seq_active && ! active)
        set_was_active();               /* weird, investigate   */

    if (active)
    {
        if (not_nullptr(m_seq))
        {
            m_seq_active = true;
            m_seq->seq_number(seqno);
            if (m_seq->name().empty())
                m_seq->set_name();
        }
        else
            m_seq_active = result = false;
    }
    else
        m_seq_active = result = false;

    return result;
}

bool
seq::deactivate ()
{
    bool result = not_nullptr(m_seq);
    if (m_seq_active)
        set_was_active();               /* weird, investigate   */

    m_seq_active = false;
    return result;
}

/**
 *  Sets was-active flags:  main, edit, perf, and names.
 *  Why do we need this routine?
 *
 * \param seq
 *      The pattern number.  It is checked for validity.
 */

void
seq::set_was_active ()
{
    if (active())
    {
        m_was_active_main = m_was_active_edit =
            m_was_active_perf = m_was_active_names = true;
    }
}

/**
 *  Indicates that the desired sequence is active, unmuted, and has
 *  a non-zero trigger count.
 *
 * \param seq
 *      The index of the desired sequence.
 *
 * \return
 *      Returns true if the sequence has the three properties noted above.
 */

bool
seq::is_exportable () const
{
    bool result = active();
    if (result)
        result = m_seq->is_exportable();

    return result;
}

/**
 *  Checks the pattern/sequence for main-dirtiness.  See the
 *  sequence::is_dirty_main() function.
 *
 * \param seq
 *      The pattern number.  It is checked for validity.
 *
 * \return
 *      Returns the was-active-main flag value, before setting it to
 *      false.  Returns false if the pattern was invalid.
 */

bool
seq::is_dirty_main () const
{
    bool was_active = false;
    if (active())
    {
        was_active = m_seq->is_dirty_main();
    }
    else
    {
        was_active = m_was_active_main;
        m_was_active_main = false;      /* mutable */
    }
    return was_active;
}

/**
 *  Checks the pattern/sequence for edit-dirtiness.
 *
 * \param seq
 *      The pattern number.  It is checked for validity.
 *
 * \return
 *      Returns the was-active-edit flag value, before setting it to
 *      false. Returns false if the pattern was invalid.
 */

bool
seq::is_dirty_edit () const
{
    bool was_active = false;
    if (active())
    {
        was_active = m_seq->is_dirty_edit();
    }
    else
    {
        was_active = m_was_active_edit;
        m_was_active_edit = false;      /* mutable */
    }
    return was_active;
}

/**
 *  Checks the pattern/sequence for perf-dirtiness.
 *
 * \param seq
 *      The pattern number.  It is checked for validity.
 *
 * \return
 *      Returns the was-active-perf flag value, before setting it to
 *      false.  Returns false if the pattern/sequence number was invalid.
 */

bool
seq::is_dirty_perf () const
{
    bool was_active = false;
    if (active())
    {
        was_active = m_seq->is_dirty_perf();
    }
    else
    {
        was_active = m_was_active_perf;
        m_was_active_perf = false;      /* mutable */
    }
    return was_active;
}

/**
 *  Checks the pattern/sequence for names-dirtiness.
 *
 * \param seq
 *      The pattern number.  It is checked for validity.
 *
 * \return
 *      Returns the was-active-names flag value, before setting it to
 *      false.  Returns false if the pattern/sequence number was invalid.
 */

bool
seq::is_dirty_names () const
{
    bool was_active = false;
    if (active())
    {
        was_active = m_seq->is_dirty_names();
    }
    else
    {
        was_active = m_was_active_names;
        m_was_active_names = false;     /* mutable */
    }
    return was_active;
}

/**
 *  Turn the playing of a sequence on or off.  Used for the implementation of
 *  sequence_playing_on() and sequence_playing_off().
 *
 * \param on
 *      True if the sequence is to be turned on, false if it is to be turned
 *      off.
 *
 * \param qinprogress
 *      Indicates if queuing is in progress.  This status is obtained from the
 *      performer.
 */

void
seq::sequence_playing_change (bool on, bool qinprogress)
{
    if (active())
    {
        bool queued = m_seq->get_queued();
        if (on)
        {
            if (qinprogress)
            {
                if (! queued)
                    m_seq->toggle_queued();
            }
            else
                m_seq->set_playing(on);
        }
        else
        {
            if (queued && qinprogress)
                m_seq->toggle_queued();
            else
                m_seq->set_playing(on);
        }
    }
}

std::string
seq::to_string (int /*index*/) const
{
    std::ostringstream result;
    if (active())
    {
        result
            << "    [" << std::setw(4) << std::right << m_seq->seq_number()
            << "]: '" << m_seq->name() << "'" << std::endl
        ;
    }
    else
    {
        /*
         * Too much information.
         *
         *  result
         *      << "    [" << std::setw(4) << std::right << index
         *      << "]: inactive slot" << std::endl
         *      ;
         */
    }
    return result.str();
}

/**
 *  Simply dumps the to_string() result to the console.
 */

void
seq::show (int index) const
{
    std::string seqmsg = to_string(index);
    if (! seqmsg.empty())
        std::cout << seqmsg;
}

}               // namespace seq66

/*
 * seq.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


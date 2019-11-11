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
 * \file          qseqframe.cpp
 *
 *  This module declares/defines the base class for managing the editing of
 *  sequences.
 *
 * \library       seq66 application
 * \author        Oli Kester; modifications by Chris Ahlstrom
 * \date          2018-07-27
 * \updates       2019-08-11
 * \license       GNU GPLv2 or above
 *
 *  Seq66 (Qt version) has two different pattern editor frames to
 *  support:
 *
 *      -   New.  This pattern-editor frame is used in its own window.  It is
 *          larger and has a lot of functionality.  Furthermore, it
 *          keeps the time, event, and data views in full view at all times
 *          when scrolling, just like the Gtkmm-2.4 version of the pattern
 *          editor.
 *      -   Kepler34.  This frame is not as functional, but it does fit in the
 *          tab better, and it scrolls the time, event, keys, and roll panels
 *          as if they were one object.
 */

#include <QWidget>

#include "cfg/settings.hpp"             /* seq66::usr()                     */
#include "qseqdata.hpp"
#include "qseqframe.hpp"
#include "qseqkeys.hpp"
#include "qseqroll.hpp"
#include "qseqtime.hpp"
#include "qstriggereditor.hpp"

/*
 *  Do not document the name space.
 */

namespace seq66
{

/**
 *
 * \param p
 *      Provides the performer object to use for interacting with this sequence.
 *      Among other things, this object provides the active PPQN.
 *
 * \param seqid
 *      Provides the sequence number.  The sequence pointer is looked up using
 *      this number.  This number is also the pattern-slot number for this
 *      sequence and for this window.  Ranges from 0 to 1024.  The caller
 *      should ensure this is a valid, non-blank sequence.
 *
 * \param parent
 *      Provides the parent window/widget for this container window.  Defaults
 *      to null.
 */

qseqframe::qseqframe
(
    performer & p,
    int seqid,
    QWidget * parent
) :
    QFrame                  (parent),
    qbase                   (p),                                /* no zoom  */
    m_seq                   (perf().sequence_pointer(seqid)),
    m_seqkeys               (nullptr),
    m_seqtime               (nullptr),
    m_seqroll               (nullptr),
    m_seqdata               (nullptr),
    m_seqevent              (nullptr)
{
    // perf().enregister(this);
}

/**
 *
 */

qseqframe::~qseqframe ()
{
    // perf().unregister(this);
}

/**
 *  \todo
 *      Check for dirtiness (perhaps), clear the table and settings, an reload
 *      as if starting again.
 */

bool
qseqframe::on_sequence_change (seq::number seqno)
{
    bool result = m_seq && seqno == m_seq->seq_number();

#if defined SEQ66_PLATFORM_DEBUG_TMI
        printf("qseqframe::on_sequence_change(%d)\n", seqno);
#endif

    if (result)
    {
        // TODO
    }
    return result;
}

/**
 *
 */

bool
qseqframe::repitch_selected ()
{
    std::string filename = rc().notemap_filespec();
    sequence & s = *seq_pointer();
    bool result = perf().repitch_selected(filename, s);
    if (result)
    {
        set_dirty();
    }
    else
    {
        // need to display error message somehow
    }
    return result;
}

/**
 *  Sets the zoom parameter, z.  If valid, then the m_zoom member is set.
 *  The new setting is passed to the roll, time, data, and event panels
 *  [which each call their own set_dirty() functions].
 *
 * \param z
 *      The desired zoom value, which is checked for validity.
 *
 * \return
 *      Returns true if the zoom value was changed.
 */

bool
qseqframe::set_zoom (int z)
{
    bool result = qbase::set_zoom(z);
    if (result)
    {
        if (not_nullptr(m_seqroll))
            m_seqroll->set_zoom(z);

        if (not_nullptr(m_seqtime))
            m_seqtime->set_zoom(z);

        if (not_nullptr(m_seqdata))
            m_seqdata->set_zoom(z);

        if (not_nullptr(m_seqevent))
            m_seqevent->set_zoom(z);
    }
    return result;
}

/**
 *  Sets the dirty status of all of the panels.  However, note that in the
 *  case of zoom, for example, it also sets dirtiness, via qseqbase.
 *
 *  Also be sure that the performer calls the notification apparatus when
 *  changes in the data in the sequence occur.
 */

void
qseqframe::set_dirty ()
{
    qbase::set_dirty();
    if (not_nullptr(m_seqroll))
        m_seqroll->set_dirty();

    if (not_nullptr(m_seqtime))
        m_seqtime->set_dirty();

    if (not_nullptr(m_seqdata))
        m_seqdata->set_dirty();

    if (not_nullptr(m_seqevent))
        m_seqevent->set_dirty();
}

}           // namespace seq66

/*
 * qseqframe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


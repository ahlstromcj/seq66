#if ! defined SEQ66_QBASE_HPP
#define SEQ66_QBASE_HPP

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
 *  Place, Suite 330, Boston, MA  02111-1307  USA.
 */

/**
 * \file          qbase.hpp
 *
 *  This module declares/defines the base class for operations common to all
 *  seq66 windows.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-07-22
 * \updates       2024-12-12
 * \license       GNU GPLv2 or above
 *
 *  Provides a abstract base class so that both the old and the new Qt
 *  sequence and performance frames can be supported.  These concepts
 *  are in the queue to be supported:
 *
 *      -   PPQN.  The performer now maintains the PPQN for the whole
 *          application.  The user-interface classes have to deal with changes
 *          in the PPQN.  (Some will also have to deal with changes in BPM,
 *          beats per minute.)
 *      -   Zoom.  Zoom interacts with PPQN.
 *      -   Dirtiness. This indicates if the user-interface should be drawn.
 */

#include "play/performer.hpp"           /* seq66::performer                 */

/*
 * Do not document namespaces, it breaks Doxygen.
 */

namespace seq66
{

/**
 *  This frame is the basis for editing an individual MIDI sequence.
 */

class qbase
{

public:

    /**
     *  We need a way to distinguish settings made at construction time versus
     *  settings made by the user, as well as changes that ignore an attempted
     *  user setting.
     */

    enum class status
    {
        startup,
        edit,
        undo
    };

private:

    /**
     *  Provides a reference to the performance object.
     */

    performer & m_performer;

    /**
     *  Dirty!  Being dirty means that not only does the window need updating,
     *  but there are changes made that need to be saved.
     */

    mutable bool m_is_dirty;

    /**
     *  All ready to go.  This variable is to be used to keep from setting dirty
     *  status over and over while initializing the user-interface... which
     *  calls paintEvent() over and over.
     */

    mutable bool m_is_initialized;

public:

    qbase (performer & p /* , int zoom */ );
    virtual ~qbase ();

    void stop_playing ()
    {
        perf().auto_stop();
    }

    void pause_playing ()
    {
        perf().auto_pause();
    }

    void start_playing ()
    {
        perf().auto_play();
    }

protected:

    int ppqn () const
    {
        return perf().ppqn();           /* go right to the source for PPQN  */
    }

    bool is_dirty () const
    {
        return m_is_dirty;
    }

    bool is_initialized () const
    {
        return m_is_initialized;
    }

public:

    void set_initialized (bool flag = true) const
    {
        m_is_initialized = flag;
    }

    void uninitialize ()
    {
        m_is_initialized = false;
    }

    const performer & perf () const
    {
        return m_performer;
    }

    performer & perf ()
    {
        return m_performer;
    }

public:

    virtual bool check_dirty () const
    {
        bool result = m_is_dirty;
        m_is_dirty = false;
        return result;
    }

    virtual bool change_ppqn (int ppqn) = 0;

    virtual bool change_bpm (midibpm /*bpm*/)
    {
        return true;                                // no code in most cases
    }

    virtual void set_dirty ()
    {
        m_is_dirty = true;
    }

protected:

    virtual void update_midi_buttons ()
    {
        // no buttons at this level
    }

};          // class qbase

}           // namespace seq66

#endif      // SEQ66_QBASE_HPP

/*
 * qbase.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


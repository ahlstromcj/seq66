#if ! defined  SEQ66_SMANAGER_HPP
#define SEQ66_SMANAGER_HPP

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
 * \file          smanager.hpp
 *
 *  This module declares/defines the base class for handling many facets
 *  of administering a session of seq66 usage.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2020-05-30
 * \updates       2020-09-05
 * \license       GNU GPLv2 or above
 *
 *  This class provides a process for starting, running, restarting, and
 *  closing down the Seq66 application, even without session management.  One
 *  of the goals is to be able to reload the performer when the set of MIDI
 *  devices in the system changes.
 */

#include <memory>                       /* std::shared_ptr<>, unique_ptr<>  */

#include "play/performer.hpp"           /* seq66::performer                 */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  This class supports manager a run of seq66.
 */

class smanager
{

public:

    /**
     *  Provides a unique pointer to a performer, to enable the performer to
     *  be recreated.
     */

    using pointer = std::unique_ptr<performer>;

private:

    /**
     *  Provides a pointer to the performer to be managed.  This performer can
     *  be removed and recreated as needed (e.g. when another MIDI device
     *  comes online.)
     */

    pointer m_perf_pointer;

    /**
     *  Holds the capabilities string (if applicable) for the application
     *  using this session manager.
     */

    std::string m_capabilities;

    /**
     *  Holds the session manager's name, or "None".
     */

    std::string m_session_manager_name;
    std::string m_session_manager_path;
    std::string m_session_display_name;
    std::string m_session_client_id;

    /**
     *  Hold the name of the currently-loaded MIDI file.
     */

    std::string m_midi_filename;

    /**
     *  Indicates if the --help or --version options were provided at
     *  start-up.
     */

    bool m_is_help;

    /**
     *  Holds the current error message.  Mutable because it is not part of
     *  the true state of the session manager.
     */

    mutable std::string m_extant_errmsg;

    /**
     *  Holds the current error state.  Mutable because it is not part of
     *  the true state of the session manager.
     */

    mutable bool m_extant_msg_active;

public:

    smanager (const std::string & caps = "");
    smanager (const smanager &) = delete;
    smanager & operator =(const smanager &) = delete;

    virtual ~smanager ()
    {
        // currently no additional code needed
    }

    bool create (int argc, char * argv []);
    bool main_settings (int argc, char * argv []);      // make protected !
    bool open_playlist ();      // make protected !
    bool create_performer ();       // make protected !

    std::string open_midi_file (const std::string & fname, std::string & msg);

    bool error_active () const
    {
        return m_extant_msg_active;
    }

    const std::string & error_message () const
    {
        return m_extant_errmsg;
    }

    const std::string & midi_filename () const
    {
        return m_midi_filename;
    }

    const std::string & capabilities () const
    {
        return m_capabilities;
    }

    bool internal_error_check (std::string & msg) const;
    void error_handling ();

    bool internal_error_pending () const
    {
        return  bool(m_perf_pointer) ? m_perf_pointer->error_pending() : true ;
    }

public:

    virtual bool create_session (int argc = 0, char * argv [] = nullptr);
    virtual bool close_session (bool ok = true);
    virtual bool save_session (std::string & msg);
    virtual bool create_window ();
    virtual bool create_project (const std::string & path) = 0;
    virtual bool run () = 0;

    virtual void show_message
    (
        const std::string & tag,
        const std::string & msg
    ) const;
    virtual void show_error
    (
        const std::string & tag,
        const std::string & msg
    ) const;

    virtual void session_manager_name (const std::string & mgrname)
    {
        m_session_manager_name = mgrname;
    }

    const std::string & manager_name () const
    {
        return m_session_manager_name;
    }

    virtual void session_manager_path (const std::string & pathname)
    {
        m_session_manager_path = pathname;
    }

    const std::string & manager_path () const
    {
        return m_session_manager_path;
    }

    virtual void session_display_name (const std::string & dispname)
    {
        m_session_display_name = dispname;
    }

    const std::string & display_name () const
    {
        return m_session_display_name;
    }

    virtual void session_client_id (const std::string & clid)
    {
        m_session_client_id = clid;
    }

    const std::string & client_id () const
    {
        return m_session_client_id;
    }

protected:

    const performer * perf () const
    {
        return m_perf_pointer.get();
    }

    performer * perf ()
    {
        return m_perf_pointer.get();
    }

    void append_error_message (const std::string & message = "") const;

};          // class smanager

}           // namespace seq66

#endif      // SEQ66_SMANAGER_HPP

/*
 * smanager.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


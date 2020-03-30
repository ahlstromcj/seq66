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
 * \date          2020-03-24
 * \updates       2020-03-24
 * \license       GNU GPLv2 or above
 *
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

    using pointer = std::unique_ptr<performer>;

private:

    pointer m_perf_pointer;

    std::string m_midi_filename;

    std::string m_extant_errmsg = "unspecified error";

    bool m_extant_msg_active = false;

public:

    smanager ();

    virtual ~smanager ()
    {
        // currently no additional code needed
    }

    bool main_settings (int argc, char * argv []);
    bool open_playlist ();
    std::string open_midi_file (const std::string & fname);

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

    virtual bool create_session ();
    virtual bool close_session ();
    virtual bool create_window ();      /* does mostly nothing by default   */
    virtual void show_message (const std::string & msg);
    virtual void show_error (const std::string & msg);
    virtual bool run () = 0;            /* app.exec(); run main window loop */

#if defined SEQ66_PORTMIDI_SUPPORT
    bool portmidi_error_check () const;
#endif

protected:

    const performer * perf () const
    {
        return m_perf_pointer.get();
    }

    performer * perf ()
    {
        return m_perf_pointer.get();
    }

    void set_error_message (const std::string & message = "")
    {
        m_extant_errmsg = message;
        m_extant_msg_active = ! message.empty();
    }

private:

    pointer create_performer ();

};          // class smanager

}           // namespace seq66

#endif      // SEQ66_SMANAGER_HPP

/*
 * smanager.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


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
 * \file          qt5nsmanager.cpp
 *
 *  This module declares/defines the main module for the Non Session Manager
 *  control of qseq66.
 *
 * \library       qt5nsmanager application
 * \author        Chris Ahlstrom
 * \date          2020-03-15
 * \updates       2020-08-31
 * \license       GNU GPLv2 or above
 *
 *  Duty now for the future!
 *
 */

#include <QApplication>                 /* QApplication etc.                */

#include "cfg/settings.hpp"             /* seq66::usr() and seq66::rc()     */
#include "nsm/nsmmessagesex.hpp"        /* seq66::nsm access functions      */
#include "util/strfunctions.hpp"        /* seq66::string_replace()          */
#include "qt5nsmanager.hpp"             /* seq66::qt5nsmanager              */
#include "qsmainwnd.hpp"                /* seq66::qsmainwnd                 */

namespace seq66
{

/*
 *-------------------------------------------------------------------------
 * qt5nsmanager
 *-------------------------------------------------------------------------
 */

/**
 *  Note that this object is created before there is any chance to get the
 *  configuration, because the smanager base class is what gets the
 *  configuration, well after this constructor.
 */

qt5nsmanager::qt5nsmanager
(
    QApplication & app,
    QObject * parent,
    const std::string & caps
) :
    QObject         (parent),
    clinsmanager    (caps),
    m_application   (app),
    m_nsm_active    (false),
#if defined SEQ66_NSM_SUPPORT
    m_nsm_client    (),
#endif
    m_window        ()
{
    // no code
}

/**
 *
 */

qt5nsmanager::~qt5nsmanager ()
{
    // no code yet
}

/**
 *  Push the qsmainwnd window onto the stack.  Also be sure to pass along the
 *  PPQN value, which might be different than the default (192), and affects
 *  some of the child objects of qsmainwnd.  Also note the future support for
 *  NSM; it will change the menus available in the main Seq66 window.
 *
 *  This function assumes that create_performer() has already been called.
 *  And this function is called before create_session().
 */

bool
qt5nsmanager::create_window ()
{
    bool result = not_nullptr(perf());
    if (result)
    {
        performer * p = perf();
        std::string mfname = midi_filename();
        bool usensm = m_nsm_active;
        qsmainwnd * qm = new (std::nothrow) qsmainwnd(*p, mfname, usensm);
        result = not_nullptr(qm);
        if (result)
        {
            m_window.reset(qm);
            m_window->show();
            (void) smanager::create_window();   /* just house-keeping */
            if (error_active())
            {
                show_error();
                result = false;
            }
            else
            {
                session_manager_name("None");
                m_window->session_path("None");
                m_window->session_log("No log entries.");
#if defined SEQ66_NSM_SUPPORT
                if (m_nsm_client)
                {
                    std::string url = m_nsm_client->nsm_url();
                    m_window->session_URL(url);
                }
                else
                    m_window->session_URL("None");
#else
                m_window->session_URL("None");
#endif
            }
        }
    }
    return result;
}

/**
 *  Will do more with this later.
 */

bool
qt5nsmanager::close_session (bool ok)
{
    return smanager::close_session(ok);
}

/**
 *
 */

bool
qt5nsmanager::run ()
{
    int exit_status = m_application.exec();     /* run main window loop     */
    return exit_status == EXIT_SUCCESS;
}

/**
 *
 */

void
qt5nsmanager::show_message (const std::string & msg) const
{
    if (m_window && ! msg.empty())
        m_window->show_message_box(msg);
}

/**
 *  Shows the collected messages in the message-box, and recommends the user
 *  exit and check the configuration.
 */

void
qt5nsmanager::show_error (const std::string & msg) const
{
    if (m_window)
    {
        if (msg.empty())
        {
#if defined SEQ66_PORTMIDI_SUPPORT
            if (Pm_error_present())
            {
                std::string pmerrmsg = std::string(Pm_error_message());
                append_error_message(pmerrmsg);
            }
#endif
            std::string html = string_replace(error_message(), "\n", "<br>");
            html += "<br>Please exit and fix the configuration.";
            m_window->show_message_box(html);
        }
        else
        {
            append_error_message(msg);
            m_window->show_message_box(msg);
        }
    }
}

void
qt5nsmanager::session_manager_name (const std::string & mgrname)
{
    if (m_window)
    {
        m_window->session_manager(mgrname.empty() ? "None" : mgrname);
    }
}


}           // namespace seq66

/*
 * qt5nsmanager.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


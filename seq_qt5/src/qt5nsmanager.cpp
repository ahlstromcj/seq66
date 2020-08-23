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
 * \updates       2020-08-23
 * \license       GNU GPLv2 or above
 *
 *  Duty now for the future!
 */

#include <QApplication>                 /* QApplication etc.                */

#include "cfg/settings.hpp"             /* seq66::usr() and seq66::rc()     */
#include "nsm/nsmmessagesex.hpp"        /* seq66::nsm access functions      */
#include "util/basic_macros.hpp"        /* seq66::msgprintf()               */
#include "util/strfunctions.hpp"        /* seq66::string_replace()          */
#include "qt5nsmanager.hpp"             /* seq66::qt5nsmanager              */
#include "qsmainwnd.hpp"                /* seq66::qsmainwnd                 */

namespace seq66
{

/*
 *-------------------------------------------------------------------------
 *  Optional NSM support [TO DO]
 *-------------------------------------------------------------------------
 */

/*
 *-------------------------------------------------------------------------
 * qt5nsmanager
 *-------------------------------------------------------------------------
 */

/**
 *
 */

qt5nsmanager::qt5nsmanager (QApplication & app, QObject * parent) :
    QObject         (parent),
    smanager        (),
    m_application   (app),
#if defined SEQ66_NSM_SUPPORT
    m_nsm_client    (),
#endif
    m_window        ()
{
#if defined SEQ66_NSM_SUPPORT
    std::string nsmfile = "dummy/file";
    std::string nsmext = nsm::default_ext();
    m_nsm_client.reset(create_nsmclient(nsmfile, nsmext));
#else
#error We want NSM support enabled for development purposes.
#endif
}

/**
 *
 */

qt5nsmanager::~qt5nsmanager ()
{
    // no code yet
}

/**
 *  Will do more with this later.
 */

bool
qt5nsmanager::create_session (int argc, char * argv [])
{
    return smanager::create_session(argc, argv);
}

/**
 *  Push the qsmainwnd window onto the stack.  Also be sure to pass along the
 *  PPQN value, which might be different than the default (192), and affects
 *  some of the child objects of qsmainwnd.  Also note the future support for
 *  NSM.
 *
 *  This function assumes that create_performer() has already been called.
 */

bool
qt5nsmanager::create_window ()
{
    bool result = not_nullptr(perf());
    if (result)
    {
        performer * p = perf();
        std::string mfname = midi_filename();
        bool usensm = false;                    /* TODO                     */
        qsmainwnd * qm = new (std::nothrow) qsmainwnd(*p, mfname, usensm);
        result = not_nullptr(qm);
        if (result)
        {
            m_window.reset(qm);

            /*
             * Let NSM handle this eventually....
             */

            m_window->show();
            (void) smanager::create_window();   /* internal house-keeping   */
            if (error_active())
                show_error();
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

}           // namespace seq66

/*
 * qt5nsmanager.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


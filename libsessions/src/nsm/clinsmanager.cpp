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
 * \file          clinsmanager.cpp
 *
 *  This module declares/defines the main module for the Non Session Manager
 *  control of seq66cli.
 *
 * \library       clinsmanager application
 * \author        Chris Ahlstrom
 * \date          2020-08-31
 * \updates       2020-08-31
 * \license       GNU GPLv2 or above
 *
 *  Duty now for the future!
 *
 */

#include "cfg/settings.hpp"             /* seq66::usr() and seq66::rc()     */
#include "nsm/nsmmessagesex.hpp"        /* seq66::nsm access functions      */
#include "nsm/clinsmanager.hpp"         /* seq66::clinsmanager              */

namespace seq66
{

/*
 *-------------------------------------------------------------------------
 * clinsmanager
 *-------------------------------------------------------------------------
 */

/**
 *  Note that this object is created before there is any chance to get the
 *  configuration, because the smanager base class is what gets the
 *  configuration, well after this constructor.
 */

clinsmanager::clinsmanager (const std::string & caps) :
    smanager        (caps),
    m_nsm_active    (false),
#if defined SEQ66_NSM_SUPPORT
    m_nsm_client    ()
#endif
{
    // no code
}

/**
 *
 */

clinsmanager::~clinsmanager ()
{
    // no code yet
}

/**
 *  This function is called before create_window().
 */

bool
clinsmanager::create_session (int argc, char * argv [])
{

#if defined SEQ66_NSM_SUPPORT
    bool result;
    bool ok = usr().is_nsm_session();               /* user wants NSM usage */
    if (ok)
    {
        std::string url = nsm::get_url();
        ok = ! url.empty();                         /* NSM running Seq66?   */
        if (! ok)
            usr().in_session(false);                /* no it is not         */
    }
    if (ok)
    {
        std::string nsmfile = "dummy/file";
        std::string nsmext = nsm::default_ext();
        m_nsm_client.reset(create_nsmclient(*this, nsmfile, nsmext));
        result = bool(m_nsm_client);
        if (result)
        {
            /*
             * Use the same name as provided when opening the JACK client.
             */

            std::string appname = seq_client_name();    /* "seq66"  */
            std::string exename = seq_arg_0();          /* "qseq66" */
            result = m_nsm_client->announce(appname, exename, capabilities());
            if (! result)
                pathprint("create_session():", "failed to announce");
        }
        else
            pathprint("create_session():", "failed to make client");

        m_nsm_active = result;
        usr().in_session(result);                       /* global flag      */
        if (result)
            result = smanager::create_session(argc, argv);
    }
    else
        result = smanager::create_session(argc, argv);
#else
    bool result = smanager::create_session(argc, argv);
#endif

    return result;
}

/**
 *  Will do more with this later.
 */

bool
clinsmanager::close_session (bool ok)
{
    return smanager::close_session(ok);
}

/**
 *
 */

bool
clinsmanager::run ()
{
    // TODO:  see the while (! seq66::session_close()) loop
    return false;
}

/**
 *
 */

void
clinsmanager::show_message (const std::string & msg) const
{
    if (! msg.empty())
        pathprint("S66:", msg);
}

void
clinsmanager::session_manager_name (const std::string & mgrname)
{
    if (! mgrname.empty())
        pathprint("S66:", mgrname);
}

/**
 *  Shows the collected messages in the message-box, and recommends the user
 *  exit and check the configuration.
 */

void
clinsmanager::show_error (const std::string & msg) const
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
        std::string msg = error_message();
        msg += "Please exit and fix the configuration.";
        show_message(msg);
    }
    else
    {
        append_error_message(msg);
        show_message(msg);
    }
}

}           // namespace seq66

/*
 * clinsmanager.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


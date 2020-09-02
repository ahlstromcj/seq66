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
 * \updates       2020-09-02
 * \license       GNU GPLv2 or above
 *
 *  Duty now for the future!
 *
 */

#include "cfg/cmdlineopts.hpp"          /* command-line functions           */
#include "cfg/settings.hpp"             /* seq66::usr() and seq66::rc()     */
#include "midi/midifile.hpp"            /* seq66::write_midi_file()         */
#include "sessions/clinsmanager.hpp"    /* seq66::clinsmanager              */
#include "util/filefunctions.hpp"       /* seq66::make_directory_path()     */

#if defined SEQ66_NSM_SUPPORT
#include "nsm/nsmmessagesex.hpp"        /* seq66::nsm access functions      */
#endif

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
#if defined SEQ66_NSM_SUPPORT
    m_nsm_client    (),
#endif
    m_nsm_active    (false)
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
        bool result = bool(m_nsm_client);
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

        return result;
    }
    else
        return smanager::create_session(argc, argv);
#else
    return smanager::create_session(argc, argv);
#endif
}

/**
 *  Will do more with this later.
 */

bool
clinsmanager::close_session (bool ok)
{
    return smanager::close_session(ok);
}

bool
clinsmanager::save_session (std::string & msg)
{
    bool result = not_nullptr(perf());
    msg.clear();
    if (result)
    {
        std::string filename = rc().midi_filename();
        if (! filename.empty())
        {
            bool is_wrk = file_extension_match(filename, "wrk");
            if (is_wrk)
                filename = file_extension_set(filename, ".midi");

            result = write_midi_file(*perf(), filename, msg);
        }
        result = smanager::save_session(msg);
    }
    else
    {
        msg = "no performer present to save session";
    }
    return result;
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
 *  Creates a session path specified by the Non Session Manager.  This
 *  function is meant to be called after receiving the /nsm/client/open
 *  message.
 *
 *  A sample session path:
 *
 *      /home/ahlstrom/NSM Sessions/QSeq66 Installed/seq66.nYMVC
 *
 *  The NSM daemon creates the directory for this project after dropping the
 *  client ID (seq66.nYMVC).  We append the client ID and create this
 *  directory, followng the lead of Non-Mixer and Qtractor.
 */

bool
clinsmanager::create_project (const std::string & path)
{
    bool result = ! path.empty();
    if (result)
    {
        /*
         * See if the configuration has already been created, using the "rc"
         * file as the test case.  The normal base-name (e.g. "qseq66") is
         * always used in an NSM session.
         */

        std::string rcfilepath = path + "/config/" + rc().config_filename();
        bool already_created = file_exists(rcfilepath);
        if (already_created)
        {
            /*
             * Read the configuration from the NSM path.
             */

            std::string errmessage;
            rcfilepath = path + "/config";
            rc().full_config_directory(rcfilepath);  /* set NSM directory   */
            result = cmdlineopts::parse_options_files(errmessage);
            if (! result)
            {
                pathprint(errmessage, rc().config_filespec());
            }
        }
        else
        {
            std::string fullpath = path;
            result = make_directory_path(fullpath);
            if (result)
            {
                fullpath = path + "/config";
                result = make_directory_path(fullpath);
                if (result)
                    rc().full_config_directory(fullpath);
            }
            if (result)
            {
                fullpath = path + "/midi";
                result = make_directory_path(fullpath);
            }
            if (result)
            {
                usr().save_user_config(true);
            }
        }
    }
    return result;
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
    smanager::session_manager_name(mgrname);
    if (! mgrname.empty())
        pathprint("S66:", mgrname);
}

void
clinsmanager::session_manager_path (const std::string & pathname)
{
    smanager::session_manager_path(pathname);
    if (! pathname.empty())
        pathprint("S66:", pathname);
}

void
clinsmanager::session_display_name (const std::string & dispname)
{
    smanager::session_display_name(dispname);
    if (! dispname.empty())
        pathprint("S66:", dispname);
}

void
clinsmanager::session_client_id (const std::string & clid)
{
    smanager::session_client_id(clid);
    if (! clid.empty())
        pathprint("S66:", clid);
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


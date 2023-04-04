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
 * \file          clinsmanager.cpp
 *
 *  This module declares/defines the main module for the Non Session Manager
 *  control of seq66cli and qseq66.
 *
 * \library       clinsmanager application
 * \author        Chris Ahlstrom
 * \date          2020-08-31
 * \updates       2023-04-03
 * \license       GNU GPLv2 or above
 *
 *  This object also works if there is no session manager in the build.  It
 *  handles non-session startup as well.
 */

#include "cfg/cmdlineopts.hpp"          /* command-line functions           */
#include "cfg/settings.hpp"             /* seq66::usr() and seq66::rc()     */
#include "os/daemonize.hpp"             /* seq66::session_setup(), _close() */
#include "os/timing.hpp"                /* seq66::millisleep()              */
#include "sessions/clinsmanager.hpp"    /* seq66::clinsmanager class        */
#include "util/filefunctions.hpp"       /* seq66::pathname_concatenate()    */
#include "util/strfunctions.hpp"        /* seq66::contains()                */

#if defined SEQ66_NSM_SUPPORT
#include "nsm/nsmmessagesex.hpp"        /* seq66::nsm access functions      */
#endif

namespace seq66
{

/**
 *  Note that this object is created before there is any chance to get the
 *  configuration, because the smanager base class is what gets the
 *  configuration, well after this constructor.
 */

clinsmanager::clinsmanager (const std::string & caps) :
    smanager            (caps),
#if defined SEQ66_NSM_SUPPORT
    m_nsm_client        (),
#endif
    m_nsm_active        (false),
    m_poll_period_ms    (3 * usr().window_redraw_rate())    /* in qsmainwnd */
{
    // no code
}

clinsmanager::~clinsmanager ()
{
    // no code
}

/**
 *  Detects if the 'usr' file defines usage of NSM and a valid URL for the
 *  nsmd daemon, or, if not, is there an NSM URL in the environment. Also, if
 *  there is an NSM URL in the environment, it overrides the one specified in
 *  the 'usr' file.
 *
 *  Also sanity-checks the URL: "osc.udp://hostname.domain:PORT#"
 *
 * \param [out] url
 *      Holds the URL that was found.  Use it only if this function returns
 *      true.
 *
 * \return
 *      Returns true if a usable NSM URL was found and nsmd was found to be
 *      running.
 */

bool
clinsmanager::detect_session (std::string & url)
{
    bool result = false;
    url.clear();

#if defined SEQ66_NSM_SUPPORT

    std::string tenturl = nsm::get_url();           /* a tentative URL      */
    session_message("Checking for NSM_URL");
    if (! tenturl.empty())
    {
        result = true;
        url = tenturl;
    }
    if (! result)                                   /* not in NSM's env.    */
    {
        result = usr().want_nsm_session();          /* user wants NSM usage */
        if (result)
        {
            tenturl = usr().session_url();          /* try 'usr' file's URL */
            if (tenturl.empty())                    /* configured NSM URL?  */
            {
                result = false;
            }
            else
            {
                result = contains(tenturl, "osc.udp://");   /* sanity check */
                if (result)
                {
                    session_message("NSM URL", tenturl);
                    url = tenturl;                  /* configured NSM URL   */
                }
                else
                    tenturl.clear();
            }
        }
    }
    if (result)
        file_message("NSM URL", tenturl);           /* comforting message   */
#endif

    return result;
}

/**
 *  This function first determines if the user wants an NSM session.  If so,
 *  it determines if it can get a valid NSM_URL environment variable.  If not,
 *  that may simply be due to nsmd running in different console window or as a
 *  daemon.  In that cause, it checks if there was a non-empty
 *  "[user-session]" "url" value.  This is useful in trouble-shooting, for
 *  example.  Finally, just in case the user has set up the "usr" file for
 *  running NSM, but hasn't started non-session-manager or nsmd itself, we use
 *  the new pid_exists() function to make sure that nsmd is indeed running.  Ay!
 *
 *  If all is well, a new nsmclient is created, and an announce/open handshake
 *  starts.  This function is called before create_window().
 */

bool
clinsmanager::create_session (int argc, char * argv [])
{
#if defined SEQ66_NSM_SUPPORT
    std::string url;
    bool ok = detect_session(url);                  /* side-effect          */
    if (! ok)
    {
        if (usr().want_nsm_session())               /* want to debug NSM?   */
        {
            nsm_active(true);                       /* class flag           */
            usr().in_nsm_session(true);             /* global flag          */
            rc().config_subdirectory("config");
        }
        return true;
    }
    if (ok)
    {
        std::string nsmfile = "dummy/file";
        std::string nsmext = nsm::default_ext();
        rc().config_subdirectory("config");         /* NEW 2023-03-31       */
        m_nsm_client.reset(create_nsmclient(*this, url, nsmfile, nsmext));
        bool result = bool(m_nsm_client);
        if (result)
        {
            /*
             * Use the same name as provided when opening the JACK client.
             */

            std::string appname = seq_client_name();    /* "seq66"          */
            std::string exename = seq_arg_0();          /* "qseq66"         */
            result = m_nsm_client->announce(appname, exename, capabilities());
            if (result)
            {
                std::string note = "Announced ";
                note += appname;
                note += " ";
                note += exename;
                note += " ";
                note += capabilities();
                status_message(note);
            }
            else
                file_error("Create session", "failed to announce");
        }
        else
            file_error("Create session", "failed to make client");

        if (url == "testing")
            result = true;

        nsm_active(result);                             /* class flag       */
        usr().in_nsm_session(result);                   /* global flag      */
        (void) smanager::create_session(argc, argv);
        return result;
    }
    else
    {
        return smanager::create_session(argc, argv);
    }
#else
    return smanager::create_session(argc, argv);
#endif
}

/**
 *  Somewhat of the inverse of create_session().
 */

bool
clinsmanager::close_session (std::string & msg, bool ok)
{
#if defined SEQ66_NSM_SUPPORT
    if (usr().in_nsm_session())
    {
        warnprint("Closing NSM session");
        nsm_active(false);                              /* class flag       */
        usr().in_nsm_session(false);                    /* global flag      */
        if (m_nsm_client)                               /* double check     */
            m_nsm_client->close_session();

        /*
         * Freezes in the lo_server_thread_stop() call: m_nsm_client.reset();
         */
    }
#endif
    return smanager::close_session(msg, ok);
}

/**
 *  Saves the active MIDI file, and then calls the base-class version of
 *  save_session().
 */

bool
clinsmanager::save_session (std::string & msg, bool ok)
{
    bool result = not_nullptr(perf());
    if (ok)
        msg.clear();

    if (result)
    {
        result = smanager::save_session(msg, ok);
        if (result)
        {
            /*
             * Only show the message if not running under a session
             * manager.  This is because the message-box will hang the
             * application until the user clicks OK.
             */

            if (! nsm_active())
                show_message(session_tag(), msg);
        }
        else
            show_error(session_tag(), msg);
    }
    return result;
}

/**
 *  This function is useful in the command-line version of the application.
 *  For the Qt version, see the qt5nsmanager class.
 */

bool
clinsmanager::run ()
{
    bool result = false;
    session_setup();
    while (! session_close())
    {
        result = true;
        if (session_save())
        {
            std::string msg;
            result = save_session(msg, true);
            if (! result)
            {
                file_error(msg, "CLI");
            }
        }
        millisleep(m_poll_period_ms);
    }
    return true;
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
 *
 *  Note: At this point, the performer [perf()] does not yet exist, but
 *  the 'rc', 'usr', 'ctrl', and 'mutes' files have been parsed.
 *
 *  The first thing to do is grab the current (and default) configuration
 *  directory (in the user's HOME area).  We may need it, in order to find the
 *  original files to recreate.  Next we see if the configuration has already
 *  been created, using the "rc" file as the test case.  The normal base-name
 *  (e.g. "qseq66") is always used in an NSM session.  We will read/write the
 *  configuration from the NSM path.  We assume (for now) that the "midi"
 *  directory was also created.
 *
 * \param argc
 *      The command-line argument count.
 *
 * \param argv
 *      The command-line argument list.
 *
 * \param path
 *      The base configuration path.  For NSM usage, this will be a directory
 *      for the project in the NSM session directory created by nsmd.  The
 *      sub-directories "config" and "midi" will be created for use by NSM.
 *      For normal usage, this directory will be the standard home directory
 *      or the one specified by the --home option.
 */

bool
clinsmanager::create_project
(
    int argc, char * argv [],
    const std::string & path
)
{
    bool result = ! path.empty();
    if (result)
    {
        std::string cfgpath;
        std::string midipath;

        /*
         * Appends "midi" and "config" (with NSM)
         */

        result = make_path_names(path, cfgpath, midipath);
        if (result)
            result = create_configuration(argc, argv, path, cfgpath, midipath);
    }
#if defined SEQ66_NSM_SUPPORT
    if (m_nsm_client)
        (void) m_nsm_client->open_reply(result);            /* issue #28 */
#endif

    return result;
}

void
clinsmanager::session_manager_name (const std::string & mgrname)
{
    smanager::session_manager_name(mgrname);
    if (! mgrname.empty())
        file_message(session_tag(), mgrname);
}

void
clinsmanager::session_manager_path (const std::string & pathname)
{
    smanager::session_manager_path(pathname);
    if (! pathname.empty())
        file_message(session_tag("path"), pathname);
}

void
clinsmanager::session_display_name (const std::string & dispname)
{
    smanager::session_display_name(dispname);
    if (! dispname.empty())
        file_message(session_tag("name"), dispname);
}

void
clinsmanager::session_client_id (const std::string & clid)
{
    smanager::session_client_id(clid);
    if (! clid.empty())
        file_message(session_tag("client ID"), clid);
}

/**
 *  Shows the collected messages in the console, and recommends the user
 *  exit and check the configuration.
 */

void
clinsmanager::show_error
(
    const std::string & tag,
    const std::string & msg
) const
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
        show_message(tag, msg);
    }
    else
    {
        append_error_message(msg);
        show_message(tag, msg);
    }
}

}           // namespace seq66

/*
 * clinsmanager.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


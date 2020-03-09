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
 * \file          nsmclient.cpp
 *
 *  This module defines some informative functions that are actually
 *  better off as functions.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2020-03-01
 * \updates       2020-03-08
 * \license       GNU GPLv2 or above
 *
 *  nsmclient is an Non Session Manager (NSM) OSC client agent.  The NSM API
 *  comprises a simple Open Sound Control (OSC) based protocol.
 *
 *  The Non project contains a daemon, nsmd, which is an implementation of the
 *  server side of the NSM API. nsmd is controlled by the non-session-manager
 *  GUI. The same server-side API can also be implemented by other session
 *  managers (such as LADISH).  The only dependency for client implementations
 *  is liblo (the OSC library) and the nsm.h header file.
 *
 *  Process:
 *
 *      -#  Find out if NSM_URL is defined in the host environment, and
 *          create the nsmclient object on the heap if so. The
 *          create_nsmclient() should be used; it returns a "unique pointer".
 *          (We may need to provide specific factory functions for Qt, Gtkmm,
 *          and command-line versions of the application.)
 *      -#  If NSM_URL is valid and reachable, send the following "sssiii"
 *          message to the provided address as soon as ready to respond to the
 *          /nsm/client/open event.  api_version_major and api_version_minor
 *          must be the two parts of the version number of the NSM API.
 *          If registering JACK clients, application_name must be passed to
 *          jack_client_open.  capabilities is a string containing a list of
 *          the capabilities the client possesses, e.g. :dirty:switch:progress:
 *          executable_name must be the executable name that launched the
 *          program (e.g argv[0]).
\verbatim
    /nsm/server/announce s:application_name s:capabilities s:executable_name
         i:api_version_major i:api_version_minor i:pid
\endverbatim
 *      -#  Connect up callbacks (e.g. signals in Qt) for the following events:
 *          -   Open NSM session. The caller should first see if this nsmclient
 *              is active.  If so, close the session, which checks the
 *              dirty-count in order to ask the user if changes need to be saved.
 *          -   Save NSM session.
 *          -   Show NSM session.
 *          -   Hide NSM session.
 *      -#  Applications must not register JACK clients until receiving an
 *          open message, which provides a unique client name prefix suitable
 *          for passing to JACK.
 *      -#  Call nsmclient::announce(APP_TITLE, ":switch:dirty:optional-gui:") if
 *          using a GUI.
 *
 *  NSM_URL:
 *
 *      The NSM_URL environment variable is used to inform clients of how to
 *      reach the nsmd daemon, which can be started as follows:
 *
 *          nsmd [--osc-port portnum] [--session-root path] [--detach]
 *
 *      In the following setting, 18440 is the 'portnum'.  127.0.0.1 is the
 *      local host (the computer on which all NSM-related apps are running.
 *
 *          NSM_URL=osc.udp://127.0.0.1:18440/
 *          NSM_URL=osc.udp://mlsleno:15325/        (on developer laptop)
 *
 *      Note that, if running an nsm_proxy client, this variable may need to be
 *      passed on the command-line (in typical bash fashion).
 *
 *      Also see the file contrib/non/nsmopen.sh for examples, and "oscsend
 *      --help".
 *
 *  New session:
 *
 *      TODO
 *
 *  Notes for the future:
 *
 *      There's the general NSM osc protocol which allows basic session
 *      management like adding programs, listing the current sessions and
 *      starting one. This would be sufficient for cadence.
 *
 *      Detection of NSM is by checking validity of NSM_URL environment variable
 *      and getting a response from it so a simple "/nsm/session/list" to
 *      NSM_URL would be sufficient to both populate the list and detect whether
 *      it's running
 *
 *      nsm-proxy integration, however, might be more difficult due to nsmd only
 *      providing details of launching programs to the non-session-manager gui.
 *
 *      Also nsmd will only bind to a single GUI, so this should be the
 *      frontend. (An alternative would be to modify nsmd to support multiple
 *      GUIs running at the same time.)
 *
 *      New tool that implements the /nsm/gui/xxx OSC endpoint to receive more
 *      details about the session which allow it to give a view of the current
 *      session.
 *
 *      INVESTIGATE the NSM replacement, RaySend!!!
 *
 *  Detecting NSM session actions:
 *
 *      In a Qt-based application, we can provide an extended NSM client that
 *      will respond to signals propagated by the "emit" operator. In a
 *      command-line application, we can set flags that are detected in a polling
 *      loop and cause actions to occur.  In Gtkmm applications, we can set
 *      callbacks to be executed.  It would be nice to use the same (callback?)
 *      system for all of them.
 */

#include <stdlib.h>                     /* C geteven() or secure_getenv()   */
#include <string.h>                     /* C strlen()                       */

#include "util/basic_macros.h"          /* not_nullptr() macro              */
#include "nsm/nsmclient.hpp"            /* seq66::nsmclient class           */
#include "nsm/nsmmessages.hpp"          /* seq66::nsm message functions     */

#if defined SEQ66_PLATFORM_DEBUG
#include "util/strfunctions.hpp"        /* seq66::bool_to_string()          */
#endif

#define NSM_API_VERSION_MAJOR 1
#define NSM_API_VERSION_MINOR 0

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *
 */

nsmclient::nsmclient
(
    const std::string & nsmurl,
    const std::string & nsmfile,
    const std::string & nsmext
) :
    nsm             (nsmurl, nsmfile, nsmext),
    m_manager       (),
    m_capabilities  (),
    m_path_name     (),
    m_display_name  (),
    m_client_id     (),
    m_nsm_file      (nsmfile),
    m_nsm_ext       (nsmext),
    m_nsm_url       (nsmurl)
{
    //
}

/**
 *
 */

nsmclient::~nsmclient ()
{
    // no code so far
}

// Session client methods.

void
nsmclient::announce
(
    const std::string & appname,
    const std::string & capabilities
)
{
    nsm::announce(appname, capabilities);
}

// Session client methods.

void
nsmclient::visible (bool isvisible)
{
    if (is_active())
    {
        const char * path = nsm_visible_msg(isvisible);
        if (m_lo_address && m_lo_server)
            lo_send_from(m_lo_address, m_lo_server, LO_TT_IMMEDIATE, path, "");

#if defined SEQ66_PLATFORM_DEBUG
        printf("visible(%s): %s\n", bool_to_string(isvisible).c_str(), path);
#endif
    }
}

void
nsmclient::progress (float percent)
{
    if (is_active())
    {
        if (m_lo_address && m_lo_server)
        {
            lo_send_from
            (
                m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
                nsm_cli_progress(), "f", percent
            );
        }

#if defined SEQ66_PLATFORM_DEBUG
        printf("progress(%g)\n", percent);
#endif
    }
}

void
nsmclient::message (int priority, const std::string & mesg)
{
    if (is_active())
    {
        if (m_lo_address && m_lo_server)
        {
            lo_send_from
            (
                m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
                nsm_cli_message(), "is", priority, mesg.c_str()
            );
        }

#if defined SEQ66_PLATFORM_DEBUG
        printf("message(%d, %s\n", priority, mesg.c_str());
#endif
    }
}

/*
// Session client reply methods.

void
nsmclient::open_reply (reply replycode)
{
    nsm_reply(nsm_cli_open(), replycode);
}

void
nsmclient::save_reply (reply replycode)
{
    nsm_reply(nsm_cli_save(), replycode);
}
*/

// Server announce error.

// Client open callback.

void
nsmclient::nsm_open
(
    const std::string & path_name,
    const std::string & display_name,
    const std::string & client_id
)
{
    m_path_name = path_name;
    m_display_name = display_name;
    m_client_id = client_id;
    nsm_debug("nsm_open");
    // emit open();
}

// Client save callback.

void
nsmclient::nsm_save ()
{
    nsm_debug("nsm_save");
    // emit save();
}

// Client loaded callback.

void
nsmclient::nsm_loaded ()
{
    nsm_debug("nsm_loaded");
    // emit loaded();
}

// Client show optional GUI.

void
nsmclient::nsm_show ()
{
    nsm_debug("nsm_show");
    // emit show();
}

// Client hide optional GUI.

void
nsmclient::nsm_hide ()
{
    nsm_debug("nsm_show");
    // emit hide();
}

/*
 * Prospective caller helpers a la qtractorMainForm.
 */

/**
 *  After calling this function and checking the return value,
 *  the caller should close out any "open" items and set up a new "session".
 *
 *  After that, call open_reply() with the boolean result of the new session
 *  setup.
 */

bool
nsmclient::open_session ()
{
    bool result = is_active();
    if (result)
    {
        // m_dirty_count = 0;
        // m_dirty = false;
        m_nsm_file.clear();
    }
    return result;
}

/**
 *  TODO? Send close message, quit, abort?
 */

bool
nsmclient::close_session ()
{
    bool result = true;
    if (is_active())
    {
        m_dirty_count = 0;
    }
    return result;
}

/**
 *  The caller should call this function, then check the result before saving the
 *  "session".
 */

bool
nsmclient::save_session ()
{
    bool result = is_active();
    if (result)
    {
        m_dirty_count = 0;
        m_dirty = false;

        /*
         * Done by caller m_nsm_file.clear();
         */
    }
    return result;
}

/**
 *  See if there is NSM "present" on the host computer.
 */

std::string
get_nsm_url ()
{
    std::string result;
#if defined _GNU_SOURCE
    char * url = secure_getenv(nsm_url());
#else
    char * url = getenv(nsm_url());
#endif
    if (not_nullptr(url) && strlen(url) > 0)
        result = std::string(url);

    return result;
}

std::unique_ptr<nsmclient>
create_nsmclient
(
    const std::string & nsmfile,
    const std::string & nsmext
)
{
    std::unique_ptr<nsmclient> result;
    std::string url = get_nsm_url();
    if (! url.empty())
        result.reset(new nsmclient(url, nsmfile, nsmext));

    return result;
}

}           // namespace seq66

/*
 * nsmclient.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


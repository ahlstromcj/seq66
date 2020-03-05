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
 * \updates       2020-03-02
 * \license       GNU GPLv2 or above
 *
 *  nsmclient is an Non Session Manager (NSM) OSC client agent.  The NSM API
 *  comprises a simple Open Sound Control (OSC) based protocol.
 *
 *  The Non project contains a daemon, nsmd, which is an implementation of
 *  the server side of the NSM API. nsmd is controlled by the non-session-manager
 *  GUI. The same server-side API can also be implemented by other
 *  session managers (such as LADISH).
 *  The only dependency for client implementations is liblo (the OSC
 *  library) and the nsm.h header file.
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
#include "sessions/nsmclient.hpp"       /* seq66::nsmclient class           */

#if ! defined SEQ66_NSM_SESSION
#include "util/strfunctions.hpp"        /* seq66::bool_to_string()          */
#endif

#define NSM_API_VERSION_MAJOR 1
#define NSM_API_VERSION_MINOR 0

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

#if defined SEQ66_NSM_SESSION
static const char * s_nsm_announce = "/nsm/server/announce";
static const char * s_nsm_error = "/error";
static const char * s_nsm_hide_opt_gui = "/nsm/client/hide_optional_gui";
static const char * s_nsm_message = "/nsm/client/message";
static const char * s_nsm_progress = "/nsm/client/progress";
static const char * s_nsm_reply = "/reply";
static const char * s_nsm_list = "/nsm/session/list";   /* see banner above */
static const char * s_nsm_session_loaded = "/nsm/client/session_is_loaded";
static const char * s_nsm_show_opt_gui = "/nsm/client/show_optional_gui";
#endif

#if defined SEQ66_NSM_ADDITIONAL_MESSAGES
/nsm/server/new s:project_name
/nsm/server/close
/nsm/server/abort
/nsm/server/quit
/nsm/server/list
/nsm/server/broadcast
#endif

static const char * s_nsm_gui_hidden = "/nsm/client/gui_is_hidden";
static const char * s_nsm_gui_shown = "/nsm/client/gui_is_shown";
static const char * s_nsm_is_clean = "/nsm/client/is_clean";
static const char * s_nsm_is_dirty = "/nsm/client/is_dirty";
static const char * s_nsm_url_var = "NSM_URL";
static const char * s_nsm_open = "/nsm/client/open";
static const char * s_nsm_save = "/nsm/client/save";

std::string nsmclient::sm_nsm_default_ext = "nsm";

#if defined SEQ66_NSM_SESSION

/*
 *  This set of functions supplies some OSC (liblo) callback methods for the
 *  nsm_client to use.
 */

static int
osc_nsm_error
(
    const char * /* path */,
    const char * /* types */,
    lo_arg ** argv,
    int /* argc */,
    lo_message /* msg */,
    void * user_data
)
{
    nsmclient * pnsmc = static_cast<nsmclient *>(user_data);
    if (is_nullptr(pnsmc) || (strcmp(&argv[0]->s, "/nsm/server/announce"))
        return -1;

    pnsmc->announce_error(&argv[2]->s);
    return 0;
}

static int
osc_nsm_reply
(
    const char * /* path */,
    const char * /* types */,
    lo_arg ** argv,
    int /* argc */,
    lo_message /* msg */,
    void * user_data
)
{
    nsmclient * pnsmc = static_cast<nsmclient *>(user_data);
    if (is_nullptr(pnsmc) || (strcmp(&argv[0]->s, "/nsm/server/announce"))
        return -1;

    pnsmc->announce_reply(&argv[1]->s, &argv[2]->s, &argv[3]->s);
    return 0;
}

static int
osc_nsm_open
(
    const char * /* path */,
    const char * /* types */,
    lo_arg ** argv,
    int /* argc */,
    lo_message /* msg */,
    void * user_data
)
{
    nsmclient * pnsmc = static_cast<nsmclient *>(user_data);
    if (is_nullptr(pnsmc))
        return -1;

    pnsmc->nsm_open(&argv[0]->s, &argv[1]->s, &argv[2]->s);
    return 0;
}

static int
osc_nsm_save
(
    const char * /* path */,
    const char * /* types */,
    lo_arg ** /* argv */,
    int /* argc */,
    lo_message /* msg */,
    void * user_data
)
{
    nsmclient * pnsmc = static_cast<nsmclient *>(user_data);
    if (is_nullptr(pnsmc))
        return -1;

    pnsmc->nsm_save();
    return 0;
}

static int
osc_nsm_loaded
(
    const char * /* path */,
    const char * /* types */,
    lo_arg ** /* argv */,
    int /* argc */,
    lo_message /* msg */,
    void * user_data
)
{
    nsmclient * pnsmc = static_cast<nsmclient *>(user_data);
    if (is_nullptr(pnsmc))
        return -1;

    pnsmc->nsm_loaded();
    return 0;
}

static int
osc_nsm_show
(
    const char * /* path */,
    const char * /* types */,
    lo_arg ** /* argv */,
    int /* argc */,
    lo_message /* msg */,
    void * user_data
)
{
    nsmclient * pnsmc = static_cast<nsmclient *>(user_data);
    if (is_nullptr(pnsmc))
        return -1;

    pnsmc->nsm_show();
    return 0;
}

static int
osc_nsm_hide
(
    const char * /* path */,
    const char * /* types */,
    lo_arg ** /* argv */,
    int /* argc */,
    lo_message /* msg */,
    void * user_data
)
{
    nsmclient * pnsmc = static_cast<nsmclient *>(user_data);
    if (pnsmc == NULL)
        return -1;

    pnsmc->nsm_hide();
    return 0;
}

#endif  // SEQ66_NSM_SESSION

/**
 *
 */

nsmclient::nsmclient
(
    const std::string & nsm_url,
    const std::string & nsm_file,
    const std::string & nsm_ext
) :
//  QObject         (pParent),
#if defined SEQ66_NSM_SESSION
    m_lo_address    (),
    m_lo_thread     (),
    m_lo_server     (),
#endif
    m_active        (false),
    m_dirty         (false),
    m_dirty_count   (0),
    m_manager       (),
    m_capabilities  (),
    m_path_name     (),
    m_display_name  (),
    m_client_id     (),
    m_nsm_file      (nsm_file),
    m_nsm_ext       (nsm_ext),
    m_nsm_url       (nsm_url)
{
#if defined SEQ66_NSM_SESSION
#define ADD(x, y, z)  lo_server_thread_add_method(m_lo_thread, x, y, z, this)
    m_lo_address = lo_address_new_from_url(nsm_url.c_str()());

    const int proto = lo_address_get_protocol(m_lo_address);
    m_lo_thread = lo_server_thread_new_with_proto(NULL, proto, NULL);
    if (m_lo_thread)
    {
        m_lo_server = lo_server_thread_get_server(m_lo_thread);
        ADD(s_nsm_error, "sis", osc_nsm_error, this);
        ADD(s_nsm_reply, "ssss", osc_nsm_reply, this);
        ADD(s_nsm_open, "sss", osc_nsm_open, this);
        ADD(s_nsm_save, "", osc_nsm_save, this);
        ADD(s_nsm_session_loaded, "", osc_nsm_loaded, this);
        ADD(s_nsm_show_opt_gui, "", osc_nsm_show, this);
        ADD(s_nsm_hide_opt_gui, "", osc_nsm_hide, this);
        lo_server_thread_start(m_lo_thread);
    }
#endif  // SEQ66_NSM_SESSION

    if (m_nsm_ext.empty())
        m_nsm_ext = default_ext();
}

/**
 *
 */

nsmclient::~nsmclient ()
{
#if defined SEQ66_NSM_SESSION
    if (m_lo_thread)
    {
        lo_server_thread_stop(m_lo_thread);
        lo_server_thread_free(m_lo_thread);
    }
    if (m_lo_address)
        lo_address_free(m_lo_address);
#endif
}

// Session clieant methods.

void
nsmclient::announce
(
    const std::string & app_name,
    const std::string & capabilities
)
{
#if defined SEQ66_NSM_SESSION_TODO
    if (m_lo_address && m_lo_server)
    {
        const QFileInfo fi(QApplication::applicationFilePath());
        lo_send_from
        (
            m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
            s_nsm_announce, "sssiii",
            app_name.c_str(), capabilities.c_str(),
            fi.fileName().c_str(),
            NSM_API_VERSION_MAJOR, NSM_API_VERSION_MINOR,
            // NEED A PID obtainer!!!!!!
            int(QApplication::applicationPid())
        );
    }
#else
    printf("announce(%s, %s)\n", app_name.c_str(), capabilities.c_str());
#endif
}

// Session client methods.

void
nsmclient::dirty (bool isdirty)
{
    if (m_active)
    {
        const char * path = isdirty ?  s_nsm_is_dirty : s_nsm_is_clean;
#if defined SEQ66_NSM_SESSION
        if (m_lo_address && m_lo_server)
        {
            lo_send_from(m_lo_address, m_lo_server, LO_TT_IMMEDIATE, path, "");
            m_dirty = true;
        }
#else
        printf("dirty(%s): %s\n", bool_to_string(isdirty).c_str(), path);
#endif
    }
}

void
nsmclient::update_dirty_count (bool flag)
{
    if (flag)
        ++m_dirty_count;
    else
        m_dirty_count = 0;

    if (is_active())
    {
        if (! m_dirty && flag)
            m_dirty = true;
        else if (m_dirty && ! flag)
            m_dirty = false;
    }
}

void
nsmclient::visible (bool isvisible)
{
    if (m_active)
    {
        const char * path = isvisible ? s_nsm_gui_shown : s_nsm_gui_hidden;
#if defined SEQ66_NSM_SESSION
        if (m_lo_address && m_lo_server)
            lo_send_from(m_lo_address, m_lo_server, LO_TT_IMMEDIATE, path, "");
#else
        printf("visible(%s): %s\n", bool_to_string(isvisible).c_str(), path);
#endif
    }
}

void
nsmclient::progress (float percent)
{
    if (m_active)
    {
#if defined SEQ66_NSM_SESSION
        if (m_lo_address && m_lo_server)
        {
            lo_send_from
            (
                m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
                s_nsm_progress, "f", percent
            );
        }
#else
        printf("progress(%g)\n", percent);
#endif
    }
}

void
nsmclient::message (int priority, const std::string & mesg)
{
    if (m_active)
    {
#if defined SEQ66_NSM_SESSION
        if (m_lo_address && m_lo_server)
        {
            lo_send_from
            (
                m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
                s_nsm_message, "is", priority, mesg.c_str()
            );
        }
#else
        printf("message(%d, %s\n", priority, mesg.c_str());
#endif
    }
}

// Session client reply methods.

void
nsmclient::open_reply (nsmreply replycode)
{
    nsm_reply(s_nsm_open, replycode);
}

void
nsmclient::save_reply (nsmreply replycode)
{
    nsm_reply(s_nsm_save, replycode);
}

void
nsmclient::nsm_reply (const std::string & path, nsmreply replycode)
{
    const char * reply_mesg;
    switch (replycode)
    {
    case nsmreply::ok:               reply_mesg = "OK";                break;
    case nsmreply::general:          reply_mesg = "General error";     break;
    case nsmreply::incompatible_api: reply_mesg = "Incompatible API";  break;
    case nsmreply::blacklisted:      reply_mesg = "Blacklisted";       break;
    case nsmreply::launch_failed:    reply_mesg = "Launch failed";     break;
    case nsmreply::no_such_file:     reply_mesg = "No such file";      break;
    case nsmreply::no_session_open:  reply_mesg = "No session open";   break;
    case nsmreply::unsaved_changes:  reply_mesg = "Unsaved changes";   break;
    case nsmreply::not_now:          reply_mesg = "Not now";           break;
    default:                         reply_mesg = "Unknown reply";     break;
    }
#if defined SEQ66_NSM_SESSION
    if (m_lo_address && m_lo_server)
    {
        if (replycode == reply::ok)
        {
            lo_send_from
            (
                m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
                "/reply", "ss", path.c_str(), reply_mesg
            );
        }
        else
        {
            lo_send_from
            (
                m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
                "/error", "sis", path.c_str(),
                int(replycode), reply_mesg
            );
        }
    }
#else
    printf("NSM reply ss %s %s\n", path.c_str(), reply_mesg);
#endif
}

// Server announce error.

void
nsmclient::announce_error (const std::string & mesg)
{
    m_active = false;
    m_manager.clear();
    m_capabilities.clear();
    m_path_name.clear();
    m_display_name.clear();
    m_client_id.clear();
    // emit active(false);
    printf("NSM: Failed to register with server: %s.", mesg.c_str());
}

// Server announce reply.

void
nsmclient::announce_reply
(
    const std::string & mesg,
    const std::string & manager,
    const std::string & capabilities
)
{
    m_active = true;
    m_manager = manager;
    m_capabilities = capabilities;
    // emit active(true);
    printf("NSM: Successfully registered with server: %s.", mesg.c_str());
}

/**
 *
 */

void
nsmclient::nsm_debug (const std::string & tag)
{
    if (! tag.empty())
    {
#if defined SEQ66_PLATFORM_DEBUG
        printf
        (
            "nsmclient::%s: path_name='%s' display_name='%s' client_id='%s'",
            tag.c_str(),
            m_path_name.c_str(),
            m_display_name.c_str(),
            m_client_id.c_str()
        );
#endif
    }
}

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
        m_dirty_count = 0;
        m_dirty = false;
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
    char * url = secure_getenv(s_nsm_url_var);
#else
    char * url = getenv(s_nsm_url_var);
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


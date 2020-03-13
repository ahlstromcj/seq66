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
 * \file          nsm.cpp
 *
 *  This module defines some informative functions that are actually
 *  better off as functions.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2020-03-07
 * \updates       2020-03-09
 * \license       GNU GPLv2 or above
 *
 *  nsm is an Non Session Manager (NSM) OSC client helper.  The NSM API
 *  comprises a simple Open Sound Control (OSC) based protocol.
 *
 *  The Non project contains a daemon, nsmd, which is an implementation of the
 *  server side of the NSM API. nsmd is controlled by the non-session-manager
 *  GUI. The same server-side API can also be implemented by other session
 *  managers (such as LADISH).  The only dependency for client implementations
 *  is liblo (the OSC library) and code based on the nsm.h header file.
 *
 *  Find out if NSM_URL is defined in the host environment, and create the nsm
 *  object on the heap if so. The create_nsm() should be used; it returns a
 *  "unique pointer".  (We may need to provide specific factory functions for
 *  Qt, Gtkmm, and command-line versions of the application.)
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
 */

#include <stdlib.h>                     /* C geteven() or secure_getenv()   */
#include <string.h>                     /* C strlen(), strcmp()             */
#include <sys/types.h>                  /* provides the pid_t typedef       */
#include <unistd.h>                     /* C getpid()                       */

#include "util/basic_macros.h"          /* not_nullptr() macro              */
#include "util/filefunctions.hpp"       /* seq66::executable_full_path()    */
#include "nsm/nsm.hpp"                  /* seq66::nsm class                 */
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

#define ADD_METHOD(x, y, z) \
    lo_server_thread_add_method(m_lo_thread, x, y, z, this)

/**
 *  This set of functions supplies some OSC (liblo) callback methods for the
 *  nsm_client to use.
 *
 *  Note that lo_arg (/usr/include/lo/lo_osc_types.h) is a union that covers
 *  various integer types, floats, doubles, chars of various kinds, 4-byte MIDI
 *  packets, lo_timetag, and a "blob" structure (size + data).
 *
 *  The lo_message type is a low-level object messages passed via OSC.  It is a
 *  void pointer.
 *
 *  Finally, note the "sis" check for types. The type characters are defined in
 *  the OSC header lo_osc_types.h, and there are values for float ('f'), string
 *  ('s'), 32-bit integers ('i'), and more.  These character tag messages and
 *  specify the arguments of lo_send().
 *
 *  This function could also be called osc_announce_error().
 */

static int
osc_nsm_error
(
    const char * /* path */,
    const char * types,
    lo_arg ** argv,
    int /* argc */,
    lo_message /* msg */,
    void * user_data
)
{
    nsm * pnsmc = static_cast<nsm *>(user_data);
    bool not_sis = strcmp(types, "sis") != 0;
    if (is_nullptr(pnsmc) || not_sis || (! nsm_is_announce(&argv[0]->s)))
        return -1;

    pnsmc->announce_error(&argv[2]->s);
    return 0;
}

/**
 *  The typical type signature of this callback is "ssss".
 *
 *  This function could also be called osc_announce_reply().
 */

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
    nsm * pnsmc = static_cast<nsm *>(user_data);
    if (is_nullptr(pnsmc) || (! nsm_is_announce(&argv[0]->s)))
        return -1;

    // If needed, how to add to nsm::announce_reply()?
    //
    // nsm_addr = lo_address_new_from_url
    // (
    //      lo_address_get_url(lo_message_get_source(mesg);
    // );

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
    nsm * pnsmc = static_cast<nsm *>(user_data);
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
    nsm * pnsmc = static_cast<nsm *>(user_data);
    if (is_nullptr(pnsmc))
        return -1;

    pnsmc->nsm_save();                  /* a virtual function   */
    return 0;
}

static int
osc_nsm_session_loaded
(
    const char * /* path */,
    const char * /* types */,
    lo_arg ** /* argv */,
    int /* argc */,
    lo_message /* msg */,
    void * user_data
)
{
    nsm * pnsmc = static_cast<nsm *>(user_data);
    if (is_nullptr(pnsmc))
        return -1;

    pnsmc->nsm_loaded();
    return 0;
}

static int
osc_nsm_label
(
    const char * /* path */,
    const char * /* types */,
    lo_arg ** argv,
    int /* argc */,
    lo_message /* msg */,
    void * user_data
)
{
    nsm * pnsmc = static_cast<nsm *>(user_data);
    if (is_nullptr(pnsmc))
        return -1;

    std::string label = std::string(&argv[0]->s);
    pnsmc->nsm_label(label);            /* a virtual function */
    return 0;
}

/**
 *
 *  This function could also be called osc_show_gui().  See the nsm-proxy code.
 */

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
    nsm * pnsmc = static_cast<nsm *>(user_data);
    if (is_nullptr(pnsmc))
        return -1;

    pnsmc->nsm_show();                  /* a virtual function   */

//  lo_send_from(m_lo_address, losrv, LO_TT_IMMEDIATE, "/reply", "ss", path, "OK");
    return 0;
}

/**
 *
 *  This function could also be called osc_hide_gui().  See the nsm-proxy code.
 */

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
    nsm * pnsmc = static_cast<nsm *>(user_data);
    if (pnsmc == NULL)
        return -1;

    pnsmc->nsm_hide();
//
//  Why is this reply different from the show-gui version?
//
//  lo_send_from
//  (
//      m_lo_address, losrv, LO_TT_IMMEDIATE,
//      nsm_cli_gui_hidden, "ss", path, "OK"
//  );
//  lo_send_from(m_lo_address, losrv, LO_TT_IMMEDIATE, "/reply", "ss", path, "OK");
    return 0;
}

//
// MORE OSC CALLBACKS:
//
// osc_stop_signal() : nsm_stop_signal() virtual function
// osc_start() : nsm_start() virtual function
// osc_kill() : nsm_kill() virtual function
// osc_update() : nsm_update() virtual function

/**
 *
 */

nsm::nsm
(
    const std::string & nsmurl,
    const std::string & nsmfile,
    const std::string & nsmext
) :
    m_lo_address    (),
    m_lo_thread     (),
    m_lo_server     (),
    m_active        (false),
    m_dirty         (false),
    m_dirty_count   (0),

    m_manager       (),
    m_capabilities  (),
    m_path_name     (),
    m_display_name  (),
    m_client_id     (),
    m_nsm_file      (nsmfile),
    m_nsm_ext       (nsmext),
    m_nsm_url       (nsmurl)
{
    m_lo_address = lo_address_new_from_url(nsm_url().c_str());

    const int proto = lo_address_get_protocol(m_lo_address);
    m_lo_thread = lo_server_thread_new_with_proto(NULL, proto, NULL);
    if (m_lo_thread)
    {
        m_lo_server = lo_server_thread_get_server(m_lo_thread);
        ADD_METHOD(nsm_basic_error(), "sis", osc_nsm_error);
        ADD_METHOD(nsm_basic_reply(), "ssss", osc_nsm_reply);
        ADD_METHOD(nsm_cli_open(), "sss", osc_nsm_open);
        ADD_METHOD(nsm_cli_save(), "", osc_nsm_save);
        ADD_METHOD(nsm_cli_is_loaded(), "", osc_nsm_session_loaded);
        ADD_METHOD(nsm_cli_label(), "", osc_nsm_label);
        ADD_METHOD(nsm_cli_show_opt_gui(), "", osc_nsm_show);
        ADD_METHOD(nsm_cli_hide_opt_gui(), "", osc_nsm_hide);
        lo_server_thread_start(m_lo_thread);
    }
    if (m_nsm_ext.empty())
        m_nsm_ext = nsm_default_ext();
}

/**
 *
 */

nsm::~nsm ()
{
    if (m_lo_thread)
    {
        lo_server_thread_stop(m_lo_thread);
        lo_server_thread_free(m_lo_thread);
    }
    if (m_lo_address)
        lo_address_free(m_lo_address);
}

bool
nsm::lo_is_valid () const
{
    return not_nullptr_2(m_lo_address, m_lo_server);
}

// Session client methods.

void
nsm::announce                           // server announce???
(
    const std::string & appname,
    const std::string & capabilities
)
{
    if (lo_is_valid())
    {
        std::string f = executable_full_path();
        const char * filename = f.c_str();
        const char * app = appname.c_str();
        const char * caps = capabilities.c_str();
        int pid = int(getpid());
        lo_send_from
        (
            m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
            nsm_srv_announce(), "sssiii",
            app, caps, filename,
            NSM_API_VERSION_MAJOR, NSM_API_VERSION_MINOR, pid
        );
    }
#if defined SEQ66_PLATFORM_DEBUG
    printf("announce(%s, %s)\n", appname.c_str(), capabilities.c_str());
#endif
}

// Session client methods.

void
nsm::dirty (bool isdirty)
{
    if (m_active)
    {
        const char * path = nsm_dirty_msg(isdirty);
        if (lo_is_valid())
        {
            lo_send_from(m_lo_address, m_lo_server, LO_TT_IMMEDIATE, path, "");
            m_dirty = true;
        }
#if defined SEQ66_PLATFORM_DEBUG
        printf("dirty(%s): %s\n", bool_to_string(isdirty).c_str(), path);
#endif
    }
}

void
nsm::update_dirty_count (bool updatedirt)
{
    if (updatedirt)
        ++m_dirty_count;
    else
        m_dirty_count = 0;

    if (is_active())
    {
        if (! m_dirty && updatedirt)
            m_dirty = true;
        else if (m_dirty && ! updatedirt)
            m_dirty = false;
    }
}

void
nsm::visible (bool isvisible)
{
    if (m_active)
    {
        const char * path = nsm_visible_msg(isvisible);
        if (lo_is_valid())
            lo_send_from(m_lo_address, m_lo_server, LO_TT_IMMEDIATE, path, "");

#if defined SEQ66_PLATFORM_DEBUG
        printf("visible(%s): %s\n", bool_to_string(isvisible).c_str(), path);
#endif
    }
}

void
nsm::progress (float percent)
{
    if (m_active)
    {
        if (lo_is_valid())
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
nsm::message (int priority, const std::string & mesg)
{
    if (m_active)
    {
        if (lo_is_valid())
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

// Session client reply methods.

void
nsm::open_reply (reply replycode)
{
    nsm_reply(nsm_cli_open(), replycode);
}

void
nsm::save_reply (reply replycode)
{
    nsm_reply(nsm_cli_save(), replycode);
}

const char *
nsm::nsm_reply_message (reply replycode)
{
    const char * result;
    switch (replycode)
    {
    case reply::ok:               result = "OK";                break;
    case reply::general:          result = "General error";     break;
    case reply::incompatible_api: result = "Incompatible API";  break;
    case reply::blacklisted:      result = "Blacklisted";       break;
    case reply::launch_failed:    result = "Launch failed";     break;
    case reply::no_such_file:     result = "No such file";      break;
    case reply::no_session_open:  result = "No session open";   break;
    case reply::unsaved_changes:  result = "Unsaved changes";   break;
    case reply::not_now:          result = "Not now";           break;
    default:                      result = "Unknown reply";     break;
    }
    return result;
}

void
nsm::nsm_reply (const std::string & path, reply replycode)
{
    const char * reply_mesg = nsm_reply_message(replycode);
    if (lo_is_valid())
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

#if defined SEQ66_PLATFORM_DEBUG
    printf("NSM reply ss %s %s\n", path.c_str(), reply_mesg);
#endif
}

// Server announce error.

void
nsm::announce_error (const std::string & mesg)
{
    m_active = false;
    m_manager.clear();
    m_capabilities.clear();
    m_path_name.clear();
    m_display_name.clear();
    m_client_id.clear();
    // emit active(false);
    printf("NSM: Failed to register with server: %s.\n", mesg.c_str());
}

// Server announce reply.

void
nsm::announce_reply
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
    //
    // From nsm-proxy:
    //
    // nsm_addr = lo_address_new_from_url
    // (
    //      lo_address_get_url(lo_message_get_source(mesg);
    // );

    printf("NSM: Successfully registered with server: %s.\n", mesg.c_str());
}

/**
 *
 */

void
nsm::nsm_debug (const std::string & tag)
{
    if (! tag.empty())
    {
#if defined SEQ66_PLATFORM_DEBUG
        printf
        (
            "nsm::%s: path_name='%s' display_name='%s' client_id='%s'",
            tag.c_str(),
            m_path_name.c_str(),
            m_display_name.c_str(),
            m_client_id.c_str()
        );
#endif
    }
}

/**
 *  Client open callback. Compare to the "open" code in nsm-proxy.
 */

void
nsm::nsm_open
(
    const std::string & pathname,
    const std::string & displayname,
    const std::string & clientid
)
{
    m_path_name = pathname;
    m_display_name = displayname;
    m_client_id = clientid;
    nsm_debug("nsm_open");
    // emit open();
}

// Client save callback.

void
nsm::nsm_save ()
{
    nsm_debug("nsm_save");
    // emit save();
    //
    // From nsm-proxy:
    //
    // bool r = snapshot( project_file );
    // nsm_proxy->save();
    // if (r)
    //  lo_send_from
    //  (
    //      m_lo_address, losrv, LO_TT_IMMEDIATE, "/reply", "ss", path, "OK"
    //  );
    // else
    //  lo_send_from
    //  (
    //      m_lo_address, losrv, LO_TT_IMMEDIATE, "/error", "sis", path, -1,
    //      "Error saving project file"
    //  );

}

// Client loaded callback.

void
nsm::nsm_loaded ()
{
    nsm_debug("nsm_loaded");
    // emit loaded();
}

#if defined USE_THIS_CODE

/**
 *  This sample is from nsm-proxy
 */

void
show_gui ()
{
    int pid;
    if (! (pid = fork()))
    {
        char executable[] = "nsm-proxy-gui";
        MESSAGE("Launching %s\n", executable);
        char * url = lo_server_get_url(losrv);
        char * args[] = { executable, strdup( "--connect-to" ), url, NULL };
        if (-1 == execvp(executable, args))
        {
            WARNING( "Error starting process: %s", strerror( errno ) );
            exit(1);
        }
    }
    gui_pid = pid;
//  lo_send_from(m_lo_address, losrv, LO_TT_IMMEDIATE, nsm_cli_gui_shown(), "");
}

#endif  // USE_SAMPLE_CODE

/**
 *  Client show optional GUI.  The derived class must provide this
 *  functionality.
 */

void
nsm::nsm_show ()
{
    nsm_debug("nsm_show");
    // emit show();
}

// Client hide optional GUI.

void
nsm::nsm_hide ()
{
    nsm_debug("nsm_hide");
    // emit hide();
    //
    // From nsm-proxy:
    //
    // if (gui_pid)
    //  kill(gui_pid, SIGTERM);
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
nsm::open_session ()
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
nsm::close_session ()
{
    bool result = is_active();
    if (result)
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
nsm::save_session ()
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

#if defined USE_THIS_CODE

/**
 *  Construct a message of the form:
 *
\verbatim
    /nsm/server/announce s:application_name s:capabilities
        s:executable_name i:api_version_major
        i:api_version_minor i:pid
\verbatim
 *
 *  Do we really need this function? See nsm::announce() above!
 *
 *  exename must be the executable name that the program was launched with.
 *  For C programs, this is simply the value of argv[0]. Note that hardcoding
 *  the name of the program here is not the same as using, as the user may have
 *  launched the program from a script with a different name using exec, or
 *  have created a symlink to the program.
 */

std::string
nsm::construct_server_announce
(
    const std::string & appname,
    const std::string & exename,
    const std::string & capabilities,
)
{
    std::string result(nsm_srv_announce());
    result += " ";
    result += "s:";
    result += appname;
    result += "s:";
    result += capabilities;
    result += "s:";
    result += exename;
    result += "i:";
    result += std::to_string(NSM_API_VERSION_MAJOR)
    result += "i:";
    result += std::to_string(NSM_API_VERSION_MINOR)
    result += "i:";
    result += std::to_string(int(getpid()));
    return result;
}

/**
 * HMMMMM.
 */

std::string
nsm::construct_caps (srvcaps c)
{
    std::string result = ":";

    return result;
}

#endif  // defined USE_THIS_CODE

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

}           // namespace seq66

/*
 * nsm.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


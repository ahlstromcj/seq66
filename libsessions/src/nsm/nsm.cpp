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
 * \updates       2020-08-15
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

#include <cstring>                      /* C strlen(), strcmp()             */
#include <sys/types.h>                  /* provides the pid_t typedef       */
#include <unistd.h>                     /* C getpid()                       */

#include "util/basic_macros.hpp"        /* not_nullptr(), warnprint(), etc. */
#include "util/filefunctions.hpp"       /* seq66::executable_full_path()    */
#include "nsm/nsm.hpp"                  /* seq66::nsm class                 */
#include "nsm/nsmmessages.hpp"          /* seq66::nsm message functions     */
#include "sessions/smfunctions.hpp"     /* seq66::get_session_url()         */

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

    pnsmc->open(&argv[0]->s, &argv[1]->s, &argv[2]->s);
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

    pnsmc->save();                  /* a virtual function   */
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

    pnsmc->loaded();
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
    pnsmc->label(label);            /* a virtual function */
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

    pnsmc->show();                  /* a virtual function   */

    //  lo_send_from
    //  (
    //      m_lo_address, losrv, LO_TT_IMMEDIATE, "/reply", "ss", path, "OK"
    //  );
    return 0;
}

/**
 *  This function could also be called osc_hide_gui().  See the nsm-proxy
 *  code.
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

    pnsmc->hide();
/*
    Why is this reply different from the show-gui version?

    lo_send_from
    (
        m_lo_address, losrv, LO_TT_IMMEDIATE,
        nsm_cli_gui_hidden, "ss", path, "OK"
    );
    lo_send_from
    (
        m_lo_address, losrv, LO_TT_IMMEDIATE, "/reply", "ss", path, "OK"
    );
*/
    return 0;
}

/**
 *
 */

static int
osc_nsm_broadcast
(
    const char * path,
    const char * /* types */,
    lo_arg ** /* argv */,
    int /* argc */,
    lo_message msg,
    void * user_data
)
{
    nsm * pnsmc = static_cast<nsm *>(user_data);
    if (pnsmc == NULL)
        return -1;

    pnsmc->broadcast(path, msg);
    return 0;
}

/*
   osc_stop_signal() : nsm_stop_signal() virtual function
   osc_start() : nsm_start() virtual function
   osc_kill() : nsm_kill() virtual function
   osc_update() : nsm_update() virtual function
 */

/**
 *  This constructor should (currently) not be called unless the NSM URL was
 *  found to be good.
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
    if (not_nullptr(m_lo_address))
    {
        const int proto = lo_address_get_protocol(m_lo_address);
        m_lo_thread = lo_server_thread_new_with_proto(NULL, proto, NULL);
        if (not_nullptr(m_lo_thread))
        {
            m_lo_server = lo_server_thread_get_server(m_lo_thread);
            if (not_nullptr(m_lo_server))
            {
                ADD_METHOD(nsm_basic_error(), "sis", osc_nsm_error);
                ADD_METHOD(nsm_basic_reply(), "ssss", osc_nsm_reply);
                ADD_METHOD(nsm_cli_open(), "sss", osc_nsm_open);
                ADD_METHOD(nsm_cli_save(), "", osc_nsm_save);
                ADD_METHOD(NULL, NULL, osc_nsm_broadcast);
                ADD_METHOD(nsm_cli_is_loaded(), "", osc_nsm_session_loaded);
                ADD_METHOD(nsm_cli_label(), "", osc_nsm_label);
                ADD_METHOD(nsm_cli_show_opt_gui(), "", osc_nsm_show);
                ADD_METHOD(nsm_cli_hide_opt_gui(), "", osc_nsm_hide);
                lo_server_thread_start(m_lo_thread);
                if (m_nsm_ext.empty())
                    m_nsm_ext = nsm_default_ext();
            }
            else
            {
#if defined SEQ66_PLATFORM_DEBUG
                printf("nsm::nsm(): bad server\n");
#endif
            }
        }
        else
        {
#if defined SEQ66_PLATFORM_DEBUG
            printf("nsm::nsm(): bad thread\n");
#endif
        }
    }
    else
    {
#if defined SEQ66_PLATFORM_DEBUG
        printf("nsm::nsm(): bad address\n");
#endif
    }
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
    if (is_nullptr_2(m_lo_address, m_lo_server))
        m_active = false;

    return m_active;    // && not_nullptr_2(m_lo_address, m_lo_server);
}

/**
 *  Provides a client-announce function.
 *
 *  If NSM_URL is valid and reachable, call this function to send the following
 *  "sssiii" message to the provided address as soon as ready to respond to the
 *  /nsm/client/open event.  api_version_major and api_version_minor must be
 *  the two parts of the version number of the NSM API.  If registering JACK
 *  clients, application_name must be passed to jack_client_open.  capabilities
 *  is a string containing a list of the capabilities the client possesses,
 *  e.g.  :dirty:switch:progress: executable_name must be the executable name
 *  that launched the program (e.g argv[0]).
 *
\verbatim
    /nsm/server/announce s:application_name s:capabilities s:executable_name
         i:api_version_major i:api_version_minor i:pid
\endverbatim
 *
 */

void
nsm::announce
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

/*
 * Session client methods.
 */

void
nsm::dirty (bool isdirty)
{
    if (lo_is_valid())
    {
        const char * path = nsm_dirty_msg(isdirty);
        lo_send_from(m_lo_address, m_lo_server, LO_TT_IMMEDIATE, path, "");
        m_dirty = true;
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
    if (lo_is_valid())
    {
        const char * path = nsm_visible_msg(isvisible);
        lo_send_from(m_lo_address, m_lo_server, LO_TT_IMMEDIATE, path, "");
#if defined SEQ66_PLATFORM_DEBUG
        printf("visible(%s): %s\n", bool_to_string(isvisible).c_str(), path);
#endif
    }
}

/**
 *  Call this function to send a progress indication to the session manager.
 *
 * \param percent
 *      The indication of progress, ranging from 0.0 to 100.0.
 */

void
nsm::progress (float percent)
{
    if (lo_is_valid())
    {
        lo_send_from
        (
            m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
            nsm_cli_progress(), "f", percent
        );
#if defined SEQ66_PLATFORM_DEBUG
        printf("progress(%g)\n", percent);
#endif
    }
}

/**
 *  Send out the indication of dirtiness status.
 */

void
nsm::is_dirty ()
{
    if (lo_is_valid())
    {
        lo_send_from
        (
            m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
            nsm_cli_is_dirty(), ""
        );
#if defined SEQ66_PLATFORM_DEBUG
        printf("is_dirty()\n");
#endif
    }
}

/**
 *  Send out the indication of cleanliness status.
 */

void
nsm::is_clean ()
{
    if (lo_is_valid())
    {
        lo_send_from
        (
            m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
            nsm_cli_is_clean(), ""
        );
#if defined SEQ66_PLATFORM_DEBUG
        printf("is_clean()\n");
#endif
    }
}

/**
 *
 */


void
nsm::message (int priority, const std::string & mesg)
{
    if (lo_is_valid())
    {
        lo_send_from
        (
            m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
            nsm_cli_message(), "is", priority, mesg.c_str()
        );
#if defined SEQ66_PLATFORM_DEBUG
        printf("message(%d, %s\n", priority, mesg.c_str());
#endif
    }
}

/*
 * Session client reply methods.
 */

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

/*
 * Server announce error.
 */

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
    warnprintf("NSM: Failed to register with server: %s.\n", mesg.c_str());
}

/*
 * Server announce reply.
 */

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

    char temp[256];
    snprintf
    (
        temp, sizeof temp,
        "NSM: Successfully registered with server:\n"
        "Message: '%s'\n"
        "Manager: '%s'\n"
        "Caps:    '%s'\n"
        ,
        mesg.c_str(), manager.c_str(), capabilities.c_str()
    );
    infoprint(temp);
}

/**
 *
 */

void
nsm::nsm_debug (const std::string & tag)
{
    if (tag.empty())
    {
#if defined SEQ66_PLATFORM_DEBUG
        printf
        (
            "nsm: path_name='%s' display_name='%s' client_id='%s'\n",
            m_path_name.c_str(),
            m_display_name.c_str(),
            m_client_id.c_str()
        );
#endif
    }
    else
    {
        infoprintf("nsm: %s\n", tag.c_str());
    }
}

/**
 *  Client open callback. Compare to the "open" code in nsm-proxy.
 */

void
nsm::open
(
    const std::string & pathname,
    const std::string & displayname,
    const std::string & clientid
)
{
    m_path_name = pathname;
    m_display_name = displayname;
    m_client_id = clientid;
    nsm_debug("open");
    // emit open();
}

/*
 * Client save callback.
 */

void
nsm::save ()
{
    nsm_debug("save");
    //
    // Here, zyn gets a character message and an error code, and replies with
    // either a reply or an error-reply.
    //
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
nsm::loaded ()
{
    nsm_debug("loaded");
    // emit loaded();
}

void
nsm::label (const std::string & label)
{
    std::string tag("label: '");
    tag += label;
    tag += "'";
    nsm_debug(tag); // no code
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

        /*
         * Note that "url" would need to be freed, as per the liblo
         * documentation.
         */

        char * url = lo_server_get_url(losrv);
        char * args[] = { executable, strdup( "--connect-to" ), url, NULL };
        if (-1 == execvp(executable, args))
        {
            WARNING( "Error starting process: %s", strerror( errno ) );
            exit(1);
        }
    }
    gui_pid = pid;
    //
    // lo_send_from
    // (
    //      m_lo_address, losrv, LO_TT_IMMEDIATE, nsm_cli_gui_shown(), ""
    // );
}

#endif  // USE_SAMPLE_CODE

/**
 *  Client show optional GUI.  The derived class must provide this
 *  functionality.
 */

void
nsm::show ()
{
    nsm_debug("show");
    // emit show();
}

/*
 * Client hide optional GUI.
 */

void
nsm::hide ()
{
    nsm_debug("hide");
    // emit hide();
    //
    // From nsm-proxy:
    //
    // if (gui_pid)
    //  kill(gui_pid, SIGTERM);
}

/**
 *  The lo_message type is a typedef of a void pointer, as are most of the
 *  "lo_" types.
 */

void
nsm::broadcast (const std::string & path, lo_message msg)
{
    nsm_debug("broadcast");
    if (lo_is_valid())
    {
        lo_send_message_from
        (
            m_lo_address, m_lo_server, nsm_srv_broadcast(), msg
        );
    }
    // emit broadcast();
    infoprintf("nsm::broadcast(%s)\n", path.c_str());
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

#endif  // defined USE_THIS_CODE

/**
 *  See if there is NSM "present" on the host computer.
 */

std::string
get_nsm_url ()
{
    return get_session_url(nsm_url());
}

}           // namespace seq66

/*
 * nsm.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


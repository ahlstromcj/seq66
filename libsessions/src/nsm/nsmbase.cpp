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
 * \file          nsmbase.cpp
 *
 *  This module defines some informative functions that are actually
 *  better off as functions.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2020-03-07
 * \updates       2020-08-25
 * \license       GNU GPLv2 or above
 *
 *  nsmbase is an Non Session Manager (NSM) OSC client helper.  The NSM API
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
#include "nsm/nsmbase.hpp"              /* seq66::nsmbase class             */
#include "nsm/nsmmessagesex.hpp"        /* seq66::nsm new message functions */
#include "sessions/smfunctions.hpp"     /* seq66::get_session_url()         */

// #if defined SEQ66_PLATFORM_DEBUG
// #include "util/strfunctions.hpp"        /* seq66::bool_to_string()          */
// #endif

#define NSM_API_VERSION_MAJOR   1
#define NSM_API_VERSION_MINOR   0

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  This set of functions supplies some OSC (liblo) callback methods for the
 *  nsmclient to use.
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
    nsmbase * pnsmc = static_cast<nsmbase *>(user_data);
    bool not_sis = strcmp(types, "sis") != 0;
    if (is_nullptr(pnsmc) || not_sis || (! nsm::is_announce(&argv[0]->s)))
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
    nsmbase * pnsmc = static_cast<nsmbase *>(user_data);
    if (is_nullptr(pnsmc) || (! nsm::is_announce(&argv[0]->s)))
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
    nsmbase * pnsmc = static_cast<nsmbase *>(user_data);
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
    nsmbase * pnsmc = static_cast<nsmbase *>(user_data);
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
    nsmbase * pnsmc = static_cast<nsmbase *>(user_data);
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
    nsmbase * pnsmc = static_cast<nsmbase *>(user_data);
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
    const char * path,
    const char * /* types */,
    lo_arg ** /* argv */,
    int /* argc */,
    lo_message /* msg */,
    void * user_data
)
{
    nsmbase * pnsmc = static_cast<nsmbase *>(user_data);
    if (is_nullptr(pnsmc))
        return -1;

    pnsmc->show(path);                  /* a virtual function   */
    return 0;
}

/**
 *  This function could also be called osc_hide_gui().  See the nsm-proxy
 *  code.
 */

static int
osc_nsm_hide
(
    const char * path,
    const char * /* types */,
    lo_arg ** /* argv */,
    int /* argc */,
    lo_message /* msg */,
    void * user_data
)
{
    nsmbase * pnsmc = static_cast<nsmbase *>(user_data);
    if (pnsmc == NULL)
        return -1;

    pnsmc->hide(path);
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
    nsmbase * pnsmc = static_cast<nsmbase *>(user_data);
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

nsmbase::nsmbase
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
                add_client_method(nsm::tag::error, osc_nsm_error);
                add_client_method(nsm::tag::reply, osc_nsm_reply);
                add_client_method(nsm::tag::open, osc_nsm_open);
                add_client_method(nsm::tag::save, osc_nsm_save);
                add_client_method(nsm::tag::loaded, osc_nsm_session_loaded);
                add_client_method(nsm::tag::label, osc_nsm_label);
                add_client_method(nsm::tag::show, osc_nsm_show);
                add_client_method(nsm::tag::hide, osc_nsm_hide);
                add_client_method(nsm::tag::null, osc_nsm_broadcast);
                lo_server_thread_start(m_lo_thread);
            }
            else
                pathprint("NSM:", "bad server\n");
        }
        else
            pathprint("NSM:", "bad server thread\n");
    }
    else
        pathprint("NSM:", "bad server address\n");
}

/**
 *
 */

nsmbase::~nsmbase ()
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
nsmbase::lo_is_valid () const
{
    if (is_nullptr_2(m_lo_address, m_lo_server))
    {
        m_active = false;
        pathprint("NSM error:", "Invalid OSC address or server");
    }
    return m_active;
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

bool
nsmbase::announce
(
    const std::string & appname,
    const std::string & capabilities
)
{
    bool result = lo_is_valid();
    if (result)
    {
        const char * filename = executable_full_path().c_str();
        const char * app = appname.c_str();
        const char * caps = capabilities.c_str();
        int pid = int(getpid());
        std::string message;
        std::string pattern;
        result = nsm::server_msg(nsm::tag::announce, message, pattern);
        if (result)
        {
            lo_send_from                /* "/nsm/server/announce" "sssiii"  */
            (
                m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
                message.c_str(), pattern.c_str(), app, caps, filename,
                NSM_API_VERSION_MAJOR, NSM_API_VERSION_MINOR, pid
            );
        }

        std::string text = msgsnprintf
        (
            "%s [%s] app '%s'; capabilities '%s'",
            message.c_str(), pattern.c_str(), app, caps
        );
        pathprint("NSM announce:", text);
    }
    return result;
}

/*
 * Session client methods.
 */

void
nsmbase::dirty (bool isdirty)
{
    if (lo_is_valid())
    {
        const char * path = nsm::dirty_msg(isdirty).c_str();
        send(path, "");
        m_dirty = isdirty;
    }
}

void
nsmbase::update_dirty_count (bool updatedirt)
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
nsmbase::visible (bool isvisible)
{
    if (lo_is_valid())
    {
        const char * path = nsm::visible_msg(isvisible).c_str();
        send(path, "");
        // m_visible = isvisible;   IN CLIENT OVERRIDE
    }
}

/**
 *  Call this function to send a progress indication to the session manager.
 *
 * \param percent
 *      The indication of progress, ranging from 0.0 to 100.0.
 */

bool
nsmbase::progress (float percent)
{
    bool result = lo_is_valid();
    if (result)
    {
        std::string message;
        std::string pattern;
        bool ok = nsm::client_msg(nsm::tag::progress, message, pattern);
        if (ok)
        {
            lo_send_from                    /* "/nsm/client/progress" "f"   */
            (
                m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
                message.c_str(), pattern.c_str(), percent
            );

            std::string text = msgsnprintf
            (
                "%s [%s] %f", message.c_str(), pattern.c_str(), percent
            );
            pathprint("OSC message sent:", text);
        }
    }
    return result;
}

/**
 *  Send out the indication of dirtiness status.
 */

bool
nsmbase::is_dirty ()
{
    bool result = lo_is_valid();
    if (result)
    {
        std::string message;
        std::string pattern;
        bool ok = nsm::client_msg(nsm::tag::dirty, message, pattern);
        if (ok)
        {
            lo_send_from                    /* "/nsm/client/is_dirty" ""    */
            (
                m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
                message.c_str(), pattern.c_str()
            );
#if defined SEQ66_PLATFORM_DEBUG
            printf("is_dirty()\n");
#endif
        }
    }
    return result;
}

/**
 *  Send out the indication of cleanliness status.
 */

bool
nsmbase::is_clean ()
{
    bool result = lo_is_valid();
    if (result)
    {
        std::string message;
        std::string pattern;
        bool ok = nsm::client_msg(nsm::tag::clean, message, pattern);
        if (ok)
            send(message, pattern);
    }
    return result;
}

/**
 *
 */

bool
nsmbase::message (int priority, const std::string & mesg)
{
    bool result = lo_is_valid();
    if (result)
    {
        std::string message;
        std::string pattern;
        bool ok = nsm::client_msg(nsm::tag::message, message, pattern);
        if (ok)
        {
            lo_send_from                    /* "/nsm/client/message" "is"   */
            (
                m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
                message.c_str(), pattern.c_str(),
                priority, mesg.c_str()
            );
            printf("message(%d, %s\n", priority, mesg.c_str());
            std::string text = msgsnprintf
            (
                "%s [%s] priority %d msg '%s'",
                message.c_str(), pattern.c_str(), priority, mesg.c_str()
            );
            pathprint("NSM message:", text);
        }
    }
    return result;
}

/*
 * Session client reply methods.
 */

bool
nsmbase::open_reply (reply replycode)
{
    return nsm_reply("/nsm/client/open", replycode);
}

bool
nsmbase::save_reply (reply replycode)
{
    return nsm_reply("/nsm/client/save", replycode);
}

std::string
nsmbase::nsm_reply_message (reply replycode)
{
    std::string result;
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

bool
nsmbase::nsm_reply (const std::string & path, reply replycode)
{
    bool result = lo_is_valid();
    if (result)
    {
        std::string reply_mesg = nsm_reply_message(replycode);
        std::string pattern;
        std::string message;
        if (replycode == reply::ok)
        {
            if (client_msg(nsm::tag::reply, message, pattern))
            {
                lo_send_from
                (
                    m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
                    message.c_str(), pattern.c_str(),
                    path.c_str(), reply_mesg.c_str()
                );
            }

            std::string text = msgsnprintf
            (
                "%s [%s] msg %s reply '%s'",
                message.c_str(), pattern.c_str(),
                path.c_str(), reply_mesg.c_str()
            );
            pathprint("NSM:", text);
        }
        else
        {
            if (client_msg(nsm::tag::error, message, pattern))
            {
                lo_send_from
                (
                    m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
                    message.c_str(), pattern.c_str(), path.c_str(),
                    int(replycode), reply_mesg.c_str()
                );
            }

            std::string text = msgsnprintf
            (
                "%s [%s] code %d msg %s reply '%s'",
                message.c_str(), pattern.c_str(), int(replycode),
                path.c_str(), reply_mesg.c_str()
            );
            pathprint("NSM:", text);
        }
    }
    return result;
}

/*
 * Server announce error.
 */

void
nsmbase::announce_error (const std::string & errmesg)
{
    m_active = false;
    m_manager.clear();
    m_capabilities.clear();
    m_path_name.clear();
    m_display_name.clear();
    m_client_id.clear();
    // emit active(false);
    pathprint("NSM Failed to register:", errmesg);
}

/*
 * Server announce reply.
 */

void
nsmbase::announce_reply
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

    std::string temp = msgsnprintf
    (
        "Msg: '%s'; Mgr: '%s'; Caps: '%s'",
        mesg.c_str(), manager.c_str(), capabilities.c_str()
    );
    pathprint("NSM Successfully registered:", temp);
}

/**
 *
 */

void
nsmbase::nsm_debug (const std::string & tag)
{
    if (tag.empty())
    {
        std::string text = msgsnprintf
        (
            "Logged path '%s', display name '%s', client ID '%s'",
            m_path_name.c_str(), m_display_name.c_str(), m_client_id.c_str()
        );
        pathprint("NSM debug:", text);
    }
    else
    {
        pathprint("NSM debug:", tag);
    }
}

/**
 *  Client open callback. Compare to the "open" code in nsm-proxy.
 */

void
nsmbase::open
(
    const std::string & pathname,
    const std::string & displayname,
    const std::string & clientid
)
{
    m_path_name = pathname;
    m_display_name = displayname;
    m_client_id = clientid;
    // emit open();
    std::string text = msgsnprintf
    (
        "%s [%s] ID %s", pathname.c_str(), displayname.c_str(), clientid.c_str()
    );
    pathprint("NSM client open:", text);
}

/*
 * Client save callback.
 */

void
nsmbase::save ()
{
    nsm_debug("save");

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
nsmbase::loaded ()
{
    nsm_debug("loaded");
    // emit loaded();
}

void
nsmbase::label (const std::string & label)
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
nsmbase::show (const std::string & path)
{
    nsm_debug("show");
    // emit show();
    send_from_client(nsm::tag::reply, path, "OK");
    //  lo_send_from
    //  (
    //      m_lo_address, losrv, LO_TT_IMMEDIATE, "/reply", "ss", path, "OK"
    //  );
}

/*
 * Client hide optional GUI.
 */

void
nsmbase::hide (const std::string & path)
{
    nsm_debug("hide");
    send_from_client(nsm::tag::hidden, path, "OK");
    send_from_client(nsm::tag::reply, path, "OK");  // ss
    // emit hide();
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
    //
    // From nsm-proxy:
    //
    // if (gui_pid)
    //  kill(gui_pid, SIGTERM);
}

/**
 *  The lo_message type is a typedef of a void pointer, as are most of the
 *  "lo_" types.
 *
 *  int lo_send_message_from
 *  (
 *      lo_address target,
 *      lo_server serv,
 *      const char * path,
 *      lo_message msg
 *  )
 */

void
nsmbase::broadcast (const std::string & path, lo_message msg)
{
    nsm_debug("broadcast");
    if (lo_is_valid())
    {
        std::string message;
        std::string pattern;
        bool ok = nsm::server_msg(nsm::tag::broadcast, message, pattern);
        pattern = std::string(static_cast<char *>(msg));
        if (ok)
        {
            send(message, pattern);
//          lo_send_message_from            /* "/nsm/server/broadcat" msg   */
//          (
//              m_lo_address, m_lo_server,
//              message.c_str(), msg                // pattern.c_str()
//          );
        }
        pathprint("NSM broadcast:", path);
    }
    // emit broadcast();
}

/*
 * Prospective caller helpers a la qtractorMainForm.
 */

/**
 *  After calling this function and checking the return value,
 *  the caller should close out any "open" items and set up a new "session".
 *
 *  After that, call open_reply() with the boolean result of the new session
 *  setup. TODO.
 */

bool
nsmbase::open_session ()
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
nsmbase::close_session ()
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
nsmbase::save_session ()
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
 *  lo_method                           // void * to a new server method
 *  lo_server_add_method
 *  (
 *      lo_server st,                   // void * created by lo_server_new()
 *      const char * path,
 *      const char * typespec,
 *      lo_method_handler h,            // see below
 *      const void * userdata
 *  )
 */

/**
 *  lo_method                           // void * to a new server method
 *  lo_server_thread_add_method
 *  (
 *      lo_server_thread st,            // void * made by lo_server_thread_new()
 *      const char * path,              // OSC path string or NULL
 *      const char * typespec,          // OSC parameters format
 *      lo_method_handler h,            // see below
 *      const void * userdata           // data for the method handler
 *  )
 *
 *  int (* lo_method_handler)
 *  (
 *      const char * path,
 *      const char * typespec,
 *      lo_arg ** argv, int argc,       // a union of many types
 *      lo_message msg,                 // void * created by lo_message_new()
 *      void * user_data
 *  )
 *  )
 */

void
nsmbase::add_client_method (nsm::tag t, lo_method_handler h)
{
    std::string message;
    std::string pattern;
    if (client_msg(t, message, pattern))
    {
        if (t == nsm::tag::null)
        {
            const char * nul = NULL;
            (void) lo_server_thread_add_method(m_lo_thread, nul, nul, h, this);
            pathprint("OSC:", "broadcast method added");
        }
        else
        {
            std::string text = msgsnprintf
            (
                "method for %s [%s] added", message.c_str(), pattern.c_str()
            );
            pathprint("OSC:", text);
            (void) lo_server_thread_add_method
            (
                m_lo_thread, message.c_str(), pattern.c_str(), h, this
            );
        }
    }
}

void
nsmbase::send
(
    const std::string & message,
    const std::string & pattern
)
{
    lo_send_from                        /* e.g. "/nsm/client/is_clean" ""   */
    (
        m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
        message.c_str(), pattern.c_str()
    );

    std::string text = msgsnprintf("%s [%s]", message.c_str(), pattern.c_str());
    pathprint("OSC message sent:", text);
}

void
nsmbase::send_from_client (nsm::tag t)
{
    std::string message;
    std::string pattern;
    bool ok = nsm::client_msg(t, message, pattern);
    if (ok)
        send(message, pattern);
}

void
nsmbase::send_from_client
(
    nsm::tag t,
    const std::string & s1,
    const std::string & s2,
    const std::string & s3
)
{
    std::string message;
    std::string pattern;
    bool ok = nsm::client_msg(t, message, pattern);
    if (ok)
    {
        if (s3.empty())
        {
            lo_send_from                /* e.g. "/nsm/client/is_clean" ""   */
            (
                m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
                message.c_str(), pattern.c_str(),
                s1.c_str(), s2.c_str()
            );
        }
        else
        {
            lo_send_from
            (
                m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
                message.c_str(), pattern.c_str(),
                s1.c_str(), s2.c_str(), s3.c_str()
            );
        }
        pathprint("OSC message sent:", message);
    }
}

/**
 *  See if there is NSM "present" on the host computer.
 */

std::string
get_nsm_url ()
{
    return get_session_url(nsm::url());
}

}           // namespace seq66

/*
 * nsmbase.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


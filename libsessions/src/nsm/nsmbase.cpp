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
 * \updates       2023-01-01
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
 *  Warning:
 *
 *      If the non-session-manager user-interface comes up completely
 *      disabled, this means that OSC cannot find the host.  If this occurs,
 *      make sure that the real host name is appended to the "localhost" entry
 *      in the /etc/hosts file for at least one of the loopback interfaces:
 *
 *          -   127.0.0.1 localhost myhostname
 *          -   127.0.1.1 myhostname.domainname
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

#undef  SHOW_CLIENT_DATA_TYPE           /* for development purposes only    */

#if defined SHOW_CLIENT_DATA_TYPE
#include <iostream>                     /* std::cout                        */
#endif

#include <cstring>                      /* std::strlen()                    */
#include <sys/types.h>                  /* provides the pid_t typedef       */
#include <unistd.h>                     /* C getpid()                       */

#include "cfg/settings.hpp"             /* seq66::usr() global object       */
#include "util/basic_macros.hpp"        /* not_nullptr(), warnprint(), etc. */
#include "nsm/nsmbase.hpp"              /* seq66::nsmbase class             */
#include "nsm/nsmmessagesex.hpp"        /* seq66::nsm new message functions */

#define NSM_API_VERSION_MAJOR   1
#define NSM_API_VERSION_MINOR   0

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  A handler for the /reply message.
 */

static int
osc_nsm_reply
(
    const char * path,
    const char * types,
    lo_arg ** /* argv */,
    int /* argc */,
    lo_message /* msg */,
    void * user_data
)
{
    nsmbase * pnsmc = static_cast<nsmbase *>(user_data);
    if (is_nullptr(pnsmc))
        return -1;

    nsm::incoming_msg("Basic Reply", path, types);
    pnsmc->nsm_reply(path, types);
    return 0;
}

/**
 *  This set of functions supplies some OSC (liblo) callback methods for the
 *  nsmclient to use.
 *
 *  Note that lo_arg (/usr/include/lo/lo_osc_types.h) is a union that covers
 *  various integer types, floats, doubles, chars of various kinds, 4-byte
 *  MIDI packets, lo_timetag, and a "blob" structure (size + data).
 *
 *  The lo_message type is a low-level object messages passed via OSC.  It is
 *  a void pointer.
 *
 *  Finally, note the "sis" check for types. The type characters are defined
 *  in the OSC header lo_osc_types.h, and there are values for float ('f'),
 *  string ('s'), 32-bit integers ('i'), and more.  These character tag
 *  messages and specify the arguments of lo_send().
 */

static int
osc_nsm_error
(
    const char * path,
    const char * types,
    lo_arg ** argv,
    int /* argc */,
    lo_message /* msg */,
    void * user_data
)
{
    nsmbase * pnsmc = static_cast<nsmbase *>(user_data);
    if (is_nullptr(pnsmc))
        return -1;

    nsm::incoming_msg("Error", path, types);
    pnsmc->error(int(argv[1]->i), &argv[2]->s);
    return 0;
}

/*
 * osc_stop_signal() : stop_signal() virtual function
 * osc_start() : start() virtual function
 * osc_kill() : kill() virtual function
 * osc_update() : update() virtual function
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
    m_lo_address        (nullptr),
    m_lo_server_thread  (nullptr),
    m_lo_server         (nullptr),
    m_active            (false),        /* an atomic boolean value          */
    m_dirty             (false),
    m_dirty_count       (0),
    m_manager           (),
    m_capabilities      (),
    m_path_name         (),
    m_display_name      (),
    m_client_id         (),
    m_nsm_file          (nsmfile),
    m_nsm_ext           (nsmext),
    m_nsm_url           (nsmurl)
{
    // No code needed
}

/**
 *  Stops and frees the server thread, and frees the OSC address.  Note that
 *  the server itself does not need to be freed.  (We will see what valgrind
 *  says about that later, if possible.)
 */

nsmbase::~nsmbase ()
{
    stop_thread();
    if (m_lo_address)
        lo_address_free(m_lo_address);
}

void
nsmbase::start_thread ()
{
    if (not_nullptr(m_lo_server_thread))
    {
        int rcode = lo_server_thread_start(m_lo_server_thread);
        if (rcode == 0)                                     /* successful?  */
        {
            if (rc().verbose())
                session_message("OSC server thread started");
        }
        else
            error_message("OSC server thread start failed");
    }
}

void
nsmbase::stop_thread ()
{
    if (not_nullptr(m_lo_server_thread))
    {
        lo_server_thread_free(m_lo_server_thread);
        m_lo_server_thread = nullptr;
    }
    else
    {
        if (not_nullptr(m_lo_server))
        {
            lo_server_free(m_lo_server);
            m_lo_server = nullptr;
        }
    }
}

/**
 *  Gets the server address from NSM_URL, creates the server and server
 *  thread, and adds the basic /error and /reply handlers.  (The rest are
 *  added by the nsmclient class.)
 */

bool
nsmbase::initialize ()
{
    m_lo_address = lo_address_new_from_url(nsm_url().c_str());

    bool result = not_nullptr(m_lo_address);
    if (result)
    {
        const int proto = lo_address_get_protocol(m_lo_address);
        if (rc().verbose())
        {
            std::string ps = "Unknown";
            switch (proto)
            {
                case LO_UDP:    ps = "UDP";     break;
                case LO_TCP:    ps = "TCP";     break;
                case LO_UNIX:   ps = "UNIX";    break;
            }
            ps += " OSC protocol";
            session_message(ps);
        }
        m_lo_server_thread = lo_server_thread_new_with_proto(NULL, proto, NULL);
        result = not_nullptr(m_lo_server_thread);
        if (result)
        {
            m_lo_server = lo_server_thread_get_server(m_lo_server_thread);
            result = not_nullptr(m_lo_server);
            if (result)
            {
                add_client_method(nsm::tag::error,   osc_nsm_error);
                add_client_method(nsm::tag::reply,   osc_nsm_reply);

                /*
                 * See nsmclient.
                 * add_client_method(nsm::tag::replyex, osc_nsm_announce_reply);
                 * add_client_method(nsm::tag::hide,    osc_nsm_hide);
                 * add_client_method(nsm::tag::label,   osc_nsm_label);
                 * add_client_method(nsm::tag::loaded,  osc_nsm_session_loaded);
                 * add_client_method(nsm::tag::null,    osc_nsm_broadcast);
                 * add_client_method(nsm::tag::open,    osc_nsm_open);
                 * add_client_method(nsm::tag::save,    osc_nsm_save);
                 * add_client_method(nsm::tag::show,    osc_nsm_show);
                 *
                 * The most-derived class should call this, we think.
                 *  lo_server_thread_start(m_lo_server_thread);
                 */
            }
            else
                error_message("OSC bad server");
        }
        else
            error_message("OSC bad server thread");
    }
    else
        error_message("OSC bad server address");

    return result;
}

/**
 *  Checks to be sure that the server and the address are usable (i.e. not
 *  null).
 */

bool
nsmbase::lo_is_valid () const
{
    bool result = not_nullptr_2(m_lo_address, m_lo_server);
    if (! result)
        error_message("Null OSC address or server");

    return result;
}

/**
 *  Wait for the next message.  Useful after sending the /nsm/server/announce
 *  message.  We added a brief sleep delay because sometimes we seem to miss a
 *  message at startup.
 *
 *  Note that lo_server_wait() waits for the given timeout, then returns 1 if
 *  a message is waiting.
 *
 * \param timeoutms
 *      Indicates how long to wait for a server message, in milliseconds.
 *      Defaults to 100 ms.
 *
 * \return
 *      Returns true if a message was received within 2 seconds, roughly.
 */

bool
nsmbase::msg_check (int timeoutms)
{
    bool result = false;
    if (timeoutms > 0)
    {
        /*
         * This cause issues when NSM responds quickly: microsleep(100);
         */

        if (lo_server_wait(m_lo_server, timeoutms))
        {
            result = true;
            if (rc().verbose())
                session_message("NSM waiting for reply...");

            while (lo_server_recv_noblock(m_lo_server, 0))
            {
                /* do nothing, handle the message(s) */
            }
        }
        if (! result)
            error_message("NSM no reply!");
    }
    return result;
}

/*
 * Session client methods.
 */

/**
 *  Sends one of these messages:
 *
 *      -   /nsm/client/is_clean
 *      -   /nsm/client/is_dirty message
 *
 *  It sets the m_dirty flag accordingly.
 */

void
nsmbase::dirty (bool isdirty)
{
    if (lo_is_valid())
    {
        const char * path = nsm::dirty_msg(isdirty).c_str();
        (void) send(path, "");
        m_dirty = isdirty;
    }
}

/**
 *  Optionally increments, or clears, the dirty-count.  If active, then the
 *  dirty-count is checked in order to set the dirty-flag.
 */

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

/**
 *  Call this function to send a progress indication to the session manager.
 *  The message looks like this:
 *
 *      /nsm/client/progress f:percent
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
            nsm::outgoing_msg(message, pattern, std::to_string(percent));
        }
    }
    return result;
}

/**
 *  Send out the indication of dirtiness status:
 *
 *      /nsm/client/is_dirty
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
        }
    }
    return result;
}

/**
 *  Send out the indication of cleanliness status:
 *
 *      /nsm/client/is_clean
 */

bool
nsmbase::is_clean ()
{
    bool result = lo_is_valid();
    if (result)
    {
        std::string message;
        std::string pattern;
        result = nsm::client_msg(nsm::tag::clean, message, pattern);
        if (result)
            result = send(message, pattern);
    }
    return result;
}

/**
 *  Sends a message:
 *
 *      /nsm/client/message i:priority s:message
 *
 *  where the priority ranges from 0 (least important) to 3 (most important).
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
            std::string text = "priority " + std::to_string(priority) +
                "; msg '" + mesg + "'";

            nsm::outgoing_msg(message, pattern, text);
        }
    }
    return result;
}

/*
 * Session client reply methods.  If the reply code is not error::ok, then the
 * reply will be an error reply.
 */

bool
nsmbase::open_reply (nsm::error errorcode, const std::string & msg)
{
    return send_nsm_reply("/nsm/client/open", errorcode, msg);
}

bool
nsmbase::save_reply (nsm::error errorcode, const std::string & msg)
{
    return send_nsm_reply("/nsm/client/save", errorcode, msg);
}

/**
 *  Sends a reply or error message, useful for the following message:
 *
 *      -   /nsm/client/open
 *      -   /nsm/client/save
 *
 *  The message sent is one of the following where "path" is either of the
 *  above messages, in double-quotes:
 *
 *      -   /reply "path" s:message
 *      -   /error "path" i:errorcode s:message
 *
 *  The "path" sent depends on the reply code provided.
 */

bool
nsmbase::send_nsm_reply
(
    const std::string & path,
    nsm::error errorcode,
    const std::string & msg
)
{
    bool result = lo_is_valid();
    if (result)
    {
        int rc = (-1);
        std::string pattern;
        std::string message;
        std::string replytype;
        std::string replymsg = reply_string(errorcode);
        replymsg += ": ";
        replymsg += msg;
        if (errorcode == nsm::error::ok)
        {
            if (client_msg(nsm::tag::reply, message, pattern))
            {
                rc = lo_send_from
                (
                    m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
                    message.c_str(), pattern.c_str(),
                    path.c_str(), replymsg.c_str()
                );
            }
            replytype = "reply";
        }
        else
        {
            if (client_msg(nsm::tag::error, message, pattern))
            {
                rc = lo_send_from
                (
                    m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
                    message.c_str(), pattern.c_str(), path.c_str(),
                    static_cast<int>(errorcode), replymsg.c_str()
                );
            }
            replytype = "error";
        }
        result = rc != (-1);

        std::string text = path + " " + replytype + " " + replymsg;
        if (! result)
            text += "; FAILED";

        nsm::outgoing_msg(message, pattern, text);
    }
    return result;
}

/**
 *  Sends the following message:
 *
 *      /nsm/server/announce s:appname s:capabilities s:exename
 *          i:apimajor i:apiminor i:pid
 *
 *  See nsmclient::announce() for more discussion.
 */

bool
nsmbase::send_announcement
(
    const std::string & appname,        /* actually a package name, "seq66" */
    const std::string & exename,        /* comes from argv[0]               */
    const std::string & capabilities    /* e.g. ":switch:dirty:"            */
)
{
    std::string message;
    std::string pattern;
    bool result = lo_is_valid();
    if (result)
        result = nsm::server_msg(nsm::tag::announce, message, pattern);

    if (result)
    {
        const char * packagename = appname.c_str();
        const char * app = exename.c_str();
        const char * caps = capabilities.c_str();
        int pid = int(getpid());
        int rc = lo_send_from           /* "/nsm/server/announce" "sssiii"  */
        (
            m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
            message.c_str(), pattern.c_str(), packagename, caps, app,
            NSM_API_VERSION_MAJOR, NSM_API_VERSION_MINOR, pid
        );
        result = rc != (-1);

        std::string text = "sent package " + appname +
            "; app " + exename +
            "; capabilities " + capabilities;

        if (! result)
            text += ", but it FAILED";

        nsm::outgoing_msg(message, pattern, text);
    }
    return result;
}

/*
 * Generic server reply.  Not sure we ever get this one.
 */

void
nsmbase:: nsm_reply (const std::string & message, const std::string & pattern)
{
    nsm::outgoing_msg(message, pattern, "Server Reply");
}

/*
 *  Handles the base server error:
 *
 *      /error i:errcode s:errormessage
 */

void
nsmbase::error (int errcode, const std::string & errmesg)
{
    is_active(false);
    m_manager.clear();
    m_capabilities.clear();
    m_path_name.clear();
    m_display_name.clear();
    m_client_id.clear();
    // emit active(false);

    std::string ecm = reply_string(static_cast<nsm::error>(errcode));
    nsm::incoming_msg("Error Values", errmesg, ecm, true);
}

void
nsmbase::nsm_debug (const std::string & tag)
{
    if (tag.empty())
        nsm::outgoing_msg(m_path_name, m_client_id, m_display_name);
    else
        nsm::outgoing_msg(m_path_name, m_client_id, tag);
}

/*
 * Prospective caller helpers a la qtractorMainForm.
 */

/**
 *  After calling this function and checking the return value, the caller
 *  should close out any "open" items and set up a new "session".  After that,
 *  we call open_reply() with the boolean result of the new session setup.
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
 *  Send close message, quit, abort?
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
 *  The caller should call this function, then check the result before saving
 *  the "session".
 *
 *  We probably need to return a reply-code instead of a boolean.
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
         * Done by caller: m_nsm_file.clear() ???
         */
    }
    return result;
}

/*
 *  lo_method                           // void * to a new server method
 *  lo_server_add_method
 *  (
 *      lo_server st,                   // void * created by lo_server_new()
 *      const char * path,
 *      const char * typespec,
 *      lo_method_handler h,            // see below
 *      const void * userdata
 *  )
 *
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
 */

/**
 *  Adds an OSC method handler function to both the server and the server
 *  thread.  The server addition is to be able to send to the OSC server, and
 *  the server thread addition is to be able to receive replies from the
 *  OSC server.  Weird.
 */

void
nsmbase::add_client_method (nsm::tag t, lo_method_handler h)
{
    std::string message;
    std::string pattern;

#if defined SHOW_CLIENT_DATA_TYPE
    const std::type_info & ti = typeid(this);
    std::cout << "Client type = " << ti.name();
#endif

    if (client_msg(t, message, pattern))
    {
        if (t == nsm::tag::null)
        {
            const char * nul = NULL;
            (void) lo_server_thread_add_method
            (
                m_lo_server_thread, nul, nul, h, this
            );
            nsm::outgoing_msg("OSC", "", "Broadcast method added");
        }
        else
        {
            const char * m = message.c_str();
            const char * p = pattern.c_str();
            (void) lo_server_thread_add_method
            (
                m_lo_server_thread, m, p, h, this
            );
            nsm::outgoing_msg(message, pattern, "Client method added");
        }
    }
}

void
nsmbase::add_server_method (nsm::tag t, lo_method_handler h)
{
    std::string message;
    std::string pattern;
    if (server_msg(t, message, pattern))
    {
        const char * m = message.c_str();
        const char * p = pattern.c_str();
        (void) lo_server_add_method(m_lo_server_thread, m, p, h, this);
        nsm::outgoing_msg(message, pattern, "Server method added");
    }
}

bool
nsmbase::send
(
    const std::string & message,
    const std::string & pattern
)
{
    int rcode = lo_send_from            /* e.g. "/nsm/client/is_clean" ""   */
    (
        m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
        message.c_str(), pattern.c_str()
    );
    bool result = rcode != (-1);
    if (result)
        nsm::outgoing_msg(message, pattern, "Sent");
    else
        nsm::outgoing_msg(message, pattern, "Send FAILURE");

    return result;
}

bool
nsmbase::send_from_client (nsm::tag t)
{
    std::string message;
    std::string pattern;
    bool result = nsm::client_msg(t, message, pattern);
    if (result)
        result = send(message, pattern);

    return result;
}

bool
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
    bool result = nsm::client_msg(t, message, pattern);
    if (result)
    {
        int rcode;
        if (s3.empty())
        {
            rcode = lo_send_from        /* e.g. "/nsm/client/is_clean" ""   */
            (
                m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
                message.c_str(), pattern.c_str(),
                s1.c_str(), s2.c_str()
            );
        }
        else
        {
            rcode = lo_send_from
            (
                m_lo_address, m_lo_server, LO_TT_IMMEDIATE,
                message.c_str(), pattern.c_str(),
                s1.c_str(), s2.c_str(), s3.c_str()
            );
        }
        result = rcode != (-1);
        if (result)
        {
            if (rc().verbose())
            {
                std::string msg = "OSC message sent" + message;
                session_message(msg);
            }
        }
        else
        {
            std::string msg = "OSC message send FAILURE" + message;
            error_message(msg);
        }
    }
    return result;
}

/*
 * This namespace makes for easier-to-read code.
 */

namespace nsm
{

/**
 *  A free function to provide a string for a reply code in the nsm::error
 *  enumeration class.
 *
\verbatim
    ok:                 OK.
    general:            General error.  This is a very common error value.
    incompatible_api:   Incompatible API version.
    blacklisted:        The client has been blacklisted.
    launch_failed:      Launch failed.
    no_such_file:       No such file.
    no_session_open:    No session open.
    unsaved_changes:    Unsaved changes would be lost.
    not_now:            The operation cannot be completed at this time.
    bad_project:        An existing project file was found to be corrupt.
    create_failed:      Create failed. A new project could not be created.
    session_locked:     Session is locked.
    operation_pending:  An operation is pending.
    save_failed:        An non-existent (heh heh) error code.
\endverbatim
 *
 *  The NSM API documentation claims there is an "ERR_SAVE_FAILED" code,
 *  meaning "The project could not be saved", which needs to be sent as a
 *  response when appropriate.  However, this code does not exist.  Other
 *  codes are private to the nsmd.C module, and are exposed in our
 *  implementation of the API.
 */

std::string
reply_string (nsm::error errorcode)
{
    std::string result;
    switch (errorcode)
    {
    case nsm::error::ok:

        result = "Acknowledged";
        break;

    case nsm::error::general:

        result = "General error";
        break;

    case nsm::error::incompatible_api:

        result = "Incompatible API";
        break;

    case nsm::error::blacklisted:

        result = "Blacklisted";
        break;

    case nsm::error::launch_failed:

        result = "Launch failed";
        break;

    case nsm::error::no_such_file:

        result = "No such file";
        break;

    case nsm::error::no_session_open:

        result = "No session open";
        break;

    case nsm::error::unsaved_changes:

        result = "Unsaved changes";
        break;

    case nsm::error::not_now:

        result = "Not now";
        break;

    case nsm::error::bad_project:

        result = "Bad project";
        break;

    case nsm::error::create_failed:

        result = "Create failed";
        break;

    case nsm::error::session_locked:

        result = "Session locked";
        break;

    case nsm::error::operation_pending:

        result = "Operation Pending";
        break;

    case nsm::error::save_failed:

        result = "Save failed.";
        break;

    default:

        result = "Unknown reply";
        break;
    }
    return result;
}

/**
 *  See if there is session-manager "present" on the host computer.
 */

static std::string
get_session_url (const std::string & env_value)
{
    std::string result;
#if defined _GNU_SOURCE
    char * url = secure_getenv(env_value.c_str());
#else
    char * url = std::getenv(env_value.c_str());
#endif
    if (not_nullptr(url) && strlen(url) > 0)
        result = std::string(url);

    return result;
}

/**
 *  See if there is NSM "present" on the host computer.  A static value is
 *  included that, if not empty, will be used, for troubleshooting and
 *  testing.  To use it, check out the value of NSM_URL, put it here, and
 *  rebuild.  This feature is meant to make it easier to debug, but will it
 *  work?
 */

std::string
get_url ()
{
    static std::string s_debug_url = "";
    std::string url = nsm::url();
    std::string result = s_debug_url.empty() ?
        get_session_url(url) : s_debug_url ;

    bool active = ! result.empty();
    usr().in_nsm_session(active);
    if (rc().verbose())
    {
        if (active)
        {
            std::string msg = "NSM URL " + result;
            session_message(msg);
        }
    }
    return result;
}

void
incoming_msg
(
    const std::string & cbname,
    const std::string & message,
    const std::string & pattern,
    bool iserror
)
{
    if (rc().investigate() || iserror)
    {
        std::string text = msgsnprintf
        (
            "%s<--NSM: %s [%s]",
            cbname.c_str(), message.c_str(), pattern.c_str()
        );
        (void) session_message(text);
    }
}

void
outgoing_msg
(
    const std::string & message,
    const std::string & pattern,
    const std::string & data
)
{
    if (rc().verbose())
    {
        std::string text = msgsnprintf
        (
            "%s-->[%s] %s",
            message.c_str(), pattern.c_str(), data.c_str()
        );
        session_message(text);
    }
}

tokenization
convert_lo_args (const std::string & pattern, int argc, lo_arg ** argv)
{
    tokenization result;
    if (argc > 0)
    {
        for (int i = 0; i < argc; ++i)
        {
            std::string temp;
            char patc = pattern[i];
            switch (patc)
            {
                case 's':

                    temp = argv[i]->s;      /* &argv[i] */
                    break;

                case 'i':

                    temp = std::to_string(argv[i]->i);
                    break;

                case 'f':

                    temp = std::to_string(argv[i]->f);
                    break;

                default:

                    temp = "unhandled format type: ";
                    temp += patc;
                    break;
            }
            result.push_back(temp);
        }
    }
    return result;
}

}           // namespace nsm

}           // namespace seq66

/*
 * nsmbase.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


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
 * \updates       2021-07-12
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
 *  Session Manager Startup:
 *
 *      To start the Non Session Manager and the GUI:
 *
 *      $ non-session-manager [ -- --session-root path ] &
 *
 *      The default path is "$HOME/NSM Sessions".  Inside this directory there
 *      will ultimately exist one directory per session, each with the name of
 *      the session as given by the user.  Inside this session directory is a
 *      file called "session.nsm".  Inside this file is a list of session
 *      clients in the following format:
 *
 *          appname:exename:nXXXX       (example:  qseq66:qseq66:nYUSM)
 *
 *      where XXXX is a set of four random uppercase ASCII letters.  The
 *      string "nXXXX" is used to look up clients.
 *
 *      At startup, the environment variable NSM_URL is added to the
 *      environment.  It's format is described below. Commands are handled as
 *      per the "Commands handling by the server" section at the top of the
 *      nsmmessageex.cpp module. A new session (e.g. "MySession") can be added
 *      using the "New" button.  It will be stored in "$HOME / New Sessions /
 *      MySession / session.nsm", which starts out empty.
 *
 *      Once running, an executable can be added as a client.  However, the
 *      executable must be in the path.  The command "/nsm/server/add" will
 *      fail with the error "Absolute paths are not permitted. Clients must be
 *      in $PATH".  Once added properly, NSM spawns the executable, and the
 *      executable inherits the environment (i.e. NSM_URL).
 *
 *      After closing/saving the session, the session.nsm file contains
 *      only a line such as "qseq66:qseq66:nMTRJ", as noted above.
 *
 *      Once qseq66 is part of the session, clicking on the session name will
 *      launch qseq66.
 *
 *  Process:
 *
 *      -#  Find out if NSM_URL is defined in the host environment, and
 *          create the nsmclient object on the heap if so. The
 *          create_nsmclient() should be used; it returns a "unique pointer".
 *          (We may need to provide specific factory functions for Qt, Gtkmm,
 *          and command-line versions of the application.)
 *      -#  If NSM_URL is valid and reachable, call the nsmbase::announce()
 *          function.
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
 *          NSM_URL=osc.udp://mlsasus.mls:12325/    (on another laptop)
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
 *  Detecting NSM session actions:
 *
 *      In a Qt-based application, we can provide an extended NSM client that
 *      will respond to signals propagated by the "emit" operator. In a
 *      command-line application, we can set flags that are detected in a
 *      polling loop and cause actions to occur.  In Gtkmm applications, we
 *      can set callbacks to be executed.  It would be nice to use the same
 *      (callback?) system for all of them.
 *
 * Shutdown:
 *
 *      When an NSM client shuts down, NSM detects its PID and detects if the
 *      client aborted or stopped normally. A normal stopped occurs when the
 *      client receives a KILL or QUIT command.  If a QUIT command, the server
 *      sends:
 *
 *          "/nsm/gui/client/status" + Client-ID + "removed" status
 *
 *      Otherwise, it sends two messages:
 *
 *          "/nsm/gui/client/label" + Client-ID + optional error message
 *          "/nsm/gui/client/status" + Client-ID + "stopped"
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
 *      INVESTIGATE the NSM replacement, RaySession.
 */

#include "util/basic_macros.hpp"        /* not_nullptr() macro              */
#include "nsm/nsmclient.hpp"            /* seq66::nsmclient class           */
#include "nsm/nsmmessagesex.hpp"        /* seq66::nsm message functions     */
#include "sessions/smanager.hpp"        /* seq66::smanager virtuals         */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  The typical type signature of this callback is "ssss".
 */

static int
osc_nsm_announce_reply
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

    if (nsm::is_announce(&argv[0]->s))
    {
        nsm::incoming_msg("Announce Reply", path, types);
        pnsmc->announce_reply(&argv[1]->s, &argv[2]->s, &argv[3]->s);
    }
    else
        nsm::incoming_msg("Basic Reply", path, types);

    return 0;
}

static int
osc_nsm_open
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

    nsm::incoming_msg("Open", path, types);
    pnsmc->open(&argv[0]->s, &argv[1]->s, &argv[2]->s);
    return 0;
}

static int
osc_nsm_save
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

    nsm::incoming_msg("Save", path, types);
    pnsmc->save();                  /* a virtual function   */
    return 0;
}

static int
osc_nsm_session_loaded
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

    nsm::incoming_msg("Session Loaded", path, types);
    pnsmc->loaded();
    return 0;
}

static int
osc_nsm_label
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

    nsm::incoming_msg("Label", path, types);
    pnsmc->label(std::string(&argv[0]->s));         /* a virtual function */
    return 0;
}

/**
 *  This function could also be called osc_show_gui().  See the nsm-proxy code.
 */

static int
osc_nsm_show
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

    nsm::incoming_msg("Show", path, types);
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
    const char * types,
    lo_arg ** /* argv */,
    int /* argc */,
    lo_message /* msg */,
    void * user_data
)
{
    nsmbase * pnsmc = static_cast<nsmbase *>(user_data);
    if (pnsmc == NULL)
        return -1;

    nsm::incoming_msg("Hide", path, types);
    pnsmc->hide(path);
    return 0;
}

static int
osc_nsm_broadcast
(
    const char * path,
    const char * types,
    lo_arg ** argv,
    int argc,
    lo_message /*msg*/,
    void * user_data
)
{
    nsmbase * pnsmc = static_cast<nsmbase *>(user_data);
    if (pnsmc == NULL)
        return -1;

    std::vector<std::string> arguments = nsm::convert_lo_args(types, argc, argv);
    nsm::incoming_msg("Broadcast", path, types);
    pnsmc->broadcast(path, types, arguments);
    return 0;
}

/* ------------------------------------------------------------------------ */

/**
 *  Principal constructor.
 */

nsmclient::nsmclient
(
    smanager & sm,
    const std::string & nsmurl,
    const std::string & nsmfile,
    const std::string & nsmext
) :
    nsmbase             (nsmurl, nsmfile, nsmext),
    m_session_manager   (sm)
{
    // no code so far
}

nsmclient::~nsmclient ()
{
    // no code so far
}

bool
nsmclient::initialize ()
{
    bool result = nsmbase::initialize();
    if (result)
    {
        add_client_method(nsm::tag::replyex, osc_nsm_announce_reply);
        add_client_method(nsm::tag::open, osc_nsm_open);
        add_client_method(nsm::tag::save, osc_nsm_save);
        add_client_method(nsm::tag::loaded, osc_nsm_session_loaded);
        add_client_method(nsm::tag::label, osc_nsm_label);
        add_client_method(nsm::tag::show, osc_nsm_show);
        add_client_method(nsm::tag::hide, osc_nsm_hide);
        add_client_method(nsm::tag::null, osc_nsm_broadcast);
        start_thread();
    }
    return result;
}

/*
 *  Server announce reply handler for the following message:
 *
 *      /nsm/server/announce s:application_name s:capabilities
 *          s:executable_name i:api_ver sion_major i:api_version_minor i:pid
 *
 *  This message is sent by the server after the client sends its announcement
 *  [see the nsmclient :: announce() and nsmbase :: send_announcement()
 *  functions].
 *
 *  Here, we can indicate that the session is connected and active, and what
 *  session manager is in force.  However, we do not want to activate this
 *  client until the session settings are made in the open() function below.
 */

void
nsmclient::announce_reply
(
    const std::string & mesg,
    const std::string & mgr,
    const std::string & caps
)
{
    capabilities(caps);
    session_manager_name(mgr);
    nsm::incoming_msg("Announce Reply Values", mgr, caps + " " + mesg);
}

/**
 *  Client open callback for the following message:
 *
 *      /nsm/client/open s:path_to_instance_specific_project
 *              s:display_name s:client_id
 *
 *  Format examples:
 *
 *      -   Display name: "JackSession" (user's chosen session name in NSM)
 *      -   Client ID:    "seq66.nUKIE"
 *      -   Path:         "/home/user/NSM Sessions/JackSession/seq66.nUKIE"
 *
 *  Compare to the "open" code in nsm-proxy.  See nsmclient::announce() for
 *  more discussion. Note that this function is where we can be active and
 *  possible copy the application configuration to the session path.
 */

void
nsmclient::open
(
    const std::string & pathname,
    const std::string & displayname,
    const std::string & clientid
)
{
    session_manager_path(pathname);
    session_display_name(displayname);
    session_client_id(clientid);
    set_client_name(clientid);          /* set "seq66.nUKIE" as client ID   */
    nsm::incoming_msg("Open Values", pathname, clientid + "" + displayname);
    is_active(true);
}

/*
 *  Client save callback for the following message:
 *
 *      /nsm/client/save
 *
 *  Note that, though documented, code nsm :: reply :: save_failed does not
 *  exist, so we use nsm :: reply :: general for now.  See nsmbase ::
 *  save_reply.
 */

void
nsmclient::save ()
{
    if (save_session())
    {
        std::string msg;
        bool saved = m_session_manager.save_session(msg);
        nsm::error r = saved ? nsm::error::ok : nsm::error::general ;
        (void) save_reply(r, msg);
    }
}

/**
 *  Handles the following message from the server:
 *
 *      /nsm/client/session_is_loaded
 */

void
nsmclient::loaded ()
{
    nsm_debug("Session loaded message from server");
}

/**
 *  Handles the following message from the server:
 *
 *      /nsm/client/label s:label
 */

void
nsmclient::label (const std::string & label)
{
    std::string tag("Label from server: '");
    tag += label;
    tag += "'";
    nsm_debug(tag); // no code
}

/**
 *  Client show optional GUI.  The derived class must provide this
 *  functionality.  The message from the server is:
 *
 *      /nsm/client/show_optional_gui
 */

void
nsmclient::show (const std::string & path)
{
    nsm_debug("show");
    hidden(false);
    send_from_client(nsm::tag::reply, path, "OK");
}

/*
 * Client hide optional GUI.  The message from the server is:
 *
 *      /nsm/client/hide_optional_gui
 */

void
nsmclient::hide (const std::string & path)
{
    nsm_debug("hide");
    hidden(true);
    send_from_client(nsm::tag::reply, path, "OK");  // ss
}

void
nsmclient::send_visibility (bool isshown)
{
    nsm::tag status = isshown ? nsm::tag::shown : nsm::tag::hidden ;
    send_from_client(status);
}

/**
 *  Receives a broadcast and figures out what to do with it.  Sort of.  The
 *  broadcast message seems to have no path and no data types.  Keep an eye on
 *  this one.
 *
 *  The following function is for *sending* broadcasts, not yet implemented:
 *
 *      nsmbase::broadcast (const std::string & path, lo_message msg)
 */

void
nsmclient::broadcast
(
    const std::string & /*message*/,
    const std::string & /*pattern*/,
    const std::vector<std::string> & argv
)
{
    if (lo_is_valid())
    {
        int argc = int(argv.size());
        for (int i = 0; i < argc; ++i)
        {
            printf("   [%d] '%s'\n", i, argv[i].c_str());
        }
    }
}

/**
 *  Provides a client-announce function.
 *
 *  If NSM_URL is valid and reachable, call this function to send the
 *  following "sssiii" message to the provided address as soon as ready to
 *  respond to the /nsm/client/open event.  api_version_major and
 *  api_version_minor must be the two parts of the version number of the NSM
 *  API.  If registering JACK clients, application_name must be passed to
 *  jack_client_open.  capabilities is a string containing a list of the
 *  capabilities the client possesses, e.g.  ":dirty:switch:progress:".
 *  executable_name must be the executable name that launched the program (e.g
 *  argv[0]).
 *
 *  We wait on the reply from NSM, which is flagged by an atomic boolean in
 *  the open() function.
 *
\verbatim
    /nsm/server/announce s:application_name s:capabilities s:executable_name
         i:api_version_major i:api_version_minor i:pid
\endverbatim
 *
 * \param appname
 *      Provides the "nick-name" for the application.  The seq66_features.cpp
 *      function seq_client_name() is used to get this name, which starts out
 *      as "seq66".  The seq_client_name() is later modified in the open()
 *      callback function, and ends up like "seq66.nUKIE".
 *
 * \param exename
 *      Comes from argv[0].  For example, it has the value "qseq66" in the
 *      graphical application.
 *
 * \param capabilities
 *      Provides the main session features the application supports.
 */

bool
nsmclient::announce
(
    const std::string & appname,        /* actually a package name, "Seq66" */
    const std::string & exename,        /* comes from argv[0]               */
    const std::string & capabilities    /* e.g. ":switch:dirty:"            */
)
{
    bool result = send_announcement(appname, exename, capabilities);
    if (result)
    {
        int count = 12;
        while (! is_active())           /* this is an atomic boolean check  */
        {
            (void) msg_check(1000);
            if (--count == 0)
            {
                errprint("Timed out waiting for NSM");
                result = false;
                break;
            }
        }
    }
    return result;
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
    bool result = nsmbase::open_session();
    if (result)
    {
        // what else?
    }
    return result;
}

/*
 *  Virtual functions to pass status to the smanager-derived class.
 */

void
nsmclient::session_manager_name (const std::string & mgrname)
{
    manager(mgrname);
    m_session_manager.session_manager_name(mgrname);
}

void
nsmclient::session_manager_path (const std::string & pathname)
{
    path_name(pathname);
    m_session_manager.session_manager_path(pathname);
}

void
nsmclient::session_display_name (const std::string & dispname)
{
    display_name(dispname);
    m_session_manager.session_display_name(dispname);
}

void
nsmclient::session_client_id (const std::string & clid)
{
    client_id(clid);
    m_session_manager.session_client_id(clid);
}

/**
 *  Provides a factory function to create an nsmclient, and then to call its
 *  virtual initialization function (so that we don't have to call it in the
 *  constructor).
 *
 *  This call is now done in the clinsmanager, who can also get the value from
 *  the "usr" file.
 *
 *      std::string url = nsm::get_url();
 *
 *  Note that this bare pointer should be assigned immediately to a smart
 *  pointer, such as std::unique_ptr<>.  See seq_qt5/src/qt5nsmanager.cpp for
 *  an example.
 *
 * \param sm
 *      Provides a reference to the existing session manager.
 *
 * \param nsmurl
 *      Provides either the value of NSM_URL or the value that might be
 *      defined in the "usr" file.
 *
 * \param nsmfile
 *      Need to revisit this and figure out what it is.  The name of the nsm
 *      file?  Currently created by nsmd, it is always "session.nsm".
 *
 * \param nsmext
 *      The NSM file extension, "nsm".
 *
 * \return
 *      Returns the pointer to the nsmclient.  Assign it to a smart pointer
 *      immediately!
 */

nsmclient *
create_nsmclient
(
    smanager & sm,
    const std::string & nsmurl,
    const std::string & nsmfile,
    const std::string & nsmext
)
{
    nsmclient * result = nullptr;
    if (! nsmurl.empty())
    {
        result = new (std::nothrow) nsmclient(sm, nsmurl, nsmfile, nsmext);
        if (not_nullptr(result))
            (void) result->initialize();
    }
    return result;
}

}           // namespace seq66

/*
 * nsmclient.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


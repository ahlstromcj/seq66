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
 * \updates       2020-08-22
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
 *      $ non-session-manager -- --session-root path
 *
 *      The default path is "$HOME/NSM Sessions".  At startup, NSM_URL is
 *      added to the environment.  Commands are handled as per the "Commands
 *      handling by the server" section at the top of the nsmmessageex.cpp
 *      module.
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
 *      command-line application, we can set flags that are detected in a polling
 *      loop and cause actions to occur.  In Gtkmm applications, we can set
 *      callbacks to be executed.  It would be nice to use the same (callback?)
 *      system for all of them.
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
 *      INVESTIGATE the NSM replacement, RaySend!!!
 */

#include <stdlib.h>                     /* C geteven() or secure_getenv()   */
#include <string.h>                     /* C strlen()                       */

#include "util/basic_macros.h"          /* not_nullptr() macro              */
#include "nsm/nsmclient.hpp"            /* seq66::nsmclient class           */
#include "nsm/nsmmessagesex.hpp"        /* seq66::nsm message functions     */

#if defined SEQ66_PLATFORM_DEBUG
#include "util/strfunctions.hpp"        /* seq66::bool_to_string()          */
#endif

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
    nsmbase         (nsmurl, nsmfile, nsmext),
    m_manager       (),
    m_capabilities  (),
    m_path_name     (),
    m_display_name  (),
    m_client_id     (),
    m_nsm_file      (nsmfile),
    m_nsm_ext       (nsmext)
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
        // m_dirty_count = 0;
        // m_dirty = false;
        m_nsm_file.clear();
    }
    return result;
}

/**
 *  Provides a factory function to create an nsmclient.
 *
 *  Note that this bare pointer should be assigned to a smart pointer, such as
 *  std::unique_ptr<>.  See seq_qt5/src/qt5nsmanager.cpp for an example.
 */

nsmclient *
create_nsmclient
(
    const std::string & nsmfile,
    const std::string & nsmext
)
{
    nsmclient * result = nullptr;
    std::string url = get_nsm_url();
    if (! url.empty())
        result = new (std::nothrow) nsmclient(url, nsmfile, nsmext);

    return result;
}

}           // namespace seq66

/*
 * nsmclient.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


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
 * \file          nsmmessages.cpp
 *
 *  This module defines some informative functions that are actually
 *  better off as functions.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2020-03-07
 * \updates       2020-03-08
 * \license       GNU GPLv2 or above
 *
 *  Provides encapsulated access to the complete set of NSM messages.
 */

#include <string>                       /* std::string                      */

#include "nsm/nsmmessages.hpp"          /* seq66::nsm message functions     */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

const char *
nsm_basic_error ()
{
    static const std::string sm_basic_error("/error");
    return sm_basic_error.c_str();
}

const char *
nsm_basic_list ()
{
    static const std::string sm_basic_list("/nsm/session/list");
    return sm_basic_list.c_str();
}

const char *
nsm_basic_reply ()
{
    static const std::string sm_basic_reply("/reply");
    return sm_basic_reply.c_str();
}

const char *
nsm_cli_gui_hidden ()
{
    static const std::string sm_cli_gui_hidden("/nsm/client/gui_is_hidden");
    return sm_cli_gui_hidden.c_str();
}

const char *
nsm_cli_gui_shown ()
{
    static const std::string sm_cli_gui_shown("/nsm/client/gui_is_shown");
    return sm_cli_gui_shown.c_str();
}

const char *
nsm_cli_hide_opt_gui ()
{
    static const std::string sm_cli_hide_opt_gui("/nsm/client/hide_optional_gui");
    return sm_cli_hide_opt_gui.c_str();
}

const char *
nsm_cli_is_clean ()
{
    static const std::string sm_cli_is_clean("/nsm/client/is_clean");
    return sm_cli_is_clean.c_str();
}

const char *
nsm_cli_is_dirty ()
{
    static const std::string sm_cli_is_dirty("/nsm/client/is_dirty");
    return sm_cli_is_dirty.c_str();
}

const char *
nsm_cli_is_loaded ()
{
    static const std::string sm_cli_is_loaded("/nsm/client/session_is_loaded");
    return sm_cli_is_loaded.c_str();
}

const char *
nsm_cli_label ()
{
    static const std::string sm_cli_label("/nsm/client/label");
    return sm_cli_label.c_str();
}

const char *
nsm_cli_message ()
{
    static const std::string sm_cli_message("/nsm/client/message");
    return sm_cli_message.c_str();
}

const char *
nsm_cli_open ()
{
    static const std::string sm_cli_open("/nsm/client/open");
    return sm_cli_open.c_str();
}

const char *
nsm_cli_progress ()
{
    static const std::string sm_cli_progress("/nsm/client/progress");
    return sm_cli_progress.c_str();
}

const char *
nsm_cli_save ()
{
    static const std::string sm_cli_save("/nsm/client/save");
    return sm_cli_save.c_str();
}

const char *
nsm_cli_show_opt_gui ()
{
    static const std::string sm_cli_show_opt_gui("/nsm/client/show_optional_gui");
    return sm_cli_show_opt_gui.c_str();
}

/**
 *
 */

const char *
nsm_gui_announce ()
{
    static const std::string sm_gui_announce("/nsm/gui/gui_announce");
    return sm_gui_announce.c_str();
}

const char *
nsm_gui_remove ()
{
    static const std::string sm_gui_remove("/nsm/gui/client/remove");
    return sm_gui_remove.c_str();
}

const char *
nsm_gui_resume ()
{
    static const std::string sm_gui_resume("/nsm/gui/client/resume");
    return sm_gui_resume.c_str();
}

const char *
nsm_gui_save ()
{
    static const std::string sm_gui_save("/nsm/gui/client/save");
    return sm_gui_save.c_str();
}

const char *
nsm_gui_stop ()
{
    static const std::string sm_gui_stop("/nsm/gui/client/stop");
    return sm_gui_stop.c_str();
}

const char *
nsm_proxy_label ()
{
    static const std::string sm_proxy_label("/nsm/proxy/label");
    return sm_proxy_label.c_str();
}

const char *
nsm_proxy_save_signal ()
{
    static const std::string sm_proxy_save_signal("/nsm/proxy/save_signal");
    return sm_proxy_save_signal.c_str();
}

const char *
nsm_proxy_stop_signal ()
{
    static const std::string sm_proxy_stop_signal("/nsm/proxy/stop_signal");
    return sm_proxy_stop_signal.c_str();
}

const char *
nsm_proxy_kill ()
{
    static const std::string sm_proxy_kill("/nsm/proxy/kill");
    return sm_proxy_kill.c_str();
}

const char *
nsm_proxy_start ()
{
    static const std::string sm_proxy_start("/nsm/proxy/start");
    return sm_proxy_start.c_str();
}

const char *
nsm_proxy_update ()
{
    static const std::string sm_proxy_update("/nsm/proxy/update");
    return sm_proxy_update.c_str();
}

const char *
nsm_srv_abort ()
{
    static const std::string sm_srv_abort("/nsm/server/abort");
    return sm_srv_abort.c_str();
}

/**
 *  A client should not consider itself to be under session management until it
 *  receives this response. For example, the Non applications activate their
 *  "SM" blinkers at this time.
 */

const char *
nsm_srv_announce ()
{
    static const std::string sm_srv_announce("/nsm/server/announce");
    return sm_srv_announce.c_str();
}

const char *
nsm_srv_broadcast ()
{
    static const std::string sm_srv_broadcast("/nsm/server/broadcast");
    return sm_srv_broadcast.c_str();
}

const char *
nsm_srv_close ()
{
    static const std::string sm_srv_close("/nsm/server/close");
    return sm_srv_close.c_str();
}

const char *
nsm_srv_list ()
{
    static const std::string sm_srv_list("/nsm/server/list");
    return sm_srv_list.c_str();
}

const char *
nsm_srv_new ()
{
    static const std::string sm_srv_new("/nsm/server/new");
    return sm_srv_new.c_str();
}

const char *
nsm_srv_quit ()
{
    static const std::string sm_srv_quit("/nsm/server/quit");
    return sm_srv_quit.c_str();
}

const char *
nsm_default_ext ()
{
    static const std::string sm_default_ext("nsm");
    return sm_default_ext.c_str();
}

const char *
nsm_dirty_msg (bool isdirty)
{
    return isdirty ? nsm_cli_is_dirty() : nsm_cli_is_clean ();
}

const char *
nsm_visible_msg (bool isvisible)
{
    return isvisible ? nsm_cli_gui_shown() : nsm_cli_gui_hidden ();
}

const char *
nsm_url ()
{
    static const std::string sm_url_var("NSM_URL");
    return sm_url_var.c_str();
}

bool
nsm_is_announce (const char * s)
{
    std::string stmp(s);
    return stmp == nsm_srv_announce();
}

}           // namespace seq66

/*
 * nsm.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


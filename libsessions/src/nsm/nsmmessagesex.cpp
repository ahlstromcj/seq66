/**
 * \file          nsmmessagesex.cpp
 *
 *    This module provides a kind of repository of all the possible OSC/NSM
 *    messages.
 *
 * \library       seq66
 * \author        Chris Ahlstrom
 * \date          2020-08-21
 * \updates       2020-08-28
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *      Upcoming support for the Non Session Manager.  Defines a number of free
 *      functions in the seq66::nsm namespace.
 *
 * Commands handling by the server:
 *
 *      -#  add.  Adds a client process. Sends either an "/error" + path
 *          message, or a "/reply" + path + OK + "Launched" message.  ("New"
 *          doesn't send OK!)
 *      -#  announce. The client sends an "announce" message. If there is no
 *          session, "/error" + path + errcode + message is sent.
 *          Incompatible API versions are detected.
 *      -#  save. Commands all clients to save.  Sends either an "/error"
 *          message or a "/reply" + path + "Saved" message.
 *      -#  duplicate. Duplicates a session.  Sends an "/error" or a
 *          "/nsm/gui/session/session" message plus a "/reply" + path +
 *          "Duplicated" message.
 *      -#  new. Commands all clients to save, and then creates a new session.
 *          Sends an "/error" or a "/nsm/gui/session/session" message plus a
 *          "/reply" + path + "Session created" message.
 *      -#  list. Lists sessions.  Sends an empty "/reply", then an
 *          "/nsm/server/list" message with an empty message.
 *      -#  open. Opens a session.  Sends an "/error" message or a "/reply" +
 *          path + "Loaded" message.
 *      -#  quit.  Closes the session.  Sends "/nsm/gui/session/name" plus an
 *          empty session name.
 *      -#  abort. If a session is open and there is no operation pending (in
 *          which cases an "/error" is sent), then the session is quit as
 *          above.
 *      -#  close.  Similar to "abort", except that all clients are first
 *          commanded to save.
 *      -#  broadcast.  The server sends out a command to all clients.
 *      -#  progress.  Sends "/nsm/gui/client/progress" + Client-ID +
 *          progress.
 *      -#  is_dirty.  A client sends "/nsm/client/is_dirty" and the server
 *          sends out "/nsm/gui/client/dirty" + Client-ID + dirty.
 *      -#  is_clean.  A client sends "/nsm/client/is_clean" and the server
 *          sends out "/nsm/gui/client/dirty" + Client-ID + 0.
 *      -#  gui_is_hidden.  The client sends "/nsm/client/gui_is_hidden" and
 *          the server sends "/ndm/gui/client/gui_visible" + Client-ID + 0.
 *      -#  gui_is_shown.  The client sends "/nsm/client/gui_is_shown" and
 *          the server sends "/ndm/gui/client/gui_visible" + Client-ID + 1.
 *      -#  message.  The client sends "/nsm/client/message" + Client_ID +
 *          integer + string, and the server forwards this information to all
 *          clients via an "/nsm/gui/client/message".
 *      -#  label. The client sends an "/nsm/client/label" message, and the
 *          server sends out an "/nsm/gui/client/label" message.
 *      -#  error. The client sends an "/error" message ("sis" parameters),
 *          and the server sends out "/nsm/gui/client/status" + Client-ID +
 *          status.  MORE TO COME.
 *      -#  reply. The client sends a "/reply" message ("ssss" parameters),
 *          and the server sends out "/nsm/gui/client/status" + Client-ID +
 *          status.  MORE TO COME.
 *      -#  stop.  A GUI operation.
 *      -#  remove.  A GUI operation.
 *      -#  resume.  A GUI operation.
 *      -#  client_save.  A GUI operation.
 *      -#  client_show_optional_gui.  A GUI operation.
 *      -#  client_hide_optional_gui.  A GUI operation.
 *      -#  gui_announce.
 *      -#  ping.
 *      -#  null.
 */

#include "nsm/nsmmessagesex.hpp"        /* seq66::nsm::tag, etc.            */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 *  The seq66::nsm namespace is a place to squirrel away concepts that are
 *  not defined in the other nsmbase classes.
 */

namespace nsm
{

/**
 *  This map of message/pattern pairs provides all the messages and patterns
 *  used in the "/nsm/client/xxxxx" series of messages, including client
 *  variations of "/error" and "/reply".
 */

static lookup s_client_msgs =
{
    {  tag::null,        { "",                                 ""         } },
    {  tag::clean,       { "/nsm/client/is_clean",             ""         } },
    {  tag::dirty,       { "/nsm/client/is_dirty",             ""         } },
    {  tag::error,       { "/error",                           "sis"      } },
    {  tag::hidden,      { "/nsm/client/gui_is_hidden",        ""         } },
    {  tag::hide,        { "/nsm/client/hide_optional_gui",    ""         } },
    {  tag::label,       { "/nsm/client/label",                "s"        } },
    {  tag::loaded,      { "/nsm/client/session_is_loaded",    ""         } },
    {  tag::message,     { "/nsm/client/message",              "is"       } },
    {  tag::open,        { "/nsm/client/open",                 "sss"      } },
    {  tag::progress,    { "/nsm/client/progress",             "f"        } },
    {  tag::reply,       { "/reply",                           "ss"       } },
    {  tag::replyex,     { "/reply",                           "ssss"     } },
    {  tag::save,        { "/nsm/client/save",                 ""         } },
    {  tag::show,        { "/nsm/client/show_optional_gui",    ""         } },
    {  tag::shown,       { "/nsm/client/gui_is_shown",         ""         } }
};

static lookup s_gui_client_msgs =
{
    {  tag::announce,    { "/nsm/gui/gui_announce",             "s"       } },
    {  tag::dirty,       { "/nsm/gui/client/dirty",             "si"      } },
    {  tag::hide,        { "/nsm/gui/client/hide_optional_gui", "s"       } },
    {  tag::label,       { "/nsm/gui/client/label",             "ss"      } },
    {  tag::message,     { "/nsm/gui/client/message",           "s"       } },
    {  tag::newcs,       { "/nsm/gui/client/new",               "ss"      } },
    {  tag::optional,    { "/nsm/gui/client/has_optional_gui",  "s"       } },
    {  tag::progress,    { "/nsm/gui/client/progress",          "sf"      } },
    {  tag::remove,      { "/nsm/gui/client/remove",            "s"       } },
    {  tag::resume,      { "/nsm/gui/client/resume",            "s"       } },
    {  tag::save,        { "/nsm/gui/client/save",              "s"       } },
    {  tag::show,        { "/nsm/gui/client/show_optional_gui", "s"       } },
    {  tag::status,      { "/nsm/gui/client/status",            "ss"      } },
    {  tag::stop,        { "/nsm/gui/client/stop",              "s"       } },
    {  tag::switchc,     { "/nsm/gui/client/switch",            "ss"      } },
    {  tag::visible,     { "/nsm/gui/client/gui_visible",       "si"      } }
};

static lookup s_gui_session_msgs =
{
    {  tag::announce,    { "/nsm/gui/server_announce",          "s"       } },
    {  tag::message,     { "/nsm/gui/server/message",           "s"       } },
    {  tag::name,        { "/nsm/gui/session/name",             "ss"      } },
    {  tag::root,        { "/nsm/gui/session/root",             "s"       } },
    {  tag::session,     { "/nsm/gui/session/session",          "s"       } }
};

static lookup s_proxy_msgs =
{
    {  tag::arguments,   { "/nsm/proxy/arguments",              "s"       } },
    {  tag::clienterror, { "/nsm/proxy/client_error",           "s"       } },
    {  tag::configfile,  { "/nsm/proxy/config_file",            "s"       } },
    {  tag::executable,  { "/nsm/proxy/executable",             "s"       } },
    {  tag::kill,        { "/nsm/proxy/kill",                   ""        } },
    {  tag::label,       { "/nsm/proxy/label",                  "s"       } },
    {  tag::savesignal,  { "/nsm/proxy/save_signal",            "i"       } },
    {  tag::start,       { "/nsm/proxy/start",                  "sss"     } },
    {  tag::stopsignal,  { "/nsm/proxy/stop_signal",            "i"       } },
    {  tag::update,      { "/nsm/proxy/update",                 ""        } }
};

static lookup s_server_msgs =
{
    {  tag::abort,      { "/nsm/server/abort",                 ""         } },
    {  tag::add,        { "/nsm/server/add",                   "s"        } },
    {  tag::announce,   { "/nsm/server/announce",              "sssiii"   } },
    {  tag::broadcast,  { "/nsm/server/broadcast",             ""         } },
    {  tag::close,      { "/nsm/server/close",                 ""         } },
    {  tag::duplicate,  { "/nsm/server/duplicate",             "s"        } },
    {  tag::list,       { "/nsm/server/list",                  ""         } },
    {  tag::newcs,      { "/nsm/server/new",                   "s"        } },
    {  tag::open,       { "/nsm/server/open",                  "s"        } },
    {  tag::quit,       { "/nsm/server/quit",                  ""         } },
    {  tag::save,       { "/nsm/server/save",                  ""         } }
};

static lookup s_misc_msgs =
{
    {  tag::error,      { "/error",                            "sis"      } },
    {  tag::list,       { "/nsm/session/list",                 "?"        } },
    {  tag::name,       { "/nsm/session/name",                 "ss"       } },
    {  tag::ping,       { "/osc/ping",                         ""         } },
    {  tag::reply,      { "/reply",                            "ssss"     } }
};

/**
 *  Used in creating an OSC server endpoint.
 */

static lookup s_signal_msgs =
{
    {  tag::connect,         { "/signal/connect",              "ss"       } },
    {  tag::created,         { "/signal/created",              "ss"       } },
    {  tag::disconnect,      { "/signal/disconnect",           "ss"       } },
    {  tag::generic,         { "",                             ""         } },
    {  tag::hello,           { "/signal/hello",                "ss"       } },
    {  tag::list,            { "/signal/list",                 ""         } },
    {  tag::removed,         { "/signal/removed",              "ss"       } },
    {  tag::renamed,         { "/signal/renamed",              "ss"       } },
    {  tag::reply,           { "/reply",                       ""         } }
};

/**
 *  Used by NSM itself.
 */

static lookup s_non_msgs =
{
    {  tag::addstrip,        { "/non/mixer/add_strip",          ""        } },
    {  tag::hello,           { "/non/hello",                    "ssss"    } },
    {  tag::oscreply,        { "",                              ""        } },
    {  tag::stripbynumber,   { "",                              ""        } }
};

/**
 *  Generic lookup function for a lookup table.
 *
 * \param table
 *      Provides the particular table (e.g. client versus server) to be looked
 *      up.
 *
 * \param t
 *      Provides the "tag" enumeration value to be used to look up the desired
 *      message and its message pattern.
 *
 * \param [out] message
 *      The destination for the message text that was found.
 *
 * \param [out] pattern
 *      The destination for the pattern text that was found.
 *
 * \return
 *      Returns true if the tag \a t was found.  If false is returned, do not
 *      use the message and pattern.
 */

static bool
nsm_lookup
(
    const lookup & table,
    tag t,
    std::string & message,
    std::string & pattern
)
{
    bool result = false;
    auto lci = table.find(t);           /* lookup::const_iterator   */
    if (lci != table.end())
    {
        result = true;
        message = lci->second.msg_text;
        pattern = lci->second.msg_pattern;
    }
    return result;
}

/*
 * The rest of these free functions provide easy lookup of the various messages
 * and their patterns.
 */

bool
client_msg (tag t, std::string & message, std::string & pattern)
{
    return nsm_lookup(s_client_msgs, t, message, pattern);
}

bool
gui_client_msg (tag t, std::string & message, std::string & pattern)
{
    return nsm_lookup(s_gui_client_msgs, t, message, pattern);
}

bool
gui_session_msg (tag t, std::string & message, std::string & pattern)
{
    return nsm_lookup(s_gui_session_msgs, t, message, pattern);
}

bool
proxy_msg (tag t, std::string & message, std::string & pattern)
{
    return nsm_lookup(s_proxy_msgs, t, message, pattern);
}

bool
server_msg (tag t, std::string & message, std::string & pattern)
{
    return nsm_lookup(s_server_msgs, t, message, pattern);
}

bool
misc_msg (tag t, std::string & message, std::string & pattern)
{
    return nsm_lookup(s_misc_msgs, t, message, pattern);
}

/**
 *  Inverse lookup.  Given the message-path name, returns the code.
 *
 *  Not sure if we need to check the pattern as well.
 */

tag
nsm_lookup_tag
(
    const lookup & table,
    const std::string & message,
    const std::string & pattern = "X"
)
{
    tag result = tag::null;
    for (auto lci = table.begin(); lci != table.end(); ++lci)
    {
        bool match = lci->second.msg_text == message;
        if (match)
        {
            if (pattern != "X")
                match = lci->second.msg_pattern == pattern;

            if (match)
            {
                result = lci->first;
                break;
            }
        }
    }
    return result;
}

/*
 * The rest of these free functions provide easy lookup of the various tags
 * from the given message.
 */

tag
client_tag (const std::string & message, const std::string & pattern)
{
    return nsm_lookup_tag(s_client_msgs, message, pattern);
}

tag
server_tag (const std::string & message, const std::string & pattern)
{
    return nsm_lookup_tag(s_server_msgs, message, pattern);
}

/*
 * Additional helpful functions.
 */

const std::string &
default_ext ()
{
    static const std::string sm_default_ext("nsm");
    return sm_default_ext;
}

const std::string &
dirty_msg (bool isdirty)
{
    tag t = isdirty ? tag::dirty : tag::clean ;
    return s_client_msgs[t].msg_text;
}

const std::string &
visible_msg (bool isvisible)
{
    tag t = isvisible ? tag::shown : tag::hidden ;
    return s_client_msgs[t].msg_text;
}

const std::string &
url ()
{
    static const std::string sm_url_var("NSM_URL");
    return sm_url_var;
}

bool
is_announce (const std::string & s)
{
    return s == s_server_msgs[tag::announce].msg_text;
}

}           // namespace nsm

}           // namespace seq66

/*
* nsmmessagesex.cpp
*
* vim: sw=4 ts=4 wm=4 et ft=cpp
*/


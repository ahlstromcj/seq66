#if ! defined SEQ66_NSMMESSAGESEX_HPP
#define SEQ66_NSMMESSAGESEX_HPP

/**
 * \file          nsmmessagesex.hpp
 *
 *    This module provides a kind of repository of all the possible OSC/NSM
 *    messages.
 *
 * \library       seq66
 * \author        Chris Ahlstrom
 * \date          2020-08-20
 * \updates       2020-09-01
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *  Upcoming support for the Non Session Manager.  This module provides a list
 *  of OSC paths (messages) for various purposes, as a way to keep track of
 *  them all and use them propertly.
 */

#include <map>                          /* std::map dictionary class        */
#include <string>                       /* std::string class                */

namespace seq66
{

/*
 *  The seq66::nsm namespace is a place to squirrel away concepts that are
 *  not defined in the other nsmbase classes.
 */

namespace nsm
{

/**
 *  The seq66::nsm::tag enumeration is used in the lookup of the long
 *  strings that are sent and received by NSM.  We can use these tags to
 *  look up both the long name and the OSC formatting string to be used in
 *  a message.
 */

enum class tag
{
    null,               // client, all items null
    abort,              // server
    add,                // server
    addstrip,           // non
    announce,           // gui, gui/server, server
    arguments,          // proxy
    broadcast,          // server
    clean,              // client
    clienterror,        // proxy
    close,              // server
    connect,            // signal
    configfile,         // proxy
    created,            // signal
    dirty,              // client, gui/client
    disconnect,         // signal
    duplicate,          // server
    error,              // used by many
    executable,         // proxy
    generic,            // signal
    hello,              // signal
    hidden,             // client
    hide,               // client, gui/client
    kill,               // proxy
    label,              // client, gui/client, proxy
    list,               // server, session, signal
    loaded,             // client
    message,            // client, gui/client, gui/server
    name,               // gui/session, session
    newcs,              // gui/client, server
    open,               // client, server
    optional,           // gui/client
    oscreply,           // osc, non
    ping,               // used by manu
    progress,           // client, gui/client
    quit,               // server
    remove,             // gui/client
    removed,            // signal
    renamed,            // signal
    reply,              // used by many, signal has no args
    replyex,            // another variation
    resume,             // gui/client
    root,               // gui/session
    save,               // client, gui/client, server
    savesignal,         // proxy
    session,            // gui/session
    show,               // client, gui/client
    shown,              // client
    start,              // proxy
    status,             // gui/client
    stop,               // gui/client
    stopsignal,         // prox
    stripbynumber,      // non
    switchc,            // gui/client
    update,             // proxy
    visible             // gui/client
};

/**
 *  This type holds the long OSC string for the message, and the data
 *  pattern string that describes the data being sent.
 */

using messagepair = struct
{
    std::string msg_text;
    std::string msg_pattern;
};

/**
 *  A lookup map for tags and message pairs.
 */

using lookup = std::map<tag, messagepair>;

/*
 *  Free functions for table lookup.
 */

extern bool client_msg (tag t, std::string & message, std::string & pattern);
extern bool gui_client_msg (tag t, std::string & message, std::string & pattern);
extern bool gui_session_msg (tag t, std::string & message, std::string & pattern);
extern bool proxy_msg (tag t, std::string & message, std::string & pattern);
extern bool server_msg (tag t, std::string & message, std::string & pattern);
extern bool misc_msg (tag t, std::string & message, std::string & pattern);

/*
 *  Free functions for inverse table lookup.
 */

extern tag client_tag
(
    const std::string & message,
    const std::string & pattern = "X"
);
extern tag server_tag
(
    const std::string & message,
    const std::string & pattern = "X"
);

/*
 *  More free functions.
 */

extern const std::string & default_ext ();
extern const std::string & dirty_msg (bool isdirty);
extern const std::string & visible_msg (bool isvisible);
extern const std::string & url ();
extern bool is_announce (const std::string & s = "");

}           // namespace nsm

}           // namespace seq66

#endif      // SEQ66_NSMMESSAGESEX_HPP

/*
 * nsmmessagesex.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


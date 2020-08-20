#if ! defined SEQ66_NSMMESSAGES_HPP
#define SEQ66_NSMMESSAGES_HPP

/**
 * \file          nsmmessages.hpp
 *
 *    This module provides a reimplementation of the nsm.h header file as a
 *    class.
 *
 * \library       seq66
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2020-03-01
 * \updates       2020-08-20
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *  Upcoming support for the Non Session Manager.
 */

#include <map>                          /* std::map dictionary class        */

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
     *  The seq66::nsm::tag enumeration is used in the lookup of the long
     *  strings that are sent and received by NSM.  We can use these tags to
     *  look up both the long name and the OSC formatting string to be used in
     *  a message.
     */

    enum class tag
    {
        abort,             // server
        add,               // server
        announce,          // gui, gui/server, server
        arguments,         // proxy
        broadcast,         // server
        clean,             // client
        clienterror,       // proxy
        close,             // server
        configfile,        // proxy
        dirty,             // client, gui/client
        duplicate,         // server
        error,             // used by many
        executable,        // proxy
        hidden,            // client
        hide,              // client
        kill,              // proxy
        label,             // client, gui/client, proxy
        list,              // server, session
        loaded,            // client
        message,           // client, gui/client, gui/server
        name,              // gui/session, session
        new,               // gui/client, server
        open,              // client, server
        optional,          // gui/client
        ping,              // used by manu
        progress,          // client, gui/client
        quit,              // server
        remove,            // gui/client
        reply,             // used by many
        resume,            // gui/client
        root,              // gui/session
        save,              // client, gui/client, server
        savesignal,        // proxy
        session,           // gui/session
        show,              // client
        shown,             // client
        start,             // proxy
        status,            // gui/client
        stop,              // gui/client
        stopsignal,        // prox
        switch,            // gui/client
        update,            // proxy
        visible            // gui/client
    };

    /**
     *  This type holds the long OSC string for the message, and the data
     *  pattern string that describes the data being sent.
     */

    using message = struct
    {
        std::string msg_text;
        std::string msg_pattern;
    };

    /**
     *  This type provides a map type for looking up the message structure.
     */

    using lookup = std::map<tag, message>;

    // Q uses parameters of path and reply-message with pattern "ss"
    // name : char * (client application name)
    // ID = client ID : char *
    // Full ID = "name.ID"
    // EXE = executable_path : char *
    // status : const char *
    // pid : int
    // progress : float
    // active, dirty, pre_existing : bool
    // Q = Qtractor uses this. Qtractor supports:
    //  capabilities = ":switch:dirty:optional-gui:"
    //  open nsm session
    //  save nsm session: also done when a file is saved
    //  show nsm session: show, raise, and active the window
    //  hide nsm session: hide the main window
    //  show event
    //  hide event
    //  confirm archive directory
    //  load session file, open session, open reply, dirtiness, symlink support
    //  set session name, extension, and directory
    //
    // status (for "/nsm'gui/client/status")
    // {
    //      "removed", "stopped", "launch", "save", "noop", "switch",
    //      "open", "quit"
    // }

    lookup client_msgs =
    {
        {  hidden,          { "/nsm/client/gui_is_hidden",       ""        } }, // Q
        {  shown,           { "/nsm/client/gui_is_shown",        ""        } }, // Q
        {  hide,            { "/nsm/client/hide_optional_gui",   "s"       } }, // Q, no parameters
        {  clean,           { "/nsm/client/is_clean",            ""        } }, // Q
        {  dirty,           { "/nsm/client/is_dirty",            ""        } }, // Q
        {  label,           { "/nsm/client/label",               "s"       } },
        {  message,         { "/nsm/client/message",             "is"      } }, // Q
        {  open,            { "/nsm/client/open",                "sss"     } }, // Q reply code, path, sess name, Full ID
        {  progress,        { "/nsm/client/progress",            "f"       } }, // Q, ID, progress value
        {  save,            { "/nsm/client/save",                ""        } }, // Q reply code, no parameters
        {  loaded,          { "/nsm/client/session_is_loaded",   ""        } }, // no parameters
        {  show,            { "/nsm/client/show_optional_gui",   ""        } }  // no parameters
    };

    {  dirty,           { "/nsm/gui/client/dirty",           "?"       } },
    {  visible,         { "/nsm/gui/client/gui_visible",     "ii"      } },
    {  optional,        { "/nsm/gui/client/has_optional_gui","?"       } }, // ID
    {  label,           { "/nsm/gui/client/label",           "is"      } },
    {  message,         { "/nsm/gui/client/message",         "s"       } },
    {  new,             { "/nsm/gui/client/new",             "??"      } }, // ID, name
    {  progress,        { "/nsm/gui/client/progress",        "?"       } },
    {  remove,          { "/nsm/gui/client/remove",          "s?"      } },
    {  resume,          { "/nsm/gui/client/resume",          "s"       } },
    {  save,            { "/nsm/gui/client/save",            ""        } }, // Q save handler
    {  status,          { "/nsm/gui/client/status",          "??"      } }, // ID, status
    {  stop,            { "/nsm/gui/client/stop",            "s"       } },
    {  switch,          { "/nsm/gui/client/switch",          "ii"      } }, // old ID, ID
    {  announce,        { "/nsm/gui/gui_announce",           ""        } },

    {  announce,        { "/nsm/gui/server/announce",        "?"       } }, // char *
    {  message,         { "/nsm/gui/server/message",         "s"       } }, // char *

    {  name,            { "/nsm/gui/session/name",           "?"       } }, // session name, path+root
    {  root,            { "/nsm/gui/session/root",           "?"       } },
    {  session,         { "/nsm/gui/session/session",        "?"       } }, // argv[0]

    {  arguments,       { "/nsm/proxy/arguments",            "s"       } }, // can be empty
    {  clienterror,     { "/nsm/proxy/client_error",         "s"       } }, // can be empty
    {  configfile,      { "/nsm/proxy/config_file",          "s"       } }, // can be empty
    {  executable,      { "/nsm/proxy/executable",           "s"       } }, // can be empty
    {  kill,            { "/nsm/proxy/kill",                 ""        } },
    {  label,           { "/nsm/proxy/label",                "s"       } },
    {  savesignal,      { "/nsm/proxy/save_signal",          "i"       } },
    {  start,           { "/nsm/proxy/start",                "sss"     } },
    {  stopsignal,      { "/nsm/proxy/stop_signal",          "i"       } },
    {  update,          { "/nsm/proxy/update",               ""        } },

    {  abort,           { "/nsm/server/abort",               ""        } },
    {  add,             { "/nsm/server/add",                 "s"       } },
    {  announce,        { "/nsm/server/announce",            "sssiii"  } }, // Q error and reply
    {  broadcast,       { "/nsm/server/broadcast",           ""        } },
    {  close,           { "/nsm/server/close",               ""        } },
    {  duplicate,       { "/nsm/server/duplicate",           "s"       } },
    {  list,            { "/nsm/server/list",                ""        } }, // path?
    {  new,             { "/nsm/server/new",                 "s"       } },
    {  open,            { "/nsm/server/open",                "s"       } },
    {  quit,            { "/nsm/server/quit",                ""        } },
    {  save,            { "/nsm/server/save",                ""        } },

    {  list,            { "/nsm/session/list",               "?"       } },
    {  name,            { "/nsm/session/name",               "ss"      } }, // 2 empty strings

    {  error,           { "/error",                          "sis"     } }, // Q error handler
    {  reply,           { "/reply",                          "ssss"    } }, // path, "Loaded"
    {  ping,            { "/osc/ping",                       ""        } },

}           // namespace msg

}           // namespace seq66

#endif      // SEQ66_NSMMESSAGES_HPP

/*
 * nsmmessages.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


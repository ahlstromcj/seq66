#if ! defined SEQ66_NSMBASE_HPP
#define SEQ66_NSMBASE_HPP

/**
 * \file          nsmbase.hpp
 *
 *    This module provides a reimplementation of the nsm.h header file as a
 *    class.
 *
 * \library       seq66
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2020-03-01
 * \updates       2022-01-09
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *  Upcoming support for the Non Session Manager.
 */

#include <atomic>                       /* std::atomic<bool>                */
#include <vector>                       /* std::vector                      */

#include "seq66_features.hpp"           /* feature (SUPPORT) macros         */
#include "util/basic_macros.hpp"          /* is_nullptr() & not_nullptr()     */
#include "nsm/nsmmessagesex.hpp"        /* seq66::nsm::tag                  */

#if defined SEQ66_LIBLO_SUPPORT
#include <lo/lo.h>                      /* library for the OSC protocol     */
#else
#error Support for liblo required for this class, install liblo-dev
#endif

#define NSM_API_VERSION_MAJOR   1
#define NSM_API_VERSION_MINOR   0

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

namespace nsm
{

/**
 *  Provides reply codes matching those of NSM>
 */

enum class error
{
    ok                  =  0,
    general             = -1,
    incompatible_api    = -2,
    blacklisted         = -3,
    launch_failed       = -4,
    no_such_file        = -5,
    no_session_open     = -6,
    unsaved_changes     = -7,
    not_now             = -8,
    bad_project         = -9,
    create_failed       = -10,
    session_locked      = -11,          /* see nsmd.C in the Non project    */
    operation_pending   = -12,          /* see nsmd.C in the Non project    */
    save_failed         = -99           /* doesn't exist in the Non project */
};

/*
 *  External helper functions in the nsm namespace.
 */

extern std::string reply_string (error errorcode);
extern std::string get_url ();
extern void incoming_msg
(
    const std::string & cbname,
    const std::string & message,
    const std::string & pattern,
    bool iserror = false
);
extern void outgoing_msg
(
    const std::string & message,
    const std::string & pattern,
    const std::string & data = "sent"
);

extern tokenization convert_lo_args
(
    const std::string & pattern,
    int argc, lo_arg ** argv
);

}           // namespace nsm

/**
 *  nsmbase is an NSM OSC server/client base class.
 */

class nsmbase
{
    friend class clinsmanager;

private:

    static std::string sm_nsm_default_ext;

    /**
     *  Provides a reference (a void pointer) to an OSC service. See
     *  /usr/include/lo/lo_types.h.
     */

    lo_address m_lo_address;

    /**
     *  Provides a reference (a void pointer) to a thread "containing"
     *  an OSC server. See /usr/include/lo/lo_types.h.
     */

    lo_server_thread m_lo_server_thread;

    /**
     *  Provides a reference (a void pointer) to an object representing an
     *  an OSC server. See /usr/include/lo/lo_types.h.
     */

    lo_server m_lo_server;

    /**
     *  This item is mutable because it can be falsified if the server and
     *  address are found to be null.  It is turned on when we receive the
     *  information about the session (including path to the session).
     */

    mutable std::atomic<bool> m_active;

    bool m_dirty;
    int m_dirty_count;
    std::string m_manager;
    std::string m_capabilities;
    std::string m_path_name;
    std::string m_display_name;
    std::string m_client_id;
    std::string m_nsm_file;
    std::string m_nsm_ext;
    std::string m_nsm_url;

public:

    nsmbase
    (
        const std::string & nsmurl,
        const std::string & nsmfile = "",
        const std::string & nsmext  = ""
    );
    virtual ~nsmbase ();

public:

    bool is_active() const              // session activation accessor
    {
        return m_active;
    }

    bool is_a_client (const nsmbase * p)
    {
        return not_nullptr(p) && p->is_active();
    }

    bool not_a_client (const nsmbase * p)
    {
        return is_nullptr(p) || ! p->is_active();
    }

    // Session manager accessors.

    const std::string & manager () const
    {
        return m_manager;
    }

    const std::string & capabilities () const
    {
        return m_capabilities;
    }

    // Session client accessors.

    const std::string & path_name () const
    {
        return m_path_name;
    }

    const std::string & display_name () const
    {
        return m_display_name;
    }

    const std::string & client_id () const
    {
        return m_client_id;
    }

    const std::string & nsm_file () const
    {
        return m_nsm_file;
    }

    const std::string & nsm_ext () const
    {
        return m_nsm_ext;
    }

    const std::string & nsm_url () const
    {
        return m_nsm_url;
    }

    bool dirty () const
    {
        return m_dirty;
    }

    void dirty (bool isdirty);          /* session managers call this one   */

protected:

    void path_name (const std::string & s)
    {
        m_path_name = s;
    }

    void display_name (const std::string & s)
    {
        m_display_name = s;
    }

    void client_id (const std::string & s)
    {
        m_client_id = s;
    }

    void is_active (bool f)
    {
        m_active = f;
    }

    void manager (const std::string & s)
    {
        m_manager = s;
    }

    void capabilities (const std::string & s)
    {
        m_capabilities = s;
    }

protected:

    bool msg_check (int timeoutms = 0);                     /* milliseconds */
    bool lo_is_valid () const;
    void nsm_debug (const std::string & tag);
    void add_client_method (nsm::tag t, lo_method_handler h);
    void add_server_method (nsm::tag t, lo_method_handler h);
    bool send_announcement
    (
        const std::string & appname,
        const std::string & exename,
        const std::string & capabilities
    );
    void start_thread ();
    void stop_thread ();
    void update_dirty_count (bool flag = true);

    // Session client reply methods

    bool open_reply
    (
        nsm::error errorcode = nsm::error::ok,
        const std::string & msg = "No info"
    );
    bool save_reply
    (
        nsm::error errorcode = nsm::error::ok,
        const std::string & msg = "No info"
    );
    bool send_nsm_reply
    (
        const std::string & path,
        nsm::error errorcode,
        const std::string & msg
    );
    bool send
    (
        const std::string & message,
        const std::string & pattern
    );
    bool send_from_client (nsm::tag t);
    bool send_from_client
    (
        nsm::tag t,
        const std::string & s1,
        const std::string & s2,
        const std::string & s3 = ""
    );

    void open_reply (bool loaded)
    {
        open_reply(loaded ? nsm::error::ok : nsm::error::general);
        if (loaded)
            m_dirty = false;
    }

    void save_reply (bool saved)
    {
        save_reply(saved ? nsm::error::ok : nsm::error::general);
        if (saved)
            m_dirty = false;
    }

public:             // virtual methods for callbacks in nsmbase

    virtual void nsm_reply                      /* generic replies */
    (
        const std::string & message,
        const std::string & pattern
    );
    virtual void error (int errcode, const std::string & mesg);

protected:          // virtual methods

    virtual bool progress (float percent);
    virtual bool is_dirty ();
    virtual bool is_clean ();
    virtual bool message (int priority, const std::string & mesg);
    virtual bool initialize ();

    /*
     * Used by the free-function OSC callbacks, and there are too many to make
     * as friends.
     */

public:

    virtual void announce_reply
    (
        const std::string & mesg,
        const std::string & manager,
        const std::string & capabilities
    ) = 0;

    virtual void open
    (
        const std::string & path_name,
        const std::string & display_name,
        const std::string & client_id
    ) = 0;
    virtual void save () = 0;
    virtual void label (const std::string & label) = 0;
    virtual void loaded () = 0;
    virtual void show (const std::string & path) = 0;
    virtual void hide (const std::string & path) = 0;
    virtual void broadcast
    (
        const std::string & message,
        const std::string & pattern,
        const tokenization & argv
    ) = 0;
    virtual bool announce
    (
        const std::string & app_name,
        const std::string & exe_name,
        const std::string & capabilities
    ) = 0;

protected:

    /*
     * Prospective caller helpers a la qtractorMainForm.
     */

    virtual bool open_session ();
    virtual bool save_session ();
    virtual bool close_session ();

#if defined SEQ66_SESSION_DETACHABLE
    virtual bool detach_session ()
    {
        return close_session();
    }
#endif

};          // class nsmbase

}           // namespace seq66

#endif      // SEQ66_NSMBASE_HPP

/*
 * nsmbase.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


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
 * \updates       2020-08-25
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *  Upcoming support for the Non Session Manager.
 */

#include "seq66_features.hpp"
#include "util/basic_macros.h"
#include "nsm/nsmmessagesex.hpp"

#if defined SEQ66_LIBLO_SUPPORT
#include <lo/lo.h>                      /* library for the OSC protocol     */
#endif

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  nsmbase is an NSM OSC server/client base class.
 */

class nsmbase
{

public:

    enum class reply
    {
        ok               =  0,
        general          = -1,
        incompatible_api = -2,
        blacklisted      = -3,
        launch_failed    = -4,
        no_such_file     = -5,
        no_session_open  = -6,
        unsaved_changes  = -7,
        not_now          = -8,
        bad_project      = -9,
        create_failed    = -10
    };

private:

    static std::string sm_nsm_default_ext;

protected:          // private:

    /**
     *  Provides a reference (a void pointer) to an OSC service. See
     *  /usr/include/lo/lo_types.h.
     */

    lo_address m_lo_address;

    /**
     *  Provides a reference (a void pointer) to a thread "containing"
     *  an OSC server. See /usr/include/lo/lo_types.h.
     */

    lo_server_thread m_lo_thread;

    /**
     *  Provides a reference (a void pointer) to an object representing an
     *  an OSC server. See /usr/include/lo/lo_types.h.
     */

    lo_server m_lo_server;

    /**
     *  This item is mutable because it can be falsified if the server and
     *  address are found to be null.
     */

    mutable bool m_active;

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

    bool lo_is_valid () const;

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

    // Session client methods.

    void dirty (bool isdirty);
    void update_dirty_count (bool flag = true);

    virtual void visible (bool isvisible);
    virtual bool progress (float percent);
    virtual bool is_dirty ();
    virtual bool is_clean ();
    virtual bool message (int priority, const std::string & mesg);

    // Session client reply methods.

    bool open_reply (reply replycode = reply::ok);
    bool save_reply (reply replycode = reply::ok);

    void open_reply (bool loaded)
    {
        open_reply(loaded ? reply::ok : reply::general);
        if (loaded)
            m_dirty = false;
    }

    void save_reply (bool saved)
    {
        save_reply(saved ? reply::ok : reply::general);
        if (saved)
            m_dirty = false;
    }

    // Server methods response methods.

    virtual void open
    (
        const std::string & path_name,
        const std::string & display_name,
        const std::string & client_id
    );
    virtual void save ();
    virtual void label (const std::string & label);
    virtual void loaded ();
    virtual void show (const std::string & path);
    virtual void hide (const std::string & path);
    virtual void broadcast (const std::string & path, lo_message msg);
    virtual void nsm_debug (const std::string & tag);

    /*
     * Prospective caller helpers a la qtractorMainForm.
     */

    virtual bool open_session ();
    virtual bool save_session ();
    virtual bool close_session ();

public:

    /*
     * Used by the free-function OSC callbacks.
     */

    virtual bool announce
    (
        const std::string & app_name,
        const std::string & capabilities
    );
    virtual void announce_error (const std::string & mesg);
    virtual void announce_reply
    (
        const std::string & mesg,
        const std::string & manager,
        const std::string & capabilities
    );
    bool nsm_reply (const std::string & path, reply replycode);
    std::string nsm_reply_message (reply replycode);

    void send
    (
        const std::string & message,
        const std::string & pattern
    );
    void send_from_client (nsm::tag t);
    void send_from_client
    (
        nsm::tag t,
        const std::string & s1,
        const std::string & s2,
        const std::string & s3 = ""
    );

protected:

    void add_client_method (nsm::tag t, lo_method_handler h);

};          // class nsmbase

/*
 *  External helper functions.
 */

extern std::string get_nsm_url ();

}           // namespace seq66

#endif      // SEQ66_NSMBASE_HPP

/*
 * nsmbase.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


#if ! defined SEQ66_NSMDUMMY_HPP
#define SEQ66_NSMDUMMY_HPP

/**
 * \file          nsmdummy.hpp
 *
 *    This module provides macros for generating simple messages, MIDI
 *    parameters, and more.
 *
 * \library       seq66
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2020-03-01
 * \updates       2020-03-20
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *  This header file is meant to be used by the application's main module when
 *  NSM support is disabled.
 */

#include "seq66_features.hpp"
#include "util/basic_macros.h"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  nsm is an NSMDUMMY OSC server/client base class.
 */

class nsmbase
{

public:

    nsmbase (const std::string &)
    {
        // no code
    }

    virtual ~nsmbase ()
    {
        // no code
    }

public:

    bool lo_is_valid ()
    {
        return false;
    }

    bool is_active() const
    {
        return false;
    }

    bool is_a_client (const nsm * /*p*/)
    {
        return false;
    }

    bool not_a_client (const nsm * /*p*/)
    {
        return true;
    }

    const std::string & manager () const
    {
        return std::string("");
    }

    const std::string & capabilities () const
    {
        return std::string("");
    }

    const std::string & path_name () const
    {
        return std::string("");
    }

    const std::string & display_name () const
    {
        return std::string("");
    }

    const std::string & client_id () const
    {
        return std::string("");
    }

    const std::string & nsm_file () const
    {
        return std::string("");
    }

    const std::string & nsm_ext () const
    {
        return std::string("");
    }

    const std::string & nsm_url () const
    {
        return std::string("");
    }

    bool dirty () const
    {
        return false;
    }

    void dirty (bool /*isdirty*/)
    {
        // no code
    }

    void update_dirty_count (bool /*flag = true*/)
    {
        // no code
    }

    virtual void visible (bool /*isvisible*/)
    {
        return true;
    }

    virtual void progress (float /*percent*/)
    {
        // no code
    }

    virtual void is_dirty ()
    {
        return false;
    }

    virtual void is_clean ()
    {
        return true;
    }

    virtual void message (int /*priority*/, const std::string & /*mesg*/)
    {
        // no code
    }

    void open_reply (reply replycode = reply::ok);
    void save_reply (reply replycode = reply::ok);

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
    virtual void show ();
    virtual void hide ();
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

    virtual void announce
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
    void nsm_reply (const std::string & path, reply replycode);
    std::string const char * nsm_reply_message (reply replycode);

};          // class nsm

/*
 *  External helper functions.
 */

extern std::string get_nsm_url ();

}           // namespace seq66

#endif      // SEQ66_NSMDUMMY_HPP

/*
 * nsm.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


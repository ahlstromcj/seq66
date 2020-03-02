#if ! defined SEQ66_NSMCLIENT_HPP
#define SEQ66_NSMCLIENT_HPP

/**
 * \file          nsmclient.hpp
 *
 *    This module provides macros for generating simple messages, MIDI
 *    parameters, and more.
 *
 * \library       seq66
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2020-03-01
 * \updates       2020-03-01
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *  Upcoming support for the Non Session Manager.
 */

#include "seq66_features.hpp"

#if defined SEQ66_NSM_SESSION
#include <lo/lo.h>                      /* library for the OSC protocol     */
#include "sessions/nsm.h"               /* copied directly from NSM project */
#endif

#include <memory>                       /* std::unique_ptr<>                */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  nsmclient is an NSM OSC client agent.
 */

class nsmclient
{

public:

	enum class nsmreply
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

private:

#if defined SEQ66_NSM_SESSION
	lo_address m_lo_address;
	lo_server_thread m_lo_thread;
	lo_server m_lo_server;
#endif
	bool m_active;
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

	nsmclient
    (
        const std::string & nsm_url,
        const std::string & nsm_file    = "",
        const std::string & nsm_ext     = ""
    );
	~nsmclient ();

    static std::string default_ext ()
    {
        return sm_nsm_default_ext;
    }

	bool is_active() const              // session activation accessor
    {
        return m_active;
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

	void announce (const std::string & app_name, const std::string & capabilities);
	void dirty (bool is_dirty);
    void update_dirty_count (bool flag = true);
	void visible (bool is_visible);
	void progress (float percent);
	void message (int priority, const std::string & mesg);

	// Session client reply methods.

	void open_reply (nsmreply replycode = nsmreply::ok);
	void save_reply (nsmreply replycode = nsmreply::ok);

	void open_reply (bool ok)
    {
        open_reply(ok ? nsmreply::ok : nsmreply::general);
        if (ok)
            m_dirty = false;
    }

	void save_reply (bool ok)
    {
        save_reply(ok ? nsmreply::ok : nsmreply::general);
        if (ok)
            m_dirty = false;
    }

	// Server methods response methods.

	void announce_error (const std::string & mesg);
	void announce_reply
    (
		const std::string & mesg,
		const std::string & manager,
		const std::string & capabilities
    );

	void nsm_open
    (
		const std::string & path_name,
		const std::string & display_name,
		const std::string & client_id
    );

	void nsm_save ();
	void nsm_loaded ();
	void nsm_show ();
	void nsm_hide ();
    void nsm_debug (const std::string & tag);

    /*
     * Prospective caller helpers a la qtractorMainForm.
     */

    bool open_session ();
    bool save_session ();
    bool close_session ();

protected:

	void nsm_reply (const std::string & path, nsmreply replycode);

/*
 *
signals:                            // Session client callbacks.

	void active (bool is_active);
	void open ();
	void save ();
	void loaded ();
	void show ();
	void hide ();
 *
 */

};          // class nsmclient

/*
 *  External helper functions.
 */

extern std::string get_nsm_url ();
extern std::unique_ptr<nsmclient> create_nsmclient
(
    const std::string & nsmfile,
    const std::string & nsmext
);

#endif      // SEQ66_NSMCLIENT_HPP

}           // namespace seq66

/*
 * nsmclient.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


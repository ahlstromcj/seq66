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
 * \updates       2020-03-08
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *  Upcoming support for the Non Session Manager.
 */

#include <memory>                       /* std::unique_ptr<>                */

#include "nsm/nsm.hpp"                  /* seq66::nsm base class            */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  nsmclient is an NSM OSC client agent.
 */

class nsmclient : public nsm
{

public:

    /**
     *
     * \var none
     *      Indicates no capabilities; provided for completeness or
     *      error-checking.
     *
     * \var cswitch
     *      The client is capable of responding to multiple open messages
     *      without restarting. The string for this value is "switch", but that
     *      is a reserved word in C/C++.
     *
     * \var dirty
     *      The client knows when it has unsaved changes.
     *
     * \var progress
     *      The client can send progress updates during time-consuming
     *      operations.
     *
     * \var message
     *      The client can send textual status updates.
     *
     * \var optional_gui
     *      The client has a optional GUI.
     */

    enum class caps
    {
        none,
        cswitch,
        dirty,
        progress,
        message,
        optional_gui
    };


private:

	std::string m_manager;
	std::string m_capabilities;
	std::string m_path_name;
	std::string m_display_name;
	std::string m_client_id;
    std::string m_nsm_file;
    std::string m_nsm_ext;

public:

	nsmclient
    (
        const std::string & nsm_url,
        const std::string & nsm_file    = "",
        const std::string & nsm_ext     = ""
    );
	~nsmclient ();

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

	// Session client methods.

#if defined THESE_METHODS_ARE_NEEDED
	virtual void announce
    (
        const std::string & app_name,
        const std::string & capabilities
    );
	virtual void visible (bool is_visible);

	// Server methods response methods.

	virtual void announce_error (const std::string & mesg);
	virtual void announce_reply
    (
		const std::string & mesg,
		const std::string & manager,
		const std::string & capabilities
    );
	virtual void nsm_open
    (
		const std::string & path_name,
		const std::string & display_name,
		const std::string & client_id
    );
	virtual void nsm_save ();
	virtual void nsm_loaded ();
	virtual void nsm_label (const std::string & /*label*/);
	virtual void nsm_show ();
	virtual void nsm_hide ();
	virtual void progress (float percent);
	virtual void message (int priority, const std::string & mesg);
    virtual bool save_session ();
    virtual bool close_session ();
#endif  // THESE_METHODS_ARE_NEEDED

    /*
     * Prospective caller helpers a la qtractorMainForm.
     */

    virtual bool open_session ();

protected:

	void nsm_reply (const std::string & path, reply replycode);

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


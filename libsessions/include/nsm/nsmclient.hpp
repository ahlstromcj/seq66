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
 * \updates       2020-08-23
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *  Upcoming support for the Non Session Manager.
 */

#include "nsm/nsmbase.hpp"              /* seq66::nsmbase base class        */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  nsmclient is an NSM OSC client agent.
 */

class nsmclient : public nsmbase
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

public:

    nsmclient
    (
        const std::string & nsm_url,
        const std::string & nsm_file    = "",
        const std::string & nsm_ext     = ""
    );
    virtual ~nsmclient ();

    // Session client methods.

    virtual bool initialize () override;
    virtual void announce_reply
    (
        const std::string & mesg,
        const std::string & manager,
        const std::string & capabilities
    ) override;
    virtual void open
    (
        const std::string & path_name,
        const std::string & display_name,
        const std::string & client_id
    ) override;
    virtual void save () override;
    virtual void loaded () override;
    virtual void label (const std::string & label) override;
    virtual void show (const std::string & path) override;
    virtual void hide (const std::string & path) override;
    virtual void broadcast
    (
        const std::string & message,
        const std::string & pattern,
        const std::vector<std::string> & argv
    ) override;
    virtual bool announce
    (
        const std::string & app_name,
        const std::string & exe_name,
        const std::string & capabilities
    ) override;

    /*
     * Prospective caller helpers a la qtractorMainForm.
     */

    virtual bool open_session ();

protected:

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

extern nsmclient * create_nsmclient
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


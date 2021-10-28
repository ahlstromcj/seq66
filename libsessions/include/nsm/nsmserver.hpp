#if ! defined SEQ66_NSMSERVER_HPP
#define SEQ66_NSMSERVER_HPP

/**
 * \file          nsmserver.hpp
 *
 *    This module provides macros for generating simple messages, MIDI
 *    parameters, and more.
 *
 * \library       seq66
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2020-03-11
 * \updates       2020-08-20
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
 *  nsmbase is an NSM OSC server/client base class.
 */

class nsmserver : public nsmbase
{

public:

    /**
     *
     * \var none
     *      Indicates no capabilities; provided for completeness or
     *      error-checking.
     *
     * \var server_control
     *      The server provides client-to-server control.
     *
     * \var broadcast
     *      The server responds to the "/nsm/server/broadcast" message.
     *
     * \var optional_gui
     *      The server responds to "optional-gui" messages.  If this
     *      capability is not present, then clients with "optional-gui" must
     *      always keep themselves visible.
     */

    enum class caps
    {
        none,
        server_control,
        broadcast,
        optional_gui
    };

    /**
     *  These command values can indicate the pending operation.
     */

    enum class command
    {
        none,
        quit,
        kill,
        save,
        open,
        start,
        close,
        duplicate,
        cnew
    };

private:

    static std::string sm_nsm_default_ext;

public:

	nsmserver (const std::string & nsmurl);
	virtual ~nsmserver ()
    {
        // no code
    }

};          // class nsmserver

/*
 *  External helper functions.
 */

extern std::unique_ptr<nsmserver> create_nsmserver ();

}           // namespace seq66

#endif      // SEQ66_NSMSERVER_HPP

/*
 * nsmserver.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


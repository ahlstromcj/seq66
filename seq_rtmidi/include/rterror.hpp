#if ! defined SEQ66_RTERROR_HPP
#define SEQ66_RTERROR_HPP

/*
 *  This file is part of seq66.
 *
 *  seq66 is free software; you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option) any later
 *  version.
 *
 *  seq66 is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with seq66; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          rterror.hpp
 *
 *  An abstract base class for MIDI error handling.
 *
 * \library       seq66 application
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2021-08-13
 * \license       See above.
 *
 *    In this refactoring...
 */

#include <exception>
#include <string>

#include "util/basic_macros.hpp"

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Exception handling class for rtexmidi.
 *
 *  The rterror class is quite simple but it does allow errors to be
 *  "caught" by rterror::Type. See the rtexmidi documentation to know
 *  which methods can throw an rterror.
 *
 *  Please note that, in this refactoring of rtmidi, we've done away with all
 *  the exception specifications, on the advice of Herb Sutter.  They may be
 *  more relevent to C++11 and beyond, but this library is too small to worry
 *  about them, for now.
 */

class rterror : public std::exception
{

public:

    enum Type
    {
        WARNING,           /**< A non-critical error.                       */
        DEBUG_WARNING,     /**< Non-critical error useful for debugging.    */
        UNSPECIFIED,       /**< The default, unspecified error type.        */
        NO_DEVICES_FOUND,  /**< No devices found on system.                 */
        INVALID_DEVICE,    /**< An invalid device ID was specified.         */
        MEMORY_ERROR,      /**< An error occured during memory allocation.  */
        INVALID_PARAMETER, /**< Invalid parameter specified to a function.  */
        INVALID_USE,       /**< The function was called incorrectly.        */
        DRIVER_ERROR,      /**< A system driver error occured.              */
        SYSTEM_ERROR,      /**< A system error occured.                     */
        THREAD_ERROR,      /**< A thread error occured.                     */
        MAX                /**< An "illegal" value for range-checking       */
    };

private:

    /**
     *  Holds the latest message information for the exception.
     */

    std::string m_message;

    /**
     *  Holds the type or severity of the exception.
     */

    Type m_type;

public:

    rterror
    (
        const std::string & message,
        Type type = rterror::UNSPECIFIED
    ) :
        m_message(message),
        m_type(type)
    {
        // no code
    }

    virtual ~rterror ()
    {
        // no code
    }

    /**
     *  Prints thrown error message to stderr.
     */

    virtual void print_message () const
    {
       infoprint(m_message.c_str());
    }

    /**
     *  Returns the thrown error message type.
     */

    virtual const Type & getType () const
    {
        return m_type;
    }

    /**
     *  Returns the thrown error message string.
     */

    virtual const std::string & get_message () const
    {
        return m_message;
    }

    /**
     *  Returns the thrown error message as a c-style string.
     */

    virtual const char * what () const noexcept override
    {
        return m_message.c_str();
    }

};

/**
 *  rtmidi error callback function prototype.
 *
 *  Note that class behaviour is undefined after a critical error (not
 *  a warning) is reported.
 *
 * \param type
 *      Type of error.
 *
 * \param errorText
 *      Error description.
 */

using rterror_callback = void (*)
(
    rterror::Type type,
    const std::string & errormsg,
    void * userdata
);

}           // namespace seq66

#endif      // SEQ66_RTEXERROR_HPP

/*
 * rterror.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


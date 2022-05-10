#if ! defined SEQ66_FEATURES_HPP
#define SEQ66_FEATURES_HPP

/*
 *  This file is part of seq66.
 *
 *  seq66 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  seq66 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq66; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          seq66_features.hpp
 *
 *    This module summarizes or defines all of the configure and build-time
 *    options available for Seq66.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-24
 * \updates       2022-05-10
 * \license       GNU GPLv2 or above
 *
 *    Provides some useful functions for displaying information about the
 *    application.  More flexible than macros.
 *
 *    Also see the seq66_features.h module.
 */

#include <string>

#include "seq66_features.h"             /* the C-compatible definitions     */

/*
 * This is the main namespace of Seq66.  Do not attempt to
 * Doxygenate the documentation here; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  An indicator of the message level.  Used in the message functions defined
 *  in the basic_macros C++ modules, and elsewhere to specify the level.
 *
 *      -   none. Used only to rever back to no color in message functions.
 *      -   info. Messages that should appear only in verbose mode.
 *      -   warn. Message about problems or statuses that are minor.
 *      -   error. More serious problems.
 *      -   status. Messages that show the progesss of the application even
 *          when not in verbose mode.
 *      -   session. Messages that need to stand out in session management.
 *      -   debug. Message that should appear only in investigate mode.
 */

enum class msglevel
{
    none,           /* default console color    */
    info,           /* blue                     */
    warn,           /* yellow                   */
    error,          /* red                      */
    status,         /* green                    */
    session,        /* cyan                     */
    debug           /* debug                    */
};

/*
 * Global (free) functions.
 */

extern void set_alsa_version (const std::string & v);
extern void set_jack_version (const std::string & v);
extern void set_qt_version (const std::string & v);
extern void set_app_build_os (const std::string & abuild_os);
extern void set_app_build_issue (const std::string & abuild_issue);
extern void set_app_engine (const std::string & aengine);
extern void set_app_name (const std::string & aname);
extern void set_app_type (const std::string & atype);
extern void set_app_cli (bool iscli);
extern void set_arg_0 (const std::string & arg);
extern void set_client_name (const std::string & cname);
extern void set_package_name (const std::string & pname);
extern const std::string & seq_app_name ();
extern const std::string & seq_app_tag ();
extern const std::string & seq_app_type ();
extern bool seq_app_cli ();
extern const std::string & seq_arg_0 ();
extern const std::string & seq_client_name ();
extern const std::string & seq_client_short ();
extern const std::string & seq_icon_name ();
extern bool is_a_tty (int fd);
extern std::string seq_client_tag (msglevel el = msglevel::none);
extern const std::string & seq_package_name ();
extern const std::string & seq_version ();
extern const std::string & seq_version_text ();
extern std::string session_tag (const std::string & refinement = "");
extern std::string seq_build_details ();

}           // namespace seq66

#endif      // SEQ66_FEATURES_HPP

/*
 * seq66_features.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


#if ! defined SEQ66_LASH_HPP
#define SEQ66_LASH_HPP

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
 * \file          lash.hpp
 *
 *  This module declares/defines the base class for LASH support.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2020-06-01
 * \license       GNU GPLv2 or above
 *
 *  Some believe that LASH support is going by the wayside, perhaps being
 *  replaced by NSM (Non Session Manager) or supported well enough at the
 *  application level.
 */

#include "seq66-config.h"                   /* compiler feature macros      */
#include "util/basic_macros.h"          /* platform and debugging macros    */

#if defined SEQ66_LASH_SUPPORT
#include <lash/lash.h>
#endif

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class performer;

/**
 *  This class supports LASH operations, if compiled with LASH support
 *  (i.e. SEQ66_LASH_SUPPORT is defined). All of the if defined skeleton work
 *  is done in this class in such a way that any other part of the code
 *  can use this class whether or not lash support is actually built in;
 *  the functions will just do nothing.
 */

class lash
{

private:

    /**
     *  A hook into the single performer object in the application.
     */

    performer & m_perform;

#if defined SEQ66_LASH_SUPPORT

    /**
     *  Holds the client "handle" returned by the lash_init() function.
     */

    lash_client_t * m_client;

    /**
     *  Holds the command-line arguments used by the lash_init() function.
     */

    lash_args_t * m_lash_args;

#endif

    /**
     *  Indicates if LASH support has been compiled into the library.  Is set
     *  to true if SEQ66_LASH_SUPPORT is defined.  This variable is not used,
     *  but we will keep it around for the possibility of testing LASH support
     *  at run time.
     */

    bool m_is_lash_supported;

public:

    lash (performer & p, int argc, char ** argv);
    void set_alsa_client_id (int id);
    void start ();

#if defined SEQ66_LASH_SUPPORT

    bool process_events ();             // only for gui_assistant

private:

    bool init ();
    void handle_event (lash_event_t * conf);
    void handle_config (lash_config_t * conf);

#endif  // SEQ66_LASH_SUPPORT

};

/*
 *  Global LASH-driver support functions.  See the cpp file for more
 *  information.
 */

extern bool create_lash_driver (performer & p, int argc, char ** argv);
extern lash * lash_driver ();
extern void delete_lash_driver ();

}           // namespace seq66

#endif      // SEQ66_LASH_HPP

/*
 * lash.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


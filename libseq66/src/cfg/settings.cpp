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
 * \file          settings.cpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  and functions in this application.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2016-05-17
 * \updates       2019-07-25
 * \license       GNU GPLv2 or above
 *
 *  The first part of this file defines a couple of global structure
 *  instances, followed by the global variables that these structures
 *  completely replace.
 */

#include "cfg/settings.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Returns a reference to the global rcsettings object.  Why a function
 *  instead of direct variable access?  Encapsulation.  We are then free to
 *  change the way "global" settings are accessed, without changing client
 *  code.
 *
 * \return
 *      Returns the global object g_rcsettings.
 */

rcsettings &
rc ()
{
    static rcsettings s_rcsettings;
    return s_rcsettings;
}

/**
 *  Provides the replacement for all of the other settings in the "user"
 *  configuration file, plus some of the "constants" in the globals module.
 *  Returns a reference to the global usrsettings object, for better
 *  encapsulation.
 *
 * \return
 *      Returns the global object g_usrsettings.
 */

usrsettings &
usr ()
{
    static usrsettings g_usrsettings;
    return g_usrsettings;
}

/**
 *  Call set_defaults() on the "rc" and "usr" objects.
 */

void
set_configuration_defaults ()
{
    rc().set_defaults();
    usr().set_defaults();
}

/**
 *  Common code for handling PPQN settings.  Putting it here means we can
 *  reduce the reliance on the global PPQN, and have a lot more flexibility in
 *  changing the PPQN.
 *
 *  However, this function works completely only if the "user" configuration
 *  file has already been read.  In some cases we may need to retrofit the
 *  desired PPQN value!  Also, the default constructor for
 *  seqmenu::sm_clipboard is called before we read either the configuration
 *  files or the MIDI file, so we don't report issues when that happens.
 *
 * \param ppqn
 *      Provides the PPQN value to be used. The default value is
 *      SEQ66_USE_DEFAULT_PPQN.  Apart from that, the legal values are:
 *
 *      -   SEQ66_USE_FILE_PPQN (0).  Start with usr().file_ppqn().  If this
 *          is zero, there is no MIDI-file PPQN in force, so try
 *          usr().midi_ppqn().  If still 0, then use SEQ66_DEFAULT_PPQN.
 *      -   SEQ66_USE_DEFAULT_PPQN (-1).  This is the default value of this
 *          parameter.  Behavior:
 *          -   If usr().midi_ppqn() is SEQ66_USE_FILE_PPQN (0), try
 *              usr().file_ppqn().
 *          -   Otherwise try the value of usr().midi_ppqn(). This value is
 *              set via command-line options "--ppqn" or in the "usr"
 *              configuration file at the "midi_ppqn" setting.
 *      -   Other PPQN.  Use it unchanged.
 *
 *      If the resultant value is out of the range SEQ66_MINIMUM_PPQN to
 *      SEQ66_MAXIMUM_PPQN, then return SEQ66_DEFAULT_PPQN.
 *
 * \return
 *      Returns the ppqn parameter, unless that parameter is one of the
 *      special values above, or is illegal, as noted above.
 */

int
choose_ppqn (int ppqn)
{
    int result = ppqn;
    if (result == SEQ66_USE_DEFAULT_PPQN)
    {
        if (usr().midi_ppqn() == SEQ66_USE_FILE_PPQN)
            result = usr().file_ppqn();
        else
            result = usr().midi_ppqn();
    }
    else if (result == SEQ66_USE_FILE_PPQN)
    {
        result = usr().file_ppqn();
    }
    if (result < SEQ66_MINIMUM_PPQN || result > SEQ66_MAXIMUM_PPQN)
    {
        if (result)
        {
            warnprint("File PPQN not yet set, setting PPQN = 192");
        }
        else
        {
            warnprint("Provided PPQN out of range, setting PPQN = 192");
        }
        result = SEQ66_DEFAULT_PPQN;                /* the legacy value 192 */
    }
    return result;
}

}           // namespace seq66

/*
 * settings.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


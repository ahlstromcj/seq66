/*
 *  This file is part of seq66, adapted from the PortMIDI project.
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
 * \file        pmerrmm.c
 *
 *      System specific error-messages for the Windows MM API.
 *
 * \library     seq66 application
 * \author      Chris Ahlstrom
 * \date        2018-04-21
 * \updates     2020-07-11
 * \license     GNU GPLv2 or above
 *
 *  Please note that this error module is used only in the pmwinmm.h Windows
 *  module.
 */

#include <windows.h>                    /* UINT, DWORD, and more            */
#include <mmsystem.h>                   /* MMRESULT, MMSYSERR_xxx           */
#include <stdio.h>                      /* C::printf() etc.                 */
#include <stdlib.h>                     /* C::calloc(), C::free()           */

#include "portmidi.h"                   /* for pm_log_buffer functions      */
#include "pmerrmm.h"
#include "util/basic_macros.h"

/**
 *  This printf() stuff is really important for debugging client app w/host
 *  errors.  Probably want to do something else besides read/write from/to
 *  console for portability, however.
 */

/**
 *  Provides error messages for:
 *
 *      -   midiInGetDevCaps(deviceid, lpincaps, szincaps).
 *      -   midiOutGetDevCaps(deviceid, lpincaps, szincaps).
 *
 * \param devicename
 *      The name of the device, either "MIDIMAPPER" or whatever the operating
 *      system discovers.
 */

const char *
midi_io_get_dev_caps_error
(
    const char * devicename,
    const char * functionname,
    MMRESULT errcode
)
{
    static char s_error_storage[PM_STRING_MAX];
    const char * result = "Unknown";
    switch (errcode)
    {
    case MMSYSERR_NOERROR:

        result = "None";
        break;

    case MMSYSERR_ALLOCATED:

        result = "The specified resource is already allocated";
        break;

    case MMSYSERR_BADDEVICEID:

        result = "The specified device identifier is out of range";
        break;

    case MMSYSERR_INVALFLAG:

        result = "The specified flags are invalid";
        break;

    case MMSYSERR_INVALHANDLE:

        result = "The specified device handle is invalid";
        break;

    case MMSYSERR_INVALPARAM:

        result = "The specified pointer or structure parameter is invalid";
        break;

    case MMSYSERR_NODRIVER:

        result = "The driver is not installed";
        break;

    case MMSYSERR_NOMEM:

        result = "The system is unable to allocate or lock memory";
        break;

    case MIDIERR_NODEVICE:

        result = "No MIDI port found (Midi Mapper)";
        break;
    }
    (void) snprintf
    (
        s_error_storage, sizeof s_error_storage,
        "%s() error for device '%s': '%s'\n",
        functionname, devicename, result
    );
    return &s_error_storage[0];
}

/*
 * pmerrmm.c
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */


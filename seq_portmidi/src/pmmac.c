/*
 *  This file is part of seq66, adapted from the PortMIDI project.
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
 * \file        pmmac.c
 *
 *      PortMidi os-dependent code for Mac OSX.
 *
 * \library     seq66 application
 * \author      PortMIDI team; modifications by Chris Ahlstrom
 * \date        2018-05-13
 * \updates     2021-06-23
 * \license     GNU GPLv2 or above

 *  This file needs to implement pm_init(), which calls various routines to
 *  register the available MIDI devices.
 *
 *  This file must be separate from the main portmidi.c file because it is
 *  system dependent, and it is separate from, say, pmmacosxcm.c, because it
 *  might need to register devices for non-CoreMIDI devices.
 */

#include <stdlib.h>

#include "util/basic_macros.h"          /* not_nullptr() macro, etc.        */
#include "portmidi.h"                   /* Pm_set_initialized(), etc.       */
#include "pmutil.h"
#include "pminternal.h"
#include "pmmacosxcm.h"

/**
 *  Provides default ID's for non-existent devices.
 */

PmDeviceID pm_default_input_device_id = -1;
PmDeviceID pm_default_output_device_id = -1;

/**
 *  pm_initialized is set when we return to Pm_Initialize(), but we need it
 *  now in order to (successfully) call Pm_CountDevices().
 */

void
pm_init (void)
{
    PmError err = pm_macosxcm_init();

    /*
     * atexit(pm_exit);
     */

    if (err != pmNoError)
    {
        Pm_set_initialized(FALSE);
    }
    else
    {
        Pm_set_initialized(TRUE);
    }
}

/**
 *  Calls pm_macosxcm_term() to end the PortMidi session.
 */

void
pm_term (void)
{
    pm_macosxcm_term();
}

/**
 *  A simple wrapper for malloc().
 *
 * \param s
 *      Provides the desired size of the allocation.
 *
 * \return
 *      Returns a void pointer to the allocated buffer.
 */

void *
pm_alloc (size_t s)
{
    return malloc(s);
}

/**
 *  The inverse of pm_alloc(), a wrapper for the free(3) function.
 *
 * \param ptr
 *      Provides the pointer to be freed, if it is not null.
 */

void
pm_free (void * ptr)
{
    if (not_nullptr(ptr))
        free(ptr);
}

/*
 * pmmac.c
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */


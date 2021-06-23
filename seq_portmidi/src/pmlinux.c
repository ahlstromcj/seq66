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
 * \file        pmlinux.c
 *
 *      PortMidi os-dependent code for Linux.
 *
 * \library     seq66 application
 * \author      PortMIDI team; modifications by Chris Ahlstrom
 * \date        2017-08-21
 * \updates     2021-06-23
 * \license     GNU GPLv2 or above
 *
 *  This file only needs to implement pm_init(), which calls various routines
 *  to register the available midi devices. This file must be separate from
 *  the main portmidi.c file because it is system dependent, and it is
 *  separate from, pmlinuxalsa.c, because it might need to register non-ALSA
 *  devices as well.
 *
 * Note:
 *
 *      If you add non-ALSA support, you need to fix alsa_poll() in
 *      pmlinuxalsa.c, which assumes all input devices are ALSA.
 */

#include "seq66-config.h"
#include "util/basic_macros.h"          /* not_nullptr() macro, etc.        */
#include "portmidi.h"                   /* Pm_set_initialized(), etc.       */
#include "pmutil.h"
#include "pminternal.h"

#if defined SEQ66_HAVE_LIBASOUND
#include "pmlinuxalsa.h"
#endif

PmDeviceID pm_default_input_device_id = -1;
PmDeviceID pm_default_output_device_id = -1;

/**
 * Note:
 *
 *  It is not an error for ALSA to fail to initialize.  It may be a design
 *  error that the client cannot query what subsystems are working properly
 *  other than by looking at the list of available devices.
 */

void
pm_init ()
{
#if defined SEQ66_HAVE_LIBASOUND
	pm_linuxalsa_init();
#else
#if defined SEQ66_PORTMIDI_NULL          // never defined, at present
    pm_linuxnull_init();
#endif
#endif

    /*
     * This is set when we return to Pm_Initialize, but we need it
     * now in order to (successfully) call Pm_CountDevices().  Ugh.
     * At least we get to assume UTF-8 here, rather than Window's UTF-16.
     */

    Pm_set_initialized(TRUE);
}

/**
 *
 */

void
pm_term (void)
{
#if defined SEQ66_HAVE_LIBASOUND
    pm_linuxalsa_term();
#endif
}

/**
 *  A wrapper for malloc().
 *
 * \param s
 *      Provides the number of bytes to allocate.
 *
 * \return
 *      Returns a pointer to the allocated memory, or a null pointer if the
 *      size parameter is 0.
 */

void *
pm_alloc (size_t s)
{
    return s > 0 ? malloc(s) : nullptr ;
}

/**
 *  A wrapper for free().  It would be nice to be able so see if the pointer
 *  was already freed, as calling free() twice on the same pointer is
 *  undefined.  The caller can guard against this by setting the pointer to
 *  null explicitly after calling this function.
 *
 * \param ptr
 *      The pointer to be freed.  It is ignored if null.
 */

void
pm_free (void * ptr)
{
    if (not_nullptr(ptr))
        free(ptr);
}

/*
 * pmlinux.c
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */


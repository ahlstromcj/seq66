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
 * \file        pmwin.c
 *
 *      PortMidi os-dependent code for Windows.
 *
 * \library     seq66 application
 * \author      PortMIDI team; modifications by Chris Ahlstrom
 * \date        2017-08-21
 * \updates     2021-06-23
 * \license     GNU GPLv2 or above

 *  This file needs to implement pm_init(), which calls various routines to
 *  register the available MIDI devices.
 *
 *  This file must be separate from the main portmidi.c file because it is
 *  system dependent, and it is separate from, say, pmwinmm.c, because it
 *  might need to register devices for WinMM, DirectX, and others.
 */

#include <stdlib.h>

#include "util/basic_macros.h"          /* not_nullptr() macro, etc.        */
#include "portmidi.h"                   /* Pm_set_initialized(), etc.       */
#include "pmutil.h"
#include "pmwinmm.h"
#include "portmidi.h"

#if defined SEQ66_PLATFORM_DEBUG
#include <stdio.h>
#endif

#include <windows.h>

/**
 *  This macro is part of Microsoft's tchar.h, but we want to use it only as
 *  a marker for now.
 *
 *      #include <tchar.h>
 */

#define _T(x)           ((char *)(x))

/**
 *  The maximum length of the name of a device.
 */

#define PATTERN_MAX     256

/**
 *  pm_exit() is called when the program exits.  It calls pm_term() to make
 *  sure PortMidi is properly closed.
 */

static void
pm_exit (void)
{
    pm_term();
}

/*
 *  pm_init() provides the windows-dependent initialization.  It also sets the
 *  atexit() callback to pm_exit().
 */

void
pm_init (void)
{
    atexit(pm_exit);
    pm_winmm_init();

    /*
     * Initialize other APIs (DirectX?) here.  Don't we need to set
     * pm_initialized = TRUE here?  And call find_default_device()?
     * No.
     */
}

/**
 *  Calls pm_winmm_term() to end the PortMidi session.
 */

void
pm_term (void)
{
    pm_winmm_term();
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
 * pmwin.c
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */


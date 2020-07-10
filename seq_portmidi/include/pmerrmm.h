#ifndef SEQ66_PMERRMM_H
#define SEQ66_PMERRMM_H

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
 * \file        pmerrmm.h
 *
 *      Provides system-specific error-messages for Windows.
 *
 * \library     seq66 application
 * \author      Chris Ahlstrom
 * \date        2018-04-21
 * \updates     2020-07-09
 * \license     GNU GPLv2 or above
 *
 */

#include <windows.h>

#include "pminternal.h"

/*
 *  Without this #define, the InitializeCriticalSectionAndSpinCount() function
 *  is undefined.  This version level means "Windows 2000 and higher".
 */

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#if defined __cplusplus
extern "C"
{
#endif

extern void pm_log_buffer_alloc ();
extern void pm_log_buffer_free ();
extern void pm_log_buffer_append (const char * msg);
extern const char * pm_log_buffer ();
extern const char * midi_io_get_dev_caps_error
(
    const char * devicename,
    const char * functionname,
    MMRESULT errcode
);

#if defined __cplusplus
}           // extern "C"
#endif

#endif      // SEQ66_PMERRMM_H

/*
 * pmerrmm.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */


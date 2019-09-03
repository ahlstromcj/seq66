#ifndef SEQ66_PMMAC_H
#define SEQ66_PMMAC_H

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
 * \file        pmmac.h
 *
 *      Device ID functions for Mac OSX.
 *
 * \library     seq66 application
 * \author      PortMIDI team; modifications by Chris Ahlstrom
 * \date        2018-05-13
 * \updates     2018-05-13
 * \license     GNU GPLv2 or above
 *
 *  Provides system-specific definitions for Mac OSX.
 */

#include "pminternal.h"                 /* PmDeviceID typedef               */
#if defined __cplusplus
extern "C"
{
#endif

extern PmDeviceID pm_default_input_device_id;
extern PmDeviceID pm_default_output_device_id;

#if defined __cplusplus
}           // extern "C"
#endif

#endif      // SEQ66_PMMAC_H

/*
 * pmmac.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */


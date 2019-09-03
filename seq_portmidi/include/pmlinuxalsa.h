#ifndef SEQ66_PMLINUXALSA_H
#define SEQ66_PMLINUXALSA_H

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
 * \file        pmlinuxalsa.h
 *
 *  ALSA setup/teardown functions for Linux.
 *
 * \library     seq66 application
 * \author      PortMIDI team; modifications by Chris Ahlstrom
 * \date        2017-08-21
 * \updates     2018-04-10
 * \license     GNU GPLv2 or above
 *
 *  System-specific definitions for the Linux ALSA sub-system. Compare to the
 *  pmwinmm.h header file.
 */

#if defined __cplusplus
extern "C"
{
#endif

extern PmError pm_linuxalsa_init (void);
extern void pm_linuxalsa_term (void);

#if defined __cplusplus
}               // extern "C"
#endif

#endif          // SEQ66_PMLINUXALSA_H

/*
 * pmlinuxalsa.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */


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
 * \file          function_calls_gnu.c
 *
 *  This module provides a function for instrumenting the entry into a function
 *  as supported by the GNU C/C++ compiler.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2022-01-21
 * \updates       2022-01-21
 * \license       GNU GPLv2 or above
 *
 *  Requires the use of gcc's -finstrument-functions option.
 */

#include "seq66_platform_macros.h"

#if defined SEQ66_PLATFORM_GNU

extern void __cyg_profile_func_enter (void *, void *)
    __attribute__((no_instrument_function));

#endif  // defined SEQ66_PLATFORM_GNU

/*
 * function_calls_gnu.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */


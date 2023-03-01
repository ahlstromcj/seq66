#ifndef SEQ66_PORTMIDI_H
#define SEQ66_PORTMIDI_H

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
 * \file        portmidi.h
 *
 *      PortMidi Portable Real-Time MIDI Library, PortMidi API Header File,
 *      Latest version available at:
 *
 *          http://sourceforge.net/projects/portmedia
 *
 * \library     seq66 application
 * \author      PortMIDI team; modifications by Chris Ahlstrom
 * \date        2017-08-21
 * \updates     2023-02-28
 * \license     GNU GPLv2 or above
 *
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
 * Copyright (c) 2001-2006 Roger B. Dannenberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * The text above constitutes the entire PortMidi license; however, the
 * PortMusic community also makes the following non-binding requests:
 *
 * Any person wishing to distribute modifications to the Software is requested
 * to send the modifications to the original developer so that they can be
 * incorporated into the canonical version. It is also requested that these
 * non-binding requests be included along with the license above.
 */

#if defined __cplusplus
extern "C"
{
#endif

#include "pminternal.h"             /* not quite internal now, refactoring  */

/**
 *  A single PortMidiStream is a descriptor for an open MIDI device.
 */

typedef void PortMidiStream;

/**
 *  Ugh.
 */

#define PmStream PortMidiStream

PMEXPORT PmError Pm_Initialize (void);
PMEXPORT PmError Pm_Terminate (void);
PMEXPORT int Pm_HasHostError (PortMidiStream * stream);
PMEXPORT const char * Pm_GetErrorText (PmError errnum);
#if defined PM_GETHOSTERRORTEXT
PMEXPORT void Pm_GetHostErrorText (char * msg, unsigned int len);
#endif
PMEXPORT int Pm_CountDevices (void);

PMEXPORT const PmDeviceInfo * Pm_GetDeviceInfo (PmDeviceID id);
PMEXPORT PmError Pm_OpenInput
(
    PortMidiStream ** stream,
    PmDeviceID inputDevice,
    void * inputDriverInfo,
    int32_t bufferSize,
    PmTimeProcPtr time_proc,
    void * time_info
);
PMEXPORT PmError Pm_OpenOutput
(
    PortMidiStream ** stream,
    PmDeviceID outputDevice,
    void * outputDriverInfo,
    int32_t bufferSize,
    PmTimeProcPtr time_proc,
    void * time_info,
    int32_t latency
);

PMEXPORT void c_millisleep (int ms);

/*
 * Filter bit-mask definitions
 */

/**
 *  filter active sensing messages (0xFE):
 */

#define PM_FILT_ACTIVE (1 << 0x0E)

/**
 *  filter system exclusive messages (0xF0):
 */

#define PM_FILT_SYSEX (1 << 0x00)

/**
 *  filter MIDI clock message (0xF8)
 */

#define PM_FILT_CLOCK (1 << 0x08)

/**
 *  filter play messages (start 0xFA, stop 0xFC, continue 0xFB)
 */

#define PM_FILT_PLAY ((1 << 0x0A) | (1 << 0x0C) | (1 << 0x0B))

/**
 *  filter tick messages (0xF9)
 */

#define PM_FILT_TICK (1 << 0x09)

/**
 *  filter undefined FD messages
 */

#define PM_FILT_FD (1 << 0x0D)

/**
 *  filter undefined real-time messages
 */

#define PM_FILT_UNDEFINED PM_FILT_FD

/**
 *  filter reset messages (0xFF)
 */

#define PM_FILT_RESET (1 << 0x0F)

/**
 *  filter all real-time messages
 */

#define PM_FILT_REALTIME (PM_FILT_ACTIVE | PM_FILT_SYSEX | PM_FILT_CLOCK | \
    PM_FILT_PLAY | PM_FILT_UNDEFINED | PM_FILT_RESET | PM_FILT_TICK)

/**
 *  filter note-on and note-off (0x90-0x9F and 0x80-0x8F
 */

#define PM_FILT_NOTE ((1 << 0x19) | (1 << 0x18))

/**
 *  filter channel aftertouch (most midi controllers use this) (0xD0-0xDF)
 */

#define PM_FILT_CHANNEL_AFTERTOUCH (1 << 0x1D)

/**
 *  per-note aftertouch (0xA0-0xAF)
 */

#define PM_FILT_POLY_AFTERTOUCH (1 << 0x1A)

/**
 *  filter both channel and poly aftertouch
 */

#define PM_FILT_AFTERTOUCH (PM_FILT_CHANNEL_AFTERTOUCH | PM_FILT_POLY_AFTERTOUCH)

/**
 *  Program changes (0xC0-0xCF)
 */

#define PM_FILT_PROGRAM (1 << 0x1C)

/**
 *  Control Changes (CC's) (0xB0-0xBF)
 */

#define PM_FILT_CONTROL (1 << 0x1B)

/**
 *  Pitch Bender (0xE0-0xEF
 */

#define PM_FILT_PITCHBEND (1 << 0x1E)

/**
 *  MIDI Time Code (0xF1)
 */

#define PM_FILT_MTC (1 << 0x01)

/**
 *  Song Position (0xF2)
 */

#define PM_FILT_SONG_POSITION (1 << 0x02)

/**
 *  Song Select (0xF3)
 */

#define PM_FILT_SONG_SELECT (1 << 0x03)

/**
 *  Tuning request (0xF6)
 */

#define PM_FILT_TUNE (1 << 0x06)

/**
 *  All System Common messages (mtc, song position, song select, tune request)
 */

#define PM_FILT_SYSTEMCOMMON (PM_FILT_MTC | PM_FILT_SONG_POSITION | PM_FILT_SONG_SELECT | PM_FILT_TUNE)

PMEXPORT PmError Pm_SetFilter (PortMidiStream * stream, int32_t filters);

#define Pm_Channel(channel)         (1 << (channel))

PMEXPORT PmError Pm_SetChannelMask (PortMidiStream * stream, int mask);
PMEXPORT PmError Pm_Abort (PortMidiStream * stream);
PMEXPORT PmError Pm_Close (PortMidiStream * stream);
PMEXPORT int Pm_Read
(
    PortMidiStream * stream, PmEvent * buffer, int32_t length
);
PMEXPORT PmError Pm_Synchronize (PortMidiStream * stream);
PMEXPORT PmError Pm_Poll (PortMidiStream * stream);
PMEXPORT PmError Pm_Write
(
    PortMidiStream * stream, PmEvent * buffer, int32_t length
);
PMEXPORT PmError Pm_WriteShort
(
    PortMidiStream * stream, PmTimestamp when, int32_t msg
);
PMEXPORT PmError Pm_WriteSysEx
(
    PortMidiStream * stream, PmTimestamp when, midibyte_t * msg
);

/*
 * New section for accessing static options in the portmidi module.
 */

PMEXPORT void Pm_set_initialized (int flag);
PMEXPORT int Pm_initialized (void);

PMEXPORT void Pm_set_exit_on_error (int flag);
PMEXPORT int Pm_exit_on_error (void);

PMEXPORT void Pm_set_show_debug (int flag);
PMEXPORT int Pm_show_debug (void);

PMEXPORT void Pm_set_error_present (int flag);
PMEXPORT int Pm_error_present (void);

PMEXPORT void Pm_set_hosterror_text (const char * msg);
PMEXPORT const char * Pm_hosterror_text (void);
PMEXPORT char * Pm_hosterror_text_mutable (void);
PMEXPORT void Pm_set_hosterror (int flag);
PMEXPORT int Pm_hosterror ();

PMEXPORT const char * Pm_error_message (void);
PMEXPORT int Pm_device_opened (int deviceid);
PMEXPORT int Pm_device_count (void);
PMEXPORT void Pm_print_devices (void);

/*
 * New section for writing messages to a log buffer for better debugging.  We
 * need to be able to trace what's happening, and not just see the first error
 * that cropped up.
 */

PMEXPORT void pm_log_buffer_alloc (void);
PMEXPORT void pm_log_buffer_free (void);
PMEXPORT void pm_log_buffer_append (const char * msg);
PMEXPORT const char * pm_log_buffer (void);

#if defined __cplusplus
}           // extern "C"
#endif

#endif      // SEQ66_PORTMIDI_H

/*
 * pminternal.h
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */


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
 * \file        pmwinmm.c
 *
 *      System specific definitions for the Windows MM API.
 *
 * \library     seq66 application
 * \author      PortMIDI team; modifications by Chris Ahlstrom
 * \date        2017-08-21
 * \updates     2023-05-19
 * \license     GNU GPLv2 or above
 *
 *  Check out this site:
 *
 *      http://donyaquick.com/midi-on-windows/
 *      "Working with MIDI on Windows (Outside of a DAW)"
 */

#include "seq66_platform_macros.h"      /* compilation environment settings */

#if defined SEQ66_PLATFORM_MSVC         /* based on _MSC_VER                */
#pragma warning(disable: 4133)          /* stop implicit typecast warnings  */
#endif

/*
 *  Without this #define, InitializeCriticalSectionAndSpinCount is undefined.
 *  This version level means "Windows 2000 and higher".
 */

#if ! defined _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

/*
 *  The former is defined in seq66_features.h (included by basic_macros.h),
 *  but what about the latter?  Sequencer64 defines the former!!!
 *
 *      SEQ66_USE_SYSEX_PROCESSING?
 *      SEQ66_USE_SYSEX_BUFFERS
 */

#include <windows.h>
#include <mmsystem.h>
#include <string.h>

#include "pmerrmm.h"                    /* Windows error support, debugging */
#include "pmutil.h"
#include "pmwinmm.h"
#include "portmidi.h"                   /* UNUSED() macro, and PortMidi API */
#include "porttime.h"
#include "util/basic_macros.h"          /* not_nullptr() macro, features    */

/*
 *  Asserts are used to verify PortMidi code logic is sound; later may want
 *  something more graceful.
 *
 *      #include <assert.h>
 */

/*
 *  WinMM API callback routines.
 */

static void CALLBACK winmm_in_callback
(
    HMIDIIN hmi, WORD wmsg,
    DWORD_PTR dwInstance,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2
);

static void CALLBACK winmm_streamout_callback
(
    HMIDIOUT hmo, UINT wmsg,
    DWORD_PTR dwInstance,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2
);

/*
 * Note that this macro is undefined, as it is in the original PortMidi library.
 * Do not confuse it with the SEQ66_USE_SYSEX_PROCESSING macro in
 * seq66_features.h, which *is* defined.
 */

#if defined SEQ66_USE_SYSEX_PROCESSING_NOT_IN_USE

static void CALLBACK winmm_out_callback
(
    HMIDIOUT hmo, UINT wmsg,
    DWORD_PTR dwInstance,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2
);

#endif

static void winmm_out_delete (PmInternal * midi);
extern pm_fns_node pm_winmm_in_dictionary;
extern pm_fns_node pm_winmm_out_dictionary;

/**
 * \note
 *      WinMM seems to hold onto buffers longer than one would expect, e.g.
 *      when I tried using 2 small buffers to send long SysEx messages, at
 *      some point WinMM held both buffers. This problem was fixed by making
 *      buffers bigger. Therefore, it seems that there should be enough buffer
 *      space to hold a whole SysEx message.
 *
 *  The bufferSize passed into Pm_OpenInput (passed into here as buffer_len)
 *  will be used to estimate the largest SysEx message (= buffer_len * 4
 *  bytes).  Call that the max_sysex_len = buffer_len * 4.
 *
 *  For simple midi output (latency == 0), allocate 3 buffers, each with half
 *  the size of max_sysex_len, but each at least 256 bytes.
 *
 *  For stream output, there will already be enough space in very short
 *  buffers, so use them, but make sure there are at least 16.
 *
 *  For input, use many small buffers rather than 2 large ones so that when
 *  there are short SysEx messages arriving frequently (as in control surfaces)
 *  there will be more free buffers to fill. Use max_sysex_len / 64 buffers,
 *  but at least 16, of size 64 bytes each.
 *
 *  The following constants help to represent these design parameters.
 */

#define NUM_SYSEX_BUFFERS             4     /* not in original PortMID lib! */
#define SYSEX_BYTES_PER_BUFFER      128     /* ditto!!!!                    */
#define NUM_SIMPLE_SYSEX_BUFFERS      3
#define MIN_SIMPLE_SYSEX_LEN        256
#define MIN_STREAM_BUFFERS           16
#define STREAM_BUFFER_LEN            24
#define INPUT_SYSEX_LEN              64
#define MIN_INPUT_BUFFERS            16

/**
 *  If we run out of space for output (assuming this is due to a SysEx
 *  message), then expand the output buffer by up to NUM_EXPANSION_BUFFERS, in
 *  increments of EXPANSION_BUFFER_LEN.
 */

#define NUM_EXPANSION_BUFFERS       128
#define EXPANSION_BUFFER_LEN       1024

/**
 *  A SysEx buffer has 3 DWORDS as a header plus the actual message size.
 *
 * \warning
 *      The size was assumed to be long, we changed it to DWORD.
 */

#define MIDIHDR_SYSEX_BUFFER_LENGTH(x) ((x) + sizeof(DWORD) * 3)

/**
 *  A MIDIHDR with a SysEx message is the buffer length plus the header size.
 */

#define MIDIHDR_SYSEX_SIZE(x) (MIDIHDR_SYSEX_BUFFER_LENGTH(x) + sizeof(MIDIHDR))

#if defined SEQ66_USE_SYSEX_PROCESSING  // SEQ66_USE_SYSEX_BUFFERS

/**
 *  Size of a MIDIHDR with a buffer contaning multiple MIDIEVENT structures.
 */

#define MIDIHDR_SIZE(x) ((x) + sizeof(MIDIHDR))

#endif

/**
 *  win32 multi-media-system-specific structure passed to MIDI callbacks;
 *  it is global WinMM device informations.
 *
 *  typedef struct
 *  {
 *      WORD    wMid;
 *      WORD    wPid;
 *      VERSION vDriverVersion;
 *      char    szPname[MAXPNAMELEN];
 *      DWORD   dwSupport;
 *  } MIDIINCAPS;
 *
 *  typedef struct
 *  {
 *      WORD    wMid;
 *      WORD    wPid;
 *      VERSION vDriverVersion;
 *      char    szPname[MAXPNAMELEN];
 *      WORD    wTechnology;
 *      WORD    wVoices;
 *      WORD    wNotes;
 *      WORD    wChannelMask;
 *      DWORD   dwSupport;
 *  } MIDIOUTCAPS;
 *
 * wMid Manufacturer identifier of the device driver for the MIDI device.
 *
 * wPid Product identifier of the MIDI device.
 *
 * vDriverVersion Version number of the device driver. High/Low-order bytes
 * are the major/minor version numbers.
 *
 * szPname[MAXPNAMELEN] Product name in a null-terminated string.
 *
 * wTechnology Type of the MIDI device, one of the following:
 *
 *      MOD_UNKNOWN (0).
 *      MOD_MIDIPORT (1). MIDI hardware port.
 *      MOD_SYNTH (2). Synthesizer.
 *      MOD_SQSYNTH (3). Square wave synthesizer.
 *      MOD_FMSYNTH (4). FM synthesizer.
 *      MOD_MAPPER (5). Microsoft MIDI mapper.
 *      MOD_WAVETABLE (6). Hardware wavetable synthesizer.
 *      MOD_SWSYNTH (7). Software synthesizer.
 *
 * wVoices Number of voices supported by internal synthesizer. If a port,
 * this vale is set to 0.
 *
 * wNotes Maximum number of notes that can be played by an internal device.
 * If a port, it is set to 0.
 *
 * wChannelMask Channels that an internal device responds to, LSB = 0, MSB = 15.
 * Port devices that transmit on all channels set this member to 0xFFFF.
 *
 * dwSupport Optional functionality supported by the device. One or more of
 * the following:
 *
 *      MIDICAPS_CACHE Supports patch caching.
 *      MIDICAPS_LRVOLUME Supports separate left and right volume control.
 *      MIDICAPS_STREAM Provides direct support for the midiStreamOut function.
 *      MIDICAPS_VOLUME Supports volume control.
 *
 */

static MIDIINCAPS * midi_in_caps = nullptr;
static MIDIINCAPS midi_in_mapper_caps;
static UINT midi_num_inputs = 0;
static int midi_input_index = 0;

static MIDIOUTCAPS * midi_out_caps = nullptr;
static MIDIOUTCAPS midi_out_mapper_caps;
static UINT midi_num_outputs = 0;
static int midi_output_index = 0;

/**
 *  The MIDI_MAPPER descriptor number (-1), defined in mmsystem.h, is another
 *  example of Microsoft's clumsy approach to C code dating back decades.
 *  PortMidi compounds it by not adopting a pointer for the descriptor
 *  universally. Weird stuff.
 */

static void * PTR_MIDIMAPPER    = ((void *) ((uintptr_t) MIDI_MAPPER));
static UINT UINT_MIDIMAPPER     = ((UINT) MIDI_MAPPER);

/**
 *  This complex structure provides per-device information.
 */

typedef struct midiwinmm_struct
{
    union
    {
        HMIDISTRM stream;   /**< Windows handle for stream.                 */
        HMIDIOUT out;       /**< Windows handle for out calls.              */
        HMIDIIN in;         /**< Windows handle for in calls.               */

    } handle;

    /**
     * MIDI output messages are sent in these buffers, which are allocated
     * in a round-robin fashion, using next_buffer as an index.
     */

    LPMIDIHDR * buffers;    /**< Pool of buffers for midi in or out data.   */
    int max_buffers;        /**< Length of buffers array.                   */
    int buffers_expanded;   /**< Buffers array expanded for extra messages? */
    int num_buffers;        /**< How many buffers allocated in the array.   */
    int next_buffer;        /**< Index of next buffer to send.              */
    HANDLE buffer_signal;   /**< Used to wait for buffer to become free.    */

    /*
     * SysEx buffers will be allocated only when
     * a SysEx message is sent. The size of the buffer is fixed.
     */

#if defined SEQ66_USE_SYSEX_PROCESSING  // SEQ66_USE_SYSEX_BUFFERS
    LPMIDIHDR sysex_buffers[NUM_SYSEX_BUFFERS]; /**< Pool for SysEx data.   */
    int next_sysex_buffer;      /**< Index of next SysEx buffer to send.    */
#endif

    unsigned long last_time;    /**< Last output time.                      */
    int first_message;          /**< Flag: treat first message differently. */
    int sysex_mode;             /**< Middle of sending SysEx.               */
    unsigned long sysex_word;   /**< Accumulate data when receiving SysEx.  */
    unsigned sysex_byte_count;  /**< Count how many SysEx bytes received.   */
    LPMIDIHDR hdr;              /**< Message accumulating SysEx to send (?) */
    unsigned long sync_time;    /**< When did we last determine delta?      */
    long delta;                 /**< Stream time minus real time.           */
    int error;                  /**< Host error from doing port MIDI call.  */
    CRITICAL_SECTION lock;      /**< Prevents reentrant callbacks (input).  */

} midiwinmm_node, * midiwinmm_type;

/**
 *  General MIDI device input queries.  If we can't open a particular
 *  system-level MIDI interface (WinMM), we consider that system/API
 *  unavailable and move on without an error.  This function is called third
 *  at startup.
 *
 *  In the original PortMidi implementation, midiInGetDevCaps() and
 *  pm_add_devices are called for devices 0 to midiInGetNumDevs()-1.  Here,
 *  since we may have already gotten an input mapper, we add to the device
 *  index appropriately.
 */

static void
pm_winmm_general_inputs (void)
{
    midi_num_inputs = midiInGetNumDevs();                           /* UINT */
    midi_in_caps = midi_num_inputs > 0 ?
        pm_alloc(sizeof(MIDIINCAPS) * midi_num_inputs) : nullptr ;

    if (is_nullptr(midi_in_caps))                       /* see banner notes */
    {
        pm_log_buffer_append("No MIDI input devices found.\n");
    }
    else
    {
        char temp[PM_STRING_MAX];
        int ins = (int) midi_num_inputs;
#if defined SEQ66_PLATFORM_64_BIT
        UINT_PTR in;
#else
        UINT in;
#endif
        int index;
        snprintf(temp, sizeof temp, "%d MIDI inputs found\n", ins);
        pm_log_buffer_append(temp);
        for
        (
            in = 0, index = midi_input_index;
            in < (UINT_PTR) midi_num_inputs;
            ++in, ++index
        )
        {
            WORD winerrcode = midiInGetDevCaps
            (
                (UINT) in,                              /* 0 and up         */
                (LPMIDIINCAPS) &midi_in_caps[in],
                sizeof(MIDIINCAPS)
            );
            const char * devname = (const char *) midi_in_caps[in].szPname;
            if (winerrcode == MMSYSERR_NOERROR)
            {
                (void) pm_add_device                    /* ignore errors    */
                (
                    "MMSystem",                         /* subsystem name   */
                    (char *) devname,                   /* interface name   */
                    TRUE,                               /* it is an input   */
                    (void *) in,                        /* void* descriptor */
                    &pm_winmm_in_dictionary,            /* pm_fns_type      */
                    index,                              /* client number    */
                    index                               /* port number      */
                );
                snprintf
                (
                    temp, sizeof temp, "[%d] MIDI input dev %d: '%s'\n",
                    index, (int) in, devname
                );
                infoprint(temp);                        /* log to console   */
            }
            else
            {
                const char * name = strlen(devname) == 0 ?
                    "Unknown Input" : devname ;

                const char * errmsg = midi_io_get_dev_caps_error
                (
                    name, "Input Devices: midiInGetDevCaps", winerrcode
                );
                snprintf
                (
                    temp, sizeof temp, "[%d] '%s' dev %d: error '%s'\n",
                    index, name, (int) in, errmsg
                );
                errprint(temp);                         /* log to console   */
            }
            pm_log_buffer_append(temp);                 /* log to buffer    */
        }
    }
}

/**
 *  Tries to open the Windows MIDI Mapper, which is represented by the value
 *  MIDI_MAPPER = (-1), which is supposed to be a legal value.  If valid,
 *  what client and port numbers to we want to use?  However, it is not
 *  valid.  This function is called first.  Is it still useless if the Coolsoft
 *  MIDI Mapper is installed?
 *
 * \note
 *      If MIDI_MAPPER opened as input (the documentation implies you can,
 *      but the current system fails to retrieve input mapper capabilities),
 *      then we still should retrieve some form of setup information.
 */

static void
pm_winmm_mapper_input (void)
{
#if defined USE_THIS_TEST_CODE
    UINT count = midiInGetNumDevs();            /* ca 2023-05-12    */
    if (count == 0)
        return;
#endif

    WORD winerrcode = midiInGetDevCaps
    (
        UINT_MIDIMAPPER,
        (LPMIDIINCAPS) &midi_in_mapper_caps,
        sizeof(MIDIINCAPS)
    );
    char temp[PM_STRING_MAX];
    const char * devname = (const char *) midi_in_mapper_caps.szPname;
    if (winerrcode == MMSYSERR_NOERROR)
    {
        (void) pm_add_device
        (
            "MMSystem",
            (char *) devname,
            TRUE,
            PTR_MIDIMAPPER,
            &pm_winmm_in_dictionary,
            midi_input_index, midi_input_index          /* client/port 0    */
        );
        ++midi_input_index;
        snprintf(temp, sizeof temp, "MIDI Mapper Input: '%s'\n", devname);
    }
    else
    {
        const char * errmsg = midi_io_get_dev_caps_error
        (
            devname, "MIDI Mapper: midiInGetDevCaps", winerrcode
        );
        snprintf(temp, sizeof temp, "'%s' error: '%s'\n", devname, errmsg);
        errprint(temp);                                 /* log to console   */
    }
    pm_log_buffer_append(temp);                         /* log to buffer    */
}

/**
 *  General MIDI device output queries.
 */

static void
pm_winmm_general_outputs (void)
{
    midi_num_outputs = midiOutGetNumDevs();                         /* UINT */
    midi_out_caps = midi_num_outputs > 0 ?
        pm_alloc(sizeof(MIDIOUTCAPS) * midi_num_outputs) : nullptr ;

    if (is_nullptr(midi_out_caps))
    {
        pm_log_buffer_append("No MIDI output devices found.\n");
    }
    else
    {
        char temp[PM_STRING_MAX];
        int outs = (int) midi_num_outputs;
#if defined SEQ66_PLATFORM_64_BIT
        UINT_PTR out;
#else
        UINT out;
#endif
        int index;
        snprintf(temp, sizeof temp, "%d MIDI outputs found\n", outs);
        pm_log_buffer_append(temp);
        for
        (
            out = 0, index = midi_output_index;
            out < (UINT) midi_num_outputs;
            ++out, ++index
        )
        {
            DWORD winerrcode = midiOutGetDevCaps
            (
                (UINT) out,                             /* 0 and up         */
                (LPMIDIOUTCAPS) &midi_out_caps[out],
                sizeof(MIDIOUTCAPS)
            );
            const char * devname = (const char *) midi_out_caps[out].szPname;
            if (winerrcode == MMSYSERR_NOERROR)
            {
                snprintf
                (
                    temp, sizeof temp, "[%d] MIDI output dev %d: '%s'\n",
                    index, (int) out, devname
                );
                (void) pm_add_device
                (
                    "MMSystem",
                    (char *) devname,
                    FALSE,
                    (void *) out,
                    &pm_winmm_out_dictionary,
                    index,
                    index
                );
            }
            else
            {
                const char * name = strlen(devname) == 0 ?
                    "Unknown Output" : devname ;

                const char * errmsg = midi_io_get_dev_caps_error
                (
                    name, "Output Devices : midiOutGetDevCaps", winerrcode
                );
                snprintf
                (
                    temp, sizeof temp, "[%d] '%s' dev %d: error '%s'\n",
                    index, name, (int) out, errmsg
                );
                errprint(temp);                             /* log to console   */
            }
            pm_log_buffer_append(temp);                     /* log to buffer    */
        }
    }
}

/**
 *  If MIDI_MAPPER is opened as output (a pseudo MIDI device that maps
 *  device-independent messages into device dependent ones, via the Windows
 *  NT midimapper program), we still should get some setup information.  This
 *  function is called second, after the mapper-input function.
 */

static void
pm_winmm_mapper_output (void)
{
#if defined USE_THIS_TEST_CODE
    UINT count = midiOutGetNumDevs();                       /* ca 2023-05-12    */
    if (count == 0)
        return;
#endif

    WORD winerrcode = midiOutGetDevCaps
    (
        UINT_MIDIMAPPER,
        (LPMIDIOUTCAPS) &midi_out_mapper_caps,
        sizeof(MIDIOUTCAPS)
    );
    char temp[PM_STRING_MAX];
    const char * devname = (const char *) midi_out_mapper_caps.szPname;
    if (winerrcode == MMSYSERR_NOERROR)
    {
        (void) pm_add_device
        (
            "MMSystem",
            (char *) devname,
            FALSE,
            PTR_MIDIMAPPER,
            &pm_winmm_out_dictionary,
            midi_output_index, midi_output_index        /* client/port 0    */
        );
        ++midi_output_index;
        snprintf(temp, sizeof temp, "Mapper output '%s'\n", devname);
    }
    else
    {
        const char * errmsg = midi_io_get_dev_caps_error
        (
            devname, "Mapper Out: midiOutGetDevCaps", winerrcode
        );
        snprintf(temp, sizeof temp, "[%s] %s\n", devname, errmsg);
        errprint(temp);                                 /* log to console   */
    }
    pm_log_buffer_append(temp);                         /* log to buffer    */
}

/*
 * Host error handling.
 */

/**
 *  Check the descriptor for an error status.
 */

static unsigned
winmm_has_host_error (PmInternal * midi)
{
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    return m->error;
}

/**
 *  str_copy_len() is like strcat, but it won't overrun the destination
 *  string.  Just in case the suffix is greater then len, terminate with
 *  zero.
 *
 * \return
 *      Length of resulting string.
 */

static int
str_copy_len (char * dst, char * src, int len)
{
    strncpy(dst, src, len);
    dst[len - 1] = 0;
    return strlen(dst);
}

/**
 *  Note that input and output use different WinMM API calls.  Make sure there
 *  is an open device (m) to examine. If there is an error, then read and
 *  record the host error.  Note that the error codes returned by the
 *  get-error-text functions are for that function.  We disable those asserts
 *  here.
 *
 *  The precondition is that midi != null.
 */

static void
winmm_get_host_error (PmInternal * midi, char * msg, UINT len)
{
    midiwinmm_node * m = not_nullptr(midi) ?
        (midiwinmm_node *) midi->descriptor : nullptr ;

    if (not_nullptr_2(m, msg))
    {
        char * hdr1 = "Host error: ";
        msg[0] = 0;                         /* set result string to empty   */
        if (pm_descriptors[midi->device_id].pub.input)
        {
            if (m->error != MMSYSERR_NOERROR)
            {
                int n = str_copy_len(msg, hdr1, len);
                int err = midiInGetErrorText(m->error, msg + n, len - n);
                if (err == MMSYSERR_NOERROR)
                    m->error = MMSYSERR_NOERROR;
            }
        }
        else                                /* output port                  */
        {
            if (m->error != MMSYSERR_NOERROR)
            {
                int n = str_copy_len(msg, hdr1, len);
                int err = midiOutGetErrorText(m->error, msg + n, len - n);
                if (err == MMSYSERR_NOERROR)
                    m->error = MMSYSERR_NOERROR;
            }
        }
    }
}

/**
 *  Buffer handling.
 */

static MIDIHDR *
allocate_buffer (long data_size)
{
    LPMIDIHDR hdr = nullptr;
    if (data_size > 0)
    {
        hdr = (LPMIDIHDR) pm_alloc(MIDIHDR_SYSEX_SIZE(data_size));
        if (not_nullptr(hdr))
        {
            MIDIEVENT * evt = (MIDIEVENT *)(hdr + 1); /* placed after header */
            hdr->lpData = (LPSTR) evt;
            hdr->dwBufferLength = MIDIHDR_SYSEX_BUFFER_LENGTH(data_size);
            hdr->dwBytesRecorded = hdr->dwFlags = 0;
            hdr->dwUser = hdr->dwBufferLength;
        }
    }
    return hdr;
}

#if defined SEQ66_USE_SYSEX_PROCESSING  // SEQ66_USE_SYSEX_BUFFERS

/**
 * we're actually allocating more than data_size because the buffer
 * will include the MIDIEVENT header in addition to the data
 */

static MIDIHDR *
allocate_sysex_buffer (long data_size)
{
    LPMIDIHDR hdr = nullptr;
    if (data_size > 0)
    {
        hdr = (LPMIDIHDR) pm_alloc(MIDIHDR_SYSEX_SIZE(data_size));
        if (not_nullptr(hdr))
        {
            MIDIEVENT * evt = (MIDIEVENT *)(hdr + 1);   /* placed after header */
            hdr->lpData = (LPSTR) evt;
            hdr->dwFlags = hdr->dwUser = 0;
        }
    }
    return hdr;
}

#endif

/**
 *  Buffers is an array of 'count' pointers to an MIDIHDR/MIDIEVENT struct.
 */

static PmError
allocate_buffers (midiwinmm_type m, long data_size, long count)
{
    int i;
    m->num_buffers = 0;             /* in case no memory can be allocated   */
    m->buffers = (LPMIDIHDR *) pm_alloc(sizeof(LPMIDIHDR) * count);
    if (is_nullptr(m->buffers))
        return pmInsufficientMemory;

    m->max_buffers = count;
    for (i = 0; i < count; ++i)
    {
        LPMIDIHDR hdr = allocate_buffer(data_size);
        if (is_nullptr(hdr))        /* free all allocations and return      */
        {
            for (i = i - 1; i >= 0; --i)
            {
                pm_free(m->buffers[i]);
                m->buffers[i] = nullptr;
            }
            pm_free(m->buffers);    /* zero out the pointers?               */
            m->buffers = nullptr;
            m->max_buffers = 0;
            return pmInsufficientMemory;
        }
        m->buffers[i] = hdr;        /* this may be null if allocation fails */
    }
    m->num_buffers = count;
    return pmNoError;
}

#if defined SEQ66_USE_SYSEX_PROCESSING  // SEQ66_USE_SYSEX_BUFFERS

/**
 * sysex_buffers is an array of count pointers to MIDIHDR/MIDIEVENT struct
 */

static PmError
allocate_sysex_buffers (midiwinmm_type m, long data_size)
{
    PmError rslt = pmNoError;
    int i;
    for (i = 0; i < NUM_SYSEX_BUFFERS; ++i)
    {
        LPMIDIHDR hdr = allocate_sysex_buffer(data_size);
        if (is_nullptr(hdr))
            rslt = pmInsufficientMemory;

        m->sysex_buffers[i] = hdr;      /* may be null if allocation fails  */
        if (not_nullptr(hdr))
            hdr->dwFlags = 0;           /* mark as free                     */
    }
    return rslt;
}

/* static */ LPMIDIHDR
get_free_sysex_buffer (PmInternal * midi)
{
    LPMIDIHDR r = nullptr;
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    if (! m->sysex_buffers[0])
    {
        if (allocate_sysex_buffers(m, SYSEX_BYTES_PER_BUFFER))
            return nullptr;
    }
    for (;;)                            /* busy wait to find a free buffer  */
    {
        int i;
        for (i = 0; i < NUM_SYSEX_BUFFERS; ++i)
        {
            /* cycle through buffers, modulo NUM_SYSEX_BUFFERS */

            m->next_sysex_buffer++;
            if (m->next_sysex_buffer >= NUM_SYSEX_BUFFERS)
                m->next_sysex_buffer = 0;

            r = m->sysex_buffers[m->next_sysex_buffer];
            if ((r->dwFlags & MHDR_PREPARED) == 0)
                goto found_sysex_buffer;
        }

        /*
         * After scanning every buffer and not finding anything, block.
         */

        if (WaitForSingleObject(m->buffer_signal, 1000) == WAIT_TIMEOUT)
        {
            warnprint
            (
                "PortMidi warning: get_free_sysex_buffer() wait timed "
                "out after 1000 ms\n"
            );
        }
    }

found_sysex_buffer:

    r->dwBytesRecorded = 0;
    r->dwBufferLength = 0; /* changed to correct value later */
    return r;
}

#endif

/**
 *  1. Cycle through buffers, modulo m->num_buffers.
 *  2. Expand SysEx buffer.  If we're trying to send a sysex message, maybe the
 *     message is too big and we need more message buffers.  Expand the buffer
 *     pool by 128KB using 1024-byte buffers.  First, expand the buffers array if
 *     necessary.
 *  3. No Memory.  If no memory, we could return a no-memory error, but user
 *     probably will be unprepared to deal with it. Maybe the MIDI driver is
 *     temporarily hung so we should just wait.  I don't know the right answer,
 *     but waiting is easier.
 *  4. Just Keep Polling. Otherwise, we've allocated all NUM_EXPANSION_BUFFERS
 *     buffers, and we have no free buffers to send. We'll just keep polling to
 *     see if any buffers show up.
 */

static LPMIDIHDR
get_free_output_buffer (PmInternal * midi)
{
    LPMIDIHDR r = nullptr;
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    for (;;)
    {
        int i;
        for (i = 0; i < m->num_buffers; ++i)
        {
            m->next_buffer++;                       /* 1. Cycle through     */
            if (m->next_buffer >= m->num_buffers)
                m->next_buffer = 0;

            r = m->buffers[m->next_buffer];
            if ((r->dwFlags & MHDR_PREPARED) == 0)
                goto found_buffer;
        }

        /*
         *  After scanning every buffer and not finding anything, block.
         */

        if (WaitForSingleObject(m->buffer_signal, 1000) == WAIT_TIMEOUT)
        {
            warnprint
            (
                "PortMidi warning: get_free_output_buffer() "
                " wait timed out after 1000ms\n"
            );
            if (! m->buffers_expanded)              /* 2. Expand SysEx buff */
            {
                int buffcount = m->num_buffers + NUM_EXPANSION_BUFFERS;
                size_t sz = buffcount * sizeof(LPMIDIHDR);
                LPMIDIHDR * new_buffers = (LPMIDIHDR *) pm_alloc(sz);
                if (is_nullptr(new_buffers))        /* 3. No Memory         */
                {
                    c_millisleep(1);
                    continue;
                }

                /*
                 * Copy buffers to new_buffers and replace buffers.
                 */

                sz = m->num_buffers * sizeof(LPMIDIHDR);
                memcpy(new_buffers, m->buffers, sz);
                pm_free(m->buffers);
                m->buffers = new_buffers;
                m->max_buffers = buffcount;
                m->buffers_expanded = TRUE;
            }

            /*
             * Next, add one buffer and return it.
             */

            if (m->num_buffers < m->max_buffers)
            {
                r = allocate_buffer(EXPANSION_BUFFER_LEN);
                if (is_nullptr(r))                  /* 3. No Memory (again) */
                {
                    c_millisleep(1);
                    continue;
                }
                m->buffers[m->num_buffers++] = r;
                goto found_buffer;                  /* break out of 2 loops */
            }

            /*
             * 4. Just Keep Polling.  We also add a 1 millisecond wait
             *    to see if we can reduce CPU usage.
             */

            c_millisleep(1);
        }
    }

found_buffer:

    /*
     * Actual buffer length is saved in dwUser field.
     */

    r->dwBytesRecorded = 0;
    r->dwBufferLength = (DWORD) r->dwUser;
    return r;
}

#if defined EXPANDING_SYSEX_BUFFERS

/*
 * Note:
 *
 *      This is not working code, but might be useful if you want to grow
 *      sysex buffers.
 *
 *      Buffer must be smaller than 64k, but be also a multiple of 4.
 */

static PmError
resize_sysex_buffer (PmInternal * midi, long old_size, long new_size)
{
    LPMIDIHDR big;
    int i;
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    if (new_size > 65520)
    {
        if (old_size >= 65520)
            return pmBufferMaxSize;
        else
            new_size = 65520;
    }
    big = allocate_sysex_buffer(new_size);  /* allocate a bigger message    */
    if (is_nullptr(big))
        return pmInsufficientMemory;

    m->error = midiOutPrepareHeader(m->handle.out, big, sizeof(MIDIHDR));
    if (m->error)
    {
        pm_free(big);
        return pmHostError;
    }

    /*
     * Make sure we're not going to overwrite any memory.
     */

    if (old_size <= new_size)
        memcpy(big->lpData, m->hdr->lpData, old_size);

    /*
     * Keep track of how many sysex bytes are in message so far.
     * Then find out which buffer this was, and replace it.
     */

    big->dwBytesRecorded = m->hdr->dwBytesRecorded;
    big->dwBufferLength = new_size;
    for (i = 0; i < NUM_SYSEX_BUFFERS; ++i)
    {
        if (m->sysex_buffers[i] == m->hdr)
        {
            m->sysex_buffers[i] = big;
            m->sysex_buffer_size[i] = new_size;
            pm_free(m->hdr);
            m->hdr = big;
            break;
        }
    }
    return pmNoError;
}

#endif

/*
 * Begin MIDI input implementation
 */

static PmError
allocate_input_buffer (HMIDIIN h, long buffer_len)
{
    LPMIDIHDR hdr = allocate_buffer(buffer_len);
    if (is_nullptr(hdr))
        return pmInsufficientMemory;

    Pm_set_hosterror(midiInPrepareHeader(h, hdr, sizeof(MIDIHDR)));
    if (Pm_hosterror())
    {
        pm_free(hdr);
        return Pm_hosterror();
    }
    Pm_set_hosterror(midiInAddBuffer(h, hdr, sizeof(MIDIHDR)));
    return Pm_hosterror();
}

/**
 *  Should use UINT: DWORD dwDevice = (DWORD) descriptors[i].descriptor;
 *
 * \note
 *      If we return an error code, the device will be closed and memory will be
 *      freed. It's up to the caller to free the parameter "midi".
 */

static PmError
winmm_in_open (PmInternal * midi, void * driverInfo)
{
    int i;
    midiwinmm_type m;
    int max_sysex_len = midi->buffer_len * 4;
    int num_input_buffers = max_sysex_len / INPUT_SYSEX_LEN;
    int devid = midi->device_id;
    int client = pm_descriptors[devid].pub.client;
    int port = pm_descriptors[devid].pub.port;

#if defined SEQ66_PLATFORM_64_BIT
    UINT_PTR dev = (UINT_PTR) pm_descriptors[devid].descriptor;
    UINT dwDevice = (UINT) (dev & 0xFFFFFFFF);
#else                                       /* warnings with 64 bit builds: */
    UINT dwDevice = (UINT) pm_descriptors[devid].descriptor;
#endif

    if (dwDevice == UINT_MIDIMAPPER)
    {
        pm_log_buffer_append("Opening MIDI Mapper for input\n");
    }
    else if (dwDevice < 32)                 /* just a mere sanity check     */
    {
        char temp[64];
        snprintf
        (
            temp, sizeof temp, "Opening MIDI input device %d (%d:%d)\n",
            (int) dwDevice, client, port
        );
        pm_log_buffer_append(temp);
    }
    else
    {
        pm_log_buffer_append("MIDI input device ID out-of-range\n");
    }
    if (not_nullptr(driverInfo))
    {
        // Anything worth doing here?
    }

    /*
     * Create system dependent device data.
     */

    m = (midiwinmm_type) pm_alloc(sizeof(midiwinmm_node));      /* create */
    midi->descriptor = m;
    if (is_nullptr(m))
        goto no_memory;

    m->handle.in = nullptr;
    m->buffers = nullptr;                   /* not used for input */
    m->num_buffers = 0;                     /* not used for input */
    m->max_buffers = FALSE;                 /* not used for input */
    m->buffers_expanded = 0;                /* not used for input */
    m->next_buffer = 0;                     /* not used for input */
    m->buffer_signal = 0;                   /* not used for input */

#if defined SEQ66_USE_SYSEX_PROCESSING  // SEQ66_USE_SYSEX_BUFFERS
    for (i = 0; i < NUM_SYSEX_BUFFERS; ++i)
        m->sysex_buffers[i] = nullptr;      /* not used for input */

    m->next_sysex_buffer = 0;               /* not used for input */
#endif

    m->last_time = 0;
    m->first_message = TRUE;                /* not used for input */
    m->sysex_mode = FALSE;
    m->sysex_word = 0;
    m->sysex_byte_count = 0;
    m->hdr = nullptr;                       /* not used for input */
    m->sync_time = 0;
    m->delta = 0;
    m->error = MMSYSERR_NOERROR;

    /*
     * 4000 is based on Windows documentation -- that's the value used in the
     * memory manager. It's small enough that it should not hurt performance
     * even if it's not optimal.
     */

    InitializeCriticalSectionAndSpinCount(&m->lock, 4000);
    Pm_set_hosterror
    (
        midiInOpen
        (
            &(m->handle.in),                /* input device handle      */
            dwDevice,                       /* device ID                */
            (DWORD_PTR) winmm_in_callback,  /* callback address         */
            (DWORD_PTR) midi,               /* callback instance data   */
            CALLBACK_FUNCTION               /* callback is a procedure  */
        )
    );
    if (Pm_hosterror())
        goto free_descriptor;

    if (num_input_buffers < MIN_INPUT_BUFFERS)
        num_input_buffers = MIN_INPUT_BUFFERS;

    for (i = 0; i < num_input_buffers; ++i)
    {
        if (allocate_input_buffer(m->handle.in, INPUT_SYSEX_LEN))
        {
            /*
             * Either Pm_hosterror() was set, or the proper return code is
             * pmInsufficientMemory.
             */

            goto close_device;
        }
    }
    Pm_set_hosterror(midiInStart(m->handle.in));       /* start device */
    if (Pm_hosterror())
        goto reset_device;

    return pmNoError;

    /*
     * Undo steps leading up to the detected error.  At the same time, we
     * ignore return codes because we already have an error to report.
     */

reset_device:

    (void) midiInReset(m->handle.in);

close_device:

    (void) midiInClose(m->handle.in);

free_descriptor:

    midi->descriptor = nullptr;
    pm_free(m);

no_memory:

    if (Pm_hosterror())
    {
        int err = midiInGetErrorText
        (
            Pm_hosterror(), Pm_hosterror_text_mutable(), PM_HOST_ERROR_MSG_LEN
        );
        if (err != MMSYSERR_NOERROR)
        {
            // Anything worth doing here to handle the error?
        }
        return pmHostError;
    }

    /*
     * if ! Pm_hosterror(), then the error must be pmInsufficientMemory.
     * Note: if we return an error code, the device will be closed and memory
     * will be freed. It's up to the caller to free the parameter midi.
     */

    return pmInsufficientMemory;
}

static PmError
winmm_in_poll (PmInternal * midi)
{
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    return m->error;
}

/**
 * Closes an open MIDI input device.  It assumes that the MIDI parameter is not
 * null (checked by the caller).
 */

static PmError
winmm_in_close(PmInternal * midi)
{
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    if (is_nullptr(m))
        return pmBadPtr;

    int err = midiInStop(m->handle.in);     /* device to be closed          */
    if (err != pmNoError)
    {
        midiInReset(m->handle.in);          /* try to reset and close port  */
        midiInClose(m->handle.in);
        Pm_set_hosterror(err);
    }
    else if ((err = midiInReset(m->handle.in)) != pmNoError)
    {
        midiInClose(m->handle.in);      /* best effort to close midi port   */
        Pm_set_hosterror(err);
    }
    else
    {
        Pm_set_hosterror(midiInClose(m->handle.in));
    }
    midi->descriptor = nullptr;
    DeleteCriticalSection(&m->lock);
    pm_free(m);
    if (Pm_hosterror() != pmNoError)
    {
        int err = midiInGetErrorText
        (
            Pm_hosterror(),
            Pm_hosterror_text_mutable(),
            PM_HOST_ERROR_MSG_LEN
        );
        if (err != MMSYSERR_NOERROR)
        {
            // handle the error
        }
        return pmHostError;
    }
    return pmNoError;
}

/**
 *  Callback function executed via midiInput SW interrupt [via midiInOpen()].
 *  NOTE: we do not just EnterCriticalSection() here because an MIM_CLOSE message
 *  arrives when the port is closed, but then the m->lock has been destroyed.
 *
 *  If this callback is reentered with data, we're in trouble.  It's hard to
 *  imagine that Microsoft would allow callbacks to be reentrant -- isn't the
 *  model that this is like a hardware interrupt? -- but I've seen reentrant
 *  behavior using a debugger, so it happens.
 *
 *  dwParam1 is MIDI data received, packed into DWORD w/ 1st byte of message LOB;
 *  dwParam2 is time message received by input device driver, specified in [ms]
 *  from when midiInStart called.  each message is expanded to include the status
 *  byte.
 *
 *  If dwParam1 is not a status byte; ignore it. This happened running the sysex.c
 *  test under Win2K with MidiMan USB 1x1 interface, but I can't reproduce it.
 *  -RBD
 */

static void CALLBACK
winmm_in_callback
(
    HMIDIIN hMidiIn,            /* midiInput device Handle                  */
    WORD wMsg,                  /* MIDI msg                                 */
    DWORD_PTR dwInstance,       /* application data                         */
    DWORD_PTR dwParam1,         /* MIDI data                                */
    DWORD_PTR dwParam2          /* device timestamp (re recent midiInStart) */
)
{
    PmInternal * midi = (PmInternal *) dwInstance;
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    EnterCriticalSection(&m->lock);
    switch (wMsg)
    {
    case MIM_DATA:              /* see the notes above about re-entry etc.  */
    {
        if ((dwParam1 & 0x80) == 0)
        {
            /*
             * See RBD's note above.
             */
        }
        else                    /* we have data to process                  */
        {
            PmEvent event;
            if (midi->time_proc)
                dwParam2 = (*midi->time_proc)(midi->time_info);

            event.timestamp = (PmTimestamp) dwParam2;
            event.message = (PmMessage) dwParam1;
            pm_read_short(midi, &event);
        }
        break;
    }
    case MIM_LONGDATA:
    {
        MIDIHDR * lpMidiHdr = (MIDIHDR *) dwParam1;
        midibyte_t * data = (midibyte_t *) lpMidiHdr->lpData;
        unsigned processed = 0;
        int remaining = lpMidiHdr->dwBytesRecorded;
        if (midi->time_proc)
            dwParam2 = (*midi->time_proc)(midi->time_info);

        /*
         * Can there be more than 1 message in a buffer?  Assume yes and
         * iterate through them.
         */

        while (remaining > 0)
        {
            unsigned amt = pm_read_bytes
            (
                midi, data + processed, remaining, dwParam2
            );
            remaining -= amt;
            processed += amt;

            /*
             * EXPERIMENTAL
             */

            if (amt == 0)
                break;
        }

        /*
         * When a device is closed, the pending MIM_LONGDATA buffers are
         * returned to this callback with dwBytesRecorded == 0. In this
         * case, we do not want to send them back to the interface (if
         * we do, the interface will not close, and Windows OS may (!!!) hang).
         * Note: no error checking.  I don't think this can fail except
         * possibly for MMSYSERR_NOMEM, but the pain of reporting this
         * unlikely but probably catastrophic error does not seem worth it.
         */

        if (lpMidiHdr->dwBytesRecorded > 0)
        {
            MMRESULT rslt;
            lpMidiHdr->dwBytesRecorded = 0;
            lpMidiHdr->dwFlags = 0;
            rslt = midiInPrepareHeader(hMidiIn, lpMidiHdr, sizeof(MIDIHDR));
            if (rslt == MMSYSERR_NOERROR)
                rslt = midiInAddBuffer(hMidiIn, lpMidiHdr, sizeof(MIDIHDR));

            if (rslt != MMSYSERR_NOERROR)
            {
                // handle the error
            }
        }
        else
        {
            midiInUnprepareHeader(hMidiIn, lpMidiHdr, sizeof(MIDIHDR));
            pm_free(lpMidiHdr);
        }
        break;
    }
    case MIM_OPEN:
        break;

    case MIM_CLOSE:
        break;

    case MIM_ERROR:
        {
            errprint("MIM_ERROR\n");
        }
        break;

    case MIM_LONGERROR:
        {
            errprint("MIM_LONGERROR\n");
        }
        break;

    default:
        break;
    }
    LeaveCriticalSection(&m->lock);
}

/*
 * MIDI output implementation.
 *
 *  This section begins the helper routines used by midiOutStream interface.
 */

/**
 *  Adds timestamped short msg to buffer, and returns fullp.
 *
 * Huh?
 *      If the addition of three more words (a message) would extend beyond
 *      the buffer length, then return TRUE (full).
 */

static int
add_to_buffer
(
    /* midiwinmm_type m, */
    LPMIDIHDR hdr, unsigned long delta, unsigned long msg
)
{
    unsigned long * ptr = (unsigned long *) (hdr->lpData + hdr->dwBytesRecorded);
    *ptr++ = delta;                                     /* dwDeltaTime  */
    *ptr++ = 0;                                         /* dwStream     */
    *ptr++ = msg;                                       /* dwEvent      */
    hdr->dwBytesRecorded += 3 * sizeof(long);
    return hdr->dwBytesRecorded + 3 * sizeof(long) > hdr->dwBufferLength;
}

static PmTimestamp
pm_time_get (midiwinmm_type m)
{
    MMTIME mmtime;
    MMRESULT winerrcode;
    mmtime.wType = TIME_TICKS;
    mmtime.u.ticks = 0;
    winerrcode = midiStreamPosition(m->handle.stream, &mmtime, sizeof(mmtime));
    return winerrcode == MMSYSERR_NOERROR ?  mmtime.u.ticks : 0 ;
}

/*
 * End helper routines used by midiOutStream interface
 */

static PmError
winmm_out_open (PmInternal * midi, void * UNUSED(driverinfo))
{
    midiwinmm_type m;
    MIDIPROPTEMPO propdata;
    MIDIPROPTIMEDIV divdata;
    int output_buffer_len;
    int num_buffers;
    int max_sysex_len = midi->buffer_len * 4;
    int devid = midi->device_id;
    int client = pm_descriptors[devid].pub.client;
    int port = pm_descriptors[devid].pub.port;

#if defined SEQ66_PLATFORM_64_BIT
    UINT_PTR dev = (UINT_PTR) pm_descriptors[devid].descriptor;
    UINT dwDevice = (UINT) (dev & 0xFFFFFFFF);
#else                                       /* warnings with 64 bit builds: */
    UINT dwDevice = (UINT) pm_descriptors[devid].descriptor;
#endif

    if (dwDevice == UINT_MIDIMAPPER)
    {
        pm_log_buffer_append("Opening the MIDI Mapper for output\n");
    }
    else if (dwDevice < 32)            /* sanity check */
    {
        char temp[64];
        snprintf
        (
            temp, sizeof temp, "Opening MIDI output device #%d (%d:%d)\n",
            (int) dwDevice, client, port
        );
        pm_log_buffer_append(temp);
    }
    else
    {
        pm_log_buffer_append("MIDI output device ID out-of-range\n");
    }

    /*
     * Create system dependent device data.
     */

    m = (midiwinmm_type) pm_alloc(sizeof(midiwinmm_node)); /* create */
    midi->descriptor = m;
    if (is_nullptr(m))
        goto no_memory;

    m->handle.out = nullptr;
    m->buffers = nullptr;
    m->num_buffers = m->max_buffers = m->next_buffer = 0;
    m->buffers_expanded = FALSE;

#if defined SEQ66_USE_SYSEX_PROCESSING  // SEQ66_USE_SYSEX_BUFFERS
    m->sysex_buffers[0] = nullptr;
    m->sysex_buffers[1] = nullptr;
    m->next_sysex_buffer = 0;
#endif

    m->last_time = 0;
    m->first_message = TRUE; /* we treat first message as special case */
    m->sysex_mode = FALSE;
    m->sysex_word = m->sysex_byte_count = 0;
    m->hdr = nullptr;
    m->sync_time = m->delta = 0;
    m->error = MMSYSERR_NOERROR;

    /*
     * Create a signal.  this should only fail when there are very serious
     * problems.
     */

    m->buffer_signal = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (is_nullptr(m->buffer_signal))
    {
        errprint("Failed to create signal for output"); // What to do???
    }
    if (midi->latency == 0)                             /* open the device  */
    {
        /*
         * Use simple MIDI out calls.  Note that winmm_streamout_callback() is
         * Note: the same callback function as for StreamOpen().  Please note that
         * the error codes returned by Microsoft functions don't line up
         * completely with the values in the PmError enumeration.
         */

        Pm_set_hosterror
        (
            midiOutOpen
            (
                (LPHMIDIOUT) & m->handle.out,           /* device Handle    */
                dwDevice,                               /* device ID        */
                (DWORD_PTR) winmm_streamout_callback,   /* callback fn      */
                (DWORD_PTR) midi,                       /* cb instance data */
                CALLBACK_FUNCTION                       /* callback type    */
            )
        );
    }
    else
    {
        /*
         * Use stream-based MIDI output (schedulable in future).
         */

        Pm_set_hosterror
        (
            midiStreamOpen
            (
                &m->handle.stream,                      /* device Handle    */
                (LPUINT) &dwDevice,                     /* device ID ptr    */
                1,                                      /* must = 1         */
                (DWORD_PTR) winmm_streamout_callback,
                (DWORD_PTR) midi,                       /* callback data    */
                CALLBACK_FUNCTION
            )
        );
    }
    if (Pm_hosterror() != pmNoError)                    /* MMSYSERR_NOERROR */
        goto free_descriptor;

    if (midi->latency == 0)
    {
        num_buffers = NUM_SIMPLE_SYSEX_BUFFERS;
        output_buffer_len = max_sysex_len / num_buffers;
        if (output_buffer_len < MIN_SIMPLE_SYSEX_LEN)
            output_buffer_len = MIN_SIMPLE_SYSEX_LEN;
    }
    else
    {
        num_buffers = max(midi->buffer_len, midi->latency / 2);
        if (num_buffers < MIN_STREAM_BUFFERS)
            num_buffers = MIN_STREAM_BUFFERS;

        output_buffer_len = STREAM_BUFFER_LEN;
        propdata.cbStruct = sizeof(MIDIPROPTEMPO);
        propdata.dwTempo = Pt_Get_Tempo_Microseconds(); /* us per quarter   */
        infoprintf("tempo in us = %ld\n", (long) propdata.dwTempo);
        Pm_set_hosterror
        (
            midiStreamProperty
            (
                m->handle.stream, (LPBYTE) & propdata,
                MIDIPROP_SET | MIDIPROP_TEMPO
            )
        );
        if (Pm_hosterror() != pmNoError)
            goto close_device;

        divdata.cbStruct = sizeof(MIDIPROPTEMPO);
        divdata.dwTimeDiv = Pt_Get_Ppqn();              /* divs per 1/4     */
        infoprintf("time div = %ld\n", (long) divdata.dwTimeDiv);
        Pm_set_hosterror
        (
            midiStreamProperty
            (
                m->handle.stream, (LPBYTE) & divdata,
                MIDIPROP_SET | MIDIPROP_TIMEDIV
            )
        );
        if (Pm_hosterror() != pmNoError)
            goto close_device;
    }
    if (allocate_buffers(m, output_buffer_len, num_buffers))
        goto free_buffers;

    if (midi->latency != 0)                             /* start the device */
    {
        Pm_set_hosterror(midiStreamRestart(m->handle.stream));
        if (Pm_hosterror() != pmNoError)                /* MMSYSERR_NOERROR */
            goto free_buffers;
    }
    return pmNoError;

free_buffers:               /* buffers freed below by winmm_out_delete()    */
close_device:

    midiOutClose(m->handle.out);

free_descriptor:

    midi->descriptor = nullptr;
    winmm_out_delete(midi);                             /* free buffers & m */

no_memory:

    if (Pm_hosterror() != pmNoError)
    {
        int err = midiOutGetErrorText
        (
            Pm_hosterror(), Pm_hosterror_text_mutable(), PM_HOST_ERROR_MSG_LEN
        );
        if (err != MMSYSERR_NOERROR)
        {
            // log the error
        }
        return pmHostError;
    }
    return pmInsufficientMemory;
}

/**
 *  Carefully frees the data associated with MIDI.  It deletes
 *  system-dependent device data.  We don't report errors here, better not
 *  to stop cleanup.
 */

static void
winmm_out_delete (PmInternal * midi)
{
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    if (not_nullptr(m))
    {
        int i;
        if (m->buffer_signal)
            CloseHandle(m->buffer_signal);

        /* if using stream output, free buffers */

        for (i = 0; i < m->num_buffers; ++i)
        {
            if (m->buffers[i])
            {
                pm_free(m->buffers[i]);
                m->buffers[i] = nullptr;
            }
        }
        m->num_buffers = 0;
        pm_free(m->buffers);
        m->buffers = nullptr;
        m->max_buffers = 0;

#if defined SEQ66_USE_SYSEX_PROCESSING  // SEQ66_USE_SYSEX_BUFFERS

        /*
         * Free the sysex buffers.
         */

        for (i = 0; i < NUM_SYSEX_BUFFERS; ++i)
            if (m->sysex_buffers[i]) pm_free(m->sysex_buffers[i]);
#endif

    }
    midi->descriptor = nullptr;
    pm_free(m);                         /* delete */
}

/*
 *  See the comments for winmm_in_close().
 */

static PmError
winmm_out_close (PmInternal * midi)
{
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    if (m->handle.out)
    {
        if (midi->latency == 0)
            Pm_set_hosterror(midiOutClose(m->handle.out));
        else
            Pm_set_hosterror(midiStreamClose(m->handle.stream));

        winmm_out_delete(midi);     /* regardless of outcome, free memory */
    }
    if (Pm_hosterror())
    {
        int err = midiOutGetErrorText
        (
            Pm_hosterror(),
            (char *) Pm_hosterror_text_mutable(),
            PM_HOST_ERROR_MSG_LEN
        );
        if (err != MMSYSERR_NOERROR)
        {
            // handle the error
        }
        return pmHostError;
    }
    return pmNoError;
}

/**
 *  This function aborts, and only stops output streams.
 */

static PmError
winmm_out_abort (PmInternal * midi)
{
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    m->error = MMSYSERR_NOERROR;
    if (midi->latency > 0)
        m->error = midiStreamStop(m->handle.stream);

    return m->error ? pmHostError : pmNoError;
}

/**
 *  Bytes recorded should be 0.  As pointed out by Nigel Brown, 20Sep06,
 *  dwBytesRecorded should be zero. This is set in get_free_sysex_buffer().  The
 *  msg length goes in dwBufferLength in spite of what Microsoft documentation
 *  says (or doesn't say).
 */

static PmError
winmm_write_flush
(
    PmInternal * midi,
    PmTimestamp timestamp        // unused
)
{
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    if (timestamp != 0)
    {
        // Just avoids a warning.
    }
    if (not_nullptr(m) && not_nullptr(m->hdr))
    {
        m->error = midiOutPrepareHeader(m->handle.out, m->hdr, sizeof(MIDIHDR));
        if (m->error)
        {
            /* do not send message */
        }
        else if (midi->latency == 0)
        {
            /*
             * Bytes recorded should be 0.
             */

            m->hdr->dwBufferLength = m->hdr->dwBytesRecorded;
            m->hdr->dwBytesRecorded = 0;
            m->error = midiOutLongMsg(m->handle.out, m->hdr, sizeof(MIDIHDR));
        }
        else
        {
            m->error = midiStreamOut(m->handle.stream, m->hdr, sizeof(MIDIHDR));
        }
        midi->fill_base = nullptr;
        m->hdr = nullptr;
        if (m->error)
        {
            m->hdr->dwFlags = 0; /* release the buffer */
            return pmHostError;
        }
    }
    return pmNoError;
}

#if defined GARBAGE

static PmError
winmm_write_sysex_byte (PmInternal * midi, midibyte_t byte)
{
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    midibyte_t * msg_buffer;

    /*
     * At the beginning of SysEx, m->hdr is null.  Allocate a buffer if none
     * allocated yet.
     */

    if (is_nullptr(m->hdr))
    {
        m->hdr = get_free_output_buffer(midi);
        if (is_nullptr(m->hdr))
            return pmInsufficientMemory;

        m->sysex_byte_count = 0;
    }

    msg_buffer = (midibyte_t *) (m->hdr->lpData);       /* where to write   */
    if (m->hdr->lpData != (char *) (m->hdr + 1))
    {
        // What to do with this error?
    }
    if (m->sysex_byte_count >= m->hdr->dwBufferLength)  /* check overflow   */
    {
        /* allocate a bigger message -- double it every time */

        LPMIDIHDR big = allocate_buffer(m->sysex_byte_count * 2);
        if (is_nullptr(big))
            return pmInsufficientMemory;

        m->error = midiOutPrepareHeader(m->handle.out, big, sizeof(MIDIHDR));
        if (m->error)
        {
            m->hdr = nullptr;
            return pmHostError;
        }
        memcpy(big->lpData, msg_buffer, m->sysex_byte_count);
        msg_buffer = (midibyte_t *)(big->lpData);
        if (m->buffers[0] == m->hdr)
        {
            m->buffers[0] = big;
            pm_free(m->hdr);
            m->hdr = nullptr;
        }
        else if (m->buffers[1] == m->hdr)
        {
            m->buffers[1] = big;
            pm_free(m->hdr);
            m->hdr = nullptr;
        }
        m->hdr = big;
    }
    msg_buffer[m->sysex_byte_count++] = byte;   /* append byte to message   */
    if (byte == MIDI_EOX)                       /* have a complete message? */
    {
        m->hdr->dwBytesRecorded = m->sysex_byte_count;
        m->error = midiOutLongMsg(m->handle.out, m->hdr, sizeof(MIDIHDR));
        m->hdr = nullptr;                       /* stop using message buffer */
        if (m->error)
            return pmHostError;
    }
    return pmNoError;
}

#endif  // GARBAGE

static PmError
winmm_write_short (PmInternal * midi, PmEvent * event)
{
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    PmError rslt = pmNoError;
    if (is_nullptr(m))
        return pmHostError;

    if (midi->latency == 0)   /* use midiOut interface, ignore timestamps */
    {
        m->error = midiOutShortMsg(m->handle.out, event->message);
        if (m->error)
            rslt = pmHostError;
    }
    else      /* use midiStream interface -- pass data through buffers */
    {
        unsigned long when = event->timestamp;
        unsigned long delta;
        int full;
        if (when == 0)
            when = midi->now;

        /*
         * 'when' is in real_time; translate it to the intended stream time.
         * Make sure we don't go backward in time
         */

        when += m->delta + midi->latency;
        if (when < m->last_time)
            when = m->last_time;

        delta = when - m->last_time;
        m->last_time = when;

        /*
         * Before we insert any data, we must have a buffer.
         * Stream interface buffers are allocated when stream is opened.
         */

        if (is_nullptr(m->hdr))
            m->hdr = get_free_output_buffer(midi);

        full = add_to_buffer(/*m,*/ m->hdr, delta, event->message);
        if (full)
            rslt = winmm_write_flush(midi, when);
    }
    return rslt;
}

#define winmm_begin_sysex winmm_write_flush

#ifndef winmm_begin_sysex

static PmError
winmm_begin_sysex (PmInternal * midi, PmTimestamp timestamp)
{
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    PmError rslt = pmNoError;

    if (midi->latency == 0)
    {
        /* do nothing -- it's handled in winmm_write_byte */
    }
    else
    {
        /*
         * SysEx expects an empty SysEx buffer, so send whatever is here.
         */

        rslt = winmm_write_flush(midi);
    }
    return rslt;
}

#endif

static PmError
winmm_end_sysex (PmInternal * midi, PmTimestamp timestamp)
{
    /*
     * Could check for callback_error here, but I haven't checked what happens
     * if we exit early and don't finish the sysex msg and clean up.
     */

    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    PmError rslt = pmNoError;
    LPMIDIHDR hdr = m->hdr;
    if (! hdr)
        return rslt;

    /*
     * Something bad happened earlier, do not report an error because it would
     * have been reported (at least) once already.  A(n old) version of MIDI
     * YOKE requires a zero byte after the sysex message, but do not increment
     * dwBytesRecorded.
     */

    hdr->lpData[hdr->dwBytesRecorded] = 0;
    if (midi->latency == 0)
    {

#if defined DEBUG_PRINT_BEFORE_SENDING_SYSEX /* DEBUG CODE: */
        {
            int i;
            int len = m->hdr->dwBufferLength;
            printf("OutLongMsg %d ", len);
            for (i = 0; i < len; ++i)
                printf("%2x ", (midibyte_t)(m->hdr->lpData[i]));

            printf("\n");
        }
#endif
    }
    else
    {
        /*
         * Using stream interface. There are accumulated bytes in m->hdr to
         * send using midiStreamOut.  add bytes recorded to MIDIEVENT length,
         * but don't count the MIDIEVENT data (3 longs)
         */

        MIDIEVENT * evt = (MIDIEVENT *) (hdr->lpData);
        evt->dwEvent += hdr->dwBytesRecorded - 3 * sizeof(long);

        /* round up BytesRecorded to multiple of 4 */

        hdr->dwBytesRecorded = (hdr->dwBytesRecorded + 3) & ~3;
    }
    rslt = winmm_write_flush(midi, timestamp);
    return rslt;
}

/**
 *  Write a sysex byte.  What a long function!
 */

static PmError
winmm_write_byte
(
    PmInternal * midi, midibyte_t byte, PmTimestamp timestamp
)
{
    PmError rslt = pmNoError;
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;   // assert(m);
    LPMIDIHDR hdr = m->hdr;
    midibyte_t * msg_buffer;
    if (is_nullptr(hdr))
    {
        m->hdr = hdr = get_free_output_buffer(midi);
        if (is_nullptr(hdr))
        {
            // handle the error
        }
        midi->fill_base = (midibyte_t *) m->hdr->lpData;
        midi->fill_offset_ptr = (uint32_t *)(&(hdr->dwBytesRecorded));

        /*
         * When buffer fills, Pm_WriteSysEx() will revert to calling
         * pmwin_write_byte(), which expects to have space, so leave one byte
         * free for pmwin_write_byte(). Leave another byte of space for zero
         * after message to make early version of MIDI YOKE driver happy --
         * therefore dwBufferLength - 2
         */

        midi->fill_length = hdr->dwBufferLength - 2;
        if (midi->latency != 0)
        {
            unsigned long when = (unsigned long) timestamp;
            unsigned long delta;
            unsigned long * ptr;
            if (when == 0)
                when = midi->now;

            /*
             * 'when' is in real_time; translate it to the intended stream
             * time Then make sure we don't go backward in time.  Data will
             * be added at an offset of dwBytesRecorded.
             */

            when += m->delta + midi->latency;
            if (when < m->last_time)
                when = m->last_time;

            delta = when - m->last_time;
            m->last_time = when;
            ptr = (unsigned long *) hdr->lpData;
            *ptr++ = delta;
            *ptr++ = 0;
            *ptr = MEVT_F_LONG;
            hdr->dwBytesRecorded = 3 * sizeof(long);    /* see note above   */
        }
    }
    msg_buffer = (midibyte_t *)(hdr->lpData);
    msg_buffer[hdr->dwBytesRecorded++] = byte;          /* add data byte    */

    /*
     * See if buffer is full; leave one byte extra for pad
     * Then write what we've got and continue.
     */

    if (hdr->dwBytesRecorded >= hdr->dwBufferLength - 1)
        rslt = winmm_end_sysex(midi, timestamp);

#if defined SEQ66_USE_EXPANDING_SYSEX_BUFFERS
    /*
     *  This code is here as an aid in case you want sysex buffers to expand
     *  to hold large messages completely. If so, you will want to change
     *  SYSEX_BYTES_PER_BUFFER above to some variable that remembers the
     *  buffer size. A good place to put this value would be in the
     *  hdr->dwUser field.
     *
     *  rslt = resize_sysex_buffer(midi, m->sysex_byte_count,
     *      m->sysex_byte_count * 2); *
     *
     *  if (rslt == pmBufferMaxSize) // if the buffer can't be resized
     *
     *  The dwBytesRecorded field gets wiped out, so we'll save it.
     */

    int bytesRecorded = hdr->dwBytesRecorded;
    rslt = resize_sysex_buffer(midi, bytesRecorded, 2 * bytesRecorded);
    hdr->dwBytesRecorded = bytesRecorded;
    if (rslt == pmBufferMaxSize)            /* if buffer can't be resized */
    {
        // no code
    }
#endif

    return rslt;
}

static PmTimestamp
winmm_synchronize (PmInternal * midi)
{
    midiwinmm_type m;
    unsigned long pm_stream_time_2;
    unsigned long real_time;
    unsigned long pm_stream_time;

    /*
     * Only synchronize if we are using the stream interface. Then
     * figure out the time. The do-loop reads real_time between two
     * reads of stream time, and repeats if more than 1 ms elapsed.
     */

    if (midi->latency == 0)
        return 0;

    m = (midiwinmm_type) midi->descriptor;
    pm_stream_time_2 = pm_time_get(m);
    do
    {
        pm_stream_time = pm_stream_time_2;
        real_time = (*midi->time_proc)(midi->time_info);
        pm_stream_time_2 = pm_time_get(m);

    } while (pm_stream_time_2 > pm_stream_time + 1);
    m->delta = pm_stream_time - real_time;
    m->sync_time = real_time;
    return real_time;
}

#if defined SEQ66_USE_SYSEX_PROCESSING_NOT_IN_USE

/**
 *  The winmm_out_callback()  recycles SysEx buffers.  A future optimization:
 *  eliminate UnprepareHeader calls -- they aren't necessary; however, this code
 *  uses the prepared-flag to indicate which buffers are free, so we need to do
 *  something to flag empty buffers if we leave them prepared.
 */

static void CALLBACK
winmm_out_callback
(
    HMIDIOUT hmo,                // unused
    UINT wMsg,
    DWORD_PTR dwInstance,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2           // unused
)
{
    PmInternal * midi = (PmInternal *) dwInstance;
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    LPMIDIHDR hdr = (LPMIDIHDR) dwParam1;
    int err = 0;  /* set to 0 so that no buffer match will also be an error */
    if (hmo != 0 && dwParam2 != 0)
    {
        // Eliminate warnings: pretend these parameters are used.
    }
    if (wMsg == MOM_DONE)                               /* see note above   */
    {
        MMRESULT ret = midiOutUnprepareHeader
        (
            m->handle.out, hdr, sizeof(MIDIHDR)
        );
        if (ret != MMSYSERR_NOERROR)
        {
            // handle the error
        }
    }
    err = SetEvent(m->buffer_signal);   /* notify sender, buffer available  */
    if (! err)                          /* false -> error                   */
    {
        // handle the error
    }
}

#endif

/**
 *  The winmm_streamout_callback() unprepares (frees) the buffer header.  Even
 *  if an error is pending, I think we should unprepare messages and signal
 *  their arrival in case the client is blocked waiting for the buffer.
 */

static void CALLBACK
winmm_streamout_callback
(
    HMIDIOUT hmo,                // unused
    UINT wMsg,
    DWORD_PTR dwInstance,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2           // unused
)
{
    PmInternal * midi = (PmInternal *) dwInstance;
    midiwinmm_type m = (midiwinmm_type) midi->descriptor;
    LPMIDIHDR hdr = (LPMIDIHDR) dwParam1;
    int err;
    if (hmo != 0 && dwParam2 != 0)
    {
        // Eliminate warnings: pretend these parameters are used.
    }
    if (wMsg == MOM_DONE)                               /* see note above   */
    {
        MMRESULT r = midiOutUnprepareHeader(m->handle.out, hdr, sizeof(MIDIHDR));
        if (r != MMSYSERR_NOERROR)
        {
            // handle the error
        }
    }
    err = SetEvent(m->buffer_signal);
    if (! err)                              /* false -> error */
    {
        // handle the error
    }
}


/*
 * Begin exported functions
 */

#define winmm_in_abort pm_fail_fn

pm_fns_node pm_winmm_in_dictionary =
{
    none_write_short,
    none_sysex,
    none_sysex,
    none_write_byte,
    none_write_short,
    none_write_flush,
    winmm_synchronize,
    winmm_in_open,
    winmm_in_abort,
    winmm_in_close,
    winmm_in_poll,
    winmm_has_host_error,
    winmm_get_host_error
};

pm_fns_node pm_winmm_out_dictionary =
{
    winmm_write_short,
    winmm_begin_sysex,
    winmm_end_sysex,
    winmm_write_byte,
    winmm_write_short,                          /* short realtime message   */
    winmm_write_flush,
    winmm_synchronize,
    winmm_out_open,
    winmm_out_abort,
    winmm_out_close,
    none_poll,
    winmm_has_host_error,
    winmm_get_host_error
};

/**
 *  Initialize WinMM interface. Note that if there is something wrong with
 *  winmm (e.g. it is not supported or installed), it is not an error. We
 *  should simply return without having added any devices to the table.
 *  Hence, no error code is returned. Furthermore, this init code is called
 *  along with every other supported interface, so the user would have a
 *  very hard time figuring out what hardware and API generated the error.
 *  Finally, it would add complexity to pmwin.c to remember where the error
 *  code came from in order to convert to text.
 */

void
pm_winmm_init (void)
{
    pm_log_buffer_alloc();               /* see portmidi.c & .h         */
    pm_winmm_mapper_input();
    pm_winmm_mapper_output();
    pm_winmm_general_inputs();
    pm_winmm_general_outputs();
}

/**
 *  No error codes are returned, even if errors are encountered, because
 *  there is probably nothing the user could do (e.g. it would be an error
 *  to retry.
 */

void
pm_winmm_term (void)
{
    int doneAny = 0;
    int i;
    for (i = 0; i < pm_descriptor_index; ++i)
    {
        PmInternal * midi = pm_descriptors[i].internalDescriptor;
        if (not_nullptr(midi))
        {
            midiwinmm_type m = (midiwinmm_type) midi->descriptor;
            if (m->handle.out)
            {
                if (doneAny == 0)               /* close next open device   */
                {
                    infoprint("Begin closing open devices...\n");
                    doneAny = 1;
                }

                /*
                 * Report any host errors; this EXTREMELY useful when
                 * trying to debug client app.
                 */

                if (winmm_has_host_error(midi))
                {
                    char temp[PM_STRING_MAX];
                    char msg[PM_HOST_ERROR_MSG_LEN];
                    winmm_get_host_error(midi, msg, PM_HOST_ERROR_MSG_LEN);
                    snprintf(temp, sizeof temp, "[%d] '%s'\n", i, msg);
                    errprint(temp);                     /* log to console   */
                    pm_log_buffer_append(temp);         /* log to buffer    */
                }
                (*midi->dictionary->close)(midi);       /* close all ports  */
            }
        }
    }
    if (midi_in_caps)
    {
        pm_free(midi_in_caps);
        midi_in_caps = nullptr;
    }
    if (midi_out_caps)
    {
        pm_free(midi_out_caps);
        midi_out_caps = nullptr;
    }
    if (doneAny)
    {
        const char * msg =
            "Warning: devices were left open. They have been closed.\n";

        warnprint(msg);                                 /* log to console   */
        pm_log_buffer_append(msg);                      /* log to buffer    */
    }
    pm_descriptor_index = 0;
    pm_log_buffer_free();
}

/*
 * pmwinmm.c
 *
 * vim: sw=4 ts=4 wm=4 et ft=c
 */


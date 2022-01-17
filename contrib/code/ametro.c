/*
 *  ALSA MIDI CLI Metronome
 *  Copyright (C) 2002-2009 Pedro Lopez-Cabanillas <plcl@users.sf.net>
 *  Copyright (C) 2002-2009 Pedro Lopez-Cabanillas <plcl@email-addr-hidden>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  this program; if not, write to the Free Software Foundation, Inc., 51
 *  Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Compile with:
 *
 *      No:     gcc -o ametro -lasound ametro.c
 *      Yes     gcc -o ametro ametro.c -lasound
 *
 *  Found at these two sites:
 *
 *  http://lists.linuxaudio.org/pipermail/linux-audio-user/2009-August/061724.html
 *  http://lalists.stanford.edu/lau/2009/08/att-0005/ametro.c
 *
 *  "Only trick I don't get in first place was to put -m flag to send master
 *  clock to my sequencer which was necessary in this point to do so."
 *
 *  Modified by C. Ahlstrom 2022-01-16 to 2022-01-17:
 *
 *      -   Changed tabs to spaces.
 *      -   White space alignment.
 *      -   Tweaks to coding conventions.
 *      -   Added ability to output start, stop, and continue clock events.
 *
 *  Further research:
 *
 *      For handling characters in order to have the user emit various MIDI
 *      clock commands:
 *
 *      https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html
 *      https://github.com/ShakaUVM/colors
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <string.h>

#include <alsa/asoundlib.h>

#define nullptr            NULL
#define MIDI_CHANNEL          9
#define MIDI_STRONG_NOTE     34
#define MIDI_WEAK_NOTE       33
#define MIDI_VELOCITY        64
#define MIDI_PROGRAM          0
#define TICKS_PER_QUARTER   120
#define TIME_SIGNATURE_NUM    4
#define TIME_SIGNATURE_FIG    4
#define BPM                 100
#define FALSE                 0
#define TRUE                  1

char * port_address     = nullptr;
snd_seq_t * seq_handle  = nullptr;
int queue_id    = (-1);
int port_in_id  = (-1);
int port_out_id = (-1);
int measure     = 0;
int bpm         = BPM;
int resolution  = TICKS_PER_QUARTER;
int weak        = MIDI_WEAK_NOTE;
int strong      = MIDI_STRONG_NOTE;
int velocity    = MIDI_VELOCITY;
int program     = MIDI_PROGRAM;
int channel     = MIDI_CHANNEL;
int num_parts   = TIME_SIGNATURE_NUM;
int part_fig    = TIME_SIGNATURE_FIG;
int verbose     = TRUE;
int master      = FALSE;
int notes       = TRUE;
int slave       = FALSE;
typedef enum
{
    CT_START,
    CT_CONTINUE,
    CT_STOP,
    CT_CLOCK

} clock_type;

void
usage ()
{
    fprintf
    (
        stderr,
"Usage:\n"
"  ametro\n"
"       [--output CLIENT:PORT]\n"
"       [--resolution PPQ]\n"
"       [--signature N:M]\n"
"       [--tempo BPM]\n"
"       [--weak NOTE]\n"
"       [--strong NOTE]\n"
"       [--quiet]\n"
"       [--velocity 0..127]\n"
"       [--channel 0..15]\n"
"       [--program 0..127]\n"
"\n"
"Options:\n"
"\n"
"  -c, --channel      MIDI channel, range 0 to 15, default 9.\n"
"  -g, --strong       MIDI note# for each measure's strong part, default 34.\n"
"  -h, --help         This message.\n"
"  -m, --master       Output also MIDI clock messages.\n"
"  -M, --masterclock  Output only MIDI clock messages, not note on/off.\n"
"  -o, --output       Pair of CLIENT:PORT, as ALSA numbers or names.\n"
"  -p, --program      MIDI Program, default 0.\n"
"  -q, --quiet        Don't display messages or banners.\n"
"  -r, --resolution   Tick resolution per quarter note (PPQ), default 120.\n"
"  -s, --signature    Time signature (#:#), default 4:4.\n"
"  -S, --slave        Accept/send MIDI start, stop and continue messages.\n"
"  -t, --tempo        Speed, in BPM, default 100.\n"
"  -v, --velocity     MIDI note on velocity, default 64.\n"
"  -w, --weak         MIDI note# for each measure's weak part, default 33.\n"
"\n"
" The output port is required, either on the command line or the environment:\n"
"\n"
"       ALSA_OUTPUT_PORTS = 128:1\n"
"       ALSA_OUT_PORT = 128:1 or the initial part of a client name\n"
"\n"
    );
}

void
open_sequencer ()
{
    int rc = snd_seq_open
    (
        &seq_handle, "default", SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK
    );
    if (rc < 0)
    {
        fprintf(stderr, "Error opening ALSA sequencer\n");
        exit(EXIT_FAILURE);
    }
    snd_seq_set_client_name(seq_handle, "Metronome");
    port_out_id = snd_seq_create_simple_port
    (
        seq_handle, "output",
        SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
        SND_SEQ_PORT_TYPE_APPLICATION | SND_SEQ_PORT_TYPE_MIDI_GENERIC
    );
    if (port_out_id < 0)
    {
        fprintf(stderr, "Error creating output port\n");
        snd_seq_close(seq_handle);
        exit(EXIT_FAILURE);
    }
    port_in_id = snd_seq_create_simple_port
    (
        seq_handle, "input",
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_APPLICATION | SND_SEQ_PORT_TYPE_MIDI_GENERIC
    );
    if (port_in_id < 0)
    {
        fprintf(stderr, "Error creating input port\n");
        snd_seq_close(seq_handle);
        exit(EXIT_FAILURE);
    }
}

/**
 *  Subscribe to a destination port. It's name or address must already be in the
 *  global variable port_address.
 */

void
subscribe ()
{
    int rc;
    snd_seq_addr_t dest, source;
    snd_seq_port_subscribe_t * subs;
    source.client = snd_seq_client_id(seq_handle);
    source.port = port_out_id;
    rc = snd_seq_parse_address(seq_handle, &dest, port_address);
    if (rc < 0)
    {
        fprintf(stderr, "Invalid source address %s\n", port_address);
        snd_seq_close(seq_handle);
        exit(EXIT_FAILURE);
    }
    snd_seq_port_subscribe_alloca(&subs);
    snd_seq_port_subscribe_set_sender(subs, &source);
    snd_seq_port_subscribe_set_dest(subs, &dest);
    snd_seq_port_subscribe_set_queue(subs, queue_id);
    snd_seq_port_subscribe_set_time_update(subs, 1);
    rc = snd_seq_get_port_subscription(seq_handle, subs);
    if (rc == 0)
    {
        fprintf(stderr, "Connection is already subscribed\n");
        snd_seq_close(seq_handle);
        exit(EXIT_FAILURE);
    }
    rc = snd_seq_subscribe_port(seq_handle, subs);
    if (rc < 0)
    {
        fprintf(stderr, "Connection failed (%s)\n", snd_strerror(errno));
        snd_seq_close(seq_handle);
        exit(EXIT_FAILURE);
    }
}

/**
 *  Queue commands
 */

void
create_queue ()
{
     queue_id = snd_seq_alloc_queue(seq_handle);
}

void
set_tempo (int tempo)
{
    snd_seq_queue_tempo_t * queue_tempo;
    int truetempo = (int) ((6e7 * part_fig) / (tempo * 4));
    snd_seq_queue_tempo_alloca(&queue_tempo);
    snd_seq_queue_tempo_set_tempo(queue_tempo, truetempo);
    snd_seq_queue_tempo_set_ppq(queue_tempo, resolution);
    snd_seq_set_queue_tempo(seq_handle, queue_id, queue_tempo);
}

void
clear_queue ()
{
    snd_seq_remove_events_t * remove_ev;
    snd_seq_remove_events_alloca(&remove_ev);
    snd_seq_remove_events_set_queue(remove_ev, queue_id);
    snd_seq_remove_events_set_condition
    (
        remove_ev, SND_SEQ_REMOVE_OUTPUT | SND_SEQ_REMOVE_IGNORE_OFF
    );
    snd_seq_remove_events(seq_handle, remove_ev);
}

void
start_queue ()
{
    snd_seq_start_queue(seq_handle, queue_id, NULL);
    snd_seq_drain_output(seq_handle);
}

void
stop_queue ()
{
    snd_seq_stop_queue(seq_handle, queue_id, NULL);
    snd_seq_drain_output(seq_handle);
}

void
continue_queue ()
{
    snd_seq_continue_queue(seq_handle, queue_id, NULL);
    snd_seq_drain_output(seq_handle);
}

/**
 *  Event commands
 */

void
make_note (unsigned char note, int tick)
{
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_note(&ev, channel, note, velocity, 1);
    snd_seq_ev_schedule_tick(&ev, queue_id, 1, tick);
    snd_seq_ev_set_source(&ev, port_out_id);
    snd_seq_ev_set_subs(&ev);
    snd_seq_event_output_direct(seq_handle, &ev);
}

void
make_echo (int tick)
{
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    ev.type = SND_SEQ_EVENT_USR1;
    snd_seq_ev_schedule_tick(&ev, queue_id, 1, tick);
    snd_seq_ev_set_dest(&ev, snd_seq_client_id(seq_handle), port_in_id);
    snd_seq_event_output_direct(seq_handle, &ev);
}

void
make_clock (int tick)
{
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    ev.type = SND_SEQ_EVENT_CLOCK;
    snd_seq_ev_schedule_tick(&ev, queue_id, 1, tick);
    snd_seq_ev_set_source(&ev, port_out_id);
    snd_seq_ev_set_subs(&ev);
    snd_seq_event_output_direct(seq_handle, &ev);
}

void
make_clock_event (int tick, clock_type ct)
{
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    switch (ct)
    {
        case CT_START:      ev.type = SND_SEQ_EVENT_START;      break;
        case CT_CONTINUE:   ev.type = SND_SEQ_EVENT_CONTINUE;   break;
        case CT_STOP:       ev.type = SND_SEQ_EVENT_STOP;       break;
        case CT_CLOCK:      ev.type = SND_SEQ_EVENT_CLOCK;      break;
    }
    snd_seq_ev_schedule_tick(&ev, queue_id, 1, tick);
    snd_seq_ev_set_source(&ev, port_out_id);
    snd_seq_ev_set_subs(&ev);
    snd_seq_event_output_direct(seq_handle, &ev);
}

void
pattern ()
{
    int part, tick, duration;

    /*
     * MIDI clock events
     */

    if (master)
    {
        int maxtick = resolution * 4 * num_parts / part_fig;
        duration = resolution / 24;
        for (tick = 0; tick < maxtick; tick += duration)
            make_clock(tick);
    }

    /*
     * Metronome notes
     */

    tick = 0;
    duration = resolution * 4 / part_fig;
    for (part = 0; part < num_parts; ++part)
    {
        if (notes)
            make_note(part ? weak : strong, tick);

        tick += duration;
    }
    make_echo(tick);
    if (verbose)
    {
        fprintf(stderr, "measure: %5d\r", ++measure);
    }
}

void
set_program ()
{
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_pgmchange(&ev, channel, program);
    snd_seq_ev_set_source(&ev, port_out_id);
    snd_seq_ev_set_subs(&ev);
    snd_seq_event_output_direct(seq_handle, &ev);
}

void
midi_action ()
{
    snd_seq_event_t * ev;
    do
    {
        snd_seq_event_input(seq_handle, &ev);
        switch (ev->type)
        {
        case SND_SEQ_EVENT_USR1:
            pattern();
            break;

        case SND_SEQ_EVENT_START:
            measure = 0;
            start_queue();
            pattern();
            break;

        case SND_SEQ_EVENT_CONTINUE:
            continue_queue();
            break;

        case SND_SEQ_EVENT_STOP:
            stop_queue();
            break;
        }
    } while (snd_seq_event_input_pending(seq_handle, 0) > 0);
}

void
sigterm_exit (int sig)
{
    clear_queue();
    sleep(1);
    snd_seq_stop_queue(seq_handle, queue_id, NULL);
    snd_seq_free_queue(seq_handle, queue_id);
    snd_seq_close(seq_handle);
    exit(0);
}

int
check_range (int val, int min, int max, char * msg)
{
    if ((val < min) | (val > max))
    {
        fprintf(stderr, "Invalid %s, range is %d to %d\n", msg, min, max);
        return 1;
    }
    return 0;
}

int
parse_options (int argc, char * argv [])
{
    int c;
    long x;
    char * sep;
    int option_index = 0;
    struct option long_options[] =
    {
        {"channel",     1, 0, 'c'},
        {"strong",      1, 0, 'g'},
        {"help",        0, 0, 'h'},
        {"master",      0, 0, 'm'},
        {"masterclock", 0, 0, 'M'},
        {"output",      1, 0, 'o'},
        {"program",     1, 0, 'p'},
        {"quiet",       0, 0, 'q'},
        {"resolution",  1, 0, 'r'},
        {"signature",   1, 0, 's'},
        {"slave",       0, 0, 'S'},
        {"tempo",       1, 0, 't'},
        {"velocity",    1, 0, 'v'},
        {"weak",        1, 0, 'w'},
        {0, 0, 0, 0}
    };

    for (;;)
    {
        c = getopt_long
        (
            argc, argv, "c:g:hmMo:p:qr:s:St:v:w:", long_options, &option_index
        );
        if (c == -1)
            break;

        switch (c)
        {
        case 'c':
            channel = atoi(optarg);
            if (check_range(channel, 0, 15, "channel"))
                return 1;
            break;

        case 'g':
            strong = atoi(optarg);
            if (check_range(strong, 0, 127, "strong note"))
                return 1;
            break;

        case 'm':
            master = TRUE;
            break;

        case 'M':
            master = TRUE;
            notes = FALSE;
            break;

        case 'o':
            port_address = optarg;
            break;

        case 'p':
            program = atoi(optarg);
            if (check_range(program, 0, 127, "program"))
                return 1;
            break;

        case 'q':
            verbose = FALSE;
            break;

        case 'r':
            resolution = atoi(optarg);
            if (check_range(resolution, 48, 480, "resolution"))
                return 1;
            break;

        case 's':
            x = strtol(optarg, &sep, 10);
            if ((x < 1) | (x > 32) | (*sep != ':'))
            {
                fprintf(stderr, "Invalid time signature\n");
                return 1;
            }
            num_parts = x;
            x = strtol(++sep, NULL, 10);
            if ((x < 1) | (x > 32))
            {
                fprintf(stderr, "Invalid time signature\n");
                return 1;
            }
            part_fig = x;
            break;

        case 'S':
            slave = TRUE;
            break;

        case 't':
            bpm = atoi(optarg);
            if (check_range(bpm, 16, 240, "tempo"))
                return 1;
            break;

        case 'v':
            velocity = atoi(optarg);
            if (check_range(velocity, 0, 127, "velocity"))
                return 1;
            break;

        case 'w':
            weak = atoi(optarg);
            if (check_range(weak, 0, 127, "weak note"))
                return 1;
            break;

        case 0:
        case 'h':
        default:
            return 1;
        }
    }
    return 0;
}

/**
 *  The main routine.
 */

int
main (int argc, char * argv [])
{
    int npfd, j;
    struct pollfd * pfd;
    if (verbose)
    {
        fprintf(stderr, "ametro - MIDI metronome using ALSA sequencer\n");
    }
    if (parse_options(argc, argv) != 0)
    {
        usage();
        return EXIT_FAILURE;
    }
    if (port_address == NULL)
    {
        port_address = getenv("ALSA_OUTPUT_PORTS");

        /*
         * Try the old name for the environment variable.
         */

        if (port_address == NULL)
            port_address = getenv("ALSA_OUT_PORT");

        if (port_address == NULL)
        {
            fprintf
            (
                stderr,
                "No client/port specified. Supply one as an argument or set\n"
                "the environment variable ALSA_OUTPUT_PORTS.\n"
            );
            usage();
            return EXIT_FAILURE;
        }
    }

    signal(SIGINT, sigterm_exit);
    signal(SIGTERM, sigterm_exit);
    open_sequencer();
    create_queue();
    subscribe();
    npfd = snd_seq_poll_descriptors_count(seq_handle, POLLIN);
    pfd = (struct pollfd *) alloca(npfd * sizeof(struct pollfd));
    snd_seq_poll_descriptors(seq_handle, pfd, npfd, POLLIN);
    set_tempo(bpm);
    set_program();
    if (slave == FALSE)
    {
        start_queue();
        pattern();
    }
    for (;;)
    {
        if (poll(pfd, npfd, 1000) > 0)
        {
            for (j = 0; j < npfd; j++)
            {
                if (pfd[j].revents > 0)
                    midi_action();
            }
        }
    }
}

/*
 * vim: sw=4 ts=4 wm=4 et
 */


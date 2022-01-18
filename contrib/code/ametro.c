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
 *      -   More comprehensive error detection.
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

#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

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

void make_clock_event (int tick, clock_type ct);

void
show_error (const char * msg)
{
    fprintf(stderr, "Error: %s\n", msg);
}

void
show_error_string (const char * msg, int rc)
{
    fprintf(stderr, "%s (%s)\n", msg, snd_strerror(rc));
}

void
show_msg (const char * msg)
{
    printf("%s\n", msg);
}

void
usage ()
{
    show_msg
    (
"Usage:\n"
"  ametro\n"
"       [ --output CLIENT:PORT ]   [ --resolution PPQ ]\n"
"       [ --signature N:M ]        [ --tempo BPM ]\n"
"       [ --weak NOTE ]            [ --strong NOTE ]\n"
"       [ --velocity 0..127 ]      [ --channel 0..15 ]\n"
"       [ --program 0..127 ]       (more options shown below)\n"
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

/**
 *  This function changes the standard input from "canonical" mode (which
 *  means it buffers until a newline is read) into raw mode, where it will
 *  return one keystroke at a time.
 *
 *      set_raw_mode(TRUE) will turn on nonblocking I/O for standard input.
 *
 *      set_raw_mode(FALSE) will reset I/O to work like normal.
 */

static int raw_mode = FALSE;                   /* default:  canonical mode */

void
set_raw_mode (int flag)
{
	static struct termios old_tio;              /* to save the old settings */
	if (flag && ! raw_mode)                     /* save original term mode  */
    {
		int rc = tcgetattr(STDIN_FILENO, &old_tio);
        if (rc == 0)
        {
            struct termios tio = old_tio;

            /*
             * Disable echo and canonical (cooked) mode.
             */

            tio.c_lflag &= ~(ICANON | ECHO);
            rc = tcsetattr(STDIN_FILENO, TCSANOW, &tio);
            if (rc != 0)
            {
                show_error("tcsetattr() failed to start raw mode");
            }
            else
            {
                raw_mode = TRUE;
                show_msg("Raw console termio activated");
            }
        }
	}
    else if (! flag && raw_mode)                /* restore terminal mode    */
    {
		int rc = tcsetattr(STDIN_FILENO,TCSANOW, &old_tio);
        if (rc != 0)
        {
            show_error("tcsetattr() failed to restore cooked mode");
        }
        else
        {
            raw_mode = FALSE;
            show_msg("Raw console termio deactivated");
        }
	}
}

/**
 *  Returns how many bytes are waiting in the input buffer.
 *
 * Precondition:
 *
 *      Requires set_raw_mode(true) to work.
 *
 * Example:
 *
 *  int bytes_available = kbcount() returns how many bytes are in the input
 *  queue to be read>
 */

int
kbcount ()
{
	int count = 0;
	if (raw_mode)
        ioctl(STDIN_FILENO, FIONREAD, &count);

	return count;
}

/**
 *  Does a non-blocking I/O read from standard input and returns one
 *  keystroke.  It is a lightweight equivalent to ncurses' getch() function.
 *  It returns all characters, including Esc.
 *
 * Precondition:
 *
 *      Requires set_raw_mode(true) to work.
 *
 * Example:
 *
 *      int ch = quick_read() returns -1 if no key has been hit, or the key
 *      actually struck.
 */

int
quick_read ()
{
    int result = (-1);
    if (raw_mode)
    {
        int bytes_available = kbcount();
        if (bytes_available > 0)
        {
            int c = getchar();
            --bytes_available;
            if (bytes_available == 0)
            {
                result = c;

                /*
                 * Debugging only:
                 *
                 * if (verbose)
                 *     printf("char 0x%02x returned\n", c);
                 */
            }

            /*
             * Dump the remaining bytes.  We don't need them for our simple
             * purposes.  Also we don't care about mouse events.
             */

            for (int i = 0; i < bytes_available; ++i)
            {
                c = getchar();

                /*
                 * Debugging only:
                 *
                 * if (verbose)
                 *     printf("Discarding char 0x%02x\n", c);
                 */
            }
        }
	}
	return result;
}

void
bail_out ()
{
    set_raw_mode(FALSE);
}

int
handle_char (int ch, int tick)
{
    int result = 0;         /* 1 == 'Esc' */
    switch (ch)
    {
    case 's':   make_clock_event(0, CT_START);      break;
    case 'c':   make_clock_event(0, CT_CONTINUE);   break;
    case 'x':   make_clock_event(0, CT_STOP);       break;
    case '.':   make_clock_event(0, CT_CLOCK);      break;
    case 033:   result = 1;                         break;
    }
    return result;
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
        show_error("Opening ALSA sequencer");
        exit(EXIT_FAILURE);
    }
    rc = snd_seq_set_client_name(seq_handle, "Metronome");
    if (rc < 0)
    {
        show_error("Naming ALSA sequencer");
        exit(EXIT_FAILURE);
    }
    port_out_id = snd_seq_create_simple_port
    (
        seq_handle, "output",
        SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
        SND_SEQ_PORT_TYPE_APPLICATION | SND_SEQ_PORT_TYPE_MIDI_GENERIC
    );
    if (port_out_id < 0)
    {
        show_error("Creating output port");
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
        show_error("creating input port");
        snd_seq_close(seq_handle);
        exit(EXIT_FAILURE);
    }
}

/**
 *  Subscribe to a destination port. It's name or address must already be in
 *  the global variable port_address.
 */

void
subscribe ()
{
    snd_seq_addr_t dest;
    snd_seq_addr_t source;
    snd_seq_port_subscribe_t * subs;
    int rc = snd_seq_client_id(seq_handle);
    if (rc >= 0)
    {
        source.client = rc;
        source.port = port_out_id;
    }
    else
    {
        show_error("Could not get client ID");
        exit(EXIT_FAILURE);
    }
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
        show_error("Connection already subscribed");
        snd_seq_close(seq_handle);
        exit(EXIT_FAILURE);
    }
    rc = snd_seq_subscribe_port(seq_handle, subs);
    if (rc < 0)
    {
        show_error_string("Connection failed", rc);
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
    if (queue_id < 0)
    {
        show_error("Could not get queue ID");
        exit(EXIT_FAILURE);
    }
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
    int sendcount;
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_note(&ev, channel, note, velocity, 1);
    snd_seq_ev_schedule_tick(&ev, queue_id, 1, tick);
    snd_seq_ev_set_source(&ev, port_out_id);
    snd_seq_ev_set_subs(&ev);
    sendcount = snd_seq_event_output_direct(seq_handle, &ev);
    if (sendcount < 0)
        show_error("make_note() output-direct");
}

void
make_echo (int tick)
{
    int sendcount;
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    ev.type = SND_SEQ_EVENT_USR1;
    snd_seq_ev_schedule_tick(&ev, queue_id, 1, tick);
    snd_seq_ev_set_dest(&ev, snd_seq_client_id(seq_handle), port_in_id);
    sendcount = snd_seq_event_output_direct(seq_handle, &ev);
    if (sendcount < 0)
        show_error("make_echo() output-direct");
}

void
make_clock (int tick)
{
    int sendcount;
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    ev.type = SND_SEQ_EVENT_CLOCK;
    snd_seq_ev_schedule_tick(&ev, queue_id, 1, tick);
    snd_seq_ev_set_source(&ev, port_out_id);
    snd_seq_ev_set_subs(&ev);
    sendcount = snd_seq_event_output_direct(seq_handle, &ev);
    if (sendcount < 0)
        show_error("make_clock() output-direct");
}

void
make_clock_event (int tick, clock_type ct)
{
    const char * msg = "?";
    int sendcount;
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    switch (ct)
    {
        case CT_START:
            ev.type = SND_SEQ_EVENT_START;
            msg = "start";
            break;

        case CT_CONTINUE:
            ev.type = SND_SEQ_EVENT_CONTINUE;
            msg = "continue";
            break;

        case CT_STOP:
            ev.type = SND_SEQ_EVENT_STOP;
            msg = "stop";
            break;

        case CT_CLOCK:
            ev.type = SND_SEQ_EVENT_CLOCK;
            msg = "clock";
            break;
    }
    snd_seq_ev_schedule_tick(&ev, queue_id, 1, tick);
    snd_seq_ev_set_source(&ev, port_out_id);
    snd_seq_ev_set_subs(&ev);
    sendcount = snd_seq_event_output_direct(seq_handle, &ev);
    if (sendcount >= 0)
        show_msg(msg);
    else
        show_error("make_clock_event() output-direct");
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
        printf("Measure: %5d\r", ++measure);
}

void
set_program ()
{
    int sendcount;
    snd_seq_event_t ev;
    if (verbose)
    {
        printf
        (
            "Setting program %d, channel %d for output port %d\n",
            program, channel, port_out_id
        );
    }
    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_pgmchange(&ev, channel, program);
    snd_seq_ev_set_source(&ev, port_out_id);
    snd_seq_ev_set_subs(&ev);
    sendcount = snd_seq_event_output_direct(seq_handle, &ev);
    if (sendcount < 0)
        show_error_string("set_program() output-direct failed", sendcount);
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
                show_error("Invalid time signature");
                return 1;
            }
            num_parts = x;
            x = strtol(++sep, NULL, 10);
            if ((x < 1) | (x > 32))
            {
                show_error("Invalid time signature");
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
        show_msg("ametro: MIDI metronome using ALSA sequencer");
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
            show_error
            (
                "No client/port specified. Use --output or set"
                "environment value ALSA_OUTPUT_PORTS"
            );
            usage();
            return EXIT_FAILURE;
        }
    }

	/*
     * These next three lines prevent us from leaving the terminal in a bad
     * state if we ctrl-c out or exit(). The bail_out() function is the
     * callback when we quit; call it to clean up the terminal.
     */

	atexit(bail_out);
    signal(SIGINT, sigterm_exit);
    signal(SIGTERM, sigterm_exit);
    set_raw_mode(TRUE);
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
        int ch = quick_read();
        if (ch > 0)
        {
            if (handle_char(ch, 0) == 1)
                break;
        }
        if (poll(pfd, npfd, 1000) > 0)
        {
            for (j = 0; j < npfd; ++j)
            {
                if (pfd[j].revents > 0)
                    midi_action();
            }
        }
        else
        {
            show_error("Poll failed");
        }
    }
}

/*
 * vim: sw=4 ts=4 wm=4 et
 */


/*
 *  This file is part of seq66.
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
 * \file          metro.cpp
 *
 *  This module declares/defines a special pattern for the metronome.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2022-08-05
 * \updates       2024-08-07
 * \license       GNU GPLv2 or above
 *
 */

#include "cfg/settings.hpp"             /* seq66::usr() config accessor     */
#include "play/metro.hpp"               /* seq66::metro sequence class      */
#include "play/performer.hpp"           /* seq66::performer class functions */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 *---------------------------------------------------------------------
 *  metrosettings
 *---------------------------------------------------------------------
 *
 *  See https://music.arts.uci.edu/dobrian/maxcookbook/
 *          metronome-using-general-midi-sounds
 */

metrosettings::metrosettings () :
    m_buss                  (0),
    m_channel               (9),            /* MIDI channel 10, drums   */
    m_recording_buss        (0),
    m_thru_buss             (0),
    m_thru_channel          (0),
    m_beats_per_bar         (4),
    m_beat_width            (4),
    m_main_patch            (0),            /* Standard drum kit        */
    m_sub_patch             (0),            /* Standard drum kit        */
    m_main_note             (75),           /* Claves                   */
    m_main_note_velocity    (96),
    m_main_note_length      (0),
    m_sub_note              (76),           /* High Wood Block          */
    m_sub_note_velocity     (84),
    m_sub_note_length       (0),
    m_main_note_fraction    (0.0),
    m_sub_note_fraction     (0.0),
    m_count_in_active       (false),
    m_count_in_measures     (1),
    m_count_in_recording    (false),
    m_recording_measures    (0)
{
    /*
     * See the principal constructor below.
     */
}

/**
 *  A helper function for metro::initialize().
 */

midipulse
metrosettings::calculate_length (int increment, float fraction)
{
    midipulse result;
    if (fraction > 0.1)             /* sanity float check   */
        result = midipulse(increment * fraction);
    else
        result = increment / 2;

    return result;
}

/**
 *  "A computer metronome could trigger any sort of sound: MIDI notes,
 *  percussion sounds, recorded samples, synthesized beeps, etc. In this
 *  example, I’ve chosen to use two specific MIDI notes, key numbers 75 and 76
 *  on MIDI channel 10 which, according to the General MIDI standard, plays
 *  the sounds of Claves and High Wood Block. Those sounds should be roughly
 *  the same on any GM-compliant synthesizer, including the synthesizers built
 *  into the Mac OS and Windows OS."
 *
 *      https://music.arts.uci.edu/dobrian/maxcookbook/
 *              metronome-using-general-midi-sounds
 *
 *  This function is called in rcsettings::set_defaults(), happening in the
 *  smanager constructor.
 */

void
metrosettings::set_defaults ()
{
    m_buss                  = 0;
    m_channel               = 9;        /* Channel 10, Percussion           */
    m_beats_per_bar         = 4;
    m_beat_width            = 4;
    m_main_patch            = 0;        /* Standard drum kit                */
    m_sub_patch             = 0;        /* Standard drum kit                */
    m_main_note             = 75;       /* Claves. 72 = middle C + 12       */
    m_main_note_velocity    = 96;
    m_main_note_length      = 0;
    m_sub_note              = 76;       /* H. Wood Block: 60 = middle C     */
    m_sub_note_velocity     = 84;
    m_sub_note_length       = 0;
    m_main_note_fraction    = 0.0;      /* same as 0.5                      */
    m_sub_note_fraction     = 0.0;      /* ditto                            */
    m_count_in_active       = false;
    m_count_in_measures     = 1;
    m_count_in_recording    = false;
    m_recording_measures    = 0;
}

bool
metrosettings::initialize (int increment)
{
    m_main_note_length = calculate_length(increment, m_main_note_fraction);
    m_sub_note_length = calculate_length(increment, m_sub_note_fraction);
    return true;
}

/*
 *---------------------------------------------------------------------
 *  metro
 *---------------------------------------------------------------------
 */

/**
 *  Default constructor.
 */

metro::metro () :
    sequence            (),
    m_metro_settings    ()
{
    /*
     * See the initialize() function below.
     */
}

/**
 *  Principal constructor.
 */

metro::metro (const metrosettings & mc) :
    sequence            (),
    m_metro_settings    (mc)
{
    /*
     * See the initialize() function below.
     */
}

/**
 *  A rote destructor.
 */

metro::~metro ()
{
    // Empty body
}

/**
 *  Helper function for initialize() and its overrides.
 */

bool
metro::init_setup (performer * p, int measures)
{
    bool result = not_nullptr(p);
    if (result)
    {
        result = settings().sanity_check();
        if (result)
            set_parent(p);
    }
    if (result)
    {
        int ppq = p->ppqn();                        /* p->get_ppqn()        */
        int bpb = settings().beats_per_bar();       /* get_beats_per_bar()  */
        int bw = settings().beat_width();           /* get_beat_width()     */
        midibyte channel = settings().channel();    /* seq_midi_channel()   */
        (void) set_midi_bus(settings().buss());     /* ...uses master-bus   */
        (void) set_midi_channel(channel);           /* metro output channel */
        set_beats_per_bar(bpb);                     /* hmm, add bool return */
        set_beat_width(bw);                         /* ditto                */
        if (measures > 0)
            (void) apply_length(bpb, ppq, bw, measures);
    }
    return result;
}

/**
 *  Fills the event list for the metronome.  Requires that all the setting
 *  functions noted above be called first.
 *
 *  For finding the length, can use measures_to_ticks() or
 *  sequence::apply_length().
 */

bool
metro::initialize (performer * p)
{
    bool result = init_setup(p, 1);                 /* set up one measure   */
    if (result)
    {
        int ppq = p->ppqn();                        /* p->get_ppqn()        */
        int bpb = settings().beats_per_bar();       /* get_beats_per_bar()  */
        int bw = settings().beat_width();           /* get_beat_width()     */
        midibyte channel = settings().channel();    /* seq_midi_channel()   */
        int increment = pulses_per_beat(ppq, bpb, bw);
        if (settings().initialize(increment))
        {
            /*
             * Must set this before the possibility of raising the modify
             * flag.
             */

            seq_number(metronome());                /* magic metro number   */
            set_name("Metronome");
        }

        midipulse tick = 0;
        for (int count = 0; count < bpb; ++count, tick += increment)
        {
            midibyte patch, note, vel, len;
            if (count == 0)
            {
                patch = settings().main_patch();
                note = settings().main_note();
                vel = settings().main_note_velocity();
                len = settings().main_note_length();
            }
            else
            {
                patch = settings().sub_patch();
                note = settings().sub_note();
                vel = settings().sub_note_velocity();
                len = settings().sub_note_length();
            }

            event prog(tick, EVENT_PROGRAM_CHANGE | channel, patch);
            event on(tick + 1, EVENT_NOTE_ON, channel, note, vel);
            event off(tick + len, EVENT_NOTE_OFF, channel, note, vel);
            result = add_event(prog);
            if (result)
                result = add_event(on);

            if (result)
                result = add_event(off);

            if (! result)
                break;
        }
        if (result)
        {
            sort_events();
            armed(true);
            unmodify();                             /* not part of song     */
        }
    }
    return result;
}

/*
 *---------------------------------------------------------------------
 *  recorder
 *---------------------------------------------------------------------
 */

/*
 *  The color is set to 1, which is the "Red" entry in the
 *  application palette.
 */

static const int s_recorder_color = 1;

/**
 *  Default constructor. Also see the metro constructor.
 */

recorder::recorder () : metro ()
{
    // set_color(s_recorder_color, true);
}

/**
 *  Principal constructor.
 */

recorder::recorder (const metrosettings & mc) : metro (mc)
{
    // set_color(s_recorder_color, true);
}

/**
 *  A rote destructor.
 */

recorder::~recorder ()
{
    // Empty body
}

/**
 *  Fills the event list for the recorder.  Requires that all the setting
 *  functions noted above be called first.
 *
 *  For finding the length, can use measures_to_ticks() or
 *  sequence::apply_length().
 *
 * Life-cycle:
 *
 *      -   Create the recorder sequence and call this initialize() function.
 *      -   It sets a few things up for recording.  Note especially the
 *          set_recording() function. It calls mastermidibus ::
 *          set_sequence_input() to log this pattern as the recording pattern.
 *
 *  Must set this before the possibility of raising the modify
 *  flag. Also note we select the new-pattern quantities. To recollect:
 *  recordstyle covers merge, overwrite, expand...; alteration covers
 *  none, quantize, notemap...;
 */

bool
recorder::initialize (performer * p)
{
    bool result = init_setup(p, settings().recording_measures());
    if (result)
    {
        int ppq = p->ppqn();                        /* p->get_ppqn()        */
        int bpb = settings().beats_per_bar();       /* get_beats_per_bar()  */
        int bw = settings().beat_width();           /* get_beat_width()     */
        int increment = pulses_per_beat(ppq, bpb, bw);
        if (settings().initialize(increment))
        {
            bool unmute = usr().new_pattern_armed();
            alteration alter = usr().record_mode();
            recordstyle rs = usr().new_pattern_record_style();
            bool usethru = usr().new_pattern_thru();
            bussbyte outbuss = settings().thru_buss();
            midibyte channel = settings().thru_channel();

            armed(unmute);
            set_recording(alter, toggler::on);      /* eg. quantize...      */
            set_recording_style(rs);                /* merge, expand, etc.  */

            /*
             * Needs explaining.
             */

            set_thru(usethru);
            set_midi_bus(outbuss);                  /* for playback         */
            set_midi_channel(channel);
            set_name("Background Recording");
            set_color(s_recorder_color, true);

            /*
             * Do not make these settings.
             *
             * seq_number(sequence::recorder());    // magic recorder seq
             * expanded_recording(true);
             * wrap-around?
             */

            unmodify();                             /* not part of song     */
        }
    }
    return result;
}

bool
recorder::uninitialize ()
{
    set_recording(alteration::none, toggler::off);  /* doesn't clear expand */
    set_color(0, true);

    /*
     * Probably want the user to remember to modify these settings.
     *
     * set_midi_bus(0);
     * set_midi_channel(0);
     */

    return true;
}

}           // namespace seq66

/*
 * recorder.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


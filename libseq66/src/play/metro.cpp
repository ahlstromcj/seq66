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
 * \updates       2022-08-08
 * \license       GNU GPLv2 or above
 *
 */

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
 */

metrosettings::metrosettings () :
    m_buss                  (0),
    m_channel               (0),
    m_beats_per_bar         (4),
    m_beat_width            (4),
    m_main_patch            (0),
    m_sub_patch             (0),
    m_main_note             (72),       /* middle C + 12 */
    m_main_note_velocity    (120),
    m_main_note_length      (0),
    m_sub_note              (60),       /* middle C      */
    m_sub_note_velocity     (84),
    m_sub_note_length       (0),
    m_main_note_fraction    (0.0),
    m_sub_note_fraction     (0.0)
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

void
metrosettings::set_defaults ()
{
    m_buss                  = 0;
    m_channel               = 0;
    m_beats_per_bar         = 4;
    m_beat_width            = 4;
    m_main_patch            = 0;
    m_sub_patch             = 0;
    m_main_note             = 72;       /* middle C + 12 */
    m_main_note_velocity    = 120;
    m_main_note_length      = 0;
    m_sub_note              = 60;       /* middle C      */
    m_sub_note_velocity     = 84;
    m_sub_note_length       = 0;
    m_main_note_fraction    = 0.0;
    m_sub_note_fraction     = 0.0;
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
 *  Fills the event list for the metronome.  Requires that all the setting
 *  functions noted above be called first.
 *
 *  For finding the length, can use measures_to_ticks() or
 *  sequence::apply_length().
 */

bool
metro::initialize (performer * p)
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
        int measures = 1;
        int increment = pulses_per_beat(ppq, bw);
        (void) set_midi_bus(settings().buss());     /* ...uses master-bus   */
        (void) set_midi_channel(channel);
        set_beats_per_bar(bpb);                     /* hmm, add bool return */
        set_beat_width(bw);                         /* ditto                */
        (void) apply_length(bpb, ppq, bw, measures);
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
            event prog(tick, EVENT_PROGRAM_CHANGE, channel, patch);
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
            unmodify();                     /* it's not part of the song    */
        }
    }
    return result;
}

}           // namespace seq66

/*
 * metro.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


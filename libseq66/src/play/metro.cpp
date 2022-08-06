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
 * \updates       2022-08-06
 * \license       GNU GPLv2 or above
 *
 */

#include "play/metro.hpp"               /* seq66::metro sequence class      */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Principal constructor.
 *
 * \param ppqn
 *      Provides the PPQN parameter to perhaps alter the default PPQN value of
 *      this metro.
 */

metro::metro () :
    sequence                (),
    m_main_patch            (0),
    m_sub_patch             (0),
    m_main_note             (72),       /* middle C + 12 */
    m_main_note_velocity    (120),
    m_main_note_length      (0),
    m_sub_note              (60),       /* middle C      */
    m_sub_note_velocity     (84),
    m_sub_note_length       (0)
{
    (void) initialize();
}

metro::metro
(
    int mainpatch, int subpatch,
    int mainnote, int mainnote_velocity,
    int subnote, int subnote_velocity,
    midipulse mainnote_len,
    midipulse subnote_len
) :
    sequence                (),
    m_main_patch            (0),
    m_sub_patch             (0),
    m_main_note             (0),
    m_main_note_velocity    (0),
    m_main_note_length      (0),
    m_sub_note              (0),
    m_sub_note_velocity     (mainnote_len),
    m_sub_note_length       (subnote_len)
{
    /*
     * The caller must call the following functions and () and check the
     * return values for the ones that have boolean returns *.
     *
     *      set_midi_bus(bus) *
     *      set_midi_channel(chan, false) *
     *      set_beats_per_bar(bpb)
     *      set_beat_width(bw)
     *      initialize() *
     */

    if (mainpatch >= 0 && mainpatch <= c_midibyte_value_max)
        m_main_patch = midibyte(mainpatch);

    if (subpatch >= 0 && subpatch <= c_midibyte_value_max)
        m_sub_patch = midibyte(subpatch);

    if (mainnote >= 0 && mainnote <= c_midibyte_value_max)
        m_main_note = midibyte(mainnote);

    if (mainnote_velocity >= 0 && mainnote_velocity <= c_midibyte_value_max)
        m_main_note_velocity = midibyte(mainnote_velocity);

    if (subnote >= 0 && subnote <= c_midibyte_value_max)
        m_sub_note = midibyte(subnote);

    if (subnote_velocity >= 0 && subnote_velocity <= c_midibyte_value_max)
        m_sub_note_velocity = midibyte(subnote_velocity);
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
metro::initialize ()
{
    bool result = m_main_note > 0 && m_sub_note > 0;    /* a sanity check   */
    if (result)
    {
        int bpb = get_beats_per_bar();
        int ppq = get_ppqn();
        int bw = get_beat_width();
        int measures = 1;
        (void) apply_length(bpb, ppq, bw, measures);    /* might not change */

        int increment = pulses_per_beat(ppq, bw);
        midipulse mainlen = m_main_note_length == 0 ?
            midipulse(increment / 2) : m_main_note_length;

        midipulse sublen = m_sub_note_length == 0 ?
            midipulse(increment / 2) : m_sub_note_length;

        midipulse tick = 0;
        midibyte channel = seq_midi_channel();
        for (int count = 0; count < bpb; ++count, tick += increment)
        {
            midibyte patch, note, vel, len;
            if (count == 0)
            {
                patch = m_main_patch;
                note = m_main_note;
                vel = m_main_note_velocity;
                len = mainlen;
            }
            else
            {
                patch = m_sub_patch;
                note = m_sub_note;
                vel = m_sub_note_velocity;
                len = sublen;
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
            seq_number(metronome());    /* magic number for metro class */
            set_name("Metronome");
            armed(true);
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


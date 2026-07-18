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
 * \file          scratchpad.cpp
 *
 *  This module declares/defines a special pattern for a"background
 *  recording" scratchpad.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2026-07-18
 * \updates       2026-07-18
 * \license       GNU GPLv2 or above
 *
 *  The scratchpad is a sequence with the following differences:
 *
 *      -   It can expand, but only when notes are received.
 *      -   It uses a predetermined buss, specified in the Edit /
 *          Preferences / Metronome / Record buss drop-down.
 *      -   It should stop itself after the measures specified
 *          in Record Measures field.
 *      -   It activates recording only to that pattern
 *          when the background recording is clicked to enable it.
 *      -   The user should be able to select notes and paste them
 *          into a new (or perhaps existing) pattern at the end
 *          of any existing notes.
 *
 *  Current creation of a pattern:
 *
 *      -   performer::new_sequence() creates one in the next empty slot.
 *          -   set_dirty() for it, which we do not need here.
 *          -   sequence_inbus_setup() sets up a play-set and sets up
 *              record by buss, which we do not need here.
 *          -   Other things we might not need for this "unofficial"
 *              pattern.
 *      -   init_setup() calls set_parent(), but we need a modified
 *          version.
 */

// #include "cfg/settings.hpp"          /* seq66::usr() config accessor     */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "play/scratchpad.hpp"          /* seq66::scratchpad class          */

namespace seq66
{

/*
 *  The color is set to 1, which is the "Red" entry in the
 *  application palette.
 */

static const int s_scratchpad_color { 1 };

/**
 *  Default constructor.
 */

scratchpad::scratchpad () : sequence ()
{
    // set_color(s_scratchpad_color, true);
}

/**
 *  A rote destructor.
 */

scratchpad::~scratchpad ()
{
    // Empty body
}

/**
 *  Fills the event list for the scratchpad.  Requires that all the setting
 *  functions noted above be called first.
 *
 *  For finding the length, can use measures_to_ticks() or
 *  sequence::apply_length().
 *
 * Life-cycle:
 *
 *      -   Create the scratchpad sequence and call this initialize() function.
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
scratchpad::initialize
(
    performer * p,
    bussbyte recbuss,
    int recmeasures
)
{
    bool result
    {
        not_nullptr(p) && is_good_buss(recbuss) && recmeasures >= 0
    };
    if (result)
    {
        /*
         * This sets, bpbp, bw, bus, masterbus, and length.
         */

        set_parent(p);

//      int ppq { p->ppqn() };
//      int bw { get_beat_width() };
//      int increment { pulses_per_beat(ppq, bw) };
        alteration alter { alteration::none };
        recordstyle rs { recordstyle::expand };
//      bussbyte thrubuss { settings().thru_buss();
//      midibyte thruchannel { settings().thru_channel();
        armed(false);
        set_recording(alter, toggler::on);          /* eg. quantize...      */
        set_recording_style(rs);                    /* merge, expand, etc.  */
        set_midi_in_bus(recbuss);                   /* for recording        */
//      if (is_good_bus(thrubuss))
//      {
    //      set_midi_bus(thrubuss);                 /* for playback         */
    //      set_midi_channel(thruchannel);
    //      set_thru(true);
//      }
        set_name("Scratch Pad");
        set_color(s_scratchpad_color, true);

        /*
         * Do not make these settings.
         * expanded_recording(true);
         */

        unmodify();                                 /* not part of song     */
    }
    return result;
}

bool
scratchpad::uninitialize ()
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
 * scratchpad.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

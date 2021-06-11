#if ! defined SEQ66_CONTROLLERS_HPP
#define SEQ66_CONTROLLERS_HPP

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
 * \file          controllers.hpp
 *
 *  This module declares the array of MIDI controller names.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-07-24
 * \updates       2020-07-17
 * \license       GNU GPLv2 or above
 *
 *  This file used to define the array itself, but now it just declares it,
 *  since more than one module now uses this array.
 */

#include <string>

#include "midibytes.hpp"                /* seq66::c_midibyte_data_max (128) */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 *  Provides the default names of MIDI controllers, which a specified in the
 *  controllers.cpp module.  This array is used
 *  only by the seqedit/qseqedit classes.
 *
 *  We could make this list a configuration option.  Overkill?
 */

extern const std::string c_controller_names[c_midibyte_data_max];

/**
 *  This enumeration summarizes the MIDI Continuous Controllers (CC) that are
 *  available.
 *
 * Notes:
 *
 *  For balance and pan: 0 = hard left, 64 = center, 127 = hard right
 *
 *  For all on/off switches, 0 to 63 = Off and 64 to 127 = On.
 *
 *  Damper Pedal 64 versus Sostenuto CC 66:
 *
 *      Sustain is a pedal on/off switch that controls sustain.  Sostenuto is
 *      an on/off switch like the Sustain controller (CC 64), but it only holds
 *      notes that were on when the pedal was pressed.  People use it to “hold”
 *      chords” and play melodies over the held chord.
 *
 *  NRPN 98 and 99: Non-Registered Parameter Number LSB and MSB. For
 *  controllers 6, 38, 96, and 97, it selects the NRPN parameter.
 *
 *  RPN 100 and 101: Registered Parameter Number LSB and MSB. For controllers
 *  6, 38, 96, and 97, it selects the RPN parameter.
 *
 *  Local On/Off Switch 122: Turns the internal connection of a MIDI
 *  keyboard/workstation, etc., On or Off.  For a computer, one will most
 *  likely want Local Control off to avoid notes being played twice, once
 *  locally and twice when the note is sent back from the computer to your
 *  keyboard.
 *
 *  All Notes Off 123: Mutes all sounding notes. Release time will be
 *  maintained, and notes held by sustain will not turn off until sustain pedal
 *  is depressed.
 *
 *  Undefined values summary: 3, 9, 14-15, 20-31, 85-90, and 102-119.
 *
 *  Values 32 to 63 are for Controllers 0 to 31, the Least Significant Bit
 *  (LSB).
 */

enum class cc
{
    bank_select       =   0, /**< Switches patch bank;16,384 patches/chann.  */
    modulation        =   1, /**< Patch vibrato (pitch/loudness/brightness). */
    breath_controller =   2, /**< Aftertouch, MIDI control, modulation.      */
    undefined_03      =   3,
    foot_controller   =   4, /**< Aftertouch, stream of pedal values, etc.   */
    portamento        =   5, /**< Controls rate to slide between 2 notes.    */
    data_entry        =   6, /**< MSB sets value for NRPN/RPN parameters.    */
    volume            =   7, /**< Controls the volume of the channel.        */
    balance           =   8, /**< Left/right balance for stereo patches.     */
    undefined_09      =   9,
    pan               =  10, /**< Left/right balance for mono patches.       */
    expression        =  11, /**< Expression, a percentage of volume (CC7).  */
    effect_control_1  =  12, /**< Control a parameter of a synth effect.     */
    effect_control_2  =  13, /**< Control a parameter of a synth effect.     */
    undefined_14      =  14,
    undefined_15      =  15,
    general_purp_16   =  16,
    general_purp_17   =  17,
    general_purp_18   =  18,
    general_purp_19   =  19,

    /*
     * 20 – 31 Undefined.
     * 32 – 63 Controllers 0 to 31, Least Significant Bit (LSB).
     */

    damper_pedal      =  64, /**< On/off switch that controls sustain.       */
    portamento_onoff  =  65, /**< On/off switch that controls portamento.    */
    sostenuto         =  66, /**< On/off switch to holds only On notes.      */
    soft_pedal        =  67, /**< On/off switch to lower volume of notes.    */
    legato            =  68, /**< On/off switch for legato between 2 notes.  */
    undefined_69      =  69,
    sound_control_1   =  70, /**< Control sound producing [Sound Variation]. */
    sound_control_2   =  71, /**< Shapes the VCF [Resonance, timbre].        */
    sound_control_3   =  72, /**< Control release time of the VCA.           */
    sound_control_4   =  73, /**< Control attack time of the VCA.            */
    sound_control_5   =  74, /**< Controls the VCF cutoff frquency.          */
    sound_control_6   =  75, /**< Shapes the VCF [Resonance, timbre].        */
    sound_control_7   =  76, /**< Manufacturer-dependent sound alteration.   */
    sound_control_8   =  77, /**< Manufacturer-dependent sound alteration.   */
    sound_control_9   =  78, /**< Manufacturer-dependent sound alteration.   */
    sound_control_10  =  79, /**< Manufacturer-dependent sound alteration.   */
    gp_onoff_switch_1 =  80, /**< Provides a general purpose on/off switch.  */
    gp_onoff_switch_2 =  81, /**< Provides a general purpose on/off switch.  */
    gp_onoff_switch_3 =  82, /**< Provides a general purpose on/off switch.  */
    gp_onoff_switch_4 =  83, /**< Provides a general purpose on/off switch.  */
    portamento_cc     =  84, /**< Controls the amount of portamento.         */

    /*
     * 85 – 90 Undefined.
     */

    effect_1_depth    =  91, /**< Usually controls reverb send amount.       */
    effect_2_depth    =  92, /**< Usually controls tremolo amount.           */
    effect_3_depth    =  93, /**< Usually controls chorus amount.            */
    effect_4_depth    =  94, /**< Usually controls detune amount.            */
    effect_5_depth    =  95, /**< Usually controls phaser amount.            */
    data_increment    =  96, /**< Increment data for RPN and NRPN messages.  */
    data_decrement    =  97, /**< Decrement data for RPN and NRPN messages.  */
    nrpn_lsb          =  98, /**< CC 6, 38, 96, and 97: selects NRPN LSB.    */
    nrpn_msb          =  99, /**< CC 6, 38, 96, and 97: selects NRPN MSB.    */
    rpn_lsb           = 100, /**< CC 6, 38, 96, and 97: selects RPN LSB.     */
    rpn_msb           = 101, /**< CC 6, 38, 96, and 97: selects RPN MSB.     */

    /*
     * 102 – 119 Undefined.
     */

    reset_all         = 121, /**< Reset all controllers to their default.    */
    local_switch      = 122, /**< Switches internal connection of a device.  */
    all_notes_off     = 123, /**< Mutes all sounding notes. See notes.       */
    omni_off          = 124, /**< Sets to “Omni Off” mode.                   */
    omni_on           = 125, /**< Sets to “Omni On” mode.                    */
    mono_on           = 126, /**< device mode to Monophonic.                 */
    poly_on           = 127, /**< device mode to Polyphonic.                 */

};          // enum class cc

}           // namespace seq66

#endif      // SEQ66_CONTROLLERS_HPP

/*
 * controllers.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


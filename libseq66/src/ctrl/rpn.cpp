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
 * \file          rpn.cpp
 *
 *  This module declares/defines a class for handling RPN/NPRN/Data
 *  controls.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2026-07-31
 * \updates       2026-08-22
 * \license       GNU GPLv2 or above
 *
 *  This class represents all the RPN and NRPN events needed to change
 *  a MIDI parameter.
 */

#include "midi/controllers.hpp"         /* RPN number<-->bytes functions    */
#include "ctrl/rpn.hpp"                 /* seq66::rpn class                 */

namespace seq66
{

rpn::rpn (const info & rinfo) :
    midimacro   { },
    m_info      { rinfo }
{
    // no code
}

rpn::rpn
(
    control control_type,
    parameter parameter_type,
    midipulse time_stamp,
    midishort rpn_parameter_value,
    midishort rpn_parameter_number,
    bool append_data,
    bool append_reset,
    bool use_fine_rpn
) :
    midimacro               { },
    m_info                  { }
{
    m_info.rpn_control_type     = control_type;
    m_info.rpn_parameter_type   = parameter_type;
    m_info.rpn_time_stamp       = time_stamp;
    m_info.rpn_parameter_number = rpn_parameter_number;
    m_info.rpn_parameter_value  = rpn_parameter_value;
    m_info.rpn_parameter_string = "0.0";
    m_info.rpn_append_data      = append_data;
    m_info.rpn_append_reset     = append_reset;
    m_info.rpn_use_fine_rpn     = use_fine_rpn;
}

/**
 *  This function makes adjustments based on the settings the
 *  constructor made.
 *
 *  Note that the names of the RPNs are provided, at present, in
 *  the controllers module in the s_rpn_names[] array.
 *
 *  TODO: also check all number ranges.
 */

bool
rpn::fix_settings ()
{
    bool result { true };
    switch (control_type())
    {
    case control::rpn:

        m_info.rpn_parameter_number = parameter_to_short(parameter_type());
        break;

    case control::nrpn:

        /* The parameter number provided is suitable here. */
        break;

    case control::slider:

        m_info.rpn_append_data = true;
        break;

    case control::increment:

        m_info.rpn_append_data = true;
        break;

    case control::decrement:

        m_info.rpn_append_data = true;
        break;

    default:

        break;
    }
    return result;
}

/**
 *  Examples:
 *
 *  RPN:
 *          pitch =
 *              0xB0 0x65 0x00 |    // pitch range semitones (coarse)
 *              0xB0 0x64 0x00 |    // pitch range cents (fine)
 *              0xB0 0x06 0x02 |    // data entry slider coarse
 *              0xB0 0x26 0x00 |    // data entry slider fine
 *              0xB0 0x65 0x7F |    // reset coarse
 *              0xB0 0x64 0x7F      // reset fine
 *
 *  NRPN: From Roland EG-101_MI.pdf (paraphrased)
 *
 *          vibrato rate (relative change from -64dec to 0 to +63dec =
 *              0xB0 0x63 0x01 |    // MSB of parameter number
 *              0xB0 0x62 0x08 |    // LSB of parameter number
 *              0xB0 0x06 0xnn |    // MSB of data entry
 *              0xB0 0x26 0xnn |    // LSB of data entry (ignored, optional)
 *              0xB0 0x65 0x7F |    // reset coarse (recommended)
 *              0xB0 0x64 0x7F      // reset fine (optional)
 *
 *  Most parameters are set when the rpn object is constructed.
 *
 * Note:
 *
 *      rpn_number_to_bytes(} returns an array of two midibyte values in
 *      big-endian style:
 *
 *          MSB: result[0]
 *          LSB: result[1]
 *
 * \param channel
 *      The channel to use, re 0.
 *
 * \return
 *      Returns a vector of midibytes, each one representing an individual
 *      event. If empty, the function failed.
 */

const midimacro::events &
rpn::create_rpn_events (int channel)
{
    midimacro::events & result { event_bytes_list() };
    bool ok { channel >= 0 && channel < 16 };
    if (ok)
        ok = fix_settings();

    if (ok)
    {
        int evcount { 0 };
        midibyte cc { 0xB0 };
        midibyte ch { midibyte(channel) };
        midibytes pnbytes { rpn_number_to_bytes(parameter_number()) };
        if (pnbytes.size() == 2)
        {
            midibytes evbytes;
            cc |= ch;
            evbytes.push_back(cc);                      /* controller event */
            evbytes.push_back(0x65);                    /* RPN MSB flag     */
            evbytes.push_back(pnbytes[0]);              /* parameter MSB    */
            result.push_back(evbytes);                  /* push first event */
            ++evcount;

            evbytes.clear();
            evbytes.push_back(cc);                      /* controller event */
            evbytes.push_back(0x64);                    /* RPN LSB flag     */
            evbytes.push_back(pnbytes[1]);              /* parameter LSB    */
            result.push_back(evbytes);                  /* push next event  */
            ++evcount;
            is_valid(true);                             /* a bit tricky     */
            if (append_data())
            {
                midibytes vbytes { rpn_number_to_bytes(parameter_value()) };
                evbytes.clear();
                evbytes.push_back(cc);                  /* controller event */
                evbytes.push_back(0x06);                /* data slider MSB  */
                evbytes.push_back(vbytes[0]);           /* value MSB        */
                result.push_back(evbytes);              /* push next event */
                ++evcount;

                if (use_fine_rpn())
                {
                    evbytes.clear();
                    evbytes.push_back(cc);              /* controller event */
                    evbytes.push_back(0x26);            /* data slider LSB  */
                    evbytes.push_back(vbytes[1]);       /* value LSB        */
                    result.push_back(evbytes);          /* push next event  */
                    ++evcount;
                }
            }
            if (append_reset())
            {
                midibytes reset_msb { cc, 0x65, 0x7f }; /* (N)RPN reset MSB */
                midibytes reset_lsb { cc, 0x64, 0x7f }; /* (N)RPN reset LSB */
                result.push_back(reset_msb);
                ++evcount;
                result.push_back(reset_lsb);
                ++evcount;
            }
        }
        event_count(evcount);
    }
    return result;
}

/**
 *  This function takes the name and the array of events (and their bytes)
 *  and generates the data needed to add these events to the midimacros
 *  set as a midimacro.
 *
 *  This line should have the format "macnam = bytes "|" bytes ....
 *  This matches the format read from the 'ctrl' file.
 *
 * \param channel
 *      The channel to use, re 0.
 *
 * \return
 *      Returns a vector of midibytes, each one representing an individual
 *      event. If empty, the function failed.
 */

tokenization
rpn::create_rpn_macro_string (const std::string & macnam, int channel)
{
    const midimacro::events & evlist { create_rpn_events(channel) };
    int sz { int(evlist.size()) };
    tokenization result;
    if (sz > 0)
    {
        std::string tokens;
        int listcount { 0 };
        const char * fmt { "0x%02x" };
        result.push_back(macnam);
        for (const auto & evbyts : evlist)
        {
            for (auto b : evbyts)
            {
                char tmp[8];
                snprintf(tmp, sizeof tmp, fmt, b);
                tokens += tmp;
            }
            ++listcount;
            if (listcount < sz)
                tokens += " | ";
        }
        result.push_back(tokens);
    }
    tokens() = result;                  /* copy to the m_tokens member      */
    return result;
}

}           // namespace seq66

/*
 * rpn.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

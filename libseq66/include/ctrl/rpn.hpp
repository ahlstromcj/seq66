#if ! defined SEQ66_RPN_HPP
#define SEQ66_RPN_HPP

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
 * \file          rpn.hpp
 *
 *  This module declares/defines a class to support the full process of
 *  NRPN and RPN control.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2026-07-31
 * \updates       2026-08-11
 * \license       GNU GPLv2 or above
 *
 *  This class represents all the RPN and NRPN events needed to change
 *  a MIDI parameter.
 */

#include "ctrl/midimacro.hpp"           /* seq66::midimacro                 */

namespace seq66
{
    class event;

/**
 *  This class implements a set of RPN/NRPN/Data events.
 */

class rpn final : public midimacro
{
    friend class qrpnframe;

public:

    /**
     *  Indicates the kind of data to be sent to a device or stored
     *  as a macro. The non-RPN/NRPN controls allow later additions
     *  to existing RPN/NRPN events.
     */

    enum class control
    {
        rpn,                            /* RPN                              */
        nrpn,                           /* NRPN                             */
        slider,                         /* Data entry slider                */
        increment,                      /* Data button increment            */
        decrement,                      /* Data button decrement            */
        max                             /* terminator and illegal value     */
    };

public:

    /**
     *  Indicates which RPN parameter is to be applied.
     */

    enum class parameter
    {
        pitchbend_range         = 0,    /* pitchbend range, semitones.cents */
        channel_fine_tuning     = 1,    /* fine tuning, sub-semitone, cents */
        channel_coarse_tuning   = 2,    /* coarse tuning off 440 Hz, semis  */
        tuning_program_change   = 3,    /* rarely used, see RPN.text        */
        tuning_bank_select      = 4,    /* rarely used, see RPN.text        */
        modulation_depth_range  = 5,    /* manufacturer specific change     */
        mpe_configuration_msg   = 6,    /* MIDI polyphone expression zone   */
        parameter_reset         = 0x7F, /* provides 0x7F to end a change    */
        nrpn_active             = -1    /* indicates there's no set number  */
    };

public:

    /**
     *  RPN-related values grouped together for convenience, and for
     *  usage by qrpnframe.
     */

    struct info
    {
        /**
         *  Indicates if this object represents an NRPN rather than an
         *  RPN. Or is being used to append the parameter value.
         */

        control rpn_control_type { control::rpn };

        /**
         *  Indicates which RPN is in force.
         */

        parameter rpn_parameter_type { parameter::pitchbend_range };

        /**
         *  Indicates the intended time of the control insertion.
         */

        midipulse rpn_time_stamp { 0 };

        /**
         *  Holds the RPN or NRPN parameter selection. If negative, the
         *  selection has not yet been made. If RPN is in force,
         *  this number is the integer version of the parameter type.
         */

        midishort rpn_parameter_number { c_midishort_14_bad };

        /**
         *  Holds the parameter value to be set. It ranged from 0 to 16383
         *  (14-bits). The default value represents the pitch-bend range
         *  parameter value.
         */

        midishort rpn_parameter_value { 0 };

        /**
         *  The string representation of the parameter value. We
         *  need this to set a suitable default for the value.
         */

        std::string rpn_parameter_string { "0.0" };

        /**
         *  Indicates that the data-entry slider or increment/decrment
         *  will be applied.
         */

        bool rpn_append_data { true };

        /**
         *  Indicates that the recommended RPN reset parameter is
         *  to be applied.
         */

        bool rpn_append_reset { true };

        /**
         *  Indicates to apply the "fine" (e.g. cents) data settings.
         */

        bool rpn_use_fine_rpn { true };

    };              // struct info

    info m_info { };

public:

    rpn () = default;
    rpn (const info & rinfo);
    rpn
    (
        control control_type,
        parameter parameter_type,
        midipulse time_stamp,
        midishort rpn_parameter_value   = 0,
        midishort rpn_parameter_number  = c_midishort_14_bad,
        bool append_data                = true,
        bool append_reset               = true,
        bool use_fine_rpn               = true
    );
    rpn (const rpn &) = default;
    rpn (rpn &&) = default;
    rpn & operator = (const rpn &) = default;
    rpn & operator = (rpn &&) = default;
    virtual ~rpn () = default;

    static midishort parameter_to_short (parameter p)
    {
        return static_cast<midishort>(p);
    }

    bool fix_settings ();

    midipulse time_stamp () const
    {
        return m_info.rpn_time_stamp;
    }

    control control_type () const
    {
        return m_info.rpn_control_type;
    }

    parameter parameter_type () const
    {
        return m_info.rpn_parameter_type;
    }

    midishort parameter_number ()
    {
        return m_info.rpn_parameter_number;
    }

    midishort parameter_value () const
    {
        return m_info.rpn_parameter_value;
    }

    std::string parameter_string () const
    {
        return m_info.rpn_parameter_string;
    }

    bool append_data () const
    {
        return m_info.rpn_append_data;
    }

    bool append_reset () const
    {
        return m_info.rpn_append_reset;
    }

    bool use_fine_rpn () const
    {
        return m_info.rpn_use_fine_rpn;
    }

    const std::string & macro_name () const
    {
        return name();
    }

    const midimacro::events & create_rpn_events (int channel);
    tokenization create_rpn_macro_string
    (
        const std::string & macnam,
        int channel
    );

private:

    /*
     * qrpnframe is a friend who needs these, but for the most part
     * this object is constructed in whole when the "Create Macro"
     * or "Insert/Append" button is pressed.
     */

    void macro_name (const std::string & n)
    {
        name(n);
    }

};          // class rpn

}           // namespace seq66

#endif      // SEQ66_RPN_HPP

/*
 * rpn.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

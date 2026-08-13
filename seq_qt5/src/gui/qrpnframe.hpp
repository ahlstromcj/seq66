#if ! defined SEQ66_QRPNFRAME_HPP
#define SEQ66_QRPNFRAME_HPP

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
 * \file          qrpnframe.hpp
 *
 *  This module declares/defines the base class for the RPN window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2026-07-30
 * \updates       2026-08-12
 * \license       GNU GPLv2 or above
 *
 *  Provides a way to more easily add NRPN and RPN controller events.
 */

#include <QButtonGroup>
#include <QFrame>
#include <QWidget>

#include "ctrl/rpn.hpp"                 /* seq66::rpn "macro" class         */
#include "midi/calculations.hpp"        /* seq66::timeformat enumeration    */

/*
 *  Forward declarations for Qt.
 */

namespace Ui
{
    class qrpnframe;
}

class QButtonGroup;

namespace seq66
{

class performer;
class sequence;

class qrpnframe : public QFrame
{
    Q_OBJECT

#if defined SEQ66_PLATFORM_DEBUG_TMI
    static rpn::info sm_rpn_test_info;
#endif

public:

    explicit qrpnframe
    (
        performer & p,
        sequence & s,
        QWidget * parent = nullptr
    );
    ~qrpnframe ();

    performer & perf ()
    {
        return m_perf;
    }

    const performer & perf () const
    {
        return m_perf;
    }

    sequence & track ()
    {
        return m_seq;
    }

    const sequence & track () const
    {
        return m_seq;
    }

    rpn::info & rpn_info ()
    {
#if defined SEQ66_PLATFORM_DEBUG_TMI
        return sm_rpn_test_info;
#else
        return m_rpn_info;
#endif
    }

    const rpn::info & rpn_info () const
    {
#if defined SEQ66_PLATFORM_DEBUG_TMI
        return sm_rpn_test_info;
#else
        return m_rpn_info;
#endif
    }

private:

    void select_rpn_control (int rpncontrol);
    void set_rpn_option_checkboxes ();
    void select_rpn_parameter_type (int rpnvalue);
    void set_rpn_parameter_number (bool is_rpn, midishort pv);
    void set_rpn_parameter_value (midishort pv);
    void set_time_stamp (midipulse ts);

private slots:

    void slot_select_rpn_control (int r);
    void slot_midi_channel (int c);
    void slot_midi_buss (int b);
    void slot_rpn_append_data (int state);
    void slot_rpn_append_reset (int state);
    void slot_rpn_use_fine_rpn (int state);
    void slot_select_rpn_parameter_type (int v);
    void slot_next_time_format ();
    void slot_timestamp_text_changed ();
    void slot_param_number_text_changed ();
    void slot_param_value_text_changed ();
    void slot_show_in_hex ();
    void slot_macro_name_changed ();
    void slot_create_macro ();
    void slot_rpn_send ();
    void slot_rpn_insert ();
    void slot_rpn_cancel ();

private:

    /**
     *  The Qt user-interface object pointer.
     */

    Ui::qrpnframe * ui;

    /**
     *  Access to performer::send_macro().
     */

    performer & m_perf;

    /**
     *  Access to buss number and pattern number.
     */

    sequence & m_seq;

    /**
     * Easier way to group radio buttons than using QGroupBox.
     */

    QButtonGroup * m_select_control_group { nullptr };
    QButtonGroup * m_select_value_group { nullptr };

    /**
     *  Contains all of the parameters needed to create the
     *  rpn compound object.
     */

    rpn::info m_rpn_info { };

    /**
     *  The channel is set here instead of in rpn::info.
     *  Does this make sense?
     */

    int m_rpn_channel { 0 };

    /**
     *  The default buss is the MIDI Control Out buss.
     *  The user can select other busses.
     */

    int m_rpn_buss { null_buss() };     /* 0xFF */

    /**
     *  Holds the current time-format. See the calculations header
     *  file.
     */

    timeformat m_time_format { timeformat::bbt };

    /**
     *  Holds timing information from the performer and the sequence.
     *  Needed for converting between pulses and B:B:T format.
     */

    midi_timing m_midi_timing { };

    /**
     *  Holds the name of the macro to be created. Otherwise it
     *  is empty.
     */

    std::string m_macro_name { };

    /**
     *  Indicates to show numbers in hexadecimal format.
     */

    bool m_show_in_hex { false };

};          // class qrpnframe

}           // namespace seq66

#endif      // SEQ66_QRPNFRAME_HPP

/*
 * qrpnframe.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

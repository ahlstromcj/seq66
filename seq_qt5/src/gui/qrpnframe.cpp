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
 * \file          qrpnframe.cpp
 *
 *  This module declares/defines the base class for the LFO window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2026-07-30
 * \updates       2026-08-05
 * \license       GNU GPLv2 or above
 *
 *  The RPN dialog provides a way to enter RPN and NRPN controller events.
 *  It is easier than trying to add them in the event editor.
 *
 */

#include <QButtonGroup>

#include "play/performer.hpp"           /* seq66::performer                 */
#include "play/sequence.hpp"            /* seq66::sequence                  */
#include "qrpnframe.hpp"                /* seq66::qrpnframe                 */
#include "qt5_helpers.hpp"              /* seq66::qt()                      */
#include "ui_qrpnframe.h"

namespace seq66
{

/**
 *  Button numbers for the Select Control radio buttons.
 */

enum rpn_control_t
{
    rpn_control_rpn,
    rpn_control_nrpn,
    rpn_control_data_slider,
    rpn_control_data_incr,
    rpn_control_data_decr,
};

/**
 *  Button numbers for the Select RPN radio buttons.
 */

enum rpn_select_t
{
    rpn_select_pitchbend_range,
    rpn_select_channel_fine_tuning,
    rpn_select_channel_coarse_tuning,
    rpn_select_tuning_program_change,
    rpn_select_tuning_bank_select,
    rpn_select_modulation_depth_range,
    rpn_select_parameter_reset
};

/**
 *  Constructor. Some members are initialized in-class.
 */

qrpnframe::qrpnframe
(
    performer & p,
    sequence & s,
    QWidget * parent
) :
    QFrame  (parent),
    ui      (new Ui::qrpnframe),
    m_perf  (p),
    m_seq   (s)
{
    ui->setupUi(this);

    /*
     * Show the pattern number
     */

    char tmp[24];
    snprintf(tmp, sizeof tmp, "Pattern #%d", track().seq_number());
    ui->label_pattern_no->setText(tmp);

    /*
     * Create a button group to manage the mutual status of the Select
     * Control buttons.
     */

    m_select_control_group = new (std::nothrow) QButtonGroup(this);
    if (not_nullptr(m_select_control_group))
    {
        m_select_control_group->addButton
        (
            ui->radio_button_rpn, rpn_control_rpn
        );
        m_select_control_group->addButton
        (
            ui->radio_button_nrpn, rpn_control_nrpn
        );
        m_select_control_group->addButton
        (
            ui->radio_button_rpn_data_slider, rpn_control_data_slider
        );
        m_select_control_group->addButton
        (
            ui->radio_button_rpn_data_incr, rpn_control_data_incr
        );
        m_select_control_group->addButton
        (
            ui->radio_button_rpn_data_decr, rpn_control_data_decr
        );
        select_rpn(rpn_control_rpn);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        auto lambdafunc = [this] (QAbstractButton * abutton)
        {
            slot_select_rpn(m_select_control_group->id(abutton));
        };
        connect
        (
            m_select_control_group,
            &QButtonGroup::buttonClicked, lambdafunc
        );
#else
        connect
        (
            m_select_control_group, SIGNAL(buttonClicked(int)),
            this, SLOT(slot_select_rpn(int))
        );
#endif
    }

    /*
     * Create a button group to manage the mutual status of the Select
     * Control buttons.
     */

    m_select_value_group = new (std::nothrow) QButtonGroup(this);
    if (not_nullptr(m_select_value_group))
    {
        m_select_value_group->addButton
        (
            ui->radio_rpn_pitch_range, rpn_select_pitchbend_range
        );
        m_select_value_group->addButton
        (
            ui->radio_rpn_chan_fine_tuning, rpn_select_channel_fine_tuning
        );
        m_select_value_group->addButton
        (
            ui->radio_rpn_chan_coarse_tuning, rpn_select_channel_coarse_tuning
        );
        m_select_value_group->addButton
        (
            ui->radio_rpn_tune_program_change, rpn_select_tuning_program_change
        );
        m_select_value_group->addButton
        (
            ui->radio_rpn_tune_bank_select, rpn_select_tuning_bank_select
        );
        m_select_value_group->addButton
        (
            ui->radio_rpn_mod_depth_range, rpn_select_modulation_depth_range
        );
        m_select_value_group->addButton
        (
            ui->radio_rpn_parameter_reset, rpn_select_parameter_reset
        );
        select_rpn_value_type(rpn_select_pitchbend_range);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        auto lambdafunc = [this] (QAbstractButton * abutton)
        {
            slot_select_rpn_value_type(m_select_value_group->id(abutton));
        };
        connect
        (
            m_select_value_group,
            &QButtonGroup::buttonClicked, lambdafunc
        );
#else
        connect
        (
            m_select_value_group, SIGNAL(buttonClicked(int)),
            this, SLOT(slot_select_rpn_value_type(int))
        );

#endif
    }
}

qrpnframe::~qrpnframe()
{
    delete ui;
}

void
qrpnframe::select_rpn (int rpncontrol)
{
    printf("Controller (index) %d\n", rpncontrol);

    switch (rpncontrol)
    {
    case rpn_control_rpn:
        rpn_info().rpn_control_type = rpn::control::rpn;
        ui->radio_button_rpn->setChecked(true);
        break;

    case rpn_control_nrpn:
        rpn_info().rpn_control_type = rpn::control::nrpn;
        ui->radio_button_nrpn->setChecked(true);
        set_rpn_value_type_text(false, 0);
        break;

    case rpn_control_data_slider:
        rpn_info().rpn_control_type = rpn::control::slider;
        ui->radio_button_rpn_data_slider->setChecked(true);
        break;

    case rpn_control_data_incr:
        rpn_info().rpn_control_type = rpn::control::increment;
        ui->radio_button_rpn_data_incr->setChecked(true);
        break;

    case rpn_control_data_decr:
        rpn_info().rpn_control_type = rpn::control::decrement;
        ui->radio_button_rpn_data_decr->setChecked(true);
        break;
    }
}

void
qrpnframe::select_rpn_value_type (int rpnvalue)
{
    printf("RPN value type (index) %d\n", rpnvalue);
    switch (rpnvalue)
    {
    case rpn_select_pitchbend_range:

        rpn_info().rpn_parameter_type = rpn::parameter::pitchbend_range;
        ui->radio_rpn_pitch_range->setChecked(true);
        break;

    case rpn_select_channel_fine_tuning:

        rpn_info().rpn_parameter_type = rpn::parameter::channel_fine_tuning;
        ui->radio_rpn_chan_fine_tuning->setChecked(true);
        break;

    case rpn_select_channel_coarse_tuning:

        rpn_info().rpn_parameter_type = rpn::parameter::channel_coarse_tuning;
        ui->radio_rpn_chan_coarse_tuning->setChecked(true);
        break;

    case rpn_select_tuning_program_change:

        rpn_info().rpn_parameter_type = rpn::parameter::tuning_program_change;
        ui->radio_rpn_tune_program_change->setChecked(true);
        break;

    case rpn_select_tuning_bank_select:

        rpn_info().rpn_parameter_type = rpn::parameter::tuning_bank_select;
        ui->radio_rpn_tune_bank_select->setChecked(true);
        break;

    case rpn_select_modulation_depth_range:

        rpn_info().rpn_parameter_type = rpn::parameter::modulation_depth_range;
        ui->radio_rpn_mod_depth_range->setChecked(true);
        break;

    case rpn_select_parameter_reset:

        rpn_info().rpn_parameter_type = rpn::parameter::parameter_reset;
        ui->radio_rpn_parameter_reset->setChecked(true);
        break;
    }

    midishort pv { rpn::parameter_to_short(rpn_info().rpn_parameter_type) };
    set_rpn_value_type_text(true, pv);

    // TODO set this when NRPN is selected
    //
    // rpn_info().rpn_parameter_type = rpn::parameter::nrpn_active;
}

void
qrpnframe::set_rpn_value_type_text (bool is_rpn, midishort pv)
{
    std::string pvtext { std::to_string(int(pv)) };
    rpn_info().rpn_parameter_value = pv;
    ui->line_edit_nrpn_param_number->setText(qt(pvtext));
    ui->line_edit_nrpn_param_number->setReadOnly(is_rpn);
}

void
qrpnframe::slot_select_rpn (int r)
{
    select_rpn(r);
}

void
qrpnframe::slot_select_rpn_value_type (int v)
{
    select_rpn_value_type(v);
}

}               // namespace seq66

/*
 * qrpnframe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

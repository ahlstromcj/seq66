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
 * \updates       2026-08-10
 * \license       GNU GPLv2 or above
 *
 *  The RPN dialog provides a way to enter RPN and NRPN controller events.
 *  It is easier than trying to add them in the event editor.
 *
 */

#include <QButtonGroup>

#include "cfg/settings.hpp"             /* seq66::rc()                      */
#include "midi/controllers.hpp"         /* seq66::string_to_rpn_number()    */
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

#if defined SEQ66_PLATFORM_DEBUG_TMI

/*
 * For testing only.
 */

rpn::info qrpnframe::sm_rpn_test_info
{
    rpn::control::rpn,
    rpn::parameter::pitchbend_range,
    0,                                  /* time-stamp                       */
    0,                                  /* same as the pitchbend range      */
    1536,                               /* twelve semitones (12 << 7)       */
    "12.0",                             /* the string version of the above  */
    true, true, true
};

#endif

/**
 *  Constructor. Some members are initialized in-class.
 *
 *      m_select_control_group  { nullptr }
 *      m_select_value_group    { nullptr }
 *      m_rpn_info              { }
 *      m_rpn_channel           { 0 }
 *      m_time_format           { timeformat::bbt }
 */

qrpnframe::qrpnframe
(
    performer & p,
    sequence & s,
    QWidget * parent
) :
    QFrame          (parent),
    ui              (new Ui::qrpnframe),
    m_perf          (p),                    /* accessor: perf()             */
    m_seq           (s),                    /* accessor: track()            */
    m_midi_timing                           /* used in time calculations    */
    (
        perf().bpm(),
        track().get_beats_per_bar(),
        track().get_beat_width(),
        track().get_ppqn()
    )
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
        select_rpn_control(rpn_control_rpn);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        auto lambdafunc = [this] (QAbstractButton * abutton)
        {
            slot_select_rpn_control(m_select_control_group->id(abutton));
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
            this, SLOT(slot_select_rpn_control(int))
        );
#endif
    }

    /*
     * Populate the channel combo box, and default to the pattern's
     * current channel selection.
     */

    int ch { int(track().seq_midi_channel()) }; /* track().midi_channel()   */
    int b { int(track().seq_midi_bus()) };
    if (populate_midich_combo(ui->combo_box_channel, b, ch, false))
    {
        /*
         * That function selects the current channel. Now connect the
         * combo-box. Note that this setting does not affect the
         * pattern associated with this dialog; it affects only the
         * data in the (N)RPN-related events created.
         */

        connect
        (
            ui->combo_box_channel, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slot_midi_channel(int))
        );
    }

    /*
     * The checkboxes in the middle of the frame.
     */

    set_rpn_option_checkboxes ();
    connect
    (
        ui->check_box_rpn_append_data, SIGNAL(stateChanged(int)),
        this, SLOT(slot_rpn_append_data(int))
    );
    connect
    (
        ui->check_box_rpn_append_reset, SIGNAL(stateChanged(int)),
        this, SLOT(slot_rpn_append_reset(int))
    );
    connect
    (
        ui->check_box_rpn_use_fine, SIGNAL(stateChanged(int)),
        this, SLOT(slot_rpn_use_fine_rpn(int))
    );

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
        select_rpn_parameter_type(rpn_select_pitchbend_range);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        auto lambdafunc = [this] (QAbstractButton * abutton)
        {
            slot_select_rpn_parameter_type(m_select_value_group->id(abutton));
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
            this, SLOT(slot_select_rpn_parameter_type(int))
        );

#endif
    }

    /*
     * Set the timestamp. The BBT utton shows the current time format
     * and changes to the next one when clicked: B:B:T, H:M:S, and ticks.
     */

    set_time_stamp(track().get_tick());
    connect
    (
        ui->button_bbt, SIGNAL(clicked(bool)),
        this, SLOT(slot_next_time_format())
    );
    connect
    (
        ui->line_edit_rpn_timestamp, SIGNAL(editingFinished()),
        this, SLOT(slot_timestamp_text_changed())
    );

    /*
     * Show hex status.
     */

    ui->button_hex->setChecked(m_show_in_hex);
    connect
    (
        ui->button_hex, SIGNAL(clicked(bool)),
        this, SLOT(slot_show_in_hex())
    );

    /*
     * Line-edit for the parameter number.
     */

    int tempnumber { int(rpn_info().rpn_parameter_number) };
    set_rpn_parameter_number(true, tempnumber);
    connect
    (
        ui->line_edit_nrpn_param_number, SIGNAL(editingFinished()),
        this, SLOT(slot_param_number_text_changed())
    );

    /*
     * Line-edit for the parameter value.
     */

    int tempvalue { int(rpn_info().rpn_parameter_value) };
    set_rpn_parameter_value(tempvalue);
    connect
    (
        ui->line_edit_rpn_param_value, SIGNAL(editingFinished()),
        this, SLOT(slot_param_value_text_changed())
    );

    /*
     * Create Macro button and line-edit.
     *
     * ui->line_edit_rpn_macro_name->setText("");
     */

    ui->line_edit_rpn_macro_name->setPlaceholderText("(name of macro)");
    connect
    (
        ui->line_edit_rpn_macro_name, SIGNAL(editingFinished()),
        this, SLOT(slot_macro_name_changed())
    );
    connect
    (
        ui->button_rpn_macro, SIGNAL(clicked(bool)),
        this, SLOT(slot_create_macro())
    );
    ui->button_rpn_macro->setEnabled(false);

    /*
     * Hide the "Reserved" buttons.
     */

    ui->button_reserved_1->hide();
    ui->button_reserved_2->hide();

    /*
     * Insert (N)RPN button.
     */

    connect
    (
        ui->button_rpn_insert, SIGNAL(clicked(bool)),
        this, SLOT(slot_rpn_insert())
    );

    /*
     * Cancel button
     */

    connect
    (
        ui->button_rpn_cancel, SIGNAL(clicked(bool)),
        this, SLOT(slot_rpn_cancel())
    );
}

/**
 *  The destructor.
 */

qrpnframe::~qrpnframe()
{
    delete ui;
}

void
qrpnframe::select_rpn_control (int rpncontrol)
{
    switch (rpncontrol)
    {
    case rpn_control_rpn:
        rpn_info().rpn_control_type = rpn::control::rpn;
        ui->radio_button_rpn->setChecked(true);
        break;

    case rpn_control_nrpn:
        rpn_info().rpn_control_type = rpn::control::nrpn;
        ui->radio_button_nrpn->setChecked(true);
        set_rpn_parameter_number(false, 0);
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
qrpnframe::select_rpn_parameter_type (int rpnparamtype)
{
    rpn::parameter paramtype { rpn::parameter::nrpn_active };
    switch (rpnparamtype)
    {
    case rpn_select_pitchbend_range:

        paramtype = rpn::parameter::pitchbend_range;
        ui->radio_rpn_pitch_range->setChecked(true);
        break;

    case rpn_select_channel_fine_tuning:

        paramtype = rpn::parameter::channel_fine_tuning;
        ui->radio_rpn_chan_fine_tuning->setChecked(true);
        break;

    case rpn_select_channel_coarse_tuning:

        paramtype = rpn::parameter::channel_coarse_tuning;
        ui->radio_rpn_chan_coarse_tuning->setChecked(true);
        break;

    case rpn_select_tuning_program_change:

        paramtype = rpn::parameter::tuning_program_change;
        ui->radio_rpn_tune_program_change->setChecked(true);
        break;

    case rpn_select_tuning_bank_select:

        paramtype = rpn::parameter::tuning_bank_select;
        ui->radio_rpn_tune_bank_select->setChecked(true);
        break;

    case rpn_select_modulation_depth_range:

        paramtype = rpn::parameter::modulation_depth_range;
        ui->radio_rpn_mod_depth_range->setChecked(true);
        break;

    case rpn_select_parameter_reset:

        paramtype = rpn::parameter::parameter_reset;
        ui->radio_rpn_parameter_reset->setChecked(true);
        break;
    }
    rpn_info().rpn_parameter_type = paramtype;
    rpn_info().rpn_parameter_number = rpn::parameter_to_short(paramtype);
    select_rpn_control(rpn_control_rpn);

    midishort pt { rpn_info().rpn_parameter_number };
    set_rpn_parameter_number(true, pt);
}

void
qrpnframe::set_rpn_parameter_number (bool is_rpn, midishort pv)
{
    std::string pvtext { std::to_string(int(pv)) };
    if (m_show_in_hex)
    {
        const char * fmt { pv <= 0xFF ? "0x%02x" : "0x%04x" };
        char tmp[16];
        snprintf(tmp, sizeof tmp, fmt, pv);
        pvtext = tmp;
    }
    ui->line_edit_nrpn_param_number->setText(qt(pvtext));
    ui->line_edit_nrpn_param_number->setReadOnly(is_rpn);
}

void
qrpnframe::set_rpn_parameter_value (midishort pv)
{
    std::string pvtext { std::to_string(int(pv)) };
    if (m_show_in_hex)
    {
        const char * fmt { pv <= 0xFF ? "0x%02x" : "0x%04x" };
        char tmp[16];
        snprintf(tmp, sizeof tmp, fmt, pv);
        pvtext = tmp;
    }
    ui->line_edit_rpn_param_value->setText(qt(pvtext));
}

void
qrpnframe::slot_select_rpn_control (int r)
{
    select_rpn_control(r);
}

void
qrpnframe::slot_midi_channel (int c)
{
    m_rpn_channel = c;
}

void
qrpnframe::set_rpn_option_checkboxes ()
{
    bool appenddata { rpn_info().rpn_append_data };
    bool appendreset { rpn_info().rpn_append_reset };
    bool usefinerpn { rpn_info().rpn_use_fine_rpn };
    ui->check_box_rpn_append_data->setChecked(appenddata);
    ui->check_box_rpn_append_reset->setChecked(appendreset);
    ui->check_box_rpn_use_fine->setChecked(usefinerpn);
}

void
qrpnframe::slot_rpn_append_data (int state)
{
    bool checked { state == Qt::Checked };
    rpn_info().rpn_append_data = checked;
    set_rpn_option_checkboxes();
}

void
qrpnframe::slot_rpn_append_reset (int state)
{
    bool checked { state == Qt::Checked };
    rpn_info().rpn_append_reset = checked;
    set_rpn_option_checkboxes();
}

void
qrpnframe::slot_rpn_use_fine_rpn (int state)
{
    bool checked { state == Qt::Checked };
    rpn_info().rpn_use_fine_rpn = checked;
    set_rpn_option_checkboxes();
}

void
qrpnframe::slot_select_rpn_parameter_type (int v)
{
    select_rpn_parameter_type(v);
}

void
qrpnframe::set_time_stamp (midipulse ts)
{
    std::string tf;
    rpn_info().rpn_time_stamp = ts;
    switch (m_time_format)
    {
    case timeformat::bbt:

        tf = pulses_to_measurestring(ts, m_midi_timing);
        break;

    case timeformat::hms:

        tf = pulses_to_time_string(ts, m_midi_timing);
        break;

    case timeformat::ticks:

        tf = pulses_to_string(ts);
        break;
    }
    ui->line_edit_rpn_timestamp->setText(qt(tf));
}

/**
 *  Gets to the next time format.
 */

void
qrpnframe::slot_next_time_format ()
{
    m_time_format = next_time_format(m_time_format);

    midipulse ts { rpn_info().rpn_time_stamp };
    std::string tf { time_format_name(m_time_format) };
    ui->button_bbt->setText(qt(tf));
    set_time_stamp(ts);
}

void
qrpnframe::slot_timestamp_text_changed ()
{
    QString t { ui->line_edit_rpn_timestamp->text() };
    std::string ts { t.toStdString() };
    midipulse ticks { string_to_pulses(ts, m_midi_timing, m_time_format) };
    rpn_info().rpn_time_stamp = ticks;
}

void
qrpnframe::slot_param_number_text_changed ()
{
    QString p { ui->line_edit_nrpn_param_number->text() };
    std::string pnumstring { p.toStdString() };
    midishort pnumber { string_to_rpn_number(pnumstring) };
    rpn_info().rpn_parameter_number = pnumber;

    /*
     * If > 6 and not 7F, select NRPN, else select (check) an RPN parameter.
     */

    if (pnumber < 6 || pnumber == 0x7F)
    {
        select_rpn_control(rpn_control_rpn);
        select_rpn_parameter_type(pnumber);
    }
    else
    {
        select_rpn_control(rpn_control_nrpn);
    }
}

void
qrpnframe::slot_param_value_text_changed ()
{
    QString v { ui->line_edit_rpn_param_value->text() };
    std::string valstring { v.toStdString() };
    midishort value { string_to_rpn_number(valstring) };
    rpn_info().rpn_parameter_value = value;
}

void
qrpnframe::slot_show_in_hex ()
{
    bool isrpn { ui->radio_button_rpn->isChecked() };
    bool checked { ui->button_hex->isChecked() };
    midishort pnumber { rpn_info().rpn_parameter_number };
    m_show_in_hex = checked;
    set_rpn_parameter_number(isrpn, pnumber);
    pnumber = rpn_info().rpn_parameter_value;
    set_rpn_parameter_value(pnumber);
}

void
qrpnframe::slot_macro_name_changed ()
{
    QString v { ui->line_edit_rpn_macro_name->text() };
    std::string macnam { v.toStdString() };
    bool isempty { macnam.empty() };
    ui->button_rpn_macro->setEnabled(! isempty);
    m_macro_name = macnam;
}

/**
 *  For the next two slots, we first need to create an rpn object with the
 *  current rpn::info data supplied in the constructor in by editing
 *  the various user-interface elements in the qrpnframe.
 *
 *  performer sets this: rc().auto_ctrl_save(true);
 *
 *  TODO: If already present, delete and re-add the macro and
 *        mark it as "modified".
 */

void
qrpnframe::slot_create_macro ()
{
    std::string macnam { m_macro_name };
    int macchannel { m_rpn_channel };
    rpn r { rpn_info() };
    tokenization name_and_data
    {
        r.create_rpn_macro_string(macnam, macchannel)
    };
    bool ok { name_and_data.size() == 2 };
    if (ok)
    {
        midicontrolout & mco { perf().midi_control_out() };
        ok = mco.add_macro(name_and_data);
        if (ok)
            ok = mco.expand_macros();
    }
    if (ok)
    {
        ui->label_warning->setText(qt(name_and_data[1]));
        perf().notify_macro_change(m_macro_name, performer::macro::added);
    }
    else
    {
        ui->label_warning->setText("Error creating macro; duplicate name?");
    }
}

/*
 *  Compare this function to qseqeditframe64::insert_macro(). But we
 *  need to make sure that the vector of events-bytes is made. We also
 *  need to create the macro string for display.
 */

void
qrpnframe::slot_rpn_insert ()
{
    rpn r(rpn_info());
    midipulse tick { rpn_info().rpn_time_stamp };
    std::string macnam { m_macro_name };
    int macchannel { m_rpn_channel };
    tokenization name_and_data
    {
        r.create_rpn_macro_string(macnam, macchannel)
    };
    bool ok { name_and_data.size() == 2 };
    if (ok)
       ok = track().add_macro(tick, r);

    if (ok)
    {
        ui->label_warning->setText(qt(name_and_data[1]));
        perf().notify_macro_change(m_macro_name, performer::macro::inserted);
    }
    else
    {
        ui->label_warning->setText("Error inserting macro");
    }
}

void
qrpnframe::slot_rpn_cancel ()
{
    close();
}

}               // namespace seq66

/*
 * qrpnframe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

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
 * \updates       2026-08-17
 * \license       GNU GPLv2 or above
 *
 *  The RPN dialog provides a way to enter RPN and NRPN controller events.
 *  It is easier than trying to add them in the event editor.
 *
 */

#include "cfg/settings.hpp"             /* seq66::rc()                      */
#include "midi/controllers.hpp"         /* seq66::string_to_rpn_number()    */
#include "play/clockslist.hpp"          /* seq66::clockslist                */
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
    rpn_control_other
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
    rpn_select_mpe_configuration_message,
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
        m_select_control_group->addButton
        (
            ui->radio_button_other, rpn_control_other
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
     * Populate the buss combo box.
     */

    const clockslist & opm = output_port_map();
    mastermidibus * mmb = perf().master_bus();
    ui->combo_box_buss->addItem("Ctrl Out");    /* the default */
    if (not_nullptr(mmb))
    {
        int buses = opm.active() ? opm.count() : mmb->get_num_out_buses() ;
        for (int b = 0; b < buses; ++b)
        {
            e_clock ec;
            std::string busname;
            if (perf().ui_get_clock(bussbyte(b), ec, busname))
            {
                ui->combo_box_buss->addItem(qt(busname));
                if (port_unusable(ec))
                    enable_combobox_item(ui->combo_box_buss, b + 1, false);
            }
        }

        /*
         * Buss combo-box.  If we set a buss, we have to add
         * 1 to it to allow for the "Ctrl Out" entry.
         */

        connect
        (
            ui->combo_box_buss, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slot_midi_buss(int))
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
            ui->radio_rpn_mpe_config_msg, rpn_select_mpe_configuration_message
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
     * A line-editor to allow entering arbitrary bytes in order to
     * send data (ok?) or create a macro.
     */

    connect
    (
        ui->line_edit_other, SIGNAL(editingFinished()),
        this, SLOT(slot_macro_other_changed())
    );

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
     * Line-edit for the parameter value. We need the place-holder
     * text to be shown.
     *
     *      int tempvalue { int(rpn_info().rpn_parameter_value) };
     *      set_rpn_parameter_value(tempvalue);
     */

    connect
    (
        ui->line_edit_rpn_param_value, SIGNAL(editingFinished()),
        this, SLOT(slot_param_value_text_changed())
    );

    /*
     * Create Macro button, Macro Name line-edit, and Delete Macro button.
     *
     * ui->line_edit_rpn_macro_name->setText("");
     * ui->line_edit_rpn_macro_name->setPlaceholderText("(name of macro)");
     *
     * We're now going to use an editable combo-box instead.
     */

#if defined USE_LINE_EDIT_RPN_MACRO_NAME
    connect
    (
        ui->line_edit_rpn_macro_name, SIGNAL(editingFinished()),
        this, SLOT(slot_macro_name_changed())
    );
#else
    populate_macro_combo();
#endif

    connect
    (
        ui->button_rpn_macro, SIGNAL(clicked(bool)),
        this, SLOT(slot_create_macro())
    );
    connect
    (
        ui->button_rpn_delete, SIGNAL(clicked(bool)),
        this, SLOT(slot_delete_macro())
    );
    ui->button_rpn_macro->setEnabled(false);
    ui->button_rpn_delete->setEnabled(false);

    /*
     * Hide the "Reserved" buttons.
     *
     *  ui->button_reserved_1->hide() now used to send (N)RPN
     *  ui->button_reserved_2->hide() now used to delete a macro.
     */

    connect
    (
        ui->button_rpn_send, SIGNAL(clicked(bool)),
        this, SLOT(slot_rpn_send())
    );

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

/**
 *  Fills the combo-box for macro names. Similar to
 *  qsessionframe::populate_macro_combo(), but it does not add the
 *  bytes to this narrow dropdown.
 */

void
qrpnframe::populate_macro_combo ()
{
    tokenization names { perf().macro_names() };
    bool macrosactive { perf().macros_active() };
    if (macrosactive)
        macrosactive = ! names.empty();

    if (names.empty())
    {
        /*
         * This should never happen, as Seq66 has a few default
         * macros.
         */
    }
    else
    {
        int counter { 0 };
        int firstenabled { -1 };
        ui->combo_box_macro_name->clear();
        for (const auto & name : names)
        {
            if (name.empty())
            {
                break;
            }
            else
            {
                const midibytes & mbytes
                {
                    perf().macro_bytes(name)        /* bytes not used here */
                };
                bool enabled = ! mbytes.empty();
                if (enabled)
                {
                    enabled = name != "header" && name != "footer";
                    if (enabled && firstenabled == (-1))
                        firstenabled = counter;
                }

                QString combotext(qt(name));
                ui->combo_box_macro_name->insertItem(counter, combotext);
                enable_combobox_item
                (
                    ui->combo_box_macro_name, counter, enabled
                );
                ++counter;
            }
        }
        if (counter > 0)
        {
            if (firstenabled != (-1))
            {
                ui->combo_box_macro_name->setCurrentIndex(firstenabled);
                set_macro_name(ui->combo_box_macro_name->currentText());
            }
            else
            {
                m_macro_name.clear();
            }
            connect
            (
                ui->combo_box_macro_name, SIGNAL(currentIndexChanged(int)),
                this, SLOT(slot_pick_macro(int))
            );
            connect
            (
                ui->combo_box_macro_name->lineEdit(),
                SIGNAL(editingFinished()),
                this, SLOT(slot_macro_text())
            );
        }
    }
}

void
qrpnframe::select_rpn_control (int rpncontrol)
{
    bool use_arbitrary { false };
    ui->line_edit_other->setEnabled(false);
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

    case rpn_control_other:
        rpn_info().rpn_control_type = rpn::control::other;
        ui->radio_button_other->setChecked(true);
        ui->line_edit_other->setEnabled(true);
        use_arbitrary = true;
        break;
    }
    m_arbitrary_macro_in_force = use_arbitrary;
    m_arbitrary_macro_tokens.clear();
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

    case rpn_select_mpe_configuration_message:

        paramtype = rpn::parameter::mpe_configuration_msg;
        ui->radio_rpn_mpe_config_msg->setChecked(true);
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
qrpnframe::slot_midi_buss (int b)
{
    m_rpn_buss = b == 0 ? null_buss() : b - 1 ;
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
 * The user can do these steps in various orders:
 *
 *      -   Select the Other radio button.
 *      -   Select or edit the macro combo-box.
 *      -   Edit the "Other data" field.
 *
 *  We should not enable the "Other data" until Other is checked
 *  and a macro-name is in place.
 */

void
qrpnframe::slot_macro_other_changed ()
{
#if defined USE_LINE_EDIT_RPN_MACRO_NAME
    QString m { ui->line_edit_rpn_macro_name->text() };
#else
    QString m { ui->combo_box_macro_name->currentText() };
#endif
    QString b { ui->line_edit_other->text() };
    std::string macnam { m.toStdString() };     /* current macro name!  */
    std::string byts { b.toStdString() };
    tokenization macpair;
    macpair.push_back(macnam);
    macpair.push_back(byts);
    m_arbitrary_macro_in_force = true;
    m_arbitrary_macro_tokens = macpair;
    ui->button_rpn_macro->setEnabled(true);
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

/**
 * Handles the interaction between the macro-buttons.
 */

void
qrpnframe::slot_macro_name_changed ()
{
#if defined USE_LINE_EDIT_RPN_MACRO_NAME
    QString m { ui->line_edit_rpn_macro_name->text() };
#else
    QString m { ui->combo_box_macro_name->currentText() };
#endif
    std::string macnam { m.toStdString() };
    bool isempty { macnam.empty() };
    if (isempty)
    {
        ui->button_rpn_macro->setText("Create Macro");
        ui->button_rpn_delete->setEnabled(false);
        ui->button_rpn_macro->setEnabled(false);
        ui->line_edit_other->clear();               /* setText(""); */
        ui->line_edit_other->setEnabled(false);
        m_macro_name.clear();
        m_arbitrary_macro_in_force = false;
        m_arbitrary_macro_tokens.clear();
    }
    else
    {
        midicontrolout & mco { perf().midi_control_out() };
        bool found { mco.find_macro(macnam) };
        ui->button_rpn_macro->setEnabled(true);
        m_macro_name = macnam;
        if (found)
        {
            ui->button_rpn_delete->setEnabled(true);
            ui->button_rpn_macro->setText("Modify Macro");
        }
        else
            ui->button_rpn_macro->setText("Create Macro");

        ui->button_rpn_macro->setEnabled(true);
    }
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
    std::string macnam { m_macro_name };        /* selected macro's name    */
    midicontrolout & mco { perf().midi_control_out() };
    bool found { mco.find_macro(macnam) };
    bool ok { false };
    if (m_arbitrary_macro_in_force)
    {
        if (m_arbitrary_macro_tokens.size() < 2)
            return;

        std::string values { m_arbitrary_macro_tokens[1] };
        m_arbitrary_macro_tokens[0] = macnam;   /* set the name to be sure  */

        midimacro mac(macnam, values);
        if (found)
        {
            ok = mco.modify_macro(m_arbitrary_macro_tokens);
            if (ok)
                ok = mco.expand_macro(macnam);

            if (ok)
            {
                ui->plain_text_edit_msg->setPlainText(qt(values));
                ui->button_rpn_macro->setEnabled(false);
                perf().notify_macro_change(macnam, performer::macro::modified);
            }
            else
                ui->plain_text_edit_msg->setPlainText("Error modifying macro");
        }
        else
        {
            /*
             * See the discussion at midimacros::expand(name).
             */

            ok = mco.add_macro(m_arbitrary_macro_tokens);
            if (ok)
                ok = mco.expand_macro(macnam);

            if (ok)
            {
                ui->plain_text_edit_msg->setPlainText(qt(values));
                ui->button_rpn_macro->setEnabled(false);
                perf().notify_macro_change(macnam, performer::macro::added);
            }
            else
            {
                ui->plain_text_edit_msg->setPlainText
                (
                    "Error creating macro; duplicate name?"
                );
            }
        }
    }
    else
    {
        int macchannel { m_rpn_channel };
        rpn r { rpn_info() };
        tokenization name_and_data
        {
            r.create_rpn_macro_string(macnam, macchannel)
        };
        if (name_and_data.size() < 2)
            return;

        std::string values { name_and_data[1] };
        if (found)
        {
            ok = mco.modify_macro(name_and_data);
            if (ok)
                ok = mco.expand_macro(macnam);

            if (ok)
            {
                ui->plain_text_edit_msg->setPlainText(qt(values));
                ui->button_rpn_macro->setEnabled(false);
                perf().notify_macro_change(macnam, performer::macro::modified);
            }
            else
            {
                ui->plain_text_edit_msg->setPlainText
                (
                    "Error modifying RPN macro"
                );
            }
        }
        else
        {
            ok = mco.add_macro(name_and_data);
            if (ok)
                ok = mco.expand_macro(macnam);

            if (ok)
            {
                ui->plain_text_edit_msg->setPlainText(qt(name_and_data[1]));
                ui->button_rpn_macro->setEnabled(false);
                perf().notify_macro_change(macnam, performer::macro::added);
            }
            else
            {
                ui->plain_text_edit_msg->setPlainText
                (
                    "Error creating RPN macro; duplicate name?"
                );
            }
        }
    }
}

void
qrpnframe::set_macro_name (const QString & name)
{
    std::string macnam { name.toStdString() };
    ui->line_edit_rpn_macro_name->setText(name);
    m_macro_name = macnam;

    /*
     * Rename this function at some point.
     */

    slot_macro_name_changed();

    midicontrolout & mco { perf().midi_control_out() };
    bool found { mco.find_macro(macnam) };
    if (found)
    {
        const midimacro & mac { mco.macro(macnam) };
        std::string line { mac.line() };
        ui->plain_text_edit_msg->setPlainText(qt(line));
    }
}

/**
 *  An alternate way of getting the text here is
 *
 *      QString name { ui->combo_box_macro_name->itemText(index) };
 */

void
qrpnframe::slot_pick_macro (int index)
{
    (void) index;

    QString name { ui->combo_box_macro_name->currentText() };
    set_macro_name(name);
}

void
qrpnframe::slot_macro_text ()
{
    QString name { ui->combo_box_macro_name->currentText() };
    int counter { ui->combo_box_macro_name->count() };
    int index { ui->combo_box_macro_name->findText(name) };
    bool notfound { index == (-1) };
    if (notfound)
        ui->combo_box_macro_name->insertItem(counter, name);

    set_macro_name(name);
    ui->button_rpn_macro->setEnabled(true);
}

void
qrpnframe::slot_delete_macro ()
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
        ok = mco.delete_macro(name_and_data[0]);
        if (ok)
            ok = mco.expand_macros();
    }
    if (ok)
    {
        ui->plain_text_edit_msg->setPlainText(qt(name_and_data[1]));
        perf().notify_macro_change(m_macro_name, performer::macro::added);
    }
    else
    {
        ui->plain_text_edit_msg->setPlainText
        (
            "Error creating macro; duplicate name?"
        );
    }
}

void
qrpnframe::modify_macro ()
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
        ok = mco.modify_macro(name_and_data);
        if (ok)
            ok = mco.expand_macros();
    }
    if (ok)
    {
        ui->plain_text_edit_msg->setPlainText(qt(name_and_data[1]));
        perf().notify_macro_change(m_macro_name, performer::macro::added);
    }
    else
    {
        ui->plain_text_edit_msg->setPlainText
        (
            "Error creating macro; duplicate name?"
        );
    }
}

/**
 *  Send the (N)RPN message out on the MIDI Control Out buss.
 */

void
qrpnframe::slot_rpn_send ()
{
    rpn r(rpn_info());
    std::string macnam { m_macro_name };
    int macchannel { m_rpn_channel };
    int macbuss { m_rpn_buss };
    tokenization name_and_data
    {
        r.create_rpn_macro_string(macnam, macchannel)
    };
    bool ok { name_and_data.size() == 2 };
    if (ok)
    {
       /*
        * This call uses the macro bytes, not the macro name. If macbuss
        * is null (0xFF), then midicontrolout::send_macro() is called,
        * and it uses it's true_buss() function to call
        * masterbus::play_and_flush(). Otherwise, we need to call
        * true_buss() here.
        */

       ok = perf().send_macro_bytes(r, macbuss);
    }

    if (ok)
    {
        ui->plain_text_edit_msg->setPlainText(qt(name_and_data[1]));
        perf().notify_macro_change(m_macro_name, performer::macro::sent);
    }
    else
    {
        ui->plain_text_edit_msg->setPlainText("Error sending macro");
    }
}

/**
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
    if (m_arbitrary_macro_in_force)
    {
        midimacro mac
        (
            m_arbitrary_macro_tokens[0], m_arbitrary_macro_tokens[1]
        );
        bool ok { track().add_macro(tick, r) };
        if (ok)
        {
            perf().notify_macro_change
            (
                m_macro_name, performer::macro::inserted
            );
        }
        else
        {
            ui->plain_text_edit_msg->setPlainText
            (
                "Error inserting arbitrary  macro"
            );
        }
    }
    else
    {
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
            ui->plain_text_edit_msg->setPlainText(qt(name_and_data[1]));
            perf().notify_macro_change
            (
                m_macro_name, performer::macro::inserted
            );
        }
        else
        {
            ui->plain_text_edit_msg->setPlainText("Error inserting macro");
        }
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

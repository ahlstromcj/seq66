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
 * \file          qlearnframe.cpp
 *
 *  This module declares/defines the MIDI Learn dialog.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2026-06-08
 * \updates       2026-09-08
 * \license       GNU GPLv2 or above
 *
 *  This dialog provides a way to initiate the MIDI learning of some
 *  of our controls.
 */

#include "ctrl/automation.hpp"          /* seq66::slot_to_string()          */
#include "ctrl/midilearn.hpp"           /* seq66::midilearn class           */
#include "qlearnframe.hpp"              /* seq66::qlearnframe gui           */
#include "qt5_helpers.hpp"              /* seq66::qt_timer() and qt()       */
#include "ui_qlearnframe.h"

namespace seq66
{

/**
 *  Button numbering for Learn-mode radio buttons. This is the order that
 *  we set up in the constructor.
 */

enum learn_mode_button_t
{
    learn_mode_button_loops,
    learn_mode_button_mutes,
    learn_mode_button_automation
};

/**
 *  Button numbering for Action radio buttons.
 */

enum action_mode_button_t
{
    action_mode_button_toggle,
    action_mode_button_on,
    action_mode_button_off,
    action_mode_button_hold
};

/*
 *  To be beefed up.
 */

qlearnframe::qlearnframe
(
    performer & p,
    automation::category opcat,
    QWidget * parent
) :
    QFrame                  (parent),
    performer::callbacks    (p),
    ui                      (new Ui::qlearnframe),
    m_perf                  (p),
    m_midi_learn            (*p.create_midi_learn())    /* fingers crossed! */
{
    ui->setupUi(this);
    connect
    (
        ui->cancel_push_button, SIGNAL(clicked()), this, SLOT(slot_cancel())
    );
    connect
    (
        ui->save_push_button, SIGNAL(clicked()), this, SLOT(slot_save())
    );
    connect
    (
        ui->reset_push_button, SIGNAL(clicked()), this, SLOT(slot_reset())
    );
    set_clear_text(opcat);
    connect
    (
        ui->clear_push_button, SIGNAL(clicked()),
        this, SLOT(slot_clear())
    );
    connect
    (
        ui->clear_all_push_button, SIGNAL(clicked()),
        this, SLOT(slot_clear_all())
    );
    connect
    (
        ui->ok_push_button, SIGNAL(clicked()), this, SLOT(slot_ok())
    );
    set_buttons(false);

    /*
     * Create a button group to manage the mutual status of the Learn
     * Mode buttons.
     */

    m_learn_button_group = new (std::nothrow) QButtonGroup(this);
    if (not_nullptr(m_learn_button_group))
    {
        m_learn_button_group->addButton
        (
            ui->loops_button, learn_mode_button_loops
        );
        m_learn_button_group->addButton
        (
            ui->mutes_button, learn_mode_button_mutes
        );
        m_learn_button_group->addButton
        (
            ui->automation_button, learn_mode_button_automation
        );
        select_category(opcat);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        auto lambdafunc = [this] (QAbstractButton * abutton)
        {
            slot_select_category(m_learn_button_group->id(abutton));
        };
        connect
        (
            m_learn_button_group, &QButtonGroup::buttonClicked, lambdafunc
        );
#else
        connect
        (
            m_learn_button_group, SIGNAL(buttonClicked(int)),
            this, SLOT(slot_select_category(int))
        );
#endif

    }
    ui->loops_line_edit->setReadOnly(true);
    ui->loops_line_edit->setText("0");
    ui->mutes_line_edit->setReadOnly(true);
    ui->mutes_line_edit->setText("0");
    ui->automation_line_edit->setReadOnly(true);
    ui->automation_line_edit->setText("0");

    /*
     * Create a button group to manage the mutual status of the Action
     * buttons.
     */

    m_action_button_group = new (std::nothrow) QButtonGroup(this);
    if (not_nullptr(m_action_button_group))
    {
        m_action_button_group->addButton
        (
            ui->radio_action_toggle, action_mode_button_toggle
        );
        m_action_button_group->addButton
        (
            ui->radio_action_on, action_mode_button_on
        );
        m_action_button_group->addButton
        (
            ui->radio_action_off, action_mode_button_off
        );
        m_action_button_group->addButton
        (
            ui->radio_action_hold, action_mode_button_hold
        );
        select_action(midi_learn().automation_action());

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

        auto actionfunc = [this] (QAbstractButton * abutton)
        {
            slot_select_action(m_action_button_group->id(abutton));
        };
        connect(m_action_button_group, &QButtonGroup::buttonClicked, actionfunc);

#else

        connect
        (
            m_action_button_group, SIGNAL(buttonClicked(int)),
            this, SLOT(slot_select_action(int))
        );

#endif

    }
    connect
    (
        ui->inverse_check_box, SIGNAL(clicked(bool)),
        this, SLOT(slot_inverse())
    );

    /*
     * Store some value in the midilearn object.
     */

    midi_learn().inverse(m_inverse);
    midi_learn().d1min(m_d1min);
    midi_learn().d1max(m_d1max);
    connect
    (
        ui->d1min_line_edit, SIGNAL(editingFinished()),
        this, SLOT(slot_d1min())
    );
    ui->reserved_push_button->hide();
    ui->current_logged_control_line_edit->setReadOnly(true);
    ui->current_logged_control_line_edit->setText("None");
    perf().enregister(this);                    /* set for notification     */

    /*
     * Set up the performer's midilearn object. Then count the
     * number of MIDI control events that are active. Actually,
     * we're counting on the creation of a midilearn to work.
     * See the member initialization list.
     *
     *      if (perf().create_midi_learn())
     */

    update_active_counts();

    /**
     * Could set it to "last index"
     *
     * midi_learn().clear_current_index();
     */

    midi_learn().initialize_current_index();

    /*
     * Check for a pending automation-control every
     * 200 = 5 x 40 milliseconds. But let's go a little slower, about
     * 4 times a second.
     */

    m_timer = qt_timer(this, "qlearnframe", 6, SLOT(slot_poll_update()));
}

qlearnframe::~qlearnframe()
{
    perf().unregister(this);
    if (not_nullptr(m_timer))
        m_timer->stop();

    delete ui;
}

bool
qlearnframe::on_automation_change (automation::slot s)
{
    if (midi_learn().is_automation())
    {
        std::string eventname
        {
            "Auto " + automation::slot_to_string(s)
        };
        ui->current_logged_control_line_edit->setText(qt(eventname));
        refresh();
    }
    return true;
}

bool
qlearnframe::on_midi_learn (seq66::event ev)
{
    /*
     * Here, we can log the slot, category, and action (the latter two
     * when selected) with the midilearn object and shorten the
     * parameter list here.
     *
     *      perf().midi_learn()->learn_control
     *
     * The learn_control() function also sets the current slot to
     * "none".
     *
     * on_automation_change() handles showing the last automation item
     * clicked in the user interface.
     */

    bool result { ev.get_status() > 0x00 };
    if (result)
    {
        set_buttons(true);

        std::string eventname;
        int index { midi_learn().current_index() };
        if (midi_learn().is_loop())
        {
            eventname = "Loop " + std::to_string(index);
        }
        else if (midi_learn().is_mute_group())
        {
            eventname = "Mute Group " + std::to_string(index);
        }
        if (! eventname.empty())
        {
            QString txt { qt(eventname) };
            ui->current_logged_control_line_edit->setText(txt);
        }
        update_active_counts();
    }
    return result;
}

/**
 *  Set the text of the Clear button to the current category.
 */

void
qlearnframe::set_clear_text (automation::category c)
{
    std::string text { "Clear " };
    text += category_to_string(c);
    ui->clear_push_button->setText(qt(text));
}

/**
 *  Handles enabling and disabling of the controls.
 */

void
qlearnframe::set_buttons (bool enable)
{
    ui->save_push_button->setEnabled(enable);
    ui->reset_push_button->setEnabled(enable);
    ui->clear_push_button->setEnabled(enable);
    ui->clear_all_push_button->setEnabled(enable);
    ui->ok_push_button->setEnabled(enable);
}

/**
 *  Here, we poll for the current last-automation value to be able to
 *  display it in the user-interface.
 *
 *  This value will be cleared once a MIDI controller event comes in.
 *
 *      if (last != midi_learn().automation_slot() || m_refresh)
 */

void
qlearnframe::slot_poll_update ()
{
    automation::slot last { perf().last_automation_slot() };
    int index { midi_learn().current_index() };
    std::string eventname;
    if (midi_learn().is_loop())
    {
        ui->current_logged_control_label->setText("Loop Control");
        eventname = "Loop " + std::to_string(index);
    }
    else if (midi_learn().is_mute_group())
    {
        ui->current_logged_control_label->setText("Mute Control");
        eventname = "Mute Group " + std::to_string(index);
    }
    else if (midi_learn().is_automation())
    {
        ui->current_logged_control_label->setText("Next Control");
        eventname = "Auto " + automation::slot_to_string(last);
        midi_learn().automation_slot(last);
    }
    if (eventname.empty())
    {
        // No code
    }
    else
    {
        QString txt { qt(eventname) };
        ui->current_logged_control_line_edit->setText(txt);
    }
    m_refresh = false;
}

void
qlearnframe::update_active_counts ()
{
    int lcount;
    int mcount;
    int acount;
    if (midi_learn().active_counts(lcount, mcount, acount))
    {
        QString lcqs { QString::number(lcount) };
        QString mcqs { QString::number(mcount) };
        QString acqs { QString::number(acount) };
        ui->loops_line_edit->setText(lcqs);
        ui->mutes_line_edit->setText(mcqs);
        ui->automation_line_edit->setText(acqs);

        bool gotsome { lcount > 0 || mcount > 0 || acount > 0 };
        ui->clear_all_push_button->setEnabled(gotsome);

        automation::category c { midi_learn().automation_category() };
        gotsome = false;
        if (c == automation::category::loop)
        {
            if (lcount > 0)
                gotsome = true;
        }
        else if (c == automation::category::mute_group)
        {
            if ( mcount > 0)
                gotsome = true;
        }
        else if (c == automation::category::automation)
        {
            if ( acount > 0)
                gotsome = true;
        }
        ui->clear_push_button->setEnabled(gotsome);
    }
}

void
qlearnframe::select_category (automation::category opcat)
{
    bool ok
    {
        opcat != automation::category::none &&
        opcat != automation::category::max
    };
    if (ok)
    {
        int targetid;
        if (opcat == automation::category::loop)
        {
            targetid = learn_mode_button_loops;
            setup_loop_process();
        }
        else if (opcat == automation::category::mute_group)
        {
            targetid = learn_mode_button_mutes;
            setup_mutes_process();
        }
        else
        {
            targetid = learn_mode_button_automation;
            setup_automation_process();
        }
        m_learn_button_group->button(targetid)->setChecked(true);
        midi_learn().initialize_current_index();
        m_refresh = true;
    }
}

void
qlearnframe::slot_select_category (int buttonno)
{
    if (buttonno == learn_mode_button_loops)
    {
        setup_loop_process();
    }
    else if (buttonno == learn_mode_button_mutes)
    {
        setup_mutes_process();
    }
    else if (buttonno == learn_mode_button_automation)
    {
        setup_automation_process();
    }
    midi_learn().initialize_current_index();
    m_refresh = true;
}

/*
 * Uses the index in midilearn.
 */

void
qlearnframe::setup_loop_process ()
{
    midi_learn().automation_category(automation::category::loop);
    set_clear_text(automation::category::loop);
}

void
qlearnframe::setup_mutes_process ()
{
    midi_learn().automation_category(automation::category::mute_group);
    set_clear_text(automation::category::mute_group);
}

void
qlearnframe::setup_automation_process ()
{
    midi_learn().automation_category(automation::category::automation);
    set_clear_text(automation::category::automation);
}

void
qlearnframe::select_action (automation::action opact)
{
    bool ok
    {
        opact != automation::action::none &&
        opact != automation::action::max
    };
    if (ok)
    {
        int targetid;
        if (opact == automation::action::toggle)
            targetid = action_mode_button_toggle;
        else if (opact == automation::action::on)
            targetid = action_mode_button_on;
        else
            targetid = action_mode_button_off;

        m_action_button_group->button(targetid)->setChecked(true);

        /*
         * midi_learn().automation_action(opcat);
         */
    }
}

void
qlearnframe::slot_select_action (int buttonno)
{
    automation::action opact { automation::action::none };
    if (buttonno == action_mode_button_toggle)
        opact = automation::action::toggle;
    else if (buttonno == action_mode_button_on)
        opact = automation::action::on;
    else if (buttonno == action_mode_button_off)
        opact = automation::action::off;
    else if (buttonno == action_mode_button_hold)
        opact = automation::action::hold;

    midi_learn().automation_action(opact);
}

void
qlearnframe::slot_cancel ()
{
    perf().delete_midi_learn();
    close();
}

/**
 *  Using more direct access than the following:
 *
 *      if (not_nullptr(perf().midi_learn()))
 *          perf().midi_learn()->save();
 */

void
qlearnframe::slot_save ()
{
    if (midi_learn().save())
        ui->save_push_button->setEnabled(false);
}

void
qlearnframe::slot_reset ()
{
    if (midi_learn().reset())
    {
        ui->reset_push_button->setEnabled(false);
        update_active_counts();
    }
}

void
qlearnframe::slot_clear ()
{
    automation::category c { midi_learn().automation_category() };
    midi_learn().clear(c);
    update_active_counts();
}

void
qlearnframe::slot_clear_all ()
{
    (void) midi_learn().clear_all();
    update_active_counts();
}

void
qlearnframe::slot_ok ()
{
    slot_save();
    slot_cancel();
}

void
qlearnframe::slot_inverse ()
{
    m_inverse = ui->inverse_check_box->isChecked();
    midi_learn().inverse(m_inverse);
}

void
qlearnframe::slot_d1min ()
{
    QString text { ui->d1min_line_edit->text() };
    std::string t { text.toStdString() };
    if (! t.empty())
    {
        int d1min { std::stoi(t, nullptr, 0) };
        m_d1min = d1min;
        midi_learn().d1min(m_d1min);
    }
}

void
qlearnframe::slot_d1max ()
{
    QString text { ui->d1max_line_edit->text() };
    std::string t { text.toStdString() };
    if (! t.empty())
    {
        int d1max { std::stoi(t, nullptr, 0) };
        m_d1max = d1max;
        midi_learn().d1max(m_d1max);
    }
}

}               // namespace seq66

/*
 * qlearnframe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

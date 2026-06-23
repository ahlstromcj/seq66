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
 *  This module declares/defines the base class for the pattern-fix window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2026-06-08
 * \updates       2026-06-23
 * \license       GNU GPLv2 or above
 *
 *  This dialog provides a way to combine the following pattern adjustments:
 *
 *      -   Left-alignment.
 *      -   Fitting to a given number of measures.
 *      -   Arbitrary scaling for compression and expansion over time.
 *      -   Quantization or tightening.
 *
 *  It acts on all events in the track; no selection needed.  In this, it is
 *  similar to the LFO dialog.
 *
 *  This dialog was inspired by Ahlstrom's poor playing and timing skills.
 */

#include <QButtonGroup>
#include <QTimer>

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
    m_midi_learn            (*p.create_midi_learn()),   /* fingers crossed! */
    m_timer                 (nullptr),
    m_learn_button_group    (nullptr),
    m_action_button_group   (nullptr),
    m_current_keyname       (),
    m_inverse               (false),
    m_d1min                 (0),
    m_d1max                 (127)
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
    connect
    (
        ui->clear_push_button, SIGNAL(clicked()), this, SLOT(slot_clear())
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

#if defined QT_VERSION_5

        connect
        (
            m_learn_button_group, SIGNAL(buttonClicked(int)),
            this, SLOT(slot_select_category(int))
        );

#elif defined QT_VERSION_6 || defined QT_VERSION_7

        auto lambdafunc = [this] (QAbstractButton * abutton)
        {
            slot_select_category(m_learn_button_group->id(abutton));
        };
        connect
        (
            m_learn_button_group, &QButtonGroup::buttonClicked, lambdafunc
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

#if defined QT_VERSION_5

        connect
        (
            m_action_button_group, SIGNAL(buttonClicked(int)),
            this, SLOT(slot_select_action(int))
        );

#elif defined QT_VERSION_6 || defined QT_VERSION_7

        auto actionfunc = [this] (QAbstractButton * abutton)
        {
            slot_select_action(m_action_button_group->id(abutton));
        };
        connect(m_action_button_group, &QButtonGroup::buttonClicked, actionfunc);

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

    /*
     * Check for a pending automation-control every 5 x 40 milliseconds.
     */

    m_timer = qt_timer(this, "qlearnframe", 5, SLOT(slot_poll_update()));
}

qlearnframe::~qlearnframe()
{
    perf().unregister(this);
    if (not_nullptr(m_timer))
        m_timer->stop();

    delete ui;
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
     */

#if 0
    bool result
    {
        midi_learn().learn_control
        (
            ev, "keyname", m_inverse, m_d1min, m_d1max
        )
    };
#else
    bool result { ev.get_status() > 0x00 };
#endif

    if (result)
    {
        set_buttons(true);
        update_active_counts();

        /*
         * Moved to midilearn:
         *
         * if (m_automation_category != automation::category::automation)
         *     ++m_control_index;
         */
    }
    return result;
}

// ALSO NEED TO MODIFY, SET DIRTY, and ALSO IGNORE follow-on events.

/**
 *  Handles enabling and disabling of the controls.
 */

void
qlearnframe::set_buttons (bool enable)
{
    ui->save_push_button->setEnabled(enable);
    ui->reset_push_button->setEnabled(enable);
    ui->clear_push_button->setEnabled(enable);
    ui->ok_push_button->setEnabled(enable);
}

/**
 *  Here, we poll for the current last-automation value to be able to
 *  display it in the user-interface.
 *
 *  This value will be cleared once a MIDI controller event comes in.
 */

void
qlearnframe::slot_poll_update ()
{
    automation::slot last { perf().last_automation_slot() };
    if (last != midi_learn().automation_slot())
    {
        std::string eventname;
        if (midi_learn().is_loop())
        {
            eventname = "Loop " +
                std::to_string(midi_learn().current_index());
        }
        else if (midi_learn().is_mute_group())
        {
            eventname = "Mute Group " +
                std::to_string(midi_learn().current_index());
        }
        else if (midi_learn().is_automation())
        {
            eventname = "Automation " + automation::slot_to_string(last);
        }
        if (! eventname.empty())
        {
            QString txt { qt(eventname) };
            ui->current_logged_control_line_edit->setText(txt);
        }
        midi_learn().automation_slot(last);
    }
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
    }
}

void
qlearnframe::slot_select_category (int buttonno)
{
    automation::category opcat { automation::category::none };
    if (buttonno == learn_mode_button_loops)
    {
        opcat = automation::category::loop;
        setup_loop_process();
    }
    else if (buttonno == learn_mode_button_mutes)
    {
        opcat = automation::category::mute_group;
        setup_mutes_process();
    }
    else if (buttonno == learn_mode_button_automation)
    {
        opcat = automation::category::automation;
        setup_automation_process();
    }
}

/*
 * Uses the index in midilearn.
 */

void
qlearnframe::setup_loop_process ()
{
    midi_learn().automation_category(automation::category::loop);
}

void
qlearnframe::setup_mutes_process ()
{
    midi_learn().automation_category(automation::category::mute_group);
}

void
qlearnframe::setup_automation_process ()
{
    midi_learn().automation_category(automation::category::automation);
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
    (void) midi_learn().clear();
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

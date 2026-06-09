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
 * \updates       2026-06-09
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

#include "play/performer.hpp"           /* seq66::performer class           */
#include "qlearnframe.hpp"
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
    ui                      (new Ui::qlearnframe),
    m_perf                  (p),
    m_automation_category   (opcat),
    m_learn_button_group    (nullptr)
{
    ui->setupUi(this);

    /*
     * Create a button group to manage the mutual status of the Learn
     * Mode buttons.
     */

    m_learn_button_group = new QButtonGroup(this);
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

#if defined QT_VERSION_5

    connect
    (
        learn_button_group, SIGNAL(buttonClicked(int)),
        this, SLOT(slot_learn_mode(int))
    );

#elif defined QT_VERSION_6 || defined QT_VERSION_7

    auto lambdafunc = [this] (QAbstractButton * abutton)
    {
        slot_learn_mode(learn_button_group->id(abutton));
    };
    connect(learn_button_group, &QButtonGroup::buttonClicked, lambdafunc);

#endif

    connect
    (
        ui->cancel_push_button, SIGNAL(clicked()), this, SLOT(slot_cancel())
    );
    connect
    (
        ui->start_push_button, SIGNAL(clicked()), this, SLOT(slot_start())
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

    // TODO: show and process the category
}

qlearnframe::~qlearnframe()
{
    delete ui;
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
        m_automation_category = opcat;
        handle_select_category(opcat);
    }
}

void
qlearnframe::handle_select_category (automation::category opcat)
{
    // TODO
}

void
qlearnframe::slot_select_category (int buttonno)
{
    automation::category opcat { automation::category::none };
    if (buttonno == learn_mode_button_loops)
        opcat = automation::category::loop;
    else if (buttonno == learn_mode_button_mutes)
        opcat = automation::category::mute_group;
    else if (buttonno == learn_mode_button_automation)
        opcat = automation::category::automation;

    m_automation_category = opcat;
    handle_select_category(opcat);
}

void
qlearnframe::slot_cancel ()
{
    close();
}

void
qlearnframe::slot_start ()
{
}

void
qlearnframe::slot_save ()
{
}

void
qlearnframe::slot_reset ()
{
}

void
qlearnframe::slot_clear ()
{
}

void
qlearnframe::slot_ok ()
{
    // TODO
    close();
}

}               // namespace seq66

/*
 * qlearnframe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

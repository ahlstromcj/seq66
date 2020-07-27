/*
 *  This file is part of seq66.
 *
 *  seq66 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  seq66 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq66; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          qinputcheckbox.hpp
 *
 *  This class supports a MIDI Input check-box, associating it with a
 *  particular MIDI input buss.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-05-20
 * \updates       2020-07-26
 * \license       GNU GPLv2 or above
 *
 */

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QWidget>

#include "play/performer.hpp"
#include "qinputcheckbox.hpp"

/*
 *  Do not document the namespace, it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Creates a single line in the MIDI Clocks "Clock" group-box.  We will use
 *  the words "clock" or "port" for the MIDI output port represented by this
 *  widget.  Here are the jobs we have to do:
 *
 *      -#  Get the label for the port and set it.
 *      -#  Add the tooltips for the clock radio-buttons.
 *      -#  Add the clock radio-buttons to m_horizlayout_clocklive.
 *        -#  Connect to the radio-button slots:
 *            -    clock_callback_disable().
 *            -    clock_callback_off().
 *            -    clock_callback_on().
 *            -    clock_callback_mod().
 */

qinputcheckbox::qinputcheckbox
(
    QWidget * parent,
    performer & p,
    int bus
) :
    QWidget                     (parent),
    m_performance               (p),
    m_bus                       (bus),
    m_parent_widget             (parent),
    m_chkbox_inputactive        (nullptr)
{
    setup_ui();

    bool ok = connect
    (
        m_chkbox_inputactive, SIGNAL(stateChanged(int)),
        this, SLOT(input_callback_clicked(int))
    );
    if (! ok)
    {
        errprint("qinputcheckbox: input-active slot failed to connect");
    }
}

/**
 *  Sets up the user-interface for one MIDI input buss. If the port is an ALSA
 *  "announce" port (an input system port), it will be disabled.
 */

void
qinputcheckbox::setup_ui ()
{
    mastermidibus * masterbus = perf().master_bus();
    if (not_nullptr(masterbus))
    {
        QString busname = masterbus->get_midi_in_bus_name(m_bus).c_str();
        bool inputing = masterbus->get_input(m_bus);
        m_chkbox_inputactive = new QCheckBox(busname);
        m_chkbox_inputactive->setChecked(inputing);
        m_chkbox_inputactive->setEnabled(! perf().is_input_system_port(m_bus));
    }
}

/**
 *  Sets the clocking value based on in incoming parameter.  We have to use
 *  this particular slot in order to handle all of the radio-buttons.
 *
 * \param state
 *      Provides the state of the check-box that was clicked.
 */

void
qinputcheckbox::input_callback_clicked (int state)
{
    bool inputing = state == Qt::Checked;
    perf().set_input_bus(m_bus, inputing);
}

}           // namespace seq66

/*
 * qinputcheckbox.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


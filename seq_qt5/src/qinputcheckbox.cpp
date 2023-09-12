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
 * \file          qinputcheckbox.cpp
 *
 *  This class supports a MIDI Input check-box, associating it with a
 *  particular MIDI input buss.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-05-20
 * \updates       2023-05-16
 * \license       GNU GPLv2 or above
 *
 *  This class is used in the qseditoptions settings-dialog class.
 */

#include <QtWidgets/QCheckBox>

#include "play/performer.hpp"           /* seq66::performer class           */
#include "qinputcheckbox.hpp"           /* seq66::qinputcheckbox class      */
#include "qseditoptions.hpp"            /* seq66::qseditoptions class       */
#include "qt5_helpers.hpp"              /* seq66::qt() string conversion    */

/*
 *  Do not document the namespace, it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Creates a single line in the MIDI Input group-box.  We will use
 *  the words "input" or "port" for the MIDI input port represented by this
 *  widget.  Here are the jobs we have to do:
 *
 *      -#  Get the label for the port and set it.
 *      -#  Add the tooltips for the input radio-buttons.
 *      -#  Add the input radio-buttons to m_horizlayout_clocklive.
 *      -#  Connect to the check-button slot.
 *
 * \param parent
 *      This parameter points to the qseditoption object that owns this
 *      check-box.
 *
 * \param p
 *      Provides the performer for some lookup activities.
 *
 * \parma bus
 *      Provides the index into the list of inputs provided by the system. If
 *      there is an input-port-map, then this bus number must be translated to
 *      the true/actual system bus number before usage.
 */

qinputcheckbox::qinputcheckbox (QWidget * parent, performer & p, int bus) :
    qportwidget             (parent, p, bus),
    m_chkbox_inputactive    (nullptr)
{
    setup_ui();
    connect
    (
        m_chkbox_inputactive, SIGNAL(stateChanged(int)),
        this, SLOT(input_callback_clicked(int))
    );
}

/**
 *  Sets up the user-interface for one MIDI input buss. If the port is an ALSA
 *  "announce" port (an input system port), it will be disabled.
 */

void
qinputcheckbox::setup_ui ()
{
    std::string busname;
    bool inputing;
    bool gotbussinfo = perf().ui_get_input(bus(), inputing, busname);
    if (gotbussinfo)
    {
        QString qbname = qt(busname);
        bool unavailable = perf().is_port_unavailable
        (
            bus(), midibase::io::input
        );
        m_chkbox_inputactive = new QCheckBox(qbname);
        m_chkbox_inputactive->setChecked(inputing);
        if (! unavailable)
            unavailable = perf().is_input_system_port(bus());

        if (unavailable)
            m_chkbox_inputactive->setToolTip("Port is unavailable");

        m_chkbox_inputactive->setEnabled(! unavailable);
    }
}

/**
 *  Sets the clocking value based on in incoming parameter.  We have to use
 *  this particular slot in order to handle all of the radio-buttons.
 *  Note that there is no explicit and separate "disabled" state for input
 *  like there is for clocks.
 *
 * \param state
 *      Provides the state of the check-box that was clicked.
 */

void
qinputcheckbox::input_callback_clicked (int state)
{
    bool inputing = state == Qt::Checked;
    perf().ui_set_input(bus(), inputing);
    parent_widget()->enable_bus_item(bus(), true);      /* tell the parent  */
}

}           // namespace seq66

/*
 * qinputcheckbox.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


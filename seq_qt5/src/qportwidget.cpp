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
 * \file          qportwidget.cpp
 *
 *  This base class supports qclocklayout and qinputcheckbox.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2022-01-20
 * \updates       2022-01-20
 * \license       GNU GPLv2 or above
 *
 *  This class represents a user-interface for output (clock) or input ports.
 */

#include "play/performer.hpp"           /* seq66::performer class           */
#include "qportwidget.hpp"              /* seq66::qportwidget class         */
#include "qseditoptions.hpp"            /* seq66::qseditoptions class       */

namespace seq66
{

/**
 *  Creates a single line in the MIDI Clocks "Clock" group-box or in the MIDI
 *  Inputs group-box. It is the base class for qclocklayout and
 *  qinputcheckbox. We use the words "clock", "input", or "port" for the MIDI
 *  input/output ports represented by this widget.  Here are the jobs we have
 *  to do:
 *
 *      -#  Get the label for the port and set it.
 *      -#  Add the tooltips for the clock radio-buttons.
 *      -#  Add the clock radio-buttons to m_horizlayout_clocklive.
 *        -#    Connect to the radio-button slots:
 *            -    clock_callback_disable().
 *            -    clock_callback_off().
 *            -    clock_callback_on().
 *            -    clock_callback_mod().
 */

qportwidget::qportwidget (QWidget * parent, performer & p, int bus) :
    QWidget                     (parent),
    m_performance               (p),
    m_bus                       (bus),
    m_parent_widget             (dynamic_cast<qseditoptions *>(parent))
{
    // setup_ui();
}

}           // namespace seq66

/*
 * qportwidget.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


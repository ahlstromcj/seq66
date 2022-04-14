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
 * \file          qperfeditex.cpp
 *
 *  This module declares/defines the base class for the external performance
 *  edit window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-07-21
 * \updates       2021-04-06
 * \license       GNU GPLv2 or above
 *
 */

#include <QGridLayout>

#include "play/performer.hpp"           /* seq66::performer class           */
#include "play/sequence.hpp"            /* seq66::sequence class            */
#include "qperfeditex.hpp"
#include "qperfeditframe64.hpp"
#include "qsmainwnd.hpp"

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#if defined SEQ66_QMAKE_RULES
#include "forms/ui_qperfeditex.h"
#else
#include "forms/qperfeditex.ui.h"
#endif

/*
 * Don't document the namespace.
 */

namespace seq66
{

/**
 *  This function wraps an external qperfeditframe64 frame.
 *
 * \param p
 *      Provides the performer object to use for interacting with this sequence.
 *
 * \param parent
 *      Provides the parent window/widget for this container window.  Defaults
 *      to null.  Note that this parameter does not link this class to the
 *      parent as a QWidget, because that would make this class appear inside
 *      the qsmainwnd user-interface.
 */

qperfeditex::qperfeditex (performer & p, qsmainwnd * parent) :
    QWidget         (nullptr),
    ui              (new Ui::qperfeditex),
    m_performer     (p),
    m_edit_parent   (parent),
    m_edit_frame    (nullptr)
{
    ui->setupUi(this);

    QGridLayout * layout = new QGridLayout(this);
    m_edit_frame = new qperfeditframe64(p, this, true); /* external frame   */
    layout->addWidget(m_edit_frame);
    show();
    m_edit_frame->show();
}

/**
 *  Deletes the user interface.  It does not tell the editor parent to remove
 *  this object.  Contrary to previous claims, why would it need to do that
 *  here?  See the closeEvent() override.
 */

qperfeditex::~qperfeditex()
{
    delete ui;
}

/**
 *  Override in order to tell the parent frame to remove this fellow
 *  from its memory.
 */

void
qperfeditex::closeEvent (QCloseEvent *)
{
    if (not_nullptr(m_edit_parent))
        m_edit_parent->hide_qperfedit(true);
}

/**
 *  See usage in qsmainwnd.  It basically tells the edit-frame to update
 *  itself based on some user-interface or zoom changes.  (Don't quote me on
 *  that!)
 */

void
qperfeditex::update_sizes ()
{
    if (not_nullptr(m_edit_frame))
        m_edit_frame->update_sizes();
}

void
qperfeditex::set_loop_button (bool looping)
{
    if (not_nullptr(m_edit_frame))
        m_edit_frame->set_loop_button(looping);
}

}               // namespace seq66

/*
 * qperfeditex.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


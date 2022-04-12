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
 * \file          qseqeditex.cpp
 *
 *  This module declares/defines the base class for the external sequence edit
 *  window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-06-15
 * \updates       2022-04-12
 * \license       GNU GPLv2 or above
 *
 */

#include <QGridLayout>

#include "play/performer.hpp"           /* seq66::performer class           */
#include "qseqeditex.hpp"               /* seq66::qseqeditex container      */
#include "qseqeditframe64.hpp"          /* seq66::qseqeditframe64 editor    */
#include "qsmainwnd.hpp"                /* seq66::qsmainwnd master class    */
#include "qt5_helpers.hpp"              /* seq66::qt() string conversion    */

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#if defined SEQ66_QMAKE_RULES
#include "forms/ui_qseqeditex.h"
#else
#include "forms/qseqeditex.ui.h"
#endif

/*
 * Don't document the namespace.
 */

namespace seq66
{

/**
 *  Creates the window holding the external edit frame, and creates that frame
 *  [qseqeditframe64] as well.
 *
 * \param p
 *      Provides the performer object to use for interacting with this sequence.
 *
 * \param seqid
 *      Provides the sequence number.  The sequence pointer is looked up using
 *      this number.
 *
 * \param parent
 *      Provides the parent window/widget for this container window.  Defaults
 *      to null.  Note that this parameter does not link this class to the
 *      parent as a QWidget, because that would make this class appear inside
 *      the qsmainwnd user-interface.
 */

qseqeditex::qseqeditex (performer & p, int seqid, qsmainwnd * parent) :
    QWidget             (nullptr),
    ui                  (new Ui::qseqeditex),
    m_performer         (p),
    m_seq_id            (seqid),
    m_edit_parent       (parent),
    m_edit_frame        (nullptr)
{
    ui->setupUi(this);

    QGridLayout * layout = new QGridLayout(this);
    m_edit_frame = new qseqeditframe64(p, seqid, this);     // no PPQN ???
    layout->addWidget(m_edit_frame);

    std::string title = "Pattern #";
    title += std::to_string(seqid);
    setWindowTitle(qt(title));
    show();
    m_edit_frame->show();
}

/**
 *  Deletes the user interface, then tells the editor parent to remove
 *  this object.
 */

qseqeditex::~qseqeditex()
{
    delete ui;
    if (not_nullptr(m_edit_frame))
        delete m_edit_frame;
}

/**
 *  First test the enclosed seqedit frame to close (which then should cause
 *  that to tell it's LFO and Pattern-Fix windows to close.  Then
 *  tells the parent window to close (and automatically remove this container
 *  frame when the user closes it.
 *
 *  Hopefully there's no race condition here.
 */

void
qseqeditex::closeEvent (QCloseEvent *)
{
    if (not_nullptr(m_edit_parent))
    {
        m_edit_frame->close();
        m_edit_parent->remove_editor(m_seq_id);
    }
}

/**
 *  Tells the contained edit frame that it needs to update itself.
 */

void
qseqeditex::update_draw_geometry ()
{
    if (not_nullptr(m_edit_frame))
        m_edit_frame->update_draw_geometry();
}

}               // namespace seq66

/*
 * qseqeditex.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


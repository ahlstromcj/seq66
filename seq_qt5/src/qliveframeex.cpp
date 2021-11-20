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
 * \file          qliveframeex.cpp
 *
 *  This module declares/defines the base class for the external sequence edit
 *  window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-09-16
 * \updates       2021-11-19
 * \license       GNU GPLv2 or above
 *
 *  This frame holds an external "Live" window that shows the grid of buttons
 *  for each set in the Seq66 MIDI file.
 */

#include <QGridLayout>

#include "seq66-config.h"               /* defines SEQ66_QMAKE_RULES        */
#include "cfg/settings.hpp"             /* seq66::usr() config functions    */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "play/sequence.hpp"            /* seq66::sequence class            */
#include "qliveframeex.hpp"             /* seq66::qliveframeex container    */
#include "qslivegrid.hpp"               /* seq66::qslivegrid panel          */
#include "qt5_helpers.hpp"              /* seq66::qt(), qt_set_icon() etc.  */
#include "qsmainwnd.hpp"                /* seq66::qsmainwnd parent class    */

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#if defined SEQ66_QMAKE_RULES
#include "forms/ui_qliveframeex.h"
#else
#include "forms/qliveframeex.ui.h"
#endif

/*
 * Don't document the namespace.
 */

namespace seq66
{

/**
 *
 * \param p
 *      Provides the performer object to use for interacting with this live
 *      frame.
 *
 * \param ssnum
 *      Provides the screen-set number.
 *
 * \param parent
 *      Provides the parent window/widget for this container window.  Defaults
 *      to null.  Note that this parameter does not link this class to the
 *      parent as a QWidget, because that would make this class appear inside
 *      the qsmainwnd user-interface.
 */

qliveframeex::qliveframeex (performer & p, int ssnum, qsmainwnd * parent) :
    QWidget             (nullptr),
    ui                  (new Ui::qliveframeex),
    m_performer         (p),
    m_screenset         (ssnum),
    m_live_parent       (parent),
    m_live_frame        (nullptr)
{
    ui->setupUi(this);

    QGridLayout * layout = new QGridLayout(this);
    m_live_frame = new (std::nothrow) qslivegrid(p, parent, ssnum, nullptr);
    if (not_nullptr(m_live_frame))
        layout->addWidget(m_live_frame);

    if (usr().window_is_scaled())           /* use scaling if applicable    */
    {
        QSize s = size();
        int h = s.height();
        int w = s.width();
        int width = usr().scale_size(w);
        int height = usr().scale_size_y(h);
        resize(width, height);
        if (not_nullptr(m_live_frame))
            m_live_frame->repaint();
    }
    show();
    if (not_nullptr(m_live_frame))
    {
        m_live_frame->update_bank(ssnum);
        m_live_frame->show();
    }
}

qliveframeex::~qliveframeex()
{
    if (not_nullptr(m_live_frame))      /* ca 2021-11-08 PROVISIONAL    */
        delete m_live_frame;

    delete ui;
}

/**
 *  Removes the child, which is the enclosed live frame.
 */

void
qliveframeex::closeEvent (QCloseEvent *)
{
    if (not_nullptr(m_live_parent))
        m_live_parent->remove_live_frame(m_screenset);
}

/**
 *  Updates the live frame.
 */

void
qliveframeex::update_draw_geometry ()
{
    if (not_nullptr(m_live_frame))
        m_live_frame->update_geometry();
}

void
qliveframeex::update_sequence (int seqno, bool redo)
{
    if (not_nullptr(m_live_frame))
        m_live_frame->update_sequence(seq::number(seqno), redo);
}

/**
 *  This function is called when focus changes.  We forward the call to the
 *  actual live-frame or live-grid.
 */

void
qliveframeex::changeEvent (QEvent * event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange)
    {
        std::string name = m_live_frame->bank_name();
        std::string t = "Set ";
        t += std::to_string(m_live_frame->bank_id());
        if (! name.empty())
        {
            t += ": ";
            t += name;
        }
        setWindowTitle(qt(t));
        m_live_frame->change_event(event);  /* changeEvent(event) */
    }
}

}               // namespace seq66

/*
 * qliveframeex.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


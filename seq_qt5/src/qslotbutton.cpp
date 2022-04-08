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
 * \file          qslotbutton.cpp
 *
 *  This module declares/defines the base class for drawing a pattern-slot
 *  button.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-06-26
 * \updates       2022-04-08
 * \license       GNU GPLv2 or above
 *
 *  This object is just a QPushButton with number label.  See seq66::qslivegrid
 *  for its usage.
 *
 * Getting system background color:
 *
 *  QWidget tmpWidget;
 *  QColor bgcolor = tmpWidget.palette().color(QPalette::Window);
 *
 * Getting system button text color:
 *
 *  QColor btextcolor = tmpWidget.palette().color(QPalette::ButtonText);
 */

#include <QPainter>

#include "cfg/settings.hpp"             /* seq66::usr() config functions    */
#include "qslotbutton.hpp"              /* seq66::qslotbutton base class    */
#include "qt5_helpers.hpp"              /* seq66::qt(), qt_set_icon() etc.  */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Note that we make the button transparent to mouse events.  It lets them
 *  through for the parent to handle.
 */

qslotbutton::qslotbutton
(
    const qslivegrid * const slotparent,
    seq::number slotnumber,
    const std::string & label,
    const std::string & hotkey,
    QWidget * parent
) :
    QPushButton         (parent),
    m_slot_parent       (slotparent),
    m_slot_number       (slotnumber),
    m_label             (label),
    m_hotkey            (hotkey),
    m_drum_color        (drum_paint()),                     /* Qt::red      */
#if defined DRAW_TEMPO_LINE
    m_tempo_color       (tempo_paint()),                    /* Qt::magenta  */
#endif
    m_progress_color    (progress_paint()),
    m_label_color       (label_paint()),                    /* tentative    */
    m_text_color        (),
    m_pen_color         (foreground_paint()),               /* tentative    */
    m_back_color        (background_paint()),               /* tentative    */
    m_vert_compressed   (usr().vertically_compressed()),
    m_horiz_compressed  (usr().horizontally_compressed()),
    m_is_active         (false),
    m_is_checkable      (false),
    m_is_dirty          (true)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    if (is_theme_color(label_paint()))
    {
        QWidget tmp;
        label_color(tmp.palette().color(QPalette::ButtonText));
    }
}

/**
 *  This function is called by qslivegrid::setup_button() for inactive slots,
 *  and it is responsible for presenting a clean button.  If not called, then
 *  the original pattern information remains on the button, inactive so that
 *  it looks ghostly.
 */

void
qslotbutton::setup ()
{
    setAutoFillBackground(true);
    setEnabled(true);
    setCheckable(is_checkable());

    /*
     * Works better here than in paintEvent().
     */

    std::string snstring = std::to_string(m_slot_number);
    setText(qt(snstring));
}

}           // namespace seq66

/*
 * qslotbutton.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


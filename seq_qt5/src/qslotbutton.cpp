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
 * \file          qslotbutton.cpp
 *
 *  This module declares/defines the base class for drawing a pattern-slot
 *  button.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-06-26
 * \updates       2021-05-15
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
#include "qslotbutton.hpp"

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
 *  QPalette::setColor(QPalette::ColorRole role, const QColor &color).
 *  Sets the color used for the given color role, in all color groups, to the
 *  specified solid color.
 *
 *  This is also an option, but let the desktop theme handle it.
 *
 *      pb->setFlat(true);
 *
 *  This sets a garish red background, even outside the cells.
 *
 *      pb->setAutoFillBackground(true);
 *      setStyleSheet(s_style.c_str());
 */

void
qslotbutton::setup ()
{
#if defined USE_OLD_GRID_STYLE      /* permanently removed 2021-05-15 */
    QPalette pal = palette();
    if (usr().grid_is_normal())
    {
        pal.setColor(QPalette::Button, QColor(Qt::gray));
        pal.setColor(QPalette::ButtonText, QColor(Qt::blue));
    }
    else if (usr().grid_is_black())
    {
        pal.setColor(QPalette::Button, QColor(Qt::black));
        pal.setColor(QPalette::ButtonText, QColor(Qt::yellow));
    }
    else if (usr().grid_is_white())
    {
        pal.setColor(QPalette::Button, QColor(Qt::white));
        pal.setColor(QPalette::ButtonText, QColor(Qt::black));
    }
    setAutoFillBackground(true);
    setPalette(pal);
    setEnabled(true);
    setCheckable(is_checkable());

    /*
     * Works better here than in paintEvent().
     */

    std::string snstring = std::to_string(m_slot_number);
    setText(snstring.c_str());
#endif
}

/**
 *  A silly little wrapper for the Qt update() function.
 */

void
qslotbutton::reupdate (bool all)
{
    if (all)
    {
        m_is_dirty = true;
        update();
    }
}

}           // namespace seq66

/*
 * qslotbutton.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


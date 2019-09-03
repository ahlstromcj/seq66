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
 * \updates       2019-07-03
 * \license       GNU GPLv2 or above
 *
 */

#include <QPainter>

#include "cfg/settings.hpp"             /* seq66::usr().key_height(), etc.  */
#include "qslotbutton.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Provide access to a basic color palette.  We use a static function to access
 *  this item (plus a couple of things we don't need).  We don't want to make it
 *  a base class of qslotbutton.
 */

gui_palette_qt5 &
qslotbutton::slotpal ()
{
    static gui_palette_qt5 s_pallete;
    return s_pallete;
}

/**
 *
 */

qslotbutton::qslotbutton
(
    seq::number slotnumber,
    const std::string & label,
    const std::string & hotkey,
    QWidget * parent
) :
    QPushButton         (parent),
    m_slot_number       (slotnumber),
    m_label             (label),
    m_hotkey            (hotkey),
    m_back_color        (Qt::black),
    m_fore_color        (Qt::white),
    m_text_color        (Qt::black),
    m_is_checkable      (false),
    m_is_dirty          (true)
{
#if defined SEQ66_SLOTBUTTON_TRANSPARENT_TO_MOUSE
    setAttribute(Qt::WA_TransparentForMouseEvents);
#endif
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
}

/**
 *
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


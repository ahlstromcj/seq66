#if ! defined SEQ66_PALETTE_HPP
#define SEQ66_PALETTE_HPP

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
 * \file          palette.hpp
 *
 *  This module declares/defines items for an abstract representation of the
 *  color of a sequence or panel item.  Colors are, of course, part of using a
 *  GUI, but here we are not tied to a GUI.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-02-18
 * \updates       2019-06-26
 * \license       GNU GPLv2 or above
 *
 *  This module is inspired by MidiPerformance::getSequenceColor() in
 *  Kepler34.
 */

#include <map>                          /* std::map container class         */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Progress bar colors as integer codes.
 */

enum class progress_colors
{
    BLACK        = 0,
    DARK_RED,
    DARK_GREEN,
    DARK_ORANGE,
    DARK_BLUE,
    DARK_MAGENTA,
    DARK_CYAN
};

/**
 *  A type to support the concept of sequence color.  The color is a number
 *  pointing to an RGB entry in a palette.
 *
 *  This enumeration provide as stock palette of colors.  For example,
 *  Kepler34 creates a color-map in this manner:
 *
 *      extern QMap<thumb_colours_e, QColor> colourMap;
 *
 *  Of course, we would use std::map instead of QMap, and include a wrapper
 *  class to keep QColor unexposed to non-Qt entities.  Also, we define the
 *  colors in standard X-terminal order, not in Kepler34 order.
 */

enum class PaletteColor
{

 /* Seq64 */            /* Kepler34 */

    NONE = -1,          // indicates no color chosen, default color
    BLACK = 0,          //  0 WHITE
    RED,                //  1 RED
    GREEN,              //  2 GREEN
    YELLOW,             //  3 BLUE
    BLUE,               //  4 YELLOW
    MAGENTA,            //  5 PURPLE
    CYAN,               //  6 PINK
    WHITE,              //  7 ORANGE
    ORANGE,             // N/A
    PINK,               // N/A
    GREY,               // N/A
    DK_BLACK,           //  8 place-holder
    DK_RED,             //  9 N/A
    DK_GREEN,           // 10 N/A
    DK_YELLOW,          // 11 N/A
    DK_BLUE,            // 12 N/A
    DK_MAGENTA,         // 13 N/A
    DK_CYAN,            // 14 N/A
    DK_WHITE,           // 15 N/A
    DK_ORANGE,          // N/A
    DK_PINK,            // N/A
    DK_GREY,            // N/A
    MAX,                // first illegal value, not in color set

};

/**
 *  A macro to handle older versions of C++.
 */

#define color_to_int(x)  static_cast<int>( PaletteColor :: x )

/**
 *  A generic collection of whatever types of color classes (QColor,
 *  Gdk::Color) one wants to hold, and reference by an index number.
 *  This template class is not meant to manage color, but just to point
 *  to them.
 */

template <typename COLOR>
class palette
{

    /**
     *  Combines the PaletteColor with a string describing the color.
     *  This color string is not necessarily standard, but can be added to a
     *  color-selection menu.
     */

    using pair = struct
    {
        const COLOR * ppt_color;
        std::string ppt_color_name;

    };

private:

    /*
     *  Provides a type definition for the color-class of interest.
     *
     * typedef COLOR Color;
     */

    /**
     *  Provides an associative container of pointers to the color-class COLOR.
     *  A vector could be used instead of a map.
     */

    std::map<PaletteColor, pair> m_container;

public:

    palette ();                         /* initially empty, filled by add() */

    void add
    (
        PaletteColor index, const COLOR & color, const std::string & name
    );
    const COLOR & get_color (PaletteColor index) const;
    const std::string & get_color_name (PaletteColor index) const;

    /**
     * \param index
     *      The color index to be tested.
     *
     * \return
     *      Returns true if there is no color applied.
     */

    bool no_color (PaletteColor index) const
    {
        return index == PaletteColor::NONE;
    }

    /*
     * Offload to GUI-specific palette class.
     *
     * COLOR get_color_inverse (PaletteColor index) const;
     */

    /**
     *  Clears the color/name container.
     */

    void clear ()
    {
        m_container.clear();
    }

};          // class palette

/**
 *  Creates the palette, and inserts a default COLOR color object as
 *  the NONE entry.  This color has to be static so that it is always around
 *  to be used.
 */

template <typename COLOR>
palette<COLOR>::palette ()
 :
    m_container   ()
{
    static COLOR color;
    add(PaletteColor::NONE, color, "None");
}

/**
 *  Inserts a color-index/color pair into the palette.  There is no indication
 *  if the item was not added, which will occur only when the item is already in
 *  the container.
 *
 * \param index
 *      The index into the palette.
 *
 * \param color
 *      The COLOR color object to add to the palette.
 */

template <typename COLOR>
void
palette<COLOR>::add
(
    PaletteColor index, const COLOR & color, const std::string & colorname
)
{
    pair colorspec;
    colorspec.ppt_color = &color;
    colorspec.ppt_color_name = colorname;

    /*
     * std::pair<PaletteColor, pair>
     */

    auto p = std::make_pair(index, colorspec);
    (void) m_container.insert(p);
}

/**
 *  Gets a color from the palette, based on the index value.
 *
 * \param index
 *      Indicates which color to get.  This index is checked for range, and, if
 *      out of range, the default color object, indexed by PaletteColor::NONE,
 *      is returned.  However, an exception will be thrown if the color does
 *      not exist, which should be the case only via programmer error.
 *
 * \return
 *      Returns a reference to the selected color object.
 */

template <typename COLOR>
const COLOR &
palette<COLOR>::get_color (PaletteColor index) const
{
    if (index >= PaletteColor::BLACK && index < PaletteColor::MAX)
    {
        return *m_container.at(index).ppt_color;
    }
    else
    {
        return *m_container.at(PaletteColor::NONE).ppt_color;
    }
}

/**
 *
 */

template <typename COLOR>
const std::string &
palette<COLOR>::get_color_name (PaletteColor index) const
{
    if (index >= PaletteColor::BLACK && index < PaletteColor::MAX)
    {
        return m_container.at(index).ppt_color_name;
    }
    else
    {
        return m_container.at(PaletteColor::NONE).ppt_color_name;
    }
}

}           // namespace seq66

#endif      // SEQ66_PALETTE_HPP

/*
 * palette.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


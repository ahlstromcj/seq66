#if ! defined SEQ66_PALETTE_HPP
#define SEQ66_PALETTE_HPP

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
 * \file          palette.hpp
 *
 *  This module declares/defines items for an abstract representation of the
 *  color of a sequence or panel item.  Colors are, of course, part of using a
 *  GUI, but here we are not tied to a GUI.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-02-18
 * \updates       2025-01-14
 * \license       GNU GPLv2 or above
 *
 *  This module is inspired by MidiPerformance::getSequenceColor() in
 *  Kepler34.
 */

#include <map>                          /* std::map container class         */
#include <string>                       /* std::string class                */

namespace seq66
{

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
 /* Seq66 */            /* Kepler34 */

    none = -1,          // indicates no color chosen, default color
    black = 0,          //  0 WHITE
    red,                //  1 RED
    green,              //  2 GREEN
    yellow,             //  3 BLUE
    blue,               //  4 YELLOW
    magenta,            //  5 PURPLE
    cyan,               //  6 PINK
    white,              //  7 ORANGE
    dk_black,           //  8 place-holder
    dk_red,             //  9 N/A
    dk_green,           // 10 N/A
    dk_yellow,          // 11 N/A
    dk_blue,            // 12 N/A
    dk_magenta,         // 13 N/A
    dk_cyan,            // 14 N/A
    dk_white,           // 15 N/A
    orange,             // color_16
    pink,               // color_17
    color_18,
    color_19,
    color_20,
    color_21,
    color_22,
    grey,               // color_23
    dk_orange,          // color_24
    dk_pink,            // color_25
    color_26,
    color_27,
    color_28,
    color_29,
    color_30,
    dk_grey,            // color_31
    max                 // first illegal palette value, not in color set
};

/**
 *  Provides indices into a list of colors that can be retrieved from a map of
 *  normal colors or a map of inverse colors.  Supports the --inverse option.
 */

enum class InvertibleColor
{
    black,              /**< Used for foreground items like grid lines.     */
    white,              /**< Used for background items (eg. drawing canvas. */
    label,              /**< Used for labeling on pattern buttons/slots.    */
    selection,          /**< Used to paint selected notes.                  */
    drum,               /**< Used for non-transposable (drum) notes.        */
    tempo,              /**< Painting for tempo events.                     */
    note_in,            /**< Color inside the note, defaults to foreground. */
    note_out,           /**< Border color of note, defaults to background.  */
    black_key,          /**< Painting for the "black keys" on the piano.    */
    white_key,          /**< Painting for the "white keys" on the piano.    */
    progress,           /**< Painting for the progress bar.                 */
    backseq,            /**< Painting for the background sequencce.         */
    grey,               /**< Medium grid lines.                             */
    dk_grey,            /**< Heavy grid lines.                              */
    lt_grey,            /**< Light grid lines.                              */
    beat,               /**< For a medium-heavy beat line; was foreground.  */
    near,               /**< Mouse is near an event in a pane. Was yellow   */
    backtime,           /**< Used for the background of time-lines.         */
    backdata,           /**< Used for the background of data panes.         */
    backevent,          /**< Used for the background of the event pane.     */
    backkeys,           /**< Used for the background of the keys pane.      */
    backnames,          /**< Used for the background of perf names pane.    */
    octave,             /**< Color for each octave line; was foreground.    */
    text,               /**< Replaces "black" (foreground) for text items.  */
    texttime,           /**< Used for the text of time-lines.               */
    textdata,           /**< Used for the text of data panes.               */
    noteevent,          /**< Used for the note brush of the event pane.     */
    textkeys,           /**< Used for the text of the keys pane.            */
    textnames,          /**< Used for the text of perf names pane.          */
    textslots,          /**< Used for the text of the grid/pattern slots.   */
    scale,              /**< Provides the color for drawing scale notes.    */
    extra,              /**< Reserved for expansion.                        */
    max                 /**< First illegal palette value, not in color set. */
};

/**
 *  Macro to simplify converting between palette color enumeration values
 *  to a simple integer.
 */

inline int
palette_to_int (PaletteColor x)
{
    return static_cast<int>(x);
}

inline int
inv_palette_to_int (InvertibleColor x)
{
    return static_cast<int>(x);
}

/**
 *  A generic collection of whatever types of color classes (QColor,
 *  Gdk::Color) one wants to hold, and reference by an index number.
 *  This template class is not meant to manage color, but just to point
 *  to them.
 */

template <typename COLOR>
class palette
{
    friend class gui_palette_qt5;

    /**
     *  Combines the PaletteColor with a string describing the color.
     *  This color string is not necessarily standard, but can be added to a
     *  color-selection menu.
     */

    using pair = struct
    {
        COLOR ppt_color;
        std::string ppt_color_name;
    };

    using container = std::map<int, pair>;

private:

    /**
     *  Provides an associative container of pointers to the color-class COLOR.
     *  A vector could be used instead of a map.
     *
     *  std::map<PaletteColor, pair> m_container;
     */

    container m_container;

public:

    palette ();                         /* initially empty, filled by add() */

    bool add (PaletteColor index, const COLOR & c, const std::string & name);
    const COLOR & get_color (PaletteColor index) const;
    std::string get_color_name (PaletteColor index) const;
    std::string get_color_name_ex (PaletteColor index) const;

    bool add (InvertibleColor index, const COLOR & c, const std::string & name);
    const COLOR & get_color (InvertibleColor index) const;
    std::string get_color_name (InvertibleColor index) const;
    std::string get_color_name_ex (InvertibleColor index) const;

    /**
     * \param index
     *      The color index to be tested.
     *
     * \return
     *      Returns true if there is no color applied.
     */

    bool no_color (PaletteColor index) const
    {
        return index == PaletteColor::none;
    }

    void clear ()
    {
        m_container.clear();
    }

    int count ()
    {
        return int(m_container.size());
    }

private:

    const container & entries () const
    {
        return m_container;
    }

    container & entries ()
    {
        return m_container;
    }

};          // class palette

/**
 *  Creates the palette, and inserts a default COLOR color object as
 *  the PaletteColor::none entry.  This color has to be static so that it is
 *  always around to be used.
 */

template <typename COLOR>
palette<COLOR>::palette () :
    m_container   ()
{
    static COLOR color;
    add(PaletteColor::none, color, "None");
}

/**
 *  Inserts a color-index/color pair into the palette.  There is no indication
 *  if the item was not added, which will occur only when the item is already
 *  in the container.  The type of a color pair is
 *  std::pair<PaletteColor, pair>.
 *
 * \param index
 *      The index into the palette.
 *
 * \param color
 *      The COLOR color object to add to the palette.
 */

template <typename COLOR>
bool
palette<COLOR>::add
(
    PaletteColor index,
    const COLOR & color,
    const std::string & colorname
)
{
    int key = palette_to_int(index);
    size_t count = m_container.size();
    pair colorspec;
    colorspec.ppt_color = color;
    colorspec.ppt_color_name = colorname;

    auto p = std::make_pair(key, colorspec);
    (void) m_container.insert(p);
    return m_container.size() == (count + 1);
}

/**
 *  Gets a color from the palette, based on the index value.
 *
 * \param index
 *      Indicates which color to get.  This index is checked for range, and, if
 *      out of range, the default color object, indexed by PaletteColor::none,
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
    if (index >= PaletteColor::black && index < PaletteColor::max)
    {
        int key = palette_to_int(index);
        return m_container.at(key).ppt_color;
    }
    else
    {
        int key = palette_to_int(PaletteColor::none);
        return m_container.at(key).ppt_color;
    }
}

template <typename COLOR>
std::string
palette<COLOR>::get_color_name (PaletteColor index) const
{
    if (index >= PaletteColor::black && index < PaletteColor::max)
    {
        int key = palette_to_int(index);
        return m_container.at(key).ppt_color_name;
    }
    else
    {
        int key = palette_to_int(PaletteColor::none);
        return m_container.at(key).ppt_color_name;
    }
}

template <typename COLOR>
std::string
palette<COLOR>::get_color_name_ex (PaletteColor index) const
{
    std::string result = std::to_string(palette_to_int(index));
    result += " ";
    result += get_color_name(index);
    return result;
}

template <typename COLOR>
bool
palette<COLOR>::add
(
    InvertibleColor index,
    const COLOR & color,
    const std::string & colorname
)
{
    int key = inv_palette_to_int(index);
    size_t count = m_container.size();
    pair colorspec;
    colorspec.ppt_color = color;
    colorspec.ppt_color_name = colorname;

    auto p = std::make_pair(key, colorspec);
    (void) m_container.insert(p);
    return m_container.size() == (count + 1);
}

template <typename COLOR>
const COLOR &
palette<COLOR>::get_color (InvertibleColor index) const
{
    int key = inv_palette_to_int(index);
    if (index >= InvertibleColor::black && index < InvertibleColor::max)
    {
        int maximum = static_cast<int>(m_container.size());
        if (key >= maximum)
            key = 0;
    }
    return m_container.at(key).ppt_color;
}

template <typename COLOR>
std::string
palette<COLOR>::get_color_name (InvertibleColor index) const
{
    int key = inv_palette_to_int(index);
    if (index >= InvertibleColor::black && index < InvertibleColor::max)
    {
        int maximum = static_cast<int>(m_container.size());
        if (key >= maximum)
            key = 0;
    }
    return m_container.at(key).ppt_color_name;
}

template <typename COLOR>
std::string
palette<COLOR>::get_color_name_ex (InvertibleColor index) const
{
    std::string result = std::to_string(inv_palette_to_int(index));
    result += " ";
    result += get_color_name(index);
    return result;
}

}           // namespace seq66

#endif      // SEQ66_PALETTE_HPP

/*
 * palette.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


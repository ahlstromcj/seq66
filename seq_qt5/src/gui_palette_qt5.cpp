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
 * \file          gui_palette_qt5.cpp
 *
 *  This module declares/defines the class for providing GTK/GDK colors.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-02-23
 * \updates       2019-10-16
 * \license       GNU GPLv2 or above
 *
 *  One possible idea would be a color configuration that would radically
 *  change drawing of the lines and pixmaps, opening up the way for night
 *  views and color schemes that match the desktop theme.
 */

#include "cfg/settings.hpp"             /* seq66::rc() or seq66::usr()      */
#include "gui_palette_qt5.hpp"          /* seq66::gui_palette_qt5           */
#include "util/palette.hpp"             /* enum class progress_colors       */

#define STATIC_COLOR                    gui_palette_qt5::Color

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  By default, the inverse color palette is not loaded.
 */

bool gui_palette_qt5::m_is_inverse = false;

/**
 * Bright constant colors
 */

const STATIC_COLOR gui_palette_qt5::m_black        = Color("black");
const STATIC_COLOR gui_palette_qt5::m_red          = Color("red");
const STATIC_COLOR gui_palette_qt5::m_green        = Color("green");
const STATIC_COLOR gui_palette_qt5::m_yellow       = Color("yellow");
const STATIC_COLOR gui_palette_qt5::m_blue         = Color("blue");
const STATIC_COLOR gui_palette_qt5::m_magenta      = Color("magenta");
const STATIC_COLOR gui_palette_qt5::m_cyan         = Color("cyan");
const STATIC_COLOR gui_palette_qt5::m_white        = Color("white");

/**
 * Dark constant colors
 */

const STATIC_COLOR gui_palette_qt5::m_dk_black     = Color("black");
const STATIC_COLOR gui_palette_qt5::m_dk_red       = Color("dark red");
const STATIC_COLOR gui_palette_qt5::m_dk_green     = Color("dark green");
const STATIC_COLOR gui_palette_qt5::m_dk_yellow    = Color("dark yellow");
const STATIC_COLOR gui_palette_qt5::m_dk_blue      = Color("dark blue");
const STATIC_COLOR gui_palette_qt5::m_dk_magenta   = Color("dark magenta");
const STATIC_COLOR gui_palette_qt5::m_dk_cyan      = Color("dark cyan");
const STATIC_COLOR gui_palette_qt5::m_dk_white     = Color("grey");

/**
 * Extended colors in the palette.  The greys are defined separately and are
 * invertible.
 */

const STATIC_COLOR gui_palette_qt5::m_orange       = Color("orange");
const STATIC_COLOR gui_palette_qt5::m_pink         = Color("pink");
const STATIC_COLOR gui_palette_qt5::m_grey         = Color("grey");

const STATIC_COLOR gui_palette_qt5::m_dk_orange    = Color("dark orange");
const STATIC_COLOR gui_palette_qt5::m_dk_pink      = Color("dark pink");
const STATIC_COLOR gui_palette_qt5::m_dk_grey      = Color("dark grey");

/**
 * Invertible colors.
 */

STATIC_COLOR gui_palette_qt5::m_grey_paint         = Color("grey");
STATIC_COLOR gui_palette_qt5::m_dk_grey_paint      = Color("grey50");
STATIC_COLOR gui_palette_qt5::m_lt_grey_paint      = Color("light grey");
STATIC_COLOR gui_palette_qt5::m_blk_paint          = Color("black");
STATIC_COLOR gui_palette_qt5::m_wht_paint          = Color("white");
STATIC_COLOR gui_palette_qt5::m_blk_key_paint      = Color("black");
STATIC_COLOR gui_palette_qt5::m_wht_key_paint      = Color("white");
STATIC_COLOR gui_palette_qt5::m_tempo_paint        = Color("magenta"); // dark
#if defined SEQ66_USE_BLACK_SELECTION_BOX
STATIC_COLOR gui_palette_qt5::m_sel_paint          = Color("black");
#else
STATIC_COLOR gui_palette_qt5::m_sel_paint          = Color("orange");
#endif

/**
 *  Principal constructor.  In the constructor one can only allocate colors;
 *  get_window() returns 0 because this window has not yet been realized.
 *  Also note that the possible color names that can be used are found in
 *  /usr/share/X11/rgb.txt.
 */

gui_palette_qt5::gui_palette_qt5 ()
 :
    m_palette           (),
    m_pen_palette       (),
    m_line_color        (Color("dark cyan")),           // alternative to black
    m_progress_color    (Color("black")),
    m_bg_color          (),
    m_fg_color          ()
{
    /**
     * \todo
     *      Use an array of colors instead of this switch.
     */

    if (usr().inverse_colors())
        load_inverse_palette(true);

    int colorcode = usr().progress_bar_colored();
    switch (colorcode)
    {
    case static_cast<int>(progress_colors::black):
        m_progress_color = m_black;
        break;

    case static_cast<int>(progress_colors::dark_red):
        m_progress_color = m_dk_red;
        break;

    case static_cast<int>(progress_colors::dark_green):
        m_progress_color = m_dk_green;
        break;

    case static_cast<int>(progress_colors::dark_orange):
        m_progress_color = m_dk_orange;
        break;

    case static_cast<int>(progress_colors::dark_blue):
        m_progress_color = m_dk_blue;
        break;

    case static_cast<int>(progress_colors::dark_magenta):
        m_progress_color = m_dk_magenta;
        break;

    case static_cast<int>(progress_colors::dark_cyan):
        m_progress_color = m_dk_cyan;
        break;
    }

    /*
     * Fill in the slot and pen palettes!
     */

    initialize();
}

/**
 *  Provides a destructor to delete allocated objects.
 */

gui_palette_qt5::~gui_palette_qt5 ()
{
    // Anything to do?
}

/**
 *  Provides an alternate color palette.  Inverse is not a complete inverse.  It
 *  is more like a "night" mode.  However, there are still some bright colors
 *  even in this mode.  Some colors, such as the selection color (orange) are
 *  the same in either mode.
 *
 * \param inverse
 *      If true, load the alternate palette.  Otherwise, load the default
 *      palette.
 */

void
gui_palette_qt5::load_inverse_palette (bool inverse)
{
    if (inverse)
    {
        m_grey_paint    = Color("grey");
        m_dk_grey_paint = Color("light grey");
        m_lt_grey_paint = Color("grey50");
        m_blk_paint     = Color("white");
        m_wht_paint     = Color("black");
        m_blk_key_paint = Color("black");
        m_wht_key_paint = Color("grey");
        m_tempo_paint   = Color("magenta");
#if defined SEQ66_USE_BLACK_SELECTION_BOX
        m_sel_paint     = Color("white");
#else
        m_sel_paint     = Color("orange");
#endif
        m_is_inverse    = true;
    }
    else
    {
        m_grey_paint    = Color("grey");
        m_dk_grey_paint = Color("grey50");
        m_lt_grey_paint = Color("light grey");
        m_blk_paint     = Color("black");
        m_wht_paint     = Color("white");
        m_blk_key_paint = Color("black");
        m_wht_key_paint = Color("white");
        m_tempo_paint   = Color("magenta");     /* or dark magenta          */
#if defined SEQ66_USE_BLACK_SELECTION_BOX
        m_sel_paint     = Color("black");
#else
        m_sel_paint     = Color("orange");
#endif
        m_is_inverse    = false;
    }
}

/**
 *  Static function to invert a given Color.
 */

gui_palette_qt5::Color
gui_palette_qt5::calculate_inverse (const Color & c)
{
    int r, g, b, a;
    c.getRgb(&r, &g, &b, &a);
    r = a - r;
    g = a - g;
    b = a - b;
    return gui_palette_qt5::Color(r, g, b, a);
}

/**
 *  Adds all of the main palette colors in the PaletteColor enumeration into
 *  the palette contain.  The palette is meant to be used to color sequences
 *  differently, though this feature is not yet supported in the Gtkmm-2.4
 *  version of Seq66.
 */

void
gui_palette_qt5::initialize ()
{
    /*
     * Slot background colors
     */

    m_palette.clear();                                      /* just in case */
    m_palette.add(PaletteColor::black,       m_black,    "Black");
    m_palette.add(PaletteColor::red,         m_red,      "Red");
    m_palette.add(PaletteColor::green,       m_green,    "Green");
    m_palette.add(PaletteColor::yellow,      m_yellow,   "Yellow");
    m_palette.add(PaletteColor::blue,        m_blue,     "Blue");
    m_palette.add(PaletteColor::magenta,     m_magenta,  "Magenta");
    m_palette.add(PaletteColor::cyan,        m_cyan,     "Cyan");
    m_palette.add(PaletteColor::white,       m_white,    "White");
    m_palette.add(PaletteColor::orange,      m_orange,   "Orange");
    m_palette.add(PaletteColor::pink,        m_pink,     "Pink");
    m_palette.add(PaletteColor::grey,        m_grey,     "Gray");        // hmmmm

    m_palette.add(PaletteColor::dk_black,    m_dk_black, "Dk Black");    // hmmmm
    m_palette.add(PaletteColor::dk_red,      m_dk_red,   "Dk Red");
    m_palette.add(PaletteColor::dk_green,    m_dk_green, "Dk Green");

    /*
     * Weird, this shows up as black!  Same in Gtkmm!
     * m_palette.add(PaletteColor::DK_YELLOW, m_dk_yellow, "Dk Yellow");
     */

    m_palette.add(PaletteColor::dk_yellow,   m_yellow,   "Dk Yellow");
    m_palette.add(PaletteColor::dk_blue,     m_dk_blue,  "Dk Blue");
    m_palette.add(PaletteColor::dk_magenta,  m_dk_magenta, "Dk Magenta");
    m_palette.add(PaletteColor::dk_cyan,     m_dk_cyan,  "Dk Cyan");
    m_palette.add(PaletteColor::dk_white,    m_dk_white, "Dk White");    // hmmmm
    m_palette.add(PaletteColor::dk_orange,   m_dk_orange, "Dk Orange");
    m_palette.add(PaletteColor::dk_pink,     m_dk_pink,  "Dk Pink");
    m_palette.add(PaletteColor::dk_grey,     m_dk_grey,  "Dk Grey");     // hmmmm
    m_palette.add(PaletteColor::none,        m_white,    "None");

    /*
     * Pen/inverse colors
     */

    m_pen_palette.clear();                  /* just in case */
    m_pen_palette.add(PaletteColor::black,   m_white, "white");
    m_pen_palette.add(PaletteColor::red,     m_white, "white");
    m_pen_palette.add(PaletteColor::green,   m_white, "white");
    m_pen_palette.add(PaletteColor::yellow,  m_black, "black");
    m_pen_palette.add(PaletteColor::blue,    m_white, "white");
    m_pen_palette.add(PaletteColor::magenta, m_white, "white");
    m_pen_palette.add(PaletteColor::cyan,    m_black, "black");
    m_pen_palette.add(PaletteColor::white,   m_black, "black");
    m_pen_palette.add(PaletteColor::orange,  m_white, "white");
    m_pen_palette.add(PaletteColor::pink,    m_black, "black");
    m_pen_palette.add(PaletteColor::grey,    m_black, "black");
    m_pen_palette.add(PaletteColor::dk_black, m_white, "white");
    m_pen_palette.add(PaletteColor::dk_red,  m_white, "white");
    m_pen_palette.add(PaletteColor::dk_green, m_white, "white");
    m_pen_palette.add(PaletteColor::dk_yellow, m_black, "black");
    m_pen_palette.add(PaletteColor::dk_blue, m_white, "white");
    m_pen_palette.add(PaletteColor::dk_magenta, m_white, "white");
    m_pen_palette.add(PaletteColor::dk_cyan, m_white, "white");
    m_pen_palette.add(PaletteColor::dk_white, m_white, "white");
    m_pen_palette.add(PaletteColor::dk_orange, m_white, "white");
    m_pen_palette.add(PaletteColor::dk_pink, m_white, "white");
    m_pen_palette.add(PaletteColor::dk_grey, m_white, "white");
    m_pen_palette.add(PaletteColor::none,    m_black, "black");
}

/**
 *  Gets a color, but returns a modified value via the function
 *  Gdk::Color::set_hsv(h, s, v).  This function sets the color, by specifying
 *  hue, saturation, and value (brightness).
 *
 * \param index
 *      The index into the Seq66 stock palette.
 *
 * \param h
 *      Hue factor, has no default at this time.  Kepler34 treats this value
 *      as a 1.0 factor for the hue.
 *
 * \param s
 *      Saturation, in the range 0..1, defaults to 0.65.
 *
 * \param v
 *      Value (a.k.a. brightness), defaults to 1, in the range 0..1.
 */

gui_palette_qt5::Color
gui_palette_qt5::get_color_ex
(
    PaletteColor index,
    double h, double s, double v
) const
{
    gui_palette_qt5::Color result = m_palette.get_color(index);
    result.setHsv(result.hue() * h, result.saturation() * s, result.value() *  v);
    return result;
}

/**
 *  Gets a color and fixes its HSV in a stock manner, unless the index does
 *  not specify a color.
 *
 * \param index
 *      The index into the Seq66 stock palette.
 *
 * \return
 *      Returns a copy of the fixed color.  If it is not a color, then
 *      "white" is returned.
 */

gui_palette_qt5::Color
gui_palette_qt5::get_color_fix (PaletteColor index) const
{
    if (m_palette.no_color(index))
    {
        return m_palette.get_color(PaletteColor::none);
    }
    else
    {
        gui_palette_qt5::Color c = m_palette.get_color(index);
        if (c.value() != 255)
            c.setHsv(c.hue(), c.saturation() * 0.65, c.value() * 1.2);

        return c;
    }
}

/**
 *  Gets a color from the palette, based on the index value, and returns the
 *  inverted version.
 *
 * \param index
 *      Indicates which color to get.  This index is checked for range, and, if
 *      out of range, the default color object, indexed by PaletteColor::none,
 *      is returned.
 *
 * \param index
 *      The index into the Seq66 stock palette.
 *
 * \return
 *      Returns a copy of the inverted color.
 */

gui_palette_qt5::Color
gui_palette_qt5::get_color_inverse (PaletteColor index) const
{
    gui_palette_qt5::Color c = m_palette.get_color(index);
    if (index != PaletteColor::none)
    {
        int r, g, b, a;
        c.getRgb(&r, &g, &b, &a);
        r = a - r;
        g = a - g;
        b = a - b;
        return Color(r, g, b, a);
    }
    else
        return m_black;
}

/**
 *  Shows the #RRGGBB string for the given color on stdout.  For debugging.
 *
 * \param c
 *      The color to show.
 */

void
show_color_rgb (const gui_palette_qt5::Color & c)
{
    QString qs = c.name();

#if defined SEQ66_PLATFORM_WINDOWS
    std::string text = qs.toLocal8Bit().constData();    /* Windows text */
#else
    std::string text = qs.toUtf8().constData();         /* UTF-8 text   */
#endif

    printf("color: %s\n", text.c_str());
}

}           // namespace seq66

/*
 * gui_palette_qt5.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


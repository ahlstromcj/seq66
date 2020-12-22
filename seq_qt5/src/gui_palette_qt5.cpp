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
 * \updates       2020-12-21
 * \license       GNU GPLv2 or above
 *
 *  One possible idea would be a color configuration that would radically
 *  change drawing of the lines and pixmaps, opening up the way for night
 *  views and color schemes that match the desktop theme.
 *
 *  There are 19 predefined QColor objects: white, black, red, darkRed, green,
 *  darkGreen, blue, darkBlue, cyan, darkCyan, magenta, darkMagenta, yellow,
 *  darkYellow, gray, darkGray, lightGray, color0 and color1, accessible as
 *  members of the Qt namespace (ie. Qt::red).
 */

#include "cfg/settings.hpp"             /* seq66::rc() or seq66::usr()      */
#include "gui_palette_qt5.hpp"          /* seq66::gui_palette_qt5           */
#include "util/basic_macros.hpp"        /* seq66::file_error() function     */
#include "util/palette.hpp"             /* enum class progress_colors       */
#include "util/strfunctions.hpp"        /* seq66 string functions           */

#define STATIC_COLOR                    gui_palette_qt5::Color

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Provide access to the internak,  basic color palette.  We use a static
 *  function to access this item (plus a couple of things we don't need).
 *  Note that we have to associate a file-name later, if any.  A little clumsy;
 *  but we don't want to move this into a non-Qt-GUI module.
 */

gui_palette_qt5 &
global_palette ()
{
    static gui_palette_qt5 s_pallete;
    return s_pallete;
}

gui_palette_qt5::Color
get_color_fix (PaletteColor index)
{
    return global_palette().get_color_fix(index);
}

gui_palette_qt5::Color
get_pen_color (PaletteColor index)
{
    return global_palette().get_pen_color(index);
}

gui_palette_qt5::Color
drum_paint ()
{
    return global_palette().drum_paint();
}

gui_palette_qt5::Color
sel_paint ()
{
    return global_palette().sel_paint();
}

gui_palette_qt5::Color
tempo_paint ()
{
    return global_palette().tempo_paint();
}

/**
 *  Background paint uses the invertible color m_wht_paint.
 */

gui_palette_qt5::Color
background_paint ()
{
    return global_palette().white_paint();
}

/**
 *  Foreground paint uses the invertible color m_blk_paint.
 */

gui_palette_qt5::Color
foreground_paint ()
{
    return global_palette().black_paint();
}

/**
 *  Beat paint uses the grey_paint.  Invertible, but the same color?
 */

gui_palette_qt5::Color
beat_paint ()
{
    return global_palette().grey_paint();
}

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

const STATIC_COLOR gui_palette_qt5::m_dk_black     = Color("dark slate grey");
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
const STATIC_COLOR gui_palette_qt5::m_color_18     = Color("pale green");
const STATIC_COLOR gui_palette_qt5::m_color_19     = Color("khaki");
const STATIC_COLOR gui_palette_qt5::m_color_20     = Color("light blue");
const STATIC_COLOR gui_palette_qt5::m_color_21     = Color("violet");
const STATIC_COLOR gui_palette_qt5::m_color_22     = Color("turquoise");
const STATIC_COLOR gui_palette_qt5::m_grey         = Color("grey");

const STATIC_COLOR gui_palette_qt5::m_dk_orange    = Color("dark orange");
const STATIC_COLOR gui_palette_qt5::m_dk_pink      = Color("deep pink");
const STATIC_COLOR gui_palette_qt5::m_color_26     = Color("sea green");
const STATIC_COLOR gui_palette_qt5::m_color_27     = Color("dark khaki");
const STATIC_COLOR gui_palette_qt5::m_color_28     = Color("dark slate blue");
const STATIC_COLOR gui_palette_qt5::m_color_29     = Color("dark violet");
const STATIC_COLOR gui_palette_qt5::m_color_30     = Color("dark turquoise");
const STATIC_COLOR gui_palette_qt5::m_dk_grey      = Color("dark grey");

/**
 * Invertible colors.
 */

STATIC_COLOR gui_palette_qt5::m_drum_paint         = Color("red");
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

gui_palette_qt5::gui_palette_qt5 (const std::string & filename)
 :
    basesettings        (filename),
    m_palette           (),
    m_pen_palette       (),
    m_line_color        (Color("dark cyan")),           // alternative to black
    m_progress_color    (Color("black")),
    m_bg_color          (),
    m_fg_color          ()
{
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

    reset();
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
        m_drum_paint    = Color("green");
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
        m_drum_paint    = Color("red");
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
 *  Clears the background and foreground color containers, and adds the "None"
 *  color to make sure it is always present.
 */

void
gui_palette_qt5::clear ()
{
    m_palette.clear();
    m_palette.add(PaletteColor::none, m_white, "None");
    m_pen_palette.clear();
    m_pen_palette.add(PaletteColor::none, m_black, "Black");
}

/**
 *  Adds all of the main palette colors in the PaletteColor enumeration into
 *  the palette contain.  The palette is meant to be used to color sequences
 *  differently, though this feature is not yet supported in the Gtkmm-2.4
 *  version of Seq66.
 *
 *  Weird, this shows up as black!  Same in Gtkmm!
 *
 *      m_palette.add(PaletteColor::DK_YELLOW, m_dk_yellow, "Dk Yellow");
 *
 *  Slot background colors.
 */

void
gui_palette_qt5::reset_backgrounds ()
{
    m_palette.clear();                                      /* just in case */
    m_palette.add(PaletteColor::black,       m_black,    "Black");
    m_palette.add(PaletteColor::red,         m_red,      "Red");
    m_palette.add(PaletteColor::green,       m_green,    "Green");
    m_palette.add(PaletteColor::yellow,      m_yellow,   "Yellow");
    m_palette.add(PaletteColor::blue,        m_blue,     "Blue");
    m_palette.add(PaletteColor::magenta,     m_magenta,  "Magenta");
    m_palette.add(PaletteColor::cyan,        m_cyan,     "Cyan");
    m_palette.add(PaletteColor::white,       m_white,    "White");

    m_palette.add(PaletteColor::dk_black,    m_dk_black, "Dark Black");    // hmmmm
    m_palette.add(PaletteColor::dk_red,      m_dk_red,   "Dark Red");
    m_palette.add(PaletteColor::dk_green,    m_dk_green, "Dark Green");
    m_palette.add(PaletteColor::dk_yellow,   m_yellow,   "Dark Yellow");
    m_palette.add(PaletteColor::dk_blue,     m_dk_blue,  "Dark Blue");
    m_palette.add(PaletteColor::dk_magenta,  m_dk_magenta, "Dark Magenta");
    m_palette.add(PaletteColor::dk_cyan,     m_dk_cyan,  "Dark Cyan");
    m_palette.add(PaletteColor::dk_white,    m_dk_white, "Dark White");    // hmmmm

    m_palette.add(PaletteColor::orange,      m_orange,   "Orange");
    m_palette.add(PaletteColor::pink,        m_pink,     "Pink");
    m_palette.add(PaletteColor::color_18,    m_color_18, "Pale Green");
    m_palette.add(PaletteColor::color_19,    m_color_19, "Khaki");
    m_palette.add(PaletteColor::color_20,    m_color_20, "Light Blue");
    m_palette.add(PaletteColor::color_21,    m_color_21, "Light Magenta");
    m_palette.add(PaletteColor::color_22,    m_color_22, "Turquoise");
    m_palette.add(PaletteColor::grey,        m_grey,     "Grey");

    m_palette.add(PaletteColor::dk_orange,   m_dk_orange, "Dk Orange");
    m_palette.add(PaletteColor::dk_pink,     m_dk_pink,   "Dark Pink");
    m_palette.add(PaletteColor::color_26,    m_color_26,  "Sea Green");
    m_palette.add(PaletteColor::color_27,    m_color_27,  "Dark Khaki");
    m_palette.add(PaletteColor::color_28,    m_color_28,  "Dark Slate Blue");
    m_palette.add(PaletteColor::color_29,    m_color_29,  "Dark Violet");
    m_palette.add(PaletteColor::color_30,    m_color_30,  "Dark Turquoise");
    m_palette.add(PaletteColor::dk_grey,     m_dk_grey,   "Dark Grey");
}

/**
 *  Sets pen/inverse colors,
 */

void
gui_palette_qt5::reset_pens ()
{
    m_pen_palette.clear();                  /* just in case */
    m_pen_palette.add(PaletteColor::black,      m_white,   "White");
    m_pen_palette.add(PaletteColor::red,        m_white,   "White");
    m_pen_palette.add(PaletteColor::green,      m_white,   "White");
    m_pen_palette.add(PaletteColor::yellow,     m_black,   "Black");
    m_pen_palette.add(PaletteColor::blue,       m_white,   "White");
    m_pen_palette.add(PaletteColor::magenta,    m_white,   "White");
    m_pen_palette.add(PaletteColor::cyan,       m_black,   "Black");
    m_pen_palette.add(PaletteColor::white,      m_black,   "Black");

    m_pen_palette.add(PaletteColor::dk_black,   m_white,   "White");
    m_pen_palette.add(PaletteColor::dk_red,     m_white,   "White");
    m_pen_palette.add(PaletteColor::dk_green,   m_white,   "White");
    m_pen_palette.add(PaletteColor::dk_yellow,  m_black,   "Black");
    m_pen_palette.add(PaletteColor::dk_blue,    m_white,   "White");
    m_pen_palette.add(PaletteColor::dk_magenta, m_white,   "White");
    m_pen_palette.add(PaletteColor::dk_cyan,    m_white,   "White");
    m_pen_palette.add(PaletteColor::dk_white,   m_white,   "White");

    m_pen_palette.add(PaletteColor::orange,     m_white,   "White");
    m_pen_palette.add(PaletteColor::pink,       m_black,   "Black");
    m_pen_palette.add(PaletteColor::color_18,   m_black,   "Black");
    m_pen_palette.add(PaletteColor::color_19,   m_black,   "Black");
    m_pen_palette.add(PaletteColor::color_20,   m_black,   "Black");
    m_pen_palette.add(PaletteColor::color_21,   m_black,   "Black");
    m_pen_palette.add(PaletteColor::color_22,   m_black,   "Black");
    m_pen_palette.add(PaletteColor::grey,       m_black,   "Black");

    m_pen_palette.add(PaletteColor::dk_orange,  m_black,   "Black");
    m_pen_palette.add(PaletteColor::dk_pink,    m_black,   "Black");
    m_pen_palette.add(PaletteColor::color_26,   m_black,   "Black");
    m_pen_palette.add(PaletteColor::color_27,   m_black,   "Black");
    m_pen_palette.add(PaletteColor::color_28,   m_black,   "Black");
    m_pen_palette.add(PaletteColor::color_29,   m_black,   "Black");
    m_pen_palette.add(PaletteColor::color_30,   m_black,   "Black");
    m_pen_palette.add(PaletteColor::dk_grey,    m_black,   "Black");
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
    gui_palette_qt5::Color c = m_palette.get_color(index);
    c.setHsv(c.hue() * h, c.saturation() * s, c.value() *  v);
    return c;
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
            c.setHsv(c.hue(), c.saturation() * 0.65, c.value());    // * 1.2);

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
    if (index != PaletteColor::none)
    {
        gui_palette_qt5::Color c = m_palette.get_color(index);
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

std::string
gui_palette_qt5::make_color_stanza (PaletteColor index) const
{
    std::string result;
    if (index != PaletteColor::none)
    {
        int number = static_cast<int>(index);
        gui_palette_qt5::Color backc = m_palette.get_color(index);
        gui_palette_qt5::Color textc = m_pen_palette.get_color(index);
        std::string bname = "\"" + get_color_name(index) + "\"";
        std::string tname = "\"" + get_pen_color_name(index) + "\"";
        int br, bg, bb, ba;
        int tr, tg, tb, ta;
        char temp[128];
        backc.getRgb(&br, &bg, &bb, &ba);
        textc.getRgb(&tr, &tg, &tb, &ta);
        snprintf
        (
            temp, sizeof temp,
            "%2d %18s [ 0x%02X%02X%02X%02X ] %18s [ 0x%02X%02X%02X%02X ]",
            number,
            bname.c_str(), ba, br, bg, bb,
            tname.c_str(), ta, tr, tb, tg
        );
        result = temp;
    }
    return result;
}

/**
 *  This line matches the string generated by gui_palette_qt5 ::
 *  get_color_stanza().  The first integer is the color number, ranging from 0
 *  to 31.  The first string is the name of the background color.  The first
 *  stanza (in square brackets) are the RGB + Alpha values for the background.
 *  The second string is the foreground color name, and the second stanza is the
 *  foreground color.  Note that the alpha values are not currently used and are
 *  set to zero.  Room for expansion.
 */

#if defined USE_PALLET_SSCANF

static const char * const sg_scanf_fmt =
    "%d \"%s\" [ %x ] \"%s\" [ %x ]";                       /* 0xAARRGGBB   */

#endif

bool
gui_palette_qt5::add_color_stanza (const std::string & stanza)
{
#if defined USE_PALLET_SSCANF
    char backgn[32];
    char textn[32];
    int number;
    unsigned backargb;
    unsigned textargb;
    int count = std::sscanf
    (
        stanza.c_str(), sg_scanf_fmt,
        &number, backgn, &backargb, textn, &textargb
    );
    bool result = count == 5;
    if (result)
    {
        std::string bname = backgn;
        std::string tname = textn;
        Color background(backargb);
        Color foreground(textargb);
        result = add(number, background, bname, foreground, tname);
    }
#else
    std::string backname;
    unsigned backargb;
    std::string textname;
    unsigned textargb;
    bool result = ! stanza.empty();
    if (result)
    {
        int number = std::stoi(stanza);     /* gets first column value  */
        std::string argb;
        std::string::size_type lpos = 0;
        backname = next_quoted_string(stanza);
        result = ! backname.empty();
        if (result)
        {
            lpos = stanza.find_first_of("[");
            argb = next_bracketed_string(stanza, lpos - 1);
            result = ! argb.empty();
            if (result)
                backargb = string_to_unsigned(argb);
        }
        if (result)
        {
            textname = next_quoted_string(stanza, lpos);
            result = ! textname.empty();
        }
        if (result)
        {
            lpos = stanza.find_first_of("[", lpos + 1);
            argb = next_bracketed_string(stanza, lpos - 1);
            result = ! argb.empty();
            if (result)
                textargb = string_to_unsigned(argb);
        }
        if (result)
        {
            Color background(backargb);
            Color foreground(textargb);
            result = add(number, background, backname, foreground, textname);
        }
    }
#endif
    return result;
}

bool
gui_palette_qt5::add
(
    int number,
    const Color & bg, const std::string & bgname,
    const Color & fg, const std::string & fgname
)
{
    bool result = number >= 0 && number < palette_size();
    if (result)
    {
        PaletteColor index = static_cast<PaletteColor>(number);
        result = m_palette.add(index, bg, bgname);
        if (result)
            result = m_pen_palette.add(index, fg, fgname);
    }
    return result;
}

}           // namespace seq66

/*
 * gui_palette_qt5.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


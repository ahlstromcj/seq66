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
 * \updates       2020-12-23
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
#include "util/palette.hpp"             /* enum class ProgressColors        */
#include "util/strfunctions.hpp"        /* seq66 string functions           */

#define COLOR     gui_palette_qt5::Color

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

std::string
get_color_name_ex (PaletteColor index)
{
    return global_palette().get_color_name_ex(index);
}

bool
no_color (int c)
{
    return global_palette().no_color(c);
}


/**
 *  Foreground paint uses the invertible color for black.
 */

gui_palette_qt5::Color
foreground_paint ()
{
    return global_palette().get_color(InvertibleColor::black);
}

/**
 *  Background paint uses the invertible color for white.
 */

gui_palette_qt5::Color
background_paint ()
{
    return global_palette().get_color(InvertibleColor::white);
}

gui_palette_qt5::Color
label_paint ()
{
    gui_palette_qt5::Color r =
        global_palette().get_color(InvertibleColor::label);
    return r;
}

gui_palette_qt5::Color
sel_paint ()
{
    return global_palette().get_color(InvertibleColor::selection);
}

gui_palette_qt5::Color
drum_paint ()
{
    return global_palette().get_color(InvertibleColor::drum);
}

gui_palette_qt5::Color
tempo_paint ()
{
    return global_palette().get_color(InvertibleColor::tempo);
}

gui_palette_qt5::Color
black_key_paint ()
{
    return global_palette().get_color(InvertibleColor::black_key);
}

gui_palette_qt5::Color
white_key_paint ()
{
    return global_palette().get_color(InvertibleColor::white_key);
}

/**
 *  Beat paint uses the grey_paint.  Invertible, but the same color?
 */

gui_palette_qt5::Color
beat_paint ()
{
    return global_palette().get_color(InvertibleColor::dk_grey);
}

gui_palette_qt5::Color
step_paint ()
{
    return global_palette().get_color(InvertibleColor::lt_grey);
}

/**
 * Bright constant colors
 */

static COLOR m_black;
static COLOR m_red;
static COLOR m_green;
static COLOR m_yellow;
static COLOR m_blue;
static COLOR m_magenta;
static COLOR m_cyan;
static COLOR m_white;

/**
 * Dark staticant colors
 */

static COLOR m_dk_black;
static COLOR m_dk_red;
static COLOR m_dk_green;
static COLOR m_dk_yellow;
static COLOR m_dk_blue;
static COLOR m_dk_magenta;
static COLOR m_dk_cyan;
static COLOR m_dk_white;

/**
 * Extended colors in the palette.
 */

static COLOR m_orange;
static COLOR m_pink;
static COLOR m_color_18;
static COLOR m_color_19;
static COLOR m_color_20;
static COLOR m_color_21;
static COLOR m_color_22;
static COLOR m_grey;

static COLOR m_dk_orange;
static COLOR m_dk_pink;
static COLOR m_color_26;
static COLOR m_color_27;
static COLOR m_color_28;
static COLOR m_color_29;
static COLOR m_lt_grey;
static COLOR m_dk_grey;

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
    m_nrm_palette       (),
    m_inv_palette       (),
    m_is_inverse        (false),
    m_is_loaded         (false),
    m_line_color        (Color("dark cyan")),           // alternative to black
    m_progress_color    (Color("black")),
    m_bg_color          (),
    m_fg_color          ()
{
    int colorcode = usr().progress_bar_colored();
    switch (colorcode)
    {
    case static_cast<int>(ProgressColors::black):
        m_progress_color = m_black;
        break;

    case static_cast<int>(ProgressColors::dark_red):
        m_progress_color = m_dk_red;
        break;

    case static_cast<int>(ProgressColors::dark_green):
        m_progress_color = m_dk_green;
        break;

    case static_cast<int>(ProgressColors::dark_orange):
        m_progress_color = m_dk_orange;
        break;

    case static_cast<int>(ProgressColors::dark_blue):
        m_progress_color = m_dk_blue;
        break;

    case static_cast<int>(ProgressColors::dark_magenta):
        m_progress_color = m_dk_magenta;
        break;

    case static_cast<int>(ProgressColors::dark_cyan):
        m_progress_color = m_dk_cyan;
        break;
    }

    /*
     * Fill in the slot and pen palettes!
     */

    load_static_colors(usr().inverse_colors());
    reset();
}

/**
 *  Provides a destructor to delete allocated objects.
 */

gui_palette_qt5::~gui_palette_qt5 ()
{
    // Anything to do?
}

void
gui_palette_qt5::load_static_colors (bool inverse)
{
    m_is_inverse = inverse;
    if (! m_is_loaded)
    {
        m_is_loaded = true;
        m_black        = COLOR("black");
        m_red          = COLOR("red");
        m_green        = COLOR("green");
        m_yellow       = COLOR("yellow");
        m_blue         = COLOR("blue");
        m_magenta      = COLOR("magenta");
        m_cyan         = COLOR("cyan");
        m_white        = COLOR("white");
        m_dk_black     = COLOR("dark slate grey");
        m_dk_red       = COLOR("dark red");
        m_dk_green     = COLOR("dark green");
        m_dk_yellow    = COLOR("dark yellow");
        m_dk_blue      = COLOR("dark blue");
        m_dk_magenta   = COLOR("dark magenta");
        m_dk_cyan      = COLOR("dark cyan");
        m_dk_white     = COLOR("grey");
        m_orange       = COLOR("orange");
        m_pink         = COLOR("pink");
        m_color_18     = COLOR("pale green");
        m_color_19     = COLOR("khaki");
        m_color_20     = COLOR("light blue");
        m_color_21     = COLOR("violet");
        m_color_22     = COLOR("turquoise");
        m_grey         = COLOR("grey");
        m_dk_orange    = COLOR("dark orange");
        m_dk_pink      = COLOR("deep pink");
        m_color_26     = COLOR("sea green");
        m_color_27     = COLOR("dark khaki");
        m_color_28     = COLOR("dark slate blue");
        m_color_29     = COLOR("dark violet");
        m_lt_grey      = COLOR("light slate grey");
        m_dk_grey      = COLOR("dark slate grey");
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

void
gui_palette_qt5::clear_invertible ()
{
    m_nrm_palette.clear();
    m_inv_palette.clear();
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
    m_palette.add(PaletteColor::none,        m_white,    "None");
    m_palette.add(PaletteColor::black,       m_black,    "Black");
    m_palette.add(PaletteColor::red,         m_red,      "Red");
    m_palette.add(PaletteColor::green,       m_green,    "Green");
    m_palette.add(PaletteColor::yellow,      m_yellow,   "Yellow");
    m_palette.add(PaletteColor::blue,        m_blue,     "Blue");
    m_palette.add(PaletteColor::magenta,     m_magenta,  "Magenta");
    m_palette.add(PaletteColor::cyan,        m_cyan,     "Cyan");
    m_palette.add(PaletteColor::white,       m_white,    "White");

    m_palette.add(PaletteColor::dk_black,    m_dk_black, "Dark Black");
    m_palette.add(PaletteColor::dk_red,      m_dk_red,   "Dark Red");
    m_palette.add(PaletteColor::dk_green,    m_dk_green, "Dark Green");
    m_palette.add(PaletteColor::dk_yellow,   m_yellow,   "Dark Yellow");
    m_palette.add(PaletteColor::dk_blue,     m_dk_blue,  "Dark Blue");
    m_palette.add(PaletteColor::dk_magenta,  m_dk_magenta, "Dark Magenta");
    m_palette.add(PaletteColor::dk_cyan,     m_dk_cyan,  "Dark Cyan");
    m_palette.add(PaletteColor::dk_white,    m_dk_white, "Dark White");

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
    m_palette.add(PaletteColor::color_30,    m_lt_grey,   "Light Grey");
    m_palette.add(PaletteColor::dk_grey,     m_dk_grey,   "Dark Grey");
}

/**
 *  Sets pen colors,
 */

void
gui_palette_qt5::reset_pens ()
{
    m_pen_palette.clear();                  /* just in case */
    m_pen_palette.add(PaletteColor::none,       m_black,   "Black");
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

void
gui_palette_qt5::reset_invertibles ()
{
    m_nrm_palette.clear();
    m_nrm_palette.add(InvertibleColor::black,       m_black,    "Foreground");
    m_nrm_palette.add(InvertibleColor::white,       m_white,    "Background");
    m_nrm_palette.add(InvertibleColor::label,       m_black,    "Label");
    m_nrm_palette.add(InvertibleColor::selection,   m_orange,   "Selection");
    m_nrm_palette.add(InvertibleColor::drum,        m_red,      "Drum");
    m_nrm_palette.add(InvertibleColor::tempo,       m_magenta,  "Tempo");
    m_nrm_palette.add(InvertibleColor::black_key,   m_black,    "Black Keys");
    m_nrm_palette.add(InvertibleColor::white_key,   m_white,    "White Keys");
    m_nrm_palette.add(InvertibleColor::grey,        m_grey,     "Medium Line");
    m_nrm_palette.add(InvertibleColor::dk_grey,     m_dk_grey,  "Beat Line");
    m_nrm_palette.add(InvertibleColor::lt_grey,     m_lt_grey,  "Step Line");

    m_inv_palette.clear();
    m_inv_palette.add(InvertibleColor::black,       m_white,    "Foreground");
    m_inv_palette.add(InvertibleColor::white,       m_black,    "Background");
    m_inv_palette.add(InvertibleColor::label,       m_white,    "Label");
    m_inv_palette.add(InvertibleColor::selection,   m_yellow,   "Selection");
    m_inv_palette.add(InvertibleColor::drum,        m_green,    "Drum");
    m_inv_palette.add(InvertibleColor::tempo,       m_magenta,  "Tempo");
    m_inv_palette.add(InvertibleColor::black_key,   m_white,    "Black Keys");
    m_inv_palette.add(InvertibleColor::white_key,   m_black,    "White Keys");
    m_inv_palette.add(InvertibleColor::grey,        m_lt_grey,  "Medium Line");
    m_inv_palette.add(InvertibleColor::dk_grey,     m_dk_grey,  "Beat Line");
    m_inv_palette.add(InvertibleColor::lt_grey,     m_grey,   "Step Line");
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
gui_palette_qt5::make_color_stanza (int number, bool inverse) const
{
    std::string result;
    if (number >= 0)
    {
        gui_palette_qt5::Color backc;
        gui_palette_qt5::Color textc;
        std::string bname = "\"";
        std::string tname = "\"";
        if (inverse)
        {
            InvertibleColor index = static_cast<InvertibleColor>(number);
            backc = m_nrm_palette.get_color(index);
            textc = m_inv_palette.get_color(index);
            bname += get_color_name(index);
            tname += get_inv_color_name(index);
        }
        else
        {
            PaletteColor index = static_cast<PaletteColor>(number);
            backc = m_palette.get_color(index);
            textc = m_pen_palette.get_color(index);
            bname += get_color_name(index);
            tname += get_pen_color_name(index);
        }
        bname += "\"";
        tname += "\"";

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

bool
gui_palette_qt5::add_color_stanza (const std::string & stanza, bool inverse)
{
    bool result = ! stanza.empty();
    if (result)
    {
        std::string backname;
        unsigned backargb;
        std::string textname;
        unsigned textargb;
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
            if (inverse)
            {
                result = add_invertible
                (
                    number, background, backname, foreground, textname
                );
            }
            else
            {
                result = add
                (
                    number, background, backname, foreground, textname
                );
            }
        }
    }
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

bool
gui_palette_qt5::add_invertible
(
    int number,
    const Color & bg, const std::string & bgname,
    const Color & fg, const std::string & fgname
)
{
    bool result = number >= 0;
    if (result)
    {
        InvertibleColor index = static_cast<InvertibleColor>(number);
        result = m_nrm_palette.add(index, bg, bgname);
        if (result)
            result = m_inv_palette.add(index, fg, fgname);
    }
    return result;
}

const gui_palette_qt5::Color &
gui_palette_qt5::get_color (InvertibleColor index) const
{
    const Color & result = is_inverse() ?
        m_inv_palette.get_color(index) :
        m_nrm_palette.get_color(index) ;

    return result;
}

}           // namespace seq66

/*
 * gui_palette_qt5.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


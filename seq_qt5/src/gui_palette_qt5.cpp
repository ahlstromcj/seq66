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
 * \file          gui_palette_qt5.cpp
 *
 *  This module declares/defines the class for providing GTK/GDK colors.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-02-23
 * \updates       2021-04-29
 * \license       GNU GPLv2 or above
 *
 *  One possible idea would be a color configuration that would radically
 *  change drawing of the lines and pixmaps, opening up the way for night
 *  views and color schemes that match the desktop theme.
 *
 *  There are some predefined QColor objects: white, black, red, darkRed,
 *  green, darkGreen, blue, darkBlue, cyan, darkCyan, magenta, darkMagenta,
 *  yellow, darkYellow, gray, darkGray, lightGray, color0 and color1,
 *  accessible as members of the Qt namespace (ie. Qt::red).  Many of these
 *  colors can now be modified from a 'palette' file.
 *
 *  These uses are made of each color:
 *
 *  -   Black.  The background color of armed patterns.  The color of
 *      most lines in the user interface, including the main grid
 *      lines.
 *  -   White.  The default background color of just about everything
 *      drawn in the application.
 *  -   Grey.  The color of minor grid lines and the markers for the
 *      currently-selected scale.
 *  -   Dark grey.  The color of some grid lines, and the background
 *      of a queued pattern slot.
 *  -   Light grey.  The color of some grid lines.
 *  -   Red.  The optional color of progress bars.
 *  -   Orange.  The fill-in color for selected notes and events.
 *  -   Dark orange.  The color of selected event data lines and the
 *      color of the selection box for events to be pasted.
 *  -   Yellow.  The background of the pattern and name slots for empty
 *      patterns.  The text color for selected empty pattern slots.
 *  -   Green.  Not yet used.
 *  -   Blue.   Not yet used.
 *  -   Dark cyan.  The background color of muted patterns currently in
 *      edit, or the pattern that contains the original data for an
 *      imported SMF 0 song.  The text color of an unmuted pattern
 *      currently in edit.  These colors apply to the pattern editor and
 *      the song editor.  The color of the selected background pattern
 *      in the song editor.
 *  -   Line color. The generic line color, meant for expansion.
 *      Currently black.
 *  -   Progress color. The progress line color.  Black by default, but
 *      can be set to other colors.
 *  -   Background color.  The currently-in-use background color.  Can
 *      vary a lot when a pixmap is being redrawn.
 *  -   Foreground color.  The currently-in-use foreground color.  Can
 *      vary a lot when a pixmap is being redrawn.
 */

#include "cfg/settings.hpp"             /* seq66::rc() or seq66::usr()      */
#include "gui_palette_qt5.hpp"          /* seq66::gui_palette_qt5           */
#include "util/basic_macros.hpp"        /* seq66::file_error() function     */
#include "util/strfunctions.hpp"        /* seq66 string functions           */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Provide access to the internal, basic color palette.  We use a static
 *  function to access this item (plus a couple of things we don't need).
 *  Note that we have to associate a file-name later, if any.  A little
 *  clumsy; but we don't want to move this into a non-Qt-GUI module.
 */

gui_palette_qt5 &
global_palette ()
{
    static gui_palette_qt5 s_pallete;
    return s_pallete;
}

Color
get_color_fix (PaletteColor index)
{
    return global_palette().get_color_fix(index);
}

Color
get_pen_color (PaletteColor index)
{
    return global_palette().get_pen_color(index);
}

std::string
get_color_name (PaletteColor index)
{
    return global_palette().get_color_name(index);
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

bool
is_theme_color (const Color & c)
{
    return global_palette().is_theme_color(c);
}

/**
 *  Foreground paint uses the invertible color for black.
 */

Color
foreground_paint ()
{
    return global_palette().get_color(InvertibleColor::black);
}

/**
 *  Background paint uses the invertible color for white.
 */

Color
background_paint ()
{
    return global_palette().get_color(InvertibleColor::white);
}

Color
label_paint ()
{
    return global_palette().get_color(InvertibleColor::label);
}

Color
sel_paint ()
{
    return global_palette().get_color(InvertibleColor::selection);
}

Color
drum_paint ()
{
    return global_palette().get_color(InvertibleColor::drum);
}

Color
tempo_paint ()
{
    return global_palette().get_color(InvertibleColor::tempo);
}

Color
note_in_paint ()
{
    return global_palette().get_color(InvertibleColor::note_in);
}

Color
note_out_paint ()
{
    return global_palette().get_color(InvertibleColor::note_out);
}

Color
black_key_paint ()
{
    return global_palette().get_color(InvertibleColor::black_key);
}

Color
white_key_paint ()
{
    return global_palette().get_color(InvertibleColor::white_key);
}

Color
progress_paint ()
{
    return global_palette().get_color(InvertibleColor::progress);
}

Color
backseq_paint ()
{
    return global_palette().get_color(InvertibleColor::backseq);
}

Color
grey_paint ()
{
    return global_palette().get_color(InvertibleColor::grey);
}

/**
 *  Beat paint uses the grey_paint.  Invertible, but the same color?
 */

Color
beat_paint ()
{
    return global_palette().get_color(InvertibleColor::dk_grey);
}

Color
step_paint ()
{
    return global_palette().get_color(InvertibleColor::lt_grey);
}

Color
extra_paint ()
{
    return global_palette().get_color(InvertibleColor::extra);
}

Brush
gui_empty_brush ()
{
    return global_palette().get_brush(gui_palette_qt5::brush::empty);
}

Brush
gui_note_brush ()
{
    return global_palette().get_brush(gui_palette_qt5::brush::note);
}

Brush
gui_scale_brush ()
{
    return global_palette().get_brush(gui_palette_qt5::brush::scale);
}

Brush
gui_backseq_brush ()
{
    return global_palette().get_brush(gui_palette_qt5::brush::backseq);
}

/**
 *  Secret color to indicate to use the corresponding theme color. Used for a
 *  label-color as the default color.
 */

static Color m_theme;

/**
 *  Bright constant colors
 */

static Color m_black;
static Color m_red;
static Color m_green;
static Color m_yellow;
static Color m_blue;
static Color m_magenta;
static Color m_cyan;
static Color m_white;

/**
 *  Dark static colors
 */

static Color m_dk_black;
static Color m_dk_red;
static Color m_dk_green;
static Color m_dk_yellow;
static Color m_dk_blue;
static Color m_dk_magenta;
static Color m_dk_cyan;
static Color m_dk_white;

/**
 * Extended colors in the palette.
 */

static Color m_orange;
static Color m_pink;
static Color m_color_18;
static Color m_color_19;
static Color m_color_20;
static Color m_color_21;
static Color m_color_22;
static Color m_grey;

static Color m_dk_orange;
static Color m_dk_pink;
static Color m_color_26;
static Color m_color_27;
static Color m_color_28;
static Color m_color_29;
static Color m_lt_grey;
static Color m_dk_grey;

/**
 *  Principal constructor.  In the constructor one can only allocate colors;
 *  get_window() returns 0 because this window has not yet been realized.
 *  Also note that the possible color names that can be used are found in
 *  /usr/share/X11/rgb.txt.
 */

gui_palette_qt5::gui_palette_qt5 (const std::string & filename) :
    basesettings            (filename),
    m_palette               (),
    m_pen_palette           (),
    m_nrm_palette           (),
    m_inv_palette           (),
    m_statics_are_loaded    (false),
    m_is_inverse            (false),
    m_empty_brush           (),
    m_empty_brush_style     (Qt::NoBrush),
    m_note_brush            (),
    m_note_brush_style      (Qt::SolidPattern),
    m_scale_brush           (),
    m_scale_brush_style     (Qt::Dense3Pattern),
    m_backseq_brush         (),
    m_backseq_brush_style   (Qt::Dense2Pattern)
{
    load_static_colors(usr().inverse_colors());     /* this must come first */
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
    if (! m_statics_are_loaded)
    {
        m_statics_are_loaded = true;
        m_theme        = Color(0xAD, 0xBE, 0xEF, 0xDE);     /* #DEADBEEF */
        m_black        = Color("black");
        m_red          = Color("red");
        m_green        = Color("green");
        m_yellow       = Color("yellow");
        m_blue         = Color("blue");
        m_magenta      = Color("magenta");
        m_cyan         = Color("cyan");
        m_white        = Color("white");
        m_dk_black     = Color("dark slate grey");
        m_dk_red       = Color("dark red");
        m_dk_green     = Color("dark green");
        m_dk_yellow    = Color("dark yellow");
        m_dk_blue      = Color("dark blue");
        m_dk_magenta   = Color("dark magenta");
        m_dk_cyan      = Color("dark cyan");
        m_dk_white     = Color("grey");
        m_orange       = Color("orange");
        m_pink         = Color("pink");
        m_color_18     = Color("pale green");
        m_color_19     = Color("khaki");
        m_color_20     = Color("light blue");
        m_color_21     = Color("violet");
        m_color_22     = Color("turquoise");
        m_grey         = Color(128, 128, 128);      /* "grey"               */
        m_dk_orange    = Color("dark orange");
        m_dk_pink      = Color("deep pink");
        m_color_26     = Color("sea green");
        m_color_27     = Color("dark khaki");
        m_color_28     = Color("dark slate blue");
        m_color_29     = Color("dark violet");
        m_lt_grey      = Color(192, 192, 192);      /* "light slate grey"   */
        m_dk_grey      = Color( 72,  72,  72);      /* "dark slate grey"    */
    }
}

/**
 *  Static function to invert a given Color.
 */

Color
gui_palette_qt5::calculate_inverse (const Color & c)
{
    int r, g, b, a;
    c.getRgb(&r, &g, &b, &a);
    r = a - r;
    g = a - g;
    b = a - b;
    return Color(r, g, b, a);
}

/**
 *  Returns true if the corresponding theme color is to be used.
 */

bool
gui_palette_qt5::is_theme_color (const Color & c) const
{
    return c == m_theme;
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
    m_nrm_palette.add(InvertibleColor::label,       m_theme,    "Label");
    m_nrm_palette.add(InvertibleColor::selection,   m_orange,   "Selection");
    m_nrm_palette.add(InvertibleColor::drum,        m_red,      "Drum");
    m_nrm_palette.add(InvertibleColor::tempo,       m_yellow,   "Tempo");
    m_nrm_palette.add(InvertibleColor::note_in,     m_white,    "Note Fill");
    m_nrm_palette.add(InvertibleColor::note_out,    m_black,    "Note Border");
    m_nrm_palette.add(InvertibleColor::black_key,   m_black,    "Black Keys");
    m_nrm_palette.add(InvertibleColor::white_key,   m_white,    "White Keys");
    m_nrm_palette.add(InvertibleColor::progress,    m_red,      "Progress Bar");
    m_nrm_palette.add(InvertibleColor::backseq,     m_dk_cyan,  "Back Pattern");
    m_nrm_palette.add(InvertibleColor::grey,        m_grey,     "Medium Line");
    m_nrm_palette.add(InvertibleColor::dk_grey,     m_dk_grey,  "Beat Line");
    m_nrm_palette.add(InvertibleColor::lt_grey,     m_lt_grey,  "Step Line");
    m_nrm_palette.add(InvertibleColor::extra,       m_dk_red,   "Extra");

    m_inv_palette.clear();
    m_inv_palette.add(InvertibleColor::black,       m_white,    "Foreground");
    m_inv_palette.add(InvertibleColor::white,       m_dk_grey,  "Background");
    m_inv_palette.add(InvertibleColor::label,       m_white,    "Label");
    m_inv_palette.add(InvertibleColor::selection,   m_yellow,   "Selection");
    m_inv_palette.add(InvertibleColor::drum,        m_green,    "Drum");
    m_inv_palette.add(InvertibleColor::tempo,       m_magenta,  "Tempo");
    m_inv_palette.add(InvertibleColor::note_in,     m_black,    "Note Fill");
    m_inv_palette.add(InvertibleColor::note_out,    m_white,    "Note Border");
    m_inv_palette.add(InvertibleColor::black_key,   m_white,    "Black Keys");
    m_inv_palette.add(InvertibleColor::white_key,   m_black,    "White Keys");
    m_inv_palette.add(InvertibleColor::progress,    m_green,    "Progress Bar");
    m_inv_palette.add(InvertibleColor::backseq,     m_dk_cyan,  "Back Pattern");
    m_inv_palette.add(InvertibleColor::grey,        m_grey,     "Medium Line");
    m_inv_palette.add(InvertibleColor::dk_grey,     m_white,    "Beat Line");
    m_inv_palette.add(InvertibleColor::lt_grey,     m_lt_grey,  "Step Line");
    m_inv_palette.add(InvertibleColor::extra,       m_dk_green, "Extra");

    m_empty_brush.setColor(get_color(InvertibleColor::white));
    m_empty_brush.setStyle(m_empty_brush_style);
    m_note_brush.setColor(get_color(InvertibleColor::white));
    m_note_brush.setStyle(m_note_brush_style);
    m_scale_brush.setColor(get_color(InvertibleColor::lt_grey));
    m_scale_brush.setStyle(m_scale_brush_style);
    m_backseq_brush.setColor(get_color(InvertibleColor::backseq));
    m_backseq_brush.setStyle(m_backseq_brush_style);
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

Color
gui_palette_qt5::get_color_ex
(
    PaletteColor index,
    double h, double s, double v
) const
{
    Color c = m_palette.get_color(index);
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

Color
gui_palette_qt5::get_color_fix (PaletteColor index) const
{
    if (m_palette.no_color(index))
    {
        return m_palette.get_color(PaletteColor::none);
    }
    else
    {
        Color c = m_palette.get_color(index);
        if (c.value() != 255)
            c.setHsv(c.hue(), c.saturation() * 0.65, c.value());

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

Color
gui_palette_qt5::get_color_inverse (PaletteColor index) const
{
    if (index != PaletteColor::none)
    {
        Color c = m_palette.get_color(index);
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
        Color backc;
        Color textc;
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
        int number = string_to_int(stanza);     /* gets first column value  */
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

const Color &
gui_palette_qt5::get_color (InvertibleColor index) const
{
    const Color & result = is_inverse() ?
        m_inv_palette.get_color(index) :
        m_nrm_palette.get_color(index) ;

    return result;
}

/**
 *  Provides the names of the Qt::BrushStyle enumeration, with the word
 *  "Pattern" dropped off (NoBrush left as is).
 */

static const std::string s_brush_names [] =
{
    "nobrush",
    "solid",
    "dense1",
    "dense2",
    "dense3",
    "dense4",
    "dense5",
    "dense6",
    "dense7",
    "horizontal",
    "vertical",
    "cross",
    "bdiag",
    "fdiag",
    "diagcross",
    "lineargradient",
    "radialgradient",
    "conicalgradient"
};

BrushStyle
gui_palette_qt5::get_brush_style (const std::string & name) const
{
    if (name.empty())
    {
        return Qt::NoBrush;
    }
    else
    {
        BrushStyle result = Qt::TexturePattern;             /* illegal here */
        int maximum = static_cast<int>(Qt::ConicalGradientPattern) + 1;
        for (int counter = 0; counter < maximum; ++counter)
        {
            if (name == s_brush_names[counter])
            {
                result = static_cast<Qt::BrushStyle>(counter);
                break;
            }
        }
        return result;
    }
}

std::string
gui_palette_qt5::get_brush_name (BrushStyle b) const
{
    std::string result;
    int maximum = static_cast<int>(Qt::ConicalGradientPattern) + 1;
    int index = static_cast<int>(b);
    if (index >= 0 && index < maximum)
        result = s_brush_names[index];

    return result;
}

bool
gui_palette_qt5::set_brushes
(
    const std::string & emptybrush,
    const std::string & notebrush,
    const std::string & scalebrush,
    const std::string & backseqbrush
)
{
    BrushStyle temp = get_brush_style(emptybrush);
    bool result = temp != Qt::TexturePattern;   /* used as illegal value    */
    if (result)
    {
        if (temp != Qt::NoBrush)
        {
            m_empty_brush_style = temp;
            m_empty_brush.setStyle(temp);
        }
        temp = get_brush_style(notebrush);
        result = temp != Qt::TexturePattern;
        if (result)
        {
            if (temp != Qt::NoBrush)
            {
                m_note_brush_style = temp;
                m_note_brush.setStyle(temp);
            }
            temp = get_brush_style(scalebrush);
            result = temp != Qt::TexturePattern;
            if (result)
            {
                if (temp != Qt::NoBrush)
                {
                    m_scale_brush_style = temp;
                    m_scale_brush.setStyle(temp);
                }
                temp = get_brush_style(backseqbrush);
                result = temp != Qt::TexturePattern;
                if (result)
                {
                    if (temp != Qt::NoBrush)
                    {
                        m_backseq_brush_style = temp;
                        m_backseq_brush.setStyle(temp);
                    }
                }
            }
        }
    }
    return result;
}

bool
gui_palette_qt5::get_brush_names
(
    std::string & emptybrush,
    std::string & notebrush,
    std::string & scalebrush,
    std::string & backseqbrush
)
{
    bool result = true;
    std::string temp = get_brush_name(m_empty_brush_style);
    emptybrush = temp;
    if (temp.empty())
        result = false;

    temp = get_brush_name(m_note_brush_style);
    notebrush = temp;
    if (temp.empty())
        result = false;

    temp = get_brush_name(m_scale_brush_style);
    scalebrush = temp;
    if (temp.empty())
        result = false;

    temp = get_brush_name(m_backseq_brush_style);
    backseqbrush = temp;
    if (temp.empty())
        result = false;

    return result;
}

Brush &
gui_palette_qt5::get_brush (brush index)
{
    static Brush s_dummy;
    switch (index)
    {
        case brush::empty:      return m_empty_brush;       break;
        case brush::note:       return m_note_brush;        break;
        case brush::scale:      return m_scale_brush;       break;
        case brush::backseq:    return m_backseq_brush;     break;
        default:                return s_dummy;             break;
    }
}

}           // namespace seq66

/*
 * gui_palette_qt5.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


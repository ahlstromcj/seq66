#if ! defined SEQ66_GUI_PALETTE_QT5_HPP
#define SEQ66_GUI_PALETTE_QT5_HPP

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
 * \file          gui_palette_qt5.hpp
 *
 *  This module declares/defines the class for providing Qt 5 colors.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-02-23
 * \updates       2019-12-23
 * \license       GNU GPLv2 or above
 *
 *  This module defines some QColor objects.  We might consider replacing the
 *  color accessor names with names that reflect their usage [e.g. instead of
 *  using light_grey(), we could provide a scale_color() function instead,
 *  since light-grey is the color used to draw scales on the pattern editor.
 *  Note that the color names come from /usr/share/X11/rgb.txt as Qt requires.
 */

#include <QColor>

#include "cfg/basesettings.hpp"         /* seq66::basesettings class        */
#include "util/palette.hpp"             /* seq66::palette map class         */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Implements a stock palette of QColor elements.
 */

class gui_palette_qt5 : public basesettings
{

public:

    /**
     *  Provides a type for the color object.  These uses are made of
     *  each color:
     *
     *  -   Black.  The background color of armed patterns.  The color of
     *      most lines in the user interface, including the main grid
     *      lines.  The default color of progress lines and text.
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
     *      can be set to red.
     *  -   Background color.  The currently-in-use background color.  Can
     *      vary a lot when a pixmap is being redrawn.
     *  -   Foreground color.  The currently-in-use foreground color.  Can
     *      vary a lot when a pixmap is being redrawn.
     */

    using Color = QColor;

protected:

    /**
     *  Holds the color palette for drawing on slot backgrounds.
     */

    palette<Color> m_palette;

    /**
     *  Holds the color palette for drawing notes on slot backgrounds.  This
     *  is not quite an inverse palette, but consists of colors that show well
     *  on the background colors.
     */

    palette<Color> m_pen_palette;

    /**
     *  Holds the invertible colors using in drawing pattern labels, drum
     *  notes, tempo, and grid lines.  This holds the normal values.
     */

    palette<Color> m_nrm_palette;

    /**
     *  Holds the invertible colors using in drawing pattern labels, drum
     *  notes, tempo, and grid lines.  This holds the inverse values.
     */

    palette<Color> m_inv_palette;

    /**
     *
     */

    bool m_is_loaded;

    /**
     *  Flags the presence of the inverse color palette.
     */

    bool m_is_inverse;

private:                            /* use the accessor functions           */

    /*
     * Non-static member colors.
     */

    Color m_line_color;             /**< Provides the line color.           */
    Color m_progress_color;         /**< Provides the progress bar color.   */
    Color m_bg_color;               /**< The current background color.      */
    Color m_fg_color;               /**< The current foreground color.      */

public:

    gui_palette_qt5 (const std::string & filename = "");
    gui_palette_qt5 (const gui_palette_qt5 &) = default;
    gui_palette_qt5 & operator = (const gui_palette_qt5 &) = default;
    virtual ~gui_palette_qt5 ();

    static int palette_size ()
    {
        return palette_to_int(maximum);
    }

    static int invertible_size ()
    {
        return inv_palette_to_int(maximum);
    }

    void reset ()
    {
        reset_backgrounds();
        reset_pens();
        reset_invertibles();
    }

    void reset_backgrounds ();
    void reset_pens ();
    void reset_invertibles ();

    /**
     * \param index
     *      Provides the color index into the palette.
     *
     * \return
     *      Returns the corresponding color name from the palette.
     */

    std::string get_color_name (PaletteColor index) const
    {
        return m_palette.get_color_name(index);
    }

    std::string get_color_name_ex (PaletteColor index) const
    {
        return m_palette.get_color_name_ex(index);
    }

    std::string get_pen_color_name (PaletteColor index) const
    {
        return m_pen_palette.get_color_name(index);
    }

    const Color & get_color (PaletteColor index) const
    {
        return m_palette.get_color(index);
    }

    std::string get_color_name (InvertibleColor index) const
    {
        return m_nrm_palette.get_color_name(index);
    }

    std::string get_inv_color_name (InvertibleColor index) const
    {
        return m_inv_palette.get_color_name(index);
    }

    const Color & get_color (InvertibleColor index) const;

    /**
     * \param index
     *      Provides the color index into the palette.
     *
     * \return
     *      Returns the corresponding color from the pen palette.
     */

    const Color & get_pen_color (PaletteColor index) const
    {
        return m_pen_palette.get_color(index);
    }

    std::string make_color_stanza (int number, bool inverse = false) const;
    bool add_color_stanza (const std::string & stanza, bool inverse = false);

    Color get_color_ex
    (
        PaletteColor index, double h, double s = 0.65, double v = 1.0
    ) const;

    Color get_color_fix (PaletteColor index) const;
    Color get_color_inverse (PaletteColor index) const;

    void load_static_colors (bool inverse = true);
    Color calculate_inverse (const Color & c);

    /**
     *  Indicates if the inverse color palette is loaded.
     */

    bool is_inverse () const
    {
        return m_is_inverse;
    }

    /**
     *  A convenience function to hide some details of checking for
     *  sequence color codes.
     */

    bool no_color (int c) const
    {
        return m_palette.no_color(PaletteColor(c));
    }

    void clear ();
    void clear_invertible ();

private:

    bool add
    (
        int index,
        const Color & bg, const std::string & bgname,
        const Color & fg, const std::string & fgname
    );
    bool add_invertible
    (
        int index,
        const Color & bg, const std::string & bgname,
        const Color & fg, const std::string & fgname
    );

};          // class gui_palette_qt5

/*
 *  Free functions for color.
 */

extern gui_palette_qt5 & global_palette ();
extern gui_palette_qt5::Color get_color_fix (PaletteColor index);
extern gui_palette_qt5::Color get_pen_color (PaletteColor index);
extern gui_palette_qt5::Color background_paint ();
extern gui_palette_qt5::Color foreground_paint ();
extern gui_palette_qt5::Color label_paint ();
extern gui_palette_qt5::Color sel_paint ();
extern gui_palette_qt5::Color drum_paint ();
extern gui_palette_qt5::Color tempo_paint ();
extern gui_palette_qt5::Color black_key_paint ();
extern gui_palette_qt5::Color white_key_paint ();
extern gui_palette_qt5::Color beat_paint ();
extern gui_palette_qt5::Color step_paint ();
extern std::string get_color_name_ex (PaletteColor index);
extern bool no_color (int c);

}           // namespace seq66

#endif      // SEQ66_GUI_PALETTE_QT5_HPP

/*
 * gui_palette_qt5.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


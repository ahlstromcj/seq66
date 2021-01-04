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
 * \updates       2021-01-04
 * \license       GNU GPLv2 or above
 *
 *  This module defines some QColor objects.  We might consider replacing the
 *  color accessor names with names that reflect their usage [e.g. instead of
 *  using light_grey(), we could provide a scale_color() function instead,
 *  since light-grey is the color used to draw scales on the pattern editor.
 *  Note that the color names come from /usr/share/X11/rgb.txt as Qt requires.
 */

#include <QBrush>
#include <QColor>

#include "cfg/basesettings.hpp"         /* seq66::basesettings class        */
#include "util/palette.hpp"             /* seq66::palette map class         */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Provides a type for the color object for the GUI framework.
 */

using Color = QColor;

/**
 *  Provides a type for the brush object for the GUI framework.
 */

using Brush = QBrush;

/**
 *  Provides a map to brush styles.  The first values are NoBrush,
 *  SolidPattern, and DenseXPattern (X = 1 to 7), and these are the ones we
 *  are most interested in.  They are defined in the QtCore/qnamespace.h
 *  header file.  The maximum useful value is ConicalGradientPattern = 17.
 */

using BrushStyle = Qt::BrushStyle;

/**
 *  Implements a stock palette of QColor elements.
 */

class gui_palette_qt5 : public basesettings
{

public:

    enum class brush
    {
        empty,
        note,
        scale,
        backseq
    };

private:

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
     *  Indicates if we have loaded the static colors.
     */

    bool m_statics_are_loaded;

    /**
     *  Flags the presence of the inverse color palette.
     */

    bool m_is_inverse;

    /**
     *  Stock brushes to increase speed.
     */

    Brush m_empty_brush;
    BrushStyle m_empty_brush_style;
    Brush m_note_brush;
    BrushStyle m_note_brush_style;
    Brush m_scale_brush;
    BrushStyle m_scale_brush_style;
    Brush m_backseq_brush;
    BrushStyle m_backseq_brush_style;

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

    /*
     * Brush handling.
     */

    Brush & get_brush (brush index);
    BrushStyle get_brush_style (const std::string & name) const;
    std::string get_brush_name (BrushStyle b) const;
    bool set_brushes
    (
        const std::string & emptybrush,
        const std::string & notebrush,
        const std::string & scalebrush,
        const std::string & backseqbrush
    );
    bool get_brush_names
    (
        std::string & emptybrush,
        std::string & notebrush,
        std::string & scalebrush,
        std::string & backseqbrush
    );

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
extern Color get_color_fix (PaletteColor index);
extern Color get_pen_color (PaletteColor index);
extern Color background_paint ();
extern Color foreground_paint ();
extern Color label_paint ();
extern Color sel_paint ();
extern Color drum_paint ();
extern Color tempo_paint ();
extern Color note_in_paint ();
extern Color note_out_paint ();
extern Color black_key_paint ();
extern Color white_key_paint ();
extern Color progress_paint ();
extern Color backseq_paint ();
extern Color grey_paint ();
extern Color beat_paint ();
extern Color step_paint ();
extern Color extra_paint ();
extern std::string get_color_name_ex (PaletteColor index);
extern bool no_color (int c);
extern Brush gui_empty_brush ();
extern Brush gui_note_brush ();
extern Brush gui_scale_brush ();
extern Brush gui_backseq_brush ();

}           // namespace seq66

#endif      // SEQ66_GUI_PALETTE_QT5_HPP

/*
 * gui_palette_qt5.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


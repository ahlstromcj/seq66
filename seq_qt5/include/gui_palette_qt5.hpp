#if ! defined SEQ66_GUI_PALETTE_QT5_HPP
#define SEQ66_GUI_PALETTE_QT5_HPP

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
 * \file          gui_palette_qt5.hpp
 *
 *  This module declares/defines the class for providing Qt 5 colors.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-02-23
 * \updates       2025-04-30
 * \license       GNU GPLv2 or above
 *
 *  This module defines some QColor objects.  We might consider replacing the
 *  color accessor names with names that reflect their usage [e.g. instead of
 *  using light_grey(), we could provide a scale_color() function instead,
 *  since light-grey is the color used to draw scales on the pattern editor.
 *  Note that the color names come from /usr/share/X11/rgb.txt as Qt requires.
 */

#include <memory>                       /* std::unique_ptr<>                */
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
 *  Provides a type for the brush object for the GUI framework. These
 *  objects are held in std::unique_ptr<>.
 */

using Brush = QBrush;

/**
 *  Provides a map to brush styles.  The first values are NoBrush,
 *  SolidPattern, and DenseXPattern (X = 1 to 7), and these are the ones we
 *  are most interested in.  They are defined in the QtCore/qnamespace.h
 *  header file.  The maximum useful value is ConicalGradientPattern = 17.
 *  Note that QBrush is a plain enumeration declared in qnamespace.h.
 *  However, we create the brushes in the gui_palette_qt5 constructor.
 */

using BrushStyle = Qt::BrushStyle;

/**
 *  Provides a map to pen styles.  Note that QPenStyle is a plain enumeration
 *  declared in qnamespace.h.
 */

using PenStyle = Qt::PenStyle;

/**
 *  Implements a stock palette of QColor elements.
 */

class gui_palette_qt5 : public basesettings
{

public:

    enum class brush
    {
        empty,      /**< Brush for empty space, usually "no brush".         */
        note,       /**< Brush for drawing notes in pattern editor.         */
        scale,      /**< Brush for drawing lines denoting musical scale.    */
        backseq     /**< Brush for drawing lines denoting background seq.   */
    };

    enum class penstyle
    {
        empty,      /**< Qt::NoPen.                                         */
        solid,      /**< The default penstyle is Qt::SolidLine.                  */
        dash,       /**< Qt::DashLine.                                      */
        dot,        /**< Qt::DotLine.                                       */
        dashdot,    /**< Qt::DashDotLine.                                   */
        dashdotdot, /**< Qt::DashDotDotLine.                                */
        customdash  /**< Qt::CustomDashLine (not supported at this time).   */
    };

    enum class pen
    {
        measure,
        beat,
        fourth,
        step
    };

private:

    using BrushPtr = std::unique_ptr<Brush>;

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
     *  notes, tempo, grid lines, and various text items. This holds the
     *  normal values.
     */

    palette<Color> m_nrm_palette;

    /**
     *  Holds the invertible colors using in drawing pattern labels, drum
     *  notes, tempo, grid lines, and various text items. This holds the
     *  inverse values.
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
     *  Provides a hint that the palette (or matching theme) is overall
     *  "dark" for the user-interface elements, which are separate from
     *  the Qt theme.
     */

    bool m_dark_theme;

    /**
     *  Provides a hint that the backgrounds of grids, etc. are dark.
     */

    bool m_dark_ui;

    /**
     *  Stock brushes to increase speed. As of 2023-02-26, we use
     *  pointers to be able to reassign brushes property for Qt.
     */

    BrushPtr m_empty_brush;
    BrushStyle m_empty_brush_style;
    BrushPtr m_note_brush;                  /* for both notes and triggers  */
    BrushStyle m_note_brush_style;          /* ditto                        */
    BrushPtr m_scale_brush;
    BrushStyle m_scale_brush_style;
    BrushPtr m_backseq_brush;
    BrushStyle m_backseq_brush_style;

    /**
     *  A convenience to indicate that the linear gradient pattern brush
     *  is in used.
     */

    bool m_use_gradient_brush;

    /**
     *  Stock pen enumeration values.
     */

    PenStyle m_measure_pen_style;           /* style of each bar line       */
    PenStyle m_beat_pen_style;              /* style of each beat line      */
    PenStyle m_fourth_pen_style;              /* style of each 1/4 beat line  */
    PenStyle m_step_pen_style;              /* style of small step lines    */

public:

    gui_palette_qt5 (const std::string & filename = "");
    gui_palette_qt5 (const gui_palette_qt5 &) = delete;
    gui_palette_qt5 & operator = (const gui_palette_qt5 &) = delete;
    virtual ~gui_palette_qt5 ();

    static Color calculate_inverse (const Color & c);

    static int palette_size ()
    {
        return palette_to_int(PaletteColor::max);
    }

    static int invertible_size ()
    {
        return inv_palette_to_int(InvertibleColor::max);
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

    const Color & get_inverse_color (InvertibleColor index) const
    {
        return m_inv_palette.get_color(index);
    }

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

    Color invert (Color c, bool usealpha = false) const;
    Color get_color_fix (PaletteColor index) const;
    Color get_color_inverse (PaletteColor index) const;

#if defined SEQ66_PROVIDE_AUTO_COLOR_INVERSION
    void fill_inverse_colors ();
#endif

    void load_static_colors (bool inverse = true);
    bool is_theme_color (const Color & c) const;

    /**
     *  Indicates if the inverse color palette is loaded, and if the
     *  use considers the matching theme to be dark.
     */

    bool is_inverse () const
    {
        return m_is_inverse;
    }

    bool dark_theme () const
    {
        return m_dark_theme;
    }

    void dark_theme (bool flag)
    {
        m_dark_theme = flag;
    }

    bool dark_ui () const
    {
        return m_dark_ui;
    }

    void dark_ui (bool flag)
    {
        m_dark_ui = flag;
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

    bool use_gradient_brush () const
    {
        return m_use_gradient_brush;
    }

    /*
     * Pen style handling.
     */

    PenStyle pen_style (pen p);
    bool get_pen_names
    (
        std::string & measurepen,
        std::string & beatpen,
        std::string & fourpen,
        std::string & steppen
    );
    bool set_pens
    (
        const std::string & measurepen,
        const std::string & beatpen,
        const std::string & fourpen,
        const std::string & steppen
    );

private:

    /*
     * Brush handling.
     */

    const std::string & brush_name (int index) const;
    bool make_brush
    (
        BrushPtr & brush,
        BrushStyle & brushstyle,
        BrushStyle temp
    );
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

    /*
     * Pen style handling.
     */

    const std::string & pen_name (int index) const;
    PenStyle get_pen (penstyle p);
    penstyle get_pen_index (PenStyle ps);
    std::string get_penstyle_name (penstyle p);
    PenStyle get_pen_style (const std::string & penname);

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
extern Color bar_paint ();                      /* heavy lines, dk_grey     */
extern Color grey_paint ();                     /* medium lines, grey       */
extern Color step_paint ();                     /* light lines, dk_grey     */
extern Color beat_paint ();                     /* beat lines               */
extern Color near_paint ();
extern Color backtime_paint ();
extern Color backdata_paint ();
extern Color backevent_paint ();
extern Color backkeys_paint ();
extern Color backnames_paint ();
extern Color octave_paint ();
extern Color text_paint ();
extern Color text_time_paint ();
extern Color text_data_paint ();
extern Color note_event_paint ();
extern Color text_keys_paint ();
extern Color text_names_paint ();
extern Color text_slots_paint ();
extern Color scale_paint ();
extern Color extra_paint ();
extern std::string get_color_name (PaletteColor index);
extern std::string get_color_name_ex (PaletteColor index);
extern bool no_color (int c);
extern bool is_theme_color (const Color & c);
extern bool is_dark_ui ();
extern Brush gui_empty_brush ();
extern Brush gui_note_brush ();                 /* for notes and triggers   */
extern bool gui_use_gradient_brush ();          /* ditto                    */
extern Brush gui_scale_brush ();
extern Brush gui_backseq_brush ();
extern PenStyle gui_measure_pen_style ();
extern PenStyle gui_beat_pen_style ();
extern PenStyle gui_fourth_pen_style ();
extern PenStyle gui_step_pen_style ();

}           // namespace seq66

#endif      // SEQ66_GUI_PALETTE_QT5_HPP

/*
 * gui_palette_qt5.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


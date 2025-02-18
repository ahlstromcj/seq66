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
 * \file          palettefile.cpp
 *
 *  This module declares/defines the base class for managing the reading and
 *  writing of the 'palette' file.
 *
 * \library       seq66 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2020-12-21
 * \updates       2025-01-24
 * \license       GNU GPLv2 or above
 *
 */

#include <iomanip>                      /* std::setw()                      */
#include <iostream>                     /* std::cout                        */

#include "cfg/settings.hpp"             /* seq66::rc(), seq66::usr()        */
#include "gui_palette_qt5.hpp"          /* seq66::gui_palette_qt5           */
#include "palettefile.hpp"              /* seq66::palettefile class         */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Principal constructor.
 *
 * \param mapper
 *      Provides the palette reference to be acted upon.
 *
 * \param filename
 *      Provides the name of the palette file; this is usually a full path
 *      file-specification to the "mutes" file using this object.
 *
 * \param rcs
 *      The configfile currently requires an rcsetting object, but it is not
 *      yet used here.
 */

palettefile::palettefile
(
    gui_palette_qt5 & mapper,
    const std::string & filename,
    rcsettings & rcs
) :
    configfile      (filename, rcs, ".palette"),
    m_palettes      (mapper)
{
    // Empty body
}

/**
 *  Parse the ~/.config/seq66/qseq66.palette file-stream.
 *
 *  [comments] Header commentary is skipped during parsing.  However, we now
 *  try to read an optional comment block.  This block is part of the palette
 *  container, not part of the rcsettings object.
 *
 *  Also, to account for expansion of the ui-palette (invertible palette).
 */

bool
palettefile::parse_stream (std::ifstream & file)
{
    bool result = true;
    file.seekg(0, std::ios::beg);                   /* seek to start    */
    (void) parse_version(file);

    std::string s = parse_comments(file);
    if (! s.empty())
        mapper().comments_block().set(s);

    const std::string tag = "[hints]";
    bool ok = line_after(file, tag);
    if (ok)
    {
        bool flag = get_boolean(file, tag, "dark-theme");
        mapper().dark_theme(flag);
        if (flag)
            usr().dark_theme(true);

        flag = get_boolean(file, tag, "dark-ui");
        mapper().dark_ui(flag);
    }
    ok = line_after(file, "[palette]");
    if (ok)
    {
        int count = 0;                  /* limited to 32 palette entries    */
        m_palettes.clear();
        for (;;)
        {
            if (count > gui_palette_qt5::palette_size())
            {
                ok = false;
                break;
            }
            else
            {
                ok = m_palettes.add_color_stanza(line());
                if (ok)
                    ++count;
                else
                    break;
            }
            if (! next_data_line(file))
                break;
        }
        if (ok)
            ok = count == gui_palette_qt5::palette_size();
    }
    if (ok)
    {
        if (line_after(file, "[ui-palette]"))
        {
            int count = 0;              /* limited to 32 palette entries    */
            m_palettes.clear_invertible();
            for (;;)
            {
                if (count > gui_palette_qt5::invertible_size())
                {
                    ok = false;
                    break;
                }
                else
                {
                    ok = m_palettes.add_color_stanza(line(), true);
                    if (ok)
                        ++count;
                    else
                        break;
                }
                if (! next_data_line(file))
                    break;
            }
            if (ok)
                ok = count == gui_palette_qt5::invertible_size();
        }
    }
    if (ok)
    {
        const std::string tag = "[brushes]";
        std::string sempty = get_variable(file, tag, "empty");
        std::string snote = get_variable(file, tag, "note");
        std::string sscale = get_variable(file, tag, "scale");
        std::string sbackseq = get_variable(file, tag, "backseq");
        (void) m_palettes.set_brushes(sempty, snote, sscale, sbackseq);
    }
    if (! ok)
    {
        warn_message("Palettes reset to defaults");
        m_palettes.reset();
    }
    return result;
}

/**
 *  Get the number of sequence definitions provided in the [mute-group]
 *  section.  See the rcfile class for full information.
 *
 * \param p
 *      Provides the performance object to which all of these options apply.
 *
 * \return
 *      Returns true if the file was able to be opened for reading.
 *      Currently, there is no indication if the parsing actually succeeded.
 */

bool
palettefile::parse ()
{
    std::ifstream file(name(), std::ios::in | std::ios::ate);
    bool result = ! name().empty() && file.is_open();
    if (result)
    {
        file_message("Read palette", name());
        result = parse_stream(file);
    }
    else
    {
        std::string msg = "Read open fail";
        file_error(msg, name());
        msg += ": ";
        msg += name();
        append_error_message(msg);
        result = false;
    }
    return result;
}

/**
 *  Writes the [palettes] section to the given file stream.
 *
 * \param file
 *      Provides the output file stream to write to.
 *
 * \return
 *      Returns true if the write operations all succeeded.
 */

bool
palettefile::write_stream (std::ofstream & file)
{
    file
        << "# Seq66 0.99.17 (and above) palette configuration file\n"
        << "#\n"
        << "# " << name() << "\n"
        << "# Written on " << get_current_date_time() << "\n"
        << "#\n"
        << "# This file can be used to change the colors used by patterns\n"
        << "# and in some parts of the user-interface. It must be active and\n"
        << "# specified in the 'rc' file.\n"
        << "\n"
        << "# Note: colors of the time display are currently set in the\n"
        << "# 'usr' file.\n"
        << "\n"
        ;

    /*
     * [comments]
     */

    file <<
        "[Seq66]\n\n"
        "config-type = \"palette\"\n"
        "version = " << version() << "\n\n"
        "# The [comments] section can document this file. Lines starting with\n"
        "# '#', '[', or that have no characters end the comment.\n\n"
        "[comments]\n\n" << mapper().comments_block().text() << "\n"
        <<
        "# Set 'dark-theme' to true if the matching style-sheet is dark. This\n"
        "# also set the 'dark-theme' setting in the 'usr' file.\n"
        "# Set 'dark-ui' to true if the palette background elements are dark.\n"
        "\n"
        "[hints]\n"
        "\n"
        ;
    write_boolean(file, "dark-theme", mapper().dark_theme());
    write_boolean(file, "dark-ui", mapper().dark_ui());
    file <<
        "\n"
        "# [palette] affects the pattern colors selected (by number). First is\n"
        "# the color number, 0 to 31. Next is the name of the background color.\n"
        "# The first stanza [square brackets] are the background ARGB values.\n"
        "# The second provides the foreground color name and ARGB values. The\n"
        "# alpha values should be set to FF.\n"
        "\n"
        "[palette]\n"
        "\n"
        ;

    for (int number = 0; number < gui_palette_qt5::palette_size(); ++number)
    {
        std::string stanza = m_palettes.make_color_stanza(number);
        if (stanza.empty())
            break;
        else
            file << stanza << "\n";
    }
    file <<
        "\n"
        "# Similar to the [palette] section, but applies to the custom-drawn\n"
        "# piano rolls and the --inverse option. The values: color number (0\n"
        "# to 31); main color feature name; main color value; inverse color\n"
        "# feature name; and the --inverse color value.\n"
        "\n"
        "[ui-palette]\n"
        "\n"
        ;

    for (int number = 0; number < gui_palette_qt5::invertible_size(); ++number)
    {
        std::string stanza = m_palettes.make_color_stanza(number, true);
        if (stanza.empty())
            break;
        else
            file << stanza << "\n";
    }

    std::string sempty;
    std::string snote;
    std::string sscale;
    std::string sbackseq;
    file <<
        "\n"
        "# This section defines brush styles to use. The names are based on the\n"
        "# names in the Qt::BrushStyle enumeration. The names are:\n"
        "#\n"
        "#    nobrush, solid, dense1, dense2, dense3, dense4, dense5, dense6,\n"
        "#    dense7, horizontal, vertical, cross, bdiag, fdiag, diagcross,\n"
        "#    lineargradient, radialgradient, and conicalgradient.\n"
        "#\n"
        "# For 'empty', best to just use 'solid' (try others and see why).\n"
        "# For 'note', use only 'solid' or the default, 'lineargradient'. These\n"
        "# also apply to the progress box and triggers.\n"
        "\n"
        "[brushes]\n"
        "\n"
        ;
    if (m_palettes.get_brush_names(sempty, snote, sscale, sbackseq))
    {
        file
            << "empty = " << sempty << "\n"
            << "note = " << snote << "\n"
            << "scale = " << sscale << "\n"
            << "backseq = " << sbackseq << "\n"
            ;
    }
    write_seq66_footer(file);
    return true;
}

/**
 *  This options-writing function is just about as complex as the
 *  options-reading function.
 *
 * \return
 *      Returns true if the write operations all succeeded.
 */

bool
palettefile::write ()
{
    std::ofstream file(name(), std::ios::out | std::ios::trunc);
    bool result = ! name().empty() && file.is_open();
    if (result)
    {
        file_message("Write palette", name());
        result = write_stream(file);
        file.close();
    }
    else
        file_error("Write open fail", name());

    return result;
}

bool
open_palette
(
    gui_palette_qt5 & pal,
    const std::string & source
)
{
    bool result = ! source.empty();
    if (result)
    {
        palettefile palfile(pal, source, rc());     /* add msg? */
        result = palfile.parse();
        if (result)
        {
            // Anything worth doing?
        }
        else
        {
            std::string msg = "Open failed: ";
            msg += source;
            (void) error_message(msg);
        }
    }
    else
    {
        file_error("Palette file to open", "none");
    }
    return result;
}

/**
 *  This function saves the palettes to a file. It is primarily useful
 *  in getting any new additions/upgrades to the palettes. Older palette
 *  files can be updated using side-by-side editing in a text editor.
 *
 *  Now, consider this sequence:
 *
 *      -#  The programmer adds more palette entries to the InvertibleColors.
 *          This changes the number of entries from N0 to N1 > N0.
 *      -#  The application loads a pre-existing palette file, or none.
 *      -#  xxx
 *
 *  \param [inout] pal
 *      Provides the palette object.
 *
 *  \param destination
 *      Provides the directory to which the play-list file is to be saved.
 *
 * \return
 *      Returns true if the operation succeeded.
 */

bool
save_palette
(
    gui_palette_qt5 & pal,
    const std::string & destination
)
{
    bool result = ! destination.empty();
    if (result)
    {
        palettefile palfile(pal, destination, rc());
        palfile.name(destination);
        result = palfile.write();
        if (! result)
            file_error("Write failed", destination);
    }
    else
        file_error("Palette file", "none");

    return result;
}

/**
 *  This function reads the source palette file and then saves it to the new
 *  location.
 *
 *  \param [inout] pal
 *      Provides the palette object.
 *
 *  \param source
 *      Provides the input file name from which the palette will be filled.
 *
 *  \param destination
 *      Provides the directory to which the play-list file is to be saved.
 *
 * \return
 *      Returns true if the operation succeeded.
 */

bool
save_palette
(
    gui_palette_qt5 & pal,
    const std::string & source,
    const std::string & destination
)
{
    bool result = ! source.empty();
    if (result)
    {
        std::string msg = source + " --> " + destination;
        palettefile palfile(pal, source, rc());

        /*
         * TMI: file_message("Palette save", msg);
         */

        result = palfile.parse();
        if (result)
            result = save_palette(pal, destination);
        else
            file_error("Open failed", source);
    }
    else
        file_error("Palette file", "none");

    return result;
}

}           // namespace seq66

/*
 * palettefile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


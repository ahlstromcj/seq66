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
 * \file          mutegroupsfile.cpp
 *
 *  This module declares/defines the base class for managing the reading and
 *  writing of the mute-group sections of the "rc" file.
 *
 * \library       seq66 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-11-13
 * \updates       2019-09-24
 * \license       GNU GPLv2 or above
 *
 */

#include <iomanip>                      /* std::setw()                      */
#include <iostream>                     /* std::cout                        */

#include "cfg/mutegroupsfile.hpp"       /* seq66::mutegroupsfile class      */
#include "cfg/settings.hpp"             /* seq66::rc(), as rc_ref()         */
#include "play/mutegroups.hpp"          /* seq66::mutegroups, etc.          */
#include "util/calculations.hpp"        /* seq66::string_to_bool(), etc.    */
#include "util/strfunctions.hpp"        /* seq66::write_stanza_bits()       */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Principal constructor.
 *
 * \param filename
 *      Provides the name of the mute-groups file; this is usually a full path
 *      file-specification to the "mutes" file using this object.
 *
 * \param rcs
 *      The source/destination for the configuration information.
 *
 * \param allowinactive
 *      If true (the default is false), allow inactive (all 0's) mute-groups
 *      to be read and stored.
 */

mutegroupsfile::mutegroupsfile
(
    const std::string & filename,
    rcsettings & rcs,
    bool allowinactive
) :
    configfile              (filename, rcs),
    m_legacy_format         (true),                 // true only for now
    m_allow_inactive        (allowinactive),
    m_section_count         (mutegroup::ROWS_DEFAULT),
    m_mute_count            (mutegroup::COLS_DEFAULT)
{
    // Empty body
}

/**
 *  A rote destructor.
 */

mutegroupsfile::~mutegroupsfile ()
{
    // ~configfile() called automatically
}

/**
 *  Parse the ~/.config/seq66/seq66.rc file-stream or the
 *  ~/.config/seq66/seq66.mutes file-stream.
 *
 *  [mute-group]
 *
 *      The mute-group starts with a line that indicates up to 32 mute-groups are
 *      defined. A common value is 1024, which means there are 32 groups times 32
 *      keys.  But this value is currently thrown away.  This value is followed by
 *      32 lines of data, each contained 4 sets of 8 settings.  See the seq66-doc
 *      project on GitHub for a much more detailed description of this section.
 */

bool
mutegroupsfile::parse_stream (std::ifstream & file)
{
    bool result = true;
    file.seekg(0, std::ios::beg);                   /* seek to start    */

    /*
     * [comments] Header commentary is skipped during parsing.  However, we
     * now try to read an optional comment block.  This block is part of the
     * mutegroups container, not part of the rcsettings object.
     */

    std::string s = parse_comments(file);
    if (! s.empty())
    {
        rc_ref().mute_groups().comments_block().set(s);
#if defined SEQ66_PLATFORM_DEBUG
        if (rc().verbose())
            std::cout << s;
#endif
    }

    s = get_variable(file, "[mute-group-flags]", "save-mutes-to");
    if (! s.empty())
        rc_ref().mute_group_save(s);

    s = get_variable(file, "[mute-group-flags]", "mute-group-rows");
    if (! s.empty())
        rc_ref().mute_groups().rows(string_to_int(s));

    s = get_variable(file, "[mute-group-flags]", "mute-group-columns");
    if (! s.empty())
        rc_ref().mute_groups().columns(string_to_int(s));

    s = get_variable(file, "[mute-group-flags]", "groups-format");
    if (! s.empty())
    {
        bool usehex = (s == "hex");
        rc_ref().mute_groups().group_format_hex(usehex);
    }

    bool good = line_after(file, "[mute-groups]");
    rc_ref().mute_groups().clear();
    while (good)                        /* not at end of section?   */
    {
        if (! line().empty())           /* any value in section?    */
        {
            good = parse_mutes_stanza();
            if (good)
                good = next_data_line(file);

            if (! good)
                break;
        }
    }
    if (rc_ref().mute_groups().count() <= 1)    /* a sanity check   */
    {
        rc_ref().mute_groups().reset_defaults();
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
mutegroupsfile::parse ()
{
    bool result = true;
    std::ifstream file(name(), std::ios::in | std::ios::ate);
    if (file.is_open())
    {
        result = parse_stream(file);
    }
    else
    {
        errprintf
        (
            "mutegroups::parse(): error opening %s for reading",
            name().c_str()
        );
        result = false;
    }
    return result;
}

/**
 *  Writes the [mute-group] section to the given file stream.
 *
 * \param file
 *      Provides the output file stream to write to.
 *
 * \param separatefile
 *      If true, the [mute-group] section is being written to a separate
 *      file.  The default value is false.
 *
 * \return
 *      Returns true if the write operations all succeeded.
 */

bool
mutegroupsfile::write_stream (std::ofstream & file)
{
    file
        << "# Seq66 0.90.1 (and above) mute-groups configuration file\n"
        << "#\n"
        << "# " << name() << "\n"
        << "# Written on " << current_date_time() << "\n"
        << "#\n"
        << "# This file replaces the [mute-group] section, making it a little\n"
        << "# easier to manage multiple sets of mute groups.\n"
        << "\n"
        ;

    /*
     * [comments]
     */

    file <<
        "[Seq66]\n\n"
        "config-type = \"mutes\"\n"
        "version = 0\n"
        "\n"
        "# The [comments] section can document this file.  Lines starting\n"
        "# with '#' and '[' are ignored.  Blank lines are ignored.  Show a\n"
        "# blank line by adding a space character to the line.\n\n"
        "[comments]\n\n" << rc_ref().comments_block().text() << "\n"
        <<
        "# This file holds the mute-groups configuration for Seq66.\n"
        "# It follows the format of the 'rc' configuration file, but is\n"
        "# stored separately for convenience.  It is always stored in the\n"
        "# main configuration directory.  To use this file, replace the\n"
        "# [mute-group] section and its contents with a [mute-group-file]\n"
        "# tag, and simply add the basename (e.g. nanomutes.mutes) on a\n"
        "# separate line.\n"
        "#\n"
        "# save-mutes-to: 'both' writes the mutes value to both the mutes\n"
        "# and the MIDI file; 'midi' writes only to the MIDI file; and\n"
        "# and 'mutes' only to the mutesfile.\n"
        "# \n"
        "# mute-group-rows and mute-group-columns: Specifies the size of the\n"
        "# grid.  For now, keep these values at 4 and 8.\n"
        "# \n"
        "# groups-format: 'bin' means to write the mutes as 0 or 1; 'hex' means\n"
        "# to write them as hexadecimal numbers (e.g. 0xff), which will be\n"
        "# useful with larger set sizes.\n"
        ;

    bool result = write_mute_groups(file);
    if (result)
    {
        file
            << "\n# End of " << name() << "\n#\n"
            << "# vim: sw=4 ts=4 wm=4 et ft=dosini\n"
            ;
    }
    else
        file_error("Write fail", name());

    return result;
}

/**
 *  This options-writing function is just about as complex as the
 *  options-reading function.
 *
 * \return
 *      Returns true if the write operations all succeeded.
 */

bool
mutegroupsfile::write ()
{
    std::ofstream file(name(), std::ios::out | std::ios::trunc);
    bool result = file.is_open();
    if (result)
    {
        pathprint("Writing 'mutes' file", name());
        result = write_stream(file);
        file.close();
    }
    else
    {
        file_error("Write open fail", name());
    }
    return result;
}

/**
 *  The default long format for writing mute groups.
 */

static const char * const sg_scanf_fmt_1 =
    "%d [ %d %d %d %d %d %d %d %d ] [ %d %d %d %d %d %d %d %d ] "
      " [ %d %d %d %d %d %d %d %d ] [ %d %d %d %d %d %d %d %d ]";

/**
 *  Writes the [mute-group] section to the given file stream.  This can also
 *  be called by the rcfile object to just dump the data into that file.
 *
 * \param file
 *      Provides the output file stream to write to.
 *
 * \return
 *      Returns true if the write operations all succeeded.
 */

bool
mutegroupsfile::write_mute_groups (std::ofstream & file)
{
    bool result = file.is_open();
    if (result)
    {
        bool usehex = rc_ref().mute_groups().group_format_hex();
        std::string save = rc_ref().mute_group_save_label();
        std::string gf = usehex ? "hex" : "bin" ;
        int rows = rc_ref().mute_groups().rows();
        int columns = rc_ref().mute_groups().columns();
        file << "\n[mute-group-flags]\n\n"
            << "save-mutes-to = " << save << "\n"
            << "mute-group-rows = " << rows << "\n"
            << "mute-group-columns = " << columns << "\n"
            << "groups-format = " << gf << "\n"
            ;

        file << "\n[mute-groups]\n\n" <<
        "# All mute-group values are saved in this 'mutes' file, even if they\n"
        "# all are zero; but if all are zero, they will be stripped out from\n"
        "# the MIDI file by the strip-empty-mutes functionality. If a hex number\n"
        "# is used, then each number represents a bit mask, rather than a single\n"
        "# bit.\n"
        "\n"
            ;

        for (const auto & stz : rc_ref().mute_groups().list())
        {
            int gmute = stz.first;
            const mutegroup & m = stz.second;
            std::string stanza = write_stanza_bits(m.get(), usehex);
            if (! stanza.empty())
            {
                file << std::setw(2) << gmute << " " << stanza << std::endl;
            }
            else
            {
                result = false;
                break;
            }
        }
    }
    return result;
}

/**
 *  We need the format of a mute-group stanza to be more flexible.
 *  Should it match the set size?
 *
 *  We want to support the misleading format "[ b b b... ] [ b b b...] ...",
 *  as well as a new format "[ 0xbb ] [ 0xbb ] ...".
 *
 *  This function handles the current line of data from the mutes file.
 */

bool
mutegroupsfile::parse_mutes_stanza ()
{
    int group = string_to_int(line());
    bool result = group >= 0 && group < 512;            /* a sanity check   */
    if (result)
    {
        midibooleans groupmutes;
        result = parse_stanza_bits(groupmutes, line());
        if (result)
            result = rc_ref().mute_groups().load(group, groupmutes);
    }
    return result;
}

}           // namespace seq66

/*
 * mutegroupsfile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


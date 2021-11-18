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
 * \file          mutegroupsfile.cpp
 *
 *  This module declares/defines the base class for managing the reading and
 *  writing of the mute-group sections of the "rc" file.
 *
 * \library       seq66 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2018-11-13
 * \updates       2021-11-04
 * \license       GNU GPLv2 or above
 *
 */

#include <iomanip>                      /* std::setw()                      */
#include <iostream>                     /* std::cout                        */

#include "cfg/mutegroupsfile.hpp"       /* seq66::mutegroupsfile class      */
#include "cfg/settings.hpp"             /* seq66::rc(), as rc_ref()         */
#include "play/mutegroups.hpp"          /* seq66::mutegroups, etc.          */
#include "util/calculations.hpp"        /* seq66::current_data_time(), etc. */
#include "util/strfunctions.hpp"        /* seq66::write_stanza_bits()       */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Provides an internal-only mutegroups object that can hold the mute-groups
 *  defined in the file to be read/written for safety of the user's data, when
 *  the settings specify storing the run-time mute-groups in the MIDI file.
 */

static mutegroups &
internal_mutegroups ()
{
    static mutegroups s_internal_mutes;
    return s_internal_mutes;
}

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
    m_section_count         (mutegroup::c_default_rows),
    m_mute_count            (mutegroup::c_default_columns)
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
 *      The mute-group starts with a line that indicates up to 32 mute-groups
 *      are defined. A common value is 1024, which means there are 32 groups
 *      times 32 keys.  But this value is currently thrown away.  This value
 *      is followed by 32 lines of data, each contained 4 sets of 8 settings.
 *      See the seq66-doc project on GitHub for a much more detailed
 *      description of this section.
 */

bool
mutegroupsfile::parse_stream (std::ifstream & file)
{
    bool result = true;
    file.seekg(0, std::ios::beg);                   /* seek to start    */
    (void) parse_version(file);

    std::string s = parse_comments(file);
    mutegroups & mutes = rc_ref().mute_groups();
    if (! s.empty())
        mutes.comments_block().set(s);

    std::string tag = "[mute-group-flags]";
    s = get_variable(file, tag, "load-mute-groups");

    bool update_needed = s.empty();
    if (update_needed)
    {
        mutes.toggle_active_only(false);
        mutes.group_save("mutes");
        mutes.group_load("none");
    }
    else
    {
        if (s == "true")
            s = "mutes";
        else if (s == "false")
            s = "midi";

        if (! s.empty())
            mutes.group_load(s);

        s = get_variable(file, tag, "save-mutes-to");
        mutes.group_save(s);

        bool flag = get_boolean(file, tag, "strip-empty");
        mutes.strip_empty(flag);

        int value = get_integer(file, tag, "mute-group-rows");
        mutes.rows(value);
        value = get_integer(file, tag, "mute-group-columns");
        mutes.columns(value);
        value = get_integer(file, tag, "mute-group-selected");
        mutes.group_selected(value);
        s = get_variable(file, tag, "groups-format");
        if (! s.empty())
        {
            bool usehex = (s == "hex");
            mutes.group_format_hex(usehex);     /* otherwise it is binary   */
        }
        flag = get_boolean(file, tag, "toggle-active-only");
        mutes.toggle_active_only(flag);
    }

    bool load = mutes.group_load_from_mutes();
    mutegroups & mutestorage = load ? mutes : internal_mutegroups() ;
    bool good = line_after(file, "[mute-groups]");
    if (! load)
        internal_mutegroups() = mutes;

    mutestorage.clear();
    if (good)
    {
        bool ok = true;
        while (ok)                              /* not at end of section?   */
        {
            if (! line().empty())               /* any value in section?    */
            {
                ok = parse_mutes_stanza(mutestorage);
                if (ok)
                {
                    ok = next_data_line(file);
                }
                else
                {
                    file_error("Add mute stanza", line());
                    good = false;
                    break;
                }
            }
        }
    }
    if (good)
    {
        if (mutestorage.count() <= 1)           /* merely a sanity check    */
            good = false;
    }

    /*
     * Whether we loaded to internal storage or not, we have to make the
     * visible mutegroups reflect the actual situation.
     */

    if (good)
    {
        mutestorage.loaded_from_mutes(load);    /* loaded to non-internal?  */
#if defined SEQ66_PLATFORM_DEBUG
        if (rc().investigate())
            mutestorage.show("read from 'mutes'");
#endif
    }
    else
    {
        mutestorage.reset_defaults();
        mutestorage.loaded_from_mutes(false);
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
    std::ifstream file(name(), std::ios::in | std::ios::ate);
    bool result = file.is_open();
    if (result)
    {
        result = parse_stream(file);
    }
    else
    {
        file_error("Mutes open failed", name());
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
 * \return
 *      Returns true if the write operations all succeeded.
 */

bool
mutegroupsfile::write_stream (std::ofstream & file)
{
    write_date(file, "mute-groups");
    file <<
           "# Used in the [mute-group-file] section of the 'rc' file, making it\n"
           "# easier to manage multiple sets of mute groups. To use this file,\n"
           "# specify it in [mute-group-file] file and set 'active = true'.\n"
        ;
    write_seq66_header(file, "mutes", version());

    std::string c = rc_ref().mute_groups().comments_block().text();
    write_comment(file, c);
    file <<
        "\n"
        "# load-mute-groups: set to 'none', or 'mutes' to load from the 'mutes'\n"
        "# file, 'midi' to load from the song, or 'both' to try to\n"
        "# to read from the 'mutes' first, then the 'midi' file.\n"
        "#\n"
        "# save-mutes-to: 'both' writes the mutes to the 'mutes' and MIDI file;\n"
        "# 'midi' writes only to the MIDI file; and the 'mutes' only to the\n"
        "# 'mutes' file.\n"
        "#\n"
        "# strip-empty: If true, all-zero mute-groups are not written to the\n"
        "# MIDI file.\n"
        "#\n"
        "# mute-group-rows and mute-group-columns: Specifies the size of the\n"
        "# grid.  Keep these values at 4 and 8; mute-group-count is only for\n"
        "# sanity-checking.\n"
        "#\n"
        "# groups-format: 'binary' means write mutes as 0 or 1; 'hex' means\n"
        "# write them as hexadecimal numbers (e.g. 0xff), useful for larger set\n"
        "# sizes.\n"
        "#\n"
        "# mute-group-selected: if 0 to 31, and mutes are available either from\n"
        "# this file or from the MIDI file, then this mute-group is applied at\n"
        "# startup; useful in restoring a session. Set to -1 to disable.\n"
        "#\n"
        "# toggle-active-only: when a mute-group is toggled off, all patterns,\n"
        "# even those outside the mute-group, are muted.  If this flag is set\n"
        "# to true, only patterns in the mute-group are muted. Any patterns\n"
        "# unmuted directly by the user remain unmuted.\n"
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
        file_message("Writing mutes", name());
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
        const mutegroups & mutes = rc_ref().mute_groups();
        bool usehex = mutes.group_format_hex();
        std::string gf = usehex ? "hex" : "binary" ;
        file << "\n[mute-group-flags]\n\n";
        write_string(file, "load-mute-groups", mutes.group_load_label());
        write_string(file, "save-mutes-to", mutes.group_save_label());
        write_boolean(file, "strip-empty", mutes.strip_empty());
        write_integer(file, "mute-group-rows", mutes.rows());
        write_integer(file, "mute-group-columns", mutes.columns());
        write_integer(file, "mute-group-count", mutes.count());
        write_integer(file, "mute-group-selected", mutes.group_selected());
        write_string(file, "groups-format", gf);
        write_boolean(file, "toggle-active-only", mutes.toggle_active_only());
        file << "\n[mute-groups]\n\n" <<
        "# Mute-group values are saved in the 'mutes' file, even if all zeroes.\n"
        "# They can be stripped out of the MIDI file by 'strip-empty-mutes'.\n"
        "# A hex number indicates each number is a bit-mask, not a single bit.\n"
        "# An optional quoted group name can be placed at the end of the line.\n"
        "\n"
        ;

        bool load = mutes.group_load_from_mutes();
        const mutegroups & mutestorage = load ? mutes : internal_mutegroups() ;
        if (mutestorage.empty())
        {
            if (mutes.group_format_hex())
            {
                for (int m = 0; m < mutegroups::Size(); ++m)
                {
                    file << std::setw(2) << m << " [ 0x00 ] " << std::endl;
                }
            }
            else
            {
                for (int m = 0; m < mutegroups::Size(); ++m)
                {
                    file << std::setw(2) << m << " " <<
                         "[ 0 0 0 0 0 0 0 0 ] [ 0 0 0 0 0 0 0 0 ] "
                         "[ 0 0 0 0 0 0 0 0 ] [ 0 0 0 0 0 0 0 0 ]"
                        << std::endl
                        ;
                }
            }
        }
        else
        {
            /*
             * TODO: Consider what to do if the user does not want to save
             * mutes to this file, but it does have mutes in it already.
             * Should we have a static mutegroups object that merely holds
             * the current mute-group data for saving later?
             */

            for (const auto & stz : mutestorage.list())
            {
                int gmute = stz.first;
                const mutegroup & m = stz.second;
                std::string stanza = write_stanza_bits(m.get(), usehex);
                if (! stanza.empty())
                {
                    std::string groupname = m.name();
                    file << std::setw(2) << gmute << " " << stanza;
                    if (! groupname.empty())
                        file << " \"" << groupname << "\"";

                    file << std::endl;
                }
                else
                {
                    result = false;
                    break;
                }
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
mutegroupsfile::parse_mutes_stanza (mutegroups & mutes)
{
    int group = string_to_int(line());
    bool result = group >= 0 && group < c_max_groups;   /* a sanity check   */
    if (result)
    {
        midibooleans groupmutes;
        result = parse_stanza_bits(groupmutes, line());
        if (result)
            result = mutes.load(group, groupmutes);

        if (result)
        {
            std::string quoted = next_quoted_string(line());    /* tricky */
            if (! quoted.empty())
                mutes.group_name(group, quoted);
        }
    }
    return result;
}

bool
open_mutegroups (const std::string & source)
{
    bool result = ! source.empty();
    if (result)
    {
        mutegroupsfile mgf(source, rc());
        result = mgf.parse();
    }
    return result;
}

/**
 *  This is tricky, as mutegroupsfile always references the
 *  rc().mute_groups() object when reading and writing.
 *
 *      const mutegroups & mg = rc().mute_groups();
 */

bool
save_mutegroups (const std::string & destination)
{
    bool result = ! destination.empty();
    if (result)
    {
        mutegroupsfile mgf(destination, rc());
        file_message("Mute-groups save", destination);
        result = mgf.write();               // mg.file_name(destination);
        if (! result)
        {
            file_error("Mute-groups write failed", destination);
        }
    }
    else
    {
        file_error("Mute-groups file to save", "none");
    }
    return result;
}

}           // namespace seq66

/*
 * mutegroupsfile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


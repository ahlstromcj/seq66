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
 * \updates       2021-06-04
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
    (void) parse_version(file);

    std::string s = parse_comments(file);
    mutegroups & mutes = rc_ref().mute_groups();
    if (! s.empty())
        mutes.comments_block().set(s);

    s = get_variable(file, "[mute-group-flags]", "load-mute-groups");

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

        s = get_variable(file, "[mute-group-flags]", "save-mutes-to");
        if (! s.empty())
            mutes.group_save(s);

        s = get_variable(file, "[mute-group-flags]", "mute-group-rows");
        if (! s.empty())
            mutes.rows(string_to_int(s));

        s = get_variable(file, "[mute-group-flags]", "mute-group-columns");
        if (! s.empty())
            mutes.columns(string_to_int(s));

        s = get_variable(file, "[mute-group-flags]", "mute-group-selected");
        if (! s.empty())
            mutes.group_selected(string_to_int(s));

        s = get_variable(file, "[mute-group-flags]", "groups-format");
        if (! s.empty())
        {
            bool usehex = (s == "hex");
            mutes.group_format_hex(usehex);     /* otherwise it is binary   */
        }

        bool flag = get_boolean
        (
            file, "[mute-group-flags]", "toggle-active-only"
        );
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
        mutes.loaded_from_mutes(load);          /* loaded to non-internal?  */
    }
    else
    {
        mutes.reset_defaults();
        mutes.loaded_from_mutes(false);
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
           "# This file replaces the [mute-group] section of the 'rc' file,\n"
           "# making it easier to manage multiple sets of mute groups.\n"
           "\n"
        ;

    file <<
        "[Seq66]\n\n"
        "config-type = \"mutes\"\n"
        "version = " << version() << "\n\n"
        "# The [comments] section documents this file.  Lines starting with\n"
        "# '#', '[', or that have no characters end the comment.\n\n"
        "[comments]\n\n" << rc_ref().comments_block().text() << "\n"
        <<
        "# The 'mutes' file holds the global mute-groups configuration. It is\n"
        "# is stored separately for flexibility, in the HOME configuration\n"
        "# directory.  To use this file, specify it in the [mute-group-file]\n"
        "# tag in the 'rc' file and set 'active = true'.\n"
        "#\n"
        "# load-mute-groups: set to 'none', or 'mutes' to load only from the\n"
        "# 'mutes' file, 'midi' to load from the song, or 'both' to try to\n"
        "# to read from the 'mutes' first, then the 'midi' if empty.\n"
        "#\n"
        "# save-mutes-to: 'both' writes the mutes values to both the mutes and\n"
        "# the MIDI file; 'midi' writes only to the MIDI file; and the 'mutes'\n"
        "# only to the mutes file.\n"
        "#\n"
        "# mute-group-rows and mute-group-columns: Specifies the size of the\n"
        "# grid.  For now, keep these values at 4 and 8.\n"
        "#\n"
        "# groups-format: 'binary' means write mutes as 0 or 1; 'hex' means to\n"
        "# write them as hexadecimal numbers (e.g. 0xff), which is useful for\n"
        "# larger set sizes.\n"
        "#\n"
        "# mute-group-selected: if 0 to 31, and mutes are available either from\n"
        "# this file or from the MIDI file, then the mute-group is applied at\n"
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
        std::string loadstr = mutes.group_load_label();
        std::string savestr = mutes.group_save_label();
        std::string gf = usehex ? "hex" : "binary" ;
        std::string togonly = bool_to_string(mutes.toggle_active_only());
        int rows = mutes.rows();
        int columns = mutes.columns();
        int selected = mutes.group_selected();
        file << "\n[mute-group-flags]\n\n"
            << "load-mute-groups = " << loadstr << "\n"
            << "save-mutes-to = " << savestr << "\n"
            << "mute-group-rows = " << rows << "\n"
            << "mute-group-columns = " << columns << "\n"
            << "mute-group-selected = " << selected << "\n"
            << "groups-format = " << gf << "\n"
            << "toggle-active-only = " << togonly << "\n"
            ;

        file << "\n[mute-groups]\n\n" <<
        "# All mute-group values are saved in this 'mutes' file, even if all\n"
        "# zero; if all are zero, they can be stripped out of the MIDI file by\n"
        "# by 'strip-empty-mutes'. If a hex number is found, each number is\n"
        "# a bit mask, rather than a single bit.\n"
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
                    file << std::setw(2) << m << " " << "[ 0x00 ] "
                        << std::endl
                        ;
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
                    file << std::setw(2) << gmute << " "
                        << stanza << std::endl
                        ;
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


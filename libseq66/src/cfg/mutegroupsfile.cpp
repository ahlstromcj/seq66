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
 * \updates       2023-12-06
 * \license       GNU GPLv2 or above
 *
 */

#include <fstream>                      /* std::ofstream and ifstream       */
#include <iomanip>                      /* std::setw()                      */
#include <iostream>                     /* std::cout                        */

#include "cfg/mutegroupsfile.hpp"       /* seq66::mutegroupsfile class      */
#include "cfg/settings.hpp"             /* seq66::rc(), as rc_ref()         */
#include "play/mutegroups.hpp"          /* seq66::mutegroups, etc.          */
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
 *      file-specification to the "mutes" file using this object. This item
 *      can be specified in the 'rc' file.
 *
 * \param rcs
 *      The source/destination for the configuration information.
 */

mutegroupsfile::mutegroupsfile
(
    const std::string & filename,
    mutegroups & mutes
) :
    configfile  (filename, rc(), ".mutes"),
    m_mute_groups   (mutes)
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
    if (! s.empty())
        mutes().comments_block().set(s);

    std::string tag = "[mute-group-flags]";
    s = get_variable(file, tag, "load-mute-groups");

    bool update_needed = s.empty();
    if (update_needed)
    {
        mutes().toggle_active_only(false);
        mutes().group_save("mutes");
        mutes().group_load("none");
    }
    else
    {
        if (s == "true")
            s = "mutes";
        else if (s == "false")
            s = "midi";

        if (! s.empty())
            mutes().group_load(s);

        s = get_variable(file, tag, "save-mutes-to");
        mutes().group_save(s);

        bool flag = get_boolean(file, tag, "strip-empty");
        mutes().strip_empty(flag);

#if defined USE_MUTE_GROUP_ROWS_COLUMNS
        /*
         * For issue #87, we see that the mute-group size (number of patterns
         * that can be set by a mute-group) must match the mainwnd_rows() and
         * mainwnd_columns() setting in usrsettings.  These are now passed to
         * the performer, who then creates the mutegroups object with the
         * correct set size. So we disable these settings here, and mark the
         * 'mutes' file for saving if found.
         */

        int value = get_integer(file, tag, "mute-group-rows");
        mutes().rows(value);
        value = get_integer(file, tag, "mute-group-columns");
        mutes().columns(value);
        value = get_integer(file, tag, "mute-group-selected");
#else
        s = get_variable(file, tag, "mute-group-rows");
        if (! s.empty())
            rc_ref().auto_mutes_save(true);

        int value = get_integer(file, tag, "mute-group-selected");
#endif

        mutes().group_selected(value);
        s = get_variable(file, tag, "groups-format");
        if (! s.empty())
        {
            bool usehex = (s == "hex");
            mutes().group_format_hex(usehex);   /* otherwise it is binary   */
        }
        flag = get_boolean(file, tag, "toggle-active-only");
        mutes().toggle_active_only(flag);
    }

    bool load = mutes().group_load_from_mutes();
    mutegroups & mutestorage = load ? mutes() : internal_mutegroups() ;
    bool good = line_after(file, "[mute-groups]");
    if (! load)
        internal_mutegroups() = mutes();

    (void) mutestorage.reset_defaults();
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
    }
    else
    {
        (void) mutestorage.reset_defaults();
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
"# Used in the [mute-group-file] section of the 'rc' file, making it easier to\n"
"# multiple mute groups. To use this file, specify it in [mute-group-file] file\n"
"# and set 'active = true'.\n"
        ;
    write_seq66_header(file, "mutes", version());

    std::string c = mutes().comments_block().text();
    write_comment(file, c);
    file <<
"\n"
"# load-mute-groups: Set to 'none' or 'mutes' to load from the 'mutes' file,\n"
"# 'midi' to load from the song, or 'both' to try to read from 'mutes' first,\n"
"# then the 'midi' file.\n"
"#\n"
"# save-mutes-to: 'both' writes mutes to the 'mutes' and MIDI file; 'midi'\n"
"# writes only to the MIDI file; and the mutes only to the 'mutes' file.\n"
"#\n"
"# strip-empty: If true, all-zero mute-groups are not written to the MIDI file.\n"
"#\n"
#if defined USE_MUTE_GROUP_ROWS_COLUMNS
"# mute-group-rows and mute-group-columns: Specifies the size of the mute-group\n"
"# grid, not the number of mutes in a group (which matches the screenset size).\n"
"# Keep these values at 4x8 and 32; mute-group-count is for sanity-checking.\n"
"#\n"
#endif
"# groups-format: 'binary' means write mutes as 0/1; 'hex' means write them as\n"
"# hexadecimal numbers (e.g. 0xff), useful for larger set sizes.\n"
"#\n"
"# mute-group-selected: If 0 to 31, and mutes are available from this file\n"
"# or from the MIDI file, then this mute-group is applied at startup; useful in\n"
"# restoring a session. Set to -1 to disable.\n"
"#\n"
"# toggle-active-only: When a group is toggled off, all patterns, even those\n"
"# outside the mute-group, are muted.  With this flag, only patterns in the\n"
"# mute-group are muted. Patterns unmuted directly by the user remain unmuted.\n"
        ;

    bool result = write_mute_groups(file);
    if (result)
        write_seq66_footer(file);

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
        file_message("Write mutes", name());
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
 *  The default long format for writing mute groups. No longer needed.
 *
 *      static const char * const sg_scanf_fmt_1 =
 *        "%d [ %d %d %d %d %d %d %d %d ] [ %d %d %d %d %d %d %d %d ] "
 *        " [ %d %d %d %d %d %d %d %d ] [ %d %d %d %d %d %d %d %d ]" ;
 */

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
        bool usehex = mutes().group_format_hex();
        std::string gf = usehex ? "hex" : "binary" ;
        file << "\n[mute-group-flags]\n\n";
        write_string(file, "load-mute-groups", mutes().group_load_label());
        write_string(file, "save-mutes-to", mutes().group_save_label());
        write_boolean(file, "strip-empty", mutes().strip_empty());
#if defined USE_MUTE_GROUP_ROWS_COLUMNS
        write_integer(file, "mute-group-rows", mutes().rows());
        write_integer(file, "mute-group-columns", mutes().columns());
        write_integer(file, "mute-group-count", mutes().count());
#endif
        write_integer(file, "mute-group-selected", mutes().group_selected());
        write_string(file, "groups-format", gf);
        write_boolean(file, "toggle-active-only", mutes().toggle_active_only());
        file << "\n[mute-groups]\n\n" <<
"# We save mute-group values in the 'mutes' file, even if all zeroes. They can\n"
"# be stripped out of the MIDI file by 'strip-empty-mutes'. Hex values indicate\n"
"# a bit-mask, not a single bit. A quoted group name can be placed at the end\n"
"# of the line.\n\n"
        ;

        bool load = mutes().group_load_from_mutes();
        const mutegroups & mutestorage = load ? mutes() : internal_mutegroups() ;
        if (mutestorage.empty())
        {
            if (mutes().group_format_hex())
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
                std::string stanza = write_stanza_bits
                (
                    m.get(), m.columns(), usehex
                );
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
        {
            if (groupmutes.size() != size_t(mutes.group_count()))
            {
                /*
                 * Here, the user changed the dimensions of a set, but
                 * we're reading an old mute-group file.  So we must adjust.
                 * Note: Also done when reading mutes-groups from the
                 * MIDI file in midifile::parse_c_mutegroups().
                 */

                groupmutes = fix_midibooleans(groupmutes, mutes.group_count());
                rc().auto_mutes_save(true);
            }
            result = mutes.load(group, groupmutes);
        }
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
open_mutegroups (const std::string & source, mutegroups & mutes)
{
    bool result = ! source.empty();
    if (result)
    {
        mutegroupsfile mgf(source, mutes);
        result = mgf.parse();
    }
    return result;
}

bool
save_mutegroups (const std::string & destination, const mutegroups & mutes)
{
    bool result = ! destination.empty();
    if (result)
    {
        mutegroups & ncmutes = const_cast<mutegroups &>(mutes);
        mutegroupsfile mgf(destination, ncmutes);
        result = mgf.write();
        if (! result)
            file_error("Mute-groups write failed", destination);
    }
    else
        file_error("Mute-groups file to save", "none");

    return result;
}

}           // namespace seq66

/*
 * mutegroupsfile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


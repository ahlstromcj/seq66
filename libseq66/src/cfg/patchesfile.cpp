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
 * \file          patchesfile.cpp
 *
 *  This module declares/defines the base class for managing the reading and
 *  writing of the note-map sections of the 'drums' file.
 *
 * \library       seq66 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2019-11-05
 * \updates       2025-02-18
 * \license       GNU GPLv2 or above
 *
 */

#include <iomanip>                      /* std::setw()                      */
#include <iostream>                     /* std::cout                        */

#include "cfg/patchesfile.hpp"          /* seq66::patchesfile class         */
#include "cfg/settings.hpp"             /* seq66::rcsettings & seq66::rc()  */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Principal constructor.
 *
 * \param mapper
 *      Provides the patches reference to be acted upon.
 *
 * \param filename
 *      Provides the name of the note-map file; this is usually a full path
 *      file-specification to the "mutes" file using this object.
 *
 * \param rcs
 *      The configfile currently requires an rcsetting object, but it is not
 *      yet used here.
 */

patchesfile::patchesfile
(
    const std::string & filename,
    rcsettings & rcs
) :
    configfile      (filename, rcs, ".patches")
{
    // Empty body
}

/**
 *  A rote destructor.
 */

patchesfile::~patchesfile ()
{
    // ~configfile() called automatically
}

/**
 *  Parse the ~/.config/seq66/qseq66.drums file-stream or the
 *  ~/.config/seq66/seq66.mutes file-stream.
 */

bool
patchesfile::parse_stream (std::ifstream & file)
{
    bool result = true;
    file.seekg(0, std::ios::beg);                   /* seek to start    */
    (void) parse_version(file);

    std::string s = parse_comments(file);
#if defined USE_THIS_CODE

    /*
     *  TODO: support comments for patches.
     */

    if (! s.empty())
        mapper().comments_block().set(s);
#endif

    int patch = (-1);
    int position = find_tag(file, "[Patch ");
    bool good = position > 0;
    if (good)
    {
        patch = get_tag_value(line());
    }
    if (patch == (-1))
    {
        errprint("No [Patch nn] tag value found");
        good = false;
    }
    if (good)
    {
        for (int inpatch = 0; inpatch < int(c_midibyte_data_max); ++inpatch)
        {
            /*
             * In the patches file, we currently do not use gm-name (not
             * relevant to our simple setup) and gm-patch. We just want
             * the patch number (0-127) and the name of the patch as
             * defined by the legacy device, For now. Also, if there is
             * no patch for the device at a given patch number, use "N/A",
             * not an empty string.
             */

            char tagtmp[24];
            snprintf(tagtmp, sizeof tagtmp, "[Patch %d]", inpatch);
            std::string tag = tagtmp;
            std::string devname = get_variable(file, tag, "dev-name");
            good = ! devname.empty();
            if (good)
            {
                (void) add_patch(inpatch, devname);
            }
            else
            {
                error_message
                (
                    "Missing dev-name for patch number", std::to_string(inpatch)
                );
                break;
            }
        }
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
patchesfile::parse ()
{
    std::ifstream file(name(), std::ios::in | std::ios::ate);
    bool result = ! name().empty() && file.is_open();
    if (result)
    {
        file_message("Read drums", name());
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
 *  Writes the [mute-group] section to the given file stream.
 *
 * \param file
 *      Provides the output file stream to write to.
 *
 * \return
 *      Returns true if the write operations all succeeded.
 */

bool
patchesfile::write_stream (std::ofstream & file)
{
    write_date(file, "Patch file ('patches')");
    file <<
"# This file resembles the files generated by 'midicvtpp', modified for Seq66:\n"
"#\n"
"#   midicvtpp --csv-drum GM_DD-11_Drums.csv --output ddrums.ini\n"
"#\n"
"# This file defines legacy device-specific non-GM patch mappings. They are\n"
"# currently used for display when editing Program-Change events.\n"
        ;

    /*
     * [comments]
     */

    write_seq66_header(file, "patches", version());
//  write_comment(file, patches().comments_block().text());
    write_comment(file, "Current comments handling not ready for patches.");
    file <<
"\n"
"# Patch-mapping configuration for Seq66, stored in the HOME configuration\n"
"# directory. To use this file, add its name to the '[patch-file]' section of\n"
"# the 'rc' file. There's no user-interface for this file.\n"
"#\n"
    ;

    file <<
"\n"
"# The patches section:\n"
"#\n"
"#  [Patch 5]. Provides the ordering number for the patch sections.\n"
"#\n"
"#  gm-name    GM name for the patch assigned to the patch number.\n"
"#  gm-patch   Patch number, same as the section number.\n"
"#  dev-name   The device's name for the patch.\n"
"#  dev-patch  GM MIDI patch whose GM sound best matches the dev-name.\n"
"#             (Not yet used).\n"
"#\n"
    ;

    bool result = write_map_entries(file);
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
patchesfile::write ()
{
    std::ofstream file(name(), std::ios::out | std::ios::trunc);
    bool result = ! name().empty() && file.is_open();
    if (result)
    {
        file_message("Write patches", name());
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
 *  Writes the [note-mapper] section to the given file stream.  This can also
 *  be called by the rcfile object to just dump the data into that file.
 *
 * \param file
 *      Provides the output file stream to write to.
 *
 * \return
 *      Returns true if the write operations all succeeded.
 */

bool
patchesfile::write_map_entries (std::ofstream & file) const
{
    bool result = file.is_open();
    if (result)
    {
        std::string lst = program_list();   /* constructed in patches.cpp   */
        if (lst.empty())
            file << "No patches to write." << std::endl;
        else
            file << lst << std::endl;
    }
    return result;
}

#if 0

/**
 *  This function reads the source patches file and then saves it to the new
 *  location.
 *
 *  \param [inout] nm
 *      Provides the patches object.
 *
 *  \param source
 *      Provides the input file name from which the patches will be filled.
 *
 *  \param destination
 *      Provides the directory to which the play-list file is to be saved.
 *
 * \return
 *      Returns true if the operation succeeded.
 */

bool
copy_patches
(
    patches & nm,
    const std::string & source,
    const std::string & destination
)
{
    bool result = ! source.empty() && ! destination.empty();
    if (result)
    {
        std::string msg = source + " --> " + destination;
        patchesfile nmf(nm, source, rc());
        file_message("Note-map save", msg);
        result = nmf.parse();
        if (result)
        {
            nmf.name(destination);
            result = nmf.write();
            if (! result)
                file_error("Write failed", destination);
        }
        else
            file_error("Copy failed", source);
    }
    else
        file_error("Note-map file", "none");

    return result;
}

#endif      // 0

}           // namespace seq66

/*
 * patchesfile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


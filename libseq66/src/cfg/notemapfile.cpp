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
 * \file          notemapfile.cpp
 *
 *  This module declares/defines the base class for managing the reading and
 *  writing of the note-map sections of the 'drums' file.
 *
 * \library       seq66 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2019-11-05
 * \updates       2021-11-04
 * \license       GNU GPLv2 or above
 *
 */

#include <iomanip>                      /* std::setw()                      */
#include <iostream>                     /* std::cout                        */

#include "cfg/notemapfile.hpp"          /* seq66::notemapfile class         */
#include "cfg/settings.hpp"             /* seq66::rcsettings & seq66::rc()  */
#include "util/calculations.hpp"        /* seq66::current_data_time()       */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Principal constructor.
 *
 * \param mapper
 *      Provides the notemapper reference to be acted upon.
 *
 * \param filename
 *      Provides the name of the note-map file; this is usually a full path
 *      file-specification to the "mutes" file using this object.
 *
 * \param rcs
 *      The configfile currently requires an rcsetting object, but it is not
 *      yet used here.
 */

notemapfile::notemapfile
(
    notemapper & mapper,
    const std::string & filename,
    rcsettings & rcs
) :
    configfile      (filename, rcs),
    m_note_mapper   (mapper)
{
    // Empty body
}

/**
 *  A rote destructor.
 */

notemapfile::~notemapfile ()
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
notemapfile::parse_stream (std::ifstream & file)
{
    bool result = true;
    file.seekg(0, std::ios::beg);                   /* seek to start    */
    (void) parse_version(file);

    std::string s = parse_comments(file);
    if (! s.empty())
        mapper().comments_block().set(s);

    s = get_variable(file, "[notemap-flags]", "map-type");
    if (! s.empty())
        mapper().map_type(s);

    s = get_variable(file, "[notemap-flags]", "gm-channel");
    if (! s.empty())
        mapper().gm_channel(std::stoi(s));

    bool flag = get_boolean(file, "[notemap-flags]", "reverse");
    mapper().map_reversed(flag);

    int note = (-1);
    int position = find_tag(file, "[Drum ");
    bool good = position > 0;
    if (good)
    {
        note = get_tag_value(line());
    }
    if (note == (-1))
    {
        errprint("No [Drum nn] tag value found");
        good = false;
    }
    if (good)
    {
        for (int in_note = note; in_note < int(c_midibyte_data_max); ++in_note)
        {
            char tagtmp[16];
            snprintf(tagtmp, sizeof tagtmp, "[Drum %d]", in_note);
            std::string tag = tagtmp;
            std::string gmname = get_variable(file, tag, "gm-name");
            good = ! gmname.empty();
            if (good)
            {
                std::string tmp = get_variable(file, tag, "gm-note");
                good = ! tmp.empty();
                if (good)
                {
                    int gmnote = std::stoi(tmp);
                    std::string devname = get_variable(file, tag, "dev-name");
                    tmp = get_variable(file, tag, "dev-note");
                    good = ! tmp.empty();
                    if (good)
                    {
                        int devnote = std::stoi(tmp);
                        good = mapper().add(devnote, gmnote, devname, gmname);
                    }
                }
            }
        }
    }
    mapper().mode(result);
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
notemapfile::parse ()
{
    std::ifstream file(name(), std::ios::in | std::ios::ate);
    bool result = ! name().empty() && file.is_open();
    if (result)
    {
        file_message("Reading drums", name());
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
notemapfile::write_stream (std::ofstream & file)
{
    write_date(file, "note-mapper ('drums')");
    file <<
           "# This file resembles the files generated by 'midicvtpp', modified\n"
           "# for Seq66:\n"
           "#\n"
           "#   midicvtpp --csv-drum GM_DD-11_Drums.csv --output ddrums.ini\n"
           "#\n"
           "# This file can convert the percussion of non-GM devices to GM, as\n"
           "# closely as possible. Although it is for drums, it can be used\n"
           "# for other note-mappings.\n"
        ;

    /*
     * [comments]
     */

    write_seq66_header(file, "drums", version());
    write_comment(file, mapper().comments_block().text());
    file <<
        "# Drum/note-mapping configuration for Seq66, stored in the HOME\n"
        "# configuration directory.  To use this file, add this file-name to\n"
        "# '[note-mapper]' section of the 'rc' file. There's no user-interface\n"
        "# for this file. The main values are:\n"
        "#\n"
        "#   map-type: drum, patch, or multi; indicates the mapping to do.\n"
        "#   gm-channel: Indicates the channel (1-16) applied to convert notes.\n"
        "#   reverse: true or false; map in the opposite direction if true.\n"
        "\n"
        "[notemap-flags]\n"
        "\n"
        ;

    write_string(file, "map-type", mapper().map_type());
    write_integer(file, "gm-channel", mapper().gm_channel());
    write_boolean(file, "reverse", mapper().map_reversed());

    file <<
   "\n"
   "# The drum section:\n"
   "#\n"
   "#  [Drum 35].  Marks a GM drum-change section, one per instrument.\n"
   "#\n"
   "#  gm-name    GM name for the drum assigned to the input note.\n"
   "#  gm-note    Input note number, same as the section number.\n"
   "#  dev-name   The device's name for the drum.\n"
   "#  dev-note   GM MIDI note whose GM sound best matches the sound of\n"
   "#             dev-name.  The gm-note value is converted to the dev-note\n"
   "#             value, unless reverse mapping is activated. The actual GM\n"
   "#             drum sound might not match what the MIDI hardware puts out.\n"
   "\n"
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
notemapfile::write ()
{
    std::ofstream file(name(), std::ios::out | std::ios::trunc);
    bool result = ! name().empty() && file.is_open();
    if (result)
    {
        file_message("Writing drums", name());
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
notemapfile::write_map_entries (std::ofstream & file) const
{
    bool result = file.is_open();
    if (result)
    {
        int count = 0;
        for (const auto & mapentry : m_note_mapper.list())
        {
            file
                << "[Drum " << mapentry.second.dev_value() << "]" << "\n\n"
                << mapentry.second.to_string()
                << "\n"
                ;
            ++count;
        }
        if (count == 0)
        {
            file <<
            "# This is a sample.  See 'data/samples/GM_DD-11.drums' "
            "for a full example.\n"
            "\n"
            "[Drum 36]\n"
            "\n"
            "dev-name = \"Bass Drum Gated Reverb\"\n"
            "gm-name = \"Bass Drum 1\"\n"
            "dev-note = 36\n"
            "gm-note = 35\n"
            ;
        }
    }
    return result;
}

/**
 *  This function reads the source notemapper file and then saves it to the new
 *  location.
 *
 *  \param [inout] nm
 *      Provides the notemapper object.
 *
 *  \param source
 *      Provides the input file name from which the notemapper will be filled.
 *
 *  \param destination
 *      Provides the directory to which the play-list file is to be saved.
 *
 * \return
 *      Returns true if the operation succeeded.
 */

bool
copy_notemapper
(
    notemapper & nm,
    const std::string & source,
    const std::string & destination
)
{
    bool result = ! source.empty() && ! destination.empty();
    if (result)
    {
        std::string msg = source + " --> " + destination;
        notemapfile nmf(nm, source, rc());
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
        {
            file_error("Copy failed", source);
        }
    }
    else
    {
        file_error("Note-map file", "none");
    }
    return result;
}

}           // namespace seq66

/*
 * notemapfile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


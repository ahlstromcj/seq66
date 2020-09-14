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
 * \file          notemapfile.cpp
 *
 *  This module declares/defines the base class for managing the reading and
 *  writing of the note-map sections of the "rc" file.
 *
 * \library       seq66 application
 * \author        Seq24 team; modifications by Chris Ahlstrom
 * \date          2019-11-05
 * \updates       2020-09-14
 * \license       GNU GPLv2 or above
 *
 */

#include <iomanip>                      /* std::setw()                      */
#include <iostream>                     /* std::cout                        */

#include "cfg/notemapfile.hpp"          /* seq66::notemapfile class         */
#include "cfg/settings.hpp"             /* seq66::rcsettings & seq66::rc()  */
#include "util/calculations.hpp"        /* seq66::current_data_time()       */
#include "util/strfunctions.hpp"        /* seq66::string_to_bool()          */

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
 *      The configfile currently requires and rcsetting object, but it is not
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
 *      The mute-group starts with a line that indicates up to 32 mute-groups are
 *      defined. A common value is 1024, which means there are 32 groups times 32
 *      keys.  But this value is currently thrown away.  This value is followed by
 *      32 lines of data, each contained 4 sets of 8 settings.  See the seq66-doc
 *      project on GitHub for a much more detailed description of this section.
 */

bool
notemapfile::parse_stream (std::ifstream & file)
{
    bool result = true;
    file.seekg(0, std::ios::beg);                   /* seek to start    */

    /*
     * [comments] Header commentary is skipped during parsing.  However, we
     * now try to read an optional comment block.  This block is part of the
     * notemap container, not part of the rcsettings object.
     */

    std::string s = parse_comments(file);
    if (! s.empty())
    {
        mapper().comments_block().set(s);
#if defined SEQ66_PLATFORM_DEBUG
        if (rc().verbose())
            std::cout << s;
#endif
    }

    s = get_variable(file, "[notemap-flags]", "map-type");
    if (! s.empty())
        mapper().map_type(s);

    s = get_variable(file, "[notemap-flags]", "gm-channel");
    if (! s.empty())
        mapper().gm_channel(std::stoi(s));

    s = get_variable(file, "[notemap-flags]", "reverse");
    if (! s.empty())
        mapper().map_reversed(string_to_bool(s));

    int note = (-1);
    int position = find_tag(file, "[Drum ");
    bool good = position > 0;
    if (good)
    {
#if defined SEQ66_PLATFORM_DEBUG_TMI
        printf("drum line %s\n", line().c_str());  // JUST A TEST
#endif
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
        pathprint("Reading 'drums'", name());
        result = parse_stream(file);
#if defined SEQ66_PLATFORM_DEBUG
        if (result && rc().verbose())
            mapper().show();
#endif
    }
    else
    {
        file_error("Read open fail", name());
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
notemapfile::write_stream (std::ofstream & file)
{
    file
        << "# Seq66 0.91.0 (and above) note-mapper configuration file\n"
        << "#\n"
        << "# " << name() << "\n"
        << "# Written on " << current_date_time() << "\n"
        << "#\n"
        << "# This file is similar to files generated by our 'midicvt' program,\n"
        << "# but heavily modified for simplicity for Seq66.\n"
        << "#\n"
        << "#   midicvtpp --csv-drum GM_DD-11_Drums.csv --output ddrums.ini\n"
        << "#\n"
        << "# This file can be used to convert the percussion of non-GM devices\n"
        << "# to GM, as best as permitted by GM percussion. Although it is a\n"
        << "# 'drums' file, it can be used for other note-mappings as well.\n"
        << "\n"
        ;

    /*
     * [comments]
     */

    file <<
        "[Seq66]\n\n"
        "config-type = \"drums\"\n"
        "version = 0\n"
        "\n"
        "# The [comments] section can document this file.  Lines starting\n"
        "# with '#' and '[' are ignored.  Blank lines are ignored.  Show a\n"
        "# blank line by adding a space character to the line.\n\n"
        "[comments]\n\n" << mapper().comments_block().text() << "\n"
        <<
        "# This file holds a drum-note mapping configuration for Seq66. It is\n"
        "# always stored in the configuration directory.  To use this file,\n"
        "# add this file-name to the '[note-mapper]' section of the 'rc' file.\n"
        "# There is currently no user interface for managing this file.\n"
        "# The main values are:\n"
        "#\n"
        "#   map-type: drum, patch, or multi; indicates the mapping to do.\n"
        "#   gm-channel: Indicates the channel (1-16) applied to converted notes.\n"
        "#   reverse: true or false; map in the opposite direction if true.\n"
        "#\n"
        "[notemap-flags]\n"
        "#\n"
        ;

    file
        << "map-type = " << mapper().map_type() << "\n"
        << "gm-channel = " << mapper().gm_channel() << "\n"
        << "reverse = " << (mapper().map_reversed() ? "true" : "false") << "\n"
        "#\n"
        ;

    file <<
   "# The drum section:\n"
   "#\n"
   "#  [Drum 35].  Marks a GM drum-change section, one per instrument.\n"
   "#\n"
   "#  gm-name          GM name for the drum assigned to the input note.\n"
   "#  gm-note          Input note number, same as the section number.\n"
   "#  dev-name         The devices name for the drum.\n"
   "#  dev-note         GM MIDI note whose GM sound best matches the sound\n"
   "#                   of dev-name.  The gm-note value is converted to the\n"
   "#                   dev-note value (unless reverse mapping is activated).\n"
   "#                   Note that the actual GM drum sound might not match\n"
   "#                   what the MIDI hardware puts out.\n"
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
notemapfile::write ()
{
    std::ofstream file(name(), std::ios::out | std::ios::trunc);
    bool result = ! name().empty() && file.is_open();
    if (result)
    {
        pathprint("Writing 'drums'", name());
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
        for (const auto & mapentry : m_note_mapper.list())
        {
            file << "[Drum " << mapentry.second.dev_value() << "]" << "\n\n";
            file << mapentry.second.to_string();
            file << "\n";
        }
    }
    return result;
}

}           // namespace seq66

/*
 * notemapfile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


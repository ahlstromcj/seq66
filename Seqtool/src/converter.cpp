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
 * \file          converter.cpp
 * \library       Seqtool (from the Seq66 project)
 * \author        Chris Ahlstrom
 * \date          2018-12-07
 * \updates       2019-03-20
 * \version       $Revision$
 * \license       $XPC_SUITE_GPL_LICENSE$
 *
 *  This module provides the conversion of Sequencer64 configuration files to
 *  Sequencer66 configuration files.  The format of the command for making the
 *  conversion is:
 *
\verbatim
      $ ./Seqtool/src/seqtool --convert infilebase --output outfilebase
      $ ./Seqtool/src/seqtool -c infilebase -o outfilebase
\endverbatim
 *
 *  This command processes files from the input directory and writes them to
 *  the output directory.  These names are currently hardwired:
 *
\verbatim
        Input directory:   ~/.config/sequencer64
        Output directory:  ~/.config/seq66
\endverbatim
 *
 *  The 'infilebase' argument is the base-name (no path, no extension) of the
 *  input 'rc' and 'usr' files to be converted.
 *
 *  The 'outfilebase' argument is the base-name of the output files to be
 *  created, which consist of 'rc', 'usr', 'ctrl', 'mutes', and 'playlist'
 *  files.
 *
 *  For brevity, we'll use 'IN' and 'OUT' for the base-names. The following
 *  transformations are made:
 *
 *      -#  The 'rc' file:
 *          -#  The file ~/.config/sequencer64/IN.rc is processed and written
 *              to ~/.config/seq66/OUT.rc (in a new format).
 *          -#  If IN.rc contains a [midi-control-file] section that specifies
 *              a control file names CTRL.rc, then the MIDI controls are
 *              written (in a new format) to ~/.config/seq66/CTRL.ctrl.
 *          -#  Otherwise, the MIDI controls are written (in a new format) to
 *              ~/.config/seq66/OUT.ctrl.
 *          -#  The [mute-group] section of IN.rc is written (in a new format)
 *              to ~/.config/seq66/OUT.mutes.
 *          -#  If a [playlist] is specified, and the file is accessible,
 *              it is copied from ~/.config/sequencer64/NAME.playlist to
 *              ~/.config/seq66/NAME.playlist.  This works only if
 *              the [playlist] file-name does not include a path.
 *      -#  The 'usr' file is simply copied from ~/.config/sequencer64/IN.usr
 *          to ~/.config/seq66/OUT.usr.
 *
 *  To summarize, without showing the source and destination directories:
 *
\verbatim
    IN.rc               -->         OUT.rc, OUT.ctrl, and OUT.mutes
    NAME.playlist       -->         NAME.playlist
    IN.usr              -->         OUT.usr
\endverbatim
 *
 *  If desired, the user can move the contents of OUT.ctrl into OUT.rc and
 *  remove (or comment out) the [midi-control-file] section.
 *
 *  If desired, the user can move the contents of OUT.mutes into OUT.rc and
 *  remove (or comment out) the [mute-group-file] section.
 */

#include <iostream>
#include <string>

#include "cfg/midicontrolfile.hpp"      /* seq66::midicontrolfile           */
#include "cfg/mutegroupsfile.hpp"       /* seq66::mutegroupsfile            */
#include "cfg/rcfile.hpp"               /* seq66::rcfile                    */
#include "cfg/settings.hpp"             /* seq66::rc() configuration class  */
#include "converter.hpp"                /* seq66::converter file conversion */
#include "optionsfile.hpp"              /* seq66::optionsfile old-style rc  */
#include "util/filefunctions.hpp"       /* seq66::file_readable()           */
#include "util/strfunctions.hpp"        /* seq66::replace()                 */

namespace seq66
{

/**
 *  Constructor, default.
 */

converter::converter () :
    m_rc_settings           (rc()),
    m_in_base_filename      ("sequencer64"),
    m_out_base_filename     ("seq66"),
    m_input_filename        (),
    m_output_filename       (),
    m_ctrl_filename         (),
    m_mutes_filename        (),
    m_playlist_filename     (),
    m_user_filename         (),
    m_input_file_exists     (false)
{
    initialize();
}

/**
 *  Constructor, specify the rcsettings object to use.
 */

converter::converter
(
    rcsettings & configuration,
    const std::string & inbasename,
    const std::string & outbasename
) :
    m_rc_settings           (configuration),
    m_in_base_filename      (inbasename),
    m_out_base_filename     (outbasename),
    m_input_filename        (),
    m_output_filename       (),
    m_ctrl_filename         (),
    m_mutes_filename        (),
    m_playlist_filename     (),
    m_user_filename         (),
    m_input_file_exists     (false)
{
    initialize();
}

/**
 *  Sets up the filenames.
 */

void
converter::initialize ()
{
    /*
     * Get the input filename, which includes the Sequencer66 configuration
     * directory, and replace that with the Sequencer64 configuration
     * directory.
     */

    m_rc_settings.set_defaults();               /* this is important to do  */
    m_input_filename = m_rc_settings.make_config_filespec
    (
        m_in_base_filename, ".rc"
    );
    m_input_filename = string_replace(m_input_filename, "66", "64");
    m_output_filename = m_rc_settings.make_config_filespec
    (
        m_out_base_filename, ".rc"
    );
    m_ctrl_filename = m_rc_settings.make_config_filespec
    (
        m_out_base_filename, ".ctrl"
    );
    m_mutes_filename = m_rc_settings.make_config_filespec
    (
        m_out_base_filename, ".mutes"
    );

    /*
     * This is determined by the [playlist] section, if it exists, in the legacy
     * (source) 'rc' file.  Plus, we don't yet want to prepend the home
     * configuration directory.
     *
     *  m_playlist_filename = m_rc_settings.make_config_filespec
     *  (
     *      m_out_base_filename, ".playlist"
     *  );
     */

    m_user_filename = m_rc_settings.make_config_filespec
    (
        m_in_base_filename, ".usr"
    );
    m_input_file_exists = file_readable(m_input_filename);
}

/**
 *  Parses the old-style "rc" file.
 */

bool
converter::parse ()
{
    bool result = input_file_exists();
    if (result)
    {
        seq66::optionsfile opfile(m_rc_settings, input_filename());
        result = opfile.parse();
        if (result)
        {
#if defined SEQ66_PLATFORM_DEBUG_TMI
            midicontrolfile dummy("Dummy");
            (void) dummy.container_to_stanzas(m_rc_settings.midi_controls());
            dummy.show_stanzas();
#endif
        }
        else
            errprintfunc("failed");
    }
    return result;
}

/**
 *  Writes the new-style "rc", "ctrl", and "mutes" files, using
 *  the rcsettings object passed to the converter.  Here are some
 *  specific settings used to control what gets saved, and how:
 *
 *      -   use_midi_control_file():    Hard-wired to true.
 *      -   use_mute_group_file():      Hard-wired to true.
 *      -   The playlist file:          Always done even in legacy code.
 *      -   mute_group_save():          The value read from the legacy file.
 */

bool
converter::write ()
{
    bool result = true;
    rcfile rcf(output_filename(), m_rc_settings);
    std::string mc = m_rc_settings.midi_control_filename();
    std::string mg = m_rc_settings.mute_group_filename();
    std::string pl = m_rc_settings.playlist_filename();
    std::string us = m_rc_settings.user_filename();
    if (mc.empty())
    {
        /*
         * The legacy 'rc' file had no [midi-control-file] section, but we
         * want to force one for the conversion, using the file-name base passed
         * to the converter.
         */

        m_rc_settings.midi_control_filename(m_ctrl_filename);
        mc = m_rc_settings.midi_control_filename();
    }
    if (mg.empty())
    {
        /*
         * The legacy 'rc' file had no [mute-group-file] section, which is
         * always the case, as this feature is new to Sequencer66. We want to
         * force one for the conversion, using the file-name base passed to the
         * converter.
         */

        m_rc_settings.mute_group_filename(m_mutes_filename);
        mg = m_rc_settings.mute_group_filename();
    }
    if (pl.empty())
    {
        /*
         * The legacy 'rc' file had no [playlist] section.  In this case, we want
         * to ensure that the converter does not try to copy one to the
         * Sequencer66 configuration directory.
         */

        m_rc_settings.playlist_active(false);
        m_rc_settings.playlist_filename("");
        m_playlist_filename.clear();
    }
    else
    {
        /*
         * Even if inactive, if a playlist was specified, we will copy it.
         * If the original rc file specified no [playlist], don't bother with
         * the copy.  We could use the m_playlist_filename as well.
         * Currently redundant.
         */

        std::string source = pl;        /* the original playlist file-name  */
        std::string path;
        std::string base;

        m_playlist_filename = string_replace(pl, "64", "66");
        m_rc_settings.playlist_filename(m_playlist_filename);
        (void) filename_split(m_playlist_filename, path, base);
        result = make_directory_path(path);
        if (result)
        {
#if defined SEQ66_PLATFORM_DEBUG_TMI
            std::cout
                << "COPYING " << pl
                << " to " << m_playlist_filename
                << std::endl
                ;
#endif
            result = file_copy(pl, m_playlist_filename);
        }
    }
    if (us.empty())
    {
        m_rc_settings.user_filename(m_user_filename);
        us = m_rc_settings.user_filename();
    }
    m_rc_settings.use_midi_control_file(true);
    m_rc_settings.use_mute_group_file(true);

#if defined SEQ66_PLATFORM_DEBUG_TMI
    std::cout
        << "converter::write():\n"
        << "   Output 'rc' config file: '" << output_filename() << "'\n"
        << "   MIDI control file:       '" << mc << "'\n"
        << "   Mute groups file:        '" << mg << "'\n"
        << "   Playlist file:           '" << pl << "'\n"
        << "   User config file:        '" << mc << "'\n"
        ;
#endif

    show();

    result = rcf.write();               /* also writes the ctrl & mutes     */
    if (result)
    {
        std::string infile = string_replace(m_user_filename, "66", "64");
        std::string outfile = m_rc_settings.make_config_filespec
        (
            m_out_base_filename, ".usr"
        );
        result = file_copy(infile, outfile);
    }
    return result;
}

/**
 *  If verbose, show the names of the configuration files.
 */

void
converter::show ()
{
    if (rc().verbose())
    {
        using namespace std;
        std::string exists = bool_to_string(m_input_file_exists);
        cout
            << "In base file-name:          '" << m_in_base_filename << "'\n"
            << "Out base file-name:         '" << m_out_base_filename << "'\n"
            << "In config file-name:        '" << m_input_filename << "'\n"
            << "Out config file-name:       '" << m_output_filename << "'\n"
            << "MIDI control file-name:     '" << m_ctrl_filename << "'\n"
            << "Mute groups file-name:      '" << m_mutes_filename << "'\n"
            << "Play-list file-name:        '" << m_playlist_filename << "'\n"
            << "User file-name:             '" << m_user_filename << "'\n"
            << "Input file exists:          " << exists << "\n"
            ;
    }
}

}           // namespace seq66

/*
 * converter.cpp
 *
 * vim: ts=4 sw=4 et ft=cpp
 */


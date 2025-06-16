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
 * \file          playlistfile.cpp
 *
 *  This module declares/defines the base class for managing the <code>
 *  ~/.seq66rc </code> legacy configuration file or the new <code>
 *  ~/.config/seq66/seq66.rc </code> ("rc") configuration file.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2020-09-19
 * \updates       2025-05-03
 * \license       GNU GPLv2 or above
 *
 *  Here is a skeletal representation of a Seq66 playlist file:
 *
 \verbatim
        [playlist]
        0                       # playlist number, a MIDI value (0 to 127)
        "Downtempo"             # playlist name, for display/selection
        /home/user/midifiles/   # directory where the songs are stored
        10 file1.mid            # MIDI value and file's base-name
        11 file2.midi
        12 file3.midi           # . . .
\endverbatim
 *
 *  See the file data/sample.playlist for a more up-to-date example and
 *  explanation.
 */

#include "cfg/playlistfile.hpp"         /* seq66::playlistfile class        */
#include "cfg/settings.hpp"             /* seq66::rc()                      */
#include "play/playlist.hpp"            /* seq66::playlist class            */
#include "util/filefunctions.hpp"       /* functions for file-names         */
#include "util/strfunctions.hpp"        /* strip_quotes()                   */

namespace seq66
{

static const int s_playlist_file_version = 1;

/**
 *  Principal constructor.
 *
 * \param p
 *      Provides the performer object that will interface between this module
 *      and the rest of the application.
 *
 * \param name
 *      Provides the name of the options file; this is usually a full path
 *      file-specification.
 *
 * \param rcs
 *      Provides a reference to an "rc" settings object to hold current options
 *      and modified options.
 *
 * \param show_on_stdout
 *      If true (the default is false), then the list/song information is
 *      written to stdout, to help with debugging.
 */

playlistfile::playlistfile
(
    const std::string & filename,
    playlist & pl,
    rcsettings & rcs,
    bool show_on_stdout
) :
    configfile          (filename, rcs, ".playlist"),
    m_play_list         (pl),
    m_show_on_stdout    (show_on_stdout)
{
    version(s_playlist_file_version);
}

/**
 *  This destructor is a rote do-nothing destructor.
 */

playlistfile::~playlistfile ()
{
    // No code needed
}

/**
 *  Helper function for error-handling.  It assembles a message and then
 *  passes it to append_error_message().
 *
 * \param additional
 *      Additional context information to help in finding the error.
 *
 * \return
 *      Always returns false.
 */

bool
playlistfile::set_error_message (const std::string & additional)
{
    std::string msg = "Playlist file";
    if (! additional.empty())
    {
        msg += ": ";
        msg += additional;
    }
    warn_message(msg);
    append_error_message(msg);
    return false;
}

/**
 *  Opens the current play-list file and optionally verifies it.
 *
 * \param verify_it
 *      If true (the default), call verify() to make sure the playlist is
 *      sane.  The verification is weak, rather than the strong option which
 *      is available.
 *
 * \return
 *      Returns true if the file was parseable and verifiable.  The caller
 *      (especially the performer) may want to do a "clear all".
 */

bool
playlistfile::open (bool verify_it)
{
    bool result = parse();
    if (result)
    {
        if (verify_it)
        {
            if (m_show_on_stdout)
            {
                msgprintf
                (
                    msglevel::status, "Verifying playlist %s", name().c_str()
                );
            }
            result = play_list().verify();                  /* weak verify  */
        }
    }
    play_list().loaded(result);
    return result;
}

/**
 *  Parses the ~/.config/seq66/file.playlistfile file.
 *
 * The next_section() function is like line-after, but scans from the
 * current line in the file.  Necessary here because all the sections
 * have the same name.  After detecting the "[playlist]" section, the
 * following items need to be obtained:
 *
 *      -   Playlist number.  This number is used as the key value for
 *          the playlist. It can be any MIDI value (0 to 127), and the order
 *          of the playlists is based on this number, and selectable via MIDI
 *          control with this number.
 *      -   Playlist name.  A human-readable string describing the
 *          nick-name for the playlist.  This is an alternate way to
 *          look up the playlist.
 *      -   Song directory name.  The directory where the songs are
 *          stored.  If this name is empty, then the song file-names
 *          need to include the individual directories for each file.
 *          But even if not empty, the play-list directory is not used if
 *          the song file-name includes a path, as indicated by "/" or "\".
 *      -   Song file-name, or path to the song file-name.
 *
 *  Note that the call to next_section() already gets to the next line of
 *  data, which should be the index number of the playlist.
 *
 *  We need to get the song's MIDI control number and it's directory name.
 *  Make sure the directory name is canonical and clean.  The existence of the
 *  file should be validated later.  Also determine if the song file-name
 *  already has a directory before using the play-list's directory.
 *
 *  Consider catching file exceptions!
 *
 * \return
 *      Returns true if the file was able to be opened for reading.
 *      Currently, there is no indication if the parsing actually succeeded.
 */

bool
playlistfile::parse ()
{
    if (is_empty_string(name()))
        return false;

    std::ifstream file(name(), std::ios::in | std::ios::ate);
    bool result = file.is_open();
    if (result)
    {
        file_message("Read", name());
        file.seekg(0, std::ios::beg);                   /* seek to start    */
        play_list().clear();
        (void) parse_version(file);

        std::string temp = parse_comments(file);
        if (! temp.empty())
            play_list().comments_block().set(temp);

        /*
         * [playlist-options]
         */

        std::string tag = "[playlist-options]";
        if (file_version_number() < s_playlist_file_version)
        {
            result = version_error_message("playlist", file_version_number());
            rc_ref().auto_playlist_save(true);
        }
        else
        {
            /*
             * The first one would be better named "auto-arm".
             */

            bool flag = get_boolean(file, tag, "unmute-next-song");
            play_list().auto_arm(flag);
            flag = get_boolean(file, tag, "auto-play");
            play_list().auto_play(flag);
            flag = get_boolean(file, tag, "auto-advance");
            play_list().auto_advance(flag);
            flag = get_boolean(file, tag, "deep-verify");
            play_list().deep_verify(flag);
        }

        int listcount = 0;
        bool have_section = line_after(file, "[playlist]");
        if (! have_section)
        {
            result = set_error_message("empty/missing [playlist] section");
        }
        while (have_section)
        {
            int listnumber = -1;
            int songcount = 0;
            playlist::play_list_t plist;            /* current playlist     */
            sscanf(scanline(), "number = %d", &listnumber);
            if (m_show_on_stdout)
            {
                msgprintf
                (
                    msglevel::status, "Processing playlist #%d", listnumber
                );
            }
            if (next_data_line(file))
            {
                std::string listline = line();
                playlist::song_list slist;
                listline = extract_variable(line(), "name");
                plist.ls_list_name = listline;
                if (m_show_on_stdout)
                {
                    msgprintf
                    (
                        msglevel::status, "Playlist name '%s'",
                        listline.c_str()
                    );
                }
                if (next_data_line(file))
                {
                    listline = line();
                    listline = extract_variable(line(), "directory");
                    plist.ls_file_directory = clean_path(listline);
                    slist.clear();
                    if (m_show_on_stdout)
                    {
                        msgprintf
                        (
                            msglevel::status, "Playlist directory '%s'",
                            listline.c_str()
                        );
                    }
                    while (next_data_line(file))
                    {
                        int songnumber = -1;
                        std::string fname;
                        result = scan_song_file(songnumber, fname);
                        fname = strip_quotes(fname);
                        if (result)
                        {
                            playlist::song_spec_t sinfo;
                            sinfo.ss_index = songcount;
                            sinfo.ss_midi_number = songnumber;
                            if (name_has_path(fname))
                            {
                                std::string path;
                                std::string filebase;
                                filename_split(fname, path, filebase);
                                sinfo.ss_song_directory = path;
                                sinfo.ss_embedded_song_directory = true;
                                sinfo.ss_filename = filebase;
                                (void) play_list().add_song(slist, sinfo);
                                ++songcount;
                            }
                            else if (! fname.empty())
                            {
                                sinfo.ss_song_directory = plist.ls_file_directory;
                                sinfo.ss_embedded_song_directory = false;
                                sinfo.ss_filename = fname;
                                (void) play_list().add_song(slist, sinfo);
                                ++songcount;
                            }
                        }
                        else
                        {
                            std::string msg = "scanning song file '";
                            msg += fname;
                            msg += "' failed";
                            result = set_error_message(msg);
                            break;
                        }
                    }

                    /*
                     * Need to deal with a false result?  It's not really a
                     * fatal error to have an empty song list.
                     */

                    plist.ls_index = listcount;         /* ordinal      */
                    plist.ls_midi_number = listnumber;  /* MIDI mapping */
                    plist.ls_song_count = songcount;
                    plist.ls_song_list = slist;         /* copy temp    */
                    result = play_list().add_list(plist);
                }
                else
                {
                    std::string msg = "no list directory in playlist #" +
                        std::to_string(listnumber);

                    result = set_error_message(msg);
                    break;
                }
            }
            else
            {
                std::string msg = "no data in playlist #" +
                    std::to_string(listnumber);

                result = set_error_message(msg);
                break;
            }
            ++listcount;
             have_section = next_section(file, "[playlist]");
        }
        file.close();                           /* done with playlist file  */
    }
    else if (rc().playlist_active() && ! name().empty())
    {
        std::string msg = "Open failed: " + name();
        result = set_error_message(msg);
    }
    (void) play_list().reset_list(0, ! result); /* reset, not clear, if ok  */
    return result;
}

/**
 *  Encapsulates some groty code for the parse() function.  It assumes that
 *  next_data_line() has retrieved a file-name line for a song.
 *
 * \param [out] song_number
 *      Holds the song number that was retrieved.  Use it only if not equal
 *      to -1 and if this function returns true.
 *
 * \param [out] song_file
 *      Holds the song file-name that was retrieved.  Use it only if not
 *      empty and if this function returns true.
 *
 * \return
 *      Returns true if this function succeeded.  If false, an error message is
 *      set up.
 */

bool
playlistfile::scan_song_file (int & song_number, std::string & song_file)
{
    bool result = false;
    int songnumber = -1;
    const char * dirname = &m_line[0];
    int sscount = sscanf(scanline(), "%d", &songnumber);
    if (sscount == EOF || sscount == 0)
    {
        song_number = -1;                                   /* side-effect  */
        song_file.clear();                                  /* side-effect  */
        result = set_error_message("song number missing");
    }
    else
    {
        while (! std::isspace(*dirname))
        {
            if (*dirname == 0)
                break;

            ++dirname;
        }
        while (std::isspace(*dirname))
        {
            if (*dirname == 0)
                break;

            ++dirname;
        }
        bool gotit = std::isalnum(*dirname) || std::ispunct(*dirname);
        if (gotit)
        {
            song_number = songnumber;                       /* side-effect  */
            song_file = dirname;                            /* side-effect  */
            result = true;
        }
        else
        {
            song_number = -1;                               /* side-effect  */
            song_file.clear();                              /* side-effect  */
            result = set_error_message("song file-path missing");
        }
    }
    return result;
}

/**
 *  Writes the play-list to the file whose name is returned by the name()
 *  function.  If the name is empty, we don't try to open it; that truncates
 *  the file!
 *
 *  Consider catching file exceptions!
 *
 * \return
 *      Returns true if the write operations all succeeded.
 */

bool
playlistfile::write ()
{
    std::ofstream file(name(), std::ios::out | std::ios::trunc);
    bool result = ! name().empty() && file.is_open();
    if (result)
    {
        file_message("Write playlist", name());
    }
    else
    {
        file_error("Write open fail", name());
        return result;
    }
    write_date(file, "playlist");
    file <<
"# This file holds multiple playlists, each in a [playlist] section. Each has\n"
"# a user-specified number for sorting and MIDI control, ranging from 0 to 127.\n"
"# Next comes a quoted name for this list, followed by the quoted name\n"
"# of the song directory using the UNIX separator ('/').\n"
"#\n"
"# Next is a list of tunes, each starting with a MIDI control number and the\n"
"# quoted name of the MIDI file, sorted by control number. They can be simple\n"
"# 'base.midi' file-names; the playlist directory is prepended to access the\n"
"# song file. If the file-name has a path, that will be used.\n"
        ;
    write_seq66_header(file, "playlist", version());

    /*
     * [comments]
     */

    std::string c = play_list().comments_block().text();
    write_comment(file, c);

    /*
     * [playlist-options]
     */

    file << "\n[playlist-options]\n\n";

    file <<
"# 'unmute-next-song' causes the next selected song to have all patterns\n"
"# armed for playback. (Should be called 'auto-arm'). Does not matter for\n"
"# songs with triggers for Song mode. 'auto-play' causes songs to start play\n"
"# automatically when loaded. 'auto-advance' implies the settings noted\n"
"# above. It automatically loads the next song in the play-list when the\n"
"# current song ends. 'deep-verify' causes each tune in the play-list to be\n"
"# loaded to make sure each one can be loaded. Otherwise, only file existence\n"
"# is checked.\n\n"
        ;

    write_boolean(file, "unmute-next-song", play_list().auto_arm());
    write_boolean(file, "auto-play", play_list().auto_play());
    write_boolean(file, "auto-advance", play_list().auto_advance());
    write_boolean(file, "deep-verify", play_list().deep_verify());
    file << "\n"
"# Here are the playlist settings, default storage folder, and then a list of\n"
"# each tune with its control number. The playlist number is arbitrary but\n"
"# unique. 0 to 127 enforced for use with MIDI playlist controls. Similar\n"
"# for the tune numbers. Each tune can include a path; it overrides the base\n"
"# directory.\n"
       ;

    /*
     * [playlist] sections
     */

    bool is_empty = true;
    for (const auto & plpair : play_list().play_list_map())
    {
        const playlist::play_list_t & pl = plpair.second;
        file << "\n[playlist]\n\n";

        write_integer(file, "number", pl.ls_midi_number);
        write_string(file, "name", pl.ls_list_name, true);
        write_string(file, "directory", pl.ls_file_directory, true);
        file << "\n";

        /*
         * For each song, write the MIDI control number, followed only by
         * the song's file-name, which could include the path-name.
         */

        const playlist::song_list & sl = pl.ls_song_list;
        for (const auto & sc : sl)
        {
            const playlist::song_spec_t & s = sc.second;
            bool haspath = s.ss_embedded_song_directory;
            std::string fname;
            if (haspath)
            {
                fname = filename_concatenate(s.ss_song_directory, s.ss_filename);
            }
            else
            {
                fname = s.ss_filename;
            }
            fname = add_quotes(fname);
            file << s.ss_midi_number << " " << fname << "\n";
        }
        is_empty = false;
    }
    if (is_empty)
    {
        file
        << "\n[playlist]\n\n"
"# This is a NON-FUNCTIONAL playlist SAMPLE. Please see one of the sample\n"
"# playlist files shipped with Seq66, or fill this in the Playlist tab.\n\n"
            ;
    }
    write_seq66_footer(file);
    file.close();
    return true;
}

/*
 * Free functions for handling play-lists and files.
 */

/**
 *  Reads the play-list data into a playlist object.
 *
 * \param source
 *      Provides the full path file-specification for the play-list file to be
 *      opened.
 *
 * \param show_on_stdout
 *      If true (the default is false), the playlist is opened to show
 *      song selections on stdout.  This is useful for trouble-shooting or for
 *      making the CLI version of Sequencer64 easier to follow when running.
 *
 * \return
 *      Returns true if the playlist object was able to be created. If the
 *      file-name is not empty, this also means that it was opened, and the
 *      play-list read.  If false is returned, then the previous playlist, if
 *      any, still exists, but is marked as inactive.
 */

bool
open_playlist
(
    playlist & pl,
    const std::string & source,
    bool show_on_stdout
)
{
    bool result = ! is_missing_string(source);  /* empty, "", or "?"        */
    if (result)
    {
        playlistfile plf(source, pl, rc(), show_on_stdout);
        result = plf.open(true);            /* parse and file verify    */
        if (result)
        {
            // Anything worth doing?
        }
        else if (rc().playlist_active())
        {
            std::string msg = "Open failed: ";
            msg += source;
            (void) error_message(msg);
        }
    }
    else
    {
        file_error("Playlist file", "none");
        pl.loaded(false);
    }
    return result;
}

/**
 *  Writes the play-list, whether it is active or not, as long as it exists.
 *  Saves a playlist to file.  Used in clinsmanager and in performer.
 *
 * \param pl
 *      Provides a reference to the playlist.  The caller is responsible for
 *      making sure this parameter is valid.
 *
 * \param destfile
 *      Provides the full path file-specification for the play-list file to be
 *      saved.  If empty, the file-name with which the play-list was created
 *      is used.
 *
 * \return
 *      Returns true if the write operation succeeded.
 *
 */

bool
save_playlist
(
    playlist & pl,
    const std::string & destfile
)
{
    std::string destination = destfile.empty() ? pl.file_name() : destfile;
    bool result = ! is_missing_string(destination);     /* empty, "", "?"   */
    if (result)
    {
        playlistfile plf(destination, pl, rc(), false); /* false --> quiet  */
        pl.file_name(destination);
        result = plf.write();
        if (! result)
            file_error("Playlist write failed", destination);
    }
    else
        file_error("Playlist file to save", "none");

    return result;
}

/**
 *  This function reads the source playlist file and then saves it to the new
 *  location.
 *
 *  \param [inout] pl
 *      Provides the playlist object.
 *
 *  \param source
 *      Provides the input file name from which the playlist will be filled.
 *
 *  \param destination
 *      Provides the directory to which the play-list file is to be saved.
 *
 * \return
 *      Returns true if the operation succeeded.
 */

bool
save_playlist
(
    playlist & pl,
    const std::string & source,
    const std::string & destination
)
{
    bool result = ! source.empty() && ! destination.empty();
    if (result)
    {
        playlistfile plf(source, pl, rc(), false);  /* false --> quiet      */
        result = plf.open(false);                   /* parse, no verify     */
        if (result)
        {
            result = save_playlist(pl, destination);
        }
        else
        {
            file_error("Open failed", source);
        }
    }
    else
    {
        file_error("Playlist file", "none");
    }
    return result;
}

/**
 *  This function uses the playlist to copy all of the MIDI files noted in the
 *  source playlist file.
 *
 *  Note that the directory structure is somewhat preserved in the destination.
 *  For example, here is an input directory and the resultant output directory
 *  for the import of ca_midi.playlist:
 *
 *      -   /pub/Audio/MIDI/archives/Ca/Mid/FM Synth/
 *      -   ~/.config/seq66/playlists/ca_midi/pub/Audio/MIDI/.../Mid/FM Synth/
 *
 *  Is this good or bad?
 *
 *  \param [in] plp
 *      Provides a pointer to the playlist, which should have been filled with
 *      playlist data.
 *
 *  \param source
 *      Simply provides the input file name for information only.
 *
 *  \param destination
 *      Provides the directory to which the play-list file is to be saved.
 *
 * \return
 *      Returns true if the operation succeeded.
 */

bool
copy_playlist_songs
(
    playlist & pl,
    const std::string & source,
    const std::string & destination
)
{
    bool result = ! source.empty() && ! destination.empty();
    if (result)
    {
        std::string msg = source + " --> " + destination;
        file_message("Playlist copy", msg);
        result = pl.copy_songs(destination);
        if (! result)
            file_error("Copy failed", destination);
    }
    else
    {
        file_error("Playlist file directories", "<empty>");
    }
    return result;
}

}           // namespace seq66

/*
 * playlistfile.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


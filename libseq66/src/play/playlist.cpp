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
 * \file          playlist.cpp
 *
 *  This module declares/defines the base class for managing the qseq66.playlist
 *  file family.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-08-26
 * \updates       2023-10-31
 * \license       GNU GPLv2 or above
 *
 *  See the playlistfile class for information on the file format.
 */

#include <cctype>                       /* std::toupper() function          */
#include <iostream>                     /* std::cout                        */
#include <utility>                      /* std::make_pair()                 */
#include <string.h>                     /* memset()                         */

#include "cfg/settings.hpp"             /* seq66::rc()                      */
#include "midi/wrkfile.hpp"             /* seq66::midifile & seq66::wrkfile */
#include "play/playlist.hpp"            /* seq66::playlist support class    */
#include "play/performer.hpp"           /* seq66::performer anchor class    */
#include "util/filefunctions.hpp"       /* functions for file-names         */
#include "util/strfunctions.hpp"        /* strip_quotes()                   */

namespace seq66
{

/**
 *  This object is used in returning a reference to a bogus object.
 */

playlist::song_list playlist::sm_dummy;


/**
 *  Principal constructor.
 *
 * \param p
 *      Provides the performer object that will interface between this module
 *      and the rest of the application.
 *
 * \param filename
 *      Provides the name of the options file; this is usually a full path
 *      file-specification.
 *
 * \param show_on_stdout
 *      If true (the default is false), then the list/song information is
 *      written to stdout, to help with debugging.
 */

playlist::playlist
(
    performer * p,
    const std::string & filename,
    bool show_on_stdout
) :
    basesettings            (filename),
    m_performer             (p),                    /* owner of this object */
    m_play_lists            (),
    m_loaded                (false),                /* playlist loaded      */
    m_deep_verify           (false),
    m_current_list          (m_play_lists.end()),
    m_current_song          (sm_dummy.end()),       /* song-list iterator   */
    m_auto_arm              (false),
    m_auto_play             (false),
    m_engage_auto_play      (false),
    m_auto_advance          (false),
    m_midi_base_directory   (rc().midi_base_directory()),
    m_show_on_stdout        (show_on_stdout)
{
    // No code
}

/**
 *  This destructor unregisters this playlist from the performer object.
 */

playlist::~playlist ()
{
    // No code
}

/**
 *  Indicates only if the loaded data is usable. A lesser version of verify()
 *  that is used before the play-lists are saved to a play-list file.
 */

bool
playlist::validated () const
{
    bool result = ! m_play_lists.empty();
    if (result)
    {
        auto beginning = m_play_lists.cbegin();
        result = beginning->second.ls_song_count > 0;
        if (! result)
            return true;                /* no songs to verify */
    }
    if (result)
    {
        for (const auto & plpair : m_play_lists)
        {
            if (plpair.second.ls_song_count > 0)
            {
                const song_list & sl = plpair.second.ls_song_list;
                for (const auto & sci : sl)
                {
                    const song_spec_t & s = sci.second;
                    std::string fname = song_filepath(s);
                    if (fname.empty())
                    {
                        result = false;
                        break;
                    }
                }
                if (! result)
                    break;
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
 *  Used (only) in the user interface to validate that usable playlist data is
 *  present and to activate the playlist.
 *
 * \return
 *      Returns true if the settings were made.
 */

bool
playlist::activate (bool flag)
{
    bool result = false;
    bool change = flag != rc().playlist_active();
    if (change)
    {
        if (flag)
        {
            bool ok = validated();
            if (ok)
            {
                loaded(true);            /* i.e. the play-lists are loaded   */
                result = true;
            }
        }
        else
            result = true;

        rc().playlist_active(flag);
        rc().auto_rc_save(true);
    }
    return result;
}


/**
 *  Returns true if in playlist mode and the playlist is active.  These are
 *  two separate conditions at present.
 */

bool
playlist::active () const
{
    return rc().playlist_active() && loaded();
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
playlist::set_error_message (const std::string & additional) const
{
    if (! additional.empty())
    {
        std::string msg = "Play-list: ";
        msg += additional;
        basesettings::set_error_message(msg);       /* add to error message */
    }
    return false;
}

/**
 *  This function checks for no songs in a play-list structure.  This is a
 *  workaround for an issue that occurs when a play-list file gets corrupted.
 *  We don't want to exit just because there is no playlist loaded. And this
 *  check does not work:
 *
 *      result = ! plist.ls_song_list.empty();
 */

bool
playlist::check_song_list (const play_list_t & plist)
{
    return plist.ls_song_count > 0;
}

/**
 *  Given a file-name, opens that file as a song.  This function holds common
 *  code.  It is similar to read_midi_file() in the midifile module, but
 *  does not affect the recent-files list.
 *
 *  Before the song is loaded, the current song is cleared from memory.
 *  Remember that clear_all() will fail if it detects a sequence being edited.
 *  In that case, this function will fail as well.
 *
 * \param fname
 *      The full path to the file to be opened.  If this parameter is empty,
 *      no load is attempted, but playback is stopped and the song is cleared.
 *
 * \param verifymode
 *      If true, open the file in play-list mode.  Currently, this means
 *      is that some output from the file-opening process is suppressed, and
 *      the performer::clear_all() function is called right after parsing the
 *      song file to verify it.
 */

bool
playlist::open_song (const std::string & fname, bool verifymode)
{
    bool result = not_nullptr(m_performer);

#if defined USE_OLD_CODE            // ca 2023-07-12
    if (result)
    {
        /*
         * These could be done in performer:delay_stop(), which
         * all playlist traversal functions call.
         */

        if (m_performer->is_pattern_playing())
            m_performer->stop_playing();

        result = m_performer->clear_song();
    }
#endif

    if (result)
    {
        std::string errmsg_dummy;
        result = m_performer->read_midi_file(fname, errmsg_dummy, false);
        if (result && verifymode)
        {
            /* nothing to do yet */
        }
    }
    return result;
}

/**
 *  Selects the song based on the index (row) value, and optionally opens it.
 *
 * \param index
 *      The ordinal location of the song within the current playlist.
 *
 * \param opensong
 *      If true (the default), the song is opened.
 *
 * \return
 *      Returns true if the song is selected, and if specified, is opened
 *      succesfully as well.
 */

bool
playlist::open_select_song (int index, bool opensong)
{
    bool result = select_song(index);
    if (result && opensong)
        result = open_current_song();

    return result;
}

/**
 *  Selects the song based on the MIDI control value, and optionally opens it.
 *
 * \param ctrl
 *      The MIDI control to access the song within the current playlist.
 *
 * \param opensong
 *      If true (the default), the song is opened.
 *
 * \return
 *      Returns true if the song is selected, and if specified, is opened
 *      succesfully as well.
 */

bool
playlist::open_select_song_by_midi (int ctrl, bool opensong)
{
    bool result = select_song_by_midi(ctrl);
    if (result && opensong)
        result = open_current_song();

    return result;
}

/**
 *  Goes through all of the playlists and makes sure that all of the song
 *  files are accessible.
 *
 * \param strong
 *      If true, also make sure the MIDI files open without error as well.
 *      The code is similar to read_midi_file() in the midifile module, but it
 *      does not make configuration settings.  Setting this option to true can
 *      slow startup way down if there are a lot of big files in the playlist.
 *
 * \return
 *      Returns true if all of the MIDI files are verifiable.  A blank
 *      filename (empty playlist) results in a false return value.
 */

bool
playlist::verify (bool strong)
{
    bool result = ! m_play_lists.empty();
    if (result)
    {
        /*
         * This can add a new (and empty) play-list :-( Not good to access a
         * map via operator [].
         *
         *  result = m_play_lists[0].ls_song_count > 0;
         */

        auto beginning = m_play_lists.cbegin();
        result = beginning->second.ls_song_count > 0;
        if (! result)
            return true;                /* no songs to verify */
    }
    if (result)
    {
        for (const auto & plpair : m_play_lists)
        {
            const song_list & sl = plpair.second.ls_song_list;
            for (const auto & sci : sl)
            {
                const song_spec_t & s = sci.second;
                std::string fname = song_filepath(s);
                if (fname.empty())
                {
                    result = false;
                    break;
                }
                if (file_exists(fname))
                {
                    if (strong)
                    {
                        /*
                         * The file is parsed.  If the result is false, then
                         * the play-list mode ends up false.  Let the caller
                         * do the reporting on errors.
                         */

                        result = open_song(fname, true);
                        if (result)
                        {
                            if (rc().verbose())
                                file_message("Verified", fname);
                        }
                        else
                        {
                            set_file_error_message("song '%s' missing", fname);
                            break;
                        }
                    }
                }
                else
                {
                    std::string fmt = plpair.second.ls_list_name;
                    fmt += ": song '%s' missing; check relative directories.";
                    result = set_file_error_message(fmt, fname);
                    break;
                }
            }
            if (! result)
                break;
        }
    }
    else
    {
        std::string msg = "empty list file '";
        msg += file_name();
        msg += "'";
        set_error_message(msg);
    }
    return result;
}

/**
 *  This function copies all of the MIDI files in all of the play-lists to a
 *  new root directory.
 *
 *  It should also update the MIDI base directory in the "rc" file.
 *
 *  The copy will silently fail at the first file that is not found to exist.
 *
 * \param destination
 *      Provides the path to the destination directory for the MIDI files.
 *      This directory is created if it does not exist, after normalizing this
 *      path.
 *
 * \return
 *      Returns true if every copy operation was able to be completed.
 */

bool
playlist::copy_songs (const std::string & destination)
{
    bool result = ! m_play_lists.empty();
    if (result)
    {
        std::string dst = os_normalize_path(destination);
        result = make_directory_path(dst);
        if (result)
        {
            file_message("Playlist directory", dst);
            for (const auto & plpair : m_play_lists)
            {
                const play_list_t & pl = plpair.second;
                const song_list & sl = pl.ls_song_list;
                file_message("Playlist", pl.ls_list_name);
                for (const auto & sci : sl)
                {
                    const song_spec_t & s = sci.second;
                    std::string fname = song_filepath(s);
                    file_message("Song", fname);
                    result = file_exists(fname);
                    if (result)
                    {
                        std::string d = append_path(dst, s.ss_song_directory);
                        result = make_directory_path(d);
                        if (result)
                        {
                            d = append_file(d, s.ss_filename);
                            result = file_copy(fname, d);
                            if (! result)
                            {
                                set_file_error_message("Copy failed", d);
                                break;
                            }
                        }
                        else
                        {
                            set_file_error_message("Create failed", d);
                            break;
                        }
                    }
                    else
                    {
                        set_file_error_message("File does not exist", fname);
                        break;
                    }
                }
                if (! result)
                    break;
            }
            if (result)
            {
                /*
                 * Now we need to make each playlist directory relative.
                 */

                for (auto & plpair : m_play_lists)
                {
                    play_list_t & pl = plpair.second;
                    std::string playdir = pl.ls_file_directory;
                    pl.ls_file_directory = make_path_relative(playdir);
                }
            }
        }
        else
        {
            set_file_error_message("Failed to create", dst);
        }
    }
    else
    {
        std::string msg = "empty list file '";
        msg += file_name();
        msg += "'";
        set_error_message(msg);
    }
    return result;
}

/**
 *  Opens/loads the current song.
 *
 * \return
 *      Returns true if there was a song to be opened, and it opened properly.
 */

bool
playlist::open_current_song ()
{
    bool result = true;
    if (active())
    {
        result = m_current_list != m_play_lists.end();
        if (result)
        {
            play_list_t & plist = m_current_list->second;
            result = check_song_list(plist);
            if (! result)
                return true;            /* no songs to open */

            if (result)
                result = m_current_song != plist.ls_song_list.end();

            if (result)
            {
                std::string fname = song_filepath(m_current_song->second);
                if (! fname.empty())
                {
                    result = open_song(fname);
                    if (! result)
                    {
                        (void) set_file_error_message
                        (
                            "Open failed: song '%s'", fname
                        );
                    }
                }
            }
        }
    }
    return result;
}

bool
playlist::open_next_list (bool opensong, bool loading)
{
    bool result = false;
    if (active() || loading)
    {
        result = next_list(true);      /* select the next list, first song */
        if (result && opensong)
            result = open_current_song();
    }
    return result;
}

bool
playlist::open_previous_list (bool opensong)
{
    bool result = false;
    if (active())
    {
        result = previous_list(true);  /* select the prev. list, first song */
        if (result && opensong)
            result = open_current_song();
    }
    return result;
}

bool
playlist::open_select_list (int index, bool opensong)
{
    bool result = select_list(index, opensong);
    if (active())
    {
        if (result && opensong)
            result = open_current_song();
    }
    return result;
}

bool
playlist::open_select_list_by_midi (int ctrl, bool opensong)
{
    bool result = true;
    if (active())
    {
        result = select_list_by_midi(ctrl, opensong);
        if (result && opensong)
            result = open_current_song();
    }
    return result;
}

bool
playlist::open_next_song (bool opensong)
{
    bool result = false;                // true;
    if (active())
    {
        result = next_song();
        if (result && opensong)
            result = open_current_song();
    }
    return result;
}

bool
playlist::open_previous_song (bool opensong)
{
    bool result = false;                // true;
    if (active())
    {
        result = previous_song();
        if (result && opensong)
            result = open_current_song();
    }
    return result;
}

/**
 *  Makes a file-error message.
 */

bool
playlist::set_file_error_message
(
    const std::string & fmt,
    const std::string & filename
)
{
    char tmp[256];
    snprintf(tmp, sizeof tmp, fmt.c_str(), filename.c_str());
    set_error_message(tmp);
    return false;
}

/**
 *  Clears the comments and the play-lists, sets the play-list mode to false,
 *  and disables the list and song iterators.
 */

void
playlist::clear ()
{
    comments_block().clear();
    m_play_lists.clear();
    loaded(false);
    m_current_list = m_play_lists.end();
    m_current_song = sm_dummy.end();
}

/**
 *  Resets to the first play-list and the first-song in that playlist.
 *
 * \param listindex
 *      Set the current list to this value, which defaults to 0.
 *
 * \param clearit
 *      If true, clear the playlist no matter what. Then false is returned.
 *
 * \return
 *      Returns true if the play-lists were present and the first song of the
 *      first play-list was able to be selected.
 */

bool
playlist::reset_list (int listindex, bool clearit)
{
    bool result = false;
    if (clearit)
    {
        clear();
    }
    else
    {
        result = ! m_play_lists.empty();
        if (result)
        {
            int index = 0;
            for (auto p = m_play_lists.begin(); p != m_play_lists.end(); ++p)
            {
                if (index == listindex)
                {
                    m_current_list = p;
                    break;
                }
                ++index;
            }
            result = select_song(0);
        }
    }
    return result;
}

/*
 *  List-container functions.
 */

/**
 *  Adds an already set playlist structure.  It is copied into the list of
 *  play-lists.  It assumes all of the fields in the play-list have been set,
 *  including an empty song-list.
 *
 * \note
 *      Do not reorder!
 *
 * \param plist
 *      Provides a reference to the new playlist to be added.  This parameter
 *      is copied into the list.
 *
 * \return
 *      Returns true if the count of playlists has changed.  However, if a
 *      playlist was simply being modified, this value is false.  So the usage
 *      of the return parameter is dependent upon the context of the call.
 */

bool
playlist::add_list (const play_list_t & plist)
{
    bool result = false;
    int count = int(m_play_lists.size());
    int listnumber = plist.ls_midi_number;      /* MIDI control number  */
    if (listnumber >= 0)
    {
        /*
         * std::pair<int, play_list_t>
         */

        auto ls = std::make_pair(listnumber, plist);
        m_play_lists.insert(ls);
        result = int(m_play_lists.size()) == count + 1;
    }
    return result;
}

/**
 *  Selects a play-list with the given index (i.e. a row value).
 *
 * \param index
 *      The index of the play-list re 0.
 *
 * \param selectsong
 *      If true, then the first (0th) song in the play-list is selected.
 *
 * \return
 *      Returns true if the selected play-list is valid.  If true, then the
 *      m_current_list iterator points to the current list.
 */

bool
playlist::select_list (int index, bool selectsong)
{
    bool result = false;
    int count = 0;
    auto minimum = m_play_lists.begin();
    auto maximum = m_play_lists.end();
    for (auto pci = minimum; pci != maximum; ++pci, ++count)
    {
        if (count == index)
        {
            if (m_show_on_stdout)
                show_list(pci->second);

            m_current_list = pci;
            if (selectsong)
                select_song(0);

            result = true;
        }
    }
    return result;
}

/**
 *  We want to provide the user with an unused list/control number.  The user
 *  can scroll to the bottom of the list in the user interface, but this
 *  is annoying and easy to forget. We look at the last entry, and increment
 *  the key value by one, if possible.
 *
 * \return
 *      Returns the next number after the last one. If there's none
 *      available, then (-1) is returned.
 */

int
playlist::next_available_list_number () const
{
    int result = (-1);
    auto last = m_play_lists.rbegin();
    if (last != m_play_lists.rend())
    {
        int controlnumber = last->first;
        if (controlnumber < 127)
            result = controlnumber + 1;
    }
    return result;
}

/**
 *  Selects a play-list with the given MIDI control value.
 *
 * \param index
 *      The MIDI control value of the play-list.  Generally should be
 *      restricted to the range of 0 to 127, to be suitable for MIDI control.
 *
 * \param selectsong
 *      If true, then the first (0th) song in the play-list is selected.
 *
 * \return
 *      Returns true if the selected play-list is valid.  If true, then the
 *      m_current_list iterator points to the current list.
 */

bool
playlist::select_list_by_midi (int ctrl, bool selectsong)
{
    bool result = false;
    int count = 0;
    auto minimum = m_play_lists.begin();
    auto maximum = m_play_lists.end();
    for (auto pci = minimum; pci != maximum; ++pci, ++count)
    {
        int midinumber = pci->second.ls_midi_number;
        if (midinumber == ctrl)
        {
            if (m_show_on_stdout)
                show_list(pci->second);

            m_current_list = pci;
            if (selectsong)
                select_song(0);

            result = true;
        }
    }
    return result;
}

/**
 *  Moves to the next play-list.  If the iterator reaches the end, this
 *  function wraps around to the beginning.  Also see the other return value
 *  conditions.
 *
 * \param selectsong
 *      If true (the default), the first song in the play-list is selected.
 *
 * \return
 *      Returns true if the play-list iterator was able to be moved, or if
 *      there was only one play-list, so that movement was unnecessary. If the
 *      there are no play-lists, then false is returned.
 */

bool
playlist::next_list (bool selectsong)
{
    bool result = m_play_lists.size() > 0;          /* there's at least one */
    if (m_play_lists.size() > 1)
    {
        ++m_current_list;
        if (m_current_list == m_play_lists.end())
            m_current_list = m_play_lists.begin();

        if (m_show_on_stdout)
            show_list(m_current_list->second);

        if (selectsong)
            select_song(0);
    }
    return result;
}

/**
 *  Moves to the previous play-list.  If the iterator reaches the beginning,
 *  this function wraps around to the end.  Also see the other return value
 *  conditions.
 *
 * \param selectsong
 *      If true (the default), the first song in the play-list is selected.
 *
 * \return
 *      Returns true if the play-list iterator was able to be moved, or if
 *      there was only one play-list, so that movement was unnecessary. If the
 *      there are no play-lists, then false is returned.
 */

bool
playlist::previous_list (bool selectsong)
{
    bool result = m_play_lists.size() > 0;
    if (m_play_lists.size() > 1)
    {
        if (m_current_list == m_play_lists.begin())
            m_current_list = std::prev(m_play_lists.end());
        else
            --m_current_list;

        if (m_show_on_stdout)
            show_list(m_current_list->second);

        if (selectsong)
            select_song(0);
    }
    return result;
}

/**
 *  The following four functions are to be used for a playlist editor, though
 *  they can also be used for parsing/loading the playlist.  Kind of a low
 *  priority, but we are leaving room for the concept.
 *
 *  In usage, the playlist and its songs are ordered by MIDI control number.
 *  That is, the MIDI control number is the key, which allows for the fast
 *  lookup of the MIDI control number during a live performance involving MIDI
 *  control from the musician.
 *
 *  In editing, the playlist and its songs are ordered by MIDI control number.
 *  When a playlist is added, we are adding a row to the table, keyed by the
 *  control number.  We then have to re-order the playlist and repopulate the
 *  table.  The song-list for the new playlist is empty, and the playlist
 *  table must reflect that.
 *
 *  Adding a song to the new playlist, if it is indeed new, should be
 *  straightforward.
 *
 *  Adding a song to an existing playlist (which might be a new playlist)
 *  means inserting the song and re-ordering the playlist's songs as
 *  necessary).
 *
 *  The add_list() function adds a playlist and re-orders the list of
 *  playlists.  The playlist is initially empty, and an empty song-list is
 *  created.
 *
 * \todo
 *
 * \return
 *      Returns the return-value of the add_list() overload, which returns
 *      true if the count has changed.
 */

/**
 *  An overloaded function to encapsulate adding a playlist and make the
 *  callers simpler.  The inserted list has an empty song-list.  This function
 *  is intended for use by a playlist editor.
 *
 * \todo
 *      Add the ability to replace a play-list as well.
 *
 * \param index
 *      Provides the location of the active list in the table.  The actual
 *      stored value may change after reordering.
 */

bool
playlist::add_list
(
    int index,
    int midinumber,
    const std::string & name,
    const std::string & directory
)
{
    play_list_t plist;                  /* will be copied upon insertion    */
    plist.ls_index = index;             /* an ordinal value from list table */
    plist.ls_midi_number = midinumber;  /* MIDI control number to use       */
    plist.ls_list_name = name;
    plist.ls_file_directory = directory;
    plist.ls_song_count = 0;            /* no songs to start in new list    */

    /*
     * Song list is empty at first, created by the playlist default constructor.
     *
     *      plist.ls_song_list = slist;
     */

    bool result = add_list(plist);
    reorder_play_list();
    return result;
}

bool
playlist::modify_list
(
    int index,
    int midinumber,
    const std::string & name,
    const std::string & directory
)
{
    bool result = m_current_list != m_play_lists.end();
    if (result)
    {
        play_list_t & plist = m_current_list->second;
        plist.ls_index = index;             /* ordinal value in list table  */
        plist.ls_midi_number = midinumber;  /* MIDI control number to use   */
        plist.ls_list_name = name;
        plist.ls_file_directory = directory;
    }
    return result;
}

/**
 *  This function removes a playlist at the given index.  The index is an
 *  ordinal, not a key, therefore we have to iterate through the whole list
 *  until we encounter the desired index.  Useful for removing the selected
 *  playlist in a table.
 *
 *  This function works by iterating to the index'th element in the playlist
 *  and deleting it.
 *
 * \param midinumber
 *      The ordinal value (not a key) of the desired table row.
 *
 * \return
 *      Returns true if the desired list was found and removed.
 */

bool
playlist::remove_list (int index)
{
    bool result = false;
    int count = 0;
    auto minimum = m_play_lists.begin();
    auto maximum = m_play_lists.end();
    for (auto pci = minimum; pci != maximum; /* ++pci, */ ++count)
    {
        if (count == index)
        {
            pci = m_play_lists.erase(pci);
            result = true;
            break;
        }
        else
            ++pci;
    }
    if (result)
        reorder_play_list();

    return result;
}

/**
 *  Moves through the play-list container in key (MIDI control number) order,
 *  modifying the index value of each playlist in the main list.
 */

void
playlist::reorder_play_list ()
{
    auto minimum = m_play_lists.begin();
    auto maximum = m_play_lists.end();
    int index = 0;
    for (auto pci = minimum; pci != maximum; ++pci, ++index)
    {
        play_list_t & p = pci->second;
        p.ls_index = index;
    }
}

/*
 *  Song-container functions.
 */

/**
 *  Obtains the current song index, which is a number starting at 0 that
 *  indicates the song's position in the list.  This value is useful to put
 *  the song information at the right place in a table, for example. If the
 *  current-list iterator is invalid, or the current-song iterator is invalid,
 *  then (-1) is returned.
 */

int
playlist::song_index () const
{
    int result = (-1);
    if (m_current_list != m_play_lists.end())
    {
        play_list_t & plist = m_current_list->second;
        if (m_current_song != plist.ls_song_list.end())
            result = m_current_song->second.ss_index;
    }
    return result;
}

/**
 *  Used to return m_current_list->second.ls_file_directory.
 */

std::string
playlist::file_directory () const
{
    std::string result;
    if (m_current_list != m_play_lists.end())
    {
        play_list_t & plist = m_current_list->second;
        return plist.ls_file_directory;
    }
    return result;
}

/**
 *  Used to return m_current_list->second.ls_file_directory.
 */

std::string
playlist::song_directory () const
{
    std::string result;
    if (m_current_list != m_play_lists.end())
    {
        play_list_t & plist = m_current_list->second;
        if (m_current_song != plist.ls_song_list.end())
            result = m_current_song->second.ss_song_directory;
    }
    return result;
}

/**
 *  Used to return m_current_list->second.ls_file_directory.
 */

bool
playlist::is_own_song_directory () const
{
    bool result = false;
    if (m_current_list != m_play_lists.end())
    {
        play_list_t & plist = m_current_list->second;
        if (m_current_song != plist.ls_song_list.end())
            result = m_current_song->second.ss_embedded_song_directory;
    }
    return result;
}

/**
 *  Obtains the current song MIDI control number, which is a number ranging
 *  from 0 to 127 that indicates the control number that triggers the song. If
 *  the current-list iterator is invalid, or the current-song iterator is
 *  invalid, then (-1) is returned.
 */

int
playlist::song_midi_number () const
{
    int result = (-1);
    if (m_current_list != m_play_lists.end())
    {
        play_list_t & plist = m_current_list->second;
        if (m_current_song != plist.ls_song_list.end())
            result = m_current_song->second.ss_midi_number;
    }
    return result;
}

/**
 *  Returns the name of the current song for display purposes.  This name may
 *  contain a hard-wired relative path, but always contains the base name of
 *  the song (*.midi).
 */

std::string
playlist::song_filename () const
{
    std::string result;
    if (m_current_list != m_play_lists.end())
    {
        play_list_t & plist = m_current_list->second;
        if (m_current_song != plist.ls_song_list.end())
        {
            /*
             * This would return the full path for display, for those cases
             * where the directory is different from the play-list's
             * directory.  However, that is too long to display in some cases.
             * We need to think this through some more.
             *
             *  if (m_current_song->second.ss_embedded_song_directory)
             *      result = song_filepath(m_current_song->second);
             *  else
             */

            result = m_current_song->second.ss_filename;
        }
    }
    return result;
}

void
playlist::midi_base_directory (const std::string & basedir)
{
    m_midi_base_directory = os_normalize_path(basedir);
}

/**
 *  Gets the MIDI base directory, if non-empty. Gets the song directory, if
 *  non-empty.  Gets the base filename of the song, which might include a
 *  relative path.  Then it concatenates the MIDI base directory,
 *  song-directory, and the song name, and returns it.  Note the calls to
 *  clean_path(), which ensure the paths end with a slash character.
 */

std::string
playlist::song_filepath (const song_spec_t & sinfo) const
{
    std::string songdir = clean_path(sinfo.ss_song_directory);
    std::string base = midi_base_directory();
    std::string basedir = clean_path(base);
    std::string result = basedir + songdir + sinfo.ss_filename;
    return result;
}

/**
 *  Gets the current song-specification from the current play-list, and, if
 *  valid concatenates the song's base directory, specificed sub-directory and
 *  file-name, which might include a relative path.
 *
 * \return
 *      Returns the song's directory and file-name as a full path
 *      specification.  However, if there's an error, then an empty string is
 *      returned.
 */

std::string
playlist::song_filepath () const
{
    std::string result;
    if (m_current_list != m_play_lists.end())
    {
        play_list_t & plist = m_current_list->second;
        if (m_current_song != plist.ls_song_list.end())
            result = song_filepath(m_current_song->second);
    }
    return result;
}

/**
 *  Provides a one-line description containing the current play-list name and
 *  song file.
 *
 * \return
 *      Returns the play-list name and song file-name.  If not in playlist
 *      mode, or an item cannot be found, then an empty string is returned.
 */

std::string
playlist::current_song () const
{
    std::string result;
    if (loaded())
    {
        if (m_current_list != m_play_lists.end())
        {
            play_list_t & plist = m_current_list->second;
            if (m_current_song != plist.ls_song_list.end())
            {
                int mnumber = m_current_song->second.ss_midi_number;
                result = plist.ls_list_name;
                result += ": ";
                result += int_to_string(mnumber);
                result += " ";
                result += m_current_song->second.ss_filename;
            }
        }
    }
    return result;
}

/**
 *  Selects a song with the given index (i.e. its ordinal position in the
 *  playlist or in a table in a GUI).
 *
 * \param index
 *      The index of the song re 0.
 *
 * \return
 *      Returns true if the current play-list and the current song are valid.
 *      If true, then the m_current_song iterator points to the current song.
 */

bool
playlist::select_song (int index)
{
    bool result = false;
    if (m_current_list != m_play_lists.end())
    {
        int count = 0;
        play_list_t & plist = m_current_list->second;
        song_list & slist = plist.ls_song_list;
        for (auto sci = slist.begin(); sci != slist.end(); ++sci, ++count)
        {
            if (count == index)
            {
                if (m_show_on_stdout)
                    show_song(sci->second);

                m_current_song = sci;
                result = true;
                break;
            }
        }
    }
    return result;
}

/**
 *  We want to provide the user with an unused song/control number.  The user
 *  can scroll to the bottom of the list in the user interface, but this
 *  is annoying and easy to forget. We look at the last entry, and increment
 *  the key value by one, if possible.
 *
 * \return
 *      Returns the next number after the last one. If there's none
 *      available, then (-1) is returned.
 */

int
playlist::next_available_song_number () const
{
    int result = (-1);
    if (m_current_list != m_play_lists.end())
    {
        play_list_t & plist = m_current_list->second;
        song_list & slist = plist.ls_song_list;
        auto last = slist.rbegin();
        if (last != slist.rend())
        {
            int controlnumber = last->first;
            if (controlnumber < 127)
                result = controlnumber + 1;
        }
    }
    return result;
}

/**
 *  Selects a song with the given MIDI control value (the key of the map).
 *
 * \param index
 *      The MIDI control value.  Generally should be restricted to the
 *      range of 0 to 127, to be suitable for MIDI control.
 *
 * \return
 *      Returns true if the current play-list and the current song are valid.
 *      If true, then the m_current_song iterator points to the current song.
 */

bool
playlist::select_song_by_midi (int ctrl)
{
    bool result = false;
    if (m_current_list != m_play_lists.end())
    {
        int count = 0;
        play_list_t & plist = m_current_list->second;
        song_list & slist = plist.ls_song_list;
        for (auto sci = slist.begin(); sci != slist.end(); ++sci, ++count)
        {
            int midinumber = sci->second.ss_midi_number;
            if (midinumber == ctrl)
            {
                if (m_show_on_stdout)
                    show_song(sci->second);

                m_current_song = sci;
                result = true;
            }
        }
    }
    return result;
}

/**
 *  Moves to the next song in the current playlist, wrapping around to the
 *  beginning.
 *
 * \return
 *      Returns true if the next song was selected.
 */

bool
playlist::next_song ()
{
    bool result = false;
    if (m_current_list != m_play_lists.end())
    {
        play_list_t & plist = m_current_list->second;
        ++m_current_song;
        if (m_current_song == plist.ls_song_list.end())
            m_current_song = plist.ls_song_list.begin();

        result = m_current_song != plist.ls_song_list.end();
        if (result)
        {
            const std::string & fname = m_current_song->second.ss_filename;
            result = ! is_empty_string(fname);
        }
        if (result && m_show_on_stdout)
            show_song(m_current_song->second);
    }
    return result;
}

bool
playlist::previous_song ()
{
    bool result = false;
    if (m_current_list != m_play_lists.end())
    {
        play_list_t & plist = m_current_list->second;
        if (m_current_song == plist.ls_song_list.begin())
            m_current_song = std::prev(plist.ls_song_list.end());
        else
            --m_current_song;

        result = m_current_song != plist.ls_song_list.end();
        if (result)
        {
            const std::string & fname = m_current_song->second.ss_filename;
            result = ! is_empty_string(fname);
        }
        if (result && m_show_on_stdout)
            show_song(m_current_song->second);
    }
    return result;
}

/**
 *  Adds a song to the current playlist, if available.  Calls the add_song()
 *  overload taking the current song-list and the provided song-specification.
 *
 * \param sspec
 *      Provides the infromation about the song to be added.
 *
 * \return
 *      Returns true if the song was added.
 */

bool
playlist::add_song (song_spec_t & sspec)
{
    bool result = m_current_list != m_play_lists.end();
    if (result)
    {
        play_list_t & plist = m_current_list->second;
        result = add_song(plist, sspec);    /* ultimately reorders the list */
    }
    return result;
}

/**
 *  Adds the given song to the given song-list.
 *
 * \param slist
 *      Provides the song-list to hold the new song.
 *
 * \param sspec
 *      Provides the infromation about the song to be added.
 *
 * \return
 *      Returns true if the song was added.  That is, if the size of the
 *      song-list increased by 1.
 */

bool
playlist::add_song (song_list & slist, song_spec_t & sspec)
{
    bool result = false;
    int count = int(slist.size());
    int songnumber = sspec.ss_midi_number;
    auto s = std::make_pair(songnumber, sspec); /* std::pair<int, song_spec_t> */
    slist.insert(s);
    result = int(slist.size()) == count + 1;
    if (result)
        reorder_song_list(slist);

    return result;
}

/**
 *      Adds the given song to the song-list of the given play-list.
 *
 * \param plist
 *      Provides the playlist whose song-list is to be updated.
 *
 * \param sspec
 *      Provides the infromation about the song to be added.
 *
 * \return
 *      Returns true if the song was added.  That is, the return value of the
 *      song-list, song-specification add_song() overload.
 */

bool
playlist::add_song (play_list_t & plist, song_spec_t & sspec)
{
    std::string listdir = plist.ls_file_directory;
    if (! listdir.empty())
    {
        std::string songdir = sspec.ss_song_directory;
        if (songdir.empty())
            sspec.ss_embedded_song_directory = false;
        else
            sspec.ss_embedded_song_directory = songdir != listdir;
    }

    song_list & sl = plist.ls_song_list;
    bool result = add_song(sl, sspec);      /* calls reorder_song_list()    */
    if (result)
        ++plist.ls_song_count;

    return result;
}

/**
 *  An overloaded function to encapsulate adding a song and make the
 *  callers simpler.  This function is intended for use by a playlist editor.
 *  It supports the replacement of existing songs.
 *
 * \param index
 *      Provides the location of the active item in the table.  The actual
 *      stored value may change after reordering.
 */

bool
playlist::add_song
(
    int index, int midinumber,
    const std::string & name,
    const std::string & directory
)
{
    bool result = ctrl_is_valid(midinumber);
    if (result)
        result = m_current_list != m_play_lists.end();

    if (result)
    {
        play_list_t & plist = m_current_list->second;
        song_list & slist = plist.ls_song_list;
        song_spec_t sspec;                  /* copied upon insertion        */
        if (do_ctrl_lookup(midinumber))     /* handle -1 via lookup         */
        {
            int indexmax;
            last_song_indices(slist, indexmax, midinumber);
            if (do_ctrl_lookup(index))                  /* do -1 via lookup */
                index = indexmax;
        }
        sspec.ss_index = index;
        sspec.ss_midi_number = midinumber;
        sspec.ss_song_directory = directory;
        sspec.ss_embedded_song_directory = false;       /* adjusted later   */
        sspec.ss_filename = name;

        /*
         * Song list is empty at first.  We need to increment the song count
         * for the current playlist, by calling the playlist overload of
         * add_song().
         */

        result = add_song(plist, sspec);        /* add song, reorder list   */
        if (! result)
        {
            if (remove_song(index))
                result = add_song(sspec);       /* add song, reorder list   */
        }
    }
    return result;
}

bool
playlist::add_song (const std::string & fullpath)
{
    bool result = ! fullpath.empty();
    if (result)
    {
        play_list_t & plist = m_current_list->second;
        song_list & slist = plist.ls_song_list;
        int index = (-1);                  // how to get next available index?
        int midinumber = (-1);
        std::string basename;
        std::string directory;
        (void) filename_split(fullpath, directory, basename);
        last_song_indices(slist, index, midinumber);
        if (directory == plist.ls_file_directory)
        {
            result = add_song(index, midinumber, basename, directory);
        }
        else
        {
            std::string dir;
            result = add_song(index, midinumber, fullpath, dir);
        }
    }
    return result;
}

bool
playlist::modify_song
(
    int index, int midinumber,
    const std::string & name,
    const std::string & directory
)
{
    bool result = ctrl_is_valid(midinumber);
    if (result)
        result = m_current_list != m_play_lists.end();

    if (result)
    {
        play_list_t & plist = m_current_list->second;
        song_list & slist = plist.ls_song_list;
        if (m_current_song != slist.end())
        {
            /*
             * We must make a copy of the original, which gets deleted.
             */

            song_spec_t sspec = m_current_song->second;
            sspec.ss_index = index;
            sspec.ss_midi_number = midinumber;
            sspec.ss_song_directory = directory;
            sspec.ss_filename = name;
            if (remove_song(index))
                result = add_song(sspec);       /* add song, reorder list   */
        }
    }
    return result;
}

/**
 *  Looks up the largest index and MIDI control number, and returns one higher
 *  for each.
 */

void
playlist::last_song_indices (song_list & slist, int & index, int & midinumber)
{
    int indexmax = (-1);
    int midimax = (-1);
    for (auto sci = slist.begin(); sci != slist.end(); ++sci)
    {
        const song_spec_t & s = sci->second;
        if (s.ss_midi_number > midimax)
            midimax = s.ss_midi_number;

        if (s.ss_index > indexmax)
            indexmax = s.ss_index;
    }
    midinumber = midimax == (-1) ? 0 : midimax + 1 ;
    index = indexmax == (-1) ? 0 : indexmax + 1 ;
}

/**
 *  This function removes a song from the current playlist at the given index.
 *  The index is an ordinal, not a key, therefore we have to iterate through
 *  the whole list until we encounter the desired index.  Useful for removing
 *  the selected song in a table.
 *
 *  This function works by iterating to the index'th element in the song-list
 *  and deleting it.
 *
 * \param index
 *      The ordinal value (not a key) of the desired table row.
 *
 * \return
 *      Returns true if the desired song was found and removed.
 */

bool
playlist::remove_song (int index)
{
    bool result = false;
    if (m_current_list != m_play_lists.end())
    {
        int count = 0;
        play_list_t & plist = m_current_list->second;
        song_list & slist = plist.ls_song_list;
        for (auto sci = slist.begin(); sci != slist.end(); ++sci, ++count)
        {
            if (count == index)
            {
                (void) slist.erase(sci);
                --plist.ls_song_count;
                result = true;
                break;
            }
        }
        if (result)
            reorder_song_list(slist);
    }
    return result;
}

/**
 *  Moves through the song-list container in key (MIDI control number) order,
 *  modifying the index value of each song in the list.
 */

void
playlist::reorder_song_list (song_list & sl)
{
    int index = 0;
    for (auto sci = sl.begin(); sci != sl.end(); ++sci, ++index)
    {
        song_spec_t & s = sci->second;
        s.ss_index = index;
    }
}

/**
 *  Shows a summary of a playlist.
 *
 * \param pl
 *      The playlist structure to show.
 */

void
playlist::show_list (const play_list_t & pl) const
{
#if defined USE_OLD_CODE
    std::cout
        << "    Playlist MIDI #" << pl.ls_midi_number
        << ", slot " << pl.ls_index
        << ": '" << pl.ls_list_name << "'"
        << std::endl
        << "    " << pl.ls_file_directory
        << " " << pl.ls_song_count << " songs"
        << std::endl
        ;
#else
    char temp[80];
    (void) snprintf
    (
        temp, sizeof temp, "Playlist MIDI #%d, slot %d: '%s'",
        int(pl.ls_midi_number), int(pl.ls_index), pl.ls_list_name.c_str()
    );
    info_message(temp);
    (void) snprintf
    (
        temp, sizeof temp, "%s, %d songs",
        pl.ls_file_directory.c_str(), int(pl.ls_song_count)
    );
    info_message(temp);
#endif
}

/**
 *  Shows a summary of a song. The directory of the song is currently not
 *  shown because it makes the summary more difficult to read in the CLI
 *  output.
 *
 * \param s
 *      The song-specification structure to show.
 */

void
playlist::show_song (const song_spec_t & s) const
{
#if defined USE_OLD_CODE
    std::cout
        << "    Song MIDI #" << s.ss_midi_number << ", slot " << s.ss_index
        << ": " /* << s.ss_song_directory */ << s.ss_filename
        << std::endl
        ;
#else
    char temp[80];
    (void) snprintf
    (
        temp, sizeof temp,
        "Song MIDI #%d, slot %d: '%s'",
        int(s.ss_midi_number), int(s.ss_index), s.ss_filename.c_str()
    );
    info_message(temp);
#endif
}

/**
 *  Performs a simple dump of the playlists, mostly for troubleshooting.
 */

void
playlist::show () const
{
    if (m_play_lists.empty())
    {
        printf("No items in playist.\n");
    }
    else
    {
        for (auto pci = m_play_lists.cbegin(); pci != m_play_lists.cend(); ++pci)
        {
            const play_list_t & pl = pci->second;
            show_list(pl);

            const song_list & sl = pl.ls_song_list;
            for (auto sci = sl.cbegin(); sci != sl.cend(); ++sci)
            {
                const song_spec_t & s = sci->second;
                show_song(s);
            }
        }
    }
}

/**
 *  A function for running tests of the play-list handling.  Normally
 *  not needed.
 */

void
playlist::test ()
{
    show();
    show_list(m_current_list->second);
    show_song(m_current_song->second);
    for (int i = 0; i < 8; ++i)
    {
        if (next_song())
        {
            std::cout << "Next song: ";
            show_song(m_current_song->second);
        }
        else
            break;
    }
    for (int i = 0; i < 8; ++i)
    {
        if (previous_song())
        {
            std::cout << "Prev song: ";
            show_song(m_current_song->second);
        }
        else
            break;
    }
    for (int i = 0; i < 8; ++i)
    {
        if (next_list())
        {
            std::cout << "Next list: ";
            show_list(m_current_list->second);
        }
        else
            break;
    }
    for (int i = 0; i < 8; ++i)
    {
        if (previous_list())
        {
            std::cout << "Prev list: ";
            show_list(m_current_list->second);
        }
        else
            break;
    }

    /*
     * reset_list();
     * write();
     */
}

}           // namespace seq66

/*
 * playlist.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


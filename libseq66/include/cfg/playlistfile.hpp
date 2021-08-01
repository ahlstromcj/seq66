#if ! defined SEQ66_PLAYLISTFILE_HPP
#define SEQ66_PLAYLISTFILE_HPP

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
 * \file          playlistfile.hpp
 *
 *  This module declares/defines the class for playlist file I/O.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2020-09-19
 * \updates       2021-06-09
 * \license       GNU GPLv2 or above
 *
 */

#include "cfg/configfile.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

    class playlist;

/**
 *  Provides a file for reading and writing the application' main
 *  configuration file.
 */

class playlistfile final : public configfile
{

    /**
     *  The playlist object to be filled or read from.
     */

    playlist & m_play_list;

    /**
     *  If true, write the lists/songs to standard output.  This is
     *  useful to test the CLI/daemon version of Seq66.
     */

    bool m_show_on_stdout;

public:

    /*
     * Only the friend class performer is able to call this function.
     */

    playlistfile
    (
        const std::string & filename,
        playlist & pl,
        rcsettings & rcs,
        bool show_on_stdout = false
    );

    playlistfile () = delete;
    playlistfile (const playlistfile &) = delete;
    playlistfile & operator = (const playlistfile &) = delete;

    /*
     * WTF?
     *
    playlistfile (playlistfile &&) = default;
    playlistfile & operator = (playlistfile &&) = default;
     */

    virtual ~playlistfile ();               // how to hide this???

    virtual bool parse ();
    virtual bool write ();

    void clear ();
    bool open (bool verify_it = true);

private:

    playlist & play_list()
    {
        return m_play_list;
    }

    bool set_error_message (const std::string & additional);
    bool scan_song_file (int & song_number, std::string & song_file);

};          // class playlistfile

/*
 *  Free functions for working with play-list files.
 */

extern bool open_playlist
(
    playlist & pl,
    const std::string & source,
    bool show_on_stdout = false
);
extern bool save_playlist
(
    playlist & pl,
    const std::string & destfile
);
extern bool save_playlist
(
    playlist & pl,
    const std::string & source,
    const std::string & destination
);
extern bool copy_playlist_songs
(
    playlist & pl,
    const std::string & source,
    const std::string & destination
);

}           // namespace seq66

#endif      // SEQ66_PLAYLISTFILE_HPP

/*
 * playlistfile.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


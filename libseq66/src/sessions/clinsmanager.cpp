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
 * \file          clinsmanager.cpp
 *
 *  This module declares/defines the main module for the Non Session Manager
 *  control of seq66cli and qseq66.
 *
 * \library       clinsmanager application
 * \author        Chris Ahlstrom
 * \date          2020-08-31
 * \updates       2020-10-17
 * \license       GNU GPLv2 or above
 *
 *  This object also works if there is no session manager in the build.  It
 *  handles non-session startup as well.
 */


#include "cfg/cmdlineopts.hpp"          /* command-line functions           */
#include "cfg/settings.hpp"             /* seq66::usr() and seq66::rc()     */
#include "midi/midifile.hpp"            /* seq66::write_midi_file()         */
#include "os/daemonize.hpp"             /* seq66::pid_exists()              */
#include "play/playlist.hpp"            /* seq66::playlist class            */
#include "cfg/playlistfile.hpp"         /* seq66::playlistfile class        */
#include "sessions/clinsmanager.hpp"    /* seq66::clinsmanager class        */
#include "util/filefunctions.hpp"       /* seq66::make_directory_path()     */

#if defined SEQ66_NSM_SUPPORT
#include "nsm/nsmmessagesex.hpp"        /* seq66::nsm access functions      */
#endif

namespace seq66
{

/*
 *-------------------------------------------------------------------------
 * clinsmanager
 *-------------------------------------------------------------------------
 */

/**
 *  Note that this object is created before there is any chance to get the
 *  configuration, because the smanager base class is what gets the
 *  configuration, well after this constructor.
 */

clinsmanager::clinsmanager (const std::string & caps) :
    smanager        (caps),
#if defined SEQ66_NSM_SUPPORT
    m_nsm_client    (),
#endif
    m_nsm_active    (false)
{
    // no code
}

/**
 *
 */

clinsmanager::~clinsmanager ()
{
    // no code yet
}

/**
 *  This function first determines if the user wants an NSM session.  If so,
 *  it determines if it can get a valid NSM_URL environment variable.  If not,
 *  that may simply be due to nsmd running in different console window or as a
 *  daemon.  In that cause, it checks if there was a non-empty
 *  "[user-session]" "url" value.  This is useful in trouble-shooting, for
 *  example.  Finally, just in case the user has set up the "usr" file for
 *  running NSM, but hasn't started non-session-manager or nsmd itself, we use
 *  the new pid_exists() function to make sure that nsmd is indeed running.
 *  Ay!
 *
 *  If all is well, a new nsmclient is created, and an announce/open handshake
 *  starts.
 *
 *  This function is called before create_window().
 */

bool
clinsmanager::create_session (int argc, char * argv [])
{
#if defined SEQ66_NSM_SUPPORT
    std::string url;
    bool testing = false;
    bool ok = usr().wants_nsm_session();            /* user wants NSM usage */
    if (ok)
    {
        infoprint("Checking 'usr' file for NSM URL");
        url = usr().session_url();                  /* try 'usr' file's URL */
        if (url.empty())
        {
            warnprint("Checking for NSM_URL in environment");
            url = nsm::get_url();
        }
        ok = ! url.empty();                         /* we got an NSM URL    */
        testing = url == "testing";
        if (ok && ! testing)                        /* for testing!         */
        {
            ok = pid_exists("nsmd");                /* one final check      */
            if (! ok)
                warnprint("nsmd not running, proceeding with normal run");
        }
    }
    if (ok)
    {
        std::string nsmfile = "dummy/file";
        std::string nsmext = nsm::default_ext();
        m_nsm_client.reset(create_nsmclient(*this, url, nsmfile, nsmext));
        bool result = bool(m_nsm_client);
        if (result)
        {
            /*
             * Use the same name as provided when opening the JACK client.
             */

            std::string appname = seq_client_name();    /* "seq66"  */
            std::string exename = seq_arg_0();          /* "qseq66" */
            result = m_nsm_client->announce(appname, exename, capabilities());
            if (! result)
                file_error("Create session", "failed to announce");
        }
        else
            file_error("Create session", "failed to make client");

        if (testing)
            result = true;

        nsm_active(result);                             /* class flag       */
        usr().in_session(result);                       /* global flag      */
        if (result)
            result = smanager::create_session(argc, argv);

        return result;
    }
    else
    {
        return smanager::create_session(argc, argv);
    }
#else
    return smanager::create_session(argc, argv);
#endif
}

/**
 *  Will do more with this later.
 */

bool
clinsmanager::close_session (bool ok)
{
    if (usr().in_session())
    {
        warnprint("Closing NSM session");
    }
    return smanager::close_session(ok);
}

bool
clinsmanager::save_session (std::string & msg)
{
    bool result = not_nullptr(perf());
    msg.clear();
    if (result)
    {
        std::string filename = rc().midi_filename();
        if (filename.empty())
        {
            warnprint("NSM session: MIDI file-name empty, will not save");
        }
        else
        {
            bool is_wrk = file_extension_match(filename, "wrk");
            if (is_wrk)
                filename = file_extension_set(filename, ".midi");

            result = write_midi_file(*perf(), filename, msg);
            if (result)
            {
                show_message("Saved", filename);
            }
            else
            {
                show_error("No MIDI tracks, cannot save", filename);
            }
        }
        result = smanager::save_session(msg);
    }
    else
    {
        msg = "no performer present to save session";
    }
    return result;
}

/**
 *
 */

bool
clinsmanager::run ()
{
    // see the while (! seq66::session_close()) loop

#if defined THIS_CODE_IS_READY

    session_setup();
    while (! session_close())
    {
        if (session_save())
            save_file(perf());

        microsleep(1000);                       /* 1 ms */
    }
#endif

    return false;
}

/**
 *  Creates a session path specified by the Non Session Manager.  This
 *  function is meant to be called after receiving the /nsm/client/open
 *  message.
 *
 *  A sample session path:
 *
 *      /home/ahlstrom/NSM Sessions/QSeq66 Installed/seq66.nYMVC
 *
 *  The NSM daemon creates the directory for this project after dropping the
 *  client ID (seq66.nYMVC).  We append the client ID and create this
 *  directory, followng the lead of Non-Mixer and Qtractor.
 *
 *  Note:
 *
 *      At this point, the performer [perf()] does not yet exist!
 *
 * \param argc
 *      The command-line argument count.
 *
 * \param argv
 *      The command-line argument list.
 *
 * \param path
 *      The base configuration path.  For NSM usage, this will be a directory
 *      for the project in the NSM session directory created by nsmd.  The
 *      sub-directories "config" and "midi" will be created for use by NSM.
 *      For normal usage, this directory will be the standard home directory
 *      or the one specified by the --home option.
 */

bool
clinsmanager::create_project
(
    int argc, char * argv [],
    const std::string & path
)
{
    bool result = ! path.empty();
    if (result)
    {
        /*
         * See if the configuration has already been created, using the "rc"
         * file as the test case.  The normal base-name (e.g. "qseq66") is
         * always used in an NSM session.  We will read/write the configuration
         * from the NSM path.  We assume (for now) that the "midi" directory was
         * also created.
         */

        std::string cfgfilepath = path;
        std::string midifilepath = path;
        if (nsm_active())
        {
            cfgfilepath += "/config/";
            midifilepath += "/midi/";
        }
        else
        {
            midifilepath.clear();
            rc().midi_filename(midifilepath);
        }

        std::string rcfile = cfgfilepath + rc().config_filename();
        bool already_created = file_exists(rcfile);
        if (already_created)
        {
            file_message("File exists", rcfile);        /* comforting       */
            rc().full_config_directory(cfgfilepath);    /* set NSM dir      */
            rc().midi_filepath(midifilepath);           /* set MIDI dir     */
            if (! midifilepath.empty())
            {
                file_message("NSM MIDI file path", rc().midi_filepath());
                file_message("NSM MIDI file name", rc().midi_filename());
            }

            std::string errmessage;
            result = cmdlineopts::parse_options_files(errmessage);
            if (result)
            {
                /*
                 * Perhaps at some point, the "rc"/"usr" options might affect
                 * NSM usage.  In the meantime, we still need command-line
                 * options, if present, to override the file-specified
                 * options.  One big example is the --buss override. The
                 * smanager::main_settings() function is called way before
                 * create_project();
                 */

                if (argc > 1)
                {
                    (void) cmdlineopts::parse_command_line_options(argc, argv);
                    (void) cmdlineopts::parse_o_options(argc, argv);
                }
            }
            else
            {
                file_error(errmessage, rc().config_filespec());
            }
        }
        else
        {
            std::string fullpath = path;
            result = make_directory_path(fullpath);
            if (result)
            {
                result = make_directory_path(cfgfilepath);
                if (result)
                    rc().full_config_directory(cfgfilepath);
            }
            if (result && ! midifilepath.empty())
            {
                result = make_directory_path(midifilepath);
                if (result)
                {
                    /*
                     * We don't want to do this yet, since we might need to
                     * copy MIDI files from the original
                     * rc().midi_base_directory() directory.
                     *
                     * rc().midi_filepath(midifilepath);
                     * rc().midi_base_directory(midifilepath);
                     */
                }
            }
            if (result)
            {
                /*
                 * Make sure the configuration is created in the session right
                 * now, in case the session manager tells the app to quit
                 * (without saving).  We need to replace the path part, if
                 * any, in the playlist and notemapper file-names, and prepend
                 * the new home directory.  Note that, at this point, the
                 * performer has not yet been created.
                 */

                std::string srcplayfile = rc().playlist_filename();
                std::string srcnotefile = rc().notemap_filename();
                if (srcplayfile.empty())
                    srcplayfile = "empty.playlist";

                if (srcnotefile.empty())
                    srcnotefile = "empty.drums";

                std::string dstplayfile = file_path_set(srcplayfile, cfgfilepath);
                std::string dstnotefile = file_path_set(srcnotefile, cfgfilepath);
                warnprint("Saving initial config files to session directory");
                usr().save_user_config(true);
                rc().playlist_filename(dstplayfile);
                rc().notemap_filename(dstnotefile);
                result = cmdlineopts::write_options_files();
                if (result)
                {
                    if (rc().playlist_active())
                    {
                        warnprint("Play-list not active, saving anyway");
                    }
                    if (dstplayfile.empty())
                    {
                        warnprint("Play-list name empty");
                    }
                    else
                    {
                        std::string s("Temp");
                        performer * p(nullptr);
                        std::shared_ptr<playlist> plp;
                        plp.reset(new (std::nothrow) playlist(p, s, false));
                        result = bool(plp);
                        if (result)
                        {
                            result = save_playlist
                            (
                                *plp, srcplayfile, dstplayfile
                            );
                        }
                        if (result && ! midifilepath.empty())
                        {
                            result = copy_playlist_songs
                            (
                                *plp, srcplayfile, midifilepath
                            );
                        }
                        if (result)
                        {
                            /*
                             * The first is where MIDI files are stored in the
                             * session, and the second is where they are
                             * stored for the play-list.  Currently, the same
                             * directory.  Not considered to be an issue at
                             * this time.
                             */

                            rc().midi_filepath(midifilepath);
                            rc().midi_base_directory(midifilepath);
                        }
                    }
                }
#if defined THIS_CODE_IS_READY
                if (rc().notemap_active())
                {
                    if (perf()->notemap_exists())
                    {
                        std::string destination = rc().notemap_filename();
                        file_message("Note-mapper save", destination);
                        result = perf()->save_note_mapper(destination);
                    }
                }
                else
                {
                    warnprint("Note-mapper is not active");
                }
#endif
            }
        }
    }
    return result;
}

void
clinsmanager::session_manager_name (const std::string & mgrname)
{
    smanager::session_manager_name(mgrname);
    if (! mgrname.empty())
        file_message("CNS", mgrname);
}

void
clinsmanager::session_manager_path (const std::string & pathname)
{
    smanager::session_manager_path(pathname);
    if (! pathname.empty())
        file_message("CNS", pathname);
}

void
clinsmanager::session_display_name (const std::string & dispname)
{
    smanager::session_display_name(dispname);
    if (! dispname.empty())
        file_message("CNS", dispname);
}

void
clinsmanager::session_client_id (const std::string & clid)
{
    smanager::session_client_id(clid);
    if (! clid.empty())
        file_message("CNS", clid);
}

/**
 *  Shows the collected messages in the console, and recommends the user
 *  exit and check the configuration.
 */

void
clinsmanager::show_error
(
    const std::string & tag,
    const std::string & msg
) const
{
    if (msg.empty())
    {
#if defined SEQ66_PORTMIDI_SUPPORT
        if (Pm_error_present())
        {
            std::string pmerrmsg = std::string(Pm_error_message());
            append_error_message(pmerrmsg);
        }
#endif
        std::string msg = error_message();
        msg += "Please exit and fix the configuration.";
        show_message(tag, msg);
    }
    else
    {
        append_error_message(msg);
        show_message(tag, msg);
    }
}

}           // namespace seq66

/*
 * clinsmanager.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


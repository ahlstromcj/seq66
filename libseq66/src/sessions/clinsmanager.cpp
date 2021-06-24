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
 * \file          clinsmanager.cpp
 *
 *  This module declares/defines the main module for the Non Session Manager
 *  control of seq66cli and qseq66.
 *
 * \library       clinsmanager application
 * \author        Chris Ahlstrom
 * \date          2020-08-31
 * \updates       2021-06-24
 * \license       GNU GPLv2 or above
 *
 *  This object also works if there is no session manager in the build.  It
 *  handles non-session startup as well.
 */

#include "cfg/cmdlineopts.hpp"          /* command-line functions           */
#include "cfg/notemapfile.hpp"          /* seq66::notemapfile               */
#include "cfg/playlistfile.hpp"         /* seq66::playlistfile class        */
#include "cfg/settings.hpp"             /* seq66::usr() and seq66::rc()     */
#include "midi/midifile.hpp"            /* seq66::write_midi_file()         */
#include "os/daemonize.hpp"             /* seq66::session_setup(), _close() */
#include "os/timing.hpp"                /* seq66::microsleep()              */
#include "play/playlist.hpp"            /* seq66::playlist class            */
#include "sessions/clinsmanager.hpp"    /* seq66::clinsmanager class        */
#include "util/filefunctions.hpp"       /* seq66::make_directory_path()     */
#include "util/strfunctions.hpp"        /* seq66::contains()                */

#if defined SEQ66_NSM_SUPPORT
#include "nsm/nsmmessagesex.hpp"        /* seq66::nsm access functions      */
#endif

namespace seq66
{

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

clinsmanager::~clinsmanager ()
{
    // no code yet
}

/**
 *  Detects if the 'usr' file defines usage of NSM and a valid URL for the
 *  nsmd daemon, or, if not, is there an NSM URL in the environment. Also, if
 *  there is an NSM URL in the environment, it overrides the one specified in
 *  the 'usr' file.
 *
 * \param [out] url
 *      Holds the URL that was found.  Use it only if this function returns
 *      true.
 *
 * \return
 *      Returns true if a usable NSM URL was found and nsmd was found to be
 *      running.
 */

bool
clinsmanager::detect_session (std::string & url)
{
#if defined SEQ66_NSM_SUPPORT
    bool result = usr().wants_nsm_session();        /* user wants NSM usage */
    std::string tenturl;                            /* a tentative URL      */
    url.clear();
    if (result)
    {
        infoprint("Checking 'usr' file for NSM URL");
        tenturl = usr().session_url();              /* try 'usr' file's URL */
        if (! tenturl.empty())                      /* configured NSM URL   */
        {
            /*
             * Sanity check the URL: "osc.udp://hostname.domain:PORT#"
             */

            result = contains(tenturl, "osc.udp://");
            if (result)
                url = tenturl;
            else
                tenturl.clear();
        }
    }
    if (tenturl.empty())                            /* configured NSM URL   */
    {
        warnprint("Checking environment for NSM_URL");
        tenturl = nsm::get_url();
        result = ! tenturl.empty();
        if (result)
            url = tenturl;
    }
    if (result)
        file_message("NSM URL", tenturl);           /* comforting message   */

    return result;
#else
    url.clear();
    return false;
#endif
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
    bool ok = detect_session(url);                  /* side-effect          */
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

        if (url == "testing")
            result = true;

        nsm_active(result);                             /* class flag       */
        usr().in_session(result);                       /* global flag      */
        (void) smanager::create_session(argc, argv);
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
 *  Somewhat of the inverse of create_session().
 */

bool
clinsmanager::close_session (std::string & msg, bool ok)
{
#if defined SEQ66_NSM_SUPPORT
    if (usr().in_session())
    {
        warnprint("Closing NSM session");
        nsm_active(false);                              /* class flag       */
        usr().in_session(false);                        /* global flag      */
        m_nsm_client->close_session();

        /*
         * Freezes in the lo_server_thread_stop() call: m_nsm_client.reset();
         */
    }
#endif
    return smanager::close_session(msg, ok);
}

/**
 *  Somewhat of the inverse of create_session().
 */

bool
clinsmanager::detach_session (std::string & msg, bool ok)
{
#if defined SEQ66_NSM_SUPPORT
    if (usr().in_session())
    {
        warnprint("Detaching (closing) NSM session");
        nsm_active(false);                              /* class flag       */
        m_nsm_client->detach_session();

        /*
         * Freezes in the lo_server_thread_stop() call: m_nsm_client.reset();
         */
    }
#endif
    return smanager::detach_session(msg, ok);
}

/**
 *  Saves the active MIDI file, and then calls the base-class version of
 *  save_session().
 */

bool
clinsmanager::save_session (std::string & msg, bool ok)
{
    bool result = not_nullptr(perf());
    if (ok)
        msg.clear();

    if (result)
    {
        if (perf()->modified())
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
                    /*
                     * Only show the message if not running under a session
                     * manager.  This is because the message-box will hang the
                     * application until the user clicks OK.
                     */

                    if (! nsm_active())
                        show_message("Saved", filename);
                }
                else
                {
                    show_error("No MIDI tracks, cannot save", filename);
                }
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
 *  This function is useful in the command-line version of the application.
 *  For the Qt version, see the qt5nsmanager class.
 */

bool
clinsmanager::run ()
{
    bool result = false;
    session_setup();
    while (! session_close())
    {
        result = true;
        if (session_save())
        {
            std::string msg;
            result = save_session(msg, true);
            if (! result)
            {
                file_error(msg, "CLI");
            }
        }
        microsleep(1000);                       /* 1 ms */
    }
    return true;
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
 *  Note: At this point, the performer [perf()] does not yet exist!
 *
 *  The first thing to do is grab the current (and default) configuration
 *  directory (in the user's HOME area).  We may need it, in order to find the
 *  original files to recreate.  Next we see if the configuration has already
 *  been created, using the "rc" file as the test case.  The normal base-name
 *  (e.g. "qseq66") is always used in an NSM session.  We will read/write the
 *  configuration from the NSM path.  We assume (for now) that the "midi"
 *  directory was also created.
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
        std::string cfgfilepath = path;
        std::string midifilepath = path;
        std::string homepath = rc().home_config_directory();
        if (nsm_active())
        {
            cfgfilepath = pathname_concatenate(cfgfilepath, "config");
            midifilepath = pathname_concatenate(midifilepath, "midi");
        }
        else
            midifilepath.clear();

        std::string rcfile = cfgfilepath + rc().config_filename();
        bool already_created = file_exists(rcfile);
        rc().midi_filepath(midifilepath);               /* do this first    */
        if (already_created)
        {
            file_message("File exists", rcfile);        /* comforting       */
            result = read_configuration(argc, argv, cfgfilepath, midifilepath);
            if (result)
            {
                if (usr().in_session())
                    rc().auto_option_save(true);
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
                result = make_directory_path(midifilepath);

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
                file_message
                (
                    "Saving initial configuration to session directory",
                    cfgfilepath
                );
                usr().save_user_config(true);
                rc().playlist_filename(dstplayfile);
                rc().notemap_filename(dstnotefile);
                if (usr().in_session())
                    rc().auto_option_save(true);

                result = cmdlineopts::write_options_files();
                if (result)
                {
                    if (! rc().playlist_active())
                    {
                        warnprint("Play-list inactive, saving anyway");
                    }
                    if (dstplayfile.empty())
                    {
                        file_error("Play-list file", "none");
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
                            srcplayfile = file_path_set(srcplayfile, homepath);
                            (void) save_playlist
                            (
                                *plp, srcplayfile, dstplayfile
                            );
                            if (! midifilepath.empty())
                            {
                                (void) copy_playlist_songs
                                (
                                    *plp, srcplayfile, midifilepath
                                );
                            }

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
                if (result)
                {
                    if (! rc().notemap_active())
                    {
                        warnprint("Note-map not active, saving anyway");
                    }

                    std::string destination = rc().notemap_filename();
                    if (destination.empty())
                    {
                        warnprint("Note-map file name empty");
                    }
                    else
                    {
                        std::shared_ptr<notemapper> nmp;
                        nmp.reset(new (std::nothrow) notemapper());
                        result = bool(nmp);
                        file_message("Note-mapper save", destination);
                        srcnotefile = file_path_set(srcnotefile, homepath);
                        (void) copy_notemapper(*nmp, srcnotefile, destination);
                    }
                }
            }
        }
    }
#if defined SEQ66_NSM_SUPPORT
    if (m_nsm_client)
        (void) m_nsm_client->open_reply(result);            /* issue #28 */
#endif

    return result;
}

bool
clinsmanager::read_configuration
(
    int argc, char * argv [],
    const std::string & cfgfilepath,
    const std::string & midifilepath
)
{
    rc().full_config_directory(cfgfilepath);    /* set NSM dir      */
    rc().midi_filepath(midifilepath);           /* set MIDI dir     */
    if (! midifilepath.empty())
    {
        file_message("NSM MIDI file path", rc().midi_filepath());
        file_message("NSM MIDI file name", rc().midi_filename());
    }

    std::string errmessage;
    bool result = cmdlineopts::parse_options_files(errmessage);
    if (result)
    {
        /*
         * Perhaps at some point, the "rc"/"usr" options might affect NSM
         * usage.  In the meantime, we still need command-line options, if
         * present, to override the file-specified options.  One big example
         * is the --buss override. The smanager::main_settings() function is
         * called way before create_project();
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
    return result;
}

void
clinsmanager::session_manager_name (const std::string & mgrname)
{
    smanager::session_manager_name(mgrname);
    if (! mgrname.empty())
        file_message("CNS manager", mgrname);
}

void
clinsmanager::session_manager_path (const std::string & pathname)
{
    smanager::session_manager_path(pathname);
    if (! pathname.empty())
        file_message("CNS path", pathname);
}

void
clinsmanager::session_display_name (const std::string & dispname)
{
    smanager::session_display_name(dispname);
    if (! dispname.empty())
        file_message("CNS name", dispname);
}

void
clinsmanager::session_client_id (const std::string & clid)
{
    smanager::session_client_id(clid);
    if (! clid.empty())
        file_message("CNS client ID", clid);
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


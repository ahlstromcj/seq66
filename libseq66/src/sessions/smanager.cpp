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
 * \file          smanager.cpp
 *
 *  This module declares/defines a module for managing a generic seq66 session.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2020-03-22
 * \updates       2020-04-09
 * \license       GNU GPLv2 or above
 *
 *  The process:
 *
 *      1. Call main_settings(argc, argv).  It sets defaults, does some parsing
 *         of command-line options and files.  It saves the MIDI file-name, if
 *         provided.
 *      2. Call create_performer(), which could delete an existing performer.
 *         It then launches the performer.  Save the unique-pointer.
 *      3. Call open_playlist().  It will open it, if specified and possible.
 *      4. If the MIDI file-name is set, open it via a call to open_midi_file().
 *      5. If a user-interface is needed, create a unique-pointer to it, then
 *         show it.  This will remove any previous pointer.  The function is
 *         virtual, create_window().
 *
 *  PortMidi:
 *
 *      PortMidi handling shall be done by the main() of the application. ???
 */

#include "seq66_features.hpp"           /* set_app_name()                   */

#include "cfg/cmdlineopts.hpp"          /* command-line functions           */
#include "cfg/settings.hpp"             /* seq66::usr() and seq66::rc()     */
#include "play/performer.hpp"           /* seq66::performer                 */
#include "sessions/smanager.hpp"        /* seq66::smanager()                */
#include "unix/daemonize.hpp"           /* seq66::reroute_stdio()           */
#include "util/basic_macros.hpp"        /* seq66::msgprintf()               */
#include "util/filefunctions.hpp"       /* seq66::file_accessible()         */

#if defined SEQ66_LASH_SUPPORT_NO_LASH_HERE
#include "lash/lash.hpp"                /* seq66::lash_driver functions     */
#endif

#if defined SEQ66_PORTMIDI_SUPPORT
#include "portmidi.h"        /*  Pm_error_present(), Pm_hosterror_message()  */
#endif

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Does the usual construction.  It also calls set_defaults() from the
 *  settings.cpp module in order to guarantee that we have rc() and usr()
 *  available.  See that function for more information.
 */

smanager::smanager () :
    m_perf_pointer      (),
    m_midi_filename     (),
    m_extant_errmsg     ("unspecified error"),
    m_extant_msg_active (false)
{
    /*
     * This has to wait:
     *
     * m_perf_pointer = create_performer();
     */

    set_defaults();
}

/**
 *  The standard C/C++ entry point to this application.  The first thing is to
 *  set the various settings defaults, and then try to read the "user" and
 *  "rc" configuration files, in that order.  There are currently no options
 *  to change the names of those files.  If we add that code, we'll move the
 *  parsing code to where the configuration file-names are changed from the
 *  command-line.  The last thing is to override any other settings via the
 *  command-line parameters.
 *
 * \param argc
 *      The number of command-line parameters, including the name of the
 *      application as parameter 0.
 *
 * \param argv
 *      The array of pointers to the command-line parameters.
 *
 * \return
 *      Returns true if this code worked properly and was not a request for
 *      help/version information.
 */

bool
smanager::main_settings (int argc, char * argv [])
{
    bool result = false;                /* EXIT_FAILURE                 */
    set_app_name(SEQ66_APP_NAME);       /* "qseq66" by default          */

    /*
     * -o log=file.ext early
     */

    (void) cmdlineopts::parse_log_option(argc, argv);

    /**
     * Set up objects that are specific to the GUI.  Pass them to the
     * performer constructor.  Then parse any command-line options to see if
     * they might affect what gets read from the 'rc' or 'user' configuration
     * files.  They will be parsed again later so that they can still override
     * whatever other settings were made via the configuration files.
     *
     * However, we currently have a issue where the mastermidibus created by
     * the performer object gets the default PPQN value, because the "user"
     * configuration file has not been read at that point.  See the
     * performer::launch() function.
     */

    (void) cmdlineopts::parse_command_line_options(argc, argv);
    bool is_help = cmdlineopts::help_check(argc, argv);
    int optionindex = -1;
    if (! is_help)
    {
        /*
         *  If parsing fails, report it and disable usage of the application
         *  and saving bad garbage out when exiting.  Still must launch,
         *  otherwise a segfault occurs via dependencies in the qsmainwnd.
         */

        std::string errmessage;                     /* just in case!        */
        result = cmdlineopts::parse_options_files(errmessage, argc, argv);
        if (! result)
        {
            errprintf("parse_options_files(): %s\n", errmessage.c_str());
        }
        optionindex = cmdlineopts::parse_command_line_options(argc, argv);
        if (cmdlineopts::parse_o_options(argc, argv))
        {
            /**
             * The user may have specified -o options that are also set up in
             * the "usr" file.  The command line needs to take precedence.  The
             * "log" option is processed early in the startup sequence.  These
             * same settings are made in the cmdlineopts module.
             */

            std::string logfile = usr().option_logfile();
            if (usr().option_use_logfile() && ! logfile.empty())
                (void) reroute_stdio(logfile);
        }
        if (result)
        {
            m_midi_filename.clear();
            if (optionindex < argc)                 /* MIDI filename given? */
            {
                std::string fname = argv[optionindex];
                std::string errmsg;
                if (file_accessible(fname))
                {
                    m_midi_filename = fname;
#if 0
        THIS CODE HAS TO COME AFTER launch()
                    std::string resultname = open_midi_file(fname, errmsg);
                    result = ! resultname.empty();
                    set_error_message(errmsg);
#endif
                }
                else
                {
                    char temp[512];
                    (void) snprintf
                    (
                        temp, sizeof temp,
                        "? MIDI file error: '%s'", fname.c_str()
                    );
                    set_error_message(temp);        /* raises the message   */
                    m_midi_filename.clear();
                }
            }
        }
    }
    return result;
}

/**
 *  This function is currently meant to be called by the owner of this smanager.
 *  This call must occur before creating the application main window.
 *  Otherwise, seq66 will not register with LASH (if enabled) in a timely
 *  fashion.  Also, we always have to launch, even if an error occurred, to
 *  avoid a segfault and show at least a minimal message.  LASH support is now
 *  back in Seq66.
 */

bool
smanager::create_performer ()
{
    bool result = false;
    int ppqn = usr().midi_ppqn();
    int rows = usr().mainwnd_rows();
    int cols = usr().mainwnd_cols();
    pointer p(new (std::nothrow) performer(ppqn, rows, cols));
    result = bool(p);
    if (result)
    {
        if (rc().verbose())                     /* good for trouble-shoots  */
        {
            rc().key_controls().show();
            rc().midi_controls().show();
            rc().mute_groups().show();
        }
        result = p->get_settings(rc());
        if (result)
        {
            result = p->launch(usr().midi_ppqn());
            if (result)
            {
                /*
                 * Odd.  Cannot just use operator =, must move as well.
                 */

                m_perf_pointer = std::move(p);      /* change the ownership */
            }
            else
            {
                errprint("performer::launch() failed");
            }
        }
    }
    else
    {
        errprint("performer creation failed");
    }
    return result;
}

/**
 *  Opens a playlist, if specified.
 *
 * \return
 *      Returns true if a playlist was specified and successfully opened, or if
 *      there is no playlist called for.  In other words, true is returned if
 *      there is no error.
 */

bool
smanager::open_playlist ()
{
    bool result = not_nullptr(perf());
    if (result)
    {
        std::string errmsg = "unspecified error";
        std::string playlistname = rc().playlist_filespec();
        if (rc().playlist_active() && ! playlistname.empty())
        {
            result = perf()->open_playlist(playlistname, rc().verbose());
            if (result)
            {
                result = perf()->open_current_song();   /* p.playlist_test()    */
            }
            else
            {
                set_error_message(perf()->playlist_error_message());
                result = true;                          /* avoid early exit     */
            }
        }
    }
    else
    {
        set_error_message("open_playlist(): no performer");
    }
    return result;
}

/**
 *  Encapsulates opening the MIDI file, if specified (on the command-line).
 *
 * \return
 *      Returns the name of the MIDI file, if successful.  Otherwise, an empty
 *      string is returned.
 */

std::string
smanager::open_midi_file (const std::string & fname, std::string & errmsg)
{
    std::string midifname;                          /* start out blank      */
    if (file_accessible(fname))
    {
        int pp = perf()->ppqn();
        bool result = perf()->read_midi_file(fname, pp, errmsg);
        if (result)
        {
            std::string infomsg = "PPQN set to ";
            infomsg += std::to_string(pp);
            (void) seq66::info_message(infomsg);
            midifname = fname;
        }
        else
            (void) seq66::error_message(errmsg);
    }
    return midifname;
}

/**
 *
 */

bool
smanager::create_session ()
{
#if defined SEQ66_PLATFORM_LINUX
#if defined SEQ66_LASH_SUPPORT_NO_LASH_HERE
    if (rc().lash_support())
        create_lash_driver(p, argc, argv);
    else
#endif
        session_setup();
#endif
    return false;
}

bool
smanager::close_session ()
{
    perf()->finish();                           /* tear down performer      */
    perf()->put_settings(rc());                 /* copy latest settings     */
    if (true)                                   /* TODO */
    {
        (void) cmdlineopts::write_options_files("erroneous");
        if (error_active())
        {
            errprint(error_message().c_str());
        }
//      else if (! errmessage.empty())
//      {
//          errprint(errmessage.c_str());
//      }
    }
    else
    {
        if (rc().auto_option_save())
            (void) cmdlineopts::write_options_files();
        else
            printf("[auto-option-save off, not saving config files]\n");
    }

#if defined SEQ66_LASH_SUPPORT_NO_LASH_HERE
    if (rc().lash_support())
        delete_lash_driver();            /* deleted only exists      */
#endif

    return true;
}

/**
 *
 */

bool
smanager::create_window ()
{
    if (rc().verbose())
        perf()->show_patterns();

    return false;
}

void
smanager::set_error_message (const std::string & message)
{
    m_extant_errmsg = message;
    m_extant_msg_active = ! message.empty();
    if (m_extant_msg_active)
        printf("smanager error %s\n", message.c_str());
}


/*
 * Having this here after creating the main window may cause issue
 * #100, where ladish doesn't see seq66's ports in time.
 *
 *  perf()->launch(usr().midi_ppqn());
 *
 * We also check for any "fatal" PortMidi errors, so we can display
 * them.  But we still want to keep going, in order to at least
 * generate the log-files and configuration files to
 * C:/Users/me/AppData/Local/seq66 or ~/.config/seq66.
 */

/**
 *
 */

void
smanager::show_message (const std::string & msg)
{
    seq66::info_message(msg);
}

/**
 *
 */

void
smanager::show_error (const std::string & msg)
{
    seq66::error_message(msg);
}

#if defined SEQ66_PORTMIDI_SUPPORT

/**
 *
 * \return
 *      Returns true if there is an error.  In this case, the caller should show
 *      the error message.
 */

bool
smanager::portmidi_error_check (std::string & errmsg) const
{
    bool result = Pm_error_present();
    if (result)
    {
        set_error_message(std::string(Pm_hosterror_message()));
    }
    return result;
}

#endif

}           // namespace seq66

/*
 * smanager.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


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
 * \file          qt5nsmanager.cpp
 *
 *  This module declares/defines the main module for the Non Session Manager
 *  control of qseq66.
 *
 * \library       qt5nsmanager application
 * \author        Chris Ahlstrom
 * \date          2020-03-15
 * \updates       2025-04-10
 * \license       GNU GPLv2 or above
 *
 *  Duty now for the future! Join the Smart Patrol!
 */

#include <QApplication>                 /* QApplication etc.                */
#include <QTimer>                       /* QTimer                           */
#include <QFile>

#include "cfg/settings.hpp"             /* seq66::usr() and seq66::rc()     */
#include "util/strfunctions.hpp"        /* seq66::string_replace()          */
#include "gui_palette_qt5.hpp"          /* seq66::gui_palette_qt5           */
#include "os/daemonize.hpp"             /* seq66::session_restart() check   */
#include "palettefile.hpp"              /* seq66::palette_file config file  */
#include "qt5nsmanager.hpp"             /* seq66::qt5nsmanager              */
#include "qsmainwnd.hpp"                /* seq66::qsmainwnd (main window)   */
#include "qt5_helpers.hpp"              /* seq66::qt() string conversion    */

#if defined SEQ66_NSM_SUPPORT
#include "nsm/nsmmessagesex.hpp"        /* seq66::nsm access functions      */
#endif

namespace seq66
{

/*
 *-------------------------------------------------------------------------
 * qt5nsmanager
 *-------------------------------------------------------------------------
 */

/**
 *  Note that this object is created before there is any chance to get the
 *  configuration, because the smanager base class is what gets the
 *  configuration, well after this constructor.
 */

qt5nsmanager::qt5nsmanager
(
    QApplication & app,
    QObject * parent,
    const std::string & caps
) :
    QObject         (parent),
    clinsmanager    (caps),
    m_application   (app),
    m_timer         (nullptr),
    m_window        (),
    m_was_hidden    (false)
{
#if defined QT_VERSION_STR
    set_qt_version(std::string(QT_VERSION_STR));
#endif
}

qt5nsmanager::~qt5nsmanager ()
{
    if (not_nullptr(m_timer))
        m_timer->stop();

    if (! is_help())
        (void) session_message("Exiting qt5nsmanager");
}

/**
 *  Here we will poll to see if the "dirty" status has changed, and tell the
 *  nsmclient to report it.
 *
 *  For visibility, we have the situation where either NSM or a MIDI control
 *  can toggle the desired visibility of the main window.  How can we tell
 *  which one requested the change?  The performer will set a "show-hide
 *  pending" variable if it made the request.
 */

void
qt5nsmanager::conditional_update ()
{
    if (not_nullptr(perf()))
    {
        if (perf()->modified() != last_dirty_status())
            set_last_dirty();

        if (perf()->show_hide_pending())
            handle_show_hide(perf()->hidden());
        else
            client_show_hide();

        if (session_restart())
            quit();
    }
}

/**
 *  Push the qsmainwnd window onto the stack.  Also be sure to pass along the
 *  PPQN value, which might be different than the default (192), and affects
 *  some of the child objects of qsmainwnd.
 *
 *  Also note the support for NSM; it changes the menus available in the main
 *  Seq66 window.  Note that we cannot use the nsm_active() call, because all
 *  that means is that the user wants NSM handling to be active.  But it isn't
 *  active until "in session", which means that the user wants NSM *and* NSM
 *  is running.
 *
 *  This function assumes that create_performer() has already been called.
 *  And this function is called before create_session().
 */

bool
qt5nsmanager::create_window ()
{
    bool result = not_nullptr(perf());
    if (result)
    {
        performer * p = perf();
        std::string mfname = midi_filename();
        bool usensm = usr().in_nsm_session();       /* not nsm_active()!    */
        if (rc().palette_active())
        {
            std::string palfile = rc().palette_filespec();
            if (! palfile.empty())
                (void) open_palette(global_palette(), palfile);
        }

        qsmainwnd * qm = new (std::nothrow) qsmainwnd(*p, mfname, usensm, this);
        result = not_nullptr(qm);
        if (result)
        {
            m_window.reset(qm);
#if defined SEQ66_NSM_SUPPORT
            if (not_nullptr(nsm_client()))
            {
                bool visible = usr().session_visibility();
                bool hidden = ! visible;
                std::string status = "NSM startup ";
                status += visible ? "visible" : "hidden" ;
                session_message(status);
                m_was_hidden = ! hidden;             /* force GUI show/hide  */
                handle_show_hide(hidden);
            }
            else
#endif
                show_gui();                         /* m_window->show()     */

            (void) smanager::create_window();       /* just house-keeping   */
            m_timer = qt_timer
            (
                this, "qt5nsmanager", 5, SLOT(conditional_update())
            );

            /*
             * Let the application stay active; let the user decide what to
             * do about the error here.  For example, if a playlist doesn't
             * load, why just exit? Should delay this message because other
             * messages are shown later in the process. For that matter,
             * we should allow otherwise normal operations so that the user
             * can recover.
             *
             * if (error_active()) show_error("", error_message()); else
             */

            std::string path;               /* config or session path   */
            std::string name;               /* config or session name   */
            std::string clid;               /* config/session client ID */
            if (usensm)
            {
                path = manager_path();      /* session manager path     */
                name = display_name();      /* session display name     */
                clid = client_id();         /* session client ID        */
            }
            else
            {
                bool have_uuid = ! rc().jack_session().empty();
                path = rc().home_config_directory();
                name = rc().config_filename();
                clid = rc().app_client_name();
                if (rc().jack_session_active())
                {
                    session_manager_name("JACK");
                    if (have_uuid)
                    {
                        clid += ":";
                        clid += rc().jack_session();    /* UUID alone   */
                    }
                }
#if 0   // see version 0.99.7
                else                                    /* 2023-11-05   */
#endif
                {
                    if (rc().alt_session())
                    {
                        std::string tag = "[";
                        tag += rc().session_tag();
                        tag += "]";
                        session_manager_name(tag);
                    }
                    if (have_uuid)                      /* wrong ???    */
                    {
                        clid += "/";
                        clid += rc().jack_session();    /* UUID alone   */
                    }
                }
            }
            m_window->session_manager(manager_name());
            m_window->session_path(path);
            m_window->session_display_name(name);
            m_window->session_client_id(clid);
            m_window->song_path(rc().midi_filename());
            m_window->last_used_dir(rc().last_used_dir());
            if (rc().investigate_disabled())
            {
                file_message("Session manager", manager_name());
                file_message("Session path", path);
                file_message("Session name", name);
                file_message("Session ID", clid);
                file_message("Session song path", rc().midi_filename());
            }
            set_session_url();
        }
    }
    if (result)
    {
        std::string themename = qt_icon_theme();
        if (! themename.empty())
            file_message("Icon theme", themename);

        if (rc().style_sheet_active())
        {
            std::string ssname = rc().style_sheet_filespec();
            if (! ssname.empty())
            {
                QFile file(qt(ssname));
                bool ok = file.open(QFile::ReadOnly);
                if (ok)
                {
                    QString ss = QLatin1String(file.readAll());
                    qApp->setStyleSheet(ss);
                }
            }
        }
    }
    return result;
}

/**
 *  Will do more with this later.  Currently we just call the base class.
 *  Note that the auto-palette-save() value will always be false.
 *  Saving the palette should be a manual option, as it is never changed
 *  during run-time.
 *
 *      bool savepalette = rc().palette_active() || rc().auto_palette_save();
 */

bool
qt5nsmanager::close_session (std::string & msg, bool ok)
{
    bool saved = true;
    bool savepalette = rc().auto_palette_save();
    if (savepalette)
    {
        std::string palfile = rc().palette_filespec();
        saved = save_palette(global_palette(), palfile);
    }

    /*
     * if (m_window)
     *     m_window->remove_all_editors();
     */

    bool closed = clinsmanager::close_session(msg, ok);
    return saved && closed;
}

/**
 *  Compare this function to clinsmanager::run(). We need to call
 *  session_setup(), otherwise Ctrl-C terminates the application peremptorily,
 *  potentially leaving the MIDI display device lit up instead of shut down.
 */

bool
qt5nsmanager::run ()
{
    bool restart = perf()->port_map_error(); // || perf()->new_ports_available();
    if (session_setup(restart))                 /* need an early exit?      */
    {
        int exit_status = m_application.exec(); /* run main window loop     */
        status_message("Early exit flagged by session setup");
        return exit_status == EXIT_SUCCESS;
    }
    else
        return true;                            /* see main()               */
}

/*
 *  Using the window's quit provides a prompt to save, and also avoids a
 *  segfault when exiting with changes pending.
 *
 *          m_application.quit();
 */

void
qt5nsmanager::quit ()
{
    if (nsm_active())
    {
        static int s_count = 0;
        if (s_count == 0)
        {
            ++s_count;
            session_message("Looping....");
        }
    }
    else
    {
        session_message("Quitting the session");
        m_window->quit();
    }
}

void
qt5nsmanager::show_message
(
    const std::string & tag,
    const std::string & msg
) const
{
    if (m_window && ! msg.empty())
    {
        bool quiet = rc().quiet() || usr().in_nsm_session();
        if (quiet)
        {
            smanager::show_message(tag, msg);
            perf()->clear_port_map_error();
        }
        else
        {
            std::string text = tag + ": " + msg;
            bool prompt = perf()->port_map_error() ||
                perf()->new_ports_available();

            bool yes = m_window->show_error_box_ex(text, prompt);
            if (yes)
            {
                /*
                 * We get here before the call to session_setup().
                 * So we check the condition above again in our run()
                 * function. See above.
                 */

                perf()->store_io_maps_and_restart();
            }
            else
                perf()->clear_port_map_error();
        }
    }
}

/**
 *  Shows the collected messages in the message-box, and recommends the user
 *  exit and check the configuration.
 */

void
qt5nsmanager::show_error
(
    const std::string & tag,
    const std::string & msg
) const
{
    if (m_window)
    {
        if (msg.empty())
        {

            /*
             * Consider moving this support to before this if-clause.
             */

#if defined SEQ66_PORTMIDI_SUPPORT
            if (Pm_error_present())
            {
                std::string pmerrmsg = std::string(Pm_error_message());
                append_error_message(pmerrmsg);
            }
#endif
            bool quiet = rc().quiet() || usr().in_nsm_session();
            if (quiet)
            {
                smanager::show_message(tag, msg);
            }
            else
            {
                std::string html = tag + " ";
                html += string_replace(error_message(), "\n", "<br>");
                html += "<br>Please exit and fix the configuration.";
                m_window->show_error_box(html);
            }
        }
        else
        {
#if defined SEQ66_PORTMIDI_SUPPORT
            if (Pm_error_present())
            {
                std::string pmerrmsg = std::string(Pm_error_message());
                append_error_message(pmerrmsg);
            }
#endif
            std::string text = tag;
            if (! tag.empty())
                text += " ";

            text += msg;
            if (rc().quiet())
            {
                smanager::show_message(tag, msg);
                perf()->clear_port_map_error();             /* keep going!  */
            }
            else
            {
                if (usr().in_nsm_session())
                {
                    (void) m_window->show_timed_error_box(text);
                    perf()->clear_port_map_error();
                }
                else
                {
                    bool doit = m_window->show_error_box_ex
                    (
                        text, perf()->port_map_error()
                    );
                    if (doit)
                        perf()->store_io_maps_and_restart();
                    else
                        perf()->clear_port_map_error();
                }
            }
        }
    }
}

void
qt5nsmanager::session_manager_name (const std::string & mgrname)
{
    clinsmanager::session_manager_name(mgrname);
    if (m_window)
        m_window->session_manager(mgrname.empty() ? "None" : mgrname);
}

void
qt5nsmanager::session_manager_path (const std::string & pathname)
{
    clinsmanager::session_manager_path(pathname);
    if (m_window)
        m_window->session_path(pathname.empty() ? "None" : pathname);
}

void
qt5nsmanager::session_display_name (const std::string & dispname)
{
    clinsmanager::session_display_name(dispname);
    if (m_window)
        m_window->session_display_name(dispname.empty() ? "None" : dispname);
}

void
qt5nsmanager::session_client_id (const std::string & clid)
{
    clinsmanager::session_client_id(clid);
    if (m_window)
        m_window->session_client_id(clid.empty() ? "None" : clid);
}

/*
 * Added in version 0.99.16 in order to clear the modified flag and
 * close editor windows.
 */

bool
qt5nsmanager::save_session (std::string & msg, bool ok)
{
    bool result = clinsmanager::save_session(msg, ok);
    if (m_window && ok)
        m_window->enable_save_update(false);

   return result;
}

void
qt5nsmanager::handle_show_hide (bool hide)
{
    if (hide != m_was_hidden)
    {
#if defined SEQ66_NSM_SUPPORT
        if (not_nullptr(nsm_client()))  /* are we under NSM control?        */
        {
            usr().session_visibility(! hide);
            rc().auto_usr_save(true);
        }
#endif
        m_was_hidden = hide;
        if (hide)
            hide_gui();
        else
            show_gui();
    }
}

void
qt5nsmanager::show_gui ()
{
    if (m_window)
    {
        perf()->hidden(false);          /* turns off show-hide pending flag */
        m_window->show();
        send_visibility(true);
    }
}

void
qt5nsmanager::hide_gui ()
{
    if (m_window)
    {
        perf()->hidden(true);          /* turns off show-hide pending flag  */
        m_window->hide();
        send_visibility(false);
    }
}

void
qt5nsmanager::send_visibility (bool visible)
{
    status_message(visible ? "GUI is showing..." : "GUI is hiding...");
#if defined SEQ66_NSM_SUPPORT
    if (not_nullptr(nsm_client()))
        nsm_client()->send_visibility(visible);
#endif
}

void
qt5nsmanager::set_last_dirty ()
{
    last_dirty_status(perf()->modified());

#if defined SEQ66_NSM_SUPPORT
    if (not_nullptr(nsm_client()))
        nsm_client()->dirty(last_dirty_status());
#endif
}

void
qt5nsmanager::client_show_hide ()
{
#if defined SEQ66_NSM_SUPPORT
    if (not_nullptr(nsm_client()))
        handle_show_hide(nsm_client()->hidden());
#endif
}

void
qt5nsmanager::set_session_url ()
{
#if defined SEQ66_NSM_SUPPORT
    if (not_nullptr(nsm_client()))
    {
        std::string url = nsm_client()->nsm_url();
        m_window->session_URL(url);
    }
    else
        m_window->session_URL("None");
#else
    m_window->session_URL("None");
#endif
}

}           // namespace seq66

/*
 * qt5nsmanager.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


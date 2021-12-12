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
 * \updates       2021-12-12
 * \license       GNU GPLv2 or above
 *
 *  Duty now for the future!
 *
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

    /*
     * EXPERIMENTAL:  Move this to create_window().
     * m_timer = qt_timer(this, "qt5nsmanager", 5, SLOT(conditional_update()));
     * When running help, the timer pointer is null in the constructor.
     */
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

        qsmainwnd * qm = new (std::nothrow) qsmainwnd(*p, mfname, usensm);
        result = not_nullptr(qm);
        if (result)
        {
            m_window.reset(qm);
            m_window->attach_session(this);         /* ATTACH/DETACH        */
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
            if (error_active())
            {
                show_error("", error_message());

                /*
                 * Let the application stay active; let the user decide what to
                 * do about the error here.  For example, if a playlist doesn't
                 * load, why just exit?
                 *
                 * result = false;
                 */
            }
            else
            {
                std::string path;               /* config or session path   */
                std::string name;               /* config or session name   */
                std::string clid;               /* config/session client ID */
                if (usensm)
                {
                    path = manager_path();
                    name = display_name();
                    clid = client_id();
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
                    {
                        if (have_uuid)
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
                m_window->session_log("No log entries.");
                m_window->song_path(rc().midi_filename());
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
    }
    if (result && usr().style_sheet_active())
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
    return result;
}

/**
 *  Will do more with this later.  Currently we just call the base class.
 */

bool
qt5nsmanager::close_session (std::string & msg, bool ok)
{
    bool saved = true;
    bool savepalette = rc().palette_active() || rc().auto_palette_save();
    if (savepalette)
    {
        std::string palfile = rc().palette_filespec();
        saved = save_palette(global_palette(), palfile);
    }
    bool closed = clinsmanager::close_session(msg, ok);
    return saved && closed;
}

/**
 *  Will do more with this later.  Currently we just call the base class.
 */

bool
qt5nsmanager::detach_session (std::string & msg, bool ok)
{
    return clinsmanager::detach_session(msg, ok);
}

bool
qt5nsmanager::run ()
{
    int exit_status = m_application.exec();     /* run main window loop     */
    return exit_status == EXIT_SUCCESS;
}

void
qt5nsmanager::quit ()
{
    session_message("Quitting the session");
    m_application.quit();
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
        std::string text = tag + ": " + msg;
        m_window->show_message_box(text);
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
#if defined SEQ66_PORTMIDI_SUPPORT
            if (Pm_error_present())
            {
                std::string pmerrmsg = std::string(Pm_error_message());
                append_error_message(pmerrmsg);
            }
#endif
            std::string html = tag + " ";
            html += string_replace(error_message(), "\n", "<br>");
            html += "<br>Please exit and fix the configuration.";
            m_window->show_message_box(html);
        }
        else
        {
            std::string text = tag;

            /*
             * This ends up duplicating messages.
             *
             * append_error_message(msg);
             */

            if (! tag.empty())
                text += " ";

            text += msg;
            m_window->show_message_box(text);
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


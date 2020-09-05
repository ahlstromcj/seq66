#if ! defined SEQ66_QT5NSMANAGER_HPP
#define SEQ66_QT5NSMANAGER_HPP

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
 * \file          qt5nsmanager.hpp
 *
 *  This module declares/defines the main module for the JACK/ALSA "qt5"
 *  implementation of this application.
 *
 * \library       qt5nsmanager application
 * \author        Chris Ahlstrom
 * \date          2020-03-15
 * \updates       2020-09-05
 * \license       GNU GPLv2 or above
 *
 *  This is an attempt to change from the hoary old (or, as H.P. Lovecraft
 *  would style it, "eldritch") gtkmm-2.4 implementation of Seq66.
 */

#include <QObject>                      /* Qt 5 QObject class               */
#include <memory>                       /* std::unique_ptr<>                */

#include "sessions/clinsmanager.hpp"    /* seq66::clinsmanager              */
#include "qsmainwnd.hpp"                /* Qt 5 qsmainwnd main window       */

#if defined SEQ66_NSM_SUPPORT
#include "nsm/nsmclient.hpp"            /* seq66::nsmclient                 */
#endif

#define SEQ66_NSM_QT5_CAPABILITIES      ":switch:dirty:message"

/*
 * Forward reference
 */

class QApplication;

namespace seq66
{

/**
 *
 */

class qt5nsmanager : public QObject, public clinsmanager
{
	Q_OBJECT

public:

    qt5nsmanager
    (
        QApplication & app,
        QObject * parent         = nullptr,
        const std::string & caps = SEQ66_NSM_QT5_CAPABILITIES
    );
    virtual ~qt5nsmanager ();

    virtual bool close_session (bool ok = true) override;
    virtual bool create_window () override;
    virtual void show_message
    (
        const std::string & tag,
        const std::string & msg
    ) const override;
    virtual void show_error
    (
        const std::string & tag,
        const std::string & msg
    ) const override;
    virtual bool run () override;
    virtual void session_manager_name (const std::string & mgrname) override;
    virtual void session_manager_path (const std::string & pathname) override;
    virtual void session_display_name (const std::string & dispname) override;
    virtual void session_client_id (const std::string & clid) override;

signals:        /* signals sent by session client callbacks */

#if 0
	void sig_active (bool isactive);
	void sig_open ();
	void sig_save ();
	void sig_loaded ();
	void sig_show ();
	void sig_hide ();
#endif

private:

    QApplication & m_application;

    std::unique_ptr<qsmainwnd> m_window;

};          // class qt5nsmanager

}           // namespace seq66

#endif      // SEQ66_QT5NSMANAGER_HPP

/*
 * qt5nsmanager.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


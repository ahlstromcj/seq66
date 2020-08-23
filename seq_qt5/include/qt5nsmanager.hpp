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
 * \updates       2020-08-23
 * \license       GNU GPLv2 or above
 *
 *  This is an attempt to change from the hoary old (or, as H.P. Lovecraft
 *  would style it, "eldritch") gtkmm-2.4 implementation of Seq66.
 */

#include <QObject>                      /* Qt 5 QObject class               */
#include <memory>                       /* std::unique_ptr<>                */

#include "sessions/smanager.hpp"        /* seq66::smanager                  */
#include "qsmainwnd.hpp"                /* Qt 5 qsmainwnd main window       */

#if defined SEQ66_NSM_SUPPORT
#include "nsm/nsmclient.hpp"            /* seq66::nsmclient                 */
#endif

class QApplication;

namespace seq66
{

/**
 *
 */

class qt5nsmanager : public QObject, public smanager
{
	Q_OBJECT

public:

    qt5nsmanager (QApplication & app, QObject * parent = nullptr);
    virtual ~qt5nsmanager ();

    virtual bool create_session
    (
        int argc        = 0,
        char * argv []  = nullptr
    ) override;
    virtual bool close_session (bool ok = true) override;
    virtual bool create_window () override;
    virtual void show_message (const std::string & msg) const override;
    virtual void show_error (const std::string & msg = "") const override;
    virtual bool run () override;

signals:        /* signals sent by session client callbacks */

	void sig_active (bool isactive);
	void sig_open ();
	void sig_save ();
	void sig_loaded ();
	void sig_show ();
	void sig_hide ();

private:

    QApplication & m_application;

#if defined SEQ66_NSM_SUPPORT

    /**
     *  The optional NSM client.  This item is not in the base class,
     *  smanager, because that class is meant to allow the option of building
     *  without NSM, but still simplifying the application's main() function.
     */

    std::unique_ptr<nsmclient> m_nsm_client;

#endif

    std::unique_ptr<qsmainwnd> m_window;

};          // class qt5nsmanager

}           // namespace seq66

#endif      // SEQ66_QT5NSMANAGER_HPP

/*
 * qt5nsmanager.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


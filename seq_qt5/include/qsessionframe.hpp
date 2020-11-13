#if ! defined SEQ66_QSESSIONFRAME_H
#define SEQ66_QSESSIONFRAME_H

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
 * \file          qsessionframe.hpp
 *
 *  This module declares/defines the class for showing session information.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2020-08-24
 * \updates       2020-09-01
 * \license       GNU GPLv2 or above
 *
 *  We want to be able to survey the existing mute-groups.
 */

#include <QFrame>

/*
 * This is necessary to keep the compiler from thinking Ui::qsessionframe
 * would be found in the seq66 namespace.
 */

namespace Ui
{
    class qsessionframe;
}

/*
 * Do not document a namespace.  It breaks Doxygen.
 */

namespace seq66
{

    class performer;
    class qsmainwnd;

/**
 *
 */

class qsessionframe : public QFrame
{
    Q_OBJECT

public:

    qsessionframe
    (
        performer & p,
        qsmainwnd * mainparent,
        QWidget * parent = nullptr
    );
    virtual ~qsessionframe();

public:

    void session_manager (const std::string & text);
    void session_path (const std::string & text);
    void session_display_name (const std::string & text);
    void session_client_id (const std::string & text);
    void session_URL (const std::string & text);
    void session_log (const std::string & text);
    void session_log_append (const std::string & text);
    void song_path (const std::string & text);

signals:

private slots:

private:

    Ui::qsessionframe * ui;

private:

    /**
     *  The main window that owns this window.
     */

    qsmainwnd * m_main_window;

    performer & m_performer;

};

}               // namespace seq66

#endif          // SEQ66_QSESSIONFRAME_H

/*
 * qsessionframe.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


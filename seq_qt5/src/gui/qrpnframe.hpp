#if ! defined SEQ66_QRPNFRAME_HPP
#define SEQ66_QRPNFRAME_HPP

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
 * \file          qrpnframe.hpp
 *
 *  This module declares/defines the base class for the RPN window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2026-07-30
 * \updates       2026-07-30
 * \license       GNU GPLv2 or above
 *
 *  Provides a way to more easily add NRPN and RPN controller events.
 */

#include <QFrame>

/*
 *  Forward declarations for Qt.
 */

namespace Ui
{
    class qrpnframe;
}

namespace seq66
{

class qrpnframe : public QFrame
{
    Q_OBJECT

public:

    explicit qrpnframe
    (
        QWidget * parent = nullptr
    );
    ~qrpnframe ();

private:

    /**
     *  The use Qt user-interface object pointer.
     */

    Ui::qrpnframe * ui;
};

}           // namespace seq66

#endif      //  SEQ66_QRPNFRAME_HPP

/*
 * qrpnframe.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

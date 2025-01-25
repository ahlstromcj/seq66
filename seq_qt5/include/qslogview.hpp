#if ! defined SEQ66_QSLOGVIEW_H
#define SEQ66_QSLOGVIEW_H

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
 * \file          qslogview.hpp
 *
 *  This dialog provides some context-specific help.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2025-01-25
 * \updates       2025-01-25
 * \license       GNU GPLv2 or above
 *
 */

#include <QDialog>

namespace Ui
{
   class qslogview;
}

namespace seq66
{

class qslogview final : public QDialog
{
    Q_OBJECT

public:

    explicit qslogview (QWidget * parent = nullptr);
    virtual ~qslogview ();

public:

    void refresh ();

private slots:

    void slot_refresh_log_view ();

private:

    Ui::qslogview * ui;

};             // class qslogview

}              // namespace seq66

#endif         // SEQ66_QSLOGVIEW_H

/*
 * qslogview.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


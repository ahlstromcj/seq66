#if ! defined SEQ66_QSBUILDINFO_H
#define SEQ66_QSBUILDINFO_H

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
 * \file          qsbuildinfo.hpp
 *
 *  The time bar shows markers and numbers for the measures of the song,
 *  and also depicts the left and right markers.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-05-30
 * \updates       2018-05-30
 * \license       GNU GPLv2 or above
 *
 */

#include <QDialog>

namespace Ui
{
   class qsbuildinfo;
}

namespace seq66
{

class qsbuildinfo final : public QDialog
{
    Q_OBJECT

public:

    explicit qsbuildinfo (QWidget * parent = nullptr);
    virtual ~qsbuildinfo ();

private:

    Ui::qsbuildinfo * ui;

};             // class qsbuildinfo

}              // namespace seq66

#endif         // SEQ66_QSBUILDINFO_H

/*
 * qsbuildinfo.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


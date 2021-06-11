#if ! defined SEQ66_QSABOUT_HPP
#define SEQ66_QSABOUT_HPP

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
 * \file          qsabout.hpp
 *
 *  The time bar shows markers and numbers for the measures of the song,
 *  and also depicts the left and right markers.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2018-03-03
 * \license       GNU GPLv2 or above
 *
 */

#include <QDialog>

/*
 * This is necessary to keep the compiler from thinking Ui::qsabout
 * should be in the seq66 namespace.
 */

namespace Ui
{
    class qsabout;
}

/*
 * Do not document a namespace.  It breaks Doxygen.
 */

namespace seq66
{

class qsabout final : public QDialog
{
    Q_OBJECT

public:

    explicit qsabout (QWidget * parent = nullptr);
    virtual ~qsabout ();

private:

    Ui::qsabout * ui;

};              // class qsabout

}               // namespace seq66

#endif          // SEQ66_QSABOUT_HPP

/*
 * qsabout.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


#if ! defined SEQ66_QSAPPINFO_H
#define SEQ66_QSAPPINFO_H

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
 * \file          qsappinfo.hpp
 *
 *  This dialog provides some context-specific help.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2023-08-21
 * \updates       2023-08-28
 * \license       GNU GPLv2 or above
 *
 */

#include <QDialog>

namespace Ui
{
   class qsappinfo;
}

namespace seq66
{

class qsappinfo final : public QDialog
{
    Q_OBJECT

public:

    explicit qsappinfo (QWidget * parent = nullptr);
    virtual ~qsappinfo ();

private slots:

    void slot_common_keys ();
    void slot_automation_keys ();
    void slot_seqroll_keys ();
    void slot_songroll_keys ();

private:

    Ui::qsappinfo * ui;

};             // class qsappinfo

}              // namespace seq66

#endif         // SEQ66_QSAPPINFO_H

/*
 * qsappinfo.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


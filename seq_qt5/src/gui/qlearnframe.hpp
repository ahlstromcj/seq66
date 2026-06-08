#if ! defined SEQ66_QLEARNFRAME_HPP
#define SEQ66_QLEARNFRAME_HPP

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
 * \file          qlearnframe.hpp
 *
 *  This module declares/defines the base class for the pattern-fix window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2026-06-08
 * \updates       2026-06-08
 * \license       GNU GPLv2 or above
 *
 *  Provides a way to modulate MIDI controller events.
 */

#include <QFrame>
#include "ctrl/opcontrol.hpp"           /* seq66::optcontrol and automation */

namespace Ui
{
    class qlearnframe;
}

class QButtonGroup;
class QLineEdit;

namespace seq66
{
    class performer;

/*
 * This class supports managing MIDI Learn.
 */

class qlearnframe final : public QFrame
{
    Q_OBJECT

public:

    qlearnframe
    (
        performer & p,
        automation::category opcat,
        QWidget * parent = nullptr
    );
    ~qlearnframe();

    void select_category (automation::category opcat);

protected:

    const performer & perf () const
    {
        return m_perf;
    }

    performer & perf ()
    {
        return m_perf;
    }

private:

    void handle_select_category (automation::category opcat);

private slots:

    void slot_select_category (int buttonno);

private:

    Ui::qlearnframe * ui;
    performer & m_perf;
    automation::category m_automation_category;
    QButtonGroup * m_learn_button_group;
};

}           // namespace seq66

#endif      // SEQ66_QLEARNFRAME_HPP

/*
 * qlearnframe.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

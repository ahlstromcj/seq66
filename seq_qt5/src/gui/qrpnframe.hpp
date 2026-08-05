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
 * \updates       2026-08-05
 * \license       GNU GPLv2 or above
 *
 *  Provides a way to more easily add NRPN and RPN controller events.
 */

#include <QFrame>

#include "ctrl/rpn.hpp"                 /* seq66::rpn "macro" class         */

/*
 *  Forward declarations for Qt.
 */

class QButtonGroup;

namespace Ui
{
    class qrpnframe;
}

namespace seq66
{

class performer;
class sequence;

class qrpnframe : public QFrame
{
    Q_OBJECT

public:

    explicit qrpnframe
    (
        performer & p,
        sequence & s,
        QWidget * parent = nullptr
    );
    ~qrpnframe ();

    performer & perf ()
    {
        return m_perf;
    }

    const performer & perf () const
    {
        return m_perf;
    }

    sequence & track ()
    {
        return m_seq;
    }

    const sequence & track () const
    {
        return m_seq;
    }

    rpn::info & rpn_info ()
    {
        return m_rpn_info;
    }

    const rpn::info & rpn_info () const
    {
        return m_rpn_info;
    }

private:

    void select_rpn (int rpncontrol);
    void select_rpn_value_type (int rpnvalue);
    void set_rpn_value_type_text (bool is_rpn, midishort pv);

private slots:

    void slot_select_rpn (int r);
    void slot_select_rpn_value_type (int v);

private:

    /**
     *  The Qt user-interface object pointer.
     */

    Ui::qrpnframe * ui;

    /**
     *  Access to performer::send_macro().
     */

    performer & m_perf;

    /**
     *  Access to buss number and pattern number.
     */

    sequence & m_seq;

    /**
     * Easier way to group radio buttons than using QGroupBox.
     */

    QButtonGroup * m_select_control_group { nullptr };
    QButtonGroup * m_select_value_group { nullptr };

    /**
     *  Contains all of the parameters needed to create the
     *  rpn compound object.
     */

    rpn::info m_rpn_info { };

    /**
     *  Selects RPN, NRPN, or the data controllers.
     *

    rpn::control m_rpn_control { rpn::control::rpn };
    rpn::parameter m_rpn_parameter { rpn::parameter::pitchbend_range };
     */

};          // class qrpnframe

}           // namespace seq66

#endif      // SEQ66_QRPNFRAME_HPP

/*
 * qrpnframe.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

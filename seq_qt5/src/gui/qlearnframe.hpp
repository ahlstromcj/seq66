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
 * \updates       2026-06-22
 * \license       GNU GPLv2 or above
 *
 *  Provides a way to modulate MIDI controller events.
 */

#include <QFrame>

#include "ctrl/opcontrol.hpp"           /* seq66::optcontrol and automation */
#include "midi/event.hpp"               /* seq66::event                     */
#include "play/performer.hpp"           /* seq66::performer::callbacks      */

namespace Ui
{
    class qlearnframe;
}

class QButtonGroup;
class QLineEdit;
class QTimer;

namespace seq66
{

/*
 * This class supports managing MIDI Learn.
 */

class qlearnframe final :
    public QFrame,
    protected performer::callbacks
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

private:

    performer & perf ()
    {
        return m_perf;
    }

    const performer & perf () const
    {
        return m_perf;
    }

    midilearn & midi_learn ()
    {
        return m_midi_learn;
    }

    const midilearn & midi_learn () const
    {
        return m_midi_learn;
    }

    void select_category (automation::category opcat);
    void select_action (automation::action opact);
    void update_active_counts ();
    void set_buttons (bool enable);
    void setup_loop_process ();
    void setup_mutes_process ();
    void setup_automation_process ();

private:        // performer::callback override

    virtual bool on_midi_learn (seq66::event) override;

private slots:

    void slot_poll_update ();
    void slot_select_category (int buttonno);
    void slot_select_action (int buttonno);
    void slot_cancel ();
    void slot_save ();
    void slot_reset ();
    void slot_clear ();
    void slot_ok ();
    void slot_inverse ();
    void slot_d1min ();
    void slot_d1max ();

private:

    Ui::qlearnframe * ui;
    performer & m_perf;
    midilearn & m_midi_learn;
    QTimer * m_timer;
    QButtonGroup * m_learn_button_group;
    QButtonGroup * m_action_button_group;
    std::string m_current_keyname;
//  automation::category m_automation_category;
//  automation::action m_automation_action;
//  automation::slot m_automation_slot;
//  int m_control_index;
    bool m_inverse;
    int m_d1min;
    int m_d1max;
};

}           // namespace seq66

#endif      // SEQ66_QLEARNFRAME_HPP

/*
 * qlearnframe.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

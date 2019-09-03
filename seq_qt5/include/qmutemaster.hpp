#if ! defined SEQ66_QMUTEMASTER_HPP
#define SEQ66_QMUTEMASTER_HPP

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
 * \file          qmutemaster.hpp
 *
 *  This module declares/defines the base class for the screen-set manager.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-05-29
 * \updates       2019-06-02
 * \license       GNU GPLv2 or above
 *
 *  We want to be able to survey the existing mute-groups.
 */

#include <QFrame>

#include "ctrl/keycontainer.hpp"        /* class seq66::keycontainer        */
#include "ctrl/opcontainer.hpp"         /* class seq66::opcontainer         */
#include "play/mutegroup.hpp"           /* seq66::mutegroup::number type    */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "play/setmapper.hpp"           /* seq66::setmapper class           */

/**
 *  Forward references.
 */

class QPushButton;
class QTableWidgetItem;
class QTimer;

/*
 * This is necessary to keep the compiler from thinking Ui::qmutemaster
 * would be found in the seq66 namespace.
 */

namespace Ui
{
    class qmutemaster;
}

/*
 * Do not document a namespace.  It breaks Doxygen.
 */

namespace seq66
{
    class mutegroup;
    class performer;
    class qsmainwnd;

/**
 *  This class helps manage screensets, including selecting the current
 *  playscreen and showing, in brief form, the contents of each set.
 */

class qmutemaster final : public QFrame, protected performer::callbacks
{

private:

    /**
     *  Provides human-readable names for the columns of the set table.
     */

    enum class column_id
    {
        group_number,
        group_count,
        group_keyname
    };

private:

    Q_OBJECT

public:

    qmutemaster
    (
        performer & p,
        qsmainwnd * mainparent,
        QWidget * parent = nullptr
    );
    virtual ~qmutemaster();

protected:

    virtual bool on_mutes_change (mutegroup::number setno) override;

    /*
     * virtual bool on_group_learn (bool state) override;
     * virtual bool on_sequence_change (seq::number seqno) override;
     */

protected:                          // overrides of event handlers

    virtual void closeEvent (QCloseEvent *) override;
    virtual void keyPressEvent (QKeyEvent *) override;
    virtual void keyReleaseEvent (QKeyEvent *) override;
    virtual void changeEvent (QEvent *) override;

private:

    bool needs_update () const
    {
        bool result = m_needs_update;
        m_needs_update = false;
        return result;
    }

    void group_needs_update ()
    {
        m_needs_update = true;
    }

    void create_group_buttons ();
    void update_group_buttons (bool modify = false);
    void handle_group (int row, int column);
    void handle_group (int setno);
    bool group_control
    (
        automation::action a, int /*d0*/, int index, bool inverse
    );

    int current_group () const
    {
        return m_current_group;
    }
    bool set_current_group (int row);

    bool modify () const
    {
        return m_modify_active;
    }

    void modify (bool flag)
    {
        m_modify_active = flag;
    }

    void set_column_widths (int total_width);
    void setup_table ();
    bool initialize_table ();
    bool group_line
    (
        mutegroup::number row,
        int mutecount,
        const std::string & keyname
    );
    bool handle_key_press (const keystroke & k);
    bool handle_key_release (const keystroke & k);
    QTableWidgetItem * cell (screenset::number row, column_id col);

signals:

private slots:

    void conditional_update ();
    void mute_table_click_ex
    (
        int row, int /*column*/, int /*prevrow*/, int /*prevcolumn*/
    );
    void clear_mutes ();
    void slot_modify ();
    void slot_reset ();
    void slot_down ();
    void slot_up ();

private:

    /**
     *  The use Qt user-interface object pointer.
     */

    Ui::qmutemaster * ui;

private:

    /**
     *  Holds a map of midioperation functors to be used to control patterns,
     *  mute-groups, and automation functions.
     */

    opcontainer m_operations;

    /**
     *  A timer for refreshing the frame as needed.
     */

    QTimer * m_timer;

    /**
     *  The main window that owns this window.
     */

    qsmainwnd * m_main_window;

    /**
     *  Access to all the screenset buttons.
     */

    QPushButton * m_group_buttons [SEQ66_MUTE_ROWS][SEQ66_MUTE_COLUMNS];

    /**
     *  Indicates the currently-selected group number.
     */

    int m_current_group;

    /**
     *
     */

    int m_group_count;

    /**
     *
     */

    bool m_modify_active;

    /**
     *  Indicates that the view should be refreshed.
     */

    mutable bool m_needs_update;

};

}           // namespace seq66

#endif // SEQ66_QMUTEMASTER_HPP

/*
 * qmutemaster.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


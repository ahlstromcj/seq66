#if ! defined SEQ66_QMUTEMASTER_HPP
#define SEQ66_QMUTEMASTER_HPP

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
 * \file          qmutemaster.hpp
 *
 *  This module declares/defines the base class for the screen-set manager.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-05-29
 * \updates       2023-04-25
 * \license       GNU GPLv2 or above
 *
 *  We want to be able to survey the existing mute-groups.
 */

#include <QFrame>
#include <vector>                       /* std::vector container            */

#include "ctrl/keycontainer.hpp"        /* class seq66::keycontainer        */
#include "ctrl/opcontainer.hpp"         /* class seq66::opcontainer         */
#include "play/mutegroups.hpp"          /* seq66::mutegroup, mutegroups     */
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
    class qsmainwnd;

/**
 *  This class helps manage screensets, including selecting the current
 *  playscreen and showing, in brief form, the contents of each set.
 */

class qmutemaster final : public QFrame, protected performer::callbacks
{
    friend class qsmainwnd;

private:

    /**
     *  Provides human-readable names for the columns of the set table.
     */

    enum class column_id
    {
        group_number,
        group_count,
        group_keyname,
        group_name
    };

    enum class enabling
    {
        disable,
        leave,
        enable
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

    virtual bool on_mutes_change
    (
        mutegroup::number setno,
        performer::change mod
    ) override;

    /*
     * virtual bool on_group_learn (bool state) override;
     * virtual bool on_sequence_change (seq::number seqno, ...) override;
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
    void update_group_buttons (enabling tomodify = enabling::leave);
    void handle_group_button (int row, int column);     /* in lambda slot   */
    void handle_group_change (int groupno);
    bool group_control
    (
        automation::action a, int /*d0*/, int index, bool inverse
    );

    int current_group () const
    {
        return m_current_group;
    }

    bool set_current_group (int row);
    void create_pattern_buttons ();
    void update_pattern_buttons (enabling tomodify = enabling::leave);
    void handle_pattern_button (int row, int column);  /* in lambda slot    */

    bool trigger () const
    {
        return m_trigger_active;
    }

    void trigger (bool flag)
    {
        m_trigger_active = flag;
    }

    void set_bin_hex (bool bin_checked);
    void mutes_file_change (bool flag);
    void set_column_widths (int total_width);
    void setup_table ();
    bool initialize_table ();
    bool group_line
    (
        mutegroup::number row,
        int mutecount,
        const std::string & keyname,
        const std::string & groupname
    );

#if defined PASS_KEYSTROKES_TO_PARENT
    bool handle_key_press (const keystroke & k);
    bool handle_key_release (const keystroke & k);
#endif

    QTableWidgetItem * cell (screenset::number row, column_id col);
    void clear_pattern_mutes ();
    bool load_mutegroups (const std::string & fullfilespec);
    bool save_mutegroups (const std::string & fullfilespec);
    void enable_save ();

signals:

private slots:

    void conditional_update ();
    void slot_pattern_offset (int index);
    void slot_table_click
    (
        int row, int /*column*/,
        int /*prevrow*/, int /*prevcolumn*/
    );
    void slot_clear_all_mutes ();
    void slot_fill_mutes ();
    void slot_cell_changed (int row, int column);
    void slot_mutes_file_modify ();
    void slot_bin_mode (bool ischecked);
    void slot_hex_mode (bool ischecked);
    void slot_trigger ();
    void slot_set_mutes ();
    void slot_down ();
    void slot_up ();
    void slot_write_to_midi ();
    void slot_write_to_mutes ();
    void slot_strip_empty ();
    void slot_load_mutes ();
    void slot_load_midi ();
    void slot_toggle_active ();
    void slot_load ();
    void slot_save ();

private:

    /**
     *  The use Qt user-interface object pointer.
     */

    Ui::qmutemaster * ui;

    /**
     *  A timer for refreshing the frame as needed.
     */

    QTimer * m_timer;

    /**
     *  The main window that owns this window.
     */

     qsmainwnd * m_main_window;

    /**
     *  Access to buttons, more flexible for swapping coordinates.
     */

    using buttons = std::vector<QPushButton *>;

    /**
     *  Access to all the mute-group buttons.  This is an array forever fixed to
     *  4 x 8, because that's about all the keystrokes we have available to
     *  allocate to mute-groups.  We would use mutegroups::Rows() and
     *  mutegroups::Columns(), but C++ does not allow functions as array sizes.
     */

    buttons m_group_buttons;

    /**
     *  Access to all the pattern buttons.  It is the same size as the group
     *  grid, but might be page-able in the future.  See the pending
     *  m_pattern_offset member.
     */

    buttons m_pattern_buttons;

    /**
     *  Indicates the currently-selected group number.
     */

    int m_current_group;

    /**
     *  Indicates the number of groups in the grid.  Essentially constant at 4
     *  x 8 = 32.
     */

    int m_group_count;

    /**
     *  Holds the row and column of the button corresponding to the
     *  currently-selected mute-group.
     */

    int m_button_row;
    int m_button_column;

    /**
     *  If true, button click can activate existing mute groups.
     *  Indicates that the group buttons are enabled, but will "only" trigger
     *  the clicked mute-group.
     */

    bool m_trigger_active;

    /**
     *  Indicates that the view should be refreshed.
     */

    mutable bool m_needs_update;

    /**
     *  Holds the current status of all of the pattern buttons in the
     *  currently active mute-group in the user-interface.
     */

    midibooleans m_pattern_mutes;

    /**
     *  A future feature to allow for slot shifting to handle set sizes like 64
     *  and 96.
     */

    seq::number m_pattern_offset;

};

}           // namespace seq66

#endif // SEQ66_QMUTEMASTER_HPP

/*
 * qmutemaster.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


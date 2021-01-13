#if ! defined SEQ66_QSETMASTER_HPP
#define SEQ66_QSETMASTER_HPP

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
 * \file          qsetmaster.hpp
 *
 *  This module declares/defines the base class for the screen-set manager.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-05-11
 * \updates       2020-12-07
 * \license       GNU GPLv2 or above
 *
 *  We want to be able to survey the existing screen-sets and sequences, and be
 *  able to pick them via buttons and keystrokes rather then using the
 *  set-spinner in the live frame.
 *
 *  Also, we want to get a quick idea of what screen-sets and sequences are
 *  loaded and active.
 */

#include <QFrame>

#include "ctrl/keycontainer.hpp"        /* class seq66::keycontainer        */
#include "ctrl/opcontainer.hpp"         /* class seq66::opcontainer         */
#include "play/performer.hpp"           /* seq66::performer class           */

/**
 *  Forward references.
 */

class QPushButton;
class QTableWidgetItem;
class QTimer;

/*
 * This is necessary to keep the compiler from thinking Ui::qsetmaster
 * would be found in the seq66 namespace.
 */

namespace Ui
{
    class qsetmaster;
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

class qsetmaster final : public QFrame, protected performer::callbacks
{

private:

    /**
     *  Provides human-readable names for the columns of the set table.
     */

    enum class column_id
    {
        set_number,
        set_seq_count,
        set_name
    };

private:

    Q_OBJECT

public:

    qsetmaster
    (
        performer & p,
        bool embedded,
        qsmainwnd * mainparent,
        QWidget * parent = nullptr
    );
    virtual ~qsetmaster();

private:

    virtual bool on_set_change
    (
        screenset::number setno,
        performer::change ctype
    ) override;

    /*
     * virtual bool on_group_learn (bool state) override;
     * virtual bool on_sequence_change (seq::number seqno, bool recr) override;
     */

private:                          // overrides of event handlers

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

    void set_needs_update ()
    {
        m_needs_update = true;
    }

    void create_set_buttons ();
    void handle_set (int row, int column);
    void handle_set (int setno);
    void delete_set (int setno);
    bool set_control
    (
        automation::action a, int /*d0*/, int index, bool inverse
    );
    bool populate_default_ops ();

    int current_row () const
    {
        return m_current_row;
    }

    void current_row (int row);
    void set_column_widths (int total_width);
    void setup_table ();
    bool initialize_table ();
    bool set_line
    (
        screenset & sset,
        screenset::number row
    );
    bool handle_key_press (const keystroke & k);
    bool handle_key_release (const keystroke & k);
    QTableWidgetItem * cell (screenset::number row, column_id col);
    void move_helper (int oldrow, int newrow);

signals:

private slots:

    void conditional_update ();
    void slot_set_name ();
    void slot_show_sets ();
    void slot_move_down ();
    void slot_move_up ();
    void slot_delete ();
    void slot_table_click_ex
    (
        int row, int /*column*/, int /*prevrow*/, int /*prevcolumn*/
    );

private:

    /**
     *  The use Qt user-interface object pointer.
     */

    Ui::qsetmaster * ui;

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

    QPushButton * m_set_buttons [SEQ66_DEFAULT_SET_ROWS][SEQ66_DEFAULT_SET_COLUMNS];

    /**
     *  Indicates the currently-selected set number.
     */

    int m_current_set;

    /**
     *  Indicates the currently-selected set-table row.
     */

    int m_current_row;

    /**
     *
     */

    int m_current_row_count;

    /**
     *  Indicates that the view should be refreshed.
     */

    mutable bool m_needs_update;

    /**
     *  Indicates that this view is embedded in a frame, and thus permanent.
     */

    bool m_is_permanent;

};

}           // namespace seq66

#endif // SEQ66_QSETMASTER_HPP

/*
 * qsetmaster.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


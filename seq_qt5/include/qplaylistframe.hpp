#if ! defined SEQ66_QPLAYLISTFRAME_HPP
#define SEQ66_QPLAYLISTFRAME_HPP

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
 * \file          qplaylistframe.hpp
 *
 *  This module declares/defines the base class for a simple playlist editor
 *  based on Qt 5.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-09-04
 * \updates       2023-07-12
 * \license       GNU GPLv2 or above
 *
 */

#include <QFrame>

#include "util/basic_macros.hpp"        /* nullptr and related macros       */

/*
 * Qt forward references.
 */

class QTableWidgetItem;
class QTimer;

/*
 * Do not document namespaces.
 */

namespace Ui
{
    class qplaylistframe;
}

/*
 * Do not document namespaces, it breaks Doxygen.
 */

namespace seq66
{
    class performer;
    class qsmainwnd;

/**
 *  Provides a frame for the Playlist tab.
 */

class qplaylistframe final : public QFrame
{
    friend class qsmainwnd;

    Q_OBJECT

private:

    /**
     *  Provides human-readable names for the columns of the playlist and song
     *  tables.
     */

    enum column_id_t
    {
        CID_MIDI_NUMBER,
        CID_ITEM_NAME
    };

public:

    qplaylistframe
    (
        performer & p,
        qsmainwnd * window,
        QWidget * frameparent = nullptr
    );

    virtual ~qplaylistframe ();

private:

    void set_row_heights (int height);
    void set_column_widths ();
    void reset_playlist (int listindex = 0);
    void reset_playlist_file_name ();
    void set_current_playlist ();
    void set_current_song ();
    void fill_playlists ();
    void fill_songs ();
    QTableWidgetItem * cell (bool isplaylist, int row, column_id_t col);

    performer & perf ()
    {
        return m_performer;
    }

    bool load_playlist (const std::string & fullfilespec = "");

protected:                          // overrides of event handlers

    virtual void keyPressEvent (QKeyEvent *) override;
    virtual void keyReleaseEvent (QKeyEvent *) override;

private:

    void list_unmodify ();
    void song_unmodify ();
    void enable_midi_widgets (bool enable);

    const qsmainwnd * parent () const
    {
        return m_parent;
    }

    qsmainwnd * parent ()
    {
        return m_parent;
    }

signals:

private slots:

    void slot_list_click_ex (int, int, int, int);
    void slot_song_click_ex (int, int, int, int);
    void slot_file_create_click();
    void slot_list_dir_click ();
    void slot_list_load_click ();
    void slot_list_add_click ();
    void slot_list_modify_click ();
    void slot_list_remove_click ();
    void slot_list_save_click ();
    void slot_song_load_click ();
    void slot_song_select_click ();
    void slot_song_add_click ();
    void slot_song_modify_click ();
    void slot_song_remove_click ();
    void slot_playlist_active_click ();
    void slot_auto_arm_click ();
    void slot_auto_play_click ();
    void conditional_update ();
    void list_modify ();
    void list_modify (int);
    void list_modify (const QString &);
    void song_modify ();
    void song_modify (int);
    void song_modify (const QString &);

private:

    Ui::qplaylistframe * ui;

private:

    /**
     *  A timer for screen refreshing.
     */

    QTimer * m_timer;

    /**
     *  The performer object.
     */

    performer & m_performer;

    /**
     *  The main window parent of this frame.
     */

    qsmainwnd * m_parent;

    /**
     *  Provides the currently-selected single rows for the playlist and song
     *  tables.
     */

    int m_current_list_index;
    int m_current_song_index;

};          // class qplaylistframe

}           // namespace seq66

#endif      // SEQ66_QPLAYLISTFRAME_HPP

/*
 * qplaylistframe.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


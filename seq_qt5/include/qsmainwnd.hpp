#if ! defined SEQ66_QSMAINWND_HPP
#define SEQ66_QSMAINWND_HPP

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
 * \file          qsmainwnd.hpp
 *
 *  This module declares/defines the base class for the main window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2020-07-27
 * \license       GNU GPLv2 or above
 *
 *  The main window is known as the "Patterns window" or "Patterns
 *  panel".  It holds the "Pattern Editor" or "Sequence Editor".  The main
 *  window consists of two object:  mainwnd, which provides the user-interface
 *  elements that surround the patterns, and mainwid, which implements the
 *  behavior of the pattern slots.
 */

#include <QMainWindow>
#include <QList>

#include "app_limits.h"                 /* SEQ66_USE_DEFAULT_PPQN           */
#include "midi/midibytes.hpp"           /* alias midibpm                    */
#include "play/performer.hpp"           /* seq66::performer class           */

/*
 *  Forward declaration.
 */

class QCloseEvent;
class QErrorMessage;
class QFileDialog;
class QMessageBox;
class QResizeEvent;
class QTimer;

/*
 *  The Qt UI namespace.
 */

namespace Ui
{
    class qsmainwnd;
}

/*
 *  Do not document namespaces, it seems to break Doxygen.
 */

namespace seq66
{
    class keystroke;
    class qliveframeex;
    class qperfeditex;
    class qperfeditframe64;
    class qplaylistframe;
    class qsabout;
    class qsbuildinfo;
    class qseditoptions;
    class qseqeditex;
    class qseqeditframe;
    class qseqeventframe;
    class qsetmaster;
    class qmutemaster;
    class qslivebase;
    class qslivegrid;
    class qsmaintime;

/**
 * The main window of Kepler34.
 */

class qsmainwnd final :
    public QMainWindow,
    protected performer::callbacks
{
    friend class qmutemaster;
    friend class qplaylistframe;
    friend class qsetmaster;
    friend class qslivebase;
    friend class qsliveframe;
    friend class qslivegrid;

    Q_OBJECT

public:

    qsmainwnd
    (
        performer & p,
        const std::string & midifilename    = "",
        bool usensm                         = false,
        QWidget * parent                    = nullptr
    );
    virtual ~qsmainwnd ();

    void open_file (const std::string & path);
    void show_message_box (const std::string & msg_text);
    void remove_editor (int seq);
    void remove_qperfedit ();
    void hide_qperfedit (bool hide = false);
    void remove_live_frame (int ssnum);

    int ppqn () const
    {
        return perf().ppqn();
    }

    void open_playlist ()
    {
        show_open_list_dialog();
    }

    bool use_nsm () const
    {
        return m_use_nsm;
    }

protected:

    void use_nsm (bool flag)
    {
        m_use_nsm = flag;
    }

    bool recreate_all_slots ();

protected:

    virtual bool on_group_learn (bool learning) override;
    virtual bool on_group_learn_complete
    (
        const keystroke & k, bool success
    ) override;
    virtual bool on_sequence_change (seq::number seqno) override;
    virtual bool on_trigger_change (seq::number seqno) override;
    virtual void keyPressEvent (QKeyEvent * event) override;

protected:

    void report_message (const std::string & msg, bool good);

protected:

    const performer & perf () const
    {
        return m_main_perf;
    }

    performer & perf ()
    {
        return m_main_perf;
    }

private:

    virtual void closeEvent (QCloseEvent *) override;
    virtual void changeEvent (QEvent *) override;
    virtual void resizeEvent (QResizeEvent *) override;

private:

    void make_perf_frame_in_tab ();

    /*
     * Check if the file has been modified.  If modified, ask the user whether
     * to save changes.
     */

    bool check ();
    std::string filename_prompt (const std::string & prompt);
    void update_window_title (const std::string & fn = "");
    void update_recent_files_menu ();
    void create_action_connections ();
    void create_action_menu ();
    void remove_all_editors ();
    void remove_all_live_frames ();
    void connect_editor_slots ();
    void set_tap_button (int beats);
    void redo_live_frame ();
    bool handle_key_press (const keystroke & k);
    bool handle_key_release (const keystroke & k);
    void show_song_mode (bool songmode);

private:

    /**
     *  An alias for keeping track of external sequence edits.
     */

    using edit_container = std::map<int, qseqeditex *>;

    /**
     *  An alias for keeping track of external live-frames.
     */

    using live_container = std::map<int, qliveframeex *>;

private:

    Ui::qsmainwnd * ui;
    qslivebase * m_live_frame;
    qperfeditex * m_perfedit;
    qperfeditframe64 * m_song_frame64;
    qseqeditframe * m_edit_frame;
    qseqeventframe * m_event_frame;
    qplaylistframe * m_playlist_frame;
    QErrorMessage * m_msg_error;
    QMessageBox * m_msg_save_changes;
    QTimer * m_timer;
    QMenu * m_menu_recent;
    QList<QAction *> m_recent_action_list;
    const int mc_max_recent_files;
    QFileDialog * m_import_dialog;
    performer & m_main_perf;
    qsmaintime * m_beat_ind;
    qseditoptions * m_dialog_prefs;
    qsabout * m_dialog_about;
    qsbuildinfo * m_dialog_build_info;
    qsetmaster * m_set_master;
    qmutemaster * m_mute_master;

    /**
     *  Holds the current setting of the song mode, since
     *  ui->btnSongPlay->isChecked() seems not to be reliable.
     */

    bool m_song_mode;

    /**
     *  Duty now for the future!
     */

    bool m_use_nsm;

    /**
     *  Provides a workaround for a race condition when a MIDI file-name is
     *  provided on the command line.  This would cause the title to be
     *  "unnamed".
     */

    bool m_is_title_dirty;

    /**
     *  Indicates whether to show the time as bar:beats:ticks or as
     *  hours:minutes:seconds.  The default is true:  bar:beats:ticks.
     */

    bool m_tick_time_as_bbt;

    /**
     *  Holds a list of the sequences currently under edit.  We do not want to
     *  open the same sequence in two different editors.  Also, we need to be
     *  able to delete any open qseqeditex windows when exiting the
     *  application.
     */

    edit_container m_open_editors;

    /**
     *  Holds a list of open external qliveframeex objects.
     */

    live_container m_open_live_frames;

    /**
     *  Indicates the visibility of the external performance-edit frame.
     */

    bool m_perf_frame_visible;

    /**
     *  Indicates the current set for the mainwnd, regardless of the current
     *  play-screen.
     */

    screenset::number m_current_main_set;

private slots:

    void update_bank (int newBank);
    void update_bank_name (const QString &);
    void start_playing ();
    void pause_playing ();
    void stop_playing ();
    void set_song_mode (bool song_mode);
    void song_recording (bool record);

    void song_recording_snap (bool /*snap*/)
    {
        /*
         * This will always be in force: perf().song_record_snap(snap);
         */
    }

    void panic ();
    void update_bpm (double bpm);
    void edit_bpm ();
    void update_ppqn (int pindex);
    void update_midi_bus (int bindex);
    void update_beats_per_measure (int bmindex);
    void update_beat_length (int blindex);
    void open_recent_file ();
    void new_file ();
    void new_session ();
    bool save_file (const std::string & fname = "");
    bool save_session ();
    bool save_file_as ();
    bool export_file_as_midi (const std::string & fname = "");
    bool export_song (const std::string & fname = "");
    void quit ();
    void show_import_dialog ();             /* import MIDI to current bank  */
    void import_into_session ();            /* for support of NSM           */
    void show_open_file_dialog ();
    void show_open_list_dialog ();
    void showqsabout ();
    void showqsbuildinfo ();
    void tabWidgetClicked (int newindex);
    void refresh ();                        /* redraw certain GUI elements  */
    void load_editor (int seqid);
    void load_event_editor (int seqid);
    void load_qseqedit (int seqid);
    void load_qperfedit (bool on);
    void load_live_frame (int ssnum);
    void load_set_master ();
    void load_mute_master ();
    void toggle_time_format (bool on);
    void show_set_master ();
    void open_performance_edit ();
    void apply_song_transpose ();
    void reload_mute_groups ();
    void clear_mute_groups ();
    void set_song_mute_on ();
    void set_song_mute_off ();
    void set_song_mute_toggle ();
    void learn_toggle ();
    void tap ();
    void queue_it ();

private:

    void remove_set_master ();

};          // class qsmainwnd

}           // namespace seq66

#endif      // SEQ66_QSMAINWND_HPP

/*
 * qsmainwnd.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


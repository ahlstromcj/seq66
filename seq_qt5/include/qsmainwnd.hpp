#if ! defined SEQ66_QSMAINWND_HPP
#define SEQ66_QSMAINWND_HPP

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
 * \file          qsmainwnd.hpp
 *
 *  This module declares/defines the base class for the main window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2023-08-26
 * \license       GNU GPLv2 or above
 *
 *  The main window is known as the "Patterns window" or "Patterns panel".  It
 *  holds the "Pattern Editor" or "Sequence Editor".  The main window consists
 *  of two object:  qsmainwnd, which provides the user-interface elements that
 *  surround the patterns, and qslivegrid, which implements the behavior of
 *  the pattern slots.  Also included are tabs for managing mute-groups, MIDI
 *  events, playlists, and information about the current session.
 */

#include <QMainWindow>
#include <QList>

#include "cfg/settings.hpp"             /* seq66::combolist helper class    */
#include "midi/midibytes.hpp"           /* alias midibpm                    */
#include "play/performer.hpp"           /* seq66::performer class           */

/*
 * Q_DECLARE_METATYPE(seq66::screenset::number) doesn't work, had to switch to
 * using int.
 *
 * Also, we get this warning, even with "#include <QItemSelection>" or
 * <QItemSelectionModel>:
 *
 *      QObject::connect: Cannot queue arguments of type 'QItemSelection'
 *      (Make sure 'QItemSelection' is registered using qRegisterMetaType().)
 */

/*
 *  Forward declarations.
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
    class qmutemaster;
    class qperfeditex;
    class qperfeditframe64;
    class qplaylistframe;
    class qsabout;
    class qsappinfo;
    class qsbuildinfo;
    class qseditoptions;
    class qseqeditex;
    class qseqframe;
    class qseqeventframe;
    class qsessionframe;
    class qsetmaster;
    class qslivebase;
    class qslivegrid;
    class qsmaintime;
    class smanager;
    class qt5nsmanager;

/**
 * The main window of Kepler34... er, I mean Seq66.
 */

class qsmainwnd final : public QMainWindow, protected performer::callbacks
{
    friend class qmutemaster;
    friend class qplaylistframe;
    friend class qsessionframe;
    friend class qsetmaster;
    friend class qslivebase;
    friend class qslivegrid;
    friend class qt5nsmanager;

    Q_OBJECT

public:

    qsmainwnd
    (
        performer & p,
        const std::string & midifilename    = "",
        bool usensm                         = false,
        qt5nsmanager * sessionmgr           = nullptr // QWidget * p = nullptr
    );
    virtual ~qsmainwnd ();

    void enable_reload_button (bool flag);
    bool open_file (const std::string & path);
    void show_error_box (const std::string & msg_text);
    bool show_error_box_ex
    (
        const std::string & msgtext,
        bool isporterror = false
    );
    void remove_editor (int seq);
    void remove_qperfedit ();
    void hide_qperfedit (bool hide = false);
    void remove_live_frame (int ssnum);
    void enable_bus_item (int bus, bool enabled);
    void set_ppqn_text (const std::string & text);
    void set_ppqn_text (int ppq);
    void lock_main_window (bool lockit);

    int ppqn () const
    {
        return cb_perf().ppqn();
    }

    std::string specify_playlist_folder (const std::string & defalt = "");

    bool specify_playlist ()
    {
        return specify_list_dialog();
    }

    bool open_playlist ()
    {
        return open_list_dialog();
    }

    bool save_playlist ()
    {
        return save_list_dialog();
    }

    bool use_nsm () const
    {
        return m_use_nsm;
    }

    void session_manager (const std::string & text);
    void session_path (const std::string & text);
    void session_display_name (const std::string & text);
    void session_client_id (const std::string & text);
    void session_URL (const std::string & text);
    void session_log_file (const std::string & text);
    void song_path (const std::string & text);
    void last_used_dir (const std::string & text);

protected:

    void use_nsm (bool flag)
    {
        m_use_nsm = flag;
    }

    bool recreate_all_slots ();
    bool refresh_captions ();
    bool load_into_session (const std::string & selectedfile);

protected:                              // performer callbacks

    virtual bool on_group_learn (bool learning) override;
    virtual bool on_group_learn_complete
    (
        const keystroke & k, bool success
    ) override;
    virtual bool on_automation_change (automation::slot s) override;
    virtual bool on_sequence_change
    (
        seq::number seqno, performer::change ctype  // bool recreate
    ) override;
    virtual bool on_trigger_change (seq::number seqno) override;
    virtual bool on_set_change
    (
        screenset::number setno,
        performer::change ctype
    ) override;
    virtual bool on_resolution_change
    (
        int ppqn, midibpm bp, performer::change ch
    ) override;
    virtual bool on_song_action (bool signal, playlist::action) override;

private:                                // overrides of event handlers

    virtual void keyPressEvent (QKeyEvent * event) override;
    virtual void keyReleaseEvent (QKeyEvent *) override;

protected:

    bool report_message
    (
        const std::string & msg, bool good, bool showcancel = true
    );

private:

    virtual void closeEvent (QCloseEvent *) override;
    virtual void changeEvent (QEvent *) override;
    virtual void resizeEvent (QResizeEvent *) override;

private:

    const combolist & ppqn_list () const
    {
        return m_ppqn_list;
    }

    const combolist & beatwidth_list () const
    {
        return m_beatwidth_list;
    }

    const combolist & beats_per_bar_list () const
    {
        return m_beats_per_bar_list;
    }

    void enable_save (bool flag = true);
    void make_perf_frame_in_tab ();
    bool check ();
    std::string filename_prompt
    (
        const std::string & prompt,
        const std::string & filename = ""
    );
    void update_play_status ();
    void update_window_title (const std::string & fn = "");
    void update_recent_files_menu ();
    void create_action_connections ();
    void create_action_menu ();
    void remove_all_editors ();
    void remove_all_live_frames ();
    void set_tap_button (int beats);
    void set_beats_per_minute (double bp, bool blockchange = false);
    void redo_live_frame ();
    bool handle_key_press (const keystroke & k);
    bool handle_key_release (const keystroke & k);
    void show_song_mode (bool songmode);
    bool make_event_frame (int seqid);
    void connect_editor_slots ();
    void connect_nsm_slots ();
    void connect_normal_slots ();
    bool show_open_file_dialog (std::string & selectedfile);
    bool specify_list_dialog ();
    bool open_list_dialog ();
    bool save_list_dialog ();
    bool open_mutes_dialog ();
    bool save_mutes_dialog (const std::string & basename = "");
    void update_tap (midibpm bpm);
    bool set_ppqn_combo ();

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
    qt5nsmanager * m_session_mgr;           /* LATER: unique_ptr()? */
    int m_initial_width;
    int m_initial_height;
    qslivebase * m_live_frame;
    qperfeditex * m_perfedit;
    qperfeditframe64 * m_song_frame64;
    qseqframe * m_edit_frame;
    qseqeventframe * m_event_frame;
    qplaylistframe * m_playlist_frame;
    QMessageBox * m_msg_error;              /* QErrorMessage        */
    QMessageBox * m_msg_save_changes;
    QTimer * m_timer;
    QMenu * m_menu_recent;
    QList<QAction *> m_recent_action_list;
    qsmaintime * m_beat_ind;
    qseditoptions * m_dialog_prefs;
    qsabout * m_dialog_about;
    qsbuildinfo * m_dialog_build_info;
    qsappinfo * m_dialog_app_info;
    qsessionframe * m_session_frame;
    qsetmaster * m_set_master;
    qmutemaster * m_mute_master;
    combolist m_ppqn_list;
    combolist m_beatwidth_list;
    combolist m_beats_per_bar_list;

    /**
     *  Experiment: how to better detect changes in BPM.
     */

    midibpm m_main_bpm;

    /**
     *  Holds the last value of the MIDI-control-in status, used in displaying
     *  the current status when it changed.
     */

    automation::ctrlstatus m_control_status;

    /**
     *  Holds the current setting of the song mode, since
     *  ui->btnSongPlay->isChecked() seems not to be reliable.
     */

    bool m_song_mode;

    /**
     *  Hold the current L/R looping status.
     */

    bool m_is_looping;

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
     *  Holds the last performer tick, so that we can avoid refreshing the
     *  B:B:T display and the beat indicator when not necessary.
     */

    midipulse m_previous_tick;

    /**
     *  Holds the current playing state. Used when needed to update the
     *  stop/pause/play buttons.
     */

    bool m_is_playing_now;

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

    /**
     *  Indicates to shrink or hide some elements of the user interface,
     *  primarily the seqedit frame.
     */

    bool m_shrunken;

signals:

    void signal_set_change (int setno);
    void signal_song_action (int action);

private slots:

    void slot_open_edit_prefs ();
    void slot_summary_save ();
    void slot_tutorial ();
    void slot_user_manual ();
    void slot_set_home ();
    void update_bank (int newBank);
    void update_bank_text ();
    void start_playing ();
    void set_loop (bool loop);
    void pause_playing ();
    void stop_playing ();
    void set_song_mode (bool song_mode);
    void song_recording (bool record);
    void panic ();
    void update_bpm (double bpm);
    void edit_bpm ();
    void update_set_change (int setno);
    void update_song_action (int playaction);
    void update_ppqn_by_text (const QString & text);
    void update_midi_bus (int bindex);
    void update_beats_per_measure (int bmindex);
    void update_beat_length (int blindex);
    void open_recent_file ();
    void new_file ();
    void new_session ();
    bool save_file (const std::string & fname = "", bool updatemenu = true);
    bool save_session ();
    bool save_file_as ();
    bool export_file_as_midi (const std::string & fname = "");
    bool export_file_as_smf_0 (const std::string & fname = "");
    bool export_song (const std::string & fname = "");
    void quit ();
    void import_midi_into_set ();           /* normal import into set       */
    void import_midi_into_session ();       /* import MIDI into session     */
    void import_project ();                 /* import a configuration       */
    void import_playlist ();
    void select_and_load_file ();
    void show_open_list_dialog ();          /* connected only in normal use */
    void show_save_list_dialog ();          /* NOT YET CONNECTED            */
    void show_open_mutes_dialog ();         /* NOT YET CONNECTED            */
    void show_save_mutes_dialog ();         /* NOT YET CONNECTED            */
    void show_qsabout ();
    void show_qsbuildinfo ();
    void show_qsappinfo ();
    void tabWidgetClicked (int newindex);
    void conditional_update ();             /* redraw certain GUI elements  */
    void load_editor (int seqid);
    void load_event_editor (int seqid);
    void load_qseqedit (int seqid);
    void load_qperfedit (bool on);
    void load_live_frame (int ssnum);
    void load_session_frame ();
    void load_set_master ();
    void load_mute_master ();
    void toggle_time_format (bool on);
    void reset_sets ();
    void open_performance_edit ();
    void apply_song_transpose ();
    void reload_mute_groups ();
    void clear_mute_groups ();
    void set_song_mute_on ();
    void set_song_mute_off ();
    void set_song_mute_toggle ();
    void set_playscreen_copy ();
    void set_playscreen_paste ();
    void learn_toggle ();
    void tap ();
    void queue_it ();
    void slot_test ();

private:

    void remove_set_master ();
    void update_time (midipulse tick);

    qt5nsmanager * session ()
    {
        return m_session_mgr;
    }

};          // class qsmainwnd

}           // namespace seq66

#endif      // SEQ66_QSMAINWND_HPP

/*
 * qsmainwnd.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


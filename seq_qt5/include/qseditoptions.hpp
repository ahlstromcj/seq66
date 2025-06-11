#if ! defined SEQ66_QSEDITOPTIONS_HPP
#define SEQ66_QSEDITOPTIONS_HPP

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
 * \file          qseditoptions.hpp
 *
 *      This dialog contains many tabs for editing various aspects of
 *      Seq66 operation.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2025-06-10
 * \license       GNU GPLv2 or above
 */

#include <QDialog>
#include <QDialogButtonBox>

#include "cfg/settings.hpp"             /* seq66::combolist class, helpers  */

namespace Ui
{
    class qseditoptions;
}

class QButtonGroup;
class QComboBox;
class QLineEdit;

namespace seq66
{
    class performer;
    class qsmainwnd;

/**
 *  Provides a dialog class for Edit / Preferences.
 */

class qseditoptions final : public QDialog
{

    friend class qsmainwnd;

    Q_OBJECT

public:

    qseditoptions
    (
        performer & perf,
        QWidget * parent = nullptr
    );
    virtual ~qseditoptions();

    void enable_bus_item (int bus, bool enabled);
    void reload_needed (bool flag);

private:

    void setup_clock_combo_box (int buses, QComboBox * out);
    void setup_input_combo_box (int buses, QComboBox * in);
    void setup_tab_midi_clock ();
    void setup_tab_midi_input ();
    void setup_tab_display ();
    void setup_tab_jack ();
    void setup_tab_play_options ();
    void setup_tab_metronome ();
    void setup_tab_pattern ();
    void setup_tab_session ();
    void exit_required ();
    void set_enabled (QDialogButtonBox::StandardButton bcode, bool on);
    void set_text
    (
        QDialogButtonBox::StandardButton bcode,
        const std::string & text
    );
    void show_button (QDialogButtonBox::StandardButton bcode, bool show);
    void enable_reload_button (bool flag);
    void repopulate_channel_menu (int buss);
    void repopulate_thru_channel_menu (int buss);
    void modify_rc ();
    void modify_metronome (bool enablereload = true);
    void modify_ctrl ();
    void modify_usr ();
    void sync ();               /* makes dialog reflect internal settings   */
    void sync_rc ();            /* makes dialog reflect internal settings   */
    void sync_usr ();           /* makes dialog reflect internal settings   */
    void backup ();             /* backup preferences for cancel-changes    */
    bool set_ppqn_combo ();
    bool set_buffer_size_combo ();
    void set_scaling_fields ();
    void set_set_size_fields ();
    void set_progress_box_fields ();
    void ui_scaling_helper
    (
        const QString & widthtext,
        const QString & heighttext
    );
    void show_sets_mode (rcsettings::setsmode sm);
    void show_start_mode (sequence::playback sm);
    void show_session (usrsettings::session sm);
    void state_unchanged ();
    void state_changed ();
    void state_applied ();
    void activate_ctrl_file ();
    bool load_file_name
    (
        QLineEdit * lineedit,
        const std::string & fileextension
    );
    bool load_executable_name
    (
        QLineEdit * lineedit,
        const std::string & fname
    );
    bool reload_needed () const
    {
        return m_reload_needed;
    }

    const combolist & ppqn_list () const
    {
        return m_ppqn_list;
    }

    const combolist & buffer_size_list () const
    {
        return m_buffer_size_list;
    }

    const performer & perf () const
    {
        return m_perf;
    }

    performer & perf ()
    {
        return m_perf;
    }

private slots:

    void slot_sets_mode (int buttonno);
    void slot_start_mode (int buttonno);
    void slot_jack_mode (int buttonno);
    void slot_jack_connect ();
    void slot_jack_disconnect ();
    void slot_master_cond ();
    void slot_time_master ();
    void slot_transport_support ();
    void slot_jack_midi ();
    void slot_jack_auto_connect ();
    void slot_io_maps ();
#if defined SEQ66_ALLOW_PORTMAP_CLEAR
    void slot_remove_io_maps ();
#endif
    void slot_activate_io_maps ();
    void slot_session (int buttonno);
    void slot_nsm_url ();
    void slot_note_resume ();
    void slot_ppqn_by_text (const QString & text);
    void slot_buffer_size_by_text (const QString & text);
    void slot_use_file_ppqn ();
    void slot_song_record_snap ();
    void slot_key_height ();
    void slot_ui_scaling ();
    void slot_grid_spacing ();
    void slot_set_size_rows ();
    void slot_set_size_columns ();
    void slot_progress_box_width ();
    void slot_progress_box_height ();
    void slot_progress_box_shown ();
    void slot_fingerprint_size ();
#if defined USE_VERBOSE_CHECKBOX
    void slot_verbose_active_click ();
#endif
    void slot_quiet_active_click ();
    void slot_load_most_recent_click ();
    void slot_show_full_paths_click ();
    void slot_long_buss_names_click ();
    void slot_pair_buss_names_click ();
    void slot_lock_main_window_click ();
    void slot_dark_theme_click ();
    void slot_swap_coordinates_click ();
    void slot_bold_grid_slots_click ();
    void slot_gridlines_thick_click ();
    void slot_elliptical_click ();
    void slot_follow_progress_click ();
    void slot_double_click_edit ();
    void slot_global_seq_feature ();
    void slot_rc_save_click ();
#if defined USE_RC_NAME_CHANGE
    void slot_rc_filename ();
    void slot_load_rc_filename ();
#endif
    void slot_usr_save_click ();
#if defined SEQ66_CAN_DEACTIVATE_USR
    void slot_usr_active_click ();
#endif
    void slot_usr_filename ();
    void slot_load_usr_filename ();
    void slot_mutes_save_click ();
    void slot_mutes_active_click ();
    void slot_mutes_filename ();
    void slot_load_mutes_filename ();
    void slot_playlist_save_click ();
    void slot_playlist_active_click ();
    void slot_playlist_filename ();
    void slot_load_playlist_filename ();
    void slot_ctrl_active_click ();
#if defined SEQ66_CAN_SAVE_CTRL
    void slot_ctrl_save_click ();
#endif
    void slot_ctrl_filename ();
    void slot_load_ctrl_filename ();
    void slot_drums_active_click ();
#if defined SEQ66_CAN_SAVE_DRUMS
    void slot_drums_save_click ();
#endif
    void slot_drums_filename ();
    void slot_load_drums_filename ();
    void slot_stylesheet_active_click ();
    void slot_stylesheet_filename ();
    void slot_load_stylesheet_filename ();
    void slot_browser_executable ();
    void slot_load_browser_executable ();
    void slot_pdf_executable ();
    void slot_load_pdf_viewer_executable ();
    void slot_patches_save_now_click ();
    void slot_patches_active_click ();
    void slot_patches_filename ();
    void slot_load_patches_filename ();
    void slot_palette_save_now_click ();
    void slot_palette_save_inverse ();
    void slot_palette_active_click ();
    void slot_palette_filename ();
    void slot_load_palette_filename ();
    void slot_clock_start_modulo (int arg);
    void slot_output_bus (int arg);
    void slot_output_bus_enable ();
    void slot_input_bus (int arg);
    void slot_input_bus_enable ();
    void slot_bpm_precision (int index);
    void slot_tempo_track ();
    void slot_tempo_track_set ();
    void slot_buss_override ();
    void slot_record_by_buss ();
    void slot_record_by_channel ();
    void slot_virtual_ports ();
    void slot_enable_virtual_ports ();
    void slot_virtual_out_count ();
    void slot_virtual_in_count ();

    void slot_metro_beats_per_bar ();
    void slot_metro_beat_width ();
    void slot_metro_main_patch ();
    void slot_metro_main_note ();
    void slot_metro_main_velocity ();
    void slot_metro_main_fraction ();
    void slot_metro_sub_patch ();
    void slot_metro_sub_note ();
    void slot_metro_sub_velocity ();
    void slot_metro_sub_fraction ();
    void slot_metro_buss (int index);
    void slot_metro_channel (int index);
    void slot_metro_count_in ();
    void slot_metro_count_in_measures ();
    void slot_metro_recording ();
    void slot_metro_recording_measures ();
    void slot_metro_record_buss (int index);
    void slot_metro_thru_buss (int index);
    void slot_metro_thru_channel (int index);
    void slot_metro_reload ();
    void slot_escape_pattern ();
    void slot_convert_to_smf_1 ();
    void slot_pattern_arm ();
    void slot_pattern_tighten ();
    void slot_pattern_qrecord ();
    void slot_pattern_notemap ();
    void slot_pattern_new_only ();
    void slot_pattern_record ();
    void slot_pattern_thru ();
    void slot_pattern_wraparound ();
    void slot_record_style (int index);
    void slot_jitter (int jitr);
    void slot_amplitude (int jitr);
    void okay ();
    void cancel ();
    void apply ();
    void reset ();

private:

    Ui::qseditoptions * ui;
    QButtonGroup * m_live_song_buttons;
    qsmainwnd * m_parent_widget;
    performer & m_perf;
    combolist m_ppqn_list;
    combolist m_buffer_size_list;
    bool m_is_initialized;

    /*
     * Backup variables for settings.
     */

    rcsettings m_backup_rc;
    usrsettings m_backup_usr;

    /**
     *  Indicates that a reload is necessary for at least one important
     *  setting.
     */

    bool m_reload_needed;

};          // class qseditoptions

}           // namespace seq66

#endif      // SEQ66_QSEDITOPTIONS_HPP

/*
 * qseditoptions.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


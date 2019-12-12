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
 * \file          usrsettings.cpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  in this application.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-09-23
 * \updates       2019-07-07
 * \license       GNU GPLv2 or above
 *
 *  Note that this module also sets the remaining legacy global variables, so
 *  that they can be used by modules that have not yet been cleaned up.
 *
 *  Now, we finally sat down and did some measurements of the user interface,
 *  to try to figure out the relationships between the screen resolution and
 *  MIDI time resolution, so that we can understand some of the magic numbers
 *  in Seq24.
 *
 *  We start with one clue, a comment in perftime (IIRC) about units being 32
 *  ticks per pixels.  Note that "ticks" is equivalent to MIDI "pulses", and
 *  sometimes the word "division" is used for "pulses".  So let's solidy the
 *  nomenclature and notation here:
 *
\verbatim
    Symbol      Units           Value       Description

     qn         quarter note    -----       The default unit for a MIDI beat
     P0         pulses/qn       192         Seq24's PPQN value, a constant
     P          pulses/qn       -----       Any other selected PPQN value
     R          -----           -----       P / P0
     Wscreen    pixels          1920        Width of the screen, pixels
     Wperfqn    pixels          6           Song editor q-note width, constant
     Zperf      pulses/pixel    32          Song editor default zoom, constant
     Dperf      minor/major     4           Song editor beats shown per measure
     ?          pulses/pixel    -----       GUI-MIDI resolution from selected P
     S          -----           16          seqroll-to-perfroll width ratio
     Zseqmin    pulses/pixel    1           Seq editor max zoom in
     Zseq0      pulses/pixel    2           Seq editor default zoom
     Zseqmax    pulses/pixel    128 (32)    Seq editor max zoom out
\endverbatim
 *
 * Sequence Editor (seqroll):
 *
 *  Careful measuring on my laptop screen shows that the perfroll covers 80
 *  measures over 1920 pixels.
 *
\verbatim
    1920 pixels
    ----------- = 24 pixels/measure = 6 pixels/qn = Wperfqn
    80 measures
\endverbatim
 *
 * Song Editor (perfroll) Zoom:
 *
 *  The value of S = 16 reflects that the sequence editor piano roll,
 *  at its default zoom (2 pulses/pixel), has 16 times the width resolution
 *  of the performer/song editor piano roll (32 pulses/pixel).  This ratio
 *  (at the default zoom) will be preserved no matter what P (PPQN) is
 *  selected for the song.
 *
 *  The sequence editor supports zooms of 1 pulse/pixel, 2 pulses/pixel (it's
 *  default), and 4, 8, 16, and 32 pulses/pixel (the song editor's only zoom).
 *
 * Song Editor (perfedit, perfroll, pertime) Guides:
 *
 *                    pulses        major
 *  measureticks = P0 ------  Dperf -----
 *                     qn           minor
 *
 *     perfedit:  m_ppqn    m_standard_bpm
 *
 * Time Signature:
 *
 *  Changing the beats-per-measure of the seqroll to from the default 4 to 8
 *  makes the measure have 8 major divisions, each with the standard 16 minor
 *  divisions. An added note still covers only 4 minor divisions.
 *
 *  Changing the beat-width of the seqroll from the default 4 to 8 halves the
 *  pixel-width of reach measure.
 */

#include "seq66_features.hpp"           /* SEQ66_USE_ZOOM_POWER_OF_2        */
#include "cfg/settings.hpp"             /* seq66::rc()                      */
#include "cfg/usrsettings.hpp"          /* seq66::usr_settings              */
#include "play/seq.hpp"                 /* seq66::seq::limit()              */

/**
 *  Provide limits for the option "--option scale=x.y".
 */

#define SEQ66_WINDOW_SCALE_MIN          0.5f
#define SEQ66_WINDOW_SCALE_DEFAULT      1.0f
#define SEQ66_WINDOW_SCALE_MAX          3.0f

/**
 *  These control sizes.  We'll try changing them and see what happens.
 *  Increasing these value spreads out the pattern grids a little bit and
 *  makes the Patterns panel slightly bigger.  Seems like it would be
 *  useful to make these values user-configurable.
 *
 *  Copped from globals.h.
 *
 *  Constants for the font class.  The c_text_x and c_text_y constants
 *  help define the "seqarea" size.  It looks like these two values are
 *  the character width (x) and height (y) in pixels.  Thus, these values
 *  would be dependent on the font chosen.  But that, currently, is
 *  hard-wired.  See the c_font_6_12[] array for the default font
 *  specification.
 *
 *  However, please note that font files are not used.  Instead, the fonts
 *  are provided by pixmaps in the <code> src/pixmap </code> directory.
 *  These pixmaps lay out all the characters of the font in a grid.
 *  See the font module for a full description of this grid.
 */

const int c_text_x =  6;            /* does not include the inner padding   */
const int c_text_y = 12;            /* does include the inner padding       */

/**
 *  Constants for the mainwid class.  The c_seqchars_x and c_seqchars_y
 *  constants help define the "seqarea" size.  These look like the number
 *  of characters per line and the number of lines of characters, in a
 *  pattern/sequence box.
 */

const int c_seqchars_x = 15;
const int c_seqchars_y =  5;

/**
 *  The c_seqarea_x and c_seqarea_y constants are derived from the width
 *  and heights of the default character set, and the number of characters
 *  in width, and the number of lines, in a pattern/sequence box.
 *
 *  Compare these two constants to c_seqarea_seq_x(y), which was in
 *  mainwid.h, but is now in this file.
 */

const int c_seqarea_x = c_text_x * c_seqchars_x;
const int c_seqarea_y = c_text_y * c_seqchars_y;

/**
 * Area of what?  Doesn't look at all like it is based on the size of
 * characters.  These are used only in the mainwid module.
 */

const int c_seqarea_seq_x = c_text_x * 13;
const int c_seqarea_seq_y = c_text_y * 2;

/**
 *  These control sizes.  We'll try changing them and see what happens.
 *  Increasing these value spreads out the pattern grids a little bit and
 *  makes the Patterns panel slightly bigger.  Seems like it would be
 *  useful to make these values user-configurable.
 */

const int c_mainwid_border = 0;             // try 2 or 3 instead of 0
const int c_mainwid_spacing = 2;            // try 4 or 6 instead of 2

/**
 *  This constants seems to be created for a future purpose, perhaps to
 *  reserve space for a new bar on the mainwid pane.  But it is used only
 *  in this header file, to define c_mainwid_y, but doesn't add anything
 *  to that value.
 */

const int c_control_height = 0;

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Default constructor.
 */

usrsettings::usrsettings () :
    basesettings                (),
    m_midi_buses                (),     /* [user-midi-bus-definitions]      */
    m_instruments               (),     /* [user-instrument-definitions]    */

    /*
     * [user-interface-settings]
     */

    m_grid_style                (grid::button), // button is better!
    m_grid_brackets             (1),
    m_mainwnd_rows              (SEQ66_DEFAULT_SET_ROWS),
    m_mainwnd_cols              (SEQ66_DEFAULT_SET_COLUMNS),
    m_max_sets                  (SEQ66_DEFAULT_SET_MAX),
    m_window_scale              (SEQ66_WINDOW_SCALE_DEFAULT),
    m_mainwid_border            (0),
    m_mainwid_spacing           (0),
    m_control_height            (0),
    m_current_zoom              (0),            // 0 is unsafe, but a feature
    m_global_seq_feature_save   (true),
    m_seqedit_scale             (c_scales_off),
    m_seqedit_key               (c_key_of_C),
    m_seqedit_bgsequence        (seq::limit()),
    m_use_new_font              (false),
    m_allow_two_perfedits       (false),
    m_h_perf_page_increment     (1),
    m_v_perf_page_increment     (1),
    m_progress_bar_colored      (0),
    m_progress_bar_thick        (true),
    m_inverse_colors            (false),
    m_window_redraw_rate_ms     (c_redraw_ms),  // 40 ms or 20 ms; 25 ms
    m_use_more_icons            (false),
    m_mainwid_block_rows        (1),
    m_mainwid_block_cols        (1),
    m_mainwid_block_independent (false),

    /*
     * The members that follow are not yet part of the .usr file.
     */

    m_text_x                    (0),
    m_text_y                    (0),
    m_seqchars_x                (0),
    m_seqchars_y                (0),

    /*
     * [user-midi-settings]
     */

    m_midi_ppqn                 (SEQ66_DEFAULT_PPQN),
    m_file_ppqn                 (SEQ66_DEFAULT_PPQN),
    m_midi_beats_per_measure    (SEQ66_DEFAULT_BEATS_PER_MEASURE),
    m_midi_bpm_minimum          (0),
    m_midi_beats_per_minute     (SEQ66_DEFAULT_BPM),
    m_midi_bpm_maximum          (c_max_midi_data_value),
    m_midi_beat_width           (SEQ66_DEFAULT_BEAT_WIDTH),
    m_midi_buss_override        (SEQ66_BAD_BUSS),
    m_velocity_override         (SEQ66_PRESERVE_VELOCITY),
    m_bpm_precision             (SEQ66_DEFAULT_BPM_PRECISION),
    m_bpm_step_increment        (SEQ66_DEFAULT_BPM_STEP_INCREMENT),
    m_bpm_page_increment        (SEQ66_DEFAULT_BPM_PAGE_INCREMENT),

    /*
     * Calculated from other member values in the normalize() function.
     */

    m_total_seqs                (0),
    m_seqs_in_set               (0),            // set correctly in normalize()
    m_gmute_tracks              (0),            // same as max-tracks
    m_max_sequence              (0),
    m_mainwnd_x                 (780),          // constant
    m_mainwnd_y                 (412),          // constant
    m_save_user_config          (false),

    /*
     * Constant values.
     */

    mc_min_zoom                 (SEQ66_MINIMUM_ZOOM),
    mc_max_zoom                 (SEQ66_MAXIMUM_ZOOM),
    mc_baseline_ppqn            (SEQ66_DEFAULT_PPQN),

    /*
     * Back to non-constant values.
     */

    m_user_option_daemonize     (false),
    m_user_use_logfile          (false),
    m_user_option_logfile       (),
    m_work_around_play_image    (false),
    m_work_around_transpose_image (false),

    /*
     * [user-ui-tweaks]
     */

    m_user_ui_key_height        (SEQ66_SEQKEY_HEIGHT),
    m_user_ui_seqedit_in_tab    (true),
    m_new_pattern_armed         (false),
    m_new_pattern_thru          (false),
    m_new_pattern_record        (false),
    m_new_pattern_qrecord       (false),
    m_new_pattern_record_style  (0)
{
    // Empty body; it's no use to call normalize() here, see set_defaults().
}

/**
 *  Copy constructor.
 */

usrsettings::usrsettings (const usrsettings & rhs) :
    basesettings                (rhs),
    m_midi_buses                (rhs.m_midi_buses),     // vector
    m_instruments               (rhs.m_instruments),    // vector
    m_grid_style                (rhs.m_grid_style),
    m_grid_brackets             (rhs.m_grid_brackets),
    m_mainwnd_rows              (rhs.m_mainwnd_rows),
    m_mainwnd_cols              (rhs.m_mainwnd_cols),
    m_max_sets                  (rhs.m_max_sets),
    m_window_scale              (rhs.m_window_scale),
    m_mainwid_border            (rhs.m_mainwid_border),
    m_mainwid_spacing           (rhs.m_mainwid_spacing),
    m_control_height            (rhs.m_control_height),
    m_current_zoom              (rhs.m_current_zoom),
    m_global_seq_feature_save   (rhs.m_global_seq_feature_save),
    m_seqedit_scale             (rhs.m_seqedit_scale),
    m_seqedit_key               (rhs.m_seqedit_key),
    m_seqedit_bgsequence        (rhs.m_seqedit_bgsequence),
    m_use_new_font              (rhs.m_use_new_font),
    m_allow_two_perfedits       (rhs.m_allow_two_perfedits),
    m_h_perf_page_increment     (rhs.m_h_perf_page_increment),
    m_v_perf_page_increment     (rhs.m_v_perf_page_increment),
    m_progress_bar_colored      (rhs.m_progress_bar_colored),
    m_progress_bar_thick        (rhs.m_progress_bar_thick),
    m_inverse_colors            (rhs.m_inverse_colors),
    m_window_redraw_rate_ms     (rhs.m_window_redraw_rate_ms),
    m_use_more_icons            (rhs.m_use_more_icons),
    m_mainwid_block_rows        (rhs.m_mainwid_block_rows),
    m_mainwid_block_cols        (rhs.m_mainwid_block_cols),
    m_mainwid_block_independent (rhs.m_mainwid_block_independent),

    /*
     * The members that follow are not yet part of the .usr file.
     */

    m_text_x                    (rhs.m_text_x),
    m_text_y                    (rhs.m_text_y),
    m_seqchars_x                (rhs.m_seqchars_x),
    m_seqchars_y                (rhs.m_seqchars_y),

    /*
     * [user-midi-settings]
     */

    m_midi_ppqn                 (rhs.m_midi_ppqn),
    m_file_ppqn                 (rhs.m_file_ppqn),
    m_midi_beats_per_measure    (rhs.m_midi_beats_per_measure),
    m_midi_bpm_minimum          (rhs.m_midi_bpm_minimum),
    m_midi_beats_per_minute     (rhs.m_midi_beats_per_minute),
    m_midi_bpm_maximum          (rhs.m_midi_bpm_maximum),
    m_midi_beat_width           (rhs.m_midi_beat_width),
    m_midi_buss_override        (rhs.m_midi_buss_override),
    m_velocity_override         (rhs.m_velocity_override),
    m_bpm_precision             (rhs.m_bpm_precision),
    m_bpm_step_increment        (rhs.m_bpm_step_increment),
    m_bpm_page_increment        (rhs.m_bpm_page_increment),

    /*
     * Calculated from other member values in the normalize() function.
     */

    m_total_seqs                (rhs.m_total_seqs),
    m_seqs_in_set               (rhs.m_seqs_in_set),
    m_gmute_tracks              (rhs.m_gmute_tracks),
    m_max_sequence              (rhs.m_max_sequence),
    m_mainwnd_x                 (rhs.m_mainwnd_x),
    m_mainwnd_y                 (rhs.m_mainwnd_y),
    m_save_user_config          (rhs.m_save_user_config),

    /*
     * Constant values.
     */

    mc_min_zoom                 (rhs.mc_min_zoom),
    mc_max_zoom                 (rhs.mc_max_zoom),
    mc_baseline_ppqn            (SEQ66_DEFAULT_PPQN),

    /*
     * Back to non-constant values.
     */

    m_user_option_daemonize     (rhs.m_user_option_daemonize),
    m_user_use_logfile          (rhs.m_user_use_logfile),
    m_user_option_logfile       (rhs.m_user_option_logfile),
    m_work_around_play_image    (rhs.m_work_around_play_image),
    m_work_around_transpose_image (rhs.m_work_around_transpose_image),

    /*
     * [user-ui-tweaks]
     */

    m_user_ui_key_height        (rhs.m_user_ui_key_height),
    m_user_ui_seqedit_in_tab    (rhs.m_user_ui_seqedit_in_tab),
    m_new_pattern_armed         (rhs.m_new_pattern_armed),
    m_new_pattern_thru          (rhs.m_new_pattern_thru),
    m_new_pattern_record        (rhs.m_new_pattern_record),
    m_new_pattern_qrecord       (rhs.m_new_pattern_qrecord),
    m_new_pattern_record_style  (rhs.m_new_pattern_record_style)
{
    // Empty body; no need to call normalize() here.
}

/**
 *  Principal assignment operator.
 */

usrsettings &
usrsettings::operator = (const usrsettings & rhs)
{
    if (this != &rhs)
    {
        basesettings::operator =(rhs),
        m_midi_buses                = rhs.m_midi_buses;
        m_instruments               = rhs.m_instruments;
        m_grid_style                = rhs.m_grid_style;
        m_grid_brackets             = rhs.m_grid_brackets;
        m_mainwnd_rows              = rhs.m_mainwnd_rows;
        m_mainwnd_cols              = rhs.m_mainwnd_cols;
        m_max_sets                  = rhs.m_max_sets;
        m_window_scale              = rhs.m_window_scale;
        m_mainwid_border            = rhs.m_mainwid_border;
        m_mainwid_spacing           = rhs.m_mainwid_spacing;
        m_control_height            = rhs.m_control_height;
        m_current_zoom              = rhs.m_current_zoom;
        m_global_seq_feature_save   = rhs.m_global_seq_feature_save;
        m_seqedit_scale             = rhs.m_seqedit_scale;
        m_seqedit_key               = rhs.m_seqedit_key;
        m_seqedit_bgsequence        = rhs.m_seqedit_bgsequence;
        m_use_new_font              = rhs.m_use_new_font;
        m_allow_two_perfedits       = rhs.m_allow_two_perfedits;
        m_h_perf_page_increment     = rhs.m_h_perf_page_increment;
        m_v_perf_page_increment     = rhs.m_v_perf_page_increment;
        m_progress_bar_colored      = rhs.m_progress_bar_colored;
        m_progress_bar_thick        = rhs.m_progress_bar_thick;
        m_inverse_colors            = rhs.m_inverse_colors;
        m_window_redraw_rate_ms     = rhs.m_window_redraw_rate_ms;
        m_use_more_icons            = rhs.m_use_more_icons;
        m_mainwid_block_rows        = rhs.m_mainwid_block_rows;
        m_mainwid_block_cols        = rhs.m_mainwid_block_cols;
        m_mainwid_block_independent = rhs.m_mainwid_block_independent;

        /*
         * The members that follow are not yet part of the .usr file.
         */

        m_text_x                    = rhs.m_text_x;
        m_text_y                    = rhs.m_text_y;
        m_seqchars_x                = rhs.m_seqchars_x;
        m_seqchars_y                = rhs.m_seqchars_y;

        /*
         * [user-midi-settings]
         */

        m_midi_ppqn                 = rhs.m_midi_ppqn;
        m_file_ppqn                 = rhs.m_file_ppqn;
        m_midi_beats_per_measure    = rhs.m_midi_beats_per_measure;
        m_midi_bpm_minimum          = rhs.m_midi_bpm_minimum;
        m_midi_beats_per_minute     = rhs.m_midi_beats_per_minute;
        m_midi_bpm_maximum          = rhs.m_midi_bpm_maximum;
        m_midi_beat_width           = rhs.m_midi_beat_width;
        m_midi_buss_override        = rhs.m_midi_buss_override;
        m_velocity_override         = rhs.m_velocity_override;
        m_bpm_precision             = rhs.m_bpm_precision;
        m_bpm_step_increment        = rhs.m_bpm_step_increment;
        m_bpm_page_increment        = rhs.m_bpm_page_increment;

        /*
         * Calculated from other member values in the normalize() function.
         *
         *  m_total_seqs                = rhs.m_total_seqs;
         *  m_seqs_in_set               = rhs.m_seqs_in_set;
         *  m_gmute_tracks              = rhs.m_gmute_tracks;
         *  m_max_sequence              = rhs.m_max_sequence;
         *  m_seqarea_x                 = rhs.m_seqarea_x;
         *  m_seqarea_y                 = rhs.m_seqarea_y;
         *  m_seqarea_seq_x             = rhs.m_seqarea_seq_x;
         *  m_seqarea_seq_y             = rhs.m_seqarea_seq_y;
         *  m_mainwid_x                 = rhs.m_mainwid_x;
         *  m_mainwid_y                 = rhs.m_mainwid_y;
         *  m_mainwnd_x                 = rhs.m_mainwnd_x;
         *  m_mainwnd_y                 = rhs.m_mainwnd_y;
         */

        m_save_user_config = rhs.m_save_user_config;
        normalize();

        /*
         * Constant values.  These values cannot be modified.
         *
         * mc_min_zoom              = rhs.mc_min_zoom;
         * mc_max_zoom              = rhs.mc_max_zoom;
         * mc_baseline_ppqn         = rhs.mc_baseline_ppqn;
         */

        m_user_option_daemonize = rhs.m_user_option_daemonize;
        m_user_use_logfile = rhs.m_user_use_logfile;
        m_user_option_logfile = rhs.m_user_option_logfile;
        m_work_around_play_image = rhs.m_work_around_play_image;
        m_work_around_transpose_image = rhs.m_work_around_transpose_image;

        /*
         * [user-ui-tweaks]
         */

        m_user_ui_key_height = rhs.m_user_ui_key_height;
        m_user_ui_seqedit_in_tab = rhs.m_user_ui_seqedit_in_tab;
        m_new_pattern_armed = rhs.m_new_pattern_armed;
        m_new_pattern_thru = rhs.m_new_pattern_thru;
        m_new_pattern_record = rhs.m_new_pattern_record;
        m_new_pattern_qrecord = rhs.m_new_pattern_qrecord;
        m_new_pattern_record_style = rhs.m_new_pattern_record_style;
    }
    return *this;
}

/**
 *  Sets the default values.  For the m_midi_buses and
 *  m_instruments members, this function can only iterate over the
 *  current size of the vectors.  But the default size is zero!
 */

void
usrsettings::set_defaults ()
{
    m_midi_buses.clear();
    m_instruments.clear();
    m_grid_style = grid::button;
    m_grid_brackets = 1;                        // range: -30 to 0 to 30
    m_mainwnd_rows = SEQ66_DEFAULT_SET_ROWS;    // range: 4-8
    m_mainwnd_cols = SEQ66_DEFAULT_SET_COLUMNS; // range: 8-8
    m_max_sets = SEQ66_DEFAULT_SET_MAX;         // range: 32-64
    m_window_scale = SEQ66_WINDOW_SCALE_DEFAULT; // range: 0.5 to 3.0
    m_mainwid_border = c_mainwid_border;        // range: 0-3, try 2 or 3
    m_mainwid_spacing = c_mainwid_spacing;      // range: 2-6, try 4 or 6
    m_control_height = 0;                       // range: 0-4?
    m_current_zoom = SEQ66_DEFAULT_ZOOM;        // range: 1-128
    m_global_seq_feature_save = true;
    m_seqedit_scale = c_scales_off;             // scales::off to < scales::max
    m_seqedit_key = c_key_of_C;                 // range: 0-11
    m_seqedit_bgsequence = seq::limit();        // range -1, 0, 1, 2, ...
    m_use_new_font = true;
    m_allow_two_perfedits = true;
    m_h_perf_page_increment = 4;
    m_v_perf_page_increment = 8;
    m_progress_bar_colored = 0;
    m_progress_bar_thick = true;
    m_inverse_colors = false;
    m_window_redraw_rate_ms = c_redraw_ms;
    m_use_more_icons = false;
    m_mainwid_block_rows = 1;
    m_mainwid_block_cols = 1;
    m_mainwid_block_independent = false;
    m_text_x =  6;                          // range: 6-6
    m_text_y = 12;                          // range: 12-12
    m_seqchars_x = 15;                      // range: 15-15
    m_seqchars_y =  5;                      // range: 5-5
    m_midi_ppqn = SEQ66_DEFAULT_PPQN;       // range: 96 to 960, default 192
    m_file_ppqn = SEQ66_DEFAULT_PPQN;       // range: 96 to 960, default 192
    m_midi_beats_per_measure = SEQ66_DEFAULT_BEATS_PER_MEASURE; // range: 1-16
    m_midi_bpm_minimum = 0;                 // range: 0 to ???
    m_midi_beats_per_minute = SEQ66_DEFAULT_BPM;    // range: 20-500
    m_midi_bpm_maximum = c_max_midi_data_value;     // range: ? to ???
    m_midi_beat_width = SEQ66_DEFAULT_BEAT_WIDTH;   // range: 1-16, powers of 2
    m_midi_buss_override = SEQ66_BAD_BUSS;          // range: 1 to 32
    m_velocity_override = SEQ66_PRESERVE_VELOCITY;  // -1, range: 0 to 127
    m_bpm_precision = SEQ66_DEFAULT_BPM_PRECISION;
    m_bpm_step_increment = SEQ66_DEFAULT_BPM_STEP_INCREMENT;
    m_bpm_page_increment = SEQ66_DEFAULT_BPM_PAGE_INCREMENT;

    /*
     * Constants:
     *
     *  mc_min_zoom
     *  mc_max_zoom
     *  mc_baseline_ppqn
     */

    m_user_option_daemonize = false;
    m_user_use_logfile = false;
    m_user_option_logfile.clear();
    m_work_around_play_image = false;
    m_work_around_transpose_image = false;
    m_user_ui_key_height = SEQ66_SEQKEY_HEIGHT;
    m_user_ui_seqedit_in_tab = true;
    m_new_pattern_armed = false;
    m_new_pattern_thru = false;
    m_new_pattern_record = false;
    m_new_pattern_qrecord = false;
    m_new_pattern_record_style = 0;
    normalize();                            // recalculate derived values
}

/**
 *  Calculate the derived values from the already-set values.
 *  Should we normalize the BPM increment values here, in case they
 *  are irregular?
 *
 *  gmute_tracks() is viable with variable set sizes only if we stick with the
 *  32 sets by 32 patterns, at this time. It's semantic meaning is......
 *
 *  m_max_sequence is now actually a constant (1024), so we enforce that here
 *  now.
 */

void
usrsettings::normalize ()
{
    m_seqs_in_set = m_mainwnd_rows * m_mainwnd_cols;
    m_max_sets = c_max_sequence / m_seqs_in_set;            /* 16 to 32...  */
    m_max_sequence = m_seqs_in_set * m_max_sets;
    m_gmute_tracks = m_seqs_in_set * m_seqs_in_set;         /* TODO!        */
    m_total_seqs = m_seqs_in_set * m_max_sets;
}

/**
 * \getter m_mainwnd_x
 */

int
usrsettings::mainwnd_x () const
{
    if (block_rows() != 1 || block_columns() != 1)
        return 0;
    else
        return scale_size(m_mainwnd_x);
}

/**
 * \getter m_mainwnd_y
 *      Scaled only if window scaling is less than 1.0.
 */

int
usrsettings::mainwnd_y () const
{
    if (block_rows() != 1 || block_columns() != 1)
        return 0;
    else
    {
        return m_window_scale > 1.0f ?
            m_mainwnd_y : int(scale_size(m_mainwnd_y)) ;
    }
}

/**
 *  Adds a user buss to the container, but only does so if the name
 *  parameter is not empty.
 */

bool
usrsettings::add_bus (const std::string & alias)
{
    bool result = ! alias.empty();
    if (result)
    {
        size_t currentsize = m_midi_buses.size();
        usermidibus temp(alias);
        result = temp.is_valid();
        if (result)
        {
            m_midi_buses.push_back(temp);
            result = m_midi_buses.size() == currentsize + 1;
        }
    }
    return result;
}

/**
 *  Adds a user instrument to the container, but only does so if the name
 *  parameter is not empty.
 */

bool
usrsettings::add_instrument (const std::string & name)
{
    bool result = ! name.empty();
    if (result)
    {
        size_t currentsize = m_instruments.size();
        userinstrument temp(name);
        result = temp.is_valid();
        if (result)
        {
            m_instruments.push_back(temp);
            result = m_instruments.size() == currentsize + 1;
        }
    }
    return result;
}

/**
 * \getter m_midi_buses[index] (internal function)
 *      If the index is out of range, then an invalid object is returned.
 *      This invalid object has an empty alias, and all the instrument
 *      numbers are -1.
 */

usermidibus &
usrsettings::private_bus (int index)
{
    static usermidibus s_invalid;                     /* invalid by default */
    if (index >= 0 && index < int(m_midi_buses.size()))
        return m_midi_buses[index];
    else
        return s_invalid;
}

/**
 * \getter m_midi_buses[index].instrument[channel]
 *      Currently this function is used, in the usrfile::parse()
 *      function.
 */

void
usrsettings::set_bus_instrument (int index, int channel, int instrum)
{
    usermidibus & mb = private_bus(index);
    mb.set_instrument(channel, instrum);
}

/**
 * \getter m_instruments[index]
 *      If the index is out of range, then a invalid object is returned.
 *      This invalid object has an empty(), instrument name, false for all
 *      controllers_active[] values, and empty controllers[] string values.
 */

userinstrument &
usrsettings::private_instrument (int index)
{
    static userinstrument s_invalid;
    if (index >= 0 && index < int(m_instruments.size()))
        return m_instruments[index];
    else
        return s_invalid;
}

/**
 * \setter m_midi_instrument_defs[index].controllers, controllers_active
 */

void
usrsettings::set_instrument_controllers
(
    int index,
    int cc,
    const std::string & ccname,
    bool isactive
)
{
    userinstrument & mi = private_instrument(index);
    mi.set_controller(cc, ccname, isactive);
}

/**
 * \setter m_window_scale
 */

void
usrsettings::window_scale (float winscale)
{
    if (winscale >= SEQ66_WINDOW_SCALE_MIN && winscale <= SEQ66_WINDOW_SCALE_MAX)
    {
        m_window_scale = winscale;
        normalize();
    }
}

/**
 * \setter m_grid_style
 */

void
usrsettings::set_grid_style (int gridstyle)
{
    if
    (
        gridstyle >= static_cast<int>(grid::normal) &&
        gridstyle < static_cast<int>(grid::max)
    )
    {
        m_grid_style = static_cast<grid>(gridstyle);
    }
}

/**
 * \setter m_mainwnd_rows
 *      This value is not modified unless the value parameter is
 *      between 4 and 8, inclusive.  The default value is 4.
 *      Dependent values are recalculated after the assignment.
 */

void
usrsettings::mainwnd_rows (int value)
{
    if (value >= SEQ66_MIN_SET_ROWS && value <= SEQ66_MAX_SET_ROWS)
    {
        m_mainwnd_rows = value;
        normalize();
    }
}

/**
 * \setter m_mainwnd_cols
 *      This value is not modified unless the value parameter is
 *      between 8 and 8, inclusive.  The default value is 8.
 *      Dependent values are recalculated after the assignment.
 */

void
usrsettings::mainwnd_cols (int value)
{
    if (value >= SEQ66_MIN_SET_COLUMNS && value <= SEQ66_MAX_SET_COLUMNS)
    {
        m_mainwnd_cols = value;
        normalize();
    }
}

/**
 * \setter m_max_sets
 *      This value is not modified unless the value parameter is between 16
 *      and 32.  The default value is 32.  Dependent values are recalculated
 *      after the assignment.
 *
 * \param value
 *      Provides the desired setting.  It might be modified by the call to
 *      normalize().  Not sure we really need this function.
 */

void
usrsettings::max_sets (int value)
{
    if (value >= SEQ66_MIN_SET_MAX && value <= SEQ66_DEFAULT_SET_MAX)
        m_max_sets = value;

    normalize();
}

/**
 * \setter m_text_x
 *      This value is not modified unless the value parameter is between 6 and
 *      6, inclusive.  The default value is 6.  Dependent values are
 *      recalculated after the assignment.  This value is currently
 *      restricted, until we can code up a bigger font.
 */

void
usrsettings::text_x (int value)
{
    if (value == 6)
    {
        m_text_x = value;
        normalize();
    }
}

/**
 * \setter m_text_y
 *      This value is not modified unless the value parameter is between 12
 *      and 12, inclusive.  The default value is 12.  Dependent values are
 *      recalculated after the assignment.  This value is currently
 *      restricted, until we can code up a bigger font.
 */

void
usrsettings::text_y (int value)
{
    if (value == 12)
    {
        m_text_y = value;
        normalize();
    }
}

/**
 * \setter m_seqchars_x
 *      This affects the size or crampiness of a pattern slot, and for now
 *      we will hardwire it to 15.
 */

void
usrsettings::seqchars_x (int value)
{
    if (value == 15)
    {
        m_seqchars_x = value;
        normalize();
    }
}

/**
 * \setter m_seqchars_y
 *      This affects the size or crampiness of a pattern slot, and for now
 *      we will hardwire it to 5.
 */

void
usrsettings::seqchars_y (int value)
{
    if (value == 5)
    {
        m_seqchars_y = value;
        normalize();
    }
}

/**
 * \setter m_mainwid_border
 *      This value is not modified unless the value parameter is
 *      between 0 and 3, inclusive.  The default value is 0.
 *      Dependent values are recalculated after the assignment.
 */

void
usrsettings::mainwid_border (int value)
{
    if (value >= 0 && value <= 3)
    {
        m_mainwid_border = value;
        normalize();
    }
}

/**
 * \setter m_mainwid_spacing
 *      This value is not modified unless the value parameter is
 *      between 2 and 16, inclusive.  The default value is 2.
 *      Dependent values are recalculated after the assignment.
 */

void
usrsettings::mainwid_spacing (int value)
{
    if (value >= 2 && value <= 16)
    {
        m_mainwid_spacing = value;
        normalize();
    }
}

/**
 * \setter m_control_height
 *      This value is not modified unless the value parameter is
 *      between 0 and 4, inclusive.  The default value is 0.
 *      Dependent values are recalculated after the assignment.
 */

void
usrsettings::control_height (int value)
{
    if (value >= 0 && value <= 4)
    {
        m_control_height = value;
        normalize();
    }
}

/**
 * \setter m_current_zoom
 *      This value is not modified unless the value parameter is
 *      between 1 and 512, inclusive.  The default value is 2.  Note that 0 is
 *      allowed as a special case, which allows the default zoom to be
 *      adjusted when the PPQN value is different from the default.
 */

void
usrsettings::zoom (int value)
{
    bool ok = value >= mc_min_zoom && value <= mc_max_zoom;
    if (ok || value == SEQ66_USE_ZOOM_POWER_OF_2)
        m_current_zoom = value;
}

/**
 * \setter m_midi_ppqn
 *      This value can be set from 96 to 19200 (this upper limit will be
 *      determined by what Seq66 can actually handle).  The default
 *      value is 192.
 */

void
usrsettings::midi_ppqn (int value)
{
    if (value >= SEQ66_MINIMUM_PPQN && value <= SEQ66_MAXIMUM_PPQN)
        m_midi_ppqn = value;
    else if (value == SEQ66_USE_FILE_PPQN)
        m_midi_ppqn = value;
    else
        m_midi_ppqn = SEQ66_DEFAULT_PPQN;
}

/**
 * \setter m_midi_beats_per_measure
 *      This value can be set from 1 to 20.  The default value is 4.
 */

void
usrsettings::midi_beats_per_bar (int value)
{
    if
    (
        value >= SEQ66_MINIMUM_BEATS_PER_MEASURE &&
        value <= SEQ66_MAXIMUM_BEATS_PER_MEASURE
    )
        m_midi_beats_per_measure = value;
}

/**
 * \setter m_midi_bpm_minimum
 *      This value can be set from 20 to 500.  The default value is 120.
 */

void
usrsettings::midi_bpm_minimum (midibpm value)
{
    if (value >= SEQ66_MINIMUM_BPM && value <= SEQ66_MAXIMUM_BPM)
        m_midi_bpm_minimum = value;
}

/**
 * \setter m_midi_beats_minute
 *      This value can be set from 20 to 500.  The default value is 120.
 */

void
usrsettings::midi_beats_per_minute (midibpm value)
{
    if (value >= SEQ66_MINIMUM_BPM && value <= SEQ66_MAXIMUM_BPM)
        m_midi_beats_per_minute = value;
}

/**
 * \setter m_midi_bpm_maximum
 *      This value can be set from 20 to 500.  The default value is 120.
 */

void
usrsettings::midi_bpm_maximum (midibpm value)
{
    if (value >= SEQ66_MINIMUM_BPM && value <= SEQ66_MAXIMUM_BPM)
        m_midi_bpm_maximum = value;
}

/**
 * \setter m_midi_beatwidth
 *      This value can be set to any power of 2 in the range from 1 to 16.
 *      The default value is 4.
 */

void
usrsettings::midi_beat_width (int bw)
{
    if (bw == 1 || bw == 2 || bw == 4 || bw == 8 ||bw == 16)
        m_midi_beat_width = bw;
}

/**
 * \setter m_midi_buss_override
 *      This value can be set from 0 to 31.  The default value is -1, which
 *      means that there is no buss override.  It provides a way to override
 *      the buss number for smallish MIDI files.  It replaces the buss-number
 *      read from the file.  This option is turned on by the --bus option, and
 *      is merely a convenience feature for the quick previewing of a tune.
 *      (It's called "developer laziness".)
 */

void
usrsettings::midi_buss_override (midibyte buss)
{
    if (buss < c_busscount_max || SEQ66_NO_BUSS_OVERRIDE(buss))
        m_midi_buss_override = buss;
}

/**
 * \setter m_velocity_override
 */

void
usrsettings::velocity_override (int vel)
{
    if (vel > SEQ66_MAX_NOTE_ON_VELOCITY)
        vel = SEQ66_MAX_NOTE_ON_VELOCITY;
    else if (vel < 0)
        vel = SEQ66_PRESERVE_VELOCITY;

    m_velocity_override = vel;
}

/**
 * \setter m_bpm_precision
 */

void
usrsettings::bpm_precision (int precision)
{
    if (precision > SEQ66_MAXIMUM_BPM_PRECISION)
        precision = SEQ66_MAXIMUM_BPM_PRECISION;
    else if (precision < SEQ66_MINIMUM_BPM_PRECISION)
        precision = SEQ66_MINIMUM_BPM_PRECISION;

    m_bpm_precision = precision;
}

/**
 * \setter m_bpm_step_increment
 */

void
usrsettings::bpm_step_increment (midibpm increment)
{
    if (increment > SEQ66_MAXIMUM_BPM_INCREMENT)
        increment = SEQ66_MAXIMUM_BPM_INCREMENT;
    else if (increment < SEQ66_MINIMUM_BPM_INCREMENT)
        increment = SEQ66_MINIMUM_BPM_INCREMENT;

    m_bpm_step_increment = increment;
}

/**
 * \setter m_bpm_page_increment
 */

void
usrsettings::bpm_page_increment (midibpm increment)
{
    if (increment > SEQ66_MAXIMUM_BPM_INCREMENT)
        increment = SEQ66_MAXIMUM_BPM_INCREMENT;
    else if (increment < SEQ66_MINIMUM_BPM_INCREMENT)
        increment = SEQ66_MINIMUM_BPM_INCREMENT;

    m_bpm_page_increment = increment;
}

/**
 *  Sets the horizontal page increment size for the horizontal scrollbar of a
 *  perfedit window.  This value ranges from 1 (the original value, really too
 *  small for a "page" operation) to 6 (which is 24 measures, the same as
 *  the typical width of the perfroll)
 */

void
usrsettings::perf_h_page_increment (int inc)
{
    if (inc >= 1 && inc <= 6)
        m_h_perf_page_increment = inc;
}

/**
 *  Sets the vertical page increment size for the vertical scrollbar of a
 *  perfedit window.  This value ranges from 1 (the original value, really too
 *  small for a "page" operation) to 18 (which is 18 tracks, slightly more
 *  than the typical height of the perfroll)
 */

void
usrsettings::perf_v_page_increment (int inc)
{
    if (inc >= 1 && inc <= 18)
        m_v_perf_page_increment = inc;
}

/**
 * \getter m_user_option_logfile
 *
 * \return
 *      This function returns rc().config_directory() + m_user_option_logfile
 *      if the latter does not contain a path marker ("/").  Otherwise, it
 *      returns m_user_option_logfile, which must be a full path specification
 *      to the desired log-file.
 */

std::string
usrsettings::option_logfile () const
{
    std::string result;
    if (! m_user_option_logfile.empty())
    {
        std::size_t slashpos = m_user_option_logfile.find_first_of("/");
        if (slashpos == std::string::npos)
        {
            result = rc().home_config_directory();
            char lastchar = result[result.length() - 1];
            if (lastchar != '/')
                result += '/';
        }
        result += m_user_option_logfile;
    }
    return result;
}

/**
 * \setter m_mainwid_block_rows
 */

void
usrsettings::block_rows (int count)
{
#if defined SEQ66_MAINWID_BLOCK_ROWS_MAX
    if (count > 0 && count <= SEQ66_MAINWID_BLOCK_ROWS_MAX)
        m_mainwid_block_rows = count;
#else
    if (count == 1)
        m_mainwid_block_rows = count;
#endif
}

/**
 * \setter m_mainwid_block_cols
 */

void
usrsettings::block_columns (int count)
{
#if defined SEQ66_MAINWID_BLOCK_ROWS_MAX
    if (count > 0 && count <= SEQ66_MAINWID_BLOCK_COLS_MAX)
        m_mainwid_block_cols = count;
#else
    if (count == 1)
        m_mainwid_block_cols = count;
#endif
}

/**
 *  Provides a debug dump of basic information to help debug a
 *  surprisingly intractable problem with all busses having the name and
 *  values of the last buss in the configuration.
 */

void
usrsettings::dump_summary ()
{
    int buscount = bus_count();
    printf("[user-midi-bus-definitions] %d busses\n", buscount);
    for (int b = 0; b < buscount; ++b)
    {
        const usermidibus & umb = bus(b);
        printf("   [user-midi-bus-%d] '%s'\n", b, umb.name().c_str());
    }

    int instcount = instrument_count();
    printf("[user-instrument-definitions] %d instruments\n", instcount);
    for (int i = 0; i < instcount; ++i)
    {
        const userinstrument & umi = instrument(i);
        printf("   [user-instrument-%d] '%s'\n", i, umi.name().c_str());
    }
    printf("\n");
    printf
    (
        "   mainwnd_rows() = %d\n"
        "   mainwnd_cols() = %d\n"
        "   seqs_in_set() = %d\n"
        "   gmute_tracks() = %d\n"
        "   max_sets() = %d\n"
        "   max_sequence() = %d\n"
        "   text_x(), _y() = %d, %d\n"
        ,
        mainwnd_rows(),
        mainwnd_cols(),
        seqs_in_set(),
        gmute_tracks(),
        max_sets(),
        max_sequence(),
        text_x(), text_y()
    );
    printf
    (
        "   seqchars_x(), _y() = %d, %d\n"
        "   mainwid_border() = %d\n"
        "   mainwid_spacing() = %d\n"
        "   control_height() = %d\n"
        ,
        seqchars_x(), seqchars_y(),
        mainwid_border(),
        mainwid_spacing(),
        control_height()
    );
    printf("\n");
    printf
    (
        "   midi_ppqn() = %d\n"
        "   midi_beats_per_bar() = %d\n"
        "   midi_beats_per_minute() = %g\n"
        "   midi_beat_width() = %d\n"
        "   midi_buss_override() = %d\n"
        ,
        midi_ppqn(),
        midi_beats_per_bar(),
        midi_beats_per_minute(),
        midi_beat_width(),
        int(midi_buss_override())
    );
}

}           // namespace seq66

/*
 * usrsettings.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


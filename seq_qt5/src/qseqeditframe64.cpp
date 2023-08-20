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
 * \file          qseqeditframe64.cpp
 *
 *  This module declares/defines the base class for plastering pattern /
 *  sequence data information in the data area of the pattern editor.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-06-15
 * \updates       2023-08-20
 * \license       GNU GPLv2 or above
 *
 *  The data pane is the drawing-area below the seqedit's event area, and
 *  contains vertical lines whose height matches the value of each data event.
 *  The height of the vertical lines is editable via the mouse.
 *
 *  https://stackoverflow.com/questions/1982986/
 *          scrolling-different-widgets-at-the-same-time
 *
 *      ...Try to make it so that each of the items that needs to scroll in
 *      concert is inside its own QScrollArea. I would then put all those
 *      widgets into one widget, with a QScrollBar underneath (and/or to the
 *      side, if needed).
 *
 *      Designate one of the interior scrolled widgets as the "master",
 *      probably the plot widget. Then do this:
 *
 *      Set every QScrollArea's horizontal scroll bar policy to never show the
 *      scroll bars.  (Set) the master QScrollArea's horizontalScrollBar()'s
 *      rangeChanged(int min, int max) signal to a slot that sets the main
 *      widget's horizontal QScrollBar to the same range. Additionally, it
 *      should set the same range for the other scroll area widget's
 *      horizontal scroll bars.
 *
 *      The horizontal QScrollBar's valueChanged(int value) signal should be
 *      connected to every scroll area widget's horizontal scroll bar's
 *      setValue(int value) slot.  Repeat for vertical scroll bars, if doing
 *      vertical scrolling.
 *
 *  The layout of this frame is depicted here:
 *
\verbatim
                 -----------------------------------------------------------
    QHBoxLayout | seqname : gridsnap : notelength : seqlength : ...         |
                 -----------------------------------------------------------
    QHBoxLayout | undo : redo : tools : zoomin : zoomout : scale : ...      |
    QVBoxLayout  -----------------------------------------------------------
    QScrollArea |   | qseqtime      (0, 1, 1, 1) Scroll horiz only      |   |
                |-- |---------------------------------------------------|---|
                | q |                                                   | v |
                | s |                                                   | e |
                | e |                                                   | r |
    QScrollArea | q | qseqroll      (1, 1, 1, 1) Scroll h/v both        | t |
                | k |                                                   | s |
                | e |                                                   | b |
                | y |                                                   | a |
                | s |                                                   | r |
                |---|---------------------------------------------------|---|
    QScrollArea |   | qtriggeredit  (2, 1, 1, 1) Scroll horiz only      |   |
                |   |---------------------------------------------------|   |
                |   |                                                   |   |
    QScrollArea |   | qseqdata      (3, 1, 1, 1) Scroll horiz only      |   |
                |   |                                                   |   |
                |   | [Note: the height can be reduced to fit in tab.]  |   |
                 -----------------------------------------------------------
                |   | Horizontal scroll bar for QWidget container       |   |
                 -----------------------------------------------------------
    QHBoxLayout | Events : ...                                              |
                 -----------------------------------------------------------
\endverbatim
 *
 */

#include <QMenu>
#include <QPaintEvent>
#include <QScrollBar>

#include "midi/controllers.hpp"         /* seq66::controller_name()         */
#include "play/performer.hpp"           /* seq66::performer reference       */
#include "util/strfunctions.hpp"        /* seq66::string_to_int()           */
#include "qlfoframe.hpp"                /* seq66::qlfoframe dialog class    */
#include "qpatternfix.hpp"              /* seq66::qpatternfix dialog class  */
#include "qseqdata.hpp"                 /* seq66::qseqdata panel            */
#include "qseqeditex.hpp"               /* seq66::qseqeditex class          */
#include "qseqeditframe64.hpp"          /* seq66::qseqeditframe64 class     */
#include "qseqkeys.hpp"                 /* seq66::qseqkeys panel            */
#include "qseqroll.hpp"                 /* seq66::qseqroll panel            */
#include "qseqtime.hpp"                 /* seq66::qseqtime panel            */
#include "qstriggereditor.hpp"          /* seq66::qstriggereditor events    */
#include "qt5_helpers.hpp"              /* seq66::qt(), qt_set_icon()       */

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#if defined SEQ66_QMAKE_RULES
#include "forms/ui_qseqeditframe64.h"
#else
#include "forms/qseqeditframe64.ui.h"
#endif

/*
 *  We prefer to load the pixmaps on the fly, rather than deal with those
 *  friggin' resource files.
 */

#include "pixmaps/bus.xpm"
#include "pixmaps/down.xpm"
#include "pixmaps/drum.xpm"
#include "pixmaps/finger.xpm"
#include "pixmaps/follow.xpm"
#include "pixmaps/key.xpm"
#include "pixmaps/loop.xpm"
#include "pixmaps/length_short.xpm"     /* not length.xpm, it is too long   */
#include "pixmaps/menu_empty.xpm"
#include "pixmaps/menu_empty_inv.xpm"
#include "pixmaps/menu_full.xpm"
#include "pixmaps/menu_full_inv.xpm"
#include "pixmaps/midi.xpm"
#include "pixmaps/note_length.xpm"
#include "pixmaps/play.xpm"
#include "pixmaps/play_on.xpm"
#include "pixmaps/q_rec.xpm"
#include "pixmaps/q_rec_on.xpm"
#include "pixmaps/quantize.xpm"
#include "pixmaps/rec.xpm"
#include "pixmaps/rec_on.xpm"
#include "pixmaps/redo.xpm"
#include "pixmaps/scale.xpm"
#include "pixmaps/sequences.xpm"
#include "pixmaps/snap.xpm"
#include "pixmaps/thru.xpm"
#include "pixmaps/thru_on.xpm"
#include "pixmaps/tools.xpm"
#include "pixmaps/transpose.xpm"
#include "pixmaps/undo.xpm"
#include "pixmaps/zoom.xpm"             /* zoom_in/_out combo-box           */
#include "pixmaps/chord3-inv.xpm"

/*
 *  Do not document the name space.
 */

namespace seq66
{

/*
 *  We have an issue that using the new (and larger) qseqeditframe64 class in
 *  the Edit tab causes the whole main window to increase in size, which
 *  stretches the Live frame's pattern slots too much vertically.  So let's
 *  try to shrink the seq-edit's piano roll to compensate.  Nope. However, we
 *  now get around that issue by halving the height of the qseqdata pane and
 *  removing the "Map" button.
 */

/**
 *  Static data members.  These items apply to all of the instances of
 *  seqedit.
 *
 *  The snap and note-length defaults would be good to write to the "user"
 *  configuration file.  The scale and key would be nice to write to the
 *  proprietary section of the MIDI song.  Or, even more flexibly, to each
 *  sequence, if that makes sense to do, since all tracks would generally be
 *  in the same key.  Right, Charles Ives?
 *
 *  Note that, currently, that some of these "initial values" are modified, so
 *  that they are "contagious".  That is, the next sequence to be opened in
 *  the sequence editor will adopt these values.  This is a long-standing
 *  feature of Seq24, but strikes us as a bit surprising.
 *
 *  If we just double the PPQN, then the snap divisor becomes 32, and the snap
 *  interval is a 32nd note.  We would like to keep it at a 16th note.  We
 *  correct the snap ticks to the actual PPQN ratio.
 */

int qseqeditframe64::sm_initial_snap         = c_base_ppqn / 4;
int qseqeditframe64::sm_initial_note_length  = c_base_ppqn / 4;
int qseqeditframe64::sm_initial_chord        = 0;

/**
 *  To reduce the amount of written code, we use the following count to cover
 *  beats/measure ranging from 1 to 16, plus an additional value of 32.  The user
 *  can always manually edit odd beats/measure values.
 */

static const int s_beat_measure_count   = 16;

/**
 *  These static items are used to fill in and select the proper zoom values for
 *  the grids.  Note that they are not members, though they could be.
 *  Also note the features of these zoom numbers:
 *
 *      -#  The lowest zoom value is defined in qeditbase and usrsettings.
 *      -#  The highest zoom value is defined in qeditbase and usrsettings.
 *      -#  The zoom values are all powers of 2.
 *      -#  The zoom values are further constrained by the configured values
 *          of usr().min_zoom() and usr().max_zoom().
 *      -#  The default zoom is specified in the user's "usr" file, and
 *          the default value of this default zoom is 2.
 *
 * \todo
 *      We still need to figure out what to do with a zoom of 0, which
 *      is supposed to tell Seq66 to auto-adjust to the current PPQN.
 *
 *  Please note that the list of zooms is now maintained in the settings
 *  module.
 */

/*
 * For set_event_entry calls.
 */

using event_popup_pair = struct
{
    std::string epp_name;
    midibyte epp_status;
};

/*
 *  This array must exactly match the qseqeditframe64::event_index enumeration.
 */

event_popup_pair
s_event_items [] =
{
    { "Note On Velocity",   EVENT_NOTE_ON               },
    { "Note Off Velocity",  EVENT_NOTE_OFF              },
    { "Aftertouch",         EVENT_AFTERTOUCH            },
    { "Control Change",     EVENT_CONTROL_CHANGE        },
    { "Program Change",     EVENT_PROGRAM_CHANGE        },
    { "Channel Pressure",   EVENT_CHANNEL_PRESSURE      },
    { "Pitch Wheel",        EVENT_PITCH_WHEEL           },
    { "Tempo",              EVENT_META_SET_TEMPO        },      /* special  */
    { "Time Signature",     EVENT_META_TIME_SIGNATURE   }       /* ditto    */
};

/**
 *
 * \param p
 *      Provides the performer object to use for interacting with this
 *      sequence.  Among other things, this object provides the active PPQN.
 *
 * \param s
 *      Provides the reference to the sequence represented by this seqedit.
 *
 * \param parent
 *      Provides the parent window/widget for this container window.  Defaults
 *      to null. Normally a pointer to a qseditex fram or the qsmainwnd's
 *      EditTab is passed in this parameter.  If the former, we want to be
 *      able to change it's title.
 *
 * \param shorter
 *      If true, the data window is halved in size, and some controls are
 *      hidden, to compact the editor for use a tab. If false, we assume
 *      that the \a parent parameter is a qseqeditex frame.
 */

qseqeditframe64::qseqeditframe64
(
    performer & p,
    sequence & s,
    QWidget * parent,
    bool shorter
) :
    qseqframe               (p, s, parent),
    performer::callbacks    (p),
    ui                      (new Ui::qseqeditframe64),
    m_qseqeditex_frame      (nullptr),              /* TBD              */
    m_short_version         (shorter),              /* short_version()  */
    m_is_looping            (false),
    m_lfo_wnd               (nullptr),
    m_patternfix_wnd        (nullptr),
    m_tools_popup           (nullptr),
    m_tools_harmonic        (nullptr),
    m_tools_pitch           (nullptr),
    m_tools_timing          (nullptr),
    m_sequences_popup       (nullptr),
    m_events_popup          (nullptr),
    m_minidata_popup        (nullptr),
    m_measures_list         (measure_items()),      /* see settings module  */
    m_measures              (0),
    m_beats_per_bar_list    (beats_per_bar_items()),
    m_beats_per_bar         (0),                    /* set in ctor body     */
    m_beats_per_bar_to_log  (0),                    /* set in ctor body     */
    m_beatwidth_list        (beatwidth_items()),    /* see settings module  */
    m_beat_width            (0),                    /* set in ctor body     */
    m_beat_width_to_log     (0),                    /* set in ctor body     */
    m_snap_list             (snap_items()),         /* see settings module  */
    m_snap                  (sm_initial_snap),
    m_zoom_list             (zoom_items()),         /* see settings module  */
    m_rec_vol_list          (rec_vol_items()),      /* see settings module  */
    m_note_length           (sm_initial_note_length),
    m_scale                 (0),                    /* set in ctor body     */
    m_chord                 (0),
    m_key                   (usr().seqedit_key()),
    m_bgsequence            (0),                    /* set in ctor body     */
    m_edit_bus              (0),
    m_edit_channel          (0),                    /* 0-15, null           */
    m_first_event           (max_midibyte()),
    m_first_event_name      ("(no events)"),
    m_have_focus            (false),
    m_edit_mode             (perf().edit_mode(s.seq_number())),
    m_last_record_style     (recordstyle::merge),
    m_timer                 (nullptr)
{
    std::string seqname = "No sequence!";
    int loopcountmax = 0;
    bool isnewpattern = false;
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);             /* part of issue #4     */
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    if (! shorter)
        m_qseqeditex_frame = dynamic_cast<qseqeditex *>(parent);

    /*
     * These indirect settings have side-effects, so we just assign the
     * values to the members.
     *
     * set_beats_per_bar(track().get_beats_per_bar(), qbase::status::startup);
     * set_beat_width(track().get_beat_width(), qbase::status::startup);
     * set_measures(track().get_measures(), qbase::status::startup);
     */

    m_beats_per_bar_to_log = m_beats_per_bar = track().get_beats_per_bar();
    m_beat_width_to_log = m_beat_width = track().get_beat_width();
    m_measures = track().get_measures();
    m_scale = track().musical_scale();
    m_edit_bus = track().seq_midi_bus();
    m_edit_channel = track().midi_channel();        /* 0-15, null           */
    seqname = track().name();
    loopcountmax = track().loop_count_max();
    initialize_panels();                            /* uses seq_pointer()   */
    if (usr().global_seq_feature())
    {
        set_scale(usr().seqedit_scale(), qbase::status::startup);
        set_key(usr().seqedit_key(), qbase::status::startup);
        set_background_sequence
        (
            usr().seqedit_bgsequence(), qbase::status::startup
        );
        track().musical_scale(usr().seqedit_scale());
        track().musical_key(usr().seqedit_key());
        track().background_sequence(usr().seqedit_bgsequence());
    }
    else
    {
        set_scale(track().musical_scale(), qbase::status::startup);
        set_key(track().musical_key(), qbase::status::startup);
        set_background_sequence
        (
            track().background_sequence(), qbase::status::startup
        );
    }
    if (track().is_new_pattern())
    {
        /*
         *  This check looks only for "Untitled" and no events. Causes
         *  opening this window to unmute patterns in generic *.mid files.
         */

        isnewpattern = true;
        play_change(usr().new_pattern_armed());
        thru_change(usr().new_pattern_thru());
        record_change(usr().new_pattern_record());
        q_record_change(usr().new_pattern_qrecord());
    }

    /*
     *  Sequence Number Label
     */

    std::string seqtext = std::to_string(track().seq_number());
    QString labeltext = qt(seqtext);
    ui->m_label_seqnumber->setText(labeltext);

    /*
     * Sequence Title
     */

    ui->m_entry_name->setText(qt(seqname));
    connect
    (
        ui->m_entry_name, SIGNAL(textChanged(const QString &)),
        this, SLOT(update_seq_name())
    );

    /*
     * Beats Per Bar, preliminary (global) value.  Fill options for the beats
     * per measure combo-box, and set the default.  These range from 1 to 16,
     * plus a value of 32.
     *
     * How to handle values outside the combobox? Pass the string as a
     * parameter to fill_combobox().
     *
     * We also set up the preliminary values of beat-width here to avoid doing
     * it while connected.
     */

    bool got_timesig = detect_time_signature(); /* find beats/bar & width   */
    std::string bstring = std::to_string(m_beats_per_bar);
    (void) fill_combobox(ui->m_combo_bpm, beats_per_bar_list(), bstring);
    bstring = std::to_string(m_beat_width);
    (void) fill_combobox(ui->m_combo_bw, beatwidth_list(), bstring);

    /*
     * Doesn't seem to be needed, as changing the index also changes the text.
     *
     *  connect
     *  (
     *      ui->m_combo_bpm, SIGNAL(currentIndexChanged(int)),
     *      this, SLOT(update_beats_per_bar(int))
     *  );
     */

    qt_set_icon(down_xpm, ui->m_button_bpm);
    connect
    (
        ui->m_combo_bpm, SIGNAL(currentTextChanged(const QString &)),
        this, SLOT(text_beats_per_bar(const QString &))
    );
    connect
    (
        ui->m_button_bpm, SIGNAL(clicked(bool)),
        this, SLOT(reset_beats_per_bar())
    );

    /*
     * Beat Width (denominator of time signature).  Fill the options for
     * the beats per measure combo-box, and set the default.
     */

    qt_set_icon(down_xpm, ui->m_button_bw);
    connect
    (
        ui->m_combo_bw, SIGNAL(currentTextChanged(const QString &)),
        this, SLOT(text_beat_width(const QString &))
    );
    connect
    (
        ui->m_button_bw, SIGNAL(clicked(bool)),
        this, SLOT(reset_beat_width())
    );

    /*
     * Button to log the current time signature as a Meta event at the
     * current "L" marker.
     */

    set_log_timesig_text(m_beats_per_bar_to_log, m_beat_width_to_log);
    set_log_timesig_status(false); // ui->m_button_log_timesig->setEnabled(false);
    connect
    (
        ui->m_button_log_timesig, SIGNAL(clicked(bool)),
        this, SLOT(slot_log_timesig())
    );

    /*
     * Pattern Length in Measures. Fill the options for the beats per measure
     * combo-box, and set the default.
     */

    std::string mstring = std::to_string(m_measures);
    (void) fill_combobox(ui->m_combo_length, measures_list(), mstring);

    /*
     * Doesn't seem to be needed, as changing the index also changes the text.
     *
     *    connect
     *    (
     *        ui->m_combo_length, SIGNAL(currentIndexChanged(int)),
     *        this, SLOT(update_measures(int))
     *    );
     */

    qt_set_icon(length_short_xpm, ui->m_button_length);
    connect
    (
        ui->m_combo_length, SIGNAL(currentTextChanged(const QString &)),
        this, SLOT(text_measures(const QString &))
    );
    connect
    (
        ui->m_button_length, SIGNAL(clicked(bool)),
        this, SLOT(reset_measures())
    );

    /*
     * Transposable button.
     */

    bool can_transpose = track().transposable();
    if (shorter)
    {
        ui->m_toggle_drum->hide();      /* ui->m_toggle_transpose->hide()   */
        ui->m_map_notes->hide();
        ui->spacer_button_4->hide();
        ui->spacer_button_5->hide();
        ui->m_button_loop->hide();
    }
    else
    {
        /*
         * Drum-Mode Button.  Qt::NoFocus is the default focus policy.
         * Also includes the new loop button, which does the same function
         * as in the main window.
         */

        ui->m_toggle_drum->setAutoDefault(false);
        ui->m_toggle_drum->setCheckable(true);
        qt_set_icon(drum_xpm, ui->m_toggle_drum);
        qt_set_icon(loop_xpm, ui->m_button_loop);
        connect
        (
            ui->m_toggle_drum, SIGNAL(toggled(bool)),
            this, SLOT(editor_mode(bool))
        );

        std::string keyname = perf().automation_key(automation::slot::loop_LR);
        tooltip_with_keystroke(ui->m_button_loop, keyname);
        connect
        (
            ui->m_button_loop, SIGNAL(toggled(bool)),
            this, SLOT(loop_mode(bool))
        );
        connect
        (
            ui->m_map_notes, SIGNAL(clicked(bool)),
            this, SLOT(remap_notes())
        );
        ui->m_map_notes->setEnabled(can_transpose);
    }

    /*
     *  Transpose button.  Qt::NoFocus is the default focus policy.
     *  When implement, add tooltip extension for mod_transpose_song. (???)
     */

    qt_set_icon(transpose_xpm, ui->m_toggle_transpose);
    ui->m_toggle_transpose->setCheckable(true);
    ui->m_toggle_transpose->setChecked(can_transpose);
    ui->m_toggle_transpose->setAutoDefault(false);
    connect
    (
        ui->m_toggle_transpose, SIGNAL(toggled(bool)),
        this, SLOT(transpose(bool))
    );
    set_transpose_image(can_transpose);

    /*
     * Chord button and combox-box.  See chord_name_ptr() in the scales.hpp
     * header file.
     */

    qt_set_icon(chord3_inv_xpm, ui->m_button_chord);
    connect
    (
        ui->m_button_chord, SIGNAL(clicked(bool)),
        this, SLOT(reset_chord())
    );
    for (int chord = 0; /* condition in loop */ ; ++chord)
    {
        const char * cn = chord_name_ptr(chord);
        if (strlen(cn) > 0)
        {
            QString combo_text = cn;
            ui->m_combo_chord->insertItem(chord, combo_text);
        }
        else
            break;
    }
    ui->m_combo_chord->setCurrentIndex(m_chord);
    connect
    (
        ui->m_combo_chord, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_chord(int))
    );
    set_chord(m_chord);

    /*
     *  MIDI buss items discovered at startup-time.  Not sure if we want to
     *  use the button to reset the buss, or increment to the next buss.
     *  The combo-box loop could be moved to a repopulate_midibuss_combo()
     *  function.
     */

    qt_set_icon(bus_xpm, ui->m_button_bus);
    connect
    (
        ui->m_button_bus, SIGNAL(clicked(bool)),
        this, SLOT(reset_midi_bus())
    );

    const clockslist & opm = output_port_map();
    mastermidibus * mmb = perf().master_bus();
    if (not_nullptr(mmb))
    {
        bussbyte seqbuss = m_edit_bus;  /* seq_pointer()->seq_midi_bus()    */
        int buses = opm.active() ? opm.count() : mmb->get_num_out_buses() ;
        for (int bus = 0; bus < buses; ++bus)
        {
            e_clock ec;
            std::string busname;
            if (perf().ui_get_clock(bussbyte(bus), ec, busname))
            {
                bool unavailable = perf().is_port_unavailable
                (
                    bussbyte(bus), midibase::io::output
                );
                bool disabled = ec == e_clock::disabled || unavailable;
                ui->m_combo_bus->addItem(qt(busname));
                if (disabled)
                    enable_combobox_item(ui->m_combo_bus, bus, false);
            }
        }
        ui->m_combo_bus->setCurrentIndex(int(seqbuss));
    }
    connect
    (
        ui->m_combo_bus, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_midi_bus(int))
    );

    /*
     *  MIDI channels.  Not sure if we want to use the button to reset the
     *  channel, or increment to the next channel.
     */

    qt_set_icon(midi_xpm, ui->m_button_channel);
    connect
    (
        ui->m_button_channel, SIGNAL(clicked(bool)),
        this, SLOT(reset_midi_channel())
    );

    /*
     * Undo and Redo Buttons.
     */

    qt_set_icon(undo_xpm, ui->m_button_undo);
    connect(ui->m_button_undo, SIGNAL(clicked(bool)), this, SLOT(undo()));

    qt_set_icon(redo_xpm, ui->m_button_redo);
    connect(ui->m_button_redo, SIGNAL(clicked(bool)), this, SLOT(redo()));

    /*
     * Quantize Button.  This is the "Q" button, and indicates to quantize
     * (just?) notes.  Compare it to the Quantize menu entry, which quantizes
     * events.  Note the usage of std::bind()... this feature requires C++11.
     * Also see q_record_change(), which handles on-the-fly quantization while
     * recording.
     */

    tooltip_with_keystroke(ui->m_button_undo, "q");
    qt_set_icon(quantize_xpm, ui->m_button_quantize);
    connect
    (
        ui->m_button_quantize, &QPushButton::clicked,
        std::bind
        (
            &qseqeditframe64::do_action, this,
            eventlist::edit::quantize_notes, 0
        )
    );

    /*
     * Follow Progress Button.
     */

    std::string keyname =
        perf().automation_key(automation::slot::follow_transport);

    tooltip_with_keystroke(ui->m_toggle_follow, keyname);
    qt_set_icon(follow_xpm, ui->m_toggle_follow);
    ui->m_toggle_follow->setEnabled(true);
    ui->m_toggle_follow->setCheckable(true);

    /*
     * Qt::NoFocus is the default focus policy.
     */

    ui->m_toggle_follow->setAutoDefault(false);
    connect
    (
        ui->m_toggle_follow, SIGNAL(toggled(bool)),
        this, SLOT(slot_follow(bool))
    );

    /**
     *  Fill "Snap" and "Note" Combo Boxes: To reduce the amount of written
     *  code, we use a static array to initialize some of these menu entries.
     *  0 denotes the separator.  This same setup is used to set up both the
     *  snap and note menu, since they are exactly the same.  Saves a *lot* of
     *  code.  This code was copped from the Gtkmm 2.4 seqedit class and
     *  adapted to Qt 5.
     */

    (void) fill_combobox(ui->m_combo_snap, snap_list(), "16", "1/"); /* 1/16th */
    (void) fill_combobox(ui->m_combo_note, snap_list(), "16", "1/"); /* ditto  */
    connect
    (
        ui->m_combo_snap, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_grid_snap(int))
    );
    connect
    (
        ui->m_combo_note, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_note_length(int))
    );

    qt_set_icon(snap_xpm, ui->m_button_snap);
    connect
    (
        ui->m_button_snap, SIGNAL(clicked(bool)),
        this, SLOT(reset_grid_snap())
    );

    set_snap(rescale_tick(sm_initial_snap, perf().ppqn(), usr().base_ppqn()));
    set_note_length
    (
        rescale_tick(sm_initial_note_length, perf().ppqn(), usr().base_ppqn())
    );
    qt_set_icon(note_length_xpm, ui->m_button_note);
    connect
    (
        ui->m_button_note, SIGNAL(clicked(bool)),
        this, SLOT(reset_note_length())
    );

    /*
     *  Zoom In and Zoom Out:  Rather than two buttons, we use one and
     *  a combo-box.
     */

    qt_set_icon(zoom_xpm, ui->m_button_zoom);
    connect
    (
        ui->m_button_zoom, SIGNAL(clicked(bool)),
        this, SLOT(slot_reset_zoom())
    );
    (void) fill_combobox(ui->m_combo_zoom, zoom_list(), "2", "1:");
    connect
    (
        ui->m_combo_zoom, SIGNAL(currentIndexChanged(int)),
        this, SLOT(slot_update_zoom(int))
    );

    int zoom = usr().zoom();
    if (usr().adapt_zoom())
        zoom = zoom_power_of_2(perf().ppqn());

    set_zoom(zoom);

    /*
     * Musical Keys Button and Combo-Box. See the scales.hpp header file.
     */

    qt_set_icon(key_xpm, ui->m_button_key);
    connect
    (
        ui->m_button_key, SIGNAL(clicked(bool)),
        this, SLOT(reset_key())
    );

    for (int key = 0; key < c_octave_size; ++key)
    {
        QString combo_text = qt(musical_key_name(key));
        ui->m_combo_key->insertItem(key, combo_text);
    }
    ui->m_combo_key->setCurrentIndex(m_key);
    connect
    (
        ui->m_combo_key, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_key(int))
    );

    /*
     * Musical Scales Button and Combo-Box. See c_scales_text[] in the
     * scales.hpp header file.
     */

    qt_set_icon(scale_xpm, ui->m_button_scale);
    connect
    (
        ui->m_button_scale, SIGNAL(clicked(bool)),
        this, SLOT(reset_scale())
    );
    for (int scale = c_scales_off; scale < c_scales_max; ++scale)
    {
        QString combo_text = qt(musical_scale_name(scale));
        ui->m_combo_scale->insertItem(scale, combo_text);
    }
    ui->m_combo_scale->setCurrentIndex(m_scale);
    connect
    (
        ui->m_combo_scale, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_scale(int))
    );

    /*
     * Tools Pop-up Menu Button.  Must come after the scale has been
     * determined.
     */

    qt_set_icon(tools_xpm, ui->m_button_tools);
    connect(ui->m_button_tools, SIGNAL(clicked(bool)), this, SLOT(tools()));
    popup_tool_menu();

    /*
     * Background Sequence/Pattern Selectors.
     */

    qt_set_icon(sequences_xpm, ui->m_button_sequence);
    connect
    (
        ui->m_button_sequence, SIGNAL(clicked(bool)),
        this, SLOT(sequences())
    );
    popup_sequence_menu();              /* create the initial popup menu    */

    /*
     * Tiny vertical zoom keys
     */

    connect
    (
        ui->btnKeyVZoomIn, SIGNAL(clicked(bool)),
        this, SLOT(v_zoom_in())
    );
    connect
    (
        ui->btnKeyVZoomReset, SIGNAL(clicked(bool)),
        this, SLOT(reset_v_zoom())
    );
    connect
    (
        ui->btnKeyVZoomOut, SIGNAL(clicked(bool)),
        this, SLOT(v_zoom_out())
    );

    /*
     * Note-entry mode
     */

    qt_set_icon(finger_xpm, ui->m_button_note_entry);
    ui->m_button_note_entry->setCheckable(true);
    ui->m_button_note_entry->setAutoDefault(false);
    ui->m_button_note_entry->setChecked(false);
    connect
    (
        ui->m_button_note_entry, SIGNAL(toggled(bool)),
        this, SLOT(note_entry(bool))
    );

    /*
     * Event Selection Button and Popup Menu for qseqdata.
     */

    connect
    (
        ui->m_button_event, SIGNAL(clicked(bool)),
        this, SLOT(events())
    );
    if (got_timesig)
        set_data_type(EVENT_META_TIME_SIGNATURE);
    else
        set_data_type(EVENT_NOTE_ON);

    /*
     * Event Data Presence-Indicator Button and Popup Menu.
     */

    connect
    (
        ui->m_button_data, SIGNAL(clicked(bool)),
        this, SLOT(data())
    );

    /*
     * LFO Button.
     */

     ui->m_button_lfo->setEnabled(true);
     connect
     (
        ui->m_button_lfo, SIGNAL(clicked(bool)),
        this, SLOT(show_lfo_frame())
     );

     /*
      * Loop-count Spin Box.
      */

    ui->m_spin_loop_count->setValue(loopcountmax);
    ui->m_spin_loop_count->setReadOnly(false);
    connect
    (
        ui->m_spin_loop_count, SIGNAL(valueChanged(int)),
        this, SLOT(update_loop_count(int))
    );

    /*
     * Enable (unmute) Play Button. It's not the triangular play button, it's
     * the box-to-MIDI-port button.
     */

    qt_set_icon(play_xpm, ui->m_toggle_play);
    ui->m_toggle_play->setCheckable(true);
    connect
    (
        ui->m_toggle_play, SIGNAL(toggled(bool)),
        this, SLOT(play_change(bool))
    );

    /*
     * MIDI Thru Button.
     */

    qt_set_icon(thru_xpm, ui->m_toggle_thru);
    ui->m_toggle_thru->setCheckable(true);
    connect
    (
        ui->m_toggle_thru, SIGNAL(toggled(bool)),
        this, SLOT(thru_change(bool))
    );

    /*
     * MIDI Record Button.
     */

    qt_set_icon(rec_xpm, ui->m_toggle_record);
    connect
    (
        ui->m_toggle_record, SIGNAL(toggled(bool)),
        this, SLOT(record_change(bool))
    );

    /*
     * MIDI Quantized Record Button.
     */

    qt_set_icon(q_rec_xpm, ui->m_toggle_qrecord);
    ui->m_toggle_qrecord->setCheckable(true);
    connect
    (
        ui->m_toggle_qrecord, SIGNAL(toggled(bool)),
        this, SLOT(q_record_change(bool))
    );

    /*
     * Recording Merge, Replace, Extend Button.  Provides a button to set the
     * recording style to "merge" (when looping, merge new incoming events
     * into the pattern), "overwrite" (replace events with incoming events),
     * and "expand" (increase the size of the loop to accomodate new events).
     */

    int lrmerge = usr().grid_record_code(recordstyle::merge);
    int lrreplace = usr().grid_record_code(recordstyle::overwrite);
    int lrexpand = usr().grid_record_code(recordstyle::expand);
    int lroneshot = usr().grid_record_code(recordstyle::oneshot);
    int lrreset = usr().grid_record_code(recordstyle::oneshot_reset);
    ui->m_combo_rec_type->insertItem(lrmerge, "Merge");
    ui->m_combo_rec_type->insertItem(lrreplace, "Overwrite");
    ui->m_combo_rec_type->insertItem(lrexpand, "Expand");
    ui->m_combo_rec_type->insertItem(lroneshot, "One-shot");
    ui->m_combo_rec_type->insertItem(lrreset, "1-shot reset");
    if (isnewpattern)
    {
        lrmerge = usr().new_pattern_record_code();
        m_last_record_style = usr().new_pattern_record_style();
        enable_combobox_item
        (
            ui->m_combo_rec_type, lrreset, lrmerge == lroneshot
        );
    }
    else
        enable_combobox_item(ui->m_combo_rec_type, lrreset, false);

    ui->m_combo_rec_type->setCurrentIndex(lrmerge);
    connect
    (
        ui->m_combo_rec_type, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_record_type(int))
    );

    /*
     * Recording Volume Button and Combo Box
     */

    connect
    (
        ui->m_button_rec_vol, SIGNAL(clicked(bool)),
        this, SLOT(reset_recording_volume())
    );
    (void) fill_combobox(ui->m_combo_rec_vol, rec_vol_list());
    connect
    (
        ui->m_combo_rec_vol, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_recording_volume(int))
    );

    /*
     * This little button is a workaround for Qt's lack of a signal
     * when the window manager's "X" button in the title bar of a non-main
     * window is clicked.  Doesn't work right, leaves a ghost window and
     * a seqfault at application exit.
     *
     * connect(ui->btn_close, SIGNAL(clicked()), this, SLOT(close()));
     */

    ui->btn_close->hide();

    set_recording_volume(usr().velocity_override());
    repopulate_usr_combos(m_edit_bus, m_edit_channel);
    set_midi_bus(m_edit_bus, qbase::status::startup);
    set_midi_channel(m_edit_channel, qbase::status::startup);  /* 0-15/0x80 */

    /*
     * For issue #94, both here and in the perf-edit frame, we may need to
     * allocate more width.  To be determined. See:
     *
     *      qseqroll::scroll_offset()
     *      qperfroll::sizeHint()
     */

    int seqwidth = m_seqroll->width();
    int scrollwidth = ui->rollScrollArea->width();
    m_seqroll->progress_follow(seqwidth > scrollwidth);
    ui->m_toggle_follow->setChecked(m_seqroll->progress_follow());
    update_midi_buttons();
    set_initialized();
    cb_perf().enregister(this);                             /* notification */
    m_timer = qt_timer(this, "qseqeditframe64", 2, SLOT(conditional_update()));
}

qseqeditframe64::~qseqeditframe64 ()
{
    m_timer->stop();
    cb_perf().unregister(this);
    delete ui;
}

void
qseqeditframe64::scroll_by_step (qscrollmaster::dir d)
{
    switch (d)
    {
    case qscrollmaster::dir::Left:
    case qscrollmaster::dir::Right:

        ui->rollScrollArea->scroll_x_by_step(d);
        break;

    case qscrollmaster::dir::Up:
    case qscrollmaster::dir::Down:

        ui->rollScrollArea->scroll_y_by_step(d);
        break;
    }
}

/**
 *  Odd, when this window has focus, this function is called roughly every 1/2
 *  second!  The type of event is always qpep->type() == 12.
 */

void
qseqeditframe64::paintEvent (QPaintEvent * qpep)
{
    qpep->ignore();                         /* QFrame::paintEvent(qpep)     */
}

/**
 *  Here, we will need to recreate the current viewport, if not the whole damn
 *  seqroll.
 */

void
qseqeditframe64::resizeEvent (QResizeEvent * qrep)
{
    update_draw_geometry();
    qrep->ignore();                         /* qseqframe::resizeEvent(qrep) */
}

/**
 *  This event is not called when moving the roll-scroll area horizontally or
 *  vertically.  However, paintEvent() calls do occur then for seqtime, seqdata,
 *  seqevent (qstriggereditor), and seqkey.
 */

void
qseqeditframe64::wheelEvent (QWheelEvent * qwep)
{
    qwep->ignore();                         /* qseqframe::resizeEvent(qwep) */
}

/**
 *  https://www.informit.com/articles/article.aspx?p=1405544&seqNum=2
 *
 *  The edit frame is the monitor of what events go into the
 *  various scroll areas.  A follow-on to issue #3.
 *
 *  This does not work, so see the new qscrollslave class, a replacement
 *  for QScrollArea that disables the arrow and paging keys.
 */

bool
qseqeditframe64::eventFilter (QObject * target, QEvent * event)
{
#if 0
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent * kev = static_cast<QKeyEvent *>(event);
        int key = kev->key();
        bool isarrow = key == Qt::Key_Down || key == Qt::Key_Up ||
            key == Qt::Key_Left || key == Qt::Key_Right;

        if (isarrow)
            return false;
    }
#endif
    return qseqframe::eventFilter(target, event);
}

/**
 *  Once the user has clicked on the qseqroll, the Space key can be used to
 *  start, stop, and restart playback.  The Period key can be used to pause
 *  and start (at the same position) playback.
 *
 *  We could simplify this a bit by creating a keystroke object.
 *  See qseqroll.
 */

void
qseqeditframe64::keyPressEvent (QKeyEvent * event)
{
    int key = event->key();
    bool isctrl = bool(event->modifiers() & Qt::ControlModifier);
    if (perf().is_pattern_playing())
    {
        if (key == Qt::Key_Space)
            stop_playing();
        else if (key == Qt::Key_Escape)
            stop_playing();
        else if (key == Qt::Key_Period)
            pause_playing();
    }
    else
    {
        if (key == Qt::Key_Space || key == Qt::Key_Period)
        {
            start_playing();
        }
        else if (isctrl)
        {
            bool isshift = bool(event->modifiers() & Qt::ShiftModifier);
            if (key == Qt::Key_Z)
            {
                if (isshift)
                    redo();                     // track().pop_redo();
                else
                    undo();                     // #110 track().pop_undo()
            }
        }
    }
    if (! isctrl)
    {
        bool isshift = bool(event->modifiers() & Qt::ShiftModifier);
        if (isshift)
        {
            if (key == Qt::Key_L)
            {
                m_seqtime->setFocus();
                m_seqtime->m_move_L_marker = true;
            }
            else if (key == Qt::Key_R)
            {
                m_seqtime->setFocus();
                m_seqtime->m_move_L_marker = false;
            }
            else
                event->accept();
        }
        else
        {
            /*
             * vi-style scrolling keystrokes
             */

            if (key == Qt::Key_J)
                scroll_by_step(qscrollmaster::dir::Down);
            else if (key == Qt::Key_K)
                scroll_by_step(qscrollmaster::dir::Up);
            else if (key == Qt::Key_H)
                scroll_by_step(qscrollmaster::dir::Left);
            else if (key == Qt::Key_L)
                scroll_by_step(qscrollmaster::dir::Right);
            else
                event->accept();
        }
    }
    else
        event->accept();
}

void
qseqeditframe64::keyReleaseEvent (QKeyEvent *)
{
    // no code
}

/**
 *  Passes along the signal to close the windows.
 */

void
qseqeditframe64::closeEvent (QCloseEvent * event)
{
    remove_lfo_frame();
    remove_patternfix_frame();
    event->accept();
}

/**
 *  This function now seems to be called. Yay!
 */

bool
qseqeditframe64::on_sequence_change
(
    seq::number seqno, performer::change /* ctype */
)
{
    seq::number trackno = track().seq_number();
    bool result = seqno == trackno;
    if (result)
    {
        set_dirty();                            /* modified for issue #90   */
        update_midi_buttons();                  /* mirror current states    */
    }
    return result;
}

/**
 *  Added this function to handle simple changes in sequence status,
 *  including recording changes.  Can be conflated with on_sequence_change().
 */

bool
qseqeditframe64::on_trigger_change (seq::number /* seqno */)
{
    set_dirty();                            /* modified for issue #90   */
    update_midi_buttons();                  /* mirror current states    */
    return true;
}

/**
 *  More redraws? Data and event panes?
 */

bool
qseqeditframe64::on_automation_change (automation::slot s)
{
    if (s == automation::slot::start || s == automation::slot::stop)
        m_seqroll->set_redraw();

    return true;
}

/**
 *  Instantiates the various editable areas (panels) of the seqedit
 *  user-interface.  seqkeys: Not quite working as we'd hope.  The scrollbars
 *  still eat up space.  They needed to be hidden.  Note that m_seqkeys and
 *  the other panel pointers are protected members of the qseqframe base
 *  class.  We could move qseqframe's members into this class, now that we no
 *  longer provide the old qseqeditframe.
 */

void
qseqeditframe64::initialize_panels ()
{
    int noteheight = usr().key_height();
    int height = noteheight * c_notes_count + 1;
    m_seqkeys = new (std::nothrow) qseqkeys
    (
        perf(), track(), this, ui->keysScrollArea, noteheight, height
    );
    ui->keysScrollArea->setWidget(m_seqkeys);
    ui->keysScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->keysScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->keysScrollArea->verticalScrollBar()->setRange(0, height);
    m_seqtime = new (std::nothrow) qseqtime
    (
        perf(), track(), this, zoom(), ui->timeScrollArea
    );
    ui->timeScrollArea->setWidget(m_seqtime);
    ui->timeScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->timeScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    /*
     * qseqroll.  Note the "this" parameter is not really a Qt parent
     * parameter.  It simply gives qseqroll access to the qseqeditframe64 ::
     * follow_progress() function.
     */

    m_seqroll = new (std::nothrow) qseqroll
    (
        perf(), track(), this, m_seqkeys, zoom(),
        m_snap, sequence::editmode::note, noteheight, height
    );
    ui->rollScrollArea->setWidget(m_seqroll);
    ui->rollScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->rollScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_seqroll->update_edit_mode(m_edit_mode);
    m_seqdata = new (std::nothrow) qseqdata
    (
        perf(), track(), this, zoom(), m_snap, ui->dataScrollArea,
        short_version() ? 64 : 0                /* 0 means "normal height"  */
    );

    /*
     *  dataScrollArea is now a qscrollslave object.
     */

    ui->dataScrollArea->setWidget(m_seqdata);
    ui->dataScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->dataScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_seqevent = new (std::nothrow) qstriggereditor
    (
        perf(), track(), this, zoom(), m_snap,
        noteheight, ui->eventScrollArea, 0
    );
    ui->eventScrollArea->setWidget(m_seqevent);
    ui->eventScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->eventScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    /*
     *  Add the various scrollbar points to the qscrollmaster object,
     *  ui->rollScrollArea.
     */

    ui->rollScrollArea->add_v_scroll(ui->keysScrollArea->verticalScrollBar());
    ui->rollScrollArea->add_h_scroll(ui->timeScrollArea->horizontalScrollBar());
    ui->rollScrollArea->add_h_scroll(ui->dataScrollArea->horizontalScrollBar());
    ui->rollScrollArea->add_h_scroll(ui->eventScrollArea->horizontalScrollBar());

    int minimum = ui->rollScrollArea->verticalScrollBar()->minimum();
    int maximum = ui->rollScrollArea->verticalScrollBar()->maximum();
    ui->rollScrollArea->verticalScrollBar()->setValue((minimum + maximum) / 2);

    /*
     * Attach the qscrollmaster to each qscrollslave so that they can
     * forward the arrow and page keys to the qscrollmaster.  This avoids
     * having to click on the seqroll to get focus to enable the direction
     * keys. However, this doesn't seem to work with wheel events. See
     * the qscrollslave module.
     */

    ui->dataScrollArea->attach_master(ui->rollScrollArea);
    ui->eventScrollArea->attach_master(ui->rollScrollArea);
    ui->keysScrollArea->attach_master(ui->rollScrollArea);
    ui->timeScrollArea->attach_master(ui->rollScrollArea);
}

/**
 *  We need to set the dirty state while the sequence has been changed.
 *
 */

void
qseqeditframe64::conditional_update ()
{
    bool expandrec = track().expand_recording();
    if (expandrec)
    {
        /*
         *  Question: how can the expansion work here without constantly
         *  increasing the length before the end of the measures is reached???
         *  We have moved this operation out of the user-interface and into
         * sequence::stream_event()
         *
         * set_measures(track().get_measures() + 1);
         */

        follow_progress(expandrec);             /* keep up with progress    */
    }
    else if (not_nullptr(m_seqroll) && m_seqroll->progress_follow())
    {
        follow_progress();
    }
    if (m_measures != track().measures())
    {
        m_measures = track().get_measures();

        std::string mstring = std::to_string(m_measures);
        int lenindex = measures_list().index(m_measures);
        if (lenindex >= 0)
        {
            int curindex = ui->m_combo_length->currentIndex();
            if (lenindex != curindex)
                ui->m_combo_length->setCurrentIndex(lenindex);
        }
        ui->m_combo_length->setEditText(qt(mstring));
    }
    if (m_beats_per_bar != track().get_beats_per_bar())
    {
        m_beats_per_bar = track().get_beats_per_bar();

        std::string mstring = std::to_string(m_beats_per_bar);
        int lenindex = beats_per_bar_list().index(m_beats_per_bar);
        if (lenindex >= 0)
        {
            int curindex = ui->m_combo_bpm->currentIndex();
            if (lenindex != curindex)
                ui->m_combo_bpm->setCurrentIndex(lenindex);
        }
        ui->m_combo_bpm->setEditText(qt(mstring));
    }
    if (m_beat_width != track().get_beat_width())
    {
        m_beat_width = track().get_beat_width();

        std::string mstring = std::to_string(m_beat_width);
        int lenindex = beatwidth_list().index(m_beat_width);
        if (lenindex >= 0)
        {
            int curindex = ui->m_combo_bw->currentIndex();
            if (lenindex != curindex)
                ui->m_combo_bw->setCurrentIndex(lenindex);
        }
        ui->m_combo_bw->setEditText(qt(mstring));
    }
    if (m_is_looping != perf().looping())
    {
        m_is_looping = perf().looping();
        ui->m_button_loop->setChecked(m_is_looping);
    }
    if (track().check_loop_reset())
    {
        /*
         * Now we need to update the event and data panes.  The notes update
         * during the next pass through the loop only if more notes come in on
         * the input buss.
         */

        set_dirty();                            /* modified for issue #90   */
        update_midi_buttons();                  /* mirror current states    */
    }
}

#if defined USE_WOULD_TRUNCATE_BPB_BW

/**
 *  Checks if the new time-signature results in a dropping events.
 *  However, this check is now not needed under recent fixes which increase
 *  the number of measures if necessary, so we just return false, indicating
 *  no truncation.
 *
 *  Interesting: divide by 0 here yields the value "inf", not an exception.
 */

bool
qseqeditframe64::would_truncate (int bpb, int bw)
{
    bool result = false;                        /* no problem by default    */
    if (bpb > 0 && bw > 0)                      /* do only after start-up   */
    {
        double fraction = double(bpb) / double(bw);
        midipulse max = track().get_max_timestamp();
        midipulse newlength = midipulse(track().get_length() * fraction);
        result = newlength < max;
        if (result)
        {
            result = ! qt_prompt_ok             /* Cancel == ack the danger */
            (
                "This time-signature might drop events.",
                "Cancel to avoid that."
            );
        }
    }
    return result;
}

#endif  // defined USE_WOULD_TRUNCATE_BPB_BW

/**
 *  Checks if a direct change in measures would truncate events.
 */

bool
qseqeditframe64::would_truncate (int m)
{
    bool result = false;                        /* no problem by default    */
    if (m > 0 && m <= 99999)                    /* a sanity check only      */
    {
        midipulse max = track().get_max_timestamp();
        midipulse newlength = midipulse(track().measures_to_ticks(m));
        result = newlength < max;
        if (result)
        {
            result = ! qt_prompt_ok             /* Cancel == ack the danger */
            (
                "This change will drop events.", "Cancel to avoid that."
            );
        }
    }
    return result;
}

/**
 *  Handles edits of the sequence title.
 *
 *  We need to revisit sequence::set_name() in light of issue #90.
 */

void
qseqeditframe64::update_seq_name ()
{
    std::string name = ui->m_entry_name->text().toStdString();
    if (perf().set_sequence_name(track(), name))
        set_track_change();                         /* to solve issue #90   */
}

/**
 *  Reacts to text changes.
 */

void
qseqeditframe64::set_log_timesig_text (int bpb, int bw)
{
    std::string text = int_to_string(bpb);
    std::string bwstr = int_to_string(bw);
    text += "/";
    text += bwstr;
    ui->m_button_log_timesig->setText(qt(text));
}

void
qseqeditframe64::set_log_timesig_status (bool flag)
{
    midipulse tick = perf().get_tick();         /* perf().get_left_tick()   */
    if (tick > 0)
        ui->m_button_log_timesig->setEnabled(flag);
    else
        ui->m_button_log_timesig->setEnabled(false);
}

/**
 *  Helper function
 */

bool
qseqeditframe64::log_timesig (bool islogbutton)
{
    midipulse tick = perf().get_tick();         /* perf().get_left_tick()   */
    midipulse tstamp;
    int n, d;
    bool found = track().detect_time_signature(tstamp, n, d, tick);
    if (found)
    {
        found = labs(tick - tstamp) < (track().snap() / 2);
        if (found)
            (void) track().delete_time_signature(tstamp);
    }

    int bpb = islogbutton ? m_beats_per_bar_to_log : m_beats_per_bar ;
    int bw = islogbutton ? m_beat_width_to_log : m_beat_width ;
    bool result = track().add_time_signature(tick, bpb, bw);
    if (result)
    {
        (void) track().analyze_time_signatures();
        set_data_type(EVENT_META_TIME_SIGNATURE);
        set_log_timesig_text(bpb, bw);
        set_log_timesig_status(false);
    }
    return result;
}

/**
 *  Given the current positions as set by clicking in the top half of the
 *  seqtime bar [performer::get_tick() versus performer::get_left_tick()],
 *  this function removes any existing time-signature at that point, and adds
 *  a new one at that point.
 *
 *  Note that there's some slop, 1/2 the event-snap value. Also note that this
 *  function will not change to stored values of m_beats_per_bar and
 *  m_beat_width.
 */

void
qseqeditframe64::slot_log_timesig ()
{
    if (log_timesig(true))
    {
        /*
         * How to get the modify flag set???
         *
         * m_beats_per_bar = m_beats_per_bar_to_log;
         * m_beat_width = m_beat_width_to_log;
         */

        set_track_change();
    }
}

/**
 *  Handles updates to the beats/measure for only the current sequences.
 *  See the similar function in qsmainwnd.  Not needed, it seems.
 *
 *  void
 *  qseqeditframe64::update_beats_per_bar (int index)
 *  {
 *      int bpb = beats_per_bar_list().ctoi(index);
 *      set_beats_per_bar(bpb);
 *  }
 */

void
qseqeditframe64::text_beats_per_bar (const QString & text)
{
    std::string temp = text.toStdString();
    if (! temp.empty())
    {
        int bpb = string_to_int(temp);
        set_beats_per_bar(bpb);
        set_log_timesig_status(true);
    }
}

void
qseqeditframe64::reset_beats_per_bar ()
{
    int index = beats_per_bar_list().index(usr().bpb_default());
    ui->m_combo_bpm->setCurrentIndex(index);
    set_log_timesig_status(true);
}

/**
 *  Applies the new beats/bar (beats/measure) value to the sequence and the
 *  user interface.
 *
 * \param bpb
 *      The desired beats/measure value.
 *
 * \param qs
 *      Set to start or edit.  Basically, the same as user_change ==
 *      false or true.
 */

void
qseqeditframe64::set_beats_per_bar (int bpb, qbase::status qs)
{
    if (usr().bpb_is_valid(bpb))
    {
        bool loggable = perf().get_tick() > track().snap() / 2;
        bool doable = loggable ?
            (bpb != m_beats_per_bar_to_log) : (bpb != m_beats_per_bar) ;

        if (doable)
        {
            bool user_change = qs == qbase::status::edit;

#if defined USE_WOULD_TRUNCATE_BPB_BW
            bool reset = false;
            if (user_change)
                reset = would_truncate(bpb, m_beat_width);

            if (reset)
            {
                /* reset_beats_per_bar();  simply ignore */
            }
            else
            {
#endif
                if (loggable)
                {
                    m_beats_per_bar_to_log = bpb;
                    set_log_timesig_text
                    (
                        m_beats_per_bar_to_log, m_beat_width_to_log
                    );
                }
                else                                    /* get_left_tick()  */
                {
                    m_beats_per_bar = m_beats_per_bar_to_log = bpb;
                    track().set_beats_per_bar(bpb, user_change);
                    (void) track().apply_length(bpb, 0, 0); /* no measures  */
                    if (log_timesig(false))
                        set_track_change();
                }
#if defined USE_WOULD_TRUNCATE_BPB_BW
            }
#endif
        }
    }
}

/**
 *  Set the measures value, using the given parameter, and some internal
 *  values passed to apply_length().
 *
 * \param m
 *      Provides the sequence length, in measures.
 */

void
qseqeditframe64::set_measures (int m, qbase::status qs)
{
    if (m > 0 && m <= 99999)                        /* a sanity check only  */
    {
        bool reset = false;
        if (qs == qbase::status::edit)
        {
            if (! perf().is_pattern_playing())      /* ca 2022-08-20        */
                reset = would_truncate(m);
        }
        if (reset)
        {
            /* reset_measures(); simply ignore */
        }
        else
        {
            bool ok = m > 0;

#if defined DO_NOT_DISABLE_FOR_ISSUE_107

            /*
             * The check for the pattern playing causes issue #107, and
             * we're not sure why this check was put here in the first place.
             */

            if (ok)
                ok = ! perf().is_pattern_playing()  /* ca 2022-08-20        */
#endif
            if (ok)
            {
                if (track().apply_length(m))
                {
                    m_measures = m;
                    set_track_change();             /* to solve issue #90   */
                }
            }
        }
    }
}

/**
 *  Resets the pattern-length in its combo-box.
 */

void
qseqeditframe64::reset_measures ()
{
    int index = beatwidth_list().index(m_measures);
    ui->m_combo_length->setCurrentIndex(index);
    set_track_change();                             /* to solve issue #90   */
}

/**
 *  Handles updates to the beat width for only the current sequences.
 *  See the similar function in qsmainwnd.
 *
 *  void
 *  qseqeditframe64::update_beat_width (int index)
 *  {
 *      int bw = beatwidth_list().ctoi(index);
 *      set_beat_width(bw);
 *  }
 */

void
qseqeditframe64::text_beat_width (const QString & text)
{
    std::string temp = text.toStdString();
    if (! temp.empty())
    {
        int bw = string_to_int(temp);
        set_beat_width(bw);
        set_log_timesig_status(true);
    }
}

/**
 *  Resets the beat-width combo-box to its default value.
 */

void
qseqeditframe64::reset_beat_width ()
{
    int index = beatwidth_list().index(usr().bw_default());
    ui->m_combo_bw->setCurrentIndex(index);
    set_log_timesig_text(m_beats_per_bar, m_beat_width);
    set_log_timesig_status(true);
}

/**
 *  Sets the beat-width value and then dirties the user-interface so that it
 *  will be repainted. usr().bw_is_valid() is only a gross range check.
 */

void
qseqeditframe64::set_beat_width (int bw, qbase::status qs)
{
    if (usr().bw_is_valid(bw))
    {
        bool loggable = perf().get_tick() > track().snap() / 2;
        bool doable = loggable ?
            (bw != m_beat_width_to_log) : (bw != m_beat_width) ;

        if (doable)
        {
            bool user_change = qs == qbase::status::edit;

#if defined USE_WOULD_TRUNCATE_BPB_BW
            bool reset = false;
            if (user_change)
                reset = would_truncate(m_beats_per_bar, bw);

            if (reset)
            {
                /* reset_beat_width(); simply ignore */
            }
            else
            {
#endif
                /*
                 * If not a power of 2, then just set for a c_timesig SeqSpec.
                 */

                bool rational = is_power_of_2(bw);
                if (rational)                       /* use OK'ed it         */
                {
                    if (loggable)
                    {
                        m_beat_width_to_log = bw;
                        set_log_timesig_text
                        (
                            m_beats_per_bar_to_log, m_beat_width_to_log
                        );
                    }
                    else                             /* get_left_tick()      */
                    {
                        m_beat_width = m_beat_width_to_log = bw;
                        track().set_beat_width(bw, user_change);
                        (void) track().apply_length(0, 0, bw);
                        if (log_timesig(false))
                            set_track_change();
                    }
                }
                else
                {
                    bool allow_odd_beat_width = qt_prompt_ok
                    (
                        "MIDI supports only powers of 2 for beat-width.",
//                      "It won't be saved properly, but you do you."
                        "It will be saved as a Seq66-specific MIDI event, "
                        "not a time-signature event. Only one seq-spec allowed. "
                        "Overriden by existing time-signature events."
                    );
                    if (allow_odd_beat_width)
                    {
                        track().set_beat_width(bw, user_change);
                        (void) track().apply_length(0, 0, bw);
                    }
                }
#if defined USE_WOULD_TRUNCATE_BPB_BW
            }
#endif
        }
    }
}

/**
 *  This function looks for a time-signature, and if it is within the first
 *  half-snap interval of the start of of the pattern, sets the beats value
 *  accordingly.
 *
 * Time signature life-cycle:
 *
 *      -#  Set seqedit time-signature to the global default (usually 4/4).
 *      -#  If an initial time-signature is detected, set it here.
 *      -#  If the numerator or denominator is changed in the GUI, and
 *          there is a time-signature at the beginning (tick == 0), then
 *          replace it.
 *      -#  Otherwise, require the "L" marker to be set and then log the
 *          time-signature at that point. See slot_log_timesig().
 */

bool
qseqeditframe64::detect_time_signature ()
{
    midipulse tstamp;
    int n, d;
    bool result = track().detect_time_signature
    (
        tstamp, n, d, 0, track().snap() / 2
    );
    (void) track().analyze_time_signatures();           /* log some or none */
    if (result)
    {
        set_beats_per_bar(n, qbase::status::startup);
        set_beat_width(d, qbase::status::startup);
    }
    return result;
}

/**
 *  Handles updates to the pattern length.
 *
 *  void
 *  qseqeditframe64::update_measures (int index)
 *  {
 *      int m = measures_list().ctoi(index);
 *      set_measures(m);
 *  }
 */

void
qseqeditframe64::text_measures (const QString & text)
{
    std::string temp = text.toStdString();
    if (! temp.empty())
    {
        int measures = string_to_int(temp);
        set_measures(measures);
    }
}

/**
 *  When the measures-length button is pushed, we go to the next length
 *  entry in the combo-box, wrapping around when the end is reached.
 */

void
qseqeditframe64::next_measures ()
{
    int index = measures_list().index(m_measures);
    if (++index >= measures_list().count())
        index = 0;

    ui->m_combo_length->setCurrentIndex(index);
    int m = measures_list().ctoi(index);
    set_measures(m);
}

/**
 *  Passes the transpose status to the sequence object.
 */

void
qseqeditframe64::transpose (bool ischecked)
{
    track().set_transposable(ischecked, true);      /* it's a user change   */
    set_transpose_image(ischecked);
    ui->m_map_notes->setEnabled(ischecked);
}

/**
 *  Changes the image used for the transpose button.  Actually, we have two
 *  drum icons next to each other now, so we won't change this one to a drum.
 *
 *      qt_set_icon(transpose_xpm, ui->m_toggle_transpose);
 *      qt_set_icon(drum_xpm, ui->m_toggle_transpose);
 *
 * \param istransposable
 *      Indicates what state we're in.
 */

void
qseqeditframe64::set_transpose_image (bool istransposable)
{
    QString text = istransposable ?
        "Pattern is transposable" : "Pattern not transposable" ;

    ui->m_toggle_transpose->setToolTip(text);
}

/**
 *  Handles updates to the beats/measure for only the current sequences.
 *  See the similar function in qsmainwnd.
 */

void
qseqeditframe64::update_chord (int index)
{
    if (index != m_chord && chord_number_valid(index))
    {
        set_chord(index);
        set_dirty();                    /* modified for issue #90           */
    }
}

/**
 *  Resets the chord-setting to the initial value in the chord combo-box.
 */

void
qseqeditframe64::reset_chord ()
{
    ui->m_combo_chord->setCurrentIndex(0);
    if (not_nullptr(m_seqroll))
        m_seqroll->set_chord(0);

    set_dirty();                        /* modified for issue #90           */
}

void
qseqeditframe64::set_chord (int chord)
{
    if (chord_number_valid(chord))
    {
        ui->m_combo_chord->setCurrentIndex(chord);
        m_chord = sm_initial_chord = chord;
        if (not_nullptr(m_seqroll))
            m_seqroll->set_chord(chord);

        set_dirty();                    /* modified for issue #90           */
    }
}

/**
 *  Sets the MIDI bus to use for output.
 *
 * \param index
 *      The index into the list of available MIDI buses.
 */

void
qseqeditframe64::update_midi_bus (int index)
{
    set_midi_bus(index);
}

/**
 *  Resets the MIDI bus value to its default.
 */

void
qseqeditframe64::reset_midi_bus ()
{
    set_midi_bus(0, qbase::status::startup);    /* indirect user-change */
}

/**
 *  Selects the given MIDI buss parameter in the main sequence object,
 *  so that it will use that buss.  Currently called only in the constructor.
 *  Otherwise, see the update_midi_bus() function.
 *
 *  Should this change set the is-modified flag?  Where should validation
 *  against the ALSA or JACK buss limits occur?
 *
 *  Also, it would be nice to be able to update this display of the MIDI bus
 *  in the field if we set it from the seqmenu.
 *
 * \param bus
 *      The nominal buss value to set.  If this value changes the selected
 *      buss, then the MIDI channel popup menu is repopulated.
 *
 * \param user_change
 *      True if the user made this change, and thus has potentially modified
 *      the song.  If true, and the bus number has changed, then the MIDI
 *      channel and event menus are repopulated to reflect the new bus.  This
 *      parameter is false in the constructor because those items have not
 *      been set up at that time.
 */

void
qseqeditframe64::set_midi_bus (int bus, qbase::status qs)
{
    bussbyte initialbus = track().seq_midi_bus();
    bussbyte b = bussbyte(bus);
    if (b != initialbus)
    {
        bool user_change = qs == qbase::status::edit;
        track().set_midi_bus(b, user_change);
        m_edit_bus = bus;
        if (user_change)
        {
            repopulate_usr_combos(m_edit_bus, m_edit_channel);
            set_track_change();                     /* to solve issue #90   */
        }
        else
            ui->m_combo_bus->setCurrentIndex(bus);
    }
}

/**
 *  Populates the MIDI Channel combo-box with the number and names of each
 *  channel.  This action is needed at startup of the seqedit window, and when
 *  the user changes the active buss for the sequence.
 *
 *  When the output buss or channel are changed, we get the 16 "channels" from
 *  the new buss's definition, get the corresponding instrument, and load its
 *  name into this midich popup.  Then we need to go to the instrument/channel
 *  that has been selected, and repopulate the event menu with that item's
 *  controller values/names.
 *
 * \param buss
 *      The new value for the buss from which to get the [user-instrument-N]
 *      settings in the [user-instrument-definitions] section.
 */

void
qseqeditframe64::repopulate_midich_combo (int buss)
{
    ui->m_combo_channel->clear();
    for (int channel = 0; channel <= c_midichannel_max; ++channel)
    {
        char b[4];                                      /* 2 digits or less */
        snprintf(b, sizeof b, "%2d", channel + 1);
        std::string name = std::string(b);
        std::string s = usr().instrument_name(buss, channel);
        if (! s.empty())
        {
            name += " [";
            name += s;
            name += "]";
        }
        if (channel == c_midichannel_max)               /* i.e. 16          */
        {
            QString combo_text("Free");
            ui->m_combo_channel->insertItem(channel, combo_text);
        }
        else
        {
            QString combo_text(qt(name));
            ui->m_combo_channel->insertItem(channel, combo_text);
        }
    }

    int ch = track().midi_channel();
    if (is_null_channel(ch))
        ch = c_midichannel_max;

    ui->m_combo_channel->setCurrentIndex(ch);
}

/**
 *  Note that c_midichannel_max (16) is a legal value.  It is remapped in
 *  sequence::set_midi_channel() to null_channel().
 */

void
qseqeditframe64::update_midi_channel (int index)
{
    set_midi_channel(index);
}

void
qseqeditframe64::reset_midi_channel ()
{
    set_midi_channel(0);
}

/**
 *  Selects the given MIDI channel parameter in the main sequence object, so
 *  that it will use that channel.  If "Free" is selected, all that happens is
 *  that the pattern is set to "no-channel" mode for MIDI output.
 *
 * \param ch
 *      The MIDI channel value to set.
 *
 * \param user_change
 *      True if the user made this change, and thus has potentially modified
 *      the song.  The default is false.
 */

void
qseqeditframe64::set_midi_channel (int ch, qbase::status qs)
{
    int initialchan = track().seq_midi_channel();
    bool user_change = qs == qbase::status::edit;
    if (ch != initialchan || ! user_change)
    {
        int chindex = ch;
        midibyte channel = midibyte(ch);
        if (! is_good_channel(channel))
        {
            chindex = c_midichannel_max;                    /* "Free" */
            channel = null_channel();
        }
        if (track().set_midi_channel(channel, user_change))
        {
            m_edit_channel = channel;
            if (is_null_channel(channel))
            {
                ui->m_combo_channel->setCurrentIndex(chindex);
            }
            else
            {
                repopulate_usr_combos(m_edit_bus, m_edit_channel);
                if (user_change)
                {
                    repopulate_event_menu(m_edit_bus, m_edit_channel);
                    repopulate_mini_event_menu(m_edit_bus, m_edit_channel);
                    set_track_change();             /* to solve issue #90   */
                }
                else
                    ui->m_combo_channel->setCurrentIndex(chindex);
            }
        }
    }
}

/**
 *  Does a pop-undo on the sequence object and then sets the dirty flag.
 */

void
qseqeditframe64::undo ()
{
    track().pop_undo();
    set_dirty();                        /* for issue #110                   */
}

/**
 *  Does a pop-redo on the sequence object and then sets the dirty flag.
 *  We can't reliably call this a track change, however.
 */

void
qseqeditframe64::redo ()
{
    track().pop_redo();
    set_dirty();
}

/**
 *  Popup menu over button.
 */

void
qseqeditframe64::tools ()
{
    if (not_nullptr(m_tools_popup))
    {
        enable_note_menus();
        m_tools_popup->exec
        (
            ui->m_button_tools->mapToGlobal
            (
                QPoint(ui->m_button_tools->width() - 2,
                ui->m_button_tools->height() - 2)
            )
        );
    }
}

/**
 *  Builds the Tools popup menu on the fly.
 */

void
qseqeditframe64::popup_tool_menu ()
{
    m_tools_popup = new QMenu(this);

    QMenu * menuselect = new QMenu(tr("&Select notes..."), m_tools_popup);
    QMenu * menutiming = new QMenu(tr("Note &timing..."), m_tools_popup);
    QMenu * menupitch  = new QMenu(tr("&Pitch transpose..."), m_tools_popup);
    QMenu * menuharmonic = new QMenu
    (
        tr("&Harmonic transpose..."), m_tools_popup
    );

#if defined USE_MORE_TOOLS
    /*
     * Can enable this if we get much more than the 2 current extra tools.
     */

    QMenu * menumore = new QMenu(tr("&More tools..."), m_tools_popup);
#endif

    QAction * selectall = new QAction(tr("Select &all"), m_tools_popup);
    connect
    (
        selectall, SIGNAL(triggered(bool)),
        this, SLOT(select_all_notes())
    );
    menuselect->addAction(selectall);

    QAction * selectinverse = new QAction(tr("&Invert selection"), m_tools_popup);
    connect
    (
        selectinverse, SIGNAL(triggered(bool)),
        this, SLOT(inverse_note_selection())
    );
    menuselect->addAction(selectinverse);

    QAction * quantize = new QAction(tr("&Quantize"), m_tools_popup);
    connect(quantize, SIGNAL(triggered(bool)), this, SLOT(quantize_notes()));
    menutiming->addAction(quantize);

    QAction * tighten = new QAction(tr("&Tighten"), m_tools_popup);
    connect(tighten, SIGNAL(triggered(bool)), this, SLOT(tighten_notes()));
    menutiming->addAction(tighten);

    QAction * jitter = new QAction(tr("&Jitter"), m_tools_popup);
    connect(jitter, SIGNAL(triggered(bool)), this, SLOT(jitter_notes()));
    menutiming->addAction(jitter);

    QAction * lfobox = new QAction(tr("&LFO..."), m_tools_popup);
    connect(lfobox, SIGNAL(triggered(bool)), this, SLOT(show_lfo_frame()));

    QAction * fixbox = new QAction(tr("Pattern &fix..."), m_tools_popup);
    connect(fixbox, SIGNAL(triggered(bool)), this, SLOT(show_pattern_fix()));

    QAction * transpose[2 * c_octave_size];     /* plain pitch transposings */
    QAction * harmonic[2 * c_harmonic_size];    /* harmonic transpositions  */
    for (int t = -c_octave_size; t <= c_octave_size; ++t)
    {
        if (t != 0)
        {
            /*
             * Pitch transpose menu entries. Note the interval_name_ptr() and
             * harmonic_interval_name_ptr() functions from the scales module.
             */

            char num[16];
            int index = t + c_octave_size;
            snprintf
            (
                num, sizeof num, "%+d [%s]",
                t, interval_name_ptr(t)
            );
            transpose[index] = new QAction(num, m_tools_popup);
            transpose[index]->setData(t);
            menupitch->addAction(transpose[index]);
            connect
            (
                transpose[index], SIGNAL(triggered(bool)),
                this, SLOT(transpose_notes())
            );
            if (harmonic_number_valid(t))
            {
                /*
                 * Harmonic transpose menu entries.
                 */

                int tn = t < 0 ? (t - 1) : (t + 1);
                index = t + c_harmonic_size;
                snprintf
                (
                    num, sizeof num, "%+d [%s]",
                    tn, harmonic_interval_name_ptr(t)
                );
                harmonic[index] = new QAction(num, m_tools_popup);
                harmonic[index]->setData(t);
                menuharmonic->addAction(harmonic[index]);
                connect
                (
                    harmonic[index], SIGNAL(triggered(bool)),
                    this, SLOT(transpose_harmonic())
                );
            }
        }
        else
        {
            menupitch->addSeparator();
            menuharmonic->addSeparator();
        }
    }
    m_tools_popup->addMenu(menuselect);
    m_tools_popup->addMenu(menutiming);
    m_tools_popup->addMenu(menupitch);
    m_tools_popup->addMenu(menuharmonic);

#if defined USE_MORE_TOOLS
    m_tools_popup->addMenu(menumore);
#else
    m_tools_popup->addAction(lfobox);
    m_tools_popup->addAction(fixbox);
#endif

    m_tools_harmonic = menuharmonic;
    m_tools_timing = menutiming;
    m_tools_pitch = menupitch;
    enable_note_menus();
}

void
qseqeditframe64::enable_note_menus ()
{
    bool gotselection = track().any_selected_notes();
    if (not_nullptr(m_tools_harmonic))
        m_tools_harmonic->setEnabled(gotselection && m_scale > 0);

    if (not_nullptr(m_tools_pitch))
        m_tools_pitch->setEnabled(gotselection);

    if (not_nullptr(m_tools_timing))
        m_tools_timing->setEnabled(gotselection);
}

/**
 *  Aftertouch events are associated with notes.
 */

void
qseqeditframe64::select_all_notes ()
{
    track().select_events(EVENT_NOTE_ON, 0);
    track().select_events(EVENT_NOTE_OFF, 0);
    track().select_events(EVENT_AFTERTOUCH, 0);
    enable_note_menus();
}

/**
 *  Aftertouch events are associated with notes.
 */

void
qseqeditframe64::inverse_note_selection ()
{
    track().select_events(EVENT_NOTE_ON, 0, true);
    track().select_events(EVENT_NOTE_OFF, 0, true);
    track().select_events(EVENT_AFTERTOUCH, 0, true);
    enable_note_menus();
}

/**
 *  This function acts on Notes On, Notes Off, and Aftertouch events.
 */

void
qseqeditframe64::quantize_notes ()
{
    track().push_quantize_notes(1);
}

/**
 *  This function acts on Notes On, Notes Off, and Aftertouch events.
 */

void
qseqeditframe64::tighten_notes ()
{
    track().push_quantize_notes(2);
}

/**
 *  Includes Aftertouch events.
 */

void
qseqeditframe64::jitter_notes ()
{
    int s = usr().jitter_range(m_seqroll->snap());
    track().push_jitter_notes(s);
}

/**
 *  Consider adding Aftertouch events.  Done in sequence::transpose_notes().
 *  Also note that this function does a push-undo operation first.
 */

void
qseqeditframe64::transpose_notes ()
{
    QAction * senderAction = (QAction *) sender();
    int transposeval = senderAction->data().toInt();
    track().transpose_notes(transposeval, 0);
}

void
qseqeditframe64::transpose_harmonic ()
{
    QAction * senderAction = (QAction *) sender();
    int transposeval = senderAction->data().toInt();
    track().transpose_notes(transposeval, m_scale, m_key);
}

/**
 *  Here, we need to get the filespec, create a notemapper, fill it from the
 *  notemapfile, and iterate through the notes, converting them.
 */

void
qseqeditframe64::remap_notes ()
{
    if (repitch_all())
        ui->m_map_notes->setEnabled(false);
}

/**
 *  Popup menu sequences button.
 */

void
qseqeditframe64::sequences ()
{
    if (not_nullptr(m_sequences_popup))
    {
        int w = ui->m_button_sequence->width() - 2;
        int h = ui->m_button_sequence->height() - 2;
        m_sequences_popup->exec
        (
            ui->m_button_sequence->mapToGlobal(QPoint(w, h))
        );
    }
}

/**
 *  A case where a macro makes the code easier to read. The first parameter is
 *  the sequence number, and the second indicates that the user made the
 *  setting.
 */

#define SET_BG_SEQ(seq, change) \
    std::bind(&qseqeditframe64::set_background_sequence, this, seq, change)

/**
 *  Builds the menu for the Background Sequences button on the fly.  Analogous
 *  to seqedit :: popup_sequence_menu().
 */

void
qseqeditframe64::popup_sequence_menu ()
{
    if (is_nullptr(m_sequences_popup))
    {
        m_sequences_popup = new QMenu(this);
    }

    QAction * off = new QAction(tr("Off"), m_sequences_popup);
    connect(off, &QAction::triggered, SET_BG_SEQ(seq::limit(), qbase::status::edit));
    (void) m_sequences_popup->addAction(off);
    (void) m_sequences_popup->addSeparator();
    int seqsinset = perf().screenset_size();
    int maxset = perf().screenset_max();
    for (int sset = 0; sset < maxset; ++sset)
    {
        QMenu * menusset = nullptr;
        if (perf().is_screenset_active(sset))
        {
            char number[16];
            snprintf(number, sizeof number, "Set %d", sset);
            menusset = m_sequences_popup->addMenu(number);
            for (int seq = 0; seq < seqsinset; ++seq)
            {
                char name[32];
                int s = sset * seqsinset + seq;
                seq::pointer sp = perf().get_sequence(s);
                if (not_nullptr(sp))
                {
                    const char * nameptr = sp->name().c_str();
                    snprintf(name, sizeof name, "[%d] %.13s", s, nameptr);

                    QAction * item = new QAction(tr(name), menusset);
                    menusset->addAction(item);
                    connect
                    (
                        item, &QAction::triggered,
                        SET_BG_SEQ(s, qbase::status::edit)
                    );
                }
            }
        }
    }
}

/**
 *  Passes the transpose status to the sequence object.
 */

void
qseqeditframe64::note_entry (bool ischecked)
{
    if (not_nullptr(m_seqroll))
        m_seqroll->set_adding(ischecked);
}

void
qseqeditframe64::update_note_entry (bool on)
{
    ui->m_button_note_entry->setChecked(on);
}

/**
 *  Sets the given background sequence for the Pattern editor so that the
 *  musician has something to see that can be played against.  As a new
 *  feature, it is also passed to the sequence, so that it can be saved as
 *  part of the sequence data, but only if less or equal to the maximum
 *  single-byte MIDI value, 127.
 *
 *  Note that the "initial value" for this parameter is a static variable that
 *  gets set to the new value, so that opening up another sequence causes the
 *  sequence to take on the new "initial value" as well.  A feature, but
 *  should it be optional?  Now it is, based on the setting of
 *  usr().global_seq_feature().
 *
 *  This function is similar to seqedit::set_background_sequence().
 */

void
qseqeditframe64::set_background_sequence (int seqnum, qbase::status qs)
{
    if (! seq::legal(seqnum))                   /* includes seq::limit()    */
        return;

    seq::pointer s = perf().get_sequence(seqnum);
    if (not_nullptr(s))
    {
        if (seqnum < usr().max_sequence())      /* even more restrictive    */
        {
            if (seq::disabled(seqnum) || ! perf().is_seq_active(seqnum))
            {
                ui->m_entry_sequence->setText("Off");
            }
            else
            {
                char n[32];
                snprintf(n, sizeof n, "[%d] %.13s", seqnum, s->name().c_str());
                m_bgsequence = seqnum;
                ui->m_entry_sequence->setText(n);
                if (usr().global_seq_feature())
                    usr().seqedit_bgsequence(seqnum);

                bool user_change = qs == qbase::status::edit;
                if (track().background_sequence(seqnum, user_change))
                {
                    if (not_nullptr(m_seqroll))
                        m_seqroll->set_background_sequence(true, seqnum);

                    set_track_change();         /* to solve issue #90       */
                }
            }
        }
    }
    else
    {
        if (seqnum == seq::limit())
        {
            ui->m_entry_sequence->setText("Off");
            if (usr().global_seq_feature())
                usr().seqedit_bgsequence(seqnum);

            bool user_change = qs == qbase::status::edit;
            if (track().background_sequence(seqnum, user_change))
            {
                if (not_nullptr(m_seqroll))
                    m_seqroll->set_background_sequence(true, seqnum);

                set_track_change();             /* to solve issue #90       */
            }
        }
        else
            msgprintf(msglevel::error, "null pattern %d", seqnum);
    }
}

/**
 *  Sets the data type based on the given parameters.  This function uses the
 *  controller_name() function defined in the controllers.cpp module.
 *
 *  See editable_event.cpp's s_channel_event_names[] for the list of names.
 *  We should centralizze them at some point.
 *
 * \param status
 *      The current editing status.
 *
 * \param control
 *      The control value.  Validated in the controller_name() function.
 */

void
qseqeditframe64::set_data_type (midibyte status, midibyte control)
{
    if (event::is_tempo_status(status))
    {
        m_seqevent->set_data_type(status, control);
        m_seqdata->set_data_type(status, control);
        ui->m_entry_data->setText("Tempo");
    }
    else if (event::is_time_signature_status(status))
    {
        m_seqevent->set_data_type(status, control);
        m_seqdata->set_data_type(status, control);
        ui->m_entry_data->setText("Time Signature");
    }
    else
    {
        char hexa[8];
        char type[32];
        snprintf(hexa, sizeof hexa, "[0x%02X]", status);
        status = event::normalized_status(status);
        m_seqevent->set_data_type(status, control);     /* qstriggereditor  */
        m_seqdata->set_data_type(status, control);
        if (status == EVENT_NOTE_OFF)
            snprintf(type, sizeof type, "Note Off");
        else if (status == EVENT_NOTE_ON)
            snprintf(type, sizeof type, "Note On");
        else if (status == EVENT_AFTERTOUCH)
            snprintf(type, sizeof type, "Aftertouch");
        else if (status == EVENT_CONTROL_CHANGE)
        {
            int bus = int(track().seq_midi_bus());
            int channel = int(track().seq_midi_channel());
            std::string ccname = usr().controller_active(bus, channel, control) ?
                usr().controller_name(bus, channel, control) :
                controller_name(control) ;

            snprintf(type, sizeof type, "CC - %s", ccname.c_str());
        }
        else if (status == EVENT_PROGRAM_CHANGE)
            snprintf(type, sizeof type, "Program");
        else if (status == EVENT_CHANNEL_PRESSURE)
            snprintf(type, sizeof type, "Ch Pressure");
        else if (status == EVENT_PITCH_WHEEL)
            snprintf(type, sizeof type, "Pitch Wheel");
        else if (event::is_meta_status(status))
            snprintf(type, sizeof type, "Meta Event");
        else
            snprintf(type, sizeof type, "Unknown");

        char text[80];
        snprintf(text, sizeof text, "%s %s", hexa, type);
        ui->m_entry_data->setText(text);
    }
}

/**
 * Follow-progress callback.  Passes the Follow status to the qseqroll object.
 */

void
qseqeditframe64::slot_follow (bool ischecked)
{
    if (not_nullptr(m_seqroll))
        m_seqroll->progress_follow(ischecked);
}

/**
 *  Checks the position of the tick, and, if it is in a different piano-roll
 *  "page" than the last page, moves the page to the next page.
 *
 *  We don't want to do any of this if the length of the sequence fits in the
 *  window, but for now it doesn't hurt; the progress bar just never meets the
 *  criterion for moving to the next page.
 *
 * \todo
 *      -   If playback is disabled (such as by a trigger), then do not update
 *          the page;
 *      -   When it comes back, make sure we're on the correct page;
 *      -   When it stops, put the window back to the beginning, even if the
 *          beginning is not defined as "0".
 *
 *      int w = m_seqroll->window_width():   781, what we set.
 *      int w = m_seqroll->width():          Dependent on seq length.
 */

void
qseqeditframe64::follow_progress (bool expand)
{
    int w = ui->rollScrollArea->width();
    if (w > 0)              /* w is constant, e.g. around 742 by default    */
    {
        QScrollBar * hadjust = ui->rollScrollArea->h_scroll();
        if (track().expanded_recording() && expand)
        {
            midipulse prog = track().progress_value();
            int newx = tix_to_pix(prog);
            hadjust->setValue(newx);
        }
        else                                        /* use for non-recording */
        {
            midipulse progtick = track().get_last_tick();
            int progx = tix_to_pix(progtick);
            int page = progx / w;
            int oldpage = m_seqroll->scroll_page();
            bool newpage = page != oldpage;
            if (newpage)
            {
                /*
                 * Here, we need to set some kind of flag for clearing the
                 * viewport before repainting.
                 */

                m_seqroll->scroll_page(page);
                m_seqroll->scroll_offset(progx);
                hadjust->setValue(progx);
            }
        }
    }
    else
    {
        errprint("qseqeditframe64::follow_progress(): 0 seqroll width!");
    }
}

void
qseqeditframe64::scroll_to_tick (midipulse tick)
{
    int w = ui->rollScrollArea->width();
    if (w > 0)              /* w is constant, e.g. around 742 by default    */
    {
        int x = tix_to_pix(tick);
        ui->rollScrollArea->scroll_to_x(x);
    }
}

void
qseqeditframe64::scroll_to_note (int note)
{
    int h = ui->rollScrollArea->height();
    if (h > 0)
    {
        if (is_good_data_byte(midibyte(note)))
        {
            int y = tix_to_pix(note);
            ui->rollScrollArea->scroll_to_y(y);
        }
    }
}

/**
 *  Updates the grid-snap values and control based on the index.  The value is
 *  passed to the set_snap() function for processing.
 *
 * \param index
 *      Provides the index selected from the combo-box.
 */

void
qseqeditframe64::update_grid_snap (int index)
{
    if (index >= 0 && index < snap_list().count())
    {
        int qnfactor = perf().ppqn() * 4;
        int item = snap_list().ctoi(index);
        int v = qnfactor / item;
        set_snap(v);
    }
}

/**
 *  Selects the given snap value, which is the number of ticks in a snap-sized
 *  interval.  It is passed to the seqroll, seqevent, and sequence objects, as
 *  well.
 *
 *  The default initial snap is the default PPQN divided by 4, or the
 *  equivalent of a 16th note (48 ticks).  The snap divisor is 192 * 4 / 48 or
 *  16.
 *
 * \param s
 *      The prospective snap value to set.  It is checked only to make
 *      sure it is greater than 0, to avoid a numeric exception.
 */

void
qseqeditframe64::set_snap (midipulse s)
{
    if (s > 0 && s != m_snap)
    {
        m_snap = int(s);
        if (not_nullptr(m_seqroll))
            m_seqroll->set_snap(s);

        track().snap(s);
        if (not_nullptr(m_seqevent))
            m_seqevent->set_snap(s);        /* qstriggereditor  */

        if (not_nullptr(m_seqtime))
            m_seqtime->set_snap(s);
    }
}

void
qseqeditframe64::reset_grid_snap ()
{
    ui->m_combo_snap->setCurrentIndex(4);
    set_dirty();                                /* modified for issue #90   */
}

/**
 *  Updates the note-length values and control based on the index.  It is
 *  passed to the set_note_length() function for processing.
 *
 * \param index
 *      Provides the index selected from the combo-box.
 */

void
qseqeditframe64::update_note_length (int index)
{
    if (index >= 0 && index < snap_list().count())
    {
        int qnfactor = perf().ppqn() * 4;
        int item = snap_list().ctoi(index);
        int v = qnfactor / item;
        set_note_length(v);
    }
}

/**
 *  Selects the given note-length value.
 *
 * \warning
 *      Currently, we don't handle changes in the global PPQN after the
 *      creation of the menu.  The creation of the menu hard-wires the values
 *      of note-length.  To adjust for a new global PQN, we will need to store
 *      the original PPQN (m_original_ppqn = m_ppqn), and then adjust the
 *      notelength based on the new PPQN.  For example if the new PPQN is
 *      twice as high as 192, then the notelength should double, though the
 *      text displayed in the "Note length" field should remain the same.
 *      However, we do adjust for a non-default PPQN at startup time.
 *
 * \param notelength
 *      Provides the note length in units of MIDI pulses.
 */

void
qseqeditframe64::set_note_length (int notelength)
{
#if defined CAN_MODIFY_GLOBAL_PPQN
    if (perf().ppqn() != m_original_ppqn)
    {
        double factor = double(perf().ppqn()) / double(m_original_ppqn);
        notelength = int(notelength * factor + 0.5);
        m_original_ppqn = perf().ppqn();
    }
#endif

    m_note_length = notelength;
    sm_initial_note_length = notelength;
    if (not_nullptr(m_seqroll))
        m_seqroll->set_note_length(notelength);
}

void
qseqeditframe64::reset_note_length ()
{
    ui->m_combo_note->setCurrentIndex(4);
    set_dirty();                                /* modified for issue #90   */
}

bool
qseqeditframe64::on_resolution_change
(
    int ppqn, midibpm bpm, performer::change /*ch*/
)
{
    bool result = change_ppqn(ppqn);
    if (result)
    {
        if (rc().verbose())
            msgprintf(msglevel::info, "PPQN = %d; BPM = %d", ppqn, int(bpm));
    }
    return result;
}

bool
qseqeditframe64::change_ppqn (int ppqn)
{
    int zoom = usr().zoom();
    set_snap(rescale_tick(sm_initial_snap, ppqn, usr().base_ppqn()));
    set_note_length
    (
        rescale_tick(sm_initial_note_length, ppqn, usr().base_ppqn())
    );
    if (usr().adapt_zoom())
        zoom = zoom_power_of_2(ppqn);

    set_zoom(zoom);
    return true;
}

/**
 *  Updates the grid-zoom values and control based on the index.  The value is
 *  passed to the set_zoom() function for processing.
 *
 * \param index
 *      Provides the index selected from the combo-box.
 */

void
qseqeditframe64::slot_update_zoom (int index)
{
    int z = zoom_list().ctoi(index);
    (void) set_zoom(z);
    set_dirty();                                /* modified for issue #90   */
}

bool
qseqeditframe64::zoom_in ()
{
    int index = zoom_list().index(zoom());
    ui->m_combo_zoom->setCurrentIndex(index);
    return true;
}

bool
qseqeditframe64::zoom_out ()
{
    if (zoom() >= usr().max_zoom())
    {
        int v = zoom_list().ctoi(0);                /* wrap to beginning    */
        set_zoom(v);
    }
    return true;
}

bool
qseqeditframe64::set_zoom (int z)
{
    int zprevious = qseqframe::zoom();
    bool result = qseqframe::set_zoom(z);
    if (result)
    {
        float factor = float(zprevious) / float(zoom());
        int index = zoom_list().index(zoom());
        ui->m_combo_zoom->setCurrentIndex(index);
        set_dirty();                            /* modified for issue #90   */
        ui->rollScrollArea->scroll_x_by_factor(factor);
    }
    return result;
}

void
qseqeditframe64::v_zoom_in ()
{
    m_seqroll->v_zoom_in();
}

void
qseqeditframe64::v_zoom_out ()
{
    m_seqroll->v_zoom_out();
}

void
qseqeditframe64::reset_v_zoom ()
{
    m_seqroll->reset_v_zoom();
}

/**
 *  This override just reset the current index of the zoom combo-box.
 *  That then triggers the callback that sets the current index.
 */

void
qseqeditframe64::slot_reset_zoom ()
{
    (void) qseqframe::reset_zoom();
}

/**
 *  Handles updates to the key selection.
 */

void
qseqeditframe64::update_key (int index)
{
    if (index != m_key && legal_key(index))
        set_key(index);
}

void
qseqeditframe64::set_key (int key, qbase::status qs)
{
    if (legal_key(key))
    {
        m_key = key;
        ui->m_combo_key->setCurrentIndex(key);
        if (not_nullptr(m_seqroll))
            m_seqroll->set_key(key);

        if (not_nullptr(m_seqkeys))
            m_seqkeys->set_key(key);

        if (usr().global_seq_feature())
            usr().seqedit_key(key);

        bool user_change = qs == qbase::status::edit;
        track().musical_key(midibyte(key), user_change);
        set_track_change();                         /* to solve issue #90   */
    }
    else
        errprint("null pattern");
}

void
qseqeditframe64::reset_key ()
{
    set_key(c_key_of_C);
}

/**
 *  Handles updates to the scale selection.
 */

void
qseqeditframe64::update_scale (int index)
{
    set_scale(index);
}

/**
 *  For internal use only.
 */

void
qseqeditframe64::set_scale (int scale, qbase::status qs)
{
    if (legal_scale(scale))
    {
        m_scale = scale;
        ui->m_combo_scale->setCurrentIndex(scale);
        if (not_nullptr(m_seqroll))
            m_seqroll->set_scale(scale);

        if (usr().global_seq_feature())
            usr().seqedit_scale(scale);

        bool user_change = qs == qbase::status::edit;
        track().musical_scale(midibyte(scale), user_change);
        enable_note_menus();
        set_track_change();                         /* to solve issue #90   */
    }
    else
        errprint("null pattern");
}

void
qseqeditframe64::reset_scale ()
{
    set_scale(c_scales_off);
}

void
qseqeditframe64::editor_mode (bool ischecked)
{
    set_editor_mode
    (
        ischecked ? sequence::editmode::drum : sequence::editmode::note
    );
    set_dirty();                        /* modified for issue #90           */
}

void
qseqeditframe64::loop_mode (bool ischecked)
{
    perf().looping(ischecked);
    set_dirty();
}

void
qseqeditframe64::set_editor_mode (sequence::editmode mode)
{
    if (mode != m_edit_mode)
    {
        m_edit_mode = mode;
        perf().edit_mode(track().seq_number(), mode);
        if (not_nullptr(m_seqroll))
            m_seqroll->update_edit_mode(mode);

        set_dirty();                    /* modified for issue #90           */
    }
}

/**
 *  Popup menu events button.
 */

void
qseqeditframe64::events ()
{
    if (not_nullptr(m_events_popup))
    {
        int w = ui->m_button_event->width() - 2;
        int h = ui->m_button_event->height() - 2;
        m_events_popup->exec(ui->m_button_event->mapToGlobal(QPoint(w, h)));
    }
}

/**
 *  Sets the menu pixmap depending on the given state, where true is a
 *  full menu (black background), and empty menu (gray background).
 *
 * \param state
 *      If true, the full-menu image will be created.  Otherwise, the
 *      empty-menu image will be created.
 *
 * \return
 *      Returns a pointer to the created image.
 */

QIcon *
qseqeditframe64::create_menu_image (bool state)
{
    if (usr().dark_theme())
    {
        QPixmap p(state? menu_full_inv_xpm : menu_empty_inv_xpm);
        return new QIcon(p);
    }
    else
    {
        QPixmap p(state? menu_full_xpm : menu_empty_xpm);
        return new QIcon(p);
    }
}

/**
 *  Functions to create event menu entries.  The first overload handles
 *  CC events, and the section handles the other kinds of events.
 */

void
qseqeditframe64::set_event_entry
(
    QMenu * menu,
    const std::string & text,
    bool present,
    midibyte status,
    midibyte control
)
{
    QAction * item = create_menu_action(text, *create_menu_image(present));
    menu->addAction(item);
    connect
    (
        item, &QAction::triggered,
        std::bind(&qseqeditframe64::set_data_type, this, status, control)
    );
    if (present && m_first_event == max_midibyte())
    {
        m_first_event = status;
        m_first_event_name = text;
        set_data_type(status, control);       // need m_first_control value?
    }
}

void
qseqeditframe64::set_event_entry
(
    QMenu * menu,
    bool present,
    event_index ei
)
{
    int index = static_cast<int>(ei);
    std::string text = s_event_items[index].epp_name;
    midibyte status = s_event_items[index].epp_status;
    QAction * item = create_menu_action(text, *create_menu_image(present));
    menu->addAction(item);
    connect
    (
        item, &QAction::triggered,
        std::bind(&qseqeditframe64::set_data_type, this, status, 0)
    );
    if (present && m_first_event == max_midibyte())
    {
        m_first_event = status;
        m_first_event_name = text;
        set_data_type(status);
    }
}

/**
 *  Populates the event-selection menu that drops from the "Event" button in
 *  the bottom row of the Pattern editor.
 *
 *  This menu has a large number of items.  They are filled in by code, but
 *  can also be loaded from seq66.usr, qpseq66.usr, etc.
 *
 *  This function first loops through all of the existing events in the
 *  sequence in order to determine what events exist in it.  If any of the
 *  following events are found, their entry in the menu is marked by a filled
 *  square, rather than a hollow square:
 *
 *      -   Note On
 *      -   Note off
 *      -   Aftertouch
 *      -   Program Change
 *      -   Channel Pressure
 *      -   Pitch Wheel
 *      -   Control Changes from 0 to 127
 *
 *  Compare this function to repopulate_mini_event_menu().
 *
 * \param buss
 *      The selected bus number.
 *
 * \param channel
 *      The selected channel number.
 */

void
qseqeditframe64::repopulate_event_menu (int buss, int channel)
{
    bool ccs[c_midibyte_data_max];
    bool note_on = false;
    bool note_off = false;
    bool aftertouch = false;
    bool program_change = false;
    bool channel_pressure = false;
    bool pitch_wheel = false;
    bool tempo = false;
    bool timesig = false;
    midibyte status = 0, cc = 0;
    memset(ccs, false, sizeof(bool) * c_midibyte_data_max);
    for (auto cev = track().cbegin(); ! track().cend(cev); ++cev)
    {
        if (! track().get_next_event(status, cc, cev))
            break;

        /*
         * Tempo and time-signature are handled after this loop.
         */

        status = event::normalized_status(status);      /* mask off channel */
        switch (status)                                 /* see event_index  */
        {
        case EVENT_NOTE_OFF:            note_off = true;            break;
        case EVENT_NOTE_ON:             note_on = true;             break;
        case EVENT_AFTERTOUCH:          aftertouch = true;          break;
        case EVENT_CONTROL_CHANGE:      ccs[cc] = true;             break;
        case EVENT_PROGRAM_CHANGE:      program_change = true;      break;
        case EVENT_PITCH_WHEEL:         pitch_wheel = true;         break;
        case EVENT_CHANNEL_PRESSURE:    channel_pressure = true;    break;
        case EVENT_MIDI_META:           /* handled below    */      break;
        }
    }

    /*
     * Currently the only meta events that can be edited here are tempo and
     * time signature.  The meta-match function keeps going until it finds
     * these meta events, or it ends.  All we care here is if one exists.
     */

    auto cev = track().cbegin();                    /* scan whole container */
    if (! track().cend(cev))
    {
        if (track().get_next_meta_match(EVENT_META_SET_TEMPO, cev))
            tempo = true;

        cev = track().cbegin();                     /* start over!          */
        if (track().get_next_meta_match(EVENT_META_TIME_SIGNATURE, cev))
            timesig = true;
    }
    if (not_nullptr(m_events_popup))
        delete m_events_popup;

    m_events_popup = new QMenu(this);
    set_event_entry(m_events_popup, note_on, event_index::note_on);
    set_event_entry(m_events_popup, note_off, event_index::note_off);
    m_events_popup->addSeparator();
    set_event_entry(m_events_popup, aftertouch, event_index::aftertouch);

    /*
     * Control changes are handled in submenus constructed below.
     *
     * set_event_entry
     * (
     *      m_events_popup, control_change, event_index::control_change
     * );
     */

    set_event_entry(m_events_popup, program_change, event_index::program_change);
    set_event_entry
    (
        m_events_popup, channel_pressure, event_index::channel_pressure
    );
    set_event_entry(m_events_popup, pitch_wheel, event_index::pitch_wheel);
    set_event_entry(m_events_popup, tempo, event_index::tempo);
    set_event_entry(m_events_popup, timesig, event_index::time_signature);
    m_events_popup->addSeparator();

    /**
     *  Create the 8 sub-menus for the various ranges of controller
     *  changes, shown 16 per sub-menu.
     */

    const int menucount = 8;
    const int itemcount = 16;
    char b[32];
    for (int submenu = 0; submenu < menucount; ++submenu)
    {
        int offset = submenu * itemcount;
        snprintf(b, sizeof b, "Controls %d-%d", offset, offset + itemcount - 1);
        QMenu * menucc = new QMenu(tr(b), m_events_popup);
        for (int item = 0; item < itemcount; ++item)
        {
            /*
             * Do we really want the default controller name to start?  There
             * was a bug in Seq24 where the instrument number was use re 1 to
             * get the proper instrument... it needs to be decremented to be
             * re 0.
             */

            std::string cname(controller_name(offset + item));
            const usermidibus & umb = usr().bus(buss);
            int inst = umb.instrument(channel);
            const userinstrument & uin = usr().instrument(inst);
            if (uin.is_valid())                             // redundant check
            {
                if (uin.controller_active(offset + item))
                    cname = uin.controller_name(offset + item);
            }
            set_event_entry
            (
                menucc, cname, ccs[offset+item],
                EVENT_CONTROL_CHANGE, offset + item
            );
        }
        m_events_popup->addMenu(menucc);
    }
}

/**
 *  Popup menu data button.
 */

void
qseqeditframe64::data ()
{
    if (not_nullptr(m_minidata_popup))
    {
        QPoint bwh(ui->m_button_data->width()-2, ui->m_button_data->height()-2);
        m_minidata_popup->exec(ui->m_button_data->mapToGlobal(bwh));
    }
}

void
qseqeditframe64::repopulate_usr_combos (int buss, int channel)
{
    disconnect
    (
        ui->m_combo_channel, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_midi_channel(int))
    );
    repopulate_midich_combo(buss);
    repopulate_event_menu(buss, channel);
    repopulate_mini_event_menu(buss, channel);
    connect
    (
        ui->m_combo_channel, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_midi_channel(int))
    );
}

/**
 *  Populates the mini event-selection menu that drops from the mini-"Event"
 *  button in the bottom row of the Pattern editor.  This menu has a much
 *  smaller number of items, only the ones that actually exist in the
 *  track/pattern/loop/sequence.
 *
 *  Compare this function to repopulate_event_menu().
 *
 * \param buss
 *      The selected bus number.
 *
 * \param channel
 *      The selected channel number.
 */

void
qseqeditframe64::repopulate_mini_event_menu (int buss, int channel)
{
    bool ccs[c_midibyte_data_max];
    bool note_on = false;
    bool note_off = false;
    bool aftertouch = false;
    bool program_change = false;
    bool channel_pressure = false;
    bool pitch_wheel = false;
    bool tempo = false;
    bool timesig = false;
    bool any_events = false;
    midibyte status = 0, cc = 0;
    memset(ccs, false, sizeof(bool) * c_midibyte_data_max);
    for (auto cev = track().cbegin(); ! track().cend(cev); ++cev)
    {
        if (! track().get_next_event(status, cc, cev))
            break;

        status = event::normalized_status(status);      /* mask off channel */

        /*
         *  Tempo and time-signatures are detected after this loop ends.
         */

        switch (status)
        {
        case EVENT_NOTE_OFF:
            note_off = any_events = true;
            break;

        case EVENT_NOTE_ON:
            note_on = any_events = true;
            break;

        case EVENT_AFTERTOUCH:
            aftertouch = any_events = true;
            break;

        case EVENT_CONTROL_CHANGE:
            ccs[cc] = any_events = true;
            break;

        case EVENT_PROGRAM_CHANGE:
            any_events = program_change = true;
            break;

        case EVENT_PITCH_WHEEL:
            any_events = pitch_wheel = true;
            break;

        case EVENT_CHANNEL_PRESSURE:
            any_events = channel_pressure = true;
            break;

        case EVENT_MIDI_META:
            break;                      /* handled below                    */

        }
    }

    auto cev = track().cbegin();                    /* scan whole container */
    if (! track().cend(cev))
    {
        if (track().get_next_meta_match(EVENT_META_SET_TEMPO, cev))
            tempo = any_events = true;

        cev = track().cbegin();                     /* start over!          */
        if (track().get_next_meta_match(EVENT_META_TIME_SIGNATURE, cev))
            timesig = any_events = true;
    }
    if (not_nullptr(m_minidata_popup))
        delete m_minidata_popup;

    m_minidata_popup = new QMenu(this);
    if (note_on)
        set_event_entry(m_minidata_popup, true, event_index::note_on);

    if (note_off)
        set_event_entry(m_minidata_popup, true, event_index::note_off);

    if (aftertouch)
        set_event_entry(m_minidata_popup, true, event_index::aftertouch);

    /*
     * Control changes are handled in submenus constructed below.
     */

    if (program_change)
        set_event_entry(m_minidata_popup, true, event_index::program_change);

    if (channel_pressure)
        set_event_entry(m_minidata_popup, true, event_index::channel_pressure);

    if (pitch_wheel)
        set_event_entry(m_minidata_popup, true, event_index::pitch_wheel);

    if (tempo)
        set_event_entry(m_minidata_popup, true, event_index::tempo);

    if (timesig)
        set_event_entry(m_minidata_popup, true, event_index::time_signature);

    if (any_events)
        m_minidata_popup->addSeparator();

    /**
     *  Create the menu for the controller changes that actually exist in
     *  the track, if any.
     */

    const int itemcount = c_midibyte_data_max;              /* 128          */
    for (int item = 0; item < itemcount; ++item)
    {
        std::string cname(controller_name(item));
        const usermidibus & umb = usr().bus(buss);
        int inst = umb.instrument(channel);
        const userinstrument & uin = usr().instrument(inst);
        if (uin.is_valid())                                 /* redundant    */
        {
            if (uin.controller_active(item))
                cname = uin.controller_name(item);
        }
        if (ccs[item])
        {
            set_event_entry
            (
                m_minidata_popup, cname, true, EVENT_CONTROL_CHANGE, item
            );
        }
    }
    if (any_events)
    {
        /*
         * Here, we'd like to pre-select the first kind of event found,
         * somehow.
         */

        qt_set_icon(menu_full_xpm, ui->m_button_data);
    }
    else
    {
        set_event_entry(m_minidata_popup, "(no events)", false, 0);
        qt_set_icon(menu_empty_xpm, ui->m_button_data);
    }
}

/*
 * Warning:
 *
 *      In the next 2 functions, not providing "this" means we have to delete
 *      both dialogs in the destructor. Should we fix this?
 */

void
qseqeditframe64::show_lfo_frame ()
{
    if (is_nullptr(m_lfo_wnd))
    {
        m_lfo_wnd = new (std::nothrow) qlfoframe(perf(), track(), *m_seqdata);
        if (not_nullptr(m_lfo_wnd))
            m_lfo_wnd->show();
    }
    else
        m_lfo_wnd->show();
}

void
qseqeditframe64::show_pattern_fix ()
{
    if (is_nullptr(m_patternfix_wnd))
    {
        m_patternfix_wnd = new (std::nothrow)
            qpatternfix(perf(), track(), this);

        if (not_nullptr(m_patternfix_wnd))
            m_patternfix_wnd->show();
    }
    else
        m_patternfix_wnd->show();
}

/**
 *  Duplicative code.  See record_change(), thru_change(), q_record_change().
 */

void
qseqeditframe64::update_midi_buttons ()
{
    static const char * const s_thru_on = "MIDI Thru Active";
    static const char * const s_thru_off = "MIDI Thru Inactive";
    static const char * const s_rec_on = "Record Active";
    static const char * const s_rec_off = "Record Inactive";
    static const char * const s_qrec_on = "Quantized Record Active";
    static const char * const s_qrec_off = "Quantized Record Inactive";

    bool thru_active = track().thru();
    ui->m_toggle_thru->setChecked(thru_active);
    ui->m_toggle_thru->setToolTip(thru_active ? s_thru_on : s_thru_off);
    qt_set_icon(thru_active ? thru_on_xpm : thru_xpm, ui->m_toggle_thru);

    bool record_active = track().recording();
    ui->m_toggle_record->setChecked(record_active);
    ui->m_toggle_record->setToolTip(record_active ? s_rec_on : s_rec_off);
    qt_set_icon(record_active ? rec_on_xpm : rec_xpm, ui->m_toggle_record);

    bool qrecord_active = track().quantized_recording();
    ui->m_toggle_qrecord->setChecked(qrecord_active);
    ui->m_toggle_qrecord->setToolTip(qrecord_active ? s_qrec_on : s_qrec_off);
    qt_set_icon(qrecord_active ? q_rec_on_xpm : q_rec_xpm, ui->m_toggle_qrecord);

    bool playing = track().armed();
    ui->m_toggle_play->setChecked(playing);
    ui->m_toggle_play->setToolTip(playing ? "Armed" : "Muted");
    qt_set_icon(playing ? play_on_xpm : play_xpm, ui->m_toggle_play);
}

/**
 *  Passes the play status to the sequence object.
 */

void
qseqeditframe64::play_change (bool ischecked)
{
    if (track().set_armed(ischecked))
        update_midi_buttons();
}

/**
 *  Passes the recording status to performer.
 */

void
qseqeditframe64::record_change (bool ischecked)
{
    if (perf().set_recording(track(), ischecked, false))
        update_midi_buttons();
}

/**
 *  Passes the quantized-recording status to the performer object.
 *
 * Stazed fix:
 *
 *      If we set Quantized recording, then also set recording, but do not
 *      unset recording if we unset Quantized recording.
 *
 *  This is not necessarily the most intuitive thing to do.  See
 *  midi_record.txt.
 */

void
qseqeditframe64::q_record_change (bool ischecked)
{
    /*
     * Tricky code.  Turn off reqular recording first. This allows the
     * Q record button to get set, and turn both Q and regular recording on.
     */

    if (ischecked)
        (void) (perf().set_recording(track(), false, false));

    if (perf().set_quantized_recording(track(), ischecked, false))
        update_midi_buttons();
}

/**
 *  Passes the MIDI Thru status to performer.
 */

void
qseqeditframe64::thru_change (bool ischecked)
{
    if (perf().set_thru(track(), ischecked, false))
        update_midi_buttons();
}

/**
 *  If the last recording style is oneshot we can select reset. If we then
 *  select oneshot_reset, we reset the sequence and reselect oneshot.
 */

void
qseqeditframe64::update_record_type (int index)
{
    int lroneshot = usr().grid_record_code(recordstyle::oneshot);
    int lrreset = usr().grid_record_code(recordstyle::oneshot_reset);
    bool ok = track().update_recording(index);
    enable_combobox_item(ui->m_combo_rec_type, lrreset, index == lroneshot);
    if (index == lrreset)                               /* oneshot reset    */
    {
        if (m_last_record_style == recordstyle::oneshot)
            ui->m_combo_rec_type->setCurrentIndex(lroneshot);
    }
    m_last_record_style = usr().grid_record_style(index);
    if (ok)
        set_dirty();                            /* modified for issue #90   */
}

void
qseqeditframe64::update_recording_volume (int index)
{
    if (index >= 0 && index < rec_vol_list().count())
    {
        int recvol = rec_vol_list().ctoi(index);
        if (index == 0)
            recvol = (-1);                      /* force preserve-velocity  */

        set_recording_volume(recvol);
        set_dirty();                            /* modified for issue #90   */
    }
}

void
qseqeditframe64::update_loop_count (int value)
{
    if (track().loop_count_max(value, true))    /* it is a user-change      */
        set_track_change();                     /* added for issue #90      */
}

void
qseqeditframe64::reset_recording_volume ()
{
    ui->m_combo_rec_vol->setCurrentIndex(0);
    set_dirty();                                /* modified for issue #90   */
}

/**
 *  Passes the given parameter to sequence::set_rec_vol().  This function also
 *  changes the button's text to match the selection, and also changes the
 *  global velocity-override setting in usrsettings.  Note that the setting
 *  will not be saved to the "usr" configuration file unless Seq66 was
 *  run with the "--user-save" option.
 *
 * \param recvol
 *      The setting to be made, obtained from the recording-volume ("Vol")
 *      menu.
 */

void
qseqeditframe64::set_recording_volume (int recvol)
{
    track().set_rec_vol(recvol);        /* save to the sequence settings    */
    usr().velocity_override(recvol);    /* save to the "usr" config file    */
    set_dirty();                                /* modified for issue #90   */
}

void
qseqeditframe64::set_dirty ()
{
    if (is_initialized())
    {
        qseqframe::set_dirty();         /* roll, time, date & event panes   */
        m_seqroll->set_redraw();        /* also calls set_dirty();          */

        /*
         * These are done in qseqframe::set_dirty().
         *
         * m_seqdata->set_dirty();      // update(); neither cause refresh  //
         * m_seqevent->set_dirty();     // how about this? same!            //
         * m_seqtime->set_dirty();      // any comment?                     //
         *
         * Make each location of change call this, if the action is indeed
         * a sequence change.  Too tricky!  See the new set_track_change()
         * function.
         *
         * track().modify(false);       // do NOT do notify-change!         //
         */
    }
    update_draw_geometry();
}

/**
 * For issue #90, we had to remove the track-change marking from
 * set_dirty().  But some of the locations where that function was
 * called need to mark the sequence as modified.  This function
 * replaces set_dirty() for those places.
 */

void
qseqeditframe64::set_track_change ()
{
    set_dirty();
    if (is_initialized())                           /* start-up changes?    */
    {
        set_external_frame_title();
        track().modify(false);                      /* do not change-notify */
    }
}

void
qseqeditframe64::set_external_frame_title ()
{
    if (not_nullptr(m_qseqeditex_frame))
        m_qseqeditex_frame->set_title(true);    /* show an asterisk     */
}

/**
 *  Causes all of the panels to be updated.
 */

void
qseqeditframe64::update_draw_geometry()
{
    if (not_nullptr(m_seqroll))
        m_seqroll->updateGeometry();

    if (not_nullptr(m_seqtime))
        m_seqtime->updateGeometry();

    if (not_nullptr(m_seqdata))
        m_seqdata->updateGeometry();

    if (not_nullptr(m_seqevent))
        m_seqevent->updateGeometry();
}

/**
 *  Implements the actions brought forth from the Tools (hammer) button.
 *  Note that the push_undo(), called internally, pushes all of the current
 *  events (in sequence::m_events) onto the stack (as a single entry).
 */

void
qseqeditframe64::do_action (eventlist::edit action, int var)
{
    track().handle_edit_action(action, var);
    set_dirty();                                /* modified for issue #90   */
}

/**
 *  Removes the LFO editor frame.  For 0.98.7, we had the issue where closing
 *  the seqedit frame would leave this window and the pattern-fix window
 *  open. So now, instead of deleting them, we signal them to close.
 */

void
qseqeditframe64::remove_lfo_frame ()
{
    if (not_nullptr(m_lfo_wnd))
        m_lfo_wnd->close();
}

/**
 *  Removes the pattern-fix frame.
 */

void
qseqeditframe64::remove_patternfix_frame ()
{
    if (not_nullptr(m_patternfix_wnd))
        m_patternfix_wnd->close();
}

QWidget *
qseqeditframe64::rollview ()
{
    return ui->rollScrollArea->viewport();
}

QWidget *
qseqeditframe64::rollwidget () const
{
    return ui->rollScrollArea->widget();
}

}           // namespace seq66

/*
 * qseqeditframe64.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


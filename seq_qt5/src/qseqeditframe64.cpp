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
 *  This module declares/defines the base class for plastering
 *  pattern/sequence data information in the data area of the pattern editor.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-06-15
 * \updates       2022-04-13
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
#include <QStandardItemModel>           /* for disabling combobox entries   */

#include "midi/controllers.hpp"         /* seq66::controller_name()         */
#include "play/performer.hpp"           /* seq66::performer reference       */
#include "qlfoframe.hpp"                /* seq66::qlfoframe dialog class    */
#include "qpatternfix.hpp"              /* seq66::qpatternfix dialog class  */
#include "qseqdata.hpp"                 /* seq66::qseqdata panel            */
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
#include "pixmaps/length_short.xpm"     /* not length.xpm, it is too long   */
#include "pixmaps/menu_empty.xpm"
#include "pixmaps/menu_empty_inv.xpm"
#include "pixmaps/menu_full.xpm"
#include "pixmaps/menu_full_inv.xpm"
#include "pixmaps/midi.xpm"
#include "pixmaps/note_length.xpm"
#include "pixmaps/play.xpm"
#include "pixmaps/q_rec.xpm"
#include "pixmaps/quantize.xpm"
#include "pixmaps/rec.xpm"
#include "pixmaps/redo.xpm"
#include "pixmaps/scale.xpm"
#include "pixmaps/sequences.xpm"
#include "pixmaps/snap.xpm"
#include "pixmaps/thru.xpm"
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
 */

static const int s_zoom_items [] =
{
    1, 2, 4, 8, 16, 32, 64, 128, 256, 512
};
static const int s_zoom_count = sizeof(s_zoom_items) / sizeof(int);
static const int s_zoom_default = 1;                    /* the array index  */

/**
 *  Looks up a zoom value and returns its index.
 */

static int
s_lookup_zoom (int zoom)
{
    int result = 0;
    for (int zi = 0; zi < s_zoom_count; ++zi)
    {
        if (s_zoom_items[zi] == zoom)
        {
            result = zi;
            break;
        }
    }
    return result;
}

/**
 *  Hold the entries in the "Vel" drop-down.  The first value matches
 *  usr().preserve_velocity().  It corresponds to the "Free" recording-volume
 *  entry.
 */

static const int s_rec_vol_items [] =
{
    -1, 127, 112, 96, 80, 64, 48, 32, 16
};
static const int s_rec_vol_count = sizeof(s_rec_vol_items) / sizeof(int);

/*
 * For set_event_entry calls.
 */

using event_popup_pair = struct
{
    std::string epp_name;
    midibyte epp_status;
};

event_popup_pair
s_event_items [] =
{
    { "Note On Velocity",   EVENT_NOTE_ON           },
    { "Note Off Velocity",  EVENT_NOTE_OFF          },
    { "Aftertouch",         EVENT_AFTERTOUCH        },
    { "Program Change",     EVENT_PROGRAM_CHANGE    },
    { "Channel Pressure",   EVENT_CHANNEL_PRESSURE  },
    { "Pitch Wheel",        EVENT_PITCH_WHEEL       },
    { "Tempo",              EVENT_META_SET_TEMPO    }       /* special */
};

/**
 *
 * \param p
 *      Provides the performer object to use for interacting with this
 *      sequence.  Among other things, this object provides the active PPQN.
 *
 * \param seqid
 *      Provides the sequence number.  The sequence pointer is looked up using
 *      this number.  This number is also the pattern-slot number for this
 *      sequence and for this window.  Ranges from 0 to 1024.
 *
 * \param parent
 *      Provides the parent window/widget for this container window.  Defaults
 *      to null.
 *
 * \param shorter
 *      If true, the data window is halved in size, and some controls are
 *      hidden, to compact the editor for use a tab.
 */

qseqeditframe64::qseqeditframe64
(
    performer & p, int seqid,
    QWidget * parent, bool shorter
) :
    qseqframe               (p, seqid, parent),
    performer::callbacks    (p),
    ui                      (new Ui::qseqeditframe64),
    m_short_version         (shorter),              /* short_version()  */
    m_lfo_wnd               (nullptr),
    m_patternfix_wnd        (nullptr),
    m_tools_popup           (nullptr),
    m_tools_harmonic        (nullptr),
    m_sequences_popup       (nullptr),
    m_events_popup          (nullptr),
    m_minidata_popup        (nullptr),
    m_measures_list         (measure_items()),      /* see settings module  */
    m_beats_per_bar         (0),                    /* set in ctor body     */
    m_beatwidth_list        (beatwidth_items()),    /* see settings module  */
    m_beat_width            (0),                    /* set in ctor body     */
    m_snap_list             (snap_items()),         /* see settings module  */
    m_snap                  (sm_initial_snap),
    m_note_length           (sm_initial_note_length),
    m_scale                 (0),                    /* set in ctor body     */
    m_chord                 (0),
    m_key                   (usr().seqedit_key()),
    m_bgsequence            (0),                    /* set in ctor body     */
    m_measures              (0),
    m_edit_bus              (0),
    m_edit_channel          (0),                    /* 0-15, null           */
    m_first_event           (max_midibyte()),
    m_first_event_name      ("(no events)"),
    m_have_focus            (false),
    m_edit_mode             (perf().edit_mode(seqid)),
    m_last_record_style     (recordstyle::merge),
    m_timer                 (nullptr)
{
    std::string seqname = "No sequence, try again";
    int loopcountmax = 0;
    bool isnewpattern = false;
    sequence * s = seq_pointer().get();
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);             /* part of issue #4     */
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    if (not_nullptr(s))
    {
        set_beats_per_measure(s->get_beats_per_bar());
        set_beat_width(s->get_beat_width());
        m_scale = s->musical_scale();
        m_edit_bus = s->seq_midi_bus();
        m_edit_channel = s->midi_channel();         /* 0-15, null           */
        seqname = s->name();
        loopcountmax = s->loop_count_max();
        initialize_panels();                        /* uses seq_pointer()   */
        if (usr().global_seq_feature())
        {
            set_scale(usr().seqedit_scale());
            set_key(usr().seqedit_key());
            set_background_sequence(usr().seqedit_bgsequence());
            s->musical_scale(usr().seqedit_scale());
            s->musical_key(usr().seqedit_key());
            s->background_sequence(usr().seqedit_bgsequence());
        }
        else
        {
            set_scale(s->musical_scale());
            set_key(s->musical_key());
            set_background_sequence(s->background_sequence());
        }
        if (s->is_new_pattern())
        {
            isnewpattern = true;

            /*
             *  This check looks only for "Untitled" and no events. Causes
             *  opening this window to unmute patterns in generic *.mid files.
             */

            play_change(usr().new_pattern_armed());
            thru_change(usr().new_pattern_thru());
            record_change(usr().new_pattern_record());
            q_record_change(usr().new_pattern_qrecord());
        }
    }
    else
    {
        set_beats_per_measure(usr().midi_beats_per_bar());
        set_beat_width(usr().midi_beat_width());
        set_scale(usr().seqedit_scale());
        set_key(usr().seqedit_key());
        set_background_sequence(usr().seqedit_bgsequence());
    }

    /*
     *  Sequence Number Label
     */

    std::string seqtext("#");
    seqtext += std::to_string(seqid);
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
     * Beats Per Bar.  Fill the options for the beats per measure combo-box,
     * and set the default.  These range from 1 to 16, plus a value of 32.
     */

    qt_set_icon(down_xpm, ui->m_button_bpm);
    connect
    (
        ui->m_button_bpm, SIGNAL(clicked(bool)),
        this, SLOT(reset_beats_per_measure())
    );

    QString thirtytwo = QString::number(32);
    for (int b = 0; b < s_beat_measure_count; ++b)
    {
        QString combo_text = QString::number(b + 1);
        ui->m_combo_bpm->insertItem(b, combo_text);
    }
    ui->m_combo_bpm->insertItem(s_beat_measure_count, thirtytwo);

    int beatspm = m_beats_per_bar;  /* seq_pointer()->get_beats_per_bar()   */
    std::string beatstring = std::to_string(beatspm);
    ui->m_combo_bpm->setCurrentIndex(beatspm - 1);
    ui->m_combo_bpm->setEditText(qt(beatstring));
    connect
    (
        ui->m_combo_bpm, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_beats_per_measure(int))
    );
    connect
    (
        ui->m_combo_bpm, SIGNAL(currentTextChanged(const QString &)),
        this, SLOT(text_beats_per_measure(const QString &))
    );
    set_beats_per_measure(beatspm);

    /*
     * Beat Width (denominator of time signature).  Fill the options for
     * the beats per measure combo-box, and set the default.
     */

    qt_set_icon(down_xpm, ui->m_button_bw);
    connect
    (
        ui->m_button_bw, SIGNAL(clicked(bool)),
        this, SLOT(reset_beat_width())
    );

#if defined USE_OLD_CODE
    for (int i = 0; i < s_beat_width_count; ++i)
    {
        int w = s_beat_width_items[i];
        QString combo_text = QString::number(w);
        ui->m_combo_bw->insertItem(w, combo_text);
    }
#else
    (void) fill_combobox(ui->m_combo_bw, m_beatwidth_list);
#endif

    int bwindex = m_beatwidth_list.index(m_beat_width);
    int bw = m_beat_width;              /* seq_pointer()->get_beat_width() */
    std::string bstring = std::to_string(bw);
    ui->m_combo_bw->setCurrentIndex(bwindex);
    ui->m_combo_bw->setEditText(qt(bstring));
    connect
    (
        ui->m_combo_bw, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_beat_width(int))
    );
    connect
    (
        ui->m_combo_bw, SIGNAL(currentTextChanged(const QString &)),
        this, SLOT(text_beat_width(const QString &))
    );
    set_beat_width(bw);

    /*
     * Pattern Length in Measures. Fill the options for the beats per measure
     * combo-box, and set the default.
     */

    qt_set_icon(length_short_xpm, ui->m_button_length);
    connect
    (
        ui->m_button_length, SIGNAL(clicked(bool)),
        this, SLOT(reset_measures())
    );
#if defined USE_OLD_CODE
    for (int m = 0; m < s_measures_count; ++m)
    {
        std::string itext = std::to_string(s_measures_items[m]);
        QString combo_text = qt(itext);
        ui->m_combo_length->insertItem(m, combo_text);
    }
#else
    (void) fill_combobox(ui->m_combo_length, m_measures_list);
#endif

    int len_index = m_measures_list.index(m_measures);
    int measures = not_nullptr(s) ? s->calculate_measures() : 1 ;
    std::string mstring = std::to_string(measures);
    ui->m_combo_length->setCurrentIndex(len_index);
    ui->m_combo_length->setEditText(qt(mstring));
    connect
    (
        ui->m_combo_length, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_measures(int))
    );
    connect
    (
        ui->m_combo_length, SIGNAL(currentTextChanged(const QString &)),
        this, SLOT(text_measures(const QString &))
    );
    if (not_nullptr(s))
        (void) s->unit_measure();        /* must precede set_measures()     */

    set_measures(get_measures());

    bool can_transpose = not_nullptr(s) ? s->transposable() : false ;
    if (shorter)
    {
        ui->m_toggle_drum->hide();      /* ui->m_toggle_transpose->hide()   */
        ui->m_map_notes->hide();
        ui->spacer_button_4->hide();
        ui->spacer_button_5->hide();
    }
    else
    {
        /*
         * Drum-Mode Button.  Qt::NoFocus is the default focus policy.
         */

        ui->m_toggle_drum->setAutoDefault(false);
        ui->m_toggle_drum->setCheckable(true);
        qt_set_icon(drum_xpm, ui->m_toggle_drum);
        connect
        (
            ui->m_toggle_drum, SIGNAL(toggled(bool)),
            this, SLOT(editor_mode(bool))
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
        int buses = opm.active() ?
            opm.count() : mmb->get_num_out_buses() ;

        for (int bus = 0; bus < buses; ++bus)
        {
            e_clock ec;
            std::string busname;
            if (perf().ui_get_clock(bussbyte(bus), ec, busname))
            {
                bool disabled = ec == e_clock::disabled;
                ui->m_combo_bus->addItem(qt(busname));
                if (disabled)
                {
                    QStandardItemModel * model =
                        qobject_cast<QStandardItemModel *>
                        (
                            ui->m_combo_bus->model()
                        );
                    QStandardItem * item = model->item(bus);
                    item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
                }
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

#if defined USE_OLD_CODE
    for (int si = 0; si < s_snap_count; ++si)
    {
        int item = s_snap_items[si];
        std::string itext = item > 1 ?
            "1/" + std::to_string(item) : std::to_string(item);

        QString combo_text = qt(itext);
        if (item == 0)
        {
            ui->m_combo_snap->insertSeparator(8);   // why 8?
            ui->m_combo_note->insertSeparator(8);   // why 8?
            continue;
        }
        else
        {
            ui->m_combo_snap->insertItem(si, combo_text);
            ui->m_combo_note->insertItem(si, combo_text);
        }
    }
    ui->m_combo_snap->setCurrentIndex(4);               /* 16th-note entry  */
    ui->m_combo_note->setCurrentIndex(4);               /* ditto            */
#else
    (void) fill_combobox(ui->m_combo_snap, m_snap_list, 4, "1/"); /* 1/16th */
    (void) fill_combobox(ui->m_combo_note, m_snap_list, 4, "1/"); /* ditto  */
#endif

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
    for (int zi = 0; zi < s_zoom_count; ++zi)
    {
        int zoom = s_zoom_items[zi];
        std::string itext = "1:" + std::to_string(zoom);
        QString combo_text = qt(itext);
        ui->m_combo_zoom->insertItem(zi, combo_text);
    }
    ui->m_combo_zoom->setCurrentIndex(1);
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
    ui->m_button_note_entry->setToolTip
    (
        "Toggle between selection and note-entry modes\nin the piano roll."
    );
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
     * Enable (unmute) Play Button.
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
    ui->m_toggle_record->setCheckable(true);
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
    for (int v = 0; v < s_rec_vol_count; ++v)
    {
        int item = s_rec_vol_items[v];
        std::string text = v == 0 ? "Free" : std::to_string(item) ;
        QString combo_text = qt(text);
        ui->m_combo_rec_vol->insertItem(v, combo_text);
    }
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
    set_midi_bus(m_edit_bus);
    set_midi_channel(m_edit_channel);        /* 0 to 15 or Free (0x80)   */

    int seqwidth = m_seqroll->width();
    int scrollwidth = ui->rollScrollArea->width();
    m_seqroll->progress_follow(seqwidth > scrollwidth);
    ui->m_toggle_follow->setChecked(m_seqroll->progress_follow());
    update_midi_buttons();

    /*
     *  This code was not present here, but was in the old-style editor and the
     *  event-editor.  This comment is a reminder.  See qseqeventframe for more
     *  information.
     *
     *      m_seq->seq_in_edit(true);
     */

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
            start_playing();
    }

    bool isctrl = bool(event->modifiers() & Qt::ControlModifier);
    if (! isctrl)
    {
        if (key == Qt::Key_J)
            scroll_by_step(qscrollmaster::dir::Down);
        else if (key == Qt::Key_K)
            scroll_by_step(qscrollmaster::dir::Up);
        else if (key == Qt::Key_H)
            scroll_by_step(qscrollmaster::dir::Left);
        else if (key == Qt::Key_L)
            scroll_by_step(qscrollmaster::dir::Right);
    }
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
 *  Currently this function is not called.  To be investigated
 */

bool
qseqeditframe64::on_sequence_change (seq::number seqno, bool /*recreate*/)
{
    bool result = seqno == seq_pointer()->seq_number();
    if (result)
        set_dirty();

    return result;
}

bool
qseqeditframe64::on_automation_change (automation::slot s)
{
    if (s == automation::slot::start || s == automation::slot::stop)
    {
        m_seqroll->set_redraw();                // more? data and event panes?
    }
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
        perf(), seq_pointer(), this, ui->keysScrollArea,
        noteheight, height
    );
    ui->keysScrollArea->setWidget(m_seqkeys);
    ui->keysScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->keysScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->keysScrollArea->verticalScrollBar()->setRange(0, height);
    m_seqtime = new (std::nothrow) qseqtime
    (
        perf(), seq_pointer(), this,
        zoom(), ui->timeScrollArea
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
        perf(), seq_pointer(), this,
        m_seqkeys, zoom(), m_snap, sequence::editmode::note,
        noteheight, height
    );

    ui->rollScrollArea->setWidget(m_seqroll);
    ui->rollScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->rollScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_seqroll->update_edit_mode(m_edit_mode);
    m_seqdata = new (std::nothrow) qseqdata
    (
        perf(), seq_pointer(), this,
        zoom(), m_snap, ui->dataScrollArea,
        short_version() ? 64 : 0                /* 0 means "normal height"  */
    );
    ui->dataScrollArea->setWidget(m_seqdata);
    ui->dataScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->dataScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_seqevent = new (std::nothrow) qstriggereditor
    (
        perf(), seq_pointer(), this,
        zoom(), m_snap, noteheight, ui->eventScrollArea, 0
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
}

/**
 *  We need to set the dirty state while the sequence has been changed.
 */

void
qseqeditframe64::conditional_update ()
{
    bool expandrec = seq_pointer()->expand_recording();
    if (expandrec)
    {
        set_measures(get_measures() + 1);
        follow_progress(expandrec);             /* keep up with progress    */
    }
    else if (not_nullptr(m_seqroll) && m_seqroll->progress_follow())
    {
        follow_progress();
    }
    if (seq_pointer() && seq_pointer()->check_loop_reset())
    {
        /*
         * Now we need to update the event and data panes.  Note that the
         * notes update during the next pass through the loop only if more
         * notes come in on the input buss.
         */

        set_dirty();
        update_midi_buttons();                  /* mirror current states    */
    }
}

/**
 *  Handles edits of the sequence title.
 */

void
qseqeditframe64::update_seq_name ()
{
    std::string name = ui->m_entry_name->text().toStdString();
    if (perf().set_sequence_name(seq_pointer(), name))
        set_dirty();
}

/**
 *  Handles updates to the beats/measure for only the current sequences.
 *  See the similar function in qsmainwnd.
 */

void
qseqeditframe64::update_beats_per_measure (int index)
{
    ++index;

    int bpb = index == s_beat_measure_count ? 32 : index ;
    if (bpb != m_beats_per_bar)
    {
        set_beats_per_measure(bpb);
        set_dirty();
    }
}

void
qseqeditframe64::text_beats_per_measure (const QString & text)
{
    std::string temp = text.toStdString();
    if (! temp.empty())
    {
        int beats = std::stoi(temp);
        if (usr().bpb_is_valid(beats))
            set_beats_per_measure(beats);
        else
            reset_beats_per_measure();
    }
}

void
qseqeditframe64::reset_beats_per_measure ()
{
    seq::number seqno = seq_pointer()->seq_number();
    ui->m_combo_bpm->setCurrentIndex(usr().bpb_default() - 1);
    perf().notify_sequence_change(seqno, performer::change::recreate);
    update_draw_geometry();
}

/**
 *  Applies the new beats/bar (beats/measure) value to the sequence and the
 *  user interface.
 *
 * \param bpb
 *      The desired beats/measure value.
 */

void
qseqeditframe64::set_beats_per_measure (int bpb)
{
    sequence * s = seq_pointer().get();
    m_beats_per_bar = bpb;
    if (not_nullptr(s))
    {
        int measures = get_measures();
        s->set_beats_per_bar(bpb);
        s->apply_length(bpb, perf().ppqn(), s->get_beat_width(), measures);
        update_draw_geometry();
    }
}

/**
 *  Set the measures value, using the given parameter, and some internal
 *  values passed to apply_length().
 *
 * \param len
 *      Provides the sequence length, in measures.
 */

void
qseqeditframe64::set_measures (int len)
{
    m_measures = len;
    seq_pointer()->apply_length(len);           /* use the simpler overload */
    set_dirty();
}

/**
 *  Resets the pattern-length in its combo-box.
 */

void
qseqeditframe64::reset_measures ()
{
    ui->m_combo_length->setCurrentIndex(0);
    update_draw_geometry();
}

/**
 *  Calculates the measures in the current sequence with its current status.
 *
 * \return
 *      Returns the calculated number of measures in the sequence managed by this
 *      object.
 */

int
qseqeditframe64::get_measures ()
{
    return seq_pointer()->get_measures();
}

/**
 *  Handles updates to the beat width for only the current sequences.
 *  See the similar function in qsmainwnd.
 */

void
qseqeditframe64::update_beat_width (int index)
{
#if defined USE_OLD_CODE
    ++index;

    int bw = index == s_beat_width_count ? 32 : index ;
#else
    int bw = m_beatwidth_list.ctoi(index);
#endif
    if (bw != m_beat_width)
    {
        set_beat_width(bw);
        set_dirty();
    }
}

void
qseqeditframe64::text_beat_width (const QString & text)
{
    std::string temp = text.toStdString();
    if (! temp.empty())
    {
        int width = std::stoi(temp);
        if (usr().bw_is_valid(width))
            set_beat_width(width);
        else
            reset_beat_width();
    }
}

/**
 *  Resets the beat-width combo-box to its default value.
 */

void
qseqeditframe64::reset_beat_width ()
{
    int index = m_beatwidth_list.index(usr().bw_default());
    ui->m_combo_bw->setCurrentIndex(index);
    update_draw_geometry();
}

/**
 *  Sets the beat-width value and then dirties the user-interface so that it
 *  will be repainted.
 */

void
qseqeditframe64::set_beat_width (int bw)
{
    sequence * s = seq_pointer().get();
    m_beat_width = bw;
    if (not_nullptr(s))
    {
        int measures = get_measures();
        s->set_beat_width(bw);
        s->apply_length(s->get_beats_per_bar(), perf().ppqn(), bw, measures);
        update_draw_geometry();
    }
}

/**
 *  Handles updates to the pattern length.
 */

void
qseqeditframe64::update_measures (int index)
{
#if defined USE_OLD_CODE
    int m = s_measures_items[index];
#else
    int m = m_measures_list.ctoi(index);
#endif
    if (m != m_measures)
    {
        set_measures(m);
        set_dirty();
    }
}

void
qseqeditframe64::text_measures (const QString & text)
{
    std::string temp = text.toStdString();
    if (! temp.empty())
    {
        int measures = std::stoi(temp);
        if (measures >= 1 && measures <= 99999)        /* sanity check only */
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
    int index = m_measures_list.index(m_measures);
    if (++index >= m_measures_list.count())
        index = 0;

    ui->m_combo_length->setCurrentIndex(index);
    int m = m_measures_list.ctoi(index);
    if (m != m_measures)
        set_measures(m);
}

/**
 *  Passes the transpose status to the sequence object.
 */

void
qseqeditframe64::transpose (bool ischecked)
{
    seq_pointer()->set_transposable(ischecked);
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
        set_dirty();
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

    update_draw_geometry();
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
    set_midi_bus(index, true);                      /* always a user-change */
}

/**
 *  Resets the MIDI bus value to its default.
 */

void
qseqeditframe64::reset_midi_bus ()
{
    set_midi_bus(0, true);                          /* always a user-change */
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
qseqeditframe64::set_midi_bus (int bus, bool user_change)
{
    bussbyte initialbus = seq_pointer()->seq_midi_bus();
    bussbyte b = bussbyte(bus);
    if (b != initialbus)
    {
        seq_pointer()->set_midi_bus(b, user_change);
        m_edit_bus = bus;
        if (user_change)
        {
            repopulate_usr_combos(m_edit_bus, m_edit_channel);
            set_dirty();
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

    int ch = seq_pointer()->midi_channel();
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
    set_midi_channel(index, true);                  /* always a user-change */
}

void
qseqeditframe64::reset_midi_channel ()
{
    set_midi_channel(0, true);                      /* always a user-change */
}

/**
 *  Selects the given MIDI channel parameter in the main sequence object,
 *  so that it will use that channel.  If "Free" is selected, all that happens
 *  is that the pattern is set to "no-channel" mode for MIDI output.
 *
 * \param ch
 *      The MIDI channel value to set.
 *
 * \param user_change
 *      True if the user made this change, and thus has potentially modified
 *      the song.  The default is false.
 */

void
qseqeditframe64::set_midi_channel (int ch, bool user_change)
{
    int initialchan = seq_pointer()->seq_midi_channel();
    if (ch != initialchan || ! user_change)
    {
        int chindex = ch;
        midibyte channel = midibyte(ch);
        if (! is_good_channel(channel))
        {
            chindex = c_midichannel_max;                    /* "Free" */
            channel = null_channel();
        }
        if (seq_pointer()->set_midi_channel(channel, user_change))
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
                    set_dirty();
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
    seq_pointer()->pop_undo();
}

/**
 *  Does a pop-redo on the sequence object and then sets the dirty flag.
 */

void
qseqeditframe64::redo ()
{
    seq_pointer()->pop_redo();
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

    QMenu * menuselect = new QMenu(tr("&Select..."), m_tools_popup);
    QMenu * menutiming = new QMenu(tr("&Timing..."), m_tools_popup);
    QMenu * menupitch  = new QMenu(tr("&Pitch transpose..."), m_tools_popup);
    QMenu * menuharmonic = new QMenu(tr("&Harmonic transpose..."), m_tools_popup);
    QMenu * menumore = new QMenu(tr("&More tools..."), m_tools_popup);
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

    QAction * lfobox = new QAction(tr("&LFO..."), m_tools_popup);
    connect(lfobox, SIGNAL(triggered(bool)), this, SLOT(show_lfo_frame()));
    menumore->addAction(lfobox);

    QAction * fixbox = new QAction(tr("Pattern &fix..."), m_tools_popup);
    connect(fixbox, SIGNAL(triggered(bool)), this, SLOT(show_pattern_fix()));
    menumore->addAction(fixbox);

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
    m_tools_popup->addMenu(menumore);
    m_tools_harmonic = menuharmonic;
    m_tools_harmonic->setEnabled(m_scale > 0);
}

/**
 *  Consider adding Aftertouch events.
 */

void
qseqeditframe64::select_all_notes ()
{
    seq_pointer()->select_events(EVENT_NOTE_ON, 0);
    seq_pointer()->select_events(EVENT_NOTE_OFF, 0);
}

/**
 *  Consider adding Aftertouch events.
 */

void
qseqeditframe64::inverse_note_selection ()
{
    seq_pointer()->select_events(EVENT_NOTE_ON, 0, true);
    seq_pointer()->select_events(EVENT_NOTE_OFF, 0, true);
}

/**
 *  Consider adding Aftertouch events.
 */

void
qseqeditframe64::quantize_notes ()
{
    seq_pointer()->push_quantize(EVENT_NOTE_ON, 0, 1, false);
}

/**
 *  Consider adding Aftertouch events.
 */

void
qseqeditframe64::tighten_notes ()
{
    seq_pointer()->push_quantize(EVENT_NOTE_ON, 0, 2, true);
}

/**
 *  Consider adding Aftertouch events.  Done in sequence::transpose_notes().
 */

void
qseqeditframe64::transpose_notes ()
{
    QAction * senderAction = (QAction *) sender();
    int transposeval = senderAction->data().toInt();
    seq_pointer()->transpose_notes(transposeval, 0);
}

void
qseqeditframe64::transpose_harmonic ()
{
    QAction * senderAction = (QAction *) sender();
    int transposeval = senderAction->data().toInt();
    seq_pointer()->transpose_notes(transposeval, m_scale, m_key);
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
    connect(off, &QAction::triggered, SET_BG_SEQ(seq::limit(), true));
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
                    connect(item, &QAction::triggered, SET_BG_SEQ(s, true));
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
qseqeditframe64::set_background_sequence (int seqnum, bool user_change)
{
    if (! seq::valid(seqnum))
        return;

    seq::pointer s = perf().get_sequence(seqnum);
    if (not_nullptr(s))
    {
        if (seqnum < usr().max_sequence())      /* even more restrictive */
        {
            if (seq::disabled(seqnum) || ! perf().is_seq_active(seqnum))
            {
                ui->m_entry_sequence->setText("Off");
            }
            else
            {
                char n[24];
                snprintf(n, sizeof n, "[%d] %.13s", seqnum, s->name().c_str());
                m_bgsequence = seqnum;
                ui->m_entry_sequence->setText(n);
                if (usr().global_seq_feature())
                    usr().seqedit_bgsequence(seqnum);

                seq_pointer()->background_sequence(seqnum, user_change);
                if (not_nullptr(m_seqroll))
                    m_seqroll->set_background_sequence(true, seqnum);

                set_dirty();
            }
        }
    }
    else
        msgprintf(msglevel::error, "null pattern %d", seqnum);
}

/**
 *  Sets the data type based on the given parameters.  This function uses the
 *  controller_name() function defined in the controllers.cpp module.
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
            int bus = int(seq_pointer()->seq_midi_bus());
            int channel = int(seq_pointer()->seq_midi_channel());
            std::string ccname = usr().controller_active(bus, channel, control) ?
                usr().controller_name(bus, channel, control) :
                controller_name(control) ;

            snprintf(type, sizeof type, "CC - %s", ccname.c_str());
        }
        else if (status == EVENT_PROGRAM_CHANGE)
            snprintf(type, sizeof type, "Program Change");
        else if (status == EVENT_CHANNEL_PRESSURE)
            snprintf(type, sizeof type, "Channel Pressure");
        else if (status == EVENT_PITCH_WHEEL)
            snprintf(type, sizeof type, "Pitch Wheel");
        else if (event::is_meta_status(status))
            snprintf(type, sizeof type, "Meta Events");
        else
            snprintf(type, sizeof type, "Unknown MIDI Event");

        char text[80];
        snprintf(text, sizeof text, "%s %s", hexa, type);
        ui->m_entry_data->setText(text);
    }
}

/**
 * Follow-progress callback.
 */

/**
 *  Passes the Follow status to the qseqroll object.  When qseqroll has been
 *  upgraded to support follow-progress, then enable this macro in
 *  libseq66/include/seq66_features.h.  Also applies to qperfroll.
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
        if (seq_pointer()->expanded_recording() && expand)
        {
            midipulse prog = seq_pointer()->progress_value();
            int newx = tix_to_pix(prog);
            hadjust->setValue(newx);
        }
        else                                        /* use for non-recording */
        {
            midipulse progtick = seq_pointer()->get_last_tick();
            int progx = tix_to_pix(progtick);
            int page = progx / w;
            int oldpage = m_seqroll->scroll_page();
            bool newpage = page != oldpage;
            if (newpage)
            {
                /*
                 * Here, we need to set some kind of flag for
                 * clearing the viewport before repainting.
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
        if (is_good_midibyte(midibyte(note)))
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
    if (index >= 0 && index < s_snap_count)
    {
        int qnfactor = perf().ppqn() * 4;
        int item = m_snap_list.ctoi(index);
        int v = qnfactor / item;
        set_snap(v);
    }
}

/**
 *  Selects the given snap value, which is the number of ticks in a snap-sized
 *  interval.  It is passed to the seqroll, seqevent, and sequence objects, as
 *  well.
 *
 *  The default initial snap is the default PPQN divided by 4, or the equivalent
 *  of a 16th note (48 ticks).  The snap divisor is 192 * 4 / 48 or 16.
 *
 * \param s
 *      The prospective snap value to set.  It is checked only to make sure it
 *      is greater than 0, to avoid a numeric exception.
 */

void
qseqeditframe64::set_snap (midipulse s)
{
    if (s > 0 && s != m_snap)
    {
        m_snap = int(s);
        if (not_nullptr(m_seqroll))
            m_seqroll->set_snap(s);

        seq_pointer()->snap(s);
        m_seqevent->set_snap(s);
    }
}

void
qseqeditframe64::reset_grid_snap ()
{
    ui->m_combo_snap->setCurrentIndex(4);
    update_draw_geometry();
}

/**
 *  Updates the note-length values and control based on the index.  It is passed
 *  to the set_note_length() function for processing.
 *
 * \param index
 *      Provides the index selected from the combo-box.
 */

void
qseqeditframe64::update_note_length (int index)
{
    if (index >= 0 && index < s_snap_count)
    {
        int qnfactor = perf().ppqn() * 4;
        int item = m_snap_list.ctoi(index);
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
    update_draw_geometry();
}

bool
qseqeditframe64::on_resolution_change (int ppqn, midibpm bpm)
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
    int z = s_zoom_items[index];
    (void) set_zoom(z);
    update_draw_geometry();
}

bool
qseqeditframe64::zoom_in ()
{
    int index = s_lookup_zoom(zoom());
    ui->m_combo_zoom->setCurrentIndex(index);
    return true;
}

bool
qseqeditframe64::zoom_out ()
{
    if (zoom() >= usr().max_zoom())
    {
        int v = s_zoom_items[0];        /* wrap around to beginning */
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
        int index = s_lookup_zoom(zoom());
        ui->m_combo_zoom->setCurrentIndex(index);
        update_draw_geometry();
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
    {
        set_key(index, true);                       /* always a user-change */
        set_dirty();
    }
}

void
qseqeditframe64::set_key (int key, bool user_change)
{
    sequence * s = seq_pointer().get();
    if (legal_key(key) && not_nullptr(s))
    {
        m_key = key;
        ui->m_combo_key->setCurrentIndex(key);
        if (not_nullptr(m_seqroll))
            m_seqroll->set_key(key);

        if (not_nullptr(m_seqkeys))
            m_seqkeys->set_key(key);

        if (usr().global_seq_feature())
            usr().seqedit_key(key);

        s->musical_key(midibyte(key), user_change);
        set_dirty();
    }
    else
        errprint("null pattern");
}

void
qseqeditframe64::reset_key ()
{
    set_key(c_key_of_C, true);                      /* always a user-change */
}

/**
 *  Handles updates to the scale selection.
 */

void
qseqeditframe64::update_scale (int index)
{
    set_scale(index, true);                         /* always a user-change */
}

/**
 *  For internal use only.
 */

void
qseqeditframe64::set_scale (int scale, bool user_change)
{
    sequence * s = seq_pointer().get();
    if (legal_scale(scale) && not_nullptr(s))
    {
        m_scale = scale;
        ui->m_combo_scale->setCurrentIndex(scale);
        if (not_nullptr(m_seqroll))
            m_seqroll->set_scale(scale);

        if (usr().global_seq_feature())
            usr().seqedit_scale(scale);

        s->musical_scale(midibyte(scale), user_change);
        if (not_nullptr(m_tools_harmonic))
            m_tools_harmonic->setEnabled(m_scale > 0);

        set_dirty();
    }
    else
        errprint("null pattern");
}

void
qseqeditframe64::reset_scale ()
{
    set_scale(c_scales_off, true);                  /* always a user-change */
}

void
qseqeditframe64::editor_mode (bool ischecked)
{
    set_editor_mode
    (
        ischecked ? sequence::editmode::drum : sequence::editmode::note
    );
    set_dirty();
}

void
qseqeditframe64::set_editor_mode (sequence::editmode mode)
{
    if (mode != m_edit_mode)
    {
        m_edit_mode = mode;
        perf().edit_mode(seq_pointer()->seq_number(), mode);
        if (not_nullptr(m_seqroll))
            m_seqroll->update_edit_mode(mode);

        set_dirty();
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
#if QT_VERSION < QT_VERSION_CHECK(5, 8, 0)
    QString mlabel(qt(text));
    QIcon micon(*create_menu_image(present));
    QAction * item = new QAction(micon, mlabel, nullptr);
#else
    QAction * item = new QAction(*create_menu_image(present), qt(text));
#endif
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
#if QT_VERSION < QT_VERSION_CHECK(5, 8, 0)
    QString mlabel(qt(text));
    QIcon micon(*create_menu_image(present));
    QAction * item = new QAction(micon, mlabel, nullptr);
#else
    QAction * item = new QAction(*create_menu_image(present), qt(text));
#endif
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
 *  Populates the event-selection menu that drops from the "Event" button
 *  in the bottom row of the Pattern editor.
 *
 *  This menu has a large number of items.  They are filled in by
 *  code, but can also be loaded from seq66.usr, qpseq66.usr, etc.
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
    midibyte status = 0, cc = 0;
    seq::pointer s = seq_pointer();
    memset(ccs, false, sizeof(bool) * c_midibyte_data_max);
    for (auto cev = s->cbegin(); ! s->cend(cev); ++cev)
    {
        if (! s->get_next_event(status, cc, cev))
            break;

        status = event::normalized_status(status);      /* mask off channel */
        switch (status)
        {
        case EVENT_NOTE_OFF:            note_off = true;            break;
        case EVENT_NOTE_ON:             note_on = true;             break;
        case EVENT_AFTERTOUCH:          aftertouch = true;          break;
        case EVENT_CONTROL_CHANGE:      ccs[cc] = true;             break;
        case EVENT_PITCH_WHEEL:         pitch_wheel = true;         break;
        case EVENT_PROGRAM_CHANGE:      program_change = true;      break;
        case EVENT_CHANNEL_PRESSURE:    channel_pressure = true;    break;
        }
    }

    /*
     * Currently the only meta event that can be edited is tempo.
     * The meta-match function keeps going until it finds the meta event or
     * it ends.  All we care here is if one exists.
     */

    auto cev = s->cbegin();
    if (! s->cend(cev))
    {
        if (s->get_next_meta_match(EVENT_META_SET_TEMPO, cev))
            tempo = true;
    }
    if (not_nullptr(m_events_popup))
        delete m_events_popup;

    m_events_popup = new QMenu(this);
    set_event_entry(m_events_popup, note_on, event_index::note_on);
    m_events_popup->addSeparator();
    set_event_entry(m_events_popup, note_off, event_index::note_off);
    set_event_entry(m_events_popup, aftertouch, event_index::aftertouch);
    set_event_entry(m_events_popup, program_change, event_index::prog_change);
    set_event_entry(m_events_popup, channel_pressure, event_index::chan_pressure);
    set_event_entry(m_events_popup, pitch_wheel, event_index::pitch_wheel);
    set_event_entry(m_events_popup, tempo, event_index::tempo);
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
             * Do we really want the default controller name to start?
             * There was a bug in Seq24 where the instrument number was use re
             * 1 to get the proper instrument... it needs to be decremented to
             * be re 0.
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
 *  button in the bottom row of the Pattern editor.
 *  This menu has a much smaller number of items, only the ones that actually
 *  exist in the track/pattern/loop/sequence.
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
    bool any_events = false;
    midibyte status = 0, cc = 0;
    seq::pointer s = seq_pointer();
    memset(ccs, false, sizeof(bool) * c_midibyte_data_max);
    for (auto cev = s->cbegin(); ! s->cend(cev); ++cev)
    {
        if (! s->get_next_event(status, cc, cev))
            break;

        status = event::normalized_status(status);      /* mask off channel */
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

        case EVENT_PITCH_WHEEL:
            any_events = pitch_wheel = true;
            break;

        case EVENT_PROGRAM_CHANGE:
            any_events = program_change = true;
            break;

        case EVENT_CHANNEL_PRESSURE:
            any_events = channel_pressure = true;
            break;
        }
    }

    auto cev = s->cbegin();
    if (! s->cend(cev))
    {
        if (s->get_next_meta_match(EVENT_META_SET_TEMPO, cev))
            tempo = any_events = true;
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

    if (program_change)
        set_event_entry(m_minidata_popup, true, event_index::prog_change);

    if (channel_pressure)
        set_event_entry(m_minidata_popup, true, event_index::chan_pressure);

    if (pitch_wheel)
        set_event_entry(m_minidata_popup, true, event_index::pitch_wheel);

    if (tempo)
        set_event_entry(m_minidata_popup, true, event_index::tempo);

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
         * Here, we would like to pre-select the first kind of event found,
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
        m_lfo_wnd = new qlfoframe(perf(), seq_pointer(), *m_seqdata);

    m_lfo_wnd->show();
}

void
qseqeditframe64::show_pattern_fix ()
{
    if (is_nullptr(m_patternfix_wnd))
    {
        m_patternfix_wnd = new qpatternfix
        (
            perf(), seq_pointer(), *m_seqdata, *m_seqevent
        );
        if (not_nullptr(m_patternfix_wnd))
            m_patternfix_wnd->show();
    }
}

/**
 *  Duplicative code.  See record_change(), thru_change(), q_record_change().
 */

void
qseqeditframe64::update_midi_buttons ()
{
    bool thru_active = seq_pointer()->thru();
    bool record_active = seq_pointer()->recording();
    bool qrecord_active = seq_pointer()->quantized_recording();
    bool playing = seq_pointer()->playing();
    ui->m_toggle_thru->setChecked(thru_active);
    ui->m_toggle_thru->setToolTip
    (
        thru_active ? "MIDI Thru Active" : "MIDI Thru Inactive"
    );
    ui->m_toggle_record->setChecked(record_active);
    ui->m_toggle_record->setToolTip
    (
        record_active ? "MIDI Record Active" : "MIDI Record Inactive"
    );
    ui->m_toggle_qrecord->setChecked(qrecord_active);
    ui->m_toggle_qrecord->setToolTip
    (
        qrecord_active ? "Quantized Record Active" : "Quantized Record Inactive"
    );
    ui->m_toggle_play->setChecked(playing);
    ui->m_toggle_play->setToolTip(playing ? "Armed" : "Muted");
}

/**
 *  Passes the play status to the sequence object.
 */

void
qseqeditframe64::play_change (bool ischecked)
{
    if (seq_pointer()->set_playing(ischecked))
        update_midi_buttons();
}

/**
 *  Passes the recording status to performer.
 */

void
qseqeditframe64::record_change (bool ischecked)
{
    if (perf().set_recording(seq_pointer(), ischecked, false))
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
    if (perf().set_quantized_recording(seq_pointer(), ischecked, false))
        update_midi_buttons();
}

/**
 *  Passes the MIDI Thru status to performer.
 */

void
qseqeditframe64::thru_change (bool ischecked)
{
    if (perf().set_thru(seq_pointer(), ischecked, false))
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
    bool ok = seq_pointer()->update_recording(index);
    enable_combobox_item(ui->m_combo_rec_type, lrreset, index == lroneshot);
    if (index == lrreset)                               /* oneshot reset    */
    {
        if (m_last_record_style == recordstyle::oneshot)
        {
            ui->m_combo_rec_type->setCurrentIndex(lroneshot);
        }
    }
    m_last_record_style = usr().grid_record_style(index);
    if (ok)
        set_dirty();
}

void
qseqeditframe64::update_recording_volume (int index)
{
    if (index >= 0 && index < s_rec_vol_count)
    {
        int recvol = s_rec_vol_items[index];
        set_recording_volume(recvol);
        set_dirty();
    }
}

void
qseqeditframe64::update_loop_count (int value)
{
    if (seq_pointer()->loop_count_max(value))
        set_dirty();
}

void
qseqeditframe64::reset_recording_volume ()
{
    ui->m_combo_rec_vol->setCurrentIndex(0);
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
    seq_pointer()->set_rec_vol(recvol); /* save to the sequence settings    */
    usr().velocity_override(recvol);    /* save to the "usr" config file    */
}

void
qseqeditframe64::set_dirty ()
{
    if (is_initialized())
    {
        qseqframe::set_dirty();         // roll, time, date & event panes
        m_seqroll->set_redraw();        // also calls set_dirty();
        m_seqdata->set_dirty();         // update(); neither cause a refresh
        m_seqevent->set_dirty();        // how about this?
        m_seqtime->set_dirty();
    }
    update_draw_geometry();
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
    seq_pointer()->handle_edit_action(action, var);
    set_dirty();
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


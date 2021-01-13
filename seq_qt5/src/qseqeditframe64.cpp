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
 * \file          qseqeditframe64.cpp
 *
 *  This module declares/defines the base class for plastering
 *  pattern/sequence data information in the data area of the pattern
 *  editor.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-06-15
 * \updates       2021-01-09
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
 *      Designate one of the interior scrolled widgets as the "master", probably
 *      the plot widget. Then do this:
 *
 *      Set every QScrollArea's horizontal scroll bar policy to never show the
 *      scroll bars.  (Set) the master QScrollArea's horizontalScrollBar()'s
 *      rangeChanged(int min, int max) signal to a slot that sets the main
 *      widget's horizontal QScrollBar to the same range. Additionally, it
 *      should set the same range for the other scroll area widget's horizontal
 *      scroll bars.
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
QVBoxLayout:     -----------------------------------------------------------
QWidget container?
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

#include "cfg/settings.hpp"             /* seq66::usr()                     */
#include "midi/controllers.hpp"         /* seq66::c_controller_names[]      */
#include "play/performer.hpp"           /* seq66::performer reference       */
#include "qlfoframe.hpp"
#include "qseqdata.hpp"
#include "qseqeditframe64.hpp"
#include "qseqkeys.hpp"
#include "qseqroll.hpp"
#include "qseqtime.hpp"
#include "qstriggereditor.hpp"
#include "qt5_helpers.hpp"              /* seq66::qt_set_icon()             */

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
#include "pixmaps/menu_full.xpm"
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
 *  try to shrink the seq-edit's piano roll to compensate.  Nope.
 */

/**
 *  Static data members.  These items apply to all of the instances of seqedit.
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
 *  interval is a 32nd note.  We would like to keep it at a 16th note.  We correct
 *  the snap ticks to the actual PPQN ratio.
 */

int qseqeditframe64::sm_initial_snap         = SEQ66_DEFAULT_PPQN / 4;
int qseqeditframe64::sm_initial_note_length  = SEQ66_DEFAULT_PPQN / 4;
int qseqeditframe64::sm_initial_chord        = 0;

/**
 * To reduce the amount of written code, we use a static array to
 * initialize the beat-width entries.
 */

static const int s_width_items [] = { 1, 2, 4, 8, 16, 32 };
static const int s_width_count = sizeof(s_width_items) / sizeof(int);

/**
 *  Looks up a beat-width value.
 */

static int
s_lookup_bw (int bw)
{
    int result = 0;
    for (int wi = 0; wi < s_width_count; ++wi)
    {
        if (s_width_items[wi] == bw)
        {
            result = wi;
            break;
        }
    }
    return result;
}

/**
 * To reduce the amount of written code, we use a static array to
 * initialize the measures entries.
 */

static const int s_measures_items [] =
{
    1, 2, 3, 4, 5, 6, 7, 8, 16, 32, 64, 128
};
static const int s_measures_count = sizeof(s_measures_items) / sizeof(int);

/**
 *  Looks up a beat-width value.
 */

static int
s_lookup_measures (int m)
{
    int result = 0;
    for (int wi = 0; wi < s_measures_count; ++wi)
    {
        if (s_measures_items[wi] == m)
        {
            result = wi;
            break;
        }
    }
    return result;
}

/**
 *  These static items are used to fill in and select the proper snap values for
 *  the grids.  Note that they are not members, though they could be.
 *  These values are also used for note length.  See update_grid_snap() and
 *  update_note_length().
 */

static const int s_snap_items [] =
{
    1, 2, 4, 8, 16, 32, 64, 128, 0, 3, 6, 12, 24, 48, 96, 192
};
static const int s_snap_count = sizeof(s_snap_items) / sizeof(int);

/**
 *  These static items are used to fill in and select the proper zoom values for
 *  the grids.  Note that they are not members, though they could be.
 *  Also note the features of these zoom numbers:
 *
 *      -#  The lowest zoom value is SEQ66_MINIMUM_ZOOM in app_limits.h.
 *      -#  The highest zoom value is SEQ66_MAXIMUM_ZOOM in app_limits.h.
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
    1, 2, 4, 8, 16, 32, 64, 128
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
 *  Looks up a chord name and returns its index.  Note that the chord names
 *  are defined in the scales.hpp file.
 *
 *  CURRENTLY NOT NEEDED.  Macroed out to avoid a warning.
 */

#if defined USE_S_LOOKUP_CHORD

static int
s_lookup_chord (const std::string & chordname)
{
    int result = 0;
    for (int chord = 0; chord < c_chord_number; ++chord)
    {
        if (c_chord_table_text[chord] == chordname)
        {
            result = chord;
            break;
        }
    }
    return result;
}

#endif  // USE_S_LOOKUP_CHORD

/**
 *  Hold the entries in the "Vel" drop-down.
 */

static const int s_rec_vol_items [] =
{
    SEQ66_PRESERVE_VELOCITY, 127, 112, 96, 80, 64, 48, 32, 16
};
static const int s_rec_vol_count = sizeof(s_rec_vol_items) / sizeof(int);

/**
 *
 * \param p
 *      Provides the performer object to use for interacting with this sequence.
 *      Among other things, this object provides the active PPQN.
 *
 * \param seqid
 *      Provides the sequence number.  The sequence pointer is looked up using
 *      this number.  This number is also the pattern-slot number for this
 *      sequence and for this window.  Ranges from 0 to 1024.
 *
 * \param parent
 *      Provides the parent window/widget for this container window.  Defaults
 *      to null.
 */

qseqeditframe64::qseqeditframe64 (performer & p, int seqid, QWidget * parent) :
    qseqframe               (p, seqid, parent),
    performer::callbacks    (p),
    ui                      (new Ui::qseqeditframe64),
    m_lfo_wnd               (nullptr),
    m_tools_popup           (nullptr),
    m_sequences_popup       (nullptr),
    m_events_popup          (nullptr),
    m_minidata_popup        (nullptr),
    m_beats_per_bar         (seq_pointer()->get_beats_per_bar()),
    m_beat_width            (seq_pointer()->get_beat_width()),
    m_snap                  (sm_initial_snap),
    m_note_length           (sm_initial_note_length),
    m_scale                 (usr().seqedit_scale()),
    m_chord                 (0),
    m_key                   (usr().seqedit_key()),
    m_bgsequence            (usr().seqedit_bgsequence()),
    m_measures              (0),                        /* fixed below      */
#if defined USE_STAZED_ODD_EVEN_SELECTION
    m_pp_whole              (0),
    m_pp_eighth             (0),
    m_pp_sixteenth          (0),
#endif
    m_editing_status        (0),
    m_editing_cc            (0),
    m_first_event           (0),
    m_first_event_name      ("(no events)"),
    m_have_focus            (false),
    m_edit_mode             (perf().edit_mode(seqid)),
    m_timer                 (nullptr)
{
    bussbyte buss = seq_pointer()->get_midi_bus();
    midibyte channel = seq_pointer()->get_midi_channel();
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);                 /* part of issue #4 */
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    initialize_panels();

    /*
     *  Sequence Number Label
     */

    QString labeltext;
    char tmp[32];
    snprintf(tmp, sizeof tmp, "#%d", seqid);
    labeltext = tmp;
    ui->m_label_seqnumber->setText(labeltext);

    /*
     * Sequence Title
     */

    ui->m_entry_name->setText(seq_pointer()->name().c_str());
    connect
    (
        ui->m_entry_name, SIGNAL(textChanged(const QString &)),
        this, SLOT(update_seq_name())
    );

    /*
     * Beats Per Bar.  Fill the options for the beats per measure combo-box,
     * and set the default.
     */

    qt_set_icon(down_xpm, ui->m_button_bpm);

#if defined SEQ66_QSEQEDIT_BUTTON_INCREMENT
    ui->m_button_bpm->setToolTip("Beats per bar. Increments to next value.");
    connect
    (
        ui->m_button_bpm, SIGNAL(clicked(bool)),
        this, SLOT(increment_beats_per_measure())
    );
#else
    connect
    (
        ui->m_button_bpm, SIGNAL(clicked(bool)),
        this, SLOT(reset_beats_per_measure())
    );
#endif

    int minimum = SEQ66_MINIMUM_BEATS_PER_MEASURE - 1;
    int maximum = SEQ66_MAXIMUM_BEATS_PER_MEASURE - 1;
    for (int b = minimum; b <= maximum; ++b)
    {
        QString combo_text = QString::number(b + 1);
        ui->m_combo_bpm->insertItem(b, combo_text);
    }

    int beatspm = seq_pointer()->get_beats_per_bar();
    std::string beatstring = std::to_string(beatspm);
    ui->m_combo_bpm->setCurrentIndex(m_beats_per_bar - 1);
    ui->m_combo_bpm->setEditText(QString::fromStdString(beatstring));
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
    set_beats_per_measure(seq_pointer()->get_beats_per_bar());

    /*
     * Beat Width (denominator of time signature).  Fill the options for
     * the beats per measure combo-box, and set the default.
     */

    qt_set_icon(down_xpm, ui->m_button_bw);

#if defined SEQ66_QSEQEDIT_BUTTON_INCREMENT
    ui->m_button_bw->setToolTip
    (
        "Beats width (denominator). Increments to next value."
    );
    connect
    (
        ui->m_button_bw, SIGNAL(clicked(bool)),
        this, SLOT(next_beat_width())
    );
#else
    connect
    (
        ui->m_button_bw, SIGNAL(clicked(bool)),
        this, SLOT(reset_beat_width())
    );
#endif

    for (int w = 0; w < s_width_count; ++w)
    {
        int item = s_width_items[w];
        char fmt[8];
        snprintf(fmt, sizeof fmt, "%d", item);
        QString combo_text = fmt;
        ui->m_combo_bw->insertItem(w, combo_text);
    }

    int bw_index = s_lookup_bw(m_beat_width);
    int bw = seq_pointer()->get_beat_width();
    std::string bstring = std::to_string(bw);
    ui->m_combo_bw->setCurrentIndex(bw_index);
    ui->m_combo_bw->setEditText(QString::fromStdString(bstring));
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
    set_beat_width(seq_pointer()->get_beat_width());

    /*
     * Pattern Length in Measures. Fill the options for
     * the beats per measure combo-box, and set the default.
     */

    qt_set_icon(length_short_xpm, ui->m_button_length);

#if defined SEQ66_QSEQEDIT_BUTTON_INCREMENT
    ui->m_button_length->setToolTip
    (
        "Pattern length (bars). Increments to next value."
    );
    connect
    (
        ui->m_button_length, SIGNAL(clicked(bool)),
        this, SLOT(next_measures())
    );
#else
    connect
    (
        ui->m_button_length, SIGNAL(clicked(bool)),
        this, SLOT(reset_measures())
    );
#endif

    for (int m = 0; m < s_measures_count; ++m)
    {
        int item = s_measures_items[m];
        char fmt[8];
        snprintf(fmt, sizeof fmt, "%d", item);
        QString combo_text = fmt;
        ui->m_combo_length->insertItem(m, combo_text);
    }

    int len_index = s_lookup_measures(m_measures);
    int measures = seq_pointer()->calculate_measures();
    std::string mstring = std::to_string(measures);
    ui->m_combo_length->setCurrentIndex(len_index);
    ui->m_combo_length->setEditText(QString::fromStdString(mstring));
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
    seq_pointer()->calculate_unit_measure(); /* must precede set_measures() */
    set_measures(get_measures());

    /*
     *  Transpose button.
     */

    bool cantranspose = seq_pointer()->transposable();
    qt_set_icon(transpose_xpm, ui->m_toggle_transpose);
    ui->m_toggle_transpose->setCheckable(true);
    ui->m_toggle_transpose->setChecked(cantranspose);

    /*
     * Qt::NoFocus is the default focus policy.
     */

    ui->m_toggle_transpose->setAutoDefault(false);
    connect
    (
        ui->m_toggle_transpose, SIGNAL(toggled(bool)),
        this, SLOT(transpose(bool))
    );
    if (! usr().work_around_transpose_image())
        set_transpose_image(cantranspose);

    /*
     * Chord button and combox-box.  See c_chord_table_text[c_chord_number][]
     * in the scales.hpp header file.
     */

#if defined SEQ66_QSEQEDIT_BUTTON_INCREMENT
    qt_set_icon(chord3_inv_xpm, ui->m_button_chord);
    ui->m_button_chord->setToolTip
    (
        "Chord generation. Increments to next value."
    );
    connect
    (
        ui->m_button_chord, SIGNAL(clicked(bool)),
        this, SLOT(increment_chord())
    );
#else
    qt_set_icon(chord3_inv_xpm, ui->m_button_chord);
    connect
    (
        ui->m_button_chord, SIGNAL(clicked(bool)),
        this, SLOT(reset_chord())
    );
#endif

    for (int chord = 0; chord < c_chord_number; ++chord)
    {
        QString combo_text = c_chord_table_text[chord].c_str();
        ui->m_combo_chord->insertItem(chord, combo_text);
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
        bussbyte seqbuss = seq_pointer()->get_midi_bus();
        std::string selectedbuss = opm.active() ?
            opm.get_name(seqbuss) : mmb->get_midi_out_bus_name(seqbuss) ;

        int buses = opm.active() ?
            opm.count() : mmb->get_num_out_buses() ;

        for (int bus = 0; bus < buses; ++bus)
        {
            e_clock ec;
            std::string busname;
            if (perf().ui_get_clock(bussbyte(bus), ec, busname))
            {
                bool disabled = ec == e_clock::disabled;
                ui->m_combo_bus->addItem(QString::fromStdString(busname));
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
        ui->m_combo_bus->setCurrentText(QString::fromStdString(selectedbuss));
    }
    connect
    (
        ui->m_combo_bus, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_midi_bus(int))
    );
    set_midi_bus(buss);

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
    repopulate_midich_combo(buss);

    /*
     * Undo and Redo Buttons.
     */

    qt_set_icon(undo_xpm, ui->m_button_undo);
    connect(ui->m_button_undo, SIGNAL(clicked(bool)), this, SLOT(undo()));

    qt_set_icon(redo_xpm, ui->m_button_redo);
    connect(ui->m_button_redo, SIGNAL(clicked(bool)), this, SLOT(redo()));

    /*
     * Quantize Button.  This is the "Q" button, and indicates to
     * quantize (just?) notes.  Compare it to the Quantize menu entry,
     * which quantizes events.  Note the usage of std::bind()... this feature
     * requires C++11. Also see q_record_change(), which handles on-the-fly
     * quantization while recording.
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
     * Tools Pop-up Menu Button.
     */

    qt_set_icon(tools_xpm, ui->m_button_tools);
    connect(ui->m_button_tools, SIGNAL(clicked(bool)), this, SLOT(tools()));
    popup_tool_menu();

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
     *  Fill "Snap" and "Note" Combo Boxes:
     *
     *      To reduce the amount of written code, we now use a static array to
     *      initialize some of these menu entries.  0 denotes the separator.
     *      This same setup is used to set up both the snap and note menu, since
     *      they are exactly the same.  Saves a *lot* of code.  This code was
     *      copped from the Gtkmm 2.4 seqedit class and adapted to Qt 5.
     */

    for (int si = 0; si < s_snap_count; ++si)
    {
        int item = s_snap_items[si];
        char fmt[16];
        if (item > 1)
            snprintf(fmt, sizeof fmt, "1/%d", item);
        else
            snprintf(fmt, sizeof fmt, "%d", item);

        QString combo_text = fmt;
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
    connect
    (
        ui->m_combo_snap, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_grid_snap(int))
    );
    ui->m_combo_note->setCurrentIndex(4);               /* ditto            */
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

    set_snap(sm_initial_snap * perf().ppqn() / SEQ66_DEFAULT_PPQN);
    qt_set_icon(note_length_xpm, ui->m_button_note);
    connect
    (
        ui->m_button_note, SIGNAL(clicked(bool)),
        this, SLOT(reset_note_length())
    );
    set_note_length(sm_initial_note_length * perf().ppqn() / SEQ66_DEFAULT_PPQN);

    /*
     *  Zoom In and Zoom Out:  Rather than two buttons, we use one and
     *  a combo-box.
     */

    qt_set_icon(zoom_xpm, ui->m_button_zoom);

#if defined SEQ66_QSEQEDIT_BUTTON_INCREMENT
    ui->m_button_zoom->setToolTip("Next zoom level. Wraps around.");
    connect
    (
        ui->m_button_zoom, SIGNAL(clicked(bool)),
        this, SLOT(zoom_out())
    );
#else
    connect
    (
        ui->m_button_zoom, SIGNAL(clicked(bool)),
        this, SLOT(slot_reset_zoom())
    );
#endif

    for (int zi = 0; zi < s_zoom_count; ++zi)
    {
        int zoom = s_zoom_items[zi];
        if (zoom >= usr().min_zoom() && zoom <= usr().max_zoom())
        {
            char fmt[16];
            snprintf(fmt, sizeof fmt, "1:%d", zoom);

            QString combo_text = fmt;
            ui->m_combo_zoom->insertItem(zi, combo_text);
        }
    }
    ui->m_combo_zoom->setCurrentIndex(1);
    connect
    (
        ui->m_combo_zoom, SIGNAL(currentIndexChanged(int)),
        this, SLOT(slot_update_zoom(int))
    );

    int zoom = usr().zoom();
    if (usr().zoom() == SEQ66_USE_ZOOM_POWER_OF_2)      /* i.e. 0 */
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
        QString combo_text = musical_key_name(key).c_str();
        ui->m_combo_key->insertItem(key, combo_text);
    }
    ui->m_combo_key->setCurrentIndex(m_key);
    connect
    (
        ui->m_combo_key, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_key(int))
    );
    if (seq_pointer()->musical_key() != c_key_of_C)
        set_key(seq_pointer()->musical_key());
    else
        set_key(m_key);

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
        QString combo_text = musical_scale_name(scale).c_str();
        ui->m_combo_scale->insertItem(scale, combo_text);
    }
    ui->m_combo_scale->setCurrentIndex(m_scale);
    connect
    (
        ui->m_combo_scale, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_scale(int))
    );
    if (seq_pointer()->musical_scale() != c_scales_off)
        set_scale(seq_pointer()->musical_scale());
    else
        set_scale(m_scale);

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

    if (seq::valid(seq_pointer()->background_sequence()))
        m_bgsequence = seq_pointer()->background_sequence();

    set_background_sequence(m_bgsequence);

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
     * Drum-Mode Button.  Qt::NoFocus is the default focus policy.
     */

    ui->m_toggle_drum->setAutoDefault(false);
    ui->m_toggle_drum->setCheckable(true);
    connect
    (
        ui->m_toggle_drum, SIGNAL(toggled(bool)),
        this, SLOT(editor_mode(bool))
    );
    qt_set_icon(drum_xpm, ui->m_toggle_drum);

    /*
     * Event Selection Button and Popup Menu for qseqdata.
     */

    connect
    (
        ui->m_button_event, SIGNAL(clicked(bool)),
        this, SLOT(events())
    );

    repopulate_event_menu(buss, channel);
    set_data_type(EVENT_NOTE_ON);

    /*
     * Event Data Presence-Indicator Button and Popup Menu.
     */

    connect
    (
        ui->m_button_data, SIGNAL(clicked(bool)),
        this, SLOT(data())
    );
    repopulate_mini_event_menu(buss, channel);

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
     * Enable (unmute) Play Button.
     */

    qt_set_icon(play_xpm, ui->m_toggle_play);
    ui->m_toggle_play->setCheckable(true);
    connect
    (
        ui->m_toggle_play, SIGNAL(toggled(bool)),
        this, SLOT(play_change(bool))
    );
    if (seq_pointer()->is_new_pattern())
        play_change(usr().new_pattern_armed());

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
    if (seq_pointer()->is_new_pattern())
        thru_change(usr().new_pattern_thru());

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
    if (seq_pointer()->is_new_pattern())
        record_change(usr().new_pattern_record());

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
    if (seq_pointer()->is_new_pattern())
        q_record_change(usr().new_pattern_qrecord());

    /*
     * Recording Merge, Replace, Extend Button.  Provides a button to set the
     * recording style to "merge" (when looping, merge new incoming events
     * into the pattern), "overwrite" (replace events with incoming events),
     * and "expand" (increase the size of the loop to accomodate new events).
     */

    int lrmerge = sequence::loop_record(recordstyle::merge);
    int lrreplace = sequence::loop_record(recordstyle::overwrite);
    int lrexpand = sequence::loop_record(recordstyle::expand);
    ui->m_combo_rec_type->insertItem(lrmerge, "Merge");
    ui->m_combo_rec_type->insertItem(lrreplace, "Overwrite");
    ui->m_combo_rec_type->insertItem(lrexpand, "Expand");
    connect
    (
        ui->m_combo_rec_type, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_record_type(int))
    );
    if (seq_pointer()->is_new_pattern())
        lrmerge = usr().new_pattern_recordcode();

    ui->m_combo_rec_type->setCurrentIndex(lrmerge);

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
        char fmt[8];
        if (v == 0)
            snprintf(fmt, sizeof fmt, "%s", "Free");
        else
            snprintf(fmt, sizeof fmt, "%d", item);

        QString combo_text = fmt;
        ui->m_combo_rec_vol->insertItem(v, combo_text);
    }
    connect
    (
        ui->m_combo_rec_vol, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_recording_volume(int))
    );
    set_recording_volume(usr().velocity_override());

#if defined USE_REMAP_NOTES_BUTTON

    /*
     * Replaced by note-entry button
     */

    connect
    (
        ui->btnAuxFunction, SIGNAL(clicked(bool)),
        this, SLOT(remap_notes())
    );

#endif

    int seqwidth = m_seqroll->width();
    int scrollwidth = ui->rollScrollArea->width();
    m_seqroll->progress_follow(seqwidth > scrollwidth);
    ui->m_toggle_follow->setChecked(m_seqroll->progress_follow());

    update_midi_buttons();
    set_initialized();
    cb_perf().enregister(this);                             /* notification */
    m_timer = new QTimer(this);                             /* redraw timer */
    m_timer->setInterval(2 * usr().window_redraw_rate());   /* 20           */
    QObject::connect
    (
        m_timer, SIGNAL(timeout()),
        this, SLOT(conditional_update())
    );
    m_timer->start();
}

/**
 *  \dtor
 */

qseqeditframe64::~qseqeditframe64 ()
{
    m_timer->stop();
    if (not_nullptr(m_lfo_wnd))
        delete m_lfo_wnd;

    cb_perf().unregister(this);
    delete ui;
}

/**
 *  Odd, when this window has focus, this function is called roughly every 1/2
 *  second!  The type of event is always qpep->type() == 12.
 */

void
qseqeditframe64::paintEvent (QPaintEvent * qpep)
{

#if defined SEQ66_PLATFORM_DEBUG_TMI
    static int s_count = 0;
    QRect r = qpep->rect();
    printf
    (
        "qseqeditframe64::paintEvent(%d) at (x,y,w,h) = (%d, %d, %d, %d)\n",
        s_count++, r.x(), r.y(), r.width(), r.height()
    );
#endif

    qpep->ignore();                         /* QFrame::paintEvent(qpep)     */
}

/**
 *  Here, we will need to recreate the current viewport, if not the whole damn
 *  seqroll.
 */

void
qseqeditframe64::resizeEvent (QResizeEvent * qrep)
{

#if defined SEQ66_PLATFORM_DEBUG_TMI
    static int s_count = 0;
    printf("qseqeditframe64::resizeEvent(%d)\n", s_count++);
#endif

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

#if defined SEQ66_PLATFORM_DEBUG_TMI
    static int s_count = 0;
    printf("qseqeditframe64::wheelEvent(%d)\n", s_count++);
#endif

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
    if (perf().is_pattern_playing())
    {
        if (event->key() == Qt::Key_Space)
            stop_playing();
        else if (event->key() == Qt::Key_Period)
            pause_playing();
    }
    else
    {
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Period)
        {
            start_playing();
        }
        else if (event->key() == Qt::Key_A)
        {
            if ((event->modifiers() & Qt::ControlModifier) == 0)
                analyze_seq_notes();
        }
    }
}

void
qseqeditframe64::keyReleaseEvent (QKeyEvent *)
{
    // no code
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
 *  still eat up space.  They needed to be hidden.
 *
 *  Although tricky, the creator of this object must call this function after
 *  the creation, just to avoid transgressing the rule about calling virtual
 *  functions in the constructor.
 *
 *  Note that m_seqkeys is a protected member of the qseqframe base class.
 */

void
qseqeditframe64::initialize_panels ()
{
    int noteheight = usr().key_height();
    int height = noteheight * c_num_keys + 1;
    m_seqkeys = new qseqkeys
    (
        perf(), seq_pointer(), ui->keysScrollArea, noteheight, height
    );
    ui->keysScrollArea->setWidget(m_seqkeys);
    ui->keysScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->keysScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->keysScrollArea->verticalScrollBar()->setRange(0, height);
    m_seqtime = new qseqtime
    (
        perf(), seq_pointer(), zoom(), ui->timeScrollArea
    );
    ui->timeScrollArea->setWidget(m_seqtime);
    ui->timeScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->timeScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    /*
     * qseqroll.  Note the last parameter, "this" is not really a Qt parent
     * parameter.  It simply gives qseqroll access to the qseqeditframe64 ::
     * follow_progress() function.
     */

    m_seqroll = new qseqroll
    (
        perf(), seq_pointer(), m_seqkeys, zoom(), m_snap,
        sequence::editmode::note, this                    /* see note above   */
    );

    ui->rollScrollArea->setWidget(m_seqroll);
    ui->rollScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->rollScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_seqroll->update_edit_mode(m_edit_mode);
    m_seqdata = new qseqdata
    (
        perf(), seq_pointer(), zoom(), m_snap, ui->dataScrollArea   // , 1
    );
    ui->dataScrollArea->setWidget(m_seqdata);
    ui->dataScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->dataScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_seqevent = new qstriggereditor
    (
        perf(), seq_pointer(), zoom(), m_snap,
        noteheight, ui->eventScrollArea
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

/*
 * Play the SLOTS!
 */

/**
 *  We need to set the dirty state while the sequence has been changed.
 */

void
qseqeditframe64::conditional_update ()
{
    bool expandrec = seq_pointer()->expand_recording();
    update_midi_buttons();                      /* mirror current states    */
    if (expandrec)
    {
        set_measures(get_measures() + 1);
        follow_progress(expandrec);             /* keep up with progress    */
    }
    else if (not_nullptr(m_seqroll) && m_seqroll->progress_follow())
    {
        follow_progress();
    }
    if (seq_pointer()->check_loop_reset())
    {
        /*
         * Now we need to update the event and data panes.  Note that the notes
         * update during the next pass through the loop only if more notes come
         * in on the input buss.
         */

        set_dirty();
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
    if
    (
        index != m_beats_per_bar &&
        index >= SEQ66_MINIMUM_BEATS_PER_MEASURE &&
        index <= SEQ66_MAXIMUM_BEATS_PER_MEASURE
    )
    {
        set_beats_per_measure(index);
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
        if
        (
            beats >= SEQ66_MINIMUM_BEATS_PER_MEASURE &&
            beats <= SEQ66_MAXIMUM_BEATS_PER_MEASURE
        )
        {
            set_beats_per_measure(beats);
        }
        else
        {
            reset_beats_per_measure();
        }
    }
}

#if defined SEQ66_QSEQEDIT_BUTTON_INCREMENT

/**
 *  When the BPM (beats-per-measure) button is pushed, we go to the next BPM
 *  entry in the combo-box, wrapping around when the end is reached.
 */

void
qseqeditframe64::increment_beats_per_measure ()
{
    int bpm = m_beats_per_bar + 1;
    if (bpm > SEQ66_MAXIMUM_BEATS_PER_MEASURE)
        bpm = SEQ66_MINIMUM_BEATS_PER_MEASURE;

    ui->m_combo_bpm->setCurrentIndex(bpm - 1);
    set_beats_per_measure(bpm);
}

#else

void
qseqeditframe64::reset_beats_per_measure ()
{
    seq::number seqno = seq_pointer()->seq_number();
    ui->m_combo_bpm->setCurrentIndex(SEQ66_DEFAULT_BEATS_PER_MEASURE - 1);

    /*
     * TODO:  work this out better.
     */

    perf().notify_sequence_change(seqno, performer::change::recreate);
    update_draw_geometry();     // set_dirty();
}

#endif  // defined SEQ66_QSEQEDIT_BUTTON_INCREMENT

/**
 *  Applies the new beats/bar (beats/measure) value to the sequence and the user
 *  interface.
 *
 * \param bpm
 *      The desired beats/measure value.
 */

void
qseqeditframe64::set_beats_per_measure (int bpm)
{
    int measures = get_measures();
    seq_pointer()->set_beats_per_bar(bpm);
    m_beats_per_bar = bpm;
    seq_pointer()->apply_length
    (
        bpm, perf().ppqn(), seq_pointer()->get_beat_width(), measures
    );
    update_draw_geometry();     // set_dirty();
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
    seq_pointer()->apply_length
    (
        seq_pointer()->get_beats_per_bar(), perf().ppqn(),
        seq_pointer()->get_beat_width(), len
    );
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
    int bw = s_width_items[index];
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
        if
        (
            width >= SEQ66_MINIMUM_BEAT_WIDTH &&
            width <= SEQ66_MAXIMUM_BEAT_WIDTH
        )
        {
            set_beat_width(width);
        }
        else
        {
            reset_beat_width();
        }
    }
}

#if defined SEQ66_QSEQEDIT_BUTTON_INCREMENT

/**
 *  When the BW (beat width) button is pushed, we go to the next beat width
 *  entry in the combo-box, wrapping around when the end is reached.
 */

void
qseqeditframe64::next_beat_width ()
{
    int index = s_lookup_bw(m_beat_width);
    if (++index >= s_width_count)
        index = 0;

    ui->m_combo_bw->setCurrentIndex(index);
    int bw = s_width_items[index];
    if (bw != m_beat_width)
        set_beat_width(bw);
}

#else

/**
 *  Resets the beat-width combo-box to its default value.
 */

void
qseqeditframe64::reset_beat_width ()
{
    ui->m_combo_bw->setCurrentIndex(2);     /* i.e. 4, see s_width_items    */
    update_draw_geometry();
}

#endif  // defined SEQ66_QSEQEDIT_BUTTON_INCREMENT

/**
 *  Sets the beat-width value and then dirties the user-interface so that it
 *  will be repainted.
 */

void
qseqeditframe64::set_beat_width (int bw)
{
    int measures = get_measures();
    seq_pointer()->set_beat_width(bw);
    seq_pointer()->apply_length
    (
        seq_pointer()->get_beats_per_bar(), perf().ppqn(), bw, measures
    );
    m_beat_width = bw;
    update_draw_geometry();     // set_dirty();
}

/**
 *  Handles updates to the pattern length.
 */

void
qseqeditframe64::update_measures (int index)
{
    int m = s_measures_items[index];
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
        if
        (
            measures >= SEQ66_MINIMUM_MEASURES &&
            measures <= SEQ66_MAXIMUM_MEASURES
        )
        {
            set_measures(measures);
        }
    }
}

/**
 *  When the measures-length button is pushed, we go to the next length
 *  entry in the combo-box, wrapping around when the end is reached.
 */

void
qseqeditframe64::next_measures ()
{
    int index = s_lookup_measures(m_measures);
    if (++index >= s_measures_count)
        index = 0;

    ui->m_combo_length->setCurrentIndex(index);
    int m = s_measures_items[index];
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
    if (! usr().work_around_transpose_image())
        set_transpose_image(ischecked);
}

/**
 *  Changes the image used for the transpose button.  Actually, we have two
 *  drum icons, so we won't change this one.
 *
 * \param istransposable
 *      If true, set the image to the "Transpose" icon.  Otherwise, set it to
 *      the "Drum" (not transposable) icon.
 */

void
qseqeditframe64::set_transpose_image (bool istransposable)
{
    if (istransposable)
    {
        ui->m_toggle_transpose->setToolTip("Sequence is transposable.");
//      qt_set_icon(transpose_xpm, ui->m_toggle_transpose);
    }
    else
    {
        ui->m_toggle_transpose->setToolTip("Sequence is not transposable.");
//      qt_set_icon(drum_xpm, ui->m_toggle_transpose);
    }
}

/**
 *  Handles updates to the beats/measure for only the current sequences.
 *  See the similar function in qsmainwnd.
 */

void
qseqeditframe64::update_chord (int index)
{
    if (index != m_chord && index >= 0 && index < c_chord_number)
    {
        set_chord(index);
        set_dirty();
    }
}

#if defined SEQ66_QSEQEDIT_BUTTON_INCREMENT

/**
 *  When the chord button is pushed, we can go to the next chord
 *  entry in the combo-box, wrapping around when the end is reached.
 *  Currently, though, we just reset to the default chord.
 */

void
qseqeditframe64::increment_chord ()
{
    int chord = m_chord + 1;
    if (chord >= c_chord_number)
        chord = 0;

    set_chord(chord);
}

#else   // SEQ66_QSEQEDIT_BUTTON_INCREMENT

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

#endif  // SEQ66_QSEQEDIT_BUTTON_INCREMENT

void
qseqeditframe64::set_chord (int chord)
{
    if (chord >= 0 && chord < c_chord_number)
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
    seq_pointer()->set_midi_bus(bussbyte(index), true);
    set_dirty();
}

/**
 *  Resets the MIDI bus value to its default.
 */

void
qseqeditframe64::reset_midi_bus ()
{
    ui->m_combo_bus->setCurrentIndex(0);        // update_midi_bus(0)
    update_draw_geometry();
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
 *      channel and event menus (but not the mini-event menu!?) are
 *      repopulated to reflect the new bus.  This parameter is false in the
 *      constructor because those items have not been set up at that time.
 */

void
qseqeditframe64::set_midi_bus (int bus, bool user_change)
{
    bussbyte initialbus = seq_pointer()->get_midi_bus();
    bussbyte b = bussbyte(bus);
    seq_pointer()->set_midi_bus(b, user_change);    /* user-modified value? */
    ui->m_combo_bus->setCurrentIndex(bus);
    if (bus != int(initialbus) && user_change)
    {
        int channel = int(seq_pointer()->get_midi_channel());
        repopulate_midich_combo(bus);
        repopulate_event_menu(bus, channel);
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
        char b[4];                                  /* 2 digits or less  */
        snprintf(b, sizeof b, "%2d", channel + 1);
        std::string name = std::string(b);
        std::string s = usr().instrument_name(buss, channel);
        if (! s.empty())
        {
            name += " ";
            name += s;
        }
        if (channel == c_midichannel_max)
        {
            QString combo_text("Any");
            ui->m_combo_channel->insertItem(c_midichannel_max, combo_text);
        }
        else
        {
            QString combo_text(name.c_str());
            ui->m_combo_channel->insertItem(channel, combo_text);
        }
    }
    midibyte chindex = seq_pointer()->get_midi_channel();
    if (is_null_channel(chindex))
        chindex = c_midichannel_max;

    connect
    (
        ui->m_combo_channel, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_midi_channel(int))
    );
    ui->m_combo_channel->setCurrentIndex(chindex);
    set_midi_channel(seq_pointer()->get_midi_channel());
}

/**
 *  Note that c_midichannel_max is a legal value, too.  It is remapped in
 *  sequence::set_midi_channel().
 */

void
qseqeditframe64::update_midi_channel (int index)
{
    if (index >= 0 && index <= c_midichannel_max)
    {
        seq_pointer()->set_midi_channel(index);     /* also handles "Any"   */
        set_dirty();
    }
}

void
qseqeditframe64::reset_midi_channel ()
{
    ui->m_combo_channel->setCurrentIndex(0);    // update_midi_channel(0)
    update_draw_geometry();
}

/**
 *  Selects the given MIDI channel parameter in the main sequence object,
 *  so that it will use that channel.
 *
 *  Should this change set the is-modified flag?  Where should validation
 *  occur?
 *
 * \param midichannel
 *      The MIDI channel  value to set.
 *
 * \param user_change
 *      True if the user made this change, and thus has potentially modified
 *      the song.
 */

void
qseqeditframe64::set_midi_channel (int midichannel, bool user_change)
{
    int initialchan = seq_pointer()->get_midi_channel();
    int chindex = is_null_channel(midichannel) ?
        c_midichannel_max : midichannel ;

    ui->m_combo_channel->setCurrentIndex(chindex);
    seq_pointer()->set_midi_channel(midichannel, user_change);
    if (midichannel != initialchan && user_change)
    {
        int initialbus = int(seq_pointer()->get_midi_bus());
        repopulate_event_menu(initialbus, midichannel);
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
    QMenu * menupitch  = new QMenu(tr("&Pitch..."), m_tools_popup);
    QAction * selectall = new QAction(tr("Select all"), m_tools_popup);
    selectall->setShortcut(tr("Ctrl+A"));
    connect
    (
        selectall, SIGNAL(triggered(bool)),
        this, SLOT(select_all_notes())
    );
    menuselect->addAction(selectall);

    QAction * selectinverse = new QAction(tr("Inverse selection"), m_tools_popup);
    selectinverse->setShortcut(tr("Ctrl+Shift+I"));
    connect
    (
        selectinverse, SIGNAL(triggered(bool)),
        this, SLOT(inverse_note_selection())
    );
    menuselect->addAction(selectinverse);

    QAction * quantize = new QAction(tr("Quantize"), m_tools_popup);
    quantize->setShortcut(tr("Ctrl+Q"));
    connect(quantize, SIGNAL(triggered(bool)), this, SLOT(quantize_notes()));
    menutiming->addAction(quantize);

    QAction * tighten = new QAction(tr("Tighten"), m_tools_popup);
    tighten->setShortcut(tr("Ctrl+T"));
    connect(tighten, SIGNAL(triggered(bool)), this, SLOT(tighten_notes()));
    menutiming->addAction(tighten);

    char num[16];
    QAction * transpose[24];     /* fill out note transpositions */
    for (int t = -12; t <= 12; ++t)
    {
        if (t != 0)
        {
            snprintf
            (
                num, sizeof num, "%+d [%s]", t, c_interval_text[abs(t)].c_str()
            );
            transpose[t + 12] = new QAction(num, m_tools_popup);
            transpose[t + 12]->setData(t);
            menupitch->addAction(transpose[t + 12]);
            connect
            (
                transpose[t + 12], SIGNAL(triggered(bool)),
                this, SLOT(transpose_notes())
            );
        }
        else
            menupitch->addSeparator();
    }
    m_tools_popup->addMenu(menuselect);
    m_tools_popup->addMenu(menutiming);
    m_tools_popup->addMenu(menupitch);
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
 *  Consider adding Aftertouch events.
 */

void
qseqeditframe64::transpose_notes ()
{
    QAction * senderAction = (QAction *) sender();
    int transposeval = senderAction->data().toInt();
    seq_pointer()->transpose_notes(transposeval, 0);
}

/**
 *  Popup menu sequences button.
 */

void
qseqeditframe64::sequences ()
{
    if (not_nullptr(m_sequences_popup))
    {
        m_sequences_popup->exec
        (
            ui->m_button_sequence->mapToGlobal
            (
                QPoint
                (
                    ui->m_button_sequence->width() - 2,
                    ui->m_button_sequence->height() - 2
                )
            )
        );
    }
}

/**
 *  A case where a macro makes the code easier to read.
 */

#define SET_BG_SEQ(seq) \
    std::bind(&qseqeditframe64::set_background_sequence, this, seq)


/**
 *  Builds the Tools popup menu on the fly.  Analogous to seqedit ::
 *  popup_sequence_menu().
 */

void
qseqeditframe64::popup_sequence_menu ()
{
    if (is_nullptr(m_sequences_popup))
    {
        m_sequences_popup = new QMenu(this);
    }

    QAction * off = new QAction(tr("Off"), m_sequences_popup);
    connect(off, &QAction::triggered, SET_BG_SEQ(seq::limit()));
    (void) m_sequences_popup->addAction(off);
    (void) m_sequences_popup->addSeparator();
    int seqsinset = perf().seqs_in_set();
    int maxset = perf().screenset_max();
    for (int sset = 0; sset < maxset; ++sset)
    {
        QMenu * menusset = nullptr;
        if (perf().is_screenset_active(sset))
        {
            char number[16];
            snprintf(number, sizeof number, "Set %d", sset);
            menusset = m_sequences_popup->addMenu(number);
        }
        for (int seq = 0; seq < seqsinset; ++seq)
        {
            char name[32];
            int s = sset * seqsinset + seq;
            seq::pointer sp = perf().get_sequence(s);
            if (not_nullptr(sp))
            {
                snprintf(name, sizeof name, "[%d] %.13s", s, sp->name().c_str());

                QAction * item = new QAction(tr(name), menusset);
                menusset->addAction(item);
                connect(item, &QAction::triggered, SET_BG_SEQ(s));
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
qseqeditframe64::set_background_sequence (int seqnum)
{
    m_bgsequence = seqnum;                      /* should check this value!  */
    if (usr().global_seq_feature())
        usr().seqedit_bgsequence(seqnum);

    if (seq::disabled(seqnum) || ! perf().is_seq_active(seqnum))
    {
        ui->m_entry_sequence->setText("Off");
        if (not_nullptr(m_seqroll))
            m_seqroll->set_background_sequence(false, seq::limit());
    }

    seq::pointer s = perf().get_sequence(seqnum);
    if (not_nullptr(s))
    {
        char name[24];
        snprintf(name, sizeof name, "[%d] %.13s", seqnum, s->name().c_str());
        ui->m_entry_sequence->setText(name);
        if (not_nullptr(m_seqroll))
            m_seqroll->set_background_sequence(true, seqnum);

        if (seqnum < usr().max_sequence())      /* even more restrictive */
            seq_pointer()->background_sequence(seqnum);
    }
}

/**
 *  Sets the data type based on the given parameters.  This function uses the
 *  hardwired array c_controller_names defined in the controllers.cpp module.
 *
 * \param status
 *      The current editing status.
 *
 * \param control
 *      The control value.  However, we really need to validate it!
 */

void
qseqeditframe64::set_data_type (midibyte status, midibyte control)
{
    char hex[8];
    char type[32];
    snprintf(hex, sizeof hex, "[0x%02X]", status);
    m_editing_status = status;                      /* not yet used, though */
    m_editing_cc = control;                         /* not yet used, though */
    m_seqevent->set_data_type(status, control);     /* qstriggereditor      */
    m_seqdata->set_data_type(status, control);
    if (status == EVENT_NOTE_OFF)
        snprintf(type, sizeof type, "Note Off");
    else if (status == EVENT_NOTE_ON)
        snprintf(type, sizeof type, "Note On");
    else if (status == EVENT_AFTERTOUCH)
        snprintf(type, sizeof type, "Aftertouch");
    else if (status == EVENT_CONTROL_CHANGE)
    {
        int bus = int(seq_pointer()->get_midi_bus());
        int channel = int(seq_pointer()->get_midi_channel());
        std::string ccname(c_controller_names[control]);
        if (usr().controller_active(bus, channel, control))
            ccname = usr().controller_name(bus, channel, control);

        snprintf(type, sizeof type, "CC - %s", ccname.c_str());
    }
    else if (status == EVENT_PROGRAM_CHANGE)
        snprintf(type, sizeof type, "Program Change");
    else if (status == EVENT_CHANNEL_PRESSURE)
        snprintf(type, sizeof type, "Channel Pressure");
    else if (status == EVENT_PITCH_WHEEL)
        snprintf(type, sizeof type, "Pitch Wheel");
    else
        snprintf(type, sizeof type, "Unknown MIDI Event");

    char text[80];
    snprintf(text, sizeof text, "%s %s", hex, type);
    ui->m_entry_data->setText(text);
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
    int qnfactor = perf().ppqn() * 4;
    int item = s_snap_items[index];
    int v = qnfactor / item;
    set_snap(v);
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
    int qnfactor = perf().ppqn() * 4;
    int item = s_snap_items[index];
    int v = qnfactor / item;
    set_note_length(v);
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
        double factor = double(perf().ppqn()) / double(m_original);
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
        infoprintf("PPQN = %d\n", ppqn);
        infoprintf("BPM = %d\n", int(bpm));
    }
    return result;
}

bool
qseqeditframe64::change_ppqn (int ppqn)
{
    int zoom = usr().zoom();
    set_snap(sm_initial_snap * ppqn / SEQ66_DEFAULT_PPQN);
    set_note_length(sm_initial_note_length * ppqn / SEQ66_DEFAULT_PPQN);
    if (usr().zoom() == SEQ66_USE_ZOOM_POWER_OF_2)      /* i.e. 0 */
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
    bool result = qseqframe::set_zoom(z);
    if (result)
    {
        int index = s_lookup_zoom(zoom());
        ui->m_combo_zoom->setCurrentIndex(index);
        update_draw_geometry();
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
 *  That then triggers the  callback.
 */

void
qseqeditframe64::slot_reset_zoom ()
{
    (void) qseqframe::reset_zoom();     // ui->m_combo_zoom->setCurrentIndex(1);
}

/**
 *  Handles updates to the key selection.
 */

void
qseqeditframe64::update_key (int index)
{
    if (index != m_key && legal_key(index))
    {
        set_key(index);
        set_dirty();
    }
}

void
qseqeditframe64::set_key (int key)
{
    if (legal_key(key))
    {
        ui->m_combo_key->setCurrentIndex(key);
        if (not_nullptr(m_seqroll))
            m_seqroll->set_key(key);

        if (not_nullptr(m_seqkeys))
            m_seqkeys->set_key(key);
    }
}

void
qseqeditframe64::reset_key ()
{
    ui->m_combo_key->setCurrentIndex(0);
    if (not_nullptr(m_seqroll))
        m_seqroll->set_key(0);

    if (not_nullptr(m_seqkeys))
        m_seqkeys->set_key(0);
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
qseqeditframe64::set_scale (int scale)
{
    if (scale != m_scale && legal_scale(scale))
    {
        m_scale = scale;
        ui->m_combo_scale->setCurrentIndex(scale);
        if (not_nullptr(m_seqroll))
            m_seqroll->set_scale(scale);

        set_dirty();
    }
}

void
qseqeditframe64::reset_scale ()
{
    set_scale(0);
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
        m_events_popup->exec
        (
            ui->m_button_event->mapToGlobal
            (
                QPoint
                (
                    ui->m_button_event->width()-2,
                    ui->m_button_event->height()-2
                )
            )
        );
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
    QPixmap p(state? menu_full_xpm : menu_empty_xpm);
    return new QIcon(p);
}

/**
 *  A case where a macro makes the code easier to read.
 */

#define SET_DATA_TYPE(status, cc) \
    std::bind(&qseqeditframe64::set_data_type, this, status, cc)

/**
 *  Function to create event menu entries.  Too damn big!
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
    QString mlabel(text.c_str());
    QIcon micon(*create_menu_image(present));
    QAction * item = new QAction(micon, mlabel, nullptr);
#else
    QAction * item = new QAction(*create_menu_image(present), text.c_str());
#endif
    menu->addAction(item);
    connect(item, &QAction::triggered, SET_DATA_TYPE(status, control));
    if (present && m_first_event == 0x00)
    {
        m_first_event = status;
        m_first_event_name = text;
        set_data_type(status, 0);       // need m_first_control value!
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
    midibyte status = 0, cc = 0;
    memset(ccs, false, sizeof(bool) * c_midibyte_data_max);
    event::buffer::const_iterator cev;
    seq_pointer()->reset_ex_iterator(cev);
    while (seq_pointer()->get_next_event_ex(status, cc, cev))
    {
        switch (status)
        {
        case EVENT_NOTE_OFF:
            note_off = true;
            break;

        case EVENT_NOTE_ON:
            note_on = true;
            break;

        case EVENT_AFTERTOUCH:
            aftertouch = true;
            break;

        case EVENT_CONTROL_CHANGE:
            ccs[cc] = true;
            break;

        case EVENT_PITCH_WHEEL:
            pitch_wheel = true;
            break;

        case EVENT_PROGRAM_CHANGE:
            program_change = true;
            break;

        case EVENT_CHANNEL_PRESSURE:
            channel_pressure = true;
            break;
        }
        ++cev;                                      /* found, must do this  */
    }

    if (not_nullptr(m_events_popup))
        delete m_events_popup;

    m_events_popup = new QMenu(this);
    set_event_entry(m_events_popup, "Note On Velocity", note_on, EVENT_NOTE_ON);
    m_events_popup->addSeparator();
    set_event_entry
    (
        m_events_popup, "Note Off Velocity", note_off, EVENT_NOTE_OFF
    );
    set_event_entry(m_events_popup, "Aftertouch", aftertouch, EVENT_AFTERTOUCH);
    set_event_entry
    (
        m_events_popup, "Program Change", program_change, EVENT_PROGRAM_CHANGE
    );
    set_event_entry
    (
        m_events_popup, "Channel Pressure", channel_pressure,
        EVENT_CHANNEL_PRESSURE
    );
    set_event_entry
    (
        m_events_popup, "Pitch Wheel", pitch_wheel, EVENT_PITCH_WHEEL
    );
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
             * That's what the merge Seq24 code does!  We need to document
             * it in the seq66-doc and seq66-doc projects.  Also, there
             * was a bug in Seq24 where the instrument number was use re 1
             * to get the proper instrument... it needs to be decremented to
             * be re 0.
             */

            std::string controller_name(c_controller_names[offset + item]);
            const usermidibus & umb = usr().bus(buss);
            int inst = umb.instrument(channel);
            const userinstrument & uin = usr().instrument(inst);
            if (uin.is_valid())                             // redundant check
            {
                if (uin.controller_active(offset + item))
                    controller_name = uin.controller_name(offset + item);
            }
            set_event_entry
            (
                menucc, controller_name, ccs[offset+item],
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
        m_minidata_popup->exec
        (
            ui->m_button_data->mapToGlobal
            (
                QPoint
                (
                    ui->m_button_data->width()-2,
                    ui->m_button_data->height()-2
                )
            )
        );
    }
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
    midibyte status = 0, cc = 0;
    memset(ccs, false, sizeof(bool) * c_midibyte_data_max);
    event::buffer::const_iterator cev;
    seq_pointer()->reset_ex_iterator(cev);
    while (seq_pointer()->get_next_event_ex(status, cc, cev))
    {
        switch (status)
        {
        case EVENT_NOTE_OFF:
            note_off = true;
            break;

        case EVENT_NOTE_ON:
            note_on = true;
            break;

        case EVENT_AFTERTOUCH:
            aftertouch = true;
            break;

        case EVENT_CONTROL_CHANGE:
            ccs[cc] = true;
            break;

        case EVENT_PITCH_WHEEL:
            pitch_wheel = true;
            break;

        case EVENT_PROGRAM_CHANGE:
            program_change = true;
            break;

        case EVENT_CHANNEL_PRESSURE:
            channel_pressure = true;
            break;
        }
        ++cev;                                      /* found, must do this  */
    }

    if (not_nullptr(m_minidata_popup))
        delete m_minidata_popup;

    m_minidata_popup = new QMenu(this);

    bool any_events = false;
    if (note_on)
    {
        any_events = true;
        set_event_entry
        (
            m_minidata_popup, "Note On Velocity", true, EVENT_NOTE_ON
        );
    }
    if (note_off)
    {
        any_events = true;
        set_event_entry
        (
            m_minidata_popup, "Note Off Velocity", true, EVENT_NOTE_OFF
        );
    }
    if (aftertouch)
    {
        any_events = true;
        set_event_entry(m_minidata_popup, "Aftertouch", true, EVENT_AFTERTOUCH);
    }
    if (program_change)
    {
        any_events = true;
        set_event_entry
        (
            m_minidata_popup, "Program Change", true, EVENT_PROGRAM_CHANGE
        );
    }
    if (channel_pressure)
    {
        any_events = true;
        set_event_entry
        (
            m_minidata_popup, "Channel Pressure", true, EVENT_CHANNEL_PRESSURE
        );
    }
    if (pitch_wheel)
    {
        any_events = true;
        set_event_entry
        (
            m_minidata_popup, "Pitch Wheel", true, EVENT_PITCH_WHEEL
        );
    }

    if (any_events)
        m_minidata_popup->addSeparator();

    /**
     *  Create the one menu for the controller changes that actually exist in
     *  the track, if any.
     */

    const int itemcount = c_midibyte_data_max;              /* 128          */
    for (int item = 0; item < itemcount; ++item)
    {
        std::string controller_name(c_controller_names[item]);
        const usermidibus & umb = usr().bus(buss);
        int inst = umb.instrument(channel);
        const userinstrument & uin = usr().instrument(inst);
        if (uin.is_valid())                                 /* redundant    */
        {
            if (uin.controller_active(item))
                controller_name = uin.controller_name(item);
        }
        if (ccs[item])
        {
            any_events = true;
            set_event_entry
            (
                m_minidata_popup, controller_name, true,
                EVENT_CONTROL_CHANGE, item
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

void
qseqeditframe64::show_lfo_frame ()
{
    if (is_nullptr(m_lfo_wnd))
        m_lfo_wnd = new qlfoframe(perf(), seq_pointer(), *m_seqdata);

    m_lfo_wnd->show();
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
 *  Passes the recording status to the performer object.  Both
 *  record_change_callback() and thru_change_callback() will call
 *  set_sequence_input() for the same sequence. We only need to call it if it is
 *  not already set, if setting. And, we should not unset it if the
 *  m_toggle_thru->get_active() is true.
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
 *  Passes the MIDI Thru status to the performer object.  Both
 *  record_change_callback() and thru_change_callback() will call
 *  set_sequence_input() for the same sequence. We only need to call it if it is
 *  not already set, if setting. And, we should not unset it if the
 *  m_toggle_thru->get_active() is true.
 */

void
qseqeditframe64::thru_change (bool ischecked)
{
    if (perf().set_thru(seq_pointer(), ischecked, false))
        update_midi_buttons();
}

void
qseqeditframe64::update_record_type (int index)
{
    bool ok = seq_pointer()->update_recording(index);
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

/**
 *  Here, we need to get the filespec, create a notemapper, fill it from the
 *  notemapfile, and iterate through the notes, converting them.
 *
 *  Currently not connected to a signal.  TODO.
 */

void
qseqeditframe64::remap_notes ()
{
    (void) repitch_selected();
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
 *  Removes the LFO editor frame.
 */

void
qseqeditframe64::remove_lfo_frame ()
{
    if (not_nullptr(m_lfo_wnd))
    {
        delete m_lfo_wnd;
        m_lfo_wnd = nullptr;
    }
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

void
qseqeditframe64::analyze_seq_notes ()
{
    keys outkey;
    scales outscale;
    if (analyze_notes(seq_pointer()->events(), outkey, outscale))
    {
        int k = static_cast<int>(outkey);
        int s = static_cast<int>(outscale);
        printf
        (
            "key %s (%d), scale %s (%d)\n",
            musical_key_name(k).c_str(),
            k, musical_scale_name(s).c_str(), s
        );
    }
    /* MORE TO DO? */
}

}           // namespace seq66

/*
 * qseqeditframe64.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


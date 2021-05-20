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
 * \file          qperfeditframe64.cpp
 *
 *  This module declares/defines the base class for the Performance Editor,
 *  also known as the Song Editor.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-07-18
 * \updates       2021-05-20
 * \license       GNU GPLv2 or above
 *
 *  Note that, as of version 0.9.11, the z and Z keys, when focus is on the
 *  perfroll (piano roll), will zoom the view horizontally.  Not working!
 */

#include <QScrollBar>

#include "play/performer.hpp"           /* seq66::performer class           */
#include "qperfeditframe64.hpp"
#include "qperfroll.hpp"
#include "qperfnames.hpp"
#include "qperftime.hpp"
#include "qt5_helpers.hpp"              /* seq66::qt_set_icon()             */

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#if defined SEQ66_QMAKE_RULES
#include "forms/ui_qperfeditframe64.h"
#else
#include "forms/qperfeditframe64.ui.h"
#endif

/*
 *  We prefer to load the pixmaps on the fly, rather than deal with those
 *  friggin' resource files.
 */

#include "pixmaps/collapse.xpm"
#include "pixmaps/copy.xpm"
#include "pixmaps/expand.xpm"
#include "pixmaps/finger.xpm"
#include "pixmaps/follow.xpm"
#include "pixmaps/loop.xpm"
#include "pixmaps/redo.xpm"
#include "pixmaps/right.xpm"
#include "pixmaps/transpose.xpm"
#include "pixmaps/undo.xpm"
#include "pixmaps/zoom_in.xpm"
#include "pixmaps/zoom_out.xpm"

namespace seq66
{

/**
 *  Helps with making the page leaps slightly smaller than the width of the
 *  piano roll scroll area.  Same value as used in qseqeditframe64.
 */

static const int c_progress_page_overlap = 80;

/**
 *  Trigger transpose ranges.
 */

static const int c_trigger_transpose_min    = (-60);
static const int c_trigger_transpose_max    =   60;

/**
 *  The number of Snap entries in the combo-box:
 *
 *      "Length", "1/1", "1/2", "1/3", "1/4", "1/8", "1/16", and "1/32"
 */

static const int c_snap_entry [] = { 0, 1, 2, 3, 4, 8, 16, 32 };
static const int c_snap_entry_count = 8;

/**
 *  Principal constructor, has a reference to a performer object.
 *
 * \param p
 *      Refers to the main performance object.
 *
 * \param parent
 *      The Qt widget that owns this frame.  Either the qperfeditex (the
 *      external window holding this frame) or the "Song" tab object in the
 *      main window.
 *
 * \param isexternal
 *      Indicates that this is an external frame, so that we can reveal the Loop
 *      button.
 */

qperfeditframe64::qperfeditframe64
(
    seq66::performer & p,
    QWidget * parent,
    bool isexternal
) :
    QFrame                  (parent),
    ui                      (new Ui::qperfeditframe64),
    m_mainperf              (p),
    m_palette               (nullptr),
    m_is_external           (isexternal),
    m_snap                  (8),
    m_beats_per_measure     (4),
    m_beat_width            (4),
    m_trigger_transpose     (0),
    m_perfroll              (nullptr),
    m_perfnames             (nullptr),
    m_perftime              (nullptr)
{
    ui->setupUi(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    /*
     * What about the window title?
     */

    /*
     * Snap.  Fill options for grid snap combo box and set the default.
     * We need an obvious macro for "6".
     */

    for (int i = 0; i < c_snap_entry_count; ++i)
    {
        QString combo_text = "1/" + QString::number(c_snap_entry[i]);
        if (i == 0)
            combo_text = "Length";

        ui->cmbGridSnap->insertItem(i, combo_text);
    }
    ui->cmbGridSnap->setCurrentIndex(3);
    connect
    (
        ui->cmbGridSnap, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_grid_snap(int))
    );

    /*
     * Create and add the scroll-area and widget-container for this frame.
     * If we're using the qscrollmaster (a QScrollArea), the scroll-area will
     * contain only the qperfoll.  Otherwise, it will contain the qperfroll,
     * qperfnames, and qperftime.  In either case the widget-container contains
     * all three panels.
     *
     * Create the piano roll panel, the names panel, and time panel of the song
     * editor frame.
     */

    m_perfnames = new qperfnames(m_mainperf, ui->namesScrollArea);
    ui->namesScrollArea->setWidget(m_perfnames);
    ui->namesScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->namesScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_perftime = new qperftime
    (
        m_mainperf, SEQ66_DEFAULT_ZOOM, SEQ66_DEFAULT_SNAP,
        this, ui->timeScrollArea
    );
    ui->timeScrollArea->setWidget(m_perftime);
    ui->timeScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->timeScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_perfroll = new qperfroll
    (
        m_mainperf, SEQ66_DEFAULT_ZOOM, SEQ66_DEFAULT_SNAP,
        m_perfnames, this, ui->rollScrollArea
    );
    ui->rollScrollArea->setWidget(m_perfroll);
    ui->rollScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->rollScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    /*
     *  Add the various scrollbar points to the qscrollmaster object.
     */

    ui->rollScrollArea->add_v_scroll(ui->namesScrollArea->verticalScrollBar());
    ui->rollScrollArea->add_h_scroll(ui->timeScrollArea->horizontalScrollBar());

    /*
     * Create the color palette for coloring the patterns.
     */

    m_palette = new QPalette();
    m_palette->setColor(QPalette::Background, Qt::darkGray);

    /*
     * Undo and Redo buttons.
     */

    connect(ui->btnUndo, SIGNAL(clicked(bool)), m_perfroll, SLOT(undo()));
    qt_set_icon(undo_xpm, ui->btnUndo);
    connect(ui->btnRedo, SIGNAL(clicked(bool)), m_perfroll, SLOT(redo()));
    qt_set_icon(redo_xpm, ui->btnRedo);

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
    ui->m_toggle_follow->setChecked(m_perfroll->progress_follow());
    connect(ui->m_toggle_follow, SIGNAL(toggled(bool)), this, SLOT(follow(bool)));

    /*
     * Zoom-In and Zoom-Out buttons.
     */

    connect(ui->btnZoomIn, SIGNAL(clicked(bool)), this, SLOT(zoom_in()));
    qt_set_icon(zoom_in_xpm, ui->btnZoomIn);
    connect(ui->btnZoomOut, SIGNAL(clicked(bool)), this, SLOT(zoom_out()));
    qt_set_icon(zoom_out_xpm, ui->btnZoomOut);

    /*
     * Transpose button and combo-box.
     */

    connect
    (
        ui->btnTranspose, SIGNAL(clicked(bool)),
        this, SLOT(reset_transpose())
    );
    qt_set_icon(transpose_xpm, ui->btnTranspose);

    char num[16];
    for (int t = -c_octave_size; t <= c_octave_size; ++t)
    {
        int index = t + c_octave_size;
        if (t != 0)
        {
            const char * cit = c_interval_text[abs(t)].c_str();
            snprintf(num, sizeof num, "%+d [%s]", t, cit);
        }
        else
            snprintf(num, sizeof num, "0 [normal]");

        ui->comboTranspose->insertItem(index, num);
    }
    ui->comboTranspose->setCurrentIndex(c_octave_size);
    connect
    (
        ui->comboTranspose, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_transpose(int))
    );

    /*
     * Collapse, Expand, Expand-Copy, Grow, and Loop buttons.
     */

    connect
    (
        ui->btnCollapse, SIGNAL(clicked(bool)),
        this, SLOT(marker_collapse())
    );
    qt_set_icon(collapse_xpm, ui->btnCollapse);
    connect(ui->btnExpand, SIGNAL(clicked(bool)), this, SLOT(marker_expand()));
    qt_set_icon(expand_xpm, ui->btnExpand);
    connect
    (
        ui->btnExpandCopy, SIGNAL(clicked(bool)),
        this, SLOT(marker_expand_copy())
    );
    qt_set_icon(copy_xpm, ui->btnExpandCopy);

    if (m_is_external)
    {
        connect
        (
            ui->btnLoop, SIGNAL(clicked(bool)),
            this, SLOT(marker_loop(bool))
        );
        qt_set_icon(loop_xpm, ui->btnLoop);
    }
    else
        ui->btnLoop->hide();

    /*
     *  The width of the qperfroll is based on its sizeHint(), which is based
     *  on the maximum trigger in all of the sequences in all sets.  At the
     *  moment, we're not sure how to deal with this, so the grow button is
     *  hidden.
     */

    connect(ui->btnGrow, SIGNAL(clicked(bool)), this, SLOT(grow()));
    qt_set_icon(right_xpm, ui->btnGrow);
    ui->btnGrow->setEnabled(false);
    ui->btnGrow->hide();

    /*
     *  Trigger transpose button and spin-box.
     */

    connect
    (
        ui->btnResetTT, SIGNAL(clicked(bool)),
        this, SLOT(reset_trigger_transpose(bool))
    );

    ui->spinBoxTT->setRange(c_trigger_transpose_min, c_trigger_transpose_max);
    ui->spinBoxTT->setSingleStep(1);
    ui->spinBoxTT->setValue(m_trigger_transpose);
    ui->spinBoxTT->setReadOnly(false);
    connect
    (
        ui->spinBoxTT, SIGNAL(valueChanged(int)),
        this, SLOT(set_trigger_transpose(int))
    );

    /*
     *  Entry mode
     */

    qt_set_icon(finger_xpm, ui->btnEntryMode);
    ui->btnEntryMode->setCheckable(true);
    ui->btnEntryMode->setAutoDefault(false);
    ui->btnEntryMode->setChecked(false);
    connect
    (
        ui->btnEntryMode, SIGNAL(toggled(bool)), this,
        SLOT(entry_mode(bool))
    );

    /*
     * Song-record snap button
     */

    ui->btnSnap->setCheckable(true);
    ui->btnSnap->setChecked(perf().song_record_snap());
    connect
    (
        ui->btnSnap, &QPushButton::toggled,
        [=]
        {
            perf().toggle_record_snap();
            ui->btnSnap->setChecked(perf().song_record_snap());
        }
    );

    /*
     * Final settings.  For snap, 8 is too small.  4, which is actually "1/4"
     * is better at normal zoom, and also represents a single beat.
     */

    set_snap(4);
    set_beats_per_measure(4);
    set_beat_width(4);
}

qperfeditframe64::~qperfeditframe64 ()
{
    delete ui;
    if (not_nullptr(m_palette))     // valgrind fix?
        delete m_palette;
}

/**
 *  Passes the Follow status to the qperfroll object.
 */

void
qperfeditframe64::follow (bool ischecked)
{
    m_perfroll->progress_follow(ischecked);
}

/**
 *  Checks the position of the tick, and, if it is in a different piano-roll
 *  "page" than the last page, moves the page to the next page.
 */

void
qperfeditframe64::follow_progress ()
{
    int w = ui->rollScrollArea->width() - c_progress_page_overlap;
    if (w > 0)
    {
        QScrollBar * hadjust = ui->rollScrollArea->h_scroll();
        midipulse progtick = perf().get_tick();
        if (progtick > 0 && m_perfroll->progress_follow())
        {
            int progx = m_perfroll->tix_to_pix(progtick);
            int page = progx / w;
            int oldpage = m_perfroll->scroll_page();
            bool newpage = page != oldpage;
            if (newpage)
            {
                m_perfroll->scroll_page(page);  // scrollmaster updates the rest
                hadjust->setValue(progx);       // set_scroll_x() not needed
            }
        }
    }
}


/**
 *  Sets the snap value per the given index.
 *
 * \param snapindex
 *      The order of the value in the menu. For 0 to 5, this is the exponent
 *      of 2 that yields the snap value.  The default is 4, which sets snap to
 *      16.
 */

void
qperfeditframe64::update_grid_snap (int snapindex)
{
    if (snapindex >= 0 && snapindex < c_snap_entry_count)
    {
        m_snap = c_snap_entry[snapindex];
        set_guides();
    }
}

void
qperfeditframe64::set_snap (midipulse s)
{
    if (s > 0)
    {
        char b[16];
        snprintf(b, sizeof b, "1/%d", int(s));
        ui->cmbGridSnap->setCurrentText(b);
        m_snap = int(s);
    }
    else
    {
        ui->cmbGridSnap->setCurrentText("Length");
        m_snap = 0;
    }
    set_guides();
}

/**
 *  These values are ticks, but passed as integers.
 *  The guides are set to 4 measures by default.
 */

void
qperfeditframe64::set_guides ()
{
    if (m_beat_width > 0 && m_snap >= 0)
    {
        midipulse pp = perf().ppqn() * 4;
        midipulse measticks = pp * m_beats_per_measure / m_beat_width;
        midipulse beatticks = pp / m_beat_width;
        midipulse snapticks = m_snap == 0 ? 0 : measticks / m_snap ;
        m_perfroll->set_guides(snapticks, measticks, beatticks);
        m_perftime->set_guides(snapticks, measticks);

#if defined SEQ66_PLATFORM_DEBUG_TMI
        printf
        (
            "set_guides(snap = %d, measure = %d, beat = %d ticks\n",
            snap, measure, beat
        );
#endif
    }
}

/**
 *  Would be called by the parent perhaps, using the
 *  on_resolution_change() function
 */

#ifdef USE_HANDLING_OF_BPM_PPQN_CHANGES

bool
qperfeditframe64::change_ppqn (int ppqn)
{
    set_guides();
}

bool
qperfeditframe64::change_bpm (int bpm)
{
    // nothing to do here, beats per minute does not affect the Song grid
    // appearance.
}

#endif

/**
 *  Pass-along function for zooming in.  Calls the same function for qperftime
 *  and qperfroll.
 */

void
qperfeditframe64::zoom_in ()
{
    int zprevious = m_perfroll->zoom();
    m_perftime->zoom_in();
    m_perfroll->zoom_in();

    float factor = float(zprevious) / float(m_perfroll->zoom());
    ui->rollScrollArea->scroll_x_by_factor(factor); // 2.0f
}

/**
 *  Pass-along function for zooming out.  Calls the same function for qperftime
 *  and qperfroll.
 */

void
qperfeditframe64::zoom_out ()
{
    int zprevious = m_perfroll->zoom();
    m_perftime->zoom_out();
    m_perfroll->zoom_out();

    float factor = float(zprevious) / float(m_perfroll->zoom());
    ui->rollScrollArea->scroll_x_by_factor(factor); // 0.5f
}

/**
 *  Pass-along function for zoom reset.  Calls the same function for qperftime
 *  and qperfroll.
 */

void
qperfeditframe64::reset_zoom ()
{
    m_perftime->reset_zoom();
    m_perfroll->reset_zoom();
}

/**
 *  The button callback for transposition for this window.  Unlike the
 *  Gtkmm-2.4 version, this version just toggles whether it is used or not,
 *  for now.  We will add a combo-box selector soon.
 */

void
qperfeditframe64::reset_transpose ()
{
#ifdef THIS_IS_BETTER
    if (perf().get_transpose() != 0)
       set_transpose(0);
#else
    ui->comboTranspose->setCurrentIndex(c_octave_size);
#endif
}

/**
 *  Handles updates to the tranposition value.  This value can be used to
 *  transpose the whole song, or just one trigger, depending on the action
 *  selected.
 */

void
qperfeditframe64::update_transpose (int index)
{
    int transpose = index - c_octave_size;
    if (transpose >= -c_octave_size && transpose <= c_octave_size)
    {
        if (perf().get_transpose() != transpose)
            set_transpose(transpose);
    }
}

/**
 *  Sets the value of transposition for this window.
 *
 * \param transpose
 *      The amount to transpose the transposable sequences.
 *      We need to add validation at some point, if the widget does not
 *      enforce that.
 */

void
qperfeditframe64::set_transpose (int transpose)
{
    perf().all_notes_off();
    perf().set_transpose(transpose);
}

void
qperfeditframe64::entry_mode (bool ischecked)
{
    if (not_nullptr(m_perfroll))
        m_perfroll->set_adding(ischecked);
}

void
qperfeditframe64::update_entry_mode (bool on)
{
    ui->btnEntryMode->setChecked(on);
}

/**
 *  Calls updateGeometry() on child elements to react to changes in MIDI file
 *  sizes.
 *
 *  For the resize() calls, see qperfbase::force_resize().
 */

void
qperfeditframe64::update_sizes ()
{
    m_perfnames->resize();
    m_perfroll->resize();

    m_perfnames->updateGeometry();
    m_perfroll->updateGeometry();
    m_perftime->updateGeometry();
}

/**
 *  Calls set_dirty() on child element to react to zoom actions. But
 *  qperfnames has no timer, so we update it directly.
 */

void
qperfeditframe64::set_dirty ()
{
    m_perfnames->reupdate();
    m_perfroll->set_dirty();
    m_perftime->set_dirty();
}

void
qperfeditframe64::marker_collapse ()
{
    perf().collapse();
    set_dirty();
}

void
qperfeditframe64::marker_expand ()
{
    perf().expand();
    set_dirty();
}

void
qperfeditframe64::marker_expand_copy ()
{
    perf().copy();
    set_dirty();
}

void
qperfeditframe64::grow ()
{
    m_perfroll->increment_size();
    m_perftime->increment_size();
}

void
qperfeditframe64::reset_trigger_transpose (bool /*ischecked*/)
{
    ui->spinBoxTT->setValue(0);
}

void
qperfeditframe64::set_trigger_transpose (int tpose)
{
    if (tpose >= c_trigger_transpose_min && tpose <= c_trigger_transpose_max)
    {
        ui->spinBoxTT->setValue(tpose);     // m_trigger_transpose
        m_perfroll->set_trigger_transpose(tpose);
    }
}

void
qperfeditframe64::marker_loop (bool loop)
{
    perf().looping(loop);
}

void
qperfeditframe64::set_loop_button (bool looping)
{
    ui->btnLoop->setChecked(looping);
}

}           // namespace seq66

/*
 * qperfeditframe64.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


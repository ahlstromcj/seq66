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
 * \file          qperfeditframe64.cpp
 *
 *  This module declares/defines the base class for the Performance Editor,
 *  also known as the Song Editor.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-07-18
 * \updates       2023-09-29
 * \license       GNU GPLv2 or above
 *
 */

#include <QKeyEvent>                    /* Needed for QKeyEvent::accept()   */
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
#include "pixmaps/expandgrid.xpm"                   /* "pixmaps/right.xpm"  */
#include "pixmaps/finger.xpm"
#include "pixmaps/follow.xpm"
#include "pixmaps/loop.xpm"
#include "pixmaps/redo.xpm"
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

static const int c_trigger_transpose_min = (-60);
static const int c_trigger_transpose_max =   60;

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
    QFrame              (parent),
    ui                  (new Ui::qperfeditframe64),
    m_mainperf          (p),
    m_palette           (nullptr),
    m_is_external       (isexternal),
    m_duration_mode     (true),
    m_move_L_marker     (false),
    m_snap_list         (perf_snap_items()),    /* issue #44, no "current"   */
    m_snap              (8),
    m_beats_per_measure (4),
    m_beat_width        (4),
    m_trigger_transpose (0),
    m_perfroll          (nullptr),
    m_perfnames         (nullptr),
    m_perftime          (nullptr)
{
    ui->setupUi(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    /*
     * Snap.  Fill options for grid snap combo box and set the default.
     * We need an obvious macro for "6".
     *
     * Not needed: snap_list().current("Length");
     */

    (void) fill_combobox(ui->cmbGridSnap, snap_list(), "4", "1/");  // "1/4"
    connect
    (
        ui->cmbGridSnap, SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_grid_snap(int))
    );

    /*
     * Create and add the scroll-area and widget-container for this frame.  If
     * we're using the qscrollmaster (a QScrollArea), the scroll-area will
     * contain only the qperfoll.  Otherwise, it will contain the qperfroll,
     * qperfnames, and qperftime.  In either case the widget-container
     * contains all three panels.
     *
     * Create the piano roll panel, the names panel, and time panel of the
     * song editor frame.
     */

    m_perfnames = new (std::nothrow)
        qperfnames(m_mainperf, ui->namesScrollArea);

    ui->namesScrollArea->setWidget(m_perfnames);

    /*
     * Leave the useless horizontal scrollbar in place in order to match the
     * qperfroll's vertical dimensions.
     *
     * ui->namesScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
     */

    ui->namesScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->namesScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_perftime = new (std::nothrow) qperftime
    (
        m_mainperf, c_default_zoom, c_default_snap,
        this, ui->timeScrollArea
    );
    ui->timeScrollArea->setWidget(m_perftime);
    ui->timeScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->timeScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_perfroll = new (std::nothrow) qperfroll
    (
        m_mainperf, c_default_zoom, c_default_snap,
        m_perfnames, this, ui->rollScrollArea
    );
    ui->rollScrollArea->setWidget(m_perfroll);
    ui->rollScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->rollScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    /*
     *  Add the various scrollbar pointers to the qscrollmaster object.
     */

    ui->rollScrollArea->add_v_scroll(ui->namesScrollArea->verticalScrollBar());
    ui->rollScrollArea->add_h_scroll(ui->timeScrollArea->horizontalScrollBar());

    /*
     * Create the color palette for coloring the patterns.
     */

    m_palette = new QPalette();
#if QT_DEPRECATED_SINCE(5,13)
    m_palette->setColor(QPalette::Background, Qt::darkGray);
#else
    m_palette->setColor(QPalette::Window, Qt::darkGray);
#endif

    /*
     * Undo and Redo buttons.
     *
     *      tooltip_with_keystroke(ui->btnUndo, "Ctrl-Z");
     *      tooltip_with_keystroke(ui->btnUndo, "Shift-Ctrl-Z");
     */

    connect(ui->btnUndo, SIGNAL(clicked(bool)), m_perfroll, SLOT(undo()));
    qt_set_icon(undo_xpm, ui->btnUndo);
    connect(ui->btnRedo, SIGNAL(clicked(bool)), m_perfroll, SLOT(redo()));
    qt_set_icon(redo_xpm, ui->btnRedo);

    /*
     * Follow Progress Button.  Qt::NoFocus is the default focus policy.
     */

    std::string keyname =
        perf().automation_key(automation::slot::follow_transport);

    tooltip_with_keystroke(ui->m_toggle_follow, keyname);
    qt_set_icon(follow_xpm, ui->m_toggle_follow);

    /*
     * Now specifiable in the 'usr' file.
     *
     * ui->m_toggle_follow->setEnabled(true);
     */

    bool followprogress = usr().follow_progress();
    ui->m_toggle_follow->setEnabled(true);
    ui->m_toggle_follow->setCheckable(true);
    ui->m_toggle_follow->setChecked(followprogress);
    ui->m_toggle_follow->setAutoDefault(false);         // ???

    /*
     *
     *  if (perf().song_mode())
     *      m_perfroll->progress_follow(true);
     *
     *  ui->m_toggle_follow->setChecked(m_perfroll->progress_follow());
     */

    follow(followprogress);
    connect
    (
        ui->m_toggle_follow, SIGNAL(toggled(bool)),
        this, SLOT(follow(bool))
    );
    set_zoom(c_default_zoom);

    /*
     * Zoom-In and Zoom-Out buttons.
     */

    connect(ui->btnZoomIn, SIGNAL(clicked(bool)), this, SLOT(slot_zoom_in()));
    qt_set_icon(zoom_in_xpm, ui->btnZoomIn);
    connect(ui->btnZoomOut, SIGNAL(clicked(bool)), this, SLOT(slot_zoom_out()));
    qt_set_icon(zoom_out_xpm, ui->btnZoomOut);

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
            const char * cit = interval_name_ptr(t);    /* see scales.hpp   */
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
        std::string keyname = perf().automation_key(automation::slot::loop_LR);
        tooltip_with_keystroke(ui->btnLoop, keyname);
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
     *  on the maximum trigger in all of the sequences in all sets.
     */

    connect(ui->btnGrow, SIGNAL(clicked(bool)), this, SLOT(grow()));
    qt_set_icon(expandgrid_xpm, ui->btnGrow);

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
     * Final settings.  For snap, 8 is too small.  4, which is actually "1/4",
     * is better at normal zoom, and also represents a single beat. But
     * let's use the actual beat length.
     */

    set_snap(perf().get_beat_width());                      /* (4) */
    set_beats_per_measure(perf().get_beats_per_bar());
    set_beat_width(perf().get_beat_width());

    std::string dur = perf().duration(m_duration_mode);
    std::string css = usr().time_colors_css();
    if (! css.empty())
        ui->btnDuration->setStyleSheet(qt(css));

    ui->btnDuration->setText(qt(dur));
    connect
    (
        ui->btnDuration, SIGNAL(clicked(bool)),
        this, SLOT(slot_duration(bool))
    );
}

qperfeditframe64::~qperfeditframe64 ()
{
    delete ui;
    if (not_nullptr(m_palette))
        delete m_palette;
}

void
qperfeditframe64::scroll_by_step (qscrollmaster::dir d)
{
    switch (d)
    {
    case qscrollmaster::dir::left:
    case qscrollmaster::dir::right:

        ui->rollScrollArea->scroll_x_by_step(d);
        break;

    case qscrollmaster::dir::up:
    case qscrollmaster::dir::down:

        ui->rollScrollArea->scroll_y_by_step(d);
        break;
    }
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
                hadjust->setValue(progx);
            }
        }
    }
}

void
qperfeditframe64::scroll_to_tick (midipulse tick)
{
    int w = ui->rollScrollArea->width();
    if (w > 0)              /* w is constant, e.g. around 742 by default    */
    {
        int x = m_perfroll->tix_to_pix(tick);
        ui->rollScrollArea->scroll_to_x(x);
    }
}

/**
 *  Sets the snap value per the given index.
 *
 * \param snapindex
 *      The order of the value in the menu. For 0 to 5, this is essentially
 *      the exponent of 2 that yields the snap value.  The default is 4, which
 *      sets snap to 16.
 */

void
qperfeditframe64::update_grid_snap (int snapindex)
{
    if (snapindex >= 0 && snapindex < snap_list().count())
    {
        m_snap = snap_list().ctoi(snapindex);
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
 *  These values are ticks, but passed as integers.  The guides are set to 4
 *  measures by default.
 */

void
qperfeditframe64::set_guides ()
{
    if (m_beat_width > 0 && m_snap >= 0)
    {
        midipulse pp = perf().ppqn() * 4;
        midipulse measticks = pp * m_beats_per_measure / m_beat_width;
        midipulse snapticks = m_snap == 0 ? 0 : measticks / m_snap ;
        midipulse beatticks = pp / m_beat_width;
        m_perfroll->set_guides(snapticks, measticks, beatticks);
        m_perftime->set_guides(snapticks, measticks, beatticks);
        perf().record_snap_length(snapticks);
    }
}

/**
 *  Pass-along function for zooming in.  Calls the same function for qperftime
 *  and qperfroll.
 */

void
qperfeditframe64::slot_zoom_in ()
{
    (void) zoom_in();
}

/**
 *  Pass-along function for zooming out.  Calls the same function for qperftime
 *  and qperfroll.
 */

void
qperfeditframe64::slot_zoom_out ()
{
    (void) zoom_out();
}

void
qperfeditframe64::adjust_for_zoom (int zprevious)
{
    int znew = m_perfroll->zoom();
    float factor = float(zprevious) / float(znew);
    ui->rollScrollArea->scroll_x_by_factor(factor);
    set_dirty();
}

bool
qperfeditframe64::zoom_in ()
{
    int zprevious = m_perfroll->zoom();
    bool result = m_perftime->zoom_in();
    if (result)
        result = m_perfroll->zoom_in();

    if (result)
        adjust_for_zoom(zprevious);

    return result;
}

bool
qperfeditframe64::zoom_out ()
{
    int zprevious = m_perfroll->zoom();
    bool result = m_perftime->zoom_out();
    if (result)
        result = m_perfroll->zoom_out();

    if (result)
        adjust_for_zoom(zprevious);

    return result;
}

bool
qperfeditframe64::set_zoom (int z)
{
    int zprevious = m_perfroll->zoom();
    bool result = m_perftime->set_zoom(z);
    if (result)
        result = m_perfroll->set_zoom(z);

    if (result)
        adjust_for_zoom(zprevious);

    return result;
}

/**
 *  Pass-along function for zoom reset.  Calls the same function for qperftime
 *  and qperfroll.
 */

bool
qperfeditframe64::reset_zoom ()
{
    int zprevious = m_perfroll->zoom();
    bool result = m_perftime->reset_zoom();
    if (result)
        result = m_perfroll->reset_zoom();

    if (result)
        adjust_for_zoom(zprevious);

    return result;
}

bool
qperfeditframe64::v_zoom_in ()
{
    return m_perfroll->v_zoom_in();
}

bool
qperfeditframe64::v_zoom_out ()
{
    return m_perfroll->v_zoom_out();
}

bool
qperfeditframe64::reset_v_zoom ()
{
    return m_perfroll->reset_v_zoom();
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
qperfeditframe64::slot_duration (bool /* ischecked */ )
{
    m_duration_mode = ! m_duration_mode;

    std::string dur = perf().duration(m_duration_mode);
    ui->btnDuration->setText(qt(dur));
}

void
qperfeditframe64::update_entry_mode (bool on)
{
    ui->btnEntryMode->setChecked(on);
}

/**
 *  Calls updateGeometry() on child elements to react to changes in MIDI file
 *  sizes when a MIDI file is opened.
 *
 *  For the resize() calls, see qperfbase::force_resize().
 */

void
qperfeditframe64::update_sizes ()
{
    std::string dur = perf().duration(m_duration_mode);
    ui->btnDuration->setText(qt(dur));
    set_guides();
    m_perfnames->resize();
    m_perfnames->updateGeometry();
    m_perfroll->resize();
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
    std::string dur = perf().duration(m_duration_mode);
    ui->btnDuration->setText(qt(dur));
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
    m_perfroll->increment_width();
    m_perftime->increment_width();
    update_sizes();
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
        ui->spinBoxTT->setValue(tpose);
        m_perfroll->set_trigger_transpose(tpose);
    }
}

/**
 *  There is a button with the same functionality in the main window.
 */

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

/*
 *  We must accept() the key-event, otherwise even key-events in the QLineEdit
 *  items are propagated to the parent, where they then get passed to the
 *  performer as if they were keyboards controls (such as a pattern-toggle
 *  hot-key).
 */

void
qperfeditframe64::keyPressEvent (QKeyEvent * event)
{
    int key = event->key();
    bool isctrl = bool(event->modifiers() & Qt::ControlModifier);
    if (! isctrl)
    {
        bool isshift = bool(event->modifiers() & Qt::ShiftModifier);
        if (! zoom_key_press(isshift, key))
        {
            if (isshift)
            {
                if (key == Qt::Key_L)
                {
                    m_perftime->setFocus();
                    m_perftime->m_move_L_marker = true;
                }
                else if (key == Qt::Key_R)
                {
                    m_perftime->setFocus();
                    m_perftime->m_move_L_marker = false;
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
                    scroll_by_step(qscrollmaster::dir::down);
                else if (key == Qt::Key_K)
                    scroll_by_step(qscrollmaster::dir::up);
                else if (key == Qt::Key_H)
                    scroll_by_step(qscrollmaster::dir::left);
                else if (key == Qt::Key_L)
                    scroll_by_step(qscrollmaster::dir::right);
                else
                    event->accept();
            }
        }
    }
    else
        event->accept();
}

void
qperfeditframe64::keyReleaseEvent (QKeyEvent * event)
{
    event->accept();
}

bool
qperfeditframe64::zoom_key_press (bool shifted, int key)
{
    bool result = false;
    if (shifted)
    {
        if (key == Qt::Key_Z)
        {
            result = zoom_in();
        }
        else if (key == Qt::Key_V)
        {
            result = v_zoom_in();
        }
    }
    else
    {
        if (key == Qt::Key_Z)
        {
            result = zoom_out();
        }
        else if (key == Qt::Key_0)
        {
            result = reset_v_zoom();
            if (result)
                result = reset_zoom();
        }
        else if (key == Qt::Key_V)
        {
            result = v_zoom_out();
        }
    }
    return result;
}

}           // namespace seq66

/*
 * qperfeditframe64.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


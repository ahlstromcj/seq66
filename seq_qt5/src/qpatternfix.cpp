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
 * \file          qpatternfix.cpp
 *
 *  This module declares/defines the base class for the pattern-fix window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2022-04-09
 * \updates       2022-04-10
 * \license       GNU GPLv2 or above
 *
 *  This dialog provides a way to combine the following pattern adjustments:
 *
 *      -   Left-alignment.
 *      -   Fitting to a given number of measures.
 *      -   Arbitrary scaling for compression and expansion over time.
 *      -   Quantization or tightening.
 *
 *  It acts on all events in the track; no selection needed.  In this, it is
 *  similar to the LFO dialog.
 *
 *  This dialog was inspired by Ahlstrom's poor playing and timing skills.
 *
 */

#include <QButtonGroup>
#include <QSlider>

#include "seq66-config.h"               /* defines SEQ66_QMAKE_RULES        */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "qpatternfix.hpp"              /* seq66::qpatternfix class         */
#include "qseqdata.hpp"                 /* seq66::qseqdata for status, CC   */
#include "qseqeditframe64.hpp"          /* seq66::qseqeditframe64, parent   */
#include "qt5_helper.h"                 /* QT5_HELPER_RADIO_SIGNAL macro    */
#include "qt5_helpers.hpp"              /* seq66::qt() string conversion    */
#include "util/calculations.hpp"        /* seq66::wave enum class values    */

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#if defined SEQ66_QMAKE_RULES
#include "forms/ui_qpatternfix.h"
#else
#include "forms/qpatternfix.ui.h"
#endif

/*
 * Don't document the namespace.
 */

namespace seq66
{

/**
 *  Static members.
 */

static const double c_scale_min = 0.001;
static const double c_scale_max = 1000.0;

/*
 *  Signal buttonClicked(int) is overloaded in this class. To connect to this
 *  signal by using the function pointer syntax, Qt provides a convenient
 *  helper for obtaining the function pointer as used below, using a lambda
 *  function.
 */

qpatternfix::qpatternfix
(
    performer & p,
    seq::pointer seqptr,
    qseqdata & sdata,
    qseqeditframe64 * editparent,
    QWidget * parent
) :
    QFrame              (parent),
    ui                  (new Ui::qpatternfix),
    m_fixlength_group   (nullptr),
    m_quan_group        (nullptr),
    m_performer         (p),
    m_seq               (seqptr),
    m_seqdata           (sdata),
    m_backup_events     (seqp()->events()),             /* for slot_reset() */
    m_backup_measures   (seqp()->get_measures()),
    m_edit_frame        (editparent),
    m_length_type       (lengthfix::none),
    m_quan_type         (quantization::none),
    m_measures          (seqp()->get_measures()),
    m_scale_factor      (1.0),
    m_align_left        (false),
    m_is_modified       (false)
{
    ui->setupUi(this);

    /*
     * Pattern Number.  Make this editable or a drop-down as well?
     */

    std::string number = std::to_string(int(seqp()->seq_number()));
    ui->line_edit_pattern->setText(qt(number));
    ui->line_edit_pattern->setEnabled(false);           /* no user-edit yet */

    /*
     *  connect
     *  (
     *      ui->line_edit_pattern, SIGNAL(editingFinished()),
     *      this, SLOT(slot_pattern_number())
     *  );
     */

    std::string plabel = "Pattern #";
    plabel += number;
    setWindowTitle(qt(plabel));

    /*
     * Length Change.
     */

    m_fixlength_group = new QButtonGroup(this);
    m_fixlength_group->addButton(ui->btn_change_none, cast(lengthfix::none));
    m_fixlength_group->addButton
    (
        ui->btn_change_pick, cast(lengthfix::measures)
    );
    m_fixlength_group->addButton
    (
        ui->btn_change_scale, cast(lengthfix::rescale)
    );
    ui->btn_change_none->setChecked(true);
    connect
    (
        m_fixlength_group, QT5_HELPER_RADIO_SIGNAL,
        [=](int id) { slot_length_fix(id); }        /* lambda slot function */
    );

    std::string temp = std::to_string(m_measures);
    ui->line_edit_pick->setText(qt(temp));
    connect
    (
        ui->line_edit_pick, SIGNAL(valueChanged(int)),
        this, SLOT(slot_measure_change())
    );
    temp = std::to_string(1);                       /* actually a float     */
    ui->line_edit_scale->setText(qt(temp));
    connect
    (
        ui->line_edit_pick, SIGNAL(editingFinished()),
        this, SLOT(slot_scale_change())
    );
    ui->line_edit_none->hide();                     /* not used, hide it    */

    /*
     * Effect.  Meant to be read-only, to show the "effect" of the current
     * settings.  Enforced in the qpatternfix.ui form.
     */

     ui->group_box_effect->setEnabled(true);
     ui->btn_effect_shift->setChecked(false);
     ui->btn_effect_shrink->setChecked(false);
     ui->btn_effect_expand->setChecked(false);

    /*
     * Quantization.
     */

    m_quan_group = new QButtonGroup(this);
    ui->group_box_quantize->setEnabled(true);
    m_quan_group->addButton(ui->btn_quan_none, cast(quantization::none));
    m_quan_group->addButton(ui->btn_quan_tighten, cast(quantization::tighten));
    m_quan_group->addButton(ui->btn_quan_full, cast(quantization::full));
    ui->btn_quan_none->setChecked(true);
    connect
    (
        m_quan_group, QT5_HELPER_RADIO_SIGNAL,
        [=](int id) { slot_quan_change(id); }       /* lambda slot function */
    );

    /*
     * Other Fixes.
     */

    ui->group_box_other->setEnabled(true);
    ui->btn_align_left->setChecked(false);
    connect
    (
        ui->btn_align_left, SIGNAL(stateChanged(int)),
        this, SLOT(slot_align_change(int))
    );

    /*
     * Bottom buttons.
     */

    ui->btn_set->setEnabled(false);
    ui->btn_reset->setEnabled(false);
    ui->btn_close->setEnabled(true);

    connect(ui->btn_set, SIGNAL(clicked()), this, SLOT(slot_set()));
    connect(ui->btn_reset, SIGNAL(clicked()), this, SLOT(slot_reset()));
    connect(ui->btn_close, SIGNAL(clicked()), this, SLOT(close()));
}

/**
 *  Deletes the user-interface object.
 */

qpatternfix::~qpatternfix()
{
    delete ui;
    if (m_is_modified)
        perf().modify();
}

void
qpatternfix::modify ()
{
    ui->btn_set->setEnabled(true);
    ui->btn_reset->setEnabled(true);
    m_is_modified = true;
}

void
qpatternfix::unmodify ()
{
    ui->btn_set->setEnabled(false);
    ui->btn_reset->setEnabled(false);
    m_is_modified = false;
}

void
qpatternfix::slot_length_fix (int fixlengthid)
{
    m_length_type = lengthfix_cast(fixlengthid);
    if (m_length_type != lengthfix::none)
        modify();
}

void
qpatternfix::slot_measure_change (int measures)
{
    bool changed = measures != seqp()->get_measures();
    seqp()->apply_length(measures);             /* use the simpler overload */
    if (changed)
        modify();
}

void
qpatternfix::slot_scale_change ()
{
    bool ok;
    QString t = ui->line_edit_pick->text();
    double v = t.toDouble(&ok);
    if (ok)
    {
        ok = v >= c_scale_min && v <= c_scale_max;
        if (ok)
        {
            bool changed = v != m_scale_factor;
            if (changed)
            {
                m_scale_factor = v;
                modify();
            }
        }
    }
}

void
qpatternfix::slot_quan_change (int quanid)
{
    m_quan_type = quantization_cast(quanid);
    if (m_quan_type != quantization::none)
        modify();
}

void
qpatternfix::slot_align_change (int state)
{
    bool is_set = state == Qt::Checked;
    bool changed = is_set != m_align_left;
    if (changed)
    {
        m_align_left = is_set;
        modify();
    }
}

void
qpatternfix::slot_set ()
{
    effect_t efx = effect_none;
    bool success = seqp()->fix_pattern
    (
        m_length_type, m_measures, m_scale_factor,
        m_quan_type, m_align_left, efx
    );
    if (success)
    {
        ui->btn_effect_shift->setChecked(efx & effect_shifted);
        ui->btn_effect_shrink->setChecked(efx & effect_shrunk);
        ui->btn_effect_expand->setChecked(efx & effect_expanded);
        seqp()->set_dirty();                            /* for redrawing    */
        m_seqdata.set_dirty();                          /* for redrawing    */
        unmodify();
    }
}

void
qpatternfix::slot_reset ()
{
    seqp()->apply_length(m_backup_measures);            /* simple overload  */
    seqp()->events() = m_backup_events;
    seqp()->set_dirty();                                /* for redrawing    */
    m_seqdata.set_dirty();                              /* for redrawing    */
    unmodify();
}

void
qpatternfix::closeEvent (QCloseEvent * event)
{
    if (not_nullptr(m_edit_frame))
        m_edit_frame->remove_patternfix_frame();

    if (m_is_modified)
        perf().modify();

    event->accept();
}

}               // namespace seq66

/*
 * qpatternfix.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


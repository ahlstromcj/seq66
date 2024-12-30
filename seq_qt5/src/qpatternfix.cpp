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
 * \updates       2024-12-30
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
 */

#include <QButtonGroup>

#include "seq66-config.h"               /* defines SEQ66_QMAKE_RULES        */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "qpatternfix.hpp"              /* seq66::qpatternfix class         */
#include "qseqdata.hpp"                 /* seq66::qseqdata for status, CC   */
#include "qstriggereditor.hpp"          /* seq66::qstriggereditor class     */
#include "qseqeditframe64.hpp"          /* seq66::qseqeditframe64, parent   */
#include "qt5_helper.h"                 /* QT5_HELPER_RADIO_SIGNAL macro    */
#include "qt5_helpers.hpp"              /* seq66::qt() string conversion    */
#include "util/strfunctions.hpp"        /* seq66::string_to_double()        */

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

/*
 *  Signal buttonClicked(int) is overloaded in this class. To connect to this
 *  signal by using the function pointer syntax, Qt provides a convenient
 *  helper for obtaining the function pointer as used below, using a lambda
 *  function.
 */

qpatternfix::qpatternfix
(
    performer & p,
    sequence & s,
    qseqeditframe64 * editparent,
    QWidget * parent
) :
    QFrame              (parent),
    ui                  (new Ui::qpatternfix),
    m_fixlength_group   (nullptr),
    m_alt_group         (nullptr),
    m_performer         (p),
    m_seq               (s),                        /* track() accessor     */
    m_backup_events     (s.events()),               /* for slot_reset()     */
    m_backup_measures   (s.get_measures()),
    m_backup_beats      (s.get_beats_per_bar()),
    m_backup_width      (s.get_beat_width()),
    m_edit_frame        (editparent),
    m_length_type       (lengthfix::none),          /* lengthfix fp_fixtype */
    m_alt_type          (alteration::none),         /* alteration fp_...    */
    m_tighten_range     (s.snap() / 2),
    m_full_range        (s.snap()),
    m_random_range      (usr().randomization_amount()),
    m_jitter_range      (usr().jitter_range(s.get_ppqn() / 4)), /* fp_...   */
    m_reverse_notemap   (false),
    m_notemap_file      (rc().notemap_filename()),
    m_measures          (double(m_backup_measures)), /* fp_measures         */
    m_scale_factor      (1.0),                      /* fp_scale_factor      */
    m_align_left        (false),                    /* bool fp_align_left   */
    m_reverse           (false),                    /* bool fp_reverse      */
    m_reverse_in_place  (false),                    /* bool fp_reverse_...  */
    m_save_note_length  (false),                    /* bool fp_save_note... */
    m_use_time_sig      (false),                    /* bool fp_use_time_... */
    m_time_sig_beats    (0),                        /* int fp_beats_per_bar */
    m_time_sig_width    (0),                        /* int fp_beat_width    */
    m_is_modified       (false),
    m_was_clean         (! s.modified())
{
    ui->setupUi(this);
    initialize(true);
    setFixedSize(width(), height());
}

/**
 *  Deletes the user-interface object.
 */

qpatternfix::~qpatternfix()
{
    delete ui;
}

void
qpatternfix::initialize (bool startup)
{
    std::string value = std::to_string(int(track().get_length()));
    ui->label_pulses->setText(qt(value));
    value = std::to_string(int(m_measures));
    ui->line_edit_measures->setText(qt(value));
    value = std::to_string(1);                          /* actually a float */
    ui->line_edit_scale->setText(qt(value));
    ui->btn_effect_alteration->setEnabled(false);
    ui->btn_effect_shift->setEnabled(false);
    ui->btn_effect_reverse->setEnabled(false);
    ui->btn_effect_shrink->setEnabled(false);
    ui->btn_effect_expand->setEnabled(false);
    ui->btn_effect_time_sig->setEnabled(false);
    ui->btn_effect_truncate->setEnabled(false);

    /*
     * ui->btn_effect_alteration->setChecked(false);
     * ui->btn_effect_shift->setChecked(false);
     * ui->btn_effect_reverse->setChecked(false);
     * ui->btn_effect_shrink->setChecked(false);
     * ui->btn_effect_expand->setChecked(false);
     * ui->btn_effect_time_sig->setChecked(false);
     * ui->btn_effect_truncate->setChecked(false);
     */

    slot_effect_clear();
    ui->btn_align_left->setChecked(false);
    ui->btn_reverse->setChecked(false);
    ui->btn_reverse_in_place->setChecked(false);
    ui->btn_save_note_length->setChecked(false);
    ui->btn_set->setEnabled(false);
    ui->btn_reset->setEnabled(false);
    if (startup)
    {
        /*
         * Read-only labelling. Pattern number and pattern length.
         */

        std::string value = std::to_string(int(track().seq_number()));
        ui->label_pattern->setText(qt(value));

        std::string plabel = "Pattern #";
        plabel += value;
        setWindowTitle(qt(plabel));

        /*
         * Length (Measures or Scaling) Change.
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
            [=](int id) { slot_length_fix(id); }        /* lambda function  */
        );
        connect
        (
            ui->line_edit_measures, SIGNAL(editingFinished()),
            this, SLOT(slot_measure_change())
        );
        connect
        (
            ui->line_edit_scale, SIGNAL(editingFinished()),
            this, SLOT(slot_scale_change())
        );
        ui->line_edit_none->hide();                     /* unused, hide it  */

        /*
         * Alteration. None, tighten, quantize, jitter, random, and notemap,
         * all mutually exclusive.
         */

        m_alt_group = new QButtonGroup(this);
        ui->group_box_quantize->setEnabled(true);

        /*
         * None
         */

        m_alt_group->addButton(ui->btn_alt_none, cast(alteration::none));

        /*
         * Tighten
         */

        m_alt_group->addButton
        (
            ui->btn_alt_tighten, cast(alteration::tighten)
        );

        /*
         * Quantize
         */

        m_alt_group->addButton(ui->btn_alt_full, cast(alteration::quantize));

        /*
         * Jitter
         */

        m_alt_group->addButton(ui->btn_alt_jitter, cast(alteration::jitter));

        /*
         * Random
         */

        m_alt_group->addButton(ui->btn_alt_random, cast(alteration::random));

        /*
         * Note-map and Reverse Note-map
         */

        m_alt_group->addButton(ui->btn_alt_notemap, cast(alteration::notemap));
        m_alt_group->addButton
        (
            ui->btn_alt_rev_notemap, cast(alteration::rev_notemap)
        );

        /*
         * Enable, disable, or hide the corresponding group items.
         * Currently the jitter edit value is used.
         *
         *  ui->line_edit_alt_jitter->hide();
         */

        ui->btn_alt_none->setChecked(true);
        ui->line_edit_alt_none->hide();     /* reveal once code in place    */
        connect
        (
            m_alt_group, QT5_HELPER_RADIO_SIGNAL,
            [=](int id) { slot_alt_change(id); }        /* lambda function  */
        );

        /*
         * TODO: Add similar slots for Tighten, Full (quantization),
         * Random, and maybe a button to load a note-map file.
         *
         *  We want to be able to change from the snap values for quantization.
         */

        value = std::to_string(m_tighten_range);
        ui->line_edit_alt_tighten->setText(qt(value));
        connect
        (
            ui->line_edit_alt_tighten, SIGNAL(editingFinished()),
            this, SLOT(slot_tighten_change())
        );
        value = std::to_string(m_full_range);
        ui->line_edit_alt_full->setText(qt(value));
        connect
        (
            ui->line_edit_alt_full, SIGNAL(editingFinished()),
            this, SLOT(slot_full_change())
        );
        value = std::to_string(m_random_range);
        ui->line_edit_alt_random->setText(qt(value));
        connect
        (
            ui->line_edit_alt_random, SIGNAL(editingFinished()),
            this, SLOT(slot_random_change())
        );
        value = std::to_string(m_jitter_range);
        ui->line_edit_alt_jitter->setText(qt(value));
        connect
        (
            ui->line_edit_alt_jitter, SIGNAL(editingFinished()),
            this, SLOT(slot_jitter_change())
        );
        connect
        (
            ui->btn_notemap_file, SIGNAL(clicked()),
            this, SLOT(slot_notemap_file())
        );

        /*
         * The "read-only" Effect group.  The slot here merely keeps them from
         * being checked by the user.  If we make them readonly the text is
         * difficult to read in many Qt themes.
         */

        connect
        (
            ui->btn_effect_alteration, SIGNAL(clicked()),
            this, SLOT(slot_effect_clear())
        );
        connect
        (
            ui->btn_effect_shift, SIGNAL(clicked()),
            this, SLOT(slot_effect_clear())
        );
        connect
        (
            ui->btn_effect_reverse, SIGNAL(clicked()),
            this, SLOT(slot_effect_clear())
        );
        connect
        (
            ui->btn_effect_shrink, SIGNAL(clicked()),
            this, SLOT(slot_effect_clear())
        );
        connect
        (
            ui->btn_effect_expand, SIGNAL(clicked()),
            this, SLOT(slot_effect_clear())
        );
        connect
        (
            ui->btn_effect_time_sig, SIGNAL(clicked()),
            this, SLOT(slot_effect_clear())
        );
        connect
        (
            ui->btn_effect_truncate, SIGNAL(clicked()),
            this, SLOT(slot_effect_clear())
        );

        /*
         * Other Fixes.
         */

        ui->group_box_effect->setEnabled(true);
        ui->group_box_other->setEnabled(true);
        connect
        (
            ui->btn_align_left, SIGNAL(stateChanged(int)),
            this, SLOT(slot_align_change(int))
        );
        connect
        (
            ui->btn_reverse, SIGNAL(stateChanged(int)),
            this, SLOT(slot_reverse_change(int))
        );
        connect
        (
            ui->btn_reverse_in_place, SIGNAL(stateChanged(int)),
            this, SLOT(slot_reverse_in_place(int))
        );
        connect
        (
            ui->btn_save_note_length, SIGNAL(stateChanged(int)),
            this, SLOT(slot_save_note_length(int))
        );

        /*
         * Bottom buttons.
         */

        ui->btn_close->setEnabled(true);
        connect(ui->btn_set, SIGNAL(clicked()), this, SLOT(slot_set()));
        connect(ui->btn_reset, SIGNAL(clicked()), this, SLOT(slot_reset()));
        connect(ui->btn_close, SIGNAL(clicked()), this, SLOT(close()));
    }
    else
    {
        /*
         * Grouped buttons.
         */

        ui->btn_change_none->setChecked(true);
        ui->btn_alt_none->setChecked(true);
    }
}

void
qpatternfix::modify ()
{
    ui->btn_set->setEnabled(true);
    ui->btn_reset->setEnabled(true);
    m_is_modified = true;
}

/**
 *  All this does is ...
 *  because we don't want to force the user to have to modify anything to
 *  Set the values again.
 */

void
qpatternfix::unmodify (bool reset_fields)
{
    if (reset_fields)
    {
        std::string temp = std::to_string(track().get_measures());
        ui->line_edit_measures->setText(qt(temp));
        ui->line_edit_scale->setText("1.0");
        ui->btn_reset->setEnabled(false);

        /*
         * ui->btn_effect_shift->setChecked(false);
         * ui->btn_effect_shrink->setChecked(false);
         * ui->btn_effect_expand->setChecked(false);
         */

        slot_effect_clear();
    }
    m_is_modified = false;
}

void
qpatternfix::slot_effect_clear ()
{
    ui->btn_effect_alteration->setChecked(false);
    ui->btn_effect_shift->setChecked(false);
    ui->btn_effect_reverse->setChecked(false);
    ui->btn_effect_shrink->setChecked(false);
    ui->btn_effect_expand->setChecked(false);
    ui->btn_effect_time_sig->setChecked(false);
    ui->btn_effect_truncate->setChecked(false);
}

void
qpatternfix::slot_length_fix (int fixlengthid)
{
    m_length_type = lengthfix_cast(fixlengthid);
    if (m_length_type != lengthfix::none)
        modify();
}

/**
 *  The user can enter either an integer measure count to set the measures
 *  directly, or a fraction of the form x/y to scale the number of measures.
 *  It sets the lengthfix::measures item rather than lengthfix::rescale.
 *
 *  Tricky: If setting integer measures, the end effect is scaling, not
 *  truncating. In fact, the only difference between measure and scale-factor
 *  changes is minimal.
 */

void
qpatternfix::slot_measure_change ()
{
    QString t = ui->line_edit_measures->text();
    std::string tc = t.toStdString();
    double m = string_to_double(tc, 1.0);       /* measures is a float here */
    if (sequence::valid_scale_factor(m, true))  /* applies to measures, too */
    {
        int beats, width;
        bool is_fraction = string_to_time_signature(tc, beats, width);
        bool different = fnotequal(m, m_measures);
        if (different || is_fraction)
        {
            ui->btn_change_pick->setChecked(true);
            m_length_type = lengthfix::measures;
            if (beats > 0 && beats < 96)            /* just a sanity check  */
                m_time_sig_beats = beats;           /* fraction numerator   */

            if (width > 0 && width < 96)            /* just a sanity check  */
                m_time_sig_width = width;           /* fraction denominator */

            m_use_time_sig = is_fraction;
            ui->btn_effect_time_sig->setChecked(is_fraction);

            midipulse curlength = track().get_length();
            midipulse newlength = midipulse(track().unit_measure() * m);
            bool shrunk = newlength < curlength;
            bool expanded = newlength > curlength;
            ui->btn_effect_shrink->setChecked(shrunk);
            ui->btn_effect_expand->setChecked(expanded);
            m_scale_factor = m / m_measures;
            m_measures = trunc_measures(m);         /* float truncation     */

            std::string scale = double_to_string(m_scale_factor, 2);
            std::string meass = double_to_string(m_measures);
            ui->line_edit_scale->setText(qt(scale));
            ui->line_edit_measures->setText(qt(meass));
            modify();                               /* indicate fix-change  */
        }
    }
    else
    {
        int beats, width;
        bool is_time_sig = string_to_time_signature(tc, beats, width);
        ui->btn_effect_time_sig->setChecked(is_time_sig);
    }
}

void
qpatternfix::slot_scale_change ()
{
    QString t = ui->line_edit_scale->text();
    std::string tc = t.toStdString();
    double v = string_to_double(tc, 1.0);
    if (sequence::valid_scale_factor(v) && v != m_scale_factor)
    {
        ui->btn_change_scale->setChecked(true);
        m_scale_factor = v;
        m_measures = trunc_measures(m_measures * v);
        m_length_type = lengthfix::rescale;
        m_time_sig_beats = m_time_sig_width = 0;
        m_use_time_sig = false;
        ui->btn_effect_time_sig->setChecked(false);

        std::string scale = double_to_string(m_scale_factor, 2);
        std::string meass = double_to_string(m_measures);
        ui->line_edit_scale->setText(qt(scale));
        ui->line_edit_measures->setText(qt(meass));
        modify();
    }
}

void
qpatternfix::slot_alt_change (int quanid)
{
    alteration quantype = quantization_cast(quanid);
    if (m_alt_type != quantype)
    {
        bool not_none = quantype != alteration::none;
        m_alt_type = quantype;
        ui->btn_effect_alteration->setChecked(not_none);
        modify();
    }
}

void
qpatternfix::slot_tighten_change ()
{
    QString t = ui->line_edit_alt_tighten->text();
    std::string tc = t.toStdString();
    int m = string_to_int(tc, 0);
    if (m > 0 && m < track().get_ppqn())        /* sanity check */
    {
        ui->btn_alt_tighten->setChecked(true);
        m_tighten_range = m;
        m_alt_type = alteration::tighten;
        modify();
    }
}

void
qpatternfix::slot_full_change ()
{
    QString t = ui->line_edit_alt_full->text();
    std::string tc = t.toStdString();
    int m = string_to_int(tc, 0);
    if (m > 0 && m < track().get_ppqn())        /* sanity check */
    {
        ui->btn_alt_full->setChecked(true);
        m_full_range = m;
        m_alt_type = alteration::quantize;
        modify();
    }
}

void
qpatternfix::slot_random_change ()
{
    QString t = ui->line_edit_alt_random->text();
    std::string tc = t.toStdString();
    int m = string_to_int(tc, 0);
    if (m > 0 && m < track().get_ppqn())        /* sanity check */
    {
        ui->btn_alt_random->setChecked(true);
        m_random_range = m;
        m_alt_type = alteration::random;
        modify();
    }
}

void
qpatternfix::slot_jitter_change ()
{
    QString t = ui->line_edit_alt_jitter->text();
    std::string tc = t.toStdString();
    int m = string_to_int(tc, 0);
    if (m > 0 && m < track().get_ppqn())        /* sanity check */
    {
        ui->btn_alt_jitter->setChecked(true);
        m_jitter_range = m;
        m_alt_type = alteration::jitter;
        modify();
    }
}

void
qpatternfix::slot_notemap_file ()
{
    printf("Current note-map file: '%s'\n", m_notemap_file.c_str());
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
qpatternfix::slot_reverse_change (int state)
{
    bool is_set = state == Qt::Checked;
    bool changed = is_set != m_reverse;
    if (changed)
    {
        m_reverse = is_set;
        if (is_set)
            ui->btn_reverse_in_place->setChecked(false);

        modify();
    }
}

void
qpatternfix::slot_reverse_in_place (int state)
{
    bool is_set = state == Qt::Checked;
    bool changed = is_set != m_reverse_in_place;
    if (changed)
    {
        m_reverse_in_place = is_set;
        if (is_set)
            ui->btn_reverse->setChecked(false);

        modify();
    }
}

void
qpatternfix::slot_save_note_length (int state)
{
    bool is_set = state == Qt::Checked;
    bool changed = is_set != m_save_note_length;
    if (changed)
    {
        m_save_note_length = is_set;
        modify();
    }
}

/**
 *  A convenience/encapsulation function to trigger redrawing.
 */

void
qpatternfix::set_dirty ()
{
    track().set_dirty();
    if (not_nullptr(m_edit_frame))
        m_edit_frame->set_dirty();
}

void
qpatternfix::slot_set ()
{
    fixeffect efx;
    fixparameters fp =                                  /* value structure  */
    {
        m_length_type, m_alt_type,
        m_tighten_range, m_full_range, m_random_range, m_jitter_range,
        m_align_left, m_reverse, m_reverse_in_place,
        m_save_note_length, m_use_time_sig, m_time_sig_beats,
        m_time_sig_width, m_measures, m_scale_factor,
        m_notemap_file, efx
    };
    bool success = perf().fix_pattern(track().seq_number(), fp);
    if (success)
    {
        bool alteration = m_alt_type != alteration::none;
        bool bitshifted = bit_test(efx, fixeffect::shifted);
        bool bitreversed = bit_test(efx, fixeffect::reversed);
        bool bitshrunk = bit_test(efx, fixeffect::shrunk);
        bool bitexpanded = bit_test(efx, fixeffect::expanded);
        std::string temp = std::to_string(int(track().get_length()));
        ui->label_pulses->setText(qt(temp));
        if (! bitreversed)
            bitreversed = bit_test(efx, fixeffect::reversed_abs);

        if (m_use_time_sig)
        {
            temp = std::to_string(m_time_sig_beats);
            temp += "/";
            temp += std::to_string(m_time_sig_width);
        }
        else
            temp = double_to_string(m_measures);

        ui->line_edit_measures->setText(qt(temp));
        temp = double_to_string(m_scale_factor);
        ui->line_edit_scale->setText(qt(temp));
        ui->btn_effect_alteration->setChecked(alteration);
        ui->btn_effect_shift->setChecked(bitshifted);
        ui->btn_effect_shrink->setChecked(bitshrunk);
        ui->btn_effect_expand->setChecked(bitexpanded);
        set_dirty();                                    /* for redrawing    */
        unmodify(false);                                /* keep fields      */
    }
}

void
qpatternfix::slot_reset ()
{
    track().set_beats_per_bar(m_backup_beats);          /* restore original */
    track().set_beat_width(m_backup_width);             /* ditto            */
    track().apply_length(m_backup_measures);            /* simple overload  */
    track().events() = m_backup_events;                 /* restore events   */
    m_measures = double(m_backup_measures);
    m_align_left = m_use_time_sig = false;
    m_save_note_length = false;
    m_time_sig_beats = m_time_sig_width = 0;
    m_scale_factor = 1.0;
    m_length_type = lengthfix::none;
    m_alt_type = alteration::none;
    m_tighten_range = track().snap() / 2;
    m_full_range = track().snap();
    m_random_range = usr().randomization_amount();
    m_jitter_range = usr().jitter_range(track().get_ppqn() / 4);
    m_reverse_notemap = false;
    m_notemap_file = rc().notemap_filename();
    initialize(false);
    slot_effect_clear();
    unmodify();                                         /* change fields    */
    set_dirty();                                        /* for redrawing    */
    if (m_was_clean)
        track().unmodify();
}

void
qpatternfix::closeEvent (QCloseEvent * event)
{
    if (not_nullptr(m_edit_frame))
        m_edit_frame->remove_patternfix_frame();

    event->accept();
}

}               // namespace seq66

/*
 * qpatternfix.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


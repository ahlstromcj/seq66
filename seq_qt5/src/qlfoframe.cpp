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
 * \file          qlfoframe.cpp
 *
 *  This module declares/defines the base class for the LFO window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2021-10-17
 * \license       GNU GPLv2 or above
 *
 *  The LFO (low-frequency oscillator) provides a way to modulate the
 *  selected type of event in the data.  One can inserts a whole series of
 *  pitch-wheel control events, for example, and then apply vibrato to them.
 *  Various waveforms (sine, triangle, etc.) can be applied, at varying depths
 *  and frequency of modulating, including a DC offset.
 */

#include <QButtonGroup>
#include <QSlider>

#include "seq66-config.h"               /* defines SEQ66_QMAKE_RULES        */
#include "play/performer.hpp"           /* seq66::performer class           */
#include "qlfoframe.hpp"                /* seq66::qlfoframe class           */
#include "qseqdata.hpp"                 /* seq66::qseqdata for status, CC   */
#include "qseqeditframe64.hpp"          /* seq66::qseqeditframe64, parent   */
#include "util/calculations.hpp"        /* seq66::wave enum class values    */

/*
 *  Qt's uic application allows a different output file-name, but not sure
 *  if qmake can change the file-name.
 */

#if defined SEQ66_QMAKE_RULES
#include "forms/ui_qlfoframe.h"
#else
#include "forms/qlfoframe.ui.h"
#endif

/*
 * Don't document the namespace.
 */

namespace seq66
{

/**
 *  Static members.
 */

static double s_value_min   =   0.0;
static double s_value_def   =  64.0;
static double s_value_max   = 127.0;
static double s_range_min   =   0.0;
static double s_range_def   =  64.0;
static double s_range_max   = 127.0;
static double s_speed_min   =   0.0;
static double s_speed_def   =   1.0;        /* actually number of periods   */
static double s_speed_max   =  16.0;
static double s_phase_min   =   0.0;
static double s_phase_max   =   1.0;

/*
 *  Signal buttonClicked(int) is overloaded in this class. To connect to this
 *  signal by using the function pointer syntax, Qt provides a convenient
 *  helper for obtaining the function pointer as used below, using a lambda
 *  function.
 */

qlfoframe::qlfoframe
(
    performer & p,
    seq::pointer seqp,
    qseqdata & sdata,
    qseqeditframe64 * editparent,
    QWidget * parent
) :
    QFrame          (parent),
    ui              (new Ui::qlfoframe),
    m_wave_group    (nullptr),
    m_performer     (p),
    m_seq           (seqp),
    m_seqdata       (sdata),
    m_backup_events (seqp->events()),   /* copy original events for reset() */
    m_edit_frame    (editparent),
    m_value         (s_value_def),
    m_range         (s_range_def),
    m_speed         (s_speed_def),
    m_phase         (s_phase_min),
    m_wave          (waveform::none),
    m_use_measure   (true)
{
    ui->setupUi(this);
    connect(ui->m_button_reset, SIGNAL(clicked()), this, SLOT(reset()));
    connect(ui->m_button_close, SIGNAL(clicked()), this, SLOT(close()));
    m_wave_group = new QButtonGroup(this);
    m_wave_group->addButton(ui->m_radio_wave_none, int(waveform::none));
    m_wave_group->addButton(ui->m_radio_wave_sine, int(waveform::sine));
    m_wave_group->addButton(ui->m_radio_wave_saw, int(waveform::sawtooth));
    m_wave_group->addButton(ui->m_radio_wave_revsaw, int(waveform::reverse_sawtooth));
    m_wave_group->addButton(ui->m_radio_wave_triangle, int(waveform::triangle));
    m_wave_group->addButton(ui->m_radio_wave_exp, int(waveform::exponential));
    m_wave_group->addButton
    (
        ui->m_radio_wave_revexp, int(waveform::reverse_exponential)
    );
    ui->m_radio_wave_none->setChecked(true);    /* match m_wave member init */
    connect
    (
        m_wave_group,
#if QT_VERSION < QT_VERSION_CHECK(5, 8, 0)
        static_cast<void(QButtonGroup::*) (int)>(&QButtonGroup::buttonClicked),
#elif QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
        QOverload<int>::of(&QButtonGroup::buttonClicked),
#else
        QOverload<int>::of(&QButtonGroup::idClicked),
#endif
        [=](int id) { wave_type_change(id); }
    );

    /*
     * ui->m_wave_type_group->setToolTip
     * (
     *     "Wave type: 1 = sine; 2 = ramp sawtooth; 3 = decay sawtooth; "
     *     "4 = triangle."
     * );
     */

    /*
     * Order of calls is important here.
     */

    ui->m_value_slider->setToolTip("The DC offset for modulation, 0 to 127.");
    ui->m_value_slider->setMinimum(to_slider(s_value_min));
    ui->m_value_slider->setMaximum(to_slider(s_value_max));
    ui->m_value_slider->setValue(to_slider(m_value));
    set_value_text(m_value, ui->m_value_text);
    connect
    (
        ui->m_value_slider, SIGNAL(valueChanged(int)),
        this, SLOT(scale_lfo_change())
    );
    connect
    (
        ui->m_value_text, SIGNAL(editingFinished()),
        this, SLOT(value_text_change())
    );

    /*
     * Order of calls is important here.
     */

    ui->m_range_slider->setToolTip("Controls depth of modulation, 0 to 127.");
    ui->m_range_slider->setMinimum(to_slider(s_range_min));
    ui->m_range_slider->setMaximum(to_slider(s_range_max));
    ui->m_range_slider->setValue(to_slider(m_range));
    set_value_text(m_range, ui->m_range_text);
    connect
    (
        ui->m_range_slider, SIGNAL(valueChanged(int)),
        this, SLOT(scale_lfo_change())
    );
    connect
    (
        ui->m_range_text, SIGNAL(editingFinished()),
        this, SLOT(range_text_change())
    );

    /*
     * Order of calls is important here. Also we replace the hardwired tool-tip.
     */

    ui->m_speed_slider->setToolTip
    (
        "Speed (periods): number of periods per pattern or measure.\n"
        "For long patterns, set this parameter high.  Beware of anti-aliasing.\n"
    );
    ui->m_speed_slider->setMinimum(to_slider(s_speed_min));
    ui->m_speed_slider->setMaximum(to_slider(s_speed_max));
    ui->m_speed_slider->setValue(to_slider(m_speed));
    set_value_text(m_speed, ui->m_speed_text);
    connect
    (
        ui->m_speed_slider, SIGNAL(valueChanged(int)),
        this, SLOT(scale_lfo_change())
    );
    connect
    (
        ui->m_speed_text, SIGNAL(editingFinished()),
        this, SLOT(speed_text_change())
    );

    /*
     * Order of calls is important here.
     */

    ui->m_phase_slider->setToolTip
    (
        "Phase: phase shift in a beat width (quarter note). "
        "A value of 1 is a phase shift of 360 degrees."
    );
    ui->m_phase_slider->setMinimum(to_slider(s_phase_min));
    ui->m_phase_slider->setMaximum(to_slider(s_phase_max));
    ui->m_phase_slider->setValue(to_slider(m_phase));
    set_value_text(m_phase, ui->m_phase_text);
    connect
    (
        ui->m_phase_slider, SIGNAL(valueChanged(int)),
        this, SLOT(scale_lfo_change())
    );
    connect
    (
        ui->m_phase_text, SIGNAL(editingFinished()),
        this, SLOT(phase_text_change())
    );
    ui->m_measures_check_box->setChecked(m_use_measure);
    connect
    (
        ui->m_measures_check_box, SIGNAL(stateChanged(int)),
        this, SLOT(use_measure_clicked(int))
    );

    std::string plabel = "Pattern #";
    std::string number = std::to_string(int(seqp->seq_number()));
    plabel += number;
    ui->m_pattern_label->setText(QString::fromStdString(plabel));

    plabel = "LFO #";
    plabel += number;
    setWindowTitle(QString::fromStdString(plabel));
}

/**
 *  Deletes the user-interface object.
 */

qlfoframe::~qlfoframe()
{
    delete ui;
}

/**
 *  A helper function to set the text of the slider control's text field.
 */

void
qlfoframe::set_value_text
(
    double value,
    QLineEdit * textline
)
{
    char valtext[16];
    snprintf(valtext, sizeof valtext, "%g", value);
    textline->setText(valtext);
}

/**
 *  Gets the "value" number from the text field when editing is finished (when
 *  Enter is struck.
 */

void
qlfoframe::value_text_change ()
{
    QString t = ui->m_value_text->text();
    bool ok;
    double v = t.toDouble(&ok);
    if (ok && (v >= s_value_min && v <= s_value_max))
        ui->m_value_slider->setValue(to_slider(v));
}

void
qlfoframe::range_text_change ()
{
    QString t = ui->m_range_text->text();
    bool ok;
    double v = t.toDouble(&ok);
    if (ok && (v >= s_range_min && v <= s_range_max))
        ui->m_range_slider->setValue(to_slider(v));
}

void
qlfoframe::speed_text_change ()
{
    QString t = ui->m_speed_text->text();
    bool ok;
    double v = t.toDouble(&ok);
    if (ok && (v >= s_speed_min && v <= s_speed_max))
        ui->m_speed_slider->setValue(to_slider(v));
}

void
qlfoframe::phase_text_change ()
{
    QString t = ui->m_phase_text->text();
    bool ok;
    double v = t.toDouble(&ok);
    if (ok && (v >= s_phase_min && v <= s_phase_max))
        ui->m_phase_slider->setValue(to_slider(v));
}

void
qlfoframe::wave_type_change (int waveid)
{
    m_wave = static_cast<waveform>(waveid);
    reset();
    scale_lfo_change();
}

/**
 *  Changes the scaling provided by this window.  Changes take place right
 *  away in this callback, and would require multiple undoes to fully undo.
 */

void
qlfoframe::scale_lfo_change ()
{
    m_value = to_double(ui->m_value_slider->value());
    m_range = to_double(ui->m_range_slider->value());
    m_speed = to_double(ui->m_speed_slider->value());
    m_phase = to_double(ui->m_phase_slider->value());
    m_seq->change_event_data_lfo
    (
        m_value, m_range, m_speed, m_phase, m_wave,
        m_seqdata.status(), m_seqdata.cc(), m_use_measure
    );
    m_seqdata.set_dirty();

    char tmp[16];
    snprintf(tmp, sizeof tmp, "%g", m_value);
    ui->m_value_text->setText(tmp);
    snprintf(tmp, sizeof tmp, "%g", m_range);
    ui->m_range_text->setText(tmp);
    snprintf(tmp, sizeof tmp, "%g", m_speed);
    ui->m_speed_text->setText(tmp);
    snprintf(tmp, sizeof tmp, "%g", m_phase);
    ui->m_phase_text->setText(tmp);
}

void
qlfoframe::use_measure_clicked (int state)
{
    bool usem = state == Qt::Checked;
    if (usem != m_use_measure)
    {
        m_use_measure = usem;
        // m_wave = waveform::none;
        scale_lfo_change();
    }
}

void
qlfoframe::reset ()
{
    m_seq->events() = m_backup_events;
    m_seq->set_dirty();
    m_seqdata.set_dirty();
}

void
qlfoframe::closeEvent (QCloseEvent * event)
{
    if (not_nullptr(m_edit_frame))
        m_edit_frame->remove_lfo_frame();

    event->accept();
}

}               // namespace seq66

/*
 * qlfoframe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


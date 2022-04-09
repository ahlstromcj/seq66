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
 * \updates       2022-04-09
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


/*
 *  Signal buttonClicked(int) is overloaded in this class. To connect to this
 *  signal by using the function pointer syntax, Qt provides a convenient
 *  helper for obtaining the function pointer as used below, using a lambda
 *  function.
 */

qpatternfix::qpatternfix
(
    performer & p,
    seq::pointer seqp,
    qseqdata & sdata,
    qseqeditframe64 * editparent,
    QWidget * parent
) :
    QFrame          (parent),
    ui              (new Ui::qpatternfix),
    m_performer     (p),
    m_seq           (seqp),
    m_seqdata       (sdata),
    m_is_modified   (false)
{
    ui->setupUi(this);
/*
    connect
    (
        ui->m_value_slider, SIGNAL(valueChanged(int)),
        this, SLOT(pattern_change())
    );
    std::string plabel = "Pattern #";
    std::string number = std::to_string(int(seqp->seq_number()));
    plabel += number;
    ui->m_pattern_label->setText(qt(plabel));

    plabel = "LFO #";
    plabel += number;
    setWindowTitle(qt(plabel));
    */
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

/**
 *  Changes the scaling provided by this window.  Changes take place right
 *  away in this callback, and would require multiple undoes to fully undo.
 */

void
qpatternfix::pattern_change ()
{
    /*
    m_seq->change_event_data_lfo
    (
        m_value, m_range, m_speed, m_phase, m_wave,
        m_seqdata.status(), m_seqdata.cc(), m_use_measure
    );
    */
    m_seqdata.set_dirty();
    m_is_modified = true;
}

void
qpatternfix::reset ()
{
    m_seq->events() = m_backup_events;
    m_seq->set_dirty();
    m_seqdata.set_dirty();
    m_is_modified = false;
}

void
qpatternfix::closeEvent (QCloseEvent * event)
{
/// if (not_nullptr(m_edit_frame))
///     m_edit_frame->remove_lfo_frame();

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


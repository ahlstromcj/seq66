#if ! defined SEQ66_QPATTERNFIX_HPP
#define SEQ66_QPATTERNFIX_HPP

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
 * \file          qpatternfix.hpp
 *
 *  This module declares/defines the base class for the pattern-fix window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2022-04-09
 * \updates       2022-04-10
 * \license       GNU GPLv2 or above
 *
 *  Provides a way to modulate MIDI controller events.
 */

#include <QFrame>

#include "play/seq.hpp"                 /* seq66::seq::pointer & sequence   */
#include "util/calculations.hpp"        /* seq66::lengthfix enum class type */

/*
 *  Forward declarations for Qt.
 */

class QButtonGroup;
class QLineEdit;

/*
 * This is necessary to keep the compiler from thinking Ui::qpatternfix
 * would be found in the seq66 namespace.
 */

namespace Ui
{
    class qpatternfix;
}

/*
 * Do not document a namespace.  It breaks Doxygen.
 */

namespace seq66
{
    class performer;
    class qseqdata;
    class qseqeditframe64;

/**
 *  This class is the Qt 5 version of the lfownd class. It has one important
 *  difference, in that the wave type is chosen via radio-buttons, rather than
 *  a slider.  And the numbers can be edited directly.
 */

class qpatternfix final : public QFrame
{
    Q_OBJECT

public:

    qpatternfix
    (
        performer & p,
        seq::pointer seqp,
        qseqdata & sdata,
        qseqeditframe64 * editparent    = nullptr,
        QWidget * parent                = nullptr
    );
    virtual ~qpatternfix ();

    /*
     * Not yet implemented !
     *
     * void toggle_visible ();
     */

    bool modified () const
    {
        return m_is_modified;
    }

    void modify ();
    void unmodify ();

protected:

    virtual void closeEvent (QCloseEvent *) override;

private:

    performer & perf ()
    {
        return m_performer;
    }

    seq::pointer seqp ()
    {
        return m_seq;
    }

    void set_value_text (double value, QLineEdit * textline);
    void wave_type_change (int waveid);

signals:

private slots:

    /*
     *  We use lambda functions for the slot for the QButtonGroup ::
     *  buttonClicked() signals.
     */

    void slot_length_fix (int fixlengthid);
    void slot_measure_change (int len);
    void slot_scale_change ();
    void slot_quan_change (int quanid);
    void slot_align_change (int dummy);
    void slot_set ();
    void slot_reset ();

private:

    /**
     *  The use Qt user-interface object pointer.
     */

    Ui::qpatternfix * ui;

    /**
     * To access the radio-buttons in the GroupBox, ui->group_box_length, we
     * need to create a button-group.
     */

    QButtonGroup * m_fixlength_group;

    /**
     * Access to radio-buttons for quantization.
     */

    QButtonGroup * m_quan_group;

    /**
     *  Access to the performance controller.
     */

    performer & m_performer;

    /**
     *  The sequence associated with this window.
     */

    seq::pointer m_seq;

    /**
     *  The qseqdata associated with this window.
     */

    qseqdata & m_seqdata;

    /**
     *  Holds the original data in order to allow for a complete undo of the
     *  changes.
     */

    eventlist m_backup_events;

    /**
     *  Holds the original pattern length in measures.
     */

    int m_backup_measures;

    /**
     *  The seqedit frame that owns (sort of) this LFO window.
     */

    qseqeditframe64 * m_edit_frame;

    /**
     *  The current way the user has selected to fix the length.
     */

    lengthfix m_length_type;

    /**
     *  The current way the user has selected for quantization.
     */

    quantization m_quan_type;

    /**
     *  The current number of measures for the adjustment.
     */

    int m_measures;

    /**
     *  The current scale factor in the user-interface.
     */

    double m_scale_factor;

    /**
     *  Indicates if left-alignment of the pattern is specified.
     */

    bool m_align_left;

    /**
     *  Indicates the LFO modified status.
     */

    bool m_is_modified;

};          // class qpatternfix

}           // namespace seq66

#endif // SEQ66_QPATTERNFIX_HPP

/*
 * qpatternfix.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


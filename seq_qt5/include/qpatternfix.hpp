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
 * \updates       2025-06-14
 * \license       GNU GPLv2 or above
 *
 *  Provides a way to modulate MIDI controller events.
 */

#include <QFrame>

#include "midi/calculations.hpp"        /* seq66::lengthfix, alteration     */
#include "play/seq.hpp"                 /* seq66::seq::pointer & sequence   */

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
    class qstriggereditor;

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
        sequence & s,
        qseqeditframe64 * editparent,
        QWidget * parent = nullptr
    );
    virtual ~qpatternfix ();

    bool modified () const
    {
        return m_is_modified;
    }

    void modify ();
    void unmodify (bool reset_fields = true);

protected:

    virtual void closeEvent (QCloseEvent *) override;

private:

    performer & perf ()
    {
        return m_performer;
    }

    sequence & track ()
    {
        return m_seq;
    }

    void set_dirty ();
    void set_value_text (double value, QLineEdit * textline);
    void wave_type_change (int waveid);
    void initialize (bool startup);

signals:

private slots:

    /*
     *  We use lambda functions for the slot for the QButtonGroup ::
     *  buttonClicked() signals.
     */

    void slot_effect_clear ();
    void slot_length_fix (int fixlengthid);
    void slot_measure_change ();
    void slot_scale_change ();
    void slot_alt_change (int altid);
    void slot_tighten_change ();
    void slot_full_change ();
    void slot_jitter_change ();
    void slot_random_change ();
    void slot_random_pitch_change ();
    void slot_notemap_file ();
    void slot_align_left_change (int dummy);
    void slot_align_right_change (int dummy);
    void slot_reverse_change (int dummy);
    void slot_reverse_in_place (int dummy);
    void slot_save_note_length (int dummy);
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
     * Access to radio-buttons for alteration.
     */

    QButtonGroup * m_alt_group;

    /**
     *  Access to the performance controller.
     */

    performer & m_performer;

    /**
     *  The sequence associated with this window.
     */

    sequence & m_seq;

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
     *  Holds the original beats per bar.
     */

    int m_backup_beats;

    /**
     *  Holds the original beat width.
     */

    int m_backup_width;

    /**
     *  The seqedit frame that owns (sort of) this LFO window.
     */

    qseqeditframe64 * m_edit_frame;

    /**
     *  The current way the user has selected to fix the length.
     */

    lengthfix m_length_type;

    /**
     *  The current way the user has selected for alteration.
     */

    alteration m_alt_type;

    /**
     *  The range of tightening to apply. Normally this is snap() / 2.
     */

    int m_tighten_range;

    /**
     *  The range of full quantization to apply. Normally this is snap().
     */

    int m_full_range;

    /**
     *  The range of amplitude randomization to apply. For control events,
     *  this is a magnitude of the control. For notes, it is the velocity.
     *  Program change need not apply. :-)
     */

    int m_random_range;

    /**
     *  The range of note randomization to apply to note events.
     */

    int m_pitch_range;

    /**
     *  The range of jitter to apply.  Here, jitter is a randomization of
     *  the event time-stamp by plus/minus a value in the range of the jitter.
     *  Defaults to usr().jitter_range(ppqn() / 4).
     */

    int m_jitter_range;

    /**
     *  Holds the file-name of the notemap file, which is a '.drums' file,
     *  but we also want to support '.notemap'.
     */

    std::string m_notemap_file;

    /**
     *  If true, use reverse-mapping via the note-map file.
     */

    bool m_reverse_notemap;

    /**
     *  The current number of measures for the adjustment.  This is a double
     *  so that it can be fractional, such as "3/4" --> 0.75. But otherwise,
     *  it must be an integer (truncated) value.
     */

    double m_measures;

    /**
     *  The current scale factor in the user-interface.
     */

    double m_scale_factor;

    /**
     *  Indicates if left-alignment of the pattern is specified.
     */

    bool m_align_left;

    /**
     *  Indicates if right-alignment of the pattern is specified.
     */

    bool m_align_right;

    /**
     *  Reverses the timestamps of event, while preserving the duration of the
     *  notes. The new timestamp is the distance of the event from the end
     *  (length) of the pattern, which we call the "reference".
     */

    bool m_reverse;

    /**
     *  Similar to reverse, except that the last event is used as the
     *  "reference" (instead of the pattern length).
     */

    bool m_reverse_in_place;

    /**
     *  Indicates to preserve note length when rescaling. Otherwise, the
     *  end-time of the note is scaled as well.
     */

    bool m_save_note_length;

    /**
     *  Indicates to treat the measures text like a time signature.
     *  Triggered by the presence of a "/" and valid beats and width.
     */

    bool m_use_time_sig;

    /**
     *  Time signature beats.
     */

    int m_time_sig_beats;

    /**
     *  Time signature beats.
     */

    int m_time_sig_width;

    /**
     *  Indicates the modified status of the user interface.
     *  The performer::modify() status is called only when the Set
     *  button is pushed.
     */

    bool m_is_modified;

    /**
     *  Indicates if the pattern was modified before the dialog was opened.
     *  If not, it is safe to unmodify it in slot_reset().
     */

    bool m_was_clean;

};          // class qpatternfix

}           // namespace seq66

#endif // SEQ66_QPATTERNFIX_HPP

/*
 * qpatternfix.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


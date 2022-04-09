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
 * \updates       2022-04-09
 * \license       GNU GPLv2 or above
 *
 *  Provides a way to modulate MIDI controller events.
 */

#include <QFrame>

#include "play/seq.hpp"                 /* seq66::seq::pointer & sequence   */
#include "util/calculations.hpp"        /* seq66::waveform enum class type  */

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

protected:

    virtual void closeEvent (QCloseEvent *) override;

private:

    performer & perf ()
    {
        return m_performer;
    }

    void set_value_text (double value, QLineEdit * textline);
    void wave_type_change (int waveid);

signals:

private slots:

    /*
     *  We use a lambda function for the slot for the QButtonGroup ::
     *  buttonClicked() signal that sets the wave-form to use.
     */

    void pattern_change ();
    void reset ();

private:

    /**
     *  The use Qt user-interface object pointer.
     */

    Ui::qpatternfix * ui;

    /**
     *  Provides a way to treat the wave radio-buttons as a group.  Had issues
     *  trying to set this up in Qt Creator. To get the checked value,
     *  use its checkedButton() function.
     */

    QButtonGroup * m_groupppppp;

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
     *  The seqedit frame that owns (sort of) this LFO window.
     */

    qseqeditframe64 * m_edit_frame;

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


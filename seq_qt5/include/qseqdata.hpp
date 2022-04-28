#if ! defined SEQ66_QSEQDATA_HPP
#define SEQ66_QSEQDATA_HPP

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
 * \file          qseqdata.hpp
 *
 *  This module declares/defines the base class for plastering
 *  pattern/sequence data information in the data area of the pattern editor.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2022-04-28
 * \license       GNU GPLv2 or above
 *
 *  The data pane is the drawing-area below the seqedit's event area, and
 *  contains vertical lines whose height matches the value of each data event.
 *  The height of the vertical lines is editable via the mouse.
 */

#include <QWidget>
#include <QTimer>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>

#include "midi/midibytes.hpp"           /* midibyte, midipulse aliases      */
#include "qseqbase.hpp"                 /* seq66::qseqbase mixin class      */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class performer;

/**
 *  Displays the data values for MIDI events such as Mod Wheel and Pitchbend.
 *  They are displayed as vertical lines with an accompanying numeric value.
 */

class qseqdata final :
    public QWidget,
    public qseqbase,
    protected performer::callbacks
{
    friend class qseqroll;
    friend class qstriggereditor;

    Q_OBJECT

public:

    qseqdata
    (
        performer & p,
        sequence & s,
        qseqeditframe64 * frame,
        int zoom, int snap,
        QWidget * parent,
        int height = 0
    );

    virtual ~qseqdata ();

    void set_data_type (midibyte a_status, midibyte a_control);

    bool is_tempo () const
    {
        return m_is_tempo;
    }

    void is_tempo (bool flag)
    {
        m_is_tempo = flag;
    }

    bool is_program_change () const
    {
        return m_is_program_change;
    }

    void is_program_change (bool flag)
    {
        m_is_program_change = flag;
    }

    midibyte status () const
    {
        return m_status;
    }

    midibyte cc () const
    {
        return m_cc;
    }

private:        // performer::callback overrides

    virtual bool on_ui_change (seq::number seqno) override;

protected:

    virtual void paintEvent (QPaintEvent * ) override;
    virtual void resizeEvent (QResizeEvent *) override;
    virtual void mousePressEvent (QMouseEvent *) override;
    virtual void mouseReleaseEvent (QMouseEvent *) override;
    virtual void mouseMoveEvent (QMouseEvent *) override;
    virtual QSize sizeHint () const override;

private:

    virtual void update_midi_buttons () override
    {
        // no code needed, no buttons or statuses to update at this time
    }

signals:

private slots:

    void conditional_update ();

private:

    QTimer * m_timer;
    QFont m_font;

    /**
     * A kludge to account for differences between the external and tabbed
     * sequence-editing frames.
     */

    int m_keyboard_padding_x;

    /**
     *  Provides a way to shrink the height of the data area. Defaults to 128.
     */

    int m_dataarea_y;

    /**
     *  Indicates we are editing tempo events.  They are drawn as circles
     *  instead of lines.
     */

    bool m_is_tempo;

    /**
     *  Indicates we are editing tempo events.  They are drawn as circles
     *  instead of lines.
     */

    bool m_is_program_change;

    /**
     * What events is the data window currently editing?
     */

    midibyte m_status;

    /**
     * What events is the data window currently editing?
     */

    midibyte m_cc;

    /**
     *  Used when dragging a new-level adjustment slope with the mouse.
     */

    bool m_line_adjust;

    /**
     *  Use when doing a relative adjustment of notes by dragging.
     */

    bool m_relative_adjust;

    /**
     *  This value is true if the mouse is being dragged in the data pane,
     *  which is done in order to change the height and value of each data
     *  line.
     */

    bool m_dragging;

};          // class qseqdata

}           // namespace seq66

#endif      // SEQ66_QSEQDATA_HPP

/*
 * qseqdata.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


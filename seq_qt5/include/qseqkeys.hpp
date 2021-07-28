#if ! defined SEQ66_QSEQKEYS_HPP
#define SEQ66_QSEQKEYS_HPP

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
 * \file          qseqkeys.hpp
 *
 *  This module declares/defines the base class for the left-side piano of
 *  the pattern/sequence panel.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2021-07-28
 * \license       GNU GPLv2 or above
 *
 *      We've added the feature of a right-click toggling between showing the
 *      main octave values (e.g. "C1" or "C#1") versus the numerical MIDI
 *      values of the keys.
 */

#include <QWidget>

#include "cfg/usrsettings.hpp"          /* seq66::show enum class           */
#include "play/seq.hpp"                 /* seq66::seq::pointer              */
#include "gui_palette_qt5.hpp"          /* gui_pallete_qt5::Color etc.      */
#include "qseqbase.hpp"                 /* seq66::qseqbase mixin class      */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

class performer;

/**
 *  Draws the piano keys in the sequence editor.
 */

class qseqkeys final : public QWidget, public qseqbase
{
    Q_OBJECT

    friend class qseqroll;

public:

    qseqkeys
    (
        performer & perf,
        seq::pointer seqp,
        qseqeditframe64 * frame,
        QWidget * parent,                                   /* QScrollArea  */
        int keyheight,      // = 12,
        int keyareaheight   // = 12 * c_num_keys + 1
    );

    virtual ~qseqkeys ()
    {
        // no code needed
    }

    void set_preview_key (int key);

    int note_height () const
    {
        return m_key_y;
    }

    int total_height () const
    {
        return m_key_area_y;
    }

    bool v_zoom_in ();
    bool v_zoom_out ();
    bool reset_v_zoom ();
    void set_key (int k);

protected:

    bool set_note_height (int h);

protected:      // Qt overrides

    virtual void paintEvent (QPaintEvent *) override;
    virtual void mousePressEvent (QMouseEvent *) override;
    virtual void mouseReleaseEvent (QMouseEvent *) override;
    virtual void mouseMoveEvent (QMouseEvent *) override;
    virtual QSize sizeHint() const override;
    virtual void wheelEvent (QWheelEvent * ev) override;

signals:

public slots:

    // void conditional_update ();

private:

    void convert_y (int y, int & note);

    void update_midi_buttons () override
    {
        // not needed for the keys panel.
    }

    /**
     *  Detects a black key.
     *
     * \param key
     *      The key to analyze.
     *
     * \return
     *      Returns true if the key is black (value 1, 3, 6, 8, or 10).
     */

    bool is_black_key (int key) const
    {
        return key == 1 || key == 3 || key == 6 || key == 8 || key == 10;
    }

    const Color & preview_color () const
    {
        return m_preview_color;
    }

    seq::pointer seq_pointer ()
    {
        return m_seq;
    }

    void total_height (int y)
    {
        if (y > 0)
            m_key_area_y = y;
    }

    void note_height (int y)
    {
        if (y > 0)
            m_key_y = y;
    }

private:

    seq::pointer m_seq;
    QFont m_font;

    /**
     *  The default value is to show the octave letters on the vertical
     *  virtual keyboard. There are 4 other modes of note name/number display.
     */

    showkeys m_show_key_names;

    /**
     *  This value indicates the key value as selected in the seqedit.  It
     *  ranges from 0 to 11, with 0 being C.
     */

    int m_key;

    int m_key_y;
    int m_key_area_y;
    const Color m_preview_color;
    bool m_is_previewing;
    int m_preview_key;

};          // class qseqkeys

}           // namespace seq66

#endif      // SEQ66_QSEQKEYS_HPP

/*
 * qseqkeys.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


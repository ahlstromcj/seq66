#if ! defined SEQ66_QSTRIGGEREDITOR_HPP
#define SEQ66_QSTRIGGEREDITOR_HPP

/*
 *  This file is part of seq66.
 *
 *  seq66 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  seq66 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq66; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          qstriggereditor.hpp
 *
 *  This module declares/defines the base class for the Performance window
 *  piano roll.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2019-07-27
 * \license       GNU GPLv2 or above
 *
 *  This class represents the central piano-roll user-interface area of the
 *  performance/song editor.
 */

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QTimer>
#include <QPen>

#include "app_limits.h"                 /* SEQ66_SEQKEY_HEIGHT macro        */
#include "midi/midibytes.hpp"           /* seq66::midibyte, other aliases   */
#include "qseqbase.hpp"                 /* seq66::qseqbase base class       */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class performer;
    class qseqdata;

/**
 *  Displays the triggers for MIDI events (e.g. Mod Wheel, Pitch Bend) in the
 *  event pane underneath the qseqroll pane.
 *
 *  Note that the qeqbase mixin class is publicly inherited so that the
 *  qseqeditrame classes can access the public member of this class.
 */

class qstriggereditor final : public QWidget, public qseqbase
{
    Q_OBJECT

public:

    qstriggereditor
    (
        performer & perf,
        seq::pointer seqp,
        int zoom                = SEQ66_DEFAULT_ZOOM,
        int snap                = SEQ66_DEFAULT_SNAP,
        int keyHeight           = SEQ66_SEQKEY_HEIGHT,
        QWidget * parent        = nullptr
    );

    virtual ~qstriggereditor ();

    void set_data_type (midibyte status, midibyte control);

protected:

    virtual void paintEvent (QPaintEvent *) override;
    virtual void resizeEvent (QResizeEvent *) override;
    virtual void mousePressEvent (QMouseEvent *) override;
    virtual void mouseReleaseEvent (QMouseEvent *) override;
    virtual void mouseMoveEvent (QMouseEvent *) override;
    virtual void keyPressEvent (QKeyEvent *) override;
    virtual void keyReleaseEvent (QKeyEvent *) override;
    virtual QSize sizeHint () const override;

signals:

public slots:

    void conditional_update ();

private:

    void x_to_w (int x1, int x2, int & x, int & w);
    void start_paste ();
    void convert_x (int x, midipulse & tick);
    void convert_t (midipulse ticks, int & x);
    void drop_event (midipulse tick);
    void set_adding (bool adding);

private:

    QTimer * m_timer;
    int m_key_y;
    midibyte m_status;      /* what is seqdata currently editing? */
    midibyte m_cc;

};          // class qstriggereditor

}           // namespace seq66

#endif      // SEQ66_QSTRIGGEREDITOR_HPP

/*
 * qstriggereditor.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


#if ! defined SEQ66_QSEQTIME_HPP
#define SEQ66_QSEQTIME_HPP

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
 * \file          qseqtime.hpp
 *
 *  This module declares/defines the base class for drawing the
 *  time/measures bar at the top of the patterns/sequence editor.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2024-11-18
 * \license       GNU GPLv2 or above
 *
 */

#include <QWidget>
#include <QTimer>
#include <QPainter>
#include <QPen>

#include "qseqbase.hpp"                 /* seq66::qseqbase mixin class      */

namespace seq66
{
    class performer;
    class qseqeditframe64;

/**
 * The timebar for the sequence editor
 */

class qseqtime final : public QWidget, public qseqbase
{
    friend class qseqeditframe64;   /* for scrolling a horizontal page  */

    Q_OBJECT

public:

    qseqtime
    (
        performer & p,
        sequence & s,
        qseqeditframe64 * frame,
        int zoom,
        QWidget * parent    /* QScrollArea */
    );

    virtual ~qseqtime ();

protected:

    virtual void paintEvent (QPaintEvent *) override;
    virtual void resizeEvent (QResizeEvent *) override;
    virtual void mousePressEvent (QMouseEvent *) override;
    virtual void mouseReleaseEvent (QMouseEvent *) override;
    virtual void mouseMoveEvent (QMouseEvent *) override;
    virtual void keyPressEvent (QKeyEvent *) override;
    virtual QSize sizeHint () const override;
    virtual void wheelEvent (QWheelEvent *) override;

private:

    void set_END_marker (bool expanding);
    void draw_grid (QPainter & painter, const QRect & r);
    void draw_markers (QPainter & painter);

signals:

private slots:

    void conditional_update ();

private:

    QTimer * m_timer;
    QFont m_font;
    bool m_move_L_marker;
    bool m_expanding;

    /*
     * Currently these are only 8-bit characters.
     */

    char m_L_marker[2];
    char m_R_marker[2];
    char m_END_marker[8];

};          // class qseqtime

}           // namespace seq66

#endif      // SEQ66_QSEQTIME_HPP

/*
 * qseqtime.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


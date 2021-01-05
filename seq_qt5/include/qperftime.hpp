#if ! defined SEQ66_QPERFTIME_HPP
#define SEQ66_QPERFTIME_HPP

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
 * \file          qperftime.hpp
 *
 *  The time bar shows markers and numbers for the measures of the song,
 *  and also depicts the left and right markers.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2021-01-04
 * \license       GNU GPLv2 or above
 *
 */

#include <QWidget>

#include "play/performer.hpp"
#include "qperfbase.hpp"

/*
 * Forward references.
 */

class QPaintEvent;
class QMouseEvent;
class QTimer;

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class performer;
    class qperfeditframe64;
    class qperfeditframe;

/**
 * The time bar for the song editor
 */

class qperftime final : public QWidget, public qperfbase
{
    friend class qperfeditframe64;  /* for scrolling a horizontal page  */
    friend class qperfeditframe;    /* for scrolling a horizontal page  */

    Q_OBJECT

public:

    qperftime
    (
        performer & a_perf,
        int zoom,
        int snap            = SEQ66_DEFAULT_SNAP,
        QWidget * parent    = nullptr
    );

    virtual ~qperftime ();

    void set_guides (midipulse snap, midipulse measure);

private:

    void increment_size ()
    {
        // TODO
    }

protected:      // override Qt event handlers

    virtual void paintEvent (QPaintEvent *) override;
    virtual void mousePressEvent (QMouseEvent *) override;
    virtual void mouseReleaseEvent (QMouseEvent *) override;
    virtual void mouseMoveEvent (QMouseEvent *) override;
    virtual void keyPressEvent (QKeyEvent *) override;
    virtual QSize sizeHint() const override;

private:

    QTimer * m_timer;
    QFont m_font;

    /**
     *  Snap value, starts out very small, equal to m_ppqn.
     */

    midipulse m_4bar_offset;
    midipulse m_measure_length;
    bool m_move_left;

signals:

public slots:

    void conditional_update ();

};          // class qperftime

}           // namespace seq66

#endif      // SEQ66_QPERFTIME_HPP

/*
 * qperftime.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


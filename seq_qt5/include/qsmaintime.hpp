#if ! defined SEQ66_QSMAINTIME_HPP
#define SEQ66_QSMAINTIME_HPP

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
 * \file          qsmaintime.hpp
 *
 *  This module declares/defines the base class for the "time" progress
 *  window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2019-07-08
 * \license       GNU GPLv2 or above
 *
 */

#include <QWidget>
#include <QPainter>
#include <QTimer>

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class performer;

/**
 * A beat indicator widget
 */

class qsmaintime final : public QWidget
{
    Q_OBJECT

public:

    qsmaintime
    (
        performer & perf,
        QWidget * parent,
        int beats_per_measure   = 4,
        int beat_width          = 4
    );

    virtual ~qsmaintime ()
    {
        // no code
    }

    int beats_per_measure () const
    {
        return m_beats_per_measure;
    }

    void beats_per_measure (int bpm);

    int beat_width () const
    {
        return m_beat_width;
    }

    void beat_width (int bw);

protected:

    virtual void paintEvent (QPaintEvent *) override;
    virtual QSize sizeHint() const override;

private:

    const performer & perf () const
    {
        return m_main_perf;
    }

private:

    const performer & m_main_perf;
    QColor m_color;
    QFont m_font;
    int m_beats_per_measure;
    int m_beat_width;
#if defined SEQ66_USE_METRONOME_FADE
    int m_alpha;
#endif
    int m_last_metro;

};          // class qsmaintime

}           // namespace seq66

#endif      // SEQ66_QSMAINTIME_HPP

/*
 * qsmaintime.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


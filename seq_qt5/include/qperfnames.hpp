#if ! defined SEQ66_QPERFNAMES_HPP
#define SEQ66_QPERFNAMES_HPP

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
 * \file          qperfnames.hpp
 *
 *  This module declares/defines the base class for performance names.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-01-01
 * \updates       2021-01-04
 * \license       GNU GPLv2 or above
 *
 */

#include <QWidget>

#include "qperfbase.hpp"                /* for constants and base class    */
#include "play/sequence.hpp"

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class performer;
    class qperfeditframe64;

/**
 * Sequence labels for the side of the song editor
 */

class qperfnames final : public QWidget, public qperfbase
{
    friend class qperfeditframe64;

    Q_OBJECT

public:

    qperfnames (performer & p, QWidget * parent);

    virtual ~qperfnames ()
    {
        // no code
    }

protected:

    void reupdate ();

    int name_x (int i)
    {
        return m_nametext_x + i;
    }

    int name_y (int i)
    {
        return m_nametext_y * i;
    }

protected:          // Qt overrides

    virtual void paintEvent (QPaintEvent *) override;
    virtual void keyPressEvent (QKeyEvent *) override;
    virtual void mousePressEvent (QMouseEvent *) override;
    virtual void mouseReleaseEvent (QMouseEvent *) override;
    virtual void mouseMoveEvent (QMouseEvent *) override;
    virtual QSize sizeHint() const override;

private:

    int convert_y (int y);

signals:

public slots:

private:

    QFont m_font;
    int m_nametext_x;
    int m_nametext_y;

    /*
     * Saves a frequent calculation.
     */

    int m_set_text_y;

};

}           // namespace seq66

#endif      // SEQ66_QPERFNAMES_HPP

/*
 * qperfnames.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


#if ! defined SEQ66_QPORTWIDGET_HPP
#define SEQ66_QPORTWIDGET_HPP

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
 * \file          qportwidget.hpp
 *
 *  This base class supports qclocklayout and qinputcheckbox.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2022-01-20
 * \updates       2022-01-20
 * \license       GNU GPLv2 or above
 *
 */

#include <QtWidgets/QWidget>

/*
 *  Do not document the namespace, it breaks Doxygen.
 */

namespace seq66
{
    class performer;
    class qseditoptions;

/**
 *  This class is a widget that supports a row of radio-buttons that let the
 *  user set the type of clocking for each MIDI output buss:
 *
 *      -   Disabled
 *      -   Off.
 *      -   On (Pos).
 *      -   On (Mod).
 */

class qportwidget : public QWidget
{

public:

    qportwidget (QWidget * parent, performer & p, int bus);

    virtual ~qportwidget ()
    {
        // no code needed
    }

protected:

    // virtual void setup_ui () = 0;

    performer & perf ()
    {
        return m_performance;
    }

    qseditoptions * parent_widget ()
    {
        return m_parent_widget;
    }

    int bus () const
    {
        return m_bus;
    }

signals:

private slots:

private:

    /**
     *  Provides a reference to the single performer object associated with the
     *  MIDI output buss represented by this layout.  One question is will we
     *  have to change the reference to a shared pointer.
     */

    performer & m_performance;

    /**
     *  Provides the buss number, re 0, of the MIDI I/O bus represented by
     *  this port widget.
     */

    int m_bus;

    /**
     *  For telling the parent window to change states.
     */

    qseditoptions * m_parent_widget;

};          // class qportwidget

}           // namespace seq66

#endif      // SEQ66_QPORTWIDGET_HPP

/*
 * qportwidget.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


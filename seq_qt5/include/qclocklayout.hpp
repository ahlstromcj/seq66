#if ! defined SEQ66_QCLOCKLAYOUT_HPP
#define SEQ66_QCLOCKLAYOUT_HPP

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
 * \file          qclocklayout.hpp
 *
 *  This class supports a MIDI Clocks label and a set of radio-buttons for
 *  selecting the clock style (off, on POS, on MOD), associating it with a
 *  particular output buss.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-05-19
 * \updates       2018-05-20
 * \license       GNU GPLv2 or above
 *
 */

#include <QtWidgets/QWidget>

class QButtonGroup;
class QGroupBox;
class QHBoxLayout;
class QLabel;
class QRadioButton;
class QSpacerItem;

/*
 *  Do not document the namespace, it breaks Doxygen.
 */

namespace Ui
{
    // class qclocklayout;
}

namespace seq66
{
    class performer;

/**
 * m_horizlayout_clockline holds all of these.
 *
 * m_horizlayout_clockoffon holds the buttons: m_rbutton_portdisabled,
 * m_rbutton_clockoff, m_rbutton_clockonmod, m_rbutton_clockonpos.
 *
 * m_spacer_clock separates m_label_outputbusname from
 * m_horizlayout_clockoffon.
 *
 * m_groupbox_clocks hold all of these.
 */

class qclocklayout : public QWidget
{
    Q_OBJECT

public:

    qclocklayout
    (
        QWidget * parent,                 // QGroupBox, QObject * parent
        performer & p,
        int bus
    );

    virtual ~qclocklayout ()
    {
        // no code needed
    }

    QHBoxLayout * layout ()
    {
        return m_horizlayout_clockline;
    }

private:

    void setup_ui ();
    performer & perf ()
    {
        return m_performance;
    }

signals:

private slots:

    void clock_callback_clicked (int id);

private:

    performer & m_performance;
    int m_bus;
    QWidget * m_parent_widget;                      /* currently not used   */
    QHBoxLayout * m_horizlayout_clockline;          /* see layout() below   */
    QSpacerItem * m_spacer_clock;
    QLabel * m_label_outputbusname;
    QRadioButton * m_rbutton_portdisabled;
    QRadioButton * m_rbutton_clockoff;
    QRadioButton * m_rbutton_clockonpos;
    QRadioButton * m_rbutton_clockonmod;
    QButtonGroup * m_rbutton_group;

};

}           // namespace seq66

#endif      // SEQ66_QCLOCKLAYOUT_HPP

/*
 * qclocklayout.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


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
 * \updates       2020-07-27
 * \license       GNU GPLv2 or above
 *
 *  Provides the layout for a single MIDI output buss clocking user-interface
 *  setup.
 */

#include <QtWidgets/QWidget>

/*
 *  Forward references.
 */

class QButtonGroup;
class QGroupBox;
class QHBoxLayout;
class QLabel;
class QRadioButton;
class QSpacerItem;

/*
 *  Do not document the namespace, it breaks Doxygen.
 */

namespace seq66
{
    class performer;

/**
 *  This class is a widget that supports a row of radio-buttons that let the
 *  user set the type of clocking for each MIDI output buss:
 *
 *      -   Disabled
 *      -   Off.
 *      -   On (Pos).
 *      -   On (Mod).
 */

class qclocklayout : public QWidget
{
    Q_OBJECT

public:

    qclocklayout (QWidget * parent, performer & p, int bus);

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

    /**
     *  Provides a reference to the single performer object associated with the
     *  MIDI output buss represented by this layout.  One question is will we
     *  have to change the reference to a shared pointer.
     */

    performer & m_performance;

    /**
     *  Provides the buss number, re 0, of the MIDI output bus represented by
     *  this layout.
     */

    int m_bus;

    /**
     *  TODO
     */

    QWidget * m_parent_widget;                      /* currently not used   */

    /**
     * m_horizlayout_clockline holds the label and all of the radio buttons for
     * a given MIDI output buss.
     */

    QHBoxLayout * m_horizlayout_clockline;          /* see layout()         */

    /**
     *  The spacer between the buss name and button-group.
     */

    QSpacerItem * m_spacer_clock;

    /**
     *  The name of the MIDI output buss represented by this object.
     */

    QLabel * m_label_outputbusname;

    /**
     *  Port disabled.  See the banner for the setup_ui() function.
     */

    QRadioButton * m_rbutton_portdisabled;

    /**
     *  Clocking off.  See the banner for the setup_ui() function.
     */

    QRadioButton * m_rbutton_clockoff;

    /**
     *  Clocking re position.  See the banner for the setup_ui() function.
     */

    QRadioButton * m_rbutton_clockonpos;

    /**
     *  Clocking re clock-start modulo setting.  See the banner for the
     *  setup_ui() function.
     */

    QRadioButton * m_rbutton_clockonmod;

    /**
     *  Contains all of the radio-buttons.
     */

    QButtonGroup * m_rbutton_group;

};          // class qclocklayout

}           // namespace seq66

#endif      // SEQ66_QCLOCKLAYOUT_HPP

/*
 * qclocklayout.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


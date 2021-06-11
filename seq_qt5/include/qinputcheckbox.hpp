#if ! defined SEQ66_QINPUTCHECKBOX_HPP
#define SEQ66_QINPUTCHECKBOX_HPP

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
 * \file          qinputcheckbox.hpp
 *
 *  This class supports a MIDI Input check-box, associating it with a
 *  particular input buss.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-05-20
 * \updates       2020-07-26
 * \license       GNU GPLv2 or above
 *
 */

#include <QtWidgets/QWidget>

/*
 *  Forward reference.
 */

class QCheckBox;

/*
 *  Do not document the namespace, it breaks Doxygen.
 */

namespace seq66
{
    class performer;

/**
 *  This class represent a single line holding the setting (enabled/disabled
 *  for a single MIDI input buss.
 */

class qinputcheckbox : public QWidget
{
    Q_OBJECT

public:

    qinputcheckbox (QWidget * parent, performer & p, int bus);

    virtual ~qinputcheckbox ()
    {
        // no code needed
    }

    QCheckBox * input_checkbox ()
    {
        return m_chkbox_inputactive;
    }

private:

    void setup_ui ();
    performer & perf ()
    {
        return m_performance;
    }

signals:

private slots:

    void input_callback_clicked (int id);

private:

    performer & m_performance;
    int m_bus;
    QWidget * m_parent_widget;                      /* currently not used   */
    QCheckBox * m_chkbox_inputactive;

};

}           // namespace seq66

#endif      // SEQ66_QINPUTCHECKBOX_HPP

/*
 * qinputcheckbox.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


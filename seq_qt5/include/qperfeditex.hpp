#if ! defined SEQ66_QPERFEDITEX_HPP
#define SEQ66_QPERFEDITEX_HPP

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
 * \file          qperfeditex.hpp
 *
 *  This module declares/defines the base class for the external
 *  performance-editing window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-07-21
 * \updates       2018-08-04
 * \license       GNU GPLv2 or above
 *
 *  The performance editing window is known as the "Song Editor".  Kepler34
 *  provides an editor embedded within a tab, but we supplement that with a
 *  more sophisticated external editor, which works a lot more like the Gtkmm
 *  perfedit class.
 */

#include <QWidget>

#include "app_limits.h"                 /* SEQ66_USE_DEFAULT_PPQN           */

/*
 * Forward reference.
 */

class QCloseEvent;

/*
 * Do not document the namespace.
 */

namespace Ui
{
    class qperfeditex;
}

/*
 * Do not document the namespace.
 */

namespace seq66
{
    class performer;
    class sequence;
    class qperfeditframe64;
    class qsmainwnd;

/**
 *  Provides a container for a qperfeditframe64 object.  Thus, the Qt 5 version
 *  of Seq66 has an external seqedit window like its Gtkmm-2.4
 *  counterpart.
 */

class qperfeditex final : public QWidget
{
    Q_OBJECT

public:

    qperfeditex (performer & p, qsmainwnd * parent = nullptr);
    virtual ~qperfeditex ();

    void update_sizes ();

protected:

    virtual void closeEvent (QCloseEvent *) override;

private:

    const performer & perf () const
    {
        return m_performer;
    }

    performer & perf ()
    {
        return m_performer;
    }

signals:

private slots:

private:

    Ui::qperfeditex * ui;
    performer & m_performer;
    qsmainwnd * m_edit_parent;
    qperfeditframe64 * m_edit_frame;

};              // class qperfeditex

}               // namespace seq66

#endif          // SEQ66_QPERFEDITEX_HPP

/*
 * qperfeditex.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


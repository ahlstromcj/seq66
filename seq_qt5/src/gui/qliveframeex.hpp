#if ! defined SEQ66_QLIVEFRAMEEX_HPP
#define SEQ66_QLIVEFRAMEEX_HPP

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
 * \file          qliveframeex.hpp
 *
 *  This module declares/defines the base class for the external
 *  sequence-editing window.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-09-16
 * \updates       2021-11-19
 * \license       GNU GPLv2 or above
 *
 *  The sequence editing window is known as the "Pattern Editor".  Kepler34
 *  provides an editor embedded within a tab, but we supplement that with a
 *  more sophisticated external editor, which works a lot more like the Gtkmm
 *  seqedit class.
 */

#include <QWidget>

class QCloseEvent;

namespace Ui
{
    class qliveframeex;
}

namespace seq66
{
    class performer;
    class sequence;
    class qslivebase;
    class qsmainwnd;

/**
 *  Provides a container for a qslivebase (the base class for qslivegrid)
 *  object.  Thus, the Qt 5 version of Seq66 has an (additional) external
 *  seqedit window like its Gtkmm-2.4 counterpart.
 */

class qliveframeex final : public QWidget
{
    Q_OBJECT

public:

    qliveframeex
    (
        performer & p,
        int ssnum,
        qsmainwnd * parent = nullptr
    );
    virtual ~qliveframeex ();

    void update_draw_geometry ();
    void update_sequence (int seqno, bool redo);

protected:

    virtual void closeEvent (QCloseEvent *) override;
    virtual void changeEvent (QEvent * event) override;

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

    Ui::qliveframeex * ui;
    performer & m_performer;
    int m_screenset;
    qsmainwnd * m_live_parent;
    qslivebase * m_live_frame;

};              // class qliveframeex

}               // namespace seq66

#endif          // SEQ66_QLIVEFRAMEEX_HPP

/*
 * qliveframeex.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


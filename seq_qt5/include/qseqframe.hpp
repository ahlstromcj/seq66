#if ! defined SEQ66_QSEQFRAME_HPP
#define SEQ66_QSEQFRAME_HPP

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
 *  Place, Suite 330, Boston, MA  02111-1307  USA.
 */

/**
 * \file          qseqframe.hpp
 *
 *  This module declares/defines the edit frame for sequences.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-07-27
 * \updates       2022-04-28
 * \license       GNU GPLv2 or above
 *
 *  Provides an abstract base class so that both the old and the new Qt
 *  sequence-edit frames can be supported.
 *
 *  For now, we're abstracting the zoom functionality.  Later, we
 *  can abstract other code common between the two frames.
 */

#include <QFrame>

#include "qbase.hpp"                    /* seq66:qbase super base class     */
#include "play/seq.hpp"                 /* seq66::seq::pointer & sequence   */
#include "play/sequence.hpp"            /* seq66::seq::pointer & sequence   */

/*
 *  Forward declarations.  Some Qt header files are in the cpp file.
 */

class QWidget;

/*
 * Do not document namespaces, it breaks Doxygen.
 */

namespace Ui
{
    class qseqframe;
}

/*
 * Do not document namespaces, it breaks Doxygen.
 */

namespace seq66
{
    class qseqkeys;
    class qseqtime;
    class qseqroll;
    class qseqrollpix;
    class qseqdata;
    class qstriggereditor;

/**
 *  This frame is the basis for editing an individual MIDI sequence.
 */

class qseqframe : public QFrame, public qbase
{
    friend class qseqroll;

    Q_OBJECT

public:

    qseqframe
    (
        performer & p,
        sequence & s,                   // int seqid,
        QWidget * parent = nullptr
    );

    virtual ~qseqframe ();

    bool repitch_all ();
    bool repitch_selected ();

public:     // protected:

    const sequence & track () const
    {
        return m_seq;
    }

    sequence & track ()
    {
        return m_seq;
    }

    virtual void update_note_entry (bool on) = 0;
    virtual void update_draw_geometry () = 0;

public:

    virtual bool set_zoom (int z) override;
    virtual void set_dirty () override;

private:

    sequence & m_seq;

protected:

    qseqkeys * m_seqkeys;
    qseqtime * m_seqtime;
    qseqroll * m_seqroll;
    qseqdata * m_seqdata;
    qstriggereditor * m_seqevent;

};          // class qseqframe

}           // namespace seq66

#endif      // SEQ66_QSEQFRAME_HPP

/*
 * qseqframe.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


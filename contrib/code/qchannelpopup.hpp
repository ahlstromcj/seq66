#if ! defined SEQ66_QCHANNELPOPUP_HPP
#define SEQ66_QCHANNELPOPUP_HPP

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
 * \file          qchannelpopup.hpp
 *
 *  This module declares/defines the edit frame for sequences.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2026-03-12
 * \updates       2026-03-12
 * \license       GNU GPLv2 or above
 *
 */

#include <QObject>

#include "cfg/settings.hpp"             /* seq66::combolist class, helpers  */

/*
 *  A few forward declarations.  The Qt header files are in the cpp file.
 */

class QComboBox;

/*
 * Note that the forward references somewhat duplicate those in qseqframe.
 */

namespace seq66
{

class sequence;

/**
 *  This frame holds tools for editing an individual MIDI sequence.  This
 *  frame is a more advanced version of qseqeditframe (now moved to
 *  contrib/code), which was based on Kepler34's EditFrame class.
 */

class qchannelpopup : public QObject
{
    // friend class qstriggereditor;

    Q_OBJECT

public:

    qchannelpopup () = delete;
    qchannelpopup
    (
        sequence & s,
        QComboBox * combo   // QWidget * parent
    );
    qchannelpopup (const qchannelpopup &) = delete;
    qchannelpopup & operator = (const qchannelpopup &) = delete;
    qchannelpopup (qchannelpopup &&) = default;
    qchannelpopup & operator = (qchannelpopup &&) = default;
    ~qchannelpopup () = default;

    void get_position (int & x, int & y);

protected:

    void set_track_change (bool modified = true);
    void set_external_frame_title (bool modified = true);

    sequence & track ()
    {
        return m_track;
    }

    const sequence & track () const
    {
        return m_track;
    }

    QComboBox * channel_combo ()
    {
        return m_channel_combo;
    }

    const QComboBox * channel_combo () const
    {
        return m_channel_combo;
    }

    int edit_channel () const
    {
        return m_edit_channel;
    }

private:

    void set_dirty ()
    {
        track().set_dirty();
    }

signals:                            // signals cannot have an access specifier

    void signal_change_channel (int chan, bool userchange);

private slots:

    void update_midi_channel (int index);
    void reset_midi_channel ();

private:        /* slot helper functions        */

    void repopulate_midich_combo (int buss);

private:        /* setters and getters          */

    void set_midi_channel
    (
        int midichannel,
        bool userchange = false // qbase::status qs = qbase::status::edit
    );

private:

    sequence & m_track;

    /**
     *  Needed for Qt.
     */

    // ? Ui::qchannelpopup * ui;

    QComboBox * m_channel_combo { nullptr };

    /**
     *  Indicates what MIDI channel the data window is currently editing.
     */

    int m_edit_channel { null_channel() };


};          // class qchannelpopup

}           // namespace seq66

#endif      // SEQ66_QCHANNELPOPUP_HPP

/*
 * qchannelpopup.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

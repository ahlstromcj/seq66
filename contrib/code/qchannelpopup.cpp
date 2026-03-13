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
 * \file          qchannelpopup.cpp
 *
 *  This module declares/defines the base class for plastering pattern /
 *  sequence data information in the pattern editor.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2026-03-12
 * \updates       2026-03-12
 * \license       GNU GPLv2 or above
 *
 *
 */

#include <QComboBox>
#include <QMenu>

#include "play/sequence.hpp"            /* seq66::sequence                  */
#include "util/strfunctions.hpp"        /* seq66::string_to_int()           */
#include "qchannelpopup.hpp"            /* seq66::qchannelpopup class       */
#include "qt5_helpers.hpp"              /* seq66::qt(), qt_set_icon()       */

namespace seq66
{

qchannelpopup::qchannelpopup
(
    sequence & s,
    QComboBox * chcombo // QWidget * parent
) :
    m_track         (s),
    m_channel_combo (chcombo),
    m_edit_channel  (s.midi_channel())
{
    set_midi_channel(m_edit_channel);   // , qbase::status::startup);
    // set_initialized();
}

/**
 *  Selects the given MIDI channel parameter in the main sequence object, so
 *  that it will use that channel.  If "Free" is selected, all that happens is
 *  that the pattern is set to "no-channel" mode for MIDI output.
 *
 * \param ch
 *      The MIDI channel value to set.
 *
 * \param user_change
 *      True if the user made this change, and thus has potentially modified
 *      the song.  The default is false.
 * \param qs
 *      Indicates if the changes was made at startup, or by a user-edit.  Set
 *      to qbase::status::startup in the former case, and qbase::status::edit
 *      in the latter case.  If the user made this change, they've potentially
 *      modified the song.  If the bus number has changed, then the MIDI
 *      channel and event menus are repopulated to reflect the new bus.  This
 *      parameter is "startup" in the constructor because those items have not
 *      been set up at that time. The default value is "edit".
 */

void
qchannelpopup::set_midi_channel (int ch, bool userchange)  // qbase::status qs)
{
    int initialchan { track().seq_midi_channel() };
    if (ch != initialchan || ! userchange)
    {
        int chindex { ch };
        midibyte channel { midibyte(ch) };
        if (! is_good_channel(channel))
        {
            chindex = c_midichannel_max;                    /* "Free" */
            channel = null_channel();
        }
        if (track().set_midi_channel(channel, userchange))
        {
            m_edit_channel = channel;
            if (is_null_channel(channel))
            {
                channel_combo()->setCurrentIndex(chindex);
            }
            else
            {
#if 0
                // WHY DOES THIS EVEN COMPILE??????

                if (user_change)
                    set_track_change();             /* to solve issue #90   */
                else
#endif
                    channel_combo()->setCurrentIndex(chindex);
            }
            emit signal_change_channel(channel, userchange);
        }
    }
}

/**
 *  Note that c_midichannel_max (16) is a legal value.  It is remapped in
 *  sequence::set_midi_channel() to null_channel().
 */

void
qchannelpopup::update_midi_channel (int index)
{
    set_midi_channel(index);
}

void
qchannelpopup::reset_midi_channel ()
{
    set_midi_channel(0);
}

/**
 *  Populates the MIDI Channel combo-box with the number and names of each
 *  channel.  This action is needed at startup of the seqedit window, and when
 *  the user changes the active buss for the sequence.
 *
 *  When the output buss or channel are changed, we get the 16 "channels" from
 *  the new buss's definition, get the corresponding instrument, and load its
 *  name into this midich popup.  Then we need to go to the instrument/channel
 *  that has been selected, and repopulate the event menu with that item's
 *  controller values/names.
 *
 * \param buss
 *      The new value for the buss from which to get the [user-instrument-N]
 *      settings in the [user-instrument-definitions] section.
 */

void
qchannelpopup::repopulate_midich_combo (int buss)
{
    disconnect
    (
        channel_combo(), SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_midi_channel(int))
    );

    channel_combo()->clear();

    for (int channel = 0; channel <= c_midichannel_max; ++channel)
    {
        char b[4];                                      /* 2 digits or less */
        snprintf(b, sizeof b, "%2d", channel + 1);
        std::string name { std::string(b) };
        std::string s { usr().instrument_name(buss, channel) };
        if (! s.empty())
        {
            name += " [";
            name += s;
            name += "]";
        }
        if (channel == c_midichannel_max)               /* i.e. 16          */
        {
            QString combo_text("Free");
            channel_combo()->insertItem(channel, combo_text);
        }
        else
        {
            QString combo_text(qt(name));
            channel_combo()->insertItem(channel, combo_text);
        }
    }

    int ch = track().midi_channel();
    if (is_null_channel(ch))
        ch = c_midichannel_max;

    channel_combo()->setCurrentIndex(ch);

    connect
    (
        channel_combo(), SIGNAL(currentIndexChanged(int)),
        this, SLOT(update_midi_channel(int))
    );
}

}           // namespace seq66

/*
 * qeventpopups.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

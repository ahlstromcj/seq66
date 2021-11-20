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
 * \file          qslivebase.cpp
 *
 *  This module declares/defines the base class for holding pattern slots.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2019-06-22
 * \updates       2021-11-20
 * \license       GNU GPLv2 or above
 *
 *  This class is the Qt counterpart to the old mainwid class.
 */

#include "cfg/settings.hpp"             /* seq66::usr().mainwnd_spacing()   */
#include "ctrl/keystroke.hpp"           /* seq66::keystroke class           */
#include "qslivebase.hpp"

/*
 * Do not document a namespace, it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Principal constructor.
 *
 * \param p
 *      Provides the performer object to use for interacting with this sequence.
 *
 * \param window
 *      Provides the functional parent of this live frame.
 *
 * \param bank
 *      Indicates the screenset number to use for the live grid.  If unassigned,
 *      then the performer's active screenset number is used.
 *
 * \param parent
 *      Provides the Qt-parent window/widget for this container window.
 *      Defaults to null.  Normally, this is a pointer to the tab-widget
 *      containing this frame.  If null, there is no parent, and this frame is
 *      in an external window.
 */

qslivebase::qslivebase
(
    performer & p,
    qsmainwnd * window,
    screenset::number bank,
    QWidget * parent
) :
    QFrame              (parent),
    m_performer         (p),
    m_parent            (window),
    m_font              (),
    m_bank_id
    (
        screenset::is_unassigned(bank) ? p.playscreen_number() : bank
    ),
    m_bank_name         (),
    m_mainwnd_spacing   (usr().mainwnd_spacing()),          /* spacing()    */
    m_space_rows        (m_mainwnd_spacing * p.rows()),
    m_space_cols        (m_mainwnd_spacing * p.columns()),
    m_screenset_slots   (p.rows() * p.columns()),
    m_slot_w            (0),
    m_slot_h            (0),
    m_last_metro        (0),
    m_alpha             (0),
    m_current_seq       (seq::unassigned()),
    m_source_seq        (seq::unassigned()),
    m_button_down       (false),
    m_moving            (false),
    m_adding_new        (false),
    m_can_paste         (false),
    m_has_focus         (false),
    m_is_external       (is_nullptr(parent)),
    m_needs_update      (false)
{
    (void) debug_message("qlivebase bank ID", std::to_string(bank_id()));
}

/**
 *  Virtual destructor.
 */

qslivebase::~qslivebase()
{
    // No code needed
}

/**
 *  Sets the bank number retrieved from performer.
 */

void
qslivebase::set_bank ()
{
    set_bank(m_bank_id);    /* set_bank(int(perf().playscreen_number()))    */
}

/**
 *  Roughly similar to log_screenset().  Note that, for import, we will have
 *  already set the bank to be filled in, and so must do the work even if the
 *  bank ID has not changed.
 *
 * \return
 *      Returns true if the bank was successfully changed.
 */

bool
qslivebase::set_bank (int bankid, bool hasfocus)
{
    bool result = perf().is_screenset_valid(bankid);
    if (result)
    {
        m_bank_id = bankid;
        if (hasfocus)
        {
            std::string bankname = perf().set_name(bankid);
            if (! is_external())                        /* show_per_bank()  */
                (void) perf().set_playing_screenset(bankid);

            set_bank_values(bankname, bankid);          /* update the GUI   */
        }
    }
    return result;
}

/**
 *  We rely on the caller (the parent window) to call this function only when
 *  the bank ID has actually changed.
 */

void
qslivebase::update_bank (int bank)
{
    m_has_focus = true;                                 /* widget active    */
    set_bank(bank, true);
}

/**
 *  We should ultimately use setmaster rather than usrsettings here.
 */

seq::number
qslivebase::seq_offset () const
{
    seq::number result = ! is_external() ?
        perf().playscreen_offset() : usr().set_offset(bank_id()) ;

    return result;
}

void
qslivebase::color_by_number (int i)
{
    perf().color(m_current_seq, i);
}

void
qslivebase::set_midi_bus (int b)
{
    (void) perf().set_midi_bus(m_current_seq, b);
}

void
qslivebase::set_midi_channel (int channel)
{
    (void) perf().set_midi_channel(m_current_seq, channel);
}

bool
qslivebase::copy_seq ()
{
    bool result = perf().copy_sequence(m_current_seq);
    if (result)
        m_can_paste = true;

    return result;
}

/**
 * Need a dialog warning that the editor is the reason this sequence cannot be
 * cut.
 */

bool
qslivebase::cut_seq ()
{
    bool result = perf().cut_sequence(m_current_seq);
    if (result)
        m_can_paste = true;

    return result;
}

/**
 *  If the sequence/pattern is delete-able (valid and not being edited), then
 *  it is deleted via the performer object.  Note that in seq66 the
 *  screenset::remove() function makes this check now.
 */

bool
qslivebase::delete_seq ()
{
    return perf().remove_sequence(m_current_seq);
}

bool
qslivebase::paste_seq ()
{
    return perf().can_paste() ? perf().paste_sequence(m_current_seq) : false ;
}

bool
qslivebase::merge_seq ()
{
    return perf().can_paste() ? perf().merge_sequence(m_current_seq) : false ;
}

}           // namespace seq66

/*
 * qslivebase.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


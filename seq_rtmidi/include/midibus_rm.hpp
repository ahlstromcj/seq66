#if ! defined SEQ66_MIDIBUS_RM_HPP
#define SEQ66_MIDIBUS_RM_HPP

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
 * \file          midibus_rm.hpp
 *
 *  This module declares/defines the base class for MIDI I/O for Linux, Mac,
 *  and Windows, using a refactored "RtMidi" library.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2016-11-21
 * \updates       2021-12-06
 * \license       GNU GPLv2 or above
 *
 *  This midibus module is the RtMidi version of the midibus
 *  module.
 */

#include "midi/midibase.hpp"            /* seq66::midibase class (new)      */
#include "rtmidi_types.hpp"             /* midibase::c_normal_port          */

/*
 * Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{
    class event;
    class rtmidi;
    class rtmidi_info;

/**
 *  This class implements with rtmidi version of the midibus object.
 */

class midibus final : public midibase
{
    /**
     *  The master MIDI bus sets up the buss, so it gets access to private
     *  details.
     */

    friend class mastermidibus;

private:

    /**
     *  The RtMidi API interface object this midibus will be creating and then
     *  using.
     */

    rtmidi * m_rt_midi;

    /**
     *  For Seq66, the ALSA model used requires that all the midibus
     *  objects use the same ALSA sequencer "handle".  The rtmidi_info object
     *  used for enumerating the ports is a good place to get this handle.
     *  It is an extension of the legacy RtMidi interface.
     */

    rtmidi_info & m_master_info;

public:

    /*
     * Virtual-port and non-virtual-port constructor.
     */

    midibus
    (
        rtmidi_info & rt,
        int index,
        bool makevirtual    = midibase::c_normal_port,
        bool isinput        = midibase::c_output_port,
        int bussoverride    = null_buss(),
        bool makesystem     = false
    );

    virtual ~midibus ();

    virtual bool api_connect ();

protected:

    virtual bool api_init_in () override;
    virtual bool api_init_in_sub () override;
    virtual bool api_init_out () override;
    virtual bool api_init_out_sub () override;
    virtual bool api_deinit_out () override;
    virtual bool api_deinit_in () override;
    virtual bool api_get_midi_event (event * inev) override;
    virtual int api_poll_for_midi () override;
    virtual void api_continue_from (midipulse tick, midipulse beats) override;
    virtual void api_start () override;
    virtual void api_stop () override;
    virtual void api_clock (midipulse tick) override;
    virtual void api_play (const event * e24, midibyte channel) override;
    virtual void api_sysex (const event * e24) override;

};          // class midibus (rtmidi version)

}           // namespace seq66

#endif      // SEQ66_MIDIBUS_RM_HPP

/*
 * midibus_rm.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


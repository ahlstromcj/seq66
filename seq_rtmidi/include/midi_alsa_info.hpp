#if ! defined SEQ66_MIDI_ALSA_INFO_HPP
#define SEQ66_MIDI_ALSA_INFO_HPP

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
 * \file          midi_alsa_info.hpp
 *
 *    A class for holding the current status of the ALSA system on the host.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2016-12-04
 * \updates       2022-02-01
 * \license       See above.
 *
 *    We need to have a way to get all of the ALSA information of
 *    the midi_alsa
 */

#include <alsa/asoundlib.h>

#include "mastermidibus_rm.hpp"
#include "midi_info.hpp"                /* seq66::midi_port_info etc.       */
#include "midi/midibus.hpp"             /* seq66::midibus                   */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq66
{
    class mastermidibus;

/**
 *  The class for handling ALSA MIDI input.
 */

class midi_alsa_info final : public midi_info
{

private:

    /**
     *  Flags that denote queries for input (read) ports.
     */

    static unsigned sm_input_caps;

    /**
     *  Flags that denote queries for output (write) ports.
     */

    static unsigned sm_output_caps;

    /**
     *  Holds the ALSA sequencer client pointer so that it can be used
     *  by the midibus objects.  This is actually an opaque pointer; there is
     *  no way to get the actual fields in this structure; they can only be
     *  accessed through functions in the ALSA API.
     */

    snd_seq_t * m_alsa_seq;

    /**
     *  The number of descriptors for polling.
     */

    int m_num_poll_descriptors;

    /**
     *  Points to the list of descriptors for polling.
     */

    struct pollfd * m_poll_descriptors;

public:

    midi_alsa_info () = delete;
    midi_alsa_info (const std::string & appname, int ppqn, midibpm bpm);
    virtual ~midi_alsa_info ();

    /**
     * \getter m_alsa_seq
     *      This is the platform-specific version of midi_handle().
     */

    snd_seq_t * seq ()
    {
        return m_alsa_seq;
    }

    virtual bool api_get_midi_event (event * inev) override;
    virtual int api_poll_for_midi () override;
    virtual void api_set_ppqn (int p) override;
    virtual void api_set_beats_per_minute (midibpm b) override;
    virtual void api_port_start
    (
        mastermidibus & masterbus, int bus, int port
    ) override;
    virtual void api_flush () override;

private:

    virtual int get_all_port_info
    (
        midi_port_info & inports,
        midi_port_info & outports
    ) override;

    void get_poll_descriptors ();
    void remove_poll_descriptors ();
    bool check_port_type (snd_seq_port_info_t * pinfo) const;
    bool show_event (snd_seq_event_t * ev, const char * tag);

};          // class midi_alsa_info

}           // namespace seq66

#endif      // SEQ66_MIDI_ALSA_INFO_HPP

/*
 * midi_alsa_info.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


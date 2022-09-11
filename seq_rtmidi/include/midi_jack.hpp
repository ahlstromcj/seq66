#if ! defined SEQ66_MIDI_JACK_HPP
#define SEQ66_MIDI_JACK_HPP

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
 * \file          midi_jack.hpp
 *
 *    A class for realtime MIDI input/output via JACK.
 *
 * \library       seq66 application
 * \author        Gary P. Scavone; severe refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2022-09-10
 * \license       See above.
 *
 *    In this refactoring, we've stripped out most of the original RtMidi
 *    functionality, leaving only the method for selecting the API to use for
 *    MIDI.  The method that Seq66's mastermidibus uses to initialize port has
 *    been transplanted to this rtmidi library.  The name "rtmidi" is now
 *    somewhat misleading.
 */

#include <string>

#include "seq66_features.hpp"
#include "midi_api.hpp"

#if defined SEQ66_JACK_SUPPORT

#include "midi_jack_data.hpp"           /* seq66::midi_jack_data            */
#include "midi_jack_info.hpp"           /* seq66::midi_jack_info            */

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq66
{
    class midibus;

/**
 *  This class implements with JACK version of the midi_alsa object.
 */

class midi_jack : public midi_api
{

    friend class midi_jack_info;

private:

    /**
     *  Preserves the original name of the remote port, so it can be used
     *  later for connection and for analyzing port registration /
     *  unregistration.
     */

    std::string m_remote_port_name;

protected:

    /**
     *  This reference is needed in order for this midi_jack object to add
     *  itself to the main midi_jack_info list when running in single-JACK
     *  client mode.
     */

    midi_jack_info & m_jack_info;

    /**
     *  Holds the data needed for JACK processing.  Please do not confuse this
     *  item with the m_midi_handle of the midi_api base class.  This object
     *  holds a JACK-client pointer and a JACK-port pointer.
     */

    midi_jack_data m_jack_data;

private:

    midi_jack ();

public:

    midi_jack (midibus & parentbus, midi_info & masterinfo);
    virtual ~midi_jack ();

    /**
     *  This is the platform-specific version of midi_handle().
     */

    jack_client_t * client_handle ()
    {
        return jack_data().m_jack_client;
    }

    const midi_jack_data & jack_data () const
    {
        return m_jack_data;
    }

    midi_jack_data & jack_data ()
    {
        return m_jack_data;
    }

    const std::string & remote_port_name () const
    {
        return m_remote_port_name;
    }

    void remote_port_name (const std::string & s)
    {
        m_remote_port_name = s;
    }

    /**
     * \getter m_jack_port
     *      This is the platform-specific version of midi_handle().
     */

    jack_port_t * port_handle ()
    {
        return jack_data().m_jack_port;
    }

protected:

    void client_handle (jack_client_t * handle)
    {
        jack_data().m_jack_client = handle;
    }

    void port_handle (jack_port_t * handle)
    {
        jack_data().m_jack_port = handle;
    }

    const midi_jack_info & jack_info () const
    {
        return m_jack_info;
    }

    midi_jack_info & jack_info ()
    {
        return m_jack_info;
    }

    const midi_jack_info::portlist & jack_ports () const
    {
        return jack_info().jack_ports();
    }

    midi_jack_info::portlist & jack_ports ()
    {
        return jack_info().jack_ports();
    }

    void close_client ();
    void close_port ();
    bool create_ringbuffer (size_t rbsize);
    bool connect_port
    (
        midibase::io iotype,
        const std::string & sourceportname,
        const std::string & destportname
    );
    bool register_port (midibase::io iotype, const std::string & portname);

protected:

    virtual bool api_connect () override;
    virtual bool api_init_out () override;
    virtual bool api_init_in () override;
    virtual bool api_init_out_sub () override;
    virtual bool api_init_in_sub () override;
    virtual bool api_deinit_out () override;
    virtual bool api_deinit_in () override;

    /**
     * \return
     *      Returns false, since this is an input function that is implemented
     *      fully only by midi_in_jack.
     */

    virtual bool api_get_midi_event (event *) override
    {
        return false;
    }

    virtual int api_poll_for_midi () override
    {
        return 0;
    }

    virtual void api_play (const event * e24, midibyte channel) override;
    virtual void api_sysex (const event * e24) override;
    virtual void api_flush () override;
    virtual void api_continue_from (midipulse tick, midipulse beats) override;
    virtual void api_start () override;
    virtual void api_stop () override;
    virtual void api_clock (midipulse tick) override;
    virtual void api_set_ppqn (int ppqn) override;
    virtual void api_set_beats_per_minute (midibpm bpm) override;
    virtual std::string api_get_port_name () override;

private:

    void send_byte (midipulse tick, midibyte evbyte);

    /*
     * Not used!
     *
     * void send_realtime_message (midibyte evbyte);
     */

    bool send_message (const midi_message & message);
    bool set_virtual_name (int portid, const std::string & portname);
    std::string details () const;

};          // class midi_jack

/**
 *  The class for handling JACK MIDI input.
 */

class midi_in_jack final : public midi_jack
{

protected:

    std::string m_client_name;

public:

    midi_in_jack (midibus & parentbus, midi_info & masterinfo);
    virtual ~midi_in_jack ();

    virtual int api_poll_for_midi () override;
    virtual bool api_get_midi_event (event *) override;

private:

};          // class midi_in_jack

/**
 *  The JACK MIDI output API class.
 */

class midi_out_jack final : public midi_jack
{

public:

    midi_out_jack (midibus & parentbus, midi_info & masterinfo);
    virtual ~midi_out_jack ();

};          // class midi_out_jack

}           // namespace seq66

#endif      //  SEQ66_JACK_SUPPORT

#endif      // SEQ66_MIDI_JACK_HPP

/*
 * midi_jack.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


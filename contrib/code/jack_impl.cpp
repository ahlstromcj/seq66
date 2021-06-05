
/*
 * Unused code from midi_jack.hpp and .cpp
 */

#if defined SEQ64_USE_OPEN_CLIENT_IMPL

    bool open_client_impl (bool input);     /* implements "connect()"   */

    /**
     *  This function is virtual, so we don't call it in the constructor,
     *  using open_client_impl() directly instead.  This function replaces the
     *  RtMidi function "connect()".
     */

    virtual bool open_client ()
    {
        return open_client_impl(SEQ66_MIDI_INPUT_PORT);
    }

    /**
     *  This function is virtual, so we don't call it in the constructor,
     *  using open_client_impl() directly instead.  This function replaces the
     *  RtMidi function "connect()".
     */

    virtual bool open_client ()
    {
        return open_client_impl(SEQ66_MIDI_OUTPUT_PORT);
    }

/**
 *  Opens input or output JACK clients, sets up the input or output callback,
 *  and actives the JACK client.  This code is combined from the former
 *  versions of the midi_in_jack::connect() and midi_out_jack::connect()
 *  functions for better readability and re-use in the input and output
 *  open_client() functions.
 *
 *  For input, it connects the MIDI input port.  The following calls are made:
 *
 *      -   jack_client_open(), to initialize JACK client.
 *      -   jack_set_process_callback(), to set jack_process_rtmidi_input() or
 *          jack_process_rtmidi_output().
 *
 *  For output, connects the MIDI output port.  The following calls are made:
 *
 *      -   jack_ringbuffer_create(), called twice, to initialize the
 *          output ringbuffers
 *      -   jack_client_open(), to initialize JACK client
 *      -   jack_set_process_callback(), to set jack_process_inpu()
 *
 *  Note that jack_activate() is no longer called for input or output.
 *  The call to jack_connect() is made in other functions.
 *  If the midi_jack_data client member is already set, this function returns
 *  immediately.  Only one client needs to be open for each midi_jack object.
 *
 *  Let's replace JackNullOption with JackNoStartServer.  We might also want to
 *  OR in the JackUseExactName option.
 *
 *  Which "client" name?  Let's start with the full name, connect_name().
 *  Is UUID an output-only, input-only option, or both?
 *
\verbatim
    const char * name = master_info().get_bus_name(bus_index()).c_str();
    const char * name = master_info().get_port_name(bus_index()).c_str();
\endverbatim
 *
 * \param input
 *      True if an input connection is to be made, and false if an output
 *      connection is to be made.
 */

bool
midi_jack::open_client_impl (bool input)
{
    bool result = true;
    master_midi_mode(input);
    if (is_nullptr(client_handle()))
    {
        std::string appname = rc().application_name();
        std::string clientname = rc().app_client_name();
        std::string rpname = remote_port_name();
        if (is_virtual_port())
        {
            set_alt_name(appname, clientname, rpname);
            parent_bus().set_alt_name(appname, clientname, rpname);
        }
        else
        {
            set_multi_name(appname, clientname, rpname);
            parent_bus().set_multi_name(appname, clientname, rpname);
        }

        const char * name = bus_name().c_str();
        jack_client_t * clipointer = create_jack_client(name);
        if (not_nullptr(clipointer))
        {
            client_handle(clipointer);
            if (input)
            {
                int rc = jack_set_process_callback
                (
                    clipointer, jack_process_rtmidi_input, &m_jack_data
                );
                if (rc != 0)
                {
                    m_error_string = "JACK error setting input callback";
                    error(rterror::WARNING, m_error_string);
                }
            }
            else
            {
                bool ok = create_ringbuffer(JACK_RINGBUFFER_SIZE);
                if (ok)
                {
                    int rc = jack_set_process_callback
                    (
                        clipointer, jack_process_rtmidi_output, &m_jack_data
                    );
                    if (rc != 0)
                    {
                        m_error_string = "JACK error setting output callback";
                        error(rterror::WARNING, m_error_string);
                    }
                }
            }
        }
    }
    return result;
}

#endif      // defined SEQ64_USE_OPEN_CLIENT_IMPL


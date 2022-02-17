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
 * \file          rtmidi.cpp
 *
 *    A class for managing various MIDI APIs.
 *
 * \library       seq66 application
 * \author        Gary P. Scavone; refactoring by Chris Ahlstrom
 * \date          2016-11-14
 * \updates       2022-02-01
 * \license       See above.
 *
 *  An abstract base class for realtime MIDI input/output.
 *
 *  This class implements some common functionality for the realtime
 *  MIDI input/output subclasses rtmidi_in and rtmidi_out.
 *
 *  RtMidi WWW site: http://music.mcgill.ca/~gary/rtmidi/
 *
 *  RtMidi:          realtime MIDI i/o C++ classes
 *
 *  GitHub issue #165: enabled a build and run with no JACK support.
 */

#include "cfg/settings.hpp"             /* seq66::rc().with_jack_...()      */
#include "rtmidi.hpp"                   /* seq66::rtmidi, etc.              */
#include "rtmidi_info.hpp"              /* seq66::rtmidi_info, etc.         */
#include "util/basic_macros.hpp"        /* C++ version of easy macros       */

#if defined SEQ66_JACK_SUPPORT
#include <jack/jack.h>                  /* jack_get_version_string()        */
#endif

#if defined SEQ66_BUILD_UNIX_JACK
#include "midi_jack.hpp"
#endif

#if defined SEQ66_BUILD_LINUX_ALSA
#include "midi_alsa.hpp"
#endif

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 * class rtmidi
 */

/**
 *  Default constructor.  We have commented out the call to
 *  jack_get_version_string() because it is not present in the original
 *  version of JACK.
 *
 * \param parentbus
 *      This is the midibus that the rtmidi object is going to implement, by
 *      forwarding calls to the selected MIDI subsystem (e.g. ALSA or JACK).
 *
 * \param info
 *      This object provides the system's enumerated busses/ports as found by
 *      the selected MIDI subsystem.
 */

rtmidi::rtmidi (midibus & parentbus, rtmidi_info & info) :
    midi_api        (parentbus, *(info.get_api_info())),
    m_rtmidi_info   (info),
    m_midi_api      (nullptr)           /* get_api() pointer accessor       */
{
#if defined SEQ66_JACK_GET_VERSION_STRING
    const char * jv = jack_get_version_string();
    if (not_nullptr(jv) && strlen(jv) > 0)
        set_jack_version(std::string(jv));
#else
    std::string jv{"JACK < v. 1"};
    set_jack_version(jv);
#endif

#if defined SND_LIB_VERSION_STR

    /*
     * Since this is build info, we do not use the run-time version of ALSA
     * that can be obtained from snd_asoundlib_version().
     */

    set_alsa_version(std::string(SND_LIB_VERSION_STR));

#endif
}

/**
 *  Destructor.
 */

rtmidi::~rtmidi ()
{
    delete_api();
}

/*
 * class rtmidi_in
 */

/**
 *  Constructs the desired MIDI API.
 *
 *  If no system support for the specified API value is found, we issue a
 *  warning and continue as if no API was specified.  In this case, we iterate
 *  through the compiled APIs and return as soon as we find one with at least
 *  one port or we reach the end of the list.
 *
 * \param parentbus
 *      This is the midibus that the rtmidi object is going to implement, by
 *      forwarding calls to the selected MIDI subsystem (e.g. ALSA or JACK).
 *
 * \param info
 *      Contains information about the existing ports and the selected MIDI
 *      API.  Actually, the selected MIDI API is a static value of the
 *      rtmidi_info class.
 *
 * \throw
 *      This function will throw an rterror object if it cannot find a MIDI
 *      API to use.
 */

rtmidi_in::rtmidi_in (midibus & parentbus, rtmidi_info & info) :
    rtmidi   (parentbus, info)
{
    if (rtmidi_info::selected_api() != rtmidi_api::unspecified)
    {
        openmidi_api(rtmidi_info::selected_api(), info);
        if (is_nullptr(get_api()))
        {
            errprintfunc("no system support for specified API");
        }
    }
    else
    {
        /*
         * Generally, we should have already selected the API at this point,
         * so this is fallback code.
         */

        rtmidi_api_list apis;
        rtmidi_info::get_compiled_api(apis);
        for (unsigned i = 0; i < apis.size(); ++i)
        {
            openmidi_api(apis[i], info);
            if (info.get_api_info()->get_port_count() > 0)
            {
                rtmidi_info::selected_api(apis[i]);  /* log API that worked */
                break;
            }
        }
        if (is_nullptr(get_api()))
        {
            /*
             * It should not be possible to get here because the preprocessor
             * definition SEQ66_BUILD_RTMIDI_DUMMY is automatically defined if no
             * API-specific definitions are passed to the compiler. But just in
             * case something weird happens, we'll throw an error.
             */

            std::string errortext = "no rtmidi API support found";
            throw(rterror(errortext, rterror::kind::unspecified));
        }
    }
}

/**
 *  A do-nothing virtual destructor.
 */

rtmidi_in::~rtmidi_in()
{
   // no code
}

/**
 *  Opens the desired MIDI API.
 *
 * \param api
 *      The enum value for the desired MIDI API.  If not specified, first JACK
 *      is tried, then ALSA.
 *
 * \param info
 *      Holds information about the API's setup (e.g. a list of the ALSA ports
 *      found on the system, the main ALSA "handle" pointer, plus the PPQN,
 *      BPM, and other information).
 */

void
rtmidi_in::openmidi_api (rtmidi_api api, rtmidi_info & info)
{
    bool got_an_api = false;
    if (not_nullptr(info.get_api_info()))
    {
        midi_info & midiinfo = *(info.get_api_info());
        delete_api();
        if (api == rtmidi_api::unspecified)
        {
            if (rc().with_jack_midi())
            {
                if (api == rtmidi_api::jack)
                {
#if defined SEQ66_BUILD_UNIX_JACK && defined SEQ66_JACK_SUPPORT
                    midi_in_jack * mijp = new (std::nothrow) midi_in_jack
                    (
                        parent_bus(), midiinfo
                    );
                    if (not_nullptr(mijp))
                    {
                        set_api(mijp);
                        got_an_api = true;
                    }
#endif
                }
            }
            if (! got_an_api)
            {
                if (api == rtmidi_api::alsa)
                {
#if defined SEQ66_BUILD_LINUX_ALSA
                    midi_in_alsa * miap = new (std::nothrow) midi_in_alsa
                    (
                        parent_bus(), midiinfo
                    );
                    if (not_nullptr(miap))
                    {
                        set_api(miap);
                        got_an_api = true;
                    }
#endif
                }
            }
        }
        else if (api == rtmidi_api::jack)
        {
#if defined SEQ66_BUILD_UNIX_JACK && defined SEQ66_JACK_SUPPORT
            midi_in_jack * mijp = new (std::nothrow) midi_in_jack
            (
                parent_bus(), midiinfo
            );
            if (not_nullptr(mijp))
                set_api(mijp);
#endif
        }
        else if (api == rtmidi_api::alsa)
        {
#if defined SEQ66_BUILD_LINUX_ALSA
            midi_in_alsa * miap = new (std::nothrow) midi_in_alsa
            (
                parent_bus(), midiinfo
            );
            if (not_nullptr(miap))
                set_api(miap);
#endif
        }
    }
}

/*
 * class rtmidi_out
 */

/**
 *  Principal constructor.  Attempt to open the specified API.  If there is no
 *  system support for the specified API value, then issue a warning and
 *  continue as if no API was specified.  In that case, we iterate through the
 *  compiled APIs and return as soon as we find one with at least one port or
 *  we reach the end of the list.
 *
 * \param parentbus
 *      This is the midibus that the rtmidi object is going to implement, by
 *      forwarding calls to the selected MIDI subsystem (e.g. ALSA or JACK).
 *
 * \param info
 *      Contains information about the existing ports and the selected MIDI
 *      API.  Actually, the selected MIDI API is a static value of the
 *      rtmidi_info class.
 *
 * \throw
 *      This function will throw an rterror object if it cannot find a MIDI
 *      API to use.
 */

rtmidi_out::rtmidi_out (midibus & parentbus, rtmidi_info & info) :
    rtmidi   (parentbus, info)
{
    if (rtmidi_info::selected_api() != rtmidi_api::unspecified)
    {
        openmidi_api(rtmidi_info::selected_api(), info);
        if (not_nullptr(get_api()))
        {
            return;
        }
        else
        {
            errprintfunc("no system support for specified API argument");
        }
    }

    /*
     * Generally, we should have already selected the API at this point,
     * so this is fallback code.
     */

    rtmidi_api_list apis;
    rtmidi_info::get_compiled_api(apis);
    for (unsigned i = 0; i < apis.size(); ++i)
    {
        openmidi_api(apis[i], info);
        if (info.get_api_info()->get_port_count() > 0)
        {
            rtmidi_info::selected_api(apis[i]); /* log the API that worked  */
            break;
        }
    }

    if (is_nullptr(get_api()))
    {
        /*
         * It should not be possible to get here because the preprocessor
         * definition SEQ66_BUILD_RTMIDI_DUMMY is automatically defined if no
         * API-specific definitions are passed to the compiler. But just in
         * case something weird happens, we'll thrown an error.
         */

        std::string errorText = "no rtmidi API support found";
        throw(rterror(errorText, rterror::kind::unspecified));
    }
}

/**
 *  A do-nothing virtual destructor.
 */

rtmidi_out::~rtmidi_out()
{
   // no code
}

/***
 *  Opens the desired MIDI API.
 *
 * \param api
 *      Provides the API to be constructed.
 *
 * \param info
 *      Provides information about the port.
 *
 * \throw
 *      This function will throw an rterror object if it cannot find a MIDI
 *      API to use.
 */

void
rtmidi_out::openmidi_api (rtmidi_api api, rtmidi_info & info)
{
    bool got_an_api = false;
    if (not_nullptr(info.get_api_info()))
    {
        midi_info & midiinfo = *(info.get_api_info());
        delete_api();
        if (api == rtmidi_api::unspecified)
        {
            if (rc().with_jack_midi())
            {
                if (api == rtmidi_api::jack)
                {
#if defined SEQ66_BUILD_UNIX_JACK && defined SEQ66_JACK_SUPPORT
                    midi_out_jack * mojp = new (std::nothrow) midi_out_jack
                    (
                        parent_bus(), midiinfo
                    );
                    if (not_nullptr(mojp))
                    {
                        set_api(mojp);
                        got_an_api = true;
                    }
#endif
                }
            }
            if (! got_an_api)
            {
                if (api == rtmidi_api::alsa)
                {
#if defined SEQ66_BUILD_LINUX_ALSA
                    midi_out_alsa * moap = new (std::nothrow) midi_out_alsa
                    (
                        parent_bus(), midiinfo
                    );
                    if (not_nullptr(moap))
                    {
                        set_api(moap);
                        got_an_api = true;
                    }
#endif
                }
            }
        }
        else if (api == rtmidi_api::jack)
        {
#if defined SEQ66_BUILD_UNIX_JACK
#if defined SEQ66_JACK_SUPPORT
            midi_out_jack * mojp = new (std::nothrow) midi_out_jack
            (
                parent_bus(), midiinfo
            );
            if (not_nullptr(mojp))
            {
                set_api(mojp);
                got_an_api = true;
            }
#endif
#endif
        }
        else if (api == rtmidi_api::alsa)
        {
#if defined SEQ66_BUILD_LINUX_ALSA
            midi_out_alsa * moap = new (std::nothrow) midi_out_alsa
            (
                parent_bus(), midiinfo
            );
            if (not_nullptr(moap))
            {
                set_api(moap);
                got_an_api = true;
            }
#endif
        }
    }
    if (! got_an_api)
    {
        errprintfunc("could not create an API");
    }
}

}           // namespace seq66

/*
 * rtmidi.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


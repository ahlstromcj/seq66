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
 * \file          rtmidi_info.cpp
 *
 *    A class for managing various MIDI APIs.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2016-12-08
 * \updates       2025-01-20
 * \license       See above.
 *
 *  An abstract base class for realtime MIDI input/output.  This class
 *  implements some common functionality enumerating MIDI clients and ports.
 *
 *  GitHub issue #165: enabled a build and run with no JACK support.
 */

#include "cfg/settings.hpp"             /* seq66::rc().with_jack_...()      */
#include "rtmidi_info.hpp"              /* seq66::rtmidi_info               */
#include "seq66_rtmidi_features.h"      /* selects the usable APIs          */
#include "util/basic_macros.hpp"        /* C++ version of easy macros       */

#if defined SEQ66_BUILD_LINUX_ALSA
#include "midi_alsa_info.hpp"
#endif

#if defined SEQ66_BUILD_UNIX_JACK
#include "midi_jack_info.hpp"
#endif

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Holds the selected API code.
 */

rtmidi_api rtmidi_info::sm_selected_api = rtmidi_api::unspecified;

/**
 *  This is a static function to replace the midi_api version.
 */

std::string
rtmidi_info::get_version ()
{
    return std::string(SEQ66_RTMIDI_VERSION);
}

/**
 *  Gets the list of APIs compiled into the application.  Note that we make
 *  ALSA versus JACK a runtime option as it is in the legacy Sequencer64
 *  application.
 *
 *  This is a static function to replace the midi_api version.
 *
 * \param apis
 *      The API structure.
 */

void
rtmidi_info::get_compiled_api (rtmidi_api_list & apis)
{
    apis.clear();

    /*
     * The order here will control the order of rtmidi's API search in the
     * constructor.  For Linux, we will try JACK first, then fall back to
     * ALSA, and then to the dummy implementation.  We were checking
     * rc().with_jack_transport(), but the "rc" configuration file has not yet
     * been read by the time we get to here.  On the other hand, we can make
     * it default to "true" and see what happens.
     *
     * However, it seems silly to not provide the API categorically.
     */

#if defined SEQ66_BUILD_UNIX_JACK
    apis.push_back(rtmidi_api::jack);
#endif

#if defined SEQ66_BUILD_LINUX_ALSA
    apis.push_back(rtmidi_api::alsa);
#endif

    if (apis.empty())
    {
        std::string errortext = "no rtmidi API found";
        throw(rterror(errortext, rterror::kind::unspecified));
    }
}

/**
 *  Default constructor.  Code basically cut-and-paste from rtmidi_in or
 *  rtmidi_out. Common code!
 */

rtmidi_info::rtmidi_info
(
    rtmidi_api api,
    const std::string & appname,
    int ppqn,
    midibpm bpm
) :
    m_info_api      (nullptr)
{
    if (api != rtmidi_api::unspecified)
    {
        bool ok = openmidi_api(api, appname, ppqn, bpm);
        if (ok)
        {
            if (not_nullptr(get_api_info()))        /* TODO: encapsulate    */
            {
                if (get_all_port_info() >= 0)
                {
                    selected_api(api);              /* log API that worked  */
                    return;
                }
            }
        }
        else
        {
            errprintfunc("No support for default MIDI API");
        }
    }

    rtmidi_api_list apis;
    get_compiled_api(apis);
    for (unsigned i = 0; i < apis.size(); ++i)
    {
        if (openmidi_api(apis[i], appname, ppqn, bpm)) // get_api_info()
        {
            /*
             * For JACK, or any other API, there may be no ports (from other
             * applications) yet in place.
             */

            if (not_nullptr(get_api_info()))        /* TODO: encapsulate    */
            {
                if (get_all_port_info() >= 0)
                {
                    selected_api(apis[i]);          /* log API that worked  */
                    break;
                }
            }
        }
        else
        {
            continue;
        }
    }
    if (is_nullptr(get_api_info()))
    {
        std::string errortext = "No rtmidi API found";
        throw(rterror(errortext, rterror::kind::unspecified));
    }
}

/**
 *  Destructor.  Gets rid of m_info_api and nullifies it.
 */

rtmidi_info::~rtmidi_info ()
{
    delete_api();
}

/**
 *  Opens the desired MIDI API.
 *
 *  If the JACK API is tried, and found missing, we turn off all of the other
 *  JACK flags found in the "rc" configuration file.  Also, the loop in the
 *  constructor will come back here to try the other compiled-in APIs
 *  (currently just ALSA).
 *
 * \param api
 *      The desired MIDI API.
 *
 * \param appname
 *      The name of the application, to be passed to the midi_info-derived
 *      constructor.
 *
 * \param ppqn
 *      The PPQN value to pass along to the midi_info_derived constructor.
 *
 * \param bpm
 *      The BPM (beats per minute) value to pass along to the
 *      midi_info_derived constructor.
 *
 * \return
 *      Returns true if a valid API is found.  A valid API is on that is both
 *      compiled into the application and is found existing on the host
 *      computer (system).
 */

bool
rtmidi_info::openmidi_api
(
    rtmidi_api api,
    const std::string & appname,
    int ppqn,
    midibpm bpm
)
{
    bool result = false;
    delete_api();

#if defined SEQ66_BUILD_UNIX_JACK
    if (api == rtmidi_api::jack)
    {
        if (rc().with_jack_midi())
        {
#if defined SEQ66_BUILD_UNIX_JACK && defined SEQ66_JACK_SUPPORT
            bool ok = detect_jack();
            if (ok)
            {
                midi_jack_info * mjip = new (std::nothrow) midi_jack_info
                (
                    appname, ppqn, bpm
                );
                result = not_nullptr(mjip);
                if (result)
                    result = set_api_info(mjip);
            }
#else
            result = false;
#endif
            if (! result)
            {
                /**
                 * Disables the usage of JACK MIDI for the rest of the program
                 * run.  This includes JACK Transport, which also obviously
                 * needs JACK to work.
                 */

                rc().with_jack_transport(false);
                rc().with_jack_master(false);
                rc().with_jack_master_cond(false);
                rc().with_jack_midi(false);
            }
        }
    }
#endif

#if defined SEQ66_BUILD_LINUX_ALSA
    if (api == rtmidi_api::alsa)
    {
        /*
         * ca 2023-04-15
         * Encountered a weird "No such device" error (even though MPD could
         * play music through pulseaudio. So now we check the handle.
         */

        midi_alsa_info * maip = new (std::nothrow) midi_alsa_info
        (
            appname, ppqn, bpm
        );
        result = not_nullptr_2(maip, maip->midi_handle());
        if (result)
        {
            result = set_api_info(maip);
            if (result)
                rc().with_alsa_midi(true);
        }
    }
#endif

    return result;
}

}           // namespace seq66

/*
 * rtmidi_info.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


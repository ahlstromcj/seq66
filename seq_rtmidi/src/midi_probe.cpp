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
 * \file          midi_probe.cpp
 *
 *  Functions to check MIDI inputs and outputs, based on the RtMidi test
 *  programs.
 *
 * \library       seq66 application
 * \author        Gary P. Scavone, 2003-2012; refactoring by Chris Ahlstrom
 * \date          2016-11-19
 * \updates       2022-03-13
 * \license       See above.
 *
 *  We include this test code in our library, rather than in a separate
 *  application, because we want to include some diagnostic code in the
 *  application.
 */

#include <iostream>
#include <cstdlib>
#include <map>

#include "midi_probe.hpp"
#include "midibus_rm.hpp"
#include "rtmidi.hpp"                   /* rtmidi_in and rt_midi_out        */
#include "util/basic_macros.hpp"

/*
 * Do not document the namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Function to get RtMidi API names in a reusable manner.
 *
 * \param i
 *      The integer value code for the desired API.  Must range from
 *      int(rtmidi_api::unspecified) to int(rtmidi_api::max) minus one.
 *
 * \return
 *      Returns a human-readable name for the API.
 */

std::string
midi_api_name (int i)
{
    static std::map<rtmidi_api, std::string> s_api_map;
    static bool s_map_is_initialized = false;
    if (! s_map_is_initialized)
    {
        s_api_map[rtmidi_api::unspecified]  = "Unspecified";
        s_api_map[rtmidi_api::alsa]         = "ALSA";
        s_api_map[rtmidi_api::jack]         = "Jack";

#if defined SEQ66_USE_RTMIDI_API_ALL
        /*
         * We're not supporting these until we get a simplified seq66-friendly
         * API worked out.  May take years to get to this.  Currently, Windows
         * is supported by our portmidi implementation.  So is OSX, but we
         * have no way to test it right now.
         */

        s_api_map[rtmidi_api::macosx_core]  = "OSX CoreMidi";
        s_api_map[rtmidi_api::windows_mm]   = "Win MultiMedia";
        s_api_map[rtmidi_api::dummy]        = "Dummy";
#endif

        s_map_is_initialized = true;
    }

    std::string result = "Unknown MIDI API";
    if (i >= int(rtmidi_api::unspecified) && i < int(rtmidi_api::max))
        result = s_api_map[rtmidi_api(i)];

    return result;
}

/**
 *  Formerly the main program of the RtMidi test program midiprobe.
 *  We will upgrade this function for some better testing eventually.
 *  It uses the functionality of the midi_info/rtmidi_info objects, plus
 *  its own version of some of that functionality.
 *
 * \return
 *      Currently always returns 0.
 */

int
midi_probe ()
{
    static rtmidi_info s_rtmidi_info_dummy
    (
        rtmidi_api::unspecified, "probe", 192, 120
    );
    static midibus s_midibus_dummy(s_rtmidi_info_dummy, 0);
    rtmidi_api_list apis;
    rtmidi_info::get_compiled_api(apis);
    std::cout << "\nCompiled APIs:\n";
    for (unsigned i = 0; i < apis.size(); ++i)
    {
        std::cout << "  " << midi_api_name(i) << std::endl;
    }

    try                         /* rtmidi constructors; exceptions possible */
    {
        rtmidi_info dummyinfo
        (
            rtmidi_api::unspecified, "probe", 192, 120
        );
        rtmidi_in midiin(s_midibus_dummy, dummyinfo);
        int index = api_to_int(rtmidi_info::selected_api());
        std::string name = midi_api_name(index);
        std::cout << "MIDI Input/Output API: " << name << std::endl ;

        int nports = midiin.get_port_count();
        std::cout << nports << " MIDI input sources:" << std::endl;
        for (int i = 0; i < nports; ++i)
        {
            std::string portname = midiin.get_port_name();
            std::cout
                << "  Input Port #" << i+1 << ": " << portname << std::endl
                ;
        }

        /*
         * We actually need to get this object in the loop!
         */

        rtmidi_out midiout(s_midibus_dummy, dummyinfo);
        std::cout << std::endl;

        nports = midiout.get_port_count();
        std::cout << nports << " MIDI output ports:" << std::endl;
        for (int i = 0; i < nports; ++i)
        {
            std::string portname = midiout.get_port_name();
            std::cout
                << "  Output Port #" << i+1 << ": " << portname << std::endl
                ;
        }
        std::cout << std::endl;
    }
    catch (const rterror & error)
    {
        error.print_message();
    }
    return 0;
}

}           // namespace seq66

/*
 * midi_probe.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


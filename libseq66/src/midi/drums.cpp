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
 * \file          drums.hpp
 *
 *  This module defines the array of MIDI drum names.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2026-02-16
 * \updates       2026-04-17
 * \license       GNU GPLv2 or above
 *
 *  This module is code extracted from the controllers module for better
 *  modularity.
 */

#include "midi/drums.hpp"             /* seq66::drum_name(), etc.           */
#include "midi/midibytes.hpp"         /* seq66::c_midibyte_data_max(), etc. */

namespace seq66
{

/**
 *  Note that the constructor and destructor are defaulted in the header
 *  file.
 */

bool
drums::add (int drumnumber, const std::string & drumname)
{
    bool result { drumnumber < int(c_midibyte_data_max) };
    if (result)
    {
        auto p { std::make_pair(drumnumber, drumname) };
        auto r { m_drum_map.insert(p) };
        result = r.second;
    }
    return result;
}

std::string
drums::name (int drumnumber) const
{
    auto it { m_drum_map.find(drumnumber) };
    return it == m_drum_map.end() ? "N/A" : it->second ;
}

std::string
drums::name_ex (int drumnumber) const
{
    std::string result { std::to_string(drumnumber) };
    auto it { m_drum_map.find(drumnumber) };
    result += " ";
    result += it == m_drum_map.end() ? "N/A" : it->second;
    return result;
}

/*
 *  A single global instance of the drums class
 */

static drums &
non_gm_drums ()
{
    static drums s_non_gm_drums;
    return s_non_gm_drums;
}

/**
 *  Works for GM drums, but other sets loaded might go beyond
 *  this range. We let end() determine if the drum number is
 *  not listed in the drum-set.
 */

#if defined USE_DRUM_IS_VALID_FUNCTION

/*
 * static const drumpair
 * c_gm_drum_names [c_midibyte_data_max] =
 */

static std::size_t s_gm_drum_min { 35 };
static std::size_t s_gm_drum_max { 81 };

static bool drum_is_valid (int drumnumber)
{
    return
    (
        std::size_t(drumnumber) >= s_gm_drum_min &&
        std::size_t(drumnumber) <= s_gm_drum_max
    );
}

#endif

/**
 *  Provides the default names of General MIDI drums.  Note that the
 *  numbering starts from 0 internally.  We could add support for this kind
 *  of list in usrsettings, or the note-mapper, or in a new 'drum'
 *  configuration file that holds this mapping.
 */

static drums::container s_gm_drum_names
{
    {  35, "Acoustic Bass Drum" },
    {  36, "Bass Drum 1"        },
    {  37, "Side Stick"         },
    {  38, "Acoustic Snare"     },
    {  39, "Hand Clap"          },
    {  40, "Electric Snare"     },
    {  41, "Low Floor Tom"      },
    {  42, "Closed Hi Hat"      },
    {  43, "High Floor Tom"     },
    {  44, "Pedal Hi-Hat"       },
    {  45, "Low Tom"            },
    {  46, "Open Hi-Hat"        },
    {  47, "Low-Mid Tom"        },
    {  48, "Hi Mid Tom"         },
    {  49, "Crash Cymbal 1"     },
    {  50, "High Tom"           },
    {  51, "Ride Cymbal 1"      },
    {  52, "Chinese Cymbal"     },
    {  53, "Ride Bell"          },
    {  54, "Tambourine"         },
    {  55, "Splash Cymbal"      },
    {  56, "Cowbell"            },
    {  57, "Crash Cymbal 2"     },
    {  58, "Vibraslap"          },
    {  59, "Ride Cymbal 2"      },
    {  60, "Hi Bongo"           },
    {  61, "Low Bongo"          },
    {  62, "Mute Hi Conga"      },
    {  63, "Open Hi Conga"      },
    {  64, "Low Conga"          },
    {  65, "High Timbale"       },
    {  66, "Low Timbale"        },
    {  67, "High Agogo"         },
    {  68, "Low Agogo"          },
    {  69, "Cabasa"             },
    {  70, "Maracas"            },
    {  71, "Short Whistle"      },
    {  72, "Long Whistle"       },
    {  73, "Short Guiro"        },
    {  74, "Long Guiro"         },
    {  75, "Claves"             },
    {  76, "Hi Wood Block"      },
    {  77, "Low Wood Block"     },
    {  78, "Mute Cuica"         },
    {  79, "Open Cuica"         },
    {  80, "Mute Triangle"      },
    {  81, "Open Triangle"      }
};

/**
 *  Adds a drum number/name pair to the non-GM map supported by the
 *  drums class. The drumnumber is not validated.
 */

bool
add_drum (int drumnumber, const std::string & drumname)
{
    bool result { non_gm_drums().add(drumnumber, drumname) };
    if (result)
        non_gm_drums().activate();

    return result;
}

/**
 *  Adds a comment to the non-GM drum set for the 'drums' file.
 */

void
set_drums_comment (const std::string & c)
{
    if (non_gm_drums().active())
        non_gm_drums().comments(c);
}

const std::string &
get_drums_comment ()
{
    static const std::string s_gm_comment
    {
        "Provides the internal set of GM drums."
    };
    return non_gm_drums().active() ?
        non_gm_drums().comments() : s_gm_comment ;
}

/**
 *  Returns the drum number plus the drum name for the hard-wired GM
 *  drum list. This function is used in displays and drop-down lists.
 *
 *  If the drum number isn't found, the empty string is returned? No.
 *  If the patch number isn't found, the name "N/A" is used.
 */

std::string
gm_drum_name (int drumnumber)
{
    std::string result;
    auto it { s_gm_drum_names.find(drumnumber) };
    result = std::to_string(drumnumber);
    result += " ";
    if (it == s_gm_drum_names.end())
        result += "N/A";                            /* result.clear()   */
    else
        result += it->second;

    return result;
}

/**
 *  This function returns the drum name and number from the user's
 *  loaded non-GM drum list, if active. Otherwise, it returns the
 *  drum name and number from the internal GM list.
 */

std::string
drum_name (int drumnumber)
{
    std::string result;
    result = non_gm_drums().active() ?
        non_gm_drums().name_ex(drumnumber) : gm_drum_name(drumnumber) ;

    return result;
}

/**
 *  This function creates a long string of all the drums in the 'drums'
 *  file format.
 *
 *  Note that it depends on having the full 128-count of drums.
 */

std::string
drum_list ()
{
    std::string result;
    const drums::container & active_drum_set
    {
        non_gm_drums().active() ? non_gm_drums().drum_map() : s_gm_drum_names
    };

    int drumnumber { 0 };
    for (const auto & p : active_drum_set)
    {
        auto it { s_gm_drum_names.find(drumnumber) };
        std::string gmname
        {
            it == s_gm_drum_names.end() ? "N/A" : it->second
        };
        std::string numb { std::to_string(drumnumber ) };
        result += "[Drum ";
        result += numb;
        result += "]\n\ngm-name = \"";
        result += gmname;
        result += "\"\n" "gm-drum = ";
        result += numb;
        result += "\ndev-name = \"";
        result += p.second;
        result += "\"\n\n";

        /*
         * We don't need to support a dev drum that matches the sound of a
         * GM drum. Let the user figure it out, if even needed.
         */

        ++drumnumber;
    }
    return result;
}

}           // namespace seq66

/*
 * drums.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

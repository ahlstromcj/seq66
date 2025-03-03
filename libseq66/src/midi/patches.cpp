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
 * \file          patches.hpp
 *
 *  This module defines the array of MIDI patch/program names.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2025-02-17
 * \updates       2025-02-19
 * \license       GNU GPLv2 or above
 *
 *  This module is code extracted from the controllers module for better
 *  modularity.
 */

#include "midi/patches.hpp"             /* seq66::controller_name(), etc.   */

namespace seq66
{

/**
 *  Note that the constructor and destructor are defaulted in the header
 *  file.
 */

bool
patches::add (int patchnumber, const std::string & patchname)
{
    midibyte pn = midibyte(patchnumber);
    bool result = pn < c_midibyte_data_max;
    if (result)
    {
        auto p = std::make_pair(patchnumber, patchname);
        auto r = m_patch_map.insert(p);
        result = r.second;
    }
    return result;
}

std::string
patches::name (int patchnumber) const
{
    auto it = m_patch_map.find(patchnumber);
    return it == m_patch_map.end() ? "N/A" : it->second ;
}

std::string
patches::name_ex (int patchnumber) const
{
    std::string result = std::to_string(patchnumber);
    auto it = m_patch_map.find(patchnumber);
    result += " ";
    result += it == m_patch_map.end() ? "N/A" : it->second;
    return result;
}

/*
 *  A single global instance of the patches class
 */

static patches &
non_gm_patches ()
{
    static patches s_non_gm_patches;
    return s_non_gm_patches;
}

/**
 *  Provides the default names of General MIDI program changes.  Note that the
 *  numbering starts from 0 internally.  We could add support for this kind of
 *  list in usrsettings, or the note-mapper, or in a new 'patch' configuration
 *  file that holds this mapping.
 */

/*
 * static const patchpair
 * c_gm_program_names [c_midibyte_data_max] =
 */

static patches::container s_gm_program_names
{
    {   0, "Acoustic Grand Piano"                 },
    {   1, "Bright Acoustic Piano"                },
    {   2, "Electric Grand Piano"                 },
    {   3, "Honky-tonk Piano"                     },
    {   4, "Electric Piano 1"                     },	// Rhodes Piano
    {   5, "Electric Piano 2"                     },	// Chorused Piano
    {   6, "Harpsichord"                          },
    {   7, "Clavinet"                             },	// Clavi
    {   8, "Celesta"                              },
    {   9, "Glockenspiel"                         },
    {  10, "Music Box"                            },
    {  11, "Vibraphone"                           },
    {  12, "Marimba"                              },
    {  13, "Xylophone"                            },
    {  14, "Tubular Bells"                        },
    {  15, "Dulcimer"                             },
    {  16, "Drawbar Organ"                        },	// Hammond Organ
    {  17, "Percussive Organ"                     },
    {  18, "Rock Organ"                           },
    {  19, "Church Organ"                         },
    {  20, "Reed Organ"                           },
    {  21, "Accordion"                            },
    {  22, "Harmonica"                            },
    {  23, "Tango Accordion"                      },
    {  24, "Acoustic Nylon Guitar"                },
    {  25, "Acoustic Steel Guitar"                },
    {  26, "Electric Jazz Guitar"                 },
    {  27, "Electric Clean Guitar"                },
    {  28, "Electric Muted Guitar"                },
    {  29, "Overdriven Guitar"                    },
    {  30, "Distortion Guitar"                    },
    {  31, "Guitar Harmonics"                     },
    {  32, "Acoustic Bass"                        },
    {  33, "Fingered Electric Bass"               },
    {  34, "Plucked Electric Bass"                },
    {  35, "Fretless Bass"                        },
    {  36, "Slap Bass 1"                          },
    {  37, "Slap Bass 2"                          },
    {  38, "Synth Bass 1"                         },
    {  39, "Synth Bass 2"                         },
    {  40, "Violin"                               },
    {  41, "Viola"                                },
    {  42, "Cello"                                },
    {  43, "Contrabass"                           },
    {  44, "Tremolo Strings"                      },
    {  45, "Pizzicato Strings"                    },
    {  46, "Orchestral Harp"                      },
    {  47, "Timpani"                              },
    {  48, "String Ensemble 1"                    },
    {  49, "String Ensemble 2"                    },
    {  50, "Synth Strings 1"                      },
    {  51, "Synth Strings 2"                      },
    {  52, "Choir Aah"                            },
    {  53, "Voice Ooh"                            },
    {  54, "Synth Voice"                          },
    {  55, "Orchestra Hit"                        },
    {  56, "Trumpet"                              },
    {  57, "Trombone"                             },
    {  58, "Tuba"                                 },
    {  59, "Muted Trumpet"                        },
    {  60, "French Horn"                          },
    {  61, "Brass Section"                        },
    {  62, "Synth Brass 1"                        },
    {  63, "Synth Brass 2"                        },
    {  64, "Soprano Sax"                          },
    {  65, "Alto Sax"                             },
    {  66, "Tenor Sax"                            },
    {  67, "Baritone Sax"                         },
    {  68, "Oboe"                                 },
    {  69, "English Horn"                         },
    {  70, "Bassoon"                              },
    {  71, "Clarinet"                             },
    {  72, "Piccolo"                              },
    {  73, "Flute"                                },
    {  74, "Recorder"                             },
    {  75, "Pan Flute"                            },
    {  76, "Bottle Blow"                          },
    {  77, "Shakuhachi"                           },
    {  78, "Whistle"                              },
    {  79, "Ocarina"                              },
    {  80, "Lead 1 (Square)"                      },
    {  81, "Lead 2 (Sawtooth)"                    },
    {  82, "Lead 3 (Calliope)"                    },
    {  83, "Lead 4 (Chiff)"                       },
    {  84, "Lead 5 (Charang)"                     },
    {  85, "Lead 6 (Voice)"                       },
    {  86, "Lead 7 (Fifths)"                      },
    {  87, "Lead 8 (bass)"                        },
    {  88, "Pad 1 (New Age)"                      },
    {  89, "Pad 2 (Warm)"                         },
    {  90, "Pad 3 (Polysynth)"                    },
    {  91, "Pad 4 (Choir)"                        },
    {  92, "Pad 5 (Bowed)"                        },
    {  93, "Pad 6 (Metallic)"                     },
    {  94, "Pad 7 (Halo)"                         },
    {  95, "Pad 8 (Sweep)"                        },
    {  96, "FX 1 (Rain)"                          },
    {  97, "FX 2 (Soundtrack)"                    },
    {  98, "FX 3 (Crystal)"                       },
    {  99, "FX 4 (Atmosphere)"                    },
    { 100, "FX 5 (Brightness)"                    },
    { 101, "FX 6 (Goblins)"                       },
    { 102, "FX 7 (Echoes)"                        },
    { 103, "FX 8 (Sci-Fi)"                        },
    { 104, "Sitar"                                },
    { 105, "Banjo"                                },
    { 106, "Shamisen"                             },
    { 107, "Koto"                                 },
    { 108, "Kalimba"                              },
    { 109, "Bagpipe"                              },
    { 110, "Fiddle"                               },
    { 111, "Shanai"                               },
    { 112, "Tinkle Bell"                          },
    { 113, "Agogo"                                },
    { 114, "Steel Drums"                          },
    { 115, "Woodblock"                            },
    { 116, "Taiko Drum"                           },
    { 117, "Melodic Tom"                          },
    { 118, "Synth Drum"                           },
    { 119, "Reverse Cymbal"                       },
    { 120, "Guitar Fret Noise"                    },
    { 121, "Breath Noise"                         },
    { 122, "Seashore"                             },
    { 123, "Bird Tweet"                           },
    { 124, "Telephone Ring"                       },
    { 125, "Helicopter"                           },
    { 126, "Applause"                             },
    { 127, "Gunshot"                              }
};

/**
 *  Adds a patch number/name pair to the non-GM map supported by the
 *  patches class.
 */

bool
add_patch (int patchnumber, const std::string & patchname)
{
    bool result = non_gm_patches().add(patchnumber, patchname);
    if (result)
        non_gm_patches().activate();

    return result;
}

/**
 *  Adds a comment to the non-GM patch set for the 'patches' file.
 */

void
set_patches_comment (const std::string & c)
{
    if (non_gm_patches().active())
        non_gm_patches().comments(c);
}

const std::string &
get_patches_comment ()
{
    static const std::string s_gm_comment
    {
        "Provides the internal set of GM patches."
    };
    return non_gm_patches().active() ?
        non_gm_patches().comments() : s_gm_comment ;
}

/**
 *  Returns the patch number plus the patch name for the hard-wired GM
 *  patch list. This function is used in displays and drop-down lists.
 *
 *  If the patch number isn't found, the name "N/A" is used.
 */

std::string
gm_program_name (int patchnumber)
{
    std::string result = std::to_string(patchnumber);
    auto it = s_gm_program_names.find(patchnumber);
    result += " ";
    if (it == s_gm_program_names.end())
        result += "N/A";
    else
        result += it->second;

    return result;
}

/**
 *  This function returns the patch name and number from the user's
 *  loaded non-GM patch list, if active. Otherwise, it returns the
 *  patch name and number from the internal GM list.
 */

std::string
program_name (int patchnumber)
{
    return non_gm_patches().active() ?
        non_gm_patches().name_ex(patchnumber) : gm_program_name(patchnumber) ;
}

/**
 *  This function creates a long string of all the patches in the 'patches'
 *  file format.
 *
 *  Note that it depends on having the full 128-count of patches.
 */

std::string
program_list ()
{
    std::string result;
    const patches::container & active_patch_set = non_gm_patches().active() ?
        non_gm_patches().patch_map() : s_gm_program_names ;

    int patchnumber = 0;
    for (const auto & p : active_patch_set)
    {
        auto it = s_gm_program_names.find(patchnumber);
        std::string gmname = it == s_gm_program_names.end() ?
            "N/A" : it->second ;

        std::string numb = std::to_string(patchnumber);
        result += "[Patch ";
        result += numb;
        result += "]\n\ngm-name = \"";
        result += gmname;
        result += "\"\n" "gm-patch = ";
        result += numb;
        result += "\ndev-name = \"";
        result += p.second;
        result += "\"\n\n";

        /*
         * We don't need to support a dev patch that matches the sound of a
         * GM patch. Let the user figure it out, if even needed.
         */

        ++patchnumber;
    }
    return result;
}

}           // namespace seq66

/*
 * patches.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


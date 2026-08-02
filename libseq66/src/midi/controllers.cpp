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
 * \file          controllers.hpp
 *
 *  This module defines the array of MIDI controller names.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-12-06
 * \updates       2026-08-02
 * \license       GNU GPLv2 or above
 *
 *  This definition used to reside in the controllers.hpp file, but now more
 *  than one module uses it, so we have to define it here to avoid a linker
 *  error about multiple definitions of this item.
 */

#include "midi/controllers.hpp"         /* seq66::controller_name(), etc.   */

namespace seq66
{

using namepair = struct
{
    int number;
    std::string name;
};

/**
 *  Provides the default names of MIDI controllers.  This array is used
 *  only by the functions below.
 *
 *  Also see:
 *
 *      https://www.paulcecchettimusic.com/full-list-of-midi-cc-numbers/
 */

static namepair
s_controller_names [c_midibyte_data_max]
{
    {   0, "Bank Select"                          },    // 0x00
    {   1, "Modulation Wheel"                     },
    {   2, "Breath controller"                    },
    {   3, "---"                                  },
    {   4, "Foot Pedal"                           },
    {   5, "Portamento Time"                      },
    {   6, "Data Entry Slider"                    },
    {   7, "Volume"                               },
    {   8, "Balance"                              },
    {   9, "---"                                  },
    {  10, "Pan position"                         },
    {  11, "Expression "                          },
    {  12, "Effect Control 1 "                    },
    {  13, "Effect Control 2 "                    },
    {  14, "---"                                  },
    {  15, "---"                                  },
    {  16, "General Purpose 1"                    },    // 0x10
    {  17, "General Purpose 2"                    },
    {  18, "General Purpose 3"                    },
    {  19, "General Purpose 4"                    },
    {  20, "---"                                  },
    {  21, "---"                                  },
    {  22, "---"                                  },
    {  23, "---"                                  },
    {  24, "---"                                  },
    {  25, "---"                                  },
    {  26, "---"                                  },
    {  27, "---"                                  },
    {  28, "---"                                  },
    {  29, "---"                                  },
    {  30, "---"                                  },
    {  31, "---"                                  },

    /*
     * 32-63 LSB for Controllers 0-31 (rarely implemented)
     */

    {  32, "Bank Select (fine)"                   },    // 0x20
    {  33, "Modulation Wheel (fine)"              },
    {  34, "Breath controller (fine)"             },
    {  35, "---"                                  },
    {  36, "Foot Pedal (fine)"                    },
    {  37, "Portamento Time (fine)"               },
    {  38, "Data Entry Slider (fine)"             },
    {  39, "Volume (fine)"                        },
    {  40, "Balance (fine)"                       },
    {  41, "---"                                  },
    {  42, "Pan position (fine)"                  },
    {  43, "Expression (fine)"                    },
    {  44, "Effect Control 1 (fine)"              },
    {  45, "Effect Control 2 (fine)"              },
    {  46, "---"                                  },
    {  47, "---"                                  },
    {  48, "---"                                  },    // 0x30
    {  49, "---"                                  },
    {  50, "---"                                  },
    {  51, "---"                                  },
    {  52, "---"                                  },
    {  53, "---"                                  },
    {  54, "---"                                  },
    {  55, "---"                                  },
    {  56, "---"                                  },
    {  57, "---"                                  },
    {  58, "---"                                  },
    {  59, "---"                                  },
    {  60, "---"                                  },
    {  61, "---"                                  },
    {  62, "---"                                  },
    {  63, "---"                                  },
    {  64, "Hold Pedal (on/off)"                  },    // 0x40
    {  65, "Portamento (on/off)"                  },
    {  66, "Sustenuto Pedal (on/off)"             },
    {  67, "Soft Pedal (on/off)"                  },
    {  68, "Legato Pedal (on/off)"                },
    {  69, "Hold 2 Pedal (on/off)"                },
    {  70, "Sound Variation"                      },
    {  71, "Sound Timbre"                         },
    {  72, "Sound Release Time"                   },
    {  73, "Sound Attack Time"                    },
    {  74, "Sound Brightness"                     },
    {  75, "Sound Control 6"                      },
    {  76, "Sound Control 7"                      },
    {  77, "Sound Control 8"                      },
    {  78, "Sound Control 9"                      },
    {  79, "Sound Control 10"                     },
    {  80, "General Purpose Button 1 (on/off)"    },    // 0x50
    {  81, "General Purpose Button 2 (on/off)"    },
    {  82, "General Purpose Button 3 (on/off)"    },
    {  83, "General Purpose Button 4 (on/off)"    },
    {  84, "---"                                  },
    {  85, "---"                                  },
    {  86, "---"                                  },
    {  87, "---"                                  },
    {  88, "---"                                  },
    {  89, "---"                                  },
    {  90, "---"                                  },
    {  91, "Effects Level"                        },
    {  92, "Tremulo Level"                        },
    {  93, "Chorus Level"                         },
    {  94, "Celeste Level"                        },
    {  95, "Phaser Level"                         },
    {  96, "Data Button Increment"                },    // 0x60
    {  97, "Data Button Decrement"                },
    {  98, "Non-registered Parameter (fine)"      },
    {  99, "Non-registered Parameter (coarse)"    },
    { 100, "Registered Parameter (fine)"          },
    { 101, "Registered Parameter (coarse)"        },
    { 102, "---"                                  },
    { 103, "---"                                  },
    { 104, "---"                                  },
    { 105, "---"                                  },
    { 106, "---"                                  },
    { 107, "---"                                  },
    { 108, "---"                                  },
    { 109, "---"                                  },
    { 110, "---"                                  },
    { 111, "---"                                  },
    { 112, "---"                                  },    // 0x70
    { 113, "---"                                  },
    { 114, "---"                                  },
    { 115, "---"                                  },
    { 116, "---"                                  },
    { 117, "---"                                  },
    { 118, "---"                                  },
    { 119, "---"                                  },
    { 120, "All Sound Off"                        },
    { 121, "All Controllers Off"                  },
    { 122, "Local Keyboard On/Off"                },
    { 123, "All Notes Off"                        },
    { 124, "Omni Mode Off"                        },
    { 125, "Omni Mode On"                         },
    { 126, "Mono On"                              },
    { 127, "Poly On"                              }     // 0x7F
};

std::string
controller_name (int index, bool usehex)
{
    std::string result;
    if (index >= 0 && index < c_midibyte_data_max)
    {
        std::string name { s_controller_names[index].name };
        if (usehex)
        {
            char tmp[32];
            (void) snprintf(tmp, sizeof tmp, "0x%02x", index);
            result = tmp;
        }
        else
            result = std::to_string(index);

        result += " ";
        result += name;
    }
    return result;
}

void
set_controller_name (int index, const std::string & newname)
{
    if (index >= 0 && index < c_midibyte_data_max)
        s_controller_names[index].name = newname;
}

using rpnpair = struct
{
    short number;
    std::string name;
};

static const int c_rpn_value_count { 9 };

static rpnpair
s_rpn_names [c_rpn_value_count]
{
    {   0x0000,     "Pitchbend range"               },
    {   0x0001,     "Channel fine Tuning"           },
    {   0x0002,     "Channel coarse Tuning"         },
    {   0x0003,     "Tuning program change"         },
    {   0x0004,     "Tuning bank select"            },
    {   0x0005,     "Modulation depth range"        },
    {   0x0006,     "Channel range"                 },        // ???
    {   0x007F,     "RPN parameter reset"           },
    {   0x3FFF,     "RPN null"                      }
};

std::string
rpn_name (int value)
{
    std::string result;
    for (auto p : s_rpn_names)
    {
        if (p.number == short(value))
        {
            result = p.name;
            break;
        }
    }

#if 0
    if (index == 0x3FFFF)
        index = c_rpn_value_count - 1;

    if (index >= 0 && index < c_rpn_value_count)
    {
        std::string name = s_rpn_names[index].name;
        result = std::to_string(index);
        result += " ";
        result += name;
    }
#endif
    return result;
}

/**
 *  Converts a 14-but RPN number to the MSB and LSB bytes.
 *
 *  Here is the process:
 *
 *  -   In binary, this is a 16-bit number.  0011111110000000.
 *  -   Ignore the two leading zeroes, i.e. it's a 14-bit number.
 *  -   Get the MSB.
 *          -   Get the next 7 bits.
 *          -   Prepend a 0.
 *  -   Get the LSB.
 *          -   Get the last 7 bits.
 *          -   Prepend a 0.
 *
 *        0MMMMMMM0LLLLLLL
 *                01111111 0x7F
 *
 * \param rpnn
 *      The 14-bit RPN number. It must be greater than zero and less
 *      than 16364 (0x4000).
 *
 * \param [out] out
 *
 * \return
 *      Returns the converted bytes. Holds the two bytes, with result[0]
 *      being the LSB, and result[1] being the MSB.
 */

midibytes
rpn_number_to_bytes (midishort rpnn)
{
    midibytes result;
    bool ok { rpnn < c_midishort_14_bad };
    if (ok)
    {
        midishort rpnn_lsb { midishort(rpnn & 0x7F) };
        midishort rpnn_msb { midishort((rpnn >> 7) & 0x7F) };
        result.push_back(midibyte(rpnn_lsb));
        result.push_back(midibyte(rpnn_msb));
    }
    return result;
}

midishort
bytes_to_rpn_number (const midibytes & in)
{
    midishort result { c_midishort_14_bad };    /* 0x4000 in midibytes.hpp  */
    if (in.size() > 1)
    {
        result = midishort(in[1] & 0x7F);       /* the MSB 7 bits           */
        result <<= 7;                           /* multiply by 128          */
        result += midishort(in[0]);
    }
    return result;
}

}           // namespace seq66

/*
 * controllers.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

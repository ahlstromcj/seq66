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
 * \updates       2021-07-22
 * \license       GNU GPLv2 or above
 *
 *  This definition used to reside in the controllers.hpp file, but now more
 *  than one module uses it, so we have to define it here to avoid a linker
 *  error about multiple definitions of this item.
 */

#include "midi/controllers.hpp"         /* seq66::controller_name(), etc.   */
#include "midi/midibytes.hpp"           /* seq66::midibyte type             */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

using namepair = struct
{
    midibyte number;
    std::string name;
};

/**
 *  Provides the default names of MIDI controllers.  This array is used
 *  only by the seqedit class.
 */

static const namepair
c_controller_names[c_midibyte_data_max] =
{
    {   0, "Bank Select"                          },
    {   1, "Modulation Wheel "                    },
    {   2, "Breath controller "                   },
    {   3, "---"                                  },
    {   4, "Foot Pedal "                          },
    {   5, "Portamento Time "                     },
    {   6, "Data Entry "                          },
    {   7, "Volume "                              },
    {   8, "Balance "                             },
    {   9, "---"                                  },
    {  10, "Pan position"                         },
    {  11, "Expression "                          },
    {  12, "Effect Control 1 "                    },
    {  13, "Effect Control 2 "                    },
    {  14, "---"                                  },
    {  15, "---"                                  },
    {  16, "General Purpose Slider 1"             },
    {  17, "General Purpose Slider 2"             },
    {  18, "General Purpose Slider 3"             },
    {  19, "General Purpose Slider 4"             },
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
    {  32, "Bank Select (fine)"                   },
    {  33, "Modulation Wheel (fine)"              },
    {  34, "Breath controller (fine)"             },
    {  35, "---"                                  },
    {  36, "Foot Pedal (fine)"                    },
    {  37, "Portamento Time (fine)"               },
    {  38, "Data Entry (fine)"                    },
    {  39, "Volume (fine)"                        },
    {  40, "Balance (fine)"                       },
    {  41, "---"                                  },
    {  42, "Pan position (fine)"                  },
    {  43, "Expression (fine)"                    },
    {  44, "Effect Control 1 (fine)"              },
    {  45, "Effect Control 2 (fine)"              },
    {  46, "---"                                  },
    {  47, "---"                                  },
    {  48, "---"                                  },
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
    {  64, "Hold Pedal (on/off)"                  },
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
    {  80, "General Purpose Button 1 (on/off)"    },
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
    {  96, "Data Button increment"                },
    {  97, "Data Button decrement"                },
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
    { 112, "---"                                  },
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
    { 127, "Poly On"                              }
};

std::string
controller_name (int index)
{
    std::string result;
    if (index < c_midibyte_data_max)
    {
        std::string name = c_controller_names[index].name;
        result = std::to_string(index);
        result += " ";
        result += name;
    }
    return result;
}

/**
 *  Provides the default names of General MIDI program changes.
 *  Note that the numbering starts from 0 internally.  We could
 *  add support for this kind of list in usrsettings, or the note-mapper,
 *  or in a new 'patch' configuration file that holds this mapping.
 */

static const namepair
c_gm_program_names[c_midibyte_data_max] =
{
    {   0, "Acoustic Grand Piano"                 },
    {   1, "Bright Acoustic Piano"                },
    {   2, "Electric Grand Piano"                 },
    {   3, "Honky-tonk Piano"                     },
    {   4, "Electric Piano 1"                     },
    {   5, "Electric Piano 2"                     },
    {   6, "Harpsichord"                          },
    {   7, "Clavi"                                },
    {   8, "Celesta"                              },
    {   9, "Glockenspiel"                         },
    {  10, "Music Box"                            },
    {  11, "Vibraphone"                           },
    {  12, "Marimba"                              },
    {  13, "Xylophone"                            },
    {  14, "Tubular Bells"                        },
    {  15, "Dulcimer"                             },
    {  16, "Drawbar Organ"                        },
    {  17, "Percussive Organ"                     },
    {  18, "Rock Organ"                           },
    {  19, "Church Organ"                         },
    {  20, "Reed Organ"                           },
    {  21, "Accordion"                            },
    {  22, "Harmonica"                            },
    {  23, "Tango Accordion"                      },
    {  24, "Acoustic Guitar (nylon)"              },
    {  25, "Acoustic Guitar (steel)"              },
    {  26, "Electric Guitar (jazz)"               },
    {  27, "Electric Guitar (clean)"              },
    {  28, "Electric Guitar (muted)"              },
    {  29, "Overdriven Guitar"                    },
    {  30, "Distortion Guitar"                    },
    {  31, "Guitar harmonics"                     },
    {  32, "Acoustic Bass"                        },
    {  33, "Electric Bass (finger)"               },
    {  34, "Electric Bass (pick)"                 },
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
    {  50, "SynthStrings 1"                       },
    {  51, "SynthStrings 2"                       },
    {  52, "Choir Aahs"                           },
    {  53, "Voice Oohs"                           },
    {  54, "Synth Voice"                          },
    {  55, "Orchestra Hit"                        },
    {  56, "Trumpet"                              },
    {  57, "Trombone"                             },
    {  58, "Tuba"                                 },
    {  59, "Muted Trumpet"                        },
    {  60, "French Horn"                          },
    {  61, "Brass Section"                        },
    {  62, "SynthBrass 1"                         },
    {  63, "SynthBrass 2"                         },
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
    {  76, "Blown Bottle"                         },
    {  77, "Shakuhachi"                           },
    {  78, "Whistle"                              },
    {  79, "Ocarina"                              },
    {  80, "Lead 1 (square)"                      },
    {  81, "Lead 2 (sawtooth)"                    },
    {  82, "Lead 3 (calliope)"                    },
    {  83, "Lead 4 (chiff)"                       },
    {  84, "Lead 5 (charang)"                     },
    {  85, "Lead 6 (voice)"                       },
    {  86, "Lead 7 (fifths)"                      },
    {  87, "Lead 8 (bass + lead)"                 },
    {  88, "Pad 1 (new age)"                      },
    {  89, "Pad 2 (warm)"                         },
    {  90, "Pad 3 (polysynth)"                    },
    {  91, "Pad 4 (choir)"                        },
    {  92, "Pad 5 (bowed)"                        },
    {  93, "Pad 6 (metallic)"                     },
    {  94, "Pad 7 (halo)"                         },
    {  95, "Pad 8 (sweep)"                        },
    {  96, "FX 1 (rain)"                          },
    {  97, "FX 2 (soundtrack)"                    },
    {  98, "FX 3 (crystal)"                       },
    {  99, "FX 4 (atmosphere)"                    },
    { 100, "FX 5 (brightness)"                    },
    { 101, "FX 6 (goblins)"                       },
    { 102, "FX 7 (echoes)"                        },
    { 103, "FX 8 (sci-fi)"                        },
    { 104, "Sitar"                                },
    { 105, "Banjo"                                },
    { 106, "Shamisen"                             },
    { 107, "Koto"                                 },
    { 108, "Kalimba"                              },
    { 109, "Bag pipe"                             },
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

std::string
gm_program_name (int index)
{
    std::string result;
    if (index < c_midibyte_data_max)
    {
        std::string name = c_gm_program_names[index].name;
        result = std::to_string(index);
        result += " ";
        result += name;
    }
    return result;
}

}           // namespace seq66

/*
 * controllers.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


/*
 *  This file is part of seq66.
 *
 *  seq66 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  seq66 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq66; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          midicontrol_unit_test.cpp
 * \library       Seqtool (from the Seq66 project)
 * \author        Chris Ahlstrom
 * \date          2018-11-11
 * \updates       2019-01-13
 * \version       $Revision$
 * \license       $XPC_SUITE_GPL_LICENSE$
 *
 *    This application provides unit tests for the midicontrol module of the
 *    libseq66 library.
 *
 * Unit Test Groups:
 *
 *      xx. seq66::keymap
 *          xx. Smoke Test
 *          xx. Gdk Translation
 *      xx. seq66::midicontrol
 *          xx. Smoke Test
 */

#include <iostream>
#include <iomanip>

#include "seq66-config.h"               /* SEQ66_SEQTOOL_TESTING_SUPPORT    */
#include "ctrl/keymap.hpp"              /* seq66::keymap functions          */
#include "ctrl/midicontainer.hpp"       /* seq66::midicontrol class         */

#if defined SEQ66_SEQTOOL_TESTING_SUPPORT

#include "midi_control_helpers.hpp"     /* formerly static test functions   */
#include "midi_control_unit_test.hpp"
#include "gdk_basic_keys.hpp"           /* seq66::gdk_key_name() function   */

/**
 *    Provides a basic "smoke test" for the keymap module.  A smoke test
 *    is a test of the basic functionality of the object or function.  It is an
 *    easy test that makes sure the code has basic integrity.  This test is
 *    about the simplest unit test function that can be written.
 *
 * \group 1. seq66::keymap
 *
 * \case
 *    1. Basic smoke test.
 *
 * \note
 *    It all of these unit-tests, it is important to understand that a
 *    status coming up invalid [i.e. status.valid() == false] is \e not a
 *    test failure -- it only indicates that the status object is invalid \e
 *    or that the test is not allowed to run.
 *
 * \tests
 *    -  seq66::keymap xxxxx() function
 *
 * \param options
 *    Provides the command-line options for the unit-test application.
 *
 * \return
 *    Returns the unit-test status object needed by the protocol.
 */

xpc::cut_status
midicontrol_unit_test_01_01 (const xpc::cut_options & options)
{
    xpc::cut_status status(options, 1, 1, "seq66::keymap", T_("Smoke Test"));
    bool ok = status.valid();                   /* invalidity not an error  */
    if (ok)
    {
        if (! status.can_proceed())             /* is test allowed to run?  */
        {
            status.pass();                      /* no, force it to pass     */
        }
        else
        {
            if (status.next_subtest("keymap: keymap_size()"))
            {
                ok = seq66::keymap_size() == 0; /* due to lazy evaluation   */
                if (ok)
                {
                    (void) seq66::qt_keyname_ordinal("BS");
                    ok = seq66::keymap_size() == 255;
                    if (! ok)
                        errprint("keymap_size() must return 255");
                }
                else
                    errprint("keymap_size() must return 0 at first");

                status.pass(ok);
            }
        }
    }
    return status;
}

struct gdk_keys
{
    unsigned gdk_key_value;
    std::string gdk_key_name;

    // std::string keymap_key_name;
};

/**
 *  Data for the next test.  Taken from a legacy "rc" file.  The keys are Gdk
 *  style numbering.
 */

static gdk_keys
sg_key_data [] =
{
    {  44,       ","        },
    {  49,       "1"        },
    {  50,       "2"        },
    {  51,       "3"        },
    {  52,       "4"        },
    {  53,       "5"        },
    {  54,       "6"        },
    {  55,       "7"        },
    {  56,       "8"        },
    {  97,       "a"        },
    {  98,       "b"        },
    {  99,       "c"        },
    { 100,       "d"        },
    { 101,       "e"        },
    { 102,       "f"        },
    { 103,       "g"        },
    { 104,       "h"        },
    { 105,       "i"        },
    { 106,       "j"        },
    { 107,       "k"        },
    { 109,       "m"        },
    { 110,       "n"        },
    { 113,       "q"        },
    { 114,       "r"        },
    { 115,       "s"        },
    { 116,       "t"        },
    { 117,       "u"        },
    { 118,       "v"        },
    { 119,       "w"        },
    { 120,       "x"        },
    { 121,       "y"        },
    { 122,       "z"        },
    {  33,       "!"        },
    {  34,       "\""       },
    {  35,       "#"        },
    {  36,       "$"        },
    {  37,       "%"        },
    {  38,       "&"        },
    {  40,       "("        },
    {  47,       "/"        },
    {  59,       ";"        },
    {  65,       "A"        },
    {  66,       "B"        },
    {  67,       "C"        },
    {  68,       "D"        },
    {  69,       "E"        },
    {  70,       "F"        },
    {  71,       "G"        },
    {  72,       "H"        },
    {  73,       "I"        },
    {  74,       "J"        },
    {  75,       "K"        },
    {  77,       "M"        },
    {  78,       "N"        },
    {  81,       "Q"        },
    {  82,       "R"        },
    {  83,       "S"        },
    {  84,       "T"        },
    {  85,       "U"        },
    {  86,       "V"        },
    {  87,       "W"        },
    {  88,       "X"        },
    {  89,       "Y"        },
    {  90,       "Z"        },
    {  39,       "'"        },
    {  59,       ";"        },
    {  93,       "]"        },
    {  91,       "["        },
    { 65360,     "Home"     },
    { 236,       "igrave"   },
    { 65535,     "Delete"   },
    { 65379,     "Insert"   },
    { 65429,     "KP_Home"  },
    { 111,       "o"        },
    { 65379,     "Insert"   },
    {  92,       "\\"       },
    {  32,       " "        },
    { 65307,     "Escape"   },
    {  46,       "."        },
    {  61,       "="        },
    {  45,       "-"        },
    {  47,       "/"        },
    { 65470,     "F1"       },
    { 65471,     "F2"       },
    { 65472,     "F3"       },
    { 65473,     "F4"       },
    { 65475,     "F6"       },
    { 65474,     "F5"       },
    { 65476,     "F7"       },
    { 65478,     "F9"       },
    { 65477,     "F8"       },
    {  80,       "P"        },
    {  48,       "0"        },
    {   0,       "EOL"      }

};          // sg_key_data[]

/**
 *    Provides a basic test for the keymap gdk functions.
 *
 * \group 1. seq66::keymap
 *
 * \case
 *    2. Gdk translation
 *
 * \tests
 *    -  seq66::keymap gdk_key_name() function
 *
 * \param options
 *    Provides the command-line options for the unit-test application.
 *
 * \return
 *    Returns the unit-test status object needed by the protocol.
 */

xpc::cut_status
midicontrol_unit_test_01_02 (const xpc::cut_options & options)
{
    xpc::cut_status status(options, 1, 2, "seq66::keymap", T_("Gdk Test"));
    bool ok = status.valid();                   /* invalidity not an error  */
    if (ok)
    {
        if (! status.can_proceed())             /* is test allowed to run?  */
        {
            status.pass();                      /* no, force it to pass     */
        }
        else
        {
            if (status.next_subtest("keymap: gdk_key_name()"))
            {
                ok = seq66::keymap_size() == 255; /* due to lazy evaluation   */
                if (ok)
                {
                    if (options.is_verbose())
                    {
                        for (int index = 0; ; ++index)
                        {
                            unsigned knumber = sg_key_data[index].gdk_key_value;
                            std::string kname = sg_key_data[index].gdk_key_name;
                            if (knumber > 0)
                            {
                                std::cout
                                    << "Key " << std::setw(5) << knumber
                                    << " " << std::setw(8) << kname
                                    << " --> '" << seq66::gdk_key_name(knumber)
                                    << "'"
                                    << std::endl
                                    ;
                            }
                            else
                                break;
                        }
                    }
                }
                else
                    errprint("keymap_size() must return 255");

                status.pass(ok);
            }
        }
    }
    return status;
}

/**
 *    Provides a basic "smoke test" for the midicontrol module.  A smoke test
 *    is a test of the basic functionality of the object or function.  It is an
 *    easy test that makes sure the code has basic integrity.  This test is
 *    about the simplest unit test function that can be written.
 *
 * \group 2. seq66::midicontrol
 *
 * \case
 *    1. Basic smoke test.
 *
 * \note
 *    It all of these unit-tests, it is important to understand that a
 *    status coming up invalid [i.e. status.valid() == false] is \e not a
 *    test failure -- it only indicates that the status object is invalid \e
 *    or that the test is not allowed to run.
 *
 * \tests
 *    -  seq66::midicontrol::midicontrol()
 *
 * \param options
 *    Provides the command-line options for the unit-test application.
 *
 * \return
 *    Returns the unit-test status object needed by the protocol.
 */

xpc::cut_status
midicontrol_unit_test_02_01 (const xpc::cut_options & options)
{
    xpc::cut_status status(options, 2, 1, "seq66::midicontrol", T_("Smoke Test"));
    bool ok = status.valid();                   /* invalidity not an error */
    if (ok)
    {
        if (! status.can_proceed())             /* is test allowed to run? */
        {
            status.pass();                      /* no, force it to pass    */
        }
        else
        {
            if (status.next_subtest("midicontrol::midicontrol()"))
            {
                seq66::midicontrol k;
                status.pass(ok);
            }
        }
    }
    return status;
}

#endif      // SEQ66_SEQTOOL_TESTING_SUPPORT

/*
 * midicontrol_unit_test.cpp
 *
 * vim: ts=4 sw=4 et ft=cpp
 */

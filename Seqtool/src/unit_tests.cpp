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
 * \file          unit_tests.cpp
 * \library       Seqtool (from the Seq66 project)
 * \author        Chris Ahlstrom
 * \date          2018-11-11
 * \updates       2018-12-25
 * \version       $Revision$
 * \license       $XPC_SUITE_GPL_LICENSE$
 *
 *    This application provides a few unit tests of the libseq66
 *    library module.
 */

#include <string>

#include "seq66-config.h"
#include "midi_control_unit_test.hpp"
#include "unit_tests.hpp"
#include "util_unit_test.hpp"

#if defined SEQ66_SEQTOOL_TESTING_SUPPORT

/**
 *    This is the test routine for the seqtool application.
 *
 * \note
 *    Parse for any additional (non-unittest) arguments.  Don't try to find
 *    erroneous options in this loop.  If a valid option is found, then turn
 *    off the complaint flag to avoid error messages.  Note that a better
 *    way to do this work is to derive a class from unit-test and extend it
 *    to support the additional variables.  (In this case, it is likely that
 *    one would also want to extend the TestOptions class.
 *
 * \return
 *    Returns POSIX_SUCCESS (0) if the function succeeds.  Other values,
 *    including possible error-codes, are returned otherwise.
 *
 */

#define SEQ66_TEST_NAME          "seqtool"
#define SEQ66_TEST_VERSION       0.90.0
#define DEFAULT_AUTHOR           "Chris Ahlstrom"

int
unit_tests
(
   int argc,               /**< Number of command-line arguments.             */
   char * argv []          /**< The actual array of command-line arguments.   */
)
{
   xpc::cut testbattery
   (
      argc, argv,
      std::string(SEQ66_TEST_NAME),
      std::string(XPCCUT_VERSION_STRING(SEQ66_TEST_VERSION)),
      std::string("No additional_help.")
   );
   bool ok = testbattery.valid();
   if (ok)
   {
      int argcount = argc;
      char ** arglist = argv;
      cbool_t load_the_tests = true;
      if (argcount > 1)
      {
         int currentarg = 1;
         while (currentarg < argcount)
         {
            std::string arg = arglist[currentarg];
            if (arg == "--none")
            {
               // currentarg++;
               // if ((currentarg < argcount) && (arglist[currentarg] != 0))
               //    strcpy(gsBasename, arglist[currentarg]);
            }
            currentarg++;
         }
      }
      if (load_the_tests)
      {
         /*
          * Unit tests for the click module/group/class:
          */

         ok = testbattery.load(midicontrol_unit_test_01_01);
         if (ok) ok = testbattery.load(midicontrol_unit_test_01_02);
         if (ok) ok = testbattery.load(midicontrol_unit_test_02_01);

         if (ok) ok = testbattery.load(util_unit_test_01_01);
         if (ok) ok = testbattery.load(util_unit_test_01_02);
         if (ok) ok = testbattery.load(util_unit_test_01_03);
      }
      if (ok)
      {
         ok = testbattery.run();
         if (! ok)
         {
            xpccut_errprint
            (
               "Some tests failed, but be aware that currently the unit-test\n"
               "application must be run from the libseq66/src directory in\n"
               "order to succeed, due to accessing a test file.  Will fix that\n"
               "issue at some point.\n"
            );
         }
      }
      else
         xpccut_errprint("load of the unit-test functions failed");
   }
   return ok ? EXIT_SUCCESS : EXIT_FAILURE ;
}

#endif      // SEQ66_SEQTOOL_TESTING_SUPPORT

/*
 * unit_tests.cpp
 *
 * vim: ts=4 sw=4 et ft=cpp
 */


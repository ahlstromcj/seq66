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
 * \file          seqtool.cpp
 * \library       Seqtool (from the Seq66 project)
 * \author        Chris Ahlstrom
 * \date          2018-11-11
 * \updates       2019-04-11
 * \version       $Revision$
 * \license       $XPC_SUITE_GPL_LICENSE$
 *
 *    This application provides a few unit tests of the libseq66
 *    library module and some handy functions for converting from Seq66
 *    to Seq66.
 *
 *    One new dependency is the xpccut++ test library, which needs to be
 *    downloaded to a directory parallel with the seq66 directory, then built
 *    and install (via ./configure, make, and make install).
 */

#include <QApplication>                 /* #include <QCoreApplication>      */

#include <cstdlib>                      /* for the EXIT_SUCCESS value       */
#include <iostream>                     /* std::cout                        */

#include "seq66_platform_macros.h"      /* detect the compiler environment  */

#if defined SEQ66_PLATFORM_UNIX || defined SEQ66_PLATFORM_MINGW
#include <getopt.h>
#endif

/*
 * This faker class is now used in qtestframe.
 */

#if defined USE_FAKER_CLASS             /* use it here or in qtestframe?    */
#include "faker.hpp"                    /* proof-of-concept class faker     */
#endif

#include "seq66_features.hpp"           /* seq66::set_app_name()            */
#include "cfg/midicontrolfile.hpp"      /* seq66::midicontrolfile           */
#include "cfg/rcfile.hpp"               /* seq66::rcfile class              */
#include "cfg/settings.hpp"             /* seq66::rcsettings, usrsettings   */
#include "converter.hpp"                /* seq66::converter class           */
#include "ctrl/keymap.hpp"              /* seq66::qt_key_name() etc.        */
#include "midi/event.hpp"               /* seq66::event class               */
#include "play/performer.hpp"           /* seq66 performer class            */
#include "qtestframe.hpp"               /* seq66::qtestframe GUI class      */
#include "unit_tests.hpp"
#include "util/victor.hpp"              /* seq66::victor<> 2-D vector       */

/**
 *    This is the main routine for the seqtool application.
 *
 * \return
 *    Returns POSIX_SUCCESS (0) if the function succeeds.  Other values,
 *    including possible error-codes, are returned otherwise.
 *
 *      #define SEQ66_APP_NAME           "seqtool"
 */

#define SEQ66_TEST_NAME                 "seqtool"
#define SEQ66_TEST_VERSION              0.90.1
#define DEFAULT_AUTHOR                  "Chris Ahlstrom"

/**
 *  The single-character command-line options.
 */

static const std::string s_short_options = "c:fhk:o:p:t6";

/**
 *  The double-dash word command-line options.
 */

static struct option s_long_options [] =
{
    { "control",    required_argument,  0, 'c'   },
    { "frame",      no_argument,        0, 'f'   },
    { "help",       no_argument,        0, 'h'   },
    { "convert",    required_argument,  0, 'k'   },
    { "out",        required_argument,  0, 'o'   },
    { "parse",      required_argument,  0, 'p'   },
    { "tests",      no_argument,        0, 't'   },
    { "66",         no_argument,        0, '6'   },     /* stock conversion */
    { 0, 0, 0, 0 }
};

/**
 *  Help!
 */

static void
s_help ()
{
    std::cout <<
"Usage: seqtool [ options ]\n\n"
"  --control, -c  Read the MIDI control file as a test, allowing inactive\n"
"                 control values to be read as well.\n"
"  --convert, -k  Convert a seq66 configuration rc file to the new format.\n"
"                 Requires the base name of the old configuration file as a\n"
"                 parameter, for example '--convert seq66'.\n"
"  --out, -o      Provides the output base name desired for the conversion\n"
"                 output.  Defaults to 'seq66'.\n"
"  --parse        Same as --control, but reads only active control values.\n"
"  --frame, -f    Run the qtestframe user-interface.\n"
    ;

#if defined SEQ66_SEQTOOL_TESTING_SUPPORT
    std::cout <<
"  --tests, -t    Run the unit tests for parts of Seq66. If the --convert option\n"
"                 was also specified, run a test of reading and writing the\n"
"                 configuration files (the so-called 'copy test').\n"
    ;
#endif

    std::cout <<
"  --help, -h     Show this help.  To show extensive help for --tests, use\n"
"                 '--tests --help'.\n"
"\n"
    ;
}

/**
 *  Globals!  But but, they're static!
 */

static bool sg_do_testing = false;
static bool sg_do_copy_test = false;
static bool sg_do_frame = false;
static bool sg_do_help = false;
static bool sg_do_control = false;
static bool sg_do_parse = false;
static bool sg_do_convert = false;
static std::string sg_control_file = "";
static std::string sg_rc_in_file_base = "seq66";
static std::string sg_rc_out_file_base = "seq66";

/**
 *  Options!
 */

static bool
get_options
(
    int argc,           /**< Number of command-line arguments.              */
    char * argv []      /**< The actual array of command-line arguments.    */
)
{
    bool result = false;
    bool have_options = true;
    int option_index = 0;
    opterr = 0;
    do
    {
	    int c = getopt_long
        (
            argc, argv, s_short_options.c_str(), s_long_options, &option_index
        );
        have_options = c != (-1);
        if (have_options)
        {
            result = true;
            switch (c)
            {
            case 0:

                if (s_long_options[option_index].flag != 0)
                {
                    have_options = false;
                }
                break;

            case '6':
                sg_do_convert = true;
                sg_rc_in_file_base = std::string("seq66");
                sg_rc_out_file_base = std::string("test");
                break;

            case 'c':

                if (not_nullptr(optarg))
                {
                    sg_control_file = std::string(optarg);
                    sg_do_control = true;
                }
                else
                {
                    errprint("--control requires a file-name");
                }
                break;

            case 'f':

                sg_do_frame = true;
                break;

            case 'k':

                if (not_nullptr(optarg))
                {
                    sg_do_convert = true;
                    sg_rc_in_file_base = std::string(optarg);
                }
                else
                {
                    sg_do_convert = false;
                    errprint("--convert requires an input file-name base");
                }
                break;

            case 'o':

                if (not_nullptr(optarg))
                {
                    sg_rc_out_file_base = std::string(optarg);
                }
                else
                {
                    sg_do_convert = false;
                    errprint("--out requires a base-name, conversion disabled");
                }
                break;

            case 'p':

                sg_do_parse = true;
                break;

            case 't':

                sg_do_testing = true;
                break;

            case 'h':

                sg_do_help = true;
                break;
            }
        }

    } while (have_options);

    return result;
}

/**
 *  Ops!  This section merely exercises some proof-of-concept functions that we
 *  don't really need anymore.
 */

static void
operator_exercise ()
{
#if defined USE_FAKER_CLASS             /* use it here or qtestframe?   */
    faker f;
    f.create_static_op();
    f.create_member_op();
    f.create_lambda_op();
#endif
}

/**
 *  Victory!
 */

static void
victor_test ()
{
#if defined USE_VICTOR_TEST             /* just a quick check of victor */
    seq66::victor<int> testmatrix(4, 5, 99);
    testmatrix.set(1, 1, 11);
    testmatrix(2, 2) = 22;
    std::cout << "seq66::victor<int> testmatrix(4, 5, 99);\n"
    std::cout << "Value[1][1] = " << testmatrix(1, 1) << "\n"
    std::cout << "Value[2][2] = " << testmatrix(2, 2) << "\n"
    std::cout << "Value[3][3] = " << testmatrix(3, 3) << std::endl;
#endif
}

/**
 *  Main!
 *
 *      std::vector<std::string> args(argv, argv + argc);
 */

int
main
(
    int argc,           /**< Number of command-line arguments.              */
    char * argv []      /**< The actual array of command-line arguments.    */
)
{
    bool ok = true;
    seq66::set_app_name("seqtool");

	/*
     * Not needed so far, we use local instance of these settings objects.
     *
    seq66::rc().set_defaults();             // start out with normal values
    seq66::usr().set_defaults();            // ditto
     */

    (void) get_options(argc, argv);
    if (sg_do_testing)
        sg_do_help = false;

    if (sg_do_help)
    {
        s_help();
    }
    else
    {
        if (sg_do_testing && sg_do_convert)
        {
            sg_do_copy_test = true;
            sg_do_testing = sg_do_convert = false;
        }
        if (sg_do_testing)
        {
            /*
             * I forget what these do. :-D
             */

            operator_exercise();
            victor_test();

            infoprint("Processing --test option...");
#if defined SEQ66_SEQTOOL_TESTING_SUPPORT
            int rcode = unit_tests(argc, argv);
            ok = is_posix_success(rcode);
#else
            errprint("Application not built for testing!");
            ok = false;
#endif
        }
        else if (sg_do_frame)
        {
            QApplication app(argc, argv); //  qtcore_task task(&app);
            qtestframe tframe;
            infoprint("Processing --frame option...");
            tframe.show();
            ok = app.exec();
        }
        else if (sg_do_control)
        {
            seq66::performer p;
            seq66::midicontrolfile file(sg_control_file, seq66::rc(), true);
            infoprint("Processing --control option...");
            bool reddit = file.parse();         // allow inactive!!!
            (void) p.get_settings(seq66::rc());
            infoprint("KEYS");
            p.key_controls().show();
            infoprint("MIDI");
            p.midi_controls().show();
            if (reddit)
            {
                /*
                 *  Create a loop 1 "q" control event and simulated processing
                 *  it.
                 */

                seq66::event ev;
                ev.set_timestamp(12345678);
                ev.set_status(144);             /* Note On, Ch. 0   */
                ev.set_data(1, 0);
                reddit = p.midi_control_event(ev);
                std::cout
                    << "MIDI control event status: " << reddit << std::endl;
            }
        }
        else if (sg_do_convert)
        {
            seq66::rcsettings rcs;
            seq66::converter cvt(rcs, sg_rc_in_file_base, sg_rc_out_file_base);
            infoprint("Processing --convert/--out options...");
            if (cvt.parse())
            {
                if (cvt.write())
                {
                    infoprint("converter:write() succeeded");
                }
                else
                {
                    errprint("converter:write() failed");
                }
            }
            else
            {
                errprint("converter::parse() failed");
            }
        }
        else if (sg_do_copy_test)
        {
            seq66::rcsettings rcs;
            seq66::usrsettings usrs;
            rcs.set_defaults();
            usrs.set_defaults();
            infoprint("Processing copy-test option...");
        }
        else if (sg_do_parse)
        {
            /*
             * The current default test is to read the MIDI control file.
             */

            seq66::midicontrolfile file("contrib/control-map.rc", seq66::rc());
            infoprint("Processing --parse option...");
            (void) file.parse();
        }
    }
    return ok ? EXIT_SUCCESS : EXIT_FAILURE ;
}

/*
 * seqtool.cpp
 *
 * vim: ts=4 sw=4 et ft=cpp
 */


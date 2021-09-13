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
 * \file          cmdlineopts.cpp
 *
 *  This module moves the command-line options processing in seq66.cpp
 *  into the libseq66 library.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-11-20
 * \updates       2021-09-13
 * \license       GNU GPLv2 or above
 *
 *  The "rc" command-line options override setting that are first read from
 *  the "rc" configuration file.  These modified settings are always saved
 *  when Seq66 exits, on the theory that somebody may have modified these
 *  settings in the user-interface (there is currently no "dirty flag" for
 *  this operation), and that command-line modifications to system-dependent
 *  settings such as the JACK setup should be saved for convenience.
 *
 *  The "user" settings are mostly not available from the command-line (--bus
 *  being one exception).  They, too, are partly system-dependent, but there
 *  is no user-interface for changing the "user" options at this time.  So the
 *  "user" configuration file is not saved unless it does not exist in the
 *  first place, or the "--user-save" option isprovided on the ommand line.
 *
 *  We should backup the old versions of any saved configuration files at some
 *  point.
 */

#include <iostream>                     /* std::cout, etc.                  */
#include <string.h>                     /* strlen() <gasp!>                 */

#include "cfg/cmdlineopts.hpp"          /* this module's header file        */
#include "cfg/rcfile.hpp"               /* seq66::rcfile class              */
#include "cfg/settings.hpp"             /* seq66::rc() and usr() access     */
#include "cfg/usrfile.hpp"              /* seq66::usrfile class             */
#include "util/basic_macros.hpp"        /* not_nullptr() and other macros   */
#include "util/filefunctions.hpp"       /* file_read_writable(), etc.       */
#include "util/strfunctions.hpp"        /* string-to-numbers functions      */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Provides a return value for parse_command_line_options() that indicates a
 *  help-related option was specified.
 */

static const int c_null_option = 99999;

/**
 *  Sets up the "hardwired" version text for Seq66.  This value
 *  ultimately comes from the configure.ac script, and available in the
 *  seq66_features module.
 */

const std::string
cmdlineopts::versiontext = seq_version_text();

/**
 *  A structure for command parsing that provides the long forms of
 *  command-line arguments, and associates them with their respective
 *  short form.  Note the terminating null structure.
 */

struct option
cmdlineopts::s_long_options [] =
{
    {"help",                0, 0, 'h'},
    {"version",             0, 0, 'V'},
    {"verbose",             0, 0, 'v'},
    {"investigate",         0, 0, 'i'},
    {"home",                required_argument, 0, 'H'},
#if defined SEQ66_NSM_SUPPORT
    {"no-nsm",              0, 0, 'T'},
    {"nsm",                 0, 0, 'n'},
#endif
    {"bus",                 required_argument, 0, 'b'},
    {"buss",                required_argument, 0, 'B'},
    {"client-name",         required_argument, 0, 'l'},
    {"ppqn",                required_argument, 0, 'q'},
    {"show-midi",           0, 0, 's'},
    {"show-keys",           0, 0, 'k'},
    {"inverse",             0, 0, 'K'},
    {"priority",            0, 0, 'p'},
    {"interaction-method",  required_argument, 0, 'x'},
    {"playlist",            required_argument, 0, 'X'},
    {"jack-start-mode",     required_argument, 0, 'M'},
#if defined SEQ66_JACK_SUPPORT
    {"jack-transport",      0, 0, 'j'},
    {"jack-slave",          0, 0, 'S'},
    {"no-jack-transport",   0, 0, 'g'},
    {"jack-master",         0, 0, 'J'},
    {"jack-master-cond",    0, 0, 'C'},
#if defined SEQ66_JACK_SESSION
    {"jack-session",        required_argument, 0, 'U'},
#endif
    {"no-jack-midi",        0, 0, 'N'},
    {"jack-midi",           0, 0, 't'},
#endif
    {"manual-ports",        0, 0, 'm'},
    {"auto-ports",          0, 0, 'a'},
    {"reveal-ports",        0, 0, 'r'},
    {"hide-ports",          0, 0, 'R'},
    {"alsa",                0, 0, 'A'},
    {"pass-sysex",          0, 0, 'P'},
    {"user-save",           0, 0, 'u'},
    {"record-by-channel",   0, 0, 'd'},
    {"legacy-record",       0, 0, 'D'},
    {"config",              required_argument, 0, 'c'},
    {"rc",                  required_argument, 0, 'f'},
    {"usr",                 required_argument, 0, 'F'},
    {"User",                0, 0, 'Z'},
    {"Native",              0, 0, 'z'},

    /*
     * New app-specific options, for easier expansion.  The -o/--option
     * processing is mostly handled outside of the get-opt setup, because it
     * can disable detection of a MIDI file-name argument.
     */

    {"option",              0, 0, 'o'},                 /* expansion!       */
    {0, 0, 0, 0}                                        /* terminator       */
};

/**
 *  Provides a complete list of the short options, and is passed to
 *  getopt_long().  The following string keeps track of the characters used so
 *  far.  An 'x' means the character is used.  An 'a' indicates we could
 *  repurpose the key with minimal impact. An asterisk indicates the option is
 *  reserved for application-specific options.  Currently we will use it for
 *  options like "daemonize" in the seq66cli application.
 *
\verbatim
        @AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz#
         xxxxxx x  xx xxx xxxxx lxxxx *xx xxxxxxxxxxx  xx  aax
\endverbatim
 *
 *  * Note that 'o' options arguments cannot be included here due to issues
 *  involving parse_o_options(), but 'o' is *reserved* here, without the
 *  argument indicator.
 *
 * Manual + User mode '-Z' versus Auto + Native mode '-z':
 *
 *      Creates virtual ports '-m' and hides the native names for the ports '-R'
 *      in favor of the 'usr' definition of the names of ports and channels.
 *      The opposite (native) setting uses '-a' and '-r'.
 *      Both modes turn on the --user-save (-u) option.
 *
 * Investigate:
 *
 *      The undocumented -i/--investigate option is used (only on the command
 *      line), to turn on the test-of-the-day and try to unearth
 *      difficult-to-find issues. Shhhh, it's a secret!
 */

const std::string
cmdlineopts::s_arg_list =
    "AaB:b:Cc:dF:f:gH:hiJjKkl:M:mNnoPpq:RrSsTtU:uVvX:x:Zz#";

/**
 *  Provides help text.
 */

static const std::string s_help_1a =
"Options:\n"
"   -h, --help               Show this help and exit.\n"
"   -V, --version            Show program version/build information and exit.\n"
"   -v, --verbose            Verbose mode, show more data to the console.\n"
#if defined SEQ66_NSM_SUPPORT
"   -n, --nsm                Activate Non Session Manager support.\n"
"   -T, --no-nsm             Ignore NSM in 'usr' file. T for 'typical'.\n"
#endif
"   -X, --playlist filename  Load playlists from the configuration directory.\n"
"   -m, --manual-ports       Don't auto-connect MIDI ports; use virtual ports.\n"
"                            Not supported in PortMidi builds.\n"
"   -a, --auto-ports         Connect MIDI ports (overrides the 'rc' file).\n"
;

/**
 *  More help text.
 */

static const std::string s_help_1b =
"   -r, --reveal-ports       Do not use the 'usr' definitions for port names.\n"
"   -R, --hide-ports         Use the 'usr' definitions for port names.\n"
"   -A, --alsa               Do not use JACK, use ALSA. A sticky option.\n"
"   -b, --bus b              Global override of bus number (for testing).\n"
"   -B, --buss b             Covers the 'bus' versus 'buss' confusion.\n"
"   -l, --client-name label  Use this name instead of 'seq66'. Overridden by a\n"
"                            session manager.\n"
"   -q, --ppqn qn            Specify default PPQN to replace 192.  The MIDI\n"
"                            file might specify its own PPQN.\n"
"   -p, --priority           Run high priority, FIFO scheduler (needs root).\n"
"   -P, --pass-sysex         Passes incoming SysEx messages to all outputs.\n"
"                            Not yet fully implemented.\n"
"   -s, --show-midi          Dump incoming MIDI events to the screen.\n"
;

/*
 * This option was never used, just settable, in Seq24.  We need that letter!
 *
 *      "   -i, --ignore n           Ignore ALSA device number.\n"
 */

/**
 *  Still more help text.
 */

static const std::string s_help_2 =
"   -k, --show-keys          Prints pressed key value.\n"
"   -K, --inverse            Inverse/night color scheme for seq/perf editors.\n"
"   -M, --jack-start-mode m  For ALSA or JACK, these play modes are available:\n"
"                            'live'; 'song'; and 'auto' (default).\n"
#if defined SEQ66_JACK_SUPPORT
"   -S, --jack-slave         Synchronize to JACK transport (as Slave).\n"
"   -j, --jack-transport     The legacy (and deprecated) form of --jack-slave.\n"
"   -g, --no-jack-transport  Turn off JACK transport.\n"
"   -J, --jack-master        Try to be JACK Master. Also sets -j.\n"
"   -C, --jack-master-cond   Fail if there's already a Jack Master; sets -j.\n"
"   -N, --no-jack-midi       Use ALSA MIDI, even with JACK Transport. See -A.\n"
"   -t, --jack-midi          Use JACK MIDI, separately from JACK Transport.\n"
#if defined SEQ66_JACK_SESSION
"   -U, --jack-session uuid  Set UUID for JACK session. This turns on JACK\n"
"                            session management, and disables NSM. Set it to\n"
"                            'on' to enable; JACK sets the UUID later.\n"
#endif
#endif
"   -d, --record-by-channel  Divert MIDI input by channel into the sequences\n"
"                            configured for each channel.\n"
"   -D, --legacy-record      Record all MIDI into the active sequence. Default.\n"
;

/**
 *  Still still more help text.
 */

static const std::string s_help_3 =
"   -u, --user-save          Save 'usr' settings, usually saved only if they\n"
"                            do not exist; 'usr' command-line options are not\n"
"                            permanent otherwise.\n"
"   -H, --home dir           Set directory for configuration files. It is\n"
"                            $HOME/.config/seq66 by default.\n"
"   -f, --rc filename        An alternate 'rc' file, in $HOME/.config/seq66 or\n"
"                            the --home directory. '.rc' extension is enforced.\n"
"   -F, --usr filename       Use a different 'usr' configuration file.  Same\n"
"                            rules as for the --rc option.\n"
"   -c, --config basename    Change 'rc', 'usr', ... base file names. Any\n"
"                            extension is stripped starting at the last period.\n"
"   -o, --option optoken     Provides app-specific options for expansion.  The\n"
"                            options supported are:\n"
"\n"
    ;

/**
 *  Still still more more help text.
 */

static const std::string s_help_4a =
"      log=filename  Redirect console output to a log file in home. If no\n"
"                    '=filename' is provided, the filename in '[user-options]'\n"
"                    in the 'usr' file is used.\n"
"      sets=RxC      Change set rows and columns from 4x8. R can be 4 to 12;\n"
"                    C can be 4 to to 12. If not 4x8, call it 'variset' mode.\n"
"                    Affects mute groups, too.\n"
;

static const std::string s_help_4b =
"      scale=x.y     Scales size of main window. Range: 0.5 to 3.0.\n"
"      mutes=value   Saving of mute-groups: 'mutes', 'midi', or 'both'.\n"
"      virtual=o,i   Like --manual-ports, except that the count of output and\n"
"                    input ports are specified. Defaults are 8 & 4, respectively.\n"
"\n"
" seq66cli:\n"
"      daemonize     Makes this application fork to the background.\n"
"      no-daemonize  Or not.  These options do not apply to Windows.\n"
"\n"
"The 'daemonize' option works only in the CLI build. The 'sets' option works in\n"
"the CLI build.  Specify the '--user-save' option to make these options\n"
"permanent in the seq66.usr configuration file.\n"
"\n"
;

/**
 *  Still still still more more more help text.
 */

static const std::string s_help_5 =
"Saving a MIDI file saves the current PPQN value. If no JACK options are shown\n"
"above, they were disabled in the build configuration. Command-line options can\n"
"be sticky; many of them are saved to the configuration\files when Seq66 exits.\n"
"See the Seq66 User Manual.\n"
;

/**
 *  Outputs the help text.
 */

void
cmdlineopts::show_help ()
{
    std::cout
        << seq_app_name() << " v " << seq_version()
        << " A reboot of the seq24 live sequencer.\n"
        << "Usage: " << seq_app_name() << " [options] [MIDI filename]\n"
        << s_help_1a << s_help_1b << s_help_2 << s_help_3
        << s_help_4a << s_help_4b << s_help_5
        ;
}

/**
 *  Gets a compound option argument.  An option argument is a value flagged on
 *  the command line by the -o/--option options.  Each option has a value
 *  associated with it, as the next argument on the command-line.  A compound
 *  option is of the form name=value, of which one example is:
 *
 *      log=filename
 *
 *  This function extracts both the name and the value.  If the name is empty,
 *  then the option is unsupported and should be ignored.  If the value is
 *  empty, then there may be a default value to use.
 *
 * \param compound
 *      The putative compound option.
 *
 * \param [out] optionname
 *      This value is filled in with the name of the option-value, if there
 *      is an equals sign in the \a compound parameter.
 *
 * \return
 *      Returns the value part of the compound option, or just the compound
 *      parameter is there is no '=' sign.  That is, it returns the
 *      option-value.
 *
 * \sideeffect
 *      The name portion is returned in the optionname parameter.
 */

std::string
cmdlineopts::get_compound_option
(
    const std::string & compound,
    std::string & optionname
)
{
    std::string value;
    auto eqpos = compound.find_first_of("=");
    if (eqpos == std::string::npos)
    {
        optionname.clear();
        value = compound;
    }
    else
    {
        optionname = compound.substr(0, eqpos); /* beginning to eqpos chars */
        value = compound.substr(eqpos + 1);     /* rest of the string       */
    }
    return value;
}

/**
 *  Checks to see if the first option is a help or version argument, just so
 *  we can skip the "Reading configuration ..." messages.  Also check for the
 *  "?" option that people sometimes use as a guess to get help.
 *
 * \param argc
 *      The number of command-line arguments.
 *
 * \param argv
 *      The array of command-line argument pointers.
 *
 * \return
 *      Returns true only if -v, -V, --version, -#, -h, --help, or "?" were
 *      encountered.
 */

bool
cmdlineopts::help_check (int argc, char * argv [])
{
    bool result = false;
    for ( ; argc > 1; --argc)
    {
        std::string arg = argv[argc - 1];
        if
        (
            (arg == "-h") || (arg == "--help") ||
            (arg == "-V") || (arg == "--version") ||
            (arg == "-#")
        )
        {
            result = true;
        }
        else if (arg == "?")
        {
            show_help();
            result = true;
            break;
        }
    }
    return result;
}

/**
 *  Checks the putative command-line arguments for any of the new "options"
 *  options.  These are flagged by "-o" or "--option".  These options are then
 *  passed to the usrsettings or rcsettings modules.  Multiple options can be
 *  supplied; the whole command-line is processed.
 *
 * \param argc
 *      The number of command-line parameters, including the name of the
 *      application as parameter 0.
 *
 * \param argv
 *      The array of pointers to the command-line parameters.
 *
 * \return
 *      Returns true if any "options" option was found, and false otherwise.
 *      If the options flags ("-o" or "--option") were found, but were not
 *      valid options, then we break out of processing and return false.
 */

bool
cmdlineopts::parse_o_options (int argc, char * argv [])
{
    bool result = false;
    if (argc > 1 && not_nullptr(argv))
    {
        int argn = 1;
        std::string arg;
        std::string optionname;
        while (argn < argc)
        {
            if (not_nullptr(argv[argn]))
            {
                arg = argv[argn];
                if ((arg == "-o") || (arg == "--option"))
                {
                    ++argn;
                    if (argn < argc && not_nullptr(argv[argn]))
                    {
                        arg = get_compound_option(argv[argn], optionname);
                        if (optionname.empty())
                        {
                            if (arg == "daemonize")
                            {
                                result = true;
                                usr().option_daemonize(true);
                            }
                            else if (arg == "no-daemonize")
                            {
                                result = true;
                                usr().option_daemonize(false);
                            }
                            else if (arg == "log")
                            {
                                /*
                                 * Without a filename, just turn on the
                                 * log-file flag, using the name already read
                                 * from the "[user-options]" section of the
                                 * "usr" file.
                                 */

                                result = true;
                                usr().option_use_logfile(true);
                            }
                        }
                        else
                        {
                            /*
                             * \tricky
                             *      Note that 'arg' is used in the clause
                             *      above, but 'optionname' is used here.
                             */

                            if (optionname == "log")
                            {
                                result = true;
                                arg = strip_quotes(arg);
                                usr().option_logfile(arg);
                                if (! arg.empty())
                                    usr().option_use_logfile(true);
                            }
                            else if (optionname == "sets")
                            {
                                result = parse_o_sets(arg);
                            }
                            else if (optionname == "scale")
                            {
                                if (arg.length() >= 1)
                                    result = usr().parse_window_scale(arg);
                            }
                            else if (optionname == "mutes")
                            {
                                result = parse_o_mutes(arg);
                            }
                            else if (optionname == "virtual")
                            {
                                result = parse_o_virtual(arg);
                            }
                        }
                        if (! result)
                        {
                            errprint("Warning: unsupported --option value");
                            break;
                        }
                    }
                }
            }
            else
                break;

            ++argn;
        }
    }
    return result;
}

/**
 *  Checks the putative command-line arguments for the "log" option.  Generally,
 *  this function needs to be called near the beginning of main().  See the
 *  rtmidi version of the main() function, for example.
 *
 * \param argc
 *      The number of command-line parameters, including the name of the
 *      application as parameter 0.
 *
 * \param argv
 *      The array of pointers to the command-line parameters.
 *
 * \return
 *      Returns true if stdio was rerouted to the "usr"-specified log-file.
 */

bool
cmdlineopts::parse_log_option (int argc, char * argv [])
{
    bool result = false;
    std::string exename = argv[0];
    if (contains(exename, "verbose"))       /* symlink to dev's program     */
    {
#if defined SEQ66_PLATFORM_DEBUG
        file_message("Running debug version", argv[0]);
#else
        file_message("Running", argv[0]);
#endif
        rc().verbose(true);                 /* turn on is_debug() output    */
        file_message(exename, "Verbose mode enabled");
    }
    if (parse_o_options(argc, argv))
    {
        std::string logfile = usr().option_logfile();
        if (! logfile.empty())
            result = true;
    }
    return result;
}

/**
 *  Provides the command-line option support, as well as some setup support,
 *  extracted from the main routine of Seq66.
 *
 *  It also requires the caller to call rc().set_defaults() and
 *  usr().set_defaults() at the appropriate time, which is before any parsing
 *  of the command-line options.  The caller can then use the command-line to
 *  make any modifications to the setting that will be used here.  The biggest
 *  example is the -r/--reveal-ports option, which determines if the MIDI
 *  buss definition strings are read from the 'usr' configuration file.
 *
 *  Instead of the legacy Seq24 names, we use the new configuration
 *  file-names, located in the ~/.config/seq66 directory. If they are not
 *  found, we no longer fall back to the Seq24 configuration file-names.  The
 *  code also ensures the directory exists.  See the rcsettings class for how
 *  this works.
 *
\verbatim
        std::string cfg_dir = seq66::rc().home_config_directory();
        if (cfg_dir.empty())
            return EXIT_FAILURE;
\endverbatim
 *
 *  We were parsing the user-file first, but we now need to parse the rc-file
 *  first, to get the manual-ports option, so that we can avoid overriding
 *  the port names that the ALSA system provides, if the manual-option is
 *  false.
 *
 * \param [out] errmessage
 *      Provides a string into which to dump any error-message that might
 *      occur.  Still not thoroughly supported in the "rc" and "user"
 *      configuration files.
 *
 * \param argc
 *      The number of command-line arguments.
 *
 * \param argv
 *      The array of command-line argument pointers.
 *
 * \return
 *      Returns true if the reading of both configuration files succeeded, or
 *      if they did not exist.  In the latter case, they will be saved as new
 *      files upon exit.
 */

bool
cmdlineopts::parse_options_files (std::string & errmessage)
{
    std::string rcn = rc().config_filespec();
    bool result = true;
    if (file_read_writable(rcn))
    {
        rcfile options(rcn, rc());
        file_message("Reading rc", rcn);
        if (options.parse())
        {
            /*
             * Nothing to do?
             */
        }
        else
        {
            /*
             * Let's continue with the defaults if there is no usable 'rc'
             * file, rather than skipping a lot of other setup, which can
             * cause mysterious segfaults.
             *
             * result = false;
             */

            errmessage = options.error_message();
        }
    }
    else
    {
        /**
         *  If no "rc" file exists, then, of course, we will create them upon
         *  exit.  But we also want to force the new style of output, where the
         *  key/MIDI controls and the mute-groups are in separate files ending
         *  with the extensions "ctrl" and "mutes", respectively.  Also, the
         *  mute-groups container is populated with zero values.  Finally, the
         *  user may have specified an altername configuration-file name on
         *  the command-line (e.g. "myseq66" versus the default, "qseq66",
         *  which is the application name.
         *
         *      std::string cfgname = rc().application_name();
         */

        std::string cfgname = rc().config_filename();       /* "xyzt.rc"    */
        cfgname = filename_base(cfgname, true);             /* strip ".rc"  */

        std::string cf = file_extension_set(cfgname, ".ctrl");
        std::string mf = file_extension_set(cfgname, ".mutes");
        std::string uf = file_extension_set(cfgname, ".usr");
        std::string pl = file_extension_set(cfgname, ".playlist");
        std::string nm = file_extension_set(cfgname, ".drums");
        std::string af = cfgname + ".rc/ctrl/midi/mutes";
        /*
         * af += "/drums/playlist", later maybe?
         */

        rc().midi_control_filename(cf);
        rc().mute_group_filename(mf);
        rc().user_filename(uf);
        rc().playlist_filename(pl);
        rc().notemap_filename(nm);
        rc().mute_groups().reset_defaults();
        file_message("No rc file, will create", af);
    }
    if (result)
    {
        rcn = rc().user_filespec();
        if (file_read_writable(rcn))
        {
            usrfile ufile(rcn, rc());
            file_message("Reading usr", rcn);
            if (ufile.parse())
            {
                /*
                 * Since we are parsing this file after the creation of the
                 * performer object, we may need to modify some performer
                 * members here.  These changes must also be made after
                 * parsing the command line in the main module (e.g. in
                 * seq66rtmidi.cpp).
                 */
            }
            else
            {
                errmessage = ufile.error_message();
                result = false;
            }
        }
        else
        {
            file_message("No usr file, will create", rcn);
        }
    }
    return result;
}

bool
cmdlineopts::parse_o_sets (const std::string & arg)
{
    bool result = arg.length() >= 3;
    if (result)
    {
        int rows = string_to_int(arg);
        auto p = arg.find_first_of("x");
        if (p != std::string::npos)
        {
            int cols = string_to_int(arg.substr(p+1));
            usr().mainwnd_rows(rows);
            usr().mainwnd_cols(cols);
#if defined SEQ66_USE_AUTO_SCALING
            /*
             * This works for FHD screens (1920 x 1080).
             */

            if (rows > 4)
            {
                float scale = float(rows) / 4.0f;
                float scaley = 1.0f;
                if (scale > 1.5)
                    scale = 1.0;

                usr().window_scale(scale, scaley);
            }
#endif
        }
        result = false;
    }
    return result;
}

bool
cmdlineopts::parse_o_mutes (const std::string & arg)
{
    bool result = arg == "mutes" || arg == "midi" || arg == "both";
    if (result)
    {
        rc().mute_groups().group_save(arg);
    }
    return result;
}

bool
cmdlineopts::parse_o_virtual (const std::string & arg)
{
    int out = 0, in = 0;
    rc().manual_ports(true);
    if (! arg.empty())
    {
        out = string_to_int(arg);
        auto p = arg.find_first_of(",");
        if (p != std::string::npos)
            in = string_to_int(arg.substr(p+1));
    }
    rc().manual_port_count(out);
    rc().manual_in_port_count(in);
    return true;
}

/**
 *  Like parse_options_files(), but reads only the [mute-group] section.
 *
 * \param errmessage
 *      A return parameter for any error message that might occur.
 *
 * \return
 *      Returns true if no errors occurred in reading the mute-groups.
 *      If not true, the caller should output the error message.
 */

bool
cmdlineopts::parse_mute_groups
(
    rcsettings & rcs,
    std::string & errmessage
)
{
    bool result = true;
    std::string rcn = rc().config_filespec();
    if (file_read_writable(rcn))
    {
        rcfile options(rcn, rcs);
        file_message("Reading mutes", rcn);
        if (options.parse_mute_group_section(rcn, true))
        {
            // Nothing to do?
        }
        else
        {
            errmessage = options.error_message();
            result = false;
        }
    }
    return result;
}

/**
 *  Parses the command-line options on behalf of the application.  Note that,
 *  since we call this function twice (once before the configuration files are
 *  parsed, and once after), we have to make sure that the global value optind
 *  is reset to 0 before calling this function.  Note that the traditional
 *  reset value for optind is 1, but 0 is used in GNU code to trigger the
 *  internal initialization routine of get_opt().
 *
 * Hidden values per getopt(3):
 *
 *      extern char *optarg;
 *      extern int optind, opterr, optopt;
 *
 *  At the end, optind is the index in argv of the first argv element that is
 *  not an option.
 *
 * \param argc
 *      The number of command-line arguments.
 *
 * \param argv
 *      The array of command-line argument pointers.
 *
 * \return
 *      Returns the value of optind if no help-related options were provided.
 */

int
cmdlineopts::parse_command_line_options (int argc, char * argv [])
{
    int result = 0;
    int option_index = 0;                   /* getopt_long index storage    */
    std::string optionval;                  /* used only with -o options    */
    std::string optionname;                 /* ditto                        */
    optind = 1;                             /* reset global for (re)scan    */
    for (;;)                                /* scan all arguments           */
    {
        int c = getopt_long
        (
            argc, argv,
            s_arg_list.c_str(),         /* e.g. "Crb:q:Li:jM:pU:Vx:..." */
            s_long_options, &option_index
        );
        if (c == (-1))
            break;

        std::string soptarg;
        if (not_nullptr(optarg))
            soptarg = std::string(optarg);

        switch (c)
        {
        case 'A':
            rc().with_jack_transport(false);
            rc().with_jack_master(false);
            rc().with_jack_master_cond(false);
            rc().with_jack_midi(false);
            infoprint("Forcing all-ALSA mode");
            break;

        case 'a':
            rc().manual_ports(false);
            break;

        case 'B':                           /* --buss for the oldsters      */
        case 'b':                           /* --bus for the youngsters     */
            usr().midi_buss_override(string_to_midibyte(soptarg));
            break;

#if defined SEQ66_JACK_SUPPORT
        case 'C':
            rc().with_jack_transport(true);
            rc().with_jack_master(false);
            rc().with_jack_master_cond(true);
            break;
#endif

        case 'c':                           /* --config                     */
            rc().set_config_files(soptarg);
            break;

        case 'D':                           /* --legacy-record              */
            rc().filter_by_channel(false);
            break;

        case 'd':                           /* --record-by-channel          */
            rc().filter_by_channel(true);
            break;

        case 'F':                           /* --usr                        */
            rc().user_filename(soptarg);
            break;

        case 'f':                           /* --rc                         */
            rc().config_filename(soptarg);
            break;

        case 'g':
            rc().with_jack_transport(false);
            rc().with_jack_master(false);
            rc().with_jack_master_cond(false);
            break;

        case 'H':
            rc().full_config_directory(soptarg, true);  /* add HOME perhaps */
            file_message("Set home config to", rc().home_config_directory());
            if (! make_directory_path(rc().home_config_directory()))
                errprint("Could not create that directory");
            break;

        case 'h':
            show_help();
            result = c_null_option;
            break;

        case 'i':
            rc().investigate(true);
            break;

#if defined SEQ66_JACK_SUPPORT
        case 'J':
            rc().with_jack_transport(true);
            rc().with_jack_master(true);
            rc().with_jack_master_cond(false);
            break;

        case 'j':                               /* -j and -S are the same   */
            rc().with_jack_transport(true);
            rc().with_jack_master(false);
            rc().with_jack_master_cond(false);
            break;
#endif

        case 'k':
            rc().print_keys(true);
            break;

        case 'K':
            usr().inverse_colors(true);
            break;

        case 'l':
            set_client_name(soptarg);
            break;

        case 'M':

            rc().song_start_mode(std::string(soptarg));
            break;

        case 'm':
            rc().manual_ports(true);
            break;

        case 'N':
            rc().with_jack_midi(false);
            printf("[Deactivating JACK MIDI]\n");
            break;

#if defined SEQ66_NSM_SUPPORT
        case 'n':
            usr().session_manager("nsm");
            printf("[Activating NSM support]\n");
            break;
#endif

        case 'o':

            /*
             * We now handle this processing separately and first, in the
             * parse_o_option() function.  Doing it here can mess up parsing.
             * We need to skip the argument in case there are other arguments
             * or a MIDI filename following the compound option.
             */

            ++optind;
            break;

        case 'P':
            rc().pass_sysex(true);
            break;

        case 'p':
            rc().priority(true);
            break;

        case 'q':
            usr().midi_ppqn(string_to_int(soptarg));
            break;

        case 'R':
            rc().reveal_ports(false);
            printf("[Showing user-configured port names]\n");
            break;

        case 'r':
            rc().reveal_ports(true);
            printf("[Showing native system port names]\n");
            break;

#if defined SEQ66_JACK_SUPPORT
        case 'S':                               /* -j and -S are the same   */
            rc().with_jack_transport(true);
            rc().with_jack_master(false);
            rc().with_jack_master_cond(false);
            break;
#endif

        case 's':
            rc().show_midi(true);
            break;

#if defined SEQ66_NSM_SUPPORT
        case 'T':
            usr().session_manager("none");
            printf("[Deactivating session support]\n");
            break;
#endif

        case 't':
            rc().with_jack_midi(true);
            printf("[Activating native JACK MIDI]\n");
            break;

#if defined SEQ66_JACK_SESSION
        case 'U':
            rc().jack_session(soptarg);
            printf("[JACK session %s]\n", soptarg.c_str());
            break;
#endif

        case 'u':
            rc().auto_usr_save(true);    /* usr(), not rc()!     */
            break;

        case 'v':
            rc().verbose(true);
            break;

        case 'V':
            std::cout << versiontext << seq_build_details();
            result = c_null_option;
            break;

        case 'X':
            rc().playlist_active(rc().playlist_filename_checked(soptarg));
            break;

        /*
         * Undocumented and unsupported in Seq66. Kept around just in case.
         */

        case 'x':
            rc().interaction_method(string_to_int(soptarg));
            break;

        case 'Z':
            rc().manual_ports(true);
            rc().reveal_ports(false);
            rc().auto_usr_save(true);
            printf("[User mode: Manual ports and user-configured port names]\n");
            break;

        case 'z':
            rc().manual_ports(false);
            rc().reveal_ports(true);
            rc().auto_usr_save(true);
            printf("[Native mode: Native ports and port names]\n");
            break;

        case '#':
            std::cout << SEQ66_VERSION << std::endl;
            result = c_null_option;
            break;

        default:
            break;
        }
    }
    if (result != c_null_option)
    {
        std::size_t applen = strlen("seq66");
        std::string appname(argv[0]);           /* "seq66", "./seq66", etc. */
        appname = appname.substr(appname.size()-applen, applen);
        result = optind;
#if defined SEQ66_PLATFORM_DEBUG // _TMI
        if (optind < argc)
        {
            printf
            (
                "NON-OPTION argv ELEMENTS for %d of %d arguments: ",
                optind, argc
            );
            while (optind < argc)
                printf("%s ", argv[optind++]);

            printf("\n");
        }
#endif
    }
    return result;
}

/**
 *  Saves all options to the "rc" and "user" configuration files.  This
 *  function ignores any global variables.
 *
 * \note
 *      If an error occurs, the files "erroneous.rc" and "erroneousl.usr" will
 *      be written as a aid to trouble-shooting.  However, if the normal "rc"
 *      file specified alternate "mutes" and "ctrl" files, those will be
 *      written to their specified names, not "erroneous" names.
 *
 * \param filename
 *      This value, if not empty, provides an altername base name for the
 *      writing of the "rc" and "user" files.  Normally empty, it can be
 *      specified in order to write alternate files without overwriting the
 *      existing ones, when a serious error occurs.  It should not include the
 *      extension; the proper one will be added.
 *
 * \return
 *      Returns true if both files were saved successfully.  Otherwise returns
 *      false.  But even if one write failed, the other might have succeeded.
 */

bool
cmdlineopts::write_options_files (const std::string & filebase)
{
    bool result = write_rc_file(filebase);
    if (result)
        result = write_usr_file(filebase);

    return result;
}

bool
cmdlineopts::write_rc_file (const std::string & filebase)
{
    bool result = true;
    if (rc().auto_rc_save())
    {
        std::string rcn;
        if (filebase.empty())
        {
            rcn = rc().config_filespec();
        }
        else
        {
            std::string name = filebase;
            name += ".rc";
            rcn = rc().config_filespec(name);
        }

        rcfile options(rcn, seq66::rc());
        if (options.write())
        {
            // Anything to do?
        }
        else
            result = false;
    }
    return result;
}

bool
cmdlineopts::write_usr_file (const std::string & filebase)
{
    bool result = true;
    if (rc().auto_usr_save())
    {
        std::string usrn;
        if (filebase.empty())
        {
            usrn = rc().user_filespec();
        }
        else
        {
            std::string name = filebase;
            name += ".usr";
            usrn = rc().user_filespec(name);
        }

        usrfile userstuff(usrn, rc());
        if (userstuff.write())
        {
            // Anything to do?
        }
        else
            result = false;
    }
    return result;
}

}           // namespace seq66

/*
 * cmdlineopts.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


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
 * \updates       2023-10-18
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
#include <locale>                       /* std::locale, etc.                */
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
 *  help-related option was specified.  Also include are values that
 *  getopt_long(3) returns when an issue is encountered.
 */

static const int c_null_option = 99999;
static const int c_missing_arg = ':';
static const int c_bad_option  = '?';

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
    {"inspect",             required_argument, 0, 'I'},
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
    {"jack",                0, 0, '1'},         /* the same as --jack-midi  */
    {"no-jack-connect",     0, 0, 'w'},
    {"jack-connect",        0, 0, 'W'},
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

    /*
     * Never implemented!
     *
     * {"load-recent",         0, 0, 'L'},
     * {"no-load-recent",      0, 0, 'L'},
     */

    {"locale",              required_argument, 0, 'L'},
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
 *  far.  An 'x' means the character is used.  A ':' means it is used and
 *  requires an argument. An 'a' indicates we could repurpose the key with
 *  minimal impact. An asterisk indicates the option is reserved for
 *  application-specific options.  Currently we will use it for options like
 *  "daemonize" in the seq66cli application. Common shell characters, except
 *  for '#', are not include
 *
\verbatim
        0123456789#@AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz
        xx       xx xx::x:xx  :: x:x:xxxxx::xxxx *xx :xxxxxxx:xxxx::  aa
\endverbatim
 *
 *  Note that 'o' options arguments cannot be included here due to issues
 *  involving parse_o_options(), but 'o' is *reserved* here, without the
 *  argument indicator.
 *
 * Manual + User mode '-Z' versus Auto + Native mode '-z':
 *
 *      Creates virtual ports '-m' and hides the native names for the ports
 *      '-R' in favor of the 'usr' definition of the names of ports and
 *      channels.  The opposite (native) setting uses '-a' and '-r'.  Both
 *      modes turn on the --user-save (-u) option.
 *
 * Investigate:
 *
 *      The -i/--investigate option is used on the command line), to turn on
 *      the test-of-the-day and try to unearth difficult-to-find issues.
 */

#if defined SEQ66_JACK_SUPPORT      // how to handle no SEQ66_NSM_SUPPORT?
#define CMD_OPTS \
    "09#AaB:b:Cc:DdF:f:gH:hI:iJjKkL:l:M:mNnoPpq:RrSsTtU:uVvWwX:x:Zz#"
#else
#define CMD_OPTS \
    "0#AaB:b:c:DdF:f:H:hI:iKkL:l:M:mnoPpq:RrsTuVvX:x:Zz#"
#endif

const std::string cmdlineopts::s_arg_list = CMD_OPTS;

/**
 *  Provides help text.
 */

static const std::string s_help_1a =
"Options:\n"
"   -h, --help, ?           Show this help and exit.\n"
"   -V, --version, #        Show program version/build and exit.\n"
"   -v, --verbose           Show more data to the console.\n"
#if defined SEQ66_NSM_SUPPORT
"   -n, --nsm               Activate debugging NSM support.\n"
"   -T, --no-nsm            Ignore NSM in 'usr' file. (Typical).\n"
#endif
"   -X, --playlist filename Load playlists (from \"home\" directory).\n"
#if ! defined SEQ66_PORTMIDI_SUPPORT
"   -m, --manual-ports      Create virtual ports (ALSA/JACK).\n"
#endif
"   -a, --auto-ports        Auto-Connect MIDI ports.\n"
;

/**
 *  More help text.
 */

static const std::string s_help_1b =
"   -r, --reveal-ports      Don't show 'usr' definitions for port names.\n"
"   -R, --hide-ports        Show 'usr' definitions for port names.\n"
#if ! defined SEQ66_PLATFORM_WINDOWS
"   -A, --alsa              Use ALSA, not JACK. A sticky option.\n"
#endif
"   -b, --bus b             Global override of bus number (for testing).\n"
"   -B, --buss b            Covers the bus/buss confusion.\n"
"   -l, --client-name label Use label instead of 'seq66'. Overridden by a\n"
"                           session manager.\n"
"   -q, --ppqn qn           Specify default PPQN to replace 192. The MIDI file\n"
"                           can specify its own PPQN.\n"
"   -p, --priority          Run high priority, FIFO scheduler (needs root).\n"
"   -P, --pass-sysex        Passes incoming SysEx messages to all outputs.\n"
"                           Not yet fully implemented.\n"
"   -s, --show-midi         Dump incoming MIDI events to the console.\n"
;

/*
 * This option was never used, just settable, in Seq24.  We need that letter!
 *
 *      "   -i, --ignore n Ignore ALSA device number."
 */

/**
 *  Still more help text.
 */

static const std::string s_help_2 =
"   -k, --show-keys         Prints pressed key value.\n"
"   -K, --inverse           Inverse/night color scheme for seq/perf editors.\n"
"   -M, --jack-start-mode m ALSA or JACK play modes: live; song; auto.\n"
#if defined SEQ66_JACK_SUPPORT
"   -S, --jack-slave        Synchronize to JACK transport as Slave.\n"
"   -j, --jack-transport    Same as --jack-slave.\n"
"   -g, --no-jack-transport Turn off JACK transport.\n"
"   -J, --jack-master       Set up as JACK Master. Also sets -j.\n"
"   -C, --jack-master-cond  Fail if there's already a JACK Master; sets -j.\n"
"   -N, --no-jack-midi      Use ALSA MIDI, even with JACK Transport. See -A.\n"
"   -t, --jack, --jack-midi Use JACK MIDI, separately from JACK Transport.\n"
"   -W, --jack-connect      Auto-connect to JACK ports. The default.\n"
"   -w, --no-jack-connect   Don't connect to JACK ports. Good with NSM.\n"
#if defined SEQ66_JACK_SESSION
"   -U, --jack-session uuid Set UUID for JACK session management. Use 'on' to\n"
"                           enable it and let JACK set the UUID.\n"
#endif
#endif
"   -d, --record-by-channel Divert MIDI input by channel into the patterns\n"
"                           numbered for each channel.\n"
"   -D, --legacy-record     Record all MIDI into the active pattern. Default.\n"
;

/**
 *  Still still more help text.
 */

static const std::string s_help_3 =
"   -0, --smf-0              Don't convert SMF 0 files to SMF 1 upon reading.\n"
"   -u, --user-save          Force the save  of 'usr' settings.\n"
"   -H, --home dir           Directory for configuration. $HOME/.config/seq66\n"
"                            by default. If not a full path, it is appended.\n"
"   -f, --rc filename        An alternate 'rc' file in $HOME/.config/seq66 or\n"
"                            the --home directory. '.rc' extension enforced.\n"
"   -F, --usr filename       An alternate 'usr' file.  Same rules as for --rc.\n"
"   -c, --config basename    Change base name of the 'rc' and 'usr' files. The\n"
"                            extension is stripped. ['qseq66' is default].\n"
"   -L, --locale lname       Set global locale, if installed on the system.\n"
"   -i, --investigate        Turn on various trouble-shooting code.\n"
"   -o, --option optoken     Provides app-specific options for expansion.\n"
"                            Options supported are:\n\n"
    ;

/**
 *  Still still more more help text.
 */

static const std::string s_help_4a =
"      log=filename  Redirect console output to a log file in home. If no\n"
"                    '=filename' is provided, the filename in '[user-options]'\n"
"                    in the 'usr' file is used.\n"
"      sets=RxC      Change set rows and columns from 4x8. R can be 4 to 12;\n"
"                    C can be 4 to to 12. Call it the 'variset' mode. Affects\n"
"                    mute groups, too.\n"
;

static const std::string s_help_4b =
"      scale=x.y     Scales size of main window. Range: 0.5 to 3.0.\n"
"      mutes=value   Saving of mute-groups: 'mutes', 'midi', or 'both'.\n"
"      virtual=o,i   Like --manual-ports, except that the count of output and\n"
"                    input ports are specified. Defaults are 8 & 4.\n"
"\n"
" seq66cli:\n"
"      daemonize     Sets this application up to fork to the background.\n"
"      no-daemonize  Or not. These options do not apply to Windows. If given,\n"
"                    the application writes these options to the 'usr' file\n"
"                    and exits. Subsequent runs are thus affected. Tricky!\n"
"\n"
"Add '--user-save' to make these options permanent.\n"
"\n"
;

/**
 *  Still still still more more more help text.
 */

static const std::string s_help_5 =
"Saving a MIDI file saves the current PPQN value. No JACK options are shown if\n"
"disabled in the build configuration. Command-line options can be sticky; many\n"
"are saved to the 'rc' files when Seq66 exits. See the Seq66 User Manual.\n"
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
            break;
        }
        else if (arg == "?")
        {
            result = true;
            break;
        }
    }
    return result;
}

/**
 *  Like help_check(), but accepts only 1 argument.  Anything else
 *  is ignored.
 *
 * \return
 *      Returns true only if a single argument, "--kill", was found.
 */

bool
cmdlineopts::kill_check (int argc, char * argv [])
{
    bool result = argc == 2;
    if (result)
    {
        std::string arg = argv[1];
        result = arg == "--kill" || "kill";
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
                                usr().option_daemonize(true, true);  // setup!
                            }
                            else if (arg == "no-daemonize")
                            {
                                result = true;
                                usr().option_daemonize(false, true); // setup!
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
                            warn_message("--option", "unsupported name");
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
        file_message("Running debug/investigate version", argv[0]);
#else
        file_message("Running", argv[0]);
#endif
        rc().verbose(true);
        rc().investigate(true);
        file_message(exename, "Verbose/investigate mode enabled");
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
 *      files upon exit.  In other words, missing configuration files is not
 *      an error.
 */

bool
cmdlineopts::parse_options_files (std::string & errmessage)
{
    std::string rcn = rc().config_filespec();
    bool result = parse_rc_file(rcn, errmessage);
    if (result)
    {
        rcn = rc().user_filespec();
        result = parse_usr_file(rcn, errmessage);
    }
    return result;
}

bool
cmdlineopts::parse_rc_file
(
    const std::string & filespec,
    std::string & errmessage
)
{
    bool result = true;                         /* file_readable(filespec)  */
    if (file_readable(filespec))
    {
        rcfile options(filespec, rc());
        file_message("Reading rc", filespec);
        result = options.parse();
        if (! result)
        {
            errmessage = options.get_error_message();
            file_error("rc", errmessage);
        }
    }
    else
    {
        file_message("Cannot read", filespec);
        rc().auto_rc_save(true);
        rc().create_config_names();
    }
    return result;
}

/**
 *  Get only the 'usr' file and its active flags from the 'rc' file. This
 *  function supports testing to see if the application should be
 *  daemonized.  See cmdlineopts::parse_daemonization() and
 *  userfile::parse_daemonization().
 */

bool
cmdlineopts::get_usr_file ()
{
    std::string rcn = rc().config_filespec();
    bool result = file_readable(rcn);
    if (result)
    {
        rcfile options(rcn, rc());
        file_message("Reading rc to get 'usr' file", rcn);
        result = options.get_usr_file();
        if (! result)
            file_error("Getting 'usr' file failed", rcn);
    }
    else
    {
        file_message("Cannot read", rcn);
        rc().auto_rc_save(true);
    }
    return result;
}

bool
cmdlineopts::parse_usr_file
(
    const std::string & filespec,
    std::string & errmessage
)
{
    bool result = true;
    if (file_readable(filespec))
    {
        usrfile ufile(filespec, rc());
        file_message("Reading usr", filespec);
        result = ufile.parse();
        if (! result)
        {
            errmessage = ufile.get_error_message();
            file_error("usr", errmessage);
        }
    }
    else
    {
        file_message("Cannot read", filespec);
        rc().auto_rc_save(true);
        rc().auto_usr_save(true);
    }

    return result;
}

/**
 *  This function figures out if the application is to be daemonized. It needs
 *  to do the following:
 *
 *      -#  Read the 'usr' file "[usr-file]" section:
 *          -   "active" flag
 *          -   "name" of the 'usr' file.
 *      -#  Make sure the 'usr' file is active and readable.
 *      -#  Parse the daemonization and logging values from the 'usr' file.
 */

bool
cmdlineopts::parse_daemonization (bool & startdaemon, std::string & logfile)
{
    bool result = cmdlineopts::get_usr_file();   /* daemon values    */
    if (result)
    {
        std::string usrn = rc().user_filespec();
        result = file_readable(usrn);
        if (result)
        {
            usrfile ufile(usrn, rc());
            result = ufile.parse_daemonization(startdaemon, logfile);
        }
        else
        {
            result = false;
            startdaemon = false;
            logfile = "";
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

                usr().window_scale(scale, scaley, true);
            }
#endif
        }
        else
            result = false;
    }
    return result;
}

/**
 *  The performer object will grab this setting and pass it to the mutegroups
 *  object that it owns.  See performer::open_mutegroup().
 */

bool
cmdlineopts::parse_o_mutes (const std::string & arg)
{
    bool result = arg == "mutes" || arg == "midi" || arg == "both";
    if (result)
    {
        mutegroups::saving v = mutegroups::string_to_group_save(arg);
        if (v != mutegroups::saving::max)
            rc().mute_group_save(v);
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
 *  not an option.  This is used in smanager::main_settings() to detect that
 *  a MIDI file has been specified on the command-line.
 *
 * \param argc
 *      The number of command-line arguments.
 *
 * \param argv
 *      The array of command-line argument pointers.
 *
 * \return
 *      Returns the value of optind if no help-related options were provided.
 *      Returns (-1) if an error occurred.
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
            s_arg_list.c_str(),             /* e.g. "Crb:q:Li:jM:pU:Vx:..." */
            s_long_options, &option_index
        );
        if (c == c_missing_arg)             /* missing argument indicator   */
        {
            errprint("Option missing an argument");
            return (-1);
        }
        else if (c == c_bad_option)
        {
            errprint("Non-existent option");
            return (-1);
        }
        else if (c == (-1))
            break;

        std::string soptarg;
        if (not_nullptr(optarg))
            soptarg = std::string(optarg);

        switch (c)
        {
        case '0':
            usr().convert_to_smf_1(false);
            break;

#if defined SEQ66_JACK_SUPPORT
        case '1':                   /* added for consistency with --alsa    */
            rc().with_jack_midi(true);
            break;
#endif

        case '#':
            std::cout << SEQ66_VERSION << std::endl;
            result = c_null_option;
            break;

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

            rc().set_config_directory(soptarg);
            break;

        case 'h':
            show_help();
            result = c_null_option;
            break;

        case 'I':
            rc().inspection_tag(soptarg);   /* see smanager and sessionfile */
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

        case 'K':
            usr().inverse_colors(true);
            break;

        case 'k':
            rc().print_keys(true);
            break;

        case 'L':
            (void) set_global_locale(soptarg);
            break;

        case 'l':
            set_client_name(soptarg);
            break;

        case 'M':

            rc().song_start_mode_by_string(std::string(soptarg));
            break;

        case 'm':
            rc().manual_ports(true);
            break;

        case 'N':
            rc().with_jack_midi(false);
            break;

#if defined SEQ66_NSM_SUPPORT
        case 'n':
            usr().session_manager("nsm");       /* mostly for debugging     */
            usr().in_nsm_session();             /* definitely debugging     */
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
            break;

        case 'r':
            rc().reveal_ports(true);
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
            break;
#endif

#if defined SEQ66_JACK_SUPPORT
        case 't':
            rc().with_jack_midi(true);
            break;
#endif

#if defined SEQ66_JACK_SESSION
        case 'U':
            rc().jack_session(soptarg);
            break;
#endif

        case 'u':
            rc().auto_usr_save(true);    /* usr(), not rc()!     */
            break;

        case 'V':
            std::cout << versiontext << seq_build_details();
            result = c_null_option;
            break;

        case 'v':
            rc().verbose(true);
            break;

#if defined SEQ66_JACK_SUPPORT

        case 'W':
            rc().jack_auto_connect(true);
            break;

        case 'w':
            rc().jack_auto_connect(false);
            break;

#endif

        case 'X':
            rc().playlist_active(rc().playlist_filename_checked(soptarg));
            break;

        /*
         * Undocumented and unsupported in Seq66. Kept around just in case.
         */

        case 'x':
            rc().interaction_method(string_to_int(soptarg));
            break;

        case 'Z':                                   /* User mode    */
            rc().manual_ports(true);
            rc().reveal_ports(false);
            rc().auto_usr_save(true);
            break;

        case 'z':                                   /* Native mode  */
            rc().manual_ports(false);
            rc().reveal_ports(true);
            rc().auto_usr_save(true);
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
#if defined SEQ66_PLATFORM_DEBUG_TMI
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
 *  Locale-related functions.
 *
 *  The MingW compiler implementation may have a bug, as this fails in our
 *  Windows 10 virtual machine development system.
 */

void
cmdlineopts::show_locale ()
{
    try
    {
        std::locale loc {""};           /* get the user's preferred locale  */
        std::string msg = loc.name();
        status_message("Locale", msg);
    }
    catch (const std::runtime_error &)
    {
        /*
         * Can occur under Windows 10.
         */
    }
}

bool
cmdlineopts::set_global_locale (const std::string & lname)
{
    bool result = ! lname.empty();
    if (result)
    {
        try
        {
            std::locale oldlocale = std::locale::global(std::locale{lname});
            std::locale newlocale;      /* default ctor yields global loc.  */
            std::string oldname = oldlocale.name();
            std::string newname = newlocale.name();
            std::string msg = oldname + " ---> " + newname;
            status_message("Locale", msg);
        }
        catch (std::runtime_error & re)
        {
            std::string tag = "Locale '" + lname + "'";
            error_message(tag, re.what());
            result = false;
        }
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
 * \param filebase
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
            std::string name = file_extension_set(filebase, ".rc");
            rcn = rc().config_filespec(name);
        }

        rcfile options(rcn, rc());
        result = options.write();
        if (! result)
            file_error("Write failed", rcn);
    }
    return result;
}

bool
cmdlineopts::alt_write_rc_file (const std::string & filebase)
{
    bool result = true;
    std::string name = file_extension_set(filebase, ".rc");
    std::string rcn = rc().config_filespec(name);
    rcfile options(rcn, rc());
    result = options.write();
    if (! result)
        file_error("Write failed", rcn);

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
            std::string name = file_extension_set(filebase, ".usr");
            usrn = rc().user_filespec(name);
        }

        usrfile userstuff(usrn, rc());
        result = userstuff.write();
        if (! result)
            file_error("Write failed", usrn);
    }
    return result;
}

bool
cmdlineopts::alt_write_usr_file (const std::string & filebase)
{
    bool result = true;
    std::string name = file_extension_set(filebase, ".usr");
    std::string usrn = rc().user_filespec(name);
    usrfile userstuff(usrn, rc());
    result = userstuff.write();
    if (! result)
        file_error("Write failed", usrn);

    return result;
}

}           // namespace seq66

/*
 * cmdlineopts.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */


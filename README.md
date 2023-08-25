# README for Seq66 0.99.8 2023-08-25

__Seq66__: MIDI sequencer/live-looper with a hardware-sampler grid interface;
pattern banks, triggers, and playlists for song management; scale and chord
aware piano-roll; song layout for creative composition; and control/status
via MIDI automation for live performance.  Mute-groups enable/disable multiple
patterns at once.  Supports the Non/New Session Manager; can also run headless.
Works in a space as small as 725x500 pixels (less if window decoration removed).
It does not support audio samples, just MIDI.

__Seq66__ is a major refactoring of Sequencer64/Kepler34/Seq24 with modern C++
and new features.  Linux and Windows users can build this application from
source code.  See the extensive INSTALL file.  Includes a comprehensive PDF
user-manual.

Support sites (still in progress):

    *   https://ahlstromcj.github.io/
    *   https://github.com/ahlstromcj/ahlstromcj.github.io/wiki

The release now includes an installer for the 64-bit Windows version of Seq66.

![Alt text](doc/latex/images/main-window/main-windows.png?raw=true "Seq66")

# Major Features

##  User interface

    *   Qt 5 (cross-platform).  Loop-button gird. Qt style-sheet support.
    *   Tabs and external windows for patterns, sets, mute-groups, song
        layout, event-editing, play-lists, and session information.
    *   Low-frequency oscillator (LFO) to modify continuous controller
        and velocity values.
    *   A "fixer" for expansion/compression/alignment of note patterns.
    *   Colorable pattern slots; the color palette can be saved and modified.
    *   Horizontal and vertical zoom in the pattern and song editors.
    *   A headless/daemon version can be built.

##  Configuration files

    *   Supports configuration files: '.rc', '.usr', '.ctrl', '.mutes',
        '.playlist', '.drums' (note-mapping), '.palette', and Qt '.qss'.
    *   Separates MIDI control and mute-group setting into their own files.
    *   Unified keystroke and MIDI controls in the '.ctrl' file; defines MIDI
        controls for automation/display of Seq66 status in grid controllers
        (e.g. LaunchPad).  Sample '.ctrl' files provided for Launchpad Mini.

##  Non/New Session Manager

    *   Support for NSM/New Session Manager, RaySession, Agordejo....
    *   Handles starting, stopping, hiding, and session saving.
    *   Displays details about the session.

##  Multiple Builds

    *   ALSA/JACK: `qseq66` using an rtmidi-based library
    *   Command-line/headless: `seq66cli`
    *   PortMidi: `qpseq66`
    *   Windows: `qpseq66.exe`

##  More Features

    *   Transposable triggers to re-use patterns more comprehensively.
    *   Song import/export from/to stock MIDI (SMF 0 or 1).
    *   Improved non-U.S. keyboard support.
    *   Many demonstration and test MIDI files.
    *   See **Recent Changes** below, and the **NEWS** file.

##  Internal

    *   More consistent use of modern C++, auto, and lambda functions.
    *   Additional performer callbacks to reduce polling.
    *   A ton of clean-up and refactoring.

Seq66 uses a Qt 5 user-interface based on Kepler34 and the Seq66 *rtmidi*
(Linux) and *portmidi* (Windows) engines.  MIDI devices are detected,
inaccessible devices are ignored, with playback (e.g. to the Windows wavetable
synth). It is built easily via *GNU Autotools*, *Qt Creator* or *qmake*, using
*MingW*.  See the INSTALL file for build-from-source instructions for Linux or
Windows, and using a conventional source tarball.

## Recent Changes

    *   Version 0.99.8:
        *   Issue #112: A new pattern now displays in the MIDI controller.
        *   Issue #114: Adding display of shortcut keys to tool tips.
        *   Added a Pattern tab to Edit / Preferences for new-pattern settings
            and jitter/randomization.
        *   Added automation for the main window Loop L/R button.
        *   Fixed seqroll drawing errors introduced in adding time-sig support.
        *   Fixed incomplete data-pane refresh in scrolling with arrow-keys.
        *   Fixed not setting up SIGINT, which prevented a proper shutdown.
        *   Fixed a couple corrupted data/midi/FM/*.mid files.
        *   Changing playlist setting enables Session Restart button.
        *   Removed coloring of record-style and -mode buttons. Added coloring
            of event-editor "Store" button to denote saving is needed.
        *   Refactoring quantization alterations for future upgrades. Added
            an option to jitter the notes in the seqroll.
        *   Enforced that configuration files are stored in the "home"
            directory.
        *   The usual raft of humiliating bug-fixes. A small sample: Updating
            the event list when recording stops; fixing record button in pattern
            editor; fixing note-selection refresh; and about a dozen more.
    *   Version 0.99.7:
        *   Issue #110 follow-ons:
            Cannot save tempo (BPM) in Windows when
            changed from main window. Caused by mixing a long and size_t,
            which messed up in the Windows build.
        *   Issue #111 follow-ons:
            *   Fixed initial time-signature drawing in data pane.
            *   Fixed errors in inserting a time-signature. Added a pulse
                calculator that iterates through time-signatures.
        *   Fixed an important port-translation bug in output port-mapping.
        *   Recent Files is disabled if there are none.
        *   Revamped the Playlist tab, as it was confusing and very buggy.
        *   Added Windows key-mapping to fix processing "native virtual" keys,
            such as the arrow keys. Also fixes issue #102.
        *   Added auto-play and auto-advance to play-lists.
        *   Fixed bug in rcsettings::make_config_filespec().
    *   Version 0.99.6:
        *   Issue #3 follow-ons:
            *   Added a qscrollslave to allow QScrollArea to allow the pattern
                editor panes to remain in sync with the seqroll when using the
                hjkl, arrow, and page keys.
        *   Issue #110 follow-ons:
            *   Addition of Start menu entries for Windows.
            *   Fixed access to the tutorial and manual. Refactored access
                to manual and tutorial for robustness.
            *   Added data/readme files and doc/tutorial files accidentally
                left out of NSIS installer.
            *   Fixed the saving of modified tempo changes.
            *   Fixed event::is_desired(), which affected changing note
                velocities in the pattern editor's data pane.  Improved
                velocity-change undo.
            *   Fixed an error preventing changing the "background" pattern.
            *   Fixed issues with port-mapping and the Windows MIDI Mapper.
            *   Issue: Building 32-bit (Windows XP) version on 64-bit Windows.
                On 64-bit Windows, this seems to require building a 32-bit
                version of the Qt toolset. Ugh.
        *   Issue #111: Adding support, as much as possible, for editing,
            storing, and displaying time signature in the pattern and event
            editors.
            *   The first time-signature in a pattern becomes the main
                time signature of the pattern. (Also stored as a c_timesig
                SeqSpec).
            *   The data pane shows a time-signature as a simple fraction.
            *   Changing the time signature if at time 0 is automatic.
            *   Time signatures at later times can be logged by setting the
                current time with a click in the top half of the time line,
                changing the beats and beat width, then clicking a time-sig log
                button.
            *   Time-signatures with a beat width that are not a power of 2
                do not add an event, but are saved as the c_timesig SeqSpec
                value.
            *   Provisional feature: we properly draw the piano roll, time line,
                event pane vertical lines as time-signature changes.
                Currently compiled in via a macro (see INSTALL).
            *   Fixed event filtering in the event (qstriggereditor) pane.
            *   Fixed time-signature editing in the event editor, and the
                B:B:T to pulses calculation.
        *   Enhanced port-mapping to prompt the user about issues and
            allow for an immediate remap-and-restart. A ton of fixes!
        *   Added 'o' keystroke to seqroll to toggle recording ('r' already
            used to randomize notes).
        *   Added a "quiet" option to not show startup message prompts.
        *   Enhanced the Edit / Preferences dialog.
        *   At first start, a log-file is now automatically created. If it
            gets larger than a megabyte, then it is deleted to start over.
        *   Fixed bug writing pattern-dump files from event editor.
        *   Improved modification detection and display in the pattern editor.
        *   Added the pattern port number to the song summary output.
        *   Updated alsa.m4 to avoid obsolete AC_TRY_COMPILE warning. Old
            version stored in contrib/scripts.
    *   Version 0.99.5:
        *   Greatly enhanced the event editor tab and the events that can
            be view and modified.
        *   Made port-mapping the default. At first startup the map
            exactly matches the existing ports; the user can edit this setup
            in the 'rc' file or the Preferences dialog.
        *   Eliminated "missing ctrl" message at first startup.
        *   Fixed port ID setting in midibus, and adding output flag in
            midi_alsa_info.
        *   Issue #110 Windows: Fixed compiler errors and added scripting to
            build NSIS-based install without leaving Windows, if desired.
        *   Internal refactoring to regularize handling of the session/config
            directory between Linux and Windows.
        *   Fixed portmidi bugs in Linux and Windows, enhanced device naming.
        *   Showing disabled/unavailable MIDI devices as grayed in various
            dropdowns.
        *   Rearranged the Seq66 man pages more sensibly.

    See the "NEWS" file for changes in earlier versions.  Some proposed features
    will be pushed off to Seq66v2; see the bottom of the TODO file. Version
    2 is probably a year or two away :-( So many things to improve!

// vim: sw=4 ts=4 wm=2 et ft=markdown
